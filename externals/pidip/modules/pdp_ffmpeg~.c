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

/*  This object is a streaming object towards a ffmeg server
 *  A lot of this object code is inspired by the excellent ffmpeg.c
 *  Copyright (c) 2000, 2001, 2002 Fabrice Bellard
 *  The rest is written by Yves Degoyon ( ydegoyon@free.fr )                             
 */


#include "pdp.h"
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <avformat.h>

#define VIDEO_BUFFER_SIZE (1024*1024)
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)
#define AUDIO_PACKET_SIZE (2*1152)

static char   *pdp_ffmpeg_version = "pdp_ffmpeg~: version 0.1, a video streaming object (towards ffserver)";

typedef struct pdp_ffmpeg_struct
{
    t_object x_obj;
    t_float x_f;

    int x_packet0;
    int x_dropped;
    int x_queue_id;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

    t_outlet *x_outlet_streaming;  // indicates the action of streaming
    t_outlet *x_outlet_nbframes;   // number of frames emitted
    t_outlet *x_outlet_framerate;  // real framerate
    t_outlet *x_outlet_nbframes_dropped; // number of frames dropped

    char  *x_feedname;
    int x_streaming;   // streaming flag
    int x_nbframes;    // number of frames emitted
    int x_nbframes_dropped; // number of frames dropped
    int x_nbvideostreams; // number of video streams
    int x_nbaudiostreams; // number of audio streams
    int x_cursec;   // current second
    int *x_secondcount; // number of frames emitted in the current second ( per video stream )

      /* AV data structures */
    AVFormatContext  *x_avcontext;
    AVFormatParameters x_avparameters; // unused but the call is necessary to allocate structures
    AVPicture *x_final_picture;
    AVPicture *x_formatted_picture;
    AVPicture x_picture_format;
    AVPicture x_picture_final;
    ImgReSampleContext *x_img_resample_ctx;
    uint8_t *x_video_buffer;
    uint8_t *x_rdata;
    uint8_t *x_buf1;
    uint8_t *x_buf2;

      /* audio structures */
    short x_audio_buf[2*MAX_AUDIO_PACKET_SIZE]; /* buffer for incoming audio */
    short x_audio_enc_buf[2*MAX_AUDIO_PACKET_SIZE]; /* buffer for audio to be encoded */
    uint8_t x_audio_out[4*MAX_AUDIO_PACKET_SIZE]; /* buffer for encoded audio */
    int x_audioin_position; // writing position for incoming audio
    int x_audio_per_frame;  // number of audio samples to transmit for each frame
    ReSampleContext *x_audio_resample_ctx; // structures for audio resample
    FifoBuffer      *x_audio_fifo;  // audio fifos ( one per codec )

} t_pdp_ffmpeg;

