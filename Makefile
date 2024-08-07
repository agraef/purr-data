
# Toplevel Makefile for Purr Data. Please note that at present this is just a
# thin wrapper around l2ork_addons/tar_em_up.sh, the traditional Pd-l2ork
# build script.

# The Pd-l2ork build system is very arcane and intricate. Its main purpose is
# putting together a staging area with a complete Pd-l2ork installation and
# then building installers for the supported platforms from that, pulling
# together a bunch of separate packages, each with their own build system.
# Compiling sources is just one of the tasks that the builder does. Therefore
# you'll notice that, unlike with other less complicated source packages,
# `make` will rebuild lots of things even if you just finished another build.

# The available build targets are:

# all: produce a native installer for the host platform (equivalent to
# `tar_em_up.sh -Tk`); note that in order to force a complete rebuild (like
# what `tar_em_up.sh -T` does), you'll have to run `make clean` first

# incremental: like `all`, but does an "incremental build" (equivalent to
# `tar_em_up.sh -tk`), bypassing Gem which takes an eternity to compile; please
# check the tar_em_up.sh script for details

# light: like `incremental`, but does a light build (equivalent to
# `tar_em_up.sh -tkl`) which only includes the most essential externals;
# please check the tar_em_up.sh script for details

# checkout: convenience target to check out all submodules in preparation for
# a subsequent build (the `all`, `incremental` and `dist` targets also do this
# automatically when needed)

# clean: does something similar to what `tar_em_up.sh` does in order to start
# from a clean slate, so that a subsequent build starts from scratch again

# realclean: put the sources into pristine state again (WARNING: this will get
# rid of any uncommitted source changes, too); use this as a last resort to
# get the sources into a compilable state again after things have gone awry

# dist: create a self-contained distribution tarball of the source

# NOTES:

# The realclean and dist targets use git commands and thus only work in a
# working copy of the git repo, not in the static tarball snapshots produced
# by the dist target.

# On Linux systems running `make` will try to produce a Debian package. On
# Linux distributions like Arch which are no Debian derivatives, the Debian
# packaging tools are not available. In this case, `make` will stop right
# before creating the actual package and leave the ready-made staged
# installation tree under `packages/linux_make/build` from where it can be
# copied or packaged up in any desired way.

# The incremental and light builds assume an existing staging area
# (packages/*/build directory) which is *not* cleaned before installing. This
# makes it possible to update the existing staging area after recompiling just
# a part of the system (all but Gem in the case of "incremental", only the
# Pd core and a few essential externals in the case of "light"). Use `make
# clean` beforehand if you want to install into a clean staging area.

# When doing a `light` build, which only includes the most essential
# externals, it may be desirable to manually include additional abstractions
# and externals in the build. To these ends, after running `make light` you
# can run `make` with the `foo_abs` or `foo_ext` target, where `foo` is the
# name of the desired abstraction or external, respectively. E.g., you can run
# `make light memento_abs pdlua_ext` to get a light build with the `memento`
# abstraction and the `pdlua` external included. (This will not rebuild the
# Debian package, though, so you'll have to install manually with `make
# install` instead.) The names of the desired addons must be specified as
# given in abstractions/Makefile and externals/Makefile, respectively (look
# for targets looking like `foo_install`). Also note that even though a
# subsequent `make install` will then include your addons, they won't be
# enabled by default, so you'll have to do that manually in Purr Data's
# `Startup` dialog. Simply adding the name of the addon in the `Libraries`
# list should normally do the trick. Or you can add an option like `-lib foo`
# when running Purr Data from the command line.

.PHONY: all incremental checkout clean realclean dist

# Target platform (OSX/macOS only): On Mojave (10.14 with Xcode 10) this needs
# to be at least 10.9, which is the default now. With older Xcode versions you
# can try earlier versions (>= 10.4) if you need to compile for legacy OSX
# versions.
export macos_target = 10.9

# Installation prefix under which Pd-l2ork is installed (Linux only). If this
# isn't set, a default location will be used (usually /usr/local). NOTE: We
# *always* assume that this variable is set properly in the install targets,
# as well as the (Linux) check target (see below).
prefix = /usr

ifneq ($(prefix),)
env = inst_dir="$(prefix)"
endif

