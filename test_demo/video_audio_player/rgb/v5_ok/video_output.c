
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <pthread.h>  
#include <sched.h>  
#include <unistd.h> 
#include <sys/times.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define	VIDEO_DEVICE "/dev/video1" 
#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__);

typedef struct
{
    unsigned int *pui_addr;
    unsigned int ui_addrlen;
    unsigned int ui_offset;
}V4L2_MAMP_ADDR_INFO_S;//保存映射后的内存信息。

#define V4L2_OSD_WIDTH              800
#define V4L2_OSD_HEIGHT             480


int __v4l2_mmap(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int *buffcnt)//传递时pui_buffcnt为1
{
    int ret;
    unsigned int ui_idx = 0;
    struct v4l2_buffer vo_reqoutput_buf;

    struct v4l2_requestbuffers vobuf_req;
    memset(&vobuf_req, 0, sizeof(vobuf_req));

    vobuf_req.count = 1;    
    vobuf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;//向驱动申请内存。
    vobuf_req.memory = V4L2_MEMORY_MMAP;   
    if (ioctl(video_fd, VIDIOC_REQBUFS, &vobuf_req) < 0)
    {
		LOGD("ioctl VIDIOC_REQBUFS error");
        return -1;
    }

    do
    {
        memset(&vo_reqoutput_buf, 0, sizeof(vo_reqoutput_buf));
        vo_reqoutput_buf.index = ui_idx;
        vo_reqoutput_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        ret = ioctl(video_fd, VIDIOC_QUERYBUF, &vo_reqoutput_buf);//查询申请到的内存。
        LOGD("__v4l2_mmap i_ret=%d VIDIOC_QUERYBUF \n \t index=%d, type=%d, bytesused=%d, flags=%#x, field=%d, time[%d.%d] \n",ret,\
            vo_reqoutput_buf.index, vo_reqoutput_buf.type, vo_reqoutput_buf.bytesused, vo_reqoutput_buf.flags,\
            vo_reqoutput_buf.field, vo_reqoutput_buf.timestamp.tv_sec, vo_reqoutput_buf.timestamp.tv_usec);
	
        LOGD("\t memory=%d, length=%d, offset=%d\n", \
            vo_reqoutput_buf.memory, vo_reqoutput_buf.length, vo_reqoutput_buf.m.offset);
        if (ret < 0)
        {
            break;
        }

        /* mmap */    
        past_buff[ui_idx].pui_addr = (unsigned int *)mmap(NULL, vo_reqoutput_buf.length,//依据查询到的信息，映射内存空间。
                                        PROT_READ | PROT_WRITE, MAP_SHARED,
                                        video_fd, vo_reqoutput_buf.m.offset);
        if (past_buff[ui_idx].pui_addr == NULL)
        {
            break;
        }
        past_buff[ui_idx].ui_addrlen = vo_reqoutput_buf.length;
        past_buff[ui_idx].ui_offset = vo_reqoutput_buf.m.offset;

        ui_idx++;
        if (ui_idx >= *buffcnt)
        {
            break;
        }
    }while (1);

    *buffcnt = ui_idx;

    for (ui_idx = 0; ui_idx < *buffcnt; ui_idx++)
    {
        LOGD("\t pui_addr=%#x, ui_addrlen=%d, ui_offset=%d\n", \
            past_buff[ui_idx].pui_addr, past_buff[ui_idx].ui_addrlen, past_buff[ui_idx].ui_offset);
    }

    if (*buffcnt == 0)
    {
        return -1;
    }

    return 0;
}


