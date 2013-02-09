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

/*  This object is a theora a/v streaming object to an icecast server
 *  The patched icecast server can be found here : http://mediacast1.com/~karl/
 *  Written by Yves Degoyon ( ydegoyon@free.fr )
 *
 */


#include "pdp.h"
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <netdb.h>
#include <theora/theora.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

#define MAX_PACKETS_STACK 1

#define MIN_FRAMERATE 1
#define MAX_FRAMERATE 100
#define DEFAULT_FRAME_RATE 12
#define MIN_VIDEO_QUALITY 0
#define MAX_VIDEO_QUALITY 63
#define DEFAULT_VIDEO_QUALITY 16
#define MIN_VIDEO_BITRATE 45
#define MAX_VIDEO_BITRATE 2000
#define DEFAULT_VIDEO_BITRATE 48
#define MIN_AUDIO_QUALITY -0.1
#define MAX_AUDIO_QUALITY 1.0
#define DEFAULT_AUDIO_QUALITY 0.5
#define MIN_AUDIO_BITRATE 8
#define MAX_AUDIO_BITRATE 2000
#define DEFAULT_AUDIO_BITRATE 64

#define DEFAULT_CHANNELS 2
#define DEFAULT_BITS 8
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)

#define MAX_COMMENT_LENGTH 1024
#define STRBUF_SIZE 1024
#define OGG_AUDIO_SIZE 1024

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

static char base64table[65] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/',
};

static char   *pdp_theonice_version = "pdp_theonice~: version 1.5, a theora a/v streaming object, written by ydegoyon@free.fr";

typedef struct pdp_theonice_struct
{
    t_object x_obj;
    t_float x_f;

    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_vwidth;
    int x_tvwidth;       // theora 16 pixels aligned width value 
    int x_vheight;
    int x_tvheight;      // theora 16 pixels aligned height value 
    int x_vsize;

    pthread_t x_connectchild;      // thread used for connecting to a stream
    int x_socketfd;                // connection socket
    int x_streaming;             // streaming on : connected and all 
    int x_pstreaming;            // previous state
    char *x_passwd;               // password
    char x_title[MAX_COMMENT_LENGTH];         // title of the stream 
    char x_url[MAX_COMMENT_LENGTH];           // url of the stream 
    char x_genre[MAX_COMMENT_LENGTH];         // genre of the stream 
    char x_description[MAX_COMMENT_LENGTH];   // description 
    char x_artist[MAX_COMMENT_LENGTH];        // artist
    char x_copyright[MAX_COMMENT_LENGTH];
    char x_date[MAX_COMMENT_LENGTH];          // starting system date 
    char x_hostname[MAX_COMMENT_LENGTH];      // name or IP of host to connect to 
    char x_mountpoint[MAX_COMMENT_LENGTH];    // mountpoint
    int x_port;                     // port number
    int x_public;                   // publish on www.oggcast.com 
    int x_framerate;
    int x_mframerate;               // measured framerate 
    int x_pmframerate;               // previous state
    int x_einit;
    int x_frameswritten;
    int x_pframeswritten;
    int x_frameslate;
    int x_nbframes_dropped;
    int x_pnbframes_dropped;
    int x_frames;
    int x_apkg;
    int x_vpkg;
    struct timeval x_tstart;
    struct timeval x_tzero;
    struct timeval x_tcurrent;
    struct timeval x_tprevstream;
    int x_cursec;   // current second
    int x_secondcount; // number of frames emitted in the current second

     /* vorbis/theora structures */
    ogg_page         x_ogg_page;       // ogg page for headers
    ogg_page         x_apage;          // ogg audio page
    ogg_page         x_vpage;          // ogg video page
    ogg_packet       x_ogg_apacket;    // ogg packet
    ogg_packet       x_ogg_vpacket;    // ogg packet
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
    int              x_eos;            // end of stream 

    int              x_akbps;          // audio bit rate
    int              x_vkbps;          // video bit rate
    t_float          x_aquality;       // audio quality
    int              x_vquality;       // video quality
    int              x_abytesout;      // audio bytes written
    int              x_vbytesout;      // video bytes written
    double           x_audiotime;      // audio stream time
    double           x_paudiotime;     // previous value
    double           x_videotime;      // video stream time
    double           x_pvideotime;     // previous value

     /* audio structures */
    t_float x_audio_buf[DEFAULT_CHANNELS][MAX_AUDIO_PACKET_SIZE]; // buffer for incoming audio
    int x_audioin_position; // writing position for incoming audio
    int x_channels;      // audio channels 
    int x_samplerate;    // audio sample rate 
    int x_bits;          // audio bits

    t_outlet *x_outlet_streaming;  // indicates the action of streaming
    t_outlet *x_outlet_nbframes;   // number of frames emitted
    t_outlet *x_outlet_framerate;  // real framerate
    t_outlet *x_outlet_nbframes_dropped; // number of frames dropped
    t_outlet *x_outlet_atime;      // audio time
    t_outlet *x_outlet_vtime;      // video time
    t_float  **x_vbuffer;          // buffer from vorbis

} t_pdp_theonice;

    /* allocate internal ressources */
static void pdp_theonice_allocate(t_pdp_theonice *x)
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
static void pdp_theonice_free_ressources(t_pdp_theonice *x)
{
    if ( x->x_yuvbuffer.y ) free( x->x_yuvbuffer.y );
    if ( x->x_yuvbuffer.u ) free( x->x_yuvbuffer.u );
    if ( x->x_yuvbuffer.v ) free( x->x_yuvbuffer.v );
}

    /* initialize the encoder */
