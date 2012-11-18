# build class specific settings

#ifdef SHARED
#LDFLAGS += -install_name $(FLEXTLIB)/$(TARGETNAME)
#LDFLAGS += -current_version $(FLEXTMAJOR).$(FLEXTMINOR).$(FLEXTMICRO)
#LDFLAGS += -compatibility_version $(FLEXTMAJOR).$(FLEXTMINOR)
#LDFLAGS += -preload -seg1addr 0xd0000000
#endif

# common compilation stuff
include $(BUILDPATH)$(PLATFORM)/gnumake-gcc-targets.inc

##### linking ###################

$(TARGET) :: $(OBJPATH) $(TARGETPATH)

$(TARGET) :: $(PRECOMDST) $(COBJS) $(CPPOBJS)
ifdef SHARED
	$(CXX) $(LDFLAGS) $(LIBPATH) -o $@ $(COBJS) $(CPPOBJS) $(LIBS)
ifdef DEBUG
else
ifdef PROFILE
else
	strip -x $@
endif
endif
	chmod 755 $@
else
	$(AR) rc $@ $(COBJS) $(CPPOBJS)
endif

ifdef TARGETPOST
$(TARGET) :: $(TARGETPOST)
endif

###################################

$(FLEXTINC):
	-mkdir -p $@

$(FLEXTLIB):
	-mkdir -p $@

_install_: $(FLEXTINC) $(FLEXTLIB)
# --- install as dynamic library ---
	install $(TARGET) $(FLEXTLIB)
ifdef VERNAME
	ln -sf $(VERNAME) $(FLEXTLIB)/$(TARGETNAME)
endif
	install $(patsubst %,$(SRCDIR)/%,$(HDRS)) $(FLEXTINC)
# transfer build system
#	cp -rf buildsys $(FLEXTSYS)
#	install build.sh $(FLEXTSYS)
#	chmod -R a+rx $(FLEXTSYS)
# make convenience script
#	echo bash $(FLEXTSYS)/build.sh $$\* > $(FLEXTBIN)/flext-build.sh
#	chmod a+rx $(FLEXTBIN)/flext-build.sh
