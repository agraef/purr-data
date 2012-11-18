/*
 *   PiDiP module.
 *   Copyright (c) by Yves Degoyon <ydegoyon@free.fr>
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

/*  This object is a theora file encoder object
 *  It uses libtheora and some of it code samples ( copyright xiph.org )
 *  Copyleft by Yves Degoyon ( ydegoyon@free.fr )
 *
 */


#include "pdp.h"
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <theora/theora.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

#define DEFAULT_FRAME_RATE 25
#define MIN_VIDEO_QUALITY 0
#define MAX_VIDEO_QUALITY 63
#define DEFAULT_VIDEO_QUALITY 16
#define MIN_VIDEO_BITRATE 45
#define MAX_VIDEO_BITRATE 2000
#define DEFAULT_VIDEO_BITRATE 96
#define MIN_AUDIO_QUALITY -0.1
#define MAX_AUDIO_QUALITY 1.0
#define DEFAULT_AUDIO_QUALITY 0.5
#define MIN_AUDIO_BITRATE 8
#define MAX_AUDIO_BITRATE 2000
#define DEFAULT_AUDIO_BITRATE 32

#define DEFAULT_CHANNELS 2
#define DEFAULT_BITS 8
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)
// streams hard-coded serial numbers
#define STREAMV_SNO 0x987654
#define STREAMA_SNO 0x456789

#ifndef _REENTRANT
# define _REENTRANT
#endif

static char   *pdp_theorout_version = "pdp_theorout~: version 0.1, a theora video/audio recording object, written by ydegoyon@free.fr";

typedef struct pdp_theorout_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_vwidth;
    int x_tvwidth;       /* theora 16 pixels aligned width value */
    int x_vheight;
    int x_tvheight;      /* theora 16 pixels aligned height value */
    int x_vsize;

    FILE  *x_tfile;
    int x_framerate;
    int x_newfile;
    int x_einit;
    int x_recflag;
    int x_enduprec;;
    int x_frameswritten;
    int x_frames;
    struct timeval x_tstart;
    struct timeval x_tzero;
    struct timeval x_tcurrent;
    struct timeval x_tlastrec;

     /* vorbis/theora structures */
    ogg_page         x_ogg_page;       // ogg page
    ogg_packet       x_ogg_packet;     // ogg packet
    ogg_stream_state x_statev;         // vorbis stream state
    ogg_stream_state x_statet;         // theora stream state
    theora_info      x_theora_info;    // theora info
    theora_comment   x_theora_comment; // theora comment
    theora_state     x_theora_state;   // theora state
    vorbis_info      x_vorbis_info;    // vorbis info
    vorbis_dsp_state x_dsp_state;      // vorbis dsp state
    vorbis_block     x_vorbis_block;   // vorbis block
    vorbis_comment   x_vorbis_comment; // vorbis comment
    yuv_buffer       x_yuvbuffer;      // yuv buffer

    int            x_akbps;          // audio bit rate
    int            x_vkbps;          // video bit rate
    t_float          x_aquality;       // audio quality
    int            x_vquality;       // video quality
    int            x_abytesout;      // audio bytes written
    int            x_vbytesout;      // video bytes written

     /* audio structures */
    t_float **x_audio_buf; /* buffer for incoming audio */
    int x_audioin_position; // writing position for incoming audio
    int x_channels;      // audio channels 
    int x_samplerate;    // audio sample rate 
    int x_bits;          // audio bits

} t_pdp_theorout;

    /* allocate internal ressources */
static void pdp_theorout_allocate(t_pdp_theorout *x)
{
  int ret;

    x->x_yuvbuffer.y_width=x->x_vwidth;
    x->x_yuvbuffer.y_height=x->x_vheight;
    x->x_yuvbuffer.y_stride=x->x_vwidth;

    x->x_yuvbuffer.uv_width=x->x_vwidth>>1;
    x->x_yuvbuffer.uv_height=x->x_vheight>>1;
    x->x_yuvbuffer.uv_stride=x->x_vwidth>>1;

    x->x_yuvbuffer.y = (unsigned char *)malloc( x->x_yuvbuffer.y_width * x->x_yuvbuffer.y_height );
    x->x_yuvbuffer.u = (unsigned char *)malloc( x->x_yuvbuffer.uv_width * x->x_yuvbuffer.uv_height );
    x->x_yuvbuffer.v = (unsigned char *)malloc( x->x_yuvbuffer.uv_width * x->x_yuvbuffer.uv_height );
}

    /* free internal ressources */
