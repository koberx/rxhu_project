#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__);

/* schedule a video refresh in 'delay' ms */  
static void schedule_refresh(VideoState *is, int delay)  
{  
    SDL_AddTimer(delay, sdl_refresh_timer_cb, is);  
}  

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


int decode_thread(void *arg)  
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
    if(avformat_open_input(&pFormatCtx,is->filename,NULL,NULL)!=0){  
		LOGD("Couldn't open input stream.");
        return -1; // Couldn't open file  
	}
  
    is->pFormatCtx = pFormatCtx;  
  
    // Retrieve stream information  
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){  
		LOGD("Couldn't find stream information.");
        return -1; // Couldn't find stream information  
	}  
  
    // Dump information about file onto standard error 
	LOGD("---------------File Information------------------");  
    av_dump_format(pFormatCtx, 0, is->filename, 0);  
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

    codec = avcodec_find_decoder(codecCtx->codec_id);  
  
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
 //       SDL_PauseAudio(0);  
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
        codecCtx->get_buffer = our_get_buffer;  // 在分配解码帧的时候调用
        codecCtx->release_buffer = our_release_buffer; //在释放解码帧的时候调用
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
  
    pFrame = avcodec_alloc_frame();  //分配解码帧
  
    for(;;)  
    {  
        if(packet_queue_get(&is->videoq, packet, 1) < 0)  
        {  
            // means we quit getting packets  
            break;  
        }  
        pts = 0;  
  
        // Save global pts to be stored in pFrame in first call  
        global_video_pkt_pts = packet->pts;  
        // Decode video frame  
        len1 = avcodec_decode_video2(is->video_st->codec, pFrame, &frameFinished, packet); 
   

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
            if(queue_picture(is, pFrame, pts) < 0)  
            {  
                break;  
            }  
        }  
        av_free_packet(packet);  
    }  
    av_free(pFrame);  
    return 0;  
} 

double synchronize_video(VideoState *is, AVFrame *src_frame, double pts)  
{  
    double frame_delay;  
  
    if(pts != 0)  
    {  
        /* if we have pts, set video clock to it */  
        is->video_clock = pts;  
    }  
    else  
    {  
        /* if we aren't given a pts, set it to the clock */  
        pts = is->video_clock;  
    }  
    /* update the video clock */  
    frame_delay = av_q2d(is->video_st->codec->time_base);  
    /* if we are repeating a frame, adjust clock accordingly */  
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);  
    is->video_clock += frame_delay;  
    return pts;  
}  

void alloc_picture(void *userdata)  
{  
    VideoState *is = (VideoState *)userdata;  
    VideoPicture *vp;  
  
    vp = &is->pictq[is->pictq_windex];  
    if(vp->bmp)  
    {  
        // we already have one make another, bigger/smaller  
        SDL_FreeYUVOverlay(vp->bmp);  
    }  
    // Allocate a place to put our YUV image on that screen  
    vp->bmp = SDL_CreateYUVOverlay(is->video_st->codec->width,  
                                   is->video_st->codec->height,  
                                   SDL_YV12_OVERLAY,  
                                   screen);  
    vp->width = is->video_st->codec->width;  
    vp->height = is->video_st->codec->height;  
  
    SDL_LockMutex(is->pictq_mutex);  
    vp->allocated = 1;  
    SDL_CondSignal(is->pictq_cond);  
    SDL_UnlockMutex(is->pictq_mutex);  
}  
  
int queue_picture(VideoState *is, AVFrame *pFrame, double pts)  
{  

    /* wait until we have space for a new pic */  
    SDL_LockMutex(is->pictq_mutex);  
    while(is->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE &&  
            !is->quit)  
    {  
        SDL_CondWait(is->pictq_cond, is->pictq_mutex);  
    }  
    SDL_UnlockMutex(is->pictq_mutex);  
  
    if(is->quit)  
        return -1;  
  
 
  
    /* allocate or resize the buffer! */  
    if(!vp->bmp ||  
            vp->width != is->video_st->codec->width ||  
            vp->height != is->video_st->codec->height)  
    {  
        SDL_Event event;  
  
        vp->allocated = 0;  
        /* we have to do it in the main thread */  
        event.type = FF_ALLOC_EVENT;  
        event.user.data1 = is;  
        SDL_PushEvent(&event);  
  
        /* wait until we have a picture allocated */  
        SDL_LockMutex(is->pictq_mutex);  
        while(!vp->allocated && !is->quit)  
        {  
            SDL_CondWait(is->pictq_cond, is->pictq_mutex);  
        }  
        SDL_UnlockMutex(is->pictq_mutex);  
        if(is->quit)  
        {  
            return -1;  
        }  
    }  
    /* We have a place to put our picture on the queue */  
    /* If we are skipping a frame, do we set this to null 
    but still return vp->allocated = 1? */  
    static struct SwsContext *img_convert_ctx;  
    if (img_convert_ctx == NULL)  
    {  
        img_convert_ctx = sws_getContext(is->video_st->codec->width, is->video_st->codec->height,  
                                         is->video_st->codec->pix_fmt,  
                                         is->video_st->codec->width, is->video_st->codec->height,  
                                         PIX_FMT_YUV420P,  
                                         SWS_BICUBIC, NULL, NULL, NULL);  
        if (img_convert_ctx == NULL)  
        {  
            fprintf(stderr, "Cannot initialize the conversion context/n");  
            exit(1);  
        }  
    }  
  
    if(vp->bmp)  
    {  
        SDL_LockYUVOverlay(vp->bmp);  
  
        //dst_pix_fmt = PIX_FMT_YUV420P;  
        /* point pict at the queue */  
  
        pict.data[0] = vp->bmp->pixels[0];  
        pict.data[1] = vp->bmp->pixels[2];  
        pict.data[2] = vp->bmp->pixels[1];  
  
        pict.linesize[0] = vp->bmp->pitches[0];  
        pict.linesize[1] = vp->bmp->pitches[2];  
        pict.linesize[2] = vp->bmp->pitches[1];  
  
        // Convert the image into YUV format that SDL uses  
        /*img_convert(&pict, dst_pix_fmt, 
                    (AVPicture *)pFrame, is->video_st->codec->pix_fmt, 
                    is->video_st->codec->width, is->video_st->codec->height);*/  
        sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize,  
                          0, is->video_st->codec->height, pict.data, pict.linesize);  
        SDL_UnlockYUVOverlay(vp->bmp);  
        vp->pts = pts;  
  
        /* now we inform our display thread that we have a pic ready */  
        if(++is->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE)  
        {  
            is->pictq_windex = 0;  
        }  
        SDL_LockMutex(is->pictq_mutex);  
        is->pictq_size++;  
        SDL_UnlockMutex(is->pictq_mutex);  
    }  
    return 0;  
} 




