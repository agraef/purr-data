/* ------------------------ shoutcast~ ---------------------------------------- */
/*                                                                              */
/* Tilde object to send mp3-stream to shoutcast/icecast server.                 */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>.                               */
/* Original version ported to Linux by Yves Degoyon                             */
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
#include "lame/lame.h"       /* lame encoder stuff */

#include <sys/types.h>
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

#ifdef UNIX
#define     sys_closesocket close
#endif
#ifdef NT
#define     sys_closesocket closesocket
#endif

#ifdef __linux__	// 'real' linux only, not for OS X !
#define SEND_OPT MSG_DONTWAIT|MSG_NOSIGNAL
#else
#define SEND_OPT 0
#endif

/************************* shoutcast~ object ******************************/

/* Each instance of shoutcast~ owns a "child" thread for doing the data
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
#define     REQUEST_REINIT 6

#define     STATE_IDLE 0
#define     STATE_STARTUP 1                     /* connecting and filling the buffer */
#define     STATE_STREAM 2                      /* streaming and audio output */

#define     DEFBUFPERCHAN           262144      /* audio output buffer by default: 256k */
#define     ENCODER_CHANNELS        2           /* we allways use 2 channels internally */
#define     MINBUFSIZE              65536       /* minimum size of ring buffer */
#define     MAXBUFSIZE              16777216 	/* arbitrary; just don't want to hang malloc */
#define     STRBUF_SIZE             1024        /* char received from server on startup */
#define		MAXSTREAMCHANS          2           /* maximum number of channels: restricted to 2 by Ogg specs */
#define     MAXDATARATE             320         /* maximum mp3 data rate is 320kbit/s */

#define     MY_MP3_MALLOC_IN_SIZE   65536       /* max size taken from lame readme */
#define     MY_MP3_MALLOC_OUT_SIZE  1.25*MY_MP3_MALLOC_IN_SIZE+7200 /* max size of encoded output */

#define     UPDATE_INTERVAL         250         /* time in milliseconds between updates of output values */


static char   *shoutcast_version = "shoutcast~: mp3 streamer version 0.3h, written by Olaf Matthes";

static t_class *shoutcast_class;

typedef struct _shoutcast
{
    t_object x_obj;
    t_float *x_f;
    t_clock *x_clock_connect;
    t_clock *x_clock_pages;
    t_outlet *x_connection;    /* outlet for connection state */
    t_outlet *x_outpages;	   /* outlet for no. of ogg pages */

    t_float *x_buf;    	    	    	    /* audio data buffer */
    t_int x_bufsize;  	    	    	    /* buffer size in bytes */
    t_int x_ninlets; 	    	    	    /* number of audio outlets */
    t_sample *(x_outvec[MAXSTREAMCHANS]);	/* audio vectors */
    t_int x_vecsize;  	    	    	    /* vector size for transfers */
    t_int x_state;    	    	    	    /* opened, running, or idle */

    	/* parameters to communicate with subthread */
    t_int x_requestcode;	   /* pending request from parent to I/O thread */
    t_int x_connecterror;	   /* slot for "errno" return */

		/* buffer stuff */
    t_int x_fifosize; 	       /* buffer size appropriately rounded down */	    
    t_int x_fifohead; 	       /* index of next byte to get from file */
    t_int x_fifotail; 	       /* index of next byte the ugen will read */
    t_int x_sigcountdown;      /* counter for signalling child for more data */
    t_int x_sigperiod;	       /* number of ticks per signal */
	t_int x_siginterval;       /* number of times per buffer (depends on data rate) */

        /* LAME stuff */
    t_int x_lame;              /* info about encoder status */
	lame_global_flags *x_lgfp;
    t_int x_lamechunk;         /* chunk size for LAME encoder */
    short *x_mp3inbuf;         /* data to be sent to LAME */
    char *x_mp3outbuf;         /* data returned by LAME -> our mp3 stream */
    t_int x_mp3mode;           /* mode (mono, joint stereo, stereo, dual mono) */
    t_int x_mp3quality;        /* quality of encoding */
	t_int x_vbr;               /* quality of VBR encoding if not -1 */

	t_int     x_eos;           /* end of stream */
	t_float   x_streamedbytes; /* number of bytes that have been output to server */
	t_float   x_oldstreamedbytes;

        /* ringbuffer stuff */
    t_float *x_buffer;         /* data to be buffered (ringbuffer)*/
    t_int    x_bytesbuffered;  /* number of unprocessed bytes in buffer */

        /* mp3 stream format stuff */
    t_int    x_samplerate;     /* samplerate of stream (default = getsr() ) */
    t_int    x_bitrate;        /* bitrate of mp3 stream (CBR only) */
    t_int    x_channels;       /* number of channels sent to the encoder */
	                           /* this is allways 2 because of lame's software design */

        /* IceCast server stuff */
    char*    x_passwd;         /* password for server */
    char*    x_bcname;         /* name of broadcast */
    char*    x_bcurl;          /* url of broadcast */
    char*    x_bcgenre;        /* genre of broadcast */
	char*    x_hostname;       /* name or IP of host to connect to */
	char*    x_mountpoint;     /* mountpoint for IceCast server */
	t_float  x_port;           /* port number on which the connection is made */
    t_int    x_bcpublic;       /* do(n't) publish broadcast on www.shoutcast.com */
	t_int    x_icecast;        /* tells if we use a IceCast server (1) or SHOUTcast (0) */
	
	t_int    x_connectstate;   /* indicates to state of socket connection */
	t_int    x_outvalue;       /* value that has last been output via outlet */
    t_int    x_fd;             /* the socket number */

		/* tread stuff */
    pthread_mutex_t   x_mutex;
    pthread_cond_t    x_requestcondition;
    pthread_cond_t    x_answercondition;
    pthread_t         x_childthread;
} t_shoutcast;

	/* check server for writeability */
