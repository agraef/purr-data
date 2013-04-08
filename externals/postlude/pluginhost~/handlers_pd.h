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

void handle_pd_bang(ph *x);
void handle_pd_info(ph *x);
void handle_pd_listplugins(ph *x);
void handle_pd_dsp(ph *x, t_signal **sp);
void handle_pd_reset(ph *x, t_float i);
void handle_pd_list(ph *x, t_symbol *s, int argc, t_atom *argv);
void handle_pd_dssi(ph *x, t_symbol *s, int argc, t_atom *argv); 
void handle_pd_control (ph *x, t_symbol* ctrl_name, t_float ctrl_value, 
        t_float instance);
void handle_pd_plug(ph *x, t_symbol *s, int argc, t_atom *argv);
void handle_pd_osc(ph *x, t_symbol *s, int argc, t_atom *argv);
