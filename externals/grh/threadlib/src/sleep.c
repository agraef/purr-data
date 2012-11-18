/* 
* 
* sleep
* like the c function sleep - blocks the system for a specific time
* Copyright (C) 2005 Georg Holzmann <grh@mur.at>
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*/

#include "threadlib.h"

// include for sleep
#ifdef MSW
#include <windows.h>
#define sleep(t) Sleep(1000*(t))
#else
#include <unistd.h>
#endif

static t_class *sleep_class;

typedef struct _sleep
{
  t_object x_obj;
  t_outlet * x_outlet;
} t_sleep;

static void sleep_float(t_sleep * x, t_float f)
{
  int time = (int)(f<0?0:f);
  sleep(time);
  outlet_bang(x->x_outlet);
}

static void *sleep_new(void)
{
  t_sleep *x = (t_sleep *)pd_new(sleep_class);
  x->x_outlet = outlet_new(&x->x_obj,&s_float);
  return (void *)x;
}

void sleep_setup(void)
{
  sleep_class = class_new(gensym("sleep"),
				 (t_newmethod)sleep_new,
				 0, sizeof(t_sleep),
				 CLASS_DEFAULT, 0);

  class_addfloat(sleep_class, sleep_float);
}
