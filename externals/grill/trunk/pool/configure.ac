threads_default=no

AC_INIT([pool],[0.0.0],[gr@grrrr.org])

###########################################################

AM_INIT_AUTOMAKE

AC_SUBST(SYSTEM)
AC_SUBST(INCLUDEDIRS)
AC_SUBST(LIBDIRS)
AC_SUBST(libs)
AC_SUBST(FRAMEWORKS)
AC_SUBST(EXT_CFLAGS)
AC_SUBST(EXT_LDFLAGS)
AC_SUBST(EXTENSION)

# configure options
AC_ARG_ENABLE(system,
	AC_HELP_STRING(--enable-system,[realtime system [default=pd]]),
    [SYSTEM=$enableval],
	[SYSTEM=pd]
)

AC_ARG_WITH(sysdir,
	AC_HELP_STRING(--with-sysdir,[path to pd or max headers]),
    [sysdir=$withval],
	AC_MSG_ERROR([path to system headers is required])
)

AC_ARG_WITH(flext-headers,
	AC_HELP_STRING(--with-flext-headers,[path to the flext headers [default=/usr/local/include/flext]]),
    [flext_headers=$withval],
	[flext_headers="/usr/local/include/flext"]
)

AC_ARG_WITH(flext-libs,
	AC_HELP_STRING(--with-flext-libs,[path to the flext libraries [default=/usr/local/lib]]),
    [flext_libs=$withval],
	[flext_libs="/usr/local/lib"]
)

AC_ARG_ENABLE(dynamic,
	AC_HELP_STRING(--enable-dynamic,[link dynamically to flext [default=no]]),
	[dynamic=$enableval],
	[dynamic=no]
)

AC_ARG_ENABLE(debug, 
	AC_HELP_STRING(--enable-debug,[enables debug build [default=no]]),
    [debug=$enableval],
    [debug=no]
)

AC_ARG_ENABLE(threads, 
	AC_HELP_STRING(--enable-threads,[enables threaded build [default=$threads_default]]),
    [threads=$enableval],
    [threads=$threads_default],
)

AC_ARG_ENABLE(optimize,
 	AC_HELP_STRING(--enable-optimize,[enables optimized builds for: pentium4, pentium3, G4, G5, etc.]),
    [optimize=$enableval]
)

# if CFLAGS aren't set by the user, set them to an empty string
# otherwise AC_PROG_CC sets them to "-O2 -g" 
test ".$CFLAGS" = "." && CFLAGS=" "
test ".$CXXFLAGS" = "." && CXXFLAGS=" "

# disable static library building (must be before AC_PROG_LIBTOOL)
AC_DISABLE_STATIC
# needed to build win32 dlls
AC_LIBTOOL_WIN32_DLL

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Language use for checks
AC_LANG(C++)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for libraries.

INCLUDEDIRS="$INCLUDEDIRS $flext_headers"
LIBDIRS="$LIBDIRS $flext_libs"


if test $SYSTEM == pd; then
	AC_DEFINE(FLEXT_SYS,2)
# check for g_canvas.h in pd folder
	AC_CHECK_FILE([$sysdir/g_canvas.h],,AC_MSG_ERROR([Cannot find $sysdir/g_canvas.h]))
	INCLUDEDIRS="$INCLUDEDIRS $sysdir"
	libs="$libs pd"
	
	if test $(uname -s) == Linux; then
		EXTENSION=.pd_linux
	elif test $(uname -s) == Darwin; then
		EXT_LDFLAGS="-flat_namespace -undefined suppress"
		EXTENSION=.pd_darwin
    elif ( test $(uname -s) == CYGWIN_NT-5.0 ) || ( test $(uname -s) == CYGWIN_NT-5.1 ) || ( test $(uname -s) == MINGW32_NT-5.1 ) ; then
		EXTENSION=.dll
		AC_DEFINE(NT)
	fi
	
elif test $SYSTEM == max; then
	AC_DEFINE(FLEXT_SYS,1)
