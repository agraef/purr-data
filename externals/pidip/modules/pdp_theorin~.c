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

/*  This object is a theora file decoder object
 *  It uses libtheora and some of it code samples ( copyright xiph.org )
 *  Copyleft by Yves Degoyon ( ydegoyon@free.fr )                             
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
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include <theora/theora.h>  /* theora stuff */
#include <vorbis/codec.h>   /* vorbis stuff */

#define VIDEO_BUFFER_SIZE (1024*1024)
#define MAX_AUDIO_PACKET_SIZE (64 * 1024)
#define MIN_AUDIO_SIZE (128*1024)

#define DEFAULT_CHANNELS 1
#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240
#define DEFAULT_FRAME_RATE 25
#define END_OF_STREAM 20
#define MIN_PRIORITY 0
#define DEFAULT_PRIORITY 1
#define MAX_PRIORITY 20
#define NB_NOFRAMES_HIT 10

#define THEORA_NUM_HEADER_PACKETS 3

static char   *pdp_theorin_version = "pdp_theorin~: version 0.1, a theora file reader ( ydegoyon@free.fr).";

typedef struct pdp_theorin_struct
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
    t_outlet *x_outlet_nbframes;   // number of frames emitted
    t_outlet *x_outlet_framerate;  // real framerate
    t_outlet *x_outlet_endoffile;  // for signaling the end of the file
    t_outlet *x_outlet_filesize;   // for informing of the file size

    pthread_t x_decodechild;       // file decoding thread
    pthread_mutex_t x_audiolock;   // audio mutex
    pthread_mutex_t x_videolock;   // video mutex
    int x_usethread;             // flag to activate decoding in a thread
    int x_autoplay;              // flag to autoplay the file ( default = true )
    int x_nextimage;             // flag to play next image in manual mode
    int x_priority;              // priority of decoding thread

    char  *x_filename;
    FILE  *x_infile;        // file descriptor
    int x_decoding;       // decoding flag
    int x_theorainit;     // flag for indicating that theora is initialized
    int x_videoready;     // video ready flag
    int x_noframeshits;   // number of tries without getting a frame  
    int x_newpicture;     // new picture flag
    int x_newpictureready;// new picture ready flag
    int x_notpackets;     // number of theora packets decoded
    int x_novpackets;     // number of vorbis packets decoded
    int x_endoffile;      // end of the file reached
    int x_nbframes;       // number of frames emitted
    int x_framerate;      // framerate
    int x_samplerate;     // audio sample rate
    int x_audiochannels;  // audio channels
    int x_blocksize;      // audio block size
    int x_audioon;        // audio buffer filling flag
    int x_reading;        // file reading flag
    int x_cursec;         // current second
    int x_secondcount;    // number of frames received in the current second
    struct timeval x_starttime; // reading starting time

      /* vorbis/theora structures */
    ogg_sync_state   x_sync_state;     // ogg sync state
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

      /* audio structures */
    int x_audio;           // flag to activate the decoding of audio
    t_float x_audio_inl[4*MAX_AUDIO_PACKET_SIZE]; // left buffer for pd
    t_float x_audio_inr[4*MAX_AUDIO_PACKET_SIZE]; // right buffer for pd
    int x_audioin_position;// writing position for incoming audio
    t_float **x_pcm;         // buffer for vorbis decoding

} t_pdp_theorin;

static void pdp_theorin_priority(t_pdp_theorin *x, t_floatarg fpriority )
{
   if ( ( x->x_priority >= MIN_PRIORITY ) && ( x->x_priority <= MAX_PRIORITY ) )
   {
     x->x_priority = (int)fpriority;
   }
}

static void pdp_theorin_threadify(t_pdp_theorin *x, t_floatarg fusethread )
{
   if ( ( fusethread == 0 ) || ( fusethread == 1 ) ) 
   {
      x->x_usethread = (int)fusethread;
   }
}

static void pdp_theorin_audio(t_pdp_theorin *x, t_floatarg faudio )
{
   if ( ( faudio == 0. ) || ( faudio == 1. ) )
   {
      x->x_audio = (int)faudio;
   }
}

static void pdp_theorin_autoplay(t_pdp_theorin *x, t_floatarg fautoplay )
{
   if ( ( fautoplay == 0. ) || ( fautoplay == 1. ) )
   {
      x->x_autoplay = (int)fautoplay;
   }
}

