DEFS += -DFLEXT_SYS=2 -DPD

ifdef WINNT
DEFS += -D_WIN32_WINNT=0x500
endif

##############################################
# let the pthreads path be first, because pthreads headers could also be a $(PDPATH)/src

INCPATH += -I$(PTHREADSINC)

THR=0
ifdef THREADED
THR=1
endif
ifdef SHARED
THR=1
endif

ifeq ($(THR),1)
ifeq ($(PTHREADSVERSION),2)
LIBS += $(PTHREADSLIB)/pthreadVC2.dll
else
LIBS += $(PTHREADSLIB)/pthreadVC.dll
endif
endif

##############################################

INCPATH += -I$(PDPATH)/src

##############################################

OBJPATH=$(OUTPATH)/$(OUTSUB)
TARGETPATH=$(OBJPATH)
TARGETNAME=$(OUTNAME).$(EXT)

LIBS += $(PDPATH)/bin/pd.dll
