#include <iostream>
#include <string>
using namespace std;
extern "C"{
	#include "libavformat/avformat.h"
	#include "libavcodec/avcodec.h"
	#include "libswscale/swscale.h"
	#include <libswresample/swresample.h>
	#include <alsa/asoundlib.h>
	#include <stdio.h>
	#include <stdbool.h>
}

#define MAX_AUDIO_FRAME_SIZE 192000 
#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__)

class AudioState{
	public:
		AudioState(char *inputfile);
		~AudioState();	
		bool open_input();
		bool init_pcm_snd();
		bool audio_play();
		char 					*filename;

	private:

		AVFormatContext 		*pFormatCtx;
		AVCodecContext  		*pCodecCtx_audio;
		AVCodec         		*pCodec_audio;
		AVPacket 				*packet; 
		AVFrame 				*frame;
		struct SwrContext 		*convert_ctx;
		int 					audio_index;

		uint64_t 				out_channel_layout;  //采样的布局方式
		int64_t 				in_channel_layout;
		int 					out_nb_samples; //采样个数
		enum AVSampleFormat		sample_fmt; //采样格式
		int 					out_sample_rate;//采样率
		int 					out_channels;//通道数
		unsigned int 			buffer_size;//缓冲区大小
		uint8_t 				*buffer;//缓冲区指针
		
		snd_pcm_t				*pcm;
		snd_pcm_hw_params_t		*params;			

};

AudioState::AudioState(char *inputfile)
	:filename(inputfile)
{
	pFormatCtx = avformat_alloc_context();
	pCodecCtx_audio = NULL;
	pCodec_audio = NULL;
	packet = (AVPacket *)malloc(sizeof(AVPacket));
	av_init_packet(packet);
	frame = av_frame_alloc();
	convert_ctx = swr_alloc();
	audio_index = -1;
	
	out_channel_layout = AV_CH_LAYOUT_STEREO;
	out_nb_samples = 1024;
	sample_fmt = AV_SAMPLE_FMT_S16;
	out_sample_rate = 44100;
	out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, sample_fmt, 1);
	buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE);
	in_channel_layout = 0;

	pcm = NULL;
	params = NULL;
}

AudioState::~AudioState()
{
	if (buffer)
		av_free(buffer);

	swr_free(&convert_ctx);
	avcodec_close(pCodecCtx_audio);
    avformat_close_input(&pFormatCtx);
    snd_pcm_close(pcm);
	
}

bool AudioState::open_input()
{
	int i;
	
	
	if(avformat_open_input(&pFormatCtx,filename,NULL,NULL)!=0){       //打开多媒体文件  
        LOGD("Couldn't open input stream.");  
        return false;  
    } 

	if(avformat_find_stream_info(pFormatCtx,NULL)<0){                 //读取音视频数据相关信息，参数0：上下文结构体指针，参数1：option  
        LOGD("Couldn't find stream information.");  
        return false;  
    } 

	
	for(i=0; i<pFormatCtx->nb_streams; i++)                //遍历多媒体文件中的每一个流，判断是否为视频。  
    {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            audio_index=i;
            break;
        }
    }

	if(audio_index==-1){
        LOGD("Didn't find a video stream.");
        return false;
    }

	pCodecCtx_audio = pFormatCtx->streams[audio_index]->codec; //codec上下文指定到格式上下文中的codec  
    pCodec_audio = avcodec_find_decoder(pCodecCtx_audio->codec_id); //找到一个codec，必须先调用av_register_all（）

	if(pCodec_audio == NULL){
        LOGD("Codec not found.");
        return false;
    }

	in_channel_layout = av_get_default_channel_layout(pCodecCtx_audio->channels);

	if(avcodec_open2(pCodecCtx_audio, pCodec_audio,NULL)<0){ //初始化一个视音频编解码器的AVCodecContext  
        LOGD("Could not open codec.");
        return false;
    }
 
	//设置转码参数
	convert_ctx = swr_alloc_set_opts(convert_ctx, out_channel_layout, sample_fmt, out_sample_rate, \
            in_channel_layout, pCodecCtx_audio->sample_fmt, pCodecCtx_audio->sample_rate, 0, NULL);

	//初始化转码器
    swr_init(convert_ctx);

	av_dump_format(pFormatCtx,0, 0, 0);
	LOGD("Bitrate :\t %3d", pFormatCtx->bit_rate);
    LOGD("Decoder Name :\t %s", pCodecCtx_audio->codec->long_name);
    LOGD("Cannels:\t %d", pCodecCtx_audio->channels);
    LOGD("Sample per Second:\t %d", pCodecCtx_audio->sample_rate);

	return true;
}

