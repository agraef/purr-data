# ChangeLog:
# 2007-07-28 L. Donnie Smith <cwiid@abstrakraft.org>
# * added config.h header
# * added with-python argument and associated logic
#
# 2007-05-30 L. Donnie Smith <cwiid@abstrakraft.org>
# * added python/Makefile
# * added PYTHON_VERSION variable
#
# 2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
# * updated for libcwiid rename
#
# 2007-04-04 L. Donnie Smith <cwiid@abstrakraft.org>
# * removed --noyywrap from lex
#
# 2007-04-01 L. Donnie Smith <cwiid@abstrakraft.org>
# * added lswm/Makefile
# * moved defs.mak
#
# 2007-03-19 L. Donnie Smith <cwiid@abstrakraft.org>
# * added --disable-ldconfig
#
# 2007-03-01 L. Donnie Smith <cwiid@abstrakraft.org>
# * Initial ChangeLog
# * Check for stdint.h

AC_INIT(CWiid,0.6.00,cwiid@abstrakraft.org)
AC_CONFIG_HEADER(common/include/config.h)

AC_PROG_CC
AC_PROG_AWK
AC_PROG_LEX
if test "$LEX" != "flex"; then
	AC_MSG_ERROR([flex not found])
fi
if test -z "`echo %%|$LEX -t|grep yypop_buffer_state`"; then
	AC_MSG_ERROR([flex missing yypop_buffer_state - upgrade to version 2.5.33 or later])
fi
AC_PROG_YACC
if test "$YACC" != "bison -y"; then
	AC_MSG_ERROR([bison not found])
fi

AC_ARG_WITH(
	[python],
	[AS_HELP_STRING([--without-python],[compile without python support])],
	[case $withval in 
		yes)
			REQUIRE_PYTHON=1
			PYTHON_NAME=python
		;;
		no)
			REQUIRE_PYTHON=
			PYTHON_NAME=
		;;
		*)
			REQUIRE_PYTHON=1
			PYTHON_NAME=$withval
		;;
	esac],
	[REQUIRE_PYTHON=1; PYTHON_NAME=python])
if test $REQUIRE_PYTHON; then
	AC_CHECK_PROGS([PYTHON],$PYTHON_NAME)
	if test $REQUIRE_PYTHON -a ! $PYTHON; then
		AC_MSG_ERROR([$PYTHON_NAME not found])
	fi
fi
AC_SUBST(PYTHON)
if test $PYTHON; then
	PYTHON_VERSION=[`$PYTHON -c 'import sys; print sys.version[:3]'`]
	AC_SUBST(PYTHON_VERSION)
	AC_DEFINE([HAVE_PYTHON],1,[Define to 1 if python support is enabled])
fi

AC_CHECK_LIB([pthread], [pthread_create],,
	AC_MSG_ERROR([pthread library not found]))
AC_CHECK_LIB([bluetooth], [hci_devid],,
	AC_MSG_ERROR([bluetooth library not found]))
AC_CHECK_LIB([dl], [dlopen],,
	AC_MSG_ERROR([dl lib not found]))
#AC_CHECK_LIB([rt], [clock_gettime],,
#	AC_MSG_ERROR([rt lib not found]))

AC_HEADER_STDC
AC_CHECK_HEADER([stdint.h],,
	AC_MSG_ERROR([stdint.h not found]))
AC_CHECK_HEADER([bluetooth/bluetooth.h],,
	AC_MSG_ERROR([bluetooth/bluetooth.h not found]))
AC_CHECK_HEADERS([bluetooth/l2cap.h bluetooth/hci.h],,
	AC_MSG_ERROR([bluetooth headers not found]),
	[#include <bluetooth/bluetooth.h>])
AC_CHECK_HEADER([linux/input.h],,
	AC_MSG_ERROR([linux/input.h not found]))
AC_CHECK_HEADER([linux/uinput.h],,
	AC_MSG_ERROR([linux/uinput.h]),
	[#include <linux/input.h>])

AC_ISC_POSIX

AC_ARG_WITH(cwiid-config-dir,AC_HELP_STRING([--with-cwiid-config-dir],
	[CWiid configuration directory, default to $(sysconfdir)/cwiid]))
if test -z $with_cwiid_config_dir; then
	CWIID_CONFIG_DIR='${sysconfdir}/cwiid'
else
	CWIID_CONFIG_DIR=$with_cwiid_config_dir
fi
AC_SUBST(CWIID_CONFIG_DIR)

AC_ARG_WITH(cwiid-plugins-dir,AC_HELP_STRING([--with-cwiid-plugins-dir],
	[CWiid plugins directory, default to $(libdir)/cwiid/plugins]))
if test -n $with_cwiid_plugins_dir; then
	CWIID_PLUGINS_DIR='${libdir}/cwiid/plugins'
else
	CWIID_PLUGINS_DIR=$with_cwiid_plugins_dir
fi
AC_SUBST(CWIID_PLUGINS_DIR)

AC_ARG_WITH(ldconfig,AC_HELP_STRING([--disable-ldconfig],
	[don't execute ldconfig after install]))
if test "$enable_ldconfig" = "no"; then
	LDCONFIG="#ldconfig"
else
	LDCONFIG="ldconfig"
fi
AC_SUBST(LDCONFIG)

pkg_modules="gtk+-2.0 >= 2.0.0 gthread-2.0"
PKG_CHECK_MODULES([GTK], [$pkg_modules])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

AC_OUTPUT(
	[Makefile]
	[defs.mak]
	[common/include/lib.mak]
	[doc/Makefile]
	[man/Makefile]
	[libcwiid/Makefile]
	[libcwiid/cwiid.pc]
	[wmdemo/Makefile]
	[wmgui/Makefile]
	[wminput/Makefile]
	[wminput/plugins/Makefile]
	[wminput/plugins/ir_ptr/Makefile]
	[wminput/plugins/acc/Makefile]
	[wminput/plugins/nunchuk_acc/Makefile]
	[wminput/plugins/led/Makefile]
 	[wminput/plugins/nunchuk_stick2btn/Makefile]
	[lswm/Makefile]
	[python/Makefile]
	)

