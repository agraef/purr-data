# On Mac OS X, this needs to be defined to enable dlopen and weak linking
# support.  Its safe on other platforms since gcc only checks this env var on
# Apple's gcc.  <hans@at.or.at>
ifeq ($(shell uname -s),Darwin)
export MACOSX_DEPLOYMENT_TARGET = 10.3
endif

VPATH = ../obj:./
OBJ_DIR = ../obj
BIN_DIR = ../bin
PDEXEC = $(BIN_DIR)/pd-l2ork
EXT= pd_linux
GUINAME= pd-gui

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
includedir = ${prefix}/include
libdir = ${exec_prefix}/lib
mandir = ${prefix}/share/man

GFLAGS = -DINSTALL_PREFIX=\"$(prefix)\"

# varibles to match packages/Makefile.buildlayout so that they can be easily
# overridden when building Pd-extended builds. <hans@at.or.at>
libpddir = $(libdir)/pd-l2ork
pddocdir = $(libpddir)/doc
libpdbindir = $(libpddir)/bin

CPPFLAGS = -DDL_OPEN -DPA_USE_OSS -DUNIX -DUNISTD        -DUSEAPI_OSS         -fno-strict-aliasing -DPA_USE_ALSA -DUSEAPI_ALSA
MORECFLAGS =  -O6 -funroll-loops -fomit-frame-pointer -DUSEAPI_JACK -D_LARGEFILE64_SOURCE
# if on 10.6/Intel, then build GUI as 32-bit
ifeq ($(shell uname -r | sed 's|\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*|\1|'), 10)
  GINCLUDE = -arch i386 $(CPPFLAGS) -framework Tcl -framework Tk
else
  GINCLUDE = $(CPPFLAGS)  -I/usr/include/tcl8.5
endif
GLIB = -ltk8.5 -ltcl8.5  -lrt -ljack -ljack

LDFLAGS = -Wl,-export-dynamic -lasound -lrt -ljack
LIB =   -ldl -lm -lpthread -lasound

WARN_CFLAGS = -Wall -W -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch
ARCH_CFLAGS = -DPD 

CFLAGS = -g -O2 $(ARCH_CFLAGS) $(WARN_CFLAGS) $(CPPFLAGS) $(MORECFLAGS)

# the sources

SYSSRC += s_midi_oss.c s_audio_oss.c s_audio_alsa.c s_audio_alsamm.c s_midi_alsa.c s_audio_jack.c d_fft_mayer.c d_fftroutine.c

ASIOSRC = 

ASIOOBJ = $(ASIOSRC:.cpp=.o)

# these files cause a warning when using auto-vectorization:
# "warning: dereferencing type-punned pointer will break strict-aliasing rules"
TYPE_PUNNING_SRC = d_ctl.c d_array.c d_delay.c d_filter.c d_math.c d_osc.c d_soundfile.c

# these are safe for full gcc 4.x optimization
OPT_SAFE_SRC = g_canvas.c g_graph.c g_text.c g_rtext.c g_array.c g_template.c g_io.c \
    g_scalar.c g_traversal.c g_guiconnect.c g_readwrite.c g_editor.c \
    g_all_guis.c g_bang.c g_hdial.c g_hslider.c g_mycanvas.c g_numbox.c \
    g_toggle.c g_vdial.c g_vslider.c g_vumeter.c g_magicglass.c \
    m_pd.c m_class.c m_obj.c m_atom.c m_memory.c m_binbuf.c \
    m_conf.c m_glob.c m_sched.c \
    s_main.c s_inter.c s_file.c s_print.c \
    s_loader.c s_path.c s_entry.c s_audio.c s_midi.c \
    d_ugen.c d_arithmetic.c d_dac.c d_misc.c \
    d_fft.c d_global.c \
    d_resample.c \
    x_arithmetic.c x_connective.c x_interface.c x_midi.c x_misc.c \
    x_time.c x_acoustics.c x_net.c x_qlist.c x_gui.c x_list.c \
	import.c \
    $(SYSSRC)

SRC = $(TYPE_PUNNING_SRC) $(OPT_SAFE_SRC)

