current: all

include Makefile.config

PDP_MOD = pdp_frei0r.o 

all: $(PDP_MOD)
	$(CC) --export-dynamic -shared -o pdp_frei0r.pd_linux *.o $(PDP_LIBS)

clean:
	rm -f *~
	rm -f *.o
	rm -f *.pd_linux

