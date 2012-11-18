# set Name and system

NAME=algocomp
#SYSTEM = nt
SYSTEM = linux
# set paths for windows

# where is VC++ ???
VC="C:\Programme\Microsoft Visual C++ Toolkit 2003"
VCLINK="C:\Programme\Microsoft Visual C++ Toolkit 2003\bin\link"
# where is pd
#PDPATH="E:\Program Files\pd-0.38-3"
PDPATH=C:\Programme\pd-0.38-3
LIBPATH="$(PDPATH)/bin"

install: install_$(SYSTEM)

all: $(SYSTEM)
#all: pd_linux install_linux

# ----------------------- LINUX i386 -----------------------

linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -DUNIX -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes -Werror \
    -Wno-unused -Wno-parentheses -Wno-switch

# where is your m_pd.h ???
LINUXINCLUDE =  -I../../src 

.c.pd_linux:
	cc -O2 -Wall -DPD -fPIC $(LINUXCFLAGS) $(LINUXINCLUDE) -c $*.c 
	ld --export-dynamic  -shared -o $*.pd_linux $*.o -lc 
	strip --strip-unneeded $*.pd_linux 

install_linux:
	sudo cp *.pd_linux /usr/lib/pd/extra/
	cp *.pd_linux ../../download

# ----------------------- NT -----------------------

nt: $(NAME).dll

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /O2 /G6 /DNT /DPD /nologo
PDNTINCLUDE = /I. /I$(PDPATH)\tcl\include /I$(PDPATH)\src /I$(VC)\include /Iinclude
PDNTLDIR = "C:\Programme\Microsoft Visual C++ Toolkit 2003\Lib"

PDNTLIB = "C:\Programme\Microsoft Visual C++ Toolkit 2003\Lib\libc.lib" \
	"C:\Programme\Microsoft Visual C++ Toolkit 2003\Lib\oldnames.lib" \
	"C:\Programme\Microsoft Visual C++ Toolkit 2003\Lib\kernel32.lib" \
	"$(PDPATH)\bin\pd.lib"


.c.dll: algocomp.c chaotic.c ifs.c selfsimilar.c chaossignals.c pan.c
	cl /W3 /WX /O2 /G6 /DNT /DPD /nologo /I. /I"C:\Programme\flextbuild\pd\tcl\include" /I"C:\Programme\flextbuild\pd\src" /I"C:\Programme\Microsoft Visual C++ Toolkit 2003\include" /Iinclude /c $(NAME).c
	link /dll /export:$(NAME)_setup $*.obj $(PDNTLIB)

#.c.dll:
#	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
#	link /dll /export:$(CSYM)_setup $*.obj $(PDNTLIB)
	
install_nt:
	cp $(NAME).dll $(PDPATH)\externs
# -------------------------- XP --------------------------

# $(CXX) $(LDFLAGS) -shared $(patsubst %,-L%,$(LIBPATH)) $^ $(patsubst %,-l%,$(LIBS)) -o $@

pd_xp:
	gcc -O2 -Wall -DPD -L$(LIBPATH) -c $(NAME).c 
	ld --export-dynamic -shared -o $(NAME).dll -lc 


install_xp:
#	copy $(NAME).dll $(PDPATH)/externs
	cp $(NAME).dll $(PDPATH)/externs

clean:
	rm -f *.o *.dll *.exp *.obj *.lib
	rm -f *.o *.pd_* *.dll so_locations
	