static void pdp_theorout_free_ressources(t_pdp_theorout *x)
{
    if ( x->x_yuvbuffer.y ) free( x->x_yuvbuffer.y );
    if ( x->x_yuvbuffer.u ) free( x->x_yuvbuffer.u );
    if ( x->x_yuvbuffer.v ) free( x->x_yuvbuffer.v );
}

    /* initialize the encoder */
static void pdp_theorout_init_encoder(t_pdp_theorout *x)
{
  int ret;

    x->x_einit=0;

    // init streams
    ogg_stream_init(&x->x_statet, STREAMA_SNO);
    ogg_stream_init(&x->x_statev, STREAMV_SNO);

    theora_info_init(&x->x_theora_info);
    x->x_theora_info.width=x->x_tvwidth;
    x->x_theora_info.height=x->x_tvheight;
    x->x_theora_info.frame_width=x->x_vwidth;
    x->x_theora_info.frame_height=x->x_vheight;
    x->x_theora_info.offset_x=(x->x_tvwidth-x->x_vwidth)>>1;
    x->x_theora_info.offset_y=(x->x_tvheight-x->x_vheight)>>1;
    x->x_theora_info.fps_numerator=x->x_framerate;
    x->x_theora_info.fps_denominator=1;
    x->x_theora_info.aspect_numerator=1;
    x->x_theora_info.aspect_denominator=1;
    x->x_theora_info.colorspace=OC_CS_UNSPECIFIED;
    x->x_theora_info.target_bitrate=x->x_vkbps;
    x->x_theora_info.quality=x->x_vquality;

    x->x_theora_info.dropframes_p=0;
    x->x_theora_info.quick_p=1;
    x->x_theora_info.keyframe_auto_p=1;
    x->x_theora_info.keyframe_frequency=64;
    x->x_theora_info.keyframe_frequency_force=64;
    x->x_theora_info.keyframe_data_target_bitrate=x->x_vkbps*1.5;
    x->x_theora_info.keyframe_auto_threshold=80;
    x->x_theora_info.keyframe_mindistance=8;
    x->x_theora_info.noise_sensitivity=1; 
    x->x_theora_info.sharpness=2; 

    theora_encode_init(&x->x_theora_state,&x->x_theora_info);
    theora_info_clear (&x->x_theora_info);

    vorbis_info_init(&x->x_vorbis_info);

    if(x->x_aquality > -0.1)
    {
      ret = vorbis_encode_init_vbr(&x->x_vorbis_info, x->x_channels, x->x_samplerate, x->x_aquality);
    }
    else
    {
      ret = vorbis_encode_init(&x->x_vorbis_info, x->x_channels, x->x_samplerate, -1, x->x_akbps, -1);
    }

    if (ret)
    {
      post( "pdp_theorout~ : could not initialize vorbis encoder" );
      x->x_einit=0;
      return;
    }

    vorbis_comment_init(&x->x_vorbis_comment);
    vorbis_comment_add_tag (&x->x_vorbis_comment, "ENCODER", "pdp_theorout~");
    vorbis_analysis_init(&x->x_dsp_state,&x->x_vorbis_info);
    vorbis_block_init(&x->x_dsp_state,&x->x_vorbis_block);
    
    post( "pdp_theorout~ : encoder initialized." );
    x->x_einit=1;

}

