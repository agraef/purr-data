NAME=clone
CSYM=clone

current: pd_nt pd_linux pd_darwin

# ----------------------- NT -----------------------

pd_nt: $(NAME).dll

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /O2 /G6 /DNT /DPD /nologo

# where is VC++ ???
VC="C:\Programme\Microsoft Visual Studio\VC98"

# where is your m_pd.h ???
PDNTINCLUDE = /I. /Ic:\pd\tcl\include /Ic:\pd\src /I$(VC)\include

PDNTOBJ = clone_connective.obj clone_signal.obj

PDNTLDIR = $(VC)\Lib
PDNTLIB = $(PDNTLDIR)\libc.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	$(PDNTLDIR)\user32.lib \
	$(PDNTLDIR)\uuid.lib \
	c:\pd\bin\pd.lib

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c clone_connective.c
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c clone_signal.c
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:$(CSYM)_setup $*.obj $(PDNTOBJ) $(PDNTLIB)

# ----------------------- MAX OS X -----------------------

pd_darwin: $(NAME).pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -DUNIX -O2 -DMACOSX \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

DARWININCLUDE =  -I../../src 
DARWINOBJ = clone_connective.o clone_signal.o

.c.pd_darwin:
	cc $(DARWINCFLAGS) $(DARWININCLUDE) -o clone_connective.o -c clone_connective.c
	cc $(DARWINCFLAGS) $(DARWININCLUDE) -o clone_signal.o -c clone_signal.c
	cc $(DARWINCFLAGS) $(DARWININCLUDE) -o $*.o -c $*.c
	cc -bundle -undefined suppress -flat_namespace -o $*.pd_darwin $*.o $(DARWINOBJ)
	rm -f $*.o ../$*.pd_darwin
	ln -s $*/$*.pd_darwin ..

# ----------------------- LINUX i386 -----------------------

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -DUNIX -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch

# where is your m_pd.h ???
LINUXINCLUDE =  -I../../src 
LINUXOBJ = clone_connective.o clone_signal.o

.c.pd_linux:
	cc -O2 -Wall -DPD -fPIC $(LINUXCFLAGS) $(LINUXINCLUDE) -c clone_connective.c
	cc -O2 -Wall -DPD -fPIC $(LINUXCFLAGS) $(LINUXINCLUDE) -c clone_signal.c
	cc -O2 -Wall -DPD -fPIC $(LINUXCFLAGS) $(LINUXINCLUDE) -c $*.c
	ld --export-dynamic  -shared -o $*.pd_linux $*.o $(LINUXOBJ) -lc
	strip --strip-unneeded $*.pd_linux 

# ----------------------------------------------------------

install:
	cp *.pd ../../doc/5.reference

clean:
	rm -f *.o *.pd_* so_locations
