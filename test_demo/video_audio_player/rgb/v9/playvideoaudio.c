

int main(int argc, char *argv[])  
{  
    SDL_Event       event;  
    VideoState      *is;  
    is = (VideoState *)av_mallocz(sizeof(VideoState));  
    if(argc < 2)  
    {    
		LOGD("Usage: test <file>")
        exit(1);  
    }  
    // Register all formats and codecs  
    av_register_all();  
    if(SDL_Init(SDL_INIT_TIMER))//初始化定时器  
    {    
		LOGD("Could not initialize SDL - %s",SDL_GetError());
        exit(1);  
    }  
   
    strcpy(is->filename,argv[1]);  
    is->pictq_mutex = SDL_CreateMutex();  
    is->pictq_cond = SDL_CreateCond();  
  
    schedule_refresh(is, 40); //40ms刷新一次 
  
    is->parse_tid = SDL_CreateThread(decode_thread, is); //创建解码线程 
    if(!is->parse_tid)  
    {  
        av_free(is);  
        return -1;  
    }  
    for(;;)  
    {  
        SDL_WaitEvent(&event);  
        switch(event.type)  
        {  
        case FF_QUIT_EVENT:  
        case SDL_QUIT:  
            is->quit = 1;  
            SDL_Quit();  
            exit(0);  
            break;  
        case FF_ALLOC_EVENT:  
            alloc_picture(event.user.data1);  
            break;  
        case FF_REFRESH_EVENT:  
            video_refresh_timer(event.user.data1);  
            break;  
        default:  
            break;  
        }  
    }  
    return 0;  
}  


int decode_thread(void *arg)  //解码线程
{  
  
    VideoState *is = (VideoState *)arg;  
    AVFormatContext *pFormatCtx;  
    AVPacket pkt1, *packet = &pkt1;  
  
    int video_index = -1;  
    int audio_index = -1;  
    int i;  
  
    is->videoStream=-1;  
    is->audioStream=-1;  
  
    global_video_state = is;  
 
  
    // Open video file  
    if(avformat_open_input(&pFormatCtx,is->filename,NULL,NULL)!=0){  //打开多媒体文件
		LOGD("Couldn't open input stream.");
        return -1; // Couldn't open file  
	}
  
    is->pFormatCtx = pFormatCtx;  
  
    // Retrieve stream information  
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){  //获取流信息
		LOGD("Couldn't find stream information.");
        return -1; // Couldn't find stream information  
	}  
  
    // Dump information about file onto standard error 
	LOGD("---------------File Information------------------");  
    av_dump_format(pFormatCtx, 0, is->filename, 0);  //打印媒体文件信息
	LOGD("-------------------------------------------------");  
  
  
    // Find the first video stream  
  
    for(i=0; i<pFormatCtx->nb_streams; i++)  
    {  
        if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO &&  
                video_index < 0)  
        {  
            video_index=i;  
        }  
        if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO &&  
                audio_index < 0)  
        {  
            audio_index=i;  
        }  
    }  
    if(audio_index >= 0)  
    {  
        stream_component_open(is, audio_index);  
    }  
    if(video_index >= 0)  
    {  
        stream_component_open(is, video_index);  
    }  
  
    if(is->videoStream < 0 || is->audioStream < 0)  
    {   
		LOGD("%s: could not open codecs",is->filename);
        goto fail;  
    }  
  
    // main decode loop  
  
    for(;;)  
    {  
        if(is->quit)  
        {  
            break;  
        }  
        // seek stuff goes here  
        if(is->audioq.size > MAX_AUDIOQ_SIZE ||  
                is->videoq.size > MAX_VIDEOQ_SIZE)  
        {  
            SDL_Delay(10);  
            continue;  
        }  
        if(av_read_frame(is->pFormatCtx, packet) < 0)  
        {  
                break;   
        }  
        // Is this a packet from the video stream?  
        if(packet->stream_index == is->videoStream)  
        {  
            packet_queue_put(&is->videoq, packet); //将视频包添加到队列尾部 
        }  
        else if(packet->stream_index == is->audioStream)  
        {  
            packet_queue_put(&is->audioq, packet); // 将音频包添加到队列的尾部
        }  
        else  
        {  
            av_free_packet(packet);  
        }  
    }  
    /* all done - wait for it */  
    while(!is->quit)  
    {  
        SDL_Delay(100);  
    }  
  
