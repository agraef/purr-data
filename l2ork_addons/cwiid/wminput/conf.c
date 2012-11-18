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
 *  2007-07-28 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added config.h include
 *  * added HAVE_PYTHON tests around all python code
 *
 *  2007-06-18 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * revised error messages
 *
 *  2007-06-05 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * refactored to isolate plugin logic
 *
 *  2007-06-03 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * fixed plugin->data malloc bug
 *
 *  2007-06-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added python plugin support
 *  * changed param interface (pass pointers)
 *
 *  2007-04-15 <work.eric@gmail.com>
 *  * fixed classic controller configuration bug
 *
 *  2007-04-08 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * fixed pointer qualifier warning in get_plugin
 *  * created conf_plugin_param_{int,float} functions
 *
 *  2007-03-03 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 *  * type audit (stdint, const, char booleans)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conf.h"
#include "util.h"
#include "y.tab.h"
#include "c_plugin.h"
#ifdef HAVE_PYTHON
#include "py_plugin.h"
#endif

extern FILE *yyin;
extern int yyparse();

extern struct lookup_enum action_enum[];
struct conf *cur_conf;

struct plugin *get_plugin(struct conf *conf, const char *name);

int conf_load(struct conf *conf, const char *conf_name,
              char *config_search_dirs[], char *plugin_search_dirs[])
{
	conf_init(conf);
	cur_conf = conf;

	cur_conf->config_search_dirs = config_search_dirs;
	cur_conf->plugin_search_dirs = plugin_search_dirs;
	if (!(yyin = conf_push_config(cur_conf, conf_name, NULL))) {
		return -1;
	}

	if (yyparse()) {
		if (fclose(yyin)) {
			wminput_err("Error closing configuration file");
		}
		conf_unload(cur_conf);
		return -1;
	}

	if (uinput_open(cur_conf)) {
		conf_unload(cur_conf);
		return -1;
	}

	return 0;
}

int conf_unload(struct conf *conf)
{
	int i;

	for (i=0; i < CONF_MAX_PLUGINS; i++) {
		if (conf->plugins[i].name) {
			free(conf->plugins[i].name);
			switch (conf->plugins[i].type) {
			case PLUGIN_C:
				c_plugin_close(&conf->plugins[i]);
				break;
#ifdef HAVE_PYTHON
			case PLUGIN_PYTHON:
				py_plugin_close(&conf->plugins[i]);
				break;
#endif
			}
		}
	}

	return 0;
}

int conf_ff(struct conf *conf, unsigned char enabled)
{
	if (enabled) {
		conf->ff = 1;
		conf->dev.ff_effects_max = 1;
	}
	else {
		conf->ff = 0;
		conf->dev.ff_effects_max = 0;
	}

	return 0;
}

int conf_button(struct conf *conf, int source, __u16 button, __u16 action)
{
	switch (source) {
	case CONF_WM:
		conf->rpt_mode_flags |= CWIID_RPT_BTN;
		conf->wiimote_bmap[button].active = 1;
		conf->wiimote_bmap[button].action = action;
		break;
	case CONF_NC:
		conf->rpt_mode_flags |= CWIID_RPT_NUNCHUK;
		conf->nunchuk_bmap[button].active = 1;
		conf->nunchuk_bmap[button].action = action;
		break;
	case CONF_CC:
		conf->rpt_mode_flags |= CWIID_RPT_CLASSIC;
		conf->classic_bmap[button].active = 1;
		conf->classic_bmap[button].action = action;
		break;
	}

	return 0;
}

