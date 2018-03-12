#==============================================================================#
#
# Centralized cross-platform build system 
#
# see for instructions: http://puredata.org/docs/developer/build
#  <hans@at.or.at>
#
#==============================================================================#

CWD := $(shell pwd)

cvs_root_dir := $(shell cd $(CWD)/.. && pwd)
DESTDIR = $(CWD)/build
BUILDLAYOUT_DIR = $(CWD)


## default target
all: pd abstractions doc externals
	@echo "Complete build succeeded!"

include $(BUILDLAYOUT_DIR)/Makefile.buildlayout

#==============================================================================#
#
# GET VERSIONS FOR RELEVANT SOFTWARE
#
#==============================================================================#


CYCLONE_MAJOR_VERSION := $(shell grep CYCLONE_VERSION \
	$(externals_src)/miXed/cyclone/build_counter | cut -d ' ' -f 3 | \
	cut -d '"' -f 2)
CYCLONE_RELEASE := $(shell grep CYCLONE_RELEASE \
	$(externals_src)/miXed/cyclone/build_counter | cut -d ' ' -f 3 | \
	cut -d '"' -f 2)
CYCLONE_BUILD := $(shell grep CYCLONE_BUILD \
	$(externals_src)/miXed/cyclone/build_counter | cut -d ' ' -f 3 | \
	cut -d '"' -f 2)
CYCLONE_VERSION := $(CYCLONE_MAJOR_VERSION) $(CYCLONE_RELEASE) $(CYCLONE_BUILD)

GEM_VERSION := $(shell sed -n 's|^AC_INIT.\[Gem\],\[\([0-9]\).*|\1|p' $(gem_src)/configure.ac).$(shell sed -n 's|^AC_INIT.\[Gem\],\[[0-9]\.\([0-9][0-9]*\).*|\1|p' $(gem_src)/configure.ac)

MAXLIB_VERSION := $(shell grep "define VERSION" \
	$(externals_src)/maxlib/maxlib.c | cut -d '"' -f 2)

OSC_VERSION := $(shell grep "define VERSION" $(externals_src)/OSCx/src/OSC.c | \
	cut -d '"' -f 2)

PDP_VERSION := $(shell grep PDP_VERSION= $(externals_src)/pdp/configure.ac | \
	cut -d '=' -f 2)

PIDIP_VERSION := $(shell grep _VERSION= $(externals_src)/pidip/configure.ac | \
       cut -d '"' -f 2)

PMPD_VERSION := $(shell grep "define VERSION" \
	$(externals_src)/pmpd/pmpd.c | cut -d '"' -f 2)


TOXY_MAJOR_VERSION := $(shell grep TOXY_VERSION \
	$(externals_src)/miXed/toxy/build_counter | cut -d ' ' -f 3 | \
	cut -d '"' -f 2)
TOXY_RELEASE := $(shell grep TOXY_RELEASE \
	$(externals_src)/miXed/toxy/build_counter | cut -d ' ' -f 3 | \
	cut -d '"' -f 2)
TOXY_BUILD := $(shell grep TOXY_BUILD \
	$(externals_src)/miXed/toxy/build_counter | cut -d ' ' -f 3 | \
	cut -d '"' -f 2)
TOXY_VERSION := $(TOXY_MAJOR_VERSION) $(TOXY_RELEASE) $(TOXY_BUILD)


ZEXY_VERSION := $(shell grep VERSION $(externals_src)/zexy/src/zexy.h | \
	grep -v _VERSION | cut -d ' ' -f 3 | cut -d '"' -f 2)


# these are sent to all of the various Makefiles so that they all copy their
# output to the same directory tree
DEST_PATHS = BUILDLAYOUT_DIR=$(BUILDLAYOUT_DIR) \
				cvs_root_dir=$(cvs_root_dir) \
				DESTDIR=$(DESTDIR) \
				prefix=$(prefix) \
				libpddir=$(libpddir) \
				OPT_CFLAGS="$(OPT_CFLAGS)" \
				UNAME=$(UNAME)


#==============================================================================#
#
# BUILD TARGETS
#
#==============================================================================#

#------------------------------------------------------------------------------
# abstractions
abstractions:


#------------------------------------------------------------------------------
# doc
doc:


#------------------------------------------------------------------------------
# externals
externals: 
	$(MAKE) -C $(externals_src) $(DEST_PATHS)


