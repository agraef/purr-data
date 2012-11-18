current: all

PDSOURCE ?= ../../../pd/src
IEMLIBSRC ?= ../../iemlib/iemlib1/src/
INTERSENSESDK ?= /tmp/zmoelnig/isense/SDK/

.SUFFIXES: .pd_linux
VPATH = $(INTERSENSESDK)

INCLUDE = -I. -I$(PDSOURCE) -I$(IEMLIBSRC) -I$(INTERSENSESDK)


LDFLAGS = -export-dynamic -shared -L$(INTERSENSESDK)/Linux
LIB = -ldl -lm -lpthread

#select either the DBG and OPT compiler flags below:

CFLAGS = -DPD -DUNIX -W -Werror -Wno-unused \
	-Wno-parentheses -Wno-switch -O6 -funroll-loops -fomit-frame-pointer \
        -DDL_OPEN

SYSTEM = $(shell uname -m)

# the sources

SRC = isense.c \
	itrax2.c

TARGET = itrax2.pd_linux


OBJ = $(SRC:.c=.o) 

#
#  ------------------ targets ------------------------------------
#

clean:
	-rm $(TARGET)
	-rm *.o

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo :: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $< $(LIB)
	strip --strip-unneeded $(TARGET)

$(OBJ) : %.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<




