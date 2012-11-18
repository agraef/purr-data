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
 *  2007-06-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed wmplugin_exec prototype (pass mesg instead of &mesg)
 *  * changed param interface (pass pointers)
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * updated for libcwiid rename
 *
 *  2007-04-08 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added param structs
 *
 *  2007-03-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#ifndef WMPLUGIN_H
#define WMPLUGIN_H

#include <stdint.h>
#include <linux/input.h>
#include <cwiid.h>

#define WMPLUGIN_MAX_BUTTON_COUNT	16
#define WMPLUGIN_MAX_AXIS_COUNT		6
#define WMPLUGIN_MAX_PARAM_COUNT	16

#define WMPLUGIN_ABS	1
#define WMPLUGIN_REL	2

struct wmplugin_button_info {
	char *name;
};

struct wmplugin_axis_info {
	char *name;
	__u16 type;
	int max;
	int min;
	int fuzz;
	int flat;
};

enum wmplugin_param_type {
	WMPLUGIN_PARAM_INT,
	WMPLUGIN_PARAM_FLOAT
};

struct wmplugin_param_info {
	char *name;
	enum wmplugin_param_type type;
	void *ptr;
};

struct wmplugin_info {
	unsigned char button_count;
	struct wmplugin_button_info button_info[WMPLUGIN_MAX_BUTTON_COUNT];
	unsigned char axis_count;
	struct wmplugin_axis_info axis_info[WMPLUGIN_MAX_AXIS_COUNT];
	unsigned char param_count;
	struct wmplugin_param_info param_info[WMPLUGIN_MAX_PARAM_COUNT];
};

struct wmplugin_axis {
	char valid;
	__s32 value;
};

struct wmplugin_data {
	uint16_t buttons;
	struct wmplugin_axis axes[WMPLUGIN_MAX_AXIS_COUNT];
};

typedef struct wmplugin_info *wmplugin_info_t(void);
typedef int wmplugin_init_t(int, cwiid_wiimote_t *);
typedef struct wmplugin_data *wmplugin_exec_t(int, union cwiid_mesg []);

int wmplugin_set_rpt_mode(int id, uint8_t rpt_mode);
void wmplugin_err(int id, char *str, ...);

#endif
