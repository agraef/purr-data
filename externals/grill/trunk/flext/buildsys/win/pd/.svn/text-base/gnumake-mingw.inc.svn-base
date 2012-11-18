DEFS += -DFLEXT_SYS=2 -DPD

ifdef WINNT
CFLAGS += -D_WIN32_WINNT=0x500
endif

##############################################
# let the pthreads path be first, because pthreads headers could also be a $(PDPATH)/src

INCPATH += -I$(PTHREADSINC)
LIBPATH += -L$(PTHREADSLIB)

ifdef THREADED
ifeq ($(PTHREADSVERSION),2)
LIBS +=	-lpthreadVC2
else
LIBS +=	-lpthreadVC
endif
endif

##############################################

INCPATH += -I$(PDPATH)/src
LIBPATH += -L$(PDPATH)/bin

LIBS += -lpd

