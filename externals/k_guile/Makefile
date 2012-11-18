NAME=k_guile
CSYM=k_guile

DIR=k_guile

current: pd_linux

# ----------------------- NT -----------------------

pd_nt: $(NAME).dll

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /DNT /DPD /nologo /DINCLUDEPATH=\"c:\\pd\"
VC="C:\Programme\Microsoft Visual Studio\Vc98"

PDNTINCLUDE = /I. /I\tcl\include /Ic:\pd\src /I$(VC)\include

PDNTLDIR = $(VC)\lib
PDNTLIB = $(PDNTLDIR)\libc.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	c:\pd\bin\pd.lib 

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c k_guile_win.c
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:$(CSYM)_setup $*.obj k_guile_win.obj $(PDNTLIB)

# ----------------------- IRIX 5.x -----------------------

pd_irix5: $(NAME).pd_irix5

.SUFFIXES: .pd_irix5

SGICFLAGS5 = -o32 -DPD -DUNIX -DIRIX -O2

SGIINCLUDE =  -I../../src

.c.pd_irix5:
	cc $(SGICFLAGS5) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -elf -shared -rdata_shared -o $*.pd_irix5 $*.o
	rm $*.o

# ----------------------- IRIX 6.x -----------------------

pd_irix6: $(NAME).pd_irix6

.SUFFIXES: .pd_irix6

SGICFLAGS6 = -n32 -DPD -DUNIX -DIRIX -DN32 -woff 1080,1064,1185 \
	-OPT:roundoff=3 -OPT:IEEE_arithmetic=3 -OPT:cray_ivdep=true \
	-Ofast=ip32

.c.pd_irix6:
	cc $(SGICFLAGS6) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -n32 -IPA -shared -rdata_shared -o $*.pd_irix6 $*.o
	rm $*.o

# ----------------------- LINUX i386 -----------------------

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -DUNIX -DICECAST -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wno-shadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch #-Werror

LINUXINCLUDEPATH=../../src
LINUXINCLUDE =  -I$(LINUXINCLUDEPATH)

$(NAME).pd_linux: $(NAME).o
	ld --export-dynamic  -shared -o $(NAME).pd_linux k_guile.o -lc -lm -lguile
	strip --strip-unneeded $*.pd_linux
	rm -f $*.o ../$*.pd_linux
	ln -s $(DIR)/$*.pd_linux ..

$(NAME).o: $(NAME).c global_scm.txt local_scm.txt
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -o $(NAME).o -c $(NAME).c


# ----------------------- Mac OSX -----------------------

pd_darwin: $(NAME).pd_darwin k_guile.c

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

.c.pd_darwin: global_scm.txt local_scm.txt
	cc $(DARWINCFLAGS) $(LINUXINCLUDE) -DINCLUDEPATH=\""`pwd`"\" -DLINUXINCLUDE=\""$(LINUXINCLUDEPATH)"\ -o $*.o -c k_guile.c
	cc -bundle -undefined suppress  -flat_namespace -o $*.pd_darwin $*.o 
	rm -f $*.o ../$*.pd_darwin
	ln -s $*/$*.pd_darwin ..

# ----------------------------------------------------------


global_scm.txt: global.scm gen_c_scheme.py
	./gen_c_scheme.py global.scm >global_scm.txt

local_scm.txt: local.scm gen_c_scheme.py
	./gen_c_scheme.py local.scm >local_scm.txt


install:
	cp help-*.pd ../../doc/5.reference

clean:
	rm -f *.o *.pd_* so_locations *~ core global_scm.txt local_scm.txt