static void pdp_theorout_write_headers(t_pdp_theorout *x)
{
  int ret;
  ogg_packet aheader, aheadercomm, aheadercode;

    if ( !x->x_einit )
    {
      post( "pdp_theorout~ : trying to write headers but encoder is not initialized." );
      return;
    }

    if ( x->x_tfile == NULL )
    {
      post( "pdp_theorout~ : trying to write headers but no file is opened." );
      return;
    }

    theora_encode_header(&x->x_theora_state, &x->x_ogg_packet);
    ogg_stream_packetin(&x->x_statet, &x->x_ogg_packet);
    if(ogg_stream_pageout(&x->x_statet, &x->x_ogg_page)!=1)
    {
      post( "pdp_theorout~ : ogg encoding error." );
      return;
    }
    if ( ( ret = fwrite(x->x_ogg_page.header, 1, x->x_ogg_page.header_len, x->x_tfile) ) <= 0 )
    {
      post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
      perror( "fwrite" );
      return;
    }
    if ( ( ret = fwrite(x->x_ogg_page.body, 1, x->x_ogg_page.body_len, x->x_tfile) ) <= 0 )
    {
      post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
      perror( "fwrite" );
      return;
    }

    theora_comment_init(&x->x_theora_comment);
    theora_comment_add_tag (&x->x_theora_comment, "ENCODER", "pdp_theorout~");
    theora_encode_comment(&x->x_theora_comment, &x->x_ogg_packet);
    ogg_stream_packetin(&x->x_statet, &x->x_ogg_packet);
    theora_encode_tables(&x->x_theora_state, &x->x_ogg_packet);
    ogg_stream_packetin(&x->x_statet, &x->x_ogg_packet);

    vorbis_analysis_headerout(&x->x_dsp_state, &x->x_vorbis_comment, 
                              &aheader,&aheadercomm,&aheadercode);
    ogg_stream_packetin(&x->x_statev,&aheader); 

    if(ogg_stream_pageout(&x->x_statev, &x->x_ogg_page)!=1)
    {
      post( "pdp_theorout~ : ogg encoding error." );
      return;
    }
    if ( ( ret = fwrite(x->x_ogg_page.header, 1, x->x_ogg_page.header_len, x->x_tfile) ) <= 0 )
    {
      post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
      perror( "fwrite" );
      return;
    }
    if ( ( ret = fwrite(x->x_ogg_page.body, 1, x->x_ogg_page.body_len, x->x_tfile) ) <= 0 )
    {
      post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
      perror( "fwrite" );
      return;
    }

    // remaining vorbis header packets 
    ogg_stream_packetin(&x->x_statev, &aheadercomm);
    ogg_stream_packetin(&x->x_statev, &aheadercode);

    // flush all the headers
    while(1)
    {
      ret = ogg_stream_flush(&x->x_statet, &x->x_ogg_page);
      if(ret<0){
        post( "pdp_theorout~ : ogg encoding error." );
        return;
      }
      if(ret==0)break;
      if ( ( ret = fwrite(x->x_ogg_page.header, 1, x->x_ogg_page.header_len, x->x_tfile) ) <= 0 )
      {
        post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
        perror( "fwrite" );
        return;
      }
      if ( ( ret = fwrite(x->x_ogg_page.body, 1, x->x_ogg_page.body_len, x->x_tfile) ) <= 0 )
      {
        post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
        perror( "fwrite" );
        return;
      }
    }

    while(1)
    {
      ret = ogg_stream_flush(&x->x_statev, &x->x_ogg_page);
      if(ret<0){
        post( "pdp_theorout~ : ogg encoding error." );
        return;
      }
      if(ret==0)break;
      if ( ( ret = fwrite(x->x_ogg_page.header, 1, x->x_ogg_page.header_len, x->x_tfile) ) <= 0 )
      {
        post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
        perror( "fwrite" );
        return;
      }
      if ( ( ret = fwrite(x->x_ogg_page.body, 1, x->x_ogg_page.body_len, x->x_tfile) ) <= 0 )
      {
        post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
        perror( "fwrite" );
        return;
      }
    }
}

    /* terminate the encoding process */
static void pdp_theorout_shutdown_encoder(t_pdp_theorout *x)
{
    ogg_stream_clear(&x->x_statev);
    vorbis_block_clear(&x->x_vorbis_block);
    vorbis_dsp_clear(&x->x_dsp_state);
    vorbis_comment_clear(&x->x_vorbis_comment);
    vorbis_info_clear(&x->x_vorbis_info);
    ogg_stream_clear(&x->x_statet);
    theora_clear(&x->x_theora_state);
}


    /* close a video file */
