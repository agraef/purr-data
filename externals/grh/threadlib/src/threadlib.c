/* 
* 
* threadlib
* library for threaded patching in PureData
* Copyright (C) 2005 Georg Holzmann <grh@mur.at>
* heavily based on code by Tim Blechmann
* (detach, join, pd_devel)
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

typedef struct threadlib 
{
  t_object x_obj;
} t_threadlib;

t_class *threadlib_class;

static void threadlib_help(void)
{
  post("\nthreadlib vers."VERSION", library for threaded patching and externals\n"
       "2005, by Georg Holzmann <grh@mur.at>\n"
       "heavily based on pd_devel code by Tim Blechmann\n"

        // help text:
        "\tdetach       run part of the patch in a helper thread\n"
        "\tjoin         synchronize messages to pd's main thread\n"
        "\tsleep        block system for specific time\n"
       
        "WARNING: this is very experimental and can crash your patches !\n");
}

void *threadlib_new(void)
{
  t_threadlib *x = (t_threadlib *)pd_new(threadlib_class);
  return (void *)x;
}

void sleep_setup();
void detach_setup();
void join_setup();

void threadlib_setup(void)
{
  // call all the setup functions:
  sleep_setup();
  detach_setup();
  join_setup();
  
  // init callback system
  h_init_callbacks();

  post("\nthreadlib vers."VERSION", library for threaded patching and externals\n"
       "2005, by Georg Holzmann <grh@mur.at>\n"
       "heavily based on pd_devel code by Tim Blechmann\n"
       "WARNING: this is very experimental and may crash your patches !\n");
  
  threadlib_class = class_new(gensym("threadlib"), threadlib_new, 0, 
			      sizeof(t_threadlib), 0, 0);
  class_addmethod(threadlib_class, (t_method)threadlib_help, gensym("help"), 0);
}
