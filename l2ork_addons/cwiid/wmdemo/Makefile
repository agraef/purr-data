#Copyright (C) 2007 L. Donnie Smith

include ../defs.mak

APP_NAME = wmdemo

SOURCES = wmdemo.c

CFLAGS += -I../libcwiid
LDFLAGS += -L../libcwiid
LDLIBS += -lcwiid -lbluetooth -lpthread -lrt
INST_DIR = ${exec_prefix}/bin

include $(COMMON)/include/app.mak

distclean: clean
	rm Makefile

.PHONY: distclean
