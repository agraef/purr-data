current:
	echo make pd_linux, pd_nt, pd_irix, pd_darwin

clean: ; rm -f *.pd_linux *.dll *.pd_darwin *.o

# ----------------------- NT -----------------------

pd_nt: netreceive~.dll netsend~.dll

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /DNT /D_WINDOWS /DPD /nologo
VC="C:\Programme\Microsoft Visual Studio\VC98"

PDNTINCLUDE = /I. /Ic:\pd\tcl\include /Ic:\pd\src /I$(VC)\include

PDNTLDIR = $(VC)\Lib
PDNTLIB = $(PDNTLDIR)\libc.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	$(PDNTLDIR)\user32.lib \
	$(PDNTLDIR)\uuid.lib \
	$(PDNTLDIR)\ws2_32.lib \
	c:\pd\bin\pthreadVC.lib \
	c:\pd\bin\pd.lib

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c netreceive~.c
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c netsend~.c
	link /dll /export:netreceive_tilde_setup netreceive~.obj $(PDNTLIB)
	link /dll /export:netsend_tilde_setup netsend~.obj $(PDNTLIB)


# -------------------- IRIX 6.x (GCC) ---------------------

pd_irix6: netreceive~.pd_irix6 netsend~.pd_irix6

.SUFFIXES: .pd_irix6

# adjust path to PD:
SGIINCLUDE =  -I../../src

SGICFLAGS6 = -mabi=n32 -DPD -DUNIX -DIRIX -DN32 -O3 \
    -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch -mips4

.c.pd_irix6:
	gcc $(SGICFLAGS6) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -n32 -IPA -shared -rdata_shared -o $*.pd_irix6 $*.o
	rm $*.o

# ----------------------- MAX OS X -------------------

pd_darwin: netreceive~.pd_darwin netsend~.pd_darwin

.SUFFIXES: .pd_darwin

DARWINFATFLAGS = -arch ppc -arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk

DARWINCFLAGS = -DPD -DUNIX -DMACOSX -DHAVE_LRINT -DHAVE_LRINTF -fast -fPIC \
    -Wall -W -Wno-unused -Wstrict-prototypes \
	$(DARWINFATFLAGS)

DARWININCLUDE =  -I../../../pd/src

DARWINLDFLAGS = -bundle -undefined dynamic_lookup $(DARWINFATFLAGS)

.c.pd_darwin:
	cc $(DARWINCFLAGS) $(DARWININCLUDE) -o $*.o -c $*.c
	cc $(DARWINLDFLAGS) -o $*.pd_darwin $*.o
	rm -f $*.o

# ----------------------- LINUX i386 -----------------------

pd_linux: netreceive~.pd_linux netsend~.pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -DUNIX -DHAVE_LRINT -DHAVE_LRINTF -O2 \
	-funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXINCLUDE =  -I../../../pd/src

.c.pd_linux:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	ld --export-dynamic  -shared -o $*.pd_linux $*.o -lc -lm
	strip --strip-unneeded $*.pd_linux
	rm $*.o

