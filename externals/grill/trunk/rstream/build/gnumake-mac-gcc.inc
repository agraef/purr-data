ifdef OGGPATH
INCPATH+=-I$(OGGPATH)
endif
ifdef VORBISPATH
INCPATH+=-I$(VORBISPATH)
endif

PREFIX=/usr/local/lib

LIBS=$(PREFIX)/libvorbis.a $(PREFIX)/libvorbisfile.a $(PREFIX)/libogg.a

