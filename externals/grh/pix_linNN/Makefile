PD-PATH=/usr/lib/pd
PD-SCR=/usr/include

# location of the GEM sources and Gem.pd_linux:
GEM-SCR=/home/Georg/pd-cvs/gem/Gem/src
GEM-LIB=$(PD-PATH)/extra/Gem.pd_linux 


CC = g++
LD = g++
INCLUDE=-I$(PD-SCR) -I$(GEM-SCR) -I.
LIB=-lc -lm -L$(GEM-LIB)
CC_FLAGS = -c -Wall -g -g -O2 -mmmx -fno-builtin -O3 -funroll-loops -ffast-math
LD_FLAGS = --export-dynamic -shared -o


TARGET=pix_linNN.pd_linux
OBJ=LinNeuralNet.o pix_linNN.o
#--------------------------------------------------------

all: pd_linux

pd_linux: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(LD_FLAGS) $(TARGET) $(OBJ) $(LIB)
	strip --strip-unneeded $(TARGET)
	chmod 755 $(TARGET)

pix_linNN.o: LinNeuralNet.o pix_linNN.h pix_linNN.cpp
	$(CC) $(CC_FLAGS) $(INCLUDE) pix_linNN.cpp

LinNeuralNet.o: LinNeuralNet.cpp LinNeuralNet.h
	$(CC) $(CC_FLAGS) $(INCLUDE) LinNeuralNet.cpp

#--------------------------------------------------------

clean:
	rm -f $(OBJ) $(TARGET)
	

install:
	cp -f $(TARGET) $(PD-PATH)/externs
	cp -f help/*.pd $(PD-PATH)/doc/5.reference