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

#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__);
#define FRAMES_NEED 100000
#define CONVERT_WITH 656
#define CONVERT_HIGH 304
#define SAVEBMPNAME "./savefile.bmp"
#define	VIDEO_DEVICE "/dev/video1" 
#define V4L2_OSD_WIDTH              656
#define V4L2_OSD_HEIGHT             304
static int first_flag = 1;

typedef struct
{
    unsigned int *pui_addr;
    unsigned int ui_addrlen;
    unsigned int ui_offset;
}V4L2_MAMP_ADDR_INFO_S;//保存映射后的内存信息。


typedef struct {
	char blue;
	char green;
	char red; 
}__attribute__((packed)) RawRgb;

int main(int argc,char* argv[])
{
	AVFormatContext *pFormatCtx;   //格式上下文结构体  
    int             i, videoindex;  
    AVCodecContext  *pCodecCtx;    //codec上下文  
    AVCodec         *pCodec;       //codec  
    AVFrame *pFrame;               //Frame结构体  
    AVFrame *pFrameYUV;            //Frame结构体  
    uint8_t *out_buffer;             
    AVPacket *packet;              //packet结构体  
    int y_size,y_size_align;  
    int ret, got_picture;  
    unsigned int frame_num = 0;  
    struct SwsContext *img_convert_ctx;  //图像格式转化上下文 

	char *filepath = argv[1];// input
	
	FILE *fp_frame = fopen("output.rgb","wb+");  //output    

	av_register_all();                         //ffmpeg flow 0,注册codec
	pFormatCtx = avformat_alloc_context();     //格式上下文结构体指针开空间  
	
	if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){       //打开多媒体文件  
        LOGD("Couldn't open input stream.");  
        return -1;  
    }  

	if(avformat_find_stream_info(pFormatCtx,NULL)<0){                 //读取音视频数据相关信息，参数0：上下文结构体指针，参数1：option  
        LOGD("Couldn't find stream information.");  
        return -1;  
    }  
	
	videoindex=-1; 
	for(i=0; i<pFormatCtx->nb_streams; i++)                //遍历多媒体文件中的每一个流，判断是否为视频。  
	{
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){  
			videoindex=i;  
			break;
		}
	}

	if(videoindex==-1){  
        LOGD("Didn't find a video stream.");  
        return -1;  
    }  

	pCodecCtx=pFormatCtx->streams[videoindex]->codec; //codec上下文指定到格式上下文中的codec  
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id); //找到一个codec，必须先调用av_register_all（）  

	 if(pCodec==NULL){  
        LOGD("Codec not found.");  
        return -1;  
    }  

	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){ //初始化一个视音频编解码器的AVCodecContext  
        LOGD("Could not open codec.");  
        return -1;  
    }  

	pFrame=av_frame_alloc();  //原始帧  
    pFrameYUV=av_frame_alloc();//YUV帧  
			/* 宏AV_PIX_FMT_YUV420P 代替宏PIX_FMT_YUV420P */
	//out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height)); 	
	out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_RGB32, CONVERT_WITH, CONVERT_HIGH)); 	
	/*  将pFrameYUV和out_buffer联系起来（pFrame指向一段内存）；宏AV_PIX_FMT_YUV420P 代替宏PIX_FMT_YUV420P   */
	//avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height); 
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_RGB32, CONVERT_WITH, CONVERT_HIGH); 

	packet=(AVPacket *)av_malloc(sizeof(AVPacket)); //开空间  

	LOGD("--------------- File Information ----------------");
	av_dump_format(pFormatCtx,0,filepath,0);//调试函数，输出文件的音、视频流的基本信息  
	LOGD("-------------------------------------------------"); 
	
	/*  初始化SWS，图片格式装换上下文//经过验证，输出YUV不需要格式转换，但需要调整尺寸  */	
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		//pCodecCtx->width, pCodecCtx->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL); 
		CONVERT_WITH, CONVERT_HIGH, PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL); 
	/*	v4l2 ops	*/
	int video_fd = -1;

    video_fd = open(VIDEO_DEVICE, O_RDWR);// 打开设备
    if (video_fd < 0)
    {
        LOGD("open the video device error!");
		return -1;
    }
    LOGD("open the video device successful!");
	
	V4L2_MAMP_ADDR_INFO_S addinfo[6];
    unsigned int addcnt = 1;
    memset(addinfo, 0, sizeof(addinfo));
	if (__v4l2_alpha_config(video_fd) != 0) ;    
	{
		LOGD("__v4l2_alpha_config error!");
//		close(video_fd);	
//		return -1;
	}

    if (__v4l2_mmap(video_fd, addinfo, &addcnt) != 0)
	{
        LOGD("__v4l2_mmap error!");
        close(video_fd);
        return -1;
    }
		
	/* end v4l2 ops*/

	while(av_read_frame(pFormatCtx, packet)>=0){       //读取码流中的音频若干帧或者视频一帧,作为packet 
		if(packet->stream_index==videoindex){          //如果是视频 
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if(ret < 0){  
				LOGD("Decode Error.");
				return -1;
			}
			if(got_picture)
			{/*	将输出结果转化成YUV，输出YUV不需要格式转换,但是需要调整尺寸，pFrame中的图像数据的对齐方式可能是按64对齐的。  */
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
					 pFrameYUV->data, pFrameYUV->linesize);

//				fwrite(pFrameYUV->data[0],CONVERT_WITH*CONVERT_HIGH*3,1,fp_frame); 
//				save_to_bmp(pFrameYUV->data[0],CONVERT_WITH,CONVERT_HIGH);	
				frame_num++; 
//				LOGD("pCodecCtx->width =%d , pCodecCtx->height = %d",pCodecCtx->width,pCodecCtx->height);	
				ret = __v4l2_mmap_fill_color(video_fd, addinfo, addcnt,pFrameYUV->data[0],CONVERT_WITH*CONVERT_HIGH*4);
				if(ret < 0)
				{
					LOGD("__v4l2_mmap_fill_color error!");
					ioctl(video_fd, VIDIOC_STREAMOFF,V4L2_BUF_TYPE_VIDEO_OUTPUT);
					close(video_fd);
					return -1;
				}
				mySleep(0,30000);
#ifndef FRAMES_NEED  
                if(frame_num == FRAMES_NEED){  
                    LOGD("%d frames done!",frame_num);  
                    break;
				}  
#endif  
				if(frame_num%100 == 0)  
                    LOGD("%d frames done!",frame_num);
			}
			
		}
		av_free_packet(packet);
	}

	__v4l2_umamp(video_fd, addinfo, addcnt);
	sws_freeContext(img_convert_ctx);  
  
    fclose(fp_frame);  
    av_frame_free(&pFrameYUV);  
    av_frame_free(&pFrame);  
    avcodec_close(pCodecCtx);  
    avformat_close_input(&pFormatCtx);  

	return 0;
}