static int shoutcast_checkserver(t_int sock)
{
    fd_set          fdset;
    struct timeval  ztout;
	fd_set writeset;
	fd_set exceptset;

	FD_ZERO(&writeset);
	FD_ZERO(&exceptset);
	FD_SET(sock, &writeset );
	FD_SET(sock, &exceptset );

	if(select(sock+1, NULL, &writeset, &exceptset, &ztout) > 0)
	{
		if(!FD_ISSET(sock, &writeset))
		{
			post("shoutcast~: can not write data to the server, quitting");
			return -1;
		}
        if(FD_ISSET(sock, &exceptset))
		{
			post("shoutcast~: socket returned an error, quitting");
			return -1;
		}   
	}
	return 0;
}

	/* lame decoder setup */
static int shoutcast_lame_init(t_shoutcast *x)
{
    int    ret;
    char *title = NULL;
    time_t now;
    int title_length;

    x->x_lgfp = lame_init(); /* set default parameters for now */
    post( "shoutcast~: using lame version %s", get_lame_version() );

		/* setting lame parameters */
    if(x->x_mp3mode!=3)
	{
		lame_set_num_channels( x->x_lgfp, 2);
	}
	else
	{
		lame_set_num_channels( x->x_lgfp, 1);
	}	/* maybe we're mono */
    lame_set_in_samplerate( x->x_lgfp, sys_getsr() );
    lame_set_out_samplerate( x->x_lgfp, x->x_samplerate );
	if(x->x_vbr == -1)	/* constand bitrate encoding (default) */
		lame_set_brate( x->x_lgfp, x->x_bitrate );
	else	/* VBR encoding */
	{
		lame_set_VBR( x->x_lgfp, vbr_rh );
		lame_set_VBR_q( x->x_lgfp, x->x_vbr);
	}
    lame_set_mode( x->x_lgfp, x->x_mp3mode );
    lame_set_quality( x->x_lgfp, x->x_mp3quality );
    lame_set_emphasis( x->x_lgfp, 0 );
    lame_set_original( x->x_lgfp, 1 );
    lame_set_copyright( x->x_lgfp, 0 ); /* viva free music societies !!! */
    lame_set_disable_reservoir( x->x_lgfp, 1 );
    lame_set_padding_type( x->x_lgfp, PAD_NO );
    ret = lame_init_params( x->x_lgfp );
    if( ret<0 )
	{
		error("shoutcast~: lame params initialization returned  %d", ret);
    }
	else
	{
		x->x_lame = 1;
			/* magic formula copied from windows dll for MPEG-I */
		if(x->x_mp3mode!=3)
		{
			x->x_lamechunk = 1152*2;
		}
		else
		{
			x->x_lamechunk = 1152;
		}
		post( "shoutcast~: lame initialization done");
    }
    lame_init_bitstream(x->x_lgfp );

        /* setting tag information */
    id3tag_init(x->x_lgfp);
    id3tag_v1_only(x->x_lgfp);
    id3tag_space_v1(x->x_lgfp);
    id3tag_set_artist(x->x_lgfp, "Pd Session");
    now=time(NULL);
    title_length = 256;
    if ( title != NULL )
	{
       freebytes(title, title_length );
    }
    title=(char*)getbytes(title_length);
    sprintf(title, "Started at %s", ctime(&now));
    id3tag_set_title(x->x_lgfp, title);
	x->x_streamedbytes = x->x_oldstreamedbytes = 0;
	return (0);
}

	/* deinit the lame decoder */
static void shoutcast_lame_deinit(t_shoutcast *x)
{
	int err, ret;
    if(x->x_lame >= 0)
	{
		ret = lame_encode_flush( x->x_lgfp, x->x_mp3outbuf, MY_MP3_MALLOC_OUT_SIZE );	/* flush lame buffers */
		err = send(x->x_fd, x->x_mp3outbuf, ret, SEND_OPT);
		if(err < 0)
		{
			error("shoutcast~: could not send flushed data to server (%d)", err);
		} 
		lame_close( x->x_lgfp );	/* close lame */
	}
	x->x_lame = 0;
}

	/* encode lame and stream new data */
static int shoutcast_encode(t_shoutcast *x, float *buf, int channels, int fifosize, int fd)
{
    unsigned short i, ch;
    int err = 0;
    int n;
	int mp3size;	/* number of mp3 bytes returned from encoder */

		/* read from buffer */
	for(n = 0; n < x->x_lamechunk; n++)	/* fill encode buffer */
	{
		x->x_mp3inbuf[n] = (short) (32767.0 * (*buf++));	/* convert to PCM */
	}

		/* encode mp3 data */
#ifndef UNIX	/* the clear WinNT solution: */
    mp3size = lame_encode_buffer_interleaved(x->x_lgfp, x->x_mp3inbuf, 
                   x->x_lamechunk / ENCODER_CHANNELS, x->x_mp3outbuf, MY_MP3_MALLOC_OUT_SIZE);
#else	/* is this really different under Linux ???? */
    mp3size = lame_encode_buffer_interleaved(x->x_lgfp, x->x_mp3inbuf, 
                   x->x_lamechunk / lame_get_num_channels(x->x_lgfp), 
                   x->x_mp3outbuf, MY_MP3_MALLOC_OUT_SIZE);
#endif
    // post( "shoutcast~: encoding returned %d frames", mp3size );

        /* check result */
    if(mp3size < 0)
    {
        lame_close( x->x_lgfp );
        error("shoutcast~: lame_encode_buffer_interleaved failed (%d)",mp3size);
        x->x_lame = -1;
		return(-1);
    }
		/* stream to server */
	err = send(fd, x->x_mp3outbuf, mp3size, SEND_OPT);
    if(err < 0)
    {
        error("shoutcast~: could not send encoded data to server (%d)", err);
        lame_close( x->x_lgfp );
        x->x_lame = -1;
		return(-1);
    } 
    if((err > 0)&&(err != mp3size))error("shoutcast~: %d bytes skipped", mp3size - err);

	return (mp3size);	/* return number of bytes we encoded */
}
  
    /* connect to icecast2 server */
