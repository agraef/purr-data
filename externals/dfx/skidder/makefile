CFLAGS=-O3 -g -ggdb
#CFLAGS=-O3 -march=athlon-xp -g -ggdb
FLAGS=-DPD

FLEXTPATH=/usr/local/lib/pd/externs/flext
DFXINC=../dfx-library

INCLUDES=/usr/local/lib/pd/include $(FLEXTPATH) $(DFXINC)
FLEXTLIB=$(FLEXTPATH)/flext.a

LIBS=m

PDEXTRA=/usr/local/lib/pd/extra

TARGET=skidder~.pd_linux

SRCS=skidder.cpp ../dfx-library/TempoRateTable.cpp

all: $(TARGET)

%.o : %.cpp %.hpp
	$(CXX) -c $(CFLAGS) $(FLAGS) $(patsubst %,-I%,$(INCLUDES)) $< -o $@

$(DFXINC)/%.o : $(DFXINC)/%.cpp
	$(CXX) -c $(CFLAGS) $(FLAGS) $(patsubst %,-I%,$(INCLUDES)) $< -o $@

$(TARGET) : $(patsubst %.cpp,%.o,$(SRCS)) $(FLEXTLIB) 
	$(CXX) $(LDFLAGS) -shared $^ $(patsubst %,-l%,$(LIBS)) -o $@ 
	strip --strip-unneeded $@
	chmod 755 $@

install: $(TARGET)
	chown root.root $^ 
	cp $^ $(PDEXTRA)

.PHONY: clean
clean:
	rm -f *.o $(TARGET)
	