static void pdp_theorin_bang(t_pdp_theorin *x)
{
   if ( x->x_nextimage == 1 )
   {
      // post( "pdp_theorin~ : banging too fast, previous image is not decoded yet... ignored" );
      return;
   }
   x->x_nextimage = 1;
}

static int pdp_theorin_get_buffer_from_file(FILE *in, ogg_sync_state *oy)
{
  char *buffer;
  int bytes;

    buffer=ogg_sync_buffer(oy,4096);
    bytes=fread(buffer,1,4096,in);
    ogg_sync_wrote(oy,bytes);
    return(bytes);
}

static int pdp_theorin_queue_page(t_pdp_theorin *x)
{
  if(x->x_notpackets) ogg_stream_pagein(&x->x_statet, &x->x_ogg_page);
  if(x->x_novpackets) ogg_stream_pagein(&x->x_statev, &x->x_ogg_page); 
  return 0;
}

static int pdp_theorin_decode_packet(t_pdp_theorin *x)
{
  int ret, count, maxsamples, samples, si=0, sj=0;
  struct timespec mwait;
  struct timeval ctime;
  long long tplaying;
  long long ttheoretical;
  unsigned char *pY, *pU, *pV; 
  unsigned char *psY, *psU, *psV; 
  t_float **lpcm;
  int px, py;

   // post( "pdp_theorin~ : decode packet" );

   if ( !x->x_reading ) return -1;

   while ( x->x_novpackets )
   {
     /* if there's pending, decoded audio, grab it */
     x->x_pcm = NULL;
     if((ret=vorbis_synthesis_pcmout(&x->x_dsp_state, &x->x_pcm))>0)
     {
       if (x->x_audio) 
       {
         if ( pthread_mutex_lock( &x->x_audiolock ) < 0 )
         {
           post( "pdp_theorin~ : unable to lock audio mutex" ); 
           perror( "pthread_mutex_lock" );
         }  
         maxsamples=(3*MAX_AUDIO_PACKET_SIZE-x->x_audioin_position);
         samples=(ret<maxsamples)?ret:maxsamples;

         memcpy( (void*)&x->x_audio_inl[x->x_audioin_position], x->x_pcm[0], samples*sizeof(t_float) );
         memcpy( (void*)&x->x_audio_inr[x->x_audioin_position], x->x_pcm[1], samples*sizeof(t_float) );
         x->x_audioin_position = ( x->x_audioin_position + samples ) % (3*MAX_AUDIO_PACKET_SIZE);

         if ( ( x->x_audioin_position > MIN_AUDIO_SIZE ) && (!x->x_audioon) )
         {
           x->x_audioon = 1;
           // post( "pdp_theorin~ : audio on (audioin=%d)", x->x_audioin_position );
         }
         // tell vorbis how many samples were read
         // post( "pdp_theorin~ : got %d audio samples (audioin=%d)", samples, x->x_audioin_position );
         vorbis_synthesis_read(&x->x_dsp_state, samples);
         if((ret=vorbis_synthesis_lapout(&x->x_dsp_state, &x->x_pcm))>0)
         {
         //   post( "pdp_theorin~ : supplemental samples (nb=%d)", ret );
         }
         if ( pthread_mutex_unlock( &x->x_audiolock ) < 0 )
         {
           post( "pdp_theorin~ : unable to audio unlock mutex" ); 
           perror( "pthread_mutex_unlock" );
         }  
       }
       else
       {
         break;
       }
     }
     else
     {
       // no pending audio: is there a pending packet to decode? 
       if( ogg_stream_packetout(&x->x_statev, &x->x_ogg_packet)>0 )
       {
         if(vorbis_synthesis(&x->x_vorbis_block, &x->x_ogg_packet)==0)
         {
           vorbis_synthesis_blockin(&x->x_dsp_state, &x->x_vorbis_block);
         }
       }
       else   /* we need more data; suck in another page */
       {
         break;
       }
     }
   }

   if ( !x->x_newpictureready && !x->x_newpicture )
   {
     while(x->x_notpackets && !x->x_videoready)
     {
       // theora is one in, one out...
       if(ogg_stream_packetout(&x->x_statet, &x->x_ogg_packet)>0)
       {
         theora_decode_packetin(&x->x_theora_state, &x->x_ogg_packet);
         // post( "pdp_theorin~ : got one video frame" );
         x->x_videoready=1;
         x->x_noframeshits=0;  
       }
       else
       {
         // post( "pdp_theorin~ : no more video frame (frames=%d)", x->x_nbframes );
         x->x_noframeshits++;  
         if ( x->x_nbframes > 0 && ( x->x_noframeshits > NB_NOFRAMES_HIT ) ) 
         {
             x->x_endoffile = 1;
         }
         break;
       }
     }

     if ( x->x_videoready )
     {
       if ( pthread_mutex_lock( &x->x_videolock ) < 0 )
       {
         post( "pdp_theorin~ : unable to lock video mutex" ); 
         perror( "pthread_mutex_lock" );
       }  
       theora_decode_YUVout(&x->x_theora_state, &x->x_yuvbuffer); 

       // create a new pdp packet from PIX_FMT_YUV420P image format
       x->x_vwidth = x->x_yuvbuffer.y_width;
       x->x_vheight = x->x_yuvbuffer.y_height;
       x->x_vsize = x->x_vwidth*x->x_vheight;
       x->x_packet0 = pdp_packet_new_bitmap_yv12( x->x_vwidth, x->x_vheight );
       // post( "pdp_theorin~ : allocated packet %d", x->x_packet0 );
       x->x_header = pdp_packet_header(x->x_packet0);
       x->x_data = (unsigned char*) pdp_packet_data(x->x_packet0);

       x->x_header->info.image.encoding = PDP_BITMAP_YV12;
       x->x_header->info.image.width = x->x_vwidth;
       x->x_header->info.image.height = x->x_vheight;

       pY = x->x_data;
       pV = x->x_data+x->x_vsize;
       pU = x->x_data+x->x_vsize+(x->x_vsize>>2);

       psY = x->x_yuvbuffer.y;
       psU = x->x_yuvbuffer.u;
       psV = x->x_yuvbuffer.v;

       for ( py=0; py<x->x_vheight; py++)
       {
          memcpy( (void*)pY, (void*)psY, x->x_vwidth );
          pY += x->x_vwidth;
          psY += x->x_yuvbuffer.y_stride;
          if ( py%2==0 )
          {
            memcpy( (void*)pU, (void*)psU, (x->x_vwidth>>1) );
            memcpy( (void*)pV, (void*)psV, (x->x_vwidth>>1) );
            pU += (x->x_vwidth>>1);
            pV += (x->x_vwidth>>1);
            psU += x->x_yuvbuffer.uv_stride;
            psV += x->x_yuvbuffer.uv_stride;
          }
       }
       if ( !x->x_autoplay )
       {
         x->x_newpicture = 1;
       }
       else
       {
         x->x_newpictureready = 1;
       }
       if ( pthread_mutex_unlock( &x->x_videolock ) < 0 )
       {
         post( "pdp_theorin~ : unable to unlock video mutex" ); 
         perror( "pthread_mutex_unlock" );
       }  
     }
   }

   if ( x->x_newpictureready )
   {
     if ( gettimeofday(&ctime, NULL) == -1)
     {
       post("pdp_theorin~ : could not read time" );
     }

     tplaying = ( ctime.tv_sec-x->x_starttime.tv_sec )*1000 + 
                ( ctime.tv_usec-x->x_starttime.tv_usec )/1000;
     ttheoretical = ((x->x_nbframes)*1000 )/x->x_framerate;
     // post( "pdp_theorin~ : %d playing since : %lldms ( theory : %lldms )", 
     //        x->x_nbframes, tplaying, ttheoretical );

     if ( ttheoretical <= tplaying ) 
     {
       x->x_newpicture = 1;
       x->x_newpictureready = 0;
     }

   }

   // read more data in
   if( ( x->x_audioin_position < MIN_AUDIO_SIZE ) || ( !x->x_newpicture && !x->x_newpictureready ) )
   {
     ret=pdp_theorin_get_buffer_from_file(x->x_infile, &x->x_sync_state);
     // post( "pdp_theorin~ : read %d bytes from file", ret );
     while( ogg_sync_pageout(&x->x_sync_state, &x->x_ogg_page)>0 )
     {
       pdp_theorin_queue_page(x);
     }
   }

   x->x_videoready = 0;

   return 0;

}