/*设置v4l2的配置*/
int __v4l2_alpha_config(int video_fd)//参数设备节点的描述符
{
    struct v4l2_format vo_format;

    memset(&vo_format, 0, sizeof(vo_format));
    vo_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;//设定为输出方向
    if (ioctl(video_fd, VIDIOC_G_FMT, &vo_format) < 0)
    {
        LOGD("ioctl VIDIOC_G_FMT error!");
        return -1;
    }
    vo_format.fmt.pix.width = V4L2_OSD_WIDTH;//像素设置
    vo_format.fmt.pix.height = V4L2_OSD_HEIGHT;
    vo_format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;//V V4L2_PIX_FMT_ARGB32
    vo_format.fmt.pix.field = V4L2_FIELD_NONE;
    vo_format.fmt.pix.bytesperline = 0;
    vo_format.fmt.pix.sizeimage = 1536000;
    vo_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    if (ioctl(video_fd, VIDIOC_S_FMT, &vo_format) < 0)//设定输出格式
    {
		LOGD("ioctl VIDIOC_S_FMT error!");
        return -1;
    }

    struct v4l2_crop vo_crop;
    
    vo_crop.type = vo_format.type;//设定显示尺寸设置，这里是满屏。
    vo_crop.c.left = 0;
    vo_crop.c.top = 0;
    vo_crop.c.width = vo_format.fmt.pix.width;
    vo_crop.c.height = vo_format.fmt.pix.height;
    
    if (ioctl(video_fd, VIDIOC_S_CROP, &vo_crop) < 0)
    {
		LOGD("ioctl VIDIOC_S_CROP error");
        return -1;
    }
    
    memset(&vo_format, 0, sizeof(vo_format));
    vo_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY;
	if (ioctl(video_fd, VIDIOC_G_FMT, &vo_format) < 0) 
    {
		LOGD("ioctl VIDIOC_G_FMT error");
        return -1;
    }

	vo_format.fmt.win.w.width = V4L2_OSD_WIDTH;//overlay的尺寸
	vo_format.fmt.win.w.height = V4L2_OSD_HEIGHT;
	vo_format.fmt.win.w.left = 0;
	vo_format.fmt.win.w.top = 0;

    vo_format.fmt.win.global_alpha = 0xff;
    if (ioctl(video_fd, VIDIOC_S_FMT, &vo_format) < 0)
    {
		LOGD("ioctl VIDIOC_S_FMT error")
        return -1;
    }

    struct v4l2_framebuffer vo_fb;

    memset(&vo_fb, 0, sizeof(vo_fb));

	if (ioctl(video_fd, VIDIOC_G_FBUF, &vo_fb) < 0) 
    {
		LOGD("ioctl VIDIOC_G_FBUF error");
        return -1;
	}
	if (!(vo_fb.capability & V4L2_FBUF_CAP_LOCAL_ALPHA)) 
    {
		LOGD("does not support per-pixel alpha");
        return -1;
	}

	vo_fb.flags |= V4L2_FBUF_FLAG_LOCAL_ALPHA;
	vo_fb.flags &= ~(V4L2_FBUF_FLAG_CHROMAKEY | V4L2_FBUF_FLAG_SRC_CHROMAKEY);
	if (ioctl(video_fd, VIDIOC_S_FBUF, &vo_fb) < 0) 
    {
        printf("%s.%d ioctl VIDIOC_S_FBUF error !\n", __FUNCTION__, __LINE__);
		LOGD("ioctl VIDIOC_S_FBUF error !");
        return -1;
	}

    return 0;
}


int __v4l2_mmap_fill_color(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt)
{
    struct v4l2_buffer vo_dqoutput_buf;
    int ret = 0;
    char *pc_tmpbuff;
    int tmpbufflen;

    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    ioctl(video_fd, VIDIOC_STREAMON, &type);

    memset(&vo_dqoutput_buf, 0, sizeof(vo_dqoutput_buf));
    vo_dqoutput_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    vo_dqoutput_buf.memory = V4L2_MEMORY_MMAP;
    vo_dqoutput_buf.field = V4L2_FIELD_NONE;
    vo_dqoutput_buf.index = 0;
    
  //  ioctl(video_fd, VIDIOC_QBUF, &vo_dqoutput_buf);//将空闲内存加入到队列。

//    memcpy(past_buff[0].pui_addr, pc_tmpbuff, tmpbufflen);//pc_tmpbuff 的值读取到虚拟地址空间。
    
}

int __v4l2_umamp(int video_fd, V4L2_MAMP_ADDR_INFO_S *past_buff, unsigned int ui_buffcnt)
{
    unsigned int ui_idx = 0;

    while (1)
    {
        munmap(past_buff[ui_idx].pui_addr, past_buff[ui_idx].ui_addrlen);
        ui_idx++;
        if (ui_idx >= ui_buffcnt)
        {
            break;
        }
    }

    struct v4l2_requestbuffers vobuf_req;
    memset(&vobuf_req, 0, sizeof(vobuf_req));

    vobuf_req.count = 0;    
    vobuf_req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    vobuf_req.memory = V4L2_MEMORY_MMAP;   
    if (ioctl(video_fd, VIDIOC_REQBUFS, &vobuf_req) < 0)
    {
		LOGD("ioctl VIDIOC_REQBUFS error");
        return -1;
    }

    return 0;
}



int __v4l2_mmap_show_do(void)
{
    int video_fd = -1;
    int ret = -1;

    video_fd = open(VIDEO_DEVICE, O_RDWR);// 打开设备
    if (video_fd < 0)
    {
		LOGD("open the video device error!");
        goto __ret;
    }
   	LOGD("open the video device successful!"); 

    V4L2_MAMP_ADDR_INFO_S addinfo[6];
    unsigned int addcnt = 1;

    memset(addinfo, 0, sizeof(addinfo));

    if (__v4l2_alpha_config(video_fd) != 0) ;//goto  __ret;    

    if (__v4l2_mmap(video_fd, addinfo, &addcnt) != 0) goto  __ret;

    ret = __v4l2_mmap_fill_color(video_fd, addinfo, addcnt);

    __v4l2_umamp(video_fd, addinfo, addcnt);
__ret:
    if (video_fd != (int)NULL)
    {
        close(video_fd);
        //i_devfd = NULL;
    }

    return ret;
}

int main(void)
{
	__v4l2_mmap_show_do();
	return 0;
}
