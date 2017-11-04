#ifndef _UART_H_
#define _UART_H_
#include<stdio.h>          
#include<stdlib.h>     
#include<unistd.h>     
#include<sys/types.h>     
#include<sys/stat.h>       
#include<fcntl.h>     
#include<termios.h>    
#include<errno.h>    
#include<string.h> 


#define FALSE  -1    
#define TRUE   0   
#define LOGD(format,...) printf("[File:%s][Line:%d] "format"\n",__FILE__, __LINE__,##__VA_ARGS__)

#define TTY_DEVICE "/dev/ttySiRF3"

extern int UART3_Send(int fd, char *send_buf,int data_len);
extern int UART3_Recv(int fd, char *rcv_buf,int data_len);
extern int UART3_Init(int fd, int speed,int flow_ctrl,int databits,int stopbits,int parity);
extern int UART3_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity);
extern void UART3_Close(int fd);
extern int UART3_Open(int fd,char* port);


#endif
