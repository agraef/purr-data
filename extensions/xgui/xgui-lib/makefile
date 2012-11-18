current:
	echo make pd_linux, pd_nt, pd_irix5, or pd_irix6

clean: ; rm -f *.pd_linux *.o *.obj *.bak *.dll

# ----------------------- NT -----------------------------

pd_nt: number.dll concat.dll oname2l.dll \
       a2s.dll recv2.dll send2.dll replace_selector.dll

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /DNT /DPD /nologo
VC="C:\Program Files\Microsoft Visual Studio\Vc98"

PDNTINCLUDE = /I. /I\tcl\include /I\pds\pd35-9\src /I$(VC)\include

PDNTLDIR = $(VC)\lib
PDNTLIB = $(PDNTLDIR)\libc.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	\pds\pd35-9\bin\pd.lib

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:$*_setup $*.obj $(PDNTLIB)
	del $*.obj
	del $*.exp
	del $*.lib
	move /Y $*.dll ../../extra/

# ----------------------- IRIX 5.x -----------------------

pd_irix5: number.pd_irix5 concat.pd_irix5 oname2l.pd_irix5 \
       a2s.pd_irix5 recv2.pd_irix5 send2.pd_irix5 replace_selector.pd_irix5

.SUFFIXES: .pd_irix5

SGICFLAGS5 = -o32 -DPD -DUNIX -DIRIX -O2


SGIINCLUDE =  -I../../src/

.c.pd_irix5:
	cc $(SGICFLAGS5) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -elf -shared -rdata_shared -o $*.pd_irix5 $*.o
	rm $*.o

# ----------------------- IRIX 6.x -----------------------

pd_irix6: number.pd_irix6 concat.pd_irix6 oname2l.pd_irix6 \
       a2s.pd_irix6 recv2.pd_irix6 send2.pd_irix6 replace_selector.pd_irix6

.SUFFIXES: .pd_irix6

SGICFLAGS6 = -n32 -DPD -DUNIX -DIRIX -DN32 -woff 1080,1064,1185 \
	-OPT:roundoff=3 -OPT:IEEE_arithmetic=3 -OPT:cray_ivdep=true \
	-Ofast=ip32

.c.pd_irix6:
	cc $(SGICFLAGS6) $(SGIINCLUDE) -o $*.o -c $*.c
	ld -IPA -n32 -shared -rdata_shared -o $*.pd_irix6 $*.o
	rm $*.o

# ----------------------- LINUX i386 -----------------------

pd_linux: number.pd_linux concat.pd_linux oname2l.pd_linux \
       a2s.pd_linux recv2.pd_linux send2.pd_linux replace_selector.pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXINCLUDE =  -I../../src

.c.pd_linux:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	ld -export_dynamic  -shared -o $*.pd_linux $*.o -lc -lm
	strip --strip-unneeded $*.pd_linux
	rm $*.o
	cp $*.pd_linux ../bin/
	mv $*.pd_linux ../../extra/

	
