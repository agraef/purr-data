!ifdef MSVCPATH
INCPATH=$(INCPATH) /I$(MSVCPATH)\include 
LIBPATH=$(LIBPATH) /LIBPATH:$(MSVCPATH)\lib
!endif

!ifdef LOCKFREEPATH
INCPATH=$(INCPATH) /I$(LOCKFREEPATH) 
!endif

!ifdef SNDOBJ_INC
INCPATH=$(INCPATH) /I$(SNDOBJ_INC)
!endif

!ifdef STK_INC
INCPATH=$(INCPATH) /I$(STK_INC)
!endif

##############################################

OBJPATH=$(OUTPATH)\$(OUTSUB)
TARGETPATH=$(OBJPATH)
TARGET=$(TARGETPATH)\$(OUTNAME).$(EXT)

##############################################
# use precompiled headers (automatic mode)

!ifdef PRECOMPILE
CFLAGS=$(CFLAGS) /YX$(PRECOMPILE) /Fp$(OBJPATH)\precompiled.pch
!endif

##############################################

# create code for DLL
CFLAGS=$(CFLAGS) /LD

# enable exception handling, RTTI and intrinsic functions
CFLAGS=$(CFLAGS) /EHsc /GR /Oi

# no banner
CFLAGS=$(CFLAGS) /nologo
LDFLAGS=$(LDFLAGS) /nologo

# inhibit VC8 warnings
CFLAGS=$(CFLAGS) /D_CRT_SECURE_NO_DEPRECATE

# default Windows flags as defined from VC IDE
CFLAGS=$(CFLAGS) /DWIN32 /D_WINDOWS

##############################################

!ifdef DEBUG

!ifdef DYNAMIC
CFLAGS=$(CFLAGS) /MDd
LIBS=$(LIBS) MSVCPRTD.LIB MSVCRTD.LIB
!else
CFLAGS=$(CFLAGS) /MTd
LIBS=$(LIBS) LIBCPMTD.LIB LIBCMTD.LIB
!endif

LDFLAGS=$(LDFLAGS) /DEBUG

!else

!ifdef DYNAMIC
CFLAGS=$(CFLAGS) /MD
LIBS=$(LIBS) MSVCPRT.LIB MSVCRT.LIB
!else
CFLAGS=$(CFLAGS) /MT
LIBS=$(LIBS) LIBCPMT.LIB LIBCMT.LIB
!endif

LDFLAGS=$(LDFLAGS) /INCREMENTAL:NO 

!endif

##############################################
# convert both *.c and *.cpp

OBJSTMP= $(SRCS:.c=.obj)
OBJS= $(OBJSTMP:.objpp=.obj)
