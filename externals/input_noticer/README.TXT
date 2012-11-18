title: input_noticer

author: David Merrill <dmerrill@media.mit.edu>

desc: Using dbus and the hardware abstraction layer (HAL) in linux, 
this external allows pd to find all linux device files for a given device type.
This scanning behavior can happen when the external is set up (via a [bang]),
and will happen automatically when a new device is added to the system. An
example linux device file would be:

/dev/input/event5

The pd user specifies device type as a string - i.e. "SideWinder Dual Strike", and
this external outputs lists containing an index, and the linux device file where 
each device of the given type was found. For example

{0, /dev/input/event5}
{1, /dev/input/event6}

These lists can be routed in PD with the [route] object - see the help file for
[route] for more details.

In order to make this code compile, I had to install the following libraries
onto my system:

 libhal-dev
 libglib2.0-dev
 libdbus-glib-1-dev

In a debian-based system, they can be installed with apt-get, as in:
 sudo apt-get install libhal-dev

To see if you have the right libraries installed in order to compile, you can
try compiling the test_noticer.c file, by running the test_noticer_compile.sh
shell script, as follows:

source test_noticer_compile.sh

Then, run test_noticer, and when it's running you should see messages when 
you plug, or un-plug a joystick.

dmerrill@dmerrill-t40:~/externals/input_noticer$ ./test_noticer
device added, udi = /org/freedesktop/Hal/devices/usb_device_45e_28_noserial
device added, udi = /org/freedesktop/Hal/devices/usb_device_45e_28_noserial_if0
device added, udi = /org/freedesktop/Hal/devices/usb_device_45e_28_noserial_usbraw
device added, udi = /org/freedesktop/Hal/devices/usb_device_45e_28_noserial_if0_logicaldev_input
found the joystick at: /dev/input/event6

Thanks to Dan Willmans, Seth Nickell, and David Zeuthen for their 
invaluable help with the whole dbus/hal part. Also, thanks to Hans-Christoph
Steiner for his help with (and creation of) the joystick external.

For good examples and reference on dbus/hal, please see: 
NetworkManager.c: http://cvs.gnome.org/viewcvs/NetworkManager/src/NetworkManager.c?rev=1.100&view=markup
libhal.h: http://webcvs.freedesktop.org/hal/hal/libhal/libhal.h?rev=1.32&view=markup

Parts of this code were pulled from those examples.