install_vars = DESTDIR=$(firstword $(wildcard $(CURDIR)/packages/*/build)) prefix=$(prefix)

# You can set the nwjsver variable to indicate the nw.js version to build
# against. This will also clear out any cached nw.js binaries beforehand.
# Note that some nw.js versions for certain platforms have to be hard-coded,
# so the nwjsver variable won't affect these, but it will still cause the
# cache to be cleared and the binaries to be downloaded. See the tar_em_up.sh
# script for details.
ifneq ($(nwjsver),)
env += nwjsver="$(nwjsver)"
endif

# You can set CFLAGS to whatever special compile options are needed. E.g., to
# build the double precision version: CFLAGS = -DPD_FLOATSIZE=64
CFLAGS =
export CFLAGS

# For the light build only, you can add externals to be included in the build.
addons =
export addons

# You can also set this variable to specify externals NOT to be built. E.g.,
# to prevent building Gem (which takes an eternity to build): blacklist = gem
blacklist =
export blacklist

all:
	cd l2ork_addons && $(env) ./tar_em_up.sh -Tk

incremental:
	cd l2ork_addons && $(env) ./tar_em_up.sh -tk

light:
	cd l2ork_addons && $(env) ./tar_em_up.sh -tkl

# Convenience targets to build the double precision version.

# Blacklist of externals which don't work with double precision yet.
double_blacklist = autotune smlib
# These are dubious, passing float* for t_float* pointers, and so are most
# likely broken, even though they compile with double precision.
double_blacklist += cyclone lyonpotpourri

all-double:
	cd l2ork_addons && $(env) CFLAGS=-DPD_FLOATSIZE=64 blacklist="$(double_blacklist)" ./tar_em_up.sh -Tk

incremental-double:
	cd l2ork_addons && $(env) CFLAGS=-DPD_FLOATSIZE=64 blacklist="$(double_blacklist)" ./tar_em_up.sh -tk

light-double:
	cd l2ork_addons && $(env) CFLAGS=-DPD_FLOATSIZE=64 blacklist="$(double_blacklist)" ./tar_em_up.sh -tkl

%_abs:
	make -C abstractions $(@:%_abs=%) $(@:%_abs=%_install) $(install_vars)

%_ext:
	make -C externals $(@:%_ext=%) $(@:%_ext=%_install) $(install_vars)

checkout:
	git submodule update --init --recursive

clean:
	test "$(shell uname -s)" = "Darwin" && make -C packages/darwin_app clean || true
	cd pd/src && aclocal && autoconf && make clean || true
	cd externals/miXed && make clean || true
	cd Gem/src/ && test -f Makefile && make distclean || true
	cd Gem/src/ && rm -rf ./.libs && rm -rf ./*/.libs || true
	cd Gem/ && test -f Makefile && make distclean || true
	cd Gem/ && rm -f gemglutwindow.pd_linux Gem.pd_linux || true
	rm -rf packages/*/build/

realclean:
# This requires a working copy of the git repo.
	@test -d .git || (echo "Not a git repository, bailing out." && false)
	git submodule deinit --all -f
	git checkout .
	git clean -dffx -e pd/nw/nw/

# Check targets. These run the regression tests in scripts.
# Requires a full build.

os = $(shell uname|sed 's/^\(MINGW[0-9]*\)_NT.*/\1/')

ifeq ($(os),Linux)
# Linux (all flavors)
pdprog = packages/linux_make/build$(prefix)/bin/pd-l2ork
else ifeq ($(os),Darwin)
# Mac
pdprog = packages/darwin_app/build/*.app/Contents/Resources/app.nw/bin/pd-l2ork
else ifeq ($(os),MINGW64)
# Msys2 mingw64
pdprog = packages/win64_inno/build/bin/pd.exe
else ifeq ($(os),MINGW32)
# Msys2 mingw32
pdprog = packages/win32_inno/build/bin/pd.exe
endif

ifneq ($(pdprog),)
# This runs just a quick regression test, useful to see whether the program
# works at all.
check1:
	$(pdprog) -noprefs -nogui -noaudio -send 'init dollarzero $$0' scripts/regression_tests.pd

# This runs the full test suite, including the test of the externals.
check:
	$(pdprog) -noprefs -nogui -noaudio -send 'init dollarzero $$0' scripts/regression_tests.pd
	$(pdprog) -noprefs -nostdpath -nogui -noaudio scripts/external-tests.pd
else
check1 check:
	@echo "Target $(os) not recognized, can't run 'make $@'!"; false
endif

# Installation targets. These don't work on Mac and Windows right now, you
# should use the generated installers on these systems instead. Also,
# $(prefix) must be set. $(DESTDIR) is supported as well, so you can do staged
# installs (but then again presumably you already have a staged install
# sitting in packages/*/build, so you might as well use that instead).

# Note that these targets simply (un)install whatever is in the
# packages/*/build directory at the time they're invoked. If no build
# directory is present then nothing will happen, so you need to run `make` (or
# `make incremental`, etc.) before running these targets. Also note that some
# old cruft under build/etc (all but the bash auto-completions) isn't
# installed as it isn't needed on modern Linux systems any more.

builddir = $(firstword $(wildcard packages/*/build))
ifneq ($(builddir),)
manifest = etc/bash_completion.d/pd-l2ork $(prefix:/%=%)/include/pd-l2ork $(prefix:/%=%)/lib/pd-l2ork $(patsubst $(builddir)/%,%, $(wildcard $(builddir)/$(prefix:/%=%)/bin/*) $(shell find $(builddir)/usr/share -type f))
endif

ifneq ($(manifest),)
install:
	test -z "$(DESTDIR)" || (rm -rf "$(DESTDIR)" && mkdir -p "$(DESTDIR)")
	tar -c -C $(builddir) $(manifest) | tar -x -C $(DESTDIR)/
# Edit the library paths in the default user.settings file so that it matches
# our installation prefix.
	test -f "$(DESTDIR)"$(prefix)/lib/pd-l2ork/default.settings && cd "$(DESTDIR)"$(prefix)/lib/pd-l2ork && sed -e "s!/usr/lib/pd-l2ork!$(prefix)/lib/pd-l2ork!g" -i default.settings || true

uninstall:
	rm -rf $(addprefix $(DESTDIR)/, $(manifest))
else
install:
	@echo "no build directory, run make first" && false

uninstall:
	@echo "no build directory, run make first" && false
endif

# Build a self-contained distribution tarball (snapshot). This is pretty much
# the same as in debuild/Makefile and must be run in a working copy of the git
# repo.

# The Debian version gets derived from the date and serial number of the last
# commit.
debversion = $(shell grep PD_L2ORK_VERSION pd/src/m_pd.h | sed 's|^.define *PD_L2ORK_VERSION *"\(.*\)".*|\1|')+git$(shell test -d .git && git rev-list --count HEAD)+$(shell test -d .git && git rev-parse --short HEAD)
# Source tarball and folder.
debsrc = purr-data_$(debversion).orig.tar.gz
debdist = purr-data-$(debversion)

# Submodules (Gem, etc.).
submodules = $(sort $(shell test -d .git && (git config --file .gitmodules --get-regexp path | awk '{ print $$2 }')))

dist: $(debsrc)

# Determine the version number of this build. We get this from m_pd.h.
PD_L2ORK_VERSION := $(shell grep PD_L2ORK_VERSION pd/src/m_pd.h | sed 's|^.define *PD_L2ORK_VERSION *"\(.*\)".*|\1|')
# Determine the build version which needs git to be computed, so we can't do
# it in a stand-alone build from a tarball.
PD_BUILD_VERSION := $(shell test -d .git && (git log -1 --format=%cd --date=short | sed -e 's/-//g'))-rev.$(shell test -d .git && git rev-parse --short HEAD)

$(debsrc):
	@test -d .git || (echo "Not a git repository, bailing out." && false)
	rm -rf $(debdist)
# Make sure that the submodules are initialized.
	git submodule update --init
# Grab the main source.
	git archive --format=tar.gz --prefix=$(debdist)/ HEAD | tar xfz -
# Grab the submodules.
	for x in $(submodules); do (cd $(debdist) && rm -rf $$x && git -C ../$$x archive --format=tar.gz --prefix=$$x/ HEAD | tar xfz -); done
# Pre-generate and put s_version.h into the tarball (see above; the build
# version is generated using git which can't be done outside the git repo).
	sed 's|^\(#define PD_BUILD_VERSION "\).*"|\1$(PD_BUILD_VERSION)"|' pd/src/s_version.h.in > $(debdist)/pd/src/s_version.h
# Pre-generate the markdown and html docs so that they will be included in the
# source. This means that we don't need any special tools as a build
# dependency, which makes live a lot easier.
	make -C packages/gendoc version="$(PD_L2ORK_VERSION)" build_version="$(PD_BUILD_VERSION)"
	mv packages/gendoc/{ReadMe,Welcome}-*.{md,html} $(debdist)/packages/gendoc
	make -C packages/gendoc clean
# Create the source tarball.
	tar cfz $(debsrc) $(debdist)
	rm -rf $(debdist)
