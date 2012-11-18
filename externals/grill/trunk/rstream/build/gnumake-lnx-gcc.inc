ifdef OGGPATH
INCPATH+=-I$(OGGPATH)
endif
ifdef VORBISPATH
INCPATH+=-I$(VORBISPATH)
endif


LIBS=-lvorbis -lvorbisfile -logg
