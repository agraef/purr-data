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

/*  This object is a video streaming emitter
 * -- compressed with a very simple codec ( smoothing + huffman + bz2 )
 *  It sends PDP packet to a pdp_i receiving object
 */


#include "pdp.h"
#include "pdp_streaming.h"
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <bzlib.h> // bz2 compression routines

#define DEFAULT_FRAME_RATE 25

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

static char   *pdp_o_version = "pdp_o: version 0.1, a video stream emitter, written by ydegoyon@free.fr";

typedef struct pdp_o_struct
{
    t_object x_obj;
    t_float x_f;

    int x_vwidth;
    int x_vheight;
    int x_vsize;
    int x_hsize;   // size of huffman coded data

    int x_packet0;
    int x_dropped;
    int x_queue_id;

    int x_emitflag;

        /* connection data        */
    int x_fd;          // info about connection status 
    int x_framessent;
    int x_framesdropped;
    int x_secondcount;
    int x_bandwidthcount;
    int x_cursec;
    int x_framerate;
    int x_smoothing;
 
    t_hpacket x_hpacket; // packet header

    short int *x_previous_frame;
    char *x_diff_frame;
    char *x_hdata; // huffman coded data
    char *x_cdata; // compressed data to be sent

    t_outlet *x_connection_status; // indicates status
    t_outlet *x_frames; // outlet for the number of frames emitted
    t_outlet *x_framesd; // outlet for the number of frames dropped
    t_outlet *x_bandwidth; // outlet for bandwidth

} t_pdp_o;

static void pdp_o_free_ressources(t_pdp_o *x)
{
   if ( x->x_diff_frame ) freebytes( x->x_diff_frame, x->x_vsize + (x->x_vsize>>1) );
   if ( x->x_previous_frame ) freebytes( x->x_previous_frame, (x->x_vsize + (x->x_vsize>>1))<<1 );
   if ( x->x_cdata ) freebytes( x->x_cdata, (x->x_vsize + (x->x_vsize>>1))*1.01+600 ); // size is taken from bzlib manual
   if ( x->x_hdata ) freebytes( x->x_hdata, ((x->x_vsize + (x->x_vsize>>1))<<1) ); // size is taken from bzlib manual
}

static void pdp_o_allocate(t_pdp_o *x)
{
   x->x_diff_frame = (char*) getbytes( x->x_vsize + (x->x_vsize>>1) );
   memset( x->x_diff_frame, 0x00, x->x_vsize + (x->x_vsize>>1) );
   x->x_previous_frame = (short int*) getbytes( (x->x_vsize + (x->x_vsize>>1))<<1 );
   memset( x->x_previous_frame, 0x00, (x->x_vsize + (x->x_vsize>>1))<<1 );
   x->x_cdata = (char*) getbytes( (x->x_vsize + (x->x_vsize>>1))*1.01+600 );
   memset( x->x_cdata, 0x00, (x->x_vsize + (x->x_vsize>>1))*1.01+600 );
   x->x_hdata = (char*) getbytes( (x->x_vsize + (x->x_vsize>>1))<<1 );
   memset( x->x_hdata, 0x00, (x->x_vsize + (x->x_vsize>>1))<<1 );
   strcpy( x->x_hpacket.tag, PDP_PACKET_TAG );
}

    /* disconnect from receiver */
static void pdp_o_disconnect(t_pdp_o *x)
{
  int ret;

    if(x->x_fd >= 0)            /* close socket */
    {
        close(x->x_fd);
        x->x_fd = -1;
        outlet_float( x->x_connection_status, 0 );
        post("pdp_o : connection closed");
    }

}

    /* set the emission frame rate */                  
static void pdp_o_framerate(t_pdp_o *x, t_floatarg fframerate)
{
  if ( fframerate > 1 )
  {
     x->x_framerate = (int)fframerate;
  }
}

    /* set the smoothing factor */
static void pdp_o_smoothing(t_pdp_o *x, t_floatarg fsmoothing)
{
  if ( ( fsmoothing >= 0 ) && ( fsmoothing < 255 ) )
  {
     x->x_smoothing = (int)fsmoothing;
  }
}

    /* smoothe image */