static void *pdp_decode_file(void *tdata)
{
  t_pdp_theorin *x = (t_pdp_theorin*)tdata;
  struct sched_param schedprio;
  int pmin, pmax, p1;
  struct timespec twait;

    twait.tv_sec = 0; 
    twait.tv_nsec = 10000000; // 10 ms
 
    schedprio.sched_priority = sched_get_priority_min(SCHED_FIFO) + x->x_priority;
#ifdef __gnu_linux__
    if ( sched_setscheduler(0, SCHED_FIFO, &schedprio) == -1)
    {
        post("pdp_theorin~ : couldn't set priority for decoding thread.");
    }
#endif
    while ( x->x_decodechild )
    {
      if ( ( x->x_reading ) && ( ( x->x_autoplay ) || ( x->x_nextimage == 1 ) ) )
      {
        x->x_decoding = 1;
     
        // decode incoming packets
        if ( x->x_reading ) pdp_theorin_decode_packet( x );
        nanosleep( &twait, NULL ); 
        x->x_nextimage = -1;
      }
      else
      {
        x->x_decoding = 0;
        nanosleep( &twait, NULL ); // nothing to do, just wait
      }
    }

    x->x_decoding = 0;
    post("pdp_theorin~ : decoding child exiting." );
    return NULL;
}

