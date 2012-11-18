CC=gcc

pd_linux: input_noticer.pd_linux parseeventnodenum.pd_linux

clean: ; rm -f *.pd_linux *.o *~

# installs all compiled externals.
# you'll want to change this to match where your externals live
install: ; sudo cp *.pd_linux /usr/local/lib/pd/extra/ ; sudo cp *help.pd /usr/local/lib/pd/doc/5.reference/

# installs the input_noticer
# you'll want to change this to match where your externals live
install_noticer: ; sudo cp input_noticer.pd_linux /usr/local/lib/pd/extra ; sudo cp input_noticer-help.pd /usr/local/lib/pd/doc/5.reference/

# ----------------------- LINUX i386 -----------------------

.SUFFIXES: .pd_linux

LINUXLDFLAGS = `pkg-config --libs glib-2.0 hal dbus-glib-1` -lpthread -lgthread-2.0 -lglib-2.0
# LINUXLDFLAGS = `pkg-config --libs glib-2.0 hal dbus-glib-1 gthread-2.0`

LINUXCFLAGS = -DUNIX -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch \
    `pkg-config --cflags --libs glib-2.0 hal dbus-glib-1 gthread-2.0`

LINUXINCLUDE =  -I/usr/local/lib/pd/include -I/usr/lib/pd/src -I/usr/local/include 

.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	ld --export-dynamic -shared -o $*.pd_linux $*.o -lc -lm $(LINUXLDFLAGS)
	strip --strip-unneeded $*.pd_linux
	rm $*.o
