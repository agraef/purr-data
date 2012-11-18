#
# For Debian : 
# apt-get install xlib-dev xmms-dev
#



current: pd_linux
	@echo make pd_linux, pd_darwin, clean ... you must be specific
	@echo default to pd_linux


clean: ; rm -f *.pd_linux *.o

# ----------------------- LINUX i386 -----------------------

pd_linux: pcre.pd_linux xmms.pd_linux x11mouse.pd_linux x11key.pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS =  -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch -fPIC

LINUXINCLUDE =  -I../src -L/usr/lib/X11 -I/usr/include/X11 -L/usr/X11R6/lib -I../../pd/src
LINUXLINKS = -lX11 -lXtst -lgtk -lgdk -lgmodule -lglib -ldl -lXi -lXext -lX11 -lm -lxmms

.c.pd_linux:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c \
    `xmms-config --cflags`
	ld --export-dynamic -shared -o $*.pd_linux $*.o -lpcre -lc -lm\
    -L/usr/lib -L/usr/X11R6/lib $(LINUXLINKS)
	strip --strip-unneeded $*.pd_linux
	rm $*.o
# ----------------------- Mac OSX -----------------------

pd_darwin: matches.pd_darwin concat.pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

.c.pd_darwin:
	cc $(DARWINCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c -lpcre
	cc -bundle -undefined suppress -flat_namespace -o $*.pd_darwin $*.o 
	rm -f $*.o