static void pdp_theorin_close(t_pdp_theorin *x)
{
 int ret, i, count=0;
 struct timespec twait;

   twait.tv_sec = 0; 
   twait.tv_nsec = 100000000; // 100 ms

   if ( x->x_infile == NULL )
   {
     post("pdp_theorin~ : close request but no file is played ... ignored" );
     return;
   }

   if ( x->x_reading )
   {
     x->x_newpicture = 0;
     x->x_newpictureready = 0;
     x->x_reading = 0;
     // post("pdp_theorin~ : waiting end of decoding..." );
     while ( x->x_decoding ) nanosleep( &twait, NULL );

     if ( fclose( x->x_infile ) < 0 )
     {
        post( "pdp_theorin~ : could not close input file" );
        perror( "fclose" );
     }
     x->x_infile = NULL;

     if ( x->x_notpackets > 0 )
     {
       ogg_stream_clear(&x->x_statet);
       theora_clear(&x->x_theora_state);
       theora_comment_clear(&x->x_theora_comment);
       theora_info_clear(&x->x_theora_info);
     }

     if ( x->x_novpackets > 0 )
     {
       ogg_stream_clear(&x->x_statev);
       vorbis_block_clear(&x->x_vorbis_block);
       vorbis_dsp_clear(&x->x_dsp_state);
       vorbis_comment_clear(&x->x_vorbis_comment);
       vorbis_info_clear(&x->x_vorbis_info);
     }

   }

   x->x_notpackets = 0;
   x->x_novpackets = 0;
   x->x_nbframes = 0;
   x->x_decoding = 0;
   x->x_theorainit = 0;

   x->x_videoready = 0;
   x->x_newpicture = 0;

   x->x_nbframes = 0;
   outlet_float( x->x_outlet_nbframes, x->x_nbframes );
   x->x_framerate = 0;
   outlet_float( x->x_outlet_framerate, x->x_framerate );
}

