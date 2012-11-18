
# Adapt the PD_PATH if your pd is not installed in the standard place

# path for compiling against an installed version of Pd
#PD_PATH="$(PROGRAMFILES)/pd"
# path for compiling from CVS with the standard developer layout
PD_PATH=../../../pd

STRIP = strip --strip-unneeded -R .note -R .comment

# only try to compile C files
EXTERNALS = $(shell ls ../src | grep -e '.*\.c$$')

# these are little macros for remapping things on Windows which are
# defined on other platforms
WINDOWS_COMPAT_DEFINES = \
	-D'drand48()=((double)rand()/RAND_MAX)' \
	-D'srand48(n)=srand((n))' \
	-D'O_NONBLOCK=1' \
# These don't seem to be needed:
#	-D'bzero(p,n)=memset(p,0,n)' \
#	-D'PROT_READ=1' \
#	-D'MAP_PRIVATE=2' \
#	-D'O_NDELAY=O_NONBLOCK'

DEFINES = -DPD -DNT $(WINDOWS_COMPAT_DEFINES)


# Generic x86 (compile for 686, since that's most common these days)
OPTIM_FLAGS = -O3 -march=i686 -mfpmath=sse -msse
# Pentium
#OPTIM_FLAGS = -O3 -march=pentium
# Pentium MMX
#OPTIM_FLAGS = -O3 -march=pentium-mmx -mmmx
# Pentium Pro
#OPTIM_FLAGS = -O3 -march=pentiumpro -mmmx
# Pentium II/Celeron
#OPTIM_FLAGS = -O3 -mfpmath=sse -mmmx -msse -march=pentium2
# Pentium III/Celeron2
#OPTIM_FLAGS = -O3 -mfpmath=sse -mmmx -msse -march=pentium3
# Pentium 4
#OPTIM_FLAGS = -O3 -mfpmath=sse -mmmx -msse -msse2 -march=pentium4

# AMD Athlon XP K7
#OPTIM_FLAGS = -O3 -march=athlon-xp -m3dnow -msse -mfpmath=sse


CFLAGS = -Wall -W -Wshadow -Wstrict-prototypes -Wno-unused -Wno-parentheses \
	 -Wno-switch $(OPTIM_FLAGS)

INCLUDE = -I. -I.. -I$(PD_PATH)/src -IC:/msys/1.0/include
LDFLAGS = -shared -LC:/msys/1.0/lib -L$(PD_PATH)/bin -lpd

all: externals


.SUFFIXES: .dll

externals: $(EXTERNALS:.c=.dll)

%.dll: ../src/%.c
	$(CC) -mms-bitfields $(CFLAGS) $(DEFINES) $(INCLUDE) \
		 -o "$*.o" -c "../src/$*.c"
	gcc $(LDFLAGS) -o "$*.dll" "$*.o" \
		`test -f $*.libs && cat $*.libs` \
		`test -f ../src/$*.libs && cat ../src/$*.libs` 
	$(STRIP) "$*.dll"
	rm "$*.o"

clean:
	-rm *.a *.def *.c *.o *.obj
	rm *.dll

install-doc:
	@test -d $(PD_PATH)/doc/5.reference || mkdir -p $(PD_PATH)/doc/5.reference
	cd ../doc && make all
	cp -r ../doc/* $(PD_PATH)/doc/5.reference

install: install-doc
	@test -d $(PD_PATH)/extra || mkdir -p $(PD_PATH)/extra
	install *.dll $(PD_PATH)/extra
