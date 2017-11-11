
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include <stdio.h>
#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"
#include "bmp.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <pthread.h>  
#include <sched.h>  
#include <unistd.h> 
#include <sys/times.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <alsa/asoundlib.h>


#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__)

/*audio */
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 
#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)

#define AV_SYNC_THRESHOLD 0.01  
#define AV_NOSYNC_THRESHOLD 10.0  
#define	VIDEO_DEVICE "/dev/video1" 

#define V4L2_OSD_WIDTH  800
#define V4L2_OSD_HEIGHT 480
#define CONVERT_WITH 800
#define CONVERT_HIGH 480

uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

typedef struct
{    
	unsigned int *pui_addr;    
	unsigned int ui_addrlen;    
	unsigned int ui_offset;
}V4L2_MAMP_ADDR_INFO_S;//保存映射后的内存信息。

typedef struct PacketQueue  
{  
    AVPacketList *first_pkt, *last_pkt;  
    int nb_packets;  //包的个数
    int size;  //队列中所有包中数据的大小
    SDL_mutex *mutex;  
    SDL_cond *cond;  
} PacketQueue;  



typedef struct MediaState  
{    
    int             quit;  
/*common*/
	AVFormatContext *pFormatCtx; 
	int             videoStream, audioStream;
	char            filename[1024];
/*video*/
	pthread_t		video_pid;
	AVCodecContext	*v4lCodecCtx;    
	AVCodec 		*v4lCodec;
	AVStream		*video_st;
	double			frame_timer;
	double          frame_last_delay;
	PacketQueue     videoq;
	double          video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame
	AVFrame			*frame_rgb;//convert use
	uint8_t			*video_out_buffer;
	struct SwsContext *img_convert_ctx;
	/*v4l2*/
	V4L2_MAMP_ADDR_INFO_S *addinfo;
	unsigned int 	addcnt;
	int 			video_fd;
	double          frame_last_pts;
/*audio*/
	pthread_t		audio_pid;
	AVStream        *audio_st;
    AVCodecContext	*sndCodecCtx;    
	AVCodec			*sndCodec;    
	struct SwrContext 		*swr_ctx;    
	snd_pcm_t 		*pcm;
	PacketQueue		audioq;
	DECLARE_ALIGNED(16,uint8_t,audio_buf) [AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
} MediaState; 


int our_get_buffer(struct AVCodecContext *c, AVFrame *pic);
void our_release_buffer(struct AVCodecContext *c, AVFrame *pic);
double synchronize_video(MediaState *media_handle, AVFrame *src_frame, double pts);
int __v4l2_umamp(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt);
int __v4l2_mmap_fill_color(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt,char *pc_tmpbuff,int tmpbufflen);
int __v4l2_alpha_config(int video_fd);
int __v4l2_mmap(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int *buffcnt);

void packet_queue_init(PacketQueue *q)  
{  
    memset(q, 0, sizeof(PacketQueue));  
    q->mutex = SDL_CreateMutex();  
    q->cond = SDL_CreateCond();  
}  

int packet_queue_put(PacketQueue *q, AVPacket *pkt)  
{  
    AVPacketList *pkt1;  
    if(av_dup_packet(pkt) < 0)  
    {  
        return -1;  
    }  
    pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));  
    if (!pkt1)  
        return -1;  
    pkt1->pkt = *pkt;  
    pkt1->next = NULL;  
  
    SDL_LockMutex(q->mutex);  
  
    if (!q->last_pkt)  
        q->first_pkt = pkt1;  
    else  
        q->last_pkt->next = pkt1;  
    q->last_pkt = pkt1;  
    q->nb_packets++;  
    q->size += pkt1->pkt.size;  
    SDL_CondSignal(q->cond);  
  
    SDL_UnlockMutex(q->mutex);  
    return 0;  
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)  
{  
    AVPacketList *pkt1;  
    int ret;  
  
    SDL_LockMutex(q->mutex);  
  
    for(;;)  
    {  
  
        pkt1 = q->first_pkt;  
        if (pkt1)  
        {  
            q->first_pkt = pkt1->next;  
            if (!q->first_pkt)  
                q->last_pkt = NULL;  
            q->nb_packets--;  
            q->size -= pkt1->pkt.size;  
            *pkt = pkt1->pkt;  
            av_free(pkt1);  
            ret = 1;  
            break;  
        }  
        else if (!block)  
        {  
            ret = 0;  
            break;  
        }  
        else  
        {  
            SDL_CondWait(q->cond, q->mutex);  
        }  
    }  
    SDL_UnlockMutex(q->mutex);  
    return ret;  
}

