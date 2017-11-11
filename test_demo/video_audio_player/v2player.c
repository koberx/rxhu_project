#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__);
bool GetNextFrame(AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx, int videoStream, AVFrame *pFrame);

int main(int argc,char* argv[])
{
	AVFormatContext *pFormatCtx;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame;
	AVFrame *pFrameRGB;
	int     numBytes;
	uint8_t *buffer;

	int i,videoStream;
	
	av_register_all();	
	const char *filename=argv[1];
	if(avformat_open_input(&pFormatCtx, filename, NULL,NULL)!=0)
	{
		LOGD("Cann't open the media file!");
		return -1;
	}	
	
	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
    {
		LOGD("Cann't find the stream info");
		return -1;
	}

	videoStream=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
    {
        videoStream=i;
        break;
    }	


	if(videoStream==-1)
    {
		LOGD("Cann't find the videoStream!");
		return -1;
	} // Didn't find a video stream
	
	pCodecCtx=pFormatCtx->streams[videoStream]->codec;

	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL)
   	{
		LOGD("Cann't find the decoder!");
		return -1;
	} //cann't find the coder
		
	

	if(pCodec->capabilities & CODEC_CAP_TRUNCATED)
    	pCodecCtx->flags|=CODEC_FLAG_TRUNCATED;

	// open the coder
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
    {
		LOGD("Cann't open the decoder");
		return -1;
	}
	//modify the frame speed
	if(pCodecCtx->framerate.num>1000 && pCodecCtx->framerate.den==1)
	    pCodecCtx->framerate.den=1000;
	
	pFrame=avcodec_alloc_frame();

	// 分配一个AVFrame 结构的空间
	pFrameRGB=avcodec_alloc_frame();
	if(pFrameRGB==NULL)
   	{
		LOGD("Cann't av alloc the FrameRGB!");
		return -1;
	}
	
	// 确认所需缓冲区大小并且分配缓冲区空间
	numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,pCodecCtx->height);
	
	buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
	
	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,pCodecCtx->width, pCodecCtx->height);	

	while(GetNextFrame(pFormatCtx, pCodecCtx, videoStream, pFrame))
	{
    	img_convert((AVPicture *)pFrameRGB, PIX_FMT_RGB24, (AVPicture*)pFrame,pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

    //	DoSomethingWithTheImage(pFrameRGB);// can output to the video device
	}
	


	return 0;
}

bool GetNextFrame(AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx, int videoStream, AVFrame *pFrame)
{
	static AVPacket packet;
    static int      bytesRemaining=0;
    static uint8_t  *rawData;
    static bool     fFirstTime=true;
    int bytesDecoded;
    int frameFinished;
	//fist call set the packet.data NULL
	if(fFirstTime)
    {
        fFirstTime=false;
        packet.data=NULL;
    }
	
	//decoder until to the frame size
	while(true)
	{
		while(bytesRemaining > 0)
		{
			bytesDecoded=avcodec_decode_video(pCodecCtx, pFrame,&frameFinished, rawData, bytesRemaining);
			
			if(bytesDecoded < 0)
			{
				LOGD("Error while decoding frame");	
				return false;	
			}
			bytesRemaining-=bytesDecoded;
			rawData+=bytesDecoded;
			
			if(frameFinished)//if finish return
                return true;
		}

		/*read the next pakage */		
		do{
			/*free the old pakage*/
			if(packet.data!=NULL)
                av_free_packet(&packet);
			/*read the new pakage*/
			if(av_read_packet(pFormatCtx, &packet)<0)
                goto loop_exit;
        }while(packet.stream_index!=videoStream);	

		bytesRemaining=packet.size;
        rawData=packet.data;
		
	}

loop_exit:
	/* the last frame rest stream*/
	bytesDecoded=avcodec_decode_video(pCodecCtx, pFrame, &frameFinished, rawData, bytesRemaining);
	
	/*free the the last pakage*/
	if(packet.data!=NULL)
        av_free_packet(&packet);
	
	return frameFinished!=0;
	
}

int img_convert(AVPicture *dst, enum PixelFormat dst_pix_fmt,const AVPicture *src, enum PixelFormat src_pix_fmt, int src_width,int src_height)
{
    int w;
    int h;
    struct SwsContext *pSwsCtx;


    w = src_width;
    h = src_height;
    pSwsCtx = sws_getContext(w, h, src_pix_fmt, w, h, dst_pix_fmt, SWS_BICUBIC,NULL, NULL, NULL);

    sws_scale(pSwsCtx, (const uint8_t * const *)src->data, src->linesize, 0, h, dst->data,dst->linesize);//把RGB转化为image
    //这样释放掉pSwsCtx的内存

    return 0;
}

