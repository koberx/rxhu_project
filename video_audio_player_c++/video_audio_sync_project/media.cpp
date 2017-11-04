
#include "media.h"

MediaState::MediaState(char *inputfile)
	:filename(inputfile)
{
	pFormatCtx = avformat_alloc_context(); 
	//video
	video_fd = -1;
	video_index = -1;
	v4lCodecCtx = NULL;
	v4lCodec = NULL;
	pFrame = av_frame_alloc(); 
    pFrameYUV = av_frame_alloc();
	packet=	(AVPacket *)av_malloc(sizeof(AVPacket));
	video_out_buffer = NULL;
	img_convert_ctx = NULL;
	addinfo = new V4L2_MAMP_ADDR_INFO_S[6];
	addcnt = 1;
    memset(addinfo, 0, sizeof(addinfo));
	
	//audio
	audio_index = -1;
	pCodecCtx_audio = NULL;
	pCodec_audio = NULL;
	audio_packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	audio_frame = av_frame_alloc();
	convert_ctx = swr_alloc();
	
	out_channel_layout = AV_CH_LAYOUT_STEREO;
	in_channel_layout = 0;
	out_nb_samples = 1024;
	sample_fmt = AV_SAMPLE_FMT_S16;
	out_sample_rate = 44100;
	out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, sample_fmt, 1);
	buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE);
	
	pcm = NULL;
	params = NULL;
}

MediaState::~MediaState()
{
	av_free(packet);
	delete[] addinfo;
	av_free(audio_packet);
	av_free(buffer);	
}

bool MediaState::open_input()
{
	if(avformat_open_input(&pFormatCtx,filename,NULL,NULL)!=0){
		LOGD("Couldn't open input stream.");         
		return false;
	}

	if(avformat_find_stream_info(pFormatCtx,NULL)<0){// find stream 
		LOGD("Couldn't find stream information.");          
		return false; 
	}

	
	video_index = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, video_index, -1, NULL, 0); 
	audio_index = av_find_best_stream(pFormatCtx,AVMEDIA_TYPE_AUDIO,audio_index,video_index,NULL,0);
	LOGD("video_index = %d audio_index = %d",video_index,audio_index);
	
	if(video_index >= 0){
	//video	
		v4lCodecCtx = pFormatCtx->streams[video_index]->codec;
		v4lCodec = avcodec_find_decoder(v4lCodecCtx->codec_id);

	

		if(v4lCodec == NULL){  
        	LOGD("Codec not found.");  
        	return false;  
    	}  

		if(avcodec_open2(v4lCodecCtx, v4lCodec,NULL)<0){ 
        	LOGD("Could not open codec.");  
        	return false;  
    	} 

		video_out_buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_RGB32, CONVERT_WITH, CONVERT_HIGH));
		avpicture_fill((AVPicture *)pFrameYUV, video_out_buffer, PIX_FMT_RGB32, CONVERT_WITH, CONVERT_HIGH); 

		img_convert_ctx = sws_getContext(v4lCodecCtx->width, v4lCodecCtx->height, v4lCodecCtx->pix_fmt,
			CONVERT_WITH, CONVERT_HIGH, PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL); 

	}
	
	if(audio_index >= 0)
	{
		pCodecCtx_audio = pFormatCtx->streams[audio_index]->codec;
		pCodec_audio = avcodec_find_decoder(pCodecCtx_audio->codec_id);
		
		if(pCodec_audio == NULL){
        	LOGD("Codec not found.");
        	return false;
    	}
		
		in_channel_layout = av_get_default_channel_layout(pCodecCtx_audio->channels);
		
		if(avcodec_open2(pCodecCtx_audio, pCodec_audio,NULL)<0){ 
        	LOGD("Could not open codec.");
        	return false;
    	}

		//设置转码参数
		convert_ctx = swr_alloc_set_opts(convert_ctx, out_channel_layout, sample_fmt, out_sample_rate, \
            	in_channel_layout, pCodecCtx_audio->sample_fmt, pCodecCtx_audio->sample_rate, 0, NULL);

		//初始化转码器
   		swr_init(convert_ctx);
		
		LOGD("Bitrate :\t %3d", pFormatCtx->bit_rate);
    	LOGD("Decoder Name :\t %s", pCodecCtx_audio->codec->long_name);
    	LOGD("Cannels:\t %d", pCodecCtx_audio->channels);
    	LOGD("Sample per Second:\t %d", pCodecCtx_audio->sample_rate);
		
	}

	LOGD("-----------------media file info-----------------");
	av_dump_format(pFormatCtx,0, 0, 0);
	LOGD("-------------------------------------------------");

	return true;
	
}