static void pdp_theorin_open(t_pdp_theorin *x, t_symbol *s)
{
  int ret, i;
  pthread_attr_t decode_child_attr;
  ogg_stream_state o_tempstate;
  struct stat fileinfos;

   if ( x->x_infile != NULL )
   {
     // post("pdp_theorin~ : open request but a file is open ... closing" );
     pdp_theorin_close(x);
   }

   if ( x->x_filename ) free( x->x_filename );
   x->x_filename = (char*) malloc( strlen( s->s_name ) + 1 );
   strcpy( x->x_filename, s->s_name );
   // post( "pdp_theorin~ : opening file : %s", x->x_filename );
   x->x_audio = 1;

   if ( ( x->x_infile = fopen(x->x_filename,"r") ) == NULL )
   {
      post( "pdp_theorin~ : unable to open file >%s<", x->x_filename );
      return;
   }

   ogg_sync_init(&x->x_sync_state);

   // init supporting Vorbis structures needed in header parsing
   vorbis_info_init(&x->x_vorbis_info);
   vorbis_comment_init(&x->x_vorbis_comment);

   // init supporting Theora structures needed in header parsing
   theora_comment_init(&x->x_theora_comment);
   theora_info_init(&x->x_theora_info);

   // parse headers
   while( !x->x_theorainit )
   {
    if ( ( ret = pdp_theorin_get_buffer_from_file(x->x_infile, &x->x_sync_state) )==0) break;

    while( ogg_sync_pageout(&x->x_sync_state, &x->x_ogg_page) > 0 )
    {
      /* is this a mandated initial header? If not, stop parsing */
      if(!ogg_page_bos(&x->x_ogg_page))
      {
        pdp_theorin_queue_page(x);
        x->x_theorainit = 1;
        break;
      }

      ogg_stream_init(&o_tempstate, ogg_page_serialno(&x->x_ogg_page));
      ogg_stream_pagein(&o_tempstate, &x->x_ogg_page);
      ogg_stream_packetout(&o_tempstate, &x->x_ogg_packet);

      /* identify the codec: try theora */
      if(!x->x_notpackets && 
          theora_decode_header(&x->x_theora_info, &x->x_theora_comment, &x->x_ogg_packet)>=0)
      {
        /* it is theora */
        memcpy(&x->x_statet, &o_tempstate, sizeof(o_tempstate));
        x->x_notpackets=1;
      }else 
      if(!x->x_novpackets && 
         vorbis_synthesis_headerin(&x->x_vorbis_info, &x->x_vorbis_comment, &x->x_ogg_packet)>=0){
        memcpy(&x->x_statev, &o_tempstate, sizeof(o_tempstate));
        x->x_novpackets=1;
      }else{
        /* whatever it is, we don't care about it */
        ogg_stream_clear(&o_tempstate);
      }
    }
   }

   // we're expecting more header packets.
   while( (x->x_notpackets && x->x_notpackets<3) || (x->x_novpackets && x->x_novpackets<3) )
   {
     // look for further theora headers
     while(x->x_notpackets && (x->x_notpackets<3) && 
           (ret=ogg_stream_packetout(&x->x_statet, &x->x_ogg_packet)))
     {
       if( ret<0 )
       {
         post("pdp_theorin~ : error parsing theora stream headers\n");
         x->x_theorainit = 0;
         return;
       }
       if( theora_decode_header(&x->x_theora_info, &x->x_theora_comment, &x->x_ogg_packet) )
       {
         post("pdp_theorin~ : error parsing theora stream headers\n");
         x->x_theorainit = 0;
         return;
       }
       x->x_notpackets++;
       if(x->x_notpackets==3) break;
     }
 
     /* look for more vorbis header packets */
     while(x->x_novpackets && (x->x_novpackets<3) && 
           (ret=ogg_stream_packetout(&x->x_statev, &x->x_ogg_packet)))
     {
       if(ret<0)
       {
         post("pdp_theorin~ : error parsing theora stream headers\n");
         x->x_theorainit = 0;
         return;
       }
       if( vorbis_synthesis_headerin(&x->x_vorbis_info, &x->x_vorbis_comment, &x->x_ogg_packet) )
       {
         post("pdp_theorin~ : error parsing theora stream headers\n");
         x->x_theorainit = 0;
         return;
       }
       x->x_novpackets++;
       if(x->x_novpackets==3) break;
     }
 
     if(ogg_sync_pageout(&x->x_sync_state, &x->x_ogg_page)>0)
     {
       pdp_theorin_queue_page(x); 
     }
     else
     {
       if( (ret=pdp_theorin_get_buffer_from_file(x->x_infile, &x->x_sync_state))==0 )
       {
         post("pdp_theorin~ : end of file while parsing headers\n");
         x->x_theorainit = 0;
         return;
       }
     }
   }
   // post( "pdp_theorin~ : parsed headers ok." );

   // initialize decoders
   if( x->x_notpackets )
   {
     theora_decode_init(&x->x_theora_state, &x->x_theora_info);
     x->x_framerate = (int)x->x_theora_info.fps_numerator/x->x_theora_info.fps_denominator;
     // post("pdp_theorin~ : stream %x is theora %dx%d %d fps video.",
     //                      x->x_statet.serialno,
     //                      x->x_theora_info.width,x->x_theora_info.height,
     //                      x->x_framerate);
     if(x->x_theora_info.width!=x->x_theora_info.frame_width || 
        x->x_theora_info.height!=x->x_theora_info.frame_height)
     {
       post("pdp_theorin~ : frame content is %dx%d with offset (%d,%d).",
                          x->x_theora_info.frame_width, x->x_theora_info.frame_height, 
                          x->x_theora_info.offset_x, x->x_theora_info.offset_y);
     }
     x->x_vwidth = x->x_theora_info.width;
     x->x_vheight = x->x_theora_info.height;
     x->x_vsize = x->x_vwidth*x->x_vheight;

     switch(x->x_theora_info.colorspace)
     {
      case OC_CS_UNSPECIFIED:
        /* nothing to report */
        break;;
      case OC_CS_ITU_REC_470M:
        // post("pdp_theorin~ : encoder specified ITU Rec 470M (NTSC) color.");
        break;;
      case OC_CS_ITU_REC_470BG:
        // post("pdp_theorin~ : encoder specified ITU Rec 470BG (PAL) color.");
        break;;
      default:
        post("pdp_theorin~ : warning: encoder specified unknown colorspace (%d).", 
                             x->x_theora_info.colorspace);
        break;;
     }
   }
   else
   {
     // tear down the partial theora setup 
     theora_info_clear(&x->x_theora_info);
     theora_comment_clear(&x->x_theora_comment);
     post("pdp_theorin~ : could not initialize theora decoder.");
     x->x_theorainit = 0;
     return;
   }

   if( x->x_novpackets )
   {
     vorbis_synthesis_init(&x->x_dsp_state, &x->x_vorbis_info);
     vorbis_block_init(&x->x_dsp_state, &x->x_vorbis_block); 
     x->x_audiochannels = x->x_vorbis_info.channels;
     x->x_samplerate = x->x_vorbis_info.rate;
     // post("pdp_theorin~ : ogg logical stream %x is vorbis %d channel %d Hz audio.",
     //                      x->x_statev.serialno,
     //                      x->x_audiochannels, x->x_samplerate);
   }
   else
   {
     /* tear down the partial vorbis setup */
     vorbis_info_clear(&x->x_vorbis_info);
     vorbis_comment_clear(&x->x_vorbis_comment);
     post("pdp_theorin~ : could not initialize vorbis decoder.");
     // x->x_theorainit = 0;
     // return;
     x->x_audio = 0;
   }
   // everything seems to be ready
   x->x_reading = 1;

   if ( x->x_usethread && ( x->x_decodechild == 0 ) )
   {
     x->x_decodechild = 1; // trick & treets
     // launch decoding thread
     if ( pthread_attr_init( &decode_child_attr ) < 0 ) 
     {
        post( "pdp_theorin~ : could not launch decoding thread" );
        perror( "pthread_attr_init" );
        pthread_exit(NULL);
     }
     if ( pthread_create( &x->x_decodechild, &decode_child_attr, pdp_decode_file, x ) < 0 ) 
     {
        post( "pdp_theorin~ : could not launch decoding thread" );
        perror( "pthread_create" );
        pthread_exit(NULL);
     }
     else
     {
        // post( "pdp_theorin~ : decoding thread %d launched", (int)x->x_decodechild );
     }
   }

   if ( stat( x->x_filename, &fileinfos ) < 0 )
   {
     post("pdp_theorin~ : couldn't get file informations" );
     perror( "stat" );
   }
   else
   {
     outlet_float( x->x_outlet_filesize, (fileinfos.st_size)/1024 );
   }

   if ( gettimeofday(&x->x_starttime, NULL) == -1)
   {
     post("pdp_theorin~ : could not set start time" );
   }

   x->x_nbframes = 0;
   x->x_endoffile = -1;

   return;
}

