/* ------------------------- shoutamp~ ---------------------------------------- */
/*                                                                              */
/* Tilde object to receive an mp3-stream from a shoutcast/icecast server.       */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>                                */
/* Graphical buffer status display by Yves Degoyon <ydegoyon@free.fr>.          */
/* Get source at http://www.akustische-kunst.org/puredata/shout/                */
/*                                                                              */
/* This library is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU Lesser General Public                   */
/* License as published by the Free Software Foundation; either                 */
/* version 2 of the License, or (at your option) any later version.             */
/*                                                                              */
/* This library is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            */
/* Lesser General Public License for more details.                              */
/*                                                                              */
/* You should have received a copy of the GNU Lesser General Public             */
/* License along with this library; if not, write to the                        */
/* Free Software Foundation, Inc., 59 Temple Place - Suite 330,                 */
/* Boston, MA  02111-1307, USA.                                                 */
/*                                                                              */
/* Based on PureData by Miller Puckette and others.                             */
/* Uses the LAME MPEG 1 Layer 3 encoding library which can be found at          */
/* http://www.mp3dev.org                                                        */
/*                                                                              */
/* ---------------------------------------------------------------------------- */

	/* Pd includes */
#include "m_pd.h"
#include "g_canvas.h"
	/* mpg123 includes (from Lame 3.92) */
#include "mpg123.h"
#include "mpglib.h"
#include "interface.h"

#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#ifdef UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#define SOCKET_ERROR -1
#else
#include <io.h>	/* for 'write' in pute-function only */
#include <winsock.h>
#include <winbase.h>
#endif

#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#ifndef NT
#define     sys_closesocket close
#define		STRDUP strdup
#else	/* if NT */
#define     sys_closesocket closesocket
#define     STRDUP _strdup
#endif


/************************* shoutamp~ object ******************************/

/*
	Each instance of shoutamp~ owns a "child" thread for doing the data
	transfer.  The parent thread signals the child each time:
		(1) a connection wants opening or closing;
		(2) we've eaten another 1/16 of the shared buffer (so that the
    		child thread should check if it's time to receive some more.)
	The child signals the parent whenever a receive has completed.  Signalling
	is done by setting "conditions" and putting data in mutex-controlled common
	areas.
*/


#define     REQUEST_NOTHING 0
#define     REQUEST_CONNECT 1
#define     REQUEST_CLOSE 2
#define     REQUEST_QUIT 3
#define     REQUEST_BUSY 4
#define     REQUEST_DATA 5
#define     REQUEST_RECONNECT 6

#define     STATE_IDLE 0
#define     STATE_STARTUP 1                     /* connecting and filling the buffer */
#define     STATE_STREAM 2                      /* streaming aund audio output */

#define     READSIZE                131072      /* size of mp3 stream receive buffer */
#define     READ                    4096        /* amount of mp3 data we pass on to decoder */
#define     DECODE_PACKET_SIZE      32768       /* max size of the data returned by mpglib : 32k */
#define     DEFBUFPERCHAN           262144      /* audio output buffer default: 256k */
#define     MINBUFSIZE              (2 * DECODE_PACKET_SIZE)	/* minimum buffer size (64k) */
#define     MAXBUFSIZE              16777216 	/* arbitrary; just don't want to hang malloc */
#define     STRBUF_SIZE             1024        /* char received from server on startup */
#define     OBJWIDTH 			    68			/* width of buffer status display */
#define     OBJHEIGHT 			    10 			/* height of buffer status display */
#define		MAXSTREAMCHANS          2           /* maximum number of channels: restricted to 2 by now */
#define     MAX_DECODERS            10          /* maximum number of decoders / object instances */

    /* useful debugging functions from mpglib */
extern int decode_header( struct frame* fr, unsigned long newhead );
extern void print_header_compact( struct frame* fr );
extern int head_check( unsigned long head, int check_layer );


static char   *shoutamp_version = "shoutamp~: mp3 streaming client v0.5, written by Olaf Matthes";

/* too bad, this needs to be static,
   handling an array to enable several decoders in pd */
static  MPSTR   mps[MAX_DECODERS];  /* decoder buffer */
int nbinstances = 0; 

extern const long  freqs[9];		/* the samplerate taken from mpg123 */


static t_class *shoutamp_class;

typedef struct _shoutamp
{
    t_object x_obj;
	t_int    x_instance;       /* instance of the object */
    t_canvas *x_canvas;        /* remember canvas */
    t_outlet *x_connection;
	t_clock  *x_clock;

    t_float *x_buf;    	    	    	    /* audio data buffer (at samplerate of stream) */
    t_float *x_rsbuf;  	    	    	    /* resampled audio data buffer */
    t_int x_bufsize;  	    	    	    /* buffer size in bytes */
    t_int x_noutlets; 	    	    	    /* number of audio outlets */
    t_sample *(x_outvec[MAXSTREAMCHANS]);	/* audio vectors */
    t_int x_vecsize;  	    	    	    /* vector size for transfers */
    t_int x_state;    	    	    	    /* opened, running, or idle */
	char x_out[DECODE_PACKET_SIZE];			/* buffer for decoded PCM samples */

    	/* parameters to communicate with subthread */
    t_int x_requestcode;	   /* pending request from parent to I/O thread */
    t_int x_connecterror;	   /* slot for "errno" return */
    t_int x_streamchannels;	   /* number of channels in Ogg mpg123 bitstream */
	t_int x_streamrate;        /* sample rate of stream */

		/* buffer stuff */
    t_int x_fifosize; 	       /* buffer size appropriately rounded down */	    
    t_int x_fifohead; 	       /* index of next byte to get from file */
    t_int x_fifotail; 	       /* index of next byte the ugen will read */
	t_int x_fifobytes;         /* number of bytes available in buffer */
    t_int x_eof;   	           /* true if ogg stream has ended */
    t_int x_sigcountdown;      /* counter for signalling child for more data */
    t_int x_sigperiod;	       /* number of ticks per signal */
	t_int x_siginterval;       /* number of times per buffer (depends on data rate) */

		/* mpg123 related stuff */
	t_int    x_eos;            /* end of stream */
	t_int    x_bitrateindex;   /* bitrate index for each frame, might change dynamically */ 
	t_int    x_packetsize;     /* size of the packets */
	char     x_mp3buffer[READSIZE];	/* input buffer for mpg123 */
	t_int    x_mp3inpos;       /* writing position in mp3 stream buffer */
    t_int    x_mpg123;         /* info about encoder status */
	t_int    x_verbose;        /* turn mpg123 error printout on or off */


	t_int    x_connectstate;   /* indicates the state of socket connection */
    t_int    x_fd;             /* the socket number */
    t_int    x_graphic;        /* indicates if we show a graphic bar */ 
	t_int    x_resample;       /* indicates if we need to resample signal (1 = no resampling) */
	t_int    x_recover;        /* indicate how to behave on buffer underruns */
	t_int    x_disconnect;     /* indicates that user want's to disconnect */
    t_int    x_samplerate;     /* Pd's sample rate */
	t_int    x_bitrate;

		/* server stuff */
	char    *x_hostname;       /* name or IP of host to connect to */
	char    *x_mountpoint;     /* mountpoint of ogg-bitstream */
	t_int    x_port;           /* port number on which the connection is made */

		/* tread stuff */
    pthread_mutex_t   x_mutex;
    pthread_cond_t    x_requestcondition;
    pthread_cond_t    x_answercondition;
    pthread_t         x_childthread;
} t_shoutamp;

	/* some prototypes */
