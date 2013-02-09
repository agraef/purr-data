
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

/*  This object is a video streaming receiver
 *  It receives PDP packets sent by a pdp_o object
 */

#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <bzlib.h>   // bz2 decompression routines
#include "pdp.h"
#include "pdp_streaming.h"

typedef void (*t_fdpollfn)(void *ptr, int fd);
extern void sys_rmpollfn(int fd);
extern void sys_addpollfn(int fd, t_fdpollfn fn, void *ptr);

#define SOCKET_ERROR -1
#define INPUT_BUFFER_SIZE  1048578 /* 1 M */

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

/* time-out used for select() call */
static struct timeval ztout;

static char   *pdp_i_version = "pdp_i : a video stream receiver, written by ydegoyon@free.fr";

extern void sys_sockerror(char *s);

void pdp_i_closesocket(int fd)
{
    if ( close(fd) < 0 )
    {
       perror( "close" );
    }
    else
    {
       post( "pdp_i : closed socket : %d", fd );
    }
}

int pdp_i_setsocketoptions(int sockfd)
{ 
     int sockopt = 1;
     if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char*) &sockopt, sizeof(int)) < 0)
     {
	  post("pdp_i : setsockopt TCP_NODELAY failed");
          perror( "setsockopt" );
          return -1;
     }
     else
     {
	  post("pdp_i : TCP_NODELAY set");
     }
     
     if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(int)) < 0)
     {
	  post("pdp_i : setsockopt SO_REUSEADDR failed");
          perror( "setsockopt" );
          return -1;
     }
     else
     {
	  post("pdp_i : setsockopt SO_REUSEADDR done.");
     }
     return 0;
}

/* ------------------------ pdp_i ----------------------------- */

static t_class *pdp_i_class;

typedef struct _pdp_i
{
     t_object x_obj;
     int x_socket;
     t_outlet *x_connection_status;
     t_outlet *x_frames;
     t_outlet *x_connectionip;
     t_outlet *x_pdp_output;
     int x_serversocket;
     int x_framesreceived;   // total number of frames received

     void *x_inbuffer;   /* accumulation buffer for incoming frames */
     int x_inwriteposition;
     int x_inbuffersize;

       /* PDP data structures */
     int x_packet; 
     t_pdp *x_header;
     int x_vheight;
     int x_vwidth;
     int x_vsize;
     int x_psize;
     int x_hsize;
     unsigned int x_bsize;
     unsigned int x_bzsize;
     short int *x_data;
     char  *x_hdata; // huffman coded data
     char  *x_ddata; // decompressed data
     unsigned short *x_bdata; // previous data

} t_pdp_i;

 /* huffman decoding */
static int pdp_i_huffman(t_pdp_i *x, char *source, char *dest, int size, int *dsize)
{
  char *pcount=source;   
  char *pvalue=(source+1);   
 
  *dsize=0;
  while ( pcount < (source+size) )
  {
    while ( (*pcount) > 0 )
    {
      *(dest++)=*(pvalue);
      *pcount-=1;
      *dsize+=1;
    }
    pcount+=2;
    pvalue+=2;
  }

  // post( "pdp_i : dsize=%d", *dsize );
  return *dsize;
}

static void pdp_i_free_ressources(t_pdp_i *x)
{
   if ( x->x_ddata ) freebytes( x->x_ddata, x->x_psize );
   if ( x->x_hdata ) freebytes( x->x_hdata, x->x_hsize );
   if ( x->x_bdata ) freebytes( x->x_bdata, x->x_bsize );
}

static void pdp_i_allocate(t_pdp_i *x)
{
   x->x_psize = x->x_vsize + (x->x_vsize>>1);
   x->x_hsize = (x->x_vsize + (x->x_vsize>>1));
   x->x_bsize = (x->x_vsize + (x->x_vsize>>1))*sizeof(unsigned short);
   x->x_ddata = (char*) getbytes(x->x_psize);
   x->x_hdata = (char*) getbytes(x->x_hsize);
   x->x_bdata = (unsigned short*) getbytes(x->x_bsize);
   if ( !x->x_ddata || !x->x_hdata )
   {
      post( "pdp_i : severe error : could not allocate buffer" );
   }
}

