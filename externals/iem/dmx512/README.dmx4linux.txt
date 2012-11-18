DMX512
======

THIS IS NOT A DOCUMENTATION.
NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED!

these are just random notes on what i found useful in getting dmx4linux running.
it might freeze your computer or boil your lights.


A. setting up dmx4linux
=======================
note: there are debian(etch)-packages for dmx4linux, 
      but these seem to be pretty old (2.5)
      i haven't really tried these.
      instead i used dmx4linux-2.6.1

first get dmx4linux from http://llg.cubic.org/dmx4linux/
and extract it.

the drivers should compile fine with 2.6.18 kernels, but
alas! i am using 2.6.25 and there are some quirks to make
these work.

first of all i had problems compiling the ISA/PCI/parport drivers,
but since i only wanted to use a USB device, i just disabled those.
second, dmx4linux's build-system tries to override CFLAGS when building
the kernel-modules, which newer kernel versions (e.g. 2.6.25) do not like
at all. i had to modify the makefiles in order to use the EXTRA_CFLAGS

all the changes i did can be found in the dmx4linux2.6.1.patch
just run:
% patch -p1 < dmx4linux2.6.1.patch

then do
% ./configure
(which will produce a /tmp/dmxconfig.mk)
and run
% make

finally become root and do
# make install

after all has gone well, load the appropriate kernel modules


btw, it is always a good idea to read the readme that comes with dmx4linux...


B. permissions
==============
the dmx device-files created by udev will be owned by root.root and not be 
read/writeable by anyone but root.
in order to use them as an ordinary user, become root and create a group 
"dmx" and add users who need access to the dmx-devices to this group:
# addgroup dmx
# adduser zmoelnig dmx

in theory this should be enough to allow you access to your dmx devices
the next time you load a dmx-driver 
if you have problems, try plugging your device out and in again

if you don't care for a clean setup, you could also just grant everyone read/write permissions.
# chmod a+rw /dev/dmx*
be aware that this might be a security risk.



C. more drivers
===============
for using a "JMS USB2DMX" device, i had some driver problems.
finally i found 
http://www.opendmx.net/index.php/Linux_ArtNet_Node
which directed me to 
http://www.erwinrol.com/index.php?opensource/dmxusb.php
and the "dmx_usb" module which seems to work fine.
i guess, it will also work for the "enttec opendmx" device


