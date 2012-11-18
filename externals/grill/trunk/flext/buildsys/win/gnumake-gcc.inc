##############################################

ifdef LOCKFREEPATH
INCPATH += -I$(LOCKFREEPATH) 
endif

ifdef ATOMICOPSPATH
INCPATH += -I$(ATOMICOPSPATH) 
endif

ifdef SNDOBJ_LIB
INCPATH += -I$(SNDOBJ_INC)
LIBPATH += -L$(SNDOBJ_LIB)
endif

ifdef STK_LIB
INCPATH += -I$(STK_INC) 
LIBPATH += -L$(STK_LIB) 
endif

##############################################

ifndef FLEXTLIB
FLEXTLIB := $(FLEXTPREFIX)/lib
endif
ifndef FLEXTINC
FLEXTINC := $(FLEXTPREFIX)/include/flext
endif
ifndef FLEXTSYS
FLEXTSYS := $(FLEXTPREFIX)/lib/flext
endif

##############################################

LDFLAGS += -dynamic

##############################################

ifdef DEBUG
CFLAGS += -g
LDFLAGS += -g
else
ifdef PROFILE
CFLAGS += -g
LDFLAGS += -g
else
LDFLAGS += -Wl,-S
endif
endif
