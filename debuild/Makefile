# This is a GNU Makefile. Special tools needed: GNU make and awk, git.

# Make Debian packages. This builds a package from the current HEAD in a
# subdirectory named $(debdist). It also creates the source archive that goes
# along with it. All files will be created in the debuild directory.

# To make this work, you need to have the Debian package toolchain (debuild
# and friends) installed. You should also set the DEBEMAIL environment (or
# make) variable to your full name and email address as explained in the
# debchange(1) manual page, e.g.: DEBEMAIL="John Doe <johndoe@mail.com>"
# This information is used to create changelog entries with 'make debchange',
# which in turn are used to sign the Debian packages created with 'make deb'
# and 'make debsrc'. (Otherwise some generic information supplied by the
# system will be used, and you may not be able to sign the package.)

# The typical workflow is as follows:

# 1. Run 'make debchange' once to create a new debian/changelog entry. You
# *must* do this once after changes in the source repository so that debuild
# knows about the proper version number of the package.

# 2. Run 'make deb' to build a signed binary package. Or 'make deb-us' for an
# unsigned one.

# If you only need the binary package for local deployment then you're done.
# Otherwise proceed to step 3.

# 3. Run 'make debsrc' to create a signed Debian source package which can be
# uploaded, e.g, to Launchpad using 'dput'. Or 'make debsrc-us' for an
# unsigned package.

# 4. Run 'make debclean' to get rid of any files that were created in steps 2
# and 3.

# Determine the version number of this build. We get this from m_pd.h.
PD_L2ORK_VERSION := $(shell grep PD_L2ORK_VERSION ../pd/src/m_pd.h | sed 's|^.define *PD_L2ORK_VERSION *"\(.*\)".*|\1|')
# Determine the build version which needs git to be computed, so we can't do
# it in a stand-alone build from a tarball
PD_BUILD_VERSION := $(shell git log -1 --format=%cd --date=short | sed -e 's/-//g')-rev.$(shell git rev-parse --short HEAD)

# The Debian version gets derived from the date and serial number of the last
# commit. You can print it with 'make debversion'.
debversion = $(PD_L2ORK_VERSION)+git$(shell git rev-list --count HEAD)+$(shell git rev-parse --short HEAD)
# Debian revision number of the package.
debrevision = 1
# Source tarball and folder.
debsrc = purr-data_$(debversion).orig.tar.gz
debdist = purr-data-$(debversion)

# nw.js sdk
# NOTE: arm packages need some special-casing, since they are released
# separately, and the version numbers do not always match up.
nwjsver = 0.55.0
nwjspkg = nwjs-sdk-v$(nwjsver)-linux
nwjsver_arm = 0.27.6
nwjspkg_arm = nwjs-sdk-v$(nwjsver_arm)-linux
nwjsver_arm64 = 0.23.7
nwjspkg_arm64 = nwjs-sdk-without-nacl-v$(nwjsver_arm64)-linux
nwjs = $(nwjspkg)-ia32.tar.gz $(nwjspkg)-x64.tar.gz $(nwjspkg_arm)-arm.tar.gz $(nwjspkg_arm64)-arm64.tar.gz

# Submodules (Gem, etc.). You can list them with 'make submodules'.
#submodules = Gem externals/miXed $(addprefix l2ork_addons/, cwiid fftease3.0-32bit lyonpotpourri rtcmix-in-pd)
submodules = $(sort $(shell git -C .. config --file .gitmodules --get-regexp path | awk '{ print $$2 }') externals/pd-lua/lua)

# This is used for automatically generated debian/changelog entries (cf. 'make
# debchange'). Adjust as needed.
debmsg = "Build from latest upstream source."
debprio = "low"

.PHONY: submodules debversion debchange debclean deb debsrc deb-us debsrc-us

submodules:
	@echo $(submodules)

debversion:
	@echo $(debversion)

debchange:
	dch -u $(debprio) -v $(debversion)-$(debrevision) $(debmsg) && dch -r ""

debclean:
	rm -rf $(debdist)
	rm -f purr-data_*+git* purr-data-dbgsym_*+git*

deb: $(debsrc) $(addprefix debian/nwjs/, $(nwjs))
# Unpack the source into our temporary build directory.
	rm -rf $(debdist)
	tar xfz $(debsrc)
# Add the NW.js binaries to the source package.
	cd debian/source && rm -f include-binaries && for x in $(addprefix debian/nwjs/, $(nwjs)); do echo "$$x" >> include-binaries; done
# Make sure to copy the Debian files which may have uncommitted changes.
# Then run debuild to create the package.
	cd $(debdist) && cp -R ../debian . && debuild $(DEBUILD_FLAGS)
# Get rid of the temporary build directory.
	rm -rf $(debdist)

debsrc:
	$(MAKE) deb DEBUILD_FLAGS="-S -d"

deb-us:
	$(MAKE) deb DEBUILD_FLAGS="-us -uc"

debsrc-us:
	$(MAKE) deb DEBUILD_FLAGS="-S -d -us -uc"

$(debsrc):
	rm -rf $(debdist)
# Make sure that the submodules are initialized.
	git -C .. submodule update --init --recursive
# Grab the main source.
	git -C .. archive --format=tar.gz --prefix=$(debdist)/ HEAD | tar xfz -
# Grab the submodules.
	for x in $(submodules); do (cd $(debdist) && rm -rf $$x && git -C ../../$$x archive --format=tar.gz --prefix=$$x/ HEAD | tar xfz -); done
# Pre-generate and put s_version.h into the tarball (see above; the build
# version is generated using git which can't be done outside the git repo).
	sed 's|^\(#define PD_BUILD_VERSION "\).*"|\1$(PD_BUILD_VERSION)"|' ../pd/src/s_version.h.in > $(debdist)/pd/src/s_version.h
# Pre-generate the markdown and html docs so that they will be included in the
# source. This means that we don't need any special tools as a build
# dependency, which makes live a lot easier.
	make -C ../packages/gendoc version="$(PD_L2ORK_VERSION)" build_version="$(PD_BUILD_VERSION)"
	mv ../packages/gendoc/*-Linux* $(debdist)/packages/gendoc
	make -C ../packages/gendoc clean
	ls -l $(debdist)/packages/gendoc/*.html
# Create the source tarball.
	tar cfz $(debsrc) $(debdist)
	rm -rf $(debdist)

debian/nwjs/$(nwjspkg_arm)-arm.tar.gz:
	mkdir -p debian/nwjs &&	cd debian/nwjs && wget https://github.com/LeonardLaszlo/nw.js-armv7-binaries/releases/download/v$(nwjsver_arm)/$(notdir $@)

debian/nwjs/$(nwjspkg_arm64)-arm64.tar.gz:
	mkdir -p debian/nwjs &&	cd debian/nwjs && wget https://github.com/LeonardLaszlo/nw.js-armv7-binaries/releases/download/v$(nwjsver_arm64)/$(notdir $@)

debian/nwjs/$(nwjspkg)-%.tar.gz:
	mkdir -p debian/nwjs &&	cd debian/nwjs && wget http://dl.nwjs.io/v$(nwjsver)/$(notdir $@)
