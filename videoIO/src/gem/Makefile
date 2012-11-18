default: all
#############################################################
# videoIO makefile
#
# 2008, Georg Holzmann
# 2008, IOhannes m zmölnig
#############################################################

-include ../Make.config

#############################################################
# build gem-external targets and objects

INCLUDE=-I$(PD_SRC) -I$(GEM_SRC) -I. -I$(VIDEOIO_SRC)
LIBS=-lc -lm -L$(GEM_LIB)
CC_FLAGS=-DHAVE_BASE_CONFIG_H_ -c -Wall -O2 -fPIC -ffast-math -mmmx
LD_FLAGS=--export-dynamic -shared -o
EXTENSION=pd_linux

SOURCE := $(wildcard *.cpp)
OBJECTS := $(SOURCE:.cpp=.o)
TMP := $(wildcard pix_*.cpp)
TARGETS := $(TMP:.cpp=.$(EXTENSION))

.PHONY: default all

# all builds gem-externals and plugins (gstreamer)

all: $(TARGETS)

$(TARGETS) : %.$(EXTENSION) : %.o $(OBJECTS)
	g++ $(LD_FLAGS) $*.$(EXTENSION) $(OBJECTS) $(VIDEOIO_SRC)/*.o $(LIBS)
	#strip --strip-unneeded $*.$(EXTENSION)
	chmod 755 $*.$(EXTENSION)

$(OBJECTS) : %.o : %.cpp
	g++ $(CC_FLAGS) $(INCLUDE) -o "$*.o" -c "$*.cpp"


#############################################################
# clean and install

clean:
	rm -f *.o $(TARGETS)

install:
	install -d $(PD_INSTALL)
	install -p $(TARGETS) $(PD_INSTALL)
	install -d $(PD_HELP)
	install -p ../../help/*.pd $(PD_HELP)

# EOF
#############################################################