/*******************************************
*				视频播放线程		       *
********************************************/

int video_play_thread(void *arg)
{
	MediaState *media_handle = (MediaState *)arg;
	AVPacket pkt1, *packet = &pkt1;  
    int len1, frameFinished;  
    AVFrame *pFrame;  
    double pts;
	unsigned int frame_num;
	int ret;
	double actual_delay, delay, sync_threshold, ref_clock, diff; 

	//pFrame = avcodec_alloc_frame();
	pFrame = av_frame_alloc();

	V4L2_MAMP_ADDR_INFO_S addinfo[6];    
        
    unsigned int addcnt = 1;    
    memset(addinfo, 0, sizeof(addinfo));    
    if (__v4l2_alpha_config(media_handle->video_fd) != 0)     
        {   LOGD("__v4l2_alpha_config error!");     
            //close(video_fd);  //      
            //return -1;    
        }    
        if (__v4l2_mmap(media_handle->video_fd, addinfo, &addcnt) != 0)   
        {        
            LOGD("__v4l2_mmap error!");        
            close(media_handle->video_fd);        
            return -1;    
        }  

	for(;;)
	{
		if(packet_queue_get(&media_handle->videoq, packet, 1) < 0)  
        {  
            // means we quit getting packets  
            break;  
        }  
		pts = 0;

		// Save global pts to be stored in pFrame in first call  
        global_video_pkt_pts = packet->pts;  

		ret = avcodec_decode_video2(media_handle->v4lCodecCtx, pFrame, &frameFinished, packet);
		if(ret < 0)
		{  				
			LOGD("Decode Error.");				
			return -1;			
		}

		if(packet->dts == AV_NOPTS_VALUE  
                && pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE)  
        {  
            pts = *(uint64_t *)pFrame->opaque;  
        }  
		else if(packet->dts != AV_NOPTS_VALUE)  
        {  
            pts = packet->dts;  
        }  
		else  
        {  
            pts = 0;  
        }
		pts *= av_q2d(media_handle->video_st->time_base);
		// Did we get a video frame? 
		if(frameFinished) 
		{
			pts = synchronize_video(media_handle, pFrame, pts);
			sws_scale(media_handle->img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, media_handle->v4lCodecCtx->height, 
				media_handle->frame_rgb->data, media_handle->frame_rgb->linesize);

			frame_num++; 

			__v4l2_mmap_fill_color(media_handle->video_fd, addinfo, addcnt,media_handle->frame_rgb->data[0],CONVERT_WITH*CONVERT_HIGH*4);
			if(ret < 0)				
			{					
				LOGD("__v4l2_mmap_fill_color error!");					
				ioctl(media_handle->video_fd, VIDIOC_STREAMOFF,V4L2_BUF_TYPE_VIDEO_OUTPUT);					
				close(media_handle->video_fd);					
				return -1;				
			}
			/*fresh the timer*/
			delay = pts - media_handle->frame_last_pts;
			if(delay <= 0 || delay >= 1.0)
			{
				/* if incorrect delay, use previous one */   
				delay = media_handle->frame_last_delay;
			}
			/* save for next time */  
  			media_handle->frame_last_delay = delay;
			media_handle->frame_last_pts = pts;

			/* update delay to sync to audio */  
 //           ref_clock = get_audio_clock(is);  *****ref the audio clock
 			ref_clock = 0;
            diff = pts - ref_clock;  

			/* Skip or repeat the frame. Take delay into account 
            FFPlay still doesn't "know if this is the best guess." */  
            sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;  
			if(fabs(diff) < AV_NOSYNC_THRESHOLD)  
            {  
                if(diff <= -sync_threshold)  
                {  
                    delay = 0;  
                }  
                else if(diff >= sync_threshold)  
                {  
                    delay = 2 * delay;  
                }  
            } 
			
			media_handle->frame_timer += delay;

			 /* computer the REAL delay */  
            actual_delay = media_handle->frame_timer - (av_gettime() / 1000000.0);
			if(actual_delay < 0.010)  
            {  
                /* Really it should skip the picture instead */  
                actual_delay = 0.010;  
            }  							
			
			/*end*/
			mySleep(0,(int)(actual_delay * 1000 + 0.5));

			if(frame_num%100 == 0)                      
				LOGD("%d frames done!",frame_num);
			
		}
		av_free_packet(packet);
		
	}
}


