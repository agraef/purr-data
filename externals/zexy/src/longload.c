/* 
 * longload: takes a long time to load
 *
 * (c) 1999-2011 IOhannes m zmölnig, forum::für::umläute, institute of electronic music and acoustics (iem)
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "zexy.h"


/* ------------------------- longload ------------------------------- */
#ifdef _WIN32
# include <windows.h>
#else
# include <unistd.h>
#endif


static t_class *longload_class;

typedef struct _longload
{
  t_object x_obj;
} t_longload;

int millisleep(unsigned int milli) {
#ifdef _WIN32
  Sleep(milli);
#else
  usleep(milli*1000);
#endif
  return 0;
}

static void *longload_new(t_float f)
{
  t_longload *x = (t_longload *)pd_new(longload_class);
  if(f>0.f)
    millisleep(f);
  else
    millisleep(1000);
  return (x);
}

void longload_setup(void)
{
  longload_class = class_new(gensym("longload"), 
                             (t_newmethod)longload_new, 
                             0, 
                             sizeof(t_longload), 
                             CLASS_NOINLET, 
                             A_DEFFLOAT, 0);
  zexy_register("longload");
}
