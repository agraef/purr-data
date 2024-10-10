

### Makefile.pdlibbuilder ###

Helper makefile for Pure Data external libraries. Written by Katja Vetter
March-June 2015 for the public domain and since then developed as a Pd
community project. No warranties. Inspired by Hans Christoph Steiner's Makefile
Template and Stephan Beal's ShakeNMake.

GNU make version >= 3.81 required.


### characteristics ###


* defines build settings based on autodetected target platform
* defines rules to build Pd class- or lib executables from C or C++ sources
* defines rules for libdir installation
* defines convenience targets for developer and user
* evaluates implicit dependencies for non-clean builds


### basic usage ###


In your Makefile, define your Pd lib name and class files, and include
Makefile.pdlibbuilder at the end of the Makefile. Like so:


      # Makefile for mylib

      lib.name = mylib

      class.sources = myclass1.c myclass2.c

      datafiles = myclass1-help.pd myclass2-help.pd README.txt LICENSE.txt

      include Makefile.pdlibbuilder


For files in class.sources it is assumed that class name == source file
basename. The default target builds all classes as individual executables
with Pd's default extension for the platform. For anything more than the
most basic usage, read the documentation sections in Makefile.pdlibbuilder.


### paths ###


Makefile.pdlibbuilder >= v0.4.0 supports pd path variables which can be
defined not only as make command argument but also in the environment, to
override platform-dependent defaults:

PDDIR:
Root directory of 'portable' pd package. When defined, PDINCLUDEDIR and
PDBINDIR will be evaluated as $(PDDIR)/src and $(PDDIR)/bin.

PDINCLUDEDIR:
Directory where Pd API m_pd.h should be found, and other Pd header files.
Overrides the default search path.

PDBINDIR:
Directory where pd.dll should be found for linking (Windows only). Overrides
the default search path.

PDLIBDIR:
Root directory for installation of Pd library directories. Overrides the
default install location.


### documentation ###


This README.md provides only basic information. A large comment section inside
Makefile.pdlibbuilder lists and explains the available user variables, default
paths, and targets. The internal documentation reflects the exact functionality
of the particular version. For suggestions about project maintenance and
advanced compilation see tips-tricks.md.


### versioning ###


The project is versioned in MAJOR.MINOR.BUGFIX format (see http://semver.org),
and maintained at https://github.com/pure-data/pd-lib-builder. Pd lib developers
are invited to regulary check for updates, and to contribute and discuss
improvements here. If you really need to distribute a personalized version with
your library, rename Makefile.pdlibbuilder to avoid confusion.


### examples ###

The list of projects using pd-lib-builder can be helpful if you are looking for
examples, from the simplest use case to more complex implementations.

- helloworld: traditional illustration of simplest use case
- pd-windowing: straightforward real world use case of a small library
- pd-nilwind / pd-cyclone: more elaborate source tree
- zexy: migrated from autotools to pd-lib-builder


### projects using pd-lib-builder ###

non-exhaustive list

https://github.com/pure-data/helloworld

https://github.com/electrickery/pd-nilwind

https://github.com/electrickery/pd-maxlib

https://github.com/electrickery/pd-sigpack

https://github.com/electrickery/pd-tof

https://github.com/electrickery/pd-windowing

https://github.com/electrickery/pd-smlib

https://github.com/porres/pd-cyclone

https://github.com/porres/pd-else

https://github.com/porres/pd-psycho

https://git.iem.at/pd/comport

https://git.iem.at/pd/hexloader

https://git.iem.at/pd/iemgui

https://git.iem.at/pd/iemguts

https://git.iem.at/pd/iemlib

https://git.iem.at/pd/iemnet

https://git.iem.at/pd/iem_ambi

https://git.iem.at/pd/iem_tab

https://git.iem.at/pd/iem_adaptfilt

https://git.iem.at/pd/iem_roomsim

https://git.iem.at/pd/iem_spec2

https://git.iem.at/pd/mediasettings

https://git.iem.at/pd/zexy

https://git.iem.at/pd-gui/punish

https://github.com/residuum/PuRestJson

https://github.com/libpd/abl_link

https://github.com/wbrent/timbreID

https://github.com/MetaluNet/moonlib


