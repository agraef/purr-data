#
# Copyright (C) 2004 Hans-Christoph Steiner
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


EXT=o
LIBS = -L../../../pd/bin -lc -lm
LIBOSC = ../libOSC/libOSC.a

CFLAGS = -g -O2 -DUNIX -Wall -Wimplicit -Wunused -Wmissing-prototypes -O1 -fPIC -I../libOSC -I../../pd/src -I../../../pd/src -I../src
INCLUDES = -I../libOSC -I../../pd/src -I../../../pd/src -I../src -I/usr/include/tirpc
LDFLAGS = -Wl,--export-dynamic -shared

prefix=/usr/local

SOURCES = OSC-pattern-match.c OSC.c dumpOSC.c htmsocket.c OSCroute.c sendOSC.c OSC-system-dependent.c
TARGETS = $(SOURCES:.c=.o)
EXTS=sendOSC.pd_linux dumpOSC.pd_linux OSCroute.pd_linux

# ----------------------- autoconf -----------------------

all: $(EXTS)

lib: OSC.pd_linux

dumpOSC.pd_linux: dumpOSC.o
	$(CC) $(LDFLAGS) -o dumpOSC.pd_linux $? $(LIBS)

sendOSC.pd_linux: sendOSC.o htmsocket.o OSC-system-dependent.o
	$(CC) $(LDFLAGS) -o sendOSC.pd_linux $? $(LIBS) $(LIBOSC)

OSCroute.pd_linux: OSCroute.o OSC-pattern-match.o OSC-system-dependent.o
	$(CC) $(LDFLAGS) -o OSCroute.pd_linux $? $(LIBS)

OSC.pd_linux: OSC.o sendOSC.o dumpOSC.o OSCroute.o htmsocket.o OSC-pattern-match.o OSC-system-dependent.o
	$(CC) $(LDFLAGS) -o OSC.pd_linux $? $(LIBS) $(LIBOSC)

$(TARGETS): %.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $*.o $*.c

# ----------------------------------------------------------

install-doc:
	@test -d $(prefix)/lib/pd/doc/5.reference || mkdir -p $(prefix)/lib/pd/doc/5.reference
	cp -r ../doc/* $(prefix)/lib/pd/doc/5.reference/

install: install-doc
	@test -d $(prefix)/lib/pd/extra || mkdir -p $(prefix)/lib/pd/extra
	install -m644 *.pd_linux $(prefix)/lib/pd/extra

clean:
	rm -rf *.$(EXT) *.pd_linux
