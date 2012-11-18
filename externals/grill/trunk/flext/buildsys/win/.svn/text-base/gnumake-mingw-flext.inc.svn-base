# build class specific settings

##############################################

# default target
_build_: $(TARGET)

#$(CSRCS) $(CPPSRCS): $(patsubst %,$(SRCDIR)/%,$(HDRS))
#	touch $@

$(OUTPATH):
	-mkdir $(subst /,\,$@)

$(TARGETPATH): $(OUTPATH)
	-mkdir $(subst /,\,$@)

$(TARGETPATH)/%.opp : $(SRCDIR)/%.cpp
	$(CXX) -c $(CFLAGS) $(DEFS) $(subst \,/,$(INCPATH)) $< -o $(subst /,\,$@)

$(TARGETPATH)/%.o : $(SRCDIR)/%.c
	$(CC) -c $(CFLAGS) $(DEFS) $(subst \,/,$(INCPATH)) $< -o $(subst /,\,$@)

$(TARGET) :: $(TARGETPATH)

$(TARGET) :: $(COBJS) $(CPPOBJS)
ifdef SHARED
	$(CXX) $(LDFLAGS) $(LIBPATH) -o $(subst /,\,$@) $(subst /,\,$(COBJS) $(CPPOBJS) $(LIBS))
	-strip --strip-unneeded $@
else
	$(AR) rc $@ $(COBJS) $(CPPOBJS)
endif

##############################################

_clean_:
	-del /q $(subst /,\,$(TARGETPATH)/*.*) > nul
	-rmdir /q $(subst /,\,$(TARGETPATH)) > nul

##############################################

ifdef SHARED
FLEXTLIBINST=$(FLEXTSHLIB)
else
FLEXTLIBINST=$(FLEXTLIB)
endif

_install_::
	@if not exist $(subst /,\,$(FLEXTINC)) -mkdir $(subst /,\,$(FLEXTINC))
	@if not exist $(subst /,\,$(FLEXTLIBINST)) -mkdir $(subst /,\,$(FLEXTLIBINST))

	copy $(subst /,\,$(TARGET) $(FLEXTLIBINST))
	-for %%i in ($(HDRS)) do @copy $(SRCDIR)\%%i $(subst /,\,$(FLEXTINC)) > nul
