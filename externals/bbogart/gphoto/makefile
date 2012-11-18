current:
	echo make pd_linux

clean: ; rm -f *.pd_linux *.o

# ----------------------- LINUX i386 -----------------------

pd_linux: gphoto.pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch \
    -lgphoto2 -lpthread

LINUXINCLUDE =  -I../../src

.c.pd_linux:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c -ggdb
	ld -shared -o $*.pd_linux $*.o -lc -lm -lgphoto2
	strip --strip-unneeded $*.pd_linux
	rm $*.o


