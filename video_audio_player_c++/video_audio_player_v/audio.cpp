#include "media.h"


AudioState::AudioState(char *inputfile)
	:MediaState(inputfile)
{
	
}

AudioState::~AudioState()
{
	swr_free(&convert_ctx);
	avcodec_close(pCodecCtx_audio);
    avformat_close_input(&pFormatCtx);
    snd_pcm_close(pcm);
}

bool AudioState::media_dev_init()
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

bool AudioState::media_play()
{
	int index = 0,got_audio;
	while (av_read_frame(pFormatCtx, audio_packet) >= 0) {
		if (audio_packet->stream_index == audio_index){
			//解码声音
            if (avcodec_decode_audio4(pCodecCtx_audio, audio_frame, &got_audio, audio_packet) < 0) {
                LOGD("decode error");
                return false;
            }

			if (got_audio > 0) {
				//转码
                swr_convert(convert_ctx, &buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)audio_frame->data, audio_frame->nb_samples);
				LOGD("index: %5d\t pts:%10lld\t packet size:%d\n", index, audio_packet->pts, audio_packet->size);
				if(snd_pcm_writei (pcm, buffer, buffer_size / 3) < 0){
					LOGD("can't send the pcm to the sound card!");
					return false;
				}
			}
		}
		index++;
		av_free_packet(packet);
	}

	return true;
}



