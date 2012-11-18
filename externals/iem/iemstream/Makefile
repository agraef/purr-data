# stream - stream externals for PD
# 
# Makefile for gcc @ linux
#
# usage:
# to build run "make -f Makefile.linux"
# to install (as root), do "make -f Makefile.linux install"
#

# Configuration: ##########################


# where are the PD header files?
# leave it blank if it is a system directory (like /usr/local/include), 
# since gcc 3.2 complains about it
PDPATH=

# where should flext libraries be built?
TARGDIR=./pd-linux

# where should the external be installed?
# (leave blank to omit installation)
INSTPATH=/usr/local/lib/pd/extra

# additional compiler flags
# (check if they fit for your system!)
# UFLAGS=-mcpu=pentiumpro  # gcc 2.95
UFLAGS=-mcpu=pentium3 -msse  # gcc 3.xx

###########################################

# compiler+linker stuff	
INCLUDES=$(PDPATH)
LIBPATH=
FLAGS=-DPD ${U_FLAGS}
CFLAGS=-O6 
#CFLAGS=-g
LIBS=m util ogg vorbis vorbisenc vorbisfile samplerate 


# ---------------------------------------------
# the rest can stay untouched
# ----------------------------------------------

NAME=stream

# all the source files from the package
DIR=.
SRCS=main.cpp socket.cpp stream.cpp streamogg.cpp

MAKEFILE=Makefile

TARGET=$(TARGDIR)/$(NAME).pd_linux

# default target
all: $(TARGDIR) $(TARGET)

$(patsubst %,$(DIR)/%,$(SRCS)): $(patsubst %,$(DIR)/%,$(HDRS)) $(MAKEFILE)
	touch $@

$(TARGDIR):
	mkdir $(TARGDIR)

$(TARGDIR)/%.o : $(DIR)/%.cpp
	$(CXX) -c $(CFLAGS) $(FLAGS) $(patsubst %,-I%,$(INCLUDES)) $< -o $@

$(TARGET) : $(patsubst %.cpp,$(TARGDIR)/%.o,$(SRCS))
	$(CXX) -shared $^ $(patsubst %,-L%,$(LIBPATH)) $(patsubst %,-l%,$(LIBS)) -o $@ 
	chmod 755 $@

$(INSTPATH):
	mkdir $(INSTPATH)

install:: $(INSTPATH)

install:: $(TARGET)
	cp $^ $(INSTPATH)
	chown root.root  $(patsubst %,$(INSTPATH)/%,$(notdir $^))
	chmod 755 $(patsubst %,$(INSTPATH)/%,$(notdir $^))

.PHONY: clean
clean:
	rm -f $(TARGDIR)/*.o $(TARGET)
