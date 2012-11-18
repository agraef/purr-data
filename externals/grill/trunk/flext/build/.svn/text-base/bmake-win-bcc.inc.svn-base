!ifdef SIMD
DEFS=$(DEFS) /DFLEXT_USE_SIMD
!endif

!ifdef SNDOBJ
SRCS=$(SRCS) $(SRCS_SNDOBJ)
HDRS=$(HDRS) $(HDRS_SNDOBJ)
INCPATH=$(INCPATH) /I$(SNDOBJ)\include
LIBS=$(LIBS) $(SNDOBJ)\lib\sndobj.lib
!endif

!ifdef STK
SRCS=$(SRCS) $(SRCS_STK)
HDRS=$(HDRS) $(HDRS_STK)
INCPATH=$(INCPATH) /I$(STK)\include
LIBS=$(LIBS) $(STK)\lib\stk.lib
!endif