static void pdp_o_smoothe(t_pdp_o *x, short int *source, int size )
{
  int i;
  char evalue, eevalue;
  char value;

    if ( x->x_smoothing == 0 ) return;

    for(i=0;i<size;i++)
    {
      value = (source[i])>>7;
      evalue = (value/x->x_smoothing)*x->x_smoothing;
      eevalue = ((value/x->x_smoothing)+1)*x->x_smoothing;

      if ( abs( value - evalue ) < x->x_smoothing/2 )
      {
        source[i] = evalue<<7;
      }
      else
      {
        source[i] = eevalue<<7;
      }
    }

    for(i=size;i<(size+(size>1));i++)
    {
      value = ((source[i])>>8)+128;
      evalue = (value/x->x_smoothing)*x->x_smoothing;
      eevalue = ((value/x->x_smoothing)+1)*x->x_smoothing;

      if ( abs( value - evalue ) < x->x_smoothing/2 )
      {
        source[i] = (evalue)<<8;
      }
      else
      {
        source[i] = (eevalue)<<8;
      }
    }
}

    /* huffman coding */
static int pdp_o_huffman(t_pdp_o *x, char *source, char *dest, int size, int *csize )
{
  int i;
  int value = source[0];
  char count = 0;
  int tcount=0;
  char *pcount=dest;
  char *pvalue=dest+1;

   *(csize)=2;
   dest++;
   for( i=0; i<size; i++)
   {
      if ( (source[i] == value) && (count<127) )
      {
        count++;
      }
      else
      {
        *(pcount)=count;
        *(pvalue) = value;
        tcount+=count;
        pcount+=2;
        pvalue+=2;
        *(csize)+=2;
        value=source[i];
        count=1;
      }
   }
   *(pcount)=count;
   tcount+=count;

   // huffman is no good for that image
   if ( (*csize) >= size )
   {
      *csize = size;
      memcpy( dest, source, size );
      return REGULAR;
   }
   else
   {
      // post( "pdp_o : huffman : compression ratio %d/%d : %f (total count=%d)", size, (*csize), 
      //                   (t_float)size/(t_float)(*csize), tcount );
      return HUFFMAN;
   }
}


    /* connect to a receiver on <hostname> <port> */
static void pdp_o_connect(t_pdp_o *x, t_symbol *shostname, t_floatarg fportno)
{
  struct          sockaddr_in csocket;
  struct          hostent *hp;
  int             portno            = fportno;    /* get port from message box */

     /* variables used for communication with the receiver */
  const char      *buf = 0;
  unsigned int    len;
  int    sockfd;
  int    ret;

    // close previous connection if existing
    pdp_o_disconnect(x);

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        error("pdp_o : error while attempting to create socket");
        perror( "socket" );
        return;
    }

        /* connect socket using hostname provided in command line */
    csocket.sin_family = AF_INET;
    hp = gethostbyname(shostname->s_name);
    if (hp == 0)
    {
        post("pdp_o : ip address of receiver could not be found");
        perror( "gethostbyname" );
        close(sockfd);
        return;
    }
    memcpy((char *)&csocket.sin_addr, (char *)hp->h_addr, hp->h_length);

        /* assign client port number */
    csocket.sin_port = htons((unsigned short)fportno);

       /* try to connect.  */
    post("pdp_o : connecting to port %d", (int)fportno);
    if (connect(sockfd, (struct sockaddr *) &csocket, sizeof (csocket)) < 0)
    {
        error("mp3streamout~: connection failed!\n");
        perror( "connect" );
        close(sockfd);
        return;
    }

    x->x_fd = sockfd;
    x->x_framessent = 0;
    x->x_framesdropped = 0;
    outlet_float( x->x_connection_status, 1 );
    post( "pdp_o : connected to receiver : %s:%d", shostname->s_name, (int)fportno );

}

   /* refresh means forcing the emission of a full frame */
static void pdp_o_refresh(t_pdp_o *x)
{
    strcpy( x->x_hpacket.tag, PDP_PACKET_TAG );
    memset( x->x_previous_frame, 0x00, (x->x_vsize + (x->x_vsize>>1))<<1 );
}

   /* start emitting */
static void pdp_o_start(t_pdp_o *x)
{
    if ( x->x_emitflag == 1 ) {
       post("pdp_o : start received but emission is started ... ignored.");
       return;
    }

    x->x_emitflag = 1;
    post("pdp_o : emission started");
}

   /* stop emitting */
