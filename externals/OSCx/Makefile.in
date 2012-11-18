dirs = libOSC send+dump  src

all: 
	cd libOSC && make
	cd src && make

lib: all
	cd src && make lib

utils:
	cd send+dump && make

install: all lib utils
	cd src && make install

clean:
	cd libOSC && make clean
	cd send+dump && make clean
	cd src && make clean

distclean: clean
	-rm *~
	rm Makefile */Makefile
	rm config.log config.status config.h
	-rm -Rf ./autom4te.cache/ extra

