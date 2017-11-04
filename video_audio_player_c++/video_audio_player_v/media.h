
#ifndef _MEDIA_H_
#define _MEDIA_H_

#include <iostream>
#include <string>

using namespace std;
extern "C"{
    #include "libavformat/avformat.h"
    #include "libavcodec/avcodec.h"
    #include "libswscale/swscale.h"
	#include "libswresample/swresample.h"
    #include <stdio.h>
    #include "SDL/SDL.h"
    #include "SDL/SDL_thread.h"
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
}

typedef struct
{    
	unsigned int *pui_addr;    
	unsigned int ui_addrlen;    
	unsigned int ui_offset;
}V4L2_MAMP_ADDR_INFO_S;//保存映射后的内存信息。

#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__)
#define CONVERT_WITH 800
#define CONVERT_HIGH 480
#define VIDEO_DEVICE "/dev/video1" 
#define MAX_AUDIO_FRAME_SIZE 192000 

extern int __v4l2_mmap(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int *buffcnt);
extern int __v4l2_alpha_config(int video_fd);
extern int __v4l2_mmap_fill_color(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt,char *pc_tmpbuff,int tmpbufflen);
extern int __v4l2_umamp(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt);

class MediaState{//基类
	public:
		MediaState(char *inputfile);
		virtual ~MediaState();//虚拟析构函数
		bool open_input();
		bool dev_init() { return media_dev_init(); }
		bool dev_play() { return media_play(); }
		
		char 					*filename;	

	private:
		virtual bool media_dev_init()=0;	//纯虚函数
		virtual bool media_play()=0;//纯虚函数

	protected:
		AVFormatContext 		*pFormatCtx;
//video
		AVCodecContext			*v4lCodecCtx;
		AVCodec 				*v4lCodec;
		AVPacket 				*packet;
		AVFrame					*pFrame;//convert use
		AVFrame					*pFrameYUV;//convert use
		uint8_t					*video_out_buffer;
		struct SwsContext 		*img_convert_ctx;
		V4L2_MAMP_ADDR_INFO_S 	*addinfo;
		unsigned int 			addcnt;
		int 					video_fd;
		int 					video_index;
//audio
		int 					audio_index;
		AVCodecContext			*pCodecCtx_audio;
		AVCodec					*pCodec_audio;		
		AVPacket				*audio_packet;
		AVFrame					*audio_frame;
		struct SwrContext 		*convert_ctx;
		
		uint64_t 				out_channel_layout;  //采样的布局方式
		int64_t 				in_channel_layout;
		int 					out_nb_samples; //采样个数
		enum AVSampleFormat		sample_fmt; //采样格式
		int 					out_sample_rate;//采样率
		int 					out_channels;//通道数
		unsigned int 			buffer_size;//缓冲区大小
		uint8_t 				*buffer;//缓冲区指针
		
		snd_pcm_t 				*pcm;
		snd_pcm_hw_params_t		*params;
};

class AudioState : public MediaState{// AudioState 是 MediaState 派生类
	public:
		AudioState(char *inputfile);
		~AudioState();

	private:
		bool media_dev_init();
		bool media_play();		
};

class VideoState : public MediaState{
	public:
		VideoState(char *inputfile);
		~VideoState();
	
	private:
		bool media_dev_init();
		bool media_play();
};

#endif