static void pdp_o_stop(t_pdp_o *x)
{
    if ( x->x_emitflag == 0 ) {
       post("mp3write~: stop received but emission is stopped ... ignored.");
       return;
    }

    x->x_emitflag = 0;

    post("pdp_o : emission stopped");
}

static void pdp_o_process_yv12(t_pdp_o *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    int     count, i, ret=0;

    /* setting video track */
    if ( x->x_emitflag )
    {
      if ( ( (int)(header->info.image.width) != x->x_vwidth ) || 
           ( (int)(header->info.image.height) != x->x_vheight ) 
           )
      {
	 pdp_o_free_ressources(x);
         x->x_vwidth = header->info.image.width;
         x->x_vheight = header->info.image.height;
         x->x_vsize = x->x_vwidth*x->x_vheight;
	 pdp_o_allocate(x);
      }

      // smoothe image
      pdp_o_smoothe(x, data, x->x_vsize );

      for ( i=0; i<x->x_vsize; i++ )
      {
        int downvalue;
          
          downvalue = (data[i]>>7);
          if ( ( downvalue > 128 ) || 
               ( downvalue < -128 ) )
          {
             // post( "pdp_o : y value out of range : %d", downvalue );
          }
          if ( ( data[i] != x->x_previous_frame[i] ) ||
               ( !strcmp( x->x_hpacket.tag, PDP_PACKET_TAG ) ) )
          {
             x->x_diff_frame[i] = (char)downvalue;
          }
          else
          {
             x->x_diff_frame[i] = 0;
          }
      }
      for ( i=x->x_vsize; i<(x->x_vsize+(x->x_vsize>>1)); i++ )
      {
        int downvalue;
          
          downvalue = (data[i]>>8);
          if ( ( downvalue > 128 ) || 
               ( downvalue < -128 ) )
          {
             // post( "pdp_o : y value out of range : %d", downvalue );
          }
          if ( ( data[i] != x->x_previous_frame[i] ) ||
               ( !strcmp( x->x_hpacket.tag, PDP_PACKET_TAG ) ) )
          {
             x->x_diff_frame[i] = (char)downvalue;
          }
          else
          {
             x->x_diff_frame[i] = 0;
          }
      }

      x->x_hpacket.width = htonl(x->x_vwidth);
      x->x_hpacket.height = htonl(x->x_vheight);
      if ( gettimeofday(&x->x_hpacket.etime, NULL) == -1)
      {
        post("pdp_o : could not set emit time" );
      }
      if ( x->x_hpacket.etime.tv_sec != x->x_cursec )
      {
         x->x_cursec = x->x_hpacket.etime.tv_sec;
         x->x_secondcount = 0;
         x->x_bandwidthcount = 0;
      }

      x->x_hpacket.etime.tv_sec = htonl( x->x_hpacket.etime.tv_sec );
      x->x_hpacket.etime.tv_usec = htonl( x->x_hpacket.etime.tv_usec );

      // do not send the frame if too many frames 
      // have been sent in the current second
      if ( x->x_secondcount < x->x_framerate )
      {

        // try a huffman coding
        x->x_hpacket.encoding = htonl( pdp_o_huffman(x, x->x_diff_frame, x->x_hdata, x->x_vsize+(x->x_vsize>>1), &x->x_hsize ) );

        x->x_hpacket.clength = (x->x_vsize+(x->x_vsize>>1))*1.01+600;
        // compress the graphic data
        if ( ( ret = BZ2_bzBuffToBuffCompress( x->x_cdata, 
                                  &x->x_hpacket.clength,
                                  (char*) x->x_hdata,
       				  x->x_hsize,
  				  9, 0, 0 ) ) == BZ_OK )
        {
          // post( "pdp_o : bz2 compression (%d)->(%d)", x->x_hsize, x->x_hpacket.clength );
  
          x->x_secondcount++;

          // memorize last emitted frame
          memcpy( x->x_previous_frame, data, (x->x_vsize+(x->x_vsize>>1))<<1 );
   
          // send header
          x->x_hpacket.clength = htonl( x->x_hpacket.clength );
          count = send(x->x_fd, &x->x_hpacket, sizeof(x->x_hpacket), MSG_NOSIGNAL);
          if(count < 0)
          {
            error("pdp_o : could not send encoded data to the peer (%d)", count);
            perror( "send" );
            pdp_o_disconnect(x);
          }
          else
          {
            if((count > 0)&&(count != sizeof(x->x_hpacket)))
            {
              error("pdp_o : %d bytes skipped", sizeof(x->x_hpacket) - count);
            }
            x->x_bandwidthcount += count/1024;
          }
          x->x_hpacket.clength = ntohl( x->x_hpacket.clength );
  
          // send data
          count = send(x->x_fd, x->x_cdata, x->x_hpacket.clength, MSG_NOSIGNAL);
          if(count < 0)
          {
            error("pdp_o : could not send encoded data to the peer (%d)", count);
            perror( "send" );
            pdp_o_disconnect(x);
          }
          else
          {
            if((count > 0)&&(count != (int)x->x_hpacket.clength))
            {
              error("pdp_o : %d bytes skipped", x->x_hpacket.clength - count);
            }
            ++x->x_framessent;
            x->x_bandwidthcount += count/1024;
          }

          // unless after a refresh, next packets are diffs
          strcpy( x->x_hpacket.tag, PDP_PACKET_DIFF );

        }
        else
        {
          post( "pdp_o : bz2 compression failed (ret=%d)", ret );
        }
      }
      else
      {
        ++x->x_framesdropped;
        // post( "pdp_o : frames dropped ( framerate limit )" );
      }
    }

    return;
}

