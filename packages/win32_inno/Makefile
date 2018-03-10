#
# This is a grand unifying Makefile for compiling Pd-extended under MinGW
#
all: install

export CC = gcc
CWD := $(shell pwd)

DESTDIR = $(CWD)/build
cvs_root_dir := $(shell cd $(CWD)/../.. && pwd)
BUILDLAYOUT_DIR = $(cvs_root_dir)/packages


include $(BUILDLAYOUT_DIR)/Makefile.buildlayout


# Generic x86 (tune for Pentium III, since that's most common these days)
OPT_CFLAGS += -mcpu=i586 -mtune=pentium3


# these are sent to all of the various Makefiles so that they all copy their
# output to the same directory tree
DEST_PATHS = BUILDLAYOUT_DIR=$(BUILDLAYOUT_DIR) \
				cvs_root_dir=$(cvs_root_dir) \
				DESTDIR=$(DESTDIR) \
				prefix=$(prefix) \
				OPT_CFLAGS="$(OPT_CFLAGS)" \
				UNAME=$(UNAME)

ifeq ($(LIGHT),yes)
PD_INNO_SETUP = pd-inno-light.iss
else
PD_INNO_SETUP = pd-inno.iss
endif

package:  $(PD_INNO_SETUP)
## this pops up the GUI app
#	start $(PD_INNO_SETUP)
## this compiles the package on the command line. On 64-bit Windows
#  we have to look in a different path, hence the ugly ifeq below...
ifeq (,$(wildcard /c/Program\ Files/Inno\ Setup\ 5/ISCC.exe))
	/c/Program\ Files\ \(x86\)/Inno\ Setup\ 5/ISCC.exe $(PD_INNO_SETUP)
else
	/c/Program\ Files/Inno\ Setup\ 5/ISCC.exe $(PD_INNO_SETUP)
endif
	@echo " "
	@echo "win32_inno install succeeded!"

git_version: $(pd_src)/src/s_stuff.h.in
	@cd $(pd_src)/src/ && \
	sed 's|^\(#define PD_BUILD_VERSION "\).*"|\1$(PD_BUILD_VERSION)"|' s_stuff.h.in > s_stuff.h.new && \
	(test -f s_stuff.h && diff -q s_stuff.h s_stuff.h.new > /dev/null && rm -f s_stuff.h.new && echo s_stuff.h unchanged || (rm -f s_stuff.h && mv s_stuff.h.new s_stuff.h))

build_pd: git_version
	cd $(pd_src)/src && $(MAKE) -f makefile.mingw

pd_install: build_pd
# the autoconf/MinGW setup doesn't compile the extras yet
#	$(MAKE) -C $(pd_src)/src $(DEST_PATHS) bin
#	-$(MAKE) -C $(pd_src)/src $(DEST_PATHS) install
	$(MAKE) -C $(pd_src)/src -f makefile.mingw $(DEST_PATHS) install

install: pd_install prebuilt_install lib_install exe_install
	@echo " "
	@echo "win32_inno install succeeded!"


prebuilt_install:
	$(MAKE) -C $(packages_src) $(DEST_PATHS) extended_install INCREMENTAL=$(INCREMENTAL) LIGHT=$(LIGHT)
	$(MAKE) -C $(packages_src) $(DEST_PATHS) doc_format
ifeq ($(LIGHT),yes)
	install -p pd-settings-light.reg $(DESTDIR)$(prefix)/pd-settings.reg
else
	install -p pd-settings.reg $(DESTDIR)$(prefix)
endif

#==============================================================================#
#
## setup Pd binaries
#
#==============================================================================#

#bin_src = /usr/local/bin
bin_src = /mingw32/bin
system32 = /c/WINDOWS/system32
lib_install:
# ultimately, the DLLs should be installed in %SystemRoot%\system32 by
# InnoSetup since they are 'officially' named and versioned DLLs, and can be
# shared between apps
# GUI port note: Pd doesn't seem to be able to find the following libs
# unless they are in $(DESTDIR)$(bindir) instead of $(DESTDIR)$(prefix).
# I figured that out by trial and error. If a Windows expert wants to figure
# out a better way to do it, that's fine-- I'm just trying to get it to build
# at all at this point...
	install -d $(DESTDIR)$(prefix)/
	install -d $(DESTDIR)$(bindir)
	install -p $(bin_src)/libfftw3-3.dll  $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libfftw3f-3.dll  $(DESTDIR)$(bindir)/
	#install -p $(bin_src)/libgnurx-0.dll  $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libogg-0.dll  $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libsndfile-1.dll  $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libvorbis-0.dll  $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libvorbisenc-2.dll  $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libvorbisfile-3.dll  $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libmp3lame-0.dll  $(DESTDIR)$(bindir)/