int conf_axis(struct conf *conf, int axis, __u16 axis_type, __u16 action,
              char flags)
{
	conf->amap[axis].active = 1;
	conf->amap[axis].axis_type = axis_type;
	conf->amap[axis].action = action;
	conf->amap[axis].flags = flags;

	if (axis_type == EV_ABS) {
		if (!(conf->dev.absmax[action] == -1) ||
		  !(conf->dev.absmin[action] == -1)) {
			wminput_err("Warning: duplicate absolute axis assignment");
		}
	}

	switch (axis) {
	case CONF_WM_AXIS_DPAD_X:
	case CONF_WM_AXIS_DPAD_Y:
		conf->rpt_mode_flags |= CWIID_RPT_BTN;
		if (axis_type == EV_ABS) {
			conf->dev.absmax[action] = 1;
			conf->dev.absmin[action] = -1;
			conf->dev.absfuzz[action] = 0;
			conf->dev.absflat[action] = 0;
		}
		break;
	case CONF_NC_AXIS_STICK_X:
	case CONF_NC_AXIS_STICK_Y:
		conf->rpt_mode_flags |= CWIID_RPT_NUNCHUK;
		if (axis_type == EV_ABS) {
			conf->dev.absmax[action] = 0xFF;
			conf->dev.absmin[action] = 0;
			conf->dev.absfuzz[action] = 0;
			conf->dev.absflat[action] = 0;
		}
		break;
	case CONF_CC_AXIS_DPAD_X:
	case CONF_CC_AXIS_DPAD_Y:
		conf->rpt_mode_flags |= CWIID_RPT_CLASSIC;
		if (axis_type == EV_ABS) {
			conf->dev.absmax[action] = 1;
			conf->dev.absmin[action] = -1;
			conf->dev.absfuzz[action] = 0;
			conf->dev.absflat[action] = 0;
		}
		break;
	case CONF_CC_AXIS_L_STICK_X:
	case CONF_CC_AXIS_L_STICK_Y:
		conf->rpt_mode_flags |= CWIID_RPT_CLASSIC;
		if (axis_type == EV_ABS) {
			conf->dev.absmax[action] = CWIID_CLASSIC_L_STICK_MAX;
			conf->dev.absmin[action] = 0;
			conf->dev.absfuzz[action] = 0;
			conf->dev.absflat[action] = 0;
		}
		break;
	case CONF_CC_AXIS_R_STICK_X:
	case CONF_CC_AXIS_R_STICK_Y:
		conf->rpt_mode_flags |= CWIID_RPT_CLASSIC;
		if (axis_type == EV_ABS) {
			conf->dev.absmax[action] = CWIID_CLASSIC_R_STICK_MAX;
			conf->dev.absmin[action] = 0;
			conf->dev.absfuzz[action] = 0;
			conf->dev.absflat[action] = 0;
		}
		break;
	case CONF_CC_AXIS_L:
	case CONF_CC_AXIS_R:
		conf->rpt_mode_flags |= CWIID_RPT_CLASSIC;
		if (axis_type == EV_ABS) {
			conf->dev.absmax[action] = CWIID_CLASSIC_LR_MAX;
			conf->dev.absmin[action] = 0;
			conf->dev.absfuzz[action] = 0;
			conf->dev.absflat[action] = 0;
		}
		break;
	}

	return 0;
}

int conf_plugin_button(struct conf *conf, const char *name, const char *button,
                       __u16 action)
{
	struct plugin *plugin;
	int i;
	unsigned char button_found = 0;

	if ((plugin = get_plugin(conf, name)) == NULL) {
		return -1;
	}

	for (i=0; i < plugin->info->button_count; i++) {
		if (!strcmp(plugin->info->button_info[i].name, button)) {
			button_found = 1;
			break;
		}
	}

	if (!button_found) {
		wminput_err("Invalid plugin button: %s.%s", name, button);
		return -1;
	}
	else {
		plugin->bmap[i].active = 1;
		plugin->bmap[i].action = action;
	}

	return 0;
}

