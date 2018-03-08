
# Toplevel Makefile for Purr Data. Please note that at present this is just a
# thin wrapper around l2ork_addons/tar_em_up.sh, the traditional Pd-l2ork
# build script.

# The Pd-l2ork build system is very arcane and intricate. Its main purpose is
# building installers for the supported platforms, compiling sources is just a
# byproduct. Therefore `make` will most likely rebuild lots of things even if
# you just finished another build.

# The available build targets are:

# all: produce a native installer for the host platform (equivalent to
# `tar_em_up.sh -Tk`); note that in order to force a complete rebuild (like
# what `tar_em_up.sh -T` does), you'll have to run `make clean` first

# incremental: like `all`, but does an "incremental build" (equivalent to
# `tar_em_up.sh -tk`), bypassing Gem which takes an eternity to compile; please
# check the tar_em_up.sh script for details

# checkout: convenience target to check out all submodules in preparation for
# a subsequent build (the `all` and `rebuild` targets also do this
# automatically when needed)

# clean: does something similar to what `tar_em_up.sh` does in order to start
# from a clean slate, so that a subsequent build starts from scratch again

# realclean: put the sources into pristine state again (WARNING: this will get
# rid of any uncommitted source changes, too); use this as a last resort to
# get the sources into a compilable state again after things have gone awry

all:
	cd l2ork_addons && ./tar_em_up.sh -Tk

incremental:
	cd l2ork_addons && ./tar_em_up.sh -tk

checkout:
	git submodule update --init

clean:
	test $os == "osx" && make -C packages/darwin_app clean || true
	cd pd/src && aclocal && autoconf && make clean || true
	cd externals/miXed && make clean || true
	cd Gem/src/ && test -f Makefile && make distclean || true
	cd Gem/src/ && rm -rf ./.libs && rm -rf ./*/.libs
	cd Gem/ && test -f Makefile && make distclean || true
	cd Gem/ && rm -f gemglutwindow.pd_linux Gem.pd_linux

realclean:
	git submodule deinit --all -f
	git checkout .
	git clean -dff
# git clean doesn't see these, but we need to get rid of them to prevent
# subsequent mysterious build failures
	rm -rf pd/lib $(addprefix externals/disis/, flext/configure stk/configure)
