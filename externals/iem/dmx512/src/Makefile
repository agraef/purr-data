default: all

include Make.version


Make.config: Make.config.in configure
	./configure

-include Make.config

#VERSIONDEFINE = _$(strip $(VERSION))_
ifneq ($(strip $(VERSION)),)
VERSIONDEFINE = -DDMX4PD_VERSION="\"rev.$(VERSION)\""
endif

.SUFFIXES: .$(EXT)

SOURCES=$(sort $(filter %.c, $(wildcard *.c)))
TARGETS = $(SOURCES:.c=.o)
OBJECTS = $(SOURCES:.c=.$(EXT))

all: $(OBJECTS)

clean:
	-rm -f *.o *.d 

binclean: clean
	-rm -f *.$(EXT) *.dll *.pd_linux *.pd_darwin *.l_i386 *.l_ia64 *.d_ppc *.d_fat 

mrproper: distclean
	-rm Make.config configure *.d.*

distclean: clean binclean
	-rm -f *~ _* config.*
	-rm -rf autom4te.cache

install: install-bin install-doc

install-bin:
	-install -d $(INSTALL_BIN)
	-install -m 644 $(LIBNAME).$(EXT) $(INSTALL_BIN)

install-doc:
	-install -d $(INSTALL_BIN)
	-install -m 644 *.pd $(INSTALL_BIN)

dist: distclean
	(cd ..;tar czvf $(TARNAME) $(LIBNAME))

distbin: distclean all clean
	(cd ..; tar cvzf $(BINTARNAME) $(LIBNAME))

everything: clean all install distclean


$(TARGETS): %.o : %.c Make.config
	$(CC) $(DMX4PD_CFLAGS) $(VERSIONDEFINE) -c -o $@ $*.c

$(OBJECTS): %.$(EXT) : %.o
	$(LD) $(LFLAGS) -o $@ $*.o $(LIBS)
	$(STRIP) $(STRIPFLAGS) $@


ifeq (,$(findstring clean, $(MAKECMDGOALS)))
## dependencies: as proposed by the GNU-make documentation
## see http://www.gnu.org/software/make/manual/html_node/make_47.html#SEC51
-include $(SOURCES:.c=.d)
%.d: %.c
	@set -e; rm -f $@; \
	 $(CC) $(MAKEDEP_FLAGS) $(DMX4PD_CFLAGS) $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$

configure: configure.ac
	autoconf
endif