int conf_plugin_axis(struct conf *conf, const char *name, const char *axis,
                     __u16 axis_type, __u16 action, char flags)
{
	struct plugin *plugin;
	int i;
	unsigned char axis_found = 0;
	unsigned char mismatch = 0;

	if ((plugin = get_plugin(conf, name)) == NULL) {
		return -1;
	}

	for (i=0; i < plugin->info->axis_count; i++) {
		if (!strcmp(plugin->info->axis_info[i].name, axis)) {
			axis_found = 1;
			break;
		}
	}

	if (!axis_found) {
		wminput_err("Invalid plugin axis: %s.%s", name, axis);
		return -1;
	}

	switch (axis_type) {
	case CONF_ABS:
		if (!(plugin->info->axis_info[i].type & WMPLUGIN_ABS)) {
			mismatch = 1;
		}
		break;
	case CONF_REL:
		if (!(plugin->info->axis_info[i].type & WMPLUGIN_REL)) {
			mismatch = 1;
		}
		break;
	}
	if (mismatch) {
		wminput_err("Warning: axis type mismatch - %s.%s", name, axis);
	}

	plugin->amap[i].active = 1;
	plugin->amap[i].axis_type = axis_type;
	plugin->amap[i].action = action;
	plugin->amap[i].flags = flags;

	if (axis_type == EV_ABS) {
		if (!(conf->dev.absmax[action] == -1) ||
		  !(conf->dev.absmin[action] == -1)) {
			wminput_err("Warning: duplicate absolute axis assignment");
		}

		conf->dev.absmax[action] = plugin->info->axis_info[i].max;
		conf->dev.absmin[action] = plugin->info->axis_info[i].min;
		conf->dev.absfuzz[action] = plugin->info->axis_info[i].fuzz;
		conf->dev.absflat[action] = plugin->info->axis_info[i].flat;
	}

	return 0;
}

int conf_plugin_param_int(struct conf *conf, const char *name,
                          const char *param, int value)
{
	struct plugin *plugin;
	int i;
	unsigned char param_found = 0;

	if ((plugin = get_plugin(conf, name)) == NULL) {
		return -1;
	}

	for (i=0; i < plugin->info->param_count; i++) {
		if (!strcmp(plugin->info->param_info[i].name, param)) {
			param_found = 1;
			break;
		}
	}

	if (!param_found) {
		wminput_err("Invalid plugin parameter: %s.%s", name, param);
		return -1;
	}

	switch (plugin->type) {
	case PLUGIN_C:
		if (c_plugin_param_int(plugin, i, value)) {
			return -1;
		}
		break;
#ifdef HAVE_PYTHON
	case PLUGIN_PYTHON:
		if (py_plugin_param_int(plugin, i, value)) {
			return -1;
		}
		break;
#endif
	}

	return 0;
}

int conf_plugin_param_float(struct conf *conf, const char *name,
                            const char *param, float value)
{
	struct plugin *plugin;
	int i;
	unsigned char param_found = 0;

	if ((plugin = get_plugin(conf, name)) == NULL) {
		return -1;
	}

	for (i=0; i < plugin->info->param_count; i++) {
		if (!strcmp(plugin->info->param_info[i].name, param)) {
			param_found = 1;
			break;
		}
	}

	if (!param_found) {
		wminput_err("Invalid plugin parameter: %s.%s", name, param);
		return -1;
	}

	switch (plugin->type) {
	case PLUGIN_C:
		if (c_plugin_param_float(plugin, i, value)) {
			return -1;
		}
		break;
#ifdef HAVE_PYTHON
	case PLUGIN_PYTHON:
		if (py_plugin_param_float(plugin, i, value)) {
			return -1;
		}
		break;
#endif
	}

	return 0;
}

