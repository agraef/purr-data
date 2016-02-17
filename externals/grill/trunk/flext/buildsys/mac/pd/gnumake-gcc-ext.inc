EXT=pd_darwin

#LDFLAGS += -bundle -bundle_loader $(PDBIN) 

# it seems we are having symbol name clashes with the following "new preferred method"
# so we rather stick to the above "deprecated method"
LDFLAGS += -dynamiclib -undefined dynamic_lookup 

TARGET=$(TARGETPATH)/$(TARGETNAME)
INSTTARGET=$(TARGET)
