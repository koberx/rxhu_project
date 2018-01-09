
#include <stdlib.h>    
#include <stdio.h>  
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>  
#include "touch_protocol.h"

#if 0
void input_data_handle(touch_event_type_t event_type, void *data, const void *ptr)
{
	touch_point*	point;
	switch(event_type) {
	case EVENT_DOWN:
		point = (touch_point*)data;
		printf("EVENT_DOWN x_abs = %d y_abs = %d\n",point->x,point->y);
		break;
	case EVENT_UP:
		point = (touch_point*)data;
		printf("EVENT_UP x_abs = %d y_abs = %d\n",point->x,point->y);
		break;
	case EVENT_MOVE:
		point = (touch_point*)data;
		printf("EVENT_MOVE x_abs = %d y_abs = %d\n",point->x,point->y);
		break;
	case EVENT_KEY:
		break;
	}
}

int main(void)
{
	get_touch_event(input_data_handle,NULL);
	return 0;
}
#else
void input_data_handle(touch_event_type_t event_type, void *data, const void *ptr)
{
	touch_point*	point;
	switch(event_type) {
	case EVENT_DOWN:
		point = (touch_point*)data;
		printf("EVENT_DOWN x_abs = %d y_abs = %d id = %d\n",point->x,point->y,point->id);
		break;
	case EVENT_UP:
		point = (touch_point*)data;
		printf("EVENT_UP x_abs = %d y_abs = %d id = %d\n",point->x,point->y,point->id);
		break;
	case EVENT_MOVE:
		point = (touch_point*)data;
		printf("EVENT_MOVE x_abs = %d y_abs = %d id = %d\n",point->x,point->y,point->id);
		break;
	case EVENT_KEY:
		break;
	}
}

int main(void)
{
	get_mt_touch_event(input_data_handle,NULL);
	return 0;
}
	
#endif