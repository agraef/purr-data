##############################################
# flext defines
##############################################

CFLAGS=$(CFLAGS) $(UFLAGS)

!ifdef DEBUG
CFLAGS=$(CFLAGS) /D_DEBUG $(DFLAGS)
!else
CFLAGS=$(CFLAGS) /DNDEBUG $(OFLAGS)
!endif


!ifdef SHARED
# --- shared ---
DEFS=$(DEFS) /DFLEXT_SHARED

!else
!ifdef THREADED
# --- static multi-threaded ---
DEFS=$(DEFS) /DFLEXT_THREADS

!else 
# --- static single-threaded ---

!endif
!endif

##############################################
# name of flext library
##############################################

!ifdef SHARED
TYPEEXT=_
!else
!ifdef THREADED
TYPEEXT=_t
!else
TYPEEXT=_s
!endif
!endif

!ifdef DEBUG
MODEEXT=d
!else
!ifdef PROFILE
MODEEXT=p
!else
MODEEXT=
!ifdef SHARED
TYPEEXT=
!endif
!endif
!endif

FLEXTNAME=flext-$(RTSYS)$(TYPEEXT)$(MODEEXT)

##############################################
# product name and folder
##############################################

!ifndef SRCDIR
SRCDIR=.
!endif

OUTNAME=$(NAME)
OUTSUB=$(TARGETMODE)-$(TARGETTYPE)
