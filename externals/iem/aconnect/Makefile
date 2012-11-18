LIBNAME    =aconnect

PREFIX     =/usr/local/lib/pd

INSTALL_BIN=$(PREFIX)/extra

EXT = pd_linux 
DEFS = -DHAVE_ALSA  -DACONNECT_VERSION=\"0.4\"
IFLAGS = -I.  $(INCLUDES)

CC = gcc
LD = ld
STRIP = strip --strip-unneeded

AFLAGS = 
LFLAGS = --export-dynamic  -shared
WFLAGS =

MAKEDEP_FLAGS = -MM

TARNAME =  $(LIBNAME)-0.4.tgz
BINTARNAME =  $(LIBNAME)-0.4-bin.tgz

.SUFFIXES: .$(EXT)

CFLAGS = $(DEFS) $(IFLAGS) $(WFLAGS) -fPIC -g -O2 -mms-bitfields 

LIBS = -lasound -lm -lc 

SOURCES=$(sort $(filter %.c, $(wildcard *.c)))
TARGETS = $(SOURCES:.c=.o)


all: $(LIBNAME)

clean:
	-rm -f *.o *.d

binclean:
	-rm -f *.$(EXT)

distclean: clean binclean
	-rm -f *~ _* config.*
	-rm -rf autom4te.cache

install: install-bin install-doc

install-bin:
	-install -d $(INSTALL_BIN)
	-install -m 644 $(LIBNAME).$(EXT) $(INSTALL_BIN)

install-doc:
	-install -d $(INSTALL_BIN)
	-install -m 644 ../doc/*.pd $(INSTALL_BIN)

dist: distclean
	(cd ..;tar czvf $(TARNAME) $(LIBNAME))

distbin: distclean all clean
	(cd ..; tar cvzf $(BINTARNAME) $(LIBNAME))

everything: clean all install distclean


$(LIBNAME): $(TARGETS)
	$(LD) $(LFLAGS) -o $(LIBNAME).$(EXT) *.o $(LIBS)
	$(STRIP) $(LIBNAME).$(EXT)


## dependencies: as proposed by the GNU-make documentation
## see http://www.gnu.org/software/make/manual/html_node/make_47.html#SEC51
-include $(SOURCES:.c=.d)
%.d: %.c
	@set -e; rm -f $@; \
	 $(CC) $(MAKEDEP_FLAGS) $(CFLAGS) $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$

configure: configure.ac
	autoconf

Makefile: Makefile.in configure
	./configure
