AC_PREREQ([2.57])

AC_INIT([wiimote],[0.4.svn],[zmoelnig@iem.at])
AC_CONFIG_SRCDIR([wiimote.c])
AC_CONFIG_MACRO_DIR([m4/generated])
AC_CONFIG_AUX_DIR([m4/config])

AM_INIT_AUTOMAKE

AC_PROG_CC


LT_INIT
AC_SUBST([LIBTOOL_DEPS])

LT_LIB_M

AC_SUBST([EXTENSION])

AC_SUBST([ARCH_CFLAGS])
AC_SUBST([ARCH_LDFLAGS])

AC_SUBST([PD_CFLAGS])
AC_SUBST([PD_LDFLAGS])

AC_CANONICAL_HOST

PD_CHECK_PD

AC_CHECK_LIB(pd, nullfn)

PKG_CHECK_MODULES([CWIID], [cwiid])
CFLAGS="$CFLAGS $CWIID_CFLAGS"
LIBS="$LIBS $CWIID_LIBS"

AC_CHECK_MEMBER([struct cwiid_motionplus_mesg.low_speed], 
                [AC_DEFINE([HAVE_CWIID_MOTIONPLUS_LOWSPEED], [1], [Define if `struct cwiid_motionplus_mesg' has `low_speed' member])
                ], [],
                [[#include <cwiid.h>]]
               )



AC_CONFIG_FILES([Makefile])
AC_OUTPUT

