
#ifndef _COMMON_H_
#define _COMMON_H_

//#include "font.h"
#include "tslib.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<errno.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<sys/reboot.h>
#include<sys/mount.h>
#include<dirent.h>
#include<signal.h>
#include<ctype.h>
#include<termios.h>
#include<sys/mman.h>
#include<unistd.h>
#include <linux/videodev2.h>

#include <sys/ioctl.h>
#include <sys/time.h>

#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/fb.h>

#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <asm/types.h>
#include <linux/input.h>


#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__)
#define XORMODE 0x80000000
#define VTNAME_LEN 128

typedef struct
{
    unsigned char *pui_addr;
    unsigned int ui_addrlen;
    unsigned int ui_offset;
}V4L2_MAMP_ADDR_INFO_S;//保存映射后的内存信息。

//像素点结构体
typedef struct
{
    unsigned char Blue;
    unsigned char Green;
    unsigned char Red;
    unsigned char Reserved;
} __attribute__((packed)) RgbQuad;

#define V4L2_OSD_WIDTH 		800
#define V4L2_OSD_HEIGHT		480

extern int __v4l2_mmap(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int *buffcnt);//传递时pui_buffcnt为1
extern int __v4l2_alpha_config(int video_fd);
extern int __v4l2_mmap_fill_color(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt,char *pc_tmpbuff,int tmpbufflen);
extern int __v4l2_umamp(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt);
extern int __v4l2_show_prepare(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt);

/* set pixel ops */
typedef unsigned int uint32_t;
unsigned char **line_addr;
uint32_t xres, yres;
int bytes_per_pixel;

struct ts_button {
	int x, y, w, h;
	char *text;
	int flags;
};
#define BUTTON_ACTIVE 0x00000001

extern void refresh_screen ();
extern void fillrect (int x1, int y1, int x2, int y2);
extern void put_string_center(int x, int y, char *s);
extern void put_string(int x, int y, char *s);
extern void put_char(int x, int y, int c);
extern void pixel (int x, int y);
extern void button_init(void);
extern void line (int x1, int y1, int x2, int y2);
extern void pixel_line (int x, int y);
extern void rect (int x1, int y1, int x2, int y2);
extern void button_draw (struct ts_button *button);
extern void put_cross(int x, int y);
extern void line_green(int x1, int y1, int x2, int y2);
extern void pixel_green (int x, int y);
extern void handle_input_data(int x_abs,int y_abs,int p_prs,int *quit_pressed,unsigned int *mode);

/*input ops*/
//extern int input_dev_init(void);
//extern int ts_read(int *x,int *y, int *p);
//extern void input_dev_close(void);
//extern int mySleep(int sec, int usec);

#endif
