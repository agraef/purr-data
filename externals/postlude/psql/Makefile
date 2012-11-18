NAME=psql
CSYM=psql

PDDIR=/usr/local/lib/pd
INCLUDE = -I/usr/include/postgresql

all: pd_linux
current: pd_linux

LDFLAGS = -lc -lm -lpq

# Postgres ----------------------

SRCDIR= /usr/include
TARGDIR= ./

#include Makefile.global #needed for postgresql stuff

# Linux ----------------------------------------------

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS =  -g -DPD -O0 -fPIC -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch

.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(INCLUDE) -o $*.o -c $*.c 
	$(CC) --export-dynamic  -shared -o $*.pd_linux $*.o $(LDFLAGS) 
	strip --strip-unneeded $*.pd_linux
	rm $*.o 

# Darwin ----------------------------------------------

pd_darwin: $(NAME).pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O3 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch -L/usr/local/lib/

.c.pd_darwin:       
	$(CC) $(DARWINCFLAGS) $(INCLUDE) -o $*.o -c $*.c
	$(CC) -bundle -undefined dynamic_lookup -o $(NAME).pd_darwin $*.o $(LDFLAGS) 
	rm -f *.o

# Install ----------------------------------------------

install:
	cp $(NAME).pd_* $(PDDIR)/extra/

# Clean ----------------------------------------------

clean:
	rm -f *.pd_* *.o