void conf_init(struct conf *conf)
{
	int i, j;

	conf->fd = -1;
	conf->config_search_dirs = NULL;
	conf->plugin_search_dirs = NULL;
	conf->current_config_filename = NULL;
	conf->stack_index = -1;
	for (i=0; i < CONF_MAX_INCLUDE_DEPTH; i++) {
		conf->config_filename_stack[i] = NULL;
	}
	conf->rpt_mode_flags = 0;
	memset(&conf->dev, 0, sizeof conf->dev);
	strncpy(conf->dev.name, UINPUT_NAME, UINPUT_MAX_NAME_SIZE);
	conf->dev.id.bustype = UINPUT_BUSTYPE;
	conf->dev.id.vendor = UINPUT_VENDOR;
	conf->dev.id.product = UINPUT_PRODUCT;
	conf->dev.id.version = UINPUT_VERSION;
	for (i=0; i < ABS_MAX; i++) {
		conf->dev.absmax[i] = -1;
		conf->dev.absmin[i] = -1;
		conf->dev.absfuzz[i] = -1;
		conf->dev.absflat[i] = -1;
	}
	conf->ff = 0;
	conf->wiimote_bmap[CONF_WM_BTN_UP].mask = CWIID_BTN_UP;
	conf->wiimote_bmap[CONF_WM_BTN_DOWN].mask = CWIID_BTN_DOWN;
	conf->wiimote_bmap[CONF_WM_BTN_LEFT].mask = CWIID_BTN_LEFT;
	conf->wiimote_bmap[CONF_WM_BTN_RIGHT].mask = CWIID_BTN_RIGHT;
	conf->wiimote_bmap[CONF_WM_BTN_A].mask = CWIID_BTN_A;
	conf->wiimote_bmap[CONF_WM_BTN_B].mask = CWIID_BTN_B;
	conf->wiimote_bmap[CONF_WM_BTN_MINUS].mask = CWIID_BTN_MINUS;
	conf->wiimote_bmap[CONF_WM_BTN_PLUS].mask = CWIID_BTN_PLUS;
	conf->wiimote_bmap[CONF_WM_BTN_HOME].mask = CWIID_BTN_HOME;
	conf->wiimote_bmap[CONF_WM_BTN_1].mask = CWIID_BTN_1;
	conf->wiimote_bmap[CONF_WM_BTN_2].mask = CWIID_BTN_2;
	conf->nunchuk_bmap[CONF_NC_BTN_C].mask = CWIID_NUNCHUK_BTN_C;
	conf->nunchuk_bmap[CONF_NC_BTN_Z].mask = CWIID_NUNCHUK_BTN_Z;
	conf->classic_bmap[CONF_CC_BTN_UP].mask = CWIID_CLASSIC_BTN_UP;
	conf->classic_bmap[CONF_CC_BTN_DOWN].mask = CWIID_CLASSIC_BTN_DOWN;
	conf->classic_bmap[CONF_CC_BTN_LEFT].mask = CWIID_CLASSIC_BTN_LEFT;
	conf->classic_bmap[CONF_CC_BTN_RIGHT].mask = CWIID_CLASSIC_BTN_RIGHT;
	conf->classic_bmap[CONF_CC_BTN_MINUS].mask = CWIID_CLASSIC_BTN_MINUS;
	conf->classic_bmap[CONF_CC_BTN_PLUS].mask = CWIID_CLASSIC_BTN_PLUS;
	conf->classic_bmap[CONF_CC_BTN_HOME].mask = CWIID_CLASSIC_BTN_HOME;
	conf->classic_bmap[CONF_CC_BTN_A].mask = CWIID_CLASSIC_BTN_A;
	conf->classic_bmap[CONF_CC_BTN_B].mask = CWIID_CLASSIC_BTN_B;
	conf->classic_bmap[CONF_CC_BTN_X].mask = CWIID_CLASSIC_BTN_X;
	conf->classic_bmap[CONF_CC_BTN_Y].mask = CWIID_CLASSIC_BTN_Y;
	conf->classic_bmap[CONF_CC_BTN_ZL].mask = CWIID_CLASSIC_BTN_ZL;
	conf->classic_bmap[CONF_CC_BTN_ZR].mask = CWIID_CLASSIC_BTN_ZR;
	conf->classic_bmap[CONF_CC_BTN_L].mask = CWIID_CLASSIC_BTN_L;
	conf->classic_bmap[CONF_CC_BTN_R].mask = CWIID_CLASSIC_BTN_R;
	for (i=0; i < CONF_WM_BTN_COUNT; i++) {
		conf->wiimote_bmap[i].active = 0;
	}
	for (i=0; i < CONF_NC_BTN_COUNT; i++) {
		conf->nunchuk_bmap[i].active = 0;
	}
	for (i=0; i < CONF_CC_BTN_COUNT; i++) {
		conf->classic_bmap[i].active = 0;
	}
	for (i=0; i < CONF_AXIS_COUNT; i++) {
		conf->amap[i].active = 0;
		conf->amap[i].axis_type = -1;
		conf->amap[i].action = -1;
		conf->amap[i].flags = 0;
	}
	for (i=0; i < CONF_MAX_PLUGINS; i++) {
		conf->plugins[i].name = NULL;
		conf->plugins[i].rpt_mode_flags = 0;
		conf->plugins[i].prev_buttons = 0;
		for (j=0; j < WMPLUGIN_MAX_BUTTON_COUNT; j++) {
			conf->plugins[i].bmap[j].active = 0;
			conf->plugins[i].bmap[j].mask = 1<<i;
			conf->plugins[i].bmap[j].action = -1;
		}
		for (j=0; j < WMPLUGIN_MAX_AXIS_COUNT; j++) {
			conf->plugins[i].amap[j].active = 0;
			conf->plugins[i].amap[j].axis_type = -1;
			conf->plugins[i].amap[j].action = -1;
			conf->plugins[i].amap[j].flags = 0;
		}
	}
}

