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
	$(MAKE) -C $(externals_src) $(DEST_PATHS) install INCREMENTAL=$(INCREMENTAL) LIGHT=$(LIGHT) CFLAGS_ADD="$(CFLAGS)"


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



# AG: Replaced the previous jumble of echo commands with document generation
# from Markdown using awk for preprocessing and pandoc for formatting. Also
# updated the document content which was really outdated. You can find all
# this stuff including the Markdown source in the gendoc subdirectory.

osname = $(shell uname -s|sed 's/\([[:alpha:]]*\).*/\1/')

WELCOME_FILE = $(DESTDIR)$(manualsdir)/$(PD_NAME)/Welcome.html
welcome_install:
	install -d $(DESTDIR)$(manualsdir)/$(PD_NAME)
	rm -f $(WELCOME_FILE)
	$(MAKE) -C gendoc Welcome-$(osname).html version="$(PD_L2ORK_VERSION)" build_version="$(PD_BUILD_VERSION)"
	cp gendoc/Welcome-$(osname).html $(WELCOME_FILE)



README_FILE = $(DESTDIR)$(manualsdir)/$(PD_NAME)/ReadMe.html
readme_install: 
	install -d $(DESTDIR)$(manualsdir)/$(PD_NAME)
	echo $(CYCLONE_RELEASE)
	rm -f $(README_FILE)
	$(MAKE) -C gendoc ReadMe-$(osname).html version="$(PD_L2ORK_VERSION)" build_version="$(PD_BUILD_VERSION)"
	cp gendoc/ReadMe-$(osname).html $(README_FILE)



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
# be kept in a source code repository. Instead, we create s_version.h from
# s_version.h.in, where the latter is kept in the repo and the former gets
# created from the latter by substituting PD_BUILD_VERSION. NOTE: This may
# fail if we're building in a static tarball snapshop of the repo, in which
# case we assume that a pre-generated s_version.h is supplied.

git_version: $(pd_src)/src/s_version.h.in
	@test -d $(cvs_root_dir)/.git && cd $(pd_src)/src/ && \
	sed 's|^\(#define PD_BUILD_VERSION "\).*"|\1$(PD_BUILD_VERSION)"|' s_version.h.in > s_version.h.new && \
	(test -f s_version.h && diff -q s_version.h s_version.h.new > /dev/null && rm -f s_version.h.new && echo s_version.h unchanged || (rm -f s_version.h && mv s_version.h.new s_version.h)) || test -f $(pd_src)/src/s_version.h

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
# These are precious, don't clean them! Otherwise pre-generated docs will be
# removed before the build starts, and the build system may not have the
# required tools (i.e., pandoc) installed to re-generate them.
#	$(MAKE) -C gendoc clean

externals_clean:
	-$(MAKE) -C $(externals_src) $(DEST_PATHS) clean

pd_clean:
	-$(MAKE) -C $(pd_src)/src $(DEST_PATHS) clean

# these targets are all from Makefile.buildlayout: install_clean cruft_clean
clean: abstractions_clean doc_clean externals_clean pd_clean 
	echo "Complete clean finished."


distclean: cruft_clean doc_clean
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
