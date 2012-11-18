# ----------Pd-Sdif makefile, using the makefile--------------
# ---------from example Pd externals--------------------------
# *** Make sure the pd header files and sdif library can
# *** be found by including the right directory
# - this external will probably work on windows if you can
#   get the SDIF IRCAM library installed, but I couldn't
#   use the pd/doc/6.extern/makefile and link with -lsdif

current:
	echo make pd_linux

clean: ; rm -f *.pd_linux *.o


# ----------------------- LINUX i386 -----------------------

pd_linux: sdiflists.pd_linux



.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXINCLUDE = -I /usr/local/include


.c.pd_linux:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	ld -lsdif --export-dynamic  -shared -o $*.pd_linux $*.o -lc -lm 
	strip --strip-unneeded $*.pd_linux
	rm $*.o


# ----------------------- Mac OSX -----------------------

pd_darwin:  sdiflists.pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

DARWIN_INCLUDE = -I /usr/local/include

.c.pd_darwin:
	cc $(DARWINCFLAGS) $(DARWIN_INCLUDE) -o $*.o -c $*.c
	cc -bundle -undefined suppress -lsdif -flat_namespace -o $*.pd_darwin $*.o
	rm -f $*.o

