# build class specific settings

INCPATH += -I$(FLEXTINC)
LIBPATH += -L$(FLEXTLIB) -L$(FLEXTSHLIB)

ifdef SHARED
LIBS += $(FLEXTSHLIB)/lib$(FLEXTNAME).dll
else
LIBS += -l$(FLEXTNAME)
endif

ifndef SHARED
ifdef SNDOBJ
LIBS += -lSndObj
endif

ifdef STK
LIBS += -lStk
endif
endif

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
	$(CXX) $(LDFLAGS) $(subst \,/,$(LIBPATH)) -o $(subst /,\,$@) $(subst /,\,$(COBJS) $(CPPOBJS) $(LIBS) $(SYSLIBS))
	-strip --strip-unneeded $@

##############################################

_clean_:
	-del /q $(subst /,\,$(TARGETPATH)/*.*) > nul
	-rmdir /q $(subst /,\,$(TARGETPATH)) > nul

##############################################

_install_:
	@if not exist $(subst /,\,$(INSTPATH)) -mkdir $(subst /,\,$(INSTPATH))
	copy $(subst /,\,$(TARGET) $(INSTPATH))
