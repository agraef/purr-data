dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.65)
AC_INIT([pdp_opencv],[0.2-rc6-svn],[lluisgomez@hangar.org],[pdp_opencv-dev],[http://www.hangar.org/wikis/lab/doku.php?id=start:puredata_opencv])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE
AC_DISABLE_OPTION_CHECKING

make clean

GEM_OPENCV_VERSION="0.2"
PD_DIR=/usr/local/pd
PDP_DIR=/usr/local/pd/pdp
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

AC_ARG_WITH(pdpsources, [  --with-pdp=PATH         pdp source tree])
if ! test -z "${with_pdp}"
then
  pdp_tree="`cd ${with_pdp} 2>/dev/null && pwd`"
  if test -z "${pdp_tree}"
  then
   dnl  The given directory can't be found
   AC_MSG_ERROR([pdp sources not found in ${with_pdp}])
  else
   PDP_DIR=${pdp_tree}
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

# Very basic check for OpenCV. This does not collect any compile flags; these
# are currently set in Makefile.in through pkg-config
# TODO: use a more robust and well-established OpenCV configure approach
AC_CHECK_LIB([cv], [cvSobel], [], [
  # try another lib name used in other versions of OpenCV
  AC_CHECK_LIB([opencv_imgproc], [cvSobel], [],
    [AC_MSG_ERROR([[missing a library from opencv try installing libcv]])
  ])
])

AC_CHECK_LIB([highgui], [cvNamedWindow], [], [
  AC_CHECK_LIB([opencv_highgui], [cvNamedWindow], [], [
    AC_MSG_ERROR([[missing a library from opencv, try installing libhighgui]])
  ])
])

AC_CHECK_LIB([cvaux], [cvSegmentImage], [], [
  AC_CHECK_LIB([opencv_legacy], [cvSegmentImage], [], [
    AC_MSG_ERROR([[missing a library from opencv try installing libcvaux]])
  ])
])

echo -n "looking for pd sources (required) ... "
if test -f $PD_DIR/src/m_pd.h
then
  AC_SUBST(PD_DIR)
  echo "ok." 
else
  echo "pd source tree not found... install it and use the --with-pd=<path> configuration option."
  exit -1
fi

echo -n "looking for pdp sources (required) ... "
if test -f $PDP_DIR/CHANGES.LOG
then
  AC_SUBST(PDP_DIR)
  echo "ok." 
else
  echo "pdp source tree not found... install it and use the --with-pdp=<path> configuration option."
  exit -1
fi

AC_CHECK_LIB(cv, cvExtractSURF, SOURCES_OPT="pdp_opencv_surf.cc",,-L/usr/local/lib)

AC_SUBST(GEM_OPENCV_VERSION)
AC_SUBST(PD_DIR)
AC_SUBST(PDP_DIR)
AC_SUBST(SOURCES_OPT)

AC_CONFIG_FILES([
Makefile
])
AC_OUTPUT
