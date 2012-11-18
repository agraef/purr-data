all: pd_linux
#all: pd_win
#all: pd_darwin

.SUFFIXES: .pd_linux .pd_darwin .dll .tk .tk2c

PDPATH = /home/pat/pd




# ----------------------- WINDOWS -------------------------
pd_win: tracker.dll
tracker.dll: tracker.c tracker.h tracker.tk2c

WINCFLAGS = -Wall -W -Wshadow -Wstrict-prototypes -DPD -DNT -W3 -WX -Werror -Wno-unused -mms-bitfields -Wno-parentheses -Wno-switch -O6 -funroll-loops -fomit-frame-pointer
WININCLUDE =  -I.. -I../include -I$(PDPATH)/src

WINLDFLAGS = -shared

.c.dll:
	gcc -mms-bitfields $(WINCFLAGS) $(WININCLUDE) -o $*.o -c $*.c
	gcc $(WINLDFLAGS) -o $*.dll $*.o $(PDPATH)/bin/pd.dll
	strip --strip-unneeded $*.dll
	rm -f $*.o

# ----------------------- LINUX i386 ----------------------
pd_linux: tracker.pd_linux
tracker.pd_linux: tracker.c tracker.h tracker.tk2c

LINUXCFLAGS = -DPD -DUNIX -O2 -funroll-loops -fomit-frame-pointer \
   -Wall -W -Wno-shadow -Wstrict-prototypes \
   -Wno-unused -Wno-parentheses -Wno-switch
LINUXINCLUDE = -I/usr/include -I$(PDPATH)/src
LINUXLDFLAGS = --export-dynamic  -shared

.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(CFLAGS) $(LINUXINCLUDE) -fPIC -o $*.o -c $*.c
	$(LD) $(LINUXLDFLAGS) -o $*.pd_linux $*.o -lc -lm
	strip --strip-unneeded $*.pd_linux

# ----------------------- Darwin (OSX) --------------------
pd_darwin: tracker.pd_darwin
tracker.pd_darwin: tracker.c tracker.h tracker.tk2c

DARWINCFLAGS = -DPD -DUNIX -O2 -funroll-loops -fomit-frame-pointer \
   -Wall -W -Wno-shadow -Wstrict-prototypes \
   -Wno-unused -Wno-parentheses -Wno-switch
DARWININCLUDE = -I/usr/include -I$(PDPATH)/src
DARWINLDFLAGS = -bundle -undefined suppress -flat_namespace

.c.pd_darwin:
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) -o $*.o -c $*.c
	$(LD) $(DARWINLDFLAGS) -o $*.pd_darwin $*.o -lc -lm

# ----------------------- Miscellaneous -------------------
.tk.tk2c:
	sh tk2c.bash < $*.tk > $*.tk2c

clean:
	rm -f *.o *.pd_linux *.pd_darwin *.pd_win *.dll so_locations *.tk2c


