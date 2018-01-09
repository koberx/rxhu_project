#include <stdlib.h>    
#include <stdio.h>  
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/un.h>
#include "tsparse.h"
#include "touch_protocol.h"

int mySleep(int sec,int msec)
{
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = msec * 1000;
	select(0, NULL, NULL, NULL, &tv);
	return 0;
}

int get_touch_event(touch_msg_handle handle, const void *ptr)
{

	touch_event_type_t 	event_type;
	touch_point		 	data;
	touch_point			last_point;
	int 				touch_flag = 0;
	bool				call_handle = true;
	int 				ret;
	TS_DEV *ts = ts_open("/dev/input/event1");
	if(ts == NULL) {
		perror("ts_open");
		return -1;
	}
	while(1) {
		struct ts_sample samp;
		ret = ts_input_read(ts, &samp, 1);
		if (ret < 0) {
			perror("ts_input_read");
			ts_close(ts);
			return -1;
		}
		if (samp.pressure > 0) {
			if(0 == touch_flag) {
				event_type 	= EVENT_DOWN;
				data.x		= samp.x;
				data.y		= samp.y;
				touch_flag 	= 1;
			} else {
				event_type 	= EVENT_MOVE;
				if(last_point.x == samp.x && last_point.y == samp.y) {
					call_handle = false;
				} else {
					data.x			= samp.x;
					data.y			= samp.y;
					last_point.x	= samp.x;
					last_point.y	= samp.y;
					call_handle 	= true;
				}
			}
		} else {
			
			event_type		= EVENT_UP;
			data.x			= samp.x;
			data.y			= samp.y;
			touch_flag 		= 0;
			call_handle 	= true;
		}
		if(call_handle)
			handle(event_type,&data,ptr);
		//mySleep(0,1);
	}
	ts_close(ts);
	return 0;
}

