DEFS += -DFLEXT_SYS=2 -DPD

INCPATH += -I$(PDPATH)/src -I$(PDPATH)/include -I$(PDPATH)/include/pd

THR=0
ifdef THREADED
THR=1
endif
ifdef SHARED
THR=1
endif

ifeq ($(THR),1)
LIBS += $(PDPATH)/bin/pthreadVC.dll
endif

SYSLIBS += $(PDPATH)/bin/pd.dll 
