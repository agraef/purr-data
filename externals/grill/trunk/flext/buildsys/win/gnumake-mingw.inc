##############################################

OBJPATH=$(OUTPATH)\$(OUTSUB)
TARGETPATH=$(OBJPATH)
TARGET=$(TARGETPATH)\$(OUTNAME).$(EXT)

##############################################

CFLAGS += -mms-bitfields -mno-cygwin
LDFLAGS += -shared -mno-cygwin 

# default Windows flags as defined from VC IDE
CFLAGS += -DWIN32 -D_WINDOWS

##############################################

ifdef DEBUG
CFLAGS += -g
else
ifdef PROFILE
CFLAGS += -g -pg
LDFLAGS += -pg
else
LDFLAGS += -Wl,-S
endif
endif

##############################################

CSRCS=$(patsubst %.c,$(SRCDIR)/%.c,$(filter %.c,$(SRCS)))
CPPSRCS=$(patsubst %.cpp,$(SRCDIR)/%.cpp,$(filter %.cpp,$(SRCS)))
COBJS=$(patsubst %.c,$(OBJPATH)/%.o,$(filter %.c,$(SRCS))) 
CPPOBJS=$(patsubst %.cpp,$(OBJPATH)/%.opp,$(filter %.cpp,$(SRCS))) 
