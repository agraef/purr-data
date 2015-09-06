current: darwin_app

# turn on weak linking and dlopen support
export MACOSX_DEPLOYMENT_TARGET = 10.4

CWD := $(shell pwd)
BUILD_BASE = $(CWD)/build
PD_APP_NAME = $(PACKAGE_NAME)
PD_APP_CONTENTS = $(BUILD_BASE)/$(PD_APP_NAME).app/Contents
DESTDIR = $(PD_APP_CONTENTS)/Resources

cvs_root_dir := $(shell cd $(CWD)/../.. && pwd)
BUILDLAYOUT_DIR = $(cvs_root_dir)/packages

include $(BUILDLAYOUT_DIR)/Makefile.buildlayout

WISH_FRAMEWORKS_SOURCE := $(shell (test -e "/Library/Frameworks/Tk.framework/Tk" && echo "/Library/Frameworks") || echo /System/Library/Frameworks)
WISH_SOURCE = $(WISH_FRAMEWORKS_SOURCE)/Tk.framework/Resources
# Wish Shell.app changed to Wish.app in TclTk 8.4.10
WISH_NAME := $(shell ( test -d $(WISH_SOURCE)/Wish.app && echo Wish) \
	|| echo Wish Shell)
WISH = "$(WISH_SOURCE)/$(WISH_NAME).app"
WISH_CONTENTS = "$(WISH)/Contents"

# CPU-specific gleaned from:
# http://developer.apple.com/releasenotes/DeveloperTools/RN-GCC4/
# http://hpc.sourceforge.net/

OPT_CFLAGS = -fast -fPIC -ftree-vectorize
FAT_FLAGS = -mmacosx-version-min=10.4

# which CPU to compile for
TARGET_PLATFORM := $(shell uname -p)
ifeq ($(TARGET_PLATFORM),i386)
  # if on 10.6/Intel, then build as 64-bit with core2 as minimum CPU
  ifeq ($(shell uname -r | sed 's|\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*|\1|'), 10)
    OPT_CFLAGS += -march=core2 -msse3 -mssse3 -mfpmath=sse
    FAT_FLAGS = -mmacosx-version-min=10.6
  else
    OPT_CFLAGS += -msse3 -mfpmath=sse
  endif
else
  OPT_CFLAGS += -mcpu=7450 -mtune=7450
endif

OPT_FLAGS += -DDEBUG_SOUNDFILE

# these are sent to all of the various Makefiles so that they all copy their
# output to the same directory tree
DEST_PATHS = BUILDLAYOUT_DIR=$(BUILDLAYOUT_DIR) \
				cvs_root_dir=$(cvs_root_dir) \
				prefix=$(prefix) \
				DESTDIR=$(DESTDIR) \
				OPT_CFLAGS="$(OPT_CFLAGS)" \
				FAT_FLAGS="$(FAT_FLAGS)" \
				UNAME=$(UNAME)

all:
	$(MAKE) -C $(packages_src) $(DEST_PATHS)

install: darwin_app

#------------------------------------------------------------------------------#
darwin_app: darwin_app_core extended_install rez_install darwin_app_embed_libs spotlight_importer_install
	@echo " "
	@echo "Pd.app build succeeded!"

extended_install:
# on Mac OS X 10.6/Snow Leopard don't build Gem since it needs Carbon
ifeq ($(shell uname -r | sed 's|10\.[0-9][0-9]*\.[0-9][0-9]*|10|'),10)
	$(MAKE) -C $(packages_src) $(DEST_PATHS) abstractions_install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) doc_install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) externals_install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) readme_install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) welcome_install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) license_install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) noncvs_install
else
	$(MAKE) -C $(packages_src) $(DEST_PATHS) extended_install
endif

#------------------------------------------------------------------------------#
# this target is for when you already have a pre-built Pd.app and you
# just want to fill it with goodies from CVS.  To do so, place your 
# Pd.app in $(BUILD_BASE)/Pd.app
extended_app_install:
	$(MAKE) -C $(packages_src) $(DEST_PATHS) abstractions_install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) doc_install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) extensions_install
	$(MAKE) -C $(packages_src) $(DEST_PATHS) externals_install


