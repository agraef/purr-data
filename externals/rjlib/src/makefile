current:
	echo make pd_linux, l_i386, l_ia64 or pd_darwin

clean: ; rm -f *.pd_linux *.pd_darwin *.pd_win *.o

SOURCES = $(shell ls *.c)
INCLUDES = -I../ -I../../extralibs/local/include

# ----------------------- LINUX i386 -----------------------

pd_linux: $(SOURCES:.c=.pd_linux)
l_i386: $(SOURCES:.c=.l_i386)
l_ia64: $(SOURCES:.c=.l_ia64)

.SUFFIXES: .pd_linux .l_i386 .l_ia64

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch


.c.pd_linux:
	cc $(LINUXCFLAGS) $(INCLUDES) -o $*.o -c $*.c
	ld -export_dynamic  -shared -o ../$*.pd_linux $*.o -lc -lm
	strip --strip-unneeded ../$*.pd_linux
	rm $*.o

.c.l_i386:
	cc $(LINUXCFLAGS) $(INCLUDES) -o $*.o -c $*.c
	ld -export_dynamic  -shared -o ../$*.l_i386 $*.o -lc -lm
	strip --strip-unneeded ../$*.l_i386
	rm $*.o

.c.l_ia64:
	cc $(LINUXCFLAGS) $(INCLUDES) -o $*.o -c $*.c
	ld -export_dynamic  -shared -o ../$*.l_ia64 $*.o -lc -lm
	strip --strip-unneeded ../$*.l_ia64
	rm $*.o

# ----------------------- Mac OSX -----------------------

pd_darwin: $(SOURCES:.c=.pd_darwin) 

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

.c.pd_darwin:
	cc $(DARWINCFLAGS) $(INCLUDES) -o $*.o -c $*.c
	cc -bundle -undefined suppress -flat_namespace -o ../$*.pd_darwin $*.o 
	rm -f $*.o

