#!/bin/bash
set -o verbose
CFGLAGS=`pkg-config --cflags x11 xcb-xlib xcb xcursor libxml-2.0 gtk+-x11-2.0 gtk+-2.0 glib-2.0 gmodule-2.0`
LIBS=`pkg-config --libs x11 xcb-xlib xcb xcursor libxml-2.0 gtk+-x11-2.0 gtk+-2.0 glib-2.0 gmodule-2.0`
INCLUDES="-I../src -I../../pd/src"

gcc -Wall -Werror $INCLUDES $CFLAGS -W -Wshadow -Wstrict-prototypes -Wno-unused -Wno-parentheses -Wno-switch -fPIC -lm -lc  -o x11key.o -c x11key.c
ld  -shared -o x11key.pd_linux x11key.o -lc -lm $LIBS
strip --strip-unneeded x11key.pd_linux

