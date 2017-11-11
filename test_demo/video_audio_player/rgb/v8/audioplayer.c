#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include <stdio.h>

#include "alsa/asoundlib.h"
#include <alsa/asoundlib.h>
#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__);
snd_pcm_t* handle=NULL; //PCI设备句柄
snd_pcm_hw_params_t* params=NULL;//硬件信息和PCM流配置
#define MAX_AUDIO_FRAME_SIZE  192000

#define SAMPLE_RATE 22050 //44100
#define CHANNELS 2  
#define PCM_DEVICE "plughw:0,0" 

void set_volume(long volume);

int main(int argc,char *argv[])
{
	AVFormatContext *pFormatCtx;   //格式上下文结构体  
	if(argc <= 2)
	{
		LOGD("please input the media file and the volume");
		return -1;
	}

	char *filepath = argv[1];// input

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

	int             i, audio_index;
	AVCodecContext  *pCodecCtx_audio;    //codec上下文  
    AVCodec         *pCodec_audio;       //codec  

	audio_index=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)                //遍历多媒体文件中的每一个流，判断是否为视频。  
    {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
            audio_index=i;
            break;
        }
    }

	if(audio_index==-1){
        LOGD("Didn't find a video stream.");
        return -1;
    }

	pCodecCtx_audio = pFormatCtx->streams[audio_index]->codec; //codec上下文指定到格式上下文中的codec  
    pCodec_audio = avcodec_find_decoder(pCodecCtx_audio->codec_id); //找到一个codec，必须先调用av_register_all（）  

    if(pCodec_audio == NULL){
        LOGD("Codec not found.");
        return -1;
    }

	if(avcodec_open2(pCodecCtx_audio, pCodec_audio,NULL)<0){ //初始化一个视音频编解码器的AVCodecContext  
        LOGD("Could not open codec.");
        return -1;
    }
	long volume = atoi(argv[2]);
	set_pcm();
	//set_volume(volume);

	//创建packet,用于存储解码前的数据
    AVPacket *packet = malloc(sizeof(AVPacket));
    av_init_packet(packet);

	//设置转码后输出相关参数
    //采样的布局方式
    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    //采样个数
    int out_nb_samples = 1024;
    //采样格式
    enum AVSampleFormat  sample_fmt = AV_SAMPLE_FMT_S16;
    //采样率
    int out_sample_rate = 44100;
    //int out_sample_rate = SAMPLE_RATE;
    //通道数
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);

    //创建buffer
    int buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, sample_fmt, 1);

	 //注意要用av_malloc
    uint8_t *buffer = av_malloc(MAX_AUDIO_FRAME_SIZE * 2);

	 //创建Frame，用于存储解码后的数据
    AVFrame *frame = av_frame_alloc();
    LOGD("Bitrate :\t %3lld", pFormatCtx->bit_rate);
    LOGD("Decoder Name :\t %s", pCodecCtx_audio->codec->long_name);
    LOGD("Cannels:\t %d", pCodecCtx_audio->channels);
    LOGD("Sample per Second:\t %d", pCodecCtx_audio->sample_rate);

	uint32_t ret,len = 0;	
	int got_audio;
	
	int64_t in_channel_layout = av_get_default_channel_layout(pCodecCtx_audio->channels);
	//打开转码器
    struct SwrContext *convert_ctx = swr_alloc();

	//设置转码参数
    convert_ctx = swr_alloc_set_opts(convert_ctx, out_channel_layout, sample_fmt, out_sample_rate, \
            in_channel_layout, pCodecCtx_audio->sample_fmt, pCodecCtx_audio->sample_rate, 0, NULL);

	//初始化转码器
    swr_init(convert_ctx);

	int index = 0;

	while (av_read_frame(pFormatCtx, packet) >= 0) {
//		LOGD("index : %d", index);
		if (packet->stream_index == audio_index) {
			//解码声音
            if (avcodec_decode_audio4(pCodecCtx_audio, frame, &got_audio, packet) < 0) {
                LOGD("decode error");
                return -1;
            }
			
			if (got_audio > 0) {
                //转码
                swr_convert(convert_ctx, &buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)frame->data, frame->nb_samples);

                LOGD("index: %5d\t pts:%10lld\t packet size:%d\n", index, packet->pts, packet->size);

 //               fwrite(buffer, 1, buffer_size, file);
				if(snd_pcm_writei (handle, buffer, buffer_size / 3) < 0)
				{
					LOGD("can't send the pcm to the sound card!");
					return -1;
				} 
            }	
//			mySleep(0,100000);
		}
		index ++;
        av_free_packet(packet);
	}

	swr_free(&convert_ctx);

	LOGD("The Pthread safe exit");

    return 0;

}

