#include "V4l2Output.h"
#include "V4l2Device.h"
#include "logger.h"
#include "V4l2Access.h"
#include "V4l2MmapDevice.h"
#include "V4l2ReadWriteDevice.h"

typedef struct{
    unsigned char Blue;
    unsigned char Green;
    unsigned char Red;
    unsigned char Reserved;
} __attribute__((packed)) RgbQuad;

int main(void)
{
	int LogLevel = INFO;
	unsigned char buffer[800 * 480 * 4];
	unsigned int bufferSize = 800 * 480 * 4;
	struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

	V4L2DeviceParameters param("/dev/video1", V4L2_PIX_FMT_RGB32, 800, 480, 30, 4);
	V4l2Output* videoOutput = V4l2Output::create(param, V4l2Access::IOTYPE_MMAP);

	int i,j;
	unsigned char *tmp = buffer;
	RgbQuad rgb;
	rgb.Blue = 255;
	rgb.Green = 0;
	rgb.Red = 0;
	rgb.Reserved = 255;
	 for(i = 0; i < 480; i++)
    {
        for(j = 0; j < 800; j++)
        {
            *tmp = rgb.Blue;
            tmp++;
            *tmp = rgb.Green;
            tmp++;
            *tmp = rgb.Red;
            tmp++;
            *tmp = rgb.Reserved;
            tmp++;
        }

    }

	while(videoOutput->isWritable(&timeout) == 1)
	{
		videoOutput->write((char *)buffer,bufferSize);
	}	
	
	delete videoOutput;

	return 0;
}