ifneq ($(LIGHT),yes)
	# I think these two are needed for zexy/matchbox and zexy/regex
	install -p $(bin_src)/libsystre-0.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libtre-5.dll $(DESTDIR)$(bindir)/
	# For fluid~
	install -p $(bin_src)/libfluidsynth-1.dll $(DESTDIR)$(bindir)/
	# For other external libs
	install -p $(bin_src)/lua53.dll          $(DESTDIR)$(bindir)/
	#install -p $(bin_src)/pthreadGC2.dll     $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libdl.dll     $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libportaudio-2.dll     $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libFLAC-8.dll     $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libspeex-1.dll     $(DESTDIR)$(bindir)/
        # These are for Gem
	install -p $(bin_src)/libftgl-2.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libfreetype-6.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libbz2-1.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libharfbuzz-0.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libglib-2.0-0.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libintl-8.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libiconv-2.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libpng16-16.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/zlib1.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libpcre-1.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libgraphite2.dll $(DESTDIR)$(bindir)/
	install -p $(bin_src)/libjpeg-8.dll $(DESTDIR)$(bindir)/
	# Apparently these Gem dependencies must be in the bindir
	# to be found on Windows
	-install -p $(DESTDIR)/extra/Gem/gem_film*.dll $(DESTDIR)$(bindir)/
	-install -p $(DESTDIR)/extra/Gem/gem_image*.dll $(DESTDIR)$(bindir)/
	-install -p $(DESTDIR)/extra/Gem/gem_model*.dll $(DESTDIR)$(bindir)/
	-install -p $(DESTDIR)/extra/Gem/gem_record*.dll $(DESTDIR)$(bindir)/
	-install -p $(DESTDIR)/extra/Gem/gem_video*.dll $(DESTDIR)$(bindir)/
endif
# these stay with pd.exe
# if these are installed, include them in the build root
	-install -p $(system32)/msvcp71.dll   $(DESTDIR)$(bindir)
	-install -p $(system32)/msvcr71.dll   $(DESTDIR)$(bindir)
	#-install -p $(system32)/pthreadVC.dll $(DESTDIR)$(bindir)
	install -p $(bin_src)/libwinpthread-1.dll $(DESTDIR)$(bindir)
	install -p $(bin_src)/libgcc_s_dw2-1.dll $(DESTDIR)$(bindir)
	install -p $(bin_src)/libstdc++-6.dll $(DESTDIR)$(bindir)
ifneq ($(LIGHT),yes)
# lyonpotpourri compiles its own shared lib. On Windows it needs to be
# in the path of the pd executable in order to get loaded. (There's probably
# also a way to add the external lib's path but I don't know how to do that.)
	install -p $(DESTDIR)/extra/lyonpotpourri/liblyonpotpourri.dll $(DESTDIR)$(bindir)
# fftease also uses a shared lib...
	install -p $(DESTDIR)/extra/fftease/libfftease.dll $(DESTDIR)$(bindir)
endif


exe_install:
	install -d $(DESTDIR)$(bindir)

$(PD_INNO_SETUP): $(PD_INNO_SETUP).in
	@echo "Making Inno Setup file: $(PD_INNO_SETUP)"
	cat "$(PD_INNO_SETUP).in" | \
		sed 's/PACKAGE_NAME/$(PACKAGE_NAME)/g' | \
		sed 's/PD_VERSION/$(PD_VERSION)/g' | \
		sed 's/PD_BUILD_VERSION/$(PD_BUILD_VERSION)/g' > \
		$(PD_INNO_SETUP)
# in incremental builds the lib directory may not exist, in that case we need
# to edit the setup file to prevent an inno-setup error
	test -d $(DESTDIR)$(libdir) || sed -i -e 's/^Source: build\\lib/;Source: build\\lib/' $(PD_INNO_SETUP)
#	start $(PD_INNO_SETUP)


#==============================================================================#
#
## CLEAN TARGETS
#
#==============================================================================#

distclean: package_clean
	$(MAKE) -C $(packages_src) $(DEST_PATHS) distclean


package_clean: 	
	-rm $(PD_INNO_SETUP)
	-rm -rf $(DESTDIR)
	-rm -rf Output


clean: package_clean
	$(MAKE) -C $(packages_src) $(DEST_PATHS) clean

#==============================================================================#
#
## TEST TARGETS
#
#==============================================================================#

test_package:
	echo "No tests here yet"


test_locations:
	@echo --------------------------------------------------
	@echo "PD_VERSION: $(PD_VERSION)"
	@echo "PD_BUILD_VERSION: $(PD_BUILD_VERSION)"
	@echo "CWD $(CWD)"
	@echo "DESTDIR $(DESTDIR)"
	@echo "PREFIX $(prefix)"
	@echo "BINDIR  $(bindir)"
	@echo "LIBDIR  $(libdir)"
	@echo "OBJECTSDIR  $(objectsdir)"
	@echo "PDDOCDIR  $(pddocdir)"
	@echo "LIBPDDIR  $(libpddir)"
	@echo "LIBPDBINDIR  $(libpdbindir)"
	@echo "HELPDIR  $(helpdir)"
	@echo "MANUALSDIR  $(manualsdir)"
	@echo "EXAMPLESDIR  $(examplesdir)"
	@echo --------------------------------------------------
	autoconf --version
	@echo --------------------------------------------------
	make --version
	@echo --------------------------------------------------
	gcc --version


.PHONY: all install package clean distclean test_locations test_package exe_install
