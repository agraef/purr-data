
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

# Installation prefix under which Pd-l2ork is installed (Linux only). If this
# isn't set, a default location will be used (usually /usr/local). NOTE: We
# *always* assume that this variable is set properly in the install targets
# (see below).
prefix = /usr

ifneq ($(prefix),)
env = inst_dir="$(prefix)"
endif

all:
	cd l2ork_addons && $(env) ./tar_em_up.sh -Tk

incremental:
	cd l2ork_addons && $(env) ./tar_em_up.sh -tk

light:
	cd l2ork_addons && $(env) ./tar_em_up.sh -tkl

%_abs:
	make -C abstractions $(@:%_abs=%_install) DESTDIR=$(firstword $(wildcard $(CURDIR)/packages/*/build)) prefix=$(prefix)

%_ext:
	make -C externals $(@:%_ext=%_install) DESTDIR=$(firstword $(wildcard $(CURDIR)/packages/*/build)) prefix=$(prefix)

checkout:
	git submodule update --init

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
# Pre-generate and put s_stuff.h into the tarball (see above; the build
# version is generated using git which can't be done outside the git repo).
	sed 's|^\(#define PD_BUILD_VERSION "\).*"|\1$(PD_BUILD_VERSION)"|' pd/src/s_stuff.h.in > $(debdist)/pd/src/s_stuff.h
# Create the source tarball.
	tar cfz $(debsrc) $(debdist)
	rm -rf $(debdist)
