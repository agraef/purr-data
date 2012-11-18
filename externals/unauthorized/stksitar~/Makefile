NAME=stksitar~
CSYM=stksitar_tilde
STK_HOME=/usr/local/stk

current: pd_linux

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXCCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch -g

LINUXINCLUDE =  -I../../src -I/usr/local/include

.cc.pd_linux: sitar.o
	g++ -O3 -Wall -D__OS_Linux_ -g -I$(STK_HOME)/include -c sitar.cc
	g++ $(LINUXCCFLAGS) $(LINUXINCLUDE) -I$(STK_HOME)/include -o $*.o -c $*.cc
	g++ --export-dynamic -shared -o $*.pd_linux *.o $(STK_HOME)/src/libstk.a -lc -lm -lpthread
	strip --strip-unneeded $*.pd_linux
	rm -f *.o ../$*.pd_linux
	ln -s $*/$*.pd_linux ..

test:
	g++ -O3 -Wall -D__OS_Linux_ -g -I$(STK_HOME)/include -c sitar.cc
	g++ -O3 -Wall -D__OS_Linux_ -g -I$(STK_HOME)/include -o mstksitar~ sitar.o $(STK_HOME)/src/stklib.a mstksitar~.cc

clean:
	rm -f *.o *.pd_* so_locations

install:
	cp help-*.pd *-freqs ../../doc/5.reference
