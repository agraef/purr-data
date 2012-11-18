## Process this file with automake to produce Makefile.in. -*-Makefile-*-

SUFFIXES = .pd_linux

pddir = @PDDIR@

## Make and install the shared libraries.

pdinstalldir = $(pddir)/extra/rhythm_estimator

# Automake won't accept something ending in ".pd_linux" as a library
pdinstall_PROGRAMS = \
	rhythm_ioi_histogram.pd_linux \
	rhythm_quantum.pd_linux \
	rhythm_slave_metro.pd_linux

rhythm_ioi_histogram_pd_linux_SOURCES = \
	pd_rhythm_estimator.c \
	pd_rhythm_estimator.h \
	pd_rhythm_ioi_histogram.c \
	pd_rhythm_ioi_histogram.h \
	rhythm_estimator.h \
	rhythm_ioi_histogram.c \
	rhythm_ioi_histogram.h

rhythm_quantum_pd_linux_SOURCES = \
	pd_rhythm_estimator.c \
	pd_rhythm_estimator.h \
	pd_rhythm_quantum.c \
	pd_rhythm_quantum.h \
	rhythm_estimator.h \
	rhythm_quantum.c \
	rhythm_quantum.h

rhythm_slave_metro_pd_linux_SOURCES = \
	pd_rhythm_estimator.c \
	pd_rhythm_estimator.h \
	pd_rhythm_slave_metro.c \
	pd_rhythm_slave_metro.h \
	rhythm_estimator.h \
	rhythm_slave_metro.c \
	rhythm_slave_metro.h

pdincludedir = $(pddir)/src
INCLUDES = -I$(top_srcdir) -I$(pdincludedir)
CFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch
LDFLAGS = --export-dynamic -shared

## Install the documentation.

pdinstall_DATA = \
	AUTHORS \
	COPYING \
	NEWS \
	README \
	TODO \
	bang-eater.pd \
	cama99.ps \
	click1.wav \
	click2.wav \
	drumboy~.pd \
	drunk-metro.pd \
	ping~.pd \
	rhythm_estimator.pd \
	rhythm_estimator_example.pd \
	rhythm_estimator_example2.pd

## My kludge
noinst_SCRIPTS = libtoolkludge

EXTRA_DIST = $(pdinstall_DATA) $(noinst_SCRIPTS)
