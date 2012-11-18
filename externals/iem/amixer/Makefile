default: all

include Make.version

Make.config: Make.config.in configure
	./configure

include Make.config

.SUFFIXES: .$(EXT)

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
	-install -m 644 *.pd $(INSTALL_BIN)

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
