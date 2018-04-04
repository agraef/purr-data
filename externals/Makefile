
#==============================================================================#
#
# Centralized build system for "externals".  
#
# see README for instructions  <hans@at.or.at>
# http://puredata.org/docs/developer/build
#
#==============================================================================#

CWD := $(shell pwd)

# these are setup to be overridden by the packages/Makefile
cvs_root_dir := $(shell cd $(CWD)/.. && pwd)
DESTDIR = $(CWD)/build/
BUILDLAYOUT_DIR = $(cvs_root_dir)/packages

# turn on weak linking and dlopen support
export MACOSX_DEPLOYMENT_TARGET = 10.3

# default target
default: all

.SUFFIXES: .$(EXTENSION) .$(SHARED_LIB)


include $(BUILDLAYOUT_DIR)/Makefile.buildlayout


# these are sent to all of the various Makefiles so that they all copy their
# output to the same directory tree
DEST_PATHS = BUILDLAYOUT_DIR=$(BUILDLAYOUT_DIR) \
				cvs_root_dir=$(cvs_root_dir) \
				DESTDIR=$(DESTDIR) \
				prefix=$(prefix)

#==============================================================================#
#
# COMPILE TARGETS
#
#==============================================================================#

# this variable is to support old "win" directories, rather than "windows"
BUILDSRC_OS_NAME = $(OS_NAME)

WARN_FLAGS = -Wall -W -Wno-unused-parameter
CFLAGS = -DPD -I$(pd_src)/src $(WARN_FLAGS) $(DEBUG_CFLAGS) -I$(gem_src)/src
LDFLAGS =  
LIBS = -lm

ifeq ($(OS_NAME),darwin)
# 10.4 Tiger
  FAT_FLAGS = -arch ppc -arch ppc64 -arch i386
# 10.5 Leopard
#  FAT_FLAGS = -arch ppc -arch ppc7400 -arch ppc64 -arch i386 -arch x86_64
  CFLAGS += -I/sw/include -I$(externals_src)/pdp/include -DMACOSX -DUNIX -Dunix -DDL_OPEN
  LDFLAGS += -bundle -bundle_loader $(pd_src)/bin/pd-l2ork -undefined dynamic_lookup \
		-L/sw/lib -weak_framework Carbon
  LIBS += -lc 
  DYLIB_LDFLAGS = -dynamiclib -undefined dynamic_lookup -read_only_relocs warning -L/sw/lib
  STRIP = strip -x
endif
ifeq ($(OS_NAME),linux)
  CFLAGS += -I$(externals_src)/pdp/include -DUNIX -Dunix -DDL_OPEN -fPIC -O2
  LDFLAGS += -Wl,--export-dynamic  -shared -fPIC
  LIBS += -lc
  DYLIB_LDFLAGS = $(LDFLAGS)
  STRIP = strip --strip-unneeded -R .note -R .comment
endif
ifeq ($(OS_NAME),windows)
  BUILDSRC_OS_NAME = win
  WINDOWS_HACKS = -D'O_NONBLOCK=1' -D'srand48(n)=srand((n))' \
    -D'drand48()=((double)rand()/RAND_MAX)' -D'bzero(p,n)=memset(p,0,n)'
# These don't seem to be needed:
#	-D'PROT_READ=1' \
#	-D'MAP_PRIVATE=2' \
#	-D'O_NDELAY=O_NONBLOCK'
  CFLAGS += -mms-bitfields -DMSW -DNT $(WINDOWS_HACKS)
  LDFLAGS += -s -shared
# all of these included libs are part of libc in UNIX platforms.  All except
# libregex are in DLLs, so they get stripped from the external's .dll binary
  LIBS += -L$(pd_src)/src -L$(pd_src)/bin -L$(pd_src)/obj -lpd \
    -lwsock32 -liphlpapi -lpthread -lkernel32 -luser32 -lgdi32 -lregex
  DYLIB_LDFLAGS = -shared
  STRIP = strip --strip-unneeded -R .note -R .comment
endif

CXXFLAGS = $(CFLAGS)

### C++ files
%.$(EXTENSION): %.cpp
%.$(EXTENSION): %.cc
	$(CXX) $(OPT_CFLAGS) $(CXXFLAGS) -o "$*.o" -c "$<"
	$(CXX) $(LDFLAGS) -o "$*.$(EXTENSION)" "$*.o" $(LIBS)
	chmod a-x "$*.$(EXTENSION)"
#	$(STRIP) $*.$(EXTENSION)
#	rm -f -- $*.o

%.o: %.cpp
%.o: %.cc
	$(CXX) $(OPT_CFLAGS) $(CXXFLAGS) -o "$*.o" -c "$<"


### C files
%.o: %.c
	$(CC) $(OPT_CFLAGS) $(CFLAGS) -o "$*.o" -c "$*.c"

%.$(EXTENSION): %.o
	$(CC) $(LDFLAGS) -o "$*.$(EXTENSION)" "$*.o"  $(LIBS) \
		`test -f $*.libs && cat $*.libs`	\
		`my_dylib=$(patsubst $(externals_src)/%,%,$(@D)); test -f $(@D)/lib$${my_dylib}.$(DYLIB_EXTENSION) && echo -L$(@D) -l$$my_dylib` \
		`my_obj=$(patsubst $(externals_src)/%,%,$(@D)); test -f $(@D)/shared/$${my_obj}.o && echo $(@D)/shared/$${my_obj}.o` \
		`test -f $(dir $*)../$(BUILDSRC_OS_NAME)/$(notdir $*).libs && \
			cat $(dir $*)../$(BUILDSRC_OS_NAME)/$(notdir $*).libs`
	chmod a-x "$*.$(EXTENSION)"
	$(STRIP) $*.$(EXTENSION)
	rm -f -- $*.o




#------------------------------------------------------------------------------#
# ALL

# if your library isn't included in LIB_TARGETS, it won't be built with
# Pd-extended.  For libraries that build on all platforms, add them directly
# below, otherwise add to the correct platforms below.

#
# WARNING!  this MUST be all on one line because the automatic package
# building scripts rely on it being that way.
ifeq ($(LIGHT),yes)
# minimal set of extensions for the "light" build
LIB_TARGETS = loaders-libdir pddp
INCREMENTAL = yes
else
LIB_TARGETS = adaptive arraysize autotune bassemu boids bsaylor comport creb cxc cyclone disis earplug ekext ext13 fftease fluid freeverb ggee hcs iem_ambi iem_bin_ambi iemlib iemgui iemguts iem_adaptfilt iemmatrix iemxmlrpc iem_delay iem_roomsim iem_spec2 iem_tab jasch_lib loaders-libdir lyonpotpourri mapping markex maxlib mjlib moocow moonlib motex mrpeach oscx pan pdcontainer pddp pdlua pdogg plugin pmpd rjlib sigpack smlib tof unauthorized vbap windowing zexy
endif

# DISABLED: flatgui

# NEW (IN-PROGRESS): flext

# this is for libraries that don't compile (yet) on all platforms
ifneq ($(LIGHT),yes)
ifeq ($(OS_NAME),windows)
  ifneq ($(INCREMENTAL),yes)
    LIB_TARGETS += gem
  endif
else
  ifeq ($(OS_NAME),darwin)
# on Mac OS X 10.6/Snow Leopard don't build hid since it needs Carbon
    LIB_TARGETS += pdp iem16 apple
    ifneq ($(shell uname -r | sed 's|10\.[0-9][0-9]*\.[0-9][0-9]*|10|'),10)
      LIB_TARGETS +=
    endif
  else ifneq ($(LIGHT),yes)
    # GNU/Linux, BSD, IRIX, etc. (we use exported variable INCREMENTAL to
    # avoid rebuilding entire Gem lib that takes a long time to compile)
    ifeq ($(INCREMENTAL),yes)
	LIB_TARGETS += hid pdp iem16
    else
	LIB_TARGETS += gem pdp hid iem16
    endif
  endif
endif
endif # ifneq ($(LIGHT),yes)

#------------------------------------------------------------------------------#

all: $(LIB_TARGETS)
	@echo " "
	@echo "Compiled externals for $(OS_NAME) aka $(UNAME)"


install: $(examplesdir) $(manualsdir) $(objectsdir) \
all $(patsubst %, %_install,$(LIB_TARGETS))
	@echo " "
	@echo "externals install succeeded!"


#==============================================================================#
#
# OLD EXTERNALS BUILD SYSTEM TARGETS
#
# This is all stuff related to the externals/build/src/*.c links.  The idea is
# that they make for a flat namespace, here they are included as a libdir
#
#==============================================================================#

