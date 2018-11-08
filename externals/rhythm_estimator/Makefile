# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = .
prefix = /usr
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/rhythm_estimator
pkglibdir = $(libdir)/rhythm_estimator
pkgincludedir = $(includedir)/rhythm_estimator

top_builddir = .

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = x86_64-unknown-linux-gnu
host_triplet = 
AS = @AS@
CC = gcc
DLLTOOL = @DLLTOOL@
LD = /usr/bin/x86_64-linux-gnu-ld
LIBTOOL = $(SHELL) $(top_builddir)/libtoolkludge
LN_S = @LN_S@
MAINT = #
MAKEINFO = makeinfo
NM = @NM@
OBJDUMP = @OBJDUMP@
PACKAGE = rhythm_estimator
PDDIR = /usr/lib/pd
RANLIB = @RANLIB@
VERSION = 0.1

SUFFIXES = .pd_linux

pddir = /usr/lib/pd

pdinstalldir = $(pddir)/extra/rhythm_estimator

# Automake won't accept something ending in ".pd_linux" as a library
pdinstall_PROGRAMS =  	rhythm_ioi_histogram.pd_linux 	rhythm_quantum.pd_linux 	rhythm_slave_metro.pd_linux


rhythm_ioi_histogram_pd_linux_SOURCES =  	pd_rhythm_estimator.c 	pd_rhythm_estimator.h 	pd_rhythm_ioi_histogram.c 	pd_rhythm_ioi_histogram.h 	rhythm_estimator.h 	rhythm_ioi_histogram.c 	rhythm_ioi_histogram.h


rhythm_quantum_pd_linux_SOURCES =  	pd_rhythm_estimator.c 	pd_rhythm_estimator.h 	pd_rhythm_quantum.c 	pd_rhythm_quantum.h 	rhythm_estimator.h 	rhythm_quantum.c 	rhythm_quantum.h


rhythm_slave_metro_pd_linux_SOURCES =  	pd_rhythm_estimator.c 	pd_rhythm_estimator.h 	pd_rhythm_slave_metro.c 	pd_rhythm_slave_metro.h 	rhythm_estimator.h 	rhythm_slave_metro.c 	rhythm_slave_metro.h


pdincludedir = $(pddir)/src
INCLUDES = -I$(top_srcdir) -I$(pdincludedir)
CFLAGS = -DPD -O2 -fPIC -funroll-loops -fomit-frame-pointer     -Wall -W -Wshadow -Wstrict-prototypes -Werror     -Wno-unused -Wno-parentheses -Wno-switch

LDFLAGS += -Wl,--export-dynamic  -shared -fPIC

pdinstall_DATA =  	AUTHORS 	COPYING 	NEWS 	README 	TODO 	bang-eater.pd 	cama99.ps 	click1.wav 	click2.wav 	drumboy~.pd 	drunk-metro.pd 	ping~.pd 	rhythm_estimator.pd 	rhythm_estimator_example.pd 	rhythm_estimator_example2.pd


noinst_SCRIPTS = libtoolkludge

EXTRA_DIST = $(pdinstall_DATA) $(noinst_SCRIPTS)
ACLOCAL_M4 = $(top_srcdir)/aclocal.m4
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_CLEAN_FILES =  version.h
PROGRAMS =  $(pdinstall_PROGRAMS)


DEFS =  -DPACKAGE=\"rhythm_estimator\" -DVERSION=\"0.1\" -DHAVE_LIBC=1 -DHAVE_LIBM=1 -DSTDC_HEADERS=1 -DHAVE_STRING_H=1  -I. -I$(srcdir) 
CPPFLAGS = 
LIBS = -lm -lc 
rhythm_ioi_histogram_pd_linux_OBJECTS =  pd_rhythm_estimator.o \
pd_rhythm_ioi_histogram.o rhythm_ioi_histogram.o
rhythm_ioi_histogram_pd_linux_LDADD = $(LDADD)
rhythm_ioi_histogram_pd_linux_DEPENDENCIES = 
rhythm_ioi_histogram_pd_linux_LDFLAGS = 
rhythm_quantum_pd_linux_OBJECTS =  pd_rhythm_estimator.o \
pd_rhythm_quantum.o rhythm_quantum.o
rhythm_quantum_pd_linux_LDADD = $(LDADD)
rhythm_quantum_pd_linux_DEPENDENCIES = 
rhythm_quantum_pd_linux_LDFLAGS = 
rhythm_slave_metro_pd_linux_OBJECTS =  pd_rhythm_estimator.o \
pd_rhythm_slave_metro.o rhythm_slave_metro.o
rhythm_slave_metro_pd_linux_LDADD = $(LDADD)
rhythm_slave_metro_pd_linux_DEPENDENCIES = 
rhythm_slave_metro_pd_linux_LDFLAGS = 
SCRIPTS =  $(noinst_SCRIPTS)

COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
DATA =  $(pdinstall_DATA)

DIST_COMMON =  README AUTHORS COPYING ChangeLog INSTALL Makefile.am \
Makefile.in NEWS TODO aclocal.m4 config.guess config.sub configure \
configure.in install-sh missing mkinstalldirs version.h.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = gtar
GZIP_ENV = --best
SOURCES = $(rhythm_ioi_histogram_pd_linux_SOURCES) $(rhythm_quantum_pd_linux_SOURCES) $(rhythm_slave_metro_pd_linux_SOURCES)
OBJECTS = $(rhythm_ioi_histogram_pd_linux_OBJECTS) $(rhythm_quantum_pd_linux_OBJECTS) $(rhythm_slave_metro_pd_linux_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .o .pd_linux .s
$(srcdir)/Makefile.in: # Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu --include-deps Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$@ CONFIG_HEADERS= $(SHELL) ./config.status

$(ACLOCAL_M4): # configure.in 
	cd $(srcdir) && $(ACLOCAL)

config.status: $(srcdir)/configure $(CONFIG_STATUS_DEPENDENCIES)
	$(SHELL) ./config.status --recheck
$(srcdir)/configure: #$(srcdir)/configure.in $(ACLOCAL_M4) $(CONFIGURE_DEPENDENCIES)
	cd $(srcdir) && $(AUTOCONF)
version.h: $(top_builddir)/config.status version.h.in
	cd $(top_builddir) && CONFIG_FILES=$@ CONFIG_HEADERS= $(SHELL) ./config.status

mostlyclean-pdinstallPROGRAMS:

clean-pdinstallPROGRAMS:
	-test -z "$(pdinstall_PROGRAMS)" || rm -f $(pdinstall_PROGRAMS)

distclean-pdinstallPROGRAMS:

maintainer-clean-pdinstallPROGRAMS:

install-pdinstallPROGRAMS: $(pdinstall_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(pdinstalldir)
	@list='$(pdinstall_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(pdinstalldir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(pdinstalldir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done

uninstall-pdinstallPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(pdinstall_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(pdinstalldir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done

.c.o:
	$(COMPILE) -c $<

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

rhythm_ioi_histogram.pd_linux: $(rhythm_ioi_histogram_pd_linux_OBJECTS) $(rhythm_ioi_histogram_pd_linux_DEPENDENCIES)
	@rm -f rhythm_ioi_histogram.pd_linux
	$(LINK) $(rhythm_ioi_histogram_pd_linux_LDFLAGS) $(rhythm_ioi_histogram_pd_linux_OBJECTS) $(rhythm_ioi_histogram_pd_linux_LDADD) $(LIBS)

rhythm_quantum.pd_linux: $(rhythm_quantum_pd_linux_OBJECTS) $(rhythm_quantum_pd_linux_DEPENDENCIES)
	@rm -f rhythm_quantum.pd_linux
	$(LINK) $(rhythm_quantum_pd_linux_LDFLAGS) $(rhythm_quantum_pd_linux_OBJECTS) $(rhythm_quantum_pd_linux_LDADD) $(LIBS)

rhythm_slave_metro.pd_linux: $(rhythm_slave_metro_pd_linux_OBJECTS) $(rhythm_slave_metro_pd_linux_DEPENDENCIES)
	@rm -f rhythm_slave_metro.pd_linux
	$(LINK) $(rhythm_slave_metro_pd_linux_LDFLAGS) $(rhythm_slave_metro_pd_linux_OBJECTS) $(rhythm_slave_metro_pd_linux_LDADD) $(LIBS)

install-pdinstallDATA: $(pdinstall_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(pdinstalldir)
	@list='$(pdinstall_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(pdinstalldir)/$$p"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(pdinstalldir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(pdinstalldir)/$$p"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(pdinstalldir)/$$p; \
	  fi; fi; \
	done

uninstall-pdinstallDATA:
	@$(NORMAL_UNINSTALL)
	list='$(pdinstall_DATA)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(pdinstalldir)/$$p; \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(PACKAGE)-$(VERSION)
top_distdir = $(distdir)

# This target untars the dist file and tries a VPATH configuration.  Then
# it guarantees that the distribution is self-contained by making another
# tarfile.
distcheck: dist
	-rm -rf $(distdir)
	GZIP=$(GZIP_ENV) $(TAR) zxf $(distdir).tar.gz
	mkdir $(distdir)/=build
	mkdir $(distdir)/=inst
	dc_install_base=`cd $(distdir)/=inst && pwd`; \
	cd $(distdir)/=build \
	  && ../configure --srcdir=.. --prefix=$$dc_install_base \
	  && $(MAKE) $(AM_MAKEFLAGS) \
	  && $(MAKE) $(AM_MAKEFLAGS) dvi \
	  && $(MAKE) $(AM_MAKEFLAGS) check \
	  && $(MAKE) $(AM_MAKEFLAGS) install \
	  && $(MAKE) $(AM_MAKEFLAGS) installcheck \
	  && $(MAKE) $(AM_MAKEFLAGS) dist
	-rm -rf $(distdir)
	@banner="$(distdir).tar.gz is ready for distribution"; \
	dashes=`echo "$$banner" | sed s/./=/g`; \
	echo "$$dashes"; \
	echo "$$banner"; \
	echo "$$dashes"
dist: distdir
	-chmod -R a+r $(distdir)
	GZIP=$(GZIP_ENV) $(TAR) chozf $(distdir).tar.gz $(distdir)
	-rm -rf $(distdir)
dist-all: distdir
	-chmod -R a+r $(distdir)
	GZIP=$(GZIP_ENV) $(TAR) chozf $(distdir).tar.gz $(distdir)
	-rm -rf $(distdir)
distdir: $(DISTFILES)
	-rm -rf $(distdir)
	mkdir $(distdir)
	-chmod 777 $(distdir)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
pd_rhythm_estimator.o: pd_rhythm_estimator.c
pd_rhythm_ioi_histogram.o: pd_rhythm_ioi_histogram.c \
	pd_rhythm_estimator.h pd_rhythm_ioi_histogram.h \
	rhythm_ioi_histogram.h rhythm_estimator.h
pd_rhythm_quantum.o: pd_rhythm_quantum.c pd_rhythm_estimator.h \
	pd_rhythm_quantum.h rhythm_quantum.h rhythm_estimator.h
pd_rhythm_slave_metro.o: pd_rhythm_slave_metro.c pd_rhythm_estimator.h \
	rhythm_estimator.h pd_rhythm_slave_metro.h rhythm_slave_metro.h
rhythm_ioi_histogram.o: rhythm_ioi_histogram.c rhythm_ioi_histogram.h \
	rhythm_estimator.h
rhythm_quantum.o: rhythm_quantum.c rhythm_quantum.h rhythm_estimator.h
rhythm_slave_metro.o: rhythm_slave_metro.c rhythm_estimator.h \
	rhythm_slave_metro.h

info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am:
install-exec: install-exec-am

install-data-am: install-pdinstallPROGRAMS install-pdinstallDATA
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am: uninstall-pdinstallPROGRAMS uninstall-pdinstallDATA
uninstall: uninstall-am
all-am: Makefile $(PROGRAMS) $(SCRIPTS) $(DATA)
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(pdinstalldir) $(DESTDIR)$(pdinstalldir)


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-pdinstallPROGRAMS mostlyclean-compile \
		mostlyclean-tags mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-pdinstallPROGRAMS clean-compile clean-tags \
		clean-generic mostlyclean-am

clean: clean-am

distclean-am:  distclean-pdinstallPROGRAMS distclean-compile \
		distclean-tags distclean-generic clean-am

distclean: distclean-am
	-rm -f config.status

maintainer-clean-am:  maintainer-clean-pdinstallPROGRAMS \
		maintainer-clean-compile maintainer-clean-tags \
		maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am
	-rm -f config.status

.PHONY: mostlyclean-pdinstallPROGRAMS distclean-pdinstallPROGRAMS \
clean-pdinstallPROGRAMS maintainer-clean-pdinstallPROGRAMS \
uninstall-pdinstallPROGRAMS install-pdinstallPROGRAMS \
mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile uninstall-pdinstallDATA install-pdinstallDATA \
tags mostlyclean-tags distclean-tags clean-tags maintainer-clean-tags \
distdir info-am info dvi-am dvi check check-am installcheck-am \
installcheck install-exec-am install-exec install-data-am install-data \
install-am install uninstall-am uninstall all-redirect all-am all \
installdirs mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
