NAME=disis_phasor~
SYM=disis_phasor~

# If you want to use a customized Pd, then define a $PD_PATH variable.
# Otherwise, the Pd must be installed on the system
PD_PATH=../../pd

######################################################
# You shouldn't need to change anything beyond here! #
######################################################


ifdef PD_PATH
PD_INCLUDE := -I$(PD_PATH)/src
PD_EXTRA_PATH := $(PD_PATH)/extra
PD_DOC_PATH := $(PD_PATH)/doc
else
PD_INCLUDE := -I/usr/local/include
PD_EXTRA_PATH := /usr/local/lib/pd/extra
PD_DOC_PATH := /usr/local/lib/pd/doc
endif
	
LIBS =
#LIBS = -lrt -lbluetooth -lpthread


current: pd_linux

##### LINUX:

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -g -fPIC #-funroll-loops -fomit-frame-pointer -Wall -Wshadow -Wstrict-prototypes

.c.pd_linux:
	gcc $(LINUXCFLAGS) $(PD_INCLUDE) -o $*.o -c $*.c
	gcc -shared -o $(NAME).pd_linux $(NAME).o -lm -lc
	strip --strip-unneeded $*.pd_linux 
	rm -f $*.o

install:

	-cp *.pd_linux $(PD_EXTRA_PATH)/.
	-cp *help*.pd $(PD_DOC_PATH)/.

clean:
	-rm -f *.o *.pd_* so_locations
