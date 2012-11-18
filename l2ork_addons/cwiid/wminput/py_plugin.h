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
 *  * added py_wiimote_deinit
 *
 *  2007-06-05 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * refactored to isolate plugin logic
 *
 *  2007-06-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 */

#ifndef PY_PLUGIN_H
#define PY_PLUGIN_H

int py_init(void);
int py_wiimote(cwiid_wiimote_t *wiimote);
void py_wiimote_deinit(void);
void py_deinit(void);
int py_plugin_open(struct plugin *plugin, char *dir);
void py_plugin_close(struct plugin *plugin);
int py_plugin_init(struct plugin *plugin, int id);
int py_plugin_exec(struct plugin *plugin, int mesg_count,
                   union cwiid_mesg mesg[]);
int py_plugin_param_int(struct plugin *plugin, int i, int value);
int py_plugin_param_float(struct plugin *plugin, int i, float value);

#endif
