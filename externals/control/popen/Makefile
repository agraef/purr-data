prefix=/usr/lib/pd

EXTERNALS = popen.c

all: $(EXTERNALS:.c=.pd_linux)

.SUFFIXES: .pd_linux

CFLAGS = -g -DUNIX -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch -fPIC

INCLUDE = -I. -I.. -I$(prefix)/src

%.pd_linux: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -o "$*.o" -c "$*.c"
	gcc -shared -o "$*.pd_linux" "$*.o"

clean:
	-rm *.pd_linux *.o

install-doc:
	@test -d $(prefix)/doc/5.reference || mkdir -p $(prefix)/doc/5.reference
	install *.pd $(prefix)/doc/5.reference

install: install-doc
	@test -d $(prefix)/extra || mkdir -p $(prefix)/extra
	install *.pd_linux $(prefix)/extra