static void pdp_theonice_init_encoder(t_pdp_theonice *x)
{
  int ret;

    x->x_einit=0;

    // init streams
    srand(time(NULL));
    ogg_stream_init(&x->x_statet, rand());
    ogg_stream_init(&x->x_statev, rand());

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
    x->x_theora_info.colorspace=OC_CS_ITU_REC_470BG;
    x->x_theora_info.target_bitrate=x->x_vkbps;
    x->x_theora_info.quality=x->x_vquality;

    x->x_theora_info.dropframes_p=0;
    x->x_theora_info.quick_p=1;
    x->x_theora_info.keyframe_auto_p=1;
    x->x_theora_info.keyframe_frequency=10;
    x->x_theora_info.keyframe_frequency_force=10;
    x->x_theora_info.keyframe_data_target_bitrate=x->x_vkbps*5;
    x->x_theora_info.keyframe_auto_threshold=80;
    x->x_theora_info.keyframe_mindistance=8;
    x->x_theora_info.noise_sensitivity=1; 
    x->x_theora_info.sharpness=0; 

    theora_encode_init(&x->x_theora_state,&x->x_theora_info);

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
      post( "pdp_theonice~ : could not initialize vorbis encoder" );
      x->x_einit=0;
      return;
    }

    vorbis_comment_init(&x->x_vorbis_comment);
    vorbis_comment_add_tag(&(x->x_vorbis_comment),"TITLE", x->x_title);
    vorbis_comment_add_tag(&(x->x_vorbis_comment),"ARTIST", x->x_artist);
    vorbis_comment_add_tag(&(x->x_vorbis_comment),"GENRE",x->x_genre);
    vorbis_comment_add_tag(&(x->x_vorbis_comment),"DESCRIPTION", x->x_description);
    vorbis_comment_add_tag(&(x->x_vorbis_comment),"LOCATION",x->x_url);
    vorbis_comment_add_tag(&(x->x_vorbis_comment),"PERFORMER",x->x_artist);
    vorbis_comment_add_tag(&(x->x_vorbis_comment),"COPYRIGHT",x->x_copyright);
    vorbis_comment_add_tag(&(x->x_vorbis_comment),"DATE",x->x_date);
    vorbis_comment_add_tag(&(x->x_vorbis_comment),"ENCODER","pdp_theonice~ v1.5");
    vorbis_analysis_init(&x->x_dsp_state,&x->x_vorbis_info);
    vorbis_block_init(&x->x_dsp_state,&x->x_vorbis_block);
    
    post( "pdp_theonice~ : encoder initialized." );
    x->x_einit=1;
}

    /* terminate the encoding process */
static void pdp_theonice_shutdown_encoder(t_pdp_theonice *x)
{
  int ret;

    if ( x->x_streaming )
    {
      post( "pdp_theonice~ : shutting down encoder");
      vorbis_analysis_wrote(&x->x_dsp_state,0);
          // get rid of remaining data in encoder, if any 
      while(vorbis_analysis_blockout( &x->x_dsp_state, &x->x_vorbis_block )==1)
      {
         vorbis_analysis( &x->x_vorbis_block, NULL );
         vorbis_bitrate_addblock( &x->x_vorbis_block );
  
         while(vorbis_bitrate_flushpacket( &x->x_dsp_state, &x->x_ogg_apacket ))
         {
            ogg_stream_packetin( &x->x_statev, &x->x_ogg_apacket );
            x->x_apkg++;
            // post( "pdp_theonice~ : audio packets :%d", x->x_apkg);
         }
      }

      while(1)
      {
        ret = ogg_stream_flush(&x->x_statev, &x->x_apage);
        if(ret<0){
          post( "pdp_theonice~ : ogg encoding error." );
          return;
        }
        if(ret==0)break;
        if ( ( ret = send(x->x_socketfd, (void*)x->x_apage.header, x->x_apage.header_len, MSG_NOSIGNAL) ) < 0 )
        {
          post( "pdp_theonice~ : could not write audio packet (ret=%d).", ret );
          perror( "send" );
        }
        if ( ( ret = send(x->x_socketfd, (void*)x->x_apage.body, x->x_apage.body_len, MSG_NOSIGNAL) ) < 0 )
        {
          post( "pdp_theonice~ : could not write audio packet (ret=%d).", ret );
          perror( "send" );
        }
        x->x_apkg -= ogg_page_packets((ogg_page *)&x->x_apage);
        x->x_audiotime = vorbis_granule_time(&x->x_dsp_state, ogg_page_granulepos(&x->x_apage));
      }
  
      ogg_stream_clear(&x->x_statev);
      vorbis_block_clear(&x->x_vorbis_block);
      vorbis_dsp_clear(&x->x_dsp_state);
      vorbis_comment_clear(&x->x_vorbis_comment);
      vorbis_info_clear(&x->x_vorbis_info);
      ogg_stream_clear(&x->x_statet);
      theora_clear(&x->x_theora_state);
    }
}

    /* disconnect from an icecast server */
static void pdp_theonice_disconnect(t_pdp_theonice *x)
{
  int ret;

   pdp_theonice_shutdown_encoder( x );
   x->x_streaming = 0;

   if ( x->x_socketfd > 0 ) 
   {
     if ( close( x->x_socketfd ) < 0 )
     {
        post( "pdp_theonice~ : could not disconnect" );
        perror( "close" );
     }
     x->x_socketfd = -1;   
   }
}

