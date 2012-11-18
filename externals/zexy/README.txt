==============================================================================
the zexy external
==============================================================================

outline of this file::
==============================================================================
 +  general
 +  installation
   +  linux
   +  w32
   +  irix
   +  osX
 +  using
 +  authors



general::
==============================================================================
the zexy external is a collection of externals for miller.s.puckette's 
realtime-computermusic-environment called "puredata" (or abbreviated "pd")
this zexy external will be of no use, if you don't have a running version of 
pd on your system.
check out for http://pd.iem.at to learn more about pd and how to get it 

note: the zexy external is published under the Gnu General Public License 
that is included (GnuGPL.txt). some parts of the code are taken directly 
from the pd source-code, they, of course, fall under the license pd is 
published under.



installation::
==============================================================================

linux :
------------------------------------------------------------------------------

short:
#1> cd src/
#2> make
#3> make install
(this will automatically call autoconf and ./configure if needed (see "long"))

long:
#1>  cd src/
#2>  ./bootstrap.sh
#3>  ./configure
#4>  make
#5>  make install

this will install the zexy external into /usr/local/lib/pd/externs
(the path can be changed either via the "--prefix"-flag to "configure"
or by editing the makefile
alternatively you can try "make everything" (after ./configure)
note: if you don't want the parallel-port object [lpt]
 (e.g.: because you don't have a parallel-port) you can disable it 
 with "--disable-lpt"


macOS-X:
------------------------------------------------------------------------------
see installation/linux

there is nothing special in the code, so it should compile out of the box:
"cd" to zexy/src
run "./bootstrap.sh; ./configure; make" (for further details please see "1) linux")

building with a special version of Pd:
        to build zexy with your special version of Pd, you should specify the path to your Pd ressources
        (e.g. "./configure --with-pd=/Applications/Pd.app/Contents/Resources")
fat-binaries
        if you want to build a multi-arch binary you have to specify this as well
        (e.g. "./configure --enable-fat-binary=i386,ppc --with-extension=d_fat")

note on generating dependencies:
 on older systems the automatic creation of build dependencies
 might fail with following error:
    cpp0: invalid option -smart
 a simple workaround is to not use the "-E" flag for the preprocessor
 try:
   make CPP=cc


win32 :
------------------------------------------------------------------------------

#1 extract the zexy-0_x.zip to your pd-path (this file should be located 
   at <mypdpath>/pd/zexy/)
#2 execute the "z_install.bat", this should copy all necessary files 
   to the correct places

to compile: 
 + w/ MSVC use makefile.nt or zexy.dsw; 
 OR
 + with GCC configure your pd path, eg:
	#> ./configure --prefix=/c/program/pd; make; make install
 OR
 + cross-compilation for windows on linux using mingw (assumes that the 
   crosscompiler is "i586-mingw32msvc-cc")
	#> ./configure --host=i586-mingw32msvc --with-extension=dll \
	   --disable-PIC --with-pd=/path/to/win/pd/
	#> make CFLAGS="-fno-unit-at-a-time"
     notes: configure tries to set the CFLAGS to "-g -O2" if the compiler
            accepts this; however, this optimization sometimes generates 
	    binaries that cannot be loaded by pd; it seems that disabling
	    the "unit-at-a-time" optimization (which gets enabled by "-O2")
	    is the cause of this problem. turning it off might help

irix :
------------------------------------------------------------------------------

though i have physical access to both SGI's O2s and indys,
i haven't tried to compile the zexy externals there for years.
the configure-script should work here too;
if not, try "make -f makefile.irix"
Good luck !



making pd run with the zexy external::
==============================================================================
make sure, that pd will be looking at this location 
(add "-path <mypath>/pd/externs" either to your .pdrc or each time 
you execute pd)
make sure, that you somehow load the zexy external (either add "-lib zexy" 
(if you advised pd somehow to look at the correct place) 
or "-lib <myzexypath>/zexy" to your startup-script (.pdrc or whatever) 
or load it via the object "zexy" at runtime



authors::
==============================================================================
this software has been mainly written by 
	IOhannes m zmoelnig <zmoelnig [at] iem [dot] at>
but a lot of others have contributed as well.

Copyright 1999-2010 IOhannes m zmoelnig
Copyright 1999-2010 zexy-contributers
Copyright 1998-2004 matt wright
Copyright 1999-2000 winfried ritsch
Copyright 1999      guenter geiger
Copyright 1996-1999 miller s puckette
Copyright 2005-2006 tim blechmann
Copyright 2009-2010 franz zotter


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



