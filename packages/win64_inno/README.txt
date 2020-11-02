

Check this webpage for full build instructions:
http://puredata.org/docs/developer/mingw

------------------------------------------------------------------------------
Software Requirements
------------------------------------------------------------------------------

MinGW
	 MinGW provides a free, complete build environment for Pd.

Inno Setup - http://www.jrsoftware.org/isinfo.php
	 This package is assembled using Inno Setup, check pd.iss for details.

ogg vorbis win32k SDK - 
	 Install into C:\ to make it work with the current Makefile

Tcl/Tk
	 Compile for MinGW.

pthreads - ftp://sources.redhat.com/pub/pthreads-win32/
	 pthreads is a standard, cross-platform threading library used in the pd 
	 core and externals.  You can use the version included with Pd.

MinGW/gcc
	 Pd is free software, and can be compiled using free tools.  MinGW is the
	 preferred way of compiling Pd on Windows.

Microsoft Visual Studio - 
	 You can use MS Visual Studio 6.0 or better to compile Pd and some

------------------------------------------------------------------------------
MinGW Makefile
------------------------------------------------------------------------------

See: http://puredata.org/docs/developer/windows


------------------------------------------------------------------------------
Microsoft Visual Studio Makefile
------------------------------------------------------------------------------

You will need to do this to compile:

nmake /f Makefile.nmake

Currently, the Makefile.nmake only compiles the 'externals' collection.  It
can also compile flext if you manually check the flext config and uncomment
things from the Makefile.nmake.

------------------------------------------------------------------------------
Directory Layout
------------------------------------------------------------------------------

This directory is for files that are used in the creation of the Windows
installer.  In order to use this to compile/assemble Pd and externals.

 +-|
   +-abstractions
   |
   +-packages-|
   |          +-win32_inno-|
   |                       +-noncvs-|
   |                                +-extra
   |                                +-doc-|
   |                                      +-5.reference
   |
   +-doc-|
   |     +-additional
   |     +-pddp
   |     +-tutorials
   |
   +-externals-|
   |           +-...
   |           +-ext13
   |           +-ggee
   |           +-maxlib
   |           +-unauthorized
   |           +-zexy
   |           +-...
   |
   +-pd-|
        +-src
        +-doc
        +-etc...

        
The recommended way to do this is (these are probably somewhat wrong):

         mkdir pure-data && cd pure-data
         setenv CVSROOT :pserver:anonymous@cvs.sourceforge.net:/cvsroot/pure-data
         unzip pd source
         cvs checkout packages
         cvs checkout doc
         cvs checkout externals
         cd packages/win32_inno
		 make clean && make

------------------------------------------------------------------------------
non-CVS binaries
------------------------------------------------------------------------------

Binary Sources I Used (I haven't tested them all, I just downloaded them):

cyclone: http://suita.chopin.edu.pl/~czaja/miXed/externs/cyclone.html
freeverb~: http://www.akustische-kunst.org/puredata/freeverb/index.html
iemlibs: http://iem.kug.ac.at/~musil/iemlib/
maxlib: http://www.akustische-kunst.org/puredata/maxlib/index.html
OSC: http://barely.a.live.fm/pd/OSC/
percolate: http://www.akustische-kunst.org/puredata/percolate/index.html
toxy: http://suita.chopin.edu.pl/~czaja/miXed/externs/toxy.html
xeq: http://suita.chopin.edu.pl/~czaja/miXed/externs/xeq.html
zexy: ftp://iem.kug.ac.at/pd/Externals/ZEXY

all of T.Grill's code: http://www.parasitaere-kapazitaeten.net/ext/




-Hans-Christoph Steiner <hans@at.or.at>