fail:  
    {  
        SDL_Event event;  
        event.type = FF_QUIT_EVENT;  
        event.user.data1 = is;  
        SDL_PushEvent(&event);  
    }  
    return 0;  
}

int stream_component_open(VideoState *is, int stream_index)  
{  
    AVFormatContext *pFormatCtx = is->pFormatCtx;  
    AVCodecContext *codecCtx;  
    AVCodec *codec;  
    SDL_AudioSpec wanted_spec, spec;  
  
    if(stream_index < 0 || stream_index >= pFormatCtx->nb_streams)  
    {  
		LOGD("not find the stream");
        return -1;  
    }  
  
    // Get a pointer to the codec context for the video stream  
    codecCtx = pFormatCtx->streams[stream_index]->codec;  
  
    if(codecCtx->codec_type == CODEC_TYPE_AUDIO)  
    {  
#if 0
        // Set audio settings from codec info  
        wanted_spec.freq = codecCtx->sample_rate;  //这里可替换为自定义的初始化，音频
        wanted_spec.format = AUDIO_S16SYS;  
        wanted_spec.channels = codecCtx->channels;  
        wanted_spec.silence = 0;  
        wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;  
        wanted_spec.callback = audio_callback;  
        wanted_spec.userdata = is;  
  
        if(SDL_OpenAudio(&wanted_spec, &spec) < 0) //替换为自定义 
        {  
            fprintf(stderr, "SDL_OpenAudio: %s/n", SDL_GetError());  
            return -1;  
        }  
        is->audio_hw_buf_size = spec.size;  //替换为自定义
#else
	//FIXME
#endif  
    }

    codec = avcodec_find_decoder(codecCtx->codec_id);//找到解码器  
  
    if(!codec || (avcodec_open2(codecCtx, codec,NULL) < 0))//初始化音视频解码器 
    {  
        fprintf(stderr, "Unsupported codec!/n");  
        return -1;  
    }  


  
    switch(codecCtx->codec_type)  
    {  
    case CODEC_TYPE_AUDIO:  
        is->audioStream = stream_index;  
        is->audio_st = pFormatCtx->streams[stream_index];  
        is->audio_buf_size = 0;  
        is->audio_buf_index = 0;  
        memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));  
        packet_queue_init(&is->audioq);//初始化音频队列  
 //       SDL_PauseAudio(0);  // call the call_back function
	/* 解码 + 音频播放*/
//FIXME
        break;  
    case CODEC_TYPE_VIDEO:  
        is->videoStream = stream_index;//保存视频流的索引值  
        is->video_st = pFormatCtx->streams[stream_index];//保存视频流  
  
        is->frame_timer = (double)av_gettime() / 1000000.0;  
        is->frame_last_delay = 40e-3;  
  
        packet_queue_init(&is->videoq);  //初始化视频流队列
        is->video_tid = SDL_CreateThread(video_thread, is);//创建视频线程  
 //       codecCtx->get_buffer = our_get_buffer;  // 在分配解码帧的时候调用
 //       codecCtx->release_buffer = our_release_buffer; //在释放解码帧的时候调用
        break;  
    default:  
        break;  
    }  
    return 0;  
} 