# check for MaxAPI.h in pd folder
	AC_CHECK_FILE([$sysdir/max-includes/MaxAPI.h],,AC_MSG_ERROR([Cannot find $sysdir/max-includes/MaxAPI.h]))
	AC_CHECK_FILE([$sysdir/max-includes/MaxAudioAPI.h],,AC_MSG_ERROR([Cannot find $sysdir/max-includes/MaxAudioAPI.h]))
	INCLUDEDIRS="$INCLUDEDIRS $sysdir/max-includes $sysdir/msp-includes"

else
	AC_MSG_ERROR([system must be pd or max])
fi


if test ".$dynamic" == ".yes"; then
	AC_DEFINE(FLEXT_SHARED)
	LIBNAME=flext-${SYSTEM}
	if test ".$debug" == ".yes"; then
		LIBNAME=${LIBNAME}_d
	fi
else
	LIBNAME=flext-${SYSTEM}_
	if test ".$threaded" == ".yes"; then
		AC_DEFINE(FLEXT_THREADS)
		LIBNAME=${LIBNAME}t
	else
		LIBNAME=${LIBNAME}s
	fi
	if test ".$debug" == ".yes"; then
		LIBNAME=${LIBNAME}d
	fi
fi

libs="$libs $LIBNAME"


# Checks are complicated because flext references symbols from pd or Max
#AC_CHECK_LIB([$LIBNAME],
#	buf_class, # check for variable "buf_class""
#	true,
#	AC_MSG_ERROR([cannot find flext library $flext_libs/$LIBNAME])
#)
# test for existence of library file
AC_MSG_CHECKING([for $flext_libs/lib$LIBNAME.*])
if test ".$(ls $flext_libs/lib$LIBNAME.*)" == "."; then
	AC_MSG_ERROR([cannot find flext library $flext_libs/$LIBNAME])
else
	AC_MSG_RESULT(yes)
fi

# we ought to check flext header files:
#AC_CHECK_HEADERS([$flext_headers/flext.h],
# but we rather check for pure existence since we are dependent on the pd/Max/pthreads etc. headers
AC_CHECK_FILE([$flext_headers/flext.h],
	,
	AC_MSG_ERROR([cannot find $flext_headers/flext.h])
)


if test $(uname -s) == Linux; then
	true
elif test $(uname -s) == Darwin; then
	FRAMEWORKS="$FRAMEWORKS ApplicationServices vecLib"
elif ( test $(uname -s) == CYGWIN_NT-5.0 ) || ( test $(uname -s) == CYGWIN_NT-5.1 ) || ( test $(uname -s) == MINGW32_NT-5.1 ) ; then
	EXT_CFLAGS="$EXT_CFLAGS -mms-bitfields -mno-cygwin"
	EXT_LDFLAGS="$EXT_LDFLAGS -mno-cygwin"
fi


# set compilation flags
if test ".$debug" == ".yes"; then
	AC_DEFINE(FLEXT_DEBUG)
	EXT_CFLAGS="$EXT_CFLAGS -g"
else
	EXT_CFLAGS="$EXT_CFLAGS -O2"
fi



if test ".$optimize" == ".yes"; then
	OPT_FLAGS="$OPT_FLAGS -mtune=$optimize"
	case $optimize in
		pentium | pentium2 | athlon | pentium-mmx)
			EXT_CFLAGS="$EXT_CFLAGS -march=$optimize";;
		pentium3 | pentium3m | pentium4 | pentium4m | pentium-m | prescott | nocona | athlon-xp | athlon-mp | athlon64 | opteron) 
			EXT_CFLAGS="$EXT_CFLAGS -march=$optimize -mfpmath=sse";
			AC_DEFINE(FLEXT_USE_SIMD);;
		G3)
			EXT_CFLAGS="$EXT_CFLAGS -mcpu=$optimize";;
		G5 | G4)
			EXT_CFLAGS="$EXT_CFLAGS -mcpu=$optimize -faltivec";
			AC_DEFINE(FLEXT_USE_SIMD);;
		*)
			;;
	esac
fi

###########################################################

AC_OUTPUT([
			Makefile
			source/Makefile
])
