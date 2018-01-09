#include "tsparse.h"

/*	function	*/
void set_pressure(TS_DEV *ts)
{
	int j, k;

	ts->dev_info.current_p = 255;

	if (ts->dev_info.mt && ts->dev_info.buf) {
		for (j = 0; j < ts->dev_info.nr; j++) {
			for (k = 0; k < ts->dev_info.max_slots; k++)
				ts->dev_info.buf[j][k].pressure = 255;
		}
	}
}

int check_fd(TS_DEV *ts)
{
	int version;
	long evbit[BITS_TO_LONGS(EV_CNT)];
	long absbit[BITS_TO_LONGS(ABS_CNT)];
	long keybit[BITS_TO_LONGS(KEY_CNT)];
	long synbit[BITS_TO_LONGS(SYN_CNT)];
	
	if (ioctl(ts->fd, EVIOCGVERSION, &version) < 0) {
		fprintf(stderr,
			"tslib: Selected device is not a Linux input event device\n");
		return -1;
	}
	
	/* support version EV_VERSION 0x010000 and 0x010001
	 * this check causes more troubles than it solves here
	 */
	if (version < EV_VERSION)
		fprintf(stderr,
			"tslib: Warning: Selected device uses a different version of the event protocol than tslib was compiled for\n");
			
	if ((ioctl(ts->fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) ||
		!(evbit[BIT_WORD(EV_ABS)] & BIT_MASK(EV_ABS)) ||
		!(evbit[BIT_WORD(EV_KEY)] & BIT_MASK(EV_KEY))) {
		fprintf(stderr,
			"tslib: Selected device is not a touchscreen (must support ABS and KEY event types)\n");
		return -1;
	}
	
	if ((ioctl(ts->fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit)) < 0 ||
	    !(absbit[BIT_WORD(ABS_X)] & BIT_MASK(ABS_X)) ||
	    !(absbit[BIT_WORD(ABS_Y)] & BIT_MASK(ABS_Y))) {
		if (!(absbit[BIT_WORD(ABS_MT_POSITION_X)] & BIT_MASK(ABS_MT_POSITION_X)) ||
		    !(absbit[BIT_WORD(ABS_MT_POSITION_Y)] & BIT_MASK(ABS_MT_POSITION_Y))) {
			fprintf(stderr,
				"tslib: Selected device is not a touchscreen (must support ABS_X/Y or ABS_MT_POSITION_X/Y events)\n");
			return -1;
		}
	}
	
	if (ioctl(ts->fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) {
		fprintf(stderr, "tslib: ioctl EVIOCGBIT error)\n");
		return -1;
	}
	
	if (evbit[BIT_WORD(EV_SYN)] & BIT_MASK(EV_SYN))
		ts->dev_info.using_syn = 1;
	
	/* read device info and set special device nr */
	//get_special_device(i);----------------------------------------------rxhu delete
	
	/* Remember whether we have a multitouch device. We need to know for ABS_X,
	 * ABS_Y and ABS_PRESSURE data.
	 */
	if ((absbit[BIT_WORD(ABS_MT_POSITION_X)] & BIT_MASK(ABS_MT_POSITION_X)) &&
	    (absbit[BIT_WORD(ABS_MT_POSITION_Y)] & BIT_MASK(ABS_MT_POSITION_Y)))
		ts->dev_info.mt = 1;
	
	/* Since some touchscreens (eg. infrared) physically can't measure pressure,
	 * the input system doesn't report it on those. Tslib relies on pressure, thus
	 * we set it to constant 255. It's still controlled by BTN_TOUCH/BTN_LEFT -
	 * when not touched, the pressure is forced to 0.
	 */
	if (!(absbit[BIT_WORD(ABS_PRESSURE)] & BIT_MASK(ABS_PRESSURE)))
		ts->dev_info.no_pressure = 1;
	
	if (ts->dev_info.mt) {
		if (!(absbit[BIT_WORD(ABS_MT_PRESSURE)] & BIT_MASK(ABS_MT_PRESSURE)))
			ts->dev_info.no_pressure = 1;
		else
			ts->dev_info.no_pressure = 0;
	}
	
	if ((ioctl(ts->fd, EVIOCGBIT(EV_SYN, sizeof(synbit)), synbit)) == -1)
		fprintf(stderr, "tslib: ioctl error\n");
	
	/* remember whether we have a multitouch type A device */
	if (ts->dev_info.mt && synbit[BIT_WORD(SYN_MT_REPORT)] & BIT_MASK(SYN_MT_REPORT) &&
	    !(absbit[BIT_WORD(ABS_MT_SLOT)] & BIT_MASK(ABS_MT_SLOT)))
		ts->dev_info.type_a = 1;
	
	if (!(keybit[BIT_WORD(BTN_TOUCH)] & BIT_MASK(BTN_TOUCH) ||
	      keybit[BIT_WORD(BTN_LEFT)] & BIT_MASK(BTN_LEFT)) && ts->dev_info.type_a != 1) {
		fprintf(stderr,
			"tslib: Selected device is not a touchscreen (missing BTN_TOUCH or BTN_LEFT)\n");
		return -1;
	}
	
	if (ts->dev_info.grab_events == GRAB_EVENTS_WANTED) {
		if (ioctl(ts->fd, EVIOCGRAB, (void *)1)) {
			fprintf(stderr,
				"tslib: Unable to grab selected input device\n");
			return -1;
		}
		ts->dev_info.grab_events = GRAB_EVENTS_ACTIVE;
	}

	return ts->fd;
}

int ts_input_read(TS_DEV *ts,struct ts_sample *samp,int nr)
{
	struct 					input_event 		ev;
	int 					ret = nr;
	int 					total = 0;
	int 					pen_up = 0;
	
	ts->fd 	= 			check_fd(ts);
	
	if (ts->fd == -1)
		return -1;
	
	if (ts->dev_info.no_pressure)
		set_pressure(ts);
	//printf("ts->dev_info.using_syn = %d\n",ts->dev_info.using_syn);//add
	if (ts->dev_info.using_syn) {
		while (total < nr) {
			ret = read(ts->fd, &ev, sizeof(struct input_event));
			if (ret < (int)sizeof(struct input_event)) {
				total = -1;
				break;
			}
			switch (ev.type) {
			case EV_KEY:
				switch (ev.code) {
				case BTN_TOUCH:
				case BTN_LEFT:
					if (ev.value == 0)
						pen_up = 1;
					break;
				}
				break;
			case EV_SYN:
				if (ev.code == SYN_REPORT) {
					/* Fill out a new complete event */
					if (pen_up) {
						//samp->x = 0;
						//samp->y = 0;
						samp->x = ts->dev_info.current_x;
						samp->y = ts->dev_info.current_y;
						samp->pressure = 0;
						pen_up = 0;
					} else {
						samp->x = ts->dev_info.current_x;
						samp->y = ts->dev_info.current_y;
						samp->pressure = ts->dev_info.current_p;
					}
					samp->tv = ev.time;
					samp++;
					total++;
				} else if (ev.code == SYN_MT_REPORT) {
						if (!ts->dev_info.type_a)
							break;
						if (ts->dev_info.type_a == 1) { /* no data: pen-up */
							pen_up = 1;
						} else {
						ts->dev_info.type_a = 1;
					}
				}
				break;
				case EV_ABS:
					if (ts->special_device == EGALAX_VERSION_112) {
						switch (ev.code) {
						case ABS_X+2:
							ts->dev_info.current_x = ev.value;
							break;
						case ABS_Y+2:
							ts->dev_info.current_y = ev.value;
							break;
						case ABS_PRESSURE:
							ts->dev_info.current_p = ev.value;
							break;
						}
					} else if (ts->special_device == EGALAX_VERSION_210) {
						switch (ev.code) {
						case ABS_X:
							ts->dev_info.current_x = ev.value;
							break;
						case ABS_Y:
							ts->dev_info.current_y = ev.value;
							break;
						case ABS_PRESSURE:
							ts->dev_info.current_p = ev.value;
							break;
						case ABS_MT_DISTANCE:
							if (ev.value > 0)
								ts->dev_info.current_p = 0;
							else
								ts->dev_info.current_p = 255;
							break;
						}
					} else {
						//printf("-------------------------------------------------------ev.code = %d\n",ev.code);
						switch (ev.code) {
						case ABS_X:
							ts->dev_info.current_x = ev.value;
						//	printf("-------------------------------------------------------ts->dev_info.current_x = %d\n",ts->dev_info.current_x);
							break;
						case ABS_Y:
							ts->dev_info.current_y = ev.value;
							//printf("-------------------------------------------------------ts->dev_info.current_y = %d\n",ts->dev_info.current_x);
							break;
						case ABS_MT_POSITION_X:
							ts->dev_info.current_x = ev.value;
							ts->dev_info.type_a++;
							break;
						case ABS_MT_POSITION_Y:
							ts->dev_info.current_y = ev.value;
							ts->dev_info.type_a++;
							break;
						case ABS_PRESSURE:
							ts->dev_info.current_p = ev.value;
							//printf("-------------------------------------------------------ts->dev_info.current_p = %d\n",ts->dev_info.current_x);
							break;
						}
					}
				break;
			}
		}
#if 0
		printf("ts->dev_info.current_x = %d ts->dev_info.current_y = %d ts->dev_info.current_p = %d\n", \
			ts->dev_info.current_x,ts->dev_info.current_y,ts->dev_info.current_p);
#endif
		ret = total;
	} else {
		unsigned char *p = (unsigned char *) &ev;
		int len = sizeof(struct input_event);
		while (total < nr) {
			ret = read(ts->fd, p, len);
			if (ret == -1) {
				if (errno == EINTR)
					continue;

				break;
			}
			
			if (ret < (int)sizeof(struct input_event)) {
				/* short read
				 * restart read to get the rest of the event
				 */
				p += ret;
				len -= ret;
				continue;
			}
			/* successful read of a whole event */
			if (ev.type == EV_ABS) {
				switch (ev.code) {
				case ABS_X:
					if (ev.value != 0) {
						samp->x = ts->dev_info.current_x = ev.value;
						samp->y = ts->dev_info.current_y;
						samp->pressure = ts->dev_info.current_p;
					} else {
						fprintf(stderr,
							"tslib: dropped x = 0\n");
						continue;
					}
					break;
				case ABS_Y:
					if (ev.value != 0) {
						samp->x = ts->dev_info.current_x;
						samp->y = ts->dev_info.current_y = ev.value;
						samp->pressure = ts->dev_info.current_p;
					} else {
						fprintf(stderr,
							"tslib: dropped y = 0\n");
						continue;
					}
					break;
				case ABS_PRESSURE:
					samp->x = ts->dev_info.current_x;
					samp->y = ts->dev_info.current_y;
					samp->pressure = ts->dev_info.current_p = ev.value;
					break;
				}
					samp->tv = ev.time;
					samp++;
					total++;
				} else if (ev.type == EV_KEY) {
					switch (ev.code) {
						case BTN_TOUCH:
						case BTN_LEFT:
							if (ev.value == 0) {
							/* pen up */
								samp->x = 0;
								samp->y = 0;
								samp->pressure = 0;
								samp->tv = ev.time;
								samp++;
								total++;
							}
							break;
					}
				} else {
					fprintf(stderr,
					"tslib: Unknown event type %d\n",
					ev.type);
				}
				p = (unsigned char *) &ev;
		}
		ret = total;
	}
	return ret;
}

TS_DEV *ts_open(const char *name)
{
	TS_DEV * ts_dev = malloc(sizeof(TS_DEV));
	if (ts_dev) {
		memset(ts_dev, 0, sizeof(TS_DEV));
		ts_dev->fd = open(name, O_RDONLY);
		if (ts_dev->fd == -1)
			goto free;
	}
	return ts_dev;
free:
	free(ts_dev);
	return NULL;
}

void ts_close(TS_DEV * ts)
{
	close(ts->fd);
	free(ts);
}