/*******************************************
*				初始化视频设备		       *
********************************************/

int video_outdev_init(MediaState *media_handle)
{
	uint8_t *out_buffer;
	AVFrame *pFrameRGB;
	struct SwsContext *img_convert_ctx;  //图像格式转化上下文 

	pFrameRGB = av_frame_alloc();

	out_buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_RGB32, CONVERT_WITH, 
		CONVERT_HIGH));

	avpicture_fill((AVPicture *)pFrameRGB, out_buffer, PIX_FMT_RGB32, CONVERT_WITH, 
		CONVERT_HIGH);

	/*  初始化SWS，图片格式装换上下文//经过验证，输出YUV不需要格式转换，但需要调整尺寸  */
	img_convert_ctx = sws_getContext(media_handle->v4lCodecCtx->width, media_handle->v4lCodecCtx->height, 
		media_handle->v4lCodecCtx->pix_fmt,CONVERT_WITH, CONVERT_HIGH, PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

		/*	v4l2 ops	*/	
		int video_fd = -1;    
		video_fd = open(VIDEO_DEVICE, O_RDWR);// 打开设备    
		if (video_fd < 0)    
		{        
			LOGD("open the video device error!");		
			return -1;    
		}    
		LOGD("open the video device successful!");		
		/* end v4l2 ops*/

		media_handle->video_out_buffer = out_buffer;
		media_handle->frame_rgb = pFrameRGB;
		media_handle->img_convert_ctx = img_convert_ctx;
		media_handle->video_fd = video_fd;
		

		return 0;
	
}


/*******************************************
*				初始化音视频设备		   *
********************************************/

int audio_video_open(MediaState *media_handle, int stream_index)
{
	AVFormatContext *pFormatCtx = media_handle->pFormatCtx;
	AVCodecContext *codecCtx;  
    AVCodec *codec;  
	pthread_t video_id;
	pthread_t audio_id;
	int ret;

	if(stream_index < 0 || stream_index >= pFormatCtx->nb_streams) //not find the stream  
    {  
        return -1;  
    }

	 // Get a pointer to the codec context for the video stream  
    codecCtx = pFormatCtx->streams[stream_index]->codec;  
	codec = avcodec_find_decoder(codecCtx->codec_id); 

	if(!codec || avcodec_open2(codecCtx, codec, NULL) < 0)
	{
		LOGD("unsupport the codec!");
		return -1;
	}

	switch(codecCtx->codec_type) 
	{
		case AVMEDIA_TYPE_AUDIO: 
			/* audio ops*/
			break;
		case AVMEDIA_TYPE_VIDEO:
			media_handle->videoStream = stream_index;
			media_handle->video_st = pFormatCtx->streams[stream_index];//保存视频流
			media_handle->frame_timer = (double)av_gettime() / 1000000.0;
			media_handle->frame_last_delay = 40e-3;
			media_handle->v4lCodecCtx = codecCtx;
			media_handle->v4lCodec = codec;
			packet_queue_init(&media_handle->videoq);  //初始化视频流队列 
			video_outdev_init(media_handle);
			ret = pthread_create(&video_id,NULL,(void *)video_play_thread,media_handle);
			if(ret != 0)
			{
				LOGD("Create video_play_thread pthread error");
				return -1;
			}
			codecCtx->get_buffer = our_get_buffer;  
        	codecCtx->release_buffer = our_release_buffer;
			break;
		default:
			break;
	}

	media_handle->video_pid = video_id;

	return 0;
	
	 		
}


/*******************************************
*				影视频解码线程			   *
********************************************/

