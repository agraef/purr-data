dnl Process this file with autoconf to produce a configure script.

AC_INIT
AC_CANONICAL_HOST
AC_CONFIG_HEADER(include/pidip_config.h)

PDP_PIDIP_VERSION="0.12.26"
PD_DIR=/usr/local/pd
PDP_DIR=/usr/local/pd/pdp
FFMPEG_SOURCE_DIR=/SOURCES/ffmpeg
MPEG4IP_SOURCE_DIR=/SOURCES/mpeg4ip
PDP_STREAMING_OBJECTS=
PDP_CAPTURE_OBJECT=
PDP_DC1394_OBJECT=
IMLIB_LIBS=
IMLIB_CFLAGS=
MAGIC_LIBS=
MAGIC_CFLAGS=
THEORA_LIBS=
DC1394_LIBS=
enable_ffmpeg=no
enable_mpeg4ip=no
enable_lcapture=no

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

AC_ARG_WITH(enable-capture, [  --enable-capture        capture optional object ( only works with Image Magick <= 6.0.7 )])
if ! test -z "${enable_capture}"
then
  enable_lcapture=yes
fi

AC_ARG_WITH(ffmpeg, [  --with-ffmpeg=PATH      ffmpeg source tree])
if ! test -z "${with_ffmpeg}"
then
  ffmpeg_tree="`cd ${with_ffmpeg} 2>/dev/null && pwd`"
  if test -z "${ffmpeg_tree}"
  then
   dnl  The given directory can't be found
   AC_MSG_ERROR([ffmpeg sources not found in ${with_ffmpeg}])
  else
   FFMPEG_SOURCE_DIR=${ffmpeg_tree}
   enable_ffmpeg=yes
  fi
fi

AC_ARG_WITH(mpeg4ip, [  --with-mpeg4ip=PATH     mpeg4ip source tree])
if ! test -z "${with_mpeg4ip}"
then
  mpeg4ip_tree="`cd ${with_mpeg4ip} 2>/dev/null && pwd`"
  if test -z "${mpeg4ip_tree}"
  then
   dnl  The given directory can't be found
   AC_MSG_RESULT(no)
   AC_MSG_ERROR([mpeg4ip sources not found in ${with_mpeg4ip}])
  else
   MPEG4IP_SOURCE_DIR=${mpeg4ip_tree}
   enable_mpeg4ip=yes
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

AC_CHECK_LIB(Magick, XWindowByProperty, 
              [ PDP_CAPTURE_OBJECT="$PDP_CAPTURE_OBJECT"
              MAGICK_LIBS="-L/usr/X11R6/lib -lMagick `Magick-config --libs` `Magick-config --ldflags`"
              MAGICK_CFLAGS="-I/usr/X11R6/include `Magick-config --cflags` " ],
              echo "Image Magick not found : not building pdp_capture", 
              -I/usr/X11R6/include -L/usr/X11R6/lib )

AC_CHECK_LIB(dc1394, dc1394_camera_enumerate, 
              [ PDP_DC1394_OBJECT="pdp_dc1394.o"
              AC_DEFINE(HAVE_DC1394, 1, dc1394 cameras interface)
              DC1394_LIBS="-ldc1394 -lraw1394"],
              echo "dc1394 not found : not building pdp_dc1394", )

AC_CHECK_LIB(bz2, BZ2_bzBuffToBuffCompress, , AC_MSG_ERROR(streaming requires bz2 library!!))
AC_CHECK_LIB(z, deflate, , AC_MSG_ERROR(streaming requires compress library!!))
AC_CHECK_LIB(dl, dlopen, , AC_MSG_ERROR(streaming requires dynamic loader library!!))
AC_CHECK_LIB(mp3lame, InitMP3, , AC_MSG_ERROR(streaming requires lame library!!), -lm)
AC_CHECK_LIB(ogg, ogg_stream_init, , AC_MSG_ERROR(streaming requires ogg library!!), -lm)
AC_CHECK_LIB(vorbis, vorbis_analysis_init, , AC_MSG_ERROR(streaming requires vorbis library!!), -lm)
AC_CHECK_LIB(vorbisenc, vorbis_encode_setup_init, , AC_MSG_ERROR(streaming requires vorbis encoder library!!), -lm)
AC_CHECK_LIB(theora, theora_encode_init, , AC_MSG_ERROR(theora library needed! sorry...), -lm)
AC_CHECK_LIB(quicktime, lqt_decode_video, , AC_MSG_ERROR(libquicktime not found) )

qt_minor_version="`lqt-config --version | cut -f3 -d'.'`"
qt_major_version="`lqt-config --version | cut -f1 -d'.'`"
echo "qt version minor : ${qt_minor_version} major : ${qt_major_version}"
if test ${qt_minor_version} == "8";
then
  IMLIB_CFLAGS="$CFLAGS -DQUICKTIME_NEWER=1"
fi
if test ${qt_minor_version} == "9";
then
  IMLIB_CFLAGS="$CFLAGS -DQUICKTIME_NEWER=1"
fi
if test ${qt_minor_version} == "10";
then
  IMLIB_CFLAGS="$CFLAGS -DQUICKTIME_NEWER=1"
