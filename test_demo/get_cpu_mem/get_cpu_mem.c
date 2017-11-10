
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <assert.h>
#include "get_cpu_mem.h"

#define VMRSS_LINE 15
#define PROCESS_ITEM 14

typedef struct
{
         unsigned int user;
         unsigned int nice;
         unsigned int system;
         unsigned int idle;
}total_cpu_occupy_t;

typedef struct
 {
     pid_t pid;
     unsigned int utime;
     unsigned int stime;
     unsigned int cutime;
     unsigned int cstime;
 }process_cpu_occupy_t;

unsigned long int get_phy_mem(const pid_t p);
unsigned long int get_total_mem();
unsigned long int get_available_mem();
unsigned int get_cpu_total_occupy();
unsigned int get_cpu_process_occupy(const pid_t p);
const char* get_items(const char* buffer,int ie);

extern float get_pcpu(pid_t p);
extern float get_pmem(pid_t p);
extern int get_rmem(pid_t p);

unsigned long int get_phy_mem(const pid_t p)
{
    char file[64] = {0};
  
    FILE *fd;         
    char line_buff[256] = {0};  
    sprintf(file,"/proc/%d/status",p);

    fprintf (stderr, "current pid:%d\n", p);                                                                                                  
    fd = fopen (file, "r"); 

    
    int i;
    char name[32];
    unsigned long int vmrss;
    for (i=0;i<VMRSS_LINE-1 + 2;i++)
    {
        fgets (line_buff, sizeof(line_buff), fd);
    }
    fgets (line_buff, sizeof(line_buff), fd);
    sscanf (line_buff, "%s %ld", name,&vmrss);
//    fprintf (stderr, "====%s º%ld====\n", name,vmrss);
    fclose(fd);   
    return vmrss;
}

int get_rmem(pid_t p)
{
    return get_phy_mem(p);
}


unsigned long int get_total_mem()
{
    char* file = "/proc/meminfo";
  
    FILE *fd;         
    char line_buff[256] = {0};  
    fd = fopen (file, "r"); 

    char name[32];
    unsigned long int memtotal;
    fgets (line_buff, sizeof(line_buff), fd);
    sscanf (line_buff, "%s %ld", name,&memtotal);
    //fprintf (stderr, "====%s %ld====\n", name,memtotal);
    fclose(fd);     
    return memtotal;
}

unsigned long int get_available_mem()
{
	char* file = "/proc/meminfo";
	FILE *fd;
	char line_buff[256] = {0};
	int i;
	fd = fopen (file, "r");

    char name[32];
    unsigned long int availbale_size;
	for(i = 0; i < 3; i++)
    	fgets (line_buff, sizeof(line_buff), fd);
    sscanf (line_buff, "%s %ld", name,&availbale_size);
    //fprintf (stderr, "====%s %ld====\n", name,availbale_size);
    fclose(fd);
    return availbale_size;
}

float get_pmem(pid_t p)
{
    int phy = get_phy_mem(p);
      int total = get_total_mem();
      float occupy = (phy*1.0)/(total*1.0);
      //fprintf(stderr,"====process mem occupy:%.6f\n====",occupy);
      return occupy;
}

unsigned int get_cpu_process_occupy(const pid_t p)
{
    char file[64] = {0};
    process_cpu_occupy_t t;
  
    FILE *fd;         
    char line_buff[1024] = {0};  
    sprintf(file,"/proc/%d/stat",p);

    fprintf (stderr, "current pid:%d\n", p);                                                                                                  
    fd = fopen (file, "r"); 
    fgets (line_buff, sizeof(line_buff), fd); 

    sscanf(line_buff,"%u",&t.pid);
    char* q = get_items(line_buff,PROCESS_ITEM);
    sscanf(q,"%u %u %u %u",&t.utime,&t.stime,&t.cutime,&t.cstime);

    //fprintf (stderr, "====pid%u:%u %u %u %u====\n", t.pid, t.utime,t.stime,t.cutime,t.cstime);
    fclose(fd);     
    return (t.utime + t.stime + t.cutime + t.cstime);
}


unsigned int get_cpu_total_occupy()
{
    FILE *fd;         
    char buff[1024] = {0};  
    total_cpu_occupy_t t;
                                                                                                             
    fd = fopen ("/proc/stat", "r"); 
    fgets (buff, sizeof(buff), fd); 
    
    char name[16];
    sscanf (buff, "%s %u %u %u %u", name, &t.user, &t.nice,&t.system, &t.idle);
    

    //fprintf (stderr, "====%s:%u %u %u %u====\n", name, t.user, t.nice,t.system, t.idle);
    fclose(fd);     
    return (t.user + t.nice + t.system + t.idle);
}


float get_pcpu(pid_t p)
{
    unsigned int totalcputime1,totalcputime2;
      unsigned int procputime1,procputime2;
    totalcputime1 = get_cpu_total_occupy();
    procputime1 = get_cpu_process_occupy(p);
    usleep(500000);
    totalcputime2 = get_cpu_total_occupy();
    procputime2 = get_cpu_process_occupy(p);
    float pcpu = 100.0*(procputime2 - procputime1)/(totalcputime2 - totalcputime1);
//    fprintf(stderr,"====pcpu:%.6f\n====",pcpu);
    return pcpu;
}

const char* get_items(const char* buffer,int ie)
{
    assert(buffer);
    char* p = buffer;
    int len = strlen(buffer);
    int count = 0;
    if (1 == ie || ie < 1)
    {
        return p;
    }
    int i;
    
    for (i=0; i<len; i++)
    {
        if (' ' == *p)
        {
            count++;
            if (count == ie-1)
            {
                p++;
                break;
            }
        }
        p++;
    }

    return p;
}

/*extern*/

float get_system_memory_occupancy(void)
{
	unsigned long int total;
	unsigned long int availble;
	float occupy;
	total = get_total_mem();
	availble = get_available_mem();	
	occupy = ((total - availble)*1.0)/(total);
	//fprintf(stderr,"====process mem occupy:%.6f\n====",occupy);
	return occupy; 
}

float get_process_memory_occupancy(pid_t ProcessID)
{
	return get_pmem(ProcessID);
}

float get_process_cpu_occupancy(pid_t ProcessID)
{
	return get_pcpu(ProcessID);
}

unsigned long int get_process_phy_mem(pid_t ProcessID)
{
	return get_phy_mem(ProcessID);
}

