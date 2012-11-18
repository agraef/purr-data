OSC, OpenSoundControl for pd
============================
for more information on OSC see: http://cnmat.cnmat.berkeley.edu/OSC

for more information on pure-data: http://lena.ucsd.edu/~msp/ and http://www.google.com/search?q=pure-data

Build:
linux, macosx:

to build run
./configure
make
make install

which installs the files in /usr/lib/pd/extra and /usr/lib/pd/doc


windows:
use extra/OSC.dll
.dsw and .dsp files are also included. You'll want to adapt include and link paths for pd and LIBOSC.


files:

src/		contains the code for OSC pd objects (send,dump,route)
README.txt	this file
doc/		pd help files
extra/		OSC.dll, the windows binary
libOSC/		CNMAT's OSC library
send+dump/	CNMAT's OSC commandline utils (with some changes)
                http://cnmat.cnmat.berkeley.edu/OpenSoundControl/


log:

	20050830: v0.3: (piotr@majdak.com)
						adapted to compile on Windows2000
						dumpOSC routes up to 128 branches (tested with 24)
						sendOSC doesn't crash on longer messages (tested with one argument of 120 characters)
						sendOSC # of arguments limited by the length of the message (tested with 110 messages)
	          tested on Windows 2000 ONLY!

  20040409: changed build setup to suit externals build system
            single object objects, no lib

  20030531: added OSCroute /* (route everything) hard-fix

  20030527: added sending to broadcast address capability to htmsocket

  20020908: 0.16-4:
  	    added non-match / unmatched outlet to OSCroute
  	    updated doc/OSCroute-help.pd including a new chapter
	    about patternmatching.

  20020901: ca., refixed MAXPDARG vs. MAX_ARGS, causing crash when sending
            messages with more than 4 arguments

  20020417: 0.16-2:
            more changes by raf + jdl (send with no argument fix, send / fix,
            ...)

  20020416: added bundle stuff to sendOSC

  200204: 0.15b1:
          windowified version and implied linux enhancements
          by raf@interaccess.com
	  for now get it at http://207.208.254.239/pd/win32_osc_02.zip
	  most importantly: enhanced connect->disconnect-connect behaviour
          (the win modifications to libOSC are still missing in _this_
	   package but coming ..)


  200203: 0-0.1b1: all the rest
	  ost_at_test.at + i22_at_test.at, 2000-2002
      	  modified to compile as pd externel




INSTALL:
 (linux)

tar zxvf OSCx.tgz
cd OSCx
cat README
cd libOSC && make
cd ../OSC && "adjust makefile" && make OSC && make install
cd ../..
pd -lib OSC OSCx/doc/OSC-help.pd

 PITFALLS:
make sure you compile libOSC before OSC objects
maybe adjust include path so pd include files will be found


 (windo$)

unzip and put .dll file in a pd-searched folder.


TYPETAGS:
supported and on by default. can be swtiched off with the "typetags 0"
message and on with 1.


TODO
====
-timetags: output timetag when receiving a bundle for scheduling
-TCP mode
-address space integration with pd patch/subpatch/receive hierarchy ?

see also TODO.txt in OSC/

--
jdl at xdv.org
http://barely.a.live.fm/pd/OSC
http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/pure-data/externals/OSCx/
windows version:
raf at interaccess.com, http://207.208.254.239/pd/win32_osc_02.zip
