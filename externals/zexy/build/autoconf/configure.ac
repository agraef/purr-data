dnl Process this file with autoconf to produce a configure script.
AC_INIT([../../src/zexy.h])
AC_CONFIG_HEADER([../../src/zexyconf.h])

LIBNAME=zexy


dnl Checks for programs.
AC_PROG_CC

AC_SUBST(STK)
AC_SUBST(CFLAGS)
AC_SUBST(DEFS)
AC_SUBST(DFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(EXT)
AC_SUBST(LD)
AC_SUBST(STRIP)
AC_SUBST(STRIPFLAGS)
AC_SUBST(LIBRARY_VERSION)
AC_SUBST(BUILDLIBRARY)
AC_SUBST(REFERENCEPATH)
AC_SUBST(PDLIBDIR)
AC_SUBST(INCLUDES)
AC_SUBST(SOURCES)
AC_SUBST(LIBNAME)
AC_SUBST(CONFIGUREFLAGS)

## store the flags passed to us
## is there no way to get the flags without quotes?
#CONFIGUREFLAGS=${ac_configure_args}
## and is this solution portable? time will show....
CONFIGUREFLAGS=$(echo ${ac_configure_args} | sed "s/'//g")

AC_ARG_WITH(pdversion,[  --with-pdversion=<ver>  enforce a certain pd-version (e.g. 0.37)])
AC_ARG_WITH(version,  [  --with-version=<ver>    enforce a certain zexy-version (e.g. 2.0)])
AC_ARG_WITH(extension,[  --with-extension=<ext>  enforce a certain extension for the dynamic library (e.g. dll)])
AC_ARG_WITH(pdpath,   [  --with-pd=</path/to/pd> where to look for pd-headers and and -libs])
AC_ARG_ENABLE(PIC,    [  --disable-PIC           disable compilation with PIC-flag])
AC_ARG_ENABLE(PIC,    [  --disable-exportdynamic disable linkage with the export_dynamic-flag])
AC_ARG_ENABLE(lpt,    [  --enable-lpt            enable parallelport-support])
AC_ARG_ENABLE(library,[  --disable-library       split the library into single externals])

if test "x" = "x${enable_PIC}" ; then
 enable_PIC="${with_PIC}"
fi

if test "x" = "x${enable_lpt}" ; then
 enable_lpt="${with_lpt}"
fi

if test "xno" != "x${enable_library}" ; then
dnl LATER: find a more generic way to generate the .._LIBRARY define
 BUILDLIBRARY="-DZEXY_LIBRARY"
fi

if test `uname | sed -e 's/^MINGW.*/NT/'` = NT ; 
then
  LD=${CC}
  EXT=dll
  if test "x" = "x${enable_exportdynamic}" ; then
    enable_exportdynamic="no"
  fi
fi



if test "x$with_pd" != "x"; then
 if test -d "${with_pd}/src"; then
  AC_MSG_RESULT([adding ${with_pd}/src to INCLUDES])
  INCLUDES="-I${with_pd}/src ${INCLUDES}"
 fi
 if test -d "${with_pd}/bin"; then
  AC_MSG_RESULT([adding ${with_pd}/bin to LDFLAGS])
  LDFLAGS="-L${with_pd}/bin ${LDFLAGS}"
 fi
fi

if test $includedir
then
 for id in $includedir
 do
  if test -d $id; then INCLUDES="-I$id $INCLUDES"; fi
 done
fi
if test $libdir
then
 for id in $libdir
 do
  if test -d $id; then LDFLAGS="-L$id $LDFLAGS"; fi
 done
fi


CFLAGS="${INCLUDES} ${CFLAGS}"
CPPFLAGS="${INCLUDES} ${CPPFLAGS}"
INCLUDES=""

dnl Checks for libraries.
dnl Replace `main' with a function in -lc:
AC_CHECK_LIB(c, main)
AC_CHECK_LIB(crtdll, fclose)

AC_CHECK_LIB(coldname, lseek)
AC_CHECK_LIB(kernel32, main)
AC_CHECK_LIB(user32, main)
AC_CHECK_LIB(gdi32, main)

dnl Replace `main' with a function in -lm:
AC_CHECK_LIB(m, main)
dnl Replace `main' with a function in -lpthread:
dnl AC_CHECK_LIB(pthread, main)
dnl Replace `main' with a function in -lstk:
dnl AC_CHECK_LIB(stk, main, STK=yes)


dnl LATER we might want to use it for other platforms (namely:windoze) too

AC_CHECK_LIB(pd, nullfn)

AC_CHECK_LIB(regex, regcomp)


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(stdlib.h stdio.h string.h math.h time.h sys/time.h regex.h alloca.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_TIME

### make-depend flags
if test "x$ac_cv_c_compiler_gnu" = "xyes"; then
    AC_SUBST(MAKEDEP_FLAGS, "-MM")
else
    AC_SUBST(MAKEDEP_FLAGS, "-M")
fi


dnl Checks for library functions.
AC_FUNC_MMAP
AC_CHECK_FUNCS(select socket strerror)

dnl check for "-mms-bitfields" cflag
dnl why is there no generic compiler-check for a given flag ?
dnl it would make things so easy: AC_CHECK_FLAG([-mms-bitfields],,)
AC_CHECK_CFLAGS([-mms-bitfields])

if test "x$enable_PIC" != "xno"; then
 AC_CHECK_CFLAGS([-fPIC])
fi

AC_CHECK_CFLAGS([-mfpmath=sse -msse])

dnl if we don't have $LD set, we set it to $(CC)
dnl LD=${LD:=$CC}
if test "x$LD" = "x"
then
 if test "x$host" != "x"
 then
  LD=${host}-ld
  if $(which ${LD} > /dev/null)
  then
    :
  else
    LD=""
  fi
 fi
fi
LD=${LD:=$CC}



dnl if we don't have $STRIP set, we set it to ${host}-strip or strip
if test "x$STRIP" = "x"
then
 if test "x$host" != "x"
 then
  STRIP=${host}-strip
  if $(which ${host}-strip > /dev/null)
  then
    :
  else
    STRIP="echo fake strip"
  fi
 else
  STRIP=strip
 fi
fi
dnl STRIP=${STRIP:=strip}


dnl Checks for pd-version, to set the correct help-path
AC_MSG_CHECKING("pd>=0.37")

if test "$with_pdversion" != ""
then
  echo -n "($with_pdversion)... "
  PD_VERSION="$with_pdversion"
else
 if test "x$cross_compiling" = "xno"
 then

cat > conftest.c << EOF
#include <stdio.h>
#include "m_pd.h"
int main(){
  printf("%d.%d\n", PD_MAJOR_VERSION, PD_MINOR_VERSION);
  return 0;
}
EOF
  if $CC $CFLAGS -o conftest.o conftest.c > /dev/null 2>&1
  then
   PD_VERSION=`./conftest.o`
  else
   PD_VERSION=""
  fi
 else
  dnl we are cross-compiling...
  echo -n "(X)..."
  PD_VERSION="0.38"
 fi
fi

let PD_MAJORVERSION=`echo $PD_VERSION | cut -d"." -f1`+0
let PD_MINORVERSION=`echo $PD_VERSION | cut -d"." -f2`+0

if test "$PD_MAJORVERSION" -gt 0 || test "$PD_MINORVERSION" -ge 37
then
  REFERENCEPATH=extra/
  echo "yes"
else
  REFERENCEPATH=doc/5.reference/
  echo "no"
fi

dnl check for zexy-version (but why...)
AC_MSG_CHECKING("zexy-version")

if test "$with_version" != ""
then
  echo -n "($with_version)...forced "
  LIBRARY_VERSION="$with_version"
else
 if test "x$cross_compiling" = "xno"
 then
cat > conftest.c << EOF
#include <stdio.h>
#include "zexy.h"
int main(){
  printf("%s\n", VERSION);
  return 0;
}
EOF

  if $CC $CFLAGS $LDFLAGS -o conftest.o conftest.c > /dev/null 2>&1
  then
   LIBRARY_VERSION=$(./conftest.o)
   echo "$LIBRARY_VERSION"
  else
   LIBRARY_VERSION="X"
   echo "(unknown)"
  fi
 else
   LIBRARY_VERSION="X"
   echo "(X)"
 fi
fi


dnl
dnl  OK, checks which machines are here now
dnl this needs some rethinking when cross-compiling (?)
dnl
dnl AC_CHECK_LDFLAGS([-export_dynamic -shared])

if test "x$enable_exportdynamic" != "xno"; then
 AC_CHECK_LDFLAGS([-export_dynamic])
fi

AC_CHECK_LDFLAGS([-shared])

if test `uname -s` = Linux; 
then
  EXT=pd_linux	
  STRIPFLAGS="--strip-unneeded"
fi

dnl This should use '-bundle_loader /path/to/pd/bin/pd' instead of'-undefined suppress'
dnl then strip might do something
if test `uname -s` = Darwin; 
then
  LD=${CC}
  EXT=pd_darwin
  STRIP="echo faking strip"
  STRIPFLAGS=

  if test "$enable_lpt" = "yes"; then
    AC_MSG_ERROR("lpt not supported on this platform");
  fi
  if test "$enable_lpt" = ""; then
    AC_MSG_WARN("lpt not supported on this platform - disabling");
    enable_lpt="no";
  fi
fi

#AC_CHECK_LDFLAGS([-dynamiclib -mmacosx-version-min=10.3 -undefined dynamic_lookup],
#                  ,
	          AC_CHECK_LDFLAGS([-bundle -undefined suppress -flat_namespace])
#                 )

AC_CHECK_LDFLAGS([-shared pd.dll])

if test `uname -s` = IRIX64; 
then
  LDFLAGS="-shared,-rdata_shared"
  CFLAGS="-DUNIX -DIRIX -DN32 -woff 1080,1064,1185 -n32 -OPT:roundoff=3 -OPT:IEEE_arithmetic=3 -OPT:cray_ivdep=true"
  EXT=pd_irix6
  dnl DFLAGS="-DUNIX -DIRIX6"
  STRIPFLAGS="--strip-unneeded"
  if test "$enable_lpt" = "yes"; then
    AC_MSG_ERROR("lpt not supported on this platform");
  fi
  if test "$enable_lpt" = ""; then
    AC_MSG_WARN("lpt not supported on this platform - disabling");
    enable_lpt="no";
  fi
fi

if test `uname -s` = IRIX32; 
then
  LDFLAGS="-o32,-shared,-rdata_shared"
  CFLAGS="-o32 -DUNIX -DIRIX -O2"

  EXT=pd_irix5
  dnl DFLAGS="-DUNIX -DIRIX5"
  STRIPFLAGS="--strip-unneeded"
  if test "$enable_lpt" = "yes"; then
    AC_MSG_ERROR("lpt not supported on this platform");
  fi
  if test "$enable_lpt" = ""; then
    AC_MSG_WARN("lpt not supported on this platform - disabling");
    enable_lpt="no";
  fi
fi


if test "x$with_extension" != "x"
then
 EXT=$with_extension
fi

AC_CHECK_HEADERS(linux/ppdev.h, [ have_ppdev="yes" ], [ have_ppdev="no" ])

dnl check for LPT
AC_MSG_CHECKING("parallel-port")
if test "$enable_lpt" != "no"
then
  if test "$enable_lpt" = "yes"
  then
    AC_DEFINE([Z_WANT_LPT])
    echo "yes (forced)"
  else

cat > conftest.c << EOF
# include <sys/io.h>
int main(){
 ioperm(0x3bc, 8, 1);
 outb(0, 0x3bc);
 ioperm(0x3bc, 8, 0);
 return 0;
}
EOF

   if $CC $CFLAGS -o conftest.o conftest.c > /dev/null 2>&1
   then
     AC_DEFINE([Z_WANT_LPT])
     if test "x$have_ppdev" = "xyes"
     then
       echo "yes (with device-support)"
     else
       echo "yes"
     fi
   else
     echo "no"
   fi
  fi
else
 echo "no (ignored)"
fi


AC_CHECK_HEADERS(m_pd.h, , 
	cat config.log
	AC_ERROR([m_pd.h is desperately needed!
        install pd and/or use
        "--with-pd=</path/to/pd/>"     or
        "--includedir=</path/to/pd/src/>"]))

AC_OUTPUT(Make.config)

