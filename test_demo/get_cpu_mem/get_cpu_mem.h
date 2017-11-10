#ifndef _GET_CPU_MEM_H_
#define _GET_CPU_MEM_H_
#include <sys/types.h>
extern unsigned long int get_process_phy_mem(pid_t ProcessID);//获取进程占用物理内存大小
extern float get_system_memory_occupancy(void);//获取系统总的内存占用率
extern float get_process_memory_occupancy(pid_t ProcessID);//获取进程内存占用率 
extern float get_process_cpu_occupancy(pid_t ProcessID);//获取进程cpu占用率

#endif