FLATSPACE_NAME=flatspace
FLATSPACE_OBJECTS := $(wildcard $(externals_src)/build/src/*.c)

flatspace: $(FLATSPACE_OBJECTS:.c=.$(EXTENSION))

flatspace_install: flatspace $(objectsdir)
	install -d $(DESTDIR)$(objectsdir)/$(FLATSPACE_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(FLATSPACE_NAME) \
		--author "Numerous" \
		--description "This is a collection of externals in a flat namespace" \
		--license "GNU GPL"
	install -p $(FLATSPACE_OBJECTS:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(FLATSPACE_NAME)
# all standard objs' help files, it had to be broken up because the list is
# soo long.  They are installed only inside of the libdir since this is so
# messy.  We don't these help patches to come up for other objects
	install -p \
			$(externals_src)/arraysize/*.pd \
			$(externals_src)/beatpipe/*.pd \
			$(externals_src)/bsaylor/help/*.pd \
			$(externals_src)/control/*/*.pd \
			$(externals_src)/bbogart/chaos/tools/*.pd \
			$(externals_src)/bbogart/*/*.pd \
				$(DESTDIR)$(objectsdir)/$(FLATSPACE_NAME)
	install -p \
			$(externals_src)/ff/*.pd \
			$(externals_src)/hcs/folder_list-help.pd \
			$(externals_src)/hcs/split_path-help.pd \
				$(DESTDIR)$(objectsdir)/$(FLATSPACE_NAME)
	install -p \
			$(externals_src)/iem/comport/*/*-help.pd \
			$(externals_src)/plugin~/*.pd \
			$(externals_src)/rhythm_estimator/*.p? \
				$(DESTDIR)$(objectsdir)/$(FLATSPACE_NAME)


flatspace_clean:
	-rm -f -- $(FLATSPACE_OBJECTS:.c=.o) 
	-rm -f -- $(FLATSPACE_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(externals_src)/build/src/*.*~
	-rm -f -- $(externals_src)/build/src/*.c.bak



#==============================================================================#
#
# PROJECT TARGETS
#
#==============================================================================#

# this is the template for creating new entries:

#------------------------------------------------------------------------------#
# TEMPLATE
TEMPLATE_NAME=template
TEMPLATE_OBJECTS := $(wildcard $(externals_src)/template/*.c)
template: $(TEMPLATE_OBJECTS:.c=.$(EXTENSION))

template_install: template
	install -d $(DESTDIR)$(objectsdir)/$(TEMPLATE_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(TEMPLATE_NAME) \
		--author "" \
		--description "" \
		--license "" \
		--version ""
	install -p $(TEMPLATE_OBJECTS:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(TEMPLATE_NAME)
#	install -d $(manualsdir)/$(TEMPLATE_NAME)
#	install -p $(externals_src)/template/manual.txt \
#		$(manualsdir)/$(TEMPLATE_NAME)
	install -d $(DESTDIR)$(examplesdir)/$(TEMPLATE_NAME)
	install -p $(externals_src)/template/examples/*.pd \
		$(DESTDIR)$(examplesdir)/$(TEMPLATE_NAME)

template_clean:
	-rm -f -- $(TEMPLATE_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(TEMPLATE_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/template/*.bak
	-rm -f -- $(externals_src)/template/*.*~




#------------------------------------------------------------------------------#
# AKA.WIIREMOTE
# this is installed into the "io" library
AKA.WIIREMOTE_NAME=io
AKA.WIIREMOTE_SRC := $(wildcard $(externals_src)/io/aka.wiiremote/*.c)

AKA.WIIREMOTE_OBJECTS := $(AKA.WIIREMOTE_SRC:.c=.o)
$(AKA.WIIREMOTE_OBJECTS) : %.o : %.c
	$(CC) $(OPT_CFLAGS) $(CFLAGS) -o "$*.o" -c "$*.c"

$(externals_src)/io/aka.wiiremote/akawiiremote.$(EXTENSION): $(AKA.WIIREMOTE_OBJECTS) 
	$(CC) $(LDFLAGS) -o $(externals_src)/io/aka.wiiremote/akawiiremote.$(EXTENSION) \
		$(AKA.WIIREMOTE_OBJECTS) -weak_framework IOBluetooth \
		-weak_framework CoreFoundation
#	$(STRIP) $(externals_src)/io/aka.wiiremote/aka.wiiremote.$(EXTENSION)

aka.wiiremote: $(externals_src)/io/aka.wiiremote/akawiiremote.$(EXTENSION)

aka.wiiremote_install: aka.wiiremote
	install -d $(DESTDIR)$(objectsdir)/$(AKA.WIIREMOTE_NAME)
	install -p $(externals_src)/io/aka.wiiremote/aka.wiiremote.$(EXTENSION) \
		$(DESTDIR)$(objectsdir)/$(AKA.WIIREMOTE_NAME)
	install -d $(DESTDIR)$(manualsdir)/$(AKA.WIIREMOTE_NAME)
	install -p $(externals_src)/io/aka.wiiremote/*.txt \
		$(DESTDIR)$(manualsdir)/$(AKA.WIIREMOTE_NAME)

aka.wiiremote_clean:
	-rm -f -- $(externals_src)/io/aka.wiiremote/aka.wiiremote.$(EXTENSION)
	-rm -f -- $(AKA.WIIREMOTE_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/io/aka.wiiremote/*.bak
	-rm -f -- $(externals_src)/io/aka.wiiremote/*.*~



#------------------------------------------------------------------------------#
# APPLE
apple:
	make -C $(externals_src)/apple PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

apple_install:
	make -C $(externals_src)/apple DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

apple_clean:
	make -C $(externals_src)/apple clean

#------------------------------------------------------------------------------#
# AUTOTUNE
autotune:
	make -C $(externals_src)/autotune PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

autotune_install:
	make -C $(externals_src)/autotune DESTDIR="$(DESTDIR)" \
		objectsdir="$(objectsdir)" install
	install -p $(externals_src)/autotune/autotune_scale_warp.png \
		$(DESTDIR)/$(objectsdir)/images

autotune_clean:
	make -C $(externals_src)/autotune clean

#------------------------------------------------------------------------------#
# ARRAYSIZE
arraysize:
	make -C $(externals_src)/arraysize PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

arraysize_install:
	make -C $(externals_src)/arraysize DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

arraysize_clean:
	make -C $(externals_src)/arraysize clean


#------------------------------------------------------------------------------#
# BASSEMU
bassemu:
	make -C $(externals_src)/bassemu~ PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

bassemu_install:
	make -C $(externals_src)/bassemu~ DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

bassemu_clean:
	make -C $(externals_src)/bassemu~ clean


#------------------------------------------------------------------------------#
# BOIDS
boids:
	make -C $(externals_src)/boids PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

boids_install:
	make -C $(externals_src)/boids \
		DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

boids_clean:
	make -C $(externals_src)/boids clean


#------------------------------------------------------------------------------#
# BSAYLOR
bsaylor:
	make -C $(externals_src)/bsaylor PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

bsaylor_install:
	make -C $(externals_src)/bsaylor \
		DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

bsaylor_clean:
	make -C $(externals_src)/bsaylor clean


#------------------------------------------------------------------------------#
# COMPORT
comport:
	make -C $(externals_src)/iem/comport/comport PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

comport_install:
	make -C $(externals_src)/iem/comport/comport \
		DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

comport_clean:
	make -C $(externals_src)/iem/comport/comport clean


#------------------------------------------------------------------------------#
# CREB
CREB_NAME=creb
CREB_OBJECTS := $(wildcard $(externals_src)/creb/modules/*.c)
CREB_CXXOBJECTS := $(wildcard $(externals_src)/creb/modules++/*.cc)

creb: $(CREB_OBJECTS:.c=.$(EXTENSION)) $(CREB_CXXOBJECTS:.cc=.$(EXTENSION))

creb_install: creb
	install -d $(DESTDIR)$(objectsdir)/$(CREB_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(CREB_NAME) \
		--author "Tom Schouten <tom@zwizwa.be>" \
		--description "This is a collection of pd externals. My bag of tricks." \
		--license "GNU GPL 2" \
		--version "0.9.2"
	install -p $(externals_src)/creb/abs/*.pd $(DESTDIR)$(objectsdir)/$(CREB_NAME)
	install -p $(CREB_OBJECTS:.c=.$(EXTENSION)) \
		$(CREB_CXXOBJECTS:.cc=.$(EXTENSION)) \
		$(DESTDIR)$(objectsdir)/$(CREB_NAME)
	install -p $(externals_src)/creb/doc/*.* $(DESTDIR)$(objectsdir)/$(CREB_NAME)
	install -d $(DESTDIR)$(objectsdir)/$(CREB_NAME)/manual
	install -p $(externals_src)/creb/doc/reference.txt \
		$(DESTDIR)$(objectsdir)/$(CREB_NAME)/manual
	install -d $(DESTDIR)$(objectsdir)/$(CREB_NAME)/examples
	install -p $(externals_src)/creb/doc/examples/*.* \
		$(DESTDIR)$(objectsdir)/$(CREB_NAME)/examples

creb_clean:
	-rm -f -- $(CREB_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(CREB_OBJECTS:.c=.o)
	-rm -f -- $(CREB_CXXOBJECTS:.cc=.$(EXTENSION))
	-rm -f -- $(CREB_CXXOBJECTS:.cc=.o)
	-rm -f -- $(externals_src)/creb/*/*.bak
	-rm -f -- $(externals_src)/creb/*/*.*~


#------------------------------------------------------------------------------#
# CXC
cxc:
	make -C $(externals_src)/cxc PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

cxc_install:
	make -C $(externals_src)/cxc DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

cxc_clean:
	make -C $(externals_src)/cxc clean


#------------------------------------------------------------------------------#
# CYCLONE
cyclone:
	make -C $(externals_src)/miXed/cyclone -f Makefile.libdir \
		PD_PATH=$(pd_src) PD_INCLUDE=$(DESTDIR)$(includedir)

cyclone_install:
	make -C $(externals_src)/miXed/cyclone -f Makefile.libdir \
		DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" \
		install

cyclone_clean:
	make -C $(externals_src)/miXed/cyclone -f Makefile.libdir \
		clean

#------------------------------------------------------------------------------#
# DISIS
disis:
ifneq ($(OS_NAME),windows)
ifneq ($(OS_NAME),darwin)
	cd $(externals_src)/disis/cwiid && aclocal && autoconf && \
		./configure --with-python=python2 && make
endif
endif
	make -C $(externals_src)/disis PD_PATH=$(pd_src) pdbinpath=$(pd_src)/src

disis_install:
	make -C $(externals_src)/disis \
		DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" \
		install

disis_clean:
ifneq ($(OS_NAME),windows)
ifneq ($(OS_NAME),darwin)
	make -C $(externals_src)/disis/cwiid clean
endif
endif
	make -C $(externals_src)/disis clean

#------------------------------------------------------------------------------#
# EKEXT
ekext:
	make -C $(externals_src)/ekext PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

ekext_install:
	make -C $(externals_src)/ekext DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

ekext_clean:
	make -C $(externals_src)/ekext clean


#------------------------------------------------------------------------------#
# EXT13
ext13:
	make -C $(externals_src)/ext13 PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

ext13_install:
	make -C $(externals_src)/ext13 DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

ext13_clean:
	make -C $(externals_src)/ext13 clean

#------------------------------------------------------------------------------#
# FFTEASE
FFTEASE_NAME=fftease

fftease:
	make -C $(externals_src)/$(FFTEASE_NAME) PD_PATH=$(pd_src)

fftease_install:
	#make -C $(externals_src)/$(FFTEASE_NAME) DESTDIR="$(DESTDIR)" \
	#	objectsdir="$(objectsdir)" install
	install -d $(DESTDIR)$(objectsdir)/$(FFTEASE_NAME)
	install -d $(DESTDIR)$(objectsdir)/$(FFTEASE_NAME)/sound
	install -p $(externals_src)/$(FFTEASE_NAME)/*.$(EXTENSION) \
		$(DESTDIR)$(objectsdir)/$(FFTEASE_NAME)
	install -p $(externals_src)/$(FFTEASE_NAME)/fftease32-helpfiles/sound/*.* \
		$(DESTDIR)$(objectsdir)/$(FFTEASE_NAME)/sound
	# copy the meta file and the shared lib
	install -p $(externals_src)/$(FFTEASE_NAME)/libfftease* \
		$(DESTDIR)$(objectsdir)/$(FFTEASE_NAME)
	# license and readme
	install -p $(externals_src)/$(FFTEASE_NAME)/*.txt \
		$(DESTDIR)$(objectsdir)/$(FFTEASE_NAME)
	# install help files
	install -p $(externals_src)/$(FFTEASE_NAME)/fftease32-helpfiles/*.pd \
		$(DESTDIR)$(objectsdir)/$(FFTEASE_NAME)

fftease_clean:
	make -C $(externals_src)/$(FFTEASE_NAME) clean

#------------------------------------------------------------------------------#
# FLATGUI
flatgui:
	make -C $(externals_src)/flatgui CFLAGS="$(CFLAGS)" \
		PD_PATH=$(pd_src) PD_INCLUDE=$(pd_src)/src

flatgui_install:
	make -C $(externals_src)/flatgui STRIP="$(STRIP)" \
		DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

flatgui_clean:
	make -C $(externals_src)/flatgui clean

#------------------------------------------------------------------------------#
# FLEXT and FLEXT externals
flext:
	cd $(externals_src)/grill/trunk/flext && \
	./bootstrap.sh || ./bootstrap.sh && \
	./build.sh pd gcc build && \
	./build.sh pd gcc build && \
	./build.sh pd gcc build && \
	./build.sh pd gcc install

	#fluid~
#	cd footils/fluid && \
#	../../grill/trunk/flext/build.sh pd gcc build && \
#	../../grill/trunk/flext/build.sh pd gcc build

	#disis_munger~
	cd ../l2ork_addons/disis_munger && \
	../../externals/grill/trunk/flext/build.sh pd gcc build && \
	../../externals/grill/trunk/flext/build.sh pd gcc build

flext_install:
	# we don't install flext but rather link it statically,
	# so we only install statically linked externals

	#fluid~
#	cd footils/fluid && \
#	install -D pd-linux/release-multi/fluid~.pd_linux $(DESTDIR)$(objectsdir)/flext/fluid~.pd_linux && \
#	install -D pd/* $(DESTDIR)$(objectsdir)/flext/

	#disis_munger~
	cd ../l2ork_addons/disis_munger && \
	install -D pd-linux/release-multi/disis_munger~.pd_linux $(DESTDIR)$(objectsdir)/flext/disis_munger~.pd_linux && \
	install -D *help.pd $(DESTDIR)$(objectsdir)/flext/

flext_clean:
	#flext
	cd grill/trunk/flext/ && \
	./build.sh pd gcc clean

	#fluid~
#	cd footils/fluid && \
#	../../grill/trunk/flext/build.sh pd gcc clean

	#fluid~
	cd ../l2ork_addons/disis_munger && \
	../../externals/grill/trunk/flext/build.sh pd gcc clean

#------------------------------------------------------------------------------#
# FLIB
FLIB_NAME=flib
# exclude the flib.c file for the single-file library
# plus ha~.c doesn't compile at the moment.
FLIB_OBJECTS := $(wildcard $(externals_src)/postlude/flib/src/[a-ei-z]*.c)
flib: $(FLIB_OBJECTS:.c=.$(EXTENSION))

flib_install: flib
	install -d $(DESTDIR)$(objectsdir)/$(FLIB_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(FLIB_NAME) \
		--author "Jamie Bullock" \
		--description "library for feature extraction" \
		--license "GNU GPL"
	install -p $(FLIB_OBJECTS:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(FLIB_NAME)
	install -p $(externals_src)/postlude/flib/doc/*.pd \
		$(DESTDIR)$(objectsdir)/$(FLIB_NAME)
	install -p $(externals_src)/postlude/flib/README \
		$(DESTDIR)$(objectsdir)/$(FLIB_NAME)/README.txt

flib_clean:
	-rm -f -- $(FLIB_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(FLIB_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/postlude/flib/*/*.bak
	-rm -f -- $(externals_src)/postlude/flib/*/*.*~

#------------------------------------------------------------------------------#
# FLUID~

fluid:
	make -C $(externals_src)/fluid~ PD_PATH=$(pd_src) \
		pdbinpath=$(pd_src)/src

fluid_install:
	make -C $(externals_src)/fluid~ DESTDIR="$(DESTDIR)" \
		objectsdir="$(objectsdir)" install

fluid_clean:
	make -C $(externals_src)/fluid~ clean

#------------------------------------------------------------------------------#
# FREEVERB
freeverb:
	make -C $(externals_src)/freeverb~ PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

freeverb_install:
	make -C $(externals_src)/freeverb~ DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

freeverb_clean:
	make -C $(externals_src)/freeverb~ clean


#------------------------------------------------------------------------------
# GEM
GEM_NAME = Gem
$(gem_src)/configure:
	cd $(gem_src) && ./autogen.sh

$(gem_src)/Gem.dll: $(gem_src)/configure
	cd $(gem_src) && ./configure \
		CXXFLAGS="-DHAVE_S_STUFF_H" \
		--host=i686-w64-mingw32 \
		--without-ALL \
		--with-jpeg \
		--with-jpeg-cflags="-I/mingw32/include" \
		--with-jpeg-libs="-L/mingw32/lib -ljpeg" \
		--with-ftgl \
		--with-ftgl-cflags="-I/mingw32/include -I/mingw32/include/freetype2" \
		--with-ftgl-libs="-L/mingw32/lib -lftgl" \
		--with-vfw32 \
		--prefix=$(prefix) \
		--libdir=$(objectsdir) \
		--with-pd=$(DESTDIR)
	$(MAKE) -C $(gem_src)

$(gem_src)/Gem.pd_linux: $(gem_src)/configure
	test -s $(gem_src)/Gem.pd_linux || \
		cd $(gem_src) && ./configure \
			CXXFLAGS="-DHAVE_S_STUFF_H" \
			--prefix=$(prefix) \
			--with-video=plugins \
			--with-film=plugins \
			--with-pd=$(pd_src)
	$(MAKE) -C $(gem_src)

$(gem_src)/Gem.pd_darwin: $(gem_src)/configure
	cd $(gem_src) && ./configure \
		CXXFLAGS="-DHAVE_S_STUFF_H" \
		PKG_FTGL_CFLAGS="-I/sw/include -I/sw/include/freetype2 -I/sw/include/FTGL" \
		--prefix=$(prefix) \
		--libdir=$(objectsdir) \
		--without-ALL \
		--with-ftgl --with-ftgl-libs=/sw/lib \
		--with-artoolkit-includes=/sw/include --with-artoolkit-libs=/sw/lib \
		--with-video=plugins \
		--with-film=plugins \
		--with-pd=$(pd_src)
	$(MAKE) -C $(gem_src)

gem: $(gem_src)/Gem.$(EXTENSION)

gem_install: gem
	$(MAKE) -C $(gem_src) DESTDIR=$(DESTDIR) prefix=$(prefix) \
		libdir=$(objectsdir) pkglibdir=$(objectsdir)/Gem install

gem_clean:
	-$(MAKE) -C $(gem_src) clean


#------------------------------------------------------------------------------#
# GEM2PDP
GEM2PDP_NAME=gem2pdp
GEM2PDP_OBJECTS := $(wildcard $(externals_src)/gem2pdp/*.cpp)
$(externals_src)/gem2pdp/configure: $(externals_src)/gem2pdp/configure.ac
	cd $(externals_src)/gem2pdp && aclocal && autoconf

$(externals_src)/gem2pdp/Makefile: $(externals_src)/gem2pdp/Makefile.in
	cd $(externals_src)/gem2pdp && ./configure --with-pddir=$(pd_src) \
		--with-gemdir=$(gem_src)  --with-pdpdir=$(externals_src)/pdp

gem2pdp: $(externals_src)/gem2pdp/configure $(externals_src)/gem2pdp/Makefile
	$(MAKE) -C $(externals_src)/gem2pdp

gem2pdp_install: gem2pdp
	install -d $(DESTDIR)$(objectsdir)
#	install -d $(DESTDIR)$(objectsdir)/$(GEM2PDP_NAME)
#	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(GEM2PDP_NAME) \
#		--author "Yves Degoyon, Jamie Tittle, Georg Holzmann" \
#		--description "Bridges between Gem and PDP" \
#		--version "0.6"
	install -p $(GEM2PDP_OBJECTS:.cpp=.$(EXTENSION)) $(DESTDIR)$(objectsdir)
	install -p $(externals_src)/gem2pdp/*.pd  $(DESTDIR)$(objectsdir)
	install -p $(externals_src)/gem2pdp/README \
		$(DESTDIR)$(objectsdir)/$(GEM2PDP_NAME)-README.txt

gem2pdp_clean:
	-rm -rf -- $(externals_src)/gem2pdp/autom4te.cache
	-rm -f -- $(externals_src)/gem2pdp/config.status
	-rm -f -- $(externals_src)/gem2pdp/config.log
	-rm -f -- $(externals_src)/gem2pdp/configure
	-rm -f -- $(externals_src)/gem2pdp/Makefile
	-rm -f -- $(GEM2PDP_OBJECTS:.cpp=.$(EXTENSION))
	-rm -f -- $(externals_src)/gem2pdp/*.o
	-rm -f -- $(externals_src)/gem2pdp/*.bak
	-rm -f -- $(externals_src)/gem2pdp/*.*~


#------------------------------------------------------------------------------#
# GGEE
ggee:
	make -C $(externals_src)/ggee PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

ggee_install:
	make -C $(externals_src)/ggee DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install
	cp $(externals_src)/ggee/gui/logo100.gif $(DESTDIR)/$(objectsdir)/ggee
	cp $(externals_src)/ggee/gui/empty_image.png $(DESTDIR)/$(objectsdir)/ggee

ggee_clean:
	make -C $(externals_src)/ggee clean


#------------------------------------------------------------------------------#
# HCS
hcs:
	make -C $(externals_src)/hcs PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

hcs_install:
	make -C $(externals_src)/hcs DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

hcs_clean:
	make -C $(externals_src)/hcs clean


#------------------------------------------------------------------------------#
# HID
HID_NAME = hid
ifeq ($(OS_NAME),windows)
  HID_CFLAGS = $(OPT_CFLAGS) $(CFLAGS) 
  HID_LIBS = $(LIBS) -lhid -lsetupapi
else
  ifeq ($(OS_NAME),darwin)
    FRAMEWORKS = Carbon IOKit ForceFeedback
    HID_CFLAGS = $(OPT_CFLAGS) $(CFLAGS) -I$(externals_src)/hid/HID\ Utilities\ Source
    HID_UTILITIES_SOURCE = $(externals_src)/hid/HID\ Utilities\ Source
    HID_LIBS = $(LIBS) -L$(HID_UTILITIES_SOURCE)/build \
		-L$(HID_UTILITIES_SOURCE)/build/Default \
      -lHIDUtilities $(patsubst %,-weak_framework %,$(FRAMEWORKS))
  else
    HID_CFLAGS = $(OPT_CFLAGS) $(CFLAGS) 
    HID_LIBS = $(LIBS)
  endif
endif

HID_SRC = input_arrays.c hid_$(OS_NAME).c hid.c
HID_OBJECTS := $(patsubst %.c, $(externals_src)/hid/%.o, $(HID_SRC))
$(HID_OBJECTS) : %.o : %.c
	$(CC) $(HID_CFLAGS) -o "$*.o" -c "$*.c"

$(HID_UTILITIES_SOURCE)/build/Default/libHIDUtilities.a:
# Apple changed the XCode CLI tool's name in xcode2... arg
# if on non-Mac OS X, this target just echos a message
ifeq ($(UNAME),Darwin)
	cd $(HID_UTILITIES_SOURCE) && \
		(test -x /usr/bin/xcodebuild && /usr/bin/xcodebuild) || \
			(test -x /usr/bin/pbxbuild && /usr/bin/pbxbuild) || \
				echo "Not building Apple HID Utilities"
endif

$(externals_src)/hid/hid.$(EXTENSION): $(HID_OBJECTS) \
$(HID_UTILITIES_SOURCE)/build/Default/libHIDUtilities.a
	$(CC) $(LDFLAGS) -o $(externals_src)/hid/hid.$(EXTENSION) \
		$(HID_OBJECTS) $(HID_LIBS)
#	$(STRIP) $(externals_src)/hid/hid.$(EXTENSION)

hid: $(externals_src)/hid/hid.$(EXTENSION)

hid_install: hid
	install -d $(DESTDIR)$(objectsdir)/$(HID_NAME)
	install -p $(externals_src)/hid/hid.$(EXTENSION) \
		$(DESTDIR)$(objectsdir)/$(HID_NAME)
	install -p $(externals_src)/hid/*.pd $(DESTDIR)$(objectsdir)/$(HID_NAME)
	install -d $(DESTDIR)$(objectsdir)/$(HID_NAME)/examples
	install -p $(externals_src)/hid/examples/*.pd  \
		$(DESTDIR)$(objectsdir)/$(HID_NAME)/examples

hid_clean:
	-rm -f -- $(externals_src)/hid/*.o
	-rm -f -- $(externals_src)/hid/hid.$(EXTENSION)
	-rm -f -- $(externals_src)/hid/*.bak
	-rm -f -- $(externals_src)/hid/*.*~
	-rm -f -- $(HID_UTILITIES_SOURCE)/build/libHIDUtilities.a
	-rm -f -- $(HID_UTILITIES_SOURCE)/build/Default/libHIDUtilities.a


#------------------------------------------------------------------------------#
# HIDIN
HIDIN_NAME=hidin
HIDIN_OBJECTS := $(wildcard $(externals_src)/olafmatt/hidin/*.c)
HIDIN_DLL := $(externals_src)/olafmatt/hidin/hidin.$(EXTENSION)

$(HIDIN_DLL): $(HIDIN_OBJECTS:.c=.o)
	$(CC) $(LDFLAGS) -o "$(HIDIN_DLL)" $(HIDIN_OBJECTS:.c=.o) $(LIBS) -lhid \
		-lsetupapi
	chmod a-x "$(HIDIN_DLL)"
	$(STRIP) $(HIDIN_DLL)

hidin: $(HIDIN_DLL)

hidin_install: hidin
	install -d $(DESTDIR)$(objectsdir)/$(HIDIN_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(HIDIN_NAME) \
		--author "Olaf Matthes" \
		--description "HID input for Windows HID" \
		--license "GNU GPL"
	install -p $(HIDIN_DLL) $(DESTDIR)$(objectsdir)/$(HIDIN_NAME)
#	install -d $(DESTDIR)$(manualsdir)/$(HIDIN_NAME)
#	install -p $(externals_src)/olafmatt/hidin/README \
#		$(DESTDIR)$(objectsdir)/$(HIDIN_NAME)
#	install -d $(DESTDIR)$(examplesdir)/$(HIDIN_NAME)
#	install -p $(externals_src)/olafmatt/hidin/examples/*.pd \
#		$(DESTDIR)$(examplesdir)/$(HIDIN_NAME)

hidin_clean:
	-rm -f -- $(HIDIN_DLL)
	-rm -f -- $(HIDIN_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/olafmatt/hidin/*.bak
	-rm -f -- $(externals_src)/olafmatt/hidin/*.*~



#------------------------------------------------------------------------------#
# IEM_AMBI
IEM_AMBI_NAME=iem_ambi
# exclude the files for the single-file library format
IEM_AMBI_OBJECTS := $(wildcard $(externals_src)/iem/iem_ambi/src/[a-hj-z]*.c)
iem_ambi: $(IEM_AMBI_OBJECTS:.c=.$(EXTENSION))

iem_ambi_install: iem_ambi
	install -d $(DESTDIR)$(objectsdir)/$(IEM_AMBI_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEM_AMBI_NAME) \
		--author "IEM/KUG, Graz, Austria" \
		--description "calculate ambisonic encoder matrices rotation matrices and decoder matrices from 1st to 4th order in 2D or 3D." \
		--license "GNU GPL" \
		--version ""
	install -p $(IEM_AMBI_OBJECTS:.c=.$(EXTENSION)) \
		$(DESTDIR)$(objectsdir)/$(IEM_AMBI_NAME)
	install -p $(externals_src)/iem/iem_ambi/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEM_AMBI_NAME)
#	install -d $(DESTDIR)$(manualsdir)/$(IEM_AMBI_NAME)
	install -p $(externals_src)/iem/iem_ambi/READ_ME.txt \
		$(DESTDIR)$(objectsdir)/$(IEM_AMBI_NAME)/README.txt
#	install -d $(DESTDIR)$(examplesdir)/$(IEM_AMBI_NAME)
#	install -p $(externals_src)/iem/iem_ambi/examples/*.pd \
#		$(DESTDIR)$(examplesdir)/$(IEM_AMBI_NAME)

iem_ambi_clean:
	-rm -f -- $(IEM_AMBI_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(IEM_AMBI_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/iem/iem_ambi/*/*.bak
	-rm -f -- $(externals_src)/iem/iem_ambi/*/*.*~



#------------------------------------------------------------------------------#
# IEM_BIN_AMBI
IEM_BIN_AMBI_NAME=iem_bin_ambi
# exclude the files for the single-file library format
IEM_BIN_AMBI_OBJECTS := $(wildcard $(externals_src)/iem/iem_bin_ambi/src/[a-hj-z]*.c)
iem_bin_ambi: $(IEM_BIN_AMBI_OBJECTS:.c=.$(EXTENSION))

iem_bin_ambi_install: iem_bin_ambi
	install -d $(DESTDIR)$(objectsdir)/$(IEM_BIN_AMBI_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEM_BIN_AMBI_NAME) \
		--author "IEM/KUG, Graz, Austria" \
		--description "calculate the product of an ambisonic decoder-matrix and the binaural HRIR's (in frequency and in time domain)" \
		--license "GNU GPL" \
		--version ""
	install -p $(IEM_BIN_AMBI_OBJECTS:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(IEM_BIN_AMBI_NAME)
	install -p $(externals_src)/iem/iem_bin_ambi/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEM_BIN_AMBI_NAME)
#	install -d $(DESTDIR)$(manualsdir)/$(IEM_BIN_AMBI_NAME)
	install -p $(externals_src)/iem/iem_bin_ambi/READ_ME.txt \
		$(DESTDIR)$(objectsdir)/$(IEM_BIN_AMBI_NAME)/README.txt
#	install -d $(DESTDIR)$(examplesdir)/$(IEM_BIN_AMBI_NAME)
#	install -p $(externals_src)/iem/iem_bin_ambi/examples/*.pd \
#		$(DESTDIR)$(examplesdir)/$(IEM_BIN_AMBI_NAME)

iem_bin_ambi_clean:
	-rm -f -- $(IEM_BIN_AMBI_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(IEM_BIN_AMBI_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/iem/iem_bin_ambi/*/*.bak
	-rm -f -- $(externals_src)/iem/iem_bin_ambi/*/*.*~



#------------------------------------------------------------------------------#
# IEM16
# more externals in one file - so a little bit complicated ... ;)
IEM16_NAME=iem16
IEM16_DIR := $(externals_src)/iem16/src
IEM16_SRC := $(wildcard $(IEM16_DIR)/*.c)

$(IEM16_DIR)/aclocal.m4: $(IEM16_DIR)/acinclude.m4
	cd $(IEM16_DIR) && aclocal

$(IEM16_DIR)/configure: $(IEM16_DIR)/configure.ac $(IEM16_DIR)/aclocal.m4
	cd $(IEM16_DIR) && autoconf

$(IEM16_DIR)/Make.config: $(IEM16_DIR)/Make.config.in \
$(IEM16_DIR)/configure
	cd $(IEM16_DIR) && ./configure --disable-library \
		--with-pd=$(pd_src)

iem16: $(IEM16_DIR)/Make.config
	$(MAKE) -C $(IEM16_DIR)


iem16_install: iem16
	install -d $(DESTDIR)$(objectsdir)/$(IEM16_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEM16_NAME) \
		--author "IOhannes m zmoelnig" \
		--description "16bit table/array objects for low memory usage" \
		--license "GNU GPL" \
		--version "0.1"
	install -p $(IEM16_DIR)/*.$(EXTENSION) $(DESTDIR)$(objectsdir)/$(IEM16_NAME)
	install -p $(externals_src)/iem16/help/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEM16_NAME)

iem16_clean:
	-$(MAKE) -C $(IEM16_DIR) clean
	-rm -f -- $(IEM16_DIR)/*.$(EXTENSION)
	-rm -f -- $(IEM16_DIR)/*.d
	-rm -rf -- $(IEM16_DIR)/autom4ate
	-rm -rf -- $(IEM16_DIR)/conf[0-9][0-9][0-9]*
	-rm -f -- $(IEM16_DIR)/configure
	-rm -f -- $(IEM16_DIR)/Make.config
	-rm -f -- $(IEM16_DIR)/*.o
	-rm -f -- $(IEM16_DIR)/*.bak
	-rm -f -- $(IEM16_DIR)/*.*~

#------------------------------------------------------------------------------#
# IEMGUTS
IEMGUTS_NAME=iemguts
IEMGUTS_OBJECTS := $(wildcard $(externals_src)/iem/iemguts/src/*.c)
iemguts: $(IEMGUTS_OBJECTS:.c=.$(EXTENSION))


iemguts_install: iemguts
	install -d $(DESTDIR)$(objectsdir)/$(IEMGUTS_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEMGUTS_NAME) \
		--author "IOhannes m zmoelnig" \
		--description "IEMguts is a collection of objects that deal with the infrastructure to build better abstractions" \
		--license "GNU GPL" \
		--version ""
	install -p $(IEMGUTS_OBJECTS:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(IEMGUTS_NAME)
	install -p $(externals_src)/iem/iemguts/help/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEMGUTS_NAME)
	install -p $(externals_src)/iem/iemguts/README.txt \
		$(DESTDIR)$(objectsdir)/$(IEMGUTS_NAME)
	install -d $(DESTDIR)$(examplesdir)/$(IEMGUTS_NAME)
	install -p $(externals_src)/iem/iemguts/examples/*.pd \
		$(DESTDIR)$(examplesdir)/$(IEMGUTS_NAME)

iemguts_clean:
	-rm -f -- $(IEMGUTS_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(IEMGUTS_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/iem/iemguts/*.bak
	-rm -f -- $(externals_src)/iem/iemguts/*.*~



#------------------------------------------------------------------------------#
# IEMLIB
IEMLIB_NAME=iemlib
# omit the lib files and iem_mp3 (for patent reasons :( )
IEMLIB_SRC := $(wildcard $(externals_src)/iemlib/iemlib1/src/*[^1].c) $(wildcard $(externals_src)/iemlib/iemlib2/src/*[^2].c)  $(wildcard $(externals_src)/iemlib/iem_t3_lib/src/t3_*.c)  $(wildcard $(externals_src)/iemlib/alias/*.c)

IEMLIB_OBJECTS := $(IEMLIB_SRC:.c=.o)
$(IEMLIB_OBJECTS) : %.o : %.c
	$(CC) -I$(externals_src)/iemlib/include $(CFLAGS) -O2 -funroll-loops -fomit-frame-pointer -fno-tree-vectorize -fno-strict-aliasing -o "$*.o" -c "$*.c"

iemlib: $(IEMLIB_SRC:.c=.$(EXTENSION))

iemlib_install: iemlib
	install -d $(DESTDIR)$(objectsdir)/$(IEMLIB_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEMLIB_NAME) \
		--description "a collection of objects written at IEM/KUG" \
		--license "GNU GPL"
	install -p $(IEMLIB_SRC:.c=.$(EXTENSION)) \
		$(externals_src)/iemlib/*/*.pd \
		$(externals_src)/iemlib/*/*.mp3 \
		$(externals_src)/iemlib/*/*.wav \
		$(DESTDIR)$(objectsdir)/$(IEMLIB_NAME)
	install -d $(DESTDIR)$(objectsdir)/$(IEMLIB_NAME)/examples
	install -p $(externals_src)/iemlib/examples/*.* \
		$(DESTDIR)$(objectsdir)/$(IEMLIB_NAME)/examples
	install -d $(DESTDIR)$(objectsdir)/$(IEMLIB_NAME)/manual
	install -p $(externals_src)/iemlib/*.pdf $(externals_src)/iemlib/*.txt \
		$(externals_src)/iemlib/iemabs/*.txt \
		$(DESTDIR)$(objectsdir)/$(IEMLIB_NAME)/manual
# kludge to get a single working output~ straight in 'extra'
	-rm -f -- $(DESTDIR)$(objectsdir)/$(IEMLIB_NAME)/output~.pd


iemlib_clean: 
	-rm -f -- $(IEMLIB_OBJECTS)
	-rm -f -- $(IEMLIB_SRC:.c=.$(EXTENSION))
	-rmdir -- $(DESTDIR)$(objectsdir)/$(IEMLIB_NAME)
	-rm -f -- $(DESTDIR)$(manualsdir)/$(IEMLIB_NAME)/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/$(IEMLIB_NAME)



#------------------------------------------------------------------------------#
# IEMMATRIX
## oh this is all sh!t: please use iemmatrix's build-system instead
IEMMATRIX_NAME=iemmatrix
IEMMATRIX_ROOT := $(externals_src)/iem/iemmatrix
IEMMATRIX_SRC := $(wildcard $(IEMMATRIX_ROOT)/src/m[at]*.c)
IEMMATRIX_ALIAS := $(wildcard $(IEMMATRIX_ROOT)/alias/*.c)
IEMMATRIX_OBJ := $(IEMMATRIX_SRC:.c=.o) $(IEMMATRIX_ALIAS:.c=.o)
IEMMATRIX_SHARED := $(wildcard $(IEMMATRIX_ROOT)/src/iemmatrix_binops.c) \
                    $(wildcard $(IEMMATRIX_ROOT)/src/iemmatrix_utility.c)

iemmatrix: $(IEMMATRIX_OBJ:.o=.$(EXTENSION))

$(IEMMATRIX_OBJ:.o=.$(EXTENSION)) : %.$(EXTENSION) : $(IEMMATRIX_OBJ) $(IEMMATRIX_SHARED:.c=.o)
	$(CC) $(LDFLAGS) -o $*.$(EXTENSION) "$*.o" $(IEMMATRIX_SHARED:.c=.o) $(LIBS)
	$(STRIP) $*.$(EXTENSION)
	chmod 755 $*.$(EXTENSION)
#	rm -f -- "$*.o"

$(IEMMATRIX_OBJ) $(IEMMATRIX_SHARED:.c=.o) : %.o : %.c
	$(CC) $(OPT_CFLAGS) $(CFLAGS) -o "$*.o" -c "$*.c" -I$(IEMMATRIX_ROOT)/src

iemmatrix_install: iemmatrix
	install -d $(DESTDIR)$(objectsdir)/$(IEMMATRIX_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEMMATRIX_NAME) \
		--author "IOhannes m zmoelnig (zmoelnig AT iem DOT at), thomas musil (musil AT iem DOT at), franz zotter (zotter AT iem DOT at)" \
		--description "objects for matrix operations and math" \
		--license "GNU GPL" \
		--version "$(IEMMATRIX_VERSION)"
	install -p $(IEMMATRIX_OBJ:.o=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(IEMMATRIX_NAME)
	install -p $(shell ls -1 $(externals_src)/iem/iemmatrix/abs/*.* | \
		grep -v '\-help.pd') $(DESTDIR)$(objectsdir)/$(IEMMATRIX_NAME)
	install -p $(externals_src)/iem/iemmatrix/doc/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEMMATRIX_NAME)
	install -p $(externals_src)/iem/iemmatrix/abs/*-help.pd \
		$(DESTDIR)$(objectsdir)/$(IEMMATRIX_NAME)
	install -d $(DESTDIR)$(objectsdir)/$(IEMMATRIX_NAME)/manual
	install -p $(externals_src)/iem/iemmatrix/*.txt \
		$(DESTDIR)$(objectsdir)/$(IEMMATRIX_NAME)/manual

iemmatrix_clean:
	-rm -f -- $(IEMMATRIX_OBJ:.o=.$(EXTENSION))
	-rm -f -- $(externals_src)/iem/iemmatrix/src/*.o
	-rm -f -- $(externals_src)/iem/iemmatrix/alias/*.o
	-rm -f -- $(externals_src)/iem/iemmatrix/src/*.bak
	-rm -f -- $(externals_src)/iem/iemmatrix/src/*.*~


#------------------------------------------------------------------------------#
# JASCH_LIB
JASCH_LIB_NAME=jasch_lib
JASCH_LIB_OBJECTS := $(wildcard $(externals_src)/jasch_lib/*/*.c)
jasch_lib: $(JASCH_LIB_OBJECTS:.c=.$(EXTENSION))

