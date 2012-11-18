## Makefile.am -- Process this file with automake to produce Makefile.in
##

AUTOMAKE_OPTIONS = foreign
#AUTOMAKE_OPTIONS = no-dependencies foreign
ACLOCAL_AMFLAGS  = -I m4/generated -I m4
AM_CPPFLAGS	 = -I$(top_srcdir)/../../../pd/src

externaldir = $(libdir)/pd/extra/wiimote

external_LTLIBRARIES = wiimote.la

SOURCES = wiimote.c
dist_external_DATA = wiimote-help.pd

wiimote_la_SOURCES = $(SOURCES)
wiimote_la_CFLAGS = @ARCH_CFLAGS@ @PD_CFLAGS@
wiimote_la_LIBADD = $(LIBM)
wiimote_la_LDFLAGS = -module -avoid-version -shared @ARCH_LDFLAGS@ @PD_LDFLAGS@


libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
