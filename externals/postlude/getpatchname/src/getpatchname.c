/* getpatchname - Returns the filename of the current patch 
 * 
 * Copyright (C) 2006 Jamie Bullock 
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "m_pd.h"
#include "g_canvas.h"

static t_class *getpatchname_class;

typedef struct _getpatchname {
  t_object  x_obj;
  t_symbol *patch_name;
  t_outlet *outlet;
  t_canvas *x_canvas;
  int x_level;
} t_getpatchname;

void getpatchname_bang(t_getpatchname *x)
{
/* At some point we need to be to get the new patch name if it changes, couldn't make this work though */
    int i = x->x_level;
    t_canvas* last = x->x_canvas;
    
    while (i>0) {
        i--;
        if (last->gl_owner) last = last->gl_owner;
    }
    
    outlet_symbol(x->outlet, last->gl_name);
}

void *getpatchname_new(t_floatarg level)
{
  t_getpatchname *x = (t_getpatchname *)pd_new(getpatchname_class);
  x->patch_name = canvas_getcurrent()->gl_name;
  x->x_canvas = canvas_getcurrent();
  x->outlet = outlet_new(&x->x_obj, &s_symbol);
  x->x_level = level;
  return (void *)x;
}

void getpatchname_setup(void) {
  getpatchname_class = class_new(gensym("getpatchname"),
        (t_newmethod)getpatchname_new,
        0, sizeof(t_getpatchname), 0,
        A_DEFFLOAT, 0);
  class_addbang(getpatchname_class, getpatchname_bang);
}
