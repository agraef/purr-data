# build class specific settings

CMPNAME=$(OUTNAME).$(EXT).$(FLEXTMAJOR).$(FLEXTMINOR)
VERNAME=$(CMPNAME).$(FLEXTMICRO)
TARGET=$(TARGETPATH)/$(VERNAME)
INSTTARGET=$(TARGET)

ifdef SHARED
LDFLAGS += -Wl,-soname,$(CMPNAME) 
endif

##############################################

# default target
_build_: $(TARGET)

$(CSRCS) $(CPPSRCS): $(patsubst %,$(SRCDIR)/%,$(HDRS))
	touch $@

$(TARGETPATH):
	-mkdir -p $@

$(TARGETPATH)/%.opp : $(SRCDIR)/%.cpp
	-mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $(CFLAGS) $(DEFS) $(INCPATH) $< -o $@

$(TARGETPATH)/%.o : $(SRCDIR)/%.c
	-mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEFS) $(INCPATH) $< -o $@

$(TARGET) :: $(TARGETPATH) 

$(TARGET) :: $(COBJS) $(CPPOBJS)
ifdef SHARED
	$(CXX) $(LDFLAGS) $(LIBPATH) -o $@ $(COBJS) $(CPPOBJS) $(LIBS)
	chmod 755 $@
ifndef DEBUG
ifndef PROFILE
	strip --strip-unneeded $@
endif
endif
else
	$(AR) rc $@ $(COBJS) $(CPPOBJS)
endif

##############################################

_clean_:
	-rm -rf $(TARGETPATH)

##############################################

$(FLEXTINC) $(FLEXTLIB) $(FLEXTSYS) $(FLEXTBIN):
	-mkdir -p $@

_install_: $(FLEXTINC) $(FLEXTLIB) $(FLEXTSYS) $(FLEXTBIN)
	install $(TARGET) $(FLEXTLIB)
ifdef SHARED
	/sbin/ldconfig -l $(FLEXTLIB)/$(VERNAME)
	-ln -sf $(CMPNAME) $(FLEXTLIB)/$(OUTNAME).$(EXT)
else
	-ln -sf $(VERNAME) $(FLEXTLIB)/$(TARGETNAME)
endif
# make directories
	for f in $(dir $(patsubst %,$(FLEXTINC)/%,$(HDRS))); do mkdir -p $$f; done
# install headers (each one separately as relative paths might occur)
	for f in $(HDRS); do install $(SRCDIR)/$$f $(FLEXTINC)/$$f; done
# transfer build system
#	cp -rf buildsys $(FLEXTSYS)
#	install build.sh $(FLEXTSYS)
#	chmod -R a+rx $(FLEXTSYS)
# make convenience script
#	echo bash $(FLEXTSYS)/build.sh $$\* > $(FLEXTBIN)/flext-build.sh
#	chmod a+rx $(FLEXTBIN)/flext-build.sh
	
