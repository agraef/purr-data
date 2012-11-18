# build class specific settings

INCPATH += -I$(FLEXTINC)
LIBPATH += -L$(FLEXTLIB) -L$(FLEXTSHLIB)
LIBS += -l$(FLEXTNAME)

##############################################

# default target
_build_: $(TARGET)

$(CSRCS) $(CPPSRCS): $(patsubst %,$(SRCDIR)/%,$(HDRS))
	touch $@

$(TARGETPATH):
	-mkdir -p $@

$(TARGETPATH)/%.opp : $(SRCDIR)/%.cpp
	$(CXX) -c $(CFLAGS) $(DEFS) $(INCPATH) $< -o $@

$(TARGETPATH)/%.o : $(SRCDIR)/%.c
	$(CC) -c $(CFLAGS) $(DEFS) $(INCPATH) $< -o $@

$(TARGET) :: $(TARGETPATH) 

$(TARGET) :: $(COBJS) $(CPPOBJS)
	$(CXX) $(LDFLAGS) $(LIBPATH) -o $@ $(COBJS) $(CPPOBJS) $(LIBS) $(SYSLIBS)
	strip --strip-unneeded $@
	chmod 755 $@

##############################################

_clean_:
	rm $(COBJS) $(CPPOBJS) $(TARGET)

##############################################

$(INSTPATH):
	-mkdir -p $(INSTPATH)

_install_: $(INSTPATH)
	install $(TARGET) $(INSTPATH)