static void pdp_theorout_close(t_pdp_theorout *x)
{
  int ret;

    if ( x->x_tfile ) 
    {
       if ( fclose( x->x_tfile ) < 0 )
       {
          post( "pdp_theorout~ : could not close output file" );
          perror( "fclose" );
       }
       x->x_tfile = NULL;   
    }
}

    /* open a new video file */
static void pdp_theorout_open(t_pdp_theorout *x, t_symbol *sfile)
{
  int ret=0;

    // close previous video file if existing
    pdp_theorout_close(x);

    if ( x->x_recflag ) {
      x->x_recflag = 0;
    }
    x->x_frameswritten = 0;

    if ( ( x->x_tfile = fopen( sfile->s_name, "w+" ) ) == NULL )
    {
      post( "pdp_theorout~ : could not open output file" );
      perror( "fopen" );
      return;
    }
    else
    {
      post( "pdp_theorout~ : opened >%s<", sfile->s_name);
    }
    x->x_newfile = 1;

}

   /* start recording */
static void pdp_theorout_start(t_pdp_theorout *x)
{
    if ( !x->x_tfile ) 
    {
       post("pdp_theorout~ : start received but no file has been opened ... ignored.");
       return;
    }

    if ( x->x_recflag == 1 ) 
    {
       post("pdp_theorout~ : start received but recording is started ... ignored.");
       return;
    }

    if ( gettimeofday(&x->x_tstart, NULL) == -1)
    {
       post("pdp_theorout~ : could not set start time" );
    }

    x->x_recflag = 1;
    pdp_theorout_init_encoder( x );
    pdp_theorout_write_headers( x );
    post("pdp_theorout~ : start recording at %d frames/second", x->x_framerate);
}

   /* stop recording */
static void pdp_theorout_stop(t_pdp_theorout *x)
{
    if ( !x->x_tfile ) 
    {
       post("pdp_theorout~ : stop received but no file has been opened ... ignored.");
       return;
    }

    if ( x->x_recflag == 0 ) 
    {
       post("pdp_theorout~ : stop received but recording is stopped ... ignored.");
       return;
    }

    x->x_recflag = 0;

    // record last packet
    x->x_enduprec = 1;

}

   /* set video bitrate */
static void pdp_theorout_vbitrate(t_pdp_theorout *x, t_floatarg vbitrate )
{
  if ( ( (int) vbitrate < MIN_VIDEO_BITRATE ) || ( (int) vbitrate > MAX_VIDEO_BITRATE ) )
  {
     post( "pdp_theorout~ : wrong video bitrate %d : should be in [%d,%d] kbps", 
                            (int) vbitrate, MIN_VIDEO_BITRATE, MAX_VIDEO_BITRATE );
     return;
  }
  x->x_vkbps = (int) vbitrate;
}

   /* set audio bitrate */
static void pdp_theorout_abitrate(t_pdp_theorout *x, t_floatarg abitrate )
{
  if ( ( (int) abitrate < MIN_AUDIO_BITRATE ) || ( (int) abitrate > MAX_AUDIO_BITRATE ) )
  {
     post( "pdp_theorout~ : wrong audio bitrate %d : should be in [%d,%d] kbps", 
                            (int) abitrate, MIN_AUDIO_BITRATE, MAX_AUDIO_BITRATE );
     return;
  }
  x->x_akbps = (int) abitrate;
}

   /* set video quality */
static void pdp_theorout_vquality(t_pdp_theorout *x, t_floatarg vquality )
{
  if ( ( (int) vquality < MIN_VIDEO_QUALITY ) || ( (int) vquality > MAX_VIDEO_QUALITY ) )
  {
     post( "pdp_theorout~ : wrong video quality %d : should be in [%d,%d]", 
                            (int) vquality, MIN_VIDEO_QUALITY, MAX_VIDEO_QUALITY );
     return;
  }
  x->x_vquality = (int) vquality;
}

   /* set audio quality */
