# include flext version number
!include $(BUILDPATH)version.inc

# general settings
!include $(BUILDPATH)config-$(PLATFORM)-$(RTSYS)-$(COMPILER).txt

##############################
# project-specific definitions

# package info
!if "$(PKGINFO)" != ""
!include $(PKGINFO)
!endif

# special package settings
!ifdef USRCONFIG
!include $(USRCONFIG)
!endif

# package specific make stuff
!ifdef USRMAKE
!include $(USRMAKE)
!endif

##############################
# flext-specific definitions

!include $(BUILDPATH)nmake.inc

!include $(BUILDPATH)nmake-$(BUILDCLASS).inc

##############################
# platform-specific make stuff

!include $(BUILDPATH)$(PLATFORM)\$(RTSYS)\nmake-$(COMPILER).inc

!include $(BUILDPATH)$(PLATFORM)\$(RTSYS)\nmake-$(COMPILER)-$(BUILDCLASS).inc

##############################
# general make stuff

!include $(BUILDPATH)$(PLATFORM)\nmake-$(COMPILER).inc

!include $(BUILDPATH)$(PLATFORM)\nmake-$(COMPILER)-$(BUILDCLASS).inc