static int pdp_theonice_write_headers(t_pdp_theonice *x)
{
  int ret;
  ogg_packet aheader, aheadercomm, aheadercode;

    if ( !x->x_einit )
    {
      post( "pdp_theonice~ : trying to write headers but encoder is not initialized." );
      return -1;
    }

    theora_encode_header(&x->x_theora_state, &x->x_ogg_vpacket);
    ogg_stream_packetin(&x->x_statet, &x->x_ogg_vpacket);
    if(ogg_stream_pageout(&x->x_statet, &x->x_ogg_page)!=1)
    {
      post( "pdp_theonice~ : ogg encoding error." );
      return -1;
    }
    if ( ( ret = send(x->x_socketfd, (void*)x->x_ogg_page.header, x->x_ogg_page.header_len, MSG_NOSIGNAL) ) < 0 )
    {
      post( "pdp_theonice~ : could not write headers (ret=%d).", ret );
      perror( "send" );
      pdp_theonice_disconnect(x);
      return -1;
    }
    if ( ( ret = send(x->x_socketfd, (void*)x->x_ogg_page.body, x->x_ogg_page.body_len, MSG_NOSIGNAL) ) < 0 )
    {
      post( "pdp_theonice~ : could not write headers (ret=%d).", ret );
      perror( "send" );
      pdp_theonice_disconnect(x);
      return -1;
    }

    theora_comment_init(&x->x_theora_comment);
    theora_comment_add_tag(&(x->x_theora_comment),"TITLE", x->x_title);
    theora_comment_add_tag(&(x->x_theora_comment),"ARTIST", x->x_artist);
    theora_comment_add_tag(&(x->x_theora_comment),"GENRE",x->x_genre);
    theora_comment_add_tag(&(x->x_theora_comment),"DESCRIPTION", x->x_description);
    theora_comment_add_tag(&(x->x_theora_comment),"LOCATION",x->x_url);
    theora_comment_add_tag(&(x->x_theora_comment),"PERFORMER",x->x_artist);
    theora_comment_add_tag(&(x->x_theora_comment),"COPYRIGHT",x->x_copyright);
    theora_comment_add_tag(&(x->x_theora_comment),"DATE",x->x_date);
    theora_comment_add_tag(&(x->x_theora_comment),"ENCODER","pdp_theonice~ v1.5");
    theora_encode_comment(&x->x_theora_comment, &x->x_ogg_vpacket);
    ogg_stream_packetin(&x->x_statet, &x->x_ogg_vpacket);
    theora_encode_tables(&x->x_theora_state, &x->x_ogg_vpacket);
    ogg_stream_packetin(&x->x_statet, &x->x_ogg_vpacket);

    vorbis_analysis_headerout(&x->x_dsp_state, &x->x_vorbis_comment, 
                              &aheader,&aheadercomm,&aheadercode);
    ogg_stream_packetin(&x->x_statev,&aheader); 

    if(ogg_stream_pageout(&x->x_statev, &x->x_ogg_page)!=1)
    {
      post( "pdp_theonice~ : ogg encoding error." );
      return -1;
    }
    if ( ( ret = send(x->x_socketfd, (void*)x->x_ogg_page.header, x->x_ogg_page.header_len, MSG_NOSIGNAL) ) < 0 )
    {
      post( "pdp_theonice~ : could not write headers (ret=%d).", ret );
      perror( "send" );
      pdp_theonice_disconnect(x);
      return -1;
    }
    if ( ( ret = send(x->x_socketfd, (void*)x->x_ogg_page.body, x->x_ogg_page.body_len, MSG_NOSIGNAL) ) < 0 )
    {
      post( "pdp_theonice~ : could not write headers (ret=%d).", ret );
      perror( "send" );
      pdp_theonice_disconnect(x);
      return -1;
    }

    // remaining vorbis header packets 
    ogg_stream_packetin(&x->x_statev, &aheadercomm);
    ogg_stream_packetin(&x->x_statev, &aheadercode);

    // flush all the headers
    while(1)
    {
      ret = ogg_stream_flush(&x->x_statet, &x->x_ogg_page);
      if(ret<0){
        post( "pdp_theonice~ : ogg encoding error." );
        return -1;
      }
      if(ret==0)break;
      if ( ( ret = send(x->x_socketfd, (void*)x->x_ogg_page.header, x->x_ogg_page.header_len, MSG_NOSIGNAL) ) < 0 )
      {
        post( "pdp_theonice~ : could not write headers (ret=%d).", ret );
        perror( "send" );
        pdp_theonice_disconnect(x);
        return -1;
      }
      if ( ( ret = send(x->x_socketfd, (void*)x->x_ogg_page.body, x->x_ogg_page.body_len, MSG_NOSIGNAL) ) < 0 )
      {
        post( "pdp_theonice~ : could not write headers (ret=%d).", ret );
        perror( "send" );
        pdp_theonice_disconnect(x);
        return -1;
      }
    }

    while(1)
    {
      ret = ogg_stream_flush(&x->x_statev, &x->x_ogg_page);
      if(ret<0){
        post( "pdp_theonice~ : ogg encoding error." );
        return -1;
      }
      if(ret==0)break;
      if ( ( ret = send(x->x_socketfd, (void*)x->x_ogg_page.header, x->x_ogg_page.header_len, MSG_NOSIGNAL) ) < 0 )
      {
        post( "pdp_theonice~ : could not write headers (ret=%d).", ret );
        perror( "send" );
        pdp_theonice_disconnect(x);
        return -1;
      }
      if ( ( ret = send(x->x_socketfd, (void*)x->x_ogg_page.body, x->x_ogg_page.body_len, MSG_NOSIGNAL) ) < 0 )
      {
        post( "pdp_theonice~ : could not write headers (ret=%d).", ret );
        perror( "send" );
        pdp_theonice_disconnect(x);
        return -1;
      }
    }

    return 0;
}

   /* start streaming */
static int pdp_theonice_start(t_pdp_theonice *x)
{
  time_t start_t;
  int ret;

    if ( gettimeofday(&x->x_tstart, NULL) == -1)
    {
       post("pdp_theonice~ : could not set start time" );
    }

    if ( gettimeofday(&x->x_tzero, NULL) == -1)
    {
      post("pdp_theonice~ : could get initial time" );
    }
 
    time( &start_t );
    strcpy( x->x_date, ctime( &start_t )); 
    post("pdp_theonice~ : initializing encoder...");
    pdp_theonice_init_encoder( x );
    post("pdp_theonice~ : writing headers...");
    if ( ( ret = pdp_theonice_write_headers( x ) ) < 0 )
    {
       return ret;
    }
    post("pdp_theonice~ : start streaming at %d frames/second", x->x_framerate);
    return 0;
}

    /* set password */
static void pdp_theonice_passwd(t_pdp_theonice *x, t_symbol *passwd)
{
    post("pdp_theonice~ : setting password to %s", passwd->s_name );
    x->x_passwd = passwd->s_name;
}

    /* set url */
static void pdp_theonice_url(t_pdp_theonice *x, t_symbol *url)
{
    post("pdp_theonice~ : setting location to %s", url->s_name );
    strcpy(x->x_url, url->s_name);
}

    /* set title */
