#ifndef _TSPARSE_H_
#define _TSPARSE_H_


#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdarg.h>

/*macro and var*/

#define EV_CNT (EV_MAX+1)

#define ABS_CNT (ABS_MAX+1)

#define KEY_CNT (KEY_MAX+1)

#ifndef SYN_CNT
# define SYN_CNT (SYN_MAX+1)
#endif

#ifndef SYN_MAX 
# define SYN_MAX 0xf
#endif

#ifndef ABS_MT_TOOL_X /* < 3.6 kernel headers */
# define ABS_MT_TOOL_X           0x3c    /* Center X tool position */
# define ABS_MT_TOOL_Y           0x3d    /* Center Y tool position */
#endif



#define BITS_PER_BYTE           8
#define BITS_PER_LONG           (sizeof(long) * BITS_PER_BYTE)

#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))

#define DIV_ROUND_UP(n, d) 		(((n) + (d) - 1) / (d))
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))

#define EGALAX_VERSION_112 1
#define EGALAX_VERSION_210 2

#define GRAB_EVENTS_WANTED	1
#define GRAB_EVENTS_ACTIVE	2

#define NUM_EVENTS_READ 1 

struct ts_sample {
	int				x;
	int				y;
	unsigned int	pressure;
	struct timeval	tv;
};

struct ts_sample_mt {
	int				x;
	int				y;
	unsigned int	pressure;
	int				slot;
	int				tracking_id;

	int				tool_type;
	int				tool_x;
	int				tool_y;
	unsigned int	touch_major;
	unsigned int	width_major;
	unsigned int	touch_minor;
	unsigned int	width_minor;
	int				orientation;
	int				distance;
	int				blob_id;

	struct 			timeval	tv;
	short			pen_down;

	short			valid;
};

typedef struct {
	int 				using_syn;
	short				mt;
	short				no_pressure;
	short				type_a;
	int					grab_events;
	
	int					current_p;
	int					current_x;
	int					current_y;
	int					nr;
	int					max_slots;
	int 				slot;
	struct ts_sample_mt **buf;
	
}TS_DEV_INFO;

typedef struct {
	int 			fd;
	unsigned int	special_device; 
	struct input_event ev[NUM_EVENTS_READ];//add
	TS_DEV_INFO		dev_info;
	
}TS_DEV;

extern void set_pressure(TS_DEV *ts);
extern int check_fd(TS_DEV *ts);
extern int ts_input_read(TS_DEV *ts,struct ts_sample *samp,int nr);
extern int ts_mt_input_read(TS_DEV *ts ,struct ts_sample_mt **samp,int max_slots, int nr);//add
extern TS_DEV *ts_open(const char *name);
extern void ts_close(TS_DEV * ts);
#endif
