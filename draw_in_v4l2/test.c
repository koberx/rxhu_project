
#include "common.h"



#define WIDTH 	800
#define HEIGHT	480

#define FBDEVICE "/dev/fb0"
#define	VIDEO_DEVICE "/dev/video1" 

//static unsigned char pic_red[800 * 480 * 4];

int main(void)
{

	int video_fd = -1;
	int ret;
	int data_x = 0,data_y = 0;
	unsigned int mode = 0;
	int quit_pressed = 0;

	video_fd = open(VIDEO_DEVICE, O_RDWR);// 打开设备
    if (video_fd < 0)
    {
        LOGD("open the video device error!");
		return -1;
    }
	V4L2_MAMP_ADDR_INFO_S addinfo[6];	
	unsigned int addcnt = 1;
	memset(addinfo, 0, sizeof(addinfo));
	
	if (__v4l2_alpha_config(video_fd) != 0) ;    
	{
		LOGD("__v4l2_alpha_config error!");
//		close(video_fd);	
//		return -1;
	}

	if (__v4l2_mmap(video_fd, addinfo, &addcnt) != 0)
	{
        LOGD("__v4l2_mmap error!");
        close(video_fd);
        return -1;
    }

#if 0
	RgbQuad rgb;
	rgb.Blue = 238;
	rgb.Green = 245;
	rgb.Red = 255;
	rgb.Reserved = 255;
	unsigned char *tmp = pic_red;
	
	int i,j;
	for(i = 0; i < HEIGHT; i++)
	{
		for(j = 0; j < WIDTH; j++)
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
	LOGD("------NOW prepare to show!-------------> i=%d j=%d",i,j);

	
	ret = __v4l2_show_prepare(video_fd, addinfo, addcnt);
	if(ret < 0)
    {
        LOGD("__v4l2_mmap_fill_color error!");
        ioctl(video_fd, VIDIOC_STREAMOFF,V4L2_BUF_TYPE_VIDEO_OUTPUT);
        close(video_fd);
        return -1;
    }
	memcpy(addinfo[0].pui_addr, pic_red, WIDTH*HEIGHT*4);//pc_tmpbuff 的值读取到虚拟地址空间。

#else
 	ret = __v4l2_show_prepare(video_fd, addinfo, addcnt);
    if(ret < 0)
    {
        LOGD("__v4l2_mmap_fill_color error!");
        ioctl(video_fd, VIDIOC_STREAMOFF,V4L2_BUF_TYPE_VIDEO_OUTPUT);
        close(video_fd);
        return -1;
    }


	xres = WIDTH;
	yres = HEIGHT;	
	bytes_per_pixel = 4;

	line_addr = malloc (sizeof (*line_addr) * yres);
	unsigned int y, addr;
	addr = 0;
	for (y = 0; y < yres; y++, addr += (xres * 4))
		line_addr[y] = addinfo[0].pui_addr + addr;
	button_init();

	refresh_screen();
#endif
	LOGD("is on the show!! y = %d bytes_per_pixel = %d",y,bytes_per_pixel);
//	input_dev_init();
	struct tsdev *ts;
	ts = ts_setup(NULL, 0);
	if (!ts) {
		perror("ts_open");
		exit(1);
	}

	while(1)
	{
		struct ts_sample samp;	
	//	ts_read(&x_abs,&y_abs,&p_prs);		
	//	LOGD("x_abs = %d y_abs = %d p_prs = %d",x_abs,y_abs,p_prs);
		/* Show the cross */
		if ((mode & 15) != 1)
			put_cross(data_x, data_y);
		
	//	ts_read(&x_abs,&y_abs,&p_prs);
		ret =  ts_read(ts, &samp, 1);
		/* Hide it */
		if ((mode & 15) != 1)
			put_cross(data_x, data_y);

		if (ret < 0) {
			perror("ts_read");
			ts_close(ts);
			exit(1);
		}
		
		if (ret != 1)
			continue;
#if 0
		for (i = 0; i < NR_BUTTONS; i++)
			if (button_handle(&buttons[i], x_abs, y_abs, p_prs))
				switch (i) {
				case 0:
					mode = 0;
					refresh_screen ();
					break;
				case 1:
					mode = 1;
					refresh_screen ();
					break;
				case 2:
					quit_pressed = 1;
				}
#endif
		handle_input_data(samp.x,samp.y,samp.pressure,&quit_pressed,&mode);
		LOGD("x_abs = %d y_abs = %d p_prs = %d",samp.x,samp.y,samp.pressure);		

		if (samp.pressure > 0) {
			if (mode == 0x80000001)
				line (data_x,data_y,samp.x,samp.y);
			data_x = samp.x;
			data_y = samp.y;
			mode |= 0x80000000;
		} else
			mode &= ~0x80000000;
		if (quit_pressed)
			break;

		
	}
	
	__v4l2_umamp(video_fd, addinfo, addcnt);
	close(video_fd);
	ts_close(ts);
	
	return 0;
	
}
