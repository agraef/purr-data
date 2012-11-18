# Makefile
# (c) 2006 IOhannes m zmölnig

# path to pd
## change this according to your setup!
PDROOT?=../../../../pd
#PDROOT=/home/zmoelnig/src/pd/

# here we find the sources of pd (and evtl. the pd.lib)
PDSRCDIR?=$(PDROOT)/src
PDLIBDIR?=$(PDROOT)/bin

# this is the filename-extension
# people have to specify it at the cmdline: eg "make pd_linux"
EXTENSION=$(MAKECMDGOALS)

# if no filename-extension is supplied by the user
# try to guess one, based on what "uname" tells us
UNAME_SYSTEM := $(shell uname -s)
UNAME_ARCH := $(shell uname -m)
ifeq ($(UNAME_SYSTEM),Linux)
  DEFAULTEXTENSION= pd_linux
  ifeq ($(UNAME_ARCH),x86_64)
    DEFAULTEXTENSION= l_ia64
  else ifeq ($(UNAME_ARCH),i686)
    DEFAULTEXTENSION= l_i386
  endif
else ifeq ($(UNAME_SYSTEM),Darwin)
    DEFAULTEXTENSION= pd_darwin
    ifeq ($(UNAME_ARCH),i386)
     DEFAULTEXTENSION= d_i386
    else ifeq ($(UNAME_ARCH),PowerPC)
     DEFAULTEXTENSION= d_ppc
    endif
else ifeq (MINGW,$(findstring MINGW,$(UNAME_SYSTEM)))
      DEFAULTEXTENSION= pd_nt
else ifeq ($(UNAME_SYSTEM),IRIX)
  UNAME_IRIXVERSION := $(shell uname -R)
  ifeq (6.,$(findstring 6.,$(UNAME_IRIXVERSION)))
    DEFAULTEXTENSION= pd_irix6
  else
    DEFAULTEXTENSION= pd_irix5
  endif
else
  DEFAULTEXTENSION=help
endif

# if no extension is given, call "make" again with a guessed extension
auto:
	make $(DEFAULTEXTENSION)

# just a stupid fallback
help: 
	@echo "choose one command:  make pd_linux, make l_i386, make l_ia64 (linux), make pd_darwin, make d_fat, make d_ppc (osX), make pd_irix5 (IRIX5), make pd_irix6 (IRIX6), make dll (MSVC), make pd_nt (MinWG)"

# delete old build files
clean:
	-rm -f *.dll *.w_i386 *.pd_linux *.l_ia64 *.l_i386  *.pd_darwin *.d_fat *.d_ppc *.d_i386 *.pd_irix5 *.pd_irix6
	-rm -f *.o *.obj *~

# we want to compile all C-files we find in the current directory
SOURCES=$(sort $(filter %.c, $(wildcard *.c)))
# each C-files maps will become an external with the given filename-extension
TARGETS=$(SOURCES:.c=.$(EXTENSION))

install: auto
	@echo " "
	@echo "	Wotcher!"
	@echo "	no installation routines found"
	@echo "	provide one yourself"
	@echo " "


# ----------------------- Linux -----------------------

l_ia64 l_i386 pd_linux: $(TARGETS)

LINUXCFLAGS = -DPD -g -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXLDFLAGS =  -shared  -lc -lm

LINUXINCLUDE =  -I$(PDSRCDIR)

STRIP?=strip

%.l_ia64 %.l_i386 %.pd_linux: %.c
	$(CC) $(LINUXLDFLAGS) $(LINUXCFLAGS) $(LINUXINCLUDE) -o $@ $<
#	$(STRIP) --strip-unneeded $@



# ----------------------- Mac OSX -----------------------

pd_darwin: $(TARGETS)

DARWINCFLAGS = -DPD  -DMISSING_LOADER_T -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

DARWININCLUDE = -I$(PDSRCDIR)

DARWINLDFLAGS = -bundle -undefined suppress -flat_namespace

%.pd_darwin: %.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) $(DARWINLDFLAGS) -o $*.pd_darwin $*.c


# ----------------------- IRIX 5.x -----------------------
pd_irix5: $(TARGETS)

SGICFLAGS5 = -o32 -DPD -DMISSING_LOADER_T -DSGI -O2

SGIINCLUDE =  -I$(PDSRCDIR)

SGILDFLAGS =  -elf -shared -rdata_shared

%.pd_irix5: %.c
	$(CC) $(SGICFLAGS5) $(SGIINCLUDE) -o $*.o -c $*.c
	$(LD) $(SGILDFLAGS) -o $*.pd_irix5 $*.o
	rm $*.o


# ----------------------- IRIX 6.x -----------------------
pd_irix6: $(TARGETS)

SGICFLAGS6 = -DPD  -DMISSING_LOADER_T -DSGI -n32 \
	-OPT:roundoff=3 -OPT:IEEE_arithmetic=3 -OPT:cray_ivdep=true \
	-Ofast=ip32

%.pd_irix6: %.c
	$(CC) $(SGICFLAGS6) $(SGIINCLUDE) -o $*.o -c $*.c
	$(LD) $(SGILDFLAGS) -o $*.pd_irix6 $*.o
	rm $*.o


# ----------------------- NT -----------------------
dll: $(TARGETS)

PDNTCFLAGS = /W3 /WX /DPD /DNT /D__WIN32__ /DMSW /DMISSING_LOADER_T /nologo

VC="C:\Programme\Microsoft Visual Studio\Vc98"

PDNTINCLUDE = /I. /I$(PDROOT)\tcl\include /I$(PDSRCDIR)\src /I$(VC)\include

PDNTLDIR = $(VC)\lib

PDNTLIB = $(PDNTLDIR)\libc.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	$(PDLIBDIR)\pd.lib 

%.dll: %.c
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:$*_setup $*.obj $(PDNTLIB)


pd_nt: $(TARGETS)

MINGWCFLAGS = -DPD -DMISSING_LOADER_T -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch -mms-bitfields

MINGWLDFLAGS =  --export-dynamic -shared -lm -lkernel32 -lcoldname -lcrtdll -lpd -L$(PDLIBDIR)

MINGWINCLUDE =  -I$(PDSRCDIR)

%.pd_nt: %.c
	$(CC) $(MINGWLDFLAGS) $(MINGWCFLAGS) $(MINGWINCLUDE) -o $*.dll $*.c
