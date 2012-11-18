/* Copyright (C) 2007 L. Donnie Smith <cwiid@abstrakraft.org>
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
 *  2007-06-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for wmplugin_exec prototype change (&mesg->mesg)
 *  * updated for param interface change (pass pointers)
 *
 *  2007-05-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * use cwiid_get_acc_cal to get acc calibration values
 *
 *  2007-04-24 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for API overhaul
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for libcwiid rename
 *
 *  2007-04-08 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * copied low-pass filter from acc plugin
 *  * initialized params
 *  * added Scale params
 *
 *  2007-03-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * type audit (stdint, const, char booleans)
 *
 *  2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * made global variables static
 */

#include <math.h>

#include "wmplugin.h"

#define PI	3.14159265358979323

static unsigned char info_init = 0;
static struct wmplugin_info info;
static struct wmplugin_data data;

static cwiid_wiimote_t *wiimote;

static struct acc_cal acc_cal;
static int plugin_id;

wmplugin_info_t wmplugin_info;
wmplugin_init_t wmplugin_init;
wmplugin_exec_t wmplugin_exec;
static void process_nunchuk(struct cwiid_nunchuk_mesg *mesg);

static float Roll_Scale = 1.0;
static float Pitch_Scale = 1.0;
static float X_Scale = 1.0;
static float Y_Scale = 1.0;

struct wmplugin_info *wmplugin_info() {
	if (!info_init) {
		info.button_count = 0;
		info.axis_count = 4;
		info.axis_info[0].name = "Roll";
		info.axis_info[0].type = WMPLUGIN_ABS;
		info.axis_info[0].max  =  3141;
		info.axis_info[0].min  = -3141;
		info.axis_info[0].fuzz = 0;
		info.axis_info[0].flat = 0;
		info.axis_info[1].name = "Pitch";
		info.axis_info[1].type = WMPLUGIN_ABS;
		info.axis_info[1].max  =  1570;
		info.axis_info[1].min  = -1570;
		info.axis_info[1].fuzz = 0;
		info.axis_info[1].flat = 0;
		info.axis_info[2].name = "X";
		info.axis_info[2].type = WMPLUGIN_ABS | WMPLUGIN_REL;
		info.axis_info[2].max  =  16;
		info.axis_info[2].min  = -16;
		info.axis_info[2].fuzz = 0;
		info.axis_info[2].flat = 0;
		info.axis_info[3].name = "Y";
		info.axis_info[3].type = WMPLUGIN_ABS | WMPLUGIN_REL;
		info.axis_info[3].max  =  16;
		info.axis_info[3].min  = -16;
		info.axis_info[3].fuzz = 0;
		info.axis_info[3].flat = 0;
		info.param_count = 4;
		info.param_info[0].name = "Roll_Scale";
		info.param_info[0].type = WMPLUGIN_PARAM_FLOAT;
		info.param_info[0].ptr = &Roll_Scale;
		info.param_info[1].name = "Pitch_Scale";
		info.param_info[1].type = WMPLUGIN_PARAM_FLOAT;
		info.param_info[1].ptr = &Pitch_Scale;
		info.param_info[2].name = "X_Scale";
		info.param_info[2].type = WMPLUGIN_PARAM_FLOAT;
		info.param_info[2].ptr = &X_Scale;
		info.param_info[3].name = "Y_Scale";
		info.param_info[3].type = WMPLUGIN_PARAM_FLOAT;
		info.param_info[3].ptr = &Y_Scale;
		info_init = 1;
	}
	return &info;
}

int wmplugin_init(int id, cwiid_wiimote_t *arg_wiimote)
{
	plugin_id = id;
	wiimote = arg_wiimote;
	data.buttons = 0;
	data.axes[0].valid = 1;
	data.axes[1].valid = 1;
	if (wmplugin_set_rpt_mode(id, CWIID_RPT_STATUS | CWIID_RPT_NUNCHUK)) {
		return -1;
	}

	return 0;
}

struct wmplugin_data *wmplugin_exec(int mesg_count, union cwiid_mesg mesg[])
{
	int i;
	enum cwiid_ext_type ext_type = CWIID_EXT_NONE;
	struct wmplugin_data *ret = NULL;

	for (i=0; i < mesg_count; i++) {
		switch (mesg[i].type) {
		case CWIID_MESG_STATUS:
			if ((mesg[i].status_mesg.ext_type == CWIID_EXT_NUNCHUK) &&
			  (ext_type != CWIID_EXT_NUNCHUK)) {
				if (cwiid_get_acc_cal(wiimote, CWIID_EXT_NUNCHUK, &acc_cal)) {
					wmplugin_err(plugin_id, "calibration error");
				}
			}
			ext_type = mesg[i].status_mesg.ext_type;
			break;
		case CWIID_MESG_NUNCHUK:
			process_nunchuk(&mesg[i].nunchuk_mesg);
			ret = &data;
			break;
		default:
			break;
		}
	}

	return ret;
}

#define NEW_AMOUNT 0.1
#define OLD_AMOUNT (1.0-NEW_AMOUNT)
double a_x = 0, a_y = 0, a_z = 0;

static void process_nunchuk(struct cwiid_nunchuk_mesg *mesg)
{
	double a;
	double roll, pitch;

	a_x = (((double)mesg->acc[CWIID_X] - acc_cal.zero[CWIID_X]) /
	      (acc_cal.one[CWIID_X] - acc_cal.zero[CWIID_X]))*NEW_AMOUNT +
	      a_x*OLD_AMOUNT;
	a_y = (((double)mesg->acc[CWIID_Y] - acc_cal.zero[CWIID_Y]) /
	      (acc_cal.one[CWIID_Y] - acc_cal.zero[CWIID_Y]))*NEW_AMOUNT +
	      a_y*OLD_AMOUNT;
	a_z = (((double)mesg->acc[CWIID_Z] - acc_cal.zero[CWIID_Z]) /
	      (acc_cal.one[CWIID_Z] - acc_cal.zero[CWIID_Z]))*NEW_AMOUNT +
	      a_z*OLD_AMOUNT;

	a = sqrt(pow(a_x,2)+pow(a_y,2)+pow(a_z,2));
	roll = atan(a_x/a_z);
	if (a_z <= 0.0) {
		roll += PI * ((a_x > 0.0) ? 1 : -1);
	}

	pitch = atan(a_y/a_z*cos(roll));

	data.axes[0].value = roll  * 1000 * Roll_Scale;
	data.axes[1].value = pitch * 1000 * Pitch_Scale;

	if ((a > 0.85) && (a < 1.15)) {
		if ((fabs(roll)*(180/PI) > 10) && (fabs(pitch)*(180/PI) < 80)) {
			data.axes[2].valid = 1;
			data.axes[2].value = roll * 5 * X_Scale;
		}
		else {
			data.axes[2].valid = 0;
		}
		if (fabs(pitch)*(180/PI) > 10) {
			data.axes[3].valid = 1;
			data.axes[3].value = pitch * 10 * Y_Scale;
		}
		else {
			data.axes[3].valid = 0;
		}
	}
	else {
		data.axes[2].valid = 0;
		data.axes[3].valid = 0;
	}
}

