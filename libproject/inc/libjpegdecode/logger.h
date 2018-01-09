#ifndef _LOGGER_H_
#define _LOGGER_H_
#include <stdlib.h>
extern int logleve;
typedef enum {
	EMERG  = 0,
    FATAL  = 0,
    ALERT  = 100,
    CRIT   = 200,
    ERROR  = 300,
    WARN   = 400,
    NOTICE = 500,
    INFO   = 600,
    DEBUG  = 700,
    NOTSET = 800
} PriorityLevel;

#define LOG(__level,format,...) if (__level<=logleve)  printf("[Func]:%s[Line:%d] "format"\n",__FUNCTION__, __LINE__,##__VA_ARGS__)

inline void initLogger(int verbose)
{
        switch (verbose)
        {
                case 2: logleve=DEBUG; break;
                case 1: logleve=INFO; break;
                default: logleve=ERROR; break;

        }
}
#endif

