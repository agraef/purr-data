#
# for making a .deb, or a tar.bz2 that installs using a Makefile
#

CWD := $(shell pwd)

# DESTDIR needs the trailing slash
DESTDIR = $(CWD)/build/
cvs_root_dir := $(shell cd $(CWD)/../.. && pwd)
BUILDLAYOUT_DIR = $(cvs_root_dir)/packages

include $(BUILDLAYOUT_DIR)/Makefile.buildlayout

# if machine has dpkg-deb, build a Debian package
ifeq ($(shell test -x /usr/bin/dpkg-deb && echo YES),YES)
  PACKAGE_TYPE = deb
else
  PACKAGE_TYPE = tarbz2
endif

# for command-line UNIX versions, you need to install Pd into the same
# directory as $(prefix) otherwise Pd won't be able to find extra, doc, etc.
ifeq ($(PACKAGE_TYPE),deb)
  prefix = /usr
else
  prefix = /usr/local
endif

TARGET_PLATFORM := $(shell uname -m)
ifeq ($(TARGET_PLATFORM),i686)
# Generic x86 (tune for Pentium III, since that's more common these days)
OPT_CFLAGS += -mtune=i686 -march=i386
#OPT_CFLAGS += -march=pentium3 -msse -mfpmath=sse
#OPT_CFLAGS += -march=pentium4 -msse -msse2 -mfpmath=sse
endif
ifeq ($(TARGET_PLATFORM),ppc)
    OPT_CFLAGS += -pipe -fsigned-char -mpowerpc
endif

GCC_VERSION := $(shell gcc -dumpversion)
ifeq ($(GCC_VERSION),4.1.2)
  OPT_CFLAGS += -ftree-vectorize -ftree-vectorizer-verbose=3
endif

# lots more here: http://www.gentoo.se/docs/doc-cflags.php

# these are sent to all of the various Makefiles so that they all copy their
# output to the same directory tree
DEST_PATHS = BUILDLAYOUT_DIR=$(BUILDLAYOUT_DIR) \
				cvs_root_dir=$(cvs_root_dir) \
				DESTDIR=$(DESTDIR) \
				prefix=$(prefix) \
				OPT_CFLAGS="$(OPT_CFLAGS)" \
				UNAME=$(UNAME)

all: 	
	$(MAKE) -C $(packages_src) $(DEST_PATHS)


install: 
	$(MAKE) -C $(packages_src) $(DEST_PATHS) install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) doc_format
	find $(DESTDIR) -name \*.pd -print0 | xargs -0 chmod a-x
	@echo " "
	@echo "linux_make install succeeded!"

#==============================================================================#
#
## installer package
#
#==============================================================================#

PACKAGE_NAME = pd-l2ork-$(PD_VERSION)-`uname -m`

ifeq ($(LIGHT),yes)
default = light
else
default = default
endif

package: $(PACKAGE_TYPE)


tarbz2: installer_settings installer_makefile installer_readme
	mv $(DESTDIR)$(prefix) $(DESTDIR)$(PACKAGE_NAME)
	cd $(DESTDIR) && \
		tar cjf $(PACKAGE_NAME).tar.bz2  $(PACKAGE_NAME)
	mv $(DESTDIR)$(PACKAGE_NAME) $(DESTDIR)$(prefix)

# Any special staging for the "Burrito Supreme" installer goes here.
debstage: $(bindir)
ifneq ($(LIGHT),yes)
# add K12 mode
	cp -rf ../../l2ork_addons/K12/ $(DESTDIR)$(libpddir)/extra/
# add arduino library
	cp -rf ../../externals/hardware/arduino $(DESTDIR)$(libpddir)/extra/
