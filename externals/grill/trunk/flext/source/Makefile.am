#
# automake template
# added by tim blechmann
# added by Thomas Grill
#

# static libraries
#lib_LIBRARIES = 
# shared libraries
lib_LTLIBRARIES = libflext-@SYSTEM@_s.la libflext-@SYSTEM@_sd.la libflext-@SYSTEM@_t.la libflext-@SYSTEM@_td.la libflext-@SYSTEM@.la libflext-@SYSTEM@_d.la 

SRCS_FLEXT = \
	flbase.cpp \
	flext.cpp \
	flbuf.cpp \
	fldsp.cpp \
	fllib.cpp \
	flxlet.cpp \
	flattr.cpp \
	flattr_ed.cpp \
	flsupport.cpp \
	flutil.cpp \
	flthr.cpp \
	fltimer.cpp \
	flsimd.cpp \
	flout.cpp \
	flatom.cpp \
	flatom_pr.cpp \
	flatom_part.cpp \
	flitem.cpp \
	flmeth.cpp \
	flmsg.cpp \
	flproxy.cpp \
	flqueue.cpp \
	flbind.cpp \
	flmap.cpp

nobase_pkginclude_HEADERS = \
	flprefix.h \
	flstdc.h \
	flbase.h \
	flclass.h \
	flext.h \
	flsupport.h \
	flmap.h \
	fldsp.h \
	flmspbuffer.h \
	flinternal.h \
	flcontainers.h \
	flpushns.h \
	flpopns.h \
	fldefs.h \
	fldefs_hdr.h \
	fldefs_setup.h \
	fldefs_methcb.h \
	fldefs_meththr.h \
	fldefs_methadd.h \
	fldefs_methbind.h \
	fldefs_methcall.h \
	fldefs_attrcb.h \
	fldefs_attrvar.h \
	fldefs_attradd.h \
	lockfree/prefix.hpp \
	lockfree/branch_hints.hpp \
	lockfree/cas.hpp \
	lockfree/atomic_int.hpp \
	lockfree/atomic_ptr.hpp \
	lockfree/fifo.hpp \
	lockfree/stack.hpp \
	$(SRCS_FLEXT)



# handling for stk / sndobj
SRCS_STK = flstk.cpp
HDRS_STK = flstk.cpp

SRCS_SNDOBJ = flsndobj.cpp
HDRS_SNDOBJ = flsndobj.cpp

if SNDOBJ
SRCS_FLEXT         += $(SRCS_SNDOBJ)
nobase_pkginclude_HEADERS += $(HDRS_SNDOBJ)
LIB_SNDOBJ         = sndobj
endif

if STK
SRCS_FLEXT         += $(SRCS_STK)
nobase_pkginclude_HEADERS += $(HDRS_STK)
LIB_STK            =  stk
endif

libflext_@SYSTEM@_s_la_SOURCES    = $(SRCS_FLEXT)
libflext_@SYSTEM@_sd_la_SOURCES  = $(SRCS_FLEXT)
libflext_@SYSTEM@_t_la_SOURCES  = $(SRCS_FLEXT)
libflext_@SYSTEM@_td_la_SOURCES = $(SRCS_FLEXT)
libflext_@SYSTEM@_la_SOURCES = $(SRCS_FLEXT)
libflext_@SYSTEM@_d_la_SOURCES = $(SRCS_FLEXT)

libflext_@SYSTEM@_s_la_CXXFLAGS    = @OPT_FLAGS@ -static $(patsubst %,-I%,@INCLUDEDIRS@)
libflext_@SYSTEM@_sd_la_CXXFLAGS  = @DBG_FLAGS@ -static $(patsubst %,-I%,@INCLUDEDIRS@)
libflext_@SYSTEM@_t_la_CXXFLAGS  = @OPT_FLAGS@ -static $(patsubst %,-I%,@INCLUDEDIRS@) -DFLEXT_THREADS 
libflext_@SYSTEM@_td_la_CXXFLAGS = @DBG_FLAGS@ -static $(patsubst %,-I%,@INCLUDEDIRS@) -DFLEXT_THREADS 
libflext_@SYSTEM@_la_CXXFLAGS    = @OPT_FLAGS@ $(patsubst %,-I%,@INCLUDEDIRS@) -DFLEXT_SHARED
libflext_@SYSTEM@_d_la_CXXFLAGS  = @DBG_FLAGS@ $(patsubst %,-I%,@INCLUDEDIRS@) -DFLEXT_SHARED

libflext_@SYSTEM@_la_LDFLAGS  = @LD_FLAGS@ $(patsubst %,-L%,@LIBDIRS@) $(patsubst %,-l%,@libs@ $(LIB_SNDOBJ) $(LIB_STK)) $(patsubst %,-framework %,@FRAMEWORKS@)
libflext_@SYSTEM@_d_la_LDFLAGS  = @LD_FLAGS@ $(patsubst %,-L%,@LIBDIRS@) $(patsubst %,-l%,@libs@ $(LIB_SNDOBJ) $(LIB_STK)) $(patsubst %,-framework %,@FRAMEWORKS@) 

#libflext_@SYSTEM@_la_LIBADD = @libs@ 
#libflext_@SYSTEM@_d_la_LIBADD = @libs@ 