bool AudioState::audio_play()
{
	int index = 0,got_audio;
	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == audio_index) {
			//解码声音
            if (avcodec_decode_audio4(pCodecCtx_audio, frame, &got_audio, packet) < 0) {
                LOGD("decode error");
                return false;
            }
			
			if (got_audio > 0) {
				//转码
                swr_convert(convert_ctx, &buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);
				LOGD("index: %5d\t pts:%10lld\t packet size:%d\n", index, packet->pts, packet->size);
				if(snd_pcm_writei (pcm, buffer, buffer_size / 2) < 0)
				{
					LOGD("can't send the pcm to the sound card!");
					return false;
				}
			}
		}
		index++;
		av_free_packet(packet);
	}
	
	LOGD("The media file is play over!");
	return true;
}

bool AudioState::init_pcm_snd()
{
	int ret,dir = 0;
	ret = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if(ret < 0)
    {
      	LOGD("open PCM device failed:");
      	return false;
    }

	snd_pcm_hw_params_alloca(&params); //分配params结构体

	ret = snd_pcm_hw_params_any(pcm, params);//初始化params
    if(ret < 0)
    {
      	LOGD("snd_pcm_hw_params_any:");
      	return false;
    }

	ret = snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED); //初始化访问权限
    if(ret < 0)
    {
      	LOGD("sed_pcm_hw_set_access:");
      	return false;
    }
	
	ret = snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16_LE); //设置16位采样精度 
    if(ret < 0)
    {
      	LOGD("snd_pcm_hw_params_set_format failed:");
      	return false;
    }
	
	ret = snd_pcm_hw_params_set_channels(pcm, params, out_channels); //设置声道,1表示单声>道，2表示立体声
    if(ret < 0)
    {
      	LOGD("snd_pcm_hw_params_set_channels:");
      	return false;
    } 

	ret = snd_pcm_hw_params_set_rate_near(pcm, params, (unsigned int *)(&out_sample_rate), &dir); //设置>频率
    if(ret < 0)
    {
      	LOGD("snd_pcm_hw_params_set_rate_near:");
      	return false;
    } 

	ret = snd_pcm_hw_params(pcm, params);
    if(ret < 0)
    {
      	LOGD("snd_pcm_hw_params: ");
      	return false;
    } 

	if ((ret = snd_pcm_prepare (pcm)) < 0) {     
      	LOGD("cannot prepare audio interface for use (%s)",snd_strerror (ret));     
      	return false;     
   	}

	LOGD("the pcm device set ok!");

	return true; 
}

int main(int argc,char *argv[])
{
	AudioState *audio = NULL;
	if(argc < 2)
	{
		LOGD("please select the media file!");
		return -1;
	}

	avcodec_register_all();
    //avfilter_register_all();
    av_register_all();

	audio = new AudioState(argv[1]);
	
	if(!audio->open_input())
	{
		LOGD("Check the media file failed!");
		return -1;
	}

	if(!audio->init_pcm_snd())
	{
		LOGD("init the snd card failed!");
		return -1;
	}
		
	if(!audio->audio_play())
	{
		LOGD("the audio playing failed!");
		return -1;
	}

	if(audio)
		delete audio;
		
	return 0;
}

