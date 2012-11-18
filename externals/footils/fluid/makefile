# flext tutorial examples
#
# Makefile for gcc @ linux
#
# usage: make -f makefile.pd-linux
#
# -----------------------------------------------------

include config-pd-linux.txt

FLEXTLIB=$(FLEXTPATH)/flext.a  # take threaded library for all
#FLUIDLIB=/usr/local/lib/libfluidsynth.a
FLUIDLIB=-L/usr/local/lib -lfluidsynth -lreadline

PDEXTRA=$(DESTDIR)/usr/lib/pd/extra

# compiler+linker stuff	### EDIT! ###
#CXX=g++ #-3.2
INCLUDES=$(PDPATH)/src $(FLEXTINC)
LIBPATH=
FLAGS=-DPD 
CFLAGS=-O6 -mcpu=pentiumpro
LIBS=m 
STRIP = strip --strip-unneeded -R .note -R .comment


# the rest can stay untouched
# ----------------------------------------------

# all the source files from the package

EXAMPLES=fluid


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

$(OUTPATH)/%.pd_linux : $(OUTPATH)/%.o 
	$(CXX) $(LDFLAGS) -shared $(patsubst %,-L%,$(LIBPATH)) $^ $(patsubst %,-l%,$(LIBS)) $(FLUIDLIB) $(FLEXTLIB) -o $@
	$(STRIP) $@
	chmod 755 $@


install:
	install -m 644 pd-linux/fluid~.pd_linux $(PDEXTRA)

.PHONY: clean
clean:
	rm -f $(OUTPATH)/*.o $(TARGETS)
	