#------------------------------------------------------------------------------
# pd

ifeq ($(OS_NAME),darwin)
  PD_CONFIGURE_FLAGS = --enable-jack
else
  ifeq ($(OS_NAME),linux)
    PD_CONFIGURE_FLAGS = --enable-jack --enable-alsa --enable-fftw
    ifneq ($(TAR_EM_UP_PREFIX), )
      PD_CONFIGURE_FLAGS +=  --prefix=$(TAR_EM_UP_PREFIX)
    endif
  else
    PD_CONFIGURE_FLAGS = 
  endif
endif

PD_BUILD_FLAGS = 

HAVE_AUTOGEN_SH := $(wildcard $(pd_src)/autogen.sh)
# test for the new build system
ifeq (autogen.sh, $(findstring autogen.sh,$(HAVE_AUTOGEN_SH)))
#----------------------------------------------------------------------------#
# new autotools build system
pd: git_version
	cd $(pd_src) \
		&& ./autogen.sh \
		&& ./configure $(PD_CONFIGURE_FLAGS) \
		&& make $(DEST_PATHS) $(PD_BUILD_FLAGS)

PD_NAME=Pd
pd_install: pd
	$(MAKE) -C $(pd_src) $(DEST_PATHS) $(PD_BUILD_FLAGS) install
	install -d $(DESTDIR)$(manualsdir)/$(PD_NAME)
	install -p $(pd_src)/src/notes.txt $(DESTDIR)$(manualsdir)/$(PD_NAME)
else
#----------------------------------------------------------------------------#
# old build system
PD_AUTOCONF = autoconf
$(pd_src)/src/configure: $(pd_src)/src/configure.ac
	cd $(pd_src)/src/ && \
	if [ -f acinclude.m4 ] ; then aclocal ; fi && \
	$(PD_AUTOCONF) && \
	if grep -q AC_CONFIG_HEADERS configure.ac ; then autoheader ; fi

# this is not used yet because MinGW doesn't use ./configure && make yet
#$(pd_src)/src/makefile: $(pd_src)/src/makefile.in
#	cd $(pd_src)/src/ && ./configure $(PD_CONFIGURE_FLAGS)

# this line has the "-" to support the current makefile.mingw hack
pd: $(pd_src)/src/configure git_version
	-cd $(pd_src)/src && ./configure $(PD_CONFIGURE_FLAGS) && \
		make OPT_CFLAGS="" depend  && \
		make $(DEST_PATHS) $(PD_BUILD_FLAGS)

PD_NAME=Pd
pd_install: pd
	$(MAKE) -C $(pd_src)/src $(DEST_PATHS) $(PD_BUILD_FLAGS) install
	install -d $(DESTDIR)$(manualsdir)/$(PD_NAME)
	install -p $(pd_src)/src/notes.txt $(DESTDIR)$(manualsdir)/$(PD_NAME)

endif


#==============================================================================#
#
# INSTALL TARGETS
#
#==============================================================================#

# this is used for installing into a pre-build Pd binary
extended_install: abstractions_install \
externals_install readme_install welcome_install \
license_install noncvs_install


#------------------------------------------------------------------------------
# install
install: pd_install extended_install
	@echo "Complete install succeeded!"

#------------------------------------------------------------------------------
# install_devel
devel_install: pd_devel_install
	@echo "Complete install succeeded!"

#------------------------------------------------------------------------------
# abstractions_install
abstractions_install:
ifneq ($(LIGHT),yes)
	$(MAKE) -C $(abstractions_src) $(DEST_PATHS) install
endif

#------------------------------------------------------------------------------
# extensions_install
extensions_install: 
	$(MAKE) -C $(extensions_src) $(DEST_PATHS) install


#------------------------------------------------------------------------------
# externals_install
externals_install: 
	$(MAKE) -C $(externals_src) $(DEST_PATHS) install INCREMENTAL=$(INCREMENTAL) LIGHT=$(LIGHT)