int save_to_bmp(unsigned char *raw,int xres,int yres)
{
	int i,j,bmp,nwrite;
	BitMapFileHeader File_head;
	BitMapInfoHeader Map_info;
	RawRgb *p = (RawRgb *)raw;

	bmp = open(SAVEBMPNAME, O_WRONLY | O_CREAT,0777);
	if(bmp < 0)
	{
		LOGD("create or open the bmp file failed!");
		return -1;
	}

	/* the file head info*/
	File_head.bfType[0] = 0x42;
	File_head.bfType[1] = 0x4d;
	File_head.bfSize = xres * yres * 3 + 54;
	File_head.bfReserved1 = 0x0;
	File_head.bfReserved2 = 0x0;
	File_head.bfOffBits = 54;
	nwrite = write(bmp,&File_head,sizeof(File_head));
	if(nwrite <= 0)
	{
		LOGD("write the file_head failed!");
		return -1;
	}

	/* the bitmap info*/
	Map_info.biSize = 40;
	Map_info.biWidth = xres;
	Map_info.biHeight = yres;
	Map_info.biPlanes = 1;
	Map_info.biBitCount = 24;
	Map_info.biCompression = 0;
	Map_info.biSizeImage = 0;
	Map_info.biXPelsPerMeter = 0;
	Map_info.biYPelsPerMeter = 0;
	Map_info.biClrUsed = 0;
	Map_info.biClrImportant = 0;
	nwrite = write(bmp,(char *)&Map_info,sizeof(Map_info));
	if(nwrite <= 0)
	{
		LOGD("write the bitmap info failed!");
		return -1;
	}

	/* read the rgb data  write to mybmpfile*/
#if 0
	raw += yres * xres * 3 -1;
	for(i = 0; i < yres * xres * 3; i++)
	{
		nwrite = write(bmp,raw,1);
		if(nwrite <= 0)
		{
			LOGD("write the pix failed! i=%d j=%d",i,j);
			return -1;
		}
		raw--;
	}
	
	LOGD("-------> i=%d j=%d",i,j);
#else
	RgbQuad rgb;
	for(i = 0; i < yres; i++)
	{
		RawRgb *c = p + (yres - 1 - i) * xres;
		RawRgb cc;
		for(j = 0; j < xres; j++)
		{
			cc = *(c + j);
//			c += j*3;
//			strncpy(cc,c,sizeof(cc));
//			rgb.Blue = cc;
//			rgb.Green = cc >> 8;
//			rgb.Red = cc >> 16;
			rgb.Blue = cc.blue;
			rgb.Green = cc.green;
			rgb.Red = cc.red;
			nwrite = write(bmp,(char *)&rgb,3);
			if(nwrite <= 0)
			{
				LOGD("write the pix failed! i=%d j=%d",i,j);
				return -1;
			}
		}

	}
	LOGD("-------> i=%d j=%d xres = %d yres = %d",i,j,xres,yres);	

#endif

	close(bmp);

	return 0;
}




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
	
        LOGD("\t memory=%d, length=%d, offset=%d\n", \
            vo_reqoutput_buf.memory, vo_reqoutput_buf.length, vo_reqoutput_buf.m.offset);
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
    }while (1);

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


