# flext tutorial examples
#
# Makefile for gcc @ linux
#
# usage: make -f makefile.pd-linux
#
# -----------------------------------------------------

include config-pd-linux.txt

FLEXTLIB=$(FLEXTPATH)/flext_t.a  # take threaded library for all

PDEXTRA=$(DESTDIR)/usr/lib/pd/extra

# compiler+linker stuff	### EDIT! ###
#CXX=g++ #-3.2
INCLUDES=$(PDPATH)/src $(FLEXTINC)
LIBPATH=
FLAGS=-DPD -DFLEXT_THREADS
CFLAGS=-O6 -mcpu=pentiumpro
LIBS=m iiwusynth


# the rest can stay untouched
# ----------------------------------------------

# all the source files from the package

EXAMPLES=fiiwu


TARGETS=$(patsubst %,$(OUTPATH)/%~.pd_linux,$(EXAMPLES))

# default target
all: $(OUTPATH) $(TARGETS)


$(OUTPATH)/%~.pd_linux : $(OUTPATH)/%.pd_linux
	mv $^ $@

$(SRCS): $(HDRS)
	touch $@

$(OUTPATH):
	mkdir $(OUTPATH)

$(OUTPATH)/%.o : %/main.cpp
	$(CXX) -c $(CFLAGS) $(FLAGS) $(patsubst %,-I%,$(INCLUDES) $(FLEXTPATH)) $< -o $@

$(OUTPATH)/%.pd_linux : $(OUTPATH)/%.o $(FLEXTLIB)
	$(CXX) $(LDFLAGS) -shared $(patsubst %,-L%,$(LIBPATH)) $^ $(patsubst %,-l%,$(LIBS)) -o $@
	chmod 755 $@


install:
	install -m 644 pd-linux/fiiwu~.pd_linux $(PDEXTRA)

.PHONY: clean
clean:
	rm -f $(OUTPATH)/*.o $(TARGETS)
	

