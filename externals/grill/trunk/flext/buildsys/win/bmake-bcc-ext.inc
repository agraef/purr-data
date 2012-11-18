# build class specific settings

INCPATH=$(INCPATH) -I$(FLEXTINC)
LIBPATH=$(LIBPATH) -L$(FLEXTLIB)
LIBS=$(LIBS) $(FLEXTNAME).lib

!ifndef SHARED
!ifdef SNDOBJ
LIBS=$(LIBS) $(SNDOBJ)\lib\SndObj.lib
!endif

!ifdef STK
LIBS=$(LIBS) $(STK)\src\Stk.obj
!endif
!endif

##############################################

# default target
_build_: $(TARGET) 

$(OUTPATH): 
	if not exist $@ mkdir $@ > nul

$(TARGETPATH): $(OUTPATH)
	if not exist $@ mkdir $@ > nul

SETUPFUNCTION=$(NAME)_setup

$(TARGETPATH)\$(NAME).def: $(TARGETPATH)
	@echo EXPORTS $(SETUPFUNCTION) = _$(SETUPFUNCTION) > $@
# this next line fixes a strange problem with implib - lacking underscore?!
	@echo IMPORTS _rtext_retext=PD.rtext_retext >> $@

$(TARGET) :: $(TARGETPATH)\$(NAME).def

{$(SRCDIR)}.cpp{}.obj:
	bcc32 -c $(CFLAGS) $(DEFS) $(INCPATH) -n$(TARGETPATH) $<

{$(SRCDIR)}.c{}.obj:
	bcc32 -c $(CFLAGS) $(DEFS) $(INCPATH) -n$(TARGETPATH) $<

$(TARGET) :: $(TARGETPATH)

$(TARGET) :: $(OBJS)
	@cd $(TARGETPATH)
	ilink32 $(LDFLAGS) $(LIBPATH) -L. $(OBJS) ,$(<F),,$(LIBS),$(NAME).def
	@cd ..\..

##############################################
# remove build

_clean_:
	-del /q $(TARGETPATH)\*.* > nul
	-rmdir /q $(TARGETPATH) > nul

##############################################

$(INSTPATH):
	if not exist $@ mkdir $@ > nul

_install_: $(INSTPATH) 
	copy $(TARGET) $(INSTPATH) > nul