static void pdp_theorout_aquality(t_pdp_theorout *x, t_floatarg aquality )
{
  if ( ( (int) aquality < MIN_AUDIO_QUALITY ) || ( (int) aquality > MAX_AUDIO_QUALITY ) )
  {
     post( "pdp_theorout~ : wrong audio quality %d : should be in [%d,%d]", 
                            (int) aquality, MIN_AUDIO_QUALITY, MAX_AUDIO_QUALITY );
     return;
  }
  x->x_aquality = (int) aquality;
}

    /* store audio data in PCM format in a buffer for now */
static t_int *pdp_theorout_perform(t_int *w)
{
  t_float *in1   = (t_float *)(w[1]);       // left audio inlet
  t_float *in2   = (t_float *)(w[2]);       // right audio inlet
  t_pdp_theorout *x = (t_pdp_theorout *)(w[3]);
  int n = (int)(w[4]);                      // number of samples
  t_float fsample;
  int   isample, i;

   if ( x->x_recflag ) 
   {
    // just fills the buffer
    while (n--)
    {
       fsample=*(in1++);
       if (fsample > 1.0) { fsample = 1.0; }
       if (fsample < -1.0) { fsample = -1.0; }
       x->x_audio_buf[0][x->x_audioin_position]=fsample;
       fsample=*(in2++);
       if (fsample > 1.0) { fsample = 1.0; }
       if (fsample < -1.0) { fsample = -1.0; }
       x->x_audio_buf[1][x->x_audioin_position]=fsample;
       x->x_audioin_position=(x->x_audioin_position+1)%(MAX_AUDIO_PACKET_SIZE);
       if ( x->x_audioin_position == MAX_AUDIO_PACKET_SIZE-1 )
       {
          post( "pdp_theorout~ : reaching end of audio buffer" );
       }
    }
  }

  return (w+5);
}