static void pdp_i_recv(t_pdp_i *x)
{
   int ret, i;
   t_hpacket *pheader;

     if ( ( ret = recv(x->x_socket, (void*) (x->x_inbuffer + x->x_inwriteposition), 
                (size_t)((x->x_inbuffersize-x->x_inwriteposition-1)), 
                MSG_NOSIGNAL) ) < 0 )
     {
        post( "pdp_i : receive error" );
        perror( "recv" );
        return; 
     }
     else
     {
        // post( "pdp_i : received %d bytes at %d on %d ( up to %d)", 
        //        ret, x->x_inwriteposition, x->x_socket,
        //        x->x_inbuffersize-x->x_inwriteposition );

        if ( ret == 0 ) 
        {
           /* peer has reset connection */
           outlet_float( x->x_connection_status, 0 );
           pdp_i_closesocket( x->x_socket ); 
           sys_rmpollfn(x->x_socket);
           post( "pdp_i : lost the connection." );
           x->x_socket = -1;
        }
        else
        { 
           // check we don't overflow input buffer
           if ( x->x_inwriteposition+ret >= x->x_inbuffersize/2 )
           {
              post( "pdp_i : too much input...resetting" );
              x->x_inwriteposition=0;
              memset( (char*) x->x_inbuffer, 0x00, x->x_inbuffersize );
              return;
           }
           x->x_inwriteposition += ret;
           if ( ( ( pheader = (t_hpacket*) strstr( (char*) x->x_inbuffer, PDP_PACKET_START ) ) != NULL ) ||
                ( ( pheader = (t_hpacket*) strstr( (char*) x->x_inbuffer, PDP_PACKET_DIFF ) ) != NULL ) )
           {
             // check if a full packet is present
             if ( x->x_inwriteposition >= 
                  (int)((char*)pheader - (char*)(x->x_inbuffer)) +
                        (int)sizeof(t_hpacket) + (int)ntohl(pheader->clength) )
             {
                if ( ( x->x_vwidth != (int)ntohl(pheader->width) ) ||
                     ( x->x_vheight != (int)ntohl(pheader->height) ) )
                {
                   pdp_i_free_ressources(x);
                   x->x_vheight = ntohl(pheader->height);
                   x->x_vwidth = ntohl(pheader->width);
                   x->x_vsize = x->x_vheight*x->x_vwidth;
                   pdp_i_allocate(x);
                   post( "pdp_i : allocated buffers : vsize=%d : hsize=%d", x->x_vsize, x->x_hsize );
                }

                x->x_packet = pdp_packet_new_image_YCrCb( x->x_vwidth, x->x_vheight );
                x->x_header = pdp_packet_header(x->x_packet);
                x->x_data = (short int *)pdp_packet_data(x->x_packet);
                memcpy( x->x_data, x->x_bdata, x->x_bsize );

                // post( "pdp_i : decompress %d in %d bytes", ntohl(pheader->clength), x->x_hsize );
                x->x_bzsize = x->x_hsize;

                if ( ( ret = BZ2_bzBuffToBuffDecompress( (char*)x->x_hdata,
                                            &x->x_bzsize,
					    (char *) pheader+sizeof(t_hpacket),
					    ntohl(pheader->clength),
					    0, 0 ) ) == BZ_OK ) 
                {
                     // post( "pdp_i : bz2 decompression (%d)->(%d)", ntohl(pheader->clength), x->x_bzsize );

                     switch( ntohl(pheader->encoding) )
                     {
                        case REGULAR :
                          memcpy( x->x_ddata, x->x_hdata, x->x_bzsize ); 
                          break;

                        case HUFFMAN :
                          pdp_i_huffman( x, x->x_hdata, x->x_ddata, x->x_bzsize, &x->x_psize );
                          break;
                     }

                     for ( i=0; i<x->x_vsize; i++ )
                     {
                       if ( !strcmp( pheader->tag, PDP_PACKET_TAG ) )
                       {
                         x->x_data[i] = x->x_ddata[i]<<7;
                       }
                       else
                       {
                         if ( x->x_ddata[i] != 0 )
                         {
                            x->x_data[i] = x->x_ddata[i]<<7;
                         }
                       }
                     }
                     for ( i=x->x_vsize; i<(x->x_vsize+(x->x_vsize>>1)); i++ )
                     {
                       if ( !strcmp( pheader->tag, PDP_PACKET_TAG ) )
                       {
                         x->x_data[i] = (x->x_ddata[i])<<8;
                       }
                       else
                       {
                         if ( x->x_ddata[i] != 0 )
                         {
                            x->x_data[i] = (x->x_ddata[i])<<8;
                         }
                       }
                     }

                     x->x_header->info.image.encoding = PDP_IMAGE_YV12;
                     x->x_header->info.image.width = x->x_vwidth;
                     x->x_header->info.image.height = x->x_vheight;

                     // post( "pdp_i : propagate packet : %d", x->x_packet );
		     pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet); 
                     outlet_float( x->x_frames, ++x->x_framesreceived );
                }
                else
                {
                     post( "pdp_i : bz2 decompression failed (ret=%d)", ret );
                }

                memcpy( x->x_bdata, x->x_data, x->x_bsize );

                // roll buffer
                x->x_inwriteposition -= (int)((char*)pheader-(char*)(x->x_inbuffer)) + sizeof(t_hpacket) + ntohl(pheader->clength);
                if ( x->x_inwriteposition > 0 )
                {
                  memcpy( x->x_inbuffer, pheader+sizeof(t_hpacket) + ntohl(pheader->clength), x->x_inwriteposition );
                }
                else
                {
                  x->x_inwriteposition = 0;
                  memset( (char*) x->x_inbuffer, 0x00, x->x_inbuffersize );
                }
             }
             else
             {
                // post( "pdp_i : not a full frame" );
             }
           }
        }
             
     }
}

