current: all

include Makefile.config

PDP_MOD = pdp_freeframe.o 


# build basic image processing modules (derived from base class)
all: $(PDP_MOD)
	$(CC) -Wl,--export-dynamic -shared -o pdp_freeframe.pd_linux *.o $(PDP_LIBS)

clean:
	rm -f *~
	rm -f *.o
	rm -f *.pd_linux

