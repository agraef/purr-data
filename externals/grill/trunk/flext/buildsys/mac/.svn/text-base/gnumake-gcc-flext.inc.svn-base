# build class specific settings

ifdef SHARED
LDFLAGS += -install_name $(FLEXTLIB)/$(TARGETNAME)
LDFLAGS += -current_version $(FLEXTMAJOR).$(FLEXTMINOR).$(FLEXTMICRO)
LDFLAGS += -compatibility_version $(FLEXTMAJOR).$(FLEXTMINOR)
LDFLAGS += -preload -seg1addr 0xd0000000
endif

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
	libtool -static -o $@ $(COBJS) $(CPPOBJS)
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
ifdef FLEXTFRAMEWORK
# --- install as framework ---
	mkdir -p $(FLEXTFRAMEWORK)/Versions/$(FLEXTMAJOR).$(FLEXTMINOR)/Headers
	ln -sfh ./$(FLEXTMAJOR).$(FLEXTMINOR) $(FLEXTFRAMEWORK)/Versions/Current
	ln -sfh ./Versions/Current/Headers $(FLEXTFRAMEWORK)/Headers
	install $(TARGET) $(FLEXTFRAMEWORK)/Versions/Current
ifdef SHARED
	# make link without lib... and extension
	ln -sf ./Versions/Current/$(VERNAME) $(FLEXTFRAMEWORK)/$(patsubst lib%,%,$(basename $(TARGETNAME)))
else
	ranlib $(FRAMEWORK)/Versions/Current/$(VERNAME) 
	ln -sf ./Versions/Current/$(VERNAME) $(FLEXTFRAMEWORK)/$(TARGETNAME)
endif
	install $(patsubst %,$(SRCDIR)/%,$(HDRS)) $(FLEXTFRAMEWORK)/Headers
else
# --- install as dynamic library ---
	install $(TARGET) $(FLEXTLIB)
ifndef SHARED
# have to rerun ranlib at install dir ?!
	ranlib $(FLEXTLIB)/$(VERNAME)  
endif
ifdef VERNAME
	ln -sf $(VERNAME) $(FLEXTLIB)/$(TARGETNAME)
endif
# make directories
	for f in $(dir $(patsubst %,$(FLEXTINC)/%,$(HDRS))); do mkdir -p $$f; done
# install headers (each one separately as relative paths might occur)
	for f in $(HDRS); do install $(SRCDIR)/$$f $(FLEXTINC)/$$f; done
endif
# transfer build system
#	cp -rf buildsys $(FLEXTSYS)
#	install build.sh $(FLEXTSYS)
#	chmod -R a+rx $(FLEXTSYS)
# make convenience script
#	echo bash $(FLEXTSYS)/build.sh $$\* > $(FLEXTBIN)/flext-build.sh
#	chmod a+rx $(FLEXTBIN)/flext-build.sh
