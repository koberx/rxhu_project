#include <stdlib.h>    
#include <stdio.h>  
#include <linux/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/un.h>
#include "tsparse.h"
#include "touch_protocol.h"

int get_mt_touch_event(touch_msg_handle handle, const void *ptr)
{

	touch_event_type_t 	event_type;
	touch_point		 	data;
	touch_point			last_point;
	int 				touch_flag = 0;
	bool				call_handle = true;
	int 				ret;
	
	struct ts_sample_mt **samp_mt = NULL;
	int i,j;
	
	TS_DEV *ts = ts_open("/dev/input/event1");
	if(ts == NULL) {
		perror("ts_open");
		return -1;
	}
	
	samp_mt = malloc(2 * sizeof(struct ts_sample_mt *));//nr = 2
	if (!samp_mt) {
		ts_close(ts);
		return -ENOMEM;
	}
	
	for (i = 0; i < 2; i++) {
		samp_mt[i] = calloc(3, sizeof(struct ts_sample_mt));//max_slots = 3 , nr = 2
		if (!samp_mt[i]) {
			free(samp_mt);
			ts_close(ts);
			return -ENOMEM;
		}
	}
	
	while(1) {
		
		ret = ts_mt_input_read(ts, samp_mt, 3,2);//max_slots = 3 nr = 2
		if (ret < 0) {
			perror("ts_mt_input_read");
			ts_close(ts);
			exit(1);
		}
		
		for (j = 0; j < ret; j++) {
			for (i = 0; i < 3; i++) {//max_slots = 3
				if (samp_mt[j][i].valid != 1)
					continue;
				if (samp_mt[j][i].pressure > 0) {
					if(0 == touch_flag) {
						event_type 	= EVENT_DOWN;
						data.x		= samp_mt[j][i].x;
						data.y		= samp_mt[j][i].y;
						data.id		= samp_mt[j][i].slot;
						touch_flag 	= 1;
					} else {
						event_type 	= EVENT_MOVE;
						if(last_point.x == samp_mt[j][i].x && last_point.y == samp_mt[j][i].y) {
							call_handle = false;
						} else {
							data.x			= samp_mt[j][i].x;
							data.y			= samp_mt[j][i].y;
							data.id			= samp_mt[j][i].slot;
							last_point.x	= samp_mt[j][i].x;
							last_point.y	= samp_mt[j][i].y;
							last_point.id	= samp_mt[j][i].slot;
							call_handle 	= true;
						}
					}
				} else {
					event_type		= EVENT_UP;
					data.x			= samp_mt[j][i].x;
					data.y			= samp_mt[j][i].y;
					data.id			= samp_mt[j][i].slot;
					touch_flag 		= 0;
					call_handle 	= true;
				}
			}
		}
		if(call_handle)
			handle(event_type,&data,ptr);
	}
	ts_close(ts);
	return 0;
}



