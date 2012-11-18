# To use this Makefile for your project, first put the name of your library in
# LIBRARY_NAME variable. The folder for your project should have the same name
# as your library.
LIBRARY_NAME = pdvjtools

# exclude videogrid for now since it doesn't build with ffmpeg from Debian/stable
CSOURCES = colorgrid/colorgrid.c pdp_freeframe/pdp_freeframe.c pdp_frei0r/pdp_frei0r.c
CXXSOURCES = pix_preview/pix_preview.cc videogrid/videogrid.cc

# For objects that only build on certain platforms, add those to the SOURCES
# line for the right platforms.
CSOURCES_Darwin =
CXXSOURCES_Darwin =

CSOURCES_Linux = 
CXXSOURCES_Linux = 

CSOURCES_Windows = 
CXXSOURCES_Windows = 

#------------------------------------------------------------------------------#
#
# you shouldn't need to edit anything below here, if we did it right :)
#
#------------------------------------------------------------------------------#

# where Pd lives
PD_PATH = ../../pd
# where to install the library
objectsdir = $(PD_PATH)/extra

CFLAGS = -DPD -I$(PD_PATH)/src -Wall -W -g -I../pdp/include -I../../Gem/src -I/usr/include/ffmpeg

LDFLAGS = 
LIBS = -lavcodec -lavformat -lavutil -lswscale -lquicktime -lstdc++ 

UNAME := $(shell uname -s)
ifeq ($(UNAME),Darwin)
  CPU := $(shell uname -p)
  ifeq ($(CPU),arm) # iPhone/iPod Touch
    CSOURCES += $(CSOURCES_Darwin)
    CXXSOURCES += $(CXXSOURCES_Darwin)
    EXTENSION = pd_darwin
    OS = iphoneos
    IPHONE_BASE=/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin
    CC=$(IPHONE_BASE)/gcc
    CPP=$(IPHONE_BASE)/cpp
    CXX=$(IPHONE_BASE)/g++
    ISYSROOT = -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS3.0.sdk
    IPHONE_CFLAGS = -miphoneos-version-min=3.0 $(ISYSROOT) -arch armv6
    OPT_CFLAGS = -fast -funroll-loops -fomit-frame-pointer
	CFLAGS := $(IPHONE_CFLAGS) $(OPT_CFLAGS) $(CFLAGS)
    LDFLAGS += -arch armv6 -bundle -undefined dynamic_lookup $(ISYSROOT)
    LIBS += -lc 
    STRIP = strip -x
  else # Mac OS X
    CSOURCES += $(CXXSOURCES_Darwin)
    CXXSOURCES += $(CXXSOURCES_Darwin)
    EXTENSION = pd_darwin
    OS = macosx
    OPT_CFLAGS = -ftree-vectorize -ftree-vectorizer-verbose=2 -fast
    FAT_FLAGS = -arch ppc -arch i386 -arch x86_64 -mmacosx-version-min=10.4
    CFLAGS += $(FAT_FLAGS) -fPIC -I/sw/include
    LDFLAGS += $(FAT_FLAGS) -bundle -undefined dynamic_lookup -L/sw/lib
    LIBS += -lc 
    STRIP = strip -x
  endif
endif
ifeq ($(UNAME),Linux)
  CSOURCES += $(CSOURCES_Linux)
  CXXSOURCES += $(CXXSOURCES_Linux)
  EXTENSION = pd_linux
  OS = linux
  OPT_CFLAGS = -O6 -funroll-loops -fomit-frame-pointer
  CFLAGS += -fPIC
  LDFLAGS += -Wl,--export-dynamic  -shared -fPIC
  LIBS += -lc
  STRIP = strip --strip-unneeded -R .note -R .comment
endif
ifeq (CYGWIN,$(findstring CYGWIN,$(UNAME)))
  CSOURCES += 
  CXXSOURCES += 
  EXTENSION = dll
  OS = cygwin
  OPT_CFLAGS = -O6 -funroll-loops -fomit-frame-pointer
  CFLAGS += 
  LDFLAGS += -Wl,--export-dynamic -shared -L$(PD_PATH)/src
  LIBS += -lc -lpd
  STRIP = strip --strip-unneeded -R .note -R .comment