static int pdp_ffmpeg_read_ffserver_streams(t_pdp_ffmpeg *x)
{
    int i, err;
    AVFormatContext *ic;
    
    err = av_open_input_file(&ic, x->x_feedname, NULL, FFM_PACKET_SIZE, NULL);
    if (err < 0)
    {
       return err;
    }

    /* copy stream format */
    x->x_avcontext->nb_streams = ic->nb_streams;
    x->x_nbvideostreams = 0;
    x->x_nbaudiostreams = 0;

    for(i=0;i<ic->nb_streams;i++) 
    {
      AVStream *st;

        st = av_mallocz(sizeof(AVFormatContext));
        memcpy(st, ic->streams[i], sizeof(AVStream));
        x->x_avcontext->streams[i] = st;
 
#if FFMPEG_VERSION_INT >= 0x000409
        if ( ic->streams[i]->codec->codec_type == CODEC_TYPE_UNKNOWN )
        {
           post( "pdp_ffmpeg~ : stream #%d # type : unknown", i ); 
        }
        if ( ic->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO )
        {
           post( "pdp_ffmpeg~ : stream #%d # type : audio # id : %d # bitrate : %d", 
                 i, ic->streams[i]->codec->codec_id, ic->streams[i]->codec->bit_rate ); 
           post( "pdp_ffmpeg~ : sample rate : %d # channels : %d", 
                 ic->streams[i]->codec->sample_rate, ic->streams[i]->codec->channels ); 
           x->x_nbaudiostreams++;
        }
        if ( ic->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
        {
           post( "pdp_ffmpeg~ : stream #%d # type : video # id : %d # bitrate : %d", 
                 i, ic->streams[i]->codec->codec_id, ic->streams[i]->codec->bit_rate ); 
           post( "pdp_ffmpeg~ : framerate : %d # width : %d # height : %d", 
                 av_q2d( ic->streams[i]->r_frame_rate ), ic->streams[i]->codec->width, ic->streams[i]->codec->height ); 
           x->x_nbvideostreams++;
        }
#else
        if ( ic->streams[i]->codec.codec_type == CODEC_TYPE_UNKNOWN )
        {
           post( "pdp_ffmpeg~ : stream #%d # type : unknown", i ); 
        }
        if ( ic->streams[i]->codec.codec_type == CODEC_TYPE_AUDIO )
        {
           post( "pdp_ffmpeg~ : stream #%d # type : audio # id : %d # bitrate : %d", 
                 i, ic->streams[i]->codec.codec_id, ic->streams[i]->codec.bit_rate ); 
           post( "pdp_ffmpeg~ : sample rate : %d # channels : %d", 
                 ic->streams[i]->codec.sample_rate, ic->streams[i]->codec.channels ); 
           x->x_nbaudiostreams++;
        }
        if ( ic->streams[i]->codec.codec_type == CODEC_TYPE_VIDEO )
        {
           post( "pdp_ffmpeg~ : stream #%d # type : video # id : %d # bitrate : %d", 
                 i, ic->streams[i]->codec.codec_id, ic->streams[i]->codec.bit_rate ); 
           post( "pdp_ffmpeg~ : framerate : %d # width : %d # height : %d", 
                 ic->streams[i]->codec.frame_rate/10000, ic->streams[i]->codec.width, ic->streams[i]->codec.height ); 
           x->x_nbvideostreams++;
        }
#endif
    }

    if ( x->x_secondcount ) free( x->x_secondcount );
    x->x_secondcount = (int*) malloc( x->x_nbvideostreams*sizeof(int) );
    memset( x->x_secondcount, 0x00, x->x_nbvideostreams*sizeof(int) );
    x->x_audio_fifo = (FifoBuffer*) malloc( x->x_nbaudiostreams*sizeof(FifoBuffer) );
    for ( i=0; i<x->x_nbaudiostreams; i++)
    {
       fifo_init(&x->x_audio_fifo[i], 2 * MAX_AUDIO_PACKET_SIZE);
    }

    av_close_input_file(ic);

    return 0;
}

static void pdp_ffmpeg_starve(t_pdp_ffmpeg *x)
{
 int ret, i;

   if (!x->x_streaming)
   {
     post("pdp_ffmpeg~ : close request but no feed is opened ... ignored" );
     return;
   }

   x->x_streaming = 0;
   outlet_float( x->x_outlet_streaming, x->x_streaming );
   x->x_nbframes = 0;
   outlet_float( x->x_outlet_nbframes, x->x_nbframes );
   x->x_nbframes_dropped = 0;
   outlet_float( x->x_outlet_nbframes_dropped, x->x_nbframes_dropped );
   if ( x->x_buf1 ) free( x->x_buf1 );
   x->x_buf1 = NULL;
   if ( x->x_buf2 ) free( x->x_buf2 );
   x->x_buf2 = NULL;

   if (x->x_img_resample_ctx) 
   {
     img_resample_close(x->x_img_resample_ctx);
     x->x_img_resample_ctx = NULL;
   }

   if (x->x_audio_resample_ctx) 
   {
     audio_resample_close(x->x_audio_resample_ctx);
     x->x_audio_resample_ctx = NULL;
   }

   if ( ( ret = url_fclose(&x->x_avcontext->pb) ) < 0 )
   {
     post("pdp_ffmpeg~ : could not close stream (ret=%d)", ret );
   }
   
   for(i=0;i<x->x_avcontext->nb_streams;i++)
   {
#if FFMPEG_VERSION_INT >= 0x000409
     avcodec_close( x->x_avcontext->streams[i]->codec );
#else
     avcodec_close( &x->x_avcontext->streams[i]->codec );
#endif
     av_free( x->x_avcontext->streams[i] );
   }

}

static void pdp_ffmpeg_feed(t_pdp_ffmpeg *x, t_symbol *s)
{
  int ret, i;

   if (x->x_streaming)
   {
     post("pdp_ffmpeg~ : feed request but a feed is running ... ignored" );
     return;
   }

   if ( !strstr( s->s_name, "http:" ) || !strstr( s->s_name, ".ffm" ) )
   {
      post( "pdp_ffmpeg~ : wrong feed format <%s>: should be like http://localhost:8090/test.ffm", s->s_name );
   }
   if ( x->x_feedname ) free( x->x_feedname );
   x->x_feedname = (char*) malloc( strlen( s->s_name ) + 1 );
   strcpy( x->x_feedname, s->s_name );

     /* set output format */
   x->x_avcontext->oformat = guess_format(NULL, s->s_name, NULL);

   if ( ( ret = pdp_ffmpeg_read_ffserver_streams(x) ) < 0 )
   {
      post( "pdp_ffmpeg~ : error encountered while reading feed informations :", ret );
      if ( ret == -1 ) post( "pdp_ffmpeg~ : unknown error" );
      if ( ret == -2 ) post( "pdp_ffmpeg~ : i/o error" );
      if ( ret == -3 ) post( "pdp_ffmpeg~ : number syntax expected in filename" );
      if ( ret == -4 ) post( "pdp_ffmpeg~ : invalid data found" );
      if ( ret == -5 ) post( "pdp_ffmpeg~ : not enough memory" );
      if ( ret == -6 ) post( "pdp_ffmpeg~ : unknown format" );

      x->x_streaming = 0;
      outlet_float( x->x_outlet_streaming, x->x_streaming );
      return;
   } 

     /* open the stream now */
   if (url_fopen(&x->x_avcontext->pb, s->s_name, URL_WRONLY) < 0) 
   {
      post("pdp_ffmpeg~ : could not open stream '%s'", s->s_name);
      x->x_streaming = 0;
      outlet_float( x->x_outlet_streaming, x->x_streaming );
      return;
   }
   else
   {
      post("pdp_ffmpeg~ : opened stream '%s'", s->s_name);
   }

     /* open each encoder */
   for(i=0;i<x->x_avcontext->nb_streams;i++) 
   {
      AVCodec *codec;
#if FFMPEG_VERSION_INT >= 0x000409
      codec = avcodec_find_encoder(x->x_avcontext->streams[i]->codec->codec_id);
#else
      codec = avcodec_find_encoder(x->x_avcontext->streams[i]->codec.codec_id);
#endif
      if (!codec) 
      {
          post("pdp_ffmpeg~ : unsupported codec for output stream #%d\n", i );
          x->x_streaming = 0;
          outlet_float( x->x_outlet_streaming, x->x_streaming );
          return;
      }
#if FFMPEG_VERSION_INT >= 0x000409
      if (avcodec_open(x->x_avcontext->streams[i]->codec, codec) < 0) 
#else
      if (avcodec_open(&x->x_avcontext->streams[i]->codec, codec) < 0) 
#endif
      {
          post("pdp_ffmpeg~ : error while opening codec for stream #%d - maybe incorrect parameters such as bit_rate, rate, width or height\n", i);
          x->x_streaming = 0;
          outlet_float( x->x_outlet_streaming, x->x_streaming );
          return;
      }
      else
      {
          post("pdp_ffmpeg~ : opened encoder for stream #%d", i);
      }
    }

      /* set parameters */
    if (av_set_parameters(x->x_avcontext, &x->x_avparameters) < 0) 
    {
       post("pdp_ffmpeg~ : severe error : could not set encoding parameters" );
       x->x_streaming = 0;
       outlet_float( x->x_outlet_streaming, x->x_streaming );
       return;
    }
    
      /* write header */
    if (av_write_header(x->x_avcontext) < 0) 
    {
       post("pdp_ffmpeg~ : could not write header (incorrect codec parameters ?)\n", i);
       x->x_streaming = 0;
       outlet_float( x->x_outlet_streaming, x->x_streaming );
       return;
    }

    x->x_streaming = 1;
    outlet_float( x->x_outlet_streaming, x->x_streaming );
    x->x_nbframes = 0;
    outlet_float( x->x_outlet_nbframes, x->x_nbframes );
    x->x_nbframes_dropped = 0;
    outlet_float( x->x_outlet_nbframes_dropped, x->x_nbframes_dropped );

}

static void pdp_ffmpeg_allocate(t_pdp_ffmpeg *x)
{
    x->x_rdata = (uint8_t*) getbytes( (x->x_vsize+(x->x_vsize>>1))*sizeof(uint8_t) );
}

static void pdp_ffmpeg_free_ressources(t_pdp_ffmpeg *x)
{
    if (x->x_rdata) freebytes( x->x_rdata, (x->x_vsize+(x->x_vsize>>1))*sizeof(uint8_t) );
}

static void pdp_ffmpeg_process_yv12(t_pdp_ffmpeg *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = 0;
    short int *newdata = 0;
    int     newpacket = -1, i, j;
    short int *pY, *pU, *pV;
    uint8_t     *pnY, *pnU, *pnV;
    int     px, py;
    int     svideoindex;
    int     saudioindex;
    struct timeval etime;
    int   sizeout, size, encsize;
    int   framebytes;
    int   owidth, oheight;
    short   *pencbuf;
    int   framerate, atime, ttime;

        /* allocate all ressources */
    if ( ((int)header->info.image.width != x->x_vwidth) ||
         ((int)header->info.image.height != x->x_vheight) )
    {
        pdp_ffmpeg_free_ressources(x); 
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_ffmpeg_allocate(x); 
    }

    if ( x->x_streaming )
    {
       AVPicture pdppict;
         
         pY = data;
         pV = data+x->x_vsize;
         pU = data+x->x_vsize+(x->x_vsize>>2);
         pnY = x->x_rdata;
         pnU = x->x_rdata+x->x_vsize;
         pnV = x->x_rdata+x->x_vsize+(x->x_vsize>>2);
         for(py=0; py<x->x_vheight; py++)
         {
            for(px=0; px<x->x_vwidth; px++)
            {
               *pnY = (uint8_t) (*(pY++)>>7); 
               pnY++;
               if ( (px%2==0) && (py%2==0) )
               {
                   *pnV = (uint8_t) (((*(pV++))>>8)+128); 
                   *pnU = (uint8_t) (((*(pU++))>>8)+128); 
                   pnU++;pnV++;
               }
            }
         }
	
         if ( avpicture_fill(&pdppict, x->x_rdata,
                        PIX_FMT_YUV420P,
                        x->x_vwidth,
                        x->x_vheight) < 0 )
         {
            post( "pdp_ffmpeg~ : could not build av picture" );
         }

         // output all video streams
         svideoindex=0;
         saudioindex=0;
#if FFMPEG_VERSION_INT >= 0x000409
        for (i=0; i<x->x_avcontext->nb_streams; i++)
        {
             /* convert pixel format and size if needed */
           if ( x->x_avcontext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
           {
             int size;

             // check if the framerate has been exceeded
             if ( gettimeofday(&etime, NULL) == -1)
             {
                post("pdp_ffmpeg~ : could not read time" );
             }
             if ( etime.tv_sec != x->x_cursec )
             {
                x->x_cursec = etime.tv_sec;
                outlet_float( x->x_outlet_framerate, x->x_secondcount[ svideoindex ] );
                for (j=0; j<x->x_nbvideostreams; j++)
                {
                   x->x_secondcount[ j ] = 0;
                }
             }
             framerate = av_q2d( x->x_avcontext->streams[i]->r_frame_rate );
             ttime = ( ( x->x_nbframes + 1 ) % framerate ) * ( 1000 / framerate );
             atime = ( etime.tv_usec / 1000 );
             // post("pdp_theonice~ : actual : %d, theoretical : %d", atime, ttime );
             if ( atime < ttime )
             {
                x->x_nbframes_dropped++;
                continue;
             }

             if ( x->x_avcontext->streams[i]->codec->pix_fmt != PIX_FMT_YUV420P )
             {
               /* create temporary picture */
               size = avpicture_get_size(x->x_avcontext->streams[i]->codec->pix_fmt, 
                                         x->x_vwidth, x->x_vheight );  
               if (!x->x_buf1) x->x_buf1 = (uint8_t*) malloc(size);
               if (!x->x_buf1)
               {
                 post ("pdp_ffmpeg~ : severe error : could not allocate image buffer" );
                 return;
               }
               x->x_formatted_picture = &x->x_picture_format;
               avpicture_fill(x->x_formatted_picture, x->x_buf1, 
                              x->x_avcontext->streams[i]->codec->pix_fmt, 
                              x->x_vwidth, x->x_vheight );  
  
               if (img_convert(x->x_formatted_picture, 
                          x->x_avcontext->streams[i]->codec->pix_fmt,
                          &pdppict, PIX_FMT_YUV420P,
                          x->x_vwidth, x->x_vheight ) < 0) 
               {
                 post ("pdp_ffmpeg~ : error : image conversion failed" );
               }
             }
             else
             {
                 x->x_formatted_picture = &pdppict;
             }

             // post ( "pdp_ffmpeg~ : resampling [%d,%d] -> [%d,%d]",
             //        x->x_vwidth, x->x_vheight,
             //        x->x_avcontext->streams[i]->codec->width,
             //        x->x_avcontext->streams[i]->codec->height );
             if ( ( x->x_avcontext->streams[i]->codec->width < x->x_vwidth ) &&
                  ( x->x_avcontext->streams[i]->codec->height < x->x_vheight ) )
             {
               owidth = x->x_avcontext->streams[i]->codec->width;
               oheight = x->x_avcontext->streams[i]->codec->height;

               if (x->x_img_resample_ctx) img_resample_close(x->x_img_resample_ctx);
		
#if LIBAVCODEC_BUILD > 4715	
               x->x_img_resample_ctx = img_resample_full_init(
                              owidth, oheight, 
                              x->x_vwidth, x->x_vheight, 
                              0, 0, 0, 0,
                              0, 0, 0, 0);
#else
               x->x_img_resample_ctx = img_resample_full_init(
                              owidth, oheight, 
                              x->x_vwidth, x->x_vheight, 0, 0, 0, 0);
#endif
                 
               size = avpicture_get_size(x->x_avcontext->streams[i]->codec->pix_fmt, 
                                         owidth, oheight );
               if ( !x->x_buf2 ) 
               {
                  x->x_buf2 = (uint8_t*) malloc(size);
               }
               if (!x->x_buf2)
               {
                 post ("pdp_ffmpeg~ : severe error : could not allocate image buffer" );
                 return;
               }
               x->x_final_picture = &x->x_picture_final;
               avpicture_fill(x->x_final_picture, x->x_buf2, 
                              x->x_avcontext->streams[i]->codec->pix_fmt, 
                              owidth, oheight );
  
               img_resample(x->x_img_resample_ctx, x->x_final_picture, x->x_formatted_picture);
    
             }
             else
             {
               x->x_final_picture = x->x_formatted_picture;
             }

             // encode and send the picture
             {
               AVFrame aframe;
#if LIBAVCODEC_BUILD > 4715	
               AVPacket vpkt;
#endif
               int fsize, ret;
                
                 memset(&aframe, 0, sizeof(AVFrame));
                 *(AVPicture*)&aframe= *x->x_final_picture;
                 aframe.pts = etime.tv_sec*1000000 + etime.tv_usec;
                 aframe.quality = x->x_avcontext->streams[i]->quality;
  
                 fsize = avcodec_encode_video(x->x_avcontext->streams[i]->codec,
                             x->x_video_buffer, VIDEO_BUFFER_SIZE,
                             &aframe);

#if LIBAVCODEC_BUILD > 4715	
                 av_init_packet(&vpkt);

                 vpkt.pts = aframe.pts;
                 if(x->x_avcontext->streams[i]->codec->coded_frame->key_frame) 
                        vpkt.flags |= PKT_FLAG_KEY;
                 vpkt.stream_index= i;
                 vpkt.data= (uint8_t *)x->x_video_buffer;
                 vpkt.size= fsize;

                 if ( ( ret = av_write_frame( x->x_avcontext, &vpkt) ) < 0 )
#else
                 if ( ( ret = av_write_frame( x->x_avcontext, i, x->x_video_buffer, fsize) ) < 0 )
#endif
                 {
                    post ("pdp_ffmpeg~ : error : could not send frame : (ret=%d)", ret );
                    return;
                 }
                 else
                 {
                    x->x_nbframes++;
                    x->x_secondcount[ svideoindex ]++;
                    // post ("pdp_ffmpeg~ : index=%d count=%d", svideoindex, x->x_secondcount[ svideoindex ] );
                    svideoindex++;
                 }
             }
           } 

              /* convert and stream audio data */
           if ( x->x_avcontext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO )
           {
                    // we assume audio is synchronized on next video stream 
                 if ( ( (i+1) < x->x_avcontext->nb_streams ) &&
                      ( x->x_avcontext->streams[i+1]->codec->codec_type == CODEC_TYPE_VIDEO ) )
                 {
                     x->x_audio_per_frame = 
                        // 2*( (int) sys_getsr() ) / av_q2d( x->x_avcontext->streams[i+1]->r_frame_rate ) ;
                        AUDIO_PACKET_SIZE;
                     // post ("pdp_ffmpeg~ : transmit %d samples", x->x_audio_per_frame );
                 }
                 else
                 {
                     post ("pdp_ffmpeg~ : can't stream audio : video stream is not found" );
                     continue;
                 }
       
                 if ( x->x_audioin_position > x->x_audio_per_frame )
                 {
                    size = x->x_audioin_position;
                    if ( ( x->x_avcontext->streams[i]->codec->sample_rate != (int)sys_getsr() ) ||
                         ( x->x_avcontext->streams[i]->codec->channels != 2 ) )
                    {
                      if (x->x_audio_resample_ctx) audio_resample_close(x->x_audio_resample_ctx);
                      x->x_audio_resample_ctx = 
                       audio_resample_init(x->x_avcontext->streams[i]->codec->channels, 2,
                                         x->x_avcontext->streams[i]->codec->sample_rate,
                                         (int)sys_getsr());
                      sizeout = audio_resample(x->x_audio_resample_ctx,
                                    x->x_audio_enc_buf, 
                                    x->x_audio_buf,
                                    size / (x->x_avcontext->streams[i]->codec->channels * 2));
                      pencbuf = (short*) &x->x_audio_enc_buf;
                      sizeout = sizeout * x->x_avcontext->streams[i]->codec->channels * 2;
                    }
                    else
                    {
                      pencbuf = (short*) &x->x_audio_buf;
                      sizeout = size;
                    }

                      /* output resampled raw samples */
                    fifo_write(&x->x_audio_fifo[saudioindex], (uint8_t*)pencbuf, sizeout,
                               &x->x_audio_fifo[saudioindex].wptr);

                    framebytes = x->x_avcontext->streams[i]->codec->frame_size * 2 * 
                                 x->x_avcontext->streams[i]->codec->channels;
 
                    while (fifo_read(&x->x_audio_fifo[saudioindex], (uint8_t*)pencbuf, framebytes,
                                     &x->x_audio_fifo[saudioindex].rptr) == 0) 
                    {
#if LIBAVCODEC_BUILD > 4715	
                      AVPacket apkt;
#endif
                         encsize = avcodec_encode_audio(x->x_avcontext->streams[i]->codec, 
                                       (uint8_t*)&x->x_audio_out, sizeof(x->x_audio_out),
                                       (short *)pencbuf);
#if LIBAVCODEC_BUILD > 4715	
                         av_init_packet(&apkt);

                         apkt.pts = etime.tv_sec*1000000 + etime.tv_usec;
                         if(x->x_avcontext->streams[i]->codec->coded_frame->key_frame) 
                                  apkt.flags |= PKT_FLAG_KEY;
                         apkt.stream_index= i;
                         apkt.data= (uint8_t *)x->x_audio_out;
                         apkt.size= encsize;
                         
                         av_write_frame(x->x_avcontext, &apkt);
#else
                         av_write_frame(x->x_avcontext, i, x->x_audio_out, encsize);
#endif
                    }
                    saudioindex++;
                 }
           }
        }
#else
        for (i=0; i<x->x_avcontext->nb_streams; i++)
        {
             /* convert pixel format and size if needed */
           if ( x->x_avcontext->streams[i]->codec.codec_type == CODEC_TYPE_VIDEO )
           {
             int size;

             // check if the framerate has been exceeded
             if ( gettimeofday(&etime, NULL) == -1)
             {
                post("pdp_ffmpeg~ : could not read time" );
             }
             if ( etime.tv_sec != x->x_cursec )
             {
                x->x_cursec = etime.tv_sec;
                outlet_float( x->x_outlet_framerate, x->x_secondcount[ svideoindex ] );
                for (j=0; j<x->x_nbvideostreams; j++)
                {
                   x->x_secondcount[ j ] = 0;
                }
             }
             framerate = x->x_avcontext->streams[i]->codec.frame_rate/10000;
             ttime = ( ( x->x_nbframes + 1 ) % framerate ) * ( 1000 / framerate );
             atime = ( etime.tv_usec / 1000 );
             // post("pdp_theonice~ : actual : %d, theoretical : %d", atime, ttime );
             if ( atime < ttime )
             {
                x->x_nbframes_dropped++;
                continue;
             }

             if ( x->x_avcontext->streams[i]->codec.pix_fmt != PIX_FMT_YUV420P )
             {
               /* create temporary picture */
               size = avpicture_get_size(x->x_avcontext->streams[i]->codec.pix_fmt, 
                                         x->x_vwidth, x->x_vheight );  
               if (!x->x_buf1) x->x_buf1 = (uint8_t*) malloc(size);
               if (!x->x_buf1)
               {
                 post ("pdp_ffmpeg~ : severe error : could not allocate image buffer" );
                 return;
               }
               x->x_formatted_picture = &x->x_picture_format;
               avpicture_fill(x->x_formatted_picture, x->x_buf1, 
                              x->x_avcontext->streams[i]->codec.pix_fmt, 
                              x->x_vwidth, x->x_vheight );  
  
               if (img_convert(x->x_formatted_picture, 
                          x->x_avcontext->streams[i]->codec.pix_fmt,
                          &pdppict, PIX_FMT_YUV420P,
                          x->x_vwidth, x->x_vheight ) < 0) 
               {
                 post ("pdp_ffmpeg~ : error : image conversion failed" );
               }
             }
             else
             {
                 x->x_formatted_picture = &pdppict;
             }

             // post ( "pdp_ffmpeg~ : resampling [%d,%d] -> [%d,%d]",
             //        x->x_vwidth, x->x_vheight,
             //        x->x_avcontext->streams[i]->codec.width,
             //        x->x_avcontext->streams[i]->codec.height );
             if ( ( x->x_avcontext->streams[i]->codec.width < x->x_vwidth ) &&
                  ( x->x_avcontext->streams[i]->codec.height < x->x_vheight ) )
             {
               owidth = x->x_avcontext->streams[i]->codec.width;
               oheight = x->x_avcontext->streams[i]->codec.height;

               if (x->x_img_resample_ctx) img_resample_close(x->x_img_resample_ctx);
		
#if LIBAVCODEC_BUILD > 4715	
               x->x_img_resample_ctx = img_resample_full_init(
                              owidth, oheight, 
                              x->x_vwidth, x->x_vheight, 
                              0, 0, 0, 0,
                              0, 0, 0, 0);
#else
               x->x_img_resample_ctx = img_resample_full_init(
                              owidth, oheight, 
                              x->x_vwidth, x->x_vheight, 0, 0, 0, 0);
#endif
                 
               size = avpicture_get_size(x->x_avcontext->streams[i]->codec.pix_fmt, 
                                         owidth, oheight );
               if ( !x->x_buf2 ) 
               {
                  x->x_buf2 = (uint8_t*) malloc(size);
               }
               if (!x->x_buf2)
               {
                 post ("pdp_ffmpeg~ : severe error : could not allocate image buffer" );
                 return;
               }
               x->x_final_picture = &x->x_picture_final;
               avpicture_fill(x->x_final_picture, x->x_buf2, 
                              x->x_avcontext->streams[i]->codec.pix_fmt, 
                              owidth, oheight );
  
               img_resample(x->x_img_resample_ctx, x->x_final_picture, x->x_formatted_picture);
    
             }
             else
             {
               x->x_final_picture = x->x_formatted_picture;
             }

             // encode and send the picture
             {
               AVFrame aframe;
#if LIBAVCODEC_BUILD > 4715	
               AVPacket vpkt;
#endif
               int fsize, ret;
                
                 memset(&aframe, 0, sizeof(AVFrame));
                 *(AVPicture*)&aframe= *x->x_final_picture;
                 aframe.pts = etime.tv_sec*1000000 + etime.tv_usec;
                 aframe.quality = x->x_avcontext->streams[i]->quality;
  
                 fsize = avcodec_encode_video(&x->x_avcontext->streams[i]->codec,
                             x->x_video_buffer, VIDEO_BUFFER_SIZE,
                             &aframe);

#if LIBAVCODEC_BUILD > 4715	
                 av_init_packet(&vpkt);

                 vpkt.pts = aframe.pts;
                 if((&x->x_avcontext->streams[i]->codec)->coded_frame->key_frame) 
                        vpkt.flags |= PKT_FLAG_KEY;
                 vpkt.stream_index= i;
                 vpkt.data= (uint8_t *)x->x_video_buffer;
                 vpkt.size= fsize;

                 if ( ( ret = av_write_frame( x->x_avcontext, &vpkt) ) < 0 )
#else
                 if ( ( ret = av_write_frame( x->x_avcontext, i, x->x_video_buffer, fsize) ) < 0 )
#endif
                 {
                    post ("pdp_ffmpeg~ : error : could not send frame : (ret=%d)", ret );
                    return;
                 }
                 else
                 {
                    x->x_nbframes++;
                    x->x_secondcount[ svideoindex ]++;
                    // post ("pdp_ffmpeg~ : index=%d count=%d", svideoindex, x->x_secondcount[ svideoindex ] );
                    svideoindex++;
                 }
             }
           } 

              /* convert and stream audio data */
           if ( x->x_avcontext->streams[i]->codec.codec_type == CODEC_TYPE_AUDIO )
           {
                    // we assume audio is synchronized on next video stream 
                 if ( ( (i+1) < x->x_avcontext->nb_streams ) &&
                      ( x->x_avcontext->streams[i+1]->codec.codec_type == CODEC_TYPE_VIDEO ) )
                 {
                     x->x_audio_per_frame = 
                        // 2*( (int) sys_getsr() ) / ( x->x_avcontext->streams[i+1]->codec.frame_rate/10000);
                        AUDIO_PACKET_SIZE;
                     // post ("pdp_ffmpeg~ : transmit %d samples", x->x_audio_per_frame );
                 }
                 else
                 {
                     post ("pdp_ffmpeg~ : can't stream audio : video stream is not found" );
                     continue;
                 }
       
                 if ( x->x_audioin_position > x->x_audio_per_frame )
                 {
                    size = x->x_audioin_position;
                    if ( ( x->x_avcontext->streams[i]->codec.sample_rate != (int)sys_getsr() ) ||
                         ( x->x_avcontext->streams[i]->codec.channels != 2 ) )
                    {
                      if (x->x_audio_resample_ctx) audio_resample_close(x->x_audio_resample_ctx);
                      x->x_audio_resample_ctx = 
                       audio_resample_init(x->x_avcontext->streams[i]->codec.channels, 2,
                                         x->x_avcontext->streams[i]->codec.sample_rate,
                                         (int)sys_getsr());
                      sizeout = audio_resample(x->x_audio_resample_ctx,
                                    x->x_audio_enc_buf, 
                                    x->x_audio_buf,
                                    size / (x->x_avcontext->streams[i]->codec.channels * 2));
                      pencbuf = (short*) &x->x_audio_enc_buf;
                      sizeout = sizeout * x->x_avcontext->streams[i]->codec.channels * 2;
                    }
                    else
                    {
                      pencbuf = (short*) &x->x_audio_buf;
                      sizeout = size;
                    }

                      /* output resampled raw samples */
                    fifo_write(&x->x_audio_fifo[saudioindex], (uint8_t*)pencbuf, sizeout,
                               &x->x_audio_fifo[saudioindex].wptr);

                    framebytes = x->x_avcontext->streams[i]->codec.frame_size * 2 * 
                                 x->x_avcontext->streams[i]->codec.channels;
 
                    while (fifo_read(&x->x_audio_fifo[saudioindex], (uint8_t*)pencbuf, framebytes,
                                     &x->x_audio_fifo[saudioindex].rptr) == 0) 
                    {
#if LIBAVCODEC_BUILD > 4715	
                      AVPacket apkt;
#endif
                         encsize = avcodec_encode_audio(&x->x_avcontext->streams[i]->codec, 
                                       (uint8_t*)&x->x_audio_out, sizeof(x->x_audio_out),
                                       (short *)pencbuf);
#if LIBAVCODEC_BUILD > 4715	
                         av_init_packet(&apkt);

                         apkt.pts = etime.tv_sec*1000000 + etime.tv_usec;
                         if((&x->x_avcontext->streams[i]->codec)->coded_frame->key_frame) 
                                  apkt.flags |= PKT_FLAG_KEY;
                         apkt.stream_index= i;
                         apkt.data= (uint8_t *)x->x_audio_out;
                         apkt.size= encsize;
                         
                         av_write_frame(x->x_avcontext, &apkt);
#else
                         av_write_frame(x->x_avcontext, i, x->x_audio_out, encsize);
#endif
                    }
                    saudioindex++;
                 }
           }
        }
#endif
        x->x_audioin_position=0;
    }
    return;
}

static void pdp_ffmpeg_killpacket(t_pdp_ffmpeg *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;
}

    /* store audio data in PCM format and stream it */
static t_int *pdp_ffmpeg_perform(t_int *w)
{
  t_float *in1   = (t_float *)(w[1]);       // left audio inlet
  t_float *in2   = (t_float *)(w[2]);       // right audio inlet 
  t_pdp_ffmpeg *x = (t_pdp_ffmpeg *)(w[3]);
  int n = (int)(w[4]);                      // number of samples 
  t_float fsample;
  int   isample, i;

    // just fills the buffer
    while (n--)
    {
       fsample=*(in1++); 
       if (fsample > 1.0) { fsample = 1.0; }
       if (fsample < -1.0) { fsample = -1.0; }
       isample=(short) (32767.0 * fsample);
       *(x->x_audio_buf+x->x_audioin_position)=isample;
       x->x_audioin_position=(x->x_audioin_position+1)%(2*MAX_AUDIO_PACKET_SIZE); 
       if ( x->x_audioin_position == 2*MAX_AUDIO_PACKET_SIZE-1 ) 
       {
          // post( "pdp_ffmpeg~ : reaching end of audio buffer" );
       }
       fsample=*(in2++); 
       if (fsample > 1.0) { fsample = 1.0; }
       if (fsample < -1.0) { fsample = -1.0; }
       isample=(short) (32767.0 * fsample);
       *(x->x_audio_buf+x->x_audioin_position)=isample;
       x->x_audioin_position=(x->x_audioin_position+1)%(2*MAX_AUDIO_PACKET_SIZE); 
       if ( x->x_audioin_position == 2*MAX_AUDIO_PACKET_SIZE-1 ) 
       {
          // post( "pdp_ffmpeg~ : reaching end of audio buffer" );
       }
    }

    return (w+5);
}

static void pdp_ffmpeg_dsp(t_pdp_ffmpeg *x, t_signal **sp)
{
    dsp_add(pdp_ffmpeg_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void pdp_ffmpeg_process(t_pdp_ffmpeg *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_ffmpeg_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            pdp_queue_add(x, pdp_ffmpeg_process_yv12, pdp_ffmpeg_killpacket, &x->x_queue_id);
            outlet_float( x->x_outlet_nbframes, x->x_nbframes );
            outlet_float( x->x_outlet_nbframes_dropped, x->x_nbframes_dropped );
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_ffmpeg_process */
	    break;
	    
	}
    }

}

static void pdp_ffmpeg_input_0(t_pdp_ffmpeg *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_ffmpeg_process(x);
    }

}

static void pdp_ffmpeg_free(t_pdp_ffmpeg *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_ffmpeg_free_ressources(x); 
    if (x->x_img_resample_ctx) 
    {
      img_resample_close(x->x_img_resample_ctx);
      x->x_img_resample_ctx = NULL;
    }
    if (x->x_audio_resample_ctx) 
    {
      audio_resample_close(x->x_audio_resample_ctx);
      x->x_audio_resample_ctx = NULL;
    }
    av_free_static();
}

t_class *pdp_ffmpeg_class;

void *pdp_ffmpeg_new(void)
{
    int i;

    t_pdp_ffmpeg *x = (t_pdp_ffmpeg *)pd_new(pdp_ffmpeg_class);
    inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym ("signal"), gensym ("signal"));

    x->x_outlet_streaming = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_nbframes = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_nbframes_dropped = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_framerate = outlet_new(&x->x_obj, &s_float);

    x->x_packet0 = -1;
    x->x_queue_id = -1;
    x->x_nbframes = 0;
    x->x_nbframes_dropped = 0;
    x->x_img_resample_ctx = NULL;
    x->x_audio_resample_ctx = NULL;
    x->x_secondcount = NULL;
    x->x_nbvideostreams = 0;
    x->x_audioin_position = 0;

    x->x_rdata = NULL;
    x->x_buf1 = NULL;
    x->x_buf2 = NULL;
    x->x_avcontext = av_mallocz(sizeof(AVFormatContext));
    x->x_video_buffer = av_malloc( VIDEO_BUFFER_SIZE );
    if ( !x->x_video_buffer || !x->x_avcontext )
    {
       post( "pdp_ffmpeg~ : severe error : could not allocate video structures." );
       return NULL;
    }

    // activate codecs
    av_register_all();

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_ffmpeg_tilde_setup(void)
{
    // post( pdp_ffmpeg_version );
    pdp_ffmpeg_class = class_new(gensym("pdp_ffmpeg~"), (t_newmethod)pdp_ffmpeg_new,
    	(t_method)pdp_ffmpeg_free, sizeof(t_pdp_ffmpeg), 0, A_NULL);

    CLASS_MAINSIGNALIN(pdp_ffmpeg_class, t_pdp_ffmpeg, x_f );
    class_addmethod(pdp_ffmpeg_class, (t_method)pdp_ffmpeg_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_ffmpeg_class, (t_method)pdp_ffmpeg_dsp, gensym("dsp"), 0);
    class_addmethod(pdp_ffmpeg_class, (t_method)pdp_ffmpeg_feed, gensym("feed"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_ffmpeg_class, (t_method)pdp_ffmpeg_starve, gensym("starve"), A_NULL);


}

#ifdef __cplusplus
}
#endif
