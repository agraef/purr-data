xsample - extended sample objects for Max/MSP and PD (pure data)

Copyright (c)2001-2006 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

----------------------------------------------------------------------------

Maximum care has been taken to prepare a delightful experience for you electronic artists.
Donations for further development of the package are HIGHLY APPRECIATED.

Visit https://www.paypal.com/xclick/business=gr%40grrrr.org&item_name=xsample&no_note=1&tax=0&currency_code=EUR

----------------------------------------------------------------------------

IMPORTANT INFORMATION for all Max/MSP users:

1) 
For Mac OSX it is best to put the max-darwin/xsample.mxd file into the folder 
/Library/Application Support/Cycling '74/externals
and the file maxmsp/xsample-objectmappings.txt into the folder 
/Library/Application Support/Cycling '74/init .

For Windows put the max-msvc\xsample.mxe file into the folder
C:\program files\common files\Cycling '74\externals (english version)
and the file maxmsp/xsample-objectmappings.txt in
C:\program files\common files\Cycling '74\init (english version)

Put the maxmsp/xsample.help file into the max-help folder.

2) 
Alternatively (or for OS9) it is advisable to put the xsample.mxd or xsample.mxe file 
into the "max-startup" folder. Hence it will be loaded at Max startup.

----------------------------------------------------------------------------

IMPORTANT INFORMATION for all PD users:

xsample is a library containing several external objects. You should load it at PD startup
by adding "-lib xsample" to the PD command line.
(If you forgot to do that, you can load the library by creating an [xsample] dummy object.

----------------------------------------------------------------------------


BUILDING XSAMPLE from source
----------------------------

You will need the flext C++ layer for PD and Max/MSP externals to compile this.
See http://grrrr.org/ext/flext
Download, install and compile the package.
Afterwards you can proceed with building xsample.


pd/Max - Windows - Microsoft Visual C, Borland C++, MinGW:
----------------------------------------------------------
Start a command shell with your eventual build environment
(e.g. run vcvars32.bat for Microsoft Visual Studio)

then run
 ..\flext\build.bat
(you would have to substitute ..\flext with the respective path to the flext package)


pd/Max - OSX/Linux - GCC:
-------------------------
From a shell run
bash ../flext/build.sh
(you would have to substitute ../flext with the respective path to the flext package)


Max - OS9 - Metrowerks CodeWarrior:
-----------------------------------
use the "xsample.mcp" project file

----------------------------------------------------------------------------


Goals/features of the package:

- portable and effective sample recording/playing objects for pd and Max/MSP
- MSP-like groove~ object for PD 
- message- or signal-triggered recording object with mix-in capability
- avoid the various bugs of the original MSP2 objects
- multi-channel capability 
- live update of respective buffer/array content
- switchable 4-point or linear interpolation for xplay~/xgroove~ object
- cross-fading loop zone for xgroove~

----------------------------------------------------------------------------

Version history:

0.3.1:
- added mixmode=2 to Max/MSP help files
- fixed limit consideration for loopmode=0 and 2.
- fixed buggy sampling in crossfade zones (and simplified it a lot)
- different initialization on buffer absence
- renew units and scalemode on buffer change
- fixed looped recording bug (thanks to Tatama Suomo)
- reconsidered all state changes
- more optimizations for interpolation functions
- use the new flext build system
- use branch hints

0.3.0:
- added resources to MaxMSP build
- xgroove~, xrecord~: introduced a loop/end bang outlet 
- added MaxMSP buffer resize recognition
- xgroove~: introduced crossfading loop zones
- adapted source for flext 0.4.1 - most methods within class scope
- introduced attributes
- restructured make procedures
- corrected names of PD makefile, set help names
- fixed scale mode bug with xgroove~
- added validity check for buffers
- Max/MSP OSX: new file xsample-objectmappings.txt fixes load of library on finding correct helpfiles!
- makefiles for command line MSVC++, BCC, cygwin GCC
- better handling of non-existent buffers
- fixed flext-related error on setting buffers
- xrecord~: mixmode has now 3 states (off, mix-in, add)

0.2.4:
- according to flext 0.2.3 changed sample type to t_sample (S)
- xrecord~: fixed mix mode bug
- fixed argument buffer problem

0.2.3:
- using flext 0.2.2 - xsample is now a library under MaxMSP
- cleaner gcc makefile
- xgroove~, xrecord~: added "all" message to select entire buffer length
- xgroove~, xplay~: revisited dsp methods, restructured the code, fixed small interpolation bugs 
- xgroove~, xplay~: added linear interpolation (message "interp 2") 
- enabled 0 output channels -> xgroove~: position output only
- xgroove~: added bidirectional looping (message "loop 2")

0.2.2:
- using flext 0.2.0
- xrecord~ for PD: new flext brings better graphics update behavior
- xrecord~: recording position doesn't jump to start when recording length is reached
- fixed bug with refresh message (min/max reset)
- xgroove~: position (by pos message) isn't sample rounded anymore
- reset/refresh messages readjust dsp routines to current buffer format (e.g. channel count)
- corrected Max/MSP assist method for multi-channel
- fixed xplay~ help method 
- changed syntax to x*~ [channels=1] [buffer] for future enhancements (MaxMSP only, warning for old syntax)
- fixed small bug concerning startup position in xgroove~ and xrecord~
- fixed deadly bug in xplay~ dsp code (only active with template optimization) 

0.2.1:
- no leftmost float inlet for position setting - use pos method
- changed dsp handling for flext 0.1.1 conformance
- workarounds for buggy/incomplete compilers
- prevent buffer warning message at patcher load (wait for loadbang)
- fixed bug: current pos is reset when changing min or max points

0.2.0: 
- first version for flext

---------------------------------------------------------------------------

TODO list:

features:
- multi-buffer handling (aka multi-channel for pd)
- anti-alias filter? (possible?)

- delay min/max changes when cur pos in cross-fade zone

bugs:
- PD: problems with timed buffer redrawing (takes a lot of cpu time) - flext bug?
- Max help files aren't correctly opened due to xsample objects residing in a library (FIXED for OSX!!!)
