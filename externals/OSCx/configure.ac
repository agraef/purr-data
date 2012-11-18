#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(OSCxpd, 0.2, jdl@xdv.org)
AC_CONFIG_SRCDIR([libOSC/OSC-client.h])
AC_CONFIG_HEADER([config.h])

# get system type
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AC_PROG_LN_S
# AC_PROG_CXX
AC_PROG_CPP
# AC_PROG_INSTALL

dnl ----------------------------------
dnl  here you can see that autoconf adds "-g -02" as a default
dnl  these are flags for debugging, and will add extra weight 
dnl  to you external.  CFLAGS, CPPFLAGS, etc are default variables
dnl ----------------------------------
echo "default flags"
# echo "$CPPFLAGS"
# echo "$CXXFLAGS"
echo cflags "$CFLAGS"
echo ldflags "$LDFLAGS"

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lc':
AC_CHECK_LIB([c], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/file.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h sys/prctl.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_HEADER_TIME
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([bzero gethostbyname inet_ntoa select socket strchr])
AC_TYPE_SIGNAL

# from augusts template

dnl ------------------------------------------
dnl  for each of the *-*-host's in the following, do the checking and then set your CFLAGS, CPPFLAGS and LDFLAGS
dnl  CFLAGS get set for C code, CPPFLAGS for c++ code.  LDFLAGS is the linking flags for both c and c++
dnl ------------------------------------------

INCLUDES="-I../libOSC -I../../pd/src -I../../../pd/src -I../src"
LIBOSC=libOSC.a
if test -z "$GCC"; then
        case $host in 
        *-*-irix*)
                dnl If we're on IRIX, we wanna use cc even if gcc 
                dnl is there (unless the user has overriden us)...
                if test -z "$CC"; then
                        CC=cc
                fi
		;;
        sparc-sun-solaris*)
                CFLAGS="-xO4 -fast -w -fsimple -native -xcg92"
					 ;;
        *)
                CFLAGS="-O" 
					 ;;
        esac
else
        case $host in 
        *86*-linux*)
                CFLAGS="$CFLAGS -DUNIX -Wall -Wimplicit -Wunused -Wmissing-prototypes -O2 -fPIC"
					 LDFLAGS="-Wl,--export-dynamic -shared"
					 dnl we could test for bad glibc here, but don't
					 pd_suffix=pd_linux
					 LIBS="-lc -lm"
					 ;;
        powerpc-*-linux*)
                CFLAGS="$CFLAGS -DUNIX -Wall -Wimplicit -Wunused -Wmissing-prototypes -O1"
					 LDFLAGS="-Wl,--export-dynamic -shared"
					 pd_suffix=pd_linux
					 LIBS="-lc -lm"
					 ;;
        *-*-linux*)
                CFLAGS="$CFLAGS -DUNIX -Wall -Wimplicit -Wunused -Wmissing-prototypes -O1"
					 LDFLAGS="-Wl,--export-dynamic -shared"
					 INCLUDES="$INCLUDES"
					 pd_suffix=pd_linux
					 LIBS="-L../../../pd/bin -lpd -lc -lm"
					 ;;
        sparc-sun-*)
		          echo "YOU HAVE A SPARC STATION,	not setting any flags, not supported yet"
			       ;;
        *-*-cygwin*)
                CFLAGS="$CFLAGS -mno-cygwin -Wall -Wimplicit -Wunused -Wmissing-prototypes -O3 "
					 LDFLAGS="-Wl,--export-dynamic -shared"
					 INCLUDES="$INCLUDES -I../../../pd/src -I/cygdrive/MinGW/include"
					 pd_suffix=dll
					 LIBS="-L../../../pd/src -L../../../pd/bin -L../../../pd/obj -lpd -lm -lwsock32"
					 ;;
        *-*-mingw*)
                CFLAGS="$CFLAGS -mms-bitfields -Wall -Wimplicit -Wunused -Wmissing-prototypes -O3 "
					 LDFLAGS="-shared"
					 INCLUDES="$INCLUDES -I../../../pd/src"
					 pd_suffix=dll
					 LIBS="-L../../../pd/src -L../../../pd/bin -L../../../pd/obj -lpd -lm -lwsock32"
					 ;;
        *-*-darwin*)
                CFLAGS="$CFLAGS -DUNIX -Wall -Wimplicit -Wunused -Wmissing-prototypes -O3 "
					 LDFLAGS=" -bundle -bundle_loader ../../../pd/bin/pd -flat_namespace "
					 pd_suffix=pd_darwin
					 LIBS="-lc -lm"
					 ;;
        *)
					 dnl assume unix
                CFLAGS="$CFLAGS -DUNIX -Wall -Wimplicit -Wunused -Wmissing-prototypes -O1"
					 LDFLAGS="-Wl,--export-dynamic -shared"
					 pd_suffix=pd_linux
					 LIBS="-lc -lm"
					 ;;
        esac
fi

CFLAGS="$CFLAGS $INCLUDES"

echo "Using cflags= $CFLAGS"
echo "Using ldflags= $LDFLAGS"
echo "Using includes= $INCLUDES"



dnl ------------------------------------------
dnl ----  add PureData includes dir 
dnl ----  usually /usr/local/include
dnl ------------------------------------------
AC_ARG_WITH(pd_dir, 
	[  --with-pd-dir=path	pd header path (default=/usr/local/include) ], 
	[
	CPPFLAGS="$CPPFLAGS -I$withval"
	echo
	echo "pd dir is $withval"
	echo
	])

if test $prefix == NONE;
then
	prefix=/usr/local
fi

dnl ------------------------------------------
dnl ---- check for PureData Header
dnl ------------------------------------------
AC_CHECK_HEADER(m_pd.h, [have_pd_hdr=yes ], [
	have_pd_hdr=no
	echo
	echo "no m_pd.h header found.  try with option --with-pd-dir=/path/to/pd/src"
	echo
	exit
	])


AC_SUBST(LIBOSC)
AC_SUBST(pd_suffix)
AC_SUBST(INCLUDES)
AC_SUBST(CFLAGS)

AC_CONFIG_FILES([src/Makefile
                 libOSC/Makefile
                 send+dump/Makefile
		 Makefile])

AC_OUTPUT
