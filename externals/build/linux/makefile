
prefix=$(DESTDIR)/usr

EXTERNALS = $(shell ls ../src)
STRIP = strip --strip-unneeded -R .note -R .comment

all: 
	make -k externals
	
externals: $(EXTERNALS:.c=.pd_linux)

# this might be a better way to do this:
# EXTERNALS    := ${patsubst %.c, %.o, ${wildcard *.c}} 
# all: ${EXTERNALS}

.SUFFIXES: .pd_linux

# cross-platform default
CPU_FLAGS = -O2

# AMD Athlon XP K7
#CPU_FLAGS = -O3 -march=athlon-xp -m3dnow -msse -mfpmath=sse

# Generic x86 (tune for 686, since that's most common these days)
# CPU_FLAGS = -O2 -mcpu=i686
# Pentium MMX
#CPU_FLAGS = -O3 -march=pentium-mmx -mmmx
# Pentium Pro
#CPU_FLAGS = -O3 -march=pentiumpro -mmmx
# Pentium II/Celeron
#CPU_FLAGS = -O3 -mfpmath=sse -mmmx -msse -march=pentium2
# Pentium III/Celeron2
#CPU_FLAGS = -O3 -mfpmath=sse -mmmx -msse -march=pentium3
# Pentium 4
#CPU_FLAGS = -O3 -mfpmath=sse -mmmx -msse -msse2 -march=pentium4

# PowerPC 750 (G3)
#CPU_FLAGS = -O3 -mpowerpc-gpopt -mcpu=750
# PowerPC 7400 (G4 <= 700MHz)
#CPU_FLAGS = -O3 -mpowerpc-gpopt -mcpu=7400 -maltivec
# PowerPC 7450 (G4 >= 533Mhz)
#CPU_FLAGS = -O3 -mpowerpc-gpopt -mcpu=7450 -maltivec
# PowerPC G5
#CPU_FLAGS = -O3 -mpowerpc64 -maltivec -falign-functions=32 -falign-labels=32 -falign-loops=32 -falign-jumps=32

OPTIM_FLAGS = -funroll-loops -fomit-frame-pointer -fno-strict-aliasing $(CPU_FLAGS)

CFLAGS = -fPIC -DPD -DUNIX $(OPTIM_FLAGS)  \
    -Wall -W -Wshadow -Wstrict-prototypes  \
    -Wno-unused -Wno-parentheses -Wno-switch

INCLUDES = -I. -I.. -I../../../pd/src -I../include

%.pd_linux: ../src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -o "$*.o" -c "../src/$*.c"
	gcc -Wl,--export-dynamic  -shared -o "$*.pd_linux" "$*.o" -lc -lm \
		`test -f $*.libs && cat $*.libs` \
		`test -f ../src/$*.libs && cat ../src/$*.libs`
	chmod a-x "$*.pd_linux"
	$(STRIP) $*.pd_linux
	rm -f "$*.o" 

clean:
	-rm *.pd_linux *~ *.c
	-rm link.stamp

install-doc:
	@test -d $(prefix)/lib/pd/doc/5.reference || mkdir -p $(prefix)/lib/pd/doc/5.reference
	cd ../doc && make all
	cp -r ../doc/* $(prefix)/lib/pd/doc/5.reference

install: install-doc
	@test -d $(prefix)/lib/pd/extra || mkdir -p $(prefix)/lib/pd/extra
	install -m644 *.pd_linux $(prefix)/lib/pd/extra


deb:
	cd ../.. && dpkg-buildpackage -b