static void pdp_theorout_dsp(t_pdp_theorout *x, t_signal **sp)
{
    dsp_add(pdp_theorout_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void pdp_theorout_process_yv12(t_pdp_theorout *x)
{
  t_pdp     *header = pdp_packet_header(x->x_packet0);
  unsigned char *data   = (unsigned char *)pdp_packet_data(x->x_packet0);
  int     i, ret;
  int     px, py;
  unsigned char *pY, *pU, *pV;
  struct timeval trec;
  int     nbaudiosamples, nbusecs, nbrecorded;
  t_float   fframerate=0.0;
  int     precflag;
  ogg_page  apage;
  ogg_page  vpage;
  t_float   **vbuffer;
  double    videotime, audiotime;
  theora_info    lti;
  theora_comment ltc;
  ogg_packet logp, logp2;


    if ( ( (int)(header->info.image.width) != x->x_vwidth ) || 
         ( (int)(header->info.image.height) != x->x_vheight ) || 
         ( x->x_newfile ) )
    {
       precflag = x->x_recflag;
       x->x_recflag = 0;
       pdp_theorout_free_ressources( x );
       pdp_theorout_shutdown_encoder( x );
       x->x_vwidth = header->info.image.width;
       x->x_vheight = header->info.image.height;
       x->x_vsize = x->x_vwidth*x->x_vheight;
       x->x_tvwidth=((x->x_vwidth + 15) >>4)<<4;
       x->x_tvheight=((x->x_vheight + 15) >>4)<<4;
       pdp_theorout_allocate( x );
       if ( x->x_tzero.tv_sec != 0 )
       {
         pdp_theorout_init_encoder( x );
         pdp_theorout_write_headers( x );
       }
       x->x_recflag = precflag;
       x->x_newfile = 0;
    }

    if ( x->x_tzero.tv_sec == 0 )
    {
      if ( gettimeofday(&x->x_tzero, NULL) == -1)
      {
         post("pdp_theorout~ : could get initial time" );
      }
    }
 
    x->x_frames++;

    // calculate current framerate
    if ( gettimeofday(&x->x_tcurrent, NULL) == -1)
    {
       post("pdp_theorout~ : could get current time" );
    }

    // calculate frame rate if it hasn't been set
    if ( ( x->x_tcurrent.tv_sec - x->x_tzero.tv_sec ) > 0 )
    {
      x->x_framerate = x->x_frames / ( x->x_tcurrent.tv_sec - x->x_tzero.tv_sec );
    }
    else
    {
      x->x_framerate = DEFAULT_FRAME_RATE;
    }

    if ( x->x_frameswritten == 0 )
    {
      if ( gettimeofday(&x->x_tlastrec, NULL) == -1)
      {
         post("pdp_theorout~ : could set start time" );
      }
    }

    pY = x->x_yuvbuffer.y;
    memcpy( (void*)pY, (void*)&data[0], x->x_vsize );
    pV = x->x_yuvbuffer.v;
    memcpy( (void*)pV, (void*)&data[x->x_vsize], (x->x_vsize>>2) );
    pU = x->x_yuvbuffer.u;
    memcpy( (void*)pU, (void*)&data[x->x_vsize+(x->x_vsize>>2)], (x->x_vsize>>2) );
      
    if ( x->x_tfile && x->x_recflag && !x->x_enduprec)
    {

      if ( ( ret = theora_encode_YUVin( &x->x_theora_state, &x->x_yuvbuffer ) ) != 0 )
      {
         post( "pdp_theorout~ : could not encode yuv image (ret=%d).", ret );
      }  
      else
      {
         // stream one packet
         theora_encode_packetout(&x->x_theora_state, 0, &logp);
         ogg_stream_packetin(&x->x_statet, &logp);
         // post( "pdp_theorout~ : new (theora) ogg packet : bytes:%ld, bos:%ld, eos:%ld, no:%lld",
         //                        logp.bytes, logp.b_o_s, 
         //                        logp.e_o_s, logp.packetno );


         while( ( ret = ogg_stream_pageout(&x->x_statet, &vpage) ) >0 )
         {
           videotime = theora_granule_time(&x->x_theora_state, ogg_page_granulepos(&vpage));
           if ( ( ret = fwrite(vpage.header, 1, vpage.header_len, x->x_tfile) ) <= 0 )
           {
             post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
             perror( "fwrite" );
           }
           x->x_vbytesout+=ret;
           if ( ( ret = fwrite(vpage.body, 1, vpage.body_len, x->x_tfile) ) <= 0 )
           {
             post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
             perror( "fwrite" );
           }
           x->x_vbytesout+=ret;
         }
      }

      // calculate the number of audio samples to output
      if ( gettimeofday(&trec, NULL) == -1)
      {
         post("pdp_theorout~ : could set stop time" );
      }
      // calculate time diff in micro seconds
      nbusecs = ( trec.tv_usec - x->x_tlastrec.tv_usec ) + 
                ( trec.tv_sec - x->x_tlastrec.tv_sec )*1000000;
      nbaudiosamples = (sys_getsr()*1000000)/nbusecs;
      memcpy( &x->x_tlastrec, &trec, sizeof( struct timeval) );

      if ( x->x_audioin_position > nbaudiosamples )
      {
         nbrecorded = nbaudiosamples;
      }
      else
      {
         nbrecorded = x->x_audioin_position;
      }

      vbuffer=vorbis_analysis_buffer( &x->x_dsp_state, nbrecorded );
      memcpy( (void*)&vbuffer[0][0], (void*)&x->x_audio_buf[0][0], nbrecorded*sizeof( t_float ) );
      memcpy( (void*)&vbuffer[1][0], (void*)&x->x_audio_buf[1][0], nbrecorded*sizeof( t_float ) );

      vorbis_analysis_wrote( &x->x_dsp_state, nbrecorded);

      while(vorbis_analysis_blockout( &x->x_dsp_state, &x->x_vorbis_block)==1)
      {

        // analysis, assume we want to use bitrate management
        vorbis_analysis( &x->x_vorbis_block, NULL);
        vorbis_bitrate_addblock( &x->x_vorbis_block );

        // weld packets into the bitstream 
        while(vorbis_bitrate_flushpacket( &x->x_dsp_state, &logp2))
        {
          ogg_stream_packetin( &x->x_statev, &logp2);
        }

      }

      while( ogg_stream_pageout( &x->x_statev, &apage) >0 )
      {
        audiotime = vorbis_granule_time(&x->x_dsp_state, ogg_page_granulepos(&apage));
        if ( ( ret = fwrite(apage.header, 1, apage.header_len, x->x_tfile) ) <= 0 )
        {
          post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
          perror( "fwrite" );
        }
        x->x_abytesout+=ret;
        if ( ( ret = fwrite(apage.body, 1, apage.body_len, x->x_tfile) ) <= 0 )
        {
          post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
          perror( "fwrite" );
        }
        x->x_abytesout+=ret;
      }

      memcpy( &x->x_audio_buf[0][0], &x->x_audio_buf[0][nbrecorded], 
                     ( x->x_audioin_position-nbrecorded ) * sizeof( t_float ) );
      memcpy( &x->x_audio_buf[1][0], &x->x_audio_buf[1][nbrecorded], 
                     ( x->x_audioin_position-nbrecorded ) * sizeof( t_float ) );
      x->x_audioin_position -= nbrecorded;
      // post ( "pdp_theorout~ : recorded %d samples.", nbrecorded );
       
      x->x_frameswritten++;

    } 

    if ( x->x_tfile && x->x_enduprec )
    {
      x->x_enduprec = 0;
      post( "pdp_theorout~ : ending up recording." );
      x->x_frameswritten++;

      if ( ( ret = theora_encode_YUVin( &x->x_theora_state, &x->x_yuvbuffer ) ) != 0 )
      {
         post( "pdp_theorout~ : could not encode yuv image (ret=%d).", ret );
      }  
      else
      {
         // stream one packet
         theora_encode_packetout(&x->x_theora_state, 1, &logp);
         ogg_stream_packetin( &x->x_statet, &logp);

         while( ( ret = ogg_stream_pageout( &x->x_statet, &vpage) ) > 0 )
         {
           videotime = theora_granule_time(&x->x_theora_state, ogg_page_granulepos(&vpage));
           if ( ( ret = fwrite(vpage.header, 1, vpage.header_len, x->x_tfile) ) <= 0 )
           {
             post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
             perror( "fwrite" );
           }
           x->x_vbytesout+=ret;
           if ( ( ret = fwrite(vpage.body, 1, vpage.body_len, x->x_tfile) ) <= 0 )
           {
             post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
             perror( "fwrite" );
           }
           x->x_vbytesout+=ret;
         }
      }

      // end up audio stream 
      vorbis_analysis_wrote( &x->x_dsp_state, 0);

      while(vorbis_analysis_blockout( &x->x_dsp_state, &x->x_vorbis_block)==1)
      {
        // analysis, assume we want to use bitrate management
        vorbis_analysis( &x->x_vorbis_block, NULL);
        vorbis_bitrate_addblock( &x->x_vorbis_block);

        // weld packets into the bitstream 
        while(vorbis_bitrate_flushpacket( &x->x_dsp_state, &logp2))
        {
          ogg_stream_packetin( &x->x_statev, &logp2);
        }
      }

      while( ogg_stream_pageout( &x->x_statev, &apage) >0 )
      {
        audiotime = vorbis_granule_time(&x->x_dsp_state, ogg_page_granulepos(&apage));
        if ( ( ret = fwrite(apage.header, 1, apage.header_len, x->x_tfile) ) <= 0 )
        {
          post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
          perror( "fwrite" );
        }
        x->x_abytesout+=ret;
        if ( ( ret = fwrite(apage.body, 1, apage.body_len, x->x_tfile) ) <= 0 )
        {
          post( "pdp_theorout~ : could not write headers (ret=%d).", ret );
          perror( "fwrite" );
        }
        x->x_abytesout+=ret;
      }

      post("pdp_theorout~ : stop recording");

      pdp_theorout_shutdown_encoder( x );
      pdp_theorout_close(x);
    }

    return;
}

static void pdp_theorout_killpacket(t_pdp_theorout *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;
}

static void pdp_theorout_process(t_pdp_theorout *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_theorout_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_BITMAP_YV12:
            if ( x->x_tfile && x->x_recflag )
            {
              outlet_float( x->x_obj.ob_outlet, x->x_frameswritten );
            }
            pdp_queue_add(x, pdp_theorout_process_yv12, pdp_theorout_killpacket, &x->x_queue_id);
	    break;

	  default:
	    /* don't know the type, so dont pdp_theorout_process */
	    break;
	    
	}
    }

}

