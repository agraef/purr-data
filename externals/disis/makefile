# Makefile for mylib

lib.name = disis

class.sources = disis_phasor~.c
disis_munger~.class.sources = disis_munger.c ADSR.c
define forLinux
    class.sources += disis_netsend.c disis_netreceive.c
    disis_wiimote.class.sources = disis_wiimote.c
    disis_wiimote.class.ldlibs = $(CWIID_LIB_STATIC) -lbluetooth -lpthread
endef
define forDarwin
    class.sources += disis_netsend.c disis_netreceive.c
endef

datafiles = disis_munger~-help.pd disis_phasor~-help.pd disis_netsend-help.pd disis_netreceive-help.pd disis_wiimote-help.pd disis_wiimote_legacy.pd README patch_name-help.pd patch_name.pd disis-meta.pd

# IMPORTANT! disis_wiimote requires a custom L2Ork version of cwiid library
# For the time being we statically link to a L2Ork version of cwiid library
# Change this paths if you are compiling disis_wiimote separately from
# the rest of the pd-l2ork source to reflect the correct location of the
# custom L2Ork version of the libcwiid
CWIID_PATH=./cwiid
CWIID_LIB_STATIC= $(CWIID_PATH)/libcwiid/libcwiid.a
CWIID_INCLUDE = -I$(CWIID_PATH)/libcwiid
cflags = $(CWIID_INCLUDE)

include Makefile.pdlibbuilder.revised