#define CONF_PATHNAME_LEN	128
FILE *conf_push_config(struct conf *conf, const char *filename, YYLTYPE *yyloc)
{
	int i;
	FILE *file;
	char pathname[CONF_PATHNAME_LEN];
	const char *stackname;

	if (conf->stack_index+1 >= CONF_MAX_INCLUDE_DEPTH) {
		wminput_err("Maximum include depth exceeded: %s", filename);
		return NULL;
	}

	/* filename == / or ./ or ../ */
	if ((filename[0] == '/') ||
	    ((filename[0] == '.') &&
	      ((filename[1] == '/') ||
	      ((filename[1] == '.') && (filename[2] == '/'))))) {
		stackname = filename;
		file = fopen(filename, "r");
	}
	else {
		for (i=0; conf->config_search_dirs[i]; i++) {
			snprintf(pathname, CONF_PATHNAME_LEN, "%s/%s",
			         conf->config_search_dirs[i], filename);
			if ((file = fopen(pathname, "r"))) {
				stackname = pathname;
				break;
			}
		}
	}

	if (!file) {
		wminput_err("File not found: %s", filename);
		return NULL;
	}

	conf->stack_index++;
	if ((conf->config_filename_stack[conf->stack_index] =
	  malloc(strlen(stackname) + 1)) == NULL) {
		wminput_err("Error allocating pathname");
		conf->stack_index--;
		return NULL;
	}
	strcpy(conf->config_filename_stack[conf->stack_index], stackname);
	if (yyloc) {
		conf->yyloc_stack[conf->stack_index] = *yyloc;
	}

	conf->current_config_filename =
		conf->config_filename_stack[conf->stack_index];

	return file;
}

int conf_pop_config(struct conf *conf, YYLTYPE *yyloc)
{
	if (conf->stack_index == -1) {
		return -1;
	}

	free(conf->config_filename_stack[conf->stack_index]);
	*yyloc = conf->yyloc_stack[conf->stack_index];

	conf->stack_index--;

	conf->current_config_filename =
		conf->config_filename_stack[conf->stack_index];

	return 0;
}

int lookup_action(const char *str_action)
{
	int i=0;

	while (action_enum[i].name) {
		if (!strcmp(str_action, action_enum[i].name)) {
			return action_enum[i].value;
		}
		i++;
	}

	return -1;
}

struct plugin *get_plugin(struct conf *conf, const char *name)
{
	int i;
	char plugin_found = 0;
	struct plugin *plugin;

	for (i=0; i < CONF_MAX_PLUGINS; i++) {
		if (!conf->plugins[i].name) {
			break;
		}
		else if (!strcmp(conf->plugins[i].name, name)) {
			return &conf->plugins[i];
		}
	}

	if (i == CONF_MAX_PLUGINS) {
		wminput_err("Maximum number of plugins exceeded");
		return NULL;
	}

	plugin = &conf->plugins[i];
	plugin->name = (char *)name;

	for (i=0; conf->plugin_search_dirs[i]; i++) {
		if (!c_plugin_open(plugin, conf->plugin_search_dirs[i])) {
			plugin_found = 1;
			break;
		}
#ifdef HAVE_PYTHON
		if (!py_plugin_open(plugin, conf->plugin_search_dirs[i])) {
			plugin_found = 1;
			break;
		}
#endif
	}

	if (!plugin_found) {
		wminput_err("Plugin not found: %s", name);
		free(plugin->name);
		plugin->name = NULL;
		return NULL;
	}

	return plugin;
}