endif
# free desktop menu, mime, and icon support
# perhaps the icons should go in /usr/share/pixmaps like 'scribus'
	install -d $(DESTDIR)/usr/share/icons/hicolor/128x128/apps
	install -p -m0644 $(packages_src)/linux_make/pd-l2ork.png \
		$(DESTDIR)/usr/share/icons/hicolor/128x128/apps/
	install -p -m0644 $(packages_src)/linux_make/pd-l2ork-red.png \
		$(DESTDIR)/usr/share/icons/hicolor/128x128/apps/
	install -p -m0644 $(packages_src)/linux_make/pd-l2ork-k12.png \
		$(DESTDIR)/usr/share/icons/hicolor/128x128/apps/
	install -p -m0644 $(packages_src)/linux_make/pd-l2ork-k12-red.png \
		$(DESTDIR)/usr/share/icons/hicolor/128x128/apps/
#	install -d $(DESTDIR)/usr/share/icons/hicolor/48x48/apps
#	install -p -m0644 $(packages_src)/linux_make/pd-48x48.png \
#		$(DESTDIR)/usr/share/icons/hicolor/48x48/apps/pd-l2ork.png
	install -d $(DESTDIR)/usr/share/icons/hicolor/128x128/mimetypes
	install -p -m0644 $(packages_src)/linux_make/text-x-pd-l2ork.png \
		$(DESTDIR)/usr/share/icons/hicolor/128x128/mimetypes
	install -d $(DESTDIR)/usr/share/applications/ 
	install -p -m0644 $(packages_src)/linux_make/pd-l2ork.desktop \
		$(DESTDIR)/usr/share/applications/
	install -p -m0644 $(packages_src)/linux_make/pd-l2ork-k12.desktop \
		$(DESTDIR)/usr/share/applications/
	install -p -m0644 $(packages_src)/linux_make/pd-l2ork-debug.desktop \
		$(DESTDIR)/usr/share/applications/
	install -p -m0644 $(packages_src)/linux_make/pd-l2ork-k12-debug.desktop \
		$(DESTDIR)/usr/share/applications/
# add mime type to .pd files
	install -d $(DESTDIR)/usr/share/mime/packages/
	install -p -m0644 $(packages_src)/linux_make/pd-l2ork.xml \
		$(DESTDIR)/usr/share/mime/packages/
# files for /etc
	install -d $(DESTDIR)/etc/bash_completion.d/
	install -p -m0644 $(scripts_src)/bash_completion/pd-l2ork \
		$(DESTDIR)/etc/bash_completion.d
	install -d $(DESTDIR)/etc/default
	install -p -m0644 $(scripts_src)/debian/default/pd-l2ork \
		$(DESTDIR)/etc/default
	install -d $(DESTDIR)/etc/init.d
	install -p -m0644 $(scripts_src)/debian/init.d/pd-l2ork \
		$(DESTDIR)/etc/init.d/
# default prefs file
	install -d $(DESTDIR)/etc/pd-l2ork
	ln -s -f $(libpddir)/default.settings \
		$(DESTDIR)/etc/pd-l2ork/default.settings
	install -p $(packages_src)/linux_make/$(default).settings $(DESTDIR)$(libpddir)/default.settings
# app window icon
	#install -p $(packages_src)/linux_make/pd-l2ork.gif $(DESTDIR)$(libpddir)
# emacs mode for .pd files
	install -d $(DESTDIR)/usr/share/emacs/site-lisp/
	install -p -m0644 $(scripts_src)/pd-l2ork-mode.el \
		$(DESTDIR)/usr/share/emacs/site-lisp/
# Pd-related scripts
	#install -p $(scripts_src)/pd-diff $(DESTDIR)$(bindir)

