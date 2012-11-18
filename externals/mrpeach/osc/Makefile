# written by Alex Norman
# added by Mathieu Bouchard
# because there are several people who want to compile mrpeach/osc without pd-extended

#change these to your liking
prefix = /usr/local/
libdir = $(DESTDIR)/$(prefix)/lib/pd/extra/

INCLUDES += -I/usr/local/include/
CFLAGS += ${INCLUDES} -shared
SUFFIX = pd_linux
INSTALL = install

SRC =   packOSC.c \
	pipelist.c \
	routeOSC.c \
	unpackOSC.c

TARGETS	 = ${SRC:.c=.${SUFFIX}}

build: ${TARGETS}

configure:

#build the libraries
%.${SUFFIX}: %.c
	${CC} ${CFLAGS} -o $*.${SUFFIX} $<

#install the libraries and documentation
install: ${TARGETS}
	${INSTALL} -t ${libdir} ${TARGETS}
	${INSTALL} -t ${libdir} *.pd

clean:
	rm -f ${TARGETS}


