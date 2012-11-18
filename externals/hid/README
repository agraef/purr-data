
BINARIES
--------

There are two included binaries:

hid.pd_darwin = Darwin/PowerPC and Mac OS X
hid.pd_linux = Linux/PowerPC



COMPILING
---------

WARNING!! This build system in this directory is deprecated and no longer
maintained!  "cd externals/ && make hid" is probably what you want now.  This
is also part of the Pd-extended build system.

The compiler needs to know where the Pd source is to find the headers, and the
linker needs to know where the Pd binary is to check symbols.  You can either
set these on the command line or edit the Makefile

  make INCLUDE=/path/to/pd/src PDEXECUTABLE=/path/to/pd/bin/pd

On GNU/Linux, this is likely to look like:

  make INCLUDE=/usr/local/src/pd-0.38-2/src PDEXECUTABLE=/usr/local/bin/pd

On Darwin/Mac OS X, this is like to look like:

  make INCLUDE=/usr/local/src/pd/src PDEXECUTABLE=/Applications/Pd.app/Contents/Resources/bin/pd



COMPILING A DEBUG VERSION
-------------------------

To compile a debug version, edit these lines in hid.c (about line 32):

//#define DEBUG(x)
#define DEBUG(x) x 

Swap the commented lines so it looks like the lines above then recompile.


INSTALL
-------

To install, you can add these lines to your .pdrc:

-path /path/to/hid-0.5
-helppath /path/to/hid-0.5

Or you can copy the files to another place:

mv *-help.pd examples/*.pd /path/to/doc/5.reference
mv *.pd *.pd_* /path/to/extra


.hc
