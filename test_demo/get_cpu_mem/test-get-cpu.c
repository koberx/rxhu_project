#include "get_cpu_mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   //头文件
#include <assert.h>

int main(void)
{
	pid_t pid;
	unsigned long int total;
	unsigned long int available;
	pid = getpid(); 
	total = get_total_mem();
	available = get_available_mem();
	printf("get the total %ld kB\n",total);
	printf("get the available %ld kB\n",available);
	printf("the occupy mem = %.6f\n",get_system_memory_occupancy());
	printf("this process occupy mem = %.6f\n",get_process_memory_occupancy(pid));
	printf("this process occupy cpu = %.6f\n",get_process_cpu_occupancy(pid));
	printf("this process phy mem size = %ld kB\n",get_process_phy_mem(pid));
	return 0;
}