int video_thread(void *arg)  
{  
    VideoState *is = (VideoState *)arg;  
    AVPacket pkt1, *packet = &pkt1;  
    int len1, frameFinished;  
    AVFrame *pFrame;  
    double pts;  
  
    pFrame = avcodec_alloc_frame();  //分配解码帧空间
  
    for(;;)  
    {  
        if(packet_queue_get(&is->videoq, packet, 1) < 0) //出队 
        {  
            // means we quit getting packets  
            break;  
        }  
        pts = 0;  
  
        // Save global pts to be stored in pFrame in first call  
        global_video_pkt_pts = packet->pts;  
        // Decode video frame  
        len1 = avcodec_decode_video(is->video_st->codec, pFrame, &frameFinished,  
                                    packet->data, packet->size);  
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
        pts *= av_q2d(is->video_st->time_base);  
  
        // Did we get a video frame?  
        if(frameFinished)  
        {  
            pts = synchronize_video(is, pFrame, pts);  
#if 0
            if(queue_picture(is, pFrame, pts) < 0)  
            {  
                break;  
            }  
#else
			//FIXME  show to the output device
#endif
        }  
        av_free_packet(packet);  
    }  
    av_free(pFrame);  
    return 0;  
}   

void audio_callback(void *userdata, Uint8 *stream, int len)  
{  
    VideoState *is = (VideoState *)userdata;  
    int len1, audio_size;  
    double pts;  
  
    while(len > 0)  
    {  
        if(is->audio_buf_index >= is->audio_buf_size)  
        {  
            /* We have already sent all our data; get more */  
            audio_size = audio_decode_frame(is, is->audio_buf, sizeof(is->audio_buf), &pts);  
            if(audio_size < 0)  
            {  
                /* If error, output silence */  
                is->audio_buf_size = 1024;  
                memset(is->audio_buf, 0, is->audio_buf_size);  
            }  
            else  
            {  
                is->audio_buf_size = audio_size;  
            }  
            is->audio_buf_index = 0;  
        }  
        len1 = is->audio_buf_size - is->audio_buf_index;  
        if(len1 > len)  
            len1 = len;  
        memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);  
        len -= len1;  
        stream += len1;  
        is->audio_buf_index += len1;  
    }  
} 

int audio_decode_frame(VideoState *is, uint8_t *audio_buf, int buf_size, double *pts_ptr)  
{  
    int len1, data_size, n;  
    AVPacket *pkt = &is->audio_pkt;  
    double pts;  
  
    for(;;)  
    {  
        while(is->audio_pkt_size > 0)  
        {  
            data_size = buf_size;  
            len1 = avcodec_decode_audio2(is->audio_st->codec,  
                                         (int16_t *)audio_buf, &data_size,  
                                         is->audio_pkt_data, is->audio_pkt_size);  
            if(len1 < 0)  
            {  
                /* if error, skip frame */  
                is->audio_pkt_size = 0;  
                break;  
            }  
            is->audio_pkt_data += len1;  
            is->audio_pkt_size -= len1;  
            if(data_size <= 0)  
            {  
                /* No data yet, get more frames */  
                continue;  
            }  
            pts = is->audio_clock;  
            *pts_ptr = pts;  
            n = 2 * is->audio_st->codec->channels;  
            is->audio_clock += (double)data_size /  
                               (double)(n * is->audio_st->codec->sample_rate);  
  
            /* We have data, return it and come back for more later */  
            return data_size;  
        }  
        if(pkt->data)  
            av_free_packet(pkt);  
  
        if(is->quit)  
        {  
            return -1;  
        }  
        /* next packet */  
        if(packet_queue_get(&is->audioq, pkt, 1) < 0)  
        {  
            return -1;  
        }  
        is->audio_pkt_data = pkt->data;  
        is->audio_pkt_size = pkt->size;  
        /* if update, update the audio clock w/pts */  
        if(pkt->pts != AV_NOPTS_VALUE)  
        {  
            is->audio_clock = av_q2d(is->audio_st->time_base)*pkt->pts;  
        }  
    }  
} 