#------------------------------------------------------------------------------#
# build a standalone Wish.app from the installed Tcl/Tk Frameworks
darwin_app_wrapper: Info.plist
# Wish
	install -d "$(PD_APP_CONTENTS)/MacOS"
	install -m0755 -p \
		"$(WISH_CONTENTS)/MacOS/$(WISH_NAME)" "$(PD_APP_CONTENTS)/MacOS"
	install -d "$(PD_APP_CONTENTS)/Resources"
# only embed the Tcl/Tk Frameworks if not building against the built-in ones
ifneq (/System,$(findstring /System,$(WISH_FRAMEWORKS_SOURCE)))
	install -d $(PD_APP_CONTENTS)/Frameworks \
		$(PD_APP_CONTENTS)/Frameworks/Tcl.framework \
		$(PD_APP_CONTENTS)/Frameworks/Tk.framework
	ditto $(WISH_FRAMEWORKS_SOURCE)/Tcl.framework \
		$(PD_APP_CONTENTS)/Frameworks/Tcl.framework/
	ditto $(WISH_FRAMEWORKS_SOURCE)/Tk.framework \
		$(PD_APP_CONTENTS)/Frameworks/Tk.framework/
	rm -rf $(PD_APP_CONTENTS)/Frameworks/Tcl.framework/{,/Versions/8.4}/{Headers,PrivateHeaders,*_debug,lib*.a,*Config.sh} && \
	rm -rf $(PD_APP_CONTENTS)/Frameworks/Tk.framework/{,/Versions/8.4}/{Headers,PrivateHeaders,*_debug,lib*.a,*Config.sh}
	chmod -R u+w $(PD_APP_CONTENTS)/Frameworks
# from http://tktoolkit.cvs.sourceforge.net/tktoolkit/tk/macosx/GNUmakefile
	fix_install_id ( ) { \
	    chmod -R a+w "$$1"; \
	    install_name_tool -id $$(otool -L "$$1" | awk "/$$2\.framework.*[^:]\$$/ {sub(\"^.*/Frameworks\",\"@executable_path/../Frameworks\",\$$1); print \$$1}") "$$1"; \
	    chmod -R a-w "$$1"; \
	} && \
	fix_install_name ( ) { \
	    chmod -R a+w "$$1"; \
	    install_name_tool -change $$(otool -L "$$1" | awk "/$$2\.framework.*[^:]\$$/ {print \$$1; sub(\"^.*/Frameworks\",\"@executable_path/../Frameworks\",\$$1); print \$$1}") "$$1"; \
	    chmod -R a-w "$$1"; \
	} && \
	fix_install_id $(PD_APP_CONTENTS)/Frameworks/Tcl.framework/Tcl Tcl && \
	fix_install_id $(PD_APP_CONTENTS)/Frameworks/Tk.framework/Tk Tk && \
	fix_install_name "$(PD_APP_CONTENTS)/MacOS/$(WISH_NAME)" Tcl && \
	fix_install_name "$(PD_APP_CONTENTS)/MacOS/$(WISH_NAME)" Tk
endif # NOT /System
# set up app wrapper
	install -d "$(PD_APP_CONTENTS)/Resources"
	install -m0644 -p Info.plist "$(PD_APP_CONTENTS)"
	install -m0644 -p *.icns  "$(PD_APP_CONTENTS)/Resources"
	mv "$(PD_APP_CONTENTS)/MacOS/${WISH_NAME}" \
		"$(PD_APP_CONTENTS)/MacOS/${PD_APP_NAME}"

#------------------------------------------------------------------------------#
# generate Info.plist using PD_APP_NAME and PD_VERSION
Info.plist: CFBUNDLEVERSION_DATE := $(shell date +%Y.%m.%d)
Info.plist: Info.plist.in 
	cat Info.plist.in | \
		sed 's/PD_APP_NAME/$(PD_APP_NAME)/' | \
		sed 's/CFBUNDLESHORTVERSIONSTRING/$(PD_MAJOR_VERSION).$(PD_MINOR_VERSION).$(PD_BUGFIX_VERSION)/' | \
		sed 's/CFBUNDLEVERSION/$(CFBUNDLEVERSION_DATE)/' > \
		Info.plist

