FFTease - A set of Live Spectral Processors
Originally written by Eric Lyon and Christopher Penrose for the Max/MSP platform

Copyright (c)Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  


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

PORTING NOTES:

The example audio files schubert.aiff and nixon.aiff have been taken from the original FFTease package for Max/MSP.


- pv-lib:
	- gcc (OSX) complains about _cfft being defined by pv-lib and pd.... any problems with that?

- burrow:
	- max_bin calculation: fundamental frequency seems to be wrong

- cross:
	- STRANGE: spectral amplitude in channel1 is undefined if gainer <= threshie
			-> value of previous frame is used then
	- (jmax) BUG: a2 for i == N2 is calculated from buffer1 
	- what about the class members for "correction"?! (superfluous)

- dentist:
	- tooth count ("teeth") is preserved and checked on every reshuffle
	- use different knee correction 

- disarray:
	- different frequency correction employed
	- max_bin calculation: fundamental frequency seems to be wrong
	- check whether freq oder number of bins should be selectable -> frequency!

- ether:
	- possibility to change qual?

- scrape:
	- maxamp is computed (from spectral amplitudes) before these are set!! (function frowned) -> corrected

- shapee:
	- danger of div by 0... corrected

- swinger:
	- (jmax) phase is calculated from signal1 (instead of correct signal 2)!! 

