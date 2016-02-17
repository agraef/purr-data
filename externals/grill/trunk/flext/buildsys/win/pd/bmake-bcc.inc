DEFS=$(DEFS) /DFLEXT_SYS=2 /DPD

!ifdef WINNT
DEFS=$(DEFS) /D_WIN32_WINNT=0x500
!endif

##############################################
# let the pthreads path be first, because pthreads headers could also be a $(PDPATH)/src

INCPATH=$(INCPATH) -I$(PTHREADSINC)
LIBPATH=$(LIBPATH) -L$(PTHREADSLIB)

!ifdef THREADED
!if $(PTHREADSVERSION) == 2
LIBS=$(LIBS) pthreadVC2.lib
!else
LIBS=$(LIBS) pthreadVC.lib
!endif
!endif

##############################################

INCPATH=$(INCPATH) -I$(PDPATH)\src -I$(PDPATH)\include -I$(PDPATH)\include\pd

LIBS=$(LIBS) pd.lib

