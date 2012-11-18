#ifndef INPUT_NOTICER_H_
#define INPUT_NOTICER_H_

/*
 * input_noticer - input noticer external for pure-data
 *
 * David Merrill <dmerrill@media.mit.edu>
 *
 * Description: Using dbus and the hardware abstraction layer (HAL) in linux, 
 * this external allows pd to find all linux device files for a given device type.
 * This scanning behavior can happen when the external is set up (via a [bang]),
 * and will happen automatically when a new device is added to the system. An
 * example linux device file would be:
 * 
 * /dev/input/event5
 * 
 * The pd user specifies device type as a string - i.e. "SideWinder Dual Strike", and
 * this external outputs lists containing an index, and the linux device file where 
 * each device of the given type was found. For example
 * 
 * {0, /dev/input/event5}
 * {1, /dev/input/event6}
 * 
 * These lists can be routed in PD with the [route] object - see the help file for
 * more details.
 * 
 * Thanks to Dan Willmans, Seth Nickell, and David Zeuthen for their 
 * invaluable help with the whole dbus/hal part. Also, thanks to Hans-Christoph
 * Steiner for his help with (and creation of) the joystick external.
 * 
 * For good examples and reference on dbus/hal, please see: 
 * NetworkManager.c: http://cvs.gnome.org/viewcvs/NetworkManager/src/NetworkManager.c?rev=1.100&view=markup
 * libhal.h: http://webcvs.freedesktop.org/hal/hal/libhal/libhal.h?rev=1.32&view=markup
 * 
 * Parts of this code were pulled from those examples.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <m_pd.h>

//#define DEBUG(x)
#define DEBUG(x) x 

#endif /*INPUT_NOTICER_H_*/
