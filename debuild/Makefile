
# The Pd-L2Ork version number used in this build. XXXFIXME: Is there a way to
# grab this from the source somewhere?
version = 0.42.6

# Make Debian packages. This builds a package from the current HEAD in a
# subdirectory named $(debdist). It also creates the source archive that goes
# along with it. All files will be created in the toplevel source directory.

# To make this work, you need to have the Debian package toolchain (debuild
# and friends) installed. Also make sure you have your DEBEMAIL and
# DEBFULLNAME environment variables set up as explained in the debchange(1)
# manual page. These are needed to create changelog entries and in order to
# sign the Debian packages created with 'make deb' and 'make debsrc'.

# The typical workflow is as follows:

# 1. Run 'make debchange' once to create a new debian/changelog entry. You
# *must* do this once so that debuild knows about the proper version number of
# the package.

# 2. Run 'make deb' to build a signed binary package. Or 'make deb-us' for an
# unsigned one.

# If you only need the binary package for local deployment then you're done.
# Otherwise proceed to step 3.

# 3. Run 'make debsrc' to create a signed Debian source package which can be
# uploaded, e.g, to Launchpad using 'dput'. Or 'make debsrc-us' for an
# unsigned package.

# 4. Run 'make debclean' to get rid of any files that were created in steps 2
# and 3.

# The Debian version gets derived from the package version $(version) as well
# as the date and serial number of the last commit.
debversion = $(version)+git$(shell git log -1 --format=%cd --date=short | sed -e 's/-//g')+$(shell git rev-list --count HEAD)
# Debian revision number of the package.
debrevision = 1
# Source tarball and folder.
debsrc = pd-l2ork_$(debversion).orig.tar.gz
debdist = pd-l2ork-$(debversion)

# Secondary sources (Gem, wiringPi).
gemdebsrc = pd-l2ork_$(debversion).orig-Gem.tar.gz
pidebsrc = pd-l2ork_$(debversion).orig-wiringPi.tar.gz

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
	rm -f pd-l2ork_$(version)+git* pd-l2ork-*.tar.bz2

deb: $(debsrc) $(gemdebsrc) $(pidebsrc)
# Unpack the primary source into our temporary build directory.
	rm -rf $(debdist)
	tar xfz $(debsrc)
# This works around some problems compiling externals on Precise and earlier
# (patch from pd-extended by Hans-Christoph Steiner, modified for pd-l2ork).
# This can be removed once Ubuntu 12.04 finally goes the way of the dodo.
	cd $(debdist) && patch -p1 < ../precise-configure.patch
# Unpack the secondary sources.
	cd $(debdist) && rm -Rf Gem && tar xfz ../$(gemdebsrc)
	cd $(debdist) && rm -Rf wiringPi && tar xfz ../$(pidebsrc)
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
	git -C .. archive --format=tar.gz -o $(CURDIR)/$(debsrc) --prefix=$(debdist)/ HEAD

$(gemdebsrc): ../Gem/ChangeLog
# Make sure that the Gem submodule is updated.
	git -C .. submodule update
	git -C ../Gem archive --format=tar.gz -o $(CURDIR)/$(gemdebsrc) --prefix=Gem/ HEAD

../Gem/ChangeLog:
# Make sure that the Gem submodule is initialized.
	git -C .. submodule init

$(pidebsrc): wiringPi/INSTALL
	git -C wiringPi pull
	git -C wiringPi archive --format=tar.gz -o $(CURDIR)/$(pidebsrc) --prefix=wiringPi/ HEAD

wiringPi/INSTALL:
	git clone git://git.drogon.net/wiringPi