endif
ifeq (MINGW,$(findstring MINGW,$(UNAME)))
  CSOURCES += $(CSOURCES_Windows)
  CXXSOURCES += $(CXXSOURCES_Windows)
  EXTENSION = dll
  OS = windows
  OPT_CFLAGS = -O3 -funroll-loops -fomit-frame-pointer
  WINDOWS_HACKS = -D'O_NONBLOCK=1'
  CFLAGS += -mms-bitfields $(WINDOWS_HACKS)
  LDFLAGS += -s -shared -Wl,--enable-auto-import
  LIBS += -L$(PD_PATH)/src -L$(PD_PATH)/bin -L$(PD_PATH)/obj -lpd -lwsock32 -lkernel32 -luser32 -lgdi32
  STRIP = strip --strip-unneeded -R .note -R .comment
endif

CFLAGS += $(OPT_CFLAGS)


.PHONY = install install-doc install-exec clean dist etags

all: $(CSOURCES:.c=.$(EXTENSION)) $(CXXSOURCES:.cc=.$(EXTENSION))

%.$(EXTENSION): %.c
	$(CC) $(CFLAGS) -o "$*.o" -c "$*.c"
	$(CC) $(LDFLAGS) -o "$*.$(EXTENSION)" "$*.o"  $(LIBS)
	chmod a-x "$*.$(EXTENSION)"

%.$(EXTENSION): %.cc
	$(CXX) $(CFLAGS) -o "$*.o" -c "$*.cc"
	$(CXX) $(LDFLAGS) -o "$*.$(EXTENSION)" "$*.o"  $(LIBS)
	chmod a-x "$*.$(EXTENSION)"

# The meta and help files are explicitly installed to make sure they are
# actually there.  Those files are not optional, then need to be there.
install: all install-doc install-exec
	install -d $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)
	install -m644 -p $(LIBRARY_NAME)-meta.pd $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)
	install -m644 -p $(CSOURCES:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)
	install -m644 -p $(CXXSOURCES:.cc=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)
	$(STRIP) $(addprefix $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)/,$(CSOURCES:.c=.$(EXTENSION)))
	$(STRIP) $(addprefix $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)/,$(CXXSOURCES:.cc=.$(EXTENSION)))

install-doc:
	install -d $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)
	install -m644 -p $(CSOURCES:.c=-help.pd) $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)
	install -m644 -p $(CXXSOURCES:.cc=-help.pd) $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)
	install -m644 -p README $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)/README.txt
	install -m644 -p VERSION $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)/VERSION.txt
	install -m644 -p CHANGES $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)/CHANGES.txt

install-exec:
	install -d $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)
#	install -m644 -p $(wildcard *.pd) $(DESTDIR)$(objectsdir)/$(LIBRARY_NAME)


clean:
	-rm -f -- $(CSOURCES:.c=.o)
	-rm -f -- $(CXXSOURCES:.cc=.o)
	-rm -f -- $(CSOURCES:.c=.$(EXTENSION))
	-rm -f -- $(CXXSOURCES:.cc=.$(EXTENSION))

distclean: clean
	-rm -f -- ../$(LIBRARY_NAME)-$(OS)-$(shell uname -m).tar.bz2
	-rm -f -- ../$(LIBRARY_NAME)-$(OS).tar.bz2

dist: all dist_$(OS)

dist_linux:
	cd .. && tar --exclude=.svn -cjpf $(LIBRARY_NAME)-$(OS)-$(shell uname -m).tar.bz2 $(LIBRARY_NAME)

dist_macosx:
	cd .. && tar --exclude=.svn -cjpf $(LIBRARY_NAME)-$(OS).tar.bz2 $(LIBRARY_NAME)

dist_windows:
	cd .. && tar --exclude=.svn -cjpf $(LIBRARY_NAME)-$(OS).tar.bz2 $(LIBRARY_NAME)


etags:
	etags *.[ch] ../../pd/src/*.[ch] /usr/include/*.h /usr/include/*/*.h

showpaths:
	@echo "PD_PATH: $(PD_PATH)"
	@echo "objectsdir: $(objectsdir)"
	@echo "LIBRARY_NAME: $(LIBRARY_NAME)"
	@echo "CSOURCES: $(CSOURCES)"
	@echo "CXXSOURCES: $(CXXSOURCES)"
	@echo "UNAME: $(UNAME)"
	@echo "CPU: $(CPU)"
