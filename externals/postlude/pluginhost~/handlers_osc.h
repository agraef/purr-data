/* pluginhost~ - A plugin host for Pd
 *
 * Copyright (C) 2012 Jamie Bullock and others
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */


#include "m_pd.h"
#include "ph_common.h"

void handle_osc_debug(const char *path);
void handle_osc_program(ph *x, t_atom *argv, unsigned int i);
void handle_osc_control(ph *x, t_atom *argv, int i);
void handle_osc_midi(ph *x, t_atom *argv, unsigned int i);
void handle_osc_configure(ph *x, t_atom *argv, int i);
void handle_osc_exiting(ph *x, t_atom *argv, int i);
void handle_osc_update(ph *x, t_atom *argv, unsigned int i);

