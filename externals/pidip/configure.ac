dnl Process this file with autoconf to produce a configure script.

AC_INIT
AC_CANONICAL_HOST
AC_DISABLE_OPTION_CHECKING
AC_CONFIG_HEADER(include/pidip_config.h)

PDP_PIDIP_VERSION="0.12.30"
PD_DIR=/usr/local/pd
ARTKP_DIR=/usr/src/ARToolKitPlus
PDP_DIR=/usr/local/pd/pdp
PDP_CAPTURE_OBJECT=
PDP_DC1394_OBJECT=
PDP_FREENECT_OBJECT=
PDP_OPENCV_OBJECT=
PDP_ARTKP_OBJECT=
IMLIB_LIBS=
IMLIB_CFLAGS=
MAGIC_LIBS=
MAGIC_CFLAGS=
THEORA_LIBS=
DC1394_LIBS=
FREENECT_LIBS=
OPENCV_LIBS=
ARTKP_CPPFLAGS=
ARTKP_LIBS=

echo $host
# Check for DarwinPorts and/or Fink on Mac OS X/Darwin
case "$host" in
  *-darwin* | *-macos10*)
     PDP_CAPTURE_OBJECT="pdp_ieee1394.o"
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

AC_ARG_WITH(artkpsources, [  --with-artkp=PATH       ARToolKitPlus source tree])
if ! test -z "${with_artkp}"
then
  artkp_tree="`cd ${with_artkp} 2>/dev/null && pwd`"
  if test -z "${artkp_tree}"
  then
   dnl  The given directory can't be found
   AC_MSG_ERROR([ARToolKitPlus sources not found in ${with_artkp}])
  else
   ARTKP_DIR=${artkp_tree}
  fi
fi

PDP_PIDIP_LIBS="$PDP_PIDIP_LIBS -lbz2 -lz -ldl -lmp3lame -logg -lvorbis -lvorbisenc"
PDP_PIDIP_INCLUDES="-I$PD_DIR/src -I. -I$PDP_DIR/include -I../include -I../charmaps"

make clean

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_SANITY_CHECK

AM_MAINTAINER_MODE
AM_WITH_DMALLOC

AC_PATH_GENERIC(imlib2, 1.0.0, 
  [ IMLIB_LIBS=`imlib2-config --libs`
  IMLIB_CFLAGS=`imlib2-config --cflags` ],
  AC_MSG_ERROR(Cannot find imlib2: Is imlib2-config in the path?) )

AC_CHECK_LIB(dc1394, dc1394_camera_enumerate, 
              [ PDP_DC1394_OBJECT="pdp_dc1394.o"
              AC_DEFINE(HAVE_DC1394, 1, dc1394 cameras interface)
              DC1394_LIBS="-ldc1394 -lraw1394"],
              echo "dc1394 not found : not building pdp_dc1394", )

AC_CHECK_LIB(freenect, freenect_init, 
              [ PDP_FREENECT_OBJECT="pdp_freenect.o"
              AC_DEFINE(HAVE_FREENECT, 1, M$ kinect usb capture utility)
              PDP_PIDIP_INCLUDES="`pkg-config --cflags libfreenect` $PDP_PIDIP_INCLUDES"
              FREENECT_LIBS="`pkg-config --libs libfreenect` -lfreenect"],
              echo "freenect not found : not building pdp_freenect", )

AC_CHECK_LIB([cv], [cvSobel], [AC_DEFINE(HAVE_OPENCV, 1, OpenCV useful library)
  OPENCV_FOUND="1"
], [
  # try another lib name used in other versions of OpenCV
  AC_CHECK_LIB([opencv_imgproc], [cvSobel], [AC_DEFINE(HAVE_OPENCV, 1, OpenCV useful library)
  OPENCV_FOUND="1"
  ],
    [AC_MSG_WARN([[missing a library from opencv, not building pdp_joint - try installing libcv]])
  ])
])

if test -n "$OPENCV_FOUND"; then
   echo "OpenCV found, yes! ====================="
   PDP_OPENCV_OBJECT="pdp_joint.o pdp_shapes.o"
   OPENCV_LIBS="`pkg-config --libs opencv`"
fi

