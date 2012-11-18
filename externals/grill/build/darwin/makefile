# generate MacOS X packages
#
# started by Adam Lindsay
# added by Hans-Christoph Steiner <hans@eds.org>

CXX=g++

# where to find the pd src for headers
PDPATH = ../../../pd/src
FLEXTPATH = ../flext
FLEXTLIB = $(FLEXTPATH)/pd-darwin/libflext.a
PD = ../../../pd/bin/pd

INSTALL_PREFIX = installroot
PD_ROOT = pd

HELP = $(INSTALL_PREFIX)/$(PD_ROOT)/doc/5.reference
EXTERNALS = $(INSTALL_PREFIX)/$(PD_ROOT)/extra
ADDITIONAL = $(INSTALL_PREFIX)/$(PD_ROOT)/doc/flext

FLEXT_VERSION = $(shell grep PROJECT_NUMBER ../../flext/flext.doxy | cut -d '"' -f2 | cut -d ' ' -f2)
PACKAGE_PREFIX = pd-flext
PACKAGE_NAME = $(PACKAGE_PREFIX)-$(FLEXT_VERSION)
PACKAGE_VERSION = ($shell date +%Y.%m.%d)

DMG_PREFIX = $(PACKAGE_PREFIX)
DMG_NAME = $(DMG_PREFIX)-$(PD_VERSION)-$(PACKAGE_VERSION)

WELCOME_FILE = Welcome.html

all:
# FLEXT
	make -C ../../flext -f makefile.pd-darwin PDPATH=$(PDPATH) CXX=$(CXX)
# FLEXTERNALS
#	make -C ../../dyn -f makefile.pd-darwin CXX=$(CXX) \
#			INCLUDES="$(PDPATH) $(FLEXTPATH)/source/"\
#			FLEXTLIB=$(FLEXTLIB) FLEXTPATH=$(FLEXTPATH) PD=$(PD)
#	make -C ../../fftease -f makefile.pd-darwin   CXX=$(CXX) \
#			INCLUDES="$(PDPATH) $(FLEXTPATH)/source/"\
#			FLEXTLIB=$(FLEXTLIB) FLEXTPATH=$(FLEXTPATH) PD=$(PD)
#	make -C ../../idelay -f makefile.pd-darwin CXX=$(CXX) \
#			INCLUDES="$(PDPATH) $(FLEXTPATH)/source/"\
#			FLEXTLIB=$(FLEXTLIB) FLEXTPATH=$(FLEXTPATH) PD=$(PD)
	make -C ../../pool -f makefile.pd-darwin CXX=$(CXX) \
			INCLUDES="$(PDPATH) $(FLEXTPATH)/source/"\
			FLEXTLIB=$(FLEXTLIB) FLEXTPATH=$(FLEXTPATH) PD=$(PD)
#	make -C ../../py -f makefile.pd-darwin CXX=$(CXX) \
#			INCLUDES="$(PDPATH) $(FLEXTPATH)/source/"\
#			FLEXTLIB=$(FLEXTLIB) FLEXTPATH=$(FLEXTPATH) PD=$(PD)
## vasp fails without the dir
#	test -d ../../vasp/pd-darwin || mkdir -p ../../vasp/pd-darwin 
#	make -C ../../vasp -f makefile.pd-darwin  CXX=$(CXX) \
#			INCLUDES="$(PDPATH) $(FLEXTPATH)/source/"\
#			FLEXTLIB=$(FLEXTLIB) FLEXTPATH=$(FLEXTPATH) PD=$(PD)
#	make -C ../../vst -f makefile.pd-darwin CXX=$(CXX) \
#			INCLUDES="$(PDPATH) $(FLEXTPATH)/source/"\
#			FLEXTLIB=$(FLEXTLIB) FLEXTPATH=$(FLEXTPATH) PD=$(PD)
	make -C ../../xsample -f makefile.pd-darwin CXX=$(CXX) \
			INCLUDES="$(PDPATH) $(FLEXTPATH)/source/"\
			FLEXTLIB=$(FLEXTLIB) FLEXTPATH=$(FLEXTPATH) PD=$(PD)

