AC_PREREQ(2.59)
AC_INIT(hidio,0.1)
AC_CONFIG_SRCDIR(hidio.c)
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE(1.9)

case $host in
*darwin*)
        MACOSX=yes
        EXTENSION=pd_darwin
        ;;
*linux*)
        LINUX=yes
        EXTENSION=pd_linux
        ;;
*mingw*)
        WINDOWS=yes
        MINGW=yes
        EXTENSION=dll
        ;;
*cygwin*)
        WINDOWS=yes
        CYGWIN=yes
        EXTENSION=dll
        ;;
esac
AM_CONDITIONAL(ANDROID, test x$ANDROID = xyes)
AM_CONDITIONAL(IPHONEOS, test x$IPHONEOS = xyes)
AM_CONDITIONAL(LINUX, test x$LINUX = xyes)
AM_CONDITIONAL(MACOSX, test x$MACOSX = xyes)
AM_CONDITIONAL(WINDOWS, test x$WINDOWS = xyes)
AM_CONDITIONAL(CYGWIN, test x$MINGW = xyes)
AM_CONDITIONAL(MINGW, test x$MINGW = xyes)

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_MAKE_SET

AC_SUBST(EXTENSION)

AC_HEADER_STDC

AC_ARG_WITH([pd],
        [AS_HELP_STRING([--with-pd],
           [location of pd sourcecode])], ,) 

if test "z$with_pd" != "z"; then
    if test "z${with_pd%%/*}" = "z" ; then
        # it's an absolute path
        if test -d $with_pd ; then
	        PD_PATH=$with_pd
        else
	        AC_MSG_ERROR([The file \"$with_pd\" does not exist.])
        fi
    else
        AC_MSG_ERROR([--with-pd requires an absolute path])
    fi
elif test -d ../../../pd; then
	PD_PATH="../../../pd"
fi
AC_MSG_RESULT([checking for pd sourcecode... $PD_PATH])
CFLAGS="-DPD $CFLAGS -I$PD_PATH/src"

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug], [use debugging support])], 
        [debug=$enableval])
if test x$debug = xyes; then
    CFLAGS="$CFLAGS -g"
fi

AC_ARG_ENABLE([universal],
    [AS_HELP_STRING([--enable-universal],
        [build universal binary on Mac OS X (use with --disable-dependency-tracking)])], 
    [universal=$enableval], [universal=no])
AM_CONDITIONAL(UNIVERSAL, test x$universal = xyes)
if test x$universal = xyes; then
# I got this error:
# ld: absolute addressing (perhaps -mdynamic-no-pic) used in _debug_post from
# hidio.o not allowed in slidable image. Use '-read_only_relocs suppress' to
# enable text relocs for architecture ppc
   UNIVERSAL_FLAGS="-arch ppc -arch i386 -read_only_relocs suppress -mmacosx-version-min=10.4"
   CFLAGS="$CFLAGS -fast -ffast-math $UNIVERSAL_FLAGS"
   LDFLAGS="$LDFLAGS $UNIVERSAL_FLAGS"
fi

# Checks for header files.
AC_CHECK_HEADERS([malloc.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
dnl AC_TYPE_INT16_T
dnl AC_TYPE_INT32_T
dnl AC_TYPE_OFF_T
dnl AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_TYPE_SIGNAL

# Specify what files are to be created.
AC_CONFIG_FILES([Makefile
                HID_Utilities_Source/Makefile])

AC_OUTPUT

dnl output some messages at the end

AS_IF([test x$universal = xyes], [
	    AC_MSG_RESULT(
To build a Universal Binary on Mac OS X run it like this: 
  ./configure --enable-universal --disable-dependency-tracking)])

echo "
Building using Pd headers in $PD_PATH/src
"
