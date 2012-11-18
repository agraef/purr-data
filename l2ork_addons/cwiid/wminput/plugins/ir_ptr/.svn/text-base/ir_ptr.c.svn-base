/* Copyright (C) 2007 L. Donnie Smith <cwiidabstrakraft.org>
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
 *  2007-06-28 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Converted to single source
 *  * Added deadspace at edges
 *
 *  2007-06-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for wmplugin_exec prototype change (&mesg->mesg)
 *
 *  2007-05-16 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed cwiid_{connect,disconnect,command} to
 *    cwiid_{open,close,request_status|set_led|set_rumble|set_rpt_mode}
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for API overhaul
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for libcwiid rename
 *
 *  2007-04-08 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * initialized param array
 *
 *  2007-04-08 Arthur Peters <amp@singingwizard.org>
 *  * added debounce and low pass filter
 *
 *  2007-03-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * type audit (stdint, const, char booleans)
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * made global variables static
 */

#include "wmplugin.h"

#define DEBOUNCE_THRESHOLD	50
#define NEW_AMOUNT	0.3
#define OLD_AMOUNT	(1.0 - NEW_AMOUNT)
#define X_EDGE	50
#define Y_EDGE	50

cwiid_wiimote_t *wiimote;

static struct wmplugin_info info;
static struct wmplugin_data data;

wmplugin_info_t wmplugin_info;
wmplugin_init_t wmplugin_init;
wmplugin_exec_t wmplugin_exec;

struct wmplugin_info *wmplugin_info() {
	static unsigned char info_init = 0;

	if (!info_init) {
		info.button_count = 0;
		info.axis_count = 2;
		info.axis_info[0].name = "X";
		info.axis_info[0].type = WMPLUGIN_ABS;
		info.axis_info[0].max  = CWIID_IR_X_MAX - X_EDGE;
		info.axis_info[0].min  = X_EDGE;
		info.axis_info[0].fuzz = 0;
		info.axis_info[0].flat = 0;
		info.axis_info[1].name = "Y";
		info.axis_info[1].type = WMPLUGIN_ABS;
		info.axis_info[1].max  = CWIID_IR_Y_MAX - Y_EDGE;
		info.axis_info[1].min  = Y_EDGE;
		info.axis_info[1].fuzz = 0;
		info.axis_info[1].flat = 0;
		info.param_count = 0;
		info_init = 1;
	}
	return &info;
}

int wmplugin_init(int id, cwiid_wiimote_t *arg_wiimote)
{
	wiimote = arg_wiimote;

	data.buttons = 0;

	if (wmplugin_set_rpt_mode(id, CWIID_RPT_IR)) {
		return -1;
	}

	return 0;
}

struct wmplugin_data *wmplugin_exec(int mesg_count, union cwiid_mesg mesg[])
{
	static int src_index = -1;
	static int debounce = 0;
	static uint8_t old_flag;

	int i;
	uint8_t flag;
	struct cwiid_ir_mesg *ir_mesg;

	ir_mesg = NULL;
	for (i=0; i < mesg_count; i++) {
		if (mesg[i].type == CWIID_MESG_IR) {
			ir_mesg = &mesg[i].ir_mesg;
		}
	}

	if (!ir_mesg) {
		return NULL;
	}

	/* invalidate src index if source is no longer present */
	if ((src_index != -1) && !ir_mesg->src[src_index].valid) {
		if (debounce > DEBOUNCE_THRESHOLD) {
			src_index = -1;
		}
		else {
			debounce++;
		}
	}
	else {
		debounce = 0;
	}

	/* of not set, pick largest available source */
	if (src_index == -1) {
		for (i=0; i < CWIID_IR_SRC_COUNT; i++) {
			if (ir_mesg->src[i].valid) {
				if ((src_index == -1) ||
				  (ir_mesg->src[i].size > ir_mesg->src[src_index].size)) {
					src_index = i;
				}
			}
		}
	}

	/* LEDs */
	/* Commented out so it doesn't overide led plugin.
	 * It shouldn't matter, since only one IR source
	 * is used now anyways. If the pointer is moving,
	 * it sees a source.
	switch (src_index) {
	case 0:
		flag = CWIID_LED1_ON;
		break;
	case 1:
		flag = CWIID_LED2_ON;
		break;
	case 2:
		flag = CWIID_LED3_ON;
		break;
	case 3:
		flag = CWIID_LED4_ON;
		break;
	default:
		flag = 0;
		break;
	}

	if (flag != old_flag) {
		cwiid_set_led(wiimote, flag);
		old_flag = flag;
	}
	*/

	if ((src_index == -1) || !ir_mesg->src[src_index].valid) {
		data.axes[0].valid = data.axes[1].valid = 0;
	}
	else {
		data.axes[0].valid = data.axes[1].valid = 1;
		data.axes[0].value = NEW_AMOUNT * (CWIID_IR_X_MAX -
		                         ir_mesg->src[src_index].pos[CWIID_X])
		                   + OLD_AMOUNT * data.axes[0].value;
		data.axes[1].value = NEW_AMOUNT * ir_mesg->src[src_index].pos[CWIID_Y]
		                   + OLD_AMOUNT * data.axes[1].value;

		if (data.axes[0].value > CWIID_IR_X_MAX - X_EDGE) {
			data.axes[0].value = CWIID_IR_X_MAX - X_EDGE;
		}
		else if (data.axes[0].value < X_EDGE) {
			data.axes[0].value = X_EDGE;
		}
		if (data.axes[1].value > CWIID_IR_Y_MAX - Y_EDGE) {
			data.axes[1].value = CWIID_IR_Y_MAX - Y_EDGE;
		}
		else if (data.axes[1].value < Y_EDGE) {
			data.axes[1].value = Y_EDGE;
		}
	}

	return &data;
}
