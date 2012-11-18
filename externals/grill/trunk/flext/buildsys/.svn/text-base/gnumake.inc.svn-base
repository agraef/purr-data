##############################################
# cross-compilation
##############################################

#ifndef ARCH
#ifeq ("$(shell uname -p)","powerpc")
#ARCH := ppc
#else
#ARCH := $(shell uname -p)
#endif
#endif

##############################################
# some compiler definitions
##############################################

CFLAGS += $(UFLAGS)

$(foreach arch,$(ARCH),$(eval CFLAGS_$(arch) += $$(UFLAGS_$(arch)) ) )


ifdef DEBUG

DEFS += -D_DEBUG
CFLAGS += $(DFLAGS)

define ARCH_FLAGS
CFLAGS_$(1) += $$(DFLAGS_$(1))
endef

else  ## profile and release

DEFS += -DNDEBUG
CFLAGS += $(OFLAGS) 

define ARCH_FLAGS
CFLAGS_$(1) += $$(OFLAGS_$(1))
endef

endif

$(foreach arch,$(ARCH),$(eval $(call ARCH_FLAGS,$(arch))))


ifdef SHARED
# --- shared ---
DEFS += -DFLEXT_SHARED

else
ifdef THREADED
# --- static multi-threaded ---
DEFS += -DFLEXT_THREADS

else 
# --- static single-threaded ---

endif
endif

##############################################
# name of flext library
##############################################

ifdef SHARED
TYPEEXT := _
else
ifdef THREADED
TYPEEXT := _t
else
TYPEEXT := _s
endif
endif

ifdef DEBUG
MODEEXT := d
else
ifdef PROFILE
MODEEXT := p
else
MODEEXT :=
ifdef SHARED
TYPEEXT :=
endif
endif
endif

FLEXTNAME := flext-$(RTSYS)$(TYPEEXT)$(MODEEXT)

##############################################
# product name and folder
##############################################

ifndef SRCDIR
SRCDIR := .
endif

OUTSUB := $(TARGETMODE)-$(TARGETTYPE)