static void pdp_theonice_title(t_pdp_theonice *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;

    strcpy( x->x_title, "" );
    for ( i=0; i<argc; i++ )
    {
       // typechecking
       if (argv[i].a_type == A_SYMBOL)
       {
          sprintf( x->x_title, "%s %s", x->x_title, argv[i].a_w.w_symbol->s_name );  
       } 
       if (argv[i].a_type == A_FLOAT)
       {
          sprintf( x->x_title, "%s %d", x->x_title, (int)argv[i].a_w.w_float );  
       } 
    }
    sprintf( x->x_title, "%s", x->x_title+1 );  
    post("pdp_theonice~ : setting title to %s", x->x_title );
}

    /* set artist */
static void pdp_theonice_artist(t_pdp_theonice *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;

    strcpy( x->x_artist, "" );
    for ( i=0; i<argc; i++ )
    {
       // typechecking
       if (argv[i].a_type == A_SYMBOL)
       {
          sprintf( x->x_artist, "%s %s", x->x_artist, argv[i].a_w.w_symbol->s_name );  
       } 
       if (argv[i].a_type == A_FLOAT)
       {
          sprintf( x->x_artist, "%s %d", x->x_artist, (int)argv[i].a_w.w_float );  
       } 
    }
    sprintf( x->x_artist, "%s", x->x_artist+1 );  
    post("pdp_theonice~ : setting artist to %s", x->x_artist );
}

    /* set description */
static void pdp_theonice_description(t_pdp_theonice *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;

    strcpy( x->x_description, "" );
    for ( i=0; i<argc; i++ )
    {
       // typechecking
       if (argv[i].a_type == A_SYMBOL)
       {
          sprintf( x->x_description, "%s %s", x->x_description, argv[i].a_w.w_symbol->s_name );  
       } 
       if (argv[i].a_type == A_FLOAT)
       {
          sprintf( x->x_description, "%s %d", x->x_description, (int)argv[i].a_w.w_float );  
       } 
    }
    sprintf( x->x_description, "%s", x->x_description+1 );  
    post("pdp_theonice~ : setting description to %s", x->x_description );
}

    /* set genre */
static void pdp_theonice_genre(t_pdp_theonice *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;

    strcpy( x->x_genre, "" );
    for ( i=0; i<argc; i++ )
    {
       // typechecking
       if (argv[i].a_type == A_SYMBOL)
       {
          sprintf( x->x_genre, "%s %s", x->x_genre, argv[i].a_w.w_symbol->s_name );  
       } 
       if (argv[i].a_type == A_FLOAT)
       {
          sprintf( x->x_genre, "%s %d", x->x_genre, (int)argv[i].a_w.w_float );  
       } 
    }
    sprintf( x->x_genre, "%s", x->x_genre+1 );  
    post("pdp_theonice~ : setting genre to %s", x->x_genre );
}

char *pdp_theonice_base64_encode(char *data)
{
  int len = strlen(data);
  char *out = t_getbytes(len*4/3 + 4);
  char *result = out;
  int chunk;

  while(len > 0) {
    chunk = (len >3)?3:len;
    *out++ = base64table[(*data & 0xFC)>>2];
    *out++ = base64table[((*data & 0x03)<<4) | ((*(data+1) & 0xF0) >> 4)];

    switch(chunk) {
      case 3:
        *out++ = base64table[((*(data+1) & 0x0F)<<2) | ((*(data+2) & 0xC0)>>6)];
        *out++ = base64table[(*(data+2)) & 0x3F];
        break;
      case 2:
        *out++ = base64table[((*(data+1) & 0x0F)<<2)];
        *out++ = '=';
        break;
      case 1:
        *out++ = '=';
        *out++ = '=';
        break;
     }
     data += chunk;
     len -= chunk;
  }
  *out = 0;

  return result;
}

static void sendsock( int sockfd, char *buf, size_t buflen )
{
   if ( send( sockfd, buf, buflen, MSG_NOSIGNAL ) != (int)buflen )
   {
      post( "pdp_theonice~ : could not send message to the server" );
      post( "pdp_theonice~ : message : %s", buf );
   }
} 

    /* connect to an icecast server */
