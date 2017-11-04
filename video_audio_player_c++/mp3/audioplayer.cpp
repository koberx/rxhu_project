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

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 
#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__)
void ffmpeg_fmt_to_alsa_fmt(AVCodecContext *pCodecCtx, snd_pcm_t *pcm, snd_pcm_hw_params_t *params);

class AudioState{
	public:
		AudioState(char *inputfile);
		AudioState(AVCodecContext *audio_ctx, int audio_stream);
		~AudioState();
		bool open_input();
		bool audio_play();

		int show_videoindex() { return videoindex; }
		int show_sndindex() { return sndindex; }
		std::string show_the_name() { return name; }

		uint8_t 			*audio_buff;
		char 				*filename;
	private:
		const uint32_t 		BUFFER_SIZE;
		int 				videoindex;
    	int 				sndindex;
    	AVFormatContext    *pFormatCtx;
    	AVCodecContext     *sndCodecCtx;
    	AVCodec            *sndCodec;
		AVPacket 			*packet;
		AVFrame 			*frame;
    	SwrContext 	*swr_ctx;
		std::string 		name;
    	snd_pcm_t 			*pcm;
		snd_pcm_hw_params_t	*params; 
};

AudioState::AudioState(char *inputfile)
	:BUFFER_SIZE(192000)
{
	videoindex = -1;
	sndindex = -1;
	pFormatCtx = avformat_alloc_context();	
	packet = (AVPacket*)av_mallocz(sizeof(AVPacket));
	frame = av_frame_alloc();
	sndCodecCtx = NULL;
	sndCodec = NULL;
	swr_ctx = NULL;
	pcm = NULL;
	params = NULL;
	audio_buff = new uint8_t[BUFFER_SIZE];
	name = "this is mp3 player demo!\n";
	filename = inputfile;
}

AudioState::~AudioState()//析构函数
{
	if (audio_buff)
		delete[] audio_buff;

	avcodec_close(sndCodecCtx);
    avformat_close_input(&pFormatCtx);
    snd_pcm_close(pcm);
}

bool AudioState::open_input()
{
	unsigned int val;
    int dir;

	
	if(avformat_open_input(&pFormatCtx, filename, NULL, NULL)!=0)
		return false;
	
	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
		return false;

	av_dump_format(pFormatCtx,0, 0, 0);

	videoindex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, videoindex, -1, NULL, 0); 
    sndindex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO,sndindex, videoindex, NULL, 0);
	
	if(sndindex != -1)
	{
		sndCodecCtx = pFormatCtx->streams[sndindex]->codec;	
		sndCodec = avcodec_find_decoder(sndCodecCtx->codec_id);
		
		if(sndCodec == NULL)
        {
            LOGD("Codec not found");
            return false;
        }

		if(avcodec_open2(sndCodecCtx, sndCodec, NULL) < 0)
            return false;
		
		snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
		snd_pcm_hw_params_alloca(&params);
		snd_pcm_hw_params_any(pcm, params);
		snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
		ffmpeg_fmt_to_alsa_fmt(sndCodecCtx, pcm, params);	
		snd_pcm_hw_params_set_channels(pcm, params, sndCodecCtx->channels);
		val = sndCodecCtx->sample_rate;
		LOGD("sndCodecCtx->sample_rate=%d", sndCodecCtx->sample_rate);
		snd_pcm_hw_params_set_rate_near(pcm, params, &val, &dir);
		snd_pcm_hw_params(pcm, params);
	}

	
	return true;

}

bool AudioState::audio_play()
{
	int got_frame,len2,data_size,ret;

	while( (av_read_frame(pFormatCtx, packet)>=0) )
	{
		if(packet->stream_index != sndindex)	
			continue;
		if((ret=avcodec_decode_audio4(sndCodecCtx, frame, &got_frame, packet)) < 0) //decode data is store in frame
		{
			LOGD("file eof");
            break;
		}

		if(got_frame <= 0) /* No data yet, get more frames */
			continue;
		if(NULL==swr_ctx)
		{
			if(swr_ctx != NULL)	
				swr_free(&swr_ctx);
			LOGD("AV_CH_LAYOUT_STEREO=%d, AV_SAMPLE_FMT_S16=%d, freq=44100", AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);
    		LOGD("frame: channnels=%d, default_layout=%d, format=%d, sample_rate=%d", frame->channels,av_get_default_channel_layout(frame->channels),
 				frame->format, frame->sample_rate);
			swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, 
				AV_SAMPLE_FMT_S16, sndCodecCtx->sample_rate, 
				av_get_default_channel_layout(frame->channels), 
				(enum AVSampleFormat)frame->format, frame->sample_rate, 0, NULL);

			if(swr_ctx == NULL)
			{
				LOGD("swr_ctx == NULL");
			}

			swr_init(swr_ctx);
			
		}
		
		len2 = swr_convert(swr_ctx, &audio_buff, sndCodecCtx->sample_rate,(const uint8_t **)frame->extended_data, frame->nb_samples);

		data_size = av_samples_get_buffer_size(NULL, sndCodecCtx->channels, frame->nb_samples, sndCodecCtx->sample_fmt, 1);

		ret = snd_pcm_writei(pcm, audio_buff, data_size/4); 
		if (ret < 0)
			ret = snd_pcm_recover(pcm, ret, 0);
		
	}	

	return true;
}

void ffmpeg_fmt_to_alsa_fmt(AVCodecContext *pCodecCtx, snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
    switch(pCodecCtx->sample_fmt) {
        case AV_SAMPLE_FMT_U8: //unsigned 8 bits
            snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S8);
            break;
        case AV_SAMPLE_FMT_S16: //signed 16 bits
            LOGD("AV_SAMPLE_FMT_S16");
            //SND_PCM_FORMAT_S16 is ok, not care SND_PCM_FORMAT_S16_LE or SND_PCM_FORMAT_S16_BE
            snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16); //SND_PCM_FORMAT_S16_LE
            break;
        case AV_SAMPLE_FMT_S16P: //signed 16 bits, planar
            LOGD("AV_SAMPLE_FMT_S16P");
            //SND_PCM_FORMAT_S16 is ok, not care SND_PCM_FORMAT_S16_LE or SND_PCM_FORMAT_S16_BE
            snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16);
            break;
        default:
            break;
    }
}

int main(int argc,char *argv[])
{
	AudioState *audio = NULL;
	
	if(argc < 2)
	{
		LOGD("please input the media file!");
		return -1;
	}	
	
	avcodec_register_all();
    //avfilter_register_all();
    av_register_all();

	audio = new AudioState(argv[1]);	

//	LOGD("audio->videoindex = %d audio->sndindex = %d filename=%s",audio->show_videoindex(),audio->show_sndindex(),audio->filename);	
	
	//cout << audio->show_the_name();	

	if(audio->open_input())	
		LOGD("open the media file ok!");		

	if(audio->audio_play())
		LOGD("the music player over!");
	
	if(audio)
		delete audio;
	
	return 0;
}