int audio_video_decode_thread(void *arg)
{
	MediaState *media_handle = (MediaState *)arg;
	AVFormatContext *pFormatCtx; 
	AVPacket pkt1, *packet = &pkt1;

	int video_index = -1;
	int audio_index = -1;  
    int i;  

	media_handle->audioStream = -1;
	media_handle->videoStream = -1;

	/*open the video file*/
	pFormatCtx = avformat_alloc_context();     //格式上下文结构体指针开空间
	if(avformat_open_input(&pFormatCtx,media_handle->filename,NULL,NULL)!=0){
		LOGD("Couldn't open input stream.");         
		return -1;
	}
	
	media_handle->pFormatCtx = pFormatCtx;//save pFormatCtx point 

	if(avformat_find_stream_info(pFormatCtx,NULL)<0){// find stream 
		LOGD("Couldn't find stream information.");          
		return -1; 
	}

	LOGD("-----------------media file info-----------------");
	av_dump_format(pFormatCtx,0, 0, 0);
	LOGD("-------------------------------------------------");

	video_index = av_find_best_stream(pFormatCtx,AVMEDIA_TYPE_VIDEO,video_index,-1,NULL,0);
	audio_index = av_find_best_stream(pFormatCtx,AVMEDIA_TYPE_AUDIO,audio_index,video_index,NULL,0);

	LOGD("video_index = %d audio_index = %d",video_index,audio_index);
#if 0
	if(audio_index >= 0)  
    {  
        stream_component_open(is, audio_index);  
    }
#endif
    if(video_index >= 0)  
    {  
        audio_video_open(media_handle, video_index);  
		LOGD("video_index = %d audio_index = %d media_handle->videoStream = %d",video_index,audio_index,media_handle->videoStream);
    }  
	if(audio_index >= 0)
	{
#if 0
		audio_video_open(media_handle,audio_index);
#endif
	}

//	if(media_handle->videoStream < 0 || media_handle->audioStream < 0)
	if(media_handle->videoStream < 0)
	{
		LOGD("%s: could not open codecs",media_handle->filename);
		return -1;
	}

	for(;;)
	{
	
#if 0
		if(media_handle->quit)  
        {  
            break;  
        }  

		// seek stuff goes here  
        if(media_handle->audioq.size > MAX_AUDIOQ_SIZE ||  
                media_handle->videoq.size > MAX_VIDEOQ_SIZE)  
        {  
            mySleep(0,10);  
            continue;  
        } 
#endif
		if(av_read_frame(media_handle->pFormatCtx,packet) < 0)
			break;

		 // Is this a packet from the video stream?  
        if(packet->stream_index == media_handle->videoStream)  
        {  
            packet_queue_put(&media_handle->videoq, packet); //将视频包添加到队列尾部 
        }  
        else if(packet->stream_index == media_handle->audioStream)  
        {  
            packet_queue_put(&media_handle->audioq, packet); // 将音频包添加到队列的尾部
        }  
        else  
        {  
            av_free_packet(packet);  
        }  
		
	}		

	return 0;
	
}



int main(int argc,char* argv[])
{
	MediaState      *media_handle;  
	int ret;
	media_handle = (MediaState *)av_mallocz(sizeof(MediaState));  
    if(argc < 2)  
    {  
        LOGD("Usage: test <file>");  
        exit(1);  
    } 

	avcodec_register_all();    
	avfilter_register_all();    
	av_register_all();
	if(SDL_Init(SDL_INIT_TIMER))  
    {  
        LOGD("Could not initialize SDL - %s", SDL_GetError());  
        exit(1);  
    }  
	

	strcpy(media_handle->filename,argv[1]);

	pthread_t id;
	ret = pthread_create(&id,NULL,(void *)audio_video_decode_thread,media_handle);
	if(ret != 0)
	{
		LOGD("Create audio_video_decode_thread pthread error");
		return -1;
	}

	pthread_join(id,NULL);//等待子进程结束
	pthread_join(media_handle->video_pid,NULL);
	
	return 0;
	
	
}



/****************************************************SYNC OPS STR***********************************************/

