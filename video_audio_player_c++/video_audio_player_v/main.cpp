
#include "media.h"

int main(int argc,char *argv[])
{
	MediaState *media = NULL;

	if(argc < 2)
    {
        LOGD("please input the media file!");
        return -1;
    }

    avcodec_register_all();
    //avfilter_register_all();
    av_register_all();

	//media = new VideoState(argv[1]); 	
	media = new AudioState(argv[1]); 	

	if(!media->open_input())
	{
		LOGD("open the media file failed!");
		return -1;
	}

	if(!media->dev_init())
	{
		LOGD("the device init failed!");
		return -1;
	}

	if(!media->dev_play())
	{
		LOGD("the player is abnormal!");
		return -1;
	}

	LOGD("player is over!");

	delete media;
	
	return 0;
}