static void pdp_theorin_frame_cold(t_pdp_theorin *x, t_floatarg kbytes)
{
    int pos = (int)kbytes;
    int ret;

    if (x->x_infile==NULL) return;

    pdp_theorin_open(x, gensym(x->x_filename));

    // it's very approximative, we're are positioning the file on the number of requested kilobytes
    if ( fseek( x->x_infile, pos*1024, SEEK_SET ) < 0 )
    {
       post( "pdp_theorin~ : could not set file at that position (%d kilobytes)", pos );
       perror( "fseek" );
       return;
    }
    // post( "pdp_theorin~ : file seeked at %d kilobytes", pos );
} 

    /* decode the audio buffer */
static t_int *pdp_theorin_perform(t_int *w)
{
  t_float *out1   = (t_float *)(w[1]);       // left audio inlet
  t_float *out2   = (t_float *)(w[2]);       // right audio inlet 
  t_pdp_theorin *x = (t_pdp_theorin *)(w[3]);
  int n = (int)(w[4]);                      // number of samples 
  struct timeval etime;
  int sn;

    // decode a packet if not in thread mode
    if ( !x->x_usethread && x->x_reading )
    {
      pdp_theorin_decode_packet( x );
    }

    x->x_blocksize = n;

    // just read the buffer
    if ( x->x_audioon && x->x_reading )
    {
      if ( pthread_mutex_lock( &x->x_audiolock ) < 0 )
      {
        post( "pdp_theorin~ : unable to lock audio mutex" ); 
        perror( "pthread_mutex_lock" );
      }  
      sn=0;
      while (n--) 
      {
        *(out1)=x->x_audio_inl[ sn ];
        if ( x->x_audiochannels == 1 )
        {
          *(out2) = *(out1);
          sn++;
        }
        if ( x->x_audiochannels == 2 )
        {
          *(out2)=x->x_audio_inr[ sn++ ];
        }
        out1++;
        out2++;
      }
      memcpy( &x->x_audio_inl[0], &x->x_audio_inl[sn], (x->x_audioin_position-sn)*sizeof(t_float) );
      memcpy( &x->x_audio_inr[0], &x->x_audio_inr[sn], (x->x_audioin_position-sn)*sizeof(t_float) );
      x->x_audioin_position-=sn;
      // post( "pdp_theorin~ : audio in position : %d", x->x_audioin_position );
      if ( x->x_audioin_position <= sn )
      {
         x->x_audioon = 0;
         // post( "pdp_theorin~ : audio off ( audioin : %d, channels=%d )", 
         //       x->x_audioin_position, x->x_audiochannels );
      }
      if ( pthread_mutex_unlock( &x->x_audiolock ) < 0 )
      {
        post( "pdp_theorin~ : unable to unlock audio mutex" ); 
        perror( "pthread_mutex_unlock" );
      }  
    }
    else
    {
      // post("pdp_theorin~ : no available audio" );
      while (n--)
      {
        *(out1++) = 0.0;
        *(out2++) = 0.0;
      }
    }	

    if ( !x->x_reading ) return (w+5);

    // check if the framerate has been exceeded
    if ( gettimeofday(&etime, NULL) == -1)
    {
       post("pdp_theorin~ : could not read time" );
    }
    if ( etime.tv_sec != x->x_cursec )
    {
       x->x_cursec = etime.tv_sec;
       if (x->x_reading) outlet_float( x->x_outlet_framerate, x->x_secondcount );
       x->x_secondcount = 0;
    }

    // output image if there's a new one decoded
    if ( x->x_newpicture )
    {
       if ( pthread_mutex_lock( &x->x_videolock ) < 0 )
       {
         post( "pdp_theorin~ : unable to lock video mutex" ); 
         perror( "pthread_mutex_lock" );
       }  
       pdp_packet_pass_if_valid(x->x_pdp_out, &x->x_packet0);
       x->x_newpicture = 0;

       // update streaming status
       x->x_nbframes++;
       x->x_secondcount++;
       // post( "pdp_theorin~ : frame #%d", x->x_nbframes ); 
       outlet_float( x->x_outlet_nbframes, x->x_nbframes );
       if ( pthread_mutex_unlock( &x->x_videolock ) < 0 )
       {
         post( "pdp_theorin~ : unable to unlock video mutex" ); 
         perror( "pthread_mutex_unlock" );
       }  
    }
    if ( x->x_endoffile == 1 ) // only once
    {
      outlet_float( x->x_outlet_endoffile, x->x_endoffile );
    }
    if ( x->x_endoffile == -1 ) // reset
    {
      x->x_endoffile = 0;
      outlet_float( x->x_outlet_endoffile, x->x_endoffile );
    }

    return (w+5);
}

