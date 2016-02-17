# required settings:
#
# PLATFORM - win/mac/lnx
# RTSYS - pd/max
# COMPILER - msvc/gcc/mingw/cygwin
# BUILDPATH including trailing \

###############################################
# package info

!ifndef PKGINFO
PKGINFO=package.txt
!endif

!if "$(PKGINFO)" != ""
!include $(PKGINFO)
!endif

!ifndef NAME
!error NAME variable must be defined (name of target)
!endif

!ifndef SRCS
!error SRCS variable must be defined (list of source files)
!endif

###############################################
# check variables

!ifndef BUILDCLASS
BUILDCLASS=ext
!endif

!ifndef BUILDMODE
BUILDMODE=release
!endif

!ifndef BUILDTYPE
BUILDTYPE=single
!endif

##############################

#!ifndef TARGETMODE
#TARGETMODE=$(BUILDMODE)
#!endif

#!ifndef TARGETTYPE
#TARGETTYPE=$(BUILDTYPE)
#!endif

###############################################

SYSCONFIG=$(BUILDPATH)config-$(PLATFORM)-$(RTSYS)-$(COMPILER).txt
SYSDEFAULT=$(BUILDPATH)$(PLATFORM)\$(RTSYS)\config-$(COMPILER).def

MAKE_OPTIONS=/NOLOGO /f $(BUILDPATH)nmake-sub.mak \
	PLATFORM=$(PLATFORM) RTSYS=$(RTSYS) COMPILER=$(COMPILER) \
	BUILDPATH=$(BUILDPATH) PKGINFO=$(PKGINFO) BUILDCLASS=$(BUILDCLASS)

###############################################

!ifdef BUILDDIR
USRCONFIG=config.txt
USRDEFAULT=$(BUILDDIR)\config-$(PLATFORM).def

USRMAKE=$(BUILDDIR)\nmake-$(PLATFORM)-$(COMPILER).inc

MAKE_OPTIONS=$(MAKE_OPTIONS) $(MFLAGS) USRCONFIG=$(USRCONFIG) USRMAKE=$(USRMAKE)
!endif

###############################################
# include file describing default target dependencies

!include $(BUILDPATH)\targets.inc

!include $(BUILDPATH)\targets-$(BUILDCLASS).inc

###############################################

.precious: $(SYSCONFIG) $(USRCONFIG)

$(SYSCONFIG): $(SYSDEFAULT)
	@copy $** $@
	@echo -------------------------------------------------------------------------
	@echo A default system configuration file has been created.
	@echo Please edit $(SYSCONFIG) 
	@echo to match your platform and start again.
	@echo -------------------------------------------------------------------------
	@exit 1

!ifdef BUILDDIR
$(USRCONFIG): $(USRDEFAULT)
	@copy $** $@
	@echo -------------------------------------------------------------------------
	@echo A default package configuration file has been created.
	@echo Please edit $(USRCONFIG) and start again.
	@echo -------------------------------------------------------------------------
	@exit 1

$(USRDEFAULT) $(USRMAKE):
	@echo -------------------------------------------------------------------------
	@echo Your combination of platform, system and compiler is not supported yet.
	@echo Required files: 
	@echo $(USRDEFAULT)
	@echo and
	@echo $(USRMAKE)
	@echo -------------------------------------------------------------------------
	@exit 1
!endif
