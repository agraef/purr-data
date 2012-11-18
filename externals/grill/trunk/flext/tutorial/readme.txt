flext - C++ layer for Max/MSP and pd (pure data) externals
tutorial examples

Copyright (c) 2001-2006 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

----------------------------------------------------------------------------

These are a few examples to demonstrate some flext features.
Contribution of examples to the package is higly appreciated!

----------------------------------------------------------------------------

The recommended order to go through the tutorial examples is the following:

1) simple*
2) adv*
3) attr*
4) timer*
5) signal*
6) lib*
7) thread*

and, if needed
8) sndobj* and/or stk*

if you choose to compile with SndObj support you will need the respective library
download from: http://www.may.ie/academic/music/musictec/SndObj/main.html

if you choose to compile with STK support you will need the respective package and build a library
download from: http://ccrma-www.stanford.edu/software/stk/
Under linux you can create such a library from the STK directory with:
"g++ -c -pipe -I include -D __LINUX_OSS__ src/*.cpp && ar r libstk.a *.o && rm -f *.o"


----------------------------------------------------------------------------

The package should at least compile (and is tested) with the following compilers:

pd - Windows:
-------------
o Microsoft Visual C++ 6: edit "config-pd-msvc.txt" & run "build-pd-msvc.bat" 

o Borland C++ 5.5 (free): edit "config-pd-bcc.txt" & run "build-pd-bcc.bat" 
	(no threading support for that compiler!)

o Cygwin: edit "config-pd-cygwin.txt" & run "sh build-pd-cygwin.sh" 
	(no threading support for that compiler!)

pd - linux:
-----------
o GCC: edit "config-pd-linux.txt" & run "sh build-pd-linux.sh" 

pd - MacOSX:
-----------
o GCC: edit "config-pd-darwin.txt" & run "sh build-pd-darwin.sh" 


Max/MSP - MacOS 9:
------------------
o Metrowerks CodeWarrior V6: edit & use the several ".cw" project files

You must have the following "Source Trees" defined:
"flext" - Pointing to the flext main directory
"Cycling74" - Pointing to the Cycling 74 SDK

Max/MSP - MacOSX:
------------------
o Metrowerks CodeWarrior V6: edit & use the several ".cw" project files

You must have the following "Source Trees" defined:
"OS X Volume" - Pointing to your OSX boot drive
"flext" - Pointing to the flext main directory
"Cycling74 OSX" - Pointing to the Cycling 74 SDK for xmax
"MP SDK" - Pointing to the Multiprocessing SDK (for threading support)

