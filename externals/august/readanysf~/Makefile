# Edit these two variables to suit your system.
# You need both gavl and gmerlin_avdec libs to compile
#
GAVLPATH=/usr/local/include
PDPATH=/usr/local/include


VERSION=0.42
UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
TARGET=pd_linux
else
# assume darwin here
GAVLPATH=/sw/include
PDPATH=/Applications/Pd-extended.app/Contents/Resources/include/
TARGET=pd_darwin
endif

##############################################
LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
   # do 64 bit stuff here, like set some CFLAGS
CFLAGS =  -fPIC -I./  -I$(GAVLPATH) -I$(GAVLPATH)/gavl -I$(GAVLPATH)/gmerlin -I$(PDPATH) -Wall
else
   # do 32 bit stuff here
CFLAGS =  -I./  -I$(GAVLPATH)  -I$(GAVLPATH)/gavl -I$(GAVLPATH)/gmerlin -I$(PDPATH) -Wall
endif


ifeq ($(UNAME), Linux)
STRIP=strip --strip-unneeded
# optimizations?
#CFLAGS += -O1 -funroll-loops -fomit-frame-pointer \
#    -Wall -W -Wshadow \
#    -Wno-unused -Wno-parentheses -Wno-switch
LDFLAGS =  -L/usr/local/lib -lpthread  -lgavl -lgmerlin_avdec 
else
# assume darwin here
STRIP=strip -x
CFLAGS += -I/sw/include -fast -fPIC
LDFLAGS = -bundle -undefined dynamic_lookup -L/sw/lib -lgavl -lgmerlin_avdec
#LDFLAGS += -bundle -bundle_loader $(pd_src)/bin/pd -undefined dynamic_lookup \
#		-L/sw/lib -weak_framework Carbon -lc -L/sw/lib -lgavl -lgmerlin_avdec  
# os 10.4
#CFLAGS += -mmacosx-version-min=10.4  -arch i386  -isysroot /Developer/SDKs/MacOSX10.4u.sdk 
#LDFLAGS =  -L/sw/lib -lgavl -lgmerlin_avdec \
#        -dynamiclib -undefined dynamic_lookup  -lsupc++ -mmacosx-version-min=10.4 \
#        -lSystem.B -arch i386  -isysroot /Developer/SDKs/MacOSX10.4u.sdk 
endif




all: $(TARGET) 

pd_linux: src/readanysf~.cpp  objs/FifoVideoFrames.o objs/FifoAudioFrames.o objs/ReadMedia.o
	g++  -shared  -o  readanysf~.pd_linux  $(CFLAGS) $(LDFLAGS) \
	src/readanysf~.cpp \
	objs/FifoAudioFrames.o \
	objs/FifoVideoFrames.o \
	objs/ReadMedia.o 
	$(STRIP) readanysf~.pd_linux

pd_darwin: src/readanysf~.cpp  objs/FifoVideoFrames.o objs/FifoAudioFrames.o objs/ReadMedia.o
	g++  $(LDFLAGS)  -o  readanysf~.pd_darwin  $(CFLAGS)  \
	src/readanysf~.cpp \
	objs/FifoAudioFrames.o \
	objs/FifoVideoFrames.o \
	objs/ReadMedia.o 
	$(STRIP) readanysf~.pd_darwin
	mkdir -p readanysf~$(VERSION)_MacOSX-Intel
	mkdir -p readanysf~$(VERSION)_MacOSX-Intel/readanysf~
	cp readanysf~.pd_darwin readanysf~-help.pd readanysf~$(VERSION)_MacOSX-Intel/readanysf~
	cp READMEmacpkg.txt anysndfiler.pd readanysf~$(VERSION)_MacOSX-Intel/
	mv readanysf~$(VERSION)_MacOSX-Intel/READMEmacpkg.txt readanysf~$(VERSION)_MacOSX-Intel/README.txt
	./embed-MacOSX-dependencies.sh readanysf~$(VERSION)_MacOSX-Intel/readanysf~
	tar -cvf readanysf~$(VERSION)_MacOSX-Intel.tar readanysf~$(VERSION)_MacOSX-Intel/
	gzip readanysf~$(VERSION)_MacOSX-Intel.tar

objs/ReadMedia.o: src/ReadMedia.cpp src/ReadMedia.h objs/FifoAudioFrames.o objs/FifoVideoFrames.o
	g++  -c -o objs/ReadMedia.o src/ReadMedia.cpp $(CFLAGS)

objs/FifoAudioFrames.o: src/FifoAudioFrames.cpp src/FifoAudioFrames.h 
	g++  -c -o objs/FifoAudioFrames.o src/FifoAudioFrames.cpp $(CFLAGS)

objs/FifoVideoFrames.o: src/FifoVideoFrames.cpp src/FifoVideoFrames.h 
	g++  -c -o objs/FifoVideoFrames.o src/FifoVideoFrames.cpp $(CFLAGS)

clean:
	if [ -d readanysf~$(VERSION)_MacOSX-Intel ]; then rm -rf readanysf~$(VERSION)_MacOSX-Intel; fi; 
	if [ -f readanysf~$(VERSION)_MacOSX-Intel.tar.gz ]; then rm -rf readanysf~$(VERSION)_MacOSX-Intel.tar.gz; fi; 
	rm -f objs/*.o readanysf~.pd_*
