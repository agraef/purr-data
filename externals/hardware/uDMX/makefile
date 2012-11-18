current:
	echo make pd_linux, pd_nt, pd_irix5, pd_irix6 or pd_darwin, then make install

clean: ; rm -f *.pd_* *.o

# ----------------------- NT -----------------------

pd_nt: edubeat.dll

INSTALL_PREFIX="C:\pd\extra"
EXT=dll
.SUFFIXES: .obj .dll

PDNTCFLAGS = /W3 /WX /DNT /DPD /nologo
VC="D:\Program Files\Microsoft Visual Studio\Vc98"

PDNTINCLUDE = /I. /I\tcl\include /I..\..\src /I$(VC)\include

PDNTLDIR = $(VC)\lib
PDNTLIB = $(PDNTLDIR)\libc.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	..\..\bin\pd.lib 

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:$*_setup $*.obj $(PDNTLIB)

# ----------------------- IRIX 5.x -----------------------

pd_irix5: edubeat.pd_irix5

INSTALL_PREFIX=/usr/local
EXT=pd_irix5
.SUFFIXES: .pd_irix5

SGICFLAGS5 = -o32 -DPD -DUNIX -DIRIX -O2

SGIINCLUDE =  -I/usr/local/include

.c.pd_irix5:
	cc $(SGICFLAGS5) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -elf -shared -rdata_shared -o $*.pd_irix5 $*.o
	rm $*.o

# ----------------------- IRIX 5.x -----------------------

pd_irix6: edubeat.pd_irix6

INSTALL_PREFIX=/usr/local
EXT=pd_irix6
.SUFFIXES: .pd_irix6

SGICFLAGS5 = -o32 -DPD -DUNIX -DIRIX -O2

SGIINCLUDE =  -I/usr/local/include

.c.pd_irix6:
	cc $(SGICFLAGS5) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -elf -shared -rdata_shared -o $*.pd_irix6 $*.o
	rm $*.o

# ----------------------- LINUX i386 -----------------------

pd_linux: uDMX.pd_linux

INSTALL_PREFIX=/usr
EXT=pd_linux
.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXINCLUDE =  -I/usr/include

.c.pd_linux:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	ld -export_dynamic  -shared -o $*.pd_linux $*.o -lc -lm -lusb
	strip --strip-unneeded $*.pd_linux
	rm $*.o

# ----------------------- Mac OSX -----------------------

pd_darwin: edubeat.pd_darwin

INSTALL_PREFIX=/usr/local
EXT=pd_darwin
.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

.c.pd_darwin:
	cc $(DARWINCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	cc -bundle -undefined suppress -flat_namespace -o $*.pd_darwin $*.o 
	rm -f $*.o

# ----------------------------------------------

install::
	install -d $(INSTALL_PREFIX)/lib/pd/extra
#	install -m 644 *.$(EXT) $(INSTALL_PREFIX)/lib/pd/externs
	-install -m 644 edubeat.$(EXT) $(INSTALL_PREFIX)/lib/pd/extra
	install -m 644 *.pd $(INSTALL_PREFIX)/lib/pd/doc/5.reference
