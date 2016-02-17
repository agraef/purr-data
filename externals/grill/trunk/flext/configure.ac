#
# autoconf template
# modified by Thomas Grill
#

# flext API version (current:release:age)
# API_VERSION=0:0:0

AC_INIT([flext],[0.6.0],[gr@grrrr.org])
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIRS([m4])

# TODO: this should definitely be changed to work with any version of CYGWIN_NT or MINGW32_NT
if ( test $(uname -s) == CYGWIN_NT-5.0 ) || ( test $(uname -s) == CYGWIN_NT-5.1 ) || ( test $(uname -s) == MINGW32_NT-5.1 ); then
	set win = 1
fi

# configure options
AC_ARG_ENABLE(system,
	AC_HELP_STRING(--enable-system,[realtime system [default=pd]]),
    [SYSTEM=$enableval],
    [SYSTEM=pd]
)

AC_ARG_WITH(sdkdir,
	AC_HELP_STRING(--with-sdkdir,[path to pd or max headers]),
    [sdkdir=$withval],
	AC_MSG_ERROR(path to system SDK headers required $withval)
)

AC_ARG_WITH(pdbindir,
	AC_HELP_STRING(--with-pdbindir,[path to pd bin dir]),
	[
		if test $win; then
# LATER: shouldn't we use AC_CHECK_LIB([pd]) ?
		 AC_CHECK_FILE([$withval/pd.dll],,AC_MSG_ERROR([Cannot find $withval/pd.dll]))	
		fi
		LIBDIRS="$LIBDIRS $withval"
	],
	[
		if test $win; then
			AC_MSG_ERROR(path to pd bin dir required)
		fi
	]
)

if test $SYSTEM == max; then
	AC_DEFINE(FLEXT_SYS,1)
# check for MaxAPI.h in pd folder
	AC_CHECK_FILE([$sdkdir/max-includes/MaxAPI.h],,AC_MSG_ERROR([Cannot find $sdkdir/max-includes/MaxAPI.h]))
	AC_CHECK_FILE([$sdkdir/max-includes/MaxAudioAPI.h],,AC_MSG_ERROR([Cannot find $sdkdir/max-includes/MaxAudioAPI.h]))
	INCLUDEDIRS="$INCLUDEDIRS $sdkdir/max-includes $sdkdir/msp-includes"

elif test $SYSTEM == pd; then
#	if test $win; then
#	fi

	AC_DEFINE(FLEXT_SYS,2)
# check for g_canvas.h in pd folder
	AC_CHECK_FILE([$sdkdir/g_canvas.h],,AC_MSG_ERROR([Cannot find $sdkdir/g_canvas.h]))
	INCLUDEDIRS="$INCLUDEDIRS $sdkdir"

	if test $win; then
		libs="$libs pd"
		AC_DEFINE(NT)
	fi
else
	AC_MSG_ERROR([system must be pd or max])
fi

AC_ARG_WITH(atomic_ops,
	AC_HELP_STRING(--with-atomic_ops,[path to atomic_ops library (needed for gcc version < 4.1 on non-i386 cpus)]),
	[
	    AC_CHECK_FILE([$withval/atomic_ops.h],,AC_MSG_ERROR([Cannot find $withval/atomic_ops.h]))	
		INCLUDEDIRS="$INCLUDEDIRS $withval"
        AC_DEFINE(USE_ATOMIC_OPS)
	]
)

AC_ARG_WITH(stkdir,
	AC_HELP_STRING(--with-stk,[path to STK headers]),
	[
	    AC_CHECK_FILE([$withval/Stk.h],,AC_MSG_ERROR([Cannot find $withval/Stk.h]))	
		stkdir=$withval
		INCLUDEDIRS="$INCLUDEDIRS $withval"
	]
)

AM_CONDITIONAL([STK],[test "$stkdir"])

AC_ARG_WITH(sndobjdir,
	AC_HELP_STRING(--with-sndobj,[path to SndObj headers]),
	[
		AC_CHECK_FILE([$withval/SndObj.h],,AC_MSG_ERROR([Cannot find $withval/SndObj.h]))
		sndobjdir=$withval
		INCLUDEDIRS="$INCLDUEDIRS $withval"
	]
)