ifeq ($(PACKAGE_TYPE),deb)
deb: DEB_BUILD_ARCH := $(shell dpkg-architecture -qDEB_BUILD_ARCH)
endif
deb: DEB_PD_VERSION := $(shell echo $(PD_VERSION) | sed 's|\(.*\)-l2ork-\(.*\)|\1-\2|')
deb: debstage
# delete these since they are provided by the 'puredata-utils' and 'cyclist' package
	rm -f $(DESTDIR)$(bindir)/pdsend
	rm -f $(DESTDIR)$(bindir)/pdreceive
	rm -f $(DESTDIR)$(bindir)/cyclist
	rm -f $(DESTDIR)/usr/share/man/man1/pdsend.1.gz
	rm -f $(DESTDIR)/usr/share/man/man1/pdreceive.1.gz
#migrate Gem.pc and includes to remove conflict with other pd variants and their packages
	-mv -f $(DESTDIR)$(libpddir)/../pkgconfig $(DESTDIR)$(libpddir)/
	-mv -f $(DESTDIR)$(bindir)/../include/Gem $(DESTDIR)$(bindir)/../include/pd-l2ork/
# debian packaging
	test -d $(DESTDIR)/DEBIAN || mkdir -p $(DESTDIR)/DEBIAN
	cp $(packages_src)/linux_make/debian/control $(DESTDIR)/DEBIAN
	cp $(packages_src)/linux_make/debian/pd-l2ork.postrm $(DESTDIR)/DEBIAN
	cp $(packages_src)/linux_make/debian/pd-l2ork.postinst $(DESTDIR)/DEBIAN
# generate the dependencies for all externals and pd itself
	$(eval PACKAGE_DEPENDS := $(shell find $(DESTDIR) \( -name "*.pd_linux" -or -name pd-gui -or -wholename \*bin/pd \) -print0 | xargs -0 dpkg-shlibdeps -O | sed 's|shlibs:Depends=|, |' ))

# set build architecture and version for the package
	sed -i 's|^Version:.*|Version: $(DEB_PD_VERSION)|' \
		$(DESTDIR)/DEBIAN/control
	sed -i 's|^Architecture:.*|Architecture: $(DEB_BUILD_ARCH)|' \
		$(DESTDIR)/DEBIAN/control
# add auto-generated dependencies
	sed -i 's!^\(Depends:.*\)!\1 $(PACKAGE_DEPENDS)!' $(DESTDIR)/DEBIAN/control
# sanitize permissions
	chmod -R go-w $(DESTDIR)
	chmod -R a+r $(DESTDIR)
	find $(DESTDIR) -name \*.pd_linux | xargs chmod a-x
	find $(DESTDIR) -name \*.pd | xargs chmod a-x
	find $(DESTDIR) -type d | xargs chmod a+x
	fakeroot dpkg-deb --build $(DESTDIR) $(packages_src)/linux_make/$(PACKAGE_NAME).deb


INSTALLER_README_FILE = $(DESTDIR)$(prefix)/README.txt
installer_readme:
	install -d $(DESTDIR)$(prefix)
	-rm $(INSTALLER_README_FILE)
	sed 's|PACKAGE_NAME|$(PACKAGE_NAME)|' $(packages_src)/linux_make/README > \
		$(INSTALLER_README_FILE)

installer_settings:
	install -p $(packages_src)/linux_make/$(default).settings $(DESTDIR)$(libpddir)/default.settings
	# app window icon
	# install -p $(packages_src)/linux_make/pd-l2ork.gif $(DESTDIR)$(libpddir)
#	install -d $(DESTDIR)/usr/share/icons/hicolor/128x128/apps
#	install -p -m0644 $(packages_src)/linux_make/pd-l2ork.png \
#		$(DESTDIR)/usr/share/icons/hicolor/128x128/apps/
#	install -d $(DESTDIR)/usr/share/icons/hicolor/48x48/apps
#	install -p -m0644 $(packages_src)/linux_make/pd-48x48.png \
#		$(DESTDIR)/usr/share/icons/hicolor/48x48/apps/pd-l2ork.png
#	install -d $(DESTDIR)/usr/share/icons/hicolor/128x128/mimetypes
#	install -p -m0644 $(packages_src)/linux_make/text-x-pd-l2ork.png \
#		$(DESTDIR)/usr/share/icons/hicolor/128x128/mimetypes
#	install -d $(DESTDIR)/usr/share/applications/ 
#	install -p -m0644 $(packages_src)/linux_make/pd-l2ork.desktop \
#		$(DESTDIR)/usr/share/applications/

