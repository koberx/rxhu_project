
#ifndef __TOUCH_PROTOCOL_H__
#define __TOUCH_PROTOCOL_H__

typedef enum
{
	EVENT_DOWN,
	EVENT_UP,
	EVENT_MOVE,
	EVENT_KEY,
}touch_event_type_t;

typedef struct
{
	int x;
	int y;
	int id;//add
}touch_point, *p_touch_point;

/********************************************************************/
/* Function Name:	touch_msg_handle								*/
/* Description  :	触摸事件处理回调函数指针类型					*/
/* Parameter    :	event_type  触摸事件的类型                      */
/*					data        触摸事件的数据						*/
/*                  ptr         调用获取触摸事件接口传入的参数      */
/* Return       :	void                 							*/
/********************************************************************/
typedef void (*touch_msg_handle)(touch_event_type_t event_type, void *data, const void *ptr);

/********************************************************************/
/* Function Name:	get_touch_event									*/
/* Description  :	开始获取触摸事件       							*/
/* Parameter    :	ptr    回调函数中直接传回的参数                 */
/*					handle 触摸事件处理回调							*/
/* Return       :	返回非零表示获取失败							*/
/********************************************************************/
extern int 
get_touch_event(touch_msg_handle handle, const void *ptr);

/********************************************************************/
/* Function Name:	get_mt_touch_event								*/
/* Description  :	开始获取多点触摸事件       						*/
/* Parameter    :	ptr    回调函数中直接传回的参数                 */
/*					handle 触摸事件处理回调							*/
/* Return       :	返回非零表示获取失败							*/
/********************************************************************/
extern int 
get_mt_touch_event(touch_msg_handle handle, const void *ptr);


#endif