/*设置v4l2的配置*/
int __v4l2_alpha_config(int video_fd)//参数设备节点的描述符
{
    struct v4l2_format vo_format;

    memset(&vo_format, 0, sizeof(vo_format));
    vo_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;//设定为输出方向
    if (ioctl(video_fd, VIDIOC_G_FMT, &vo_format) < 0)
    {
        LOGD("ioctl VIDIOC_G_FMT error!");
        return -1;
    }
    vo_format.fmt.pix.width = V4L2_OSD_WIDTH;//像素设置
    vo_format.fmt.pix.height = V4L2_OSD_HEIGHT;
    vo_format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;//V V4L2_PIX_FMT_ARGB32
    vo_format.fmt.pix.field = V4L2_FIELD_NONE;
    vo_format.fmt.pix.bytesperline = 0;
    //vo_format.fmt.pix.sizeimage = 1536000;
    vo_format.fmt.pix.sizeimage = V4L2_OSD_WIDTH * V4L2_OSD_HEIGHT * 4;
    vo_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    if (ioctl(video_fd, VIDIOC_S_FMT, &vo_format) < 0)//设定输出格式
    {
		LOGD("ioctl VIDIOC_S_FMT error!");
        return -1;
    }
#if 0
    struct v4l2_crop vo_crop;
    
    vo_crop.type = vo_format.type;//设定显示尺寸设置，这里是满屏。
    vo_crop.c.left = 0;
    vo_crop.c.top = 0;
    vo_crop.c.width = vo_format.fmt.pix.width;
    vo_crop.c.height = vo_format.fmt.pix.height;
    
    if (ioctl(video_fd, VIDIOC_S_CROP, &vo_crop) < 0)
    {
		LOGD("ioctl VIDIOC_S_CROP error");
        return -1;
    }
    
    memset(&vo_format, 0, sizeof(vo_format));
    vo_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY;
	if (ioctl(video_fd, VIDIOC_G_FMT, &vo_format) < 0) 
    {
		LOGD("ioctl VIDIOC_G_FMT error");
        return -1;
    }

	vo_format.fmt.win.w.width = V4L2_OSD_WIDTH;//overlay的尺寸
	vo_format.fmt.win.w.height = V4L2_OSD_HEIGHT;
	vo_format.fmt.win.w.left = 0;
	vo_format.fmt.win.w.top = 0;

    vo_format.fmt.win.global_alpha = 0xff;
    if (ioctl(video_fd, VIDIOC_S_FMT, &vo_format) < 0)
    {
		LOGD("ioctl VIDIOC_S_FMT error")
        return -1;
    }
#endif
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

    while (1)
    {
        munmap(past_buff[ui_idx].pui_addr, past_buff[ui_idx].ui_addrlen);
        ui_idx++;
        if (ui_idx >= ui_buffcnt)
        {
            break;
        }
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

    return 0;
}


int mySleep(int sec, int usec)
{
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = usec;

    select(0, NULL, NULL, NULL, &tv);

    return 0;
}

