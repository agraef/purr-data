/*
 *   PiDiP module.
 *   Copyright (c) by Yves Degoyon (ydegoyon@free.fr)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*  This object is a stream decoder object
 *  A lot of this object code is inspired by the excellent ffmpeg.c
 *  Copyright (c) 2000, 2001, 2002 Fabrice Bellard
 *  The rest is written by Yves Degoyon ( ydegoyon@free.fr )                             
 */


#include "pdp.h"
#include "yuv.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <avformat.h>
#include <sys/types.h>
#include <signal.h>

#define VIDEO_BUFFER_SIZE (1024*1024)
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)
#define MIN_AUDIO_SIZE (512)
#define AUDIO_PACKET_SIZE (2*1152)

#define DEFAULT_CHANNELS 1
#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240
#define DEFAULT_FRAME_RATE 25
#define END_OF_STREAM 20
#define MIN_PRIORITY 0
#define DEFAULT_PRIORITY 1
#define MAX_PRIORITY 20

/* a trick to cope with ffmpeg versions */
#ifndef AVFMT_NOHEADER
#define AVFMT_NOHEADER AVFMTCTX_NOHEADER
#endif

static char   *pdp_live_version = "pdp_live~: version 0.1, a video stream decoder ( ydegoyon@free.fr).";

typedef struct pdp_live_struct
{
    t_object x_obj;
    t_float x_f;

    int x_packet0;
    int x_dropped;

    t_pdp *x_header;
    unsigned char *x_data;
    int x_vwidth;
    int x_vheight;
    int x_vsize;

    t_outlet *x_pdp_out;           // output decoded pdp packets
    t_outlet *x_outlet_left;       // left audio output
    t_outlet *x_outlet_right;      // right audio output
    t_outlet *x_outlet_streaming;  // indicates the action of streaming
    t_outlet *x_outlet_nbframes;   // number of frames emitted
    t_outlet *x_outlet_framerate;  // real framerate
    t_outlet *x_outlet_endofstream;// for signaling the end of the stream

    pthread_t x_connectchild;      // thread used for connecting to a stream
    pthread_t x_decodechild;       // stream decoding thread
    pthread_mutex_t x_audiolock;   // audio mutex
    pthread_mutex_t x_videolock;   // video mutex
    int x_usethread;             // flag to activate decoding in a thread
    int x_autoplay;              // flag to autoplay the file ( default = true )
    int x_nextimage;             // flag to play next image in manual mode
    int x_priority;              // priority of decoding thread

    char  *x_url;
    int x_streaming;      // streaming flag
    int x_decoding;       // decoding flag
    int x_loop;           // looping flag ( default = on )
    int x_nopackets;      // no packet to decode
    int x_endofstream;    // end of the stream reached
    int x_nbframes;       // number of frames emitted
    int x_framerate;      // framerate
    int x_samplerate;     // audio sample rate
    int x_audiochannels;  // audio channels
    int x_audioon;        // enough audio data to start playing
    int x_blocksize;      // audio block size
    struct timeval x_starttime; // streaming starting time
    int x_cursec;         // current second
    int x_secondcount;    // number of frames received in the current second
    int x_nbvideostreams; // number of video streams
    int x_nbaudiostreams; // number of audio streams
    int x_videoindex;     // index of the first video stream

      /* AV data structures */
    AVFormatContext  *x_avcontext;
    AVFormatParameters x_avparameters; // unused but the call is necessary to allocate structures
    AVPacket x_pkt;                    // packet received on the stream
    AVPicture x_picture_decoded;
    long long int x_pts;               // presentation time stamp
    long long int x_previouspts;       // previous presentation time stamp
    long long int x_firstpts;          // first presentation time stamp ( time origin )
    int x_newpicture;

      /* audio structures */
    int x_audio;           // flag to activate the decoding of audio
    short x_audio_buf[4*MAX_AUDIO_PACKET_SIZE]; /* buffer for audio from stream*/
    short x_audio_in[4*MAX_AUDIO_PACKET_SIZE]; /* buffer for resampled PCM audio */
    int x_audioin_position; // writing position for incoming audio
    ReSampleContext *x_audio_resample_ctx; // structures for audio resample

} t_pdp_live;

static void pdp_live_priority(t_pdp_live *x, t_floatarg fpriority )
{
   if ( ( x->x_priority >= MIN_PRIORITY ) && ( x->x_priority <= MAX_PRIORITY ) )
   {
     x->x_priority = (int)fpriority;
   }
}

static void pdp_live_threadify(t_pdp_live *x, t_floatarg fusethread )
{
   if ( ( fusethread == 0 ) || ( fusethread == 1 ) ) 
   {
      x->x_usethread = (int)fusethread;
   }
}

static void pdp_live_audio(t_pdp_live *x, t_floatarg faudio )
{
   if ( ( faudio == 0. ) || ( faudio == 1. ) )
   {
      x->x_audio = (int)faudio;
   }
}

static void pdp_live_autoplay(t_pdp_live *x, t_floatarg fautoplay )
{
   if ( ( fautoplay == 0. ) || ( fautoplay == 1. ) )
   {
      x->x_autoplay = (int)fautoplay;
   }
}

