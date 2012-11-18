/* Copyright (C) 2007 L. Donnie Smith <wiimote@abstrakraft.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ChangeLog:
 *  2007-05-16 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed cwiid_{connect,disconnect,command} to
 *    cwiid_{open,close,request_status|set_led|set_rumble|set_rpt_mode}
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for libcwiid rename
 *
 *  2007-04-08 L. Donnie Smith <cwiid@anstrakraft.org>
 *  * fixed signed/unsigned comparison warning in uinput_open
 *
 *  2007-03-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#include <stdint.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include "conf.h"
#include "util.h"

/* UInput */
char *uinput_filename[] = {"/dev/uinput", "/dev/input/uinput",
                           "/dev/misc/uinput"};
#define UINPUT_FILENAME_COUNT (sizeof(uinput_filename)/sizeof(char *))

int uinput_open(struct conf *conf)
{
	unsigned int i;
	int j;
	int request;

	/* Open uinput device */
	for (i=0; i < UINPUT_FILENAME_COUNT; i++) {
		if ((conf->fd = open(uinput_filename[i], O_RDWR)) >= 0) {
			break;
		}
	}
	if (conf->fd < 0) {
		wminput_err("unable to open uinput");
		return -1;
	}

	if (write(conf->fd, &conf->dev, sizeof conf->dev) != sizeof conf->dev) {
		wminput_err("error on uinput device setup");
		close(conf->fd);
		return -1;
	}

	if (conf->ff) {
		if (ioctl(conf->fd, UI_SET_EVBIT, EV_FF) < 0) {
			wminput_err("error on uinput ioctl");
			close(conf->fd);
			return -1;
		}
		if (ioctl(conf->fd, UI_SET_FFBIT, FF_RUMBLE) < 0) {
			wminput_err("error on uinput ioctl");
			close(conf->fd);
			return -1;
		}
	}

	if (ioctl(conf->fd, UI_SET_EVBIT, EV_KEY) < 0) {
		wminput_err("error on uinput ioctl");
		close(conf->fd);
		return -1;
	}	

	for (i=0; i < CONF_WM_BTN_COUNT; i++) {
		if (conf->wiimote_bmap[i].active) {
			if (ioctl(conf->fd, UI_SET_KEYBIT, conf->wiimote_bmap[i].action)
			  < 0) {
				wminput_err("error on uinput ioctl");
				close(conf->fd);
				return -1;
			}
		}			
	}
	for (i=0; i < CONF_NC_BTN_COUNT; i++) {
		if (conf->nunchuk_bmap[i].active) {
			if (ioctl(conf->fd, UI_SET_KEYBIT, conf->nunchuk_bmap[i].action)
			  < 0) {
				wminput_err("error on uinput ioctl");
				close(conf->fd);
				return -1;
			}
		}			
	}
	for (i=0; i < CONF_CC_BTN_COUNT; i++) {
		if (conf->classic_bmap[i].active) {
			if (ioctl(conf->fd, UI_SET_KEYBIT, conf->classic_bmap[i].action)
			  < 0) {
				wminput_err("error on uinput ioctl");
				close(conf->fd);
				return -1;
			}
		}			
	}
	for (i=0; i < CONF_AXIS_COUNT; i++) {
		if (conf->amap[i].active) {
			if (ioctl(conf->fd, UI_SET_EVBIT, conf->amap[i].axis_type) < 0) {
				wminput_err("error uinput ioctl");
				close(conf->fd);
				return -1;
			}
			request = (conf->amap[i].axis_type == EV_ABS) ? UI_SET_ABSBIT
			                                              : UI_SET_RELBIT;
			if (ioctl(conf->fd, request, conf->amap[i].action) < 0) {
				wminput_err("error uinput ioctl");
				close(conf->fd);
				return -1;
			}
			if ((conf->amap[i].flags & CONF_POINTER) &&
			  (conf->amap[i].axis_type == EV_ABS) &&
			  ((conf->amap[i].action == ABS_X) ||
			   (conf->amap[i].action == ABS_Y) ||
			   (conf->amap[i].action == ABS_Z))) {
				if (ioctl(conf->fd, UI_SET_EVBIT, EV_REL) < 0) {
					wminput_err("error uinput ioctl");
					close(conf->fd);
					return -1;
				}
				if (ioctl(conf->fd, UI_SET_RELBIT, conf->amap[i].action) < 0) {
					wminput_err("error uinput ioctl");
					close(conf->fd);
					return -1;
				}
			}
		}
	}
	for (i=0; i < CONF_MAX_PLUGINS; i++) {
		if (conf->plugins[i].name) {
			for (j=0; j < conf->plugins[i].info->button_count; j++) {
				if (conf->plugins[i].bmap[j].active) {
					if (ioctl(conf->fd, UI_SET_KEYBIT,
					          conf->plugins[i].bmap[j].action) < 0) {
						wminput_err("error on uinput ioctl");
						close(conf->fd);
						return -1;
					}
				}
			}
			for (j=0; j < conf->plugins[i].info->axis_count; j++) {
				if (conf->plugins[i].amap[j].active) {
					if (ioctl(conf->fd, UI_SET_EVBIT,
					          conf->plugins[i].amap[j].axis_type) < 0) {
						wminput_err("error uinput ioctl");
						close(conf->fd);
						return -1;
					}
					request = (conf->plugins[i].amap[j].axis_type == EV_ABS)
					          ? UI_SET_ABSBIT
					          : UI_SET_RELBIT;
					if (ioctl(conf->fd, request,
					          conf->plugins[i].amap[j].action) < 0) {
						wminput_err("error uinput ioctl");
						close(conf->fd);
						return -1;
					}
					if ((conf->plugins[i].amap[j].flags & CONF_POINTER) &&
					  (conf->plugins[i].amap[j].axis_type == EV_ABS) &&
					  ((conf->plugins[i].amap[j].action == ABS_X) ||
					   (conf->plugins[i].amap[j].action == ABS_Y) ||
					   (conf->plugins[i].amap[j].action == ABS_Z))) {
						if (ioctl(conf->fd, UI_SET_EVBIT, EV_REL) < 0) {
							wminput_err("error uinput ioctl");
							close(conf->fd);
							return -1;
						}
						if (ioctl(conf->fd, UI_SET_RELBIT,
						          conf->plugins[i].amap[j].action) < 0) {
							wminput_err("error uinput ioctl");
							close(conf->fd);
							return -1;
						}
					}
				}
			}
		}
	}		

	if (ioctl(conf->fd, UI_DEV_CREATE) < 0) {
		wminput_err("Error on uinput dev create");
		close(conf->fd);
		return -1;
	}

	return 0;
}

