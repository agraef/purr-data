#Copyright (C) 2007 L. Donnie Smith

include ../defs.mak

APP_NAME = wmgui

SOURCES = main.c interface.c support.c

CFLAGS += -pthread -I/usr/include/gtk-2.0 -I/usr/lib/x86_64-linux-gnu/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/gio-unix-2.0/ -I/usr/include/freetype2 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pixman-1 -I/usr/include/libpng12 -I/usr/include/harfbuzz   -I../libcwiid
LDFLAGS += -L../libcwiid
LDLIBS += -pthread -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfontconfig -lgobject-2.0 -lfreetype -lgthread-2.0 -lglib-2.0   -lm -lcwiid -lbluetooth
INST_DIR = ${exec_prefix}/bin

include $(COMMON)/include/app.mak

distclean: clean
	rm Makefile

.PHONY: distclean
