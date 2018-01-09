#include<iostream>
#include<string>
#include<vector>
#include<utility>
#include <algorithm>

#include "V4l2Output.h"
#include "V4l2Device.h"
#include "logger.h"
#include "V4l2Access.h"
#include "V4l2MmapDevice.h"

extern "C" {
	#include "stdio.h"
	#include "stdlib.h"
	#include "string.h"
	#include <dirent.h>
	#include <unistd.h>
	#include "jpeglib.h"
	#include <jerror.h>
}
using namespace std;

#define THE_PATH_BASE 	"/usr/src/bootshow/"   //保存资源的基路径

typedef pair<string,int> PAIR;

//Define outside
int cmp(const PAIR& x, const PAIR& y)
{
    return x.second < y.second;
}

vector<PAIR> vecpair;


V4l2Output* init_display(void)
{
	initLogger(1);
	V4L2DeviceParameters param("/dev/video1",V4L2_PIX_FMT_RGB32, 800, 480, 30, 4);
	return V4l2Output::create(param,V4l2Access::IOTYPE_MMAP);
}


int display_pic(V4l2Output* VideoOutput,const char *file_name)
{
	FILE *input_file;
	unsigned long width;
	unsigned long height;
	unsigned short depth;
	unsigned int x,y;
	char file_path[50];
	memset(file_path,0,sizeof(file_path));
	strcpy(file_path,THE_PATH_BASE);
	strcat(file_path,file_name);
	
	input_file=fopen(file_path,"rb");
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo,input_file);
    jpeg_read_header(&cinfo,TRUE);
    jpeg_start_decompress(&cinfo);
	
	width = cinfo.output_width;
    height = cinfo.output_height;
    depth = cinfo.output_components;
	
	 unsigned int show_buffer[width * height];
	 unsigned char src_buff[sizeof(unsigned char) * width * height * depth];
	 memset(src_buff, 0, sizeof(unsigned char) * width * height * depth);
	 JSAMPARRAY buffer;
	 buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, width*depth, 1);
	
	unsigned char *point = src_buff;
    while(cinfo.output_scanline < height)
    {
        jpeg_read_scanlines(&cinfo, buffer, 1); 
        memcpy(point, *buffer, width*depth);    
        point += width * depth;            
    }
	
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

	/*prepare the show buffer*/
	unsigned int cnt = 0, a = 0;
	for (y=0; y<height; y++)
	{
		for (x=0; x<width; x++)
		{ 
			cnt = width * y + x;
			*(show_buffer + cnt) = ((src_buff[a+2]<<0) | (src_buff[a+1]<<8)| (src_buff[a+0]<<16) | (0xFF << 24)); 
			a += 3;
		}
	}
	
	struct timeval timeout;
	timeout.tv_sec  = 0;
	timeout.tv_usec = 30 * 1000; 
	
	while(VideoOutput->isWritable(&timeout) == 1) {
		VideoOutput->write((char *)show_buffer,sizeof(unsigned int) * width * height);
	}
	
    fclose(input_file);
	
	return 0;
}

int readFileList(char *basePath)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1000];
	char name_cut[10];
	
    if ((dir=opendir(basePath)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }

    while ((ptr=readdir(dir)) != NULL)
    {
		memset(name_cut,0,sizeof(name_cut));
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8) {    ///file
			strncpy(name_cut,ptr->d_name,3);//文件名编号表示播放顺序 3 表示文件名长度
			vecpair.push_back(make_pair(ptr->d_name, atoi(name_cut)));  
		}
        else if(ptr->d_type == 10)    ///link file
            printf("d_name:%s/%s\n",basePath,ptr->d_name);
        else if(ptr->d_type == 4)    ///dir
        {
            memset(base,'\0',sizeof(base));
            strcpy(base,basePath);
            strcat(base,"/");
            strcat(base,ptr->d_name);
            readFileList(base);
        }
    }
	sort(vecpair.begin(), vecpair.end(), cmp);
	
    closedir(dir);
    return 1;
}

int main()
{
    char basePath[1000];

    ///get the file list
    memset(basePath,'\0',sizeof(basePath));
    strcpy(basePath,THE_PATH_BASE);
	
	V4l2Output* VideoOutput = init_display();
	
    readFileList(basePath);
	
	for(unsigned int i = 0 ;i<vecpair.size();i++)  
    { 
        std::cout<<"  "<<vecpair[i].first<<" "<<vecpair[i].second<<std::endl;
		display_pic(VideoOutput,vecpair[i].first.data());
    }
	
	delete VideoOutput;
	
    return 0;
}


