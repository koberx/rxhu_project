#if 0
#include <libavutil/avutil.h>
#include <libavutil/attributes.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <libswresample/swresample.h>
#include <alsa/asoundlib.h>
#else
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include <stdio.h>

#include "alsa/asoundlib.h"
#include <alsa/asoundlib.h>


#endif

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 

#include <libavutil/avutil.h>
#include <libavutil/attributes.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
#include <libswresample/swresample.h>
#include <alsa/asoundlib.h>
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 
#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__);

typedef struct {
    int videoindex;
    int sndindex;
    AVFormatContext* pFormatCtx;
    AVCodecContext* sndCodecCtx;
    AVCodec* sndCodec;
    SwrContext *swr_ctx;
    snd_pcm_t *pcm;
    DECLARE_ALIGNED(16,uint8_t,audio_buf) [AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
}AudioState;

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

int init_ffmpeg_alsa(AudioState* is, char* filepath)
{
    int i=0;
    int ret;
    //alsa    
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t buffer_size;

    is->sndindex = -1;
    if(NULL == filepath)
    {
        LOGD("input file is NULL");
        return -1;
    }
    avcodec_register_all();
    avfilter_register_all();
    av_register_all();

    is->pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&is->pFormatCtx, filepath, NULL, NULL)!=0)
        return -1;

    if(avformat_find_stream_info(is->pFormatCtx, NULL)<0)
        return -1;
    av_dump_format(is->pFormatCtx,0, 0, 0);
    is->videoindex = av_find_best_stream(is->pFormatCtx, AVMEDIA_TYPE_VIDEO, is->videoindex, -1, NULL, 0); 
    is->sndindex = av_find_best_stream(is->pFormatCtx, AVMEDIA_TYPE_AUDIO,is->sndindex, is->videoindex, NULL, 0);
    LOGD("videoindex=%d, sndindex=%d", is->videoindex, is->sndindex);
    if(is->sndindex != -1)
    {
        is->sndCodecCtx = is->pFormatCtx->streams[is->sndindex]->codec;
        is->sndCodec = avcodec_find_decoder(is->sndCodecCtx->codec_id);
        if(is->sndCodec == NULL)
        {
            LOGD("Codec not found");
            return -1;
        }
        if(avcodec_open2(is->sndCodecCtx, is->sndCodec, NULL) < 0)
            return -1;
        snd_pcm_open(&is->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
        snd_pcm_hw_params_alloca(&params);
        snd_pcm_hw_params_any(is->pcm, params);
        snd_pcm_hw_params_set_access(is->pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        ffmpeg_fmt_to_alsa_fmt(is->sndCodecCtx, is->pcm, params);
        snd_pcm_hw_params_set_channels(is->pcm, params, is->sndCodecCtx->channels);
        val = is->sndCodecCtx->sample_rate;
        LOGD("is->sndCodecCtx->sample_rate=%d", is->sndCodecCtx->sample_rate);
        snd_pcm_hw_params_set_rate_near(is->pcm, params, &val, &dir);    
        snd_pcm_hw_params(is->pcm, params);
    }
    return 0;
}

int main(int argc, char **argv)
{
    int ret;
    FILE* fp; 
    int file_data_size = 0;
    int len2,len3, data_size, got_frame;
    AVPacket *packet = av_mallocz(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();
    AudioState* is = (AudioState*) av_mallocz(sizeof(AudioState));
    uint8_t *out[] = { is->audio_buf };
    if( (ret=init_ffmpeg_alsa(is, argv[1])) != 0)
    {
        LOGD("init_ffmpeg error");
        return -1;
    }
    while( (av_read_frame(is->pFormatCtx, packet)>=0) )
    { 
        if(packet->stream_index != is->sndindex)
            continue;
        if((ret=avcodec_decode_audio4(is->sndCodecCtx, frame, &got_frame, packet)) < 0) //decode data is store in frame
        {
            LOGD("file eof");
            break;
        }

        if(got_frame <= 0) /* No data yet, get more frames */
            continue;
        if(NULL==is->swr_ctx)
        {
            if(is->swr_ctx != NULL)
                swr_free(&is->swr_ctx);
            LOGD("AV_CH_LAYOUT_STEREO=%d, AV_SAMPLE_FMT_S16=%d, freq=44100", AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16);
            LOGD("frame: channnels=%d, default_layout=%d, format=%d, sample_rate=%d", frame->channels,av_get_default_channel_layout(frame->channels), frame->format, frame->sample_rate);
            is->swr_ctx = swr_alloc_set_opts(NULL, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, is->sndCodecCtx->sample_rate, av_get_default_channel_layout(frame->channels), frame->format, frame->sample_rate, 0, NULL);
            if(is->swr_ctx == NULL)
            {
                LOGD("swr_ctx == NULL");
            }
            swr_init(is->swr_ctx);
        }
        //LOGD("next swr_convert");
        len2 = swr_convert(is->swr_ctx, out, is->sndCodecCtx->sample_rate,(const uint8_t **)frame->extended_data, frame->nb_samples);
        data_size = av_samples_get_buffer_size(NULL, is->sndCodecCtx->channels, frame->nb_samples, is->sndCodecCtx->sample_fmt, 1);
        //LOGD("data_size 111 = %d", data_size);
        //data_size = len2 * 2 * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);         -->通过av_sample_get_buffer_size的值与这儿计算的值是一样的
        //LOGD("data_size 222 = %d", data_size);
        //LOGD("next pcm write, data_size=%d", data_size);
        ret = snd_pcm_writei(is->pcm, is->audio_buf, data_size/4);                   
        if (ret < 0)
            ret = snd_pcm_recover(is->pcm, ret, 0); //ALSA lib pcm.c:7843:(snd_pcm_recover) underrun occurred
        //LOGD("pcm write over, data_size=%d", data_size/4);
    }
    av_free_packet(packet);
    av_free(frame);
    avcodec_close(is->sndCodecCtx);
    avformat_close_input(&is->pFormatCtx);
    snd_pcm_close(is->pcm);
    fclose(fp);
    return 0;
}
