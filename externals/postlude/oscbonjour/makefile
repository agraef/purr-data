PD-PATH=/usr/lib/pd
PD-SRC=/usr/local/include
ZEROCONF-SRC=../zeroconf

CC = g++
LD = g++
INCLUDE=-I$(PD-SRC) -I$(ZEROCONF-SRC) -I.
LIB=-lc -lm
CC_FLAGS = -DPD -c -Wall -fPIC -O3
LD_FLAGS = --export-dynamic -shared -o


TARGET=oscbonjour.pd_linux
OBJ=oscbonjour.o


#--------------------------------------------------------

all: pd_linux

pd_linux: $(TARGET)


$(TARGET): $(OBJ)
	$(LD) $(LD_FLAGS) $(TARGET) $(OBJ) $(LIB)
	strip --strip-unneeded $(TARGET)
	chmod 755 $(TARGET)

oscbonjour.o:
	$(CC) $(CC_FLAGS) $(INCLUDE) oscbonjour.cpp
#TinyXML:

#tinyxml.o: tinyxml/tinyxml.h tinyxml/tinyxml.cpp
#	$(CC) $(CC_FLAGS) $(INCLUDE) tinyxml/tinyxml.cpp


#-----------------------------------------------------


clean:
	rm -f $(OBJ) $(TARGET)
	

install:
	cp -f $(TARGET) $(PD-PATH)/externs
	cp -f help/*.pd $(PD-PATH)/doc/5.reference