static void pdp_i_acceptconnection(t_pdp_i *x)
{
    struct sockaddr_in incomer_address;
    unsigned int sockaddrl;

    int fd = accept(x->x_serversocket, (struct sockaddr*)&incomer_address, &sockaddrl );

    if (fd < 0) {
	 post("pdp_i : accept failed");
	 return;
    }

    if ( x->x_socket > 0 )
    {
       post( "pdp_i : accepting a new source : %s", inet_ntoa( incomer_address.sin_addr) );
       pdp_i_closesocket( x->x_socket );
       sys_rmpollfn(x->x_socket);
       outlet_float( x->x_connection_status, 0 );
    }

    x->x_socket = fd;
    x->x_framesreceived = 0;
    sys_addpollfn(x->x_socket, (t_fdpollfn)pdp_i_recv, x);
    post("pdp_i : new source : %s.", inet_ntoa( incomer_address.sin_addr ));
    outlet_float( x->x_connection_status, 1 );
    outlet_float( x->x_frames, x->x_framesreceived );
    outlet_symbol( x->x_connectionip, gensym( inet_ntoa( incomer_address.sin_addr) ) );

}


static int pdp_i_startservice(t_pdp_i* x, int portno)
{
    struct sockaddr_in server;
    int sockfd;

    /* create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
    	sys_sockerror("socket");
    	return (0);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    /* assign server port number */
    server.sin_port = htons((u_short)portno);
    post("listening to port number %d", portno);

    pdp_i_setsocketoptions(sockfd);

    /* name the socket */
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
	 sys_sockerror("bind");
	 pdp_i_closesocket(sockfd);
	 return (0);
    }

    if (listen(sockfd, 5) < 0) {
      sys_sockerror("listen");
      pdp_i_closesocket(sockfd);
    }
    else 
    {
      x->x_serversocket = sockfd;
      sys_addpollfn(x->x_serversocket, (t_fdpollfn)pdp_i_acceptconnection, x);
    }
    
    return 1;
}

static void pdp_i_free(t_pdp_i *x)
{
     post( "pdp_i : free %x", x );
     if (x->x_serversocket > 0) {
        post( "pdp_i : closing server socket" );
	pdp_i_closesocket(x->x_serversocket);
        sys_rmpollfn(x->x_serversocket);
        x->x_serversocket = -1;
     }
     if (x->x_socket > 0) {
        post( "pdp_i : closing socket" );
        pdp_i_closesocket(x->x_socket);
        sys_rmpollfn(x->x_socket);
        x->x_socket = -1;
     }
     if ( x->x_inbuffer ) freebytes( x->x_inbuffer, x->x_inbuffersize );
     pdp_i_free_ressources( x );
}

static void *pdp_i_new(t_floatarg fportno)
{
    t_pdp_i *x;
    int i;
    
    if ( fportno <= 0 || fportno > 65535 )
    {
       post( "pdp_i : error : wrong portnumber : %d", (int)fportno );
       return NULL;
    }

    x = (t_pdp_i *)pd_new(pdp_i_class);
    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything);
    x->x_connection_status = outlet_new(&x->x_obj, &s_float);
    x->x_frames = outlet_new(&x->x_obj, &s_float);
    x->x_connectionip = outlet_new(&x->x_obj, &s_symbol);
    
    x->x_serversocket = -1;

    x->x_inbuffersize = INPUT_BUFFER_SIZE;
    x->x_inbuffer = (char*) getbytes( x->x_inbuffersize );
    memset( x->x_inbuffer, 0x0, INPUT_BUFFER_SIZE );

    if ( !x->x_inbuffer )
    {
       post( "pdp_i : could not allocate buffer." );
       return NULL;
    }

    x->x_inwriteposition = 0;
    x->x_socket = -1;
    x->x_packet = -1;
    x->x_ddata = NULL;
    x->x_hdata = NULL;
    x->x_bdata = NULL;
    
    ztout.tv_sec = 0;
    ztout.tv_usec = 0;

    post( "pdp_i : starting service on port %d", (int)fportno );
    pdp_i_startservice(x, (int)fportno);

    return (x);
}


void pdp_i_setup(void)
{
    // post( pdp_i_version );
    pdp_i_class = class_new(gensym("pdp_i"), 
    	(t_newmethod) pdp_i_new, (t_method) pdp_i_free,
    	sizeof(t_pdp_i),  CLASS_NOINLET, A_DEFFLOAT, A_DEFFLOAT, A_NULL);


}
