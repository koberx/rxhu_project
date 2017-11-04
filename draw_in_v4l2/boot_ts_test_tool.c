#include <stdlib.h>    
#include <stdio.h>  
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>  

int thread(void)
{
	system("/usr/local/tslib/share/ts_test_tool");
	return 0;
}

int main(int argc,char *argv[])  
{
	putenv("TSLIB_ROOT");
	putenv("TSLIB_TSDEVICE");
	putenv("TSLIB_CONFFILE");
	putenv("TSLIB_PLUGINDIR");
	putenv("TSLIB_CALIBFILE");
	putenv("TSLIB_CONSOLEDEVICE");
	putenv("TSLIB_FBDEVICE");
	putenv("LD_LIBRARY_PATH");
	

	setenv("TSLIB_ROOT","/usr/local/tslib",1);	
	getenv("TSLIB_ROOT");
	setenv("TSLIB_TSDEVICE","/dev/input/event1",1);	
	getenv("TSLIB_TSDEVICE");
	setenv("TSLIB_CONFFILE","/usr/local/tslib/etc/ts.conf",1);
	getenv("TSLIB_CONFFILE");
	setenv("TSLIB_PLUGINDIR","/usr/local/tslib/lib/ts",1);
	getenv("TSLIB_PLUGINDIR");
	setenv("TSLIB_CALIBFILE","/etc/pointercal",1);
	getenv("TSLIB_CALIBFILE");
	setenv("TSLIB_CONSOLEDEVICE","none",1);
	getenv("TSLIB_CONSOLEDEVICE");
	setenv("TSLIB_FBDEVICE","/dev/fb0",1);
	getenv("TSLIB_FBDEVICE");
	setenv("LD_LIBRARY_PATH","/usr/lib:/home/root/project/lib:/usr/local/tslib/lib",1);
	getenv("LD_LIBRARY_PATH");

	pthread_t id;
	int ret;
	ret = pthread_create(&id,NULL,(void *)thread,NULL);
	if(ret != 0)
	{
		printf("Create pthread error");
		return -1;
	}
	pthread_join(id,NULL);//等待子进程结束
	
	return 0;

}