static void *pdp_theonice_do_connect(void *tdata)
{
  char            buf[MAX_COMMENT_LENGTH]; /* buffer to hold commands sent to the server */
  char            *base64;                 /* buffer to hold 64bit encoded strings */
  char            resp[STRBUF_SIZE];
  unsigned int    len;
  fd_set          fdset;
  struct timeval  tv;
  int           sockfd, ret;
  struct          sockaddr_in sinfo;
  struct          hostent *hp;
  t_pdp_theonice  *x;

    x = (t_pdp_theonice *)tdata;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        error("pdp_theonice~: internal error while attempting to open socket");
        return NULL;
    }

        /* connect socket using hostname provided in command line */
    sinfo.sin_family = AF_INET;
    hp = gethostbyname(x->x_hostname);
    if (hp == 0)
    {
        post("pdp_theonice~: wrong server name or IP.");
        close(sockfd);
        return NULL;
    }
    memcpy((char *)&sinfo.sin_addr, (char *)hp->h_addr, hp->h_length);

        /* assign client port number */
    sinfo.sin_port = htons((unsigned short)x->x_port);

        /* try to connect.  */
    post("pdp_theonice~: connecting to port %d", x->x_port);
    if (connect(sockfd, (struct sockaddr *) &sinfo, sizeof (sinfo)) < 0)
    {
        error("pdp_theonice~: connection failed!\n");
        close(sockfd);
        return NULL;
    }

        /* check if we can read/write from/to the socket */
    FD_ZERO( &fdset);
    FD_SET( sockfd, &fdset);
    tv.tv_sec  = 0;            /* seconds */
    tv.tv_usec = 500;        /* microseconds */

    ret = select(sockfd + 1, &fdset, NULL, NULL, &tv);
    if(ret < 0)
    {
        error("pdp_theonice~: can not read from socket.");
        close(sockfd);
        return NULL;
    }
    ret = select(sockfd + 1, NULL, &fdset, NULL, &tv);
    if(ret < 0)
    {
        error("pdp_theonice~: can not write to socket.");
        close(sockfd);
        return NULL;
    }

       /* send the request, a string like: "SOURCE /<mountpoint> HTTP/1.0\r\n" */
    sprintf(buf, "SOURCE /%s", x->x_mountpoint);
    sendsock(sockfd, buf, strlen(buf));
    strcpy( buf, " HTTP/1.0\r\n");
    sendsock(sockfd, buf, strlen(buf));
       /* send basic authorization as base64 encoded string */
    sprintf(resp, "source:%s", x->x_passwd);
    len = strlen(resp);
    base64 = pdp_theonice_base64_encode(resp);
    sprintf(resp, "Authorization: Basic %s\r\n", base64);
    sendsock(sockfd, resp, strlen(resp));
    t_freebytes(base64, len*4/3 + 4);
      /* send application name */
    strcpy( buf, "User-Agent: pdp_theonice~");
    sendsock(sockfd, buf, strlen(buf));
      /* send content type: vorbis */
    strcpy( buf, "\r\nContent-Type: application/ogg");
    sendsock(sockfd, buf, strlen(buf));
      /* send the ice headers */
      /* name */
    sprintf( buf, "\r\nice-name: %s", x->x_title );
    sendsock(sockfd, buf, strlen(buf));
      /* url */
    sprintf( buf, "\r\nice-url: %s", x->x_url );
    sendsock(sockfd, buf, strlen(buf));
      /* genre */
    sprintf( buf, "\r\nice-genre: %s", x->x_genre );
    sendsock(sockfd, buf, strlen(buf));
      /* public */
    strcpy( buf, "\r\nice-public: 1" );
    sendsock(sockfd, buf, strlen(buf));
     /* bitrate */
    sprintf(buf, "\r\nice-audio-info: bitrate=%d", x->x_akbps);
    sendsock(sockfd, buf, strlen(buf));
      /* description */
    sprintf(buf, "\r\nice-description: %s", x->x_description);
    sendsock(sockfd, buf, strlen(buf));
      /* end of header: write an empty line */
    strcpy( buf, "\r\n\r\n");
    sendsock(sockfd, buf, strlen(buf));

      /* read the anticipated response: "OK" */
    len = recv(sockfd, resp, STRBUF_SIZE, MSG_NOSIGNAL);
    if ( strstr( resp, "OK" ) == NULL )
    {
        post("pdp_theonice~: login failed!");
        if ( len>0 ) post("pdp_theonice~: server answered : %s", resp);
        close(sockfd);
        return NULL;
    }
    post("pdp_theonice~: logged in to %s", x->x_hostname);

    post("pdp_theonice~: connecting child %d exiting....", x->x_connectchild);
    x->x_connectchild = 0;

    x->x_socketfd = sockfd;
    if ( ( ret = pdp_theonice_start( x ) ) < 0 )
    {
       x->x_socketfd = -1;
       return NULL;
    }
    else
    {
      x->x_streaming = 1;
      x->x_frameswritten = 0;
      x->x_videotime = 0.;
      x->x_audiotime = 0.;
      x->x_frameslate = 0;
      x->x_nbframes_dropped = 0;
      x->x_secondcount = 0;
      x->x_frames = 0;
    }

    return NULL;

}

    /* launch a connection thread */
static void pdp_theonice_connect(t_pdp_theonice *x, t_symbol *shost, t_symbol *smountpoint, t_floatarg fport)
{
  int ret=0;
  pthread_attr_t connect_child_attr;

   // check parameters
   if ( gethostbyname( shost->s_name ) == NULL )
   {
     post("pdp_theonice~ : it looks like your server is unknown here.." );
     return;
   }
   else
   {
     strcpy( x->x_hostname, shost->s_name);
   }

   if ( ( (int)fport < 0 ) || ( (int)fport > 65535 ) )
   {
     post("pdp_theonice~ : wrong port number." );
     return;
   }
   else
   {
     x->x_port = (int)fport; 
   }

   strcpy( x->x_mountpoint, smountpoint->s_name );
   sprintf( x->x_url, "http://%s:%d/%s", x->x_hostname, x->x_port, x->x_mountpoint );
   post("pdp_theonice~ : connecting to %s", x->x_url );

   if ( ( x->x_streaming ) || ( x->x_connectchild != 0 ) )
   {
     post("pdp_theonice~ : connection request but a connection is pending ... disconnecting" );
     pdp_theonice_disconnect(x);
   }

   // launch connection thread
   if ( pthread_attr_init( &connect_child_attr ) < 0 ) {
       post( "pdp_theonice~ : could not launch connection thread" );
       perror( "pthread_attr_init" );
       return;
   }
   if ( pthread_attr_setdetachstate( &connect_child_attr, PTHREAD_CREATE_DETACHED ) < 0 ) {
       post( "pdp_theonice~ : could not launch connection thread" );
       perror( "pthread_attr_setdetachstate" );
       return;
   }
   if ( pthread_create( &x->x_connectchild, &connect_child_attr, pdp_theonice_do_connect, x ) < 0 ) {
       post( "pdp_theonice~ : could not launch connection thread" );
       perror( "pthread_create" );
       return;
   }
   else
   {
       post( "pdp_theonice~ : connection thread %d launched", x->x_connectchild );
   }

   return;

}

   /* set video bitrate */
static void pdp_theonice_vbitrate(t_pdp_theonice *x, t_floatarg vbitrate )
{
  if ( ( (int) vbitrate < MIN_VIDEO_BITRATE ) || ( (int) vbitrate > MAX_VIDEO_BITRATE ) )
  {
     post( "pdp_theonice~ : wrong video bitrate %d : should be in [%d,%d] kbps", 
                            (int) vbitrate, MIN_VIDEO_BITRATE, MAX_VIDEO_BITRATE );
     return;
  }
  x->x_vkbps = (int) vbitrate;
}

   /* set audio bitrate */
