current: all

.SUFFIXES: .pd_linux

INCLUDE = -I. -I/usr/local/src/pd/src

LDFLAGS = --export-dynamic -shared
LIB = -ldl -lm -lpthread

#select either the DBG and OPT compiler flags below:

CFLAGS = -DPD -DUNIX -W -Wno-unused \
	-Wno-parentheses -Wno-switch -O6 -funroll-loops -fomit-frame-pointer \
        -DDL_OPEN

SYSTEM = $(shell uname -m)

# the sources

SRC = hdspm_mixer.c hdspmmixer.c

TARGET = hdspmmixer hdspmmixer.pd_linux


OBJ = $(SRC:.c=.o) 

#
#  ------------------ targets ------------------------------------
#
all: $(TARGET)

hdspmmixer: hdspm_mixer.c hdspm_mixer.h
	$(CC) $(CFLAGS) -DMAIN -DVERBOSE -lm -lc -lasound -o $@ hdspm_mixer.c

clean:
	-rm $(TARGET)
	-rm *.o

hdspmmixer.pd_linux: $(OBJ)
	@echo :: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ *.o $(LIB)
	strip --strip-unneeded $@

$(OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $*.o $*.c




