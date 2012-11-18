PD_DIR = ../../../pd
GEM_DIR = ../../../Gem

LIBS =  -lm
GEM2PNM_VERSION = 0.6

GEM2PNM_DISTRO = /mnt/c/ydegoyon.free.fr/gem2pdp-$(GEM2PNM_VERSION)
GEM2PNM_TARBALL = $(GEM2PNM_DISTRO).tar.gz
# build flags

GEM2PNM_INCLUDE =  -I$(PD_DIR)/src -I.  -I$(GEM_DIR)/src -I$(PD_DIR)/src
GEM2PNM_CPPFLAGS  = -fPIC -DPD -O2 -funroll-loops -fomit-frame-pointer  -ffast-math \
    -Wall -W -Wno-unused -Wno-parentheses -Wno-switch \
    -DGEM2PNM_VERSION=\"$(GEM2PNM_VERSION)\" -g

all: pix_preview.pd_linux


pix_preview.pd_linux: pix_preview.o 
	rm -f pix_preview.pd_linux
	gcc -Wl,--export-dynamic -shared -o pix_preview.pd_linux pix_preview.o $(LIBS)

clean:
	rm -f *.o
	rm -f pix_preview.pd_linux

distro: clean all
	rm *.o

.cc.o:
	g++ $(GEM2PNM_CPPFLAGS) $(GEM2PNM_INCLUDE) -o $*.o -c $*.cc

.c.o:
	gcc $(GEM2PNM_CPPFLAGS) $(GEM2PNM_INCLUDE) -o $*.o -c $*.c

install:
	cp -f --remove-destination *.pd $(PD_DIR)/doc/5.reference
