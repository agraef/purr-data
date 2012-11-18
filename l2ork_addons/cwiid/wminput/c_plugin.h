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

#ifndef C_PLUGIN_H
#define C_PLUGIN_H

int c_init(void);
int c_wiimote(cwiid_wiimote_t *wiimote);
void c_wiimote_deinit(void);
void c_deinit(void);
int c_plugin_open(struct plugin *plugin, char *dir);
void c_plugin_close(struct plugin *plugin);
int c_plugin_init(struct plugin *plugin, int id);
int c_plugin_exec(struct plugin *plugin, int mesg_count,
                   union cwiid_mesg mesg[]);
int c_plugin_param_int(struct plugin *plugin, int i, int value);
int c_plugin_param_float(struct plugin *plugin, int i, float value);

#endif

