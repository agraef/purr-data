#CFLAGS=  -O2 -Wall -fPIC
ARFLAGS=srv
# uncomment the following for linux/win
# DEFS= -Dunix
LIB=libOSC.a

CFLAGS=-g -O2 -DUNIX -Wall -Wimplicit -Wunused -Wmissing-prototypes -O1 -fPIC -I../libOSC -I../../pd/src -I../../../pd/src -I../src
INCLUDES=-I../libOSC -I../../pd/src -I../../../pd/src -I../src

LIBOBJS= ${LIB}(OSC-client.o) ${LIB}(OSC-timetag.o)

all:	${LIBOBJS}

.c.a:
	${CC} -c ${CFLAGS} ${INCLUDES} ${DEFS} $<
	${AR} ${ARFLAGS} $@ $*.o
	rm -f $*.o

test_OSC: test_OSC.o ${LIB}
	$(CC) -o test_OSC test_OSC.o ${LIB}

test_OSC_timeTag: test_OSC_timeTag.o OSC-timetag.o
	$(CC) -o test_OSC_timeTag test_OSC_timeTag.o  OSC-timetag.o


clean:
	rm -f ${LIB} *.o