#------------------------------------------------------------------------------
# noncvs_install
# this is for including pre-compiled binaries in a build
noncvs_install:
	-install -p $(packages_src)/noncvs/$(OS_NAME)/bin/*.* $(DESTDIR)$(bindir)
	-install -p $(packages_src)/noncvs/$(OS_NAME)/doc/5.reference/*.* $(DESTDIR)$(helpdir)
	-install -p $(packages_src)/noncvs/$(OS_NAME)/extra/*.* $(DESTDIR)$(objectsdir)


#==============================================================================#
#
# GENERATE TEXT FILES FOR PACKAGE
#
#==============================================================================#

LICENSE_FILE = $(DESTDIR)$(manualsdir)/$(PD_NAME)/License.html
license_install:
  # generate HTML version of License
	install -d $(DESTDIR)$(manualsdir)/$(PD_NAME)
	rm -f $(LICENSE_FILE)
	sed -e 's:pd/LICENSE.txt:Pd-LICENSE.txt:' $(cvs_root_dir)/LICENSE.html > $(LICENSE_FILE)
# Pd's license file
	install -p "$(pd_src)/LICENSE.txt" "$(DESTDIR)$(manualsdir)/$(PD_NAME)/Pd-LICENSE.txt"



WELCOME_FILE = $(DESTDIR)$(manualsdir)/$(PD_NAME)/Welcome.html
welcome_install:
	install -d $(DESTDIR)$(manualsdir)/$(PD_NAME)
	-rm $(WELCOME_FILE)
	touch $(WELCOME_FILE)
	echo "<html><head>" >> $(WELCOME_FILE)
	echo "<link rel=\"stylesheet\" type=\"text/css\" href=\"http://puredata.org/ploneCustom.css\" />" >> $(WELCOME_FILE)
	echo "</head>" >> $(WELCOME_FILE)
	echo "<body><p><p>" >> $(WELCOME_FILE)
	echo "<h2>Pd-l2ork version $(PD_L2ORK_VERSION) ($(PD_BUILD_VERSION))</h2>" >> $(WELCOME_FILE)
	echo "<p>Pd-l2ork was written by Ivica Bukvic and Jonathan Wilkes. It is a fork of Hans-Christoph Steiner's Pd-extended, which in turn is based on Miller S. Puckette's Pd. The included Pd abstractions and externals are the work of many people whose contributions are gratefully acknowledged.</p>" >> $(WELCOME_FILE)
	echo "<font size=\"-1\">" >> $(WELCOME_FILE)
	echo "<p>`grep -A9 ACKNOWLEDG $(pd_src)/README.txt`</p>" >> $(WELCOME_FILE)
	echo "</font>" >> $(WELCOME_FILE)
	echo "</body></html>" >> $(WELCOME_FILE)



README_FILE = $(DESTDIR)$(manualsdir)/$(PD_NAME)/ReadMe.html
readme_install: 
	install -d $(DESTDIR)$(manualsdir)/$(PD_NAME)
	echo $(CYCLONE_RELEASE)
	-rm $(README_FILE)
	touch $(README_FILE)
	echo "<html>" >> $(README_FILE)
	echo "<head>" >> $(README_FILE)
	echo "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />" >> $(README_FILE)
	echo "<link rel=\"stylesheet\" type=\"text/css\" href=\"http://puredata.org/plone.css\" />" >> $(README_FILE)
	echo "</head>" >> $(README_FILE)
	echo "<body>" >> $(README_FILE)
	echo "<h2>Pd-l2ork version $(PD_L2ORK_VERSION) ($(PD_BUILD_VERSION))</h2>" 	>> $(README_FILE)
	echo "<p>Pd is a real-time, graphical programming language for media processing created by Miller S. Puckette.  It provides an environment for audio analysis, synthesis, and processing, with a rich set of multimedia capabilities.  You can get Pd for Linux, Windows, MacOS X, BSD, or IRIX.</p>" >> $(README_FILE)
	echo "<p>Pd-l2ork is a version of Pd based on Hans-Christoph Steiner's Pd-extended which features many bugfixes and enhancements, especially in the user interface.  Pd-l2ork was created by Ivica Ico Bukvic and is maintained by a team of volunteers including Jonathan Wilkes who ported it to the Mac and Windows.  It is available for Linux, Windows and MacOS X.</p>" >> $(README_FILE)
	echo "<p>For general information about Pd, go to: <a href=\"http://puredata.org\" target=\"pd\">http://puredata.org</a>.  More information about Pd-l2ork can be found on the <a href=\"http://l2ork.music.vt.edu/main/make-your-own-l2ork/software/\" target=\"pd-l2ork\">Pd-l2ork website</a>.</p>" >> $(README_FILE)
	echo "<h3>Installation</h3>" >> $(README_FILE)	
ifeq ($(OS_NAME),linux)
	echo "<p>Links to Pd-l2ork packages for various Linux distributions are available on the <a href=\"http://l2ork.music.vt.edu/main/make-your-own-l2ork/software/\" target=\"pd-l2ork\">Pd-l2ork website</a>.</p>" >> $(README_FILE)
	echo "<p>If you're running Ubuntu or Arch, you may also want to check out Albert Gräf's packages on Launchpad and the Arch User Repositories (AUR).  Please check his corresponding <a href=\"https://l2orkaur.bitbucket.io/\" target=\"l2orkaur\">Bitbucket</a> <a href=\"https://l2orkubuntu.bitbucket.io/\" target=\"l2orkubuntu\">pages</a> for details.</p>" >> $(README_FILE)
endif
ifeq ($(OS_NAME),darwin)
	echo "<p>To install Pd-l2ork, drag the $(APPLICATION_NAME).app into your <b>/Applications</b> folder or to the shortcut in the disk image.</p>" >> $(README_FILE)
	echo "<p>By default, most of the included libraries are loaded at startup.  To change this, set your own preferences in the <b>Startup</b> tab of the Preferences dialog.  If you want to reset Pd-l2ork to the default preferences, remove the Pd-l2ork preferences file <strong>~/Library/Preferences/org.puredata.pd-l2ork.plist</strong> (<code>~</code> means your home folder), or run the following command in the Terminal.app: </p><p><code>rm ~/Library/Preferences/org.puredata.pd-l2ork.plist</code></p>" >> $(README_FILE)
	echo "<p>If you want to use PDP on Mac OS X, you may have to install X11.  See <a href=\"http://puredata.info/docs/faq/macosx\" target=\"_blank\">How do I install Pd on MacOS X?</a> for details.</p>" >> $(README_FILE)
endif
ifeq ($(OS_NAME),windows)
	echo "<p>To make sure that all of the included libraries are loaded when Pd runs, " >> $(README_FILE)
	echo "double-click <code>C:\Program Files\pd\pd-settings.reg</code> to import the settings to the registry.  WARNING: this will overwrite any existing Pd preferences!</p>" >> $(README_FILE)
	echo "<h3>ASIO4ALL</h3>" >> $(README_FILE)	
	echo "<p><a href=\"http://www.asio4all.com/\" target=\"_blank\">ASIO4ALL</a> is a cost-free <a href=\"http://en.wikipedia.org/wiki/Audio_Stream_Input/Output\" target=\"_blank\">ASIO</a> audio driver.  Is it highly recommended for use with Pd.  Download and install it, then when you launch Pd, choose the <strong>ASIO</strong> option from the <strong>Media</strong> menu.  Using ASIO4ALL should mean less crashes and troubles, as well as better audio performance." >> $(README_FILE)
	echo "</p>" >> $(README_FILE)
endif
	echo "<h3>Installing Externals, Objects, and Help files</h3>" >> $(README_FILE)
	echo "<p>" >> $(README_FILE)
	echo "If you would like to install other externals, objects, help files, etc. there are special folders that Pd-l2ork uses, which are listed below.  If the folder does not exist, you should create it.  You can find out more details about this by reading this FAQ:  <a href=\"http://puredata.info/docs/faq/how-do-i-install-externals-and-help-files\" target=\"_blank\">How do I install externals and help files?</a>." >> $(README_FILE)
	echo "<dl>" >> $(README_FILE)
ifeq ($(OS_NAME),darwin)
	echo "<dt>Only for the current user account</dt>" >> $(README_FILE)
	echo "<dd>~/Library/Pd-l2ork</dd>" >> $(README_FILE)
	echo "<dt>For all user accounts on the computer</dt>" >> $(README_FILE)
	echo "<dd>/Library/Pd-l2ork</dd>" >> $(README_FILE)
endif
ifeq ($(OS_NAME),linux)
	echo "<dt>Only for the current user account</dt>" >> $(README_FILE)
	echo "<dd>~/pd-l2ork-externals/</dd>" >> $(README_FILE)
	echo "<dt>For all user accounts on the computer</dt>" >> $(README_FILE)
	echo "<dd>/usr/local/lib/pd-l2ork-externals</dd>" >> $(README_FILE)
endif
ifeq ($(OS_NAME),windows)
	echo "<dt>Only for the current user account</dt>" >> $(README_FILE)
	echo "<dd><a href=\"file:///%UserProfile%/Application%20Data\">%UserProfile%\Application Data\Pd-l2ork</a></dd>" >> $(README_FILE)
	echo "<dt>For all user accounts on the computer</dt>" >> $(README_FILE)
	echo "<dd><a href=\"file:///%CommonProgramFiles%/\">%ProgramFiles%\Common Files\Pd-l2ork</a></dd>" >> $(README_FILE)
endif
	echo "</dl>" >> $(README_FILE)
	echo "</p>" >> $(README_FILE)
	echo "<h3>License</h3>" >> $(README_FILE)	
	echo "<p>" >> $(README_FILE)
	echo 'This package is released under the <a href="http://www.gnu.org/copyleft/gpl.html" target="gpl">GNU GPL</a>.  The Pd core and some other included code is available under a <a href="http://pure-data.cvs.sourceforge.net/pure-data/pd/LICENSE.txt">BSD license</a> from the Pd source code repository on SourceForge.' >> $(README_FILE)
	echo "</p>" >> $(README_FILE)
	echo "<h3>Patented Algorithms</h3>" >> $(README_FILE)
	echo "<p>" >> $(README_FILE)
	echo "This package may contain software that is covered by patents in certain countries, like the U.S. and Germany.  In order to use this software you must have the proper license.  Below is a list of the known software packages that are covered by patents in some countries:" >> $(README_FILE)
	echo "</p>" >> $(README_FILE)
	echo "<ul>" >> $(README_FILE)
	echo '<li><a href="http://en.wikipedia.org/wiki/MP3" target="license">MP3 aka MPEG-1 Part 3 Layer 3</a></li>' >> $(README_FILE)
	echo '<li><a href="http://en.wikipedia.org/wiki/MPEG-2" target="license">MPEG-2</a></li>' >> $(README_FILE)
	echo '<li><a href="http://en.wikipedia.org/wiki/MPEG-4#Licensing" target="license">MPEG-4</a></li>' >> $(README_FILE)
	echo "</ul>" >> $(README_FILE)
	echo '<p>Please consider trying to get rid of software patents in your country: <a href="http://www.nosoftwarepatents.com" target="nsp">http://www.nosoftwarepatents.com</a></p>' >> $(README_FILE)
	#echo "<h3>Pure Data CVS Developers</h3>" >> $(README_FILE)	
# this may seem whack, but it generates the list of developers from the SourceForge site: (as of 11-13-2014 this 404s, so we disable it)
	#curl 'http://sourceforge.net/project/memberlist.php?group_id=55736' | grep -A2 -e '<td>' | sed 's|\(href="\)|target="w" \1http://sourceforge.net|' >> $(README_FILE)
	#echo "</p>" >> $(README_FILE)
	#echo "<p>" >> $(README_FILE)
	#echo "Many others not listed have contributed their time and effort, this is just a list of the current developers in the SourceForge project.  But really, every Pd user is a developer and is encouraged to contribute to the CVS repository." >> $(README_FILE)
	#echo "</p>" >> $(README_FILE)
	#echo "<h3>Included Versions</h3>" >> $(README_FILE)	
	#echo "<p>These externals are all included from the Pd CVS repository:</p>" >> $(README_FILE)
	#echo "<ul>" >> $(README_FILE)
	#echo "<li>pure data: $(PD_VERSION) " >> $(README_FILE)
	#echo "<li>cyclone: $(CYCLONE_VERSION)" >> $(README_FILE)
	#echo "<li>gem: $(GEM_VERSION)" >> $(README_FILE)
	#echo "<li>iemlib: `date +20%y.%m.%d`" >> $(README_FILE)
	#echo "<li>maxlib: $(MAXLIB_VERSION)" >> $(README_FILE)
	#echo "<li>osc: $(OSC_VERSION)" >> $(README_FILE)
	#echo "<li>pmpd: $(PMPD_VERSION)" >> $(README_FILE)
	#echo "<li>pd-abstractions: `date +20%y.%m.%d`" >> $(README_FILE)
	#echo "<li>pd-doc: `date +20%y.%m.%d`" >> $(README_FILE)
	#echo "<li>pd-externals: `date +20%y.%m.%d`" >> $(README_FILE)
	#echo "<li>pddp: `date +20%y.%m.%d`" >> $(README_FILE)
	#echo "<li>pdp: $(PDP_VERSION)" >> $(README_FILE)
	#echo "<li>pidip: $(PIDIP_VERSION)" >> $(README_FILE)
	#echo "<li>toxy: $(TOXY_VERSION)" >> $(README_FILE)
	#echo "<li>unauthorized: `date +20%y.%m.%d`" >> $(README_FILE)
	#echo "<li>zexy: $(ZEXY_VERSION)" >> $(README_FILE)
	#echo "</ul>" >> $(README_FILE)
	echo "(this package was built on `date`) <BR>" >> $(README_FILE)
	echo "</body></html>" >> $(README_FILE)



#==============================================================================#
#
## FINAL ASSEMBLY
#
#==============================================================================#


#----------------------------------------------------------------------------
# clean up the documentation before packaging
doc_format:
# clean out cruft files
	-find $(DESTDIR) -name .DS_Store -delete
	-find $(DESTDIR) -name '*.*.bak' -delete
	-find $(DESTDIR) -name .svn -print0 | xargs -0 rm -rf


# AG: The set_version target is broken since it modifies m_pd.h in-place
# during the build, which is a bad thing to do to files which are supposed to
# be kept in a source code repository. Instead, we create s_stuff.h from
# s_stuff.h.in, where the latter is kept in the repo and the former gets
# created from the latter by substituting PD_BUILD_VERSION. NOTE: This may
# fail if we're building in a static tarball snapshop of the repo, in which
# case we assume that a pre-generated s_stuff.h is supplied.

git_version: $(pd_src)/src/s_stuff.h.in
	@test -d $(cvs_root_dir)/.git && cd $(pd_src)/src/ && \
	sed 's|^\(#define PD_BUILD_VERSION "\).*"|\1$(PD_BUILD_VERSION)"|' s_stuff.h.in > s_stuff.h.new && \
	(test -f s_stuff.h && diff -q s_stuff.h s_stuff.h.new > /dev/null && rm -f s_stuff.h.new && echo s_stuff.h unchanged || (rm -f s_stuff.h && mv s_stuff.h.new s_stuff.h)) || test -f $(pd_src)/src/s_stuff.h

set_version:
# change Pd's version number to reflect the extended build
# this needs the complete_version_defines patch to work
	cd $(pd_src)/src/ && \
		sed 's|^\(#define PD_TEST_VERSION "\).*"|\1$(PD_BUILD_VERSION)"|' m_pd.h > \
			m_pd.h.tmp && mv m_pd.h.tmp m_pd.h

unset_version:
# change the version number back to the original 
# this needs the complete_version_defines patch to work
	cd $(pd_src)/src && \
		sed 's|^\(#define PD_TEST_VERSION ".*\)$(PD_BUILD_VERSION_PREFIX).*"|\1"|' \
			m_pd.h > m_pd.h.tmp && \
		mv m_pd.h.tmp m_pd.h


#==============================================================================#
#
# CLEAN TARGETS
#
#==============================================================================#
abstractions_clean:
	-$(MAKE) -C $(abstractions_src) $(DEST_PATHS) clean

doc_clean:

externals_clean:
	-$(MAKE) -C $(externals_src) $(DEST_PATHS) clean

pd_clean:
	-$(MAKE) -C $(pd_src)/src $(DEST_PATHS) clean

# these targets are all from Makefile.buildlayout: install_clean cruft_clean
clean: abstractions_clean doc_clean externals_clean pd_clean 
	echo "Complete clean finished."


distclean: cruft_clean
	$(MAKE) -C $(abstractions_src) distclean
	-$(MAKE) -C $(extensions_src) distclean
	$(MAKE) -C $(externals_src) distclean
# Make sure that we don't remove the precious s_stuff.h file if we're building
# outside of the repo.
	-test -d $(cvs_root_dir)/.git && $(MAKE) -C $(pd_src) distclean || $(MAKE) -C $(pd_src) clean

test_locations:
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
	@echo "HAVE_AUTOGEN_SH --$(HAVE_AUTOGEN_SH)--"