static int shoutcast_child_connect(char *hostname, char *mountpoint, t_int portno, 
								 char *passwd, char *bcname, char *bcurl,
								 char *bcgenre, t_int bcpublic, t_int bitrate, t_int icecast)
{
    struct          sockaddr_in server;
    struct          hostent *hp;

        /* variables used for communication with server */
    const char      * buf = 0;
    char            resp[STRBUF_SIZE];
    unsigned int    len;
    fd_set          fdset;
    struct timeval  tv;
    int    sockfd;
    int    ret;

    if(icecast == 0)portno++;	/* use SHOUTcast, portno is one higher */

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        error("shoutcast~: internal error while attempting to open socket");
        return(-1);
    }

        /* connect socket using hostname provided in command line */
    server.sin_family = AF_INET;
    hp = gethostbyname(hostname);
    if (hp == 0)
    {
        post("shoutcast~: bad host?");
        sys_closesocket(sockfd);
        return(-1);
    }
    memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);

        /* assign client port number */
    server.sin_port = htons((unsigned short)portno);

        /* try to connect.  */
    post("shoutcast~: connecting to port %d", portno);
    if (connect(sockfd, (struct sockaddr *) &server, sizeof (server)) < 0)
    {
        error("shoutcast~: connection failed!\n");
        sys_closesocket(sockfd);
        return(-1);
    }

        /* sheck if we can read/write from/to the socket */
    FD_ZERO( &fdset);
    FD_SET( sockfd, &fdset);
    tv.tv_sec  = 0;            /* seconds */
    tv.tv_usec = 500;        /* microseconds */

    ret = select(sockfd + 1, &fdset, NULL, NULL, &tv);
    if(ret != 0)
    {
        error("shoutcast~: can not read from socket");
        sys_closesocket(sockfd);
        return(-1);
    }
   /* ret = select(sockfd + 1, NULL, &fdset, NULL, &tv);
    if(ret < 0)
    {
        error("shoutcast~: can not write to socket");
        sys_closesocket(sockfd);
        return;
    } */

	if(icecast == 0) /* SHOUTCAST */
	{
			/* now try to log in at SHOUTcast server */
		post("shoutcast~: logging in to SHOUTcast server...");
			/* first line is the passwd */
		buf = passwd;
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = "\n";
		send(sockfd, buf, strlen(buf), SEND_OPT);
			 /* header for SHOUTcast server */
		buf = "icy-name:";                        /* name of broadcast */
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = bcname;
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = "\nicy-url:";                        /* URL of broadcast */
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = bcurl;
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = "\nicy-genre:";                    /* genre of broadcast */
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = bcgenre;
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = "\nicy-br:";
		send(sockfd, buf, strlen(buf), SEND_OPT);
		if(sprintf(resp, "%d", bitrate) == -1)    /* convert int to a string */
		{
			error("shoutcast~: wrong bitrate");
		}
		send(sockfd, resp, strlen(resp), SEND_OPT);
		buf = "\nicy-pub:";
		send(sockfd, buf, strlen(buf), SEND_OPT);
		if(bcpublic==0)                            /* set the public flag for broadcast */
		{
			buf = "0";
		}
		else
		{
			buf ="1";
		}
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = "\n\n";
		send(sockfd, buf, strlen(buf), SEND_OPT);
	}
	else	/* IceCast */
	{
			/* now try to log in at IceCast server */
		post("shoutcast~: logging in to IceCast server...");
			/* send the request, a string like: "SOURCE <password> /<mountpoint>\n" */
		buf = "SOURCE ";
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = passwd;
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = " /";
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = mountpoint;
		send(sockfd, buf, strlen(buf), SEND_OPT);
			/* send the x-audiocast headers */
		buf = "\nx-audiocast-bitrate: ";
		send(sockfd, buf, strlen(buf), SEND_OPT);
		if(sprintf(resp, "%d", bitrate) == -1)    /* convert int to a string */
		{
			error("shoutcast~: wrong bitrate");
		}
		send(sockfd, resp, strlen(resp), SEND_OPT);
		buf = "\nx-audiocast-public: ";
		send(sockfd, buf, strlen(buf), SEND_OPT);
		if(bcpublic==0)                            /* set the public flag for broadcast */
		{
			buf = "0";
		}
		else
		{
			buf ="1";
		}
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = "\nx-audiocast-name: ";
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = bcname;
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = "\nx-audiocast-url: ";
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = bcurl;
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = "\nx-audiocast-genre: ";
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = bcgenre;
		send(sockfd, buf, strlen(buf), SEND_OPT);
		buf = "\n\n";
		send(sockfd, buf, strlen(buf), SEND_OPT);
			/* end login for IceCast */
	}

        /* read the anticipated response: "OK" */
    len = recv(sockfd, resp, STRBUF_SIZE, 0);
    if ( len < 2 || resp[0] != 'O' || resp[1] != 'K' ) 
    {
        post("shoutcast~: login failed!");
        sys_closesocket(sockfd);
        return(-1);
    }
    
		/* check if we can write to server */
	if(shoutcast_checkserver(sockfd)!= 0)
	{
		post("shoutcast~: error: server refused to receive data");
        sys_closesocket(sockfd);
		return (-1);
	}

    post("shoutcast~: logged in to http://%s:%d/%s", hp->h_name, portno, mountpoint);

	return (sockfd);
}



static void shoutcast_child_disconnect(t_int fd)
{
    sys_closesocket(fd);
	post("shoutcast~: connection closed");
}
/************** the child thread which performs data I/O ***********/

#if 0			/* set this to 1 to get debugging output */
static void pute(char *s)   /* debug routine */
{
    write(2, s, strlen(s));
}
#else
#define pute(x)
#endif