int uinput_close(struct conf *conf)
{
	if (close(conf->fd)) {
		wminput_err("Error on uinput close");
		return -1;
	}

	return 0;
}

int send_event(struct conf *conf, __u16 type, __u16 code, __s32 value)
{
	struct input_event event;

	memset(&event, 0, sizeof(event));
	event.type = type;
	event.code = code;
	event.value = value;

	if (write(conf->fd, &event, sizeof(event)) != sizeof(event)) {
		wminput_err("Error on send_event");
		return -1;
	}

	return 0;
}

void *uinput_listen(struct uinput_listen_data *data)
{
	size_t len;
	struct input_event event;
	struct uinput_ff_upload upload;
	struct uinput_ff_erase erase;

	do {
		if ((len = read(data->conf->fd, &event, sizeof event)) !=
		  sizeof event) {
			wminput_err("Error on uinput read");
			continue;
		}

		switch (event.type) {
		case EV_UINPUT:
			switch (event.code) {
			case UI_FF_UPLOAD:
				erase.request_id = event.value;
				if (ioctl(data->conf->fd, UI_BEGIN_FF_UPLOAD, &upload) < 0) {
					wminput_err("Error on ff upload begin");
				}
				if (cwiid_set_rumble(data->wiimote, 1)) {
					wminput_err("Error setting rumble");
				}
				if (ioctl(data->conf->fd, UI_END_FF_UPLOAD, &upload) < 0) {
					wminput_err("Error on ff upload end");
				}
				break;
			case UI_FF_ERASE:
				erase.request_id = event.value;
				if (ioctl(data->conf->fd, UI_BEGIN_FF_ERASE, &erase) < 0) {
					wminput_err("Error on ff erase begin");
				}
				if (cwiid_set_rumble(data->wiimote, 0)) {
					wminput_err("Error clearing rumble");
				}
				if (ioctl(data->conf->fd, UI_END_FF_ERASE, &erase) < 0) {
					wminput_err("Error on ff erase end");
				}
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	} while (-1);
}
