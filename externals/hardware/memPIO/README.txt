memPIO: an external to read from/write to the "mem-PIO"-device 
	by bmcm ( httP://www.bmcm.de ) with pure-data.

the "mem-PIO" is a USB-device that offers digital (this is: 1 & 0) I/O.
there are 3 ports, each 8 channels.
each port can be set to either input or output
(not quite true: port3 is split into 2 subports (4 channels each)
that can be set to input resp. output 
separately; however this is not supported by this external)



general::
memPIO is a plugin for miller.s.puckette's realtime-computermusic-environment 
"puredata" (or abbreviated "pd")
this software will be of no use, if you don't have a running version of pd on your system.
check out for http://pd.iem.at to learn more about pd and how to get it. 

note: [memPIO] should be published under the Gnu General Public License. 
However, it is heavily based on the example-code provided by bmcm, 
which does not come with a copyright notice (at least i didn't find one)


IMPORTANT NOTE:
unfortunately [memPIO] is WINDOZE only (there are no drivers for other OS! shame on bmcm)

win32 :
extract the memPIO-0_x.zip.
start pd with the extraction path included (e.g: pd -path PATH\TO\MY\MEMPIO\)
create an object [memPIO]
there is a helpfile help-memPIO.pd that explains what is happening.

compiling:
if you want to compile it for yourself, use the workspace memPIO.dsw. 
i am not sure, whether it works with anything but m$ visual-c++ 6.0

authors::
this software is copyleft by iohannes m zmoelnig <zmoelnig@iem.at>