static void shoutamp_child_disconnect(t_int fd);

	/* check if we can read from socket */
static int shoutamp_check_for_data(t_int sock)
{

	fd_set set;
	struct timeval tv;
	t_int ret;

	tv.tv_sec = 0;
	tv.tv_usec = 20000;
	FD_ZERO(&set);
	FD_SET(sock, &set);
	ret = select(sock + 1, &set, NULL, NULL, &tv);
	if (ret > 0)
		return 1;
	return 0;
}

	/* receive 'size' bytes from server */
static int shoutamp_child_receive(int fd, char *rcvbuffer, int size)
{
	int   ret = -1;
	int   i;
 
	ret = recv(fd, rcvbuffer, size, 0);
	if(ret < 0)
	{
		post("shoutamp~: receive error" );
	}
	return ret;
}

static int strip_shout_header(char *head, int n)
{
    int i;
    for (i = 0; i < (n - 2); i++)
    {
        if (head[i] == 10 && head[i + 1] == 13)
            break;
        if (head[i] == '\n' && head[i + 1] == '\n')
            break;
    }
    head[i + 1] = '\0';
    return n - (i + 1);
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

	/* mpg123 decoder setup */
static int shoutamp_mpg123_init(t_shoutamp *x, int fd)
{
	int offset, bytes;
	float resample = 0;
	struct frame hframe;
	unsigned int a,b,c,d;
	unsigned long cheader;

    InitMP3(&mps[x->x_instance]);

		/* receive some bytes and check them */
	bytes = shoutamp_child_receive(fd, x->x_mp3buffer, READ);
	if(bytes < 0)
	{
		return (bytes);
	}

	offset=0;
		/* search for an header to check dynamic bitrate */
	while(offset < READ - 4)
	{
			/* decode first 4 bytes as the header */
        a = *((unsigned char*)x->x_mp3buffer+offset);
        b = *((unsigned char*)x->x_mp3buffer+offset+1);
        c = *((unsigned char*)x->x_mp3buffer+offset+2);
        d = *((unsigned char*)x->x_mp3buffer+offset+3);
   
        cheader = 0;
        cheader = a;
        cheader <<= 8;
        cheader |= b;
        cheader <<= 8;
        cheader |= c; 
        cheader <<= 8;
        cheader |= d;
        if ( head_check( cheader, 0 ) )
        { 
			decode_header( &hframe, cheader );
            print_header_compact( &hframe );
			x->x_packetsize = hframe.framesize+sizeof(unsigned long);
			x->x_streamchannels = hframe.stereo;	/* how many channels ? */
			if ( hframe.framesize == 0 )
			{
				post( "shoutamp~: weird header ( frame size = 0 ) .... ignored" );
				offset++;
				continue;
			}
				// when the bitrate change, reinit decoder
			x->x_bitrateindex = hframe.bitrate_index;
				/* setting for resampling */
			x->x_streamrate = freqs[hframe.sampling_frequency];
			if(x->x_streamrate != x->x_samplerate)
			{
				if(x->x_streamrate > 0)
				resample = (float)x->x_samplerate / (float)x->x_streamrate;
				if(resample == 1.0)x->x_resample = 1;
				else if(resample == 2.0)x->x_resample = 2;
				else if(resample == 4.0)x->x_resample = 4;
				else if(resample == 3.0)x->x_resample = 3;
				else x->x_resample = -1;
				if(x->x_resample==-1)
				{
					post("shoutamp~: resampling from %ld Hz to %d not supported", 
					x->x_streamrate, x->x_samplerate );
					return -1;
				}
				else post("shoutamp~: resampling from %ld Hz to %d Hz", x->x_streamrate, x->x_samplerate );
			} 
			else x->x_resample = 1;
			break;
        }
        offset++;
	}
	x->x_mp3inpos = READ - offset;	/* throw away first few bytes */
	memcpy(x->x_mp3buffer, x->x_mp3buffer+offset, x->x_mp3inpos);	/* move start of header to start of buffer */

		/* read next 4k of data */
	bytes = shoutamp_child_receive(fd, x->x_mp3buffer+x->x_mp3inpos, READ);
	if(bytes < 0)
	{
		x->x_eos = 1;	/* indicate end of stream */
		return (bytes);
	}
	else x->x_mp3inpos += bytes;
	
	x->x_mpg123 = 1;	/* indicate that mpg123 is OK */
	return (1);
}

	/* deinit the mpg123 decoder */
static void shoutamp_mpg123_deinit(t_shoutamp *x)
{
	if(x->x_mpg123)
	{
		x->x_mpg123 = 0;
		ExitMP3(&mps[x->x_instance]);
		post("shoutamp~: decoder deinitialised");
	}
}

	/* decode ogg/mpg123 and receive new data */
static int shoutamp_decode_input(t_shoutamp *x, float *buf, int fifohead, int fifosize, int fd)
{
	int i, j, ch, ret;
	
	signed short int *p = (signed short int *) x->x_out;
	int samples = 0;/* number of samples returned by decoder at each block! */
	int channels;   /* number of channels */
	int n = 0;      /* total number of samples returned by decoder at this call */
	int bytes;		/* number of bytes submitted to decoder */
	int position = fifohead;
	int pbytes;
	struct frame hframe;
	unsigned int a,b,c,d;
	unsigned long cheader;
	int offset;

	while(x->x_mp3inpos > READ)	/* at least two packets in buffer */
	{
		offset = 0;
			/* search for an header to check dynamic bitrate */
		while(offset < x->x_mp3inpos - 4)
		{
				/* decode first 4 bytes as the header */
			a = *((unsigned char*)x->x_mp3buffer+offset);
			b = *((unsigned char*)x->x_mp3buffer+offset+1);
			c = *((unsigned char*)x->x_mp3buffer+offset+2);
			d = *((unsigned char*)x->x_mp3buffer+offset+3);

			cheader = 0;
			cheader = a;
			cheader <<= 8;
			cheader |= b;
			cheader <<= 8;
			cheader |= c; 
			cheader <<= 8;
			cheader |= d;
			if(head_check( cheader, 0 ))	/* is this a header ? */
			{ 
				decode_header(&hframe, cheader);
				x->x_packetsize = hframe.framesize + (int)sizeof(unsigned long);
				if(hframe.framesize == 0)
				{
					post( "shoutamp~: weird header ( frame size = 0 ) .... ignored" );
					offset++;
					continue;
				}
				// print_header_compact( &hframe );
				/* when the bitrate changes, reinit decoder */
				if(x->x_bitrateindex != hframe.bitrate_index)
				{
					post("shoutamp~: reinitialize decoder (bitrate changed)");
					ExitMP3(&mps[x->x_instance]);
					InitMP3(&mps[x->x_instance]);
					x->x_bitrateindex = hframe.bitrate_index;
				}
				break;
			}
			offset++;
		}

		ret = decodeMP3(&mps[x->x_instance], (unsigned char*)(x->x_mp3buffer+offset),
							x->x_packetsize, (char *)p, sizeof(x->x_out), &pbytes);

		channels = mps[x->x_instance].fr.stereo;	/* get current number of channels */

		switch (ret) 
		{
			case MP3_OK:
				switch (channels)
				{
					case 1:
					case 2:
						samples = ((channels==1)?pbytes >> 1 : pbytes >> 2);
							/* copy decoded PCM into our output buffer */
						pthread_mutex_lock(&x->x_mutex);
						for(j = 0; j < samples; j++)
						{
							for(ch = 0; ch < channels; ch++)
							{
								buf[position] = ((t_float)(*p++))/32767.0;
								position++;
							}
							if (position >= fifosize)	/* check for buffer boundaries */
								position = 0;
						}
						pthread_mutex_unlock(&x->x_mutex);
						n += (samples * channels);	/* count bytes we wrote to buffer */
						p -= (samples * channels);	/* point to start of buffer again */

							/* move unused data to start of buffer ("roll buffer") */
						if(x->x_mp3inpos > x->x_packetsize) 
						{
							x->x_mp3inpos -= x->x_packetsize + offset;	/* move one frame */
							memcpy( (void *)(x->x_mp3buffer), 
								(void *)(x->x_mp3buffer+x->x_packetsize + offset),
								x->x_mp3inpos);
						}
						else /* sorry, it will be ignored */
						{
							/* error( "shoutamp~: incomplete frame...ignored");
							x->x_mp3inpos = 0; */
							goto receive;
						}
						break;
					default:
						samples = 0;	/* nothing added to buffer */
						break;
				}
				break;
     
			case MP3_NEED_MORE:
				if(mps[x->x_instance].framesize == 0 && mps[x->x_instance].fsizeold != 0) 
				{
					post( "shoutamp~: decoding done");
				}
				else
				{
					post( "shoutamp~: retry lame decoding (more data needed)" );
					goto receive;
				}
				break;
     
			case MP3_ERR:
				post( "shoutamp~: lame decoding failed" );
				return (-1);
				break;
     
		}
	}
    
receive:
		/* read data from socket */
	if(x->x_mp3inpos < READSIZE - READ)	/* any space in buffer ? */
	{
		if(!x->x_eos)	/* read data from input */
		{
				/* read next 4k of data out of buffer */
			bytes = shoutamp_child_receive(fd, x->x_mp3buffer+x->x_mp3inpos, READ);
			if(bytes < 0)
			{
				x->x_eos = 1;	/* indicate end of stream */
				return (bytes);
			}
			else x->x_mp3inpos += bytes;
		}
		else			/* we read through all the file... */
		{				/* will have to reinit decoder for new file */
			post("shoutamp~: end of stream detected");
			return (0);
		}
	}
	else	/* no? - drop the whole thing */
	{
		post("shoutamp~: mp3 stream buffer dropped");
		x->x_mp3inpos = 0;
		shoutamp_mpg123_init(x, fd);
	}
	// post("n = %d", n);
	return (n);
}
  
    /* connect to shoutcast server */
static int shoutamp_child_connect(char *hostname, char *mountpoint, t_int portno)
{
    struct          sockaddr_in server;
    struct          hostent *hp;

        /* variables used for communication with server */
    char            *sptr = NULL;
    char            request[STRBUF_SIZE];           /* string to be send to server */
	char            *url;           /* used for relocation */
	char*			bcname;         /* name of broadcast */
	char*			bcurl;          /* url of broadcast */
	char*			bcgenre;        /* genre of broadcast */
	char*			bcaim;          /* aim of broadcast */
    fd_set          fdset;
    struct timeval  tv;
    t_int           sockfd;                         /* socket to server */
    t_int           relocate, numrelocs = 0;
    t_int           i, ret, rest, nanswers=0;
    char            *cpoint = NULL;
	t_int           eof = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        error("shoutamp~: internal error while attempting to open socket");
        return (-1);
    }

        /* connect socket using hostname provided in command line */
    server.sin_family = AF_INET;
    hp = gethostbyname(hostname);
    if (hp == 0)
    {
        post("shoutamp~: bad host?");
        sys_closesocket(sockfd);
        return (-1);
    }
    memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);

        /* assign client port number */
    server.sin_port = htons((unsigned short)portno);

       /* check mountpoint :: SHOUcast does not support mountpoints */
    if(strstr(mountpoint, "listen.pls") || strstr(mountpoint, "/"))
    {
               mountpoint = "";	/* the '/' is not part of the mountpoint !!! */
    }

        /* try to connect.  */
    post("shoutamp~: connecting to http://%s:%d/%s", hostname, portno, mountpoint);
    if (connect(sockfd, (struct sockaddr *) &server, sizeof (server)) < 0)
    {
        error("shoutamp~: connection failed!\n");
        sys_closesocket(sockfd);
        return (-1);
    }

        /* sheck if we can read/write from/to the socket */
    FD_ZERO( &fdset);
    FD_SET( sockfd, &fdset);
    tv.tv_sec  = 0;            /* seconds */
    tv.tv_usec = 500;        /* microseconds */

    ret = select(sockfd + 1, &fdset, NULL, NULL, &tv);
    if(ret != 0)
    {
        error("shoutamp~: can not read from socket");
        sys_closesocket(sockfd);
        return (-1);
    }

       /* build up stuff we need to send to server */
    sprintf(request, "GET /%s HTTP/1.0 \r\nHost: %s\r\nUser-Agent: shoutamp~ 0.5\r\nAccept: */*\r\n\r\n", 
            mountpoint, hostname);

    if(send(sockfd, request, strlen(request), 0) < 0)    /* say hello to server */
    {
       post("shoutamp~: could not contact server... ");
#ifndef NT
       perror("send");
#endif
       return (-1);
    }

    relocate = FALSE;

    if( recv(sockfd, request, STRBUF_SIZE, 0)<0)
    {
        error("shoutamp~: no response from server");
#ifndef NT
        perror( "recv" );
#endif
		shoutamp_child_disconnect(sockfd);
        return (-1);
    }
        
        /* time to parse content of the response... */
    if(strstr(request, "audio/x-scpls") )    /* SHOUTcast playlist */
    {
		/* LATER: parse playlist and retry */
		post("shoutamp~: SHOUTcast server returned a playlist, quitting");
		shoutamp_child_disconnect(sockfd);
		return (-1);
    } 
    if(strstr(request, "HTTP"))    /* seems to be IceCast server */
    {
        strip_ice_header(request, STRBUF_SIZE);    
        if(sptr = strstr(request, "302")) 
        {
            cpoint = strstr(request, "Location:");
            url = STRDUP(cpoint + 10);
            post("shoutamp~: relocation requested, try \"%s\" instead", url);
            shoutamp_child_disconnect(sockfd);
            // return (shoutamp_connect_url(x, gensym(url)));
            relocate = TRUE;
			return (-1);
        }
        if(!(sptr = strstr(request, "200")) && !relocate ) 
        {
            error("shoutamp~: cannot connect to the (default) stream");
			shoutamp_child_disconnect(sockfd);
            return (-1);
        }

        post("shoutamp~: IceCast server detected");

            /* eventually read the rest of server's announcement */
        while((!(cpoint = strstr(request, "x-audiocast-public") ) ||
               !(cpoint = strstr(request, "x-audiocast-mount") ) ||
               !(cpoint = strstr(request, "x-audiocast-server-url") ) ||
               !(cpoint = strstr(request, "x-audiocast-location") ) ||
               !(cpoint = strstr(request, "x-audiocast-admin") ) ||
               !(cpoint = strstr(request, "x-audiocast-name") ) ||
               !(cpoint = strstr(request, "x-audiocast-genre") ) ||
               !(cpoint = strstr(request, "x-audiocast-url") ) ||
               !(cpoint = strstr(request, "x-audiocast-public") ) ||
               !(cpoint = strstr(request, "x-audiocast-bitrate") ) ) &&
               ( ret < STRBUF_SIZE ) )
        {
			if((rest = recv(sockfd, request+ret, STRBUF_SIZE-ret, 0) ) < 0 )
			{
				error("shoutamp~: no response from server");
#ifndef NT
				perror("recv");
#endif
				shoutamp_child_disconnect(sockfd);
				return(-1);
			}
			ret += rest;
        }

        // post("shoutamp~: server's header : %s", request );

             /* check what we got */
        if(cpoint = strstr(request, "x-audiocast-mount:"))
        {
             mountpoint = STRDUP(cpoint + 18);
             for ( i=0; i<(int)strlen(mountpoint); i++ )
             {
                if(mountpoint[i] == '\n' )
                {
                   mountpoint[i] = '\0';
                   break;
                }    
             }
             post("           mountpoint: %s", mountpoint);
        }
        if(cpoint = strstr(request, "x-audiocast-server-url:"))
        {
             sptr = STRDUP( cpoint + 24);
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
         if(cpoint = strstr(request, "x-audiocast-location:"))
         {
             sptr = STRDUP( cpoint + 22);
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
         if(cpoint = strstr(request, "x-audiocast-admin:"))
         {
             sptr = STRDUP( cpoint + 19);
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
         if(cpoint = strstr(request, "x-audiocast-name:"))
         {
             bcname = STRDUP( cpoint + 17);
             for ( i=0; i<(int)strlen(bcname); i++ )
             {
                if ( bcname[i] == '\n' )
                {
                   bcname[i] = '\0';
                   break;
                }    
             }
             post("           name: %s", bcname);
         }
         if(cpoint = strstr(request, "x-audiocast-genre:"))
         {
             bcgenre = STRDUP( cpoint + 18);
             for ( i=0; i<(int)strlen(bcgenre); i++ )
             {
                if ( bcgenre[i] == '\n' )
                {
                   bcgenre[i] = '\0';
                   break;
                }    
             }
             post("           genre: %s", bcgenre);
         }
         if(cpoint = strstr(request, "x-audiocast-url:"))
         {
             bcurl = STRDUP( cpoint + 16);
             for ( i=0; i<(int)strlen(bcurl); i++ )
             {
                if ( bcurl[i] == '\n' )
                {
                   bcurl[i] = '\0';
                   break;
                }    
             }
             post("           url: %s", bcurl);
         }
         if(cpoint = strstr(request, "x-audiocast-public:1"))
         {
             post("           broadcast is public");
         }
         else if(cpoint = strstr(request, "x-audiocast-public:0"))
         {
             post("           broadcast is NOT public");
         }
         if(cpoint = strstr(request, "x-audiocast-bitrate:"))
         {
             sptr = STRDUP( cpoint + 20);
             for ( i=0; i<(int)strlen(sptr); i++ )
             {
                if ( sptr[i] == '\n' )
                {
                   sptr[i] = '\0';
                   break;
                }    
             }
             post("           bitrate: %s", sptr);
         }
         if(cpoint = strstr(request, "x-audiocast-udpport:"))
         {
             post("shoutamp~: sorry, server wants UDP connection!");
             return (-1);
         } 
    }
    else    /* it is a SHOUTcast server */
    {
        strip_shout_header (request, STRBUF_SIZE);
        post("shoutamp~: SHOUTcast server detected");
        if(strncmp(request, "ICY 401", 7) == 0)
        {
            post("shoutamp~: ICY 401 Service Unavailable");
            return (-1);
        }
        if(strncmp(request, "ICY 200 OK", 10) == 0)
        {
                /* recv and decode info about broadcast line by line */
            post("shoutamp~: connecting to stream...");
            i = ret;
                /* break response down to single lines */
            while(i < STRBUF_SIZE - 1)
            {
                for(rest = ret; rest < STRBUF_SIZE - 1; rest++)
                {
                    if(recv(sockfd, request+rest, 1, 0)!=1)
                        break;
                    if (request[rest] == '\n')
                        break;
                    if (request[rest] == '\r')
                        break;
                    i++;
                }
                request[rest] = '\0';

                    /* check what we got */
                if(!strncmp(request, "icy-name:", 9))
                {
                    bcname = STRDUP(request + 9);
                    post("           name: %s", bcname);
                }
                else if(!strncmp(request, "x-audiocast-name:", 17))
                {
                    bcname = STRDUP(request + 17);
                    post("           name: %s", bcname);
                }
                if(!strncmp(request, "icy-genre:", 9))
                {
                    bcgenre = STRDUP(request + 10);
                    post("           genre: %s", bcgenre);
                }
                if(!strncmp(request, "icy-aim:", 7))
                {
                    bcaim = STRDUP(request + 8);
                    post("           aim: %s", bcaim);
                }
                if(!strncmp(request, "icy-url:", 7))
                {
                    bcurl = STRDUP(request + 8);
                    post("           url: %s", bcurl);  
                }
                if(!strncmp(request, "icy-pub:1", 8))
                {
                    post("           broadcast is public");
                }
                else if(!strncmp(request, "icy-pub:0", 8))
                {
                    post("           broadcast is NOT public");
                }
                if(!strncmp(request, "icy-br:", 6))
                {
                    sptr = STRDUP(request + 7);
                    post("           bitrate: %s", sptr);
                }
                if(!strncmp(request, "x-audiocast-udpport:", 20))
                {
                    post("shoutamp~: sorry, server wants UDP connection!");
                    return (-1);
                } 

            }
        }
        else 
        {
            post("shoutamp~: unknown response from server");
            return (-1);
        }
        relocate = FALSE;    
    }
    if(relocate)
	{
        error("shoutamp~: too many HTTP relocations");
        return (-1);
    }
    post("shoutamp~: connected to http://%s:%d/%s", hp->h_name, portno, mountpoint);

	return (sockfd);
}


static void shoutamp_child_dographics(t_shoutamp *x)
{
		/* do graphics stuff :: create rectangle */
    if ( x->x_graphic && glist_isvisible( x->x_canvas ) )
    {
		sys_vgui(".x%x.c create rectangle %d %d %d %d -fill lightblue -tags %xPBAR\n",
                  x->x_canvas, x->x_obj.te_xpix, x->x_obj.te_ypix-OBJHEIGHT-1,
                  x->x_obj.te_xpix + OBJWIDTH, x->x_obj.te_ypix - 1, x );
    } 
}

static void shoutamp_child_updategraphics(t_shoutamp *x)
{
		/* update buffer status display */
	if(x->x_graphic && glist_isvisible(x->x_canvas))
	{
			/* update graphical read status */
		char color[32];

		sys_vgui(".x%x.c delete rectangle %xSTATUS\n", x->x_canvas, x); 
		if(x->x_fifobytes < (x->x_fifosize / 8))
		{
			strcpy(color, "red");
		}
		else
		{
			strcpy(color, "lightgreen");
		}
		sys_vgui(".x%x.c create rectangle %d %d %d %d -fill %s -tags %xSTATUS\n",
		x->x_canvas, x->x_obj.te_xpix, x->x_obj.te_ypix-OBJHEIGHT-1,
		x->x_obj.te_xpix+((x->x_fifobytes*OBJWIDTH)/x->x_fifosize),
		x->x_obj.te_ypix - 1, color, x);
	}
}
static void shoutamp_child_delgraphics(t_shoutamp *x)
{
    if(x->x_graphic)			/* delete graphics */
    {
       sys_vgui(".x%x.c delete rectangle %xPBAR\n", x->x_canvas, x );
       sys_vgui(".x%x.c delete rectangle %xSTATUS\n", x->x_canvas, x ); 
    }
}

static void shoutamp_child_disconnect(t_int fd)
{
    sys_closesocket(fd);
	post("shoutamp~: connection closed");
}

/************** the child thread which performs data I/O ***********/

#if 0			/* set this to get debugging output */
static void pute(char *s)   /* debug routine */
{
    write(2, s, strlen(s));
}
#else
#define pute(x)
#endif

#if 1
#define shoutamp_cond_wait pthread_cond_wait
#define shoutamp_cond_signal pthread_cond_signal
#else
#include <sys/time.h>    /* debugging version... */
#include <sys/types.h>
static void shoutamp_fakewait(pthread_mutex_t *b)
{
    struct timeval timout;
    timout.tv_sec = 0;
    timout.tv_usec = 1000000;
    pthread_mutex_unlock(b);
    select(0, 0, 0, 0, &timout);
    pthread_mutex_lock(b);
}

void shoutamp_banana( void)
{
    struct timeval timout;
    timout.tv_sec = 0;
    timout.tv_usec = 200000;
    pute("banana1\n");
    select(0, 0, 0, 0, &timout);
    pute("banana2\n");
}


#define shoutamp_cond_wait(a,b) shoutamp_fakewait(b)
#define shoutamp_cond_signal(a) 
#endif

static void *shoutamp_child_main(void *zz)
{
    t_shoutamp *x = zz;
    pute("1\n");
    pthread_mutex_lock(&x->x_mutex);
    while (1)
    {
    	int fd, fifohead;
		char *buffer;	/* mp3 data */
		float *buf;		/* encoded PCM floats */
		pute("0\n");
		if (x->x_requestcode == REQUEST_NOTHING)
		{
    			pute("wait 2\n");
			shoutamp_cond_signal(&x->x_answercondition);
			shoutamp_cond_wait(&x->x_requestcondition, &x->x_mutex);
    			pute("3\n");
		}
			// connect to SHOUTcast or Icecast server
		else if (x->x_requestcode == REQUEST_CONNECT)
		{
    			char boo[80];
			int sysrtn, wantbytes;
			
	    		/* copy connect stuff out of the data structure so we can
			relinquish the mutex while we're in oggcast_child_connect(). */
			char *hostname = x->x_hostname;
			char *mountpoint = x->x_mountpoint;
			t_int portno = x->x_port;
			x->x_disconnect = 0;
	    		/* alter the request code so that an ensuing "open" will get
			noticed. */
    			pute("4\n");
			x->x_requestcode = REQUEST_BUSY;
			x->x_connecterror = 0;

	    		/* if there's already a connection open, close it */
			if (x->x_fd >= 0)
			{
	    		fd = x->x_fd;
	    		pthread_mutex_unlock(&x->x_mutex);
    	    	shoutamp_child_disconnect(fd);
    	    	pthread_mutex_lock(&x->x_mutex);
				x->x_connectstate = 0;
				clock_delay(x->x_clock, 0);
	    		x->x_fd = -1;
				if (x->x_requestcode != REQUEST_BUSY)
					goto lost;
			}
    	    		/* open the socket with the mutex unlocked */
			pthread_mutex_unlock(&x->x_mutex);
			fd = shoutamp_child_connect(hostname, mountpoint, portno);
			pthread_mutex_lock(&x->x_mutex);
    		pute("5\n");
    	    		/* copy back into the instance structure. */
			x->x_connectstate = 1;
			clock_delay(x->x_clock, 0);
			x->x_fd = fd;
			if (fd < 0)
			{
    	    	x->x_connecterror = fd;
				x->x_eof = 1;
				x->x_connectstate = 0;
				clock_delay(x->x_clock, 0);
    	    	pute("connect failed\n");
				goto lost;
			}
			else
			{
					/* initialise the decoder */
				if(shoutamp_mpg123_init(x, fd) != -1)
				{
					post("shoutamp~: decoder initialised");
					shoutamp_child_dographics(x);
				}
				else
				{
					post("shoutamp~: could not init decoder");
					shoutamp_child_disconnect(fd);
					post("shoutamp~: connection closed due to bitstream error");
					x->x_disconnect = 1;
					x->x_fd = -1;
					x->x_eof = 1;
					x->x_connectstate = 0;
					clock_delay(x->x_clock, 0);
    	    		pute("initialisation failed\n");
					goto lost;
				} 
			}
	    		/* check if another request has been made; if so, field it */
			if (x->x_requestcode != REQUEST_BUSY)
	    		goto lost;
    		pute("6\n");
    		x->x_fifohead = fifohead = 0;
	    		/* set fifosize from bufsize.  fifosize must be a
			multiple of the number of bytes eaten for each DSP
			tick.  We pessimistically assume MAXVECSIZE samples
			per tick since that could change.  There could be a
			problem here if the vector size increases while a
			stream is being played...  */
			x->x_fifosize = x->x_bufsize - (x->x_bufsize %
	    		(x->x_streamchannels * 2));
				/* arrange for the "request" condition to be signalled x->x_siginterval
				times per buffer */
    		sprintf(boo, "fifosize %d\n", 
    	    	x->x_fifosize);
    		pute(boo);
			x->x_sigcountdown = x->x_sigperiod = (x->x_fifosize / (x->x_siginterval * x->x_streamchannels * x->x_vecsize));

    	    	/* in a loop, wait for the fifo to get hungry and feed it */
			while (x->x_requestcode == REQUEST_BUSY)
			{
	    		int fifosize = x->x_fifosize;
				buf = x->x_buf;
    	    		pute("77\n");
				if (x->x_eof)
					break;
		    		/* try to get new data from decoder whenever
				there is some space at end of buffer */
				if(x->x_fifobytes < fifosize - DECODE_PACKET_SIZE)
				{
		    		sprintf(boo, "head %d, tail %d\n", x->x_fifohead, x->x_fifotail);
					pute(boo);

					/* we pass x on to the routine since we need the mpg123 stuff
					   to be presend. all other values should not be changed because
					   mutex is unlocked ! */
					pute("decode... ");
				    pthread_mutex_unlock(&x->x_mutex);
					sysrtn = shoutamp_decode_input(x, buf, fifohead, fifosize, fd);
					shoutamp_child_updategraphics(x);
	    			pthread_mutex_lock(&x->x_mutex);
					if (x->x_requestcode != REQUEST_BUSY)
						break;
					if (sysrtn == 0)
					{
						if (x->x_eos && !x->x_disconnect)	/* got end of stream */
						{
							pute("end of stream\n");
							shoutamp_mpg123_deinit(x);
							if(shoutamp_mpg123_init(x, fd) == -1)	/* reinit stream */
							{
								x->x_state = STATE_IDLE;
								x->x_disconnect = 1;
								goto quit;
							}
						}
						else if (x->x_eos && x->x_disconnect)	/* we're disconnecting */
						{
							x->x_state = STATE_IDLE;
							pute("end of stream: disconnecting\n");
							x->x_connectstate = 0;
							clock_delay(x->x_clock, 0);
							break;		/* go to disconnect */
						}
					}
					else if (sysrtn < 0)		/* got any other error from decoder */
					{
						pute("connecterror\n");
	    				x->x_connecterror = sysrtn;
						x->x_state = STATE_IDLE;
						x->x_connectstate = 0;
						clock_delay(x->x_clock, 0);
						break;
					}
					x->x_fifohead = (fifohead + sysrtn) % fifosize;
					x->x_fifobytes += sysrtn;
    	    		sprintf(boo, "after: head %d, tail %d\n", 
    	    			x->x_fifohead, x->x_fifotail);
    	    		pute(boo);
						/* check wether the buffer is filled enough to start streaming */
				}
				else	/* there is enough data in the buffer :: do nothing */
				{
					x->x_state = STATE_STREAM;
    	    	    pute("wait 7...\n");
	    	    	shoutamp_cond_signal(&x->x_answercondition);
		    		shoutamp_cond_wait(&x->x_requestcondition, &x->x_mutex);
    	    	    pute("7 done\n");
					continue;
				}
    	    	pute("8\n");
				fd = x->x_fd;
				buf = x->x_buf;
				fifohead = x->x_fifohead;
	    		pthread_mutex_unlock(&x->x_mutex);

					/* signal parent in case it's waiting for data */
				shoutamp_cond_signal(&x->x_answercondition);
			}

lost:

    		if (x->x_requestcode == REQUEST_BUSY)
	    	x->x_requestcode = REQUEST_NOTHING;
    	    		/* fell out of read loop: close connection if necessary,
			set EOF and signal once more */
			if (x->x_fd >= 0)
			{
	    		fd = x->x_fd;
    	    	pthread_mutex_unlock(&x->x_mutex);
				shoutamp_mpg123_deinit(x);
    	    	shoutamp_child_disconnect(fd);
				pthread_mutex_lock(&x->x_mutex);
    	    	x->x_connectstate = 0;
				clock_delay(x->x_clock, 0);
	    		x->x_fd = -1;
				shoutamp_child_delgraphics(x);
    		}
			shoutamp_cond_signal(&x->x_answercondition);

		}
			/* reconnect to server */
		else if (x->x_requestcode == REQUEST_RECONNECT)
		{
			if (x->x_fd >= 0)
			{
	    		fd = x->x_fd;
	    		pthread_mutex_unlock(&x->x_mutex);
				shoutamp_mpg123_deinit(x);
    	    	shoutamp_child_disconnect(fd);
    	    	pthread_mutex_lock(&x->x_mutex);
				shoutamp_child_delgraphics(x);
	    		x->x_fd = -1;
			}
				/* connect again */
				x->x_requestcode = REQUEST_CONNECT;
			
			x->x_connectstate = 0;
			clock_delay(x->x_clock, 0);
			shoutamp_cond_signal(&x->x_answercondition);
		}
			/* close connection to server (disconnect) */
		else if (x->x_requestcode == REQUEST_CLOSE)
		{
quit:
			if (x->x_fd >= 0)
			{
	    		fd = x->x_fd;
	    		pthread_mutex_unlock(&x->x_mutex);
				shoutamp_mpg123_deinit(x);
    	    	shoutamp_child_disconnect(fd);
    	    	pthread_mutex_lock(&x->x_mutex);
				shoutamp_child_delgraphics(x);
	    		x->x_fd = -1;
			}
			if (x->x_requestcode == REQUEST_CLOSE)
	    		x->x_requestcode = REQUEST_NOTHING;
			else if (x->x_requestcode == REQUEST_BUSY)
	    		x->x_requestcode = REQUEST_NOTHING;
			else if (x->x_requestcode == REQUEST_CONNECT)
	    		x->x_requestcode = REQUEST_NOTHING;
			x->x_connectstate = 0;
			clock_delay(x->x_clock, 0);
			shoutamp_cond_signal(&x->x_answercondition);
		}
			// quit everything
		else if (x->x_requestcode == REQUEST_QUIT)
		{
			if (x->x_fd >= 0)
			{
	    		fd = x->x_fd;
	    		pthread_mutex_unlock(&x->x_mutex);
				shoutamp_mpg123_deinit(x);
    	    	shoutamp_child_disconnect(fd);
    	    	pthread_mutex_lock(&x->x_mutex);
				shoutamp_child_delgraphics(x);
				x->x_fd = -1;
			}
			x->x_connectstate = 0;
			clock_delay(x->x_clock, 0);
			x->x_requestcode = REQUEST_NOTHING;
			shoutamp_cond_signal(&x->x_answercondition);
			break;
		}
		else
		{
			pute("13\n");
		}
    }
    pute("thread exit\n");
    pthread_mutex_unlock(&x->x_mutex);
    return (0);
}

/******** the object proper runs in the calling (parent) thread ****/

static void shoutamp_tick(t_shoutamp *x)
{
	outlet_float(x->x_connection, x->x_connectstate);
}

static void *shoutamp_new(t_floatarg fdographics, t_floatarg fnchannels, t_floatarg fbufsize)
{
    t_shoutamp *x;
    int nchannels = fnchannels, bufsize = fbufsize * 1024, i;
    float *buf;
    
    if (nchannels < 1)
    	nchannels = 2;		/* two channels as default */
    else if (nchannels > MAXSTREAMCHANS)
    	nchannels = MAXSTREAMCHANS;
		/* check / set buffer size */
    if (!bufsize) bufsize = DEFBUFPERCHAN * nchannels;
    else if (bufsize < MINBUFSIZE)
    	bufsize = MINBUFSIZE;
    else if (bufsize > MAXBUFSIZE)
    	bufsize = MAXBUFSIZE;
    buf = getbytes(bufsize*sizeof(t_float));
    if (!buf) return (0);
    
    x = (t_shoutamp *)pd_new(shoutamp_class);
    
    if ( nbinstances < MAX_DECODERS )
    {
       x->x_instance = nbinstances++;
    }
    else
    {
       post( "shoutamp~: cannot create more decoders (memory issues), sorry" );
       return NULL;
    } 
		/* what happens if blacksize changes ? */
    x->x_rsbuf = getbytes(sys_getblksize()*sizeof(t_float)*2);

    for (i = 0; i < nchannels; i++)
    	outlet_new(&x->x_obj, gensym("signal"));
    x->x_noutlets = nchannels;
    x->x_connection = outlet_new(&x->x_obj, gensym("float"));
	x->x_clock = clock_new(x, (t_method)shoutamp_tick);

    pthread_mutex_init(&x->x_mutex, 0);
    pthread_cond_init(&x->x_requestcondition, 0);
    pthread_cond_init(&x->x_answercondition, 0);

    x->x_vecsize = sys_getblksize();
	x->x_disconnect = 0;
    x->x_state = STATE_IDLE;
    x->x_canvas = canvas_getcurrent();
    x->x_streamchannels = 2;
    x->x_fd = -1;
    x->x_buf = buf;
    x->x_bufsize = bufsize;
	x->x_siginterval = 16;	/* signal 16 times per buffer */
    x->x_fifosize = x->x_fifohead = x->x_fifotail = x->x_fifobytes = x->x_requestcode = 0;
	x->x_verbose = 1;

	x->x_connectstate = 0;  /* indicating state of connection */
    
    x->x_samplerate = x->x_streamrate = sys_getsr();
	x->x_resample = 0;
	x->x_mpg123 = 0;
	x->x_recover = -1;   /* just ignore buffer underruns */
	x->x_mp3inpos = 0;

		/* graphical buffer status display */
    x->x_graphic = (int)fdographics;
    x->x_canvas = canvas_getcurrent(); 
    
    post(shoutamp_version);
	post("shoutamp~: set buffer to %dk bytes", bufsize/1024);

		/* start child thread */
    pthread_create(&x->x_childthread, 0, shoutamp_child_main, x);
    return (x);
}

	/* resample audio data */
static int shoutamp_resample(t_float *indata, int insize, t_float *outdata, int outsize)
{
	if(insize == outsize)	/* no resampling needed */
	{
		memcpy(indata, outdata, insize);	/* just copy audio data */
	}

}

static t_int *shoutamp_perform(t_int *w)
{
    t_shoutamp *x = (t_shoutamp *)(w[1]);
    t_int vecsize = x->x_vecsize, noutlets = x->x_noutlets, i, j, r;
    t_float *fp;
	t_float *sp = x->x_buf;
	t_float *buffer = x->x_rsbuf; /* buffer for resampled data */
	t_float resample = x->x_resample;
	t_int skip = (t_int)(1.0 / resample);

    if (x->x_state == STATE_STREAM)
    {
    	t_int wantbytes, getbytes, havebytes, nchannels, streamchannels = x->x_streamchannels;

		pthread_mutex_lock(&x->x_mutex);

			/* get 'getbytes' bytes from input buffer, convert them to 
		       'wantbytes' which is the number of bytes after resampling */
		getbytes = streamchannels * vecsize;		/* number of bytes we need to get after resampling */
		wantbytes = (t_float)getbytes  / resample;	/* we need vecsize bytes per channel */
		havebytes = x->x_fifobytes;

			/* check for error */
		if(havebytes < wantbytes)
		{
			// post("shoutamp~: found error");
			if(x->x_connecterror)
			{		/* report error and close connection */
	    		pd_error(x, "dsp: error %d", x->x_connecterror);
				x->x_state = STATE_IDLE;
				x->x_requestcode = REQUEST_CLOSE;
				x->x_disconnect = 1;
				shoutamp_cond_signal(&x->x_requestcondition);
				pthread_mutex_unlock(&x->x_mutex);
			}
			if(!x->x_disconnect)	/* it's not due to disconnect */
			{
				if(x->x_recover == 0)			/* disconnect */
				{
					x->x_state = STATE_IDLE;
					x->x_requestcode = REQUEST_CLOSE;
					x->x_disconnect = 1;
					shoutamp_cond_signal(&x->x_requestcondition);
					pthread_mutex_unlock(&x->x_mutex);
				}
				else if(x->x_recover == 1)		/* reconnect */
				{
					x->x_state = STATE_IDLE;
					x->x_requestcode = REQUEST_RECONNECT;
					x->x_disconnect = 1;
					shoutamp_cond_signal(&x->x_requestcondition);
					pthread_mutex_unlock(&x->x_mutex);
				}
				else		/* resume */
				{
					x->x_state = STATE_IDLE;
					x->x_disconnect = 0;
					shoutamp_cond_signal(&x->x_requestcondition);
					pthread_mutex_unlock(&x->x_mutex);
				}
			}
			goto idle;
		}

			/* output audio */
		sp += x->x_fifotail;	/* go to actual audio position */

			/* resample if necessary */
		if (resample > 1.0)	/* upsampling */
		{
			int parent = vecsize / resample;	/* how many samples to read from buffer */
			for(j = 0; j < parent; j++)
			{
				for(i = 0; i < streamchannels; i++)
				{		/* copy same sample several times */
					for (r = 0; r < resample; r++)
						*buffer++ = *sp;
					sp++;	/* get next sample from stream */
				}
			}
		}
		else if (resample < 1.0)	/* downsampling */
		{
			int parent = vecsize * skip;/* how many samples to read from buffer */
			for(j = 0; j < parent; j++)
			{
				for(i = 0; i < streamchannels; i++)
				{
					*buffer++ = *sp;	/* get one sample */
					sp += skip;			/* skip next few samples in stream */
				}
			}
		}
		else if (resample == 1.0)	/* no resampling */
		{		/* copy without any changes */
			for(i = 0; i < getbytes; i++)*buffer++ = *sp++;
		}
		buffer -= getbytes;	/* reset to beginning of buffer */


		if(noutlets == streamchannels)
		{      /* normal output */
			for(j = 0; j < vecsize; j++)
			{
				for(i = 0; i < noutlets; i++)
				{
					x->x_outvec[i][j] = *buffer++;
				}
			}
		}
		else if((noutlets / 2) == streamchannels)
		{       /* mono to stereo conversion */
			for(j = 0; j < vecsize; j++)
			{
				for(i = 0; i < noutlets; i++)
				{
					x->x_outvec[i][j] = *buffer;
				}
				buffer++;
			}
		}
		else if((noutlets * 2) == streamchannels)
		{      /* stereo to mono conversion */
			for(j = 0; j < vecsize; j++)
			{
				for(i = 0; i < streamchannels; i++)
				{
					x->x_outvec[i/2][j] += (float) (*buffer++ * 0.5);
				}
			}
		}
		else goto idle;

		
		x->x_fifotail += wantbytes;
		x->x_fifobytes -= wantbytes;
		if (x->x_fifotail >= x->x_fifosize)
		{
			x->x_fifotail = 0;
		}
			/* signal the child thread */
		if ((--x->x_sigcountdown) <= 0)
		{
    		shoutamp_cond_signal(&x->x_requestcondition);
			x->x_sigcountdown = x->x_sigperiod;
		}
		pthread_mutex_unlock(&x->x_mutex);
    }
    else
    {
    idle:
    	for (i = 0; i < noutlets; i++)
	    for (j = vecsize, fp = x->x_outvec[i]; j--; )
	    	*fp++ = 0;
    }

    return (w+2);
}


static void shoutamp_disconnect(t_shoutamp *x)
{
    	/* LATER rethink whether you need the mutex just to set a variable? */
    pthread_mutex_lock(&x->x_mutex);
	x->x_disconnect = 1;
    x->x_state = STATE_IDLE;
    x->x_requestcode = REQUEST_CLOSE;
    shoutamp_cond_signal(&x->x_requestcondition);
    pthread_mutex_unlock(&x->x_mutex);
}


    /* connect method.  Called as:
    connect <hostname or IP> <mountpoint> <portnumber>
    */

static void shoutamp_connect(t_shoutamp *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *hostsym = atom_getsymbolarg(0, argc, argv);
    t_symbol *mountsym = atom_getsymbolarg(1, argc, argv);
    t_float portno = atom_getfloatarg(2, argc, argv);
    if (!*hostsym->s_name)	/* check for hostname */
    	return;
    if (!portno)			/* check wether the portnumber is specified */
    	portno = 8000;		/* ...assume port 8000 as standard */
    pthread_mutex_lock(&x->x_mutex);
	if(x->x_fd == -1)
	{
		x->x_hostname = hostsym->s_name;
		x->x_mountpoint = mountsym->s_name;
		x->x_port = portno;
		x->x_requestcode = REQUEST_CONNECT;
			/* empty buffer */
		x->x_fifotail = 0;
		x->x_fifohead = 0;
		x->x_fifobytes = 0;
        x->x_mp3inpos = 0;	/* also forget all mp3 data */
		x->x_streamchannels = 2;
		x->x_eof = 0;
		x->x_connecterror = 0;
		x->x_state = STATE_STARTUP;
		x->x_disconnect = 0;
		shoutamp_cond_signal(&x->x_requestcondition);
	}
	else post("shoutamp~: already connected");
    pthread_mutex_unlock(&x->x_mutex);
}

	/* connect using url like "http://localhost:8000/mountpoint" */
static void shoutamp_connect_url(t_shoutamp *x, t_symbol *url)
{
	char *hname, *port;
	char *h, *p;
	char *hostptr;
	char *r_hostptr;
	char *pathptr;
	char *portptr;
	char *p0;
	char *defaultportstr = "8000";
	t_int stringlength;
	t_int portno;

		/* strip http:// or ftp:// */
	p = url->s_name;
	if (strncmp(p, "http://", 7) == 0)
		p += 7;

        if (strncmp(p, "ftp://", 6) == 0)
               p += 6;

	hostptr = p;
	while (*p && *p != '/')	/* look for end of hostname:port */
		p++;
	p++;					/* also skip '/' */
	pathptr = p;

	r_hostptr = --p;
	while (*p && hostptr < p && *p != ':' && *p != ']')	/* split at ':' */
		p--;

	if (!*p || p < hostptr || *p != ':') {
		portptr = NULL;
	}
	else{
		portptr = p + 1;
		r_hostptr = p - 1;
	}
	if (*hostptr == '[' && *r_hostptr == ']') {
		hostptr++;
		r_hostptr--;
	}

	stringlength = r_hostptr - hostptr + 1;
	h = getbytes(stringlength + 1);
	if (h == NULL) {
		hname = NULL;
		port = NULL;
		pathptr = NULL; 
	}
	strncpy(h, hostptr, stringlength);
	*(h+stringlength) = '\0';
	hname = h;	/* the hostname */

	if (portptr) {
		stringlength = (pathptr - portptr);
		if(!stringlength) portptr = NULL;
	}
	if (portptr == NULL) {
		portptr = defaultportstr;
		stringlength = strlen(defaultportstr);
	}
	p0 = getbytes(stringlength + 1);
	if (p0 == NULL) {
		freebytes(h, stringlength + 1);
		hname = NULL;
		port = NULL;
		pathptr = NULL;
	}
	strncpy(p0, portptr, stringlength);
	*(p0 + stringlength) = '\0';

	for (p = p0; *p && isdigit((unsigned char) *p); p++) ;

	*p = '\0';
	port = (unsigned char *) p0;
		/* convert port from string to int */
	portno = (int)strtol(port, NULL, 10);
	freebytes(p0, stringlength + 1);
		/* set values and signal child to connect */
	pthread_mutex_lock(&x->x_mutex);
	if(x->x_fd == -1)
	{
		x->x_hostname = hname;
		x->x_mountpoint = pathptr;
		x->x_port = portno;
		x->x_requestcode = REQUEST_CONNECT;
		x->x_fifotail = 0;
		x->x_fifohead = 0;
		x->x_fifobytes = 0;
		x->x_streamchannels = 2;
		x->x_eof = 0;
		x->x_connecterror = 0;
		x->x_state = STATE_STARTUP;
		shoutamp_cond_signal(&x->x_requestcondition);
	}
	else post("shoutamp~: already connected");
	pthread_mutex_unlock(&x->x_mutex);
}

static void shoutamp_float(t_shoutamp *x, t_floatarg f)
{
    if (f != 0)
	{
		pthread_mutex_lock(&x->x_mutex);
		if(x->x_fd == -1)
		{
			x->x_requestcode = REQUEST_CONNECT;

			x->x_fifotail = 0;
			x->x_fifohead = 0;
			x->x_fifobytes = 0;
			x->x_streamchannels = 2;
			x->x_eof = 0;
			x->x_connecterror = 0;
			x->x_state = STATE_STARTUP;
			shoutamp_cond_signal(&x->x_requestcondition);
		}
		else post("shoutamp~: already connected");
		pthread_mutex_unlock(&x->x_mutex);
	}
    else shoutamp_disconnect(x);
}

static void shoutamp_dsp(t_shoutamp *x, t_signal **sp)
{
    int i, noutlets = x->x_noutlets;
    pthread_mutex_lock(&x->x_mutex);
    x->x_vecsize = sp[0]->s_n;
    
    x->x_sigperiod = (x->x_fifosize / (x->x_siginterval * x->x_streamchannels * x->x_vecsize));
    for (i = 0; i < noutlets; i++)
    	x->x_outvec[i] = sp[i]->s_vec;
    pthread_mutex_unlock(&x->x_mutex);
    dsp_add(shoutamp_perform, 1, x);
}

static void shoutamp_print(t_shoutamp *x)
{
    pthread_mutex_lock(&x->x_mutex);
	if(x->x_fd >= 0)
	{
		post("shoutamp~: connected to http://%s:%d/%s", x->x_hostname, x->x_port, x->x_mountpoint);
		post("shoutamp~: bitstream is %d channels @ %ld Hz with %ldkbps nominal bitrate",
			x->x_streamchannels, x->x_streamrate, x->x_bitrateindex / 1000);
	} else post("shoutamp~: not connected");
	if(x->x_recover == 0)
	post("shoutamp~: recover mode set to \"disconnect\" (0)");
	else if(x->x_recover == 1)
	post("shoutamp~: recover mode set to \"reconnect\" (1)");
	else if(x->x_recover == -1)
	post("shoutamp~: recover mode set to \"resume\" (-1)");
    pthread_mutex_unlock(&x->x_mutex);
}

	/* set behavior for buffer underruns */
static void shoutamp_recover(t_shoutamp *x, t_floatarg f)
{
    pthread_mutex_lock(&x->x_mutex);
	if(f <= -1)
	{		/* mute audio and try to fill buffer again: the default */
		post("shoutamp~: set recover mode to \"resume\" (-1)");
		f = -1;
	}
	else if(f >= 1)
	{		/* reconnect to server */
		post("shoutamp~: set recover mode to \"reconnect\" (1)");
		f = 1;
	}
	else
	{		/* disconnect from server */
		post("shoutamp~: set recover mode to \"disconnect\" (0)");
		f = 0;
	}
	x->x_recover = f;
    pthread_mutex_unlock(&x->x_mutex);
}

static void shoutamp_free(t_shoutamp *x)
{
    	/* request QUIT and wait for acknowledge */
    void *threadrtn;
    pthread_mutex_lock(&x->x_mutex);
    x->x_requestcode = REQUEST_QUIT;
    x->x_disconnect = 1;
    post("stopping shoutamp thread...");
    shoutamp_cond_signal(&x->x_requestcondition);
    while (x->x_requestcode != REQUEST_NOTHING)
    {
    	post("signalling...");
		shoutamp_cond_signal(&x->x_requestcondition);
    	shoutamp_cond_wait(&x->x_answercondition, &x->x_mutex);
    }
    pthread_mutex_unlock(&x->x_mutex);
    if (pthread_join(x->x_childthread, &threadrtn))
    	error("shoutamp_free: join failed");
    post("... done.");
    
    pthread_cond_destroy(&x->x_requestcondition);
    pthread_cond_destroy(&x->x_answercondition);
    pthread_mutex_destroy(&x->x_mutex);
    freebytes(x->x_buf, x->x_bufsize*sizeof(t_float));
	freebytes(x->x_rsbuf, sys_getblksize()*sizeof(t_float)*2);
	clock_free(x->x_clock);
}

void shoutamp_tilde_setup(void)
{
    shoutamp_class = class_new(gensym("shoutamp~"), (t_newmethod)shoutamp_new, 
    	(t_method)shoutamp_free, sizeof(t_shoutamp), 0, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(shoutamp_class, (t_method)shoutamp_float);
    class_addmethod(shoutamp_class, (t_method)shoutamp_disconnect, gensym("disconnect"), 0);
    class_addmethod(shoutamp_class, (t_method)shoutamp_dsp, gensym("dsp"), 0);
    class_addmethod(shoutamp_class, (t_method)shoutamp_connect, gensym("connect"), A_GIMME, 0);
    class_addmethod(shoutamp_class, (t_method)shoutamp_connect_url, gensym("connecturl"), A_SYMBOL, 0);
    class_addmethod(shoutamp_class, (t_method)shoutamp_recover, gensym("recover"), A_FLOAT, 0);
    class_addmethod(shoutamp_class, (t_method)shoutamp_print, gensym("print"), 0);
    class_sethelpsymbol(shoutamp_class, gensym("help-shoutamp~.pd"));
}