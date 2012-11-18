# required settings:
#
# PLATFORM - win/mac/lnx
# RTSYS - pd/max
# COMPILER - msvc/gcc/mingw/cygwin
# BUILDPATH including trailing /

###############################################
# package info

ifndef PKGINFO
PKGINFO=package.txt
endif

ifneq ($(PKGINFO),)
include $(PKGINFO)
endif

ifndef NAME
$(error "NAME variable must be defined (name of target)")
endif

ifndef SRCS
$(error "SRCS variable must be defined (list of source files)")
endif

###############################################
# check variables

ifndef BUILDCLASS
BUILDCLASS := ext
endif

ifndef BUILDMODE
BUILDMODE := release
endif

ifndef BUILDTYPE
BUILDTYPE := single
endif

##############################

#ifndef TARGETMODE
#TARGETMODE := $(BUILDMODE)
#endif

#ifndef TARGETTYPE
#TARGETTYPE := $(BUILDTYPE)
#endif

###############################################

ifeq ($(PLATFORM),win)
	# substitute eventual \ by /
	UBUILDPATH := $(subst \,/,$(BUILDPATH))
else
	UBUILDPATH := $(BUILDPATH)
endif

###############################################

SYSCONFIG := $(UBUILDPATH)config-$(PLATFORM)-$(RTSYS)-$(COMPILER).txt
SYSDEFAULT := $(UBUILDPATH)$(PLATFORM)/$(RTSYS)/config-$(COMPILER).def

MAKE_OPTIONS := -f $(UBUILDPATH)gnumake-sub.mak \
	$(MFLAGS) PLATFORM=$(PLATFORM) RTSYS=$(RTSYS) COMPILER=$(COMPILER) \
	BUILDPATH=$(UBUILDPATH) PKGINFO=$(PKGINFO) BUILDCLASS=$(BUILDCLASS)

###############################################

ifdef BUILDDIR
USRCONFIG := config.txt
USRDEFAULT := $(BUILDDIR)/config-$(PLATFORM).def

USRMAKE := $(BUILDDIR)/gnumake-$(PLATFORM)-$(COMPILER).inc

MAKE_OPTIONS += USRCONFIG=$(USRCONFIG) USRMAKE=$(USRMAKE)
endif

###############################################
# include file describing default target dependencies

.PHONY : all build clean install profile

include $(BUILDPATH)targets.inc

include $(BUILDPATH)targets-$(BUILDCLASS).inc

###############################################

.PRECIOUS: $(SYSCONFIG) $(USRCONFIG)

$(SYSCONFIG): $(SYSDEFAULT)
ifeq ($(COMPILER),mingw)
	@copy $(subst /,\,$<) $(subst /,\,$@)
else
	@cp $< $@
endif
	@echo -------------------------------------------------------------------------
	@echo A default system configuration file has been created.
	@echo Please edit $(SYSCONFIG) 
	@echo to match your platform, then start again.
	@echo -------------------------------------------------------------------------
ifeq ($(COMPILER),mingw)
	@exit 1
else
	@false
endif

ifdef BUILDDIR
$(USRCONFIG): $(USRDEFAULT)
ifeq ($(COMPILER),mingw)
	@copy $(subst /,\,$<) $(subst /,\,$@)
else
	@cp $< $@
endif
	@echo -------------------------------------------------------------------------
	@echo A default package configuration file has been created.
	@echo Please edit $(USRCONFIG), then start again.
	@echo -------------------------------------------------------------------------
ifeq ($(COMPILER),mingw)
	@exit 1
else
	@false
endif

$(USRDEFAULT) $(USRMAKE):
	@echo -------------------------------------------------------------------------
	@echo Your combination of platform, system and compiler is not supported yet.
	@echo Required files: 
	@echo $(USRDEFAULT)
	@echo and
	@echo $(USRMAKE)
	@echo -------------------------------------------------------------------------
ifeq ($(COMPILER),mingw)
	@exit 1
else
	@false
endif

endif
