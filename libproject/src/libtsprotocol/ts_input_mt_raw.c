#include "tsparse.h"
#include <linux/errno.h>

int ts_mt_input_read(TS_DEV *ts ,struct ts_sample_mt **samp,int max_slots, int nr)
{
	int j,k;
	int total = 0;
	int rd;
	unsigned int it;
	unsigned char pen_up;
	int ret = nr;
	
	ts->dev_info.buf = malloc(nr * sizeof(struct ts_sample_mt *));
	if (!ts->dev_info.buf)
			return -ENOMEM;
		
	for (j = 0; j < nr; j++) {
			ts->dev_info.buf[j] = malloc(max_slots *
					   sizeof(struct ts_sample_mt));
			if (!ts->dev_info.buf[j])
				return -ENOMEM;
	}
	
	ts->dev_info.max_slots = max_slots;
	ts->dev_info.nr = nr;
	
	ts->fd = check_fd(ts);
	
	if (ts->fd == -1)
		return -1;
	
	if (ts->dev_info.no_pressure)
		set_pressure(ts);
	
	for (j = 0; j < nr; j++) {
		for (k = 0; k < max_slots; k++) {
			ts->dev_info.buf[j][k].valid = 0;
			ts->dev_info.buf[j][k].pen_down = -1;
		}
	}
	
	if (ts->dev_info.using_syn) {
		while(total < nr) {
			memset(ts->ev, 0, sizeof(ts->ev));
			rd = read(ts->fd,ts->ev,sizeof(struct input_event) * NUM_EVENTS_READ);
			if (rd == -1) {
				if (total == 0) {
					if (errno > 0)
						return errno * -1;
					else
						return errno;
				}
			} else if (rd < (int) sizeof(struct input_event)) { //if (rd == -1)
				if (total == 0)
					return -1;
				else
					return total;
			}
			
			for (it = 0; it < rd / sizeof(struct input_event); it++){
				switch (ts->ev[it].type) {
				case EV_KEY:
					switch (ts->ev[it].code) {
					case BTN_TOUCH:
					case BTN_LEFT:
						if (ts->ev[it].code == BTN_TOUCH) {
							ts->dev_info.buf[total][ts->dev_info.slot].pen_down = ts->ev[it].value;
							ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
							if (ts->ev[it].value == 0)
								pen_up = 1;
						}
						break;
					}//EV_KEY
					break;
				case EV_SYN:
					switch (ts->ev[it].code) {
					case SYN_REPORT:
						if (pen_up && ts->dev_info.no_pressure) {
							for (k = 0; k < max_slots; k++) {
								if (ts->dev_info.buf[total][k].x != 0 ||
								    ts->dev_info.buf[total][k].y != 0 ||
								    ts->dev_info.buf[total][k].pressure != 0)
									ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;

								ts->dev_info.buf[total][k].x = 0;
								ts->dev_info.buf[total][k].y = 0;
								ts->dev_info.buf[total][k].pressure = 0;
							}

						if (pen_up)
							pen_up = 0;
						}

						for (j = 0; j < nr; j++) {
							for (k = 0; k < max_slots; k++) {
								memcpy(&samp[j][k],
									&ts->dev_info.buf[j][k],
									sizeof(struct ts_sample_mt));
							}
						}

						if (ts->dev_info.type_a)
							ts->dev_info.slot = 0;

						total++;
						break;
						
					case SYN_MT_REPORT:
						if (!ts->dev_info.type_a)
							break;

						if (ts->dev_info.buf[total][ts->dev_info.slot].valid != 1) {
							ts->dev_info.buf[total][ts->dev_info.slot].pressure = 0;
							if (ts->dev_info.slot == 0) {
								pen_up = 1;
								break;
							}

						} else if (ts->dev_info.slot < (max_slots - 1)) {
							ts->dev_info.slot++;
							ts->dev_info.buf[total][ts->dev_info.slot].slot = ts->dev_info.slot;
						}

						break;
					}//EV_SYN
					break;
					case EV_ABS:
					switch (ts->ev[it].code) {
					case ABS_X:
						if (ts->dev_info.mt && ts->dev_info.buf[total][ts->dev_info.slot].valid == 1)
							break;
					case ABS_MT_POSITION_X:
						ts->dev_info.buf[total][ts->dev_info.slot].x = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_Y:
						if (ts->dev_info.mt && ts->dev_info.buf[total][ts->dev_info.slot].valid == 1)
							break;
					case ABS_MT_POSITION_Y:
						ts->dev_info.buf[total][ts->dev_info.slot].y = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_PRESSURE:
						if (ts->dev_info.mt && ts->dev_info.buf[total][ts->dev_info.slot].valid == 1)
							break;
					case ABS_MT_PRESSURE:
						ts->dev_info.buf[total][ts->dev_info.slot].pressure = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_MT_TOOL_X:
						ts->dev_info.buf[total][ts->dev_info.slot].tool_x = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_MT_TOOL_Y:
						ts->dev_info.buf[total][ts->dev_info.slot].tool_y = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_MT_TOOL_TYPE:
						ts->dev_info.buf[total][ts->dev_info.slot].tool_type = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_MT_ORIENTATION:
						ts->dev_info.buf[total][ts->dev_info.slot].orientation = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_MT_DISTANCE:
						ts->dev_info.buf[total][ts->dev_info.slot].distance = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						if (ts->special_device == EGALAX_VERSION_210) {
							if (ts->ev[it].value > 0)
								ts->dev_info.buf[total][ts->dev_info.slot].pressure = 0;
							else
								ts->dev_info.buf[total][ts->dev_info.slot].pressure = 255;
						}
						break;
					case ABS_MT_BLOB_ID:
						ts->dev_info.buf[total][ts->dev_info.slot].blob_id = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_MT_TOUCH_MAJOR:
						ts->dev_info.buf[total][ts->dev_info.slot].touch_major = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_MT_WIDTH_MAJOR:
						ts->dev_info.buf[total][ts->dev_info.slot].width_major = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_MT_TOUCH_MINOR:
						ts->dev_info.buf[total][ts->dev_info.slot].touch_minor = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_MT_WIDTH_MINOR:
						ts->dev_info.buf[total][ts->dev_info.slot].width_minor = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						break;
					case ABS_MT_TRACKING_ID:
						ts->dev_info.buf[total][ts->dev_info.slot].tracking_id = ts->ev[it].value;
						ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
						ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						if (ts->ev[it].value == -1)
							ts->dev_info.buf[total][ts->dev_info.slot].pressure = 0;
						break;
					case ABS_MT_SLOT:
						if (ts->ev[it].value < 0 || ts->ev[it].value >= max_slots) {
							fprintf(stderr, "ADAYO: warning: slot out of range. data corrupted!\n");
							ts->dev_info.slot = max_slots - 1;
						} else {
							ts->dev_info.slot = ts->ev[it].value;
							ts->dev_info.buf[total][ts->dev_info.slot].slot = ts->ev[it].value;
							ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						}
						break;
					case ABS_X+2:
						if (ts->special_device == EGALAX_VERSION_112) {
							/* this is ABS_Z wrongly used as ABS_X here */
							if (ts->dev_info.mt && ts->dev_info.buf[total][ts->dev_info.slot].valid == 1)
								break;

							ts->dev_info.buf[total][ts->dev_info.slot].x = ts->ev[it].value;
							ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
							ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						}
						break;
					case ABS_Y+2:
						if (ts->special_device == EGALAX_VERSION_112) {
							/* this is ABS_RX wrongly used as ABS_Y here */
							if (ts->dev_info.mt && ts->dev_info.buf[total][ts->dev_info.slot].valid == 1)
								break;

							ts->dev_info.buf[total][ts->dev_info.slot].y = ts->ev[it].value;
							ts->dev_info.buf[total][ts->dev_info.slot].tv = ts->ev[it].time;
							ts->dev_info.buf[total][ts->dev_info.slot].valid = 1;
						}
						break;
					}//EV_ABS
					break;
				
				}//switch type
				if (total == ret)
					break;
			}//for
		}//while
		ret = total;
	} else {
		struct input_event ev_single;
		unsigned char *p = (unsigned char *) &ev_single;
		int len = sizeof(struct input_event);
		while (total < nr) {
			ret = read(ts->fd, p, len);
			if (ret == -1) {
				if (errno == EINTR)
					continue;

				break;
			}
			if (ret < (int)sizeof(struct input_event)) {
				
				p += ret;
				len -= ret;
				continue;
			}
			
			if (ev_single.type == EV_ABS) {
				switch (ev_single.code) {
				case ABS_X:
					if (ev_single.value != 0) {
						samp[total][0].x = ts->dev_info.current_x = ev_single.value;
						samp[total][0].y = ts->dev_info.current_y;
						samp[total][0].pressure = ts->dev_info.current_p;
					} else {
						fprintf(stderr, "ADAYO: dropped x = 0\n");
						continue;
					}
					break;	
				case ABS_Y:
					if (ev_single.value != 0) {
						samp[total][0].x = ts->dev_info.current_x;
						samp[total][0].y = ts->dev_info.current_y = ev_single.value;
						samp[total][0].pressure = ts->dev_info.current_p;
					} else {
						fprintf(stderr, "ADAYO: dropped y = 0\n");
						continue;
					}
					break;
				case ABS_PRESSURE:
					samp[total][0].x = ts->dev_info.current_x;
					samp[total][0].y = ts->dev_info.current_y;
					samp[total][0].pressure = ts->dev_info.current_p = ev_single.value;
					break;
				}
				samp[total][0].tv = ev_single.time;
				samp++;
				total++;
			} else if(ev_single.type == EV_KEY) {//if EV_ABS
				switch (ev_single.code) {
				case BTN_TOUCH:	
				case BTN_LEFT:
					if (ev_single.value == 0) {
						/* pen up */
						samp[total][0].x = 0;
						samp[total][0].y = 0;
						samp[total][0].pressure = 0;
						samp[total][0].tv = ev_single.time;
						total++;
					}
					break;
				}
			} else {
				fprintf(stderr, "ADAYO: Unknown event type %d\n", ev_single.type);
			}
			p = (unsigned char *) &ev_single;
		}//while
		ret = total;
	}//if (ts->dev_info.using_syn)
	
	return ret;
	
}





