static void pdp_theonice_abitrate(t_pdp_theonice *x, t_floatarg abitrate )
{
  if ( ( (int) abitrate < MIN_AUDIO_BITRATE ) || ( (int) abitrate > MAX_AUDIO_BITRATE ) )
  {
     post( "pdp_theonice~ : wrong audio bitrate %d : should be in [%d,%d] kbps", 
                            (int) abitrate, MIN_AUDIO_BITRATE, MAX_AUDIO_BITRATE );
     return;
  }
  x->x_akbps = (int) abitrate;
}

   /* set video quality */
static void pdp_theonice_vquality(t_pdp_theonice *x, t_floatarg vquality )
{
  if ( ( (int) vquality < MIN_VIDEO_QUALITY ) || ( (int) vquality > MAX_VIDEO_QUALITY ) )
  {
     post( "pdp_theonice~ : wrong video quality %d : should be in [%d,%d]", 
                            (int) vquality, MIN_VIDEO_QUALITY, MAX_VIDEO_QUALITY );
     return;
  }
  x->x_vquality = (int) vquality;
}

   /* set audio quality */
static void pdp_theonice_aquality(t_pdp_theonice *x, t_floatarg aquality )
{
  if ( ( (int) aquality < MIN_AUDIO_QUALITY ) || ( (int) aquality > MAX_AUDIO_QUALITY ) )
  {
     post( "pdp_theonice~ : wrong audio quality %d : should be in [%d,%d]", 
                            (int) aquality, MIN_AUDIO_QUALITY, MAX_AUDIO_QUALITY );
     return;
  }
  x->x_aquality = (int) aquality;
}

   /* set framerate */
static void pdp_theonice_framerate(t_pdp_theonice *x, t_floatarg fframerate )
{
  if ( ( (int) fframerate < MIN_FRAMERATE ) || ( (int) fframerate > MAX_FRAMERATE ) )
  {
     post( "pdp_theonice~ : wrong framerate %d : should be in [%d,%d]", 
                            (int) fframerate, MIN_FRAMERATE, MAX_FRAMERATE );
     return;
  }
  x->x_framerate = (int) fframerate;
}

static void pdp_theonice_send_video(t_pdp_theonice *x);
static void pdp_theonice_send_audio(t_pdp_theonice *x);

    /* store audio data in PCM format in a buffer for now */
static t_int *pdp_theonice_perform(t_int *w)
{
  t_float *in1   = (t_float *)(w[1]);       // left audio inlet
  t_float *in2   = (t_float *)(w[2]);       // right audio inlet
  t_pdp_theonice *x = (t_pdp_theonice *)(w[3]);
  int n = (int)(w[4]);                      // number of samples
  t_float fsample;
  int   isample, i;

   if ( x->x_streaming ) 
   {
    // just fills the buffer
    while (n--)
    {
       fsample=*(in1++);
       // if (fsample > 1.0) { fsample = 1.0; }
       // if (fsample < -1.0) { fsample = -1.0; }
       x->x_audio_buf[0][x->x_audioin_position]=fsample;
       fsample=*(in2++);
       // if (fsample > 1.0) { fsample = 1.0; }
       // if (fsample < -1.0) { fsample = -1.0; }
       x->x_audio_buf[1][x->x_audioin_position]=fsample;
       x->x_audioin_position=(x->x_audioin_position+1)%(MAX_AUDIO_PACKET_SIZE);
       if ( x->x_audioin_position > MAX_AUDIO_PACKET_SIZE-1 )
       {
          post( "pdp_theonice~ : audio x-run" );
       }
    }
  }

  if ( x->x_streaming != x->x_pstreaming ) 
  {
    x->x_pstreaming = x->x_streaming;
    outlet_float(x->x_outlet_streaming, x->x_streaming);
  }
  if ( x->x_frameswritten != x->x_pframeswritten ) 
  {
    x->x_pframeswritten = x->x_frameswritten;
    outlet_float(x->x_outlet_nbframes, x->x_frameswritten);
  }
  if ( x->x_nbframes_dropped != x->x_pnbframes_dropped ) 
  {
    x->x_pnbframes_dropped = x->x_nbframes_dropped;
    outlet_float(x->x_outlet_nbframes_dropped, x->x_nbframes_dropped);
  }
  if ( x->x_mframerate != x->x_pmframerate ) 
  {
    x->x_pmframerate = x->x_mframerate;
    outlet_float(x->x_outlet_framerate, x->x_mframerate);
  }
  if ( x->x_audiotime != x->x_paudiotime ) 
  {
    x->x_paudiotime = x->x_audiotime;
    if ( x->x_audiotime >= 0. ) outlet_float(x->x_outlet_atime, x->x_audiotime);
  }
  if ( x->x_videotime != x->x_pvideotime ) 
  {
    x->x_pvideotime = x->x_videotime;
    if ( x->x_videotime >= 0. ) outlet_float(x->x_outlet_vtime, x->x_videotime);
  }

  return (w+5);
}

