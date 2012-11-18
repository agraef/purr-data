current: all

.SUFFIXES: .pd_linux

INCLUDE = -I. -I/usr/local/lib/pd/src -I/home/fzotter/downloads/pd-0.39-0/pd/obj

LDFLAGS = -export-dynamic -shared
LIB = -ldl -lm -lpthread

#select either the DBG and OPT compiler flags below:

CFLAGS = -DPD -DUNIX -Wno-unused \
	-Wno-parentheses -Wno-switch -O6 -funroll-loops -fomit-frame-pointer \
        -DDL_OPEN

SYSTEM = $(shell uname -m)

# the sources

SRC = pdoctave_dataframe.c \
      pdoctave.c \
      pdoctave_send.c \
      pdoctave_command.c \
      pdoctave_get.c

TARGET = ./pdoctave.pd_linux

#TARGET2 = ./pdoctave_send.pd_linux


OBJ = $(SRC:.c=.o) 

#
#  ------------------ targets ------------------------------------
#

install: all 
	sudo cp pdoctave.pd_linux /usr/local/lib/pd/externs/

clean:
	rm -f $(OBJ) read_shared_mem.oct write_shared_mem.o write_shared_mem.oct read_shared_mem.o

all: pdoctave
#all: pdoctave pdoctave_send

pdoctave: $(OBJ) read_shared_mem.oct write_shared_mem.oct

	@echo :: $(OBJ)

	ld $(LDFLAGS) -o $(TARGET) $(OBJ) $(LIB)
	strip --strip-unneeded $(TARGET)
	#rm *.o

#pdoctave_send: $(OBJ2)
#
#	@echo :: $(OBJ2)
#
#	ld $(LDFLAGS) -o $(TARGET2) $(OBJ) $(LIB)
#	strip --strip-unneeded $(TARGET2)

$(OBJ) : %.o : %.c
	#touch $*.c
	cc $(CFLAGS) $(INCLUDE) -c -o $*.o $*.c

pdoctave_dataframe.o: pdoctave_dataframe.h pdoctave_dataframe.c

read_shared_mem.oct: read_shared_mem.cc pdoctave_dataframe.o
	mkoctfile read_shared_mem.cc -c -o read_shared_mem.o
	mkoctfile read_shared_mem.o pdoctave_dataframe.o -o read_shared_mem.oct

write_shared_mem.oct: write_shared_mem.cc pdoctave_dataframe.o
	mkoctfile write_shared_mem.cc -c -o write_shared_mem.o
	mkoctfile write_shared_mem.o pdoctave_dataframe.o -o write_shared_mem.oct

pdoctave_send.o: pdoctave_dataframe.o pdoctave_send.c

pdoctave_command.o: pdoctave_dataframe.o pdoctave_command.c

pdoctave_get.o: pdoctave_dataframe.o pdoctave_get.c
