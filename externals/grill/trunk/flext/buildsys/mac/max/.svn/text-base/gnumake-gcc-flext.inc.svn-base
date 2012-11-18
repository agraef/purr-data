ifdef SHARED
EXT=dylib
else
EXT=a
endif

##############################################

LDFLAGS += -dynamiclib

##############################################

OBJPATH=$(OUTPATH)/$(OUTSUB)
TARGETPATH=$(OBJPATH)

TARGETNAME=$(OUTNAME).$(EXT)
VERNAME=$(OUTNAME).$(FLEXTMAJOR).$(FLEXTMINOR).$(FLEXTMICRO).$(EXT)
TARGET=$(TARGETPATH)/$(VERNAME)

INSTTARGET=$(TARGET)
