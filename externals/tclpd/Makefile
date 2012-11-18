#!/usr/bin/make

DEBUG?=0
OS := $(shell uname -s)
TCL_VERSION := $(shell echo 'puts $$tcl_version' | tclsh)

ifeq ($(DEBUG),1)
	CFLAGS += -O0 -g -ggdb -DDEBUG
endif
ifeq ($(OS),Linux)
  PDSUF = .pd_linux
  PDBUNDLEFLAGS = -shared -rdynamic
  LDSOFLAGS = -lm -ltcl$(TCL_VERSION)
endif
ifeq ($(OS),Darwin)
  PDSUF = .pd_darwin
  INCLUDES = -I/Library/Frameworks/Tcl.framework/Headers
  PDBUNDLEFLAGS = -bundle -flat_namespace -undefined dynamic_lookup
  LDSOFLAGS = -lm -framework Tcl
endif
ifeq (MINGW,$(findstring MINGW,$(UNAME)))
  PDSUF = .dll
  PDBUNDLEFLAGS = -shared
  LDSOFLAGS = -lm -ltcl$(TCL_VERSION)
endif

LIBNAME = tcl
INCLUDES =  -I../../pd/src -I/usr/include -I/usr/include/tcl$(TCL_VERSION)
CFLAGS += -funroll-loops -fno-operator-names -fno-omit-frame-pointer -falign-functions=16 -Wall -fPIC
CFLAGS += -DPDSUF=\"$(PDSUF)\"
ifeq ($(DEBUG),0)
	CFLAGS += -O2
endif
LDSHARED = $(CXX) $(PDBUNDLEFLAGS)

all:: $(LIBNAME)$(PDSUF)
	@echo '-----------------------------------------------------------------------------'
	@echo ' $(LIBNAME)$(PDSUF) ('`test $(DEBUG) -eq 1 && echo debug || echo release`' build) '\
		'[size: '`ls -gGh $(LIBNAME)$(PDSUF) | cut -d " " -f 3`']'

clean::
	rm -f tcl.pd_linux tcl_wrap.cxx *.o *~

.SUFFIXES: .cxx .o

SRCS = tcl_wrap.cxx tcl_typemap.cxx tcl_class.cxx tcl_widgetbehavior.cxx tcl_proxyinlet.cxx tcl_setup.cxx tcl_loader.cxx
OBJS = ${SRCS:.cxx=.o}

tcl_wrap.cxx:: tcl.i tcl_extras.h Makefile
	swig -v -c++ -tcl -o tcl_wrap.cxx $(INCLUDES) tcl.i

.cxx.o:: tcl_extras.h Makefile
	$(CXX) $(CFLAGS) $(INCLUDES) -xc++ -c $<

$(LIBNAME)$(PDSUF):: tcl_extras.h Makefile $(OBJS)
	$(LDSHARED) $(CFLAGS) -o $(LIBNAME)$(PDSUF) $(OBJS) $(LDSOFLAGS)

pdlib_tcl_syntax: pdlib.tcl
	@echo -n "checking pdlib.tcl for syntax..."
	@tclsh pdlib.tcl >/dev/null 2>&1
	@echo " OK"
