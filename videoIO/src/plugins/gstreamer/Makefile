# all builds gem-externals and plugins (gstreamer)
default: all

#############################################################
# videoIO makefile
#
# 2008 (c) Georg Holzmann, IOhannes m zmölnig
#############################################################

-include ../../Make.config

#############################################################
# build gem-external targets and objects

INCLUDE=-I. -I../../core
LIBS=-lc -lm
CC_FLAGS=-c -Wall -O2 -fPIC -ffast-math -mmmx
LD_FLAGS=--export-dynamic -shared
EXTENSION=pd_linux

#############################################################
# build plugins/gstreamer/gstreamer interface

GST_INCL=-I$(GST_INCLUDE) -I$(GLIB_INCLUDE1) -I$(GLIB_INCLUDE2) \
         -I$(XML_INCLUDE)
GST_LIBS=-lgstreamer-0.10 -lgstapp-0.10

all: FileReadGst.so FileWriteGst.so DeviceReadGst.so

%.so: %.cpp %.h
	$(CXX) $(CC_FLAGS) $(GST_INCL) $(INCLUDE) $<
	$(CXX) $(LD_FLAGS) -o $@ $*.o $(GST_LIBS)


#############################################################
# clean and install

clean:
	rm -f *.o $.so

install:
	install -d $(PLUGIN_INSTALL)
	install -p *.so $(PLUGIN_INSTALL)

# EOF
#############################################################
