#!/usr/bin/make

DEBUG?=0
OS := $(shell uname -s)

ifeq ($(DEBUG),1)
	CFLAGS += -O0 -g -ggdb -DDEBUG
endif
ifeq ($(OS),Linux)
  PDSUF = .pd_linux
  PDBUNDLEFLAGS = -shared -rdynamic
  LDSOFLAGS = -lm
endif
ifeq ($(OS),Darwin)
  PDSUF = .pd_darwin
  INCLUDES = -I/Library/Frameworks/Tcl.framework/Headers
  PDBUNDLEFLAGS = -bundle -flat_namespace -undefined dynamic_lookup
  LDSOFLAGS = -lm
endif
ifeq (MINGW,$(findstring MINGW,$(UNAME)))
  PDSUF = .dll
  PDBUNDLEFLAGS = -shared
  LDSOFLAGS = -lm
endif

INCLUDES =  -I../../pd/src -I/usr/include
CFLAGS += -Wall -fPIC
CFLAGS += -DPDSUF=\"$(PDSUF)\"
ifeq ($(DEBUG),0)
	CFLAGS += -O2
endif

all: atoi.pd_linux atof.pd_linux

clean:
	rm -f *.pd_linux *.o *~ atoi.c atof.c

atoi.c: atox.c
	sed -e s/ATOX/$*/g $< > $@

atof.c: atox.c
	sed -e s/ATOX/$*/g $< > $@

.SUFFIXES: .c .o $(PDSUF)

.o$(PDSUF):
	$(CC) $(PDBUNDLEFLAGS) $(CFLAGS) $(INCLUDES) -o $*$(PDSUF) $< $(LDSOFLAGS)