static void pdp_live_loop(t_pdp_live *x, t_floatarg floop )
{
   if ( ( floop == 0. ) || ( floop == 1. ) )
   {
      x->x_loop = (int)floop;
   }
}

static void pdp_live_bang(t_pdp_live *x)
{
   if ( x->x_nextimage == 1 )
   {
      // post( "pdp_live~ : banging too fast, previous image is not decoded yet... ignored" );
      return;
   }
   x->x_nextimage = 1;
}

static void pdp_live_frame_cold(t_pdp_live *x, t_floatarg frameindex)
{
    int frame = (int)frameindex;
    int ret, flags=0;
    uint64_t newpts;

    if (!(x->x_streaming)) return;

#if FFMPEG_VERSION_INT >= 0x000409
    if ( ( ( x->x_avcontext->iformat->flags & AVFMT_NOFILE ) == 0 ) && ( x->x_videoindex != -1 ) )
    {
      if ( x->x_framerate != 0 ) 
      {
        newpts = x->x_firstpts + ( frame * 1000000 ) / x->x_framerate;
      }
      else
      {
        post( "pdp_live~ : couldn't seek requested frame ( framerate = %d )", x->x_framerate );
        return;
      }
      if ( frame < x->x_nbframes ) flags = AVSEEK_FLAG_BACKWARD;
      if ( ( ret = av_seek_frame(x->x_avcontext, x->x_videoindex, newpts, flags) ) < 0 )
      {
        post( "pdp_live~ : couldn't seek the requested frame (ret=%d)", ret );
      }
    }
#else
    post( "pdp_live~ : no seek function with ffmpeg < 4.0.9");
#endif
} 

static int pdp_live_decode_packet(t_pdp_live *x)
{
  int chunksize=0, length, err, ret;
  int audiosize, sizeout, imagesize, pictureok;
  AVFrame frame;
  uint8_t *pcktptr;
  unsigned char *pY, *pU, *pV; 
  uint8_t *psY, *psU, *psV; 
  int px, py;
  long long tplaying;
  long long ttheoretical;
  struct timeval ctime;
  struct timespec mwait;

   if ( !x->x_streaming )
   {
      return -1;
   }

   // post( "pdp_live~ : trying to read packet" );
   // read new packet on the stream
   if ( x->x_streaming && av_read_packet(x->x_avcontext, &x->x_pkt) < 0) 
   {
      x->x_nopackets++;
      // post( "pdp_live~ : decoding thread : nothing to decode : no packets :%d", x->x_nopackets );
      if ( x->x_nopackets > END_OF_STREAM )
      {
        x->x_endofstream = 1;
        if ( x->x_loop )
        {

#if FFMPEG_VERSION_INT >= 0x000409

          if ( ( x->x_avcontext->iformat->flags & AVFMT_NOFILE ) == 0 )
          {
            post( "pdp_live~ : looping file reading..." );
            if ( ( ret = av_seek_frame(x->x_avcontext, x->x_videoindex, 0, 0) ) < 0 )
            {
               post( "pdp_live~ : couldn't seek the requested frame (ret=%d)", ret );
            }
            else
            {
              x->x_endofstream = 0;
            }
          }

#else
          post( "pdp_live~ : no seek function with ffmpeg < 4.0.9");
#endif
        }
      }
      return -1;
   }
   // post( "pdp_live~ : read packet ( size=%d )", x->x_pkt.size );
   x->x_nopackets = 0;

   if (x->x_pkt.stream_index >= x->x_avcontext->nb_streams)
   {
      post("pdp_live~ : stream received out of range !! ");
      return -1;
   }

   length = x->x_pkt.size;
   pcktptr = x->x_pkt.data;
   while (length > 0) 
   {
         if ( !x->x_streaming ) break;
#if FFMPEG_VERSION_INT >= 0x000409
         switch(x->x_avcontext->streams[x->x_pkt.stream_index]->codec->codec_type) 
#else
         switch(x->x_avcontext->streams[x->x_pkt.stream_index]->codec.codec_type) 
#endif
         {
             case CODEC_TYPE_AUDIO:
                    if ( !x->x_audio )
                    {
                      av_free_packet(&x->x_pkt);
                      return 0;
                    }
#if FFMPEG_VERSION_INT >= 0x000409
                    chunksize = avcodec_decode_audio(x->x_avcontext->streams[x->x_pkt.stream_index]->codec, 
#else
                    chunksize = avcodec_decode_audio(&x->x_avcontext->streams[x->x_pkt.stream_index]->codec, 
#endif
                                               &x->x_audio_buf[0], &audiosize,
                                               pcktptr, length);
                    if (chunksize < 0)
                    {
                       post("pdp_live~ : could not decode audio input (ret=%d)", chunksize );
                       av_free_packet(&x->x_pkt);
                       continue;
                    }
                    // some bug in mpeg audio decoder gives
                    // data_size < 0, it seems they are overflows
                    if (audiosize <= 0) 
                    {
                        /* no audio frame */
                        pcktptr += chunksize;
                        length -= chunksize;
                        // post("pdp_live~ : audio overflow in decoder!");
                        continue;
                    }

                    if ( pthread_mutex_lock( &x->x_audiolock ) < 0 )
                    {
                      post( "pdp_live~ : unable to lock audio mutex" );
                      perror( "pthread_mutex_lock" );
                    }

                    // resample received audio
                    // post( "pdp_live~ : resampling from %dHz-%dch to %dHz-%dch (in position=%d)",
                    //                x->x_avcontext->streams[x->x_pkt.stream_index]->codec.sample_rate,
                    //                x->x_avcontext->streams[x->x_pkt.stream_index]->codec.channels,
                    //                (int)sys_getsr(), 2, x->x_audioin_position );

#if FFMPEG_VERSION_INT >= 0x000409
                    x->x_audiochannels = x->x_avcontext->streams[x->x_pkt.stream_index]->codec->channels;
                    x->x_samplerate = x->x_avcontext->streams[x->x_pkt.stream_index]->codec->sample_rate;
                    if (x->x_audio_resample_ctx) audio_resample_close(x->x_audio_resample_ctx);
                    x->x_audio_resample_ctx =
                          audio_resample_init(DEFAULT_CHANNELS, 
                                         x->x_avcontext->streams[x->x_pkt.stream_index]->codec->channels,
                                         (int)sys_getsr(),
                                         x->x_avcontext->streams[x->x_pkt.stream_index]->codec->sample_rate);

                    sizeout = audio_resample(x->x_audio_resample_ctx,
                                    &x->x_audio_in[x->x_audioin_position],
                                    &x->x_audio_buf[0],
                                    audiosize/(x->x_avcontext->streams[x->x_pkt.stream_index]->codec->channels * sizeof(short)));
#else
                    x->x_audiochannels = x->x_avcontext->streams[x->x_pkt.stream_index]->codec.channels;
                    x->x_samplerate = x->x_avcontext->streams[x->x_pkt.stream_index]->codec.sample_rate;
                    if (x->x_audio_resample_ctx) audio_resample_close(x->x_audio_resample_ctx);
                    x->x_audio_resample_ctx =
                          audio_resample_init(DEFAULT_CHANNELS, 
                                         x->x_avcontext->streams[x->x_pkt.stream_index]->codec.channels,
                                         (int)sys_getsr(),
                                         x->x_avcontext->streams[x->x_pkt.stream_index]->codec.sample_rate);

                    sizeout = audio_resample(x->x_audio_resample_ctx,
                                    &x->x_audio_in[x->x_audioin_position],
                                    &x->x_audio_buf[0],
                                    audiosize/(x->x_avcontext->streams[x->x_pkt.stream_index]->codec.channels * sizeof(short)));
#endif
                    sizeout = sizeout * DEFAULT_CHANNELS;

                    if ( ( x->x_audioin_position + sizeout ) < 3*MAX_AUDIO_PACKET_SIZE )
                    {
                      x->x_audioin_position = x->x_audioin_position + sizeout;
                    }
                    else
                    {
                      post( "pdp_live~ : audio overflow : packet ignored...");
                      x->x_audioin_position = 0;
                    }
                    if ( ( x->x_audioin_position > 4*x->x_blocksize ) && (!x->x_audioon) )
                    {
                       x->x_audioon = 1;
                       // post( "pdp_live~ : audio on" );
                    }
                    if ( pthread_mutex_unlock( &x->x_audiolock ) < 0 )
                    {
                      post( "pdp_live~ : unable to unlock audio mutex" );
                      perror( "pthread_mutex_unlock" );
                    }
                    break;

             case CODEC_TYPE_VIDEO:

#if FFMPEG_VERSION_INT >= 0x000409
                    imagesize = (x->x_avcontext->streams[x->x_pkt.stream_index]->codec->width * 
                                 x->x_avcontext->streams[x->x_pkt.stream_index]->codec->height * 3) / 2; // yuv planar

                    x->x_framerate = ( int ) av_q2d( x->x_avcontext->streams[x->x_pkt.stream_index]->r_frame_rate );
                    if ( x->x_framerate == 0 ) x->x_framerate = DEFAULT_FRAME_RATE;
                    x->x_videoindex = x->x_pkt.stream_index; 

                    chunksize = avcodec_decode_video(
                                   x->x_avcontext->streams[x->x_pkt.stream_index]->codec,
                                   &frame, &pictureok, 
                                   pcktptr, length);
                    if ( x->x_avcontext->streams[x->x_pkt.stream_index]->codec->pix_fmt != PIX_FMT_YUV420P )
                    {
                       post( "pdp_live~ : unsupported image format : %d", 
                              x->x_avcontext->streams[x->x_pkt.stream_index]->codec->pix_fmt ); 
                       pictureok = 0;
                    }
#else
                    imagesize = (x->x_avcontext->streams[x->x_pkt.stream_index]->codec.width * 
                                 x->x_avcontext->streams[x->x_pkt.stream_index]->codec.height * 3) / 2; // yuv planar

                    x->x_framerate = x->x_avcontext->streams[x->x_pkt.stream_index]->codec.frame_rate / 10000;
                    if ( x->x_framerate == 0 ) x->x_framerate = DEFAULT_FRAME_RATE;
                    x->x_videoindex = x->x_pkt.stream_index; 

                    chunksize = avcodec_decode_video(
                                   &x->x_avcontext->streams[x->x_pkt.stream_index]->codec,
                                   &frame, &pictureok, 
                                   pcktptr, length);
                    if ( x->x_avcontext->streams[x->x_pkt.stream_index]->codec.pix_fmt != PIX_FMT_YUV420P )
                    {
                       post( "pdp_live~ : unsupported image format : %d", 
                              x->x_avcontext->streams[x->x_pkt.stream_index]->codec.pix_fmt ); 
                       pictureok = 0;
                    }
#endif
                    x->x_picture_decoded = *(AVPicture*)&frame;
                    x->x_avcontext->streams[x->x_pkt.stream_index]->quality= frame.quality;
                    if (chunksize < 0) 
                    {
                       av_free_packet(&x->x_pkt);
                       post("pdp_live~ : could not decode video frame (ret=%d)", chunksize );
                       return 0;
                    }
                    if (!pictureok) 
                    {
                       // no picture yet 
                       pcktptr += chunksize;
                       length -= chunksize;
                       continue;
                    }
                    else
                    {  
                        if ( pthread_mutex_lock( &x->x_videolock ) < 0 )
                        {
                          post( "pdp_live~ : unable to lock video mutex" );
                          perror( "pthread_mutex_lock" );
                        }
                        x->x_newpicture=1;
#if FFMPEG_VERSION_INT >= 0x000409
                        x->x_vwidth = x->x_avcontext->streams[x->x_pkt.stream_index]->codec->width;
                        x->x_vheight = x->x_avcontext->streams[x->x_pkt.stream_index]->codec->height;
#else
                        x->x_vwidth = x->x_avcontext->streams[x->x_pkt.stream_index]->codec.width;
                        x->x_vheight = x->x_avcontext->streams[x->x_pkt.stream_index]->codec.height;
#endif
                        x->x_vsize = x->x_vwidth*x->x_vheight;

                        // create a new pdp packet from BITMAP YV12 image format
                        x->x_packet0 = pdp_packet_new_bitmap_yv12( x->x_vwidth, x->x_vheight );
                        x->x_header = pdp_packet_header(x->x_packet0);
                        x->x_data = (unsigned char *)pdp_packet_data(x->x_packet0);

                        pY = x->x_data;
                        pV = x->x_data+x->x_vsize;
                        pU = x->x_data+x->x_vsize+(x->x_vsize>>2);

                        psY = x->x_picture_decoded.data[0];
                        psU = x->x_picture_decoded.data[1];
                        psV = x->x_picture_decoded.data[2];

                        for ( py=0; py<x->x_vheight; py++)
                        {
                          memcpy( (void*)pY, (void*)psY, x->x_vwidth );
                          pY += x->x_vwidth;
                          psY += x->x_picture_decoded.linesize[0];
                          if ( py%2==0 )
                          {
                            memcpy( (void*)pU, (void*)psU, (x->x_vwidth>>1) );
                            memcpy( (void*)pV, (void*)psV, (x->x_vwidth>>1) );
                            pU += (x->x_vwidth>>1);
                            pV += (x->x_vwidth>>1);
                            psU += x->x_picture_decoded.linesize[1];
                            psV += x->x_picture_decoded.linesize[2];
                          }
                        }
         
                        if ( x->x_firstpts == -1 )
                        {
                          x->x_firstpts = x->x_pts;
                        }
                        x->x_previouspts = x->x_pts;
                        x->x_pts = frame.pts;
                        // post( "pdp_live : frame pts : %ld", x->x_pts );

                        if ( x->x_nbframes > 0 )
                        {
                          if ( gettimeofday(&ctime, NULL) == -1)
                          {
                            post("pdp_live~ : could not read time" );
                          }

                          tplaying = ( ctime.tv_sec-x->x_starttime.tv_sec )*1000 +
                                     ( ctime.tv_usec-x->x_starttime.tv_usec )/1000;
                          ttheoretical = ((x->x_nbframes)*1000 )/x->x_framerate;
                          // post( "pdp_live~ : %d playing since : %lldms ( theory : %lldms )",
                          //              x->x_nbframes, tplaying, ttheoretical );
     
                          if ( tplaying < ttheoretical )
                          {
                            mwait.tv_sec = 0; 
                            mwait.tv_nsec = (ttheoretical - tplaying)*1000000;

                            // wait between the two successive frames
                            if ( x->x_autoplay ) nanosleep( &mwait, NULL );
                          }
                        }
                        if ( pthread_mutex_unlock( &x->x_videolock ) < 0 )
                        {
                          post( "pdp_live~ : unable to unlock video mutex" );
                          perror( "pthread_mutex_unlock" );
                        }
                    }
                    break;
         }
         pcktptr += chunksize;
         length -= chunksize;
   }
   av_free_packet(&x->x_pkt);

   // post( "pdp_live~ : decoded one packet" );
   return 0;

}

static void *pdp_decode_stream_from_url(void *tdata)
{
  t_pdp_live *x = (t_pdp_live*)tdata;
  struct sched_param schedprio;
  int pmin, pmax;
  struct timespec twait;

    twait.tv_sec = 0; 
    twait.tv_nsec = 10000000; // 10 ms
 
    schedprio.sched_priority = sched_get_priority_min(SCHED_FIFO) + x->x_priority;
#ifdef __gnu_linux__
    if ( sched_setscheduler(0,SCHED_FIFO,&schedprio) == -1)
    {
        post("pdp_live~ : couldn't set priority for decoding thread.");
    }
#endif
    if ( ! (x->x_avcontext->iformat->flags & AVFMT_NOHEADER ) )
    {
       if (x->x_avcontext->iformat->read_header(x->x_avcontext, &x->x_avparameters) < 0) 
       {
          post( "pdp_live~ : couldn't read header" );
       }
       post( "pdp_live~ : read header." );
    }

    while ( x->x_decodechild )
    {
      if ( ( x->x_streaming ) && ( ( x->x_autoplay ) || ( x->x_nextimage == 1 ) ) )
      {
        x->x_decoding = 1;
        while ( x->x_newpicture ) nanosleep( &twait, NULL );
     
        // decode incoming packets
        if ( ( x->x_streaming ) && ( pdp_live_decode_packet( x ) < 0 ) )
        {
           nanosleep( &twait, NULL ); // nothing to read, just wait
        }
        x->x_nextimage = -1;
      }
      else
      {
        x->x_decoding = 0;
        nanosleep( &twait, NULL ); // nothing to do, just wait
      }
    }

    x->x_decodechild = 0;
    pthread_exit(NULL);
}

static void *pdp_live_connect_to_url(void *tdata)
{
  int i, err;
  t_pdp_live *x = (t_pdp_live*)tdata;
  pthread_attr_t decode_child_attr;

    memset(&x->x_avparameters, 0, sizeof(AVFormatParameters));
    x->x_avparameters.sample_rate = sys_getsr();
    x->x_avparameters.channels = DEFAULT_CHANNELS;
    x->x_avparameters.width = DEFAULT_WIDTH;
    if ( x->x_framerate = 0 ) x->x_framerate = DEFAULT_FRAME_RATE;
    x->x_avparameters.height = DEFAULT_HEIGHT;
    x->x_avparameters.image_format = PIX_FMT_YUV420P;

    post( "pdp_live~ : opening url : %s", x->x_url );
    err = av_open_input_file(&x->x_avcontext, x->x_url, x->x_avcontext->iformat, 0, &x->x_avparameters);
    if (err < 0)
    {
      if ( err == -1 ) post( "pdp_live~ : unknown error" );
      if ( err == -2 ) post( "pdp_live~ : i/o error" );
      if ( err == -3 ) post( "pdp_live~ : number syntax expected in filename" );
      if ( err == -4 ) post( "pdp_live~ : invalid data found" );
      if ( err == -5 ) post( "pdp_live~ : not enough memory" );
      if ( err == -6 ) post( "pdp_live~ : unknown format ( stream not found? )" );
      x->x_connectchild = 0;
      pthread_exit(NULL);
    }
    /* If not enough info to get the stream parameters, we decode the
       first frames to get it. (used in mpeg case for example) */
    err = av_find_stream_info(x->x_avcontext);
    if (err < 0) 
    {
      post( "pdp_live~ : %s: could not find codec parameters\n", x->x_url);
      x->x_connectchild = 0;
      av_close_input_file(x->x_avcontext);
      x->x_avcontext = av_mallocz(sizeof(AVFormatContext));
      pthread_exit(NULL);
    }

    // post( "pdp_live~ : stream reader : %x", x->x_avcontext->iformat );

    /* copy stream format */
    x->x_nbvideostreams = 0;
    x->x_nbaudiostreams = 0;

    for(i=0;i<x->x_avcontext->nb_streams;i++) 
    {
      AVStream *st;

#if FFMPEG_VERSION_INT >= 0x000409
        if ( x->x_avcontext->streams[i]->codec->codec_type == CODEC_TYPE_UNKNOWN )
        {
           post( "pdp_live~ : stream #%d # type : unknown", i ); 
        }
        if ( x->x_avcontext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO )
        {
           post( "pdp_live~ : stream #%d # type : audio # id : %d # bitrate : %d", 
                 i, x->x_avcontext->streams[i]->codec->codec_id, x->x_avcontext->streams[i]->codec->bit_rate ); 
           post( "pdp_live~ : sample rate : %d # channels : %d", 
                 x->x_avcontext->streams[i]->codec->sample_rate, x->x_avcontext->streams[i]->codec->channels ); 
           x->x_nbaudiostreams++;
        }
        if ( x->x_avcontext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
        {
           post( "pdp_live~ : stream #%d # type : video # id : %d # bitrate : %d", 
                 i, x->x_avcontext->streams[i]->codec->codec_id, 
                 x->x_avcontext->streams[i]->codec->bit_rate ); 
           post( "pdp_live~ : framerate : %d # width : %d # height : %d", 
                 av_q2d( x->x_avcontext->streams[i]->r_frame_rate )/10000, 
                 x->x_avcontext->streams[i]->codec->width, 
                 x->x_avcontext->streams[i]->codec->height ); 
           x->x_nbvideostreams++;
        }
#else
        if ( x->x_avcontext->streams[i]->codec.codec_type == CODEC_TYPE_UNKNOWN )
        {
           post( "pdp_live~ : stream #%d # type : unknown", i ); 
        }
        if ( x->x_avcontext->streams[i]->codec.codec_type == CODEC_TYPE_AUDIO )
        {
           post( "pdp_live~ : stream #%d # type : audio # id : %d # bitrate : %d", 
                 i, x->x_avcontext->streams[i]->codec.codec_id, x->x_avcontext->streams[i]->codec.bit_rate ); 
           post( "pdp_live~ : sample rate : %d # channels : %d", 
                 x->x_avcontext->streams[i]->codec.sample_rate, x->x_avcontext->streams[i]->codec.channels ); 
           x->x_nbaudiostreams++;
        }
        if ( x->x_avcontext->streams[i]->codec.codec_type == CODEC_TYPE_VIDEO )
        {
           post( "pdp_live~ : stream #%d # type : video # id : %d # bitrate : %d", 
                 i, x->x_avcontext->streams[i]->codec.codec_id, 
                 x->x_avcontext->streams[i]->codec.bit_rate ); 
           post( "pdp_live~ : framerate : %d # width : %d # height : %d", 
                 x->x_avcontext->streams[i]->codec.frame_rate/10000, 
                 x->x_avcontext->streams[i]->codec.width, 
                 x->x_avcontext->streams[i]->codec.height ); 
           x->x_nbvideostreams++;
        }
#endif
    }

     /* open each decoder */
    for(i=0;i<x->x_avcontext->nb_streams;i++) 
    {
      AVCodec *codec;
      post("pdp_live~ : opening decoder for stream #%d", i);
#if FFMPEG_VERSION_INT >= 0x000409
      codec = avcodec_find_decoder(x->x_avcontext->streams[i]->codec->codec_id);
#else
      codec = avcodec_find_decoder(x->x_avcontext->streams[i]->codec.codec_id);
#endif
      if (!codec) 
      {
          post("pdp_live~ : unsupported codec for output stream #%d\n", i );
          x->x_streaming = 0;
          x->x_connectchild = 0;
          av_close_input_file(x->x_avcontext);
          x->x_avcontext = av_mallocz(sizeof(AVFormatContext));
          pthread_exit(NULL);
      }
#if FFMPEG_VERSION_INT >= 0x000409
      if (avcodec_open(x->x_avcontext->streams[i]->codec, codec) < 0) 
#else
      if (avcodec_open(&x->x_avcontext->streams[i]->codec, codec) < 0) 
#endif
      {
          post("pdp_live~ : error while opening codec for stream #%d - maybe incorrect parameters such as bit_rate, rate, width or height\n", i);
          x->x_streaming = 0;
          x->x_connectchild = 0;
          av_close_input_file(x->x_avcontext);
          x->x_avcontext = av_mallocz(sizeof(AVFormatContext));
          pthread_exit(NULL);
      }
      else
      {
          post("pdp_live~ : opened decoder for stream #%d", i);
      }
    }

    if ( gettimeofday(&x->x_starttime, NULL) == -1)
    {
       post("pdp_live~ : could not set start time" );
    }
    x->x_streaming = 1;
    x->x_nopackets = 0;
    x->x_endofstream = 0;
    x->x_nbframes = 0;
    x->x_pts = -1;
    x->x_previouspts = -1;
    x->x_firstpts = -1;

    if ( x->x_usethread && ( x->x_decodechild == 0 ))
    {
      x->x_decodechild = 1; // trick & treets
      // launch decoding thread
      if ( pthread_attr_init( &decode_child_attr ) < 0 ) 
      {
         post( "pdp_live~ : could not launch decoding thread" );
         perror( "pthread_attr_init" );
         av_close_input_file(x->x_avcontext);
         x->x_avcontext = av_mallocz(sizeof(AVFormatContext));
         pthread_exit(NULL);
      }
      if ( pthread_create( &x->x_decodechild, &decode_child_attr, pdp_decode_stream_from_url, x ) < 0 ) 
      {
         post( "pdp_live~ : could not launch decoding thread" );
         perror( "pthread_create" );
         av_close_input_file(x->x_avcontext);
         x->x_avcontext = av_mallocz(sizeof(AVFormatContext));
         pthread_exit(NULL);
      }
      else
      {
         post( "pdp_live~ : decoding thread %d launched", (int)x->x_decodechild );
      }
    }
   
    pthread_exit(NULL);
}

static void pdp_live_disconnect(t_pdp_live *x)
{
 int ret, i, count=0;
 struct timespec twait;

   twait.tv_sec = 0; 
   twait.tv_nsec = 10000000; // 10 ms

   if (!x->x_streaming)
   {
     post("pdp_live~ : close request but no stream is played ... ignored" );
     return;
   }

   if ( x->x_streaming )
   {
     x->x_streaming = 0;
     x->x_newpicture = 0;
     post("pdp_live~ : waiting end of decoding..." );
     while ( x->x_decoding ) nanosleep( &twait, NULL );
     /* close each decoder */
     for(i=0;i<x->x_avcontext->nb_streams;i++) 
     {
#if FFMPEG_VERSION_INT >= 0x000409
       if (avcodec_close(x->x_avcontext->streams[i]->codec) < 0) 
#else
       if (avcodec_close(&x->x_avcontext->streams[i]->codec) < 0) 
#endif
       {
           post("pdp_live~ : error while closing codec for stream #%d", i);
       }
     }
     post("pdp_live~ : closing input file..." );
     av_close_input_file(x->x_avcontext);
     x->x_avcontext = av_mallocz(sizeof(AVFormatContext));
   }

   outlet_float( x->x_outlet_streaming, x->x_streaming );
   x->x_nbframes = 0;
   outlet_float( x->x_outlet_nbframes, x->x_nbframes );
   x->x_framerate = 0;
   outlet_float( x->x_outlet_framerate, x->x_framerate );

   if (x->x_audio_resample_ctx) 
   {
     audio_resample_close(x->x_audio_resample_ctx);
     x->x_audio_resample_ctx = NULL;
   }

}

static void pdp_live_connect(t_pdp_live *x, t_symbol *s)
{
  int ret, i;
  pthread_attr_t connect_child_attr;

   if ( ( x->x_streaming ) || ( x->x_connectchild != 0 ) )
   {
     post("pdp_live~ : connection request but a connection is pending ... disconnecting" );
     pdp_live_disconnect(x);
   }

   if ( x->x_url ) free( x->x_url );
   x->x_url = (char*) malloc( strlen( s->s_name ) + 1 );
   strcpy( x->x_url, s->s_name );

   // launch connection thread
   if ( pthread_attr_init( &connect_child_attr ) < 0 ) {
       post( "pdp_live~ : could not launch connection thread" );
       perror( "pthread_attr_init" );
       return;
   }
   if ( pthread_attr_setdetachstate( &connect_child_attr, PTHREAD_CREATE_DETACHED ) < 0 ) {
       post( "pdp_live~ : could not launch connection thread" );
       perror( "pthread_attr_setdetachstate" );
       return;
   }
   if ( pthread_create( &x->x_connectchild, &connect_child_attr, pdp_live_connect_to_url, x ) < 0 ) {
       post( "pdp_live~ : could not launch connection thread" );
       perror( "pthread_create" );
       return;
   }
   else
   {
       post( "pdp_live~ : connection thread %d launched", (int)x->x_connectchild );
   }
   
   return;
}

    /* decode the stream to fill up buffers */
static t_int *pdp_live_perform(t_int *w)
{
  t_float *out1   = (t_float *)(w[1]);       // left audio inlet
  t_float *out2   = (t_float *)(w[2]);       // right audio inlet 
  t_pdp_live *x = (t_pdp_live *)(w[3]);
  int n = (int)(w[4]);                      // number of samples 
  short sampleL, sampleR;
  struct timeval etime;
  int sn;

    // decode a packet if not in thread mode
    if ( !x->x_usethread && x->x_streaming )
    {
      pdp_live_decode_packet( x );
    }

    x->x_blocksize = n;

    // just read the buffer
    if ( x->x_audioon )
    {
      if ( pthread_mutex_lock( &x->x_audiolock ) < 0 )
      {
        post( "pdp_live~ : unable to lock audio mutex" );
        perror( "pthread_mutex_lock" );
      }
      sn=0;
      while (n--) 
      {
        sampleL=x->x_audio_in[ sn++ ];
        *(out1) = ((t_float)sampleL)/32768.0;
        if ( DEFAULT_CHANNELS == 1 )
        {
          *(out2) = *(out1);
        }
        if ( DEFAULT_CHANNELS == 2 )
        {
          sampleR=x->x_audio_in[ sn++ ];
          *(out2) = ((t_float)sampleR)/32768.0;
        }
        out1++;
        out2++;
      }
      memcpy( &x->x_audio_in[0], &x->x_audio_in[sn], 4*MAX_AUDIO_PACKET_SIZE-sn );
      x->x_audioin_position-=sn;
      // post( "pdp_live~ : audio in position : %d", x->x_audioin_position );
      if ( x->x_audioin_position <= 0 )
      {
         x->x_audioon = 0;
         // post( "pdp_live~ : audio off" );
      }
      if ( pthread_mutex_unlock( &x->x_audiolock ) < 0 )
      {
        post( "pdp_live~ : unable to audio unlock mutex" );
        perror( "pthread_mutex_unlock" );
      }
    }
    else
    {
      // post("pdp_live~ : no available audio" );
      while (n--)
      {
        *(out1++) = 0.0;
        *(out2++) = 0.0;
      }
    }	

    // check if the framerate has been exceeded
    if ( gettimeofday(&etime, NULL) == -1)
    {
       post("pdp_live~ : could not read time" );
    }
    if ( etime.tv_sec != x->x_cursec )
    {
       x->x_cursec = etime.tv_sec;
       if (x->x_streaming) outlet_float( x->x_outlet_framerate, x->x_secondcount );
       x->x_secondcount = 0;
    }
    if ( x->x_secondcount >= x->x_framerate )
    {
       // return (w+5);
    }

    // output image if there's a new one decoded
    if ( x->x_newpicture )
    {
       if ( pthread_mutex_lock( &x->x_videolock ) < 0 )
       {
         post( "pdp_live~ : unable to lock video mutex" );
         perror( "pthread_mutex_lock" );
       }
       pdp_packet_pass_if_valid(x->x_pdp_out, &x->x_packet0);
       x->x_newpicture = 0;

       // update streaming status
       x->x_nbframes++;
       x->x_secondcount++;
       outlet_float( x->x_outlet_nbframes, x->x_nbframes );
       if ( pthread_mutex_unlock( &x->x_videolock ) < 0 )
       {
         post( "pdp_live~ : unable to unlock video mutex" );
         perror( "pthread_mutex_unlock" );
       }
    }
    outlet_float( x->x_outlet_streaming, x->x_streaming );
    outlet_float( x->x_outlet_endofstream, x->x_endofstream );

    return (w+5);
}

static void pdp_live_dsp(t_pdp_live *x, t_signal **sp)
{
    dsp_add(pdp_live_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void pdp_live_free(t_pdp_live *x)
{
  int i;

    if ( x->x_decodechild )
    {
      x->x_decodechild = 0;
      x->x_decoding = 0;
      sleep( 5 ); // let the decoding thread exiting
    }

    if ( x->x_streaming )
    {
       pdp_live_disconnect(x);
    }
    post( "pdp_live~ : freeing object" );
    pdp_packet_mark_unused(x->x_packet0);
    av_free_static();
    
    if ( pthread_mutex_destroy( &x->x_audiolock ) < 0 )
    {
      post( "pdp_live~ : unable to destroy audio mutex" );
      perror( "pthread_mutex_destroy" );
    }
    if ( pthread_mutex_destroy( &x->x_videolock ) < 0 )
    {
      post( "pdp_live~ : unable to destroy video mutex" );
      perror( "pthread_mutex_destroy" );
    }
}

t_class *pdp_live_class;

void *pdp_live_new(void)
{
    int i;

    t_pdp_live *x = (t_pdp_live *)pd_new(pdp_live_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("frame_cold"));

    x->x_pdp_out = outlet_new(&x->x_obj, &s_anything);

    x->x_outlet_left = outlet_new(&x->x_obj, &s_signal);
    x->x_outlet_right = outlet_new(&x->x_obj, &s_signal);

    x->x_outlet_streaming = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_nbframes = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_framerate = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_endofstream = outlet_new(&x->x_obj, &s_float);

    x->x_packet0 = -1;
    x->x_connectchild = 0;
    x->x_decodechild = 0;
    x->x_decoding = 0;
    x->x_usethread = 1;
    x->x_priority = DEFAULT_PRIORITY;
    x->x_framerate = DEFAULT_FRAME_RATE;
    x->x_nbframes = 0;
    x->x_framerate = 0;
    x->x_samplerate = 0;
    x->x_audiochannels = 0;
    x->x_cursec = 0;
    x->x_secondcount = 0;
    x->x_audio_resample_ctx = NULL;
    x->x_nbvideostreams = 0;
    x->x_videoindex = -1;
    x->x_audioin_position = 0;
    x->x_newpicture = 0;
    x->x_endofstream = 0;
    x->x_nopackets = 0;
    x->x_blocksize = MIN_AUDIO_SIZE;
    x->x_autoplay = 1;
    x->x_loop = 1;
    x->x_nextimage = 0;

    x->x_pts = -1;
    x->x_previouspts = -1;
    x->x_firstpts = -1;

    if ( pthread_mutex_init( &x->x_audiolock, NULL ) < 0 )
    {
       post( "pdp_live~ : unable to initialize audio mutex" );
       perror( "pthread_mutex_init" );
       return NULL;
    }
    if ( pthread_mutex_init( &x->x_videolock, NULL ) < 0 )
    {
       post( "pdp_live~ : unable to initialize video mutex" );
       perror( "pthread_mutex_init" );
       return NULL;
    }

    x->x_avcontext = av_mallocz(sizeof(AVFormatContext));
    if ( !x->x_avcontext )
    {
       post( "pdp_live~ : severe error : could not allocate video structures." );
       return NULL;
    }

    // activate codecs
    av_register_all();

    memset( &x->x_audio_buf[0], 0x0, 4*MAX_AUDIO_PACKET_SIZE*sizeof(short) );
    memset( &x->x_audio_in[0], 0x0, 4*MAX_AUDIO_PACKET_SIZE*sizeof(short) );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_live_tilde_setup(void)
{
    // post( pdp_live_version );
    pdp_live_class = class_new(gensym("pdp_live~"), (t_newmethod)pdp_live_new,
    	(t_method)pdp_live_free, sizeof(t_pdp_live), 0, A_NULL);

    class_addmethod(pdp_live_class, (t_method)pdp_live_dsp, gensym("dsp"), A_NULL);
    class_addmethod(pdp_live_class, (t_method)pdp_live_connect, gensym("connect"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_live_class, (t_method)pdp_live_disconnect, gensym("disconnect"), A_NULL);
    class_addmethod(pdp_live_class, (t_method)pdp_live_priority, gensym("priority"), A_FLOAT, A_NULL);
    class_addmethod(pdp_live_class, (t_method)pdp_live_audio, gensym("audio"), A_FLOAT, A_NULL);
    class_addmethod(pdp_live_class, (t_method)pdp_live_autoplay, gensym("autoplay"), A_FLOAT, A_NULL);
    class_addmethod(pdp_live_class, (t_method)pdp_live_loop, gensym("loop"), A_FLOAT, A_NULL);
    class_addmethod(pdp_live_class, (t_method)pdp_live_threadify, gensym("thread"), A_FLOAT, A_NULL);
    class_addmethod(pdp_live_class, (t_method)pdp_live_bang, gensym("bang"), A_NULL);
    class_addmethod(pdp_live_class, (t_method)pdp_live_frame_cold, gensym("frame_cold"), A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