static void pdp_o_killpacket(t_pdp_o *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;
}

static void pdp_o_process(t_pdp_o *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_o_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            pdp_queue_add(x, pdp_o_process_yv12, pdp_o_killpacket, &x->x_queue_id);
            outlet_float( x->x_framesd, x->x_framesdropped );
            outlet_float( x->x_frames, x->x_framessent );
            outlet_float( x->x_bandwidth, x->x_bandwidthcount );
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_o_process */
	    break;
	    
	}
    }

}

static void pdp_o_input_0(t_pdp_o *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_o_process(x);

    }
}

static void pdp_o_free(t_pdp_o *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    // close connection if existing
    pdp_o_disconnect(x);
}

t_class *pdp_o_class;

void *pdp_o_new(void)
{
    int i;

    t_pdp_o *x = (t_pdp_o *)pd_new(pdp_o_class);
    x->x_connection_status = outlet_new (&x->x_obj, &s_float);
    x->x_frames = outlet_new (&x->x_obj, &s_float);
    x->x_framesd = outlet_new (&x->x_obj, &s_float);
    x->x_bandwidth = outlet_new (&x->x_obj, &s_float);

    x->x_packet0 = -1;
    x->x_queue_id = -1;

    x->x_framessent = 0;
    x->x_framerate = DEFAULT_FRAME_RATE;
    x->x_smoothing = 0;
    x->x_secondcount = 0;
    x->x_bandwidthcount = 0;
    x->x_cursec = 0;
    x->x_fd = -1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_o_setup(void)
{
    // post( pdp_o_version );
    pdp_o_class = class_new(gensym("pdp_o"), (t_newmethod)pdp_o_new,
    	(t_method)pdp_o_free, sizeof(t_pdp_o), 0, A_NULL);

    class_addmethod(pdp_o_class, (t_method)pdp_o_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_o_class, (t_method)pdp_o_connect, gensym("connect"), A_SYMBOL, A_FLOAT, A_NULL);
    class_addmethod(pdp_o_class, (t_method)pdp_o_disconnect, gensym("disconnect"), A_NULL);
    class_addmethod(pdp_o_class, (t_method)pdp_o_start, gensym("start"), A_NULL);
    class_addmethod(pdp_o_class, (t_method)pdp_o_stop, gensym("stop"), A_NULL);
    class_addmethod(pdp_o_class, (t_method)pdp_o_refresh, gensym("refresh"), A_NULL);
    class_addmethod(pdp_o_class, (t_method)pdp_o_framerate, gensym("framerate"), A_FLOAT, A_NULL);
    class_addmethod(pdp_o_class, (t_method)pdp_o_smoothing, gensym("smoothing"), A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
