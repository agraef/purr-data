AUTOMAKE_OPTIONS = foreign dist-bzip2 no-dist-gzip

EXEEXT = .@EXTENSION@

noinst_PROGRAMS = hidio
hidio_SOURCES = hidio.c hidio_types.c
LDADD = 

noinst_HEADERS = hidio.h input_arrays.h

EXTRA_SUBDIRS = 

if MACOSX
hidio_SOURCES += hidio_darwin.c
EXTRA_SUBDIRS += HID_Utilities_Source
LDADD += HID_Utilities_Source/libHIDUtilities.a
CFLAGS += -IHID_Utilities_Source
LDFLAGS += -bundle -undefined dynamic_lookup \
	-framework Carbon -framework IOKit -framework ForceFeedback
endif

if LINUX
hidio_SOURCES += hidio_linux.c input_arrays.c
CFLAGS += -fPIC
LDFLAGS += -Wl,--export-dynamic  -shared -fPIC
endif

if WINDOWS
hidio_SOURCES += hidio_windows.c
LDADD += -L$(PD_PATH)/src -L$(PD_PATH)/bin -L$(PD_PATH)/obj -lpd
CFLAGS += -mms-bitfields
LDFLAGS += -s -shared
endif

# files that are included but not built
EXTRA_DIST = COPYING README TODO hidio-help.pd

# subdirs that are built
SUBDIRS = $(EXTRA_SUBDIRS)

# subdirs that are always included in the dist, etc.
DIST_SUBDIRS = HID_Utilities_Source

dist-hook:
	mkdir $(distdir)/examples
	cp -p $(srcdir)/examples/*.pd $(distdir)/examples
	mkdir $(distdir)/test
	cp -p $(srcdir)/test/*.pd $(distdir)/test
