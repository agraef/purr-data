# Makefile hacked together from various others, mostly the old zexy makefile and pique. All credit to IOhannes Zmoelnig, Miller Puckette and anyone else who understands make better than me.
#
# Should work with Linux and OS X. No idea about Windows and Irix.

NAME=flib
CSYM=flib

current: pd_linux

DESTDIR =
INSTALL_PREFIX = /usr/local
PDLIBDIR = /lib/pd
PDDIR = $(INSTALL_PREFIX)$(PDLIBDIR)

TARGETS = sc~ ss~ irreg~ mspec~ peak~ pspec~ sfm~ trist~ pp~ bmax~ melf~ clean~ wdv~ hca~ cc~


# ----------------------- NT -----------------------

pd_nt: src/$(NAME).dll

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /MD /O2 /G6 /DNT /DPD /nologo
VC="C:\Programme\Microsoft Visual Studio\VC98"

PDNTINCLUDE = /I. /Ic:\pd\tcl\include /Ic:\pd\src /I$(VC)\include /Iinclude

PDNTOBJECTS = $(TARGETS:%=%.obj)

PDNTLDIR = $(VC)\Lib
PDNTLIB = $(PDNTLDIR)\msvcrt.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	$(PDNTLDIR)\user32.lib \
	$(PDNTLDIR)\uuid.lib \
	$(PDNTLDIR)\ws2_32.lib \
	$(PDNTLDIR)\pthreadVC.lib \
	c:\pd\bin\pd.lib
        
.c.dll:
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/sc~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/ss~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/irreg~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/mspec~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/peak~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/pspec~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/sfm~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/trist~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/pp~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/bmax~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/melf~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/clean~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/wdv~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/hca~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) -c src/cc~.c
	$(CC) $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:$(CSYM)_setup $(NAME).obj $(PDNTOBJECTS) $(PDNTLIB)


# ----------------------- Mac OS X (Darwin) -----------------------


pd_darwin: src/$(NAME).pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -DUNIX -DMACOSX -O2 \
    -Wall -W -Wno-shadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

# where is your m_pd.h ???
DARWININCLUDE =  -I ../src -I../../src -I../../obj -I/Applications/Pd-0.38-4-extended-RC1.app/Contents/Resources/include

DARWINOBJECTS = $(TARGETS:%=%.o)

.c.pd_darwin:
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/sc~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/ss~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/irreg~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/mspec~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/peak~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/pspec~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/sfm~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/trist~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/pp~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/bmax~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/melf~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/clean~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/wdv~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/hca~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/cc~.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c $*.c 
	$(CC) -bundle -undefined suppress -flat_namespace -o $(NAME).pd_darwin $(NAME).o $(DARWINOBJECTS)
	rm -f $(NAME).o ../$(NAME).pd_darwin
	ln -s $(NAME)/$(NAME).pd_darwin ..

darwin_package: pd_darwin
	test -d root/doc/5.reference || mkdir -p root/doc/5.reference
	-cp  help/* root/doc/5.reference
	test -d root/extra || mkdir -p root/extra
	install -m644 *.pd_darwin root/extra
	open darwin_package.pmsp

darwin_altpackage: pd_darwin
	test -d root/Help || mkdir -p root/Help
	-cp help/* root/Help
	test -d root/Externals || mkdir -p root/Externals
	install -m644 *.pd_darwin root/Externals
	open darwin_altpackage.pmsp

# ----------------------- LINUX i386 -----------------------

pd_linux: src/$(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXOBJECTS = $(TARGETS:%=%.o)
ARCH = $(shell uname --machine)

PD_DIR = $(DESTDIR)$(INSTALL_PREFIX)$(PDLIBDIR)

ifeq (${ARCH},alpha)
AFLAGS = -mieee  -mcpu=ev56
endif


$(LINUXOBJECTS): *.h

LINUXCFLAGS = -DPD -O3 -fPIC -funroll-loops -Wall -W -Wshadow -Wstrict-prototypes -Werror -fomit-frame-pointer -Wno-unused -Wno-parentheses -Wno-switch $(AFLAGS)

LINUXINCLUDE =  -I/usr/include


.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/sc~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/ss~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/irreg~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/mspec~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/peak~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/pspec~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/sfm~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/trist~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/pp~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/bmax~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/melf~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/clean~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/wdv~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/hca~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/cc~.c
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c $*.c
	ld --export-dynamic  -shared -o $(NAME).pd_linux $(NAME).o $(LINUXOBJECTS) -lc -lm
	strip --strip-unneeded $(NAME).pd_linux

# ----------------------- IRIX 5.x -----------------------

pd_irix5: src/$(NAME).pd_irix5

.SUFFIXES: .pd_irix5

SGIOBJECTS = $(TARGETS:%=%.o)

SGICFLAGS5 = -o32 -DPD -DUNIX -DIRIX -O2

SGIINCLUDE =  -I/usr/include/ -I/usr/local/include/

.c.pd_irix5:
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/sc~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/ss~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/irreg~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/mspec~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/peak~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/pspec~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/sfm~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/trist~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/pp~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/bmax~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/melf~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/clean~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/wdv~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/hca~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/cc~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c $*.c
	ld -elf -shared -rdata_shared -o $(NAME).pd_irix5 $(NAME).o $(LINUXOBJECTS) -lm -lc
	rm $*.o

# ----------------------- IRIX 6.x -----------------------

pd_irix6: src/$(NAME).pd_irix6

.SUFFIXES: .pd_irix6

SGICFLAGS6 = -n32 -DPD -DUNIX -DIRIX -DN32 -woff 1080,1064,1185 \
	-OPT:roundoff=3 -OPT:IEEE_arithmetic=3 -OPT:cray_ivdep=true \
	-Ofast=ip32

SGIOBJECTS = $(TARGETS:%=%.o)

SGIINCLUDE =  -I/usr/include/ -I/usr/local/include/

.c.pd_irix6:
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/ss~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/irreg~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/mspec~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/peak~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/pspec~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/sfm~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/trist~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/pp~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/bmax~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/melf~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/clean~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/wdv~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/hca~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c src/cc~.c
	$(CC) $(SGICFLAGS) $(SGIINCLUDE) -c $*.c
	ld -n32 -IPA -shared -rdata_shared -o $*.pd_irix6 $(NAME).o $(LINUXOBJECTS) -lm -lc
	rm $*.o

# ----------------------------------------------------------

install:
	install -d $(PDDIR)/doc/5.reference/flib
	cp doc/help-* $(PDDIR)/doc/5.reference/flib
	install -d $(PDDIR)/externs
	install -m 644 *.pd_linux $(PD_DIR)/externs
	
clean:
	rm -f *.o *.pd_* so_locations
