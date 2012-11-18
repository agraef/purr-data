##############################################

CSRCS := $(patsubst %.c,$(SRCDIR)/%.c,$(filter %.c,$(SRCS)))
CPPSRCS := $(patsubst %.cpp,$(SRCDIR)/%.cpp,$(filter %.cpp,$(SRCS)))
COBJS := $(patsubst %.c,$(OBJPATH)/%.o,$(filter %.c,$(SRCS))) 
CPPOBJS := $(patsubst %.cpp,$(OBJPATH)/%.opp,$(filter %.cpp,$(SRCS))) 

##############################################

# default target
_build_: $(TARGET)

$(CSRCS) $(CPPSRCS): $(patsubst %,$(SRCDIR)/%,$(HDRS))
	touch $@

# Attention: $@ doesn't work for paths with spaces....
$(OBJPATH):
	-mkdir -p $(OBJPATH)

# Attention: $@ doesn't work for paths with spaces....
$(TARGETPATH):
	-mkdir -p $(TARGETPATH)

##### precompiled header file ###################

ifdef PRECOMPILE
PRECOMSRC := $(SRCDIR)/$(PRECOMPILE)
else
PRECOMSRC := $(FLEXTINC)/flext.h
endif

ifdef PRECOMSRC
PRECOMDST := $(OBJPATH)/$(notdir $(PRECOMSRC)).gch
PRECOMINC := -I $(OBJPATH)
endif

$(PRECOMDST) : $(PRECOMSRC) $(patsubst %,$(SRCDIR)/%,$(HDRS))
	-mkdir -p $(dir $@)
	$(CXX) -c $(DEFS) $(INCPATH) $(PRECOMSRC) -o $@


$(OBJPATH)/%.opp : $(SRCDIR)/%.cpp
	-mkdir -p $(dir $@)
	$(CXX) -c $(CFLAGS) $(DEFS) $(PRECOMINC) $(INCPATH) $^ -o $@

$(OBJPATH)/%.o : $(SRCDIR)/%.c
	-mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEFS) $(INCPATH) $^ -o $@

##############################################

_clean_:
	-rm -r $(TARGETPATH)
	-rm -r $(OBJPATH)

##############################################