AC_LANG(C++)
AC_LANG(C++)
SAVED_LDFLAGS=$LDFLAGS
CXXFLAGS="-I$ARTKP_DIR $CXXFLAGS  -fPIC"
LDFLAGS="$LDFLAGS -lARToolKitPlus"
AC_LINK_IFELSE(
        [AC_LANG_PROGRAM([#include <ARToolKitPlus/TrackerMultiMarker.h>],
          [ARToolKitPlus::TrackerMultiMarker *dummy])],
        [AC_MSG_RESULT([libARToolKitPlus found ... building pdp_artkp.])
        PDP_ARTKP_OBJECT="pdp_artkp.o" 
        ARTKP_LIBS="-lARToolKitPlus"
        ARTKP_CPPFLAGS="-I$ARTKP_DIR -fPIC"
        AC_DEFINE(HAVE_ARTKP, 1, augmented reality toolkit plus)],
        [AC_MSG_WARN([libARToolKitPlus is not installed.])])
LDFLAGS=$SAVED_LDFLAGS

AC_CHECK_LIB(bz2, BZ2_bzBuffToBuffCompress, , AC_MSG_ERROR(streaming requires bz2 library!!))
AC_CHECK_LIB(z, deflate, , AC_MSG_ERROR(streaming requires compress library!!))
AC_CHECK_LIB(dl, dlopen, , AC_MSG_ERROR(streaming requires dynamic loader library!!))
AC_CHECK_LIB(mp3lame, InitMP3, , AC_MSG_ERROR(streaming requires lame library!!), -lm)
AC_CHECK_LIB(ogg, ogg_stream_init, , AC_MSG_ERROR(streaming requires ogg library!!), -lm)
AC_CHECK_LIB(vorbis, vorbis_analysis_init, , AC_MSG_ERROR(streaming requires vorbis library!!), -lm)
AC_CHECK_LIB(vorbisenc, vorbis_encode_setup_init, , AC_MSG_ERROR(streaming requires vorbis encoder library!!), -lm)
AC_CHECK_LIB(theora, theora_encode_init, , AC_MSG_ERROR(theora library needed! sorry...), -lm)
AC_CHECK_LIB(quicktime, lqt_decode_video, , AC_MSG_ERROR(libquicktime not found) )

IMLIB_CFLAGS="$CFLAGS -DQUICKTIME_NEWER=1"

THEORA_LIBS="-ltheora -logg -lvorbis -lvorbisenc"
if test -f /usr/lib/libtheora.a
then
   THEORA_LIBS="$THEORA_LIBS /usr/lib/libtheora.a"
fi
if test -f /usr/local/lib/libtheora.a
then
   THEORA_LIBS="$THEORA_LIBS /usr/local/lib/libtheora.a"
fi
if test -f /usr/lib/libogg.a
then
   THEORA_LIBS="$THEORA_LIBS /usr/lib/libogg.a"
fi
if test -f /usr/local/lib/libogg.a
then
   THEORA_LIBS="$THEORA_LIBS /usr/local/lib/libogg.a"
fi
if test -f /usr/lib/libvorbis.a
then
   THEORA_LIBS="$THEORA_LIBS /usr/lib/libvorbis.a"
fi
if test -f /usr/local/lib/libvorbis.a
then
   THEORA_LIBS="$THEORA_LIBS /usr/local/lib/libvorbis.a"
fi
if test -f /usr/lib/libvorbisenc.a
then
   THEORA_LIBS="$THEORA_LIBS /usr/lib/libvorbisenc.a"
fi
if test -f /usr/local/lib/libvorbisenc.a
then
   THEORA_LIBS="$THEORA_LIBS /usr/local/lib/libvorbisenc.a"
fi

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

AC_CHECK_HEADER(libv4l1-videodev.h,
                PDP_CAPTURE_OBJECT="$PDP_CAPTURE_OBJECT pdp_vloopback.o"
                AC_DEFINE(HAVE_V4L, 1, build pdp_vloopback),
                echo "   libv4l1-videodev.h not found: not building pdp_vloopback")

AC_CHECK_HEADER(linux/videodev2.h,
                PDP_CAPTURE_OBJECT="$PDP_CAPTURE_OBJECT pdp_v4l2.o"
                AC_DEFINE(HAVE_V4L2, 1, build pdp_v4l2),
                echo "   linux/videodev2.h not found: not building pdp_v4l2")

case "$host" in
  *-linux* )
      AC_CHECK_HEADER(libdv/dv.h,
                PDP_CAPTURE_OBJECT="$PDP_CAPTURE_OBJECT pdp_ieee1394l.o"
                PDP_PIDIP_LIBS="-ldv $PDP_PIDIP_LIBS"
                AC_DEFINE(HAVE_LIBDV, 1, build pdp_ieee1394 for linux),
                echo "   libdv/dv.h not found: not building pdp_ieee1394")
  ;;
esac

AC_SUBST(PDP_PIDIP_VERSION)
AC_SUBST(IMLIB_LIBS)
AC_SUBST(IMLIB_CFLAGS)
AC_SUBST(THEORA_LIBS)
AC_SUBST(DC1394_LIBS)
AC_SUBST(FREENECT_LIBS)
AC_SUBST(OPENCV_LIBS)
AC_SUBST(ARTKP_CPPFLAGS)
AC_SUBST(ARTKP_LIBS)
AC_SUBST(PDP_CAPTURE_OBJECT)
AC_SUBST(PDP_DC1394_OBJECT)
AC_SUBST(PDP_FREENECT_OBJECT)
AC_SUBST(PDP_OPENCV_OBJECT)
AC_SUBST(PDP_ARTKP_OBJECT)
AC_SUBST(PDP_PIDIP_LIBS)
AC_SUBST(PDP_PIDIP_INCLUDES)

AC_CONFIG_FILES([
Makefile
system/Makefile
modules/Makefile
])
AC_OUTPUT