int our_get_buffer(struct AVCodecContext *c, AVFrame *pic)  
{  
    int ret = avcodec_default_get_buffer(c, pic);  
    uint64_t *pts = (uint64_t *)av_malloc(sizeof(uint64_t));  
    *pts = global_video_pkt_pts;  
    pic->opaque = pts;  
    return ret;  
}  
void our_release_buffer(struct AVCodecContext *c, AVFrame *pic)  
{  
    if(pic) av_freep(&pic->opaque);  
    avcodec_default_release_buffer(c, pic);  
}  

double synchronize_video(MediaState *media_handle, AVFrame *src_frame, double pts)  
{  
    double frame_delay;  
  
    if(pts != 0)  
    {  
        /* if we have pts, set video clock to it */  
		media_handle->video_clock = pts;
    }  
    else  
    {  
        /* if we aren't given a pts, set it to the clock */    
		pts = media_handle->video_clock;
    }  
    /* update the video clock */ 
    frame_delay = av_q2d(media_handle->video_st->codec->time_base);  
    /* if we are repeating a frame, adjust clock accordingly */  
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);  
    media_handle->video_clock += frame_delay;  
	
    return pts;  
} 

/****************************************************SYNC OPS END***********************************************/



/****************************************************V4L2 OPS STR***********************************************/


int __v4l2_mmap(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int *buffcnt)//传递时pui_buffcnt为1
{
	int ret;
	unsigned int ui_idx = 0;
	struct v4l2_buffer vo_reqoutput_buf;

	struct v4l2_requestbuffers vobuf_req;
	memset(&vobuf_req, 0, sizeof(vobuf_req));

	vobuf_req.count = 1;
	vobuf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;//向驱动申请内存。
	vobuf_req.memory = V4L2_MEMORY_MMAP;
	if (ioctl(video_fd, VIDIOC_REQBUFS, &vobuf_req) < 0)
	{
		LOGD("ioctl VIDIOC_REQBUFS error");
		return -1;
	}

	do
	{
		memset(&vo_reqoutput_buf, 0, sizeof(vo_reqoutput_buf));
		vo_reqoutput_buf.index = ui_idx;
		vo_reqoutput_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		ret = ioctl(video_fd, VIDIOC_QUERYBUF, &vo_reqoutput_buf);//查询申请到的内存。
		LOGD("__v4l2_mmap i_ret=%d VIDIOC_QUERYBUF \n \t index=%d, type=%d, bytesused=%d, flags=%#x, field=%d, time[%d.%d] \n",ret,\
			vo_reqoutput_buf.index, vo_reqoutput_buf.type, vo_reqoutput_buf.bytesused, vo_reqoutput_buf.flags,\
			vo_reqoutput_buf.field, vo_reqoutput_buf.timestamp.tv_sec, vo_reqoutput_buf.timestamp.tv_usec);

		 LOGD("\t memory=%d, length=%d, offset=%d\n",vo_reqoutput_buf.memory, vo_reqoutput_buf.length, vo_reqoutput_buf.m.offset);

		 if (ret < 0)        
		 {            
		 	break;        
		 }

		 /* mmap */            
		 past_buff[ui_idx].pui_addr = (unsigned int *)mmap(NULL, vo_reqoutput_buf.length,//依据查询到的信息，映射内存空间。
		 								 PROT_READ | PROT_WRITE, MAP_SHARED,
		 								 video_fd, vo_reqoutput_buf.m.offset);
		 if (past_buff[ui_idx].pui_addr == NULL)
		 {
			break;
		 }

		 past_buff[ui_idx].ui_addrlen = vo_reqoutput_buf.length;
		 past_buff[ui_idx].ui_offset = vo_reqoutput_buf.m.offset;

		 ui_idx++;
		 if (ui_idx >= *buffcnt)
		 {
			 break;
		 }
		
	}while(1);

	*buffcnt = ui_idx;

	for (ui_idx = 0; ui_idx < *buffcnt; ui_idx++)
	{
		LOGD("\t pui_addr=%#x, ui_addrlen=%d, ui_offset=%d\n", \
			past_buff[ui_idx].pui_addr, past_buff[ui_idx].ui_addrlen, past_buff[ui_idx].ui_offset);
	}

	if (*buffcnt == 0)
	{
		 return -1;
	}

	return 0;
}