fi
if test ${qt_minor_version} == "11";
then
  IMLIB_CFLAGS="$CFLAGS -DQUICKTIME_NEWER=1"
fi
if test ${qt_minor_version} == "12";
then
  IMLIB_CFLAGS="$CFLAGS -DQUICKTIME_NEWER=1"
fi
if test ${qt_major_version} == "1";
then
  IMLIB_CFLAGS="$CFLAGS -DQUICKTIME_NEWER=1"
fi

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

if test $enable_lcapture == yes;
then
    PDP_CAPTURE_OBJECT="$PDP_CAPTURE_OBJECT pdp_capture.o"
    AC_DEFINE(HAVE_IMAGE_MAGICK, 1, image magick capture capability)
    PDP_PIDIP_LIBS="$MAGICK_LIBS $PDP_PIDIP_LIBS"
    PDP_PIDIP_INCLUDES="$MAGICK_CFLAGS $PDP_PIDIP_INCLUDES"
    echo "building pdp_capture."
fi

AC_CHECK_HEADER(linux/videodev2.h,
                PDP_CAPTURE_OBJECT="$PDP_CAPTURE_OBJECT pdp_v4l2.o pdp_vloopback.o"
                AC_DEFINE(HAVE_V4L2, 1, build pdp_v4l2 and pdp_vloopback),
                echo "   linux/videodev2.h not found: not building pdp_v4l2 and pdp_vloopback")

case "$host" in
  *-linux* )
      AC_CHECK_HEADER(libdv/dv.h,
                PDP_CAPTURE_OBJECT="$PDP_CAPTURE_OBJECT pdp_ieee1394l.o"
                PDP_PIDIP_LIBS="-ldv $PDP_PIDIP_LIBS"
                AC_DEFINE(HAVE_LIBDV, 1, build pdp_ieee1394 for linux),
                echo "   libdv/dv.h not found: not building pdp_ieee1394")
  ;;
esac

if test $enable_ffmpeg == yes;
then
  echo -n "looking for ffmpeg sources (required) ... "
  if test -f $FFMPEG_SOURCE_DIR/libavformat/avformat.h
  then
    AC_SUBST(FFMPEG_SOURCE_DIR)
    PDP_STREAMING_OBJECTS="pdp_live~.o pdp_ffmpeg~.o"
    AC_DEFINE(HAVE_PIDIP_FFMPEG, 1, build pdp_live~/pdp_ffmpeg~)
    PDP_PIDIP_LIBS="$PDP_PIDIP_LIBS $FFMPEG_SOURCE_DIR/libavformat/libavformat.a $FFMPEG_SOURCE_DIR/libavcodec/libavcodec.a"
    PDP_PIDIP_INCLUDES="$PDP_PIDIP_INCLUDES  -I$FFMPEG_SOURCE_DIR/libavcodec -I$FFMPEG_SOURCE_DIR/libavformat"
    echo "ok." 
  else
    echo "ffmpeg source tree not found... install it and use the --with-ffmpeg=<path> configuration option."
    exit -1
  fi
fi

if test $enable_mpeg4ip == yes;
then
   echo -n "looking for mpeg4ip sources (required) ... "
   if test -f $MPEG4IP_SOURCE_DIR/server/mp4live/mp4live_common.h
   then
    AC_SUBST(MPEG4IP_SOURCE_DIR)
    PDP_STREAMING_OBJECTS="pdp_mp4config.o pdp_mp4videosource.o pdp_mp4audiosource.o pdp_mp4live~.o pdp_mp4player~.o pdp_mp4audiosync.o pdp_mp4videosync.o pdp_mp4playermedia.o pdp_mp4playersession.o"
    AC_DEFINE(HAVE_PIDIP_MPEG4IP, 1, build pdp_mp4live~/pdp_mp4player~)
    PDP_PIDIP_LIBS="$PDP_PIDIP_LIBS $MPEG4IP_SOURCE_DIR/player/src/our_bytestream_file.o $MPEG4IP_SOURCE_DIR/player/src/our_config_file.o $MPEG4IP_SOURCE_DIR/player/src/qtime_file.o $MPEG4IP_SOURCE_DIR/player/src/mp4_file.o $MPEG4IP_SOURCE_DIR/player/src/mpeg3_file.o $MPEG4IP_SOURCE_DIR/player/src/avi_file.o $MPEG4IP_SOURCE_DIR/player/src/ip_port.o $MPEG4IP_SOURCE_DIR/player/src/media_utils.o $MPEG4IP_SOURCE_DIR/player/src/mpeg2t_thread.o $MPEG4IP_SOURCE_DIR/player/src/mpeg2t_thread_nx.o $MPEG4IP_SOURCE_DIR/player/src/*bytestream.o $MPEG4IP_SOURCE_DIR/player/src/*plugin.o $MPEG4IP_SOURCE_DIR/player/src/player_sdp.o $MPEG4IP_SOURCE_DIR/player/src/player_util.o $MPEG4IP_SOURCE_DIR/player/src/audio.o $MPEG4IP_SOURCE_DIR/player/src/video.o $MPEG4IP_SOURCE_DIR/server/mp4live/video_util_mpeg4.o $MPEG4IP_SOURCE_DIR/server/mp4live/sdp_file.o $MPEG4IP_SOURCE_DIR/server/mp4live/util.o $MPEG4IP_SOURCE_DIR/server/mp4live/video_encoder.o $MPEG4IP_SOURCE_DIR/server/mp4live/video_xvid.o $MPEG4IP_SOURCE_DIR/server/mp4live/media_source.o $MPEG4IP_SOURCE_DIR/server/mp4live/video_util_rgb.o $MPEG4IP_SOURCE_DIR/server/mp4live/video_util_resize.o $MPEG4IP_SOURCE_DIR/server/mp4live/resample.o $MPEG4IP_SOURCE_DIR/server/mp4live/video_encoder_base.o $MPEG4IP_SOURCE_DIR/server/mp4live/audio_encoder.o $MPEG4IP_SOURCE_DIR/server/mp4live/audio_encoder_base.o $MPEG4IP_SOURCE_DIR/server/mp4live/audio_encoder_tables.o $MPEG4IP_SOURCE_DIR/server/mp4live/audio_lame.o $MPEG4IP_SOURCE_DIR/server/mp4live/audio_faac.o $MPEG4IP_SOURCE_DIR/server/mp4live/rtp_transmitter.o $MPEG4IP_SOURCE_DIR/server/mp4live/h261/.libs/*.a $MPEG4IP_SOURCE_DIR/lib/sdp/.libs/libsdp.so $MPEG4IP_SOURCE_DIR/player/lib/mp4util/.libs/libmp4util.so $MPEG4IP_SOURCE_DIR/player/lib/libhttp/.libs/libhttp.a $MPEG4IP_SOURCE_DIR/lib/config_file/.libs/libconfig_file.so $MPEG4IP_SOURCE_DIR/lib/msg_queue/.libs/libmsg_queue.so $MPEG4IP_SOURCE_DIR/lib/mp4av/.libs/libmp4av.so $MPEG4IP_SOURCE_DIR/lib/mp4/.libs/libmp4.so $MPEG4IP_SOURCE_DIR/lib/mp4v2/.libs/libmp4v2.so $MPEG4IP_SOURCE_DIR/lib/SDL/src/.libs/libSDL.so $MPEG4IP_SOURCE_DIR/lib/rtp/*.o $MPEG4IP_SOURCE_DIR/player/lib/rtsp/.libs/librtsp.a $MPEG4IP_SOURCE_DIR/common/video/libmpeg32/.libs/*.a $MPEG4IP_SOURCE_DIR/lib/avi/.libs/libavi.al $MPEG4IP_SOURCE_DIR/lib/mpeg2t/.libs/libmpeg2_transport.al $MPEG4IP_SOURCE_DIR/player/lib/audio/mp3util/.libs/libmp3utils.al -lm -L/usr/X11R6/lib -lX11 -lXext -lpthread $MPEG4IP_SOURCE_DIR/lib/SDL/src/main/libSDLmain.a -ldl -Wl,--rpath -Wl,/usr/local/lib -lfaac -lmpeg4ip_xvid"
    PDP_PIDIP_INCLUDES="$PDP_PIDIP_INCLUDES -I/usr/include/SDL -I/usr/local/include/SDL -I$MPEG4IP_SOURCE_DIR/server/mp4live -I$MPEG4IP_SOURCE_DIR/player/src -I$MPEG4IP_SOURCE_DIR/player/lib -I$MPEG4IP_SOURCE_DIR/lib/msg_queue -I$MPEG4IP_SOURCE_DIR/lib"
    MPEG4IP_CFLAGS="-D_REENTRANT -DNOCONTROLS -fexceptions -Wno-char-subscripts -Wno-unknown-pragmas -Wno-deprecated -DPLAYER_PLUGIN_DIR=\"/usr/local/lib/mp4player_plugin\" -g -O2 -DUSE_MMX -DMPEG4IP"
    AC_SUBST(MPEG4IP_CFLAGS)
    echo "ok." 
  else
    echo "mpeg4ip source tree not found... install it and use the --with-mpeg4ip=<path> configuration option."
    exit -1
  fi
fi

AC_SUBST(PDP_PIDIP_VERSION)
AC_SUBST(IMLIB_LIBS)
AC_SUBST(IMLIB_CFLAGS)
AC_SUBST(THEORA_LIBS)
AC_SUBST(DC1394_LIBS)
AC_SUBST(PDP_STREAMING_OBJECTS)
AC_SUBST(PDP_CAPTURE_OBJECT)
AC_SUBST(PDP_DC1394_OBJECT)
AC_SUBST(PDP_PIDIP_LIBS)
AC_SUBST(PDP_PIDIP_INCLUDES)

AC_CONFIG_FILES([
Makefile
system/Makefile
modules/Makefile
])
AC_OUTPUT

echo "used configure options:"
echo "  --enable_ffmpeg=$enable_ffmpeg"
echo "  --enable_mpeg4ip=$enable_mpeg4ip"