#if 1
#define shoutcast_cond_wait pthread_cond_wait
#define shoutcast_cond_signal pthread_cond_signal
#else
#include <sys/time.h>    /* debugging version... */
#include <sys/types.h>
static void shoutcast_fakewait(pthread_mutex_t *b)
{
    struct timeval timout;
    timout.tv_sec = 0;
    timout.tv_usec = 1000000;
    pthread_mutex_unlock(b);
    select(0, 0, 0, 0, &timout);
    pthread_mutex_lock(b);
}

void shoutcast_banana( void)
{
    struct timeval timout;
    timout.tv_sec = 0;
    timout.tv_usec = 200000;
    pute("banana1\n");
    select(0, 0, 0, 0, &timout);
    pute("banana2\n");
}


#define shoutcast_cond_wait(a,b) shoutcast_fakewait(b)
#define shoutcast_cond_signal(a) 
#endif

static void *shoutcast_child_main(void *zz)
{
    t_shoutcast *x = zz;
    pute("1\n");
    pthread_mutex_lock(&x->x_mutex);
    while (1)
    {
    	int fd, fifotail;
		pute("0\n");
		if (x->x_requestcode == REQUEST_NOTHING)
		{
    			pute("wait 2\n");
			shoutcast_cond_signal(&x->x_answercondition);
			shoutcast_cond_wait(&x->x_requestcondition, &x->x_mutex);
    			pute("3\n");
		}
			// connect to Icecast2 server
		else if (x->x_requestcode == REQUEST_CONNECT)
		{
    		char boo[80];
			int sysrtn, wantbytes;
			
	    		/* copy connect stuff out of the data structure so we can
			relinquish the mutex while we're connecting to server. */
			char *hostname = x->x_hostname;
			char *mountpoint = x->x_mountpoint;
			t_int portno = x->x_port;
			char *passwd = x->x_passwd;
			char *bcname = x->x_bcname;
			char *bcgenre = x->x_bcgenre;
			char *bcurl = x->x_bcurl;
			t_int bcpublic = x->x_bcpublic;
			t_int br_nom = x->x_bitrate;
			t_int servertype = x->x_icecast;
	    		/* alter the request code so that an ensuing "open" will get
			noticed. */
    			pute("4\n");
			x->x_requestcode = REQUEST_BUSY;
			x->x_connecterror = 0;

   	    		/* open the socket with the mutex unlocked in case 
			we're not already connected */
			if(x->x_fd < 0)
			{
				pthread_mutex_unlock(&x->x_mutex);
				fd = shoutcast_child_connect(hostname, mountpoint, portno, passwd, bcname,
											bcurl, bcgenre, bcpublic, br_nom, servertype);
				pthread_mutex_lock(&x->x_mutex);
    			pute("5\n");
    	    		/* copy back into the instance structure. */
				x->x_connectstate = 1;
				clock_delay(x->x_clock_connect, 0);
				x->x_fd = fd;
				if (fd < 0)
				{
    	    		x->x_connecterror = fd;
					x->x_connectstate = 0;
					clock_delay(x->x_clock_connect, 0);
    	    		pute("connect failed\n");
					goto lost;
				}
				else
				{
					x->x_streamedbytes = 0;
					clock_delay(x->x_clock_pages, 0);
						/* initialise the encoder */
					if(shoutcast_lame_init(x) == 0)
					{
						post("shoutcast~: lame encoder initialised");
						x->x_state = STATE_STREAM;
					}
					else
					{
						post("shoutcast~: could not init encoder");
						shoutcast_child_disconnect(fd);
						post("shoutcast~: connection closed due to initialisation error");
						x->x_fd = -1;
						x->x_connectstate = 0;
						clock_delay(x->x_clock_connect, 0);
    	    			pute("shoutcast~: initialisation failed\n");
						goto lost;
					} 
				}
    			x->x_fifotail = fifotail = 0;
	    			/* set fifosize from bufsize.  fifosize must be a
				multiple of the number of bytes eaten for each DSP
				tick.  We pessimistically assume MAXVECSIZE samples
				per tick since that could change.  There could be a
				problem here if the vector size increases while a
				stream is being played...  */
				x->x_fifosize = x->x_bufsize - (x->x_bufsize % (x->x_channels * x->x_lamechunk));
					/* arrange for the "request" condition to be signalled x->x_siginterval
					times per buffer */
    			sprintf(boo, "fifosize %d\n", x->x_fifosize);
    			pute(boo);
				x->x_sigcountdown = x->x_sigperiod = (x->x_fifosize / (x->x_siginterval * x->x_channels * x->x_vecsize));
			}
	    		/* check if another request has been made; if so, field it */
			if (x->x_requestcode != REQUEST_BUSY)
	    		goto lost;
    		pute("6\n");

			while (x->x_requestcode == REQUEST_BUSY)
			{
	    		int fifosize = x->x_fifosize, fifotail, channels;
				float *buf = x->x_buf;
    	    	pute("77\n");

				/* if the head is < the tail, we can immediately write
				from tail to end of fifo to disk; otherwise we hold off
				writing until there are at least x->x_lamechunk bytes in the
				buffer */
				if (x->x_fifohead < x->x_fifotail ||
					x->x_fifohead >= x->x_fifotail + x->x_lamechunk
					|| (x->x_requestcode == REQUEST_CLOSE &&
		    			x->x_fifohead != x->x_fifotail))
    	    	{	/* encode audio and send to server */
    	    		pute("8\n");
					fifotail = x->x_fifotail;
					channels = x->x_channels;
					fd = x->x_fd;
	    			pthread_mutex_unlock(&x->x_mutex);
					sysrtn = shoutcast_encode(x, buf + fifotail, channels, fifosize, fd);
	    			pthread_mutex_lock(&x->x_mutex);
					if (x->x_requestcode != REQUEST_BUSY &&
	    					x->x_requestcode != REQUEST_CLOSE)
		    				break;
					if (sysrtn < 0)
					{
						post("shoutcast~: closing due to error...");
						goto lost;
					}
					else
					{
						x->x_fifotail += x->x_lamechunk;
						x->x_streamedbytes += sysrtn;
						if (x->x_fifotail >= fifosize)
    	    	    				x->x_fifotail = 0;
    	    		}
    	    		sprintf(boo, "after: head %d, tail %d, pages %d\n", x->x_fifohead, x->x_fifotail, sysrtn);
    	    		pute(boo);
				}
				else	/* just wait... */
				{
    	    		pute("wait 7a ...\n");
	    			shoutcast_cond_signal(&x->x_answercondition);
					pute("signalled\n");
					shoutcast_cond_wait(&x->x_requestcondition,
					&x->x_mutex);
    	    		pute("7a done\n");
					continue;
				}
				/* signal parent in case it's waiting for data */
				shoutcast_cond_signal(&x->x_answercondition);
			}
		}

			/* reinit encoder (settings have changed) */
		else if (x->x_requestcode == REQUEST_REINIT)
		{
	    	pthread_mutex_unlock(&x->x_mutex);
			shoutcast_lame_deinit(x);
    	    shoutcast_lame_init(x);
   	    	pthread_mutex_lock(&x->x_mutex);
			post("shoutcast~: lame encoder reinitialised");
			x->x_state = STATE_STREAM;
			if (x->x_requestcode == REQUEST_REINIT)
	    		x->x_requestcode = REQUEST_CONNECT;
			shoutcast_cond_signal(&x->x_answercondition);
		}
			/* close connection to server (disconnect) */
		else if (x->x_requestcode == REQUEST_CLOSE)
		{
lost:
			x->x_state = STATE_IDLE;
    		if (x->x_fd >= 0)
			{
	    		fd = x->x_fd;
	    		pthread_mutex_unlock(&x->x_mutex);
				shoutcast_lame_deinit(x);
    	    	shoutcast_child_disconnect(fd);
    	    	pthread_mutex_lock(&x->x_mutex);
	    		x->x_fd = -1;
			}
			x->x_requestcode = REQUEST_NOTHING;
			x->x_connectstate = 0;
			clock_delay(x->x_clock_connect, 0);
			shoutcast_cond_signal(&x->x_answercondition);
			// shoutcast_cond_signal(&x->x_requestcondition);
			// break;
		}
			// quit everything
		else if (x->x_requestcode == REQUEST_QUIT)
		{
			x->x_state = STATE_IDLE;
			if (x->x_fd >= 0)
			{
	    		fd = x->x_fd;
	    		pthread_mutex_unlock(&x->x_mutex);
				shoutcast_lame_deinit(x);
    	    	shoutcast_child_disconnect(fd);
    	    	pthread_mutex_lock(&x->x_mutex);
				x->x_fd = -1;
			}
			x->x_connectstate = 0;
			clock_delay(x->x_clock_connect, 0);
			x->x_requestcode = REQUEST_NOTHING;
			shoutcast_cond_signal(&x->x_answercondition);
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

static void shoutcast_tick_connect(t_shoutcast *x)
{
	pthread_mutex_lock(&x->x_mutex);
	outlet_float(x->x_connection, x->x_connectstate);
	pthread_mutex_unlock(&x->x_mutex);
}

static void shoutcast_tick_pages(t_shoutcast *x)
{
		/* output kB we sent to server */
	t_float bytes;
	pthread_mutex_lock(&x->x_mutex);
	bytes = x->x_streamedbytes;
	pthread_mutex_unlock(&x->x_mutex);
	outlet_float(x->x_outpages, (t_int)(bytes / 1024));
	x->x_oldstreamedbytes = bytes;
	clock_delay(x->x_clock_pages, UPDATE_INTERVAL);
}

static void *shoutcast_new(t_floatarg fnchannels, t_floatarg fbufsize)
{
    t_shoutcast *x;
    int nchannels = fnchannels, bufsize = fbufsize * 1024, i;
    float *buf;
    
    if (nchannels < 1)
    	nchannels = 2;		/* two channels as default */
    else if (nchannels > MAXSTREAMCHANS)
    	nchannels = MAXSTREAMCHANS;
		/* check / set buffer size */
    if (bufsize <= 0) bufsize = DEFBUFPERCHAN * nchannels;
    else if (bufsize < MINBUFSIZE)
    	bufsize = MINBUFSIZE;
    else if (bufsize > MAXBUFSIZE)
    	bufsize = MAXBUFSIZE;
    buf = getbytes(bufsize*sizeof(t_float));
    if (!buf) return (0);
    
    x = (t_shoutcast *)pd_new(shoutcast_class);
    
    for (i = 1; i < nchannels; i++)
		inlet_new (&x->x_obj, &x->x_obj.ob_pd, gensym ("signal"), gensym ("signal"));
    x->x_connection = outlet_new(&x->x_obj, gensym("float"));
    x->x_outpages = outlet_new(&x->x_obj, gensym("float"));
    x->x_ninlets = nchannels;

	x->x_clock_connect = clock_new(x, (t_method)shoutcast_tick_connect);
	x->x_clock_pages = clock_new(x, (t_method)shoutcast_tick_pages);

    x->x_mp3inbuf = getbytes(MY_MP3_MALLOC_IN_SIZE*sizeof(short));  /* buffer for encoder input */
    x->x_mp3outbuf = getbytes(MY_MP3_MALLOC_OUT_SIZE*sizeof(char)); /* our mp3 stream */

    pthread_mutex_init(&x->x_mutex, 0);
    pthread_cond_init(&x->x_requestcondition, 0);
    pthread_cond_init(&x->x_answercondition, 0);

    x->x_vecsize = 2;
    x->x_state = STATE_IDLE;
    x->x_buf = buf;
    x->x_bufsize = bufsize;
	x->x_siginterval = 32;	/* signal 32 times per buffer */
    x->x_fifosize = x->x_fifohead = x->x_fifotail = x->x_requestcode = 0;

	x->x_connectstate = 0;  /* indicating state of connection */
	x->x_outvalue = 0;      /* value at output currently is 0 */
    
    x->x_samplerate = sys_getsr();
    x->x_fd = -1;
	x->x_vbr = -1;                  /* don't use the vbr setting by default */
    x->x_passwd = "letmein";
    x->x_samplerate = sys_getsr();	/* default to Pd's sampling rate */
    x->x_mp3quality = 5;            /* quality 5 - medium CPU usage */
	x->x_channels = ENCODER_CHANNELS;
	x->x_lamechunk = 2304;          /* the mp3 chunk size for stereo */
    x->x_bitrate = 128;
	x->x_streamedbytes = x->x_oldstreamedbytes = 0;
	x->x_bcname = "mp3 stream emitted from Pure Data";
	x->x_bcurl = "http://www.akustische-kunst.org/";
	x->x_bcgenre = "experimental";
	x->x_bcpublic = 1;
	x->x_mountpoint = "puredata.mp3";
	x->x_hostname = "localhost";
	x->x_port = 8000;
    
    post(shoutcast_version);
	post("shoutcast~: set buffer to %dk bytes", bufsize / 1024);
	post("shoutcast~: encoding %d channels @ %d Hz", x->x_channels, x->x_samplerate);
	if(x->x_ninlets > ENCODER_CHANNELS)
	{
		post("shoutcast~: a maximum of 2 channels is supported");
		x->x_ninlets = 2;
	}

    clock_delay(x->x_clock_pages, 0);
	pthread_create(&x->x_childthread, 0, shoutcast_child_main, x);
    return (x);
}

static t_int *shoutcast_perform(t_int *w)
{
    t_shoutcast *x = (t_shoutcast *)(w[1]);
    int vecsize = x->x_vecsize, ninlets = x->x_ninlets, channels = x->x_channels, i, j, k;
	float *sp = x->x_buf;
	pthread_mutex_lock(&x->x_mutex);
    if (x->x_state != STATE_IDLE)	/* don't fill buffers in case everything's turned off */
    {
    	int wantbytes;


			/* get 'wantbytes' bytes from input */
		wantbytes = channels * vecsize;	/* we need vecsize bytes per channel */

			/* check if there is enough space in buffer to write samples */
		while((x->x_fifotail > x->x_fifohead &&
			x->x_fifotail < x->x_fifohead + wantbytes + 1) && x->x_fd > 0)
		{
			pute("wait...\n");
			shoutcast_cond_signal(&x->x_requestcondition);
			shoutcast_cond_wait(&x->x_answercondition, &x->x_mutex);
			pute("done\n");
		}

			/* output audio */
		sp += x->x_fifohead;

		if(ninlets >= channels)
		{
			for(j = 0; j < vecsize; j++)
			{
				for(i = 0; i < channels; i++)
				{
					*sp++ = x->x_outvec[i][j];
				}
			}
		}
		else if(channels > ninlets)	/* convert mono -> stereo */
		{
			for(j = 0; j < vecsize; j++)
			{
				for(i = 0; i < ninlets; i++)
				{
					for(k = 0; k < channels; k++)
						*sp++ = x->x_outvec[i][j];
				}
			}
		}

		
		x->x_fifohead += wantbytes;
		if (x->x_fifohead >= x->x_fifosize)
			x->x_fifohead = 0;
			/* signal the child thread */
		if ((--x->x_sigcountdown) <= 0)
		{
    		shoutcast_cond_signal(&x->x_requestcondition);
			x->x_sigcountdown = x->x_sigperiod;
		}
    }

	pthread_mutex_unlock(&x->x_mutex);

    return (w+2);
}


static void shoutcast_disconnect(t_shoutcast *x)
{
    	/* LATER rethink whether you need the mutex just to set a variable? */
    pthread_mutex_lock(&x->x_mutex);
	if(x->x_fd >= 0)
	{
		x->x_state = STATE_IDLE;
		x->x_requestcode = REQUEST_CLOSE;
		shoutcast_cond_signal(&x->x_requestcondition);
	}
	else post("shoutcast~: not connected");
    pthread_mutex_unlock(&x->x_mutex);
}


    /* connect method.  Called as:
       connect <hostname or IP> <mountpoint> <portnumber>
    */

static void shoutcast_connect(t_shoutcast *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *hostsym, *mountsym;
    t_float portno;
    if(argc > 0)hostsym = atom_getsymbolarg(0, argc, argv);
    if(argc > 1)mountsym = atom_getsymbolarg(1, argc, argv);
    if(argc > 2)portno = atom_getfloatarg(2, argc, argv);
    pthread_mutex_lock(&x->x_mutex);
	if(x->x_fd >= 0)
	{
		post("shoutcast~: already connected");
	}
	else
	{
		x->x_requestcode = REQUEST_CONNECT;
		if(argc >= 1)x->x_hostname = hostsym->s_name;
		if(argc >= 2)x->x_mountpoint = mountsym->s_name;
		if(argc >= 3)x->x_port = portno;

		x->x_fifotail = 0;
		x->x_fifohead = 0;

		x->x_connecterror = 0;
		x->x_streamedbytes = x->x_oldstreamedbytes = 0;
		x->x_state = STATE_STARTUP;
		shoutcast_cond_signal(&x->x_requestcondition);
	}
    pthread_mutex_unlock(&x->x_mutex);
}

static void shoutcast_float(t_shoutcast *x, t_floatarg f)
{
    if (f != 0)
	{
		pthread_mutex_lock(&x->x_mutex);
		if(x->x_fd >= 0)
		{
			post("shoutcast~: already connected");
		}
		else
		{
			x->x_requestcode = REQUEST_CONNECT;

			x->x_fifotail = 0;
			x->x_fifohead = 0;

			x->x_connecterror = 0;
			x->x_state = STATE_STARTUP;
			shoutcast_cond_signal(&x->x_requestcondition);
		}
		pthread_mutex_unlock(&x->x_mutex);
	}
    else shoutcast_disconnect(x);
}

static void shoutcast_dsp(t_shoutcast *x, t_signal **sp)
{
    int i, ninlets = x->x_ninlets;
    pthread_mutex_lock(&x->x_mutex);
    x->x_vecsize = sp[0]->s_n;
    
    x->x_sigperiod = (x->x_fifosize / (x->x_siginterval * ninlets * x->x_vecsize));
    for (i = 0; i < ninlets; i++)
    	x->x_outvec[i] = sp[i]->s_vec;
    pthread_mutex_unlock(&x->x_mutex);
    dsp_add(shoutcast_perform, 1, x);
}

    /* set password for shoutcast server */
static void shoutcast_password(t_shoutcast *x, t_symbol *password)
{
    pthread_mutex_lock(&x->x_mutex);
    x->x_passwd = password->s_name;
    pthread_mutex_unlock(&x->x_mutex);
}

	/* set broadcast name */
static void shoutcast_bcname(t_shoutcast *x, t_symbol *s, int ac, t_atom* av)
{
	t_binbuf *b = binbuf_new();
	char* text;
	int length;
	binbuf_add(b, ac, av);
	binbuf_gettext(b, &text, &length);
	// text[length] = '\0';
    pthread_mutex_lock(&x->x_mutex);
	x->x_bcname = text;
	post("shoutcast~: set broadcast name to:\n     %s", x->x_bcname);
	if(x->x_state == STATE_STREAM) 
		post("shoutcast~: reconnect to make changes take effect!");
    pthread_mutex_unlock(&x->x_mutex);
	binbuf_free(b);
}

	/* set broadcast genre */
static void shoutcast_bcgenre(t_shoutcast *x, t_symbol *s, int ac, t_atom* av)
{
	t_binbuf *b = binbuf_new();
	char* text;
	int length;
	binbuf_add(b, ac, av);
	binbuf_gettext(b, &text, &length);
	// text[length] = '\0';
    pthread_mutex_lock(&x->x_mutex);
	x->x_bcgenre = text;
	post("shoutcast~: set broadcast genre to:\n     %s", x->x_bcgenre);
	if(x->x_state == STATE_STREAM) 
		post("shoutcast~: reconnect to make changes take effect!");
    pthread_mutex_unlock(&x->x_mutex);
	binbuf_free(b);
}

	/* set broadcast URL */
static void shoutcast_bcurl(t_shoutcast *x, t_symbol *s, int ac, t_atom* av)
{
	t_binbuf *b = binbuf_new();
	char* text;
	int length;
	binbuf_add(b, ac, av);
	binbuf_gettext(b, &text, &length);
	// text[length] = '\0';
    pthread_mutex_lock(&x->x_mutex);
	x->x_bcurl = text;
	post("shoutcast~: set broadcast URL to:\n     %s", x->x_bcurl);
	if(x->x_state == STATE_STREAM) 
		post("shoutcast~: reconnect to make changes take effect!");
    pthread_mutex_unlock(&x->x_mutex);
	binbuf_free(b);
}

	/* set broadcast public */
static void shoutcast_bcpublic(t_shoutcast *x, t_floatarg fp)
{
    pthread_mutex_lock(&x->x_mutex);
	x->x_bcpublic = (t_int)fp;
	if(x->x_bcpublic == 0)
		post("shoutcast~: set broadcast to not public");
	else
		post("shoutcast~: set broadcast to public");
	if(x->x_state == STATE_STREAM) 
		post("shoutcast~: reconnect to make changes take effect!");
    pthread_mutex_unlock(&x->x_mutex);
}

static void shoutcast_icecast(t_shoutcast *x)
{
	x->x_icecast = 1;
	post("shoutcast~: set server type to IceCast");
}

	/* we use SHOUTcast server (default) */
static void shoutcast_shoutcast(t_shoutcast *x)
{
	x->x_icecast = 0;
	post("shoutcast~: set server type to SHOUTcast");
}

    /* settings for mp3 encoding */
static void shoutcast_mpeg(t_shoutcast *x, t_floatarg fsamplerate, t_floatarg fbitrate,
                           t_floatarg fmode, t_floatarg fquality)
{
	char *mode[4] = { "stereo", "joint stereo", "dual channel", "mono"};
    pthread_mutex_lock(&x->x_mutex);
    x->x_samplerate = fsamplerate;
    if(fbitrate > MAXDATARATE)
    {
        fbitrate = MAXDATARATE;
    }
    x->x_bitrate = (t_int)fbitrate;
    x->x_mp3mode = (t_int)fmode;
	x->x_vbr = -1;
	x->x_channels = ENCODER_CHANNELS;
    x->x_mp3quality = (t_int)fquality;
    post("shoutcast~: mp3 stream: CBR, %dHz, %dkbit/s, %s (mode %d), quality %d",
          x->x_samplerate, x->x_bitrate, mode[x->x_mp3mode], x->x_mp3mode, x->x_mp3quality);
    if(x->x_fd>=0)post("shoutcast~: reconnect to make changes take effect! ");
    pthread_mutex_unlock(&x->x_mutex);
}

    /* settings for vbr mp3 encoding */
static void shoutcast_vbr(t_shoutcast *x, t_floatarg fsamplerate, t_floatarg fmode, t_floatarg fquality)
{
	char *mode[4] = { "stereo", "joint stereo", "dual channel", "mono"};
    pthread_mutex_lock(&x->x_mutex);
    x->x_samplerate = fsamplerate;
    x->x_bitrate = 0;
	x->x_vbr = (t_int)fquality;
    x->x_mp3mode = (t_int)fmode;
	x->x_channels = 2;
    post("shoutcast~: mp3 stream: VBR, %dHz, %s (mode %d), quality %d",
          x->x_samplerate, mode[x->x_mp3mode], x->x_mp3mode, x->x_vbr);
    if(x->x_fd>=0)post("shoutcast~: reconnect to make changes take effect! ");
    pthread_mutex_unlock(&x->x_mutex);
}

    /* print settings to pd's console window */
static void shoutcast_print(t_shoutcast *x)
{
    const char        *mode[4] = { "stereo", "joint stereo", "dual channel", "mono" };
    pthread_mutex_lock(&x->x_mutex);
    post(shoutcast_version);
    post("  LAME mp3 settings:\n"
         "    output sample rate: %d Hz", x->x_samplerate);
    post("    mode: %s", mode[x->x_mp3mode]);
	if(x->x_vbr == -1)
	{
		post("    bitrate: %d kB/s", x->x_bitrate);
		post("    quality: %d", x->x_mp3quality);
	}
	else
	{
		post("    using variable bitrate encoding");
		post("    quality: %d", x->x_vbr);
	}
    post("    mp3 chunk size: %d", x->x_lamechunk);
    if(x->x_samplerate!=sys_getsr())
    {
        post("    resampling from %d to %d Hz!", (t_int)sys_getsr(), x->x_samplerate);
    }
	if(x->x_icecast == 0)
	{
		post("  server type: SHOUTcast");
		if(x->x_fd > 0)
			post("  server state: connected");
		else
			post("  server state: not connected");
		post("  URL: http://%s:%d/", x->x_hostname, (t_int)x->x_port);
	}
	else
	{
		post("  server type: IceCast");
		if(x->x_fd > 0)
			post("  server state: connected");
		else
			post("  server state: not connected");
		post("  stream URL: http://%s:%d/%s", x->x_hostname, (t_int)x->x_port, x->x_mountpoint);
	}
    pthread_mutex_unlock(&x->x_mutex);
}

    /* print lame settings to pd's console window */
static void shoutcast_printlame(t_shoutcast *x)
{
    pthread_mutex_lock(&x->x_mutex);
	if(x->x_state == STATE_STREAM) 
		lame_print_config(x->x_lgfp);
	else
		post("shoutcast~: can't print, lame is not running");
    pthread_mutex_unlock(&x->x_mutex);
}

static void shoutcast_free(t_shoutcast *x)
{
    	/* request QUIT and wait for acknowledge */
    void *threadrtn;
    pthread_mutex_lock(&x->x_mutex);
    x->x_requestcode = REQUEST_QUIT;
    post("stopping shoutcast thread...");
    shoutcast_cond_signal(&x->x_requestcondition);
    while (x->x_requestcode != REQUEST_NOTHING)
    {
    	post("signalling...");
		shoutcast_cond_signal(&x->x_requestcondition);
    	shoutcast_cond_wait(&x->x_answercondition, &x->x_mutex);
    }
    pthread_mutex_unlock(&x->x_mutex);
    if (pthread_join(x->x_childthread, &threadrtn))
    	error("shoutcast_free: join failed");
    post("... done.");
    
    pthread_cond_destroy(&x->x_requestcondition);
    pthread_cond_destroy(&x->x_answercondition);
    pthread_mutex_destroy(&x->x_mutex);
    freebytes(x->x_buf, x->x_bufsize*sizeof(t_float));
    freebytes(x->x_mp3inbuf, MY_MP3_MALLOC_IN_SIZE*sizeof(short));
    freebytes(x->x_mp3outbuf, MY_MP3_MALLOC_OUT_SIZE);
	clock_free(x->x_clock_connect);
	clock_free(x->x_clock_pages);
}

void shoutcast_tilde_setup(void)
{
    shoutcast_class = class_new(gensym("shoutcast~"), (t_newmethod)shoutcast_new, 
    	(t_method)shoutcast_free, sizeof(t_shoutcast), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(shoutcast_class, t_shoutcast, x_f );
    class_addfloat(shoutcast_class, (t_method)shoutcast_float);
    class_addmethod(shoutcast_class, (t_method)shoutcast_disconnect, gensym("disconnect"), 0);
    class_addmethod(shoutcast_class, (t_method)shoutcast_dsp, gensym("dsp"), 0);
    class_addmethod(shoutcast_class, (t_method)shoutcast_connect, gensym("connect"), A_GIMME, 0);
    class_addmethod(shoutcast_class, (t_method)shoutcast_print, gensym("print"), 0);
    class_addmethod(shoutcast_class, (t_method)shoutcast_printlame, gensym("printlame"), 0);
	class_addmethod(shoutcast_class, (t_method)shoutcast_icecast, gensym("icecast"), 0);
	class_addmethod(shoutcast_class, (t_method)shoutcast_shoutcast, gensym("shoutcast"), 0);
    class_addmethod(shoutcast_class, (t_method)shoutcast_password, gensym("passwd"), A_SYMBOL, 0);
    class_addmethod(shoutcast_class, (t_method)shoutcast_mpeg, gensym("mpeg"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
	class_addmethod(shoutcast_class, (t_method)shoutcast_mpeg, gensym("cbr"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
	class_addmethod(shoutcast_class, (t_method)shoutcast_vbr, gensym("vbr"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
	class_addmethod(shoutcast_class, (t_method)shoutcast_bcname, gensym("name"), A_GIMME, 0);
	class_addmethod(shoutcast_class, (t_method)shoutcast_bcgenre, gensym("genre"), A_GIMME, 0);
	class_addmethod(shoutcast_class, (t_method)shoutcast_bcurl, gensym("url"), A_GIMME, 0);
	class_addmethod(shoutcast_class, (t_method)shoutcast_bcpublic, gensym("public"), A_FLOAT, 0);
    class_sethelpsymbol(shoutcast_class, gensym("help-shoutcast~.pd"));
}