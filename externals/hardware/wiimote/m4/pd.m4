dnl Copyright (C) 2010 IOhannes m zmölnig
dnl This file is free software; IOhannes m zmölnig
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# PD_CHECK_UNIVERSAL([VARIABLE-NAME], [ACTION-IF-SUCCESS], [ACTION-IF-NO-SUCCESS])
# will enable the "--enable-universal=<ARCHS>" flag
# if <ARCH> is "yes", platform defaults are used
# the system tries to build a test program with the archs, on succes it calls ACTION-IF-SUCCESS, and ACTION-IF-NO-SUCCESS otherwise
# on success it will also add the flags to:
# [VARIABLE-NAME]_CFLAGS will hold a list of cflags to compile for all requested archs
# [VARIABLE-NAME]_LDFLAGS will hold a list of ldflags to link for all requested archs


AC_DEFUN([PD_CHECK_PD],
[
AC_REQUIRE([AC_CANONICAL_HOST])dnl
AC_ARG_WITH([pd], AS_HELP_STRING([--with-pd=<path/to/pd>],[where to find pd-binary (./bin/pd.exe) and pd-sources]))
if test -d "${with_pd}" ; then
 AC_MSG_CHECKING([adding Pd-path(s) to build])
 if test -d "${with_pd}/src" ; then
   AC_LIB_APPENDTOVAR([PD_CFLAGS],"-I${with_pd}/src")
   AC_MSG_RESULT([${PD_CFLAGS}])
 else
   AC_LIB_APPENDTOVAR([PD_CFLAGS],"-I${with_pd}")
   AC_MSG_RESULT([${PD_CFLAGS}])
 fi
 if test -d "${with_pd}/bin" ; then
   PD_LDFLAGS="${PD_LDFLAGS}${PD_LDFLAGS:+ }-L${with_pd}/bin"
   AC_MSG_RESULT([${PD_LDFLAGS}])
 else
   PD_LDFLAGS="${PD_LDFLAGS}${PD_LDFLAGS:+ }-L${with_pd}"
   AC_MSG_RESULT([${PD_LDFLAGS}])
 fi
fi


case $host in
powerpc-apple-darwin* | i*86*-apple-darwin*)
	EXTENSION="pd_darwin"
	;;
*linux*)
	EXTENSION=pd_linux
	;;
*mingw* )
	EXTENSION=dll
	LIBS+="-lpd"
	;;
*cygwin*)
	EXTENSION=dll

    tmp_arch_cflags="$CFLAGS"
    CFLAGS="$CFLAGS -mms-bitfield"
    AC_TRY_COMPILE([], [return 0;], , CFLAGS="$tmp_arch_cflags")
	;;
esac

AC_CHECK_LIB(pd, nullfn)


PD_CHECK_UNIVERSAL
shrext_cmds=".${EXTENSION}"

])
