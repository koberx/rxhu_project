#include "uart.h"

int main(int argc,char *argv[])
{
	int fd = 0;                            
    int err;                           
    int len;                            
    int i;    
    char rcv_buf[100];           
    char send_buf[20]="tiger john";  
	
	if(argc != 2)
	{
		LOGD("usage : ./uart_test 0/1");
		return FALSE;
	}

	fd = UART3_Open(fd,TTY_DEVICE); 
    do{    
		err = UART3_Init(fd,115200,0,8,1,'N');    
        LOGD("Set Port Exactly!");    
    }while(FALSE == err || FALSE == fd); 

	if(0 == strcmp(argv[1],"0"))
	{
		for(i = 0; i < 5; i++)
		{
			len = UART3_Send(fd,send_buf,10);
			if(len > 0)
				LOGD("the uart tx successful!");
			else
				LOGD("the uart tx failed!");
			sleep(1);
		}
		UART3_Close(fd);
	}
	else
	{	
		while(1)
		{
			len = UART3_Recv(fd, rcv_buf,sizeof(rcv_buf));
			if(len > 0)
			{
				rcv_buf[len] = '\0';
				LOGD("the uart rx recv: %s",rcv_buf);	
			}
			else
				LOGD("thr uart rx failed!");
			sleep(2);
		}
		UART3_Close(fd);
	
	}		
	

	return 0;
}
