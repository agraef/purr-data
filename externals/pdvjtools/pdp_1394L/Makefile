current: all

include Makefile.config

PDP_MOD = pdp_1394L.o 

all: $(PDP_MOD)
	$(CC) -Wl,--export-dynamic -shared -o pdp_1394L.pd_linux *.o $(PDP_LIBS)

clean:
	rm -f *~
	rm -f *.o
	rm -f *.pd_linux

