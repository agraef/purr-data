# build class specific settings

LDFLAGS += -framework ApplicationServices

INCPATH += -I$(FLEXTINC)
LIBPATH += -L$(FLEXTLIB)
LIBS += -l$(FLEXTNAME)

# common compilation stuff
include $(BUILDPATH)$(PLATFORM)/gnumake-gcc-targets.inc

##### linking ###################

$(TARGET) :: $(OBJPATH) $(TARGETPATH)

$(TARGET) :: $(PRECOMDST) $(COBJS) $(CPPOBJS)
	$(CXX) $(LDFLAGS) $(LIBPATH) -o $@ $(COBJS) $(CPPOBJS) $(LIBS)
ifdef DEBUG
else
ifdef PROFILE
else
	strip -x $@
endif
endif
	chmod 755 $@

ifdef TARGETPOST
$(TARGET) :: $(TARGETPOST)
endif

###################################

# Attention: $@ doesn't work for paths with spaces....
$(INSTPATH):
	-mkdir -p $(INSTPATH)

_install_:: $(INSTPATH)
# copy plain file or whole bundle
	cp -R $(INSTTARGET) $(INSTPATH)