static void pdp_theonice_dsp(t_pdp_theonice *x, t_signal **sp)
{
    dsp_add(pdp_theonice_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void pdp_theonice_process_yv12(t_pdp_theonice *x)
{
  t_pdp     *header = pdp_packet_header(x->x_packet0);
  unsigned char *data   = (unsigned char *)pdp_packet_data(x->x_packet0);
  int     i;
  int     px, py;
  unsigned char *pY, *pU, *pV;
  t_float   fframerate=0.0;

   if ( ( (int)(header->info.image.width) != x->x_vwidth ) || 
        ( (int)(header->info.image.height) != x->x_vheight ) )
   {
      post( "pdp_theonice~: reallocating ressources" );
      pdp_theonice_free_ressources( x );
      pdp_theonice_shutdown_encoder( x );
      x->x_vwidth = header->info.image.width;
      x->x_vheight = header->info.image.height;
      x->x_vsize = x->x_vwidth*x->x_vheight;
      x->x_tvwidth=((x->x_vwidth + 15) >>4)<<4;
      x->x_tvheight=((x->x_vheight + 15) >>4)<<4;
      pdp_theonice_allocate( x );
      if ( x->x_tzero.tv_sec != 0 )
      {
        pdp_theonice_init_encoder( x );
        pdp_theonice_write_headers( x );
      }
   }

   pY = x->x_yuvbuffer.y;
   memcpy( (void*)pY, (void*)&data[0], x->x_vsize );
   pV = x->x_yuvbuffer.v;
   memcpy( (void*)pV, (void*)&data[x->x_vsize], (x->x_vsize>>2) );
   pU = x->x_yuvbuffer.u;
   memcpy( (void*)pU, (void*)&data[x->x_vsize+(x->x_vsize>>2)], (x->x_vsize>>2) );
      
   x->x_frames++;

   pdp_theonice_send_video(x);
   pdp_theonice_send_audio(x);

}

static void pdp_theonice_send_video(t_pdp_theonice *x)
{
  struct timeval etime;
  int ttime, atime, ret;
  struct timeval tstream;

    if ( x->x_frameswritten == 0 )
    {
      if ( gettimeofday(&x->x_tprevstream, NULL) == -1)
      {
         post("pdp_theonice~ : couldn't set start time" );
      }
    }

    // check if it's time to emit a frame
    if ( gettimeofday(&etime, NULL) == -1)
    {
       post("pdp_theonice~ : could not read time" );
    }
    if ( etime.tv_sec != x->x_cursec )
    {
       x->x_cursec = etime.tv_sec;
       x->x_mframerate = x->x_secondcount;
       if ( x->x_mframerate < x->x_framerate )
       {
          x->x_frameslate += x->x_framerate-x->x_mframerate;
       }
       x->x_secondcount = 0;
    }
    ttime = ( ( x->x_frameswritten ) % x->x_framerate ) * ( 1000 / x->x_framerate );
    atime = ( etime.tv_usec / 1000 );
    // post("pdp_theonice~ : actual : %d, theoretical : %d", atime, ttime );
    if ( ( atime < ttime ) && ( x->x_frameslate <= 0 ) )
    {
       x->x_nbframes_dropped++;
       return;
    }

    if ( x->x_socketfd > 0 && x->x_streaming )
    {
      if ( ( ret = theora_encode_YUVin( &x->x_theora_state, &x->x_yuvbuffer ) ) != 0 )
      {
         post( "pdp_theonice~ : could not encode yuv image (ret=%d).", ret );
      }  
      else
      {
         x->x_frameswritten++;
         if ( x->x_frameslate > 0 ) x->x_frameslate--;
         x->x_secondcount++;

         // weld packets into the bitstream 
         while(theora_encode_packetout(&x->x_theora_state, 0, &x->x_ogg_vpacket))
         {
           ogg_stream_packetin( &x->x_statet, &x->x_ogg_vpacket);
           x->x_vpkg++;
           // post("pdp_theonice~ : video packets : %d", x->x_vpkg);
         }

         // post( "pdp_theonice~ : new (theora) ogg packet : bytes:%ld, bos:%ld, eos:%ld, no:%lld",
         //                        x->x_ogg_vpacket.bytes, x->x_ogg_vpacket.b_o_s, 
         //                        x->x_ogg_vpacket.e_o_s, x->x_ogg_vpacket.packetno );

         while(1)
         {
           ret = ogg_stream_flush(&x->x_statet, &x->x_vpage);
           if(ret<0){
              post( "pdp_theonice~ : ogg encoding error." );
              return;
           }
           if(ret==0)break;
           // post("pdp_theonice~ : writing video : header : %d, body : %d", x->x_vpage.header_len, x->x_vpage.body_len); 
           if ( ( ret = send(x->x_socketfd, (void*)x->x_vpage.header, x->x_vpage.header_len, MSG_NOSIGNAL) ) < 0 )
           {
             post( "pdp_theonice~ : could not write video packet (ret=%d).", ret );
             perror( "send" );
             pdp_theonice_disconnect(x);
             return;
           }
           if ( ( ret = send(x->x_socketfd, (void*)x->x_vpage.body, x->x_vpage.body_len, MSG_NOSIGNAL) ) < 0 )
           {
             post( "pdp_theonice~ : could not write video packet (ret=%d).", ret );
             perror( "send" );
             pdp_theonice_disconnect(x);
             return;
           }
           x->x_vpkg -= ogg_page_packets((ogg_page *)&x->x_vpage);
           x->x_videotime = theora_granule_time(&x->x_theora_state, ogg_page_granulepos(&x->x_vpage));
           // post( "pdp_theonice~ : sent %d bytes (%d video frames)", x->x_vpage.header_len+x->x_vpage.body_len, ogg_page_packets((ogg_page *)&x->x_vpage ) );
         }
      }

    }
}

static void pdp_theonice_send_audio(t_pdp_theonice *x)
{
  int    nbaudiosamples, nbsamples, ret, send_page;
  float  nbusecs;
  struct timeval tstream;

    // calculate the number of audio samples to output
    if ( gettimeofday(&tstream, NULL) == -1)
    {
       post("pdp_theonice~ : could get time" );
    }
    // calculate time diff in micro seconds
    // nbusecs = ( tstream.tv_usec - x->x_tprevstream.tv_usec ) + 
    //           ( tstream.tv_sec - x->x_tprevstream.tv_sec )*1000000;
    // nbaudiosamples = sys_getsr()*(nbusecs/1000000);

    memcpy( &x->x_tprevstream, &tstream, sizeof( struct timeval) );
      
    nbsamples = x->x_audioin_position;
    if( nbsamples <= 0 ) return;

    // audio is ahead of video, do not send audio
    if ( x->x_audiotime > x->x_videotime ) return;

    // post("pdp_theonice~ : writing audio : %d samples, audioin : %d", nbsamples, x->x_audioin_position ); 

    if ( x->x_socketfd > 0 && x->x_streaming )
    {
      x->x_vbuffer=vorbis_analysis_buffer( &x->x_dsp_state, nbsamples );
      if ( !x->x_vbuffer ) 
      {
         post( "pdp_theonice~ : error getting audio buffers" );
         return;
      }
 
      memcpy( (void*)&x->x_vbuffer[0][0], (void*)&x->x_audio_buf[0][0], nbsamples*sizeof( t_float ) );
      memcpy( (void*)&x->x_vbuffer[1][0], (void*)&x->x_audio_buf[1][0], nbsamples*sizeof( t_float ) );

      vorbis_analysis_wrote( &x->x_dsp_state, nbsamples);

      while(vorbis_analysis_blockout( &x->x_dsp_state, &x->x_vorbis_block )==1)
      {

        // analysis, assume we want to use bitrate management
        vorbis_analysis( &x->x_vorbis_block, NULL);
        vorbis_bitrate_addblock( &x->x_vorbis_block );

        // weld packets into the bitstream 
        while( vorbis_bitrate_flushpacket( &x->x_dsp_state, &x->x_ogg_apacket ) > 0 )
        {
          ogg_stream_packetin( &x->x_statev, &x->x_ogg_apacket);
          x->x_apkg++;
          // post( "pdp_theonice~ : audio packets :%d", x->x_apkg);
        }

      }

      while(1)
      {
        ret = ogg_stream_flush(&x->x_statev, &x->x_apage);
        if(ret<0){
           post( "pdp_theonice~ : ogg encoding error." );
           return;
        }
        if(ret==0)break;
        if ( ( ret = send(x->x_socketfd, (void*)x->x_apage.header, x->x_apage.header_len, MSG_NOSIGNAL) ) < 0 )
        {
          post( "pdp_theonice~ : could not write audio packet (ret=%d).", ret );
          perror( "send" );
          pdp_theonice_disconnect(x);
          return;
        }
        if ( ( ret = send(x->x_socketfd, (void*)x->x_apage.body, x->x_apage.body_len, MSG_NOSIGNAL) ) < 0 )
        {
          post( "pdp_theonice~ : could not write audio packet (ret=%d).", ret );
          perror( "send" );
          pdp_theonice_disconnect(x);
          return;
        }
        x->x_apkg -= ogg_page_packets((ogg_page *)&x->x_apage);
        x->x_audiotime = vorbis_granule_time(&x->x_dsp_state, ogg_page_granulepos(&x->x_apage));
        // post( "pdp_theonice~ : sent %d bytes (%d audio blocks)", x->x_apage.header_len+x->x_apage.body_len, ogg_page_packets((ogg_page *)&x->x_apage) );
      }

      memcpy( &x->x_audio_buf[0][0], &x->x_audio_buf[0][nbsamples], 
                     ( x->x_audioin_position-nbsamples ) * sizeof( t_float ) );
      memcpy( &x->x_audio_buf[1][0], &x->x_audio_buf[1][nbsamples], 
                     ( x->x_audioin_position-nbsamples ) * sizeof( t_float ) );
      x->x_audioin_position -= nbsamples;
       
    } 

    return;
}

static void pdp_theonice_killpacket(t_pdp_theonice *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;
}

static void pdp_theonice_process(t_pdp_theonice *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_theonice_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_BITMAP_YV12:
            pdp_queue_add(x, pdp_theonice_process_yv12, pdp_theonice_killpacket, &x->x_queue_id);
	    break;

	  default:
	    /* don't know the type, so dont pdp_theonice_process */
	    break;
	    
	}
    }
}