int set_pcm(void)
{
    int rc; 
    int dir=0;
    int rate = 44100; /* 采样频率 44.1KHz*/
//    int rate = SAMPLE_RATE; /* 采样频率 44.1KHz*/
    //int format = SND_PCM_FORMAT_S16_LE; /* 量化位数 16 */
    int channels = 2; /* 声道数 2 */
    
    rc=snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  //  rc=snd_pcm_open(&handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    if(rc<0)
    {
      LOGD("\nopen PCM device failed:");
      exit(1);
    }
    snd_pcm_hw_params_alloca(&params); //分配params结构体

    rc=snd_pcm_hw_params_any(handle, params);//初始化params
    if(rc<0)
    {
      LOGD("\nsnd_pcm_hw_params_any:");
      exit(1);
    }
    rc=snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED); //初始化访问权限
    if(rc<0)
    {
      LOGD("\nsed_pcm_hw_set_access:");
      exit(1);
    }
    rc=snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE); //设置16位采样精度 
    if(rc<0)
    {
      LOGD("snd_pcm_hw_params_set_format failed:");
      exit(1);
    } 
    rc=snd_pcm_hw_params_set_channels(handle, params, channels); //设置声道,1表示单声>道，2表示立体声
    if(rc<0)
    {
      LOGD("\nsnd_pcm_hw_params_set_channels:");
      exit(1);
    }
    rc=snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir); //设置>频率
    if(rc<0)
    {
      LOGD("\nsnd_pcm_hw_params_set_rate_near:");
      exit(1);
    } 
    rc = snd_pcm_hw_params(handle, params);
    if(rc<0)
    {
      LOGD("\nsnd_pcm_hw_params: ");
      exit(1);
    } 

//   	snd_pcm_hw_params_free (params);     
   if ((rc = snd_pcm_prepare (handle)) < 0) {     
      printf("cannot prepare audio interface for use (%s)",     
      snd_strerror (rc));     
      exit(1);     
   }     
	
	printf("[%s][%d] : the pcm device set ok \n",__FUNCTION__,__LINE__);
    return 0; 
}

void set_volume(long volume)
{
  snd_mixer_t *mixerFd;
  snd_mixer_elem_t *elem;
  long minVolume = 0,maxVolume = 100;
  int result;
  // 打开混音器
   if ((result = snd_mixer_open( &mixerFd, 0)) < 0)
   {
        LOGD("snd_mixer_open error!\n");
        mixerFd = NULL;
   }
  // Attach an HCTL to an opened mixer
   if ((result = snd_mixer_attach( mixerFd, "default")) < 0)
   {
        LOGD("snd_mixer_attach error!\n");
        snd_mixer_close(mixerFd);
        mixerFd = NULL;
   }
  // 注册混音器
   if ((result = snd_mixer_selem_register( mixerFd, NULL, NULL)) < 0)
  {
        LOGD("snd_mixer_selem_register error!\n");
        snd_mixer_close(mixerFd);
        mixerFd = NULL;
  }
  // 加载混音器
  if ((result = snd_mixer_load( mixerFd)) < 0)
  {
        LOGD("snd_mixer_load error!\n");
        snd_mixer_close(mixerFd);
        mixerFd = NULL;
  }

   // 遍历混音器元素
    for(elem=snd_mixer_first_elem(mixerFd); elem; elem=snd_mixer_elem_next(elem))
    {
        if (snd_mixer_elem_get_type(elem) == SND_MIXER_ELEM_SIMPLE &&
             snd_mixer_selem_is_active(elem)) // 找到可以用的, 激活的elem
        {
            snd_mixer_selem_get_playback_volume_range(elem, &minVolume, &maxVolume);
            snd_mixer_selem_set_playback_volume_all(elem, volume);
        }
    }
    snd_mixer_close(mixerFd);
}
