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

/*  This object is a theora stream reader from icecast2
 *  It uses libtheora and some of it code samples ( copyright xiph.org )
 *  Written by Yves Degoyon ( ydegoyon@free.fr )                             
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#include <theora/theora.h>  /* theora stuff */
#include <vorbis/codec.h>   /* vorbis stuff */

#define STRBUF_SIZE 4096
#define NET_BUFFER_SIZE (4*1024)
#define VIDEO_BUFFER_SIZE (1024*1024)
#define MAX_AUDIO_PACKET_SIZE (64 * 1024)
#define MIN_AUDIO_SIZE (64*1024)

#define DEFAULT_CHANNELS 1
#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240
#define DEFAULT_FRAME_RATE 25
#define END_OF_STREAM 20
#define MIN_PRIORITY 0
#define DEFAULT_PRIORITY 1
#define MAX_PRIORITY 20
#define MAX_NO_STREAM 50
#define THEORA_NUM_HEADER_PACKETS 3
#define MAX_WRONG_PACKETS 10

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

static char   *pdp_icedthe_version = "pdp_icedthe~: version 0.1, a theora stream reader ( ydegoyon@free.fr).";

typedef struct pdp_icedthe_struct
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
    t_outlet *x_outlet_state;      // for informing of the connection state
    t_outlet *x_outlet_nbframes;   // number of frames emitted
    t_outlet *x_outlet_framerate;  // real framerate
    t_outlet *x_outlet_endofstream;// for signaling the end of the stream
    t_outlet *x_outlet_time;       // outputing the video/audio delay

    pthread_t x_decodechild;       // stream decoding thread
    pthread_t x_connectchild;      // connecting thread
    pthread_mutex_t x_audiolock;   // audio mutex
    pthread_mutex_t x_videolock;   // video mutex
    int x_priority;              // priority of decoding thread

    char  *x_url;           // url to connect to
    char  *x_hostname;      // hostname of the server ( or IP )
    char  *x_mountpoint;    // mountpoint requested
    int x_bitrate;        // bitrate of stream read at connection time
    char  *x_name;          // name of stream 
    char  *x_genre;         // genre of stream
    int x_portnum;        // port number
    int x_insock;         // socket file descriptor
    int x_decoding;       // decoding flag
    int x_theorainit;     // flag for indicating that theora is initialized
    int x_videoready;     // video ready flag
    int x_newpicture;     // new picture flag
    int x_notpackets;     // number of theora packets decoded
    int x_novpackets;     // number of vorbis packets decoded
    int x_nbnostream;     // number of cycles without a video stream
    int x_endofstream;    // end of the stream reached
    int x_nbframes;       // number of frames emitted
    t_float x_framerate;    // framerate
    int x_forcedframerate;// the framerate we want to receive
    int x_samplerate;     // audio sample rate
    int x_audiochannels;  // audio channels
    int x_blocksize;      // audio block size
    int x_audioon;        // audio buffer filling flag
    int x_connected;      // connection flag
    int x_pconnected;     // previous state
    int x_cursec;         // current second
    int x_secondcount;    // number of frames received in the current second
    struct timeval x_starttime; // reading starting time
    char  x_request[STRBUF_SIZE]; // string to be send to server

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

    double           x_videotime;      // video logical time of last packet
    double           x_audiotime;      // audio logical time of last packet
    double           x_ptime;          // previous state

      /* audio structures */
    int x_audio;           // flag to activate the decoding of audio
    t_float x_audio_inl[4*MAX_AUDIO_PACKET_SIZE]; /* buffer for float audio decoded from ogg */
    t_float x_audio_inr[4*MAX_AUDIO_PACKET_SIZE]; /* buffer for float audio decoded from ogg */
    int x_audioin_position; // writing position for incoming audio

} t_pdp_icedthe;

static void pdp_icedthe_priority(t_pdp_icedthe *x, t_floatarg fpriority )
{
   if ( ( (int)fpriority >= MIN_PRIORITY ) && ( (int)fpriority <= MAX_PRIORITY ) )
   {
     x->x_priority = (int)fpriority;
   }
}

static void pdp_icedthe_framerate(t_pdp_icedthe *x, t_floatarg fframerate )
{
   if ( fframerate > 0. )
   {
     x->x_forcedframerate = (int)fframerate;
   }
}

static void pdp_icedthe_audio(t_pdp_icedthe *x, t_floatarg faudio )
{
   if ( ( faudio == 0. ) || ( faudio == 1. ) )
   {
      x->x_audio = (int)faudio;
   }
}

static int strip_ice_header(char *head, int n)
{
    int i;
    for (i = 0; i < (n - 2); i++)
    {
        if ((head[i] == '\n') && (head[i + 1] == '\n'))
            break;
    }
    head[i + 1] = '\0';
    return n - (i + 1);
}