AM_CONDITIONAL([SNDOBJ],[test "$sndobjdir"])

# if CFLAGS aren't set by the user, set them to an empty string
# otherwise AC_PROG_CC sets them to "-O2 -g" 
test ".$CFLAGS" = "." && CFLAGS=" "
test ".$CXXFLAGS" = "." && CXXFLAGS=" "

# needed for libtool to build win32 dlls
AC_LIBTOOL_WIN32_DLL

AM_ENABLE_STATIC
AM_ENABLE_SHARED

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_LANG(C++)

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.

# system specific
if test $(uname -s) == Linux; then
	true
elif test $(uname -s) == Darwin; then
	FRAMEWORKS="$FRAMEWORKS ApplicationServices Accelerate"
	LD_FLAGS="$LD_FLAGS -flat_namespace -undefined dynamic_lookup"
elif test $win; then
	C_FLAGS="$C_FLAGS -mms-bitfields -mno-cygwin"
	LD_FLAGS="$LD_FLAGS -mno-cygwin"
fi


# set compilation flags

OPT_FLAGS="$C_FLAGS -O2"
DBG_FLAGS="$C_FLAGS -DFLEXT_DEBUG -g"

AC_ARG_ENABLE(optimize, 
	AC_HELP_STRING(--enable-optimize,[enables optimized architecture specific builds for pentium4, pentium3, G4, G5, etc.]),
    [
        # tune to a specific CPU
		OPT_FLAGS="$OPT_FLAGS -mtune=$enableval"
		case $enableval in
			pentium | pentium2 | athlon | pentium-mmx)
				OPT_FLAGS="$OPT_FLAGS -march=$enableval";;
			pentium3 | pentium3m | pentium4 | pentium4m | pentium-m | prescott | nocona | athlon-xp | athlon-mp | athlon64 | opteron) 
				OPT_FLAGS="$OPT_FLAGS -march=$enableval -mfpmath=sse";
				AC_DEFINE(FLEXT_USE_SIMD);;
			G3)
                # set specific architecture (like march)
				OPT_FLAGS="$OPT_FLAGS -mcpu=$enableval";;
			G5 | G4)
                # set specific architecture (like march)
				OPT_FLAGS="$OPT_FLAGS -mcpu=$enableval -faltivec";
				AC_DEFINE(FLEXT_USE_SIMD);;
			*)
				;;
		esac
	]
)


AC_SUBST(SYSTEM)
AC_SUBST(INCLUDEDIRS)
AC_SUBST(LIBDIRS)
AC_SUBST(OPT_FLAGS)
AC_SUBST(DBG_FLAGS)
AC_SUBST(LD_FLAGS)
# AC_SUBST(API_VERSION)
AC_SUBST(libs)
AC_SUBST(stkdir)
AC_SUBST(sndobjdir)
AC_SUBST(FRAMEWORKS)

AC_OUTPUT([
			Makefile
			source/Makefile
			$SYSTEM-flext.pc
])

#			tutorial/Makefile
#			tutorial/adv1/Makefile
#			tutorial/adv2/Makefile
#			tutorial/adv3/Makefile
#			tutorial/attr1/Makefile
#			tutorial/attr2/Makefile
#			tutorial/attr3/Makefile
#			tutorial/bind1/Makefile
#			tutorial/buffer1/Makefile
#			tutorial/lib1/Makefile
#			tutorial/signal1/Makefile
#			tutorial/signal2/Makefile
#			tutorial/simple1/Makefile
#			tutorial/simple2/Makefile
#			tutorial/simple3/Makefile
#			tutorial/sndobj1/Makefile
#			tutorial/stk1/Makefile
#			tutorial/stk2/Makefile
#			tutorial/thread1/Makefile
#			tutorial/thread2/Makefile
#			tutorial/timer1/Makefile
#			tutorial/pd/Makefile
#			tutorial/maxmsp/Makefile