clean:
	make -C ../../flext -f makefile.pd-darwin clean
	make -C ../../dyn -f makefile.pd-darwin clean
	make -C ../../fftease -f makefile.pd-darwin  clean
	make -C ../../idelay -f makefile.pd-darwin clean
	make -C ../../pool -f makefile.pd-darwin clean
	make -C ../../py -f makefile.pd-darwin clean
	make -C ../../vasp -f makefile.pd-darwin  clean
	make -C ../../vst -f makefile.pd-darwin  clean
	make -C ../../xsample -f makefile.pd-darwin clean

darwin_pkg_clean:
	-sudo rm -Rf $(INSTALL_PREFIX) $(PACKAGE_PREFIX)*.pkg/
	-sudo rm -Rf "$(DMG_NAME)" 
	-rm -f $(PACKAGE_PREFIX)-*.info 1 License.???* Welcome.???* *.dmg

darwin_pkg_welcome:
	-rm $(WELCOME_FILE)
	echo "<HTML><BODY>" > $(WELCOME_FILE)
	echo "<H2>FLEXT $(FLEXT_VERSION)-$(PACKAGE_VERSION)</H2>" >> $(WELCOME_FILE)
	echo "<P>For more information, go to: http://www.parasitaere-kapazitaeten.net/~thomas/ext/flext/</P>" >> $(WELCOME_FILE)
	echo "<H3>Included Versions</H3>" >> $(WELCOME_FILE)	
	echo "<P>The various versions of the included packages:</P>" >> $(WELCOME_FILE)
	echo "<UL>" >> $(WELCOME_FILE)
	echo "<LI>flext: $(FLEXT_VERSION) " >> $(WELCOME_FILE)
	echo "</UL>" >> $(WELCOME_FILE)
	echo "(this package was built on `date`) <BR>" >> $(WELCOME_FILE)
	echo "</BODY></HTML>" >> $(WELCOME_FILE)

darwin_pkg_license:
  # generate HTML version of License
	echo "<HTML><BODY><FONT SIZE="-1">" > License.html
	cat ../../flext/gpl.txt | sed -e 's/^$$/\<P\>/g' >> License.html	
	echo "</FONT></BODY></HTML>" >> License.html

# install into MSP's default: /usr/local/lib

darwin_pkg: all darwin_pkg_clean darwin_pkg_license
# set up installroot dir
	test -d $(HELP)  || mkdir -p  $(HELP)
	test -d $(EXTERNALS)  || mkdir -p  $(EXTERNALS)
	test -d $(ADDITIONAL)  || mkdir -p  $(ADDITIONAL)
	test -d $(INSTALL_PREFIX)$(PD_ROOT)/flext \
			|| mkdir -p $(INSTALL_PREFIX)$(PD_ROOT)/flext
# FLEXT
	test -d $(HELP)/../tutorials || mkdir -p $(HELP)/../tutorials
	cp -r ../../flext/tutorial $(HELP)/../tutorials/flext
	mkdir $(ADDITIONAL)/flext
	install -m444 ../../flext/gpl.txt ../../flext/readme.txt ../../flext/license.txt \
		$(ADDITIONAL)/flext
