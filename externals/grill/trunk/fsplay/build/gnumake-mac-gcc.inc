ifdef SNDFILEPATH
INCPATH=-I$(SNDFILEPATH)
endif

ifdef BOOSTPATH
INCPATH=-I$(BOOSTPATH)
endif

LIBS=-lsndfile -framework Quicktime