static void pdp_theonice_input_0(t_pdp_theonice *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
        x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("bitmap/yv12/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_theonice_process(x);
    }

}

static void pdp_theonice_free(t_pdp_theonice *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    // close video file if existing
    pdp_theonice_disconnect(x);
    
}

t_class *pdp_theonice_class;

void *pdp_theonice_new(void)
{
  int i;

    t_pdp_theonice *x = (t_pdp_theonice *)pd_new(pdp_theonice_class);
    inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym ("signal"), gensym ("signal"));
    x->x_outlet_streaming = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_nbframes = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_nbframes_dropped = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_framerate = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_atime = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_vtime = outlet_new(&x->x_obj, &s_float);

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

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

    x->x_socketfd = -1;
    x->x_passwd = "letmein";
    strcpy( x->x_title, "The Aesthetics Of Our Anger" );
    strcpy(x->x_url, "http://www.indymedia.org");
    strcpy( x->x_genre, "angrrry");
    strcpy( x->x_description, "Images From Infowar");
    strcpy( x->x_artist, "Recuerdos De Luchas");
    strcpy( x->x_copyright, "Creative Commons");
    x->x_public = 1;
    strcpy( x->x_mountpoint, "theora.ogg");
    strcpy( x->x_hostname, "localhost");
    x->x_port = 8000; 

    x->x_frames = 0;
    x->x_eos = 0;
    x->x_frameswritten = 0;
    x->x_pframeswritten = 0;
    x->x_frameslate = 0;
    x->x_mframerate = 0;
    x->x_pmframerate = 0;
    x->x_nbframes_dropped = 0;
    x->x_pnbframes_dropped = 0;
    x->x_apkg = 0;
    x->x_vpkg = 0;

    x->x_tzero.tv_sec = 0;
    x->x_tzero.tv_usec = 0;

    x->x_audiotime = -1.;
    x->x_paudiotime = -1.;
    x->x_videotime = -1.;
    x->x_pvideotime = -1.;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_theonice_tilde_setup(void)
{
    // post( pdp_theonice_version );
    pdp_theonice_class = class_new(gensym("pdp_theonice~"), (t_newmethod)pdp_theonice_new,
    	(t_method)pdp_theonice_free, sizeof(t_pdp_theonice), 0, A_NULL);

    CLASS_MAINSIGNALIN(pdp_theonice_class, t_pdp_theonice, x_f );
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_dsp, gensym("dsp"), 0);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_connect, gensym("connect"), A_SYMBOL, A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_disconnect, gensym("disconnect"), A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_abitrate, gensym("audiobitrate"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_vbitrate, gensym("videobitrate"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_aquality, gensym("audioquality"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_vquality, gensym("videoquality"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_framerate, gensym("framerate"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_passwd, gensym("passwd"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_title, gensym("title"), A_GIMME, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_artist, gensym("artist"), A_GIMME, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_url, gensym("url"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_description, gensym("description"), A_GIMME, A_NULL);
    class_addmethod(pdp_theonice_class, (t_method)pdp_theonice_genre, gensym("genre"), A_GIMME, A_NULL);

}

#ifdef __cplusplus
}
#endif
