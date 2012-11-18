
CFLAGS = -O2 -W -Wall

all: ff

ff: ff.c
	$(CC) $(CFLAGS) $(LINUXCFLAGS) $(LINUXINCLUDE) -o ff.o -c ff.c
	ld --export-dynamic  -shared -o ff.pd_linux ff.o -lc -lm
	strip --strip-unneeded ff.pd_linux
	rm ff.o


clean: 
	rm -f *~ *.pd_* *.dll *.o