jasch_lib_install: jasch_lib
	install -d $(DESTDIR)$(objectsdir)/$(JASCH_LIB_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(JASCH_LIB_NAME) \
		--author "Jasch" \
		--license "GNU GPLv2" \
		--version ""
	install -p $(JASCH_LIB_OBJECTS:.c=.$(EXTENSION)) \
		$(DESTDIR)$(objectsdir)/$(JASCH_LIB_NAME)
	install -p $(externals_src)/jasch_lib/*/*-help.pd \
		$(DESTDIR)$(objectsdir)/$(JASCH_LIB_NAME)
#	install -d $(DESTDIR)$(manualsdir)/$(JASCH_LIB_NAME)
#	install -p $(externals_src)/jasch_lib/manual.txt \
#		$(DESTDIR)$(manualsdir)/$(JASCH_LIB_NAME)
#	install -p $(externals_src)/jasch_lib/README \
#		$(DESTDIR)$(objectsdir)/$(JASCH_LIB_NAME)
#	install -d $(DESTDIR)$(examplesdir)/$(JASCH_LIB_NAME)
#	install -p $(externals_src)/jasch_lib/examples/*.pd \
#		$(DESTDIR)$(examplesdir)/$(JASCH_LIB_NAME)

jasch_lib_clean:
	-rm -f -- $(JASCH_LIB_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(JASCH_LIB_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/jasch_lib/*/*.bak
	-rm -f -- $(externals_src)/jasch_lib/*/*.*~

#------------------------------------------------------------------------------#
# LOADERS-LIBDIR
loaders-libdir:
	make -C $(externals_src)/loaders/libdir PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

loaders-libdir_install:
	make -C $(externals_src)/loaders/libdir DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

loaders-libdir_clean:
	make -C $(externals_src)/loaders/libdir clean

#------------------------------------------------------------------------------#
# LYON
# Ivica named the libdir "lyon" for Pd-l2ork, but the submodule is named
# lyonpotpourri. So we must do extra work to differentiate the two...
LYON_SRC=lyonpotpourri
LYON_DEST=lyonpotpourri

lyonpotpourri:
	make -C $(externals_src)/$(LYON_SRC) PD_PATH=$(pd_src)

lyonpotpourri_install:
	#make -C $(externals_src)/$(LYON_SRC) DESTDIR="$(DESTDIR)" \
	#	objectsdir="$(objectsdir)" install
	install -d $(DESTDIR)$(objectsdir)/$(LYON_DEST)
	install -p $(wildcard $(externals_src)/$(LYON_SRC)/*.$(EXTENSION)) \
		$(DESTDIR)$(objectsdir)/$(LYON_DEST)
	install -p $(externals_src)/$(LYON_SRC)/*.pd \
		$(DESTDIR)$(objectsdir)/$(LYON_DEST)
	install -d $(DESTDIR)$(objectsdir)/$(LYON_DEST)/sound
	install -p $(externals_src)/$(LYON_SRC)/examples/*.* \
		$(DESTDIR)$(objectsdir)/$(LYON_DEST)/sound
	# some of the help patches expect to find the sounds in the examples
	# subfolder instead of 'sound', so make sure that both are available
ifeq ($(OS_NAME),windows)
	# symbolic links don't work under Windows, just copy the folder instead
	rm -rf $(DESTDIR)$(objectsdir)/$(LYON_DEST)/examples
	cp -r $(DESTDIR)$(objectsdir)/$(LYON_DEST)/sound  $(DESTDIR)$(objectsdir)/$(LYON_DEST)/examples
else
	rm -f $(DESTDIR)$(objectsdir)/$(LYON_DEST)/examples
	ln -sf sound $(DESTDIR)$(objectsdir)/$(LYON_DEST)/examples
endif
	# don't include cartopol/poltocar-- cyclone library already has those
	rm $(DESTDIR)$(objectsdir)/$(LYON_DEST)/cartopol*.$(EXTENSION)
	rm $(DESTDIR)$(objectsdir)/$(LYON_DEST)/poltocar*.$(EXTENSION)
	rm $(DESTDIR)$(objectsdir)/$(LYON_DEST)/cartopol*.pd
	rm $(DESTDIR)$(objectsdir)/$(LYON_DEST)/poltocar*.pd
	# copy the meta file and the shared lib
	install -p $(externals_src)/$(LYON_SRC)/*lyonpotpourri*.* \
		$(DESTDIR)$(objectsdir)/$(LYON_DEST)
	# license and readme
	install -p $(externals_src)/$(LYON_SRC)/*.txt \
		$(DESTDIR)$(objectsdir)/$(LYON_DEST)

lyonpotpourri_clean:
	make -C $(externals_src)/$(LYON_SRC) clean

#------------------------------------------------------------------------------#
# MAPPING
mapping:
	make -C $(externals_src)/mapping PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

mapping_install:
	make -C $(externals_src)/mapping DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

mapping_clean:
	make -C $(externals_src)/mapping clean

#------------------------------------------------------------------------------#
# MARKEX
markex:
	make -C $(externals_src)/markex PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

markex_install:
	make -C $(externals_src)/markex DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

markex_clean:
	make -C $(externals_src)/markex clean

#------------------------------------------------------------------------------#
# MAXLIB
maxlib:
	make -C $(externals_src)/maxlib PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

maxlib_install:
	make -C $(externals_src)/maxlib DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

maxlib_clean:
	make -C $(externals_src)/maxlib clean


#------------------------------------------------------------------------------#
# MJLIB
mjlib:
	make -C $(externals_src)/mjlib PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

mjlib_install:
	make -C $(externals_src)/mjlib DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

mjlib_clean:
	make -C $(externals_src)/mjlib clean



#------------------------------------------------------------------------------#
# MOOCOW
MOOCOW_NAME=moocow
#MOOCOW_OBJECTS := $(shell cat $(externals_src)/moocow/extended/objects)
MOOCOW_DIR=$(externals_src)/moocow/extended
MOOCOW_BUILD=$(MOOCOW_DIR)/build.moo

##-- pass some variables on to sub-make
## + we should probably just use make's "export" for this,
##   maybe even exporting all variables by default...
MOOCOW_MAKEFLAGS = \
	CFLAGS="$(CFLAGS)" \
	pd_src="$(pd_src)"

$(externals_src)/moocow/extended/build.stamp:
	$(MAKE) -C $(MOOCOW_DIR) $(MOOCOW_MAKEFLAGS) build.stamp \
	  || echo "moocow: WARNING: build failed"

moocow: $(MOOCOW_DIR)/build.stamp

moocow_install:
	install -d $(DESTDIR)$(objectsdir)/$(MOOCOW_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(MOOCOW_NAME) \
		--author "Bryan Jurish <moocow@bbaw.de>" \
		--description "moocow's externals" \
		--license "GNU GPL" \
		--version "CVS.`date +%Y-%m-%d`"
	install -p $(MOOCOW_BUILD)/ext*/*.$(EXTENSION) \
		$(DESTDIR)$(objectsdir)/$(MOOCOW_NAME) \
	  || echo 'moocow_install: WARNING: no library externals to install!'
	install -p $(MOOCOW_BUILD)/ext*/*.pd \
		$(DESTDIR)$(objectsdir)/$(MOOCOW_NAME) \
	  || echo 'moocow_install: WARNING: no library patches to install!'
	install -p $(MOOCOW_BUILD)/doc/5.reference/*.pd \
		$(DESTDIR)$(objectsdir)/$(MOOCOW_NAME) \
	  || echo 'moocow_install: WARNING: no help patches to install!'
#	install -d $(DESTDIR)$(manualsdir)/$(MOOCOW_NAME)
#	install -p $(externals_src)/moocow/manual.txt \
#		$(DESTDIR)$(manualsdir)/$(MOOCOW_NAME) \
#	  || echo 'moocow_install: WARNING: no manuals to install!'
	install -p $(MOOCOW_DIR)/README.txt \
		$(DESTDIR)$(objectsdir)/$(MOOCOW_NAME) \
	  || echo 'moocow_install: WARNING: no README to install!'
#	install -d $(DESTDIR)$(examplesdir)/$(MOOCOW_NAME)
#	install -p $(MOOCOW_BUILD)/examples/*.pd \
#		$(DESTDIR)$(examplesdir)/$(MOOCOW_NAME) \
#	  || echo "moocow_install: WARNING: no examples patches to install!"

moocow_clean:
	$(MAKE) -C $(externals_src)/moocow/extended distclean
	-rm -f -- $(MOOCOW_DIR)/*.bak
	-rm -f -- $(MOOCOW_DIR)/*.*~
	cp -f $(MOOCOW_DIR)/../gfsm/gfsm/src/libgfsm/default/* $(MOOCOW_DIR)/../gfsm/gfsm/src/libgfsm/ 


#------------------------------------------------------------------------------#
# MOONLIB
moonlib:
	make -C $(externals_src)/moonlib PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

moonlib_install:
	make -C $(externals_src)/moonlib DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

moonlib_clean:
	make -C $(externals_src)/moonlib clean



#------------------------------------------------------------------------------#
# MOTEX
motex:
	make -C $(externals_src)/motex PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

motex_install:
	make -C $(externals_src)/motex DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

motex_clean:
	make -C $(externals_src)/motex clean




#------------------------------------------------------------------------------#
# MRPEACH
MRPEACH_NAME=mrpeach
MRPEACH_OBJECTS := $(wildcard $(externals_src)/mrpeach/*/*.c)
mrpeach: $(MRPEACH_OBJECTS:.c=.$(EXTENSION))

