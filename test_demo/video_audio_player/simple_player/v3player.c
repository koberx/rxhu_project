
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include <stdio.h>
#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"  

  
//Refresh  
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)  
  
int thread_exit=0;  
//Thread  
int sfp_refresh_thread(void *opaque)  
{  
    SDL_Event event;  
    while (thread_exit==0) {  
        event.type = SFM_REFRESH_EVENT;  
        SDL_PushEvent(&event);  
        //Wait 40 ms  
        SDL_Delay(20);  
    }  
    return 0;  
}  
  
  
int main(int argc, char* argv[])  
{  
    AVFormatContext *pFormatCtx;  
    int             i, videoindex;  
    AVCodecContext  *pCodecCtx;  
    AVCodec         *pCodec;  
    AVFrame *pFrame,*pFrameYUV;  
    AVPacket *packet;  
    struct SwsContext *img_convert_ctx;  
    //SDL  
    int ret, got_picture;  
    int screen_w=0,screen_h=0;  
    SDL_Surface *screen;   
    SDL_Overlay *bmp;   
    SDL_Rect rect;  
    SDL_Thread *video_tid;  
    SDL_Event event;  
  
//    char filepath[]="bigbuckbunny_480x272.h265";  
    char *filepath=argv[1];  
    av_register_all();  
    avformat_network_init();  
    pFormatCtx = avformat_alloc_context();  
      
    if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){  
        printf("Couldn't open input stream.\n");  
        return -1;  
    }  
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){  
        printf("Couldn't find stream information.\n");  
        return -1;  
    }  
    videoindex=-1;  
    for(i=0; i<pFormatCtx->nb_streams; i++)   
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){  
            videoindex=i;  
            break;  
        }  
    if(videoindex==-1){  
        printf("Didn't find a video stream.\n");  
        return -1;  
    }  
    pCodecCtx=pFormatCtx->streams[videoindex]->codec;  
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);  
    if(pCodec==NULL)  
    {  
        printf("Codec not found.\n");  
        return -1;  
    }  
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)  
    {  
        printf("Could not open codec.\n");  
        return -1;  
    }  
  
    pFrame=av_frame_alloc();  
    pFrameYUV=av_frame_alloc();  
    //uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));  
    //avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);  
//------------SDL----------------  
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {    
        printf( "Could not initialize SDL - %s\n", SDL_GetError());   
        return -1;  
    }   
  
      
    screen_w = pCodecCtx->width;  
    screen_h = pCodecCtx->height;  
    screen = SDL_SetVideoMode(screen_w, screen_h, 0,0);  
  
    if(!screen) {    
        printf("SDL: could not set video mode - exiting:%s\n",SDL_GetError());    
        return -1;  
    }  
      
    bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY, screen);   
      
    rect.x = 0;      
    rect.y = 0;      
    rect.w = screen_w;      
    rect.h = screen_h;    
  
    packet=(AVPacket *)av_malloc(sizeof(AVPacket));  
  
    printf("---------------File Information------------------\n");  
    av_dump_format(pFormatCtx,0,filepath,0);  
    printf("-------------------------------------------------\n");  
      
      
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);   
    //--------------  
    video_tid = SDL_CreateThread(sfp_refresh_thread,NULL);  
    //  
    SDL_WM_SetCaption("Simple FFmpeg Player (SDL Update)",NULL);  
  
    //Event Loop  
      
    for (;;) {  
        //Wait  
        SDL_WaitEvent(&event);  
        if(event.type==SFM_REFRESH_EVENT){  
            //------------------------------  
            if(av_read_frame(pFormatCtx, packet)>=0){  
                if(packet->stream_index==videoindex){  
                    ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);  
                    if(ret < 0){  
                        printf("Decode Error.\n");  
                        return -1;  
                    }  
                    if(got_picture){  
  
                        SDL_LockYUVOverlay(bmp);  
                        pFrameYUV->data[0]=bmp->pixels[0];  
                        pFrameYUV->data[1]=bmp->pixels[2];  
                        pFrameYUV->data[2]=bmp->pixels[1];       
                        pFrameYUV->linesize[0]=bmp->pitches[0];  
                        pFrameYUV->linesize[1]=bmp->pitches[2];     
                        pFrameYUV->linesize[2]=bmp->pitches[1];  
                        sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);  
  
                        SDL_UnlockYUVOverlay(bmp);   
                          
                        SDL_DisplayYUVOverlay(bmp, &rect);   
  
                    }  
                }  
                av_free_packet(packet);  
            }else{  
                //Exit Thread  
                thread_exit=1;  
                break;  
            }  
        }  
  
    }  
      
    SDL_Quit();  
  
    sws_freeContext(img_convert_ctx);  
  
    //--------------  
    //av_free(out_buffer);  
    av_free(pFrameYUV);  
    avcodec_close(pCodecCtx);  
    avformat_close_input(&pFormatCtx);  
  
    return 0;  
}  

