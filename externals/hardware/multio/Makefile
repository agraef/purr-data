NAME=multio
CSYM=multio

current: pd_linux

pd_src = ../../../pd

# ----------------------- NT -----------------------

pd_nt: $(NAME).dll

.SUFFIXES: .dll

VC="C:\Programmi\Microsoft Visual Studio .NET\Vc7"
PDPATH="H:\PureData\pd-0.38-3.msw\pd"
LIBUSBPATH="H:\PureData\multIO\libusb-win32-device-bin-0.1.10.1"
PDNTCFLAGS = /W3 /WX /DNT /DPD /nologo


PDNTINCLUDE = /I. (PDPATH)\tcl\include /I$(PDPATH)\src /I$(VC)\include /I$(LIBUSBPATH)\include

PDNTLDIR = $(VC)\lib
PDNTLIB = $(PDNTLDIR)\libc.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	$(LIBUSBPATH)\lib\msvc\libusb.lib \
	$(PDPATH)\bin\pd.lib  \
	$(PDPATH)\bin\pthreadVC.lib

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:$(CSYM)_setup $*.obj $(PDNTLIB)

# ----------------------- LINUX i386 -----------------------

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch $(CFLAGS)

LINUXINCLUDE =  -I../../src -I$(pd_src)/src

.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	ld --export-dynamic  -shared -o $*.pd_linux $*.o -lc -lm -lusb
	strip --strip-unneeded $*.pd_linux
	rm -f $*.o

# ----------------------- Mac OSX -----------------------

pd_darwin: $(NAME).pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W

DARWININCLUDE =  -I../../src -I$(pd_src)/src -I/sw/include
DARWINLIBS = -L/sw/lib -lusb

.c.pd_darwin:
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -o $*.o -c $*.c
	$(CC) -bundle -bundle_loader $(pd_src)/bin/pd $(DARWINLIBS) \
		-o $*.pd_darwin $*.o 
	rm -f $*.o

# ----------------------------------------------------------

clean:
	rm -f *.o *.pd_* so_locations
