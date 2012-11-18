PD-PATH=/usr/lib/pd
PD-SCR=/usr/include

# location of the GEM sources and Gem.pd_linux:
GEM-SCR=/home/Georg/pd-cvs/gem/Gem/src
GEM-LIB=$(PD-PATH)/extra/Gem.pd_linux


CC = g++
LD = g++
INCLUDE=-I$(PD-SCR) -I$(GEM-SCR) -I./src
LIB=-lc -lm -L$(GEM-LIB)
CC_FLAGS = -c -Wall -g -g -O2 -mmmx -fno-builtin -O3 -funroll-loops -ffast-math
LD_FLAGS = --export-dynamic -shared -o


TARGET=pix_recNN.pd_linux
OBJ=RecurrentNeuron.o Neuron.o pix_recNN.o
#--------------------------------------------------------

all: pd_linux

pd_linux: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(LD_FLAGS) $(TARGET) $(OBJ) $(LIB)
	strip --strip-unneeded $(TARGET)
	chmod 755 $(TARGET)

pix_recNN.o: RecurrentNeuron.o pix_recNN.h pix_recNN.cpp NNet.h NNException.h
	$(CC) $(CC_FLAGS) $(INCLUDE) pix_recNN.cpp


RecurrentNeuron.o: RecurrentNeuron.cpp RecurrentNeuron.h Neuron.o NNActivation.h

Neuron.o: Neuron.cpp Neuron.h NNActivation.h

#--------------------------------------------------------

clean:
	rm -f $(OBJ) $(TARGET)
	

install:
	cp -f $(TARGET) $(PD-PATH)/externs
	cp -f *.pd $(PD-PATH)/doc/5.reference
