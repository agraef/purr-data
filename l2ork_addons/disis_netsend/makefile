NAME=disis_netsend
SYM=disis_netsend

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

current: pd_linux

##### LINUX:

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -g -funroll-loops -fomit-frame-pointer \
    -Wall -Wshadow -Wstrict-prototypes -fPIC

.c.pd_linux:
	cc $(LINUXCFLAGS) $(PD_INCLUDE) $(CWIID_INCLUDE) -o $*.o -c $*.c
	ld --export-dynamic -shared -o $*.pd_linux $*.o $(LIBS) -lc -lm
	rm -f $*.o

install:

ifdef ASCAPE_INSTALLED
	-cp *help*.pd $(ASCAPE_PATH)/ss_engine/pd/help/.
ifeq ($(findstring Linux,$(ASCAPE_OS)),Linux)
	-cp *.pd_linux $(ASCAPE_PATH)/ss_engine/pd/externs/$(ASCAPE_OS)$(ASCAPE_ARCH)/.
endif
ifeq ($(findstring Darwin,$(SS_OS)),Darwin)
	-cp *.pd_darwin $(ASCAPE_PATH)/ss_engine/pd/externs/$(ASCAPE_OS)$(ASCAPE_ARCH)/.
endif
endif

	-cp *.pd_linux $(PD_EXTRA_PATH)/.
	-cp *help*.pd $(PD_DOC_PATH)/.

clean:
	-rm -f *.o *.pd_* so_locations
