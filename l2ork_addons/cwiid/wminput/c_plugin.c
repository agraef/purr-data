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
 *  2007-08-14 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * added c_wiimote_deinit
 *
 *  2007-06-05 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 */

#include <stdlib.h>

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cwiid.h"
#include "conf.h"
#include "util.h"
#include "wmplugin.h"

struct c_plugin {
	void *handle;
	wmplugin_init_t *init;
	wmplugin_exec_t *exec;
};

static cwiid_wiimote_t *wiimote;

int c_init(void)
{
	return 0;
}

int c_wiimote(cwiid_wiimote_t *arg_wiimote)
{
	wiimote = arg_wiimote;

	return 0;
}

void c_wiimote_deinit()
{
	return;
}

void c_deinit(void)
{
	return;
}

#define PLUGIN_PATHNAME_LEN	128
int c_plugin_open(struct plugin *plugin, char *dir)
{
	char pathname[PLUGIN_PATHNAME_LEN];
	struct stat buf;
	wmplugin_info_t *info;
	void *handle;

	snprintf(pathname, PLUGIN_PATHNAME_LEN, "%s/%s.so", dir, plugin->name);
	if (stat(pathname, &buf)) {
		return -1;
	}
	if (!(handle = dlopen(pathname, RTLD_NOW))) {
		wminput_err(dlerror());
		return -1;
	}

	plugin->type = PLUGIN_C;
	if (!(plugin->p = malloc(sizeof(struct c_plugin)))) {
		wminput_err("malloc error");
		return -1;
	}
	if (!(info = dlsym(handle, "wmplugin_info"))) {
		wminput_err("Unable to load plugin info function: %s",
		            dlerror());
		free(plugin->p);
		dlclose(handle);
		return -1;
	}
	if (!(plugin->info = (*(wmplugin_info_t *)info)())) {
		wminput_err("Invalid plugin info from %s", plugin->name);
		free(plugin->p);
		dlclose(handle);
		return -1;
	}
	if (!(((struct c_plugin *)plugin->p)->init = dlsym(handle,
	                                                   "wmplugin_init"))) {
		wminput_err("Unable to load plugin init function: %s", dlerror());
		free(plugin->p);
		dlclose(handle);
		return -1;
	}
	if (!(((struct c_plugin *)plugin->p)->exec = dlsym(handle,
	                                                   "wmplugin_exec"))) {
		wminput_err("Unable to load plugin exec function: %s", dlerror());
		free(plugin->p);
		dlclose(handle);
		return -1;
	}

	((struct c_plugin *)plugin->p)->handle = handle;

	return 0;
}

void c_plugin_close(struct plugin *plugin)
{
	dlclose(((struct c_plugin *)plugin->p)->handle);
	free(plugin->p);
}

int c_plugin_init(struct plugin *plugin, int id)
{
	return ((struct c_plugin *)plugin->p)->init(id, wiimote);
}

int c_plugin_exec(struct plugin *plugin, int mesg_count,
                   union cwiid_mesg mesg[])
{
	if (!(plugin->data = ((struct c_plugin *)plugin->p)->exec(mesg_count,
	                                                          mesg))) {
		return -1;
	}

	return 0;
}

int c_plugin_param_int(struct plugin *plugin, int i, int value)
{
	switch (plugin->info->param_info[i].type) {
	case WMPLUGIN_PARAM_INT:
		*(int *)plugin->info->param_info[i].ptr = value;
		break;
	case WMPLUGIN_PARAM_FLOAT:
		*(float *)plugin->info->param_info[i].ptr = value;
		break;
	}

	return 0;
}

int c_plugin_param_float(struct plugin *plugin, int i, float value)
{
	switch (plugin->info->param_info[i].type) {
	case WMPLUGIN_PARAM_INT:
		wminput_err("possible loss of precision: %s.%s (cast float to int)",
		            plugin->name, plugin->info->param_info[i].name);
		*(int *)plugin->info->param_info[i].ptr = value;
		break;
	case WMPLUGIN_PARAM_FLOAT:
		*(float *)plugin->info->param_info[i].ptr = value;
		break;
	}

	return 0;
}

void wmplugin_err(int id, char *str, ...)
{
	va_list ap;

	va_start(ap, str);
	vfprintf(stderr, str, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}
