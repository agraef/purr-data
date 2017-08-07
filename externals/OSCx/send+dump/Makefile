LIBS = -L../../../pd/bin -lc -lm
LIBOSCDIR = ../libOSC
LIBOSC = ${LIBOSCDIR}/libOSC.a

# strange bug requires this...
CFLAGS= -g -O2 -DUNIX -Wall -Wimplicit -Wunused -Wmissing-prototypes -O1 -fPIC -I../libOSC -I../../pd/src -I../../../pd/src -I../src -I../libOSC -I../../pd/src -I../../../pd/src -I../src
# INCLUDES= -I../libOSC -I../../pd/src -I../../../pd/src -I../src

DUMPOBJS=dumpOSC.o


both: sendOSC dumpOSC

sendOSC: sendOSC.o htmsocket.o ${LIBOSC}
	${CC} ${CFLAGS} $(INCLUDES) -o sendOSC sendOSC.o htmsocket.o $(LIBS) ${LIBOSC}

dumpOSC: ${DUMPOBJS}
	${CC} ${CFLAGS} $(INCLUDES) -o $@ ${DUMPOBJS} $(LIBS)

dumpUDP: dumpUDP.o
	${CC} ${CFLAGS} $(INCLUDES) -o dumpUDP dumpUDP.o $(LIBS)

${LIBOSC}:
	echo "You need to go to " ${LIBOSCDIR} " and do a make."
	(cd ../libOSC ; make)

clean:
	rm -f sendOSC dumpOSC *.o


