
CFLAGS = -DPD $(OPT_CFLAGS) -I../../pd/src -Wall -W $(DEBUG_CFLAGS)
LDFLAGS =  
LIBS = -lm

UNAME := $(shell uname -s)
ifeq ($(UNAME),Linux)
  OS_NAME = linux
  EXTENSION = pd_linux
  CFLAGS += -DUNIX -Dunix -fPIC
  LDFLAGS += -Wl,--export-dynamic  -shared
endif
ifeq ($(UNAME),Darwin)
  OS_NAME = darwin
  EXTENSION = pd_darwin
  CFLAGS += -DMACOSX -DUNIX -Dunix
  LDFLAGS += -bundle -bundle_loader ../../pd/bin/pd -undefined dynamic_lookup
endif

all: jackx.$(EXTENSION)

%.o: %.c
	$(CC) $(CFLAGS) -o "$*.o" -c "$*.c"

%.$(EXTENSION): %.o
	$(CC) $(LDFLAGS) -o "$*.$(EXTENSION)" "$*.o"  $(LIBS)
	chmod a-x "$*.$(EXTENSION)"
	rm -f -- $*.o

clean:
	-rm -f -- jackx.$(EXTENSION)
	-rm -f -- jackx.o
