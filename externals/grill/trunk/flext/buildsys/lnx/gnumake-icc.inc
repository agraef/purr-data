##############################################

ifndef FLEXTINC
FLEXTINC=$(FLEXTPREFIX)/include/flext
endif
ifndef FLEXTLIB
FLEXTLIB=$(FLEXTPREFIX)/lib
endif
ifndef FLEXTSYS
FLEXTSYS=$(FLEXTPREFIX)/lib/flext
endif
ifndef FLEXTBIN
FLEXTBIN=$(FLEXTPREFIX)/bin
endif

##############################################

OBJPATH=$(OUTPATH)/$(OUTSUB)
TARGETPATH=$(OBJPATH)
TARGETNAME=$(OUTNAME).$(EXT)

##############################################

CFLAGS +=
LDFLAGS += -shared -Wl,-x 

##############################################

ifdef DEBUG
CFLAGS += -g
LDFLAGS += -g
else
ifdef PROFILE
CFLAGS += -g -qp
LDFLAGS += -g -qp
else
LDFLAGS += -Wl,-S
endif
endif

##############################################

ifdef STK_INC
INCPATH += -I$(STK_INC)
endif
ifdef STK_LIB
LIBS += $(STK_LIB)
endif

##############################################

ifdef SNDOBJ_INC
INCPATH += -I$(SNDOBJ_INC)
endif
ifdef SNDOBJ_LIB
LIBS += $(SNDOBJ_LIB)
endif

##############################################

CSRCS=$(patsubst %.c,$(SRCDIR)/%.c,$(filter %.c,$(SRCS)))
CPPSRCS=$(patsubst %.cpp,$(SRCDIR)/%.cpp,$(filter %.cpp,$(SRCS)))
COBJS=$(patsubst %.c,$(OBJPATH)/%.o,$(filter %.c,$(SRCS))) 
CPPOBJS=$(patsubst %.cpp,$(OBJPATH)/%.opp,$(filter %.cpp,$(SRCS))) 
