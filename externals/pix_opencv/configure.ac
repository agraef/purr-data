dnl Process this file with autoconf to produce a configure script.

AC_INIT
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE
AC_DISABLE_OPTION_CHECKING

make clean

GEM_OPENCV_VERSION="0.2"
PD_DIR=/usr/local/pd
GEM_DIR=/usr/local/pd/gem
SOURCES_OPT=

echo $host
# Check for DarwinPorts and/or Fink on Mac OS X/Darwin
case "$host" in
  *-darwin* | *-macos10*)
     if test -d /sw ; then
       # Fink
       PATH="/sw/bin:/sw/sbin:$PATH"
       CFLAGS="$CFLAGS -I/sw/include"
       CPPFLAGS="$CPPFLAGS -I/sw/include"
       LDFLAGS="$LDFLAGS -L/sw/lib"
     elif test -d /opt/local ; then
       # DarwinPorts
       PATH="/opt/local/bin:/opt/local/sbin:$PATH"
       CFLAGS="$CFLAGS -I/opt/local/include"
       CPPFLAGS="$CPPFLAGS -I/opt/local/include"
       LDFLAGS="$LDFLAGS -L/opt/local/lib"
     fi
  ;;
esac

AC_ARG_WITH(pdsources, [  --with-pd=PATH          pd source tree])
if ! test -z "${with_pd}"
then
  pd_tree="`cd ${with_pd} 2>/dev/null && pwd`"
  if test -z "${pd_tree}"
  then
   dnl  The given directory can't be found
   AC_MSG_ERROR([pd sources not found in ${with_pd}])
  else
   PD_DIR=${pd_tree}
  fi
fi

AC_ARG_WITH(gemsources, [  --with-gem=PATH         gem source tree])
if ! test -z "${with_gem}"
then
  gem_tree="`cd ${with_gem} 2>/dev/null && pwd`"
  if test -z "${gem_tree}"
  then
   dnl  The given directory can't be found
   AC_MSG_ERROR([gem sources not found in ${with_gem}])
  else
   GEM_DIR=${gem_tree}
  fi
fi

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_SANITY_CHECK

AM_MAINTAINER_MODE
AM_WITH_DMALLOC

AC_CHECK_LIB(cv, cvSobel, , AC_MSG_ERROR(you need to install opencv library (libcv)))
AC_CHECK_LIB(highgui, cvNamedWindow, , AC_MSG_ERROR(you need to install high gui library (libhighgui)))
AC_CHECK_LIB(cvaux, cvSegmentImage, , AC_MSG_ERROR(you need to install opencv auxiliary library (libcvaux) ))

echo -n "looking for pd sources (required) ... "
if test -f $PD_DIR/src/m_pd.h
then
  AC_SUBST(PD_DIR)
  echo "ok." 
else
  echo "pd source tree not found... install it and use the --with-pd=<path> configuration option."
  exit -1
fi

echo -n "looking for gem sources (required) ... "
if test -f $GEM_DIR/src/Base/GemBase.h
then
  AC_SUBST(GEM_DIR)
  echo "ok." 
else
  echo "gem source tree not found... install it and use the --with-gem=<path> configuration option."
  exit -1
fi

AC_CHECK_LIB(cv, cvExtractSURF, SOURCES_OPT="pix_opencv_surf.cc",,-L/usr/local/lib)

AC_SUBST(GEM_OPENCV_VERSION)
AC_SUBST(PD_DIR)
AC_SUBST(GEM_DIR)
AC_SUBST(SOURCES_OPT)

AC_CONFIG_FILES([
Makefile
])
AC_OUTPUT
