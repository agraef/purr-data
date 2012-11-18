#############################################################
# videoIO makefile
#
# 2008, IOhannes m zmölnig
#############################################################

-include ../Make.config

#############################################################
# build core library

INCLUDE=-I$(PD_SRC) -I.
LIBS=-lc -lm
CC_FLAGS=-c -Wall -O2 -fPIC -ffast-math -mmmx

SOURCE := $(wildcard *.cpp)
OBJECTS := $(SOURCE:.cpp=.o)

.PHONY: 

# all builds gem-externals and plugins (gstreamer)
all: $(OBJECTS)

gem-externals: $(TARGETS)

$(OBJECTS) : %.o : %.cpp
	g++ $(CC_FLAGS) $(INCLUDE) -o "$*.o" -c "$*.cpp"


clean:
	rm -f *.o $(TARGETS)

# EOF
#############################################################