TYPE_PUNNING_OBJ = $(TYPE_PUNNING_SRC:.c=.o) 
OPT_SAFE_OBJ = $(OPT_SAFE_SRC:.c=.o) 
OBJ = $(SRC:.c=.o) 

GSRC =  t_main.c t_tkcmd.c

GOBJ = $(GSRC:.c=.o)

# get version from m_pd.h to use in doc/1.manual/1.introduction.txt
PD_MAJOR_VERSION := $(shell grep PD_MAJOR_VERSION m_pd.h | \
	sed 's|^.define *PD_MAJOR_VERSION *\([0-9]*\).*|\1|' )
PD_MINOR_VERSION := $(shell grep PD_MINOR_VERSION m_pd.h | \
	sed 's|^.define *PD_MINOR_VERSION *\([0-9]*\).*|\1|' )
PD_BUGFIX_VERSION := $(shell grep PD_BUGFIX_VERSION m_pd.h | \
	sed 's|^.define *PD_BUGFIX_VERSION *\([0-9]*\).*|\1|' )
PD_TEST_VERSION := $(shell grep PD_TEST_VERSION m_pd.h | \
	sed 's|^.define *PD_TEST_VERSION *"\(.*\)".*|\1|' )
PD_VERSION := $(PD_MAJOR_VERSION).$(PD_MINOR_VERSION).$(PD_BUGFIX_VERSION)
ifneq ($(PD_TEST_VERSION),)
	PD_VERSION := $(PD_VERSION)-$(PD_TEST_VERSION)
endif

#
#  ------------------ targets ------------------------------------
#

.PHONY: pd gui externs all

all: pd $(BIN_DIR)/pd-watchdog gui $(BIN_DIR)/pdsend \
    $(BIN_DIR)/pdreceive externs

bin: pd $(BIN_DIR)/pd-watchdog gui $(BIN_DIR)/pdsend \
    $(BIN_DIR)/pdreceive