installer_makefile:
# it is very hard to escape the $ in shell scripts so delete files
#	-rm -f -- $(helpdir)/iem*/*\$$*.pd $(objectsdir)/iem*/*\$$*.pd
# don't put the Makefile into the package yet, otherwise it'll get installed
	cp -rf ../../l2ork_addons/K12/ $(DESTDIR)$(libpddir)/extra/
	cp -rf ../../externals/hardware/arduino $(DESTDIR)$(libpddir)/extra/
	cp -f $(packages_src)/linux_make/pd-l2ork.png $(DESTDIR)$(libpddir)
	cp -f $(packages_src)/linux_make/pd-l2ork-red.png $(DESTDIR)$(libpddir)
	cp -f $(packages_src)/linux_make/pd-l2ork-k12.png $(DESTDIR)$(libpddir)
	cp -f $(packages_src)/linux_make/pd-l2ork-k12-red.png $(DESTDIR)$(libpddir)
	cp -f $(packages_src)/linux_make/text-x-pd-l2ork.png $(DESTDIR)$(libpddir)
	cp -f $(packages_src)/linux_make/pd-l2ork.desktop $(DESTDIR)$(libpddir)
	cp -f $(packages_src)/linux_make/pd-l2ork-k12.desktop $(DESTDIR)$(libpddir)
	cp -f $(packages_src)/linux_make/pd-l2ork-debug.desktop $(DESTDIR)$(libpddir)
	cp -f $(packages_src)/linux_make/pd-l2ork-k12-debug.desktop $(DESTDIR)$(libpddir)
	./generate_install_makefile.bash  $(DESTDIR)$(prefix) > tarbz2Makefile
	mv -f tarbz2Makefile $(DESTDIR)$(prefix)/Makefile

installer_clean:
	-chmod -R +w $(packages_src)/linux_make/build
	-rm -rf $(packages_src)/linux_make/build


#==============================================================================#
#
## CLEAN TARGETS
#
#==============================================================================#

distclean: installer_clean 
	$(MAKE) -C $(packages_src) $(DEST_PATHS) distclean


clean: installer_clean
	$(MAKE) -C $(packages_src) $(DEST_PATHS) clean

package_clean: installer_clean


#==============================================================================#
#
## TEST TARGETS
#
#==============================================================================#

#==============================================================================#
#
## TEST TARGETS
#
#==============================================================================#

test_package:
	cd /tmp && tar xjf $(DESTDIR)/$(PACKAGE_NAME).tar.bz2
	$(MAKE) -C /tmp/$(PACKAGE_NAME) prefix=/tmp/usr/local install
	$(MAKE) -C /tmp/$(PACKAGE_NAME) prefix=/tmp/usr/local uninstall
	-rmdir /tmp/usr/local
	-rmdir /tmp/usr
	-rm -rf -- /tmp/$(PACKAGE_NAME)

test_locations:
	@echo --------------------------------------------------
	@echo "PD_VERSION: $(PD_VERSION)"
	@echo "DEB_PD_VERSION: $(DEB_PD_VERSION)"
	@echo "PD_BUILD_VERSION: $(PD_BUILD_VERSION)"
	@echo "CWD $(CWD)"
	@echo "DESTDIR $(DESTDIR)"
	@echo "PACKAGE_TYPE $(PACKAGE_TYPE)"
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
#	@echo "PACKAGE_DEPENDS 	$(PACKAGE_DEPENDS)"
	@echo --------------------------------------------------
	autoconf --version
	@echo --------------------------------------------------
	make --version
	@echo --------------------------------------------------
	gcc --version
