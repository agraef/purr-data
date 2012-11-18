# build class specific settings

INCPATH=$(INCPATH) /I$(FLEXTINC)
LIBPATH=$(LIBPATH) /LIBPATH:$(FLEXTLIB)
LIBS=$(LIBS) $(FLEXTNAME).lib

!ifndef SHARED
!ifdef SNDOBJ
LIBS=$(LIBS) $(SNDOBJ)\lib\SndObj.lib
!endif

!ifdef STK
LIBS=$(LIBS) $(STK)\stk.lib
!endif
!endif

##############################################

# default target
_build_: $(TARGET)

$(OUTPATH):
	if not exist $@ mkdir $@ > nul

$(TARGETPATH): $(OUTPATH)
	if not exist $@ mkdir $@ > nul

{$(SRCDIR)}.cpp{}.obj:
	cl /c $(CFLAGS) $(DEFS) $(INCPATH) $** /Fo$(TARGETPATH)\$@

.cpp.obj:
    @-mkdir $(TARGETPATH)\$(@D:/=\) 2> NUL
	cl /c $(CFLAGS) $(DEFS) $(INCPATH) $(SRCDIR)\$** /Fo$(TARGETPATH)\$@

{$(SRCDIR)}.c{}.obj:
	cl /c $(CFLAGS) $(DEFS) $(INCPATH) $** /Fo$(TARGETPATH)\$@

.c.obj:
    @-mkdir $(TARGETPATH)\$(@D:/=\) 2> NUL
	cl /c $(CFLAGS) $(DEFS) $(INCPATH) $(SRCDIR)\$** /Fo$(TARGETPATH)\$@

$(TARGET):: $(TARGETPATH)

$(TARGET):: $(OBJS)
	@cd $(TARGETPATH)
	link /DLL $(LDFLAGS) $(LIBPATH) $(OBJS) $(LIBS) /out:$(@F)
	@cd ..\..

##############################################
# remove build

_clean_:
	-del /q $(TARGETPATH)\*.* > nul
	-rmdir /q $(TARGETPATH) > nul

##############################################

_install_:
	@if not exist $(INSTPATH) mkdir $(INSTPATH) > nul

	copy $(TARGET) $(INSTPATH) > nul