int __v4l2_alpha_config(int video_fd)
{
	struct v4l2_format vo_format;
	memset(&vo_format, 0, sizeof(vo_format));
	vo_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (ioctl(video_fd, VIDIOC_G_FMT, &vo_format) < 0)
	{
		LOGD("ioctl VIDIOC_G_FMT error!");
		return -1;
	}

	vo_format.fmt.pix.width = V4L2_OSD_WIDTH;//像素设置
	vo_format.fmt.pix.height = V4L2_OSD_HEIGHT;
	vo_format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
	vo_format.fmt.pix.field = V4L2_FIELD_NONE;
	vo_format.fmt.pix.bytesperline = 0;
	vo_format.fmt.pix.sizeimage = V4L2_OSD_WIDTH * V4L2_OSD_HEIGHT * 4;
	vo_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
	if (ioctl(video_fd, VIDIOC_S_FMT, &vo_format) < 0)//设定输出格式
	{
		LOGD("ioctl VIDIOC_S_FMT error!");
		return -1;
	}

	struct v4l2_framebuffer vo_fb;
	memset(&vo_fb, 0, sizeof(vo_fb));
	if (ioctl(video_fd, VIDIOC_G_FBUF, &vo_fb) < 0) 
	{
		LOGD("ioctl VIDIOC_G_FBUF error");
		return -1;
	}

	if (!(vo_fb.capability & V4L2_FBUF_CAP_LOCAL_ALPHA))
	{
		LOGD("does not support per-pixel alpha");
		return -1;
	}

	vo_fb.flags |= V4L2_FBUF_FLAG_LOCAL_ALPHA;
	vo_fb.flags &= ~(V4L2_FBUF_FLAG_CHROMAKEY | V4L2_FBUF_FLAG_SRC_CHROMAKEY);
	if (ioctl(video_fd, VIDIOC_S_FBUF, &vo_fb) < 0) 
	{
		LOGD("ioctl VIDIOC_S_FBUF error !");
		return -1;
	}
	
	return 0;
	
	
}


static int first_flag = 1;

int __v4l2_mmap_fill_color(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt,char *pc_tmpbuff,int tmpbufflen)
{
	 struct v4l2_buffer vo_dqoutput_buf;
	 int ret = 0;
	 int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	 ioctl(video_fd, VIDIOC_STREAMON, &type);
	 memset(&vo_dqoutput_buf, 0, sizeof(vo_dqoutput_buf));

	 vo_dqoutput_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	 vo_dqoutput_buf.memory = V4L2_MEMORY_MMAP;
	 vo_dqoutput_buf.field = V4L2_FIELD_NONE;
	 vo_dqoutput_buf.index = 0;

	 if(first_flag == 1) 
	 {
	 		ret = ioctl(video_fd, VIDIOC_QBUF, &vo_dqoutput_buf);//将空闲内存加入到队列。
	 		if(ret < 0)
	 		{
				LOGD("ioctl VIDIOC_QBUF error!");
				//	return -1;
			}
			first_flag = 0;
	 }

	 memcpy(past_buff[0].pui_addr, pc_tmpbuff, tmpbufflen);//pc_tmpbuff 的值读取到虚拟地址空间。
	 
	 return 0;
}


int __v4l2_umamp(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt)
{
	unsigned int ui_idx = 0;
	while(1)
	{
		munmap(past_buff[ui_idx].pui_addr, past_buff[ui_idx].ui_addrlen);
		ui_idx++;
		if (ui_idx >= ui_buffcnt)
		{
			break;
		}

		struct v4l2_requestbuffers vobuf_req;
		memset(&vobuf_req, 0, sizeof(vobuf_req));
		vobuf_req.count = 0; 
		vobuf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		vobuf_req.memory = V4L2_MEMORY_MMAP;
		if (ioctl(video_fd, VIDIOC_REQBUFS, &vobuf_req) < 0)
		{
			LOGD("ioctl VIDIOC_REQBUFS error");
			return -1;
		}
	}
	
	return 0;
}


/****************************************************V4L2 OPS END***********************************************/

int mySleep(int sec, int msec)
{
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = msec * 1000;

	select(0, NULL, NULL, NULL, &tv);

	return 0;
}

