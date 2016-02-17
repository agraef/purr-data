DEFS += -DFLEXT_SYS=2 -DPD

INCPATH += -I$(PDPATH)/src -I$(PDPATH)/include -I$(PDPATH)/include/pd
LIBPATH += -L$(PDPATH)/bin

##############################################

OBJPATH=$(OUTPATH)/$(OUTSUB)
TARGETPATH=$(OBJPATH)
TARGETNAME=$(OUTNAME).$(EXT)
