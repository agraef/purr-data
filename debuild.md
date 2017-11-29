# Purr Data -- Linux Installation from Source on Ubuntu

The recommended way of building Purr Data packages on Ubuntu its derivatives
is via the debuild system in the debuild folder of the source code. This
folder contains a Makefile which automates most of the process and creates a
deb package with all the proper dependencies. Both signed and unsigned
packages can be created that way, either binary packages, ready to be
installed on your system, or source packages which can be uploaded to a PPA
(personal package archive) on Launchpad, if you have an account there. This is
also the way that the JGU Ubuntu packages are built,
cf. <https://l2orkubuntu.bitbucket.io/>.

## Prerequisites

You need to have the Debian package toolchain (debuild and friends) installed.
The following command, executed on an Ubuntu system, will give you all the
things that you need (the gnupg package is only required if you plan to sign
your packages, more on that in section "Preparations" below):

    sudo apt install gnupg ubuntu-dev-tools

## Quick Start

Long story short, here's how you create an unsigned binary package for your
own use:

    git clone https://git.purrdata.net/jwilkes/purr-data.git
	cd purr-data/debuild
	make debchange
	make deb-us

This will leave the deb package and some accompanying files in the debuild
folder. To clean up after a build:

    make debclean

For more details on what the above commands do, please check the Makefile or
the "Detailed Guide" section below.

## Detailed Guide

In order to create source packages and signed binary packages you'll have to
jump through some extra hoops, which are explained in some detail in the
following.

### Preparations

You should set the `DEBEMAIL` environment (or make) variable to your full name
and email address as explained in the debchange(1) manual page, e.g.:
`DEBEMAIL="John Doe <johndoe@mail.com>"`. This information is used to create
changelog entries with `make debchange`, which in turn are used to sign the
Debian packages created with `make deb` and `make debsrc`. (Otherwise some
generic information supplied by the system will be used, and you may not be
able to sign the package.)

If you want to actually sign your packages (which is necessary if you want to
upload them to Launchpad), you'll also have to create a GPG signature for the
name and email address you set in `DEBEMAIL`. E.g.:

    gpg --gen-key
	gpg --send-keys --keyserver keyserver.ubuntu.com <key-id>

Here `<key-id>` is the id of the key created with `gpg --gen-key`. (If you
want to upload anything securely to Launchpad, you'll also have to create and
upload an ssh key, please check the
[Getting Set Up](http://packaging.ubuntu.com/html/getting-set-up.html)
section in the Ubuntu Packaging Guide for details.)

### Preparing for the Build

To create any kind of package, you'll have to clone Purr Data's git repository
(or pull the current revision, if you already have a working copy). Then chdir
to the debuild folder in the sources and run `make` with various different
targets there. The first step is always the same:

**Step 1.** Run `make debchange` once to create a new debian/changelog
entry. You *must* do this once every time there are changes in the source
repository, so that debuild knows about the proper version number of the
package.

Also note that you *must* do this in a working copy of Purr Data's git
repository, otherwise the debuild Makefile has no way of knowing what revision
of the software you're building.

### Creating a Binary Package

The following step creates a binary package:

**Step 2.** Run `make deb` to build a signed binary package.

Note that the signing requires some special preparations (see "Preparations"
above). At the end of the build, debuild will prompt you to sign the package
by entering the passphrase for your GPG key. (If all you need is a deb package
for local deployment, then just use `make deb-us` to create an unsigned
package instead.)

This does all the necessary steps to grab the sources and set them up as
required in a separate staging directory (so that your working copy of the
sources remain untarnished), runs the `tar_em_up.sh` build tool to compile the
sources and then runs debuild to create the package. When finished, you can
find the resulting deb package along with a few other related files in the
debuild folder (the temporary build directory will be removed automatically
after the build is finished).

If you only need the binary package then you're done. Otherwise proceed to
the next section.

### Creating a Source Package

In order to create a source package:

**Step 3.** Run `make debsrc` to create a signed Debian source package.

You only need to do this in order to create a Debian source package which can
be uploaded to a site like Launchpad using `dput`, please check the Ubuntu
Packaging Guide for details.

You can also create an unsigned source package with `make debsrc-us`, but this
is generally not very useful since the sole purpose of source packages is to
upload to package repositories which generally will only accept signed
packages (at least that's the case on Launchpad).

Note that if you just need the source package, there's no need to build a
binary package first, as described in step 2, unless you first want to check
that the package builds locally.

### Cleaning Up

Finally, you may want to do this to remove any build products in the debuild
folder:

**Step 4.** Run 'make debclean' to get rid of any files that were created in
steps 2 and 3.

Note that this will *also* delete the created packages, so you may want to
move these elsewhere if they're still needed.

## Further Information

For general information about Ubuntu packaging please consult the Ubuntu
Packaging Guide at <http://packaging.ubuntu.com/>.