static void pdp_theorin_dsp(t_pdp_theorin *x, t_signal **sp)
{
    dsp_add(pdp_theorin_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void pdp_theorin_free(t_pdp_theorin *x)
{
  int i;

    if ( x->x_decodechild )
    {
      x->x_decodechild = 0;
    }

    if ( x->x_reading )
    {
       pdp_theorin_close(x);
    }
    
    if ( pthread_mutex_destroy( &x->x_audiolock ) < 0 )
    {
      post( "pdp_theorin~ : unable to destroy audio mutex" );
      perror( "pthread_mutex_destroy" );
    }
    if ( pthread_mutex_destroy( &x->x_videolock ) < 0 )
    {
      post( "pdp_theorin~ : unable to destroy video mutex" );
      perror( "pthread_mutex_destroy" );
    }

    // post( "pdp_theorin~ : freeing object" );
}

t_class *pdp_theorin_class;

void *pdp_theorin_new(void)
{
    int i;

    t_pdp_theorin *x = (t_pdp_theorin *)pd_new(pdp_theorin_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("frame_cold"));

    x->x_pdp_out = outlet_new(&x->x_obj, &s_anything);

    x->x_outlet_left = outlet_new(&x->x_obj, &s_signal);
    x->x_outlet_right = outlet_new(&x->x_obj, &s_signal);

    x->x_outlet_nbframes = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_framerate = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_endoffile = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_filesize = outlet_new(&x->x_obj, &s_float);

    x->x_packet0 = -1;
    x->x_decodechild = 0;
    if ( pthread_mutex_init( &x->x_audiolock, NULL ) < 0 )
    {
       post( "pdp_theorin~ : unable to initialize audio mutex" );
       perror( "pthread_mutex_init" );
       return NULL;
    }
    if ( pthread_mutex_init( &x->x_videolock, NULL ) < 0 )
    {
       post( "pdp_theorin~ : unable to initialize video mutex" );
       perror( "pthread_mutex_init" );
       return NULL;
    }
    x->x_decoding = 0;
    x->x_theorainit = 0;
    x->x_usethread = 1;
    x->x_priority = DEFAULT_PRIORITY;
    x->x_framerate = DEFAULT_FRAME_RATE;
    x->x_nbframes = 0;
    x->x_samplerate = 0;
    x->x_audio = 1;
    x->x_audiochannels = 0;
    x->x_audioin_position = 0;
    x->x_videoready = 0;
    x->x_newpicture = 0;
    x->x_newpictureready = 0;
    x->x_endoffile = 0;
    x->x_notpackets = 0;
    x->x_novpackets = 0;
    x->x_blocksize = MIN_AUDIO_SIZE;
    x->x_autoplay = 1;
    x->x_nextimage = 0;
    x->x_infile = NULL;
    x->x_reading = 0;

    memset( &x->x_audio_inl[0], 0x0, 4*MAX_AUDIO_PACKET_SIZE*sizeof(t_float) );
    memset( &x->x_audio_inr[0], 0x0, 4*MAX_AUDIO_PACKET_SIZE*sizeof(t_float) );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_theorin_tilde_setup(void)
{
    // post( pdp_theorin_version );
    pdp_theorin_class = class_new(gensym("pdp_theorin~"), (t_newmethod)pdp_theorin_new,
    	(t_method)pdp_theorin_free, sizeof(t_pdp_theorin), 0, A_NULL);

    class_addmethod(pdp_theorin_class, (t_method)pdp_theorin_dsp, gensym("dsp"), A_NULL);
    class_addmethod(pdp_theorin_class, (t_method)pdp_theorin_open, gensym("open"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_theorin_class, (t_method)pdp_theorin_close, gensym("close"), A_NULL);
    class_addmethod(pdp_theorin_class, (t_method)pdp_theorin_priority, gensym("priority"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theorin_class, (t_method)pdp_theorin_audio, gensym("audio"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theorin_class, (t_method)pdp_theorin_autoplay, gensym("autoplay"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theorin_class, (t_method)pdp_theorin_threadify, gensym("thread"), A_FLOAT, A_NULL);
    class_addmethod(pdp_theorin_class, (t_method)pdp_theorin_bang, gensym("bang"), A_NULL);
    class_addmethod(pdp_theorin_class, (t_method)pdp_theorin_frame_cold, gensym("frame_cold"), A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
