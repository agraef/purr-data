CC=gcc

EXT=$(shell uname -s | sed -e 's/^[lL]/pd_l/' | sed -e 's/^[dD]/pd_d/' | sed -e 's/MINGW.*/dll/')

# This is Miller's default install location
INSTALL_PREFIX=/usr/local/lib/pd

# find all files to compile
TARGETS=$(subst .tk,.tk2c,$(wildcard */*.tk)) $(subst .c,.$(EXT),$(wildcard */*.c))

current: $(TARGETS)
	@echo "Making $(TARGETS)"

.SUFFIXES: .pd_linux .pd_darwin .pd_irix5 .pd_irix6 .dll .tk .tk2c

# ----------------------- Common ------------------------

.tk.tk2c:
	bash ./tk2c.bash < $*.tk > $*.tk2c


# ----------------------- MinGW -----------------------

pd_nt: $(TARGETS)

MINGW_CFLAGS = -DPD -DUNIX -DICECAST -DWANT_WINSOCK2_H \
		-O2 -funroll-loops -fomit-frame-pointer \
		-Wall -W -Wno-shadow -Wstrict-prototypes -g \
		-Wno-unused -Wno-parentheses -Wno-switch \
		-mno-cygwin -c -DBUILD_DLL
MINGW_CFLAGS = -DPD -DNT -DICECAST -funroll-loops \
    -Wall -W -Wno-shadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

MINGW_INCLUDE =  -I../../src -I../../pd/src -IC:/msys/1.0/include


MINGW_LDFLAGS = -shared -L../../pd/bin -LC:/msys/1.0/lib \
		 -lpd -logg -lvorbis -lspeex -lpthreadGC2


.c.dll:
	$(CC) -mms-bitfields $(MINGW_CFLAGS) $(DEFINES) $(MINGW_INCLUDE) \
		 -o "$*.o" -c "$*.c"
	gcc $(MINGW_LDFLAGS) -o "$*.dll" "$*.o"
	strip --strip-unneeded "$*.dll"
	rm "$*.o"


#.c.dll: CURRENT_DIR = `echo $* | cut -d '/' -f 1`
#	$(CC) $(MINGW_CFLAGS) $(MINGW_INCLUDE) -o $*.o -c $*.c
#	ld --export-dynamic  -shared -o $*.dll $*.o -lc -lm $(MINGW_LFLAGS)
#	strip --strip-unneeded $*.dll
#	dllwrap --target=i386-mingw32 -mno-cygwin --output-lib=lib$*.a \
#				--dllname=$*.dll --driver-name=gcc $*.o $(MINGW_LFLAGS)
#	-rm $*.o


# ----------------------- Windows -----------------------

# PDNTCFLAGS = /W3 /WX /DNT /DPD /nologo
# VC="C:\Program Files\Microsoft Visual Studio\Vc98"

# PDNTINCLUDE = /I. /I\tcl\include /I\ftp\pd\src /I$(VC)\include

# PDNTLDIR = $(VC)\lib
# PDNTLIB = $(PDNTLDIR)\libc.lib \
# 	$(PDNTLDIR)\oldnames.lib \
# 	$(PDNTLDIR)\kernel32.lib \
# 	\ftp\pd\bin\pd.lib 

# .c.ont:
# 	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c

# .c.dll:
# 	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
# need to find a way to replace $(CSYM)
#	link /dll /export:$(CSYM)_setup $*.obj $(PDNTLIB)
#=======
#	$(CC) $(WINCFLAGS) $(WININCLUDE) -o $*.o -c $*.c
#	gcc -shared -o $*.dll $*.o ../../bin/pd.dll -lwsock32 -lmp3lame -lpthread
#	-rm $*.o
#>>>>>>> 1.10


# ----------------------- IRIX 5.x -----------------------

pd_irix5: $(NAME).pd_irix5 

SGICFLAGS5 = -o32 -DPD -DUNIX -DIRIX -O2

SGIINCLUDE =  -I../../src

.c.oi5:
	$(CC) $(SGICFLAGS5) $(SGIINCLUDE) -o $*.o -c $*.c

.c.pd_irix5:
	$(CC) $(SGICFLAGS5) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -elf -shared -rdata_shared -o $*.pd_irix5 $*.o
	rm $*.o

# ----------------------- IRIX 6.x -----------------------

pd_irix6: $(NAME).pd_irix6

SGICFLAGS6 = -n32 -DPD -DUNIX -DIRIX -DN32 -woff 1080,1064,1185 \
	-OPT:roundoff=3 -OPT:IEEE_arithmetic=3 -OPT:cray_ivdep=true \
	-Ofast=ip32

.c.oi6:
	$(CC) $(SGICFLAGS6) $(SGIINCLUDE) -o $*.o -c $*.c

