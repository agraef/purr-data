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

PD_INNO_SETUP = pd-inno.iss

package:  $(PD_INNO_SETUP)
## this pops up the GUI app
#	start $(PD_INNO_SETUP)
## this compiles the package on the command line
	/c/Program\ Files/Inno\ Setup\ 5/ISCC.exe $(PD_INNO_SETUP)
	@echo " "
	@echo "win32_inno install succeeded!"

build_pd:
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
	$(MAKE) -C $(packages_src) $(DEST_PATHS) extended_install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) doc_format
	install -p pd-settings.reg $(DESTDIR)$(prefix)

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
	install -d $(DESTDIR)$(prefix)/
	install -p $(bin_src)/libfftw3-3.dll  $(DESTDIR)$(prefix)/
	install -p $(bin_src)/libfftw3f-3.dll  $(DESTDIR)$(prefix)/
	#install -p $(bin_src)/libgnurx-0.dll  $(DESTDIR)$(prefix)/
	install -p $(bin_src)/libogg-0.dll  $(DESTDIR)$(prefix)/
	install -p $(bin_src)/libsndfile-1.dll  $(DESTDIR)$(prefix)/
	install -p $(bin_src)/libvorbis-0.dll  $(DESTDIR)$(prefix)/
	install -p $(bin_src)/libvorbisenc-2.dll  $(DESTDIR)$(prefix)/
	install -p $(bin_src)/libvorbisfile-3.dll  $(DESTDIR)$(prefix)/
	install -p $(bin_src)/lua53.dll          $(DESTDIR)$(prefix)/
	#install -p $(bin_src)/pthreadGC2.dll     $(DESTDIR)$(prefix)/
	install -p $(bin_src)/libdl-0.dll     $(DESTDIR)$(prefix)/
# these stay with pd.exe
	install -d $(DESTDIR)$(bindir)
# if these are installed, include them in the build root
	-install -p $(system32)/msvcp71.dll   $(DESTDIR)$(bindir)
	-install -p $(system32)/msvcr71.dll   $(DESTDIR)$(bindir)
	#-install -p $(system32)/pthreadVC.dll $(DESTDIR)$(bindir)


exe_install:
	install -d $(DESTDIR)$(bindir)
	install -p $(bin_src)/wish85.exe  $(DESTDIR)$(bindir)
	install -p $(bin_src)/tclsh85.exe  $(DESTDIR)$(bindir)


$(PD_INNO_SETUP): $(PD_INNO_SETUP).in
	@echo "Making Inno Setup file: $(PD_INNO_SETUP)"
	cat "$(PD_INNO_SETUP).in" | \
		sed 's/PACKAGE_NAME/$(PACKAGE_NAME)/g' | \
		sed 's/PD_VERSION/$(PD_VERSION)/g' | \
		sed 's/PD-EXTENDED_VERSION/$(PD-EXTENDED_VERSION)/g' > \
		$(PD_INNO_SETUP)
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
	@echo "PD-EXTENDED_VERSION: $(PD-EXTENDED_VERSION)"
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