static void pdp_theorout_input_0(t_pdp_theorout *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
        x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("bitmap/yv12/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_theorout_process(x);
    }

}

static void pdp_theorout_free(t_pdp_theorout *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    // close video file if existing
    pdp_theorout_close(x);
    for ( i=0; i<x->x_channels; i++)
    {
       if ( x->x_audio_buf[i] ) freebytes( x->x_audio_buf[i], MAX_AUDIO_PACKET_SIZE*sizeof(t_float) );
    }
    if ( x->x_audio_buf ) freebytes( x->x_audio_buf, x->x_channels*sizeof(t_float*) );
    
}

t_class *pdp_theorout_class;

void *pdp_theorout_new(void)
{
  int i;

    t_pdp_theorout *x = (t_pdp_theorout *)pd_new(pdp_theorout_class);
    inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym ("signal"), gensym ("signal"));
    outlet_new (&x->x_obj, &s_float);

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_tfile = NULL;
    x->x_yuvbuffer.y = NULL;
    x->x_yuvbuffer.u = NULL;
    x->x_yuvbuffer.v = NULL;

     /* audio defaults */
    x->x_samplerate = sys_getsr();
    x->x_channels = DEFAULT_CHANNELS;
    x->x_bits = DEFAULT_BITS;

    x->x_framerate = DEFAULT_FRAME_RATE;
    x->x_vkbps = DEFAULT_VIDEO_BITRATE;
    x->x_vquality = DEFAULT_VIDEO_QUALITY;
    x->x_akbps = DEFAULT_AUDIO_BITRATE;
    x->x_aquality = DEFAULT_AUDIO_QUALITY;

    x->x_audio_buf = (t_float**) getbytes( x->x_channels*sizeof(t_float*) );
    for ( i=0; i<x->x_channels; i++)
    {
       x->x_audio_buf[i] = (t_float*) getbytes( MAX_AUDIO_PACKET_SIZE*sizeof(t_float) );
    }

    x->x_newfile = 0;
    x->x_frames = 0;
    x->x_frameswritten = 0;

    x->x_tzero.tv_sec = 0;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_theorout_tilde_setup(void)
{
    // post( pdp_theorout_version );
    pdp_theorout_class = class_new(gensym("pdp_theorout~"), (t_newmethod)pdp_theorout_new,
    	(t_method)pdp_theorout_free, sizeof(t_pdp_theorout), 0, A_NULL);

    CLASS_MAINSIGNALIN(pdp_theorout_class, t_pdp_theorout, x_f );
    class_addmethod(pdp_theorout_class, (t_method)pdp_theorout_dsp, gensym("dsp"), 0);
    class_addmethod(pdp_theorout_class, (t_method)pdp_theorout_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_theorout_class, (t_method)pdp_theorout_open, gensym("open"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_theorout_class, (t_method)pdp_theorout_close, gensym("close"), A_NULL);
    class_addmethod(pdp_theorout_class, (t_method)pdp_theorout_start, gensym("start"), A_NULL);
    class_addmethod(pdp_theorout_class, (t_method)pdp_theorout_stop, gensym("stop"), A_NULL);
    class_addmethod(pdp_theorout_class, (t_method)pdp_theorout_abitrate, gensym("audiobitrate"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theorout_class, (t_method)pdp_theorout_vbitrate, gensym("videobitrate"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theorout_class, (t_method)pdp_theorout_aquality, gensym("audioquality"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theorout_class, (t_method)pdp_theorout_vquality, gensym("videoquality"), A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
