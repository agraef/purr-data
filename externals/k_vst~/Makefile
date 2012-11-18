


NAME=k_vst~
CSYM=k_vst_tilde

current: pd_linux


# ----------------------- LINUX i386 -----------------------

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -DUNIX -DICECAST -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wno-shadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch #-Werror

LINUXINCLUDEPATH=../../src
LINUXINCLUDE =  -I$(LINUXINCLUDEPATH) -I../../vstserver/include

.c.pd_linux:
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -I/site/include/wine -c $(NAME).c -I../include -DPLUGIN_TILDE_USE_VST -DPLUGIN_DEBUG -DPLUGIN_TILDE_VERBOSE
	cc $(LINUXCFLAGS) $(LINUXINCLUDE) -I/site/include/wine  -c plugin~_vst.c -I../include -DPLUGIN_TILDE_USE_VST -DPLUGIN_DEBUG -DPLUGIN_TILDE_VERBOSE 

	ld --export-dynamic k_vst~.o plugin~_vst.o -shared -o $(NAME).pd_linux -lc -lm -L/usr/local/lib -L../../vstserver -lvst
	strip --strip-unneeded $*.pd_linux
	rm -f $*.o ../$*.pd_linux
	ln -s k_vst~/$*.pd_linux ..

# ----------------------------------------------------------

install:
	cp help-*.pd ../../doc/5.reference

clean:
	rm -f *.o *.pd_* so_locations