mrpeach_install: mrpeach
	install -d $(DESTDIR)$(objectsdir)/$(MRPEACH_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(MRPEACH_NAME) \
		--author "Martin Peach <martin.peach@sympatico.ca>" \
		--description "" \
		--license "GNU GPL" \
		--version "0.1"
	install -p $(MRPEACH_OBJECTS:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(MRPEACH_NAME)
	install -p $(externals_src)/mrpeach/*/*.pd \
		$(DESTDIR)$(objectsdir)/$(MRPEACH_NAME)
	install -p $(externals_src)/mrpeach/midifile/I_Wanna_Be_Sedated.mid \
		$(DESTDIR)$(objectsdir)/$(MRPEACH_NAME)
	install -d $(DESTDIR)$(objectsdir)/$(MRPEACH_NAME)/examples
	install -p $(externals_src)/mrpeach/net/examples/test.txt \
		$(DESTDIR)$(objectsdir)/$(MRPEACH_NAME)/examples

mrpeach_clean:
	-rm -f -- $(MRPEACH_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(MRPEACH_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/mrpeach/*.bak
	-rm -f -- $(externals_src)/mrpeach/*.*~


#----------------------------------------------------------------------------
# OSCx
OSCX_NAME=oscx
$(externals_src)/OSCx/configure: $(externals_src)/OSCx/configure.ac
	cd $(externals_src)/OSCx && autoconf

$(externals_src)/OSCx/Makefile: $(externals_src)/OSCx/Makefile.in
	cd $(externals_src)/OSCx && ./configure
$(externals_src)/OSCx/libOSC/Makefile: $(externals_src)/OSCx/libOSC/Makefile.in
	cd $(externals_src)/OSCx && ./configure
$(externals_src)/OSCx/src/Makefile: $(externals_src)/OSCx/src/Makefile.in
	cd $(externals_src)/OSCx && ./configure

$(externals_src)/OSCx/src/OSCroute.$(EXTENSION):  $(externals_src)/OSCx/configure \
$(externals_src)/OSCx/Makefile
	$(MAKE) -C $(externals_src)/OSCx
$(externals_src)/OSCx/src/dumpOSC.$(EXTENSION):  $(externals_src)/OSCx/configure \
$(externals_src)/OSCx/Makefile
	$(MAKE) -C $(externals_src)/OSCx
$(externals_src)/OSCx/src/sendOSC.$(EXTENSION):  $(externals_src)/OSCx/configure \
$(externals_src)/OSCx/Makefile
	$(MAKE) -C $(externals_src)/OSCx

oscx: $(externals_src)/OSCx/src/OSCroute.$(EXTENSION) \
$(externals_src)/OSCx/src/dumpOSC.$(EXTENSION) \
$(externals_src)/OSCx/src/sendOSC.$(EXTENSION)

oscx_install: oscx
	install -d $(DESTDIR)$(objectsdir)/$(OSCX_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(OSCX_NAME) \
		--author "<jdl@xdv.org>" \
		--license "BSD" \
		--description "objects for working with OpenSoundControl"
	install -p $(externals_src)/OSCx/src/*.$(EXTENSION) $(DESTDIR)$(objectsdir)/$(OSCX_NAME)
	install -p $(externals_src)/OSCx/doc/*.* $(DESTDIR)$(objectsdir)/$(OSCX_NAME)


oscx_clean:
	-$(MAKE) -C $(externals_src)/OSCx $(DEST_PATHS) clean
	-$(MAKE) -C $(externals_src)/OSCx CC=gcc clean
	-rm $(externals_src)/OSCx/Makefile
	-rm $(externals_src)/OSCx/configure



#------------------------------------------------------------------------------#
# PAN
pan:
	make -C $(externals_src)/pan PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

pan_install:
	make -C $(externals_src)/pan DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

pan_clean:
	make -C $(externals_src)/pan clean


#------------------------------------------------------------------------------#
# PDDP
pddp:
	make -C $(externals_src)/pddp PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

pddp_install:
	make -C $(externals_src)/pddp DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

pddp_clean:
	make -C $(externals_src)/pddp clean


#------------------------------------------------------------------------------#
# PDLUA

LUA_CFLAGS = $(shell (pkg-config --cflags lua5.3 || pkg-config --cflags lua5.2 || pkg-config --cflags lua) 2> /dev/null)
LUA_LIBS = $(shell (pkg-config --libs lua5.3 || pkg-config --libs lua5.2 || pkg-config --libs lua) 2> /dev/null)

pdlua:
	make -C $(externals_src)/pd-lua LUA_CFLAGS="$(LUA_CFLAGS)" LUA_LIBS="$(LUA_LIBS)" PD_PATH="$(pd_src)" PD_LIB="$(pd_src)/src" CFLAGS="$(CFLAGS)"

pdlua_install:
	make -C $(externals_src)/pd-lua DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

pdlua_clean:
	make -C $(externals_src)/pd-lua clean


#------------------------------------------------------------------------------#
# PDOGG
pdogg:
	make -C $(externals_src)/pdogg PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

pdogg_install:
	make -C $(externals_src)/pdogg DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

pdogg_clean:
	make -C $(externals_src)/pdogg clean



#------------------------------------------------------------------------------#
# PDP
ifeq ($(ARCH),i386)
  PDP_OPTIONS = --enable-quicktime --enable-mmx
else
  PDP_OPTIONS = --enable-quicktime
endif

PDP_NAME=pdp
$(externals_src)/pdp/configure: $(externals_src)/pdp/configure.ac
	cd $(externals_src)/pdp && autoconf

$(externals_src)/pdp/Makefile.config: $(externals_src)/pdp/configure $(externals_src)/pdp/Makefile.config.in
	cd $(externals_src)/pdp && ./configure PD_CPPFLAGS="-I$(pd_src)/src" \
		$(PDP_OPTIONS)

$(externals_src)/pdp/pdp.$(EXTENSION): $(externals_src)/pdp/Makefile.config 
	$(MAKE) -C $(externals_src)/pdp PD_EXECUTABLE="$(pd_src)/bin/pd"

pdp: $(externals_src)/pdp/pdp.$(EXTENSION)

pdp_install: pdp
	install -d $(DESTDIR)$(objectsdir)/$(PDP_NAME)
	install -p $(externals_src)/pdp/*.$(EXTENSION) $(DESTDIR)$(objectsdir)/$(PDP_NAME)
	install -p $(externals_src)/pdp/abstractions/*.pd $(DESTDIR)$(objectsdir)/$(PDP_NAME)
	install -p $(externals_src)/pdp/doc/objects/*.* $(DESTDIR)$(objectsdir)/$(PDP_NAME)
	install -d $(DESTDIR)$(objectsdir)/$(PDP_NAME)/manual
	install -p $(externals_src)/pdp/doc/reference.txt \
		$(DESTDIR)$(objectsdir)/$(PDP_NAME)/manual
	install -d $(DESTDIR)$(objectsdir)/$(PDP_NAME)/examples
	install -p $(externals_src)/pdp/doc/introduction/*.* \
		$(DESTDIR)$(objectsdir)/$(PDP_NAME)/examples
	install -p $(externals_src)/pdp/doc/examples/*.* \
		$(DESTDIR)$(objectsdir)/$(PDP_NAME)/examples
ifeq ($(OS_NAME),darwin)
# switch Linux-only pdp_v4l to Mac-only pdp_ieee1394
	sed -i.bak 's|pdp_v4l|pdp_ieee1394|' $(DESTDIR)$(objectsdir)/$(PDP_NAME)/*.pd
	rm -f --  $(DESTDIR)$(objectsdir)/$(PDP_NAME)/*.pd.bak
	sed -i.bak 's|pdp_v4l|pdp_ieee1394|' $(DESTDIR)$(objectsdir)/$(PDP_NAME)/examples/*.pd
	rm -f --  $(DESTDIR)$(objectsdir)/$(PDP_NAME)/examples/*.pd.bak
endif

pdp_clean:
	-rm -f -- $(externals_src)/pdp/*.$(EXTENSION)
	-find $(externals_src)/pdp -name '*.o' | xargs rm -f --
	-find $(externals_src)/pdp -name '*.bak' | xargs rm -f --
	-rm -f -- $(externals_src)/pdp/Makefile.config
	-rm -f -- $(externals_src)/pdp/configure


#------------------------------------------------------------------------------#
# PDP_OPENGL
PDP_OPENGL_NAME=3dp
PDP_OPENGL_BINARY := $(externals_src)/pdp/opengl/pdp_opengl.$(EXTENSION)
$(PDP_OPENGL_BINARY): $(externals_src)/pdp/Makefile.config
	make -C $(externals_src)/pdp/opengl

pdp_opengl: $(PDP_OPENGL_BINARY)
	echo $(PDP_OPENGL_BINARY)

pdp_opengl_install: pdp_opengl
	install -d $(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(PDP_OPENGL_NAME) \
		--author "Tom Schouten" \
		--description "opengl extensions for pdp" \
		--license "GPL v2 or later"
	install -p $(PDP_OPENGL_BINARY) $(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)
	install -p $(externals_src)/pdp/opengl/abstractions/*.pd \
		$(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)
	install -p $(externals_src)/pdp/opengl/doc/objects/*.pd \
		$(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)
	install -p $(externals_src)/pdp/opengl/README \
		$(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)/README.txt
	install -d $(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)/examples
	install -p $(externals_src)/pdp/opengl/doc/examples/*.pd \
		$(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)/examples
	install -p $(externals_src)/pdp/opengl/test/*.pd \
		$(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)/examples
ifeq ($(OS_NAME),darwin)
# switch Linux-only pdp_v4l to Mac-only pdp_ieee1394
	sed -i.bak 's|pdp_v4l|pdp_ieee1394|' \
		$(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)/*.pd
	rm -f --  $(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)/*.pd.bak
	sed -i.bak 's|pdp_v4l|pdp_ieee1394|' \
		$(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)/examples/*.pd
	rm -f --  $(DESTDIR)$(objectsdir)/$(PDP_OPENGL_NAME)/examples/*.pd.bak
endif

pdp_opengl_clean:
	make -C $(externals_src)/pdp/opengl clean
	-rm -f -- $(PDP_OPENGL_BINARY)
	-rm -f -- $(externals_src)/pdp_opengl/*.bak
	-rm -f -- $(externals_src)/pdp_opengl/*.*~


#------------------------------------------------------------------------------#
# PIDIP
PIDIP_NAME=pidip
$(externals_src)/pidip/configure: $(externals_src)/pidip/configure.ac
	cd $(externals_src)/pidip && autoconf

$(externals_src)/pidip/Makefile: $(externals_src)/pidip/Makefile.in
	-cd $(externals_src)/pidip && ./configure --with-pd=$(pd_src) \
		--with-pdp=$(externals_src)/pdp

$(externals_src)/pidip/pidip.$(EXTENSION):  $(externals_src)/pidip/configure \
$(externals_src)/pidip/Makefile
	-$(MAKE) -C $(externals_src)/pidip

pidip: $(externals_src)/pidip/pidip.$(EXTENSION)

pidip_install: pidip
	install -d $(DESTDIR)$(objectsdir)/$(PIDIP_NAME)
#	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(PIDIP_NAME) \
#		--author "Yves Degoyon" \
#		--description "PiDiP is Definitely in Pieces" \
#		--license "GNU GPLv2"
	install -p $(externals_src)/pidip/*.$(EXTENSION) $(DESTDIR)$(objectsdir)/$(PIDIP_NAME)
	install -p $(externals_src)/pidip/doc/*.pd $(DESTDIR)$(objectsdir)/$(PIDIP_NAME)
	install -d $(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/examples
	install -p $(externals_src)/pidip/patches/*.* \
		$(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/examples
	install -p $(externals_src)/pidip/README \
		$(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/README.txt
	install -d $(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/examples/images
	install -p $(externals_src)/pidip/patches/images/*.* \
		$(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/examples/images
	install -d $(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/examples/morphology
	install -p $(externals_src)/pidip/patches/morphology/*.* \
		$(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/examples/morphology
ifeq ($(OS_NAME),darwin)
# switch Linux-only pdp_v4l to Mac-only pdp_ieee1394
	sed -i.bak 's|pdp_v4l|pdp_ieee1394|' $(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/*.pd
	rm -f --  $(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/*.pd.bak
	sed -i.bak 's|pdp_v4l|pdp_ieee1394|' $(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/examples/*.pd
	rm -f --  $(DESTDIR)$(objectsdir)/$(PIDIP_NAME)/examples/*.pd.bak
endif

pidip_clean:
	-rm -f -- $(externals_src)/pidip/*.$(EXTENSION)
	-find $(externals_src)/pidip -name '*.o' | xargs rm -f --
	-rm -f -- $(externals_src)/pidip/*.bak
	-rm -f -- $(externals_src)/pidip/Makefile
	-rm -f -- $(externals_src)/pidip/configure


#------------------------------------------------------------------------------#
# PLUGIN
plugin:
	make -C $(externals_src)/plugin~ PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

plugin_install:
	make -C $(externals_src)/plugin~ DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

plugin_clean:
	make -C $(externals_src)/plugin~ clean


#------------------------------------------------------------------------------#
# PMPD
pmpd:
	make -C $(externals_src)/pmpd PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

pmpd_install:
	make -C $(externals_src)/pmpd DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

pmpd_clean:
	make -C $(externals_src)/pmpd clean


#------------------------------------------------------------------------------#
# POSTLUDE
POSTLUDE_NAME=postlude
# flib is separate, so exclude it here
POSTLUDE_OBJECTS := $(wildcard $(externals_src)/postlude/[a-eg-z]*/src/*.c)
#	$(externals_src)/postlude/psql/psql.c
postlude: $(POSTLUDE_OBJECTS:.c=.$(EXTENSION))

postlude_install: postlude
	install -d $(DESTDIR)$(objectsdir)/$(POSTLUDE_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(POSTLUDE_NAME) \
		--author "Jamie Bullock" \
		--license "GNU GPL"
	install -p $(POSTLUDE_OBJECTS:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(POSTLUDE_NAME)
	install -p $(externals_src)/postlude/dssi/doc/output~.pd \
		$(DESTDIR)$(objectsdir)/$(POSTLUDE_NAME)
	install -p $(externals_src)/postlude/dssi/README \
		$(DESTDIR)$(objectsdir)/$(POSTLUDE_NAME)/README-dssi.txt
	install -p $(externals_src)/postlude/psql/README \
		$(DESTDIR)$(objectsdir)/$(POSTLUDE_NAME)README-psql.txt

postlude_clean:
	-rm -f -- $(POSTLUDE_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(POSTLUDE_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/postlude/*/*.bak
	-rm -f -- $(externals_src)/postlude/*/*.*~

#------------------------------------------------------------------------------#
# RJLIB

rjlib:

rjlib_install:
	install -d $(DESTDIR)$(objectsdir)/rjlib
	cp -rf $(externals_src)/rjlib/* $(DESTDIR)/$(objectsdir)/rjlib

rjlib_clean:

#------------------------------------------------------------------------------#
# RTCMIX~

rtcmix:
	cd $(externals_src)/rtcmix-in-pd/RTcmix-pd-4.0.1.6 \
		&& ./configure && make
	make -C $(externals_src)/rtcmix-in-pd \
		PD_PATH=$(pd_src) \
		CFLAGS="$(CFLAGS)" \
		LINUXINCLUDE=-I$(pd_src)/src

rtcmix_install:
	install -d $(DESTDIR)$(objectsdir)/rtcmix
	install -p $(externals_src)/rtcmix-in-pd/rtcmix~.$(EXTENSION) \
		$(DESTDIR)/$(objectsdir)/rtcmix
	install -p $(externals_src)/rtcmix-in-pd/rtcmix~-help.pd \
		$(DESTDIR)/$(objectsdir)/rtcmix
	cp -rf $(externals_src)/rtcmix-in-pd/lib \
		$(DESTDIR)/$(objectsdir)/rtcmix
	cp -rf $(externals_src)/rtcmix-in-pd/scores \
		$(DESTDIR)/$(objectsdir)/rtcmix

rtcmix_clean:
	make -C $(externals_src)/rtcmix-in-pd clean
	make -C $(externals_src)/rtcmix-in-pd/RTcmix*

#------------------------------------------------------------------------------#
# SIGPACK
sigpack:
	make -C $(externals_src)/sigpack PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

sigpack_install:
	make -C $(externals_src)/sigpack DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

sigpack_clean:
	make -C $(externals_src)/sigpack clean



#------------------------------------------------------------------------------#
# SMLIB
smlib:
	make -C $(externals_src)/smlib PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

smlib_install:
	make -C $(externals_src)/smlib DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

smlib_clean:
	make -C $(externals_src)/smlib clean


#------------------------------------------------------------------------------#
# TCLPD
TCLPD_NAME=tclpd
TCLPD_LIB := $(externals_src)/tclpd/tcl.$(EXTENSION)

$(TCLPD_LIB):
	cd $(externals_src)/tclpd && make

tclpd: $(TCLPD_LIB)

tclpd_install: tclpd
	install -d $(DESTDIR)$(objectsdir)
	install -p $(TCLPD_LIB) $(DESTDIR)$(objectsdir)
	install -p $(externals_src)/tclpd/README \
		$(DESTDIR)$(objectsdir)/$(TCLPD_NAME)
#	install -d $(DESTDIR)$(examplesdir)/$(TCLPD_NAME)
#	install -p $(externals_src)/tclpd/examples/*.pd \
#		$(DESTDIR)$(examplesdir)/$(TCLPD_NAME)

tclpd_clean:
	make -C $(externals_src)/tclpd clean
	-rm -f -- $(TCLPD_LIB)
	-rm -f -- $(externals_src)/tclpd/*.bak
	-rm -f -- $(externals_src)/tclpd/*.*~


#------------------------------------------------------------------------------#
# TKWIDGETS
tkwidgets:
	make -C $(externals_src)/tkwidgets PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

tkwidgets_install:
	make -C $(externals_src)/tkwidgets \
		DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

tkwidgets_clean:
	make -C $(externals_src)/tkwidgets clean

#------------------------------------------------------------------------------#
# TOF
TOF_NAME=tof
TOF_OBJECTS := $(wildcard $(externals_src)/tof/src/*.c)
tof: $(TOF_OBJECTS:.c=.$(EXTENSION))

tof_install: tof
	install -d $(DESTDIR)$(objectsdir)/$(TOF_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(TOF_NAME) \
		--author "Thomas Ouellet Fredericks" \
		--description "Various utilities" \
		--license "Whatever Hans wants" \
		--version "2009-09-22"
	install -p $(TOF_OBJECTS:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(TOF_NAME)
	install -p $(externals_src)/tof/tof/*.gif \
		$(DESTDIR)$(objectsdir)/$(TOF_NAME)
	install -p $(externals_src)/tof/tof/*.pd \
		$(DESTDIR)$(objectsdir)/$(TOF_NAME)

tof_clean:
	-rm -f -- $(TOF_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(TOF_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/tof/src/*.bak
	-rm -f -- $(externals_src)/tof/src/*.*~

#------------------------------------------------------------------------------#
# TOXY
TOXY_NAME=toxy
# toxy is compiled straight into $(OUT_DIR)
TOXY_OUT_DIR=$(DESTDIR)$(objectsdir)/$(TOXY_NAME)
toxy: $(bindir)
	$(MAKE) OPT_CFLAGS="-O2 -fno-strict-aliasing -fPIC" \
		-C $(externals_src)/miXed/toxy


toxy_install: toxy
	install -d $(DESTDIR)$(objectsdir)/$(TOXY_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(TOXY_NAME) \
		--author "Kzrysztof Czaja" \
		--license "BSD" \
		--description "objects for working with Tcl and Pd's Tk GUI"
	cd $(externals_src)/miXed/toxy && $(MAKE) OUT_DIR=$(TOXY_OUT_DIR)
	install -p $(externals_src)/miXed/doc/help/toxy/*.*  \
		$(DESTDIR)$(objectsdir)/$(TOXY_NAME)
#	install -d $(DESTDIR)$(manualsdir)/$(TOXY_NAME)
	install -d $(DESTDIR)$(examplesdir)/$(TOXY_NAME)
	install -p $(externals_src)/miXed/test/toxy/*.*  \
		$(DESTDIR)$(examplesdir)/$(TOXY_NAME)


toxy_clean:
	-$(MAKE) -C $(externals_src)/mixed/toxy $(DEST_PATHS) clean


#------------------------------------------------------------------------------#
# UNAUTHORIZED
unauthorized:
	make -C $(externals_src)/unauthorized CFLAGS="$(CFLAGS)" \
		PD_PATH=$(pd_src) PD_INCLUDE=$(pd_src)/src

unauthorized_install:
	make -C $(externals_src)/unauthorized \
		DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

unauthorized_clean:
	make -C $(externals_src)/unauthorized clean


#------------------------------------------------------------------------------#
# VBAP
vbap:
	make -C $(externals_src)/vbap PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

vbap_install:
	make -C $(externals_src)/vbap DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

vbap_clean:
	make -C $(externals_src)/vbap clean


#------------------------------------------------------------------------------#
# VANILLA
vanilla:
	make -C $(externals_src)/vanilla PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

vanilla_install:
	make -C $(externals_src)/vanilla DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

vanilla_clean:
	make -C $(externals_src)/vanilla clean


#------------------------------------------------------------------------------#
# WINDOWING
windowing:
	make -C $(externals_src)/windowing PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

windowing_install:
	make -C $(externals_src)/windowing DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

windowing_clean:
	make -C $(externals_src)/windowing clean



#------------------------------------------------------------------------------#
# Zexy
ZEXY_NAME = zexy
zexy_src := $(externals_src)/$(ZEXY_NAME)

$(zexy_src)/configure: $(zexy_src)/configure.ac
	cd $(zexy_src) && ./autogen.sh

$(zexy_src)/Makefile: $(zexy_src)/configure $(zexy_src)/Makefile.am $(zexy_src)/src/Makefile.am
	cd $(zexy_src) && ./configure --disable-library \
		--with-extension=$(EXTENSION) \
		--prefix=$(prefix) \
		--libdir=$(objectsdir) \
		--with-pd=$(pd_src)

zexy: $(zexy_src)/Makefile
	$(MAKE) -C $(zexy_src)

zexy_install: zexy
	$(MAKE) -C $(zexy_src) DESTDIR=$(DESTDIR) prefix=$(prefix) \
		libdir=$(objectsdir) pkglibdir=$(objectsdir)/$(ZEXY_NAME) install
# remove all .la files (libtool archives) created by autotools
	find $(DESTDIR)$(objectsdir)/$(ZEXY_NAME) -name "*.la" -exec rm {} \;
	find $(DESTDIR)$(objectsdir)/$(ZEXY_NAME) -name "*.dll.a" -exec rm {} \;
# add missing aliases
# can't do these on Windows because FAT, NTFS don't support these chars in filenames
# on Windows, these are faked in by loading them as libs
#ifneq ($(OS_NAME),windows)
#	test -e $(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/..$(EXTENSION) || \
#		ln -s 0x2e.$(EXTENSION) \
#			$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/..$(EXTENSION)
#	test -e '$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/&&~.$(EXTENSION)' || \
#		ln -s 0x260x26_tilde.$(EXTENSION) \
#			'$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/&&~.$(EXTENSION)'
#	test -e '$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/<~.$(EXTENSION)' || \
#		ln -s 0x3c_tilde.$(EXTENSION) \
#			'$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/<~.$(EXTENSION)'
#	test -e '$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/==~.$(EXTENSION)' || \
#		ln -s 0x3d0x3d_tilde.$(EXTENSION) \
#			'$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/==~.$(EXTENSION)'
#	test -e '$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/>~.$(EXTENSION)' || \
#		ln -s 0x3e_tilde.$(EXTENSION) \
#			'$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/>~.$(EXTENSION)'
#	test -e '$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/||~.$(EXTENSION)' || \
#		ln -s 0x7c0x7c_tilde.$(EXTENSION) \
#			'$(DESTDIR)$(objectsdir)/$(ZEXY_NAME)/||~.$(EXTENSION)'
#endif

zexy_clean:
	cd $(externals_src)/zexy && ./autogen.sh
	-$(MAKE) -C $(externals_src)/zexy clean
	-rm -rf -- $(externals_src)/zexy/autom4te.cache
	-rm -f -- $(externals_src)/zexy/aclocal.m4
	-rm -f -- $(externals_src)/zexy/libtool
	-rm -f -- $(externals_src)/zexy/ltmain.sh
	-rm -f -- $(externals_src)/zexy/m4/lt*.m4
	-rm -f -- $(externals_src)/zexy/m4/libtool.m4
	-rm -f -- $(externals_src)/zexy/configure
	-rm -f -- $(externals_src)/zexy/src/*.o



#------------------------------------------------------------------------------#
# pdcontainer
PDCONTAINER_NAME = pdcontainer
PDCONTAINER_INCLUDE = -I$(externals_src)/grh/PDContainer
PDCONTAINER_SRC = $(wildcard $(externals_src)/grh/PDContainer/src/[hH]*.cpp) \
                  $(wildcard $(externals_src)/grh/PDContainer/tinyxml/*.cpp)
PDCONTAINER_OBJ = $(PDCONTAINER_SRC:.cpp=.o)
PDCONTAINER_TMP = $(wildcard $(externals_src)/grh/PDContainer/src/h_*.cpp)
PDCONTAINER_TARGETS = $(PDCONTAINER_TMP:.cpp=.$(EXTENSION))
PDC_SHARED = $(wildcard $(externals_src)/grh/PDContainer/tinyxml/*.cpp)

pdcontainer: $(PDCONTAINER_TARGETS)

$(PDCONTAINER_TARGETS) : %.$(EXTENSION) : $(PDCONTAINER_OBJ)
	$(CXX) $(LDFLAGS) -o $*.$(EXTENSION) "$*.o" $(shell ls `echo "$*.o" | sed -e s/"h_[a-z]"/"\?\?"/g -e s/"map"/"Map"/g -e s/"set"/"Set"/g -e s/"queue"/"Queue"/g`) $(PDC_SHARED:.cpp=.o) $(LIBS)
	$(STRIP) $*.$(EXTENSION)
	chmod 755 $*.$(EXTENSION)
#	rm -f -- "$*.o"

$(PDCONTAINER_OBJ) : %.o : %.cpp
	$(CXX) $(CXXFLAGS) -DPDCONTAINER_SINGLE_OBJECT $(PDCONTAINER_INCLUDE) -o "$*.o" -c "$*.cpp"

pdcontainer_install: pdcontainer
	install -d $(DESTDIR)$(objectsdir)/$(PDCONTAINER_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(PDCONTAINER_NAME) \
		--author "Georg Holzmann" \
		--description "container objects of the C++ STL for Pd" \
		--license "GPL" \
		--version ""
	install -p $(PDCONTAINER_TARGETS) $(DESTDIR)$(objectsdir)/$(PDCONTAINER_NAME)
	install -p $(externals_src)/grh/PDContainer/help/*.pd \
		$(DESTDIR)$(objectsdir)/$(PDCONTAINER_NAME)
	install -p $(externals_src)/grh/PDContainer/readme.txt \
		$(DESTDIR)$(objectsdir)/$(PDCONTAINER_NAME)

pdcontainer_clean:
	-rm -f -- $(PDCONTAINER_TARGETS)
	-rm -f -- $(externals_src)/grh/PDContainer/src/*.o
	-rm -f -- $(externals_src)/grh/PDContainer/src/*.bak
	-rm -f -- $(externals_src)/grh/PDContainer/src/*.*~
	-rm -f -- $(externals_src)/grh/PDContainer/tinyxml/*.o
	-rm -f -- $(externals_src)/grh/PDContainer/tinyxml/*.bak
	-rm -f -- $(externals_src)/grh/PDContainer/tinyxml/*.*~



#------------------------------------------------------------------------------#
# adaptive
ADAPTIVE_NAME=adaptive
ADAPTIVE_SRC := $(wildcard $(externals_src)/grh/adaptive/src/*.c)
ADAPTIVE_OBJECTS := $(ADAPTIVE_SRC:.c=.o)
ADAPTIVE_FLAGS := -DADAPTIVE_SINGLE_OBJ
ADAPTIVE_TMP := $(wildcard $(externals_src)/grh/adaptive/src/*lms*.c)
ADAPTIVE_TARGETS := $(ADAPTIVE_TMP:.c=.$(EXTENSION))

adaptive: $(ADAPTIVE_TARGETS)

$(ADAPTIVE_TARGETS) : %.$(EXTENSION) : %.o $(ADAPTIVE_OBJECTS)
	$(CC) $(LDFLAGS) -o $*.$(EXTENSION) "$*.o" $(externals_src)/grh/adaptive/src/adaptive.o $(LIBS)
	$(STRIP) $*.$(EXTENSION)
	chmod 755 $*.$(EXTENSION)
	rm -f -- "$*.o"

$(ADAPTIVE_OBJECTS) : %.o : %.c
	$(CC) $(CFLAGS) $(ADAPTIVE_FLAGS) -o "$*.o" -c "$*.c"

adaptive_install: adaptive
	install -d $(DESTDIR)$(objectsdir)/$(ADAPTIVE_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(ADAPTIVE_NAME) \
		--author "Georg Holzmann, Gerda Strobl" \
		--description "library for adaptive systems and filters" \
		--license "GNU GPL" \
		--version ""
	install -p $(ADAPTIVE_TARGETS) $(DESTDIR)$(objectsdir)/$(ADAPTIVE_NAME)
	install -p $(externals_src)/grh/adaptive/doc/*.pd \
		$(DESTDIR)$(objectsdir)/$(ADAPTIVE_NAME)
	install -p $(externals_src)/grh/adaptive/readme \
		$(DESTDIR)$(objectsdir)/$(ADAPTIVE_NAME)
	install -d $(DESTDIR)$(examplesdir)/$(ADAPTIVE_NAME)
	install -p $(externals_src)/grh/adaptive/examples/*.pd \
		$(externals_src)/grh/adaptive/examples/*.dat \
		$(DESTDIR)$(examplesdir)/$(ADAPTIVE_NAME)
	install -d $(DESTDIR)$(objectsdir)/$(ADAPTIVE_NAME)/manual
	install -p $(externals_src)/grh/adaptive/readme \
		$(DESTDIR)$(objectsdir)/$(ADAPTIVE_NAME)/manual/README.txt
	install -d $(DESTDIR)$(objectsdir)/$(ADAPTIVE_NAME)/examples
	install -p $(externals_src)/grh/adaptive/examples/*.pd \
		$(externals_src)/grh/adaptive/examples/*.dat \
		$(DESTDIR)$(objectsdir)/$(ADAPTIVE_NAME)/examples

adaptive_clean:
	-rm -f -- $(ADAPTIVE_TARGETS)
	-rm -f -- $(ADAPTIVE_OBJECTS)
	-rm -f -- $(externals_src)/grh/adaptive/src/*.bak
	-rm -f -- $(externals_src)/grh/adaptive/src/*.*~



#------------------------------------------------------------------------------#
# iem_adaptfilt
IEMADAPT_NAME=iem_adaptfilt
IEMADAPT_SRC := $(wildcard $(externals_src)/iem/iem_adaptfilt/src/*~.c)

iem_adaptfilt: $(IEMADAPT_SRC:.c=.$(EXTENSION))

iem_adaptfilt_install: iem_adaptfilt
	install -d $(DESTDIR)$(objectsdir)/$(IEMADAPT_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEMADAPT_NAME) \
		--author "Markus Noisternig, Thomas Musil" \
		--description "several algorithms for adaptive filters" \
		--license "GNU GPL" \
		--version ""
	install -p $(IEMADAPT_SRC:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(IEMADAPT_NAME)
	install -p $(externals_src)/iem/iem_adaptfilt/help/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEMADAPT_NAME)
	install -d $(DESTDIR)$(objectsdir)/$(IEMADAPT_NAME)/manual
	install -p $(externals_src)/iem/iem_adaptfilt/doc/adapt_filt_lib.pdf \
		$(DESTDIR)$(objectsdir)/$(IEMADAPT_NAME)/manual

iem_adaptfilt_clean:
	-rm -f -- $(IEMADAPT_SRC:.c=.$(EXTENSION))
	-rm -f -- $(IEMADAPT_SRC:.c=.o)
	-rm -f -- $(externals_src)/iem/iem_adaptfilt/src/*.bak
	-rm -f -- $(externals_src)/iem/iem_adaptfilt/src/*.*~



#------------------------------------------------------------------------------#
# iem_delay
IEMDELAY_NAME=iem_delay
IEMDELAY_SRC := $(wildcard $(externals_src)/iem/iem_delay/src/*~.c)

iem_delay: $(IEMDELAY_SRC:.c=.$(EXTENSION))

iem_delay_install: iem_delay
	install -d $(DESTDIR)$(objectsdir)/$(IEMDELAY_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEMDELAY_NAME) \
		--author "Thomas Musil" \
		--description "various delay objects" \
		--license "GNU GPL" \
		--version ""
	install -p $(IEMDELAY_SRC:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(IEMDELAY_NAME)
	install -p $(externals_src)/iem/iem_delay/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEMDELAY_NAME)
	install -p $(externals_src)/iem/iem_delay/READ_ME.txt \
		$(DESTDIR)$(objectsdir)/$(IEMDELAY_NAME)/README.txt

iem_delay_clean:
	-rm -f -- $(IEMDELAY_SRC:.c=.$(EXTENSION))
	-rm -f -- $(IEMDELAY_SRC:.c=.o)
	-rm -f -- $(externals_src)/iem/iem_delay/src/*.bak
	-rm -f -- $(externals_src)/iem/iem_delay/src/*.*~



#------------------------------------------------------------------------------#
# iem_roomsim
IEMROOM_NAME=iem_roomsim
IEMROOM_SRC := $(wildcard $(externals_src)/iem/iem_roomsim/src/*d.c)

iem_roomsim: $(IEMROOM_SRC:.c=.$(EXTENSION))

iem_roomsim_install: iem_roomsim
	install -d $(DESTDIR)$(objectsdir)/$(IEMROOM_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEMROOM_NAME) \
		--author "Thomas Musil" \
		--description "objects for room simulation" \
		--license "GNU GPL" \
		--version ""
	install -p $(IEMROOM_SRC:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(IEMROOM_NAME)
	install -p $(externals_src)/iem/iem_roomsim/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEMROOM_NAME)
	install -p $(externals_src)/iem/iem_roomsim/READ_ME.txt \
		$(DESTDIR)$(objectsdir)/$(IEMROOM_NAME)/README.txt

iem_roomsim_clean:
	-rm -f -- $(IEMROOM_SRC:.c=.$(EXTENSION))
	-rm -f -- $(IEMROOM_SRC:.c=.o)
	-rm -f -- $(externals_src)/iem/iem_roomsim/src/*.bak
	-rm -f -- $(externals_src)/iem/iem_roomsim/src/*.*~



#------------------------------------------------------------------------------#
# iem_spec2
IEMSPEC2_NAME=iem_spec2
IEMSPEC2_SRC := $(wildcard $(externals_src)/iem/iem_spec2/src/*~.c)

iem_spec2: $(IEMSPEC2_SRC:.c=.$(EXTENSION))

iem_spec2_install: iem_spec2
	install -d $(DESTDIR)$(objectsdir)/$(IEMSPEC2_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEMSPEC2_NAME) \
		--author "Thomas Musil" \
		--description "special spectral processing objects, which only calculates blocksize/2 + 1 samples of a signal" \
		--license "GNU GPL" \
		--version ""
	install -p $(IEMSPEC2_SRC:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(IEMSPEC2_NAME)
	install -p $(externals_src)/iem/iem_spec2/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEMSPEC2_NAME)
	install -p $(externals_src)/iem/iem_spec2/READ_ME.txt \
		$(DESTDIR)$(objectsdir)/$(IEMSPEC2_NAME)/README.txt

iem_spec2_clean:
	-rm -f -- $(IEMSPEC2_SRC:.c=.$(EXTENSION))
	-rm -f -- $(IEMSPEC2_SRC:.c=.o)
	-rm -f -- $(externals_src)/iem/iem_spec2/src/*.bak
	-rm -f -- $(externals_src)/iem/iem_spec2/src/*.*~



#------------------------------------------------------------------------------#
# iem_tab
IEMTAB_NAME=iem_tab
IEMTAB_SRC := $(wildcard $(externals_src)/iem/iem_tab/src/*.c)
IEMTAB_OBJ := $(IEMTAB_SRC:.c=.o)
IEMTAB_TARTMP := $(wildcard $(externals_src)/iem/iem_tab/src/tab_*.c)
IEMTAB_TARGETS := $(IEMTAB_TARTMP:.c=.$(EXTENSION))
IEMTAB_FLAGS := -DIEMTAB_SINGLE_OBJ

iem_tab: $(IEMTAB_TARGETS)

$(IEMTAB_TARGETS) : %.$(EXTENSION) : %.o $(IEMTAB_OBJ)
	$(CC) $(LDFLAGS) -o $*.$(EXTENSION) "$*.o" $(externals_src)/iem/iem_tab/src/iem_tab.o $(LIBS)
	$(STRIP) $*.$(EXTENSION)
	chmod 755 $*.$(EXTENSION)
	rm -f -- "$*.o"

$(IEMTAB_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(IEMTAB_FLAGS) -o "$*.o" -c "$*.c"

iem_tab_install: iem_tab
	install -d $(DESTDIR)$(objectsdir)/$(IEMTAB_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEMTAB_NAME) \
		--author "Thomas Musil" \
		--description "library to manipulate tables or arrays" \
		--license "GNU GPL" \
		--version ""
	install -p $(IEMTAB_TARGETS) $(DESTDIR)$(objectsdir)/$(IEMTAB_NAME)
	install -p $(externals_src)/iem/iem_tab/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEMTAB_NAME)
	install -p $(externals_src)/iem/iem_tab/READ_ME.txt \
		$(DESTDIR)$(objectsdir)/$(IEMTAB_NAME)/README.txt

iem_tab_clean:
	-rm -f -- $(IEMTAB_TARGETS)
	-rm -f -- $(IEMTAB_OBJ)
	-rm -f -- $(externals_src)/iem/iem_tab/src/*.bak
	-rm -f -- $(externals_src)/iem/iem_tab/src/*.*~



#------------------------------------------------------------------------------#
# flashserver
FLASHSERVER_NAME=flashserver
FLASHSERVER_OBJECTS := $(wildcard $(externals_src)/olafmatt/flashserver/*.c)
flashserver: $(FLASHSERVER_OBJECTS:.c=.$(EXTENSION))

flashserver_install: flashserver
	install -d $(DESTDIR)$(objectsdir)/$(FLASHSERVER_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(FLASHSERVER_NAME) \
		--author "Olaf Matthes" \
		--description "a flashserver for multiple clients" \
		--license "GNU GPL" \
		--version ""
	install -p $(FLASHSERVER_OBJECTS:.c=.$(EXTENSION)) $(DESTDIR)$(objectsdir)/$(FLASHSERVER_NAME)
	install -p $(externals_src)/olafmatt/flashserver/*.pd \
		$(DESTDIR)$(objectsdir)/$(FLASHSERVER_NAME)
	install -p $(externals_src)/olafmatt/flashserver/README \
		$(DESTDIR)$(objectsdir)/$(FLASHSERVER_NAME)

flashserver_clean:
	-rm -f -- $(FLASHSERVER_OBJECTS:.c=.$(EXTENSION))
	-rm -f -- $(FLASHSERVER_OBJECTS:.c=.o)
	-rm -f -- $(externals_src)/olafmatt/flashserver/*.bak
	-rm -f -- $(externals_src)/olafmatt/flashserver/*.*~



#------------------------------------------------------------------------------#
# hdspm_mixer
HDSPM_NAME=hdspm_mixer
HDSPM_SRC := $(wildcard $(externals_src)/iem/hdspm_mixer/*.c)
HDSPM_OBJ := $(HDSPM_SRC:.c=.o)
HDSPM_TARGET := $(externals_src)/iem/hdspm_mixer/hdspmmixer.$(EXTENSION)

hdspm_mixer: $(HDSPM_TARGET)

$(HDSPM_TARGET) : $(HDSPM_OBJ)
	$(CC) $(LDFLAGS) -o $(HDSPM_TARGET) $(HDSPM_OBJ) $(LIBS)
	$(STRIP) $(HDSPM_TARGET)
	chmod 755 $(HDSPM_TARGET)
	rm -f -- $(HDSPM_OBJ)

$(HDSPM_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) -o "$*.o" -c "$*.c"

hdspm_mixer_install: hdspm_mixer
	install -d $(DESTDIR)$(objectsdir)/$(HDSPM_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(HDSPM_NAME) \
		--author "Winfried Ritsch" \
		--description "pd-mixer for RME hammerfall devices" \
		--license "GNU GPL" \
		--version ""
	install -p $(HDSPM_TARGET) $(DESTDIR)$(objectsdir)/$(HDSPM_NAME)
	install -p $(externals_src)/iem/hdspm_mixer/*.pd \
		$(DESTDIR)$(objectsdir)/$(HDSPM_NAME)

hdspm_mixer_clean:
	-rm -f -- $(HDSPM_TARGET)
	-rm -f -- $(HDSPM_OBJ)
	-rm -f -- $(externals_src)/iem/hdspm_mixer/*.bak
	-rm -f -- $(externals_src)/iem/hdspm_mixer/*.*~



#------------------------------------------------------------------------------#
# iemgui
IEMGUI_NAME=iemgui
IEMGUI_SRC := $(wildcard $(externals_src)/iem/iemgui/src/*.c)
IEMGUI_OBJ := $(IEMGUI_SRC:.c=.o)
IEMGUI_TARTMP := $(wildcard $(externals_src)/iem/iemgui/src/*_*.c)
IEMGUI_TARGETS := $(IEMGUI_TARTMP:.c=.$(EXTENSION))
IEMGUI_FLAGS := -DIEMGUI_SINGLE_OBJ

iemgui: $(IEMGUI_TARGETS)

$(IEMGUI_TARGETS) : %.$(EXTENSION) : %.o $(IEMGUI_OBJ)
	$(CC) $(LDFLAGS) -o $*.$(EXTENSION) "$*.o" $(externals_src)/iem/iemgui/src/iemgui.o $(LIBS)
	$(STRIP) $*.$(EXTENSION)
	chmod 755 $*.$(EXTENSION)
	rm -f -- "$*.o"

$(IEMGUI_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(IEMGUI_FLAGS) -o "$*.o" -c "$*.c"

iemgui_install: iemgui
	install -d $(DESTDIR)$(objectsdir)/$(IEMGUI_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEMGUI_NAME) \
		--author "Thomas Musil" \
		--description "some more GUI objects for PD" \
		--license "GNU GPL" \
		--version ""
	install -p $(IEMGUI_TARGETS) $(DESTDIR)$(objectsdir)/$(IEMGUI_NAME)
	install -p $(externals_src)/iem/iemgui/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEMGUI_NAME)
	install -p $(externals_src)/iem/iemgui/READ_ME.txt \
		$(DESTDIR)$(objectsdir)/$(IEMGUI_NAME)/README.txt

iemgui_clean:
	-rm -f -- $(IEMGUI_TARGETS)
	-rm -f -- $(IEMGUI_OBJ)
	-rm -f -- $(externals_src)/iem/iemgui/src/*.bak
	-rm -f -- $(externals_src)/iem/iemgui/src/*.*~



#------------------------------------------------------------------------------#
# iemxmlrpc
IEMXMLRPC_NAME=iemxmlrpc
IEMXMLRPC_SRC := $(externals_src)/iem/iemxmlrpc/main.cpp
IEMXMLRPC_TARGET := $(externals_src)/iem/iemxmlrpc/xmlrpc.$(EXTENSION)
IEMXMLRPC_FLAGS := -I$(externals_src)/iem/iemxmlrpc/xmlrpc++/src
XMLRPCPP_SRC := $(wildcard $(externals_src)/iem/iemxmlrpc/xmlrpc++/src/*.cpp)

iemxmlrpc: $(IEMXMLRPC_TARGET)

$(IEMXMLRPC_TARGET) : $(IEMXMLRPC_SRC:.cpp=.o) $(XMLRPCPP_SRC:.cpp=.o)
	$(CXX) $(LDFLAGS) -o $(IEMXMLRPC_TARGET) $(IEMXMLRPC_SRC:.cpp=.o) $(XMLRPCPP_SRC:.cpp=.o) $(LIBS)
	$(STRIP) $(IEMXMLRPC_TARGET)
	chmod 755 $(IEMXMLRPC_TARGET)
	#rm -f -- "$*.o"

$(IEMXMLRPC_SRC:.cpp=.o) $(XMLRPCPP_SRC:.cpp=.o) : %.o : %.cpp
	$(CXX) $(CXXFLAGS) $(IEMXMLRPC_FLAGS) -o "$*.o" -c "$*.cpp"

iemxmlrpc_install: iemxmlrpc
	install -d $(DESTDIR)$(objectsdir)/$(IEMXMLRPC_NAME)
	$(scripts_src)/generate-libdir-metafile.sh $(DESTDIR)$(objectsdir) $(IEMXMLRPC_NAME) \
		--author "Thomas Grill, Winfried Ritsch" \
		--description "XMLRPC external for PD" \
		--license "GNU GPL" \
		--version ""
	install -p $(IEMXMLRPC_TARGET) $(DESTDIR)$(objectsdir)/$(IEMXMLRPC_NAME)
	install -p $(externals_src)/iem/iemxmlrpc/*.pd \
		$(DESTDIR)$(objectsdir)/$(IEMXMLRPC_NAME)
	install -p $(externals_src)/iem/iemxmlrpc/README.txt \
		$(DESTDIR)$(objectsdir)/$(IEMXMLRPC_NAME)
	install -d $(DESTDIR)$(examplesdir)/$(IEMXMLRPC_NAME)
	install -p $(externals_src)/iem/iemxmlrpc/*.pd \
                   $(externals_src)/iem/iemxmlrpc/*.py \
		   $(DESTDIR)$(examplesdir)/$(IEMXMLRPC_NAME)

iemxmlrpc_clean:
	-rm -f -- $(IEMXMLRPC_TARGET)
	-rm -f -- $(externals_src)/iem/iemxmlrpc/xmlrpc++/src/*.o
	-rm -f -- $(externals_src)/iem/iemxmlrpc/*.o
	-rm -f -- $(externals_src)/iem/iemxmlrpc/*.bak
	-rm -f -- $(externals_src)/iem/iemxmlrpc/*.*~



#------------------------------------------------------------------------------#
# EARPLUG
earplug:
	make -C $(externals_src)/earplug~ PD_PATH=$(pd_src) CFLAGS="$(CFLAGS)"

earplug_install:
	make -C $(externals_src)/earplug~ DESTDIR="$(DESTDIR)" objectsdir="$(objectsdir)" install

earplug_clean:
	make -C $(externals_src)/earplug~ clean



#==============================================================================#
#
# DEVELOPER'S TARGETS
#
#==============================================================================#

TAGS: etags

etags:
	etags $(pd_src)/src/*.[ch] 
	find $(externals_src) -type f -name '*.[ch]' -exec etags -a '{}' \;
	find $(externals_src) -type f -name '*.cc' -exec etags -a '{}' \;
	find $(externals_src) -type f -name '*.cpp' -exec etags -a '{}' \;
	etags --append --language=none --regex="/proc[ \t]+\([^ \t]+\)/\1/" \
		$(externals_src)/unauthorized/*/*.tk
	find /usr/include -type f -name \*.h -exec etags -a '{}' \;
	make etags_`uname -s`

etags_Darwin:
	etags -a $(externals_src)/hcs/hid/HID\ Utilities\ Source/*.[ch]
	find /System/Library/Frameworks  -type f -name \*.h -exec etags -a '{}' \;
	find /Library/Frameworks  -type f -name \*.h -exec etags -a '{}' \;
	find /sw/include -type f -name \*.h -exec etags -a '{}' \;

etags_Linux:

etags_MINGW:
	find /usr/local/include/ -type f -name \*.h -exec etags -a '{}' \;


#==============================================================================#
#
# CLEAN TARGETS
#
#==============================================================================#

# the destination-specific clean targets are in Makefile.buildlayout
clean:  $(patsubst %, %_clean,$(LIB_TARGETS))


distclean: clean cruft_clean


test_locations:
	@echo "PD_VERSION: $(PD_VERSION)"
	@echo "PACKAGE_VERSION: $(PACKAGE_VERSION)"
	@echo "CWD $(CWD)"
	@echo "DESTDIR $(DESTDIR)"
	@echo "PREFIX $(prefix)"
	@echo "BINDIR  $(bindir)"
	@echo "LIBDIR  $(libdir)"
	@echo "OBJECTSDIR  $(objectsdir)"
	@echo "PDDOCDIR  $(pddocdir)"
	@echo "LIBPDDIR  $(libpddir)"
	@echo "LIBPDBINDIR  $(libpdbindir)"
	@echo "MANUALSDIR  $(manualsdir)"
	@echo "EXAMPLESDIR  $(examplesdir)"
	@echo " "
	@echo "Compiling these libs:"
	@echo "$(LIB_TARGETS)"


.PHONY: all install clean distclean test_locations $(LIB_TARGETS) \
$(patsubst %, %_install,$(LIB_TARGETS)) $(patsubst %, %_clean,$(LIB_TARGETS))


