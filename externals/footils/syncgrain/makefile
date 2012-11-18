# flext tutorial examples
# Copyright (c) 2001-2003 Thomas Grill (xovo@gmx.net)
#
# Makefile for gcc @ linux
#
# usage: make -f makefile.pd-linux
#
# -----------------------------------------------------

include config-pd-linux.txt

FLEXTLIB=$(FLEXTPATH)/flext.a  # take normal library 
SNDOBJLIB=/usr/lib/libsndobj.a

# compiler+linker stuff	### EDIT! ###
INCLUDES=$(PDPATH)/src $(SNDOBJ) $(FLEXTINC)
LIBPATH=
FLAGS=-DFLEXT_SYS=2 -DFLEXT_ATTRIBUTES
CFLAGS=-O6 -fno-rtti 
CFLAGS+=-mcpu=pentiumpro  # gcc 2.95
# CFLAGS+=-mcpu=pentium3 -msse  # gcc 3.2
LIBS=m
#LIBS+=sndobj


# the rest can stay untouched
# ----------------------------------------------

# all the source files from the package

EXAMPLES=syncgrain~
SRCS=buftable.cpp BufSyncGrain.cpp main.cpp

#O_FILES = $(patsubst %,$(OUTPATH)/%.o,$(EXAMPLES)/$(SRCS))

TARGETS=$(patsubst %,$(OUTPATH)/%.pd_linux,$(EXAMPLES))

# default target
all: $(OUTPATH) $(TARGETS)


$(OUTPATH)/%~.pd_linux : $(OUTPATH)/%.pd_linux
	mv $^ $@

$(SRCS): $(HDRS)
	touch $@

$(OUTPATH):
	mkdir $(OUTPATH)

$(OUTPATH)/%.o : $(EXAMPLES)/%.cpp
	$(CXX) -c $(CFLAGS) $(FLAGS) $(patsubst %,-I%,$(INCLUDES) $(FLEXTPATH)) $< -o $@

$(OUTPATH)/%.pd_linux :  $(OUTPATH)/buftable.o $(OUTPATH)/BufSyncGrain.o $(OUTPATH)/main.o $(FLEXTLIB) $(SNDOBJLIB)
	$(CXX) $(LDFLAGS) -shared $(patsubst %,-L%,$(LIBPATH)) $^ $(patsubst %,-l%,$(LIBS)) -o $@
	chmod 755 $@


.PHONY: clean
clean:
	rm -f $(OUTPATH)/*.o $(TARGETS)
	

ifdef INSTPATH
$(INSTPATH):
	-mkdir $(INSTPATH)

install:: $(INSTPATH)
endif

install:: $(patsubst %,$(OUTPATH)/%.pd_linux,$(EXAMPLES))
	cp $^ $(INSTPATH)
	chmod 755 $(patsubst %,$(INSTPATH)/%,$(notdir $^))
	chown root.users $(patsubst %,$(INSTPATH)/%,$(notdir $^))
