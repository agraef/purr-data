PD-PATH=/usr/lib/pd
PD-SCR=/usr/include


INCLUDE=-I$(PD-SCR) -I.
LIB=-lc -lm
CC_FLAGS = -DPD -c -Wall
LD_FLAGS = --export-dynamic -shared -o


TARGET=notemono.pd_linux
OBJ=notemono.o
#--------------------------------------------------------

all: pd_linux

pd_linux: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(LD_FLAGS) $(TARGET) $(OBJ) $(LIB)
	strip --strip-unneeded $(TARGET)
	chmod 755 $(TARGET)

.c.o:
	$(CC) $(CC_FLAGS) $(INCLUDE) $<

clean:
	rm -f $(OBJ) $(TARGET)
