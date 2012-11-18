NAME=getpatchname
CSYM=getpatchname

LIBDIR=/usr/local/lib
PDDIR=$(LIBDIR)/pd
INSTALLPATH=$(PDDIR)/extra/

current: pd_linux


# ----------------------- Linux -----------------------

pd_linux: src/$(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -ggdb -DPD -O3 -fPIC -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch

# Debug
#LINUXCFLAGS = -ggdb -g -DPD -O0 -fPIC -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXINCLUDE =  -I/usr/include -I./include

.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -c src/$(NAME).c
	ld --export-dynamic  -shared -o $(NAME).pd_linux $(NAME).o -lc 
	strip --strip-unneeded $(NAME).pd_linux
	rm -f *.o

# ----------------------- Mac OSX -----------------------

pd_darwin: src/$(NAME).pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O3 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch -L/usr/local/lib/

DARWININCLUDE = $(LINUXINCLUDE)

.c.pd_darwin:
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -c src/$(NAME).c
	$(CC) -bundle -undefined suppress -flat_namespace -o $(NAME).pd_darwin $(NAME).o
	rm -f *.o

# ----------------------- Generic -----------------------

clean:
	rm -f *.o *.pd_* so_locations

install:
	cp getpatchname.pd_linux $(INSTALLPATH)
	install -d  $(PDDIR)/doc/5.reference/getpatchname/
	install -m 644 doc/help-* $(PDDIR)/doc/5.reference/
