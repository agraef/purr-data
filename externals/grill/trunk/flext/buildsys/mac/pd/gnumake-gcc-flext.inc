ifdef SHARED
EXT=dylib
else
EXT=a
endif

LDFLAGS += -dynamiclib -flat_namespace -undefined suppress

VERNAME=$(OUTNAME).$(FLEXTMAJOR).$(FLEXTMINOR).$(FLEXTMICRO).$(EXT)
TARGET=$(TARGETPATH)/$(VERNAME)
INSTTARGET=$(TARGET)
