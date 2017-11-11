#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include <stdio.h>
#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"

#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__);
#define FRAMES_NEED 100
#define CONVERT_WITH 1280
#define CONVERT_HIGH 720

int main(int argc,char* argv[])
{
	AVFormatContext *pFormatCtx;   //格式上下文结构体  
    int             i, videoindex;  
    AVCodecContext  *pCodecCtx;    //codec上下文  
    AVCodec         *pCodec;       //codec  
    AVFrame *pFrame;               //Frame结构体  
    AVFrame *pFrameYUV;            //Frame结构体  
    uint8_t *out_buffer;             
    AVPacket *packet;              //packet结构体  
    int y_size,y_size_align;  
    int ret, got_picture;  
    unsigned int frame_num = 0;  
    struct SwsContext *img_convert_ctx;  //图像格式转化上下文 

	char *filepath = argv[1];// input
	
	FILE *fp_frame = fopen("output.yuv","wb+");  //output    
    FILE *fp_yuv=fopen("output_sws.yuv","wb+");      

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
	
	videoindex=-1; 
	for(i=0; i<pFormatCtx->nb_streams; i++)                //遍历多媒体文件中的每一个流，判断是否为视频。  
	{
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){  
			videoindex=i;  
			break;
		}
	}

	if(videoindex==-1){  
        LOGD("Didn't find a video stream.");  
        return -1;  
    }  

	pCodecCtx=pFormatCtx->streams[videoindex]->codec; //codec上下文指定到格式上下文中的codec  
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id); //找到一个codec，必须先调用av_register_all（）  

	 if(pCodec==NULL){  
        LOGD("Codec not found.");  
        return -1;  
    }  

	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){ //初始化一个视音频编解码器的AVCodecContext  
        LOGD("Could not open codec.");  
        return -1;  
    }  

	pFrame=av_frame_alloc();  //原始帧  
    pFrameYUV=av_frame_alloc();//YUV帧  
			/* 宏AV_PIX_FMT_YUV420P 代替宏PIX_FMT_YUV420P */
//	out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height)); 	
	out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, CONVERT_WITH, CONVERT_HIGH)); 	
	/*  将pFrameYUV和out_buffer联系起来（pFrame指向一段内存）；宏AV_PIX_FMT_YUV420P 代替宏PIX_FMT_YUV420P   */
//	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height); 
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, CONVERT_WITH, CONVERT_HIGH); 

	packet=(AVPacket *)av_malloc(sizeof(AVPacket)); //开空间  

	LOGD("--------------- File Information ----------------");
	av_dump_format(pFormatCtx,0,filepath,0);//调试函数，输出文件的音、视频流的基本信息  
	LOGD("-------------------------------------------------"); 
	
	/*  初始化SWS，图片格式装换上下文//经过验证，输出YUV不需要格式转换，但需要调整尺寸  */	
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 

	while(av_read_frame(pFormatCtx, packet)>=0){       //读取码流中的音频若干帧或者视频一帧,作为packet 
		if(packet->stream_index==videoindex){          //如果是视频 
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if(ret < 0){  
				LOGD("Decode Error.");
				return -1;
			}
			if(got_picture)
			{/*	将输出结果转化成YUV，输出YUV不需要格式转换,但是需要调整尺寸，pFrame中的图像数据的对齐方式可能是按64对齐的。  */
//				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
//					 pFrameYUV->data, pFrameYUV->linesize);
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
					 pFrameYUV->data, pFrameYUV->linesize);
				y_size=CONVERT_WITH*CONVERT_HIGH; 
				y_size_align=((CONVERT_WITH+63)/64*64)*CONVERT_HIGH; 
//				y_size_align=CONVERT_WITH*CONVERT_HIGH;  				

				fwrite(pFrame->data[0],1,y_size_align,fp_frame);    //Y   
                fwrite(pFrame->data[1],1,y_size_align/4,fp_frame);  //U  
                fwrite(pFrame->data[2],1,y_size_align/4,fp_frame);  //V  
#if 0
				fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y   
                fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U  
                fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V 
#else
				fwrite(pFrameYUV->data[0],y_size,1,fp_yuv);    //Y   
                fwrite(pFrameYUV->data[1],y_size/4,1,fp_yuv);  //U  
                fwrite(pFrameYUV->data[2],y_size/4,1,fp_yuv);  //V 

#endif
				frame_num++; 
				LOGD("pCodecCtx->width =%d , pCodecCtx->height = %d",pCodecCtx->width,pCodecCtx->height);	
#ifdef FRAMES_NEED  
                if(frame_num == FRAMES_NEED){  
                    LOGD("%d frames done!",frame_num);  
                    break;
				}  
#endif  
				if(frame_num%100 == 0)  
                    LOGD("%d frames done!",frame_num);
			}
			
		}
		av_free_packet(packet);
	}

	sws_freeContext(img_convert_ctx);  
  
    fclose(fp_yuv);  
    fclose(fp_frame);  
    av_frame_free(&pFrameYUV);  
    av_frame_free(&pFrame);  
    avcodec_close(pCodecCtx);  
    avformat_close_input(&pFormatCtx);  

	return 0;
}