# FLEXTERNALS
# dyn
#	install -m444 ../../dyn/pd-darwin/*.pd_darwin $(EXTERNALS)
	install -m444 ../../dyn/pd/*.pd $(HELP)
	mkdir $(ADDITIONAL)/dyn
	install -m444 ../../dyn/gpl.txt ../../dyn/readme.txt \
		../../dyn/license.txt $(ADDITIONAL)/dyn
# fftease
#	install -m444 ../../fftease/pd-darwin/*.pd_darwin $(EXTERNALS)
	install -m444 ../../fftease/pd/*.pd $(HELP)
	mkdir $(ADDITIONAL)/fftease
	install -m444 ../../fftease/gpl.txt ../../fftease/readme.txt \
		../../fftease/license.txt $(ADDITIONAL)/fftease
	cp  -r ../../fftease/media $(ADDITIONAL)/fftease
# idelay
#	install -m444 ../../dyn/pd-darwin/*.pd_darwin $(EXTERNALS)
	install -m444 ../../idelay/pd/*.pd $(HELP)
	mkdir $(ADDITIONAL)/idelay
	install -m444 ../../idelay/gpl.txt ../../idelay/license.txt $(ADDITIONAL)/idelay
# pool
	install -m444 ../../pool/pd-darwin/*.pd_darwin $(EXTERNALS)
	install -m444 ../../pool/pool.pd $(HELP)
	mkdir $(ADDITIONAL)/pool
	install -m444 ../../pool/gpl.txt ../../pool/readme.txt ../../pool/license.txt \
		$(ADDITIONAL)/pool
# py/pyext
#	install -m444 ../../py/pd-darwin/*.pd_darwin $(EXTERNALS)
	mkdir -p $(ADDITIONAL)/py/examples
	mkdir -p $(ADDITIONAL)/py/scripts
	install -m444 ../../py/pd/*.pd $(ADDITIONAL)/py/examples
	install -m444 ../../py/scripts/*.py $(ADDITIONAL)/py/scripts
	install -m444 ../../py/gpl.txt ../../py/readme.txt ../../py/license.txt \
		$(ADDITIONAL)/py
#	mkdir -p $(INSTALL_PREFIX)/Library/Frameworks
#	cp -R /Library/Frameworks/Python.framework $(INSTALL_PREFIX)/Library/Frameworks
# vasp
#  fails without the dir
#	test -d ../../vasp/pd-darwin || mkdir -p ../../vasp/pd-darwin 
#	install -m444 ../../vasp/pd-darwin/*.pd_darwin $(EXTERNALS)
	install -m444 ../../vasp/pd-help/*.pd $(HELP)
	mkdir $(ADDITIONAL)/vasp
	install -m444 ../../vasp/gpl.txt ../../vasp/readme.txt ../../vasp/license.txt \
		../../vasp/changes.txt ../../vasp/todo.txt  $(ADDITIONAL)/vasp
	cp -r ../../vasp/pd-ex $(ADDITIONAL)/vasp
# vst
#	install -m444 ../../vst/pd-darwin/*.pd_darwin $(EXTERNALS)
	install -m444 ../../vst/pd/*.pd $(HELP)
	mkdir $(ADDITIONAL)/vst
	install -m444 ../../vst/gpl.txt ../../vst/readme.txt \
		../../vst/license.txt $(ADDITIONAL)/vst
# xsample
	install -m444 ../../xsample/pd-darwin/*.pd_darwin $(EXTERNALS)
	install -m444 ../../xsample/pd/*.pd $(HELP)
	mkdir $(ADDITIONAL)/xsample
	install -m444 ../../xsample/gpl.txt ../../xsample/readme.txt \
		 ../../xsample/license.txt $(ADDITIONAL)/xsample
#
	cp -f pd-flext.info $(PACKAGE_NAME).info
# delete cruft
	-find installroot -name .DS_Store -delete
	-rm -f 1
# set proper permissions
	sudo chown -R root:staff $(INSTALL_PREFIX)
	package $(INSTALL_PREFIX) $(PACKAGE_NAME).info -d . -ignoreDSStore
# install pkg docs
	install -m644 License.html $(PACKAGE_NAME).pkg/Contents/Resources
	cd Resources && \
		install -m755 postinstall postupgrade ../$(PACKAGE_NAME).pkg/Contents/Resources
	sudo chown -R root:staff $(PACKAGE_NAME).pkg/Contents/Resources

dmg: darwin_pkg
	mv installroot "$(DMG_NAME)"
	/bin/sh mkdmg "$(DMG_NAME)"
