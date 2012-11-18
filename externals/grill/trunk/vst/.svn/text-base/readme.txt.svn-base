vst~ - VST plugin external for PD
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-05 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

Donations for further development of the package are highly appreciated.
Visit https://www.paypal.com/xclick/business=gr%40grrrr.org&item_name=vst&no_note=1&tax=0&currency_code=EUR

----------------------------------------------------------------------------

BUILDING from source
--------------------

You will need the flext C++ layer for PD and Max/MSP externals to compile this.
See http://grrrr.org/ext/flext
Download, install and compile the package.
Afterwards you can proceed with building this external.


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


----------------------------------------------------------------------------

Version history:

0.1.0:
- fixed crash when there's no "VST_PATH" environment string found
- included pd path into VST search path
- made thread of editor window low priority
- introduced A LOT of attributes to get info about the plugin
- make editor window closable by patch
- plugin can be changed with plug attribute
- fixed crash on destroying vst~ with open editor window
- stripped all MFC code
- fixed DSP initialization, zero dangling audio vectors
- pre12: added "bypass" and "mute" attributes
- pre13: with flext 0.4.7 no more interruptions on window close
- pre14: allow window titles with spaces and update it on window startup
- pre18: open plug interface on Alt-Click
- pre18: experimental plug shell support
- pre19: better shell support
- pre19: restructured code and added time info
- pre19: support for event processing (like MIDI in)
- pre21: consistent MIDI handling, correct handling of parameters with spaces
- pre22: cleaner GUI code, all kinds of window handling
- pre22: catch exceptions like crashing plugs
- pre23: security measures for open edit window
- pre24: workarounds for Waves5 strangenesses

0.0.0:
- version of mark@junklight.com

---------------------------------------------------------------------------

BUGS:
- mouse interaction in editor can cause audio dropouts

TODO:
- include necessary Steinberg license stuff
- do plugin loading, name scanning in the background
- translate special characters in strings (like ° as param_label) into system-digestible form
