default: all

.PHONEY: default all everything dist \
	clean realclean distclean \
	install install-bin install-doc install-abs \
	tests

LIBDIR=../..

TESTDIR=$(LIBDIR)/tests
SOURCEDIR=$(LIBDIR)/src

HELPERSOURCES=$(SOURCEDIR)/z_zexy.c $(SOURCEDIR)/zexy.c $(SOURCEDIR)/winNT_portio.c

OBJECTSOURCES=$(sort $(filter-out $(HELPERSOURCES), $(filter %.c, $(wildcard $(SOURCEDIR)/*.c))))

SOURCES=$(OBJECTSOURCES) $(HELPERSOURCES)


VPATH = $(SOURCEDIR)

-include $(SOURCES:.c=.d)

-include Make.config

##  2nd only generate depend-files when we have Make.config included
##  and thus MAKEDEP_FLAGS defined
ifdef MAKEDEP_FLAGS
## dependencies: as proposed by the GNU-make documentation
## see http://www.gnu.org/software/make/manual/html_node/make_47.html#SEC51
%.d: %.c
	@set -e; rm -f $@; \
	 $(CPP) $(MAKEDEP_FLAGS) $(Z_CFLAGS) $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$
endif

.SUFFIXES: .$(EXT)

TARGETS = $(SOURCES:.c=.o)

OBJECTS = $(OBJECTSOURCES:.c=.$(EXT))

## if $(BUILDLIBRARY) is defined, we build everything as a single library
## else we build separate externals
ifneq "$(BUILDLIBRARY)" ""
all: $(LIBNAME)
	cp $(LIBNAME).$(EXT) $(LIBDIR)
else
all: $(OBJECTS)
endif

$(OBJECTS): %.$(EXT) : %.o
	$(CC) $(LDFLAGS) -o $@ $*.o $(LIBS)
	$(STRIP) $(STRIPFLAGS) $@

$(LIBNAME): $(TARGETS) z_zexy.c z_zexy.h
	$(CC) $(LDFLAGS) -o $@.$(EXT) $(SOURCEDIR)/*.o $(LIBS)
	$(STRIP) $(STRIPFLAGS) $@.$(EXT)

$(TARGETS): %.o : %.c
	$(CC) $(Z_CFLAGS) -c -o $@ $*.c

externals: $(OBJECTS)

clean:
	-rm -f $(SOURCEDIR)/*.$(EXT) $(SOURCEDIR)/*.o 

realclean: clean alias-clean
	-rm -f *~ config.*
	-rm -f $(SOURCEDIR)/*.d $(SOURCEDIR)/*.d.* $(SOURCEDIR)/*~

distclean: realclean
	-rm -f Make.config $(SOURCEDIR)/zexyconf.h $(LIBDIR)/*.$(EXT)
	-rm -rf autom4te.cache aclocal.m4

tests:  all
	make -C $(TESTDIR)

alias: alias-bin alias-abs

alias-bin: all
	$(SOURCEDIR)/makealias.sh $(SOURCEDIR)/alias $(SOURCEDIR) $(LIBDIR)/reference

alias-abs:
	$(SOURCEDIR)/makealias.sh $(SOURCEDIR)/alias $(LIBDIR)/abs

alias-clean:  alias-clean-bin alias-clean-abs

alias-clean-bin:
	$(SOURCEDIR)/makealias.sh -clean $(SOURCEDIR)/alias $(SOURCEDIR) $(LIBDIR)/reference

alias-clean-abs:
	$(SOURCEDIR)/makealias.sh -clean $(SOURCEDIR)/alias $(LIBDIR)/abs

install: all alias install-abs install-bin install-doc

install-bin:
	-install -d $(INSTALL_BIN)
	-install -m 644 $(LIBDIR)$(LIBNAME).$(EXT) $(INSTALL_BIN)

install-doc:
	-install -d $(INSTALL_DOC)
	-install -m 644 $(LIBDIR)/reference/*.pd $(INSTALL_DOC)

install-abs:
	-install -d $(INSTALL_BIN)
	-install -m 644 $(LIBDIR)/abs/*.pd $(INSTALL_BIN)


uninstall-bin:
	-install -d $(INSTALL_BIN)
	-install -m 644 $(LIBNAME).$(EXT) $(INSTALL_BIN)

uninstall-doc:
	-install -d $(INSTALL_DOC)
	-install -m 644 $(LIBDIR)/reference/*.pd $(INSTALL_DOC)

uninstall-abs:
	-install -d $(INSTALL_BIN)
	-install -m 644 $(LIBDIR)/abs/*.pd $(INSTALL_BIN)


dist: all realclean
	(cd $(LIBDIR)/..;tar czvf $(TARNAME) $(LIBNAME))

everything: clean all install distclean