static void pdp_icedthe_disconnect(t_pdp_icedthe *x)
{
 int ret, i, count=0;
 struct timespec twait;

   twait.tv_sec = 0; 
   twait.tv_nsec = 100000000; // 100 ms

   if ( x->x_insock == -1 )
   {
     post("pdp_icedthe~ : close request but no stream is played ... ignored" );
     return;
   }

   if ( x->x_connected )
   {
     x->x_connected = 0;
     // post("pdp_icedthe~ : waiting end of decoding..." );
     // while ( x->x_decoding ) nanosleep( &twait, NULL );

     if ( close( x->x_insock ) < 0 )
     {
        post( "pdp_icedthe~ : could not close input stream" );
        perror( "fclose" );
     }
     x->x_insock = -1;

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
   x->x_nbnostream = 0;
   x->x_nbframes = 0;
   x->x_decoding = 0;
   x->x_theorainit = 0;
   x->x_videoready = 0;
   x->x_newpicture = 0;

   x->x_nbframes = 0;
   x->x_framerate = 0.;
   x->x_videotime = 0.;
   x->x_audiotime = 0.;
   x->x_endofstream = 1;
}

static int pdp_icedthe_get_buffer_from_network(int socket, ogg_sync_state *oy)
{
  char *buffer;
  int bytes;

    buffer=ogg_sync_buffer(oy, NET_BUFFER_SIZE);
    if ( ( bytes = read( socket, buffer, NET_BUFFER_SIZE ) ) < 0 )
    {
      post( "pdp_icedthe~ : could not read data from the server" );
      perror( "read" );  
      return -1;
    }
    ogg_sync_wrote(oy,bytes);
    return(bytes);
}

static int pdp_icedthe_queue_page(t_pdp_icedthe *x)
{
 
  if(x->x_notpackets) 
  {
    if ( x->x_connected && ( ogg_page_serialno(&x->x_ogg_page) == x->x_statet.serialno ) )
    { 
       // post( "pdp_icedthe~ : got a video page (#=%ld)", x->x_statet.pageno );
       x->x_videotime = theora_granule_time(&x->x_theora_state, ogg_page_granulepos(&x->x_ogg_page));
    }
    ogg_stream_pagein(&x->x_statet, &x->x_ogg_page);
  }
  if(x->x_novpackets) 
  {
    if ( x->x_connected && ( ogg_page_serialno(&x->x_ogg_page) == x->x_statev.serialno ) )
    { 
       // post( "pdp_icedthe~ : got an audio page (#=%ld)", x->x_statev.pageno );
       x->x_audiotime = vorbis_granule_time(&x->x_dsp_state, ogg_page_granulepos(&x->x_ogg_page));
    }
    ogg_stream_pagein(&x->x_statev, &x->x_ogg_page); 
  }
  return 0;
}

static int pdp_icedthe_decode_stream(t_pdp_icedthe *x)
{
  int ret, count, maxsamples, samples, si=0, sj=0;
  float **pcm;
  struct timespec mwait;
  struct timeval ctime;
  long long tplaying;
  long long ttheoretical;
  unsigned char *pY, *pU, *pV; 
  unsigned char *psY, *psU, *psV; 
  int px, py;

   // post( "pdp_icedthe~ : decode packet" );

   while ( x->x_novpackets && !x->x_audioon && x->x_connected )
   {
     /* if there's pending, decoded audio, grab it */
     if((ret=vorbis_synthesis_pcmout(&x->x_dsp_state, &pcm))>0)
     {
       if ( x->x_audio && x->x_connected ) 
       {
         if ( pthread_mutex_lock( &x->x_audiolock ) < 0 )
         {
           post( "pdp_theorin~ : unable to lock audio mutex" );
           perror( "pthread_mutex_lock" );
         }
         maxsamples=(4*MAX_AUDIO_PACKET_SIZE-x->x_audioin_position);
         samples=(ret<maxsamples)?ret:maxsamples;

         memcpy( (void*)&x->x_audio_inl[x->x_audioin_position], pcm[0], samples*sizeof(t_float) );
         memcpy( (void*)&x->x_audio_inr[x->x_audioin_position], pcm[1], samples*sizeof(t_float) );
         x->x_audioin_position = ( x->x_audioin_position + samples ) % (4*MAX_AUDIO_PACKET_SIZE);

         if ( ( x->x_audioin_position > MIN_AUDIO_SIZE ) && (!x->x_audioon) )
         {
           x->x_audioon = 1;
           // post( "pdp_icedthe~ : audio on (audioin=%d)", x->x_audioin_position );
         }
         // tell vorbis how many samples were read
         // post( "pdp_icedthe~ : got %d audio samples (audioin=%d)", samples, x->x_audioin_position );
         vorbis_synthesis_read(&x->x_dsp_state, samples);
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

   if ( !x->x_newpicture && x->x_connected )
   {
     while(x->x_notpackets && !x->x_videoready && x->x_connected )
     {
       // theora is one in, one out...
       if(ogg_stream_packetout(&x->x_statet, &x->x_ogg_packet)>0)
       {
         theora_decode_packetin(&x->x_theora_state, &x->x_ogg_packet);
         // post( "pdp_icedthe~ : got one video frame" );
         x->x_videoready=1;
         x->x_nbnostream=0;
       }
       else
       {
         if ( x->x_nbframes > 0 )
         {
           x->x_nbnostream++; 
         }
         
         if ( x->x_nbnostream > MAX_NO_STREAM )
         {
           post ( "pdp_icedthe~ : receiving too few frames... disconnecting." ); 
           x->x_endofstream = 1;
           x->x_nbframes = 0;
           x->x_audioin_position = 0; // reset audio
           x->x_audioon = 0; 
           x->x_connected = 0; 
           x->x_notpackets = 0; 
           x->x_novpackets = 0; 
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

       if ( x->x_secondcount < x->x_forcedframerate )
       { 
         // create a new pdp packet from PIX_FMT_YUV420P image format
         x->x_vwidth = x->x_yuvbuffer.y_width;
         x->x_vheight = x->x_yuvbuffer.y_height;
         x->x_vsize = x->x_vwidth*x->x_vheight;
         x->x_packet0 = pdp_packet_new_bitmap_yv12( x->x_vwidth, x->x_vheight );
         // post( "pdp_icedthe~ : allocated packet %d", x->x_packet0 );
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
         x->x_newpicture = 1;
       }
 
       // post( "pdp_icedthe~ : new picture decoded" );
       if ( pthread_mutex_unlock( &x->x_videolock ) < 0 )
       {
         post( "pdp_theorin~ : unable to unlock video mutex" );
         perror( "pthread_mutex_unlock" );
       }
     }
   }

   // read more data in
   if( ( x->x_audioin_position < MIN_AUDIO_SIZE ) || ( !x->x_newpicture ) )
   {
     if ( ( ret=pdp_icedthe_get_buffer_from_network(x->x_insock, &x->x_sync_state) ) < 0 )
     {
        pdp_icedthe_disconnect(x);
        return -1;
     }
     
     // post( "pdp_icedthe~ : read %d bytes from network", ret );
     while( ogg_sync_pageout(&x->x_sync_state, &x->x_ogg_page)>0 )
     {
       pdp_icedthe_queue_page(x);
     }
   }

   x->x_videoready = 0;

   return 0;

}

static void *pdp_icedthe_decode(void *tdata)
{
  t_pdp_icedthe *x = (t_pdp_icedthe*)tdata;
  struct sched_param schedprio;
  int pmin, pmax, p1;
  struct timespec twait;

    twait.tv_sec = 0; 
    twait.tv_nsec = 25000000; // 25 ms
 
    schedprio.sched_priority = sched_get_priority_min(SCHED_FIFO) + x->x_priority;
#ifdef __gnu_linux__
    if ( sched_setscheduler(0, SCHED_FIFO, &schedprio) == -1)
    {
        post("pdp_icedthe~ : couldn't set priority for decoding thread.");
    }
#endif

    while ( x->x_decodechild )
    {
      if ( x->x_connected ) 
      {
        if ( x->x_decoding == 0 )
        {
          post( "pdp_icedthe~ : child started decoding" );  
          x->x_decoding = 1;
        }
        // decode incoming packets
        pdp_icedthe_decode_stream( x );
        nanosleep( &twait, NULL ); 
      }
      else
      {
        if ( x->x_decoding == 1 ) 
        {
          post( "pdp_icedthe~ : child stopped decoding" );  
          x->x_decoding = 0;
        }
        nanosleep( &twait, NULL ); // nothing to do, just wait
      }
    }

    x->x_decodechild = 0;
    post("pdp_icedthe~ : decoding child exiting." );
    return NULL;
}

static void pdp_icedthe_split_url(t_pdp_icedthe *x)
{
  char *hostptr = NULL, *p, *endhost = NULL, *pathptr = NULL;
  int length;

     /* strip http:// or ftp:// */
   p = x->x_url;
   if (strncmp(p, "http://", 7) == 0) p+=7;

   hostptr = p;
   while (*p && *p != '/' && *p != ':')  /* look for end of hostname: */ p++;

   endhost = p;
   switch ( *p )
   {
      case ':' :
         x->x_portnum = atoi( p+1 );
         while (*p && *p != '/') p++;
         pathptr = p+1;
         break;
      case '/' :
         x->x_portnum = 8000;
         pathptr = p+1;
         break;
      default :
         if ( ( p - x->x_url ) != (int)strlen( x->x_url ) )
         {
            post( "pdp_icedthe~ : wrong url : %s", hostptr );
            return;
         }
         pathptr = "";
         break;
   }

   length = (int)(endhost - hostptr);
   if ( x->x_hostname ) 
   {
     post ("pdp_icedthe~ : freeing hostname" );
     free( x->x_hostname );
   }
   x->x_hostname=(char*)malloc( length + 1);
   strncpy( x->x_hostname, hostptr, length );
   x->x_hostname[ length ] = '\0';

   if ( x->x_mountpoint ) free( x->x_mountpoint );
   x->x_mountpoint=(char*)malloc( strlen( pathptr ) + 1);
   strncpy( x->x_mountpoint, pathptr, strlen( pathptr ) );
   x->x_mountpoint[ strlen( pathptr ) ] = '\0';

   post ("pdp_icedthe~ : connecting to host=>%s< port=>%d< mountpoint=>%s<", x->x_hostname, x->x_portnum, x->x_mountpoint );
}

static void pdp_icedthe_connect(t_pdp_icedthe *x, t_symbol *s);

static void *pdp_icedthe_do_connect(void *tdata)
{
  pthread_attr_t decode_child_attr;
  ogg_stream_state o_tempstate;
  t_pdp_icedthe *x = (t_pdp_icedthe*)tdata;
  int         sockfd;
  struct        sockaddr_in server;
  struct        hostent *hp;
  fd_set        fdset;
  struct timeval tv;
  int         numrelocs = 0;
  int         i, ret, rest, nanswers=0;
  char          *cpoint = NULL;
  int         offset = 0, endofheaders = 0, wpackets = 0;
  char          *sptr = NULL;
   
   if ( x->x_insock != -1 )
   {
     post("pdp_icedthe~ : connect request but a stream is open ... disconnecting" );
     pdp_icedthe_disconnect(x);
   }

   if ( ( sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) ) <= 0 )
   {
      error("pdp_icedthe~: couldn't obtain a socket");
      x->x_connectchild = 0;
      return NULL;
   }

   server.sin_family = AF_INET;
   hp = gethostbyname(x->x_hostname);
   if (hp == 0)
   {
      post("pdp_icedthe~: your server is unresolved here.");
      if ( close(sockfd) < 0 ) post("pdp_icedthe~: could not close socket" );
      x->x_connectchild = 0;
      return NULL;
   }
   memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);

        /* assign client port number */
   server.sin_port = htons((unsigned short)x->x_portnum);

        /* try to connect.  */
   if (connect(sockfd, (struct sockaddr *) &server, sizeof (server)) < 0)
   {
      error("pdp_icedthe~: connection failed!\n");
      if ( close(sockfd) < 0 ) post("pdp_icedthe~: could not close socket" );
      x->x_connectchild = 0;
      return NULL;
   }
   post("pdp_icedthe~: connected  : socket opened" );

          /* check if we can read from socket */
   FD_ZERO( &fdset);
   FD_SET( sockfd, &fdset);
   tv.tv_sec  = 0;            /* seconds */
   tv.tv_usec = 500;          /* microseconds */

   ret = select(sockfd + 1, &fdset, NULL, NULL, &tv);
   if(ret != 0)
   {
      error("pdp_icedthe~: can not read from socket");
      if ( close(sockfd) < 0 ) post("pdp_icedthe~: could not close socket" );
      x->x_connectchild = 0;
      return NULL;
   }

         /* build up stuff we need to send to server */
   sprintf(x->x_request, "GET /%s HTTP/1.0 \r\nHost: %s\r\nUser-Agent: pdp_icedthe~ 0.1\r\nAccept: */*\r\n\r\n",
            x->x_mountpoint, x->x_hostname);

   if ( send(sockfd, x->x_request, strlen(x->x_request), 0) < 0 )    /* say hello to server */
   {
      post( "pdp_icedthe~: could not contact server... " );
      perror( "send" );
      x->x_connectchild = 0;
      return NULL;
   }
   post("pdp_icedthe~: send done" );

   memset( x->x_request, 0x00, STRBUF_SIZE );

     // read all the answer
   endofheaders=0;
   while ( !endofheaders )
   {
     if( ( ret = recv(sockfd, x->x_request+offset, 1, MSG_NOSIGNAL) ) <0)
     {
       error("pdp_icedthe~: no response from server");
       perror( "recv" );
       x->x_connectchild = 0;
       return NULL;
     }
     // post ( "pdp_icedthe~ : received %d bytes at %d", ret, offset );
     for ( i=0; i<=offset; i++ )
     {
       if ( ( x->x_request[i] == '\n' && x->x_request[i+1] == '\n' ) ||
          ( x->x_request[i] == 10 && x->x_request[i+1] == 13 ) )
       {
          endofheaders=1;
       }
     }
     if ( offset+ret < STRBUF_SIZE )
     {
        offset+=ret;
     }
     else
     {
        post( "pdp_icedthe~ : headers too long." );
        x->x_connectchild = 0;
        return NULL;
     } 
   }

   post( "pdp_icedthe~ : read HTTP headers : %s", x->x_request );
   post( "pdp_icedthe~ : got HTTP answer" );

   strip_ice_header(x->x_request, STRBUF_SIZE);
   if(sptr = strstr(x->x_request, "302"))
   {
       cpoint = NULL;
       cpoint = strstr(x->x_request, "Location:");
       if ( cpoint == NULL )
       {
          post( "pdp_icedthe~ : stream has moved but couldn't find new location out of this :" );
          post("pdp_icedthe~: %s", x->x_request );
          x->x_connectchild = 0;
          return NULL;
       }
       if ( x->x_url ) free( x->x_url );
       x->x_url = strdup(cpoint + 10);
       post("pdp_icedthe~: relocating to %s", x->x_url);
       pdp_icedthe_split_url(x);
       x->x_connected = 0;
       pdp_icedthe_do_connect(x);
       return NULL;
   }
   if ( !(sptr = strstr(x->x_request, "200")) )
   {
       error("pdp_icedthe~: cannot connect to the stream");
       error("pdp_icedthe~: server answered : %s", x->x_request);
       x->x_connectchild = 0;
       return NULL;
   }

       // check what we got
   if( cpoint = strstr(x->x_request, "x-audiocast-mount:"))
   {
      if ( x->x_mountpoint ) free( x->x_mountpoint );
      x->x_mountpoint = strdup(cpoint + 18);
      for ( i=0; i<(int)strlen(x->x_mountpoint); i++ )
      {
         if ( x->x_mountpoint[i] == '\n' )
         {
            x->x_mountpoint[i] = '\0';
            break;
         }
      }
      post("           mountpoint: %s", x->x_mountpoint);
   }
   if( cpoint = strstr(x->x_request, "x-audiocast-server-url:"))
   {
    sptr = strdup( cpoint + 24);
    for ( i=0; i<(int)strlen(sptr); i++ )
    {
       if ( sptr[i] == '\n' )
       {
         sptr[i] = '\0';
         break;
       }
     }
     post("           server-url: %s", sptr);
   }
   if( cpoint = strstr(x->x_request, "x-audiocast-location:"))
   {
     sptr = strdup( cpoint + 22);
     for ( i=0; i<(int)strlen(sptr); i++ )
     {
       if ( sptr[i] == '\n' )
       {
         sptr[i] = '\0';
         break;
       }
     }
     post("           location: %s", sptr);
   }
   if( cpoint = strstr(x->x_request, "x-audiocast-admin:"))
   {
      sptr = strdup( cpoint + 19);
      for ( i=0; i<(int)strlen(sptr); i++ )
      {
        if ( sptr[i] == '\n' )
        {
           sptr[i] = '\0';
           break;
         }
      }
      post("           admin: %s", sptr);
   }
   if( cpoint = strstr(x->x_request, "x-audiocast-name:"))
   {
      x->x_name = strdup( cpoint + 17);
      for ( i=0; i<(int)strlen(x->x_name); i++ )
      {
        if ( x->x_name[i] == '\n' )
        {
           x->x_name[i] = '\0';
           break;
        }
      }
      post("           name: %s", x->x_name);
    }
    if( cpoint = strstr(x->x_request, "x-audiocast-genre:"))
    {
      x->x_genre = strdup( cpoint + 18);
      for ( i=0; i<(int)strlen(x->x_genre); i++ )
      {
        if ( x->x_genre[i] == '\n' )
        {
          x->x_genre[i] = '\0';
          break;
        }
      }
      post("           genre: %s", x->x_genre);
    }
    if( cpoint = strstr(x->x_request, "x-audiocast-url:"))
    {
      if ( x->x_url ) free( x->x_url );
      x->x_url = strdup( cpoint + 16);
      for ( i=0; i<(int)strlen(x->x_url); i++ )
      {
         if ( x->x_url[i] == '\n' )
         {
           x->x_url[i] = '\0';
           break;
         }
       }
       post("           url: %s", x->x_url);
    }
    if( cpoint = strstr(x->x_request, "x-audiocast-public:1"))
    {
       post("           broadcast is public");
    }
    else if( cpoint = strstr(x->x_request, "x-audiocast-public:0"))
    {
       post("           broadcast is NOT public");
    }
    if( cpoint = strstr(x->x_request, "x-audiocast-bitrate:"))
    {
      sptr = strdup( cpoint + 20);
      for ( i=0; i<(int)strlen(sptr); i++ )
      {
        if ( sptr[i] == '\n' )
        {
           sptr[i] = '\0';
           break;
        }
      }
      if(!strncmp(sptr, "320", 3))x->x_bitrate = 320;
      else if(!strncmp(sptr, "256", 3))x->x_bitrate = 256;
      else if(!strncmp(sptr, "224", 3))x->x_bitrate = 224;
      else if(!strncmp(sptr, "192", 3))x->x_bitrate = 192;
      else if(!strncmp(sptr, "160", 3))x->x_bitrate = 160;
      else if(!strncmp(sptr, "144", 3))x->x_bitrate = 144;
      else if(!strncmp(sptr, "128", 3))x->x_bitrate = 128;
      else if(!strncmp(sptr, "112", 3))x->x_bitrate = 112;
      else if(!strncmp(sptr, "96", 2))x->x_bitrate = 96;
      else if(!strncmp(sptr, "80", 2))x->x_bitrate = 80;
      else if(!strncmp(sptr, "64", 2))x->x_bitrate = 64;
      else if(!strncmp(sptr, "56", 2))x->x_bitrate = 56;
      else if(!strncmp(sptr, "48", 2))x->x_bitrate = 48;
      else if(!strncmp(sptr, "40", 2))x->x_bitrate = 40;
      else if(!strncmp(sptr, "32", 2))x->x_bitrate = 32;
      else if(!strncmp(sptr, "24", 2))x->x_bitrate = 24;
      else if(!strncmp(sptr, "16", 2))x->x_bitrate = 16;
      else if(!strncmp(sptr, "8", 1))x->x_bitrate = 8;
      else
      {
        post("pdp_icedthe~: unsupported bitrate! : %s", sptr);
        x->x_connectchild = 0;
        return NULL;
      }
      post("           bitrate: %d", x->x_bitrate);
   }

   ogg_sync_init(&x->x_sync_state);
   x->x_notpackets=0;
   x->x_novpackets=0;

   // init supporting Vorbis structures needed in header parsing
   vorbis_info_init(&x->x_vorbis_info);
   vorbis_comment_init(&x->x_vorbis_comment);

   // init supporting Theora structures needed in header parsing
   theora_comment_init(&x->x_theora_comment);
   theora_info_init(&x->x_theora_info);

   // parse headers
   while( !x->x_theorainit )
   {
    ret = pdp_icedthe_get_buffer_from_network(sockfd, &x->x_sync_state);
    if ( ret==0) break;
    if ( ret<0) 
    {
       post( "pdp_icedthe~ : unable to read from server" );
       pdp_icedthe_disconnect( x );
       x->x_theorainit = 0;
       x->x_connectchild = 0;
       return NULL;
    }

    while( ogg_sync_pageout(&x->x_sync_state, &x->x_ogg_page) > 0 )
    {
      pdp_icedthe_queue_page(x);

      ogg_stream_init(&o_tempstate, ogg_page_serialno(&x->x_ogg_page));
      ogg_stream_pagein(&o_tempstate, &x->x_ogg_page);
      ogg_stream_packetout(&o_tempstate, &x->x_ogg_packet);

      /* identify the codec: try theora */
      if(!x->x_notpackets && 
          theora_decode_header(&x->x_theora_info, &x->x_theora_comment, &x->x_ogg_packet)>=0)
      {
        // it is theora 
        // post( "pdp_icedthe~ : got one theora header.");
        memcpy(&x->x_statet, &o_tempstate, sizeof(o_tempstate));
        x->x_notpackets=1;
        x->x_theorainit = 1;
      }else 
      if(!x->x_novpackets && 
         vorbis_synthesis_headerin(&x->x_vorbis_info, &x->x_vorbis_comment, &x->x_ogg_packet)>=0)
      {
        // post( "pdp_icedthe~ : got one vorbis header.");
        memcpy(&x->x_statev, &o_tempstate, sizeof(o_tempstate));
        x->x_novpackets=1;
      }
      else
      {
        // post( "pdp_icedthe~ : got something else.");
        wpackets++;
        ogg_stream_clear(&o_tempstate);
        if ( wpackets > MAX_WRONG_PACKETS )
        {
          post( "pdp_icedthe~ : couldn't read any headers.");
          x->x_theorainit = 0;
          x->x_connectchild = 0;
          return NULL;
        }
      }
    }
   }

   // we're expecting more header packets.
   while( (x->x_notpackets && x->x_notpackets<THEORA_NUM_HEADER_PACKETS) || (x->x_novpackets && x->x_novpackets<THEORA_NUM_HEADER_PACKETS) )
   {
     // look for further theora headers
     while(x->x_notpackets && (x->x_notpackets<THEORA_NUM_HEADER_PACKETS) && 
           (ret=ogg_stream_packetout(&x->x_statet, &x->x_ogg_packet)))
     {
       if( ret<0 )
       {
         post("pdp_icedthe~ : error parsing theora stream headers\n");
         x->x_theorainit = 0;
         x->x_connectchild = 0;
         return NULL;
       }
       if( theora_decode_header(&x->x_theora_info, &x->x_theora_comment, &x->x_ogg_packet) )
       {
         post("pdp_icedthe~ : error parsing theora stream headers\n");
         x->x_theorainit = 0;
         x->x_connectchild = 0;
         return NULL;
       }
       x->x_notpackets++;
       if(x->x_notpackets==THEORA_NUM_HEADER_PACKETS) break;
     }
 
     /* look for more vorbis header packets */
     while(x->x_novpackets && (x->x_novpackets<THEORA_NUM_HEADER_PACKETS) && 
           (ret=ogg_stream_packetout(&x->x_statev, &x->x_ogg_packet)))
     {
       if(ret<0)
       {
         post("pdp_icedthe~ : error parsing theora stream headers\n");
         x->x_theorainit = 0;
         x->x_connectchild = 0;
         return NULL;
       }
       if( vorbis_synthesis_headerin(&x->x_vorbis_info, &x->x_vorbis_comment, &x->x_ogg_packet) )
       {
         post("pdp_icedthe~ : error parsing theora stream headers\n");
         x->x_theorainit = 0;
         x->x_connectchild = 0;
         return NULL;
       }
       x->x_novpackets++;
       if(x->x_novpackets==THEORA_NUM_HEADER_PACKETS) break;
     }
 
     if(ogg_sync_pageout(&x->x_sync_state, &x->x_ogg_page)>0)
     {
       pdp_icedthe_queue_page(x); 
     }
     else
     {
       ret=pdp_icedthe_get_buffer_from_network(sockfd, &x->x_sync_state);
       if( ret==0 )
       {
         post("pdp_icedthe~ : end of stream while parsing headers\n");
         x->x_theorainit = 0;
         x->x_connectchild = 0;
         return NULL;
       }
       if( ret<0 )
       {
         post("pdp_icedthe~ : error reading from server\n");
         x->x_theorainit = 0;
         x->x_connectchild = 0;
         return NULL;
       }
     }
   }
   post( "pdp_icedthe~ : parsed headers ok (not=%d) (nov=%d)", x->x_notpackets, x->x_novpackets );

   // initialize decoders
   if( x->x_notpackets )
   {
     theora_decode_init(&x->x_theora_state, &x->x_theora_info);
     if ( x->x_theora_info.fps_denominator != 0 )
     {
       x->x_framerate = x->x_theora_info.fps_numerator/x->x_theora_info.fps_denominator;
     }
     else
     {
       x->x_framerate = DEFAULT_FRAME_RATE;
     }
     post("pdp_icedthe~ : stream %x is theora %dx%d %f fps video.",
                          x->x_statet.serialno,
                          x->x_theora_info.width,x->x_theora_info.height,
                          x->x_framerate);
     if(x->x_theora_info.width!=x->x_theora_info.frame_width || 
        x->x_theora_info.height!=x->x_theora_info.frame_height)
     {
       post("pdp_icedthe~ : frame content is %dx%d with offset (%d,%d).",
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
        post("pdp_icedthe~ : encoder specified ITU Rec 470M (NTSC) color.");
        break;;
      case OC_CS_ITU_REC_470BG:
        post("pdp_icedthe~ : encoder specified ITU Rec 470BG (PAL) color.");
        break;;
      default:
        post("pdp_icedthe~ : warning: encoder specified unknown colorspace (%d).", 
                             x->x_theora_info.colorspace);
        break;;
     }
   }
   else
   {
     // tear down the partial theora setup 
     theora_info_clear(&x->x_theora_info);
     theora_comment_clear(&x->x_theora_comment);
     post("pdp_icedthe~ : could not initialize theora decoder (theora packets=%d).", x->x_notpackets);
     x->x_theorainit = 0;
     x->x_connectchild = 0;
     return NULL;
   }

   if( x->x_novpackets )
   {
     vorbis_synthesis_init(&x->x_dsp_state, &x->x_vorbis_info);
     vorbis_block_init(&x->x_dsp_state, &x->x_vorbis_block); 
     x->x_audiochannels = x->x_vorbis_info.channels;
     x->x_samplerate = x->x_vorbis_info.rate;
     post("pdp_icedthe~ : ogg logical stream %x is vorbis %d channel %d Hz audio.",
                          x->x_statev.serialno,
                          x->x_audiochannels, x->x_samplerate);
   }
   else
   {
     /* tear down the partial vorbis setup */
     vorbis_info_clear(&x->x_vorbis_info);
     vorbis_comment_clear(&x->x_vorbis_comment);
     post("pdp_icedthe~ : could not initialize vorbis decoder (vorbis packets=%d).", x->x_novpackets);
     // x->x_theorainit = 0;
     // return NULL;
     x->x_audio = 0;
   }
   // everything seems to be ready
   x->x_insock = sockfd;
   x->x_connected = 1;

   if ( x->x_decodechild == 0 )
   {
     x->x_decodechild = 1; // trick & treets
     // launch decoding thread
     if ( pthread_attr_init( &decode_child_attr ) < 0 ) 
     {
        post( "pdp_icedthe~ : could not launch decoding thread" );
        perror( "pthread_attr_init" );
        pthread_exit(NULL);
     }
     if ( pthread_create( &x->x_decodechild, &decode_child_attr, pdp_icedthe_decode, x ) < 0 ) 
     {
        post( "pdp_icedthe~ : could not launch decoding thread" );
        perror( "pthread_create" );
        pthread_exit(NULL);
     }
     else
     {
        post( "pdp_icedthe~ : decoding thread %d launched", (int)x->x_decodechild );
     }
   }

   if ( gettimeofday(&x->x_starttime, NULL) == -1)
   {
     post("pdp_icedthe~ : could not set start time" );
   }

   x->x_nbframes = 0;
   x->x_endofstream = -1;
   x->x_connectchild = 0;
   post("pdp_icedthe~ : connect child exiting." );

   return NULL;
}

static void pdp_icedthe_connect(t_pdp_icedthe *x, t_symbol *s)
{
  int ret, i;
  pthread_attr_t connect_child_attr;

   if ( x->x_connectchild != 0 )
   {
     post("pdp_icedthe~ : connection request but a connection is pending ... ignored." );
     return;
   }

   if ( x->x_connected )
   {
     post("pdp_icedthe~ : connection request but a connection is established ... disconnecting." );
     pdp_icedthe_disconnect(x);
   }

   if ( x->x_url ) free( x->x_url );
   x->x_url = (char*) malloc( strlen( s->s_name ) + 1 );
   strcpy( x->x_url, s->s_name );

   post ("pdp_icedthe~ : connecting to url=%s", x->x_url );

   pdp_icedthe_split_url(x);

   // launch connection thread
   if ( pthread_attr_init( &connect_child_attr ) < 0 ) {
       post( "pdp_icedthe~ : could not launch connection thread" );
       perror( "pthread_attr_init" );
       return;
   }
   if ( pthread_attr_setdetachstate( &connect_child_attr, PTHREAD_CREATE_DETACHED ) < 0 ) {
       post( "pdp_icedthe~ : could not launch connection thread" );
       perror( "pthread_attr_setdetachstate" );
       return;
   }
   if ( pthread_create( &x->x_connectchild, &connect_child_attr, pdp_icedthe_do_connect, x ) < 0 ) {
       post( "pdp_icedthe~ : could not launch connection thread" );
       perror( "pthread_create" );
       return;
   }
   else
   {
       post( "pdp_icedthe~ : connection thread %d launched", (int)x->x_connectchild );
   }

   return;
}

    /* decode the audio buffer */
static t_int *pdp_icedthe_perform(t_int *w)
{
  t_float *out1   = (t_float *)(w[1]);       // left audio inlet
  t_float *out2   = (t_float *)(w[2]);       // right audio inlet 
  t_pdp_icedthe *x = (t_pdp_icedthe *)(w[3]);
  int n = (int)(w[4]);                       // number of samples 
  struct timeval etime;
  int sn;

    x->x_blocksize = n;

    // just read the buffer
    if ( x->x_audioon && x->x_connected )
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
      // post( "pdp_icedthe~ : audio in position : %d", x->x_audioin_position );
      if ( x->x_audioin_position <= sn )
      {
         x->x_audioon = 0;
         // post( "pdp_icedthe~ : audio off ( audioin : %d, channels=%d )", 
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
      // post("pdp_icedthe~ : no available audio" );
      while (n--)
      {
        *(out1++) = 0.0;
        *(out2++) = 0.0;
      }
    }	

    // update framerate
    if ( gettimeofday(&etime, NULL) == -1)
    {
       post("pdp_icedthe~ : could not read time" );
    }
    if ( etime.tv_sec != x->x_cursec )
    {
       x->x_cursec = etime.tv_sec;
       if (x->x_connected) outlet_float( x->x_outlet_framerate, x->x_secondcount );
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
       outlet_float( x->x_outlet_nbframes, x->x_nbframes );
       if ( pthread_mutex_unlock( &x->x_videolock ) < 0 )
       {
         post( "pdp_theorin~ : unable to unlock video mutex" );
         perror( "pthread_mutex_unlock" );
       }
    }
    if ( x->x_endofstream == 1 ) // only once
    {
      outlet_float( x->x_outlet_endofstream, x->x_endofstream );
      outlet_float( x->x_outlet_time, 0. );
      x->x_endofstream = 0;
    }
    if ( x->x_endofstream == -1 ) // reset
    {
      x->x_endofstream = 0;
      outlet_float( x->x_outlet_endofstream, x->x_endofstream );
    }
    if ( x->x_connected != x->x_pconnected )
    {
       x->x_pconnected = x->x_connected; 
       outlet_float( x->x_outlet_state, x->x_connected );
    }
    if ( ( x->x_videotime != -1) && ( x->x_audiotime != -1 ) && 
         ( (x->x_videotime-x->x_audiotime) != x->x_ptime ) )
    {
      x->x_ptime = x->x_videotime-x->x_audiotime;
      outlet_float( x->x_outlet_time, (t_float)(x->x_videotime-x->x_audiotime) );
    }

    return (w+5);
}

static void pdp_icedthe_dsp(t_pdp_icedthe *x, t_signal **sp)
{
    dsp_add(pdp_icedthe_perform, 4, sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

static void pdp_icedthe_free(t_pdp_icedthe *x)
{
  int i;

    if ( x->x_decodechild )
    {
      x->x_decodechild = 0;
    }

    if ( x->x_connected )
    {
       pdp_icedthe_disconnect(x);
       x->x_connected = 0;
    }

    post( "pdp_icedthe~ : freeing object" );
}

t_class *pdp_icedthe_class;

void *pdp_icedthe_new(void)
{
    int i;

    t_pdp_icedthe *x = (t_pdp_icedthe *)pd_new(pdp_icedthe_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("frame_cold"));

    x->x_pdp_out = outlet_new(&x->x_obj, &s_anything);

    x->x_outlet_left = outlet_new(&x->x_obj, &s_signal);
    x->x_outlet_right = outlet_new(&x->x_obj, &s_signal);

    x->x_outlet_state = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_nbframes = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_framerate = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_endofstream = outlet_new(&x->x_obj, &s_float);
    x->x_outlet_time = outlet_new(&x->x_obj, &s_float);

    x->x_packet0 = -1;
    x->x_decodechild = 0;
    x->x_connectchild = 0;
    x->x_decoding = 0;
    x->x_theorainit = 0;
    x->x_priority = DEFAULT_PRIORITY;
    x->x_framerate = DEFAULT_FRAME_RATE;
    x->x_forcedframerate = 1024; // think it will not occur or maybe in 2156
    x->x_nbframes = 0;
    x->x_samplerate = 0;
    x->x_audio = 1;
    x->x_audiochannels = 0;
    x->x_audioin_position = 0;
    x->x_videoready = 0;
    x->x_newpicture = 0;
    x->x_endofstream = 0;
    x->x_notpackets = 0;
    x->x_novpackets = 0;
    x->x_blocksize = MIN_AUDIO_SIZE;
    x->x_insock = -1;
    x->x_connected = 0;
    x->x_pconnected = 0;
    x->x_url = NULL;
    x->x_hostname = NULL;
    x->x_mountpoint = NULL;
    x->x_portnum = 8000;
    x->x_nbnostream = 0;
    x->x_videotime = -1.0;
    x->x_audiotime = -1.0;
    x->x_ptime = 0.;

    memset( &x->x_audio_inl[0], 0x0, 4*MAX_AUDIO_PACKET_SIZE*sizeof(t_float) );
    memset( &x->x_audio_inr[0], 0x0, 4*MAX_AUDIO_PACKET_SIZE*sizeof(t_float) );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_icedthe_tilde_setup(void)
{
    // post( pdp_icedthe_version );
    pdp_icedthe_class = class_new(gensym("pdp_icedthe~"), (t_newmethod)pdp_icedthe_new,
    	(t_method)pdp_icedthe_free, sizeof(t_pdp_icedthe), 0, A_NULL);

    class_addmethod(pdp_icedthe_class, (t_method)pdp_icedthe_dsp, gensym("dsp"), A_NULL);
    class_addmethod(pdp_icedthe_class, (t_method)pdp_icedthe_connect, gensym("connect"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_icedthe_class, (t_method)pdp_icedthe_disconnect, gensym("disconnect"), A_NULL);
    class_addmethod(pdp_icedthe_class, (t_method)pdp_icedthe_priority, gensym("priority"), A_FLOAT, A_NULL);
    class_addmethod(pdp_icedthe_class, (t_method)pdp_icedthe_framerate, gensym("framerate"), A_FLOAT, A_NULL);
    class_addmethod(pdp_icedthe_class, (t_method)pdp_icedthe_audio, gensym("audio"), A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
