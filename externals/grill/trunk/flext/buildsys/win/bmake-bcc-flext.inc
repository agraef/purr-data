# build class specific settings

!ifdef SHARED

!ifdef SNDOBJ
LIBS=$(LIBS) $(SNDOBJ)\lib\SndObj.lib
SRCS=$(SRCS) $(SRCS_SNDOBJ)
!endif

!ifdef STK
LIBS=$(LIBS) $(STK)\src\Stk.obj
SRCS=$(SRCS) $(SRCS_STKOBJ)
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
	bcc32 -c $(CFLAGS) $(DEFS) $(INCPATH) -n$(TARGETPATH) $<

{$(SRCDIR)}.c{}.obj:
	bcc32 -c $(CFLAGS) $(DEFS) $(INCPATH) -n$(TARGETPATH) $<


$(TARGETPATH)\pd.lib : $(PDPATH)\bin\pd.dll
	implib -a $< $**

$(TARGETPATH)\pthreadVC.lib : $(PDPATH)\bin\pthreadVC.dll
	implib -a $< $**

PDTARGETS=$(TARGETPATH)\pd.lib $(TARGETPATH)\pthreadVC.lib


$(TARGET) :: $(TARGETPATH) $(PDTARGETS)

$(TARGET) :: $(OBJS)
	@cd $(TARGETPATH)
!ifdef SHARED
	ilink32 $(LDFLAGS) $(LIBPATH) -L. $(OBJS) ,$(<F),,$(LIBS)
	implib -a $(<B).lib $(<F)
!else
	-del $(<F)
	tlib "$(<F)" +$(OBJS: = +)
!endif
	@cd ..\..

##############################################

_clean_:
	-del /q $(TARGETPATH)\*.* > nul
	-rmdir /q $(TARGETPATH) > nul

##############################################

!ifdef SHARED
FLEXTLIBINST=$(FLEXTSHLIB)
!else
FLEXTLIBINST=$(FLEXTLIB)
!endif

_install_:
	@if not exist $(FLEXTINC) mkdir $(FLEXTINC) > nul
	@if not exist $(FLEXTLIB) mkdir $(FLEXTLIB) > nul
	@if not exist $(FLEXTLIBINST) mkdir $(FLEXTLIBINST) > nul

	copy $(TARGET) $(FLEXTLIBINST) > nul
!ifdef SHARED
# copy import library
	copy $(TARGET:.dll=.lib) $(FLEXTLIB) > nul	
!endif
# copy headers
	for %%i in ($(HDRS) $(SRCS)) do @copy $(SRCDIR)\%%i $(FLEXTINC) > nul	
# copy import libraries
	for %%i in ($(PDTARGETS)) do @copy %%i $(FLEXTLIB) > nul	
