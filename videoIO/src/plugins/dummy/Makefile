#############################################################
# videoIO makefile
#
# 2008, IOhannes m zmölnig
#############################################################

-include ../../Make.config

CC_FLAGS=-c -Wall -O2 -fPIC -ffast-math -mmmx
CC_INCLUDE=-I../../core
LD_FLAGS=--export-dynamic -shared

all: FileReadDummy.so DeviceReadDummy.so

%.so: %.cpp %.h
	$(CXX) $(CC_FLAGS) $(CC_INCLUDE) $<
	$(CXX) $(LD_FLAGS) -o $@ $*.o

clean:
	rm -f *.o *.so

install:
	# we don't install the dummy plugins !!!

# EOF
#############################################################