spotlight_importer_install:
	cd spotlight_importer && xcodebuild
	install -d -m0755 "$(PD_APP_CONTENTS)/Library/Spotlight"
	cp -Rp spotlight_importer/build/Release/PureData.mdimporter "$(PD_APP_CONTENTS)/Library/Spotlight"

# tigital's Gem window focus black magic
rez_install:
	install -pv "$(CWD)/mac.r" "$(DESTDIR)$(bindir)"
	cd "$(DESTDIR)$(bindir)" && /Developer/Tools/Rez -t APPL mac.r -o pd


darwin_app_embed_libs:
# embed libquicktime plugins
	install -d $(PD_APP_CONTENTS)/lib/libquicktime1
	install -p /sw/lib/libquicktime1/*.so $(PD_APP_CONTENTS)/lib/libquicktime1
	./embed-MacOSX-dependencies.sh $(PD_APP_CONTENTS)

# shortcut to install the readme on non-Extended builds
readme_install:
	make -C $(packages_src) $(DEST_PATHS) readme_install


#------------------------------------------------------------------------------#
darwin_app_core: darwin_app_wrapper
	$(MAKE) -C $(packages_src) $(DEST_PATHS) \
		PD_CONFIGURE_FLAGS="--enable-jack --disable-fat" \
		pd_install 
	install -d "$(PD_APP_CONTENTS)/Resources/bin"
	cd "$(PD_APP_CONTENTS)/Resources" && \
		ln -s bin Scripts
	cd "$(PD_APP_CONTENTS)/Resources/bin" && \
		test -e AppMain.tcl || ln -s pd.tk AppMain.tcl
	cd "$(PD_APP_CONTENTS)/Resources/bin" && \
		test -e pdextended || ln -s pd pdextended
# support for Info Panel Plugins mgmt
	cd "$(PD_APP_CONTENTS)" && ln -s Resources/extra Plugins
	install -p -m0644 org.puredata.pdextended.default.plist "$(PD_APP_CONTENTS)/"
# since Pd-vanilla on Mac OS X doesn't have an 'include/pd' folder,
# Pd-extended will mimic Pd-vanilla's include/pd until Pd-vanilla gets one
	ln -s pdextended "$(PD_APP_CONTENTS)/Resources/include/pd"
	ln -s pd/m_pd.h "$(PD_APP_CONTENTS)/Resources/include/m_pd.h"


#==============================================================================#
#
## PACKAGING
#
#==============================================================================#

package: dmg

VOLUME_NAME = Pd-extended
dmg:
	hdiutil create -format UDRW -fs HFS+ -srcfolder "$(BUILD_BASE)" \
		-volname $(VOLUME_NAME) build.dmg
# detach one with the same name first
	-hdiutil detach "/Volumes/$(VOLUME_NAME)"
	hdiutil attach build.dmg
	install -p $(DESTDIR)$(manualsdir)/Pd/ReadMe.html "/Volumes/$(VOLUME_NAME)"
# add link to /Applications for easy install
	ln -s /Applications /Volumes/$(VOLUME_NAME)/Applications
# Install to .background for easier manual DS_Store makeing 
	install -d "/Volumes/$(VOLUME_NAME)/.background"
# The full path to the background image needs to be added to .DS_Store to work
# Hence the background image will only show when the disk image is called 
# "$(VOLUME_NAME)=Pd-extended - that is what the current DS_Store uses.
# To create the DS_Store file one must... search the net.
	install -p background.png "/Volumes/$(VOLUME_NAME)/.background/"
	install -p DS_Store "/Volumes/$(VOLUME_NAME)/.DS_Store"
	install -p VolumeIcon.icns "/Volumes/$(VOLUME_NAME)/.VolumeIcon.icns"
# To enable the local image icon
	/Developer/Tools/SetFile -a C /Volumes/$(VOLUME_NAME)/.VolumeIcon.icns \
		/Volumes/$(VOLUME_NAME)
	$(MAKE) -C $(packages_src) \
		DESTDIR="/Volumes/$(VOLUME_NAME)/$(PD_APP_NAME).app/Contents/Resources" doc_format
	chmod -R a-w "/Volumes/$(VOLUME_NAME)/$(PD_APP_NAME).app/Contents/Resources"
	hdiutil detach `mount | grep $(VOLUME_NAME) | cut -d ' ' -f 1`
	hdiutil convert -format UDZO -o "$(PACKAGE_NAME).dmg" build.dmg
	rm -f build.dmg
# install license
	hdiutil unflatten "$(PACKAGE_NAME).dmg"
	/Developer/Tools/Rez /Developer/Headers/FlatCarbon/*.r SLA.r -a \
		-o "$(PACKAGE_NAME).dmg"
	hdiutil flatten "$(PACKAGE_NAME).dmg"



#==============================================================================#
#
## CLEAN TARGETS
#
#==============================================================================#


distclean: package_clean
	$(MAKE) -C $(packages_src) $(DEST_PATHS) distclean


clean: package_clean
	$(MAKE) -C $(packages_src) $(DEST_PATHS) clean


package_clean: darwin_app_clean spotlight_importer_clean
	-chmod -R u+w -- $(BUILD_BASE)
	-rm -rf -- $(BUILD_BASE)
	-rm Info.plist
	-rm -f -- "$(CWD)/$(PACKAGE_NAME)/ReadMe.html"

spotlight_importer_clean:
	-rm -rf -- spotlight_importer/build

darwin_app_clean:
	-chmod -R u+w $(PD_APP_CONTENTS)/Frameworks
	-rm -rf -- $(PD_APP_CONTENTS)/Frameworks
	-rm -rf -- $(PD_APP_CONTENTS)/Library
	-rm -f -- $(PD_APP_CONTENTS)/Plugins $(PD_APP_CONTENTS)/Info.plist
	-rm -f -- "$(PD_APP_CONTENTS)/MacOS/$(PD_APP_NAME)"
	-rmdir  $(PD_APP_CONTENTS)/MacOS
	-rm -f -- $(PD_APP_CONTENTS)/Resources/bin/*.tk
	-rm -f -- $(PD_APP_CONTENTS)/Resources/bin/*.tcl
	-rm -f -- $(PD_APP_CONTENTS)/Resources/Scripts
	-rm -f -- "$(PD_APP_CONTENTS)/Resources/$(WISH_NAME).rsrc" \
		"$(PD_APP_CONTENTS)/Resources/pd*.icns"
	-rmdir  $(PD_APP_CONTENTS)/Resources
	-rmdir  $(BUILD_BASE)/$(PD_APP_NAME).app/Contents
	-rmdir  $(BUILD_BASE)/$(PD_APP_NAME).app
	-rmdir  $(BUILD_BASE)
	-rm -f -- "$(PACKAGE_NAME).dmg"
	-rm -f -- build.dmg


#==============================================================================#
#
## TEST TARGETS
#
#==============================================================================#

test_package:
	echo "Bypassing test..."
#	echo "Mounting $(PACKAGE_NAME)"
# this triggers the license prompt... hmm how to work around that...
#	cd $(CWD) && hdiutil mount "$(PACKAGE_NAME).dmg"
# some better tests should go in here
#	echo "Ejecting $(PACKAGE_NAME)"
#	cd $(CWD) && hdiutil eject `mount | grep "$(PACKAGE_NAME)" | cut -d ' ' -f 1`



test_locations:
	@echo --------------------------------------------------
	@echo "OPT_CFLAGS: $(OPT_CFLAGS)"
	@echo "PD_VERSION: $(PD_VERSION)"
	@echo "PACKAGE_NAME: $(PACKAGE_NAME)"
	@echo "PD-EXTENDED_VERSION: $(PD-EXTENDED_VERSION)"
	@echo "WISH: $(WISH)"
	@echo "WISH_FRAMEWORKS_SOURCE: $(WISH_FRAMEWORKS_SOURCE)"
	@echo "Tcl/Tk Version: $(shell sed -n 's|.*>\(8\..*\)<.*|\1|p' $(WISH_FRAMEWORKS_SOURCE)/Tk.framework/Resources/Info.plist | head -1)"
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
	sw_vers
	@echo --------------------------------------------------
	autoconf --version
	@echo --------------------------------------------------
	make --version
	@echo --------------------------------------------------
	gcc --version
	@echo --------------------------------------------------
	xcodebuild -version || pbxbuild -version