$(OPT_SAFE_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(OPT_CFLAGS) $(GFLAGS) $(INCLUDE) -c -o $(OBJ_DIR)/$*.o $*.c 

$(TYPE_PUNNING_OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(GFLAGS) $(INCLUDE) -c -o $(OBJ_DIR)/$*.o $*.c 

$(GOBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(GFLAGS) $(GINCLUDE) -c -o $(OBJ_DIR)/$*.o $*.c 

$(ASIOOBJ): %.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $(OBJ_DIR)/$*.o $*.cpp

pd: $(PDEXEC)

ifneq ($(GSRC),)
gui: $(BIN_DIR)/$(GUINAME)
else
gui:
endif

pd-watchdog: $(BIN_DIR)/pd-watchdog

$(BIN_DIR):
	test -d $(BIN_DIR) || mkdir -p $(BIN_DIR)

$(BIN_DIR)/pd-watchdog: s_watchdog.c $(BIN_DIR)
	$(CC) $(CFLAGS) $(STRIPFLAG) -o $(BIN_DIR)/pd-watchdog s_watchdog.c

$(BIN_DIR)/pdsend: u_pdsend.c $(BIN_DIR)
	$(CC) $(CFLAGS)  $(STRIPFLAG) -o $(BIN_DIR)/pdsend u_pdsend.c

$(BIN_DIR)/pdreceive: u_pdreceive.c $(BIN_DIR)
	$(CC) $(CFLAGS)  $(STRIPFLAG) -o $(BIN_DIR)/pdreceive u_pdreceive.c

$(PDEXEC): $(OBJ) $(BIN_DIR)
	cd ../obj;  $(CC) $(LDFLAGS) $(DBG_CFLAGS) -o $(PDEXEC) $(OBJ) $(LIB)

$(BIN_DIR)/pd-gui: $(GOBJ) $(GSRC)
	cd ../obj; $(CC) $(INCLUDE) -o $(BIN_DIR)/$(GUINAME) $(GOBJ) $(GLIB)

# if on 10.6/Intel, then force build GUI as 32-bit
ifeq ($(shell uname -r | sed 's|\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*|\1|'), 10)
  ARCH_FLAG = -arch i386
else
  ARCH_FLAG = 
endif
#this is for Max OSX only...
$(BIN_DIR)/libPdTcl.dylib: $(GOBJ) $(GSRC)
	cd ../obj && $(CC) $(ARCH_FLAG) $(CFLAGS) -dynamiclib -read_only_relocs warning  \
		-o $(BIN_DIR)/libPdTcl.dylib $(GOBJ)  \
		-F \
		-framework Tcl  -framework Tk  -framework System  \
		-Wl,-install_name,@executable_path/../Resources/bin/libPdTcl.dylib
	install_name_tool -change /Tcl.framework/Versions/8.4/Tcl\
		 @executable_path/../Frameworks/Tcl.framework/Versions/8.4/Tcl \
		 -change /Tk.framework/Versions/8.4/Tk \
		 @executable_path/../Frameworks/Tk.framework/Versions/8.4/Tk \
		../bin/libPdTcl.dylib

# this is for Windows/MinGW (only?)
$(BIN_DIR)/pdtcl.dll: $(GOBJ)
	cd $(BIN_DIR); dllwrap --export-all-symbols --output-def pdtcl.def \
	--output-lib=pdtcl.a --dllname=$(GUINAME) $(OBJ_DIR)/t_tkcmd.o $(LIB) $(GLIB)
	strip --strip-unneeded $(BIN_DIR)/pdtcl.dll

externs: 
	make -C ../extra/bonk~    
	make -C ../extra/choice   
	make -C ../extra/expr~    
	make -C ../extra/fiddle~  
	make -C ../extra/loop~    
	make -C ../extra/lrshift~ 
	make -C ../extra/pique    
	make -C ../extra/sigmund~ 
	make -C ../extra/pd~      
	make -C ../extra/stdout   

BINARYMODE=-m755

ABOUT_FILE=$(DESTDIR)$(pddocdir)/1.manual/1.introduction.txt
install:  all
	install -d $(DESTDIR)$(libpdbindir)
	install $(BIN_DIR)/$(GUINAME) $(DESTDIR)$(libpdbindir)/$(GUINAME)
	install $(BIN_DIR)/pd-watchdog $(DESTDIR)$(libpdbindir)/pd-watchdog
	install -m644 pd.tk $(DESTDIR)$(libpdbindir)/pd.tk
	install -m644 pkgIndex.tcl $(DESTDIR)$(libpdbindir)/pkgIndex.tcl
	install -m644 helpbrowser.tcl $(DESTDIR)$(libpdbindir)/helpbrowser.tcl
	install -d $(DESTDIR)$(bindir)
	install $(BINARYMODE) $(PDEXEC) $(DESTDIR)$(bindir)/pd-l2ork
# kludge to allow pd~ to work by default in pd-l2ork
	rm -f $(DESTDIR)$(libpddir)/pd
	ln -s $(bindir)/pd-l2ork $(DESTDIR)$(libpddir)/pd
	install -m755 $(BIN_DIR)/pdsend $(DESTDIR)$(bindir)/pdsend
	install -m755 $(BIN_DIR)/pdreceive $(DESTDIR)$(bindir)/pdreceive 
	for dir in $(shell ls -1 ../doc | grep -v CVS); do \
		echo "installing $$dir"; \
		install -d $(DESTDIR)$(pddocdir)/$$dir ; \
		install -m644 -p ../doc/$$dir/*.* $(DESTDIR)$(pddocdir)/$$dir ; \
	done
	for dir in $(shell ls -1 ../doc/7.stuff | grep -v CVS); do \
		echo "installing 7.stuff/$$dir"; \
		install -d $(DESTDIR)$(pddocdir)/7.stuff/$$dir ; \
		install -m644 -p ../doc/7.stuff/$$dir/*.* \
                    $(DESTDIR)$(pddocdir)/7.stuff/$$dir ; \
	done
	mv $(ABOUT_FILE) $(ABOUT_FILE).tmp
	cat $(ABOUT_FILE).tmp | sed 's|PD_VERSION|Pd version $(PD_VERSION)|' \
		> $(ABOUT_FILE)
	rm $(ABOUT_FILE).tmp
	rsync -ax --exclude=.git --exclude=.svn ../extra $(DESTDIR)$(libpddir)/
	install -d $(DESTDIR)$(includedir)/pdl2ork
	install -m644 m_pd.h $(DESTDIR)$(includedir)/pdl2ork/m_pd.h
	install -m644 m_imp.h $(DESTDIR)$(includedir)/pdl2ork/m_imp.h
	install -m644 g_canvas.h $(DESTDIR)$(includedir)/pdl2ork/g_canvas.h
	install -m644 s_stuff.h $(DESTDIR)$(includedir)/pdl2ork/s_stuff.h
	install -m644 g_all_guis.h $(DESTDIR)$(includedir)/pdl2ork/g_all_guis.h
	install -d $(DESTDIR)$(mandir)/man1
	gzip < ../man/pd.1 >  $(DESTDIR)$(mandir)/man1/pd-l2ork.1.gz
	chmod 644 $(DESTDIR)$(mandir)/man1/pd-l2ork.1.gz
	gzip < ../man/pdsend.1 >  $(DESTDIR)$(mandir)/man1/pdsend.1.gz
	chmod 644 $(DESTDIR)$(mandir)/man1/pdsend.1.gz
	gzip < ../man/pdreceive.1 >  $(DESTDIR)$(mandir)/man1/pdreceive.1.gz
	chmod 644 $(DESTDIR)$(mandir)/man1/pdreceive.1.gz
	@echo "Pd install succeeded."

local-clean:	
	-rm -f -- $(OBJ)
	-rm -f ../obj/* $(PDEXEC) $(BIN_DIR)/$(GUINAME) $(BIN_DIR)/pdsend \
	    $(BIN_DIR)/pdreceive $(BIN_DIR)/pd-watchdog m_stamp.c
	-rm -f -- *~
	-(cd ../doc/6.externs; rm -f *.pd_linux)
	-rm -f makefile.dependencies
	touch makefile.dependencies
	chmod 666 makefile.dependencies

extra-clean:
	-rm -f `find ../extra/ -name "*.pd_*"`
	-rm -f tags

clean: extra-clean local-clean

distclean: clean
	-rm -f config.cache config.log config.status makefile tags \
		autom4te.cache/output.* autom4te.cache/traces.* autom4te.cache/requests
	-rmdir autom4te.cache
	-rm -rf autom4te-*.cache

tags: $(SRC) $(GSRC); ctags *.[ch]

depend: makefile.dependencies

makefile.dependencies: makefile
	$(CC) $(CPPFLAGS) -M $(SRC) > makefile.dependencies

uninstall:
	rm -f -r $(DESTDIR)$(libpddir)
	rm -f $(DESTDIR)$(bindir)/pd-l2ork
	rm -f $(DESTDIR)$(bindir)/pdsend
	rm -f $(DESTDIR)$(bindir)/pdreceive
	rm -f $(DESTDIR)$(includedir)/m_pd.h
	rm -f $(DESTDIR)$(includedir)/m_imp.h
	rm -f $(DESTDIR)$(includedir)/s_stuff.h
	rm -f $(DESTDIR)$(includedir)/g_all_guis.h
	rm -f $(DESTDIR)$(includedir)/g_canvas.
	rm -f $(DESTDIR)$(mandir)/man1/pd.1.gz
	rm -f $(DESTDIR)$(mandir)/man1/pdsend.1.gz
	rm -f $(DESTDIR)$(mandir)/man1/pdreceive.1.gz

include makefile.dependencies


TAGS: etags

etags:
	etags *.h $(SRC) $(SYSSRC)
	etags --append --language=none --regex="/proc[ \t]+\([^ \t]+\)/\1/" pd.tk *.tcl
	find /usr/include -type f -name \*.h -exec etags -a '{}' \;
	make etags_`uname -s`

etags_Darwin:
	find /System/Library/Frameworks  -type f -name \*.h -exec etags -a '{}' \;
	find /Library/Frameworks  -type f -name \*.h -exec etags -a '{}' \;
	find /sw/include -type f -name \*.h -exec etags -a '{}' \;

etags_Linux:

etags_MINGW:
	find /usr/local/include/ -type f -name \*.h -exec etags -a '{}' \;