.c.pd_irix6:
	$(CC) $(SGICFLAGS6) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -n32 -IPA -shared -rdata_shared -o $*.pd_irix6 $*.o
	rm $*.o

# ----------------------- LINUX i386 -----------------------

pd_linux: $(TARGETS)

LINUXCFLAGS = -DPD -DUNIX -DICECAST -O2 -funroll-loops -fomit-frame-pointer \
    -fPIC -Wall -W -Wno-shadow -Wstrict-prototypes -g \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXINCLUDE =  -I../../src -I../../pd/src

.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	ld --export-dynamic  -shared -o $*.pd_linux $*.o -lc -lm
	strip --strip-unneeded $*.pd_linux
	-rm $*.o

# ----------------------- Mac OSX -----------------------

pd_darwin: $(TARGETS)

# I added these defines since Darwin doesn't have this signals.  I do
# not know whether the objects will work, but they will compile.
#     -DMSG_NOSIGNAL=0 -DSOL_TCP=0
# I got this from here: http://www.holwegner.com/forum/viewtopic.php?t=4
# <hans@eds.org>
DARWINCFLAGS = -DPD -DUNIX \
	-O2 -Wall -W -Wshadow -Wstrict-prototypes -Wno-unused -Wno-parentheses \
	-Wno-switch

##  if you point the linker to the pd binary, then it can check the symbols,
## and therefore allow for a two-level namespace, and that makes Darwin happy
## but this doesn't work for some of the files yet.
DARWINLINKFLAGS = -bundle -bundle_loader ../../pd/bin/pd
DARWINLIBS = -L/sw/lib -logg -lvorbis -lmp3lame -lspeex
#DARWINLINKFLAGS = -bundle -undefined suppress -flat_namespace

DARWININCLUDE =  -I../../src -I../../pd/src -I/sw/include

.c.pd_darwin:
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -o $*.o -c $*.c
	$(CC) $(DARWINLINKFLAGS) -o $*.pd_darwin $*.o $(DARWINLIBS)
	chmod a-x "$*.pd_darwin"
	strip -u -r $*.pd_darwin
	-rm $*.o

# added by Hans-Christoph Steiner <hans@eds.org>
# to generate MacOS X packages

PACKAGE_PREFIX = pd-unauthorized
PACKAGE_VERSION = $(shell date +20%y.%m.%d)
PACKAGE_NAME = $(PACKAGE_PREFIX)-$(PACKAGE_VERSION)

darwin_pkg_license:
  # generate HTML version of License
	echo "<HTML><BODY><FONT SIZE="-1">" > License.html
	sed -e 's/^$$/\<P\>/g' COPYING >> License.html	
	echo "</FONT></BODY></HTML>" >> License.html

darwin_pkg_clean:
	-sudo rm -Rf installroot/ $(PACKAGE_PREFIX)*.pkg/
	-rm -f $(PACKAGE_PREFIX)-*.info 1 License.html

# install into MSP's default: /usr/local/lib

darwin_pkg: pd_darwin darwin_pkg_clean darwin_pkg_license
# set up installroot dir
	test -d installroot/pd/doc/5.reference || mkdir -p installroot/pd/doc/5.reference
	install -m644 --group=staff */*.pd installroot/pd/doc/5.reference
	install -m644 --group=staff */*.txt installroot/pd/doc/5.reference
	install -m644 --group=staff */*.pls installroot/pd/doc/5.reference
	cp -Rf blinkenlights/blm  installroot/pd/doc/5.reference
	test -d installroot/pd/extra || mkdir -p installroot/pd/extra
	install -m644  --group=staff */*.pd_darwin installroot/pd/extra
	cp -f pd-unauthorized.info $(PACKAGE_NAME).info
# delete cruft
	-find installroot -name .DS_Store -delete
	-rm -f 1
# set proper permissions
	sudo chown -R root:staff installroot
	package installroot $(PACKAGE_NAME).info -d . -ignoreDSStore
# install pkg docs
	install -m 644 License.html $(PACKAGE_NAME).pkg/Contents/Resources
	sudo chown -R root:staff $(PACKAGE_NAME).pkg/Contents/Resources


# ----------------------------------------------------------

install:
	cp */*.pd $(INSTALL_PREFIX)/doc/5.reference

clean:
# delete emacs backup files
	-rm -f */*.?~ */*.~?.?.~
# delete compile products
	-rm -f *.o *.pd_* *.dll *.tk2c core so_locations ChangeLog
	-rm -f */*.?~ */*.o */*.pd_* */*.dll */*.tk2c */.*.swp */core so_locations
# delete autoconf/automake product
	-rm -Rf */autom4te.cache

