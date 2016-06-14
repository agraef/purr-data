
# Make Debian packages. This builds a package from the current HEAD in a
# subdirectory named $(debdist). It also creates the source archive that goes
# along with it. All files will be created in the toplevel source directory.

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

# The Debian version gets derived from the date and serial number of the last
# commit.
debversion = $(shell git log -1 --format=%cd --date=short | sed -e 's/-//g')+git$(shell git rev-list --count HEAD)
# Debian revision number of the package.
debrevision = 1
# Source tarball and folder.
debsrc = pd-l2ork_$(debversion).orig.tar.gz
debdist = pd-l2ork-$(debversion)

# Submodules (Gem, etc.).
submodules = Gem externals/miXed $(addprefix l2ork_addons/, cwiid fftease3.0-32bit lyonpotpourri rtcmix-in-pd)

# This is used for automatically generated debian/changelog entries (cf. 'make
# debchange'). Adjust as needed.
debmsg = "Build from latest upstream source."
debprio = "low"

.PHONY: debversion debchange debclean deb debsrc deb-us debsrc-us

debversion:
	@echo $(debversion)

debchange:
	dch -u $(debprio) -v $(debversion)-$(debrevision) $(debmsg) && dch -r ""

debclean:
	rm -rf $(debdist)
	rm -f pd-l2ork_*+git*

deb: $(debsrc)
# Unpack the source into our temporary build directory.
	rm -rf $(debdist)
	tar xfz $(debsrc)
# This works around some problems compiling externals on Precise and earlier
# (patch from pd-extended by Hans-Christoph Steiner, modified for pd-l2ork).
# This can be removed once Ubuntu 12.04 finally goes the way of the dodo.
#	cd $(debdist) && patch -p1 < ../precise-configure.patch
# Make sure to copy the Debian files which may have uncommitted changes.
# Then run debuild to create the package.
	cd $(debdist) && cp -R ../debian . && debuild $(DEBUILD_FLAGS)
# Get rid of the temporary build directory.
	rm -rf $(debdist)

debsrc:
	$(MAKE) deb DEBUILD_FLAGS=-S

deb-us:
	$(MAKE) deb DEBUILD_FLAGS="-us -uc"

debsrc-us:
	$(MAKE) deb DEBUILD_FLAGS="-S -us -uc"

$(debsrc):
	rm -rf $(debdist)
# Make sure that the submodules are initialized.
	git -C .. submodule update --init
# Grab the main source.
	git -C .. archive --format=tar.gz --prefix=$(debdist)/ HEAD | tar xfz -
# Grab the submodules.
	for x in $(submodules); do (cd $(debdist) && rm -rf $$x && git -C ../../$$x archive --format=tar.gz --prefix=$$x/ HEAD | tar xfz -); done
# The build script tries to grab this during compilation which doesn't work on
# Launchpad. Why isn't this a submodule?
	git -C $(debdist)/l2ork_addons/raspberry_pi/disis_gpio clone "git://git.drogon.net/wiringPi"
	rm -rf $(debdist)/l2ork_addons/raspberry_pi/disis_gpio/wiringPi/.git
# Create the source tarball.
	tar cfz $(debsrc) $(debdist)
	rm -rf $(debdist)
