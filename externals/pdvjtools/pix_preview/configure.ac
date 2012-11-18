dnl Process this file with autoconf to produce a configure script.

AC_INIT
GEM2PNM_VERSION=0.6

if test -d /usr/local/pd; 
then
	PD_DIR=/usr/local/pd
elif test -d ../../pd; 
then
	PD_DIR=../../pd
else
   echo "Pd sources not found"
fi


if test -d /win/Georg/pd-cvs/gem/Gem; 
then
   GEM_DIR=/win/Georg/pd-cvs/gem/Gem
elif test -d ../../gem/Gem; 
then
	GEM_DIR=../../gem/Gem
elif  test -d ../../gem/Gem; 
then
	GEM_DIR=../../gem/Gem
else
   echo "Gem sources not found"
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

AC_SUBST(GEM2PNM_VERSION)

AC_ARG_WITH(pddir,       [  --with-pddir=<dir>      specify an alternate pd source tree])
AC_ARG_WITH(gemdir,      [  --with-gemdir=<dir>     specify an alternate gem source tree])

if test "$with_gemdir" != ""
then
 GEM_DIR=$with_gemdir
fi

if test "$with_pddir" != ""
then
 PD_DIR=$with_pddir
fi

echo -n "looking for gem sources (required) ... "
if test -f $GEM_DIR/src/Base/GemMan.h
then
  AC_SUBST(GEM_DIR)
  echo "ok." 
else
  echo "gem source tree not found... use ./configure --with-gemdir= option"
  exit -1
fi


echo -n "looking for pd sources (required) ... "
if test -f $PD_DIR/src/m_imp.h
then
  AC_SUBST(PD_DIR)
  echo "ok." 
else
  echo "pd source tree not found... use ./configure --with-pddir= option"
  exit -1
fi

LIBS="$LIBS -lm"
CFLAGS="$CFLAGS -Wall"

AC_CONFIG_FILES([
Makefile
])
AC_OUTPUT
