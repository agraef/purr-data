/* ------------------------ netreceive~ --------------------------------------- */
/*                                                                              */
/* Tilde object to receive uncompressed audio data from netsend~.               */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>.                               */
/* Based on streamin~ by Guenter Geiger.                                        */
/* Get source at http://www.akustische-kunst.org/                               */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* See file LICENSE for further informations on licensing terms.                */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* Based on PureData by Miller Puckette and others.                             */
/*                                                                              */
/* This project was commissioned by the Society for Arts and Technology [SAT],  */
/* Montreal, Quebec, Canada, http://www.sat.qc.ca/.                             */
/*                                                                              */
/* ---------------------------------------------------------------------------- */


#ifdef PD
#include "m_pd.h"
#else
#include "ext.h"
#include "z_dsp.h"
#endif

#include "netsend~.h"

#ifdef USE_FAAC
#include "faad/faad.h"
#endif

#include <sys/types.h>
#include <string.h>
#ifdef UNIX
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
#define SOCKET_ERROR -1
#else
#include <winsock.h>
#endif

#ifndef SOL_IP
#define SOL_IP IPPROTO_IP
#endif

#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define DEFAULT_AUDIO_BUFFER_FRAMES 16	/* a small circ. buffer for 16 frames */
#define DEFAULT_AVERAGE_NUMBER 10		/* number of values we store for average history */
#define DEFAULT_NETWORK_POLLTIME 1		/* interval in ms for polling for input data (Max/MSP only) */
#define DEFAULT_QUEUE_LENGTH 3			/* min. number of buffers that can be used reliably on your hardware */


#ifdef UNIX
#define CLOSESOCKET(fd) close(fd)
#endif
#ifdef _WINDOWS
#define CLOSESOCKET(fd) closesocket(fd)
#endif

#ifdef PD
/* these would require to include some headers that are different
   between pd 0.36 and later, so it's easier to do it like this! */
EXTERN void sys_rmpollfn(int fd);
EXTERN void sys_addpollfn(int fd, void* fn, void *ptr);
#endif

static int netreceive_tilde_sockerror(char *s)
{
#ifdef NT
    int err = WSAGetLastError();
    if (err == 10054) return 1;
    else if (err == 10040) post("netsend~: %s: message too long (%d)", s, err);
    else if (err == 10053) post("netsend~: %s: software caused connection abort (%d)", s, err);
    else if (err == 10055) post("netsend~: %s: no buffer space available (%d)", s, err);
    else if (err == 10060) post("netsend~: %s: connection timed out (%d)", s, err);
    else if (err == 10061) post("netsend~: %s: connection refused (%d)", s, err);
    else post("netreceive~: %s: %s (%d)", s, strerror(err), err);
#else
    int err = errno;
    post("netreceive~: %s: %s (%d)", s, strerror(err), err);
#endif
#ifdef NT
	if (err == WSAEWOULDBLOCK)
#endif
#ifdef UNIX
	if (err == EAGAIN)
#endif
	{
		return 1;	/* recoverable error */
	}
	return 0;	/* indicate non-recoverable error */
}


static int netreceive_tilde_setsocketoptions(int sockfd)
{ 
	int sockopt = 1;
	if (setsockopt(sockfd, SOL_IP, TCP_NODELAY, (const char*)&sockopt, sizeof(int)) < 0)
		post("setsockopt NODELAY failed");

	sockopt = 1;    
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&sockopt, sizeof(int)) < 0)
		post("netreceive~: setsockopt REUSEADDR failed");
	return 0;
}



/* ------------------------ netreceive~ ----------------------------- */


static t_class *netreceive_tilde_class;
static t_symbol *ps_format, *ps_channels, *ps_framesize, *ps_overflow, *ps_underflow,
                *ps_queuesize, *ps_average, *ps_sf_float, *ps_sf_16bit, *ps_sf_8bit, 
                *ps_sf_mp3, *ps_sf_aac, *ps_sf_unknown, *ps_bitrate, *ps_hostname, *ps_nothing;


typedef struct _netreceive_tilde
{
#ifdef PD
	t_object x_obj;
	t_outlet *x_outlet1;
	t_outlet *x_outlet2;
#else
	t_pxobject x_obj;
	void *x_outlet1;
	void *x_outlet2;
	void *x_connectpoll;
	void *x_datapoll;
#endif
	int x_socket;
	int x_connectsocket;
	int x_nconnections;
	int x_ndrops;
	int x_tcp;
	t_symbol *x_hostname;

	/* buffering */
	int x_framein;
	int x_frameout;
	t_frame x_frames[DEFAULT_AUDIO_BUFFER_FRAMES];
	int x_maxframes;
	long x_framecount;
	int x_blocksize;
	int x_blocksperrecv;
	int x_blockssincerecv;

	int x_nbytes;
	int x_counter;
	int x_average[DEFAULT_AVERAGE_NUMBER];
	int x_averagecur;
	int x_underflow;
	int x_overflow;

#ifdef USE_FAAC
    faacDecHandle x_faac_decoder;
    faacDecFrameInfo x_faac_frameInfo;
    faacDecConfigurationPtr x_faac_config;
	int x_faac_init;
	unsigned char x_faac_buf[FAAD_MIN_STREAMSIZE * DEFAULT_AUDIO_CHANNELS];
	unsigned long x_faac_bytes;
#endif

	long x_samplerate;
	int x_noutlets;
	int x_vecsize;
	t_int **x_myvec;            /* vector we pass on to the DSP routine */
} t_netreceive_tilde;



/* prototypes (as needed) */
static void netreceive_tilde_kick(t_netreceive_tilde *x);



#ifdef USE_FAAC
/* open encoder and set default values */
static void netreceive_tilde_faac_open(t_netreceive_tilde* x)
{
	x->x_faac_decoder = faacDecOpen();
	x->x_faac_config = faacDecGetCurrentConfiguration(x->x_faac_decoder);
	x->x_faac_config->defSampleRate = x->x_samplerate;
	x->x_faac_config->defObjectType = MAIN; // LC;
	x->x_faac_config->outputFormat = FAAD_FMT_FLOAT;
	faacDecSetConfiguration(x->x_faac_decoder, x->x_faac_config);
	x->x_faac_init = 0;
	x->x_faac_bytes = 0;
}

static void netreceive_tilde_faac_close(t_netreceive_tilde* x)
{
	if (x->x_faac_decoder != NULL)
		faacDecClose(x->x_faac_decoder);
	x->x_faac_decoder = NULL;
	x->x_faac_init = 0;
}

/* init decoder when we get a new stream */
static int netreceive_tilde_faac_init(t_netreceive_tilde* x, int frame)
{
	unsigned long samplerate;
    unsigned char channels;
	long bytes_consumed = 0;

	if ((bytes_consumed = faacDecInit(x->x_faac_decoder, x->x_faac_buf, x->x_faac_bytes, &samplerate, &channels)) < 0)
	{
		faacDecConfigurationPtr config;
		error("netreceive~: faac: initializing decoder library failed");
		netreceive_tilde_faac_close(x);
		return -1;
	}
	else if (samplerate != (unsigned long)x->x_samplerate)
	{
		error("netreceive~: incoming stream has wrong samplerate");
		netreceive_tilde_faac_close(x);
		return -1;
	}

	/* adjust accumulating AAC buffer */
	memmove(x->x_faac_buf, x->x_faac_buf + bytes_consumed, x->x_faac_bytes - bytes_consumed);
	x->x_faac_bytes -= bytes_consumed;

	x->x_faac_init = 1;	/* indicate that decoder is ready */
	return 0;
}

/* decode AAC using FAAD2 library */
static int netreceive_tilde_faac_decode(t_netreceive_tilde* x, int frame)
{
	unsigned int i, ret;
	float *sample_buffer;

	/* open decoder, if not yet done */
	if (x->x_faac_decoder == NULL)
	{
		netreceive_tilde_faac_open(x);
	}

	/* add new AAC data into buffer */
	memcpy(x->x_faac_buf + x->x_faac_bytes, x->x_frames[frame].data, x->x_frames[frame].tag.framesize);
	x->x_faac_bytes += x->x_frames[frame].tag.framesize;

	/* in case we have more than FAAD_MIN_STREAMSIZE bytes per channel try decoding */
	if (x->x_faac_bytes >= (unsigned long)(FAAD_MIN_STREAMSIZE * x->x_frames[frame].tag.channels))
	{
		/* init decoder, if not yet done */
		if (!x->x_faac_init)
		{
			ret = netreceive_tilde_faac_init(x, frame);
			if (ret == -1)
			{
				return -1;
			}
		}

		/* decode data */
		memset(&x->x_faac_frameInfo, 0, sizeof(faacDecFrameInfo));
		sample_buffer = (float *)faacDecDecode(x->x_faac_decoder, &x->x_faac_frameInfo, x->x_faac_buf, x->x_faac_bytes);
		if (x->x_faac_frameInfo.error != 0)
		{
			error("netreceive~: faac: %s", faacDecGetErrorMessage(x->x_faac_frameInfo.error));
			netreceive_tilde_faac_close(x);
			return -1;
		}

		/* adjust accumulating AAC buffer */
		memmove(x->x_faac_buf, x->x_faac_buf + x->x_faac_frameInfo.bytesconsumed, x->x_faac_bytes - x->x_faac_frameInfo.bytesconsumed);
		x->x_faac_bytes -= x->x_faac_frameInfo.bytesconsumed;

		/* copy decoded PCM samples back to frame */
		memcpy(x->x_frames[frame].data, sample_buffer, x->x_faac_frameInfo.samples * sizeof(float));

		/* return number of decoded PCM samples */
		return x->x_faac_frameInfo.samples * SF_SIZEOF(SF_FLOAT);
	}
	else
	{
		return 0;	/* indicate we didn't get any new audio data */
	}
}
#endif	/* USE_FAAC */




/* remove all pollfunctions and close socket */
static void netreceive_tilde_closesocket(t_netreceive_tilde* x)
{
#ifdef PD
	sys_rmpollfn(x->x_socket);
	outlet_float(x->x_outlet1, 0);
#else
	clock_unset(x->x_datapoll);
	outlet_int(x->x_outlet1, 0);
#endif
	CLOSESOCKET(x->x_socket);
	x->x_socket = -1;
}



#ifdef PD
static void netreceive_tilde_reset(t_netreceive_tilde* x, t_floatarg buffer)
#else
static void netreceive_tilde_reset(t_netreceive_tilde* x, double buffer)
#endif
{
	int i;
	x->x_counter = 0;
	x->x_nbytes = 0;
	x->x_framein = 0;
	x->x_frameout = 0;
	x->x_blockssincerecv = 0;
	x->x_blocksperrecv = x->x_blocksize / x->x_vecsize;
#ifdef USE_FAAC
	x->x_faac_bytes = 0;
#endif

	for (i = 0; i < DEFAULT_AVERAGE_NUMBER; i++)
		x->x_average[i] = x->x_maxframes;
	x->x_averagecur = 0;

	if (buffer == 0.0)	/* set default */
		x->x_maxframes = DEFAULT_QUEUE_LENGTH;
	else
	{
		buffer = (float)CLIP((float)buffer, 0., 1.);
		x->x_maxframes = (int)(DEFAULT_AUDIO_BUFFER_FRAMES * buffer);
		x->x_maxframes = CLIP(x->x_maxframes, 1, DEFAULT_AUDIO_BUFFER_FRAMES - 1);
		post("netreceive~: set buffer to %g (%d frames)", buffer, x->x_maxframes);
	}
	x->x_underflow = 0;
	x->x_overflow = 0;
}


static void netreceive_tilde_datapoll(t_netreceive_tilde *x)
{
#ifndef PD
	int ret;
    struct timeval timout;
    fd_set readset;
    timout.tv_sec = 0;
    timout.tv_usec = 0;
    FD_ZERO(&readset);
    FD_SET(x->x_socket, &readset);

	ret = select(x->x_socket + 1, &readset, NULL, NULL, &timout);
    if (ret < 0)
    {
    	netreceive_tilde_sockerror("select");
		return;
    }

	if (FD_ISSET(x->x_socket, &readset))	/* data available */
#endif
	{
		int ret;
		int n;

		if (x->x_tcp)
		{
			n = x->x_nbytes;

			if (x->x_nbytes == 0)	/* we ate all the samples and need a new header tag */
			{
				/* get the new tag */
				ret = recv(x->x_socket, (char*)&x->x_frames[x->x_framein].tag, sizeof(t_tag), MSG_PEEK);
				if (ret == 0)	/* disconnect */
				{
					post("netreceive~: EOF on socket %d", x->x_socket);
					netreceive_tilde_closesocket(x);
					x->x_socket = -1;
					x->x_counter = 0;
					return;
				}
				if (ret < 0)	/* error */
				{
					if (netreceive_tilde_sockerror("recv tag"))
						goto bail;
					netreceive_tilde_closesocket(x);
					x->x_socket = -1;
					x->x_counter = 0;
					return;
				}
				else if (ret != sizeof(t_tag))
				{
					/* incomplete header tag: return and try again later */
					/* in the hope that more data will be available */
					return;
				}

				/* receive header tag */
				ret = recv(x->x_socket, (char*)&x->x_frames[x->x_framein].tag, sizeof(t_tag), 0);

				/* adjust byte order if neccessarry */
				if (x->x_frames[x->x_framein].tag.version != SF_BYTE_NATIVE)
				{
					x->x_frames[x->x_framein].tag.count = netsend_long(x->x_frames[x->x_framein].tag.count);
					x->x_frames[x->x_framein].tag.framesize = netsend_long(x->x_frames[x->x_framein].tag.framesize);
				}

				/* get info from header tag */
				if (x->x_frames[x->x_framein].tag.channels > x->x_noutlets)
				{
					error("netreceive~: incoming stream has too many channels (%d), kicking client", x->x_frames[x->x_framein].tag.channels);
					netreceive_tilde_kick(x);
					x->x_socket = -1;
					x->x_counter = 0;
					return;
				}
				x->x_nbytes = n = x->x_frames[x->x_framein].tag.framesize;

				/* check whether the data packet has the correct count */
				if ((x->x_framecount != x->x_frames[x->x_framein].tag.count)
					&& (x->x_frames[x->x_framein].tag.count > 2))
				{
					error("netreceive~: we lost %d frames", (int)(x->x_frames[x->x_framein].tag.count - x->x_framecount));
					post("netreceive~: current package is %d, expected %d", x->x_frames[x->x_framein].tag.count, x->x_framecount);
				}
				x->x_framecount = x->x_frames[x->x_framein].tag.count + 1;
			}
			else	/* we already have the header tag or some data and need more */
			{
				ret = recv(x->x_socket, (char*)x->x_frames[x->x_framein].data + x->x_frames[x->x_framein].tag.framesize - n, n, 0);
				if (ret > 0)
				{
					n -= ret;
				}
				else if (ret < 0)	/* error */
				{
					if (netreceive_tilde_sockerror("recv data"))
						goto bail;
					netreceive_tilde_closesocket(x);
					x->x_socket = -1;
					x->x_counter = 0;
					return;
				}

				x->x_nbytes = n;
				if (n == 0)			/* a complete packet is received */
				{
#ifdef USE_FAAC     /* decode aac data if format is SF_AAC */
					if (x->x_frames[x->x_framein].tag.format == SF_AAC)
					{
						ret = netreceive_tilde_faac_decode(x, x->x_framein);
						if (ret == -1)
						{
							netreceive_tilde_kick(x);
							x->x_socket = -1;
							x->x_counter = 0;
							return;
						}
						else
						{
							/* update framesize */
							x->x_frames[x->x_framein].tag.framesize = ret;
						}
					}
#else
					if (x->x_frames[x->x_framein].tag.format == SF_AAC)
					{
						error("netreceive~: don't know how to decode AAC format");
						netreceive_tilde_kick(x);
						x->x_socket = -1;
						x->x_counter = 0;
						return;
					}
#endif

					x->x_counter++;
					x->x_framein++;
					x->x_framein %= DEFAULT_AUDIO_BUFFER_FRAMES;

					/* check for buffer overflow */
					if (x->x_framein == x->x_frameout)
					{
						x->x_overflow++;
					}
				}
			}
		}
		else /* UDP */
		{
			n = x->x_nbytes;

			if (x->x_nbytes == 0)	/* we ate all the samples and need a new header tag */
			{
				/* receive header tag */
				ret = recv(x->x_socket, (char*)&x->x_frames[x->x_framein].tag, sizeof(t_tag), 0);
				if (ret <= 0)	/* error */
				{
					if (netreceive_tilde_sockerror("recv tag"))
						goto bail;
					netreceive_tilde_reset(x, 0);
					x->x_counter = 0;
					return;
				}
				else if (ret != sizeof(t_tag))
				{
					/* incomplete header tag: return and try again later */
					/* in the hope that more data will be available */
					error("netreceive~: got incomplete header tag");
					return;
				}
				/* adjust byte order if neccessarry */
				if (x->x_frames[x->x_framein].tag.version != SF_BYTE_NATIVE)
				{
					x->x_frames[x->x_framein].tag.count = netsend_long(x->x_frames[x->x_framein].tag.count);
					x->x_frames[x->x_framein].tag.framesize = netsend_long(x->x_frames[x->x_framein].tag.framesize);
				}
				/* get info from header tag */
				if (x->x_frames[x->x_framein].tag.channels > x->x_noutlets)
				{
					error("netreceive~: incoming stream has too many channels (%d)", x->x_frames[x->x_framein].tag.channels);
					x->x_counter = 0;
					return;
				}
				x->x_nbytes = n = x->x_frames[x->x_framein].tag.framesize;
			}
			else	/* we already have header tag or some data and need more */
			{
				ret = recv(x->x_socket, (char*)x->x_frames[x->x_framein].data + x->x_frames[x->x_framein].tag.framesize - n, n, 0);
				if (ret > 0)
				{
					n -= ret;
				}
				else if (ret < 0)	/* error */
				{
					if (netreceive_tilde_sockerror("recv data"))
						goto bail;
					netreceive_tilde_reset(x, 0);
					x->x_counter = 0;
					return;
				}

				x->x_nbytes = n;
				if (n == 0)			/* a complete packet is received */
				{
#ifdef USE_FAAC     /* decode aac data if format is SF_AAC and update framesize */
					if (x->x_frames[x->x_framein].tag.format == SF_AAC)
					{
						ret = netreceive_tilde_faac_decode(x, x->x_framein);
						if (ret == -1)
						{
							return;
						}
						else
						{
							/* update framesize */
							x->x_frames[x->x_framein].tag.framesize = ret;
						}
					}
#else
					if (x->x_frames[x->x_framein].tag.format == SF_AAC)
					{
						error("netreceive~: don't know how to decode AAC format");
						return;
					}
#endif
					x->x_counter++;
					x->x_framein++;
					x->x_framein %= DEFAULT_AUDIO_BUFFER_FRAMES;

					/* check for buffer overflow */
					if (x->x_framein == x->x_frameout)
					{
						x->x_overflow++;
					}
				}
			}
		}
	}
bail:
	;
#ifndef PD
	clock_delay(x->x_datapoll, DEFAULT_NETWORK_POLLTIME);
#endif
}


static void netreceive_tilde_connectpoll(t_netreceive_tilde *x)
{
#ifndef PD
	int ret;
    struct timeval timout;
    fd_set readset;
    timout.tv_sec = 0;
    timout.tv_usec = 0;
    FD_ZERO(&readset);
    FD_SET(x->x_connectsocket, &readset);

	ret = select(x->x_connectsocket + 1, &readset, NULL, NULL, &timout);
    if (ret < 0)
    {
    	netreceive_tilde_sockerror("select");
		return;
    }

	if (FD_ISSET(x->x_connectsocket, &readset))	/* pending connection */
#endif
	{
		int sockaddrl = (int)sizeof(struct sockaddr);
		struct sockaddr_in incomer_address;
		int fd = accept(x->x_connectsocket, (struct sockaddr*)&incomer_address, &sockaddrl);
		if (fd < 0) 
		{
			post("netreceive~: accept failed");
			return;
		}
#ifdef O_NONBLOCK
		fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
		if (x->x_socket != -1)
		{
			post("netreceive~: new connection");
			netreceive_tilde_closesocket(x);
		}

		netreceive_tilde_reset(x, 0);
		x->x_socket = fd;
		x->x_nbytes = 0;
		x->x_hostname = gensym(inet_ntoa(incomer_address.sin_addr));
#ifdef PD
		sys_addpollfn(fd, netreceive_tilde_datapoll, x);
		outlet_float(x->x_outlet1, 1);
#else
		clock_delay(x->x_datapoll, 0);
		outlet_int(x->x_outlet1, 1);
#endif
	}
#ifndef PD
	clock_delay(x->x_connectpoll, DEFAULT_NETWORK_POLLTIME);
#endif
}


static int netreceive_tilde_createsocket(t_netreceive_tilde* x, int portno)
{
    struct sockaddr_in server;
    int sockfd;
    int tcp = x->x_tcp;

     /* create a socket */
    if (!tcp)
      sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    else
      sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        netreceive_tilde_sockerror("socket");
        return 0;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    /* assign server port number */

    server.sin_port = htons((u_short)portno);
    post("listening to port number %d", portno);

    netreceive_tilde_setsocketoptions(sockfd);

    /* name the socket */
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
         netreceive_tilde_sockerror("bind");
         CLOSESOCKET(sockfd);
         return 0;
    }


    if (!tcp)
	{
         x->x_socket = sockfd;
         x->x_nbytes = 0;
#ifdef PD
         sys_addpollfn(sockfd, netreceive_tilde_datapoll, x);
#else
		 clock_delay(x->x_datapoll, 0);
#endif
    }
    else
	{
         if (listen(sockfd, 5) < 0)
		 {
              netreceive_tilde_sockerror("listen");
              CLOSESOCKET(sockfd);
			  return 0;
         }
         else
		 {
              x->x_connectsocket = sockfd;
			  /* start polling for connection requests */
#ifdef PD
              sys_addpollfn(sockfd, netreceive_tilde_connectpoll, x);
#else
			  clock_delay(x->x_connectpoll, 0);
#endif
         }
    }
    return 1;
}



/* kick connected client */
static void netreceive_tilde_kick(t_netreceive_tilde *x)
{
	if (x->x_tcp)
	{
		if (x->x_socket != -1)
		{
			shutdown(x->x_socket, 1);
			netreceive_tilde_closesocket(x);
			post("netreceive~: kicked client!");
		}
		else error("netreceive~: no client to kick");
	}
	else error("netreceive~: kicking clients in UDP mode not possible");
}


#define QUEUESIZE (int)((x->x_framein + DEFAULT_AUDIO_BUFFER_FRAMES - x->x_frameout) % DEFAULT_AUDIO_BUFFER_FRAMES)
#define BLOCKOFFSET (x->x_blockssincerecv * x->x_vecsize * x->x_frames[x->x_frameout].tag.channels)

static t_int *netreceive_tilde_perform(t_int *w)
{
	t_netreceive_tilde *x = (t_netreceive_tilde*) (w[1]);
	int n = (int)(w[2]);
	t_float *out[DEFAULT_AUDIO_CHANNELS];
	const int offset = 3;
	const int channels = x->x_frames[x->x_frameout].tag.channels;
	int i = 0;

	for (i = 0; i < x->x_noutlets; i++)
	{
		out[i] = (t_float *)(w[offset + i]);
	}

	if (n != x->x_vecsize)
	{
		x->x_vecsize = n;
		x->x_blocksperrecv = x->x_blocksize / x->x_vecsize;
		x->x_blockssincerecv = 0;
	}

	/* check whether there is enough data in buffer */
	if (x->x_counter < x->x_maxframes)
	{
		goto bail;
	}

	/* check for buffer underflow */
	if (x->x_framein == x->x_frameout)
	{
		x->x_underflow++;
		goto bail;
	}


	/* queue balancing */
	x->x_average[x->x_averagecur] = QUEUESIZE;
	if (++x->x_averagecur >= DEFAULT_AVERAGE_NUMBER)
		x->x_averagecur = 0;

	switch (x->x_frames[x->x_frameout].tag.format)
	{
		case SF_FLOAT:
		{
			t_float* buf = (t_float *)x->x_frames[x->x_frameout].data + BLOCKOFFSET;

			if (x->x_frames[x->x_frameout].tag.version == SF_BYTE_NATIVE)
			{
				while (n--)
				{
					for (i = 0; i < channels; i++)
					{     
						*out[i]++ = *buf++;
					}
					for (i = channels; i < x->x_noutlets; i++)
					{     
						*out[i]++ = 0.;
					}
				}
			}
			else	/* swap bytes */
			{
				while (n--)
				{
					for (i = 0; i < channels; i++)
					{     
						*out[i]++ = netsend_float(*buf++);
					}
					for (i = channels; i < x->x_noutlets; i++)
					{     
						*out[i]++ = 0.;
					}
				}
			}
			break;
		}
		case SF_16BIT:     
		{
			short* buf = (short *)x->x_frames[x->x_frameout].data + BLOCKOFFSET;

			if (x->x_frames[x->x_frameout].tag.version == SF_BYTE_NATIVE)
			{
				while (n--)
				{
					for (i = 0; i < channels; i++)
					{
						*out[i]++ = (t_float)(*buf++ * 3.051850e-05);
					}
					for (i = channels; i < x->x_noutlets; i++)
					{
						*out[i]++ = 0.;
					}
				}
			}
			else /* swap bytes */
			{
				while (n--)
				{
					for (i = 0; i < channels; i++)
					{
						*out[i]++ = (t_float)(netsend_short(*buf++) * 3.051850e-05);
					}
					for (i = channels; i < x->x_noutlets; i++)
					{
						*out[i]++ = 0.;
					}
				}
			}
			break;
		}
		case SF_8BIT:     
		{
			unsigned char* buf = (char *)x->x_frames[x->x_frameout].data + BLOCKOFFSET;

			while (n--)
			{
				for (i = 0; i < channels; i++)
				{ 
					*out[i]++ = (t_float)((0.0078125 * (*buf++)) - 1.0);
				}
				for (i = channels; i < x->x_noutlets; i++)
				{
					*out[i]++ = 0.;
				}
			}
			break;
		}
		case SF_MP3:     
		{
			post("netreceive~: mp3 format not supported");
			if (x->x_tcp)
				netreceive_tilde_kick(x);
			break;
		}
		case SF_AAC:     
		{
#ifdef USE_FAAC
			t_float* buf = (t_float *)x->x_frames[x->x_frameout].data + BLOCKOFFSET;

			while (n--)
			{
				for (i = 0; i < channels; i++)
				{     
					*out[i]++ = (t_float)(*buf++);
				}
				for (i = channels; i < x->x_noutlets; i++)
				{     
					*out[i]++ = 0.;
				}
			}
			break;
#else
			post("netreceive~: aac format not supported");
			if (x->x_tcp)
				netreceive_tilde_kick(x);
#endif
			break;
		}
		default:
			post("netreceive~: unknown format (%d)",x->x_frames[x->x_frameout].tag.format);
			if (x->x_tcp)
				netreceive_tilde_kick(x);
			break;
	}

	if (!(x->x_blockssincerecv < x->x_blocksperrecv - 1))
	{
		x->x_blockssincerecv = 0;
		x->x_frameout++;
		x->x_frameout %= DEFAULT_AUDIO_BUFFER_FRAMES;
	}
	else
	{
		x->x_blockssincerecv++;
	}

	return (w + offset + x->x_noutlets);

bail:
	/* set output to zero */
	while (n--)
	{
		for (i = 0; i < x->x_noutlets; i++)
		{
			*(out[i]++) = 0.;
		}
	}
	return (w + offset + x->x_noutlets);
}



static void netreceive_tilde_dsp(t_netreceive_tilde *x, t_signal **sp)
{
	int i;

	x->x_myvec[0] = (t_int*)x;
	x->x_myvec[1] = (t_int*)sp[0]->s_n;

	x->x_samplerate = (long)sp[0]->s_sr;

	if (DEFAULT_AUDIO_BUFFER_SIZE % sp[0]->s_n)
	{
		error("netsend~: signal vector size too large (needs to be even divisor of %d)", DEFAULT_AUDIO_BUFFER_SIZE);
	}
	else
	{
#ifdef PD
		for (i = 0; i < x->x_noutlets; i++)
		{
			x->x_myvec[2 + i] = (t_int*)sp[i + 1]->s_vec;
		}
		dsp_addv(netreceive_tilde_perform, x->x_noutlets + 2, (t_int*)x->x_myvec);
#else
		for (i = 0; i < x->x_noutlets; i++)
		{
			x->x_myvec[2 + i] = (t_int*)sp[i]->s_vec;
		}
		dsp_addv(netreceive_tilde_perform, x->x_noutlets + 2, (void **)x->x_myvec);
#endif	/* PD */
	}
}


/* send stream info when banged */
static void netreceive_tilde_bang(t_netreceive_tilde *x)
{
	t_atom list[2];
	t_symbol *sf_format;
	t_float bitrate;
	int i, avg = 0;
	for (i = 0; i < DEFAULT_AVERAGE_NUMBER; i++)
		avg += x->x_average[i];

	bitrate = (t_float)((SF_SIZEOF(x->x_frames[x->x_frameout].tag.format) * x->x_samplerate * 8 * x->x_frames[x->x_frameout].tag.channels) / 1000.);

	switch (x->x_frames[x->x_frameout].tag.format)
	{
		case SF_FLOAT:
		{
			sf_format = ps_sf_float;
			break;
		}
		case SF_16BIT:
		{
			sf_format = ps_sf_16bit;
			break;
		}
		case SF_8BIT:
		{
			sf_format = ps_sf_8bit;
			break;
		}
		case SF_MP3:
		{
			sf_format = ps_sf_mp3;
			break;
		}
		case SF_AAC:
		{
			sf_format = ps_sf_aac;
			break;
		}
		default:
		{
			sf_format = ps_sf_unknown;
			break;
		}
	}

#ifdef PD
	/* --- stream information (t_tag) --- */
	/* audio format */
	SETSYMBOL(list, (t_symbol *)sf_format);
	outlet_anything(x->x_outlet2, ps_format, 1, list);

	/* channels */
	SETFLOAT(list, (t_float)x->x_frames[x->x_frameout].tag.channels);
	outlet_anything(x->x_outlet2, ps_channels, 1, list);

	/* framesize */
	SETFLOAT(list, (t_float)x->x_frames[x->x_frameout].tag.framesize);
	outlet_anything(x->x_outlet2, ps_framesize, 1, list);

	/* bitrate */
	SETFLOAT(list, (t_float)bitrate);
	outlet_anything(x->x_outlet2, ps_bitrate, 1, list);

	/* --- internal info (buffer and network) --- */
	/* overflow */
	SETFLOAT(list, (t_float)x->x_overflow);
	outlet_anything(x->x_outlet2, ps_overflow, 1, list);

	/* underflow */
	SETFLOAT(list, (t_float)x->x_underflow);
	outlet_anything(x->x_outlet2, ps_underflow, 1, list);

	/* queuesize */
	SETFLOAT(list, (t_float)QUEUESIZE);
	outlet_anything(x->x_outlet2, ps_queuesize, 1, list);

	/* average queuesize */
	SETFLOAT(list, (t_float)((t_float)avg / (t_float)DEFAULT_AVERAGE_NUMBER));
	outlet_anything(x->x_outlet2, ps_average, 1, list);

	if (x->x_tcp)
	{
		/* IP address */
		SETSYMBOL(list, (t_symbol *)x->x_hostname);
		outlet_anything(x->x_outlet2, ps_hostname, 1, list);
	}
#else
	/* --- stream information (t_tag) --- */
	/* audio format */
	SETSYM(list, ps_format);
	SETSYM(list + 1, (t_symbol *)sf_format);
	outlet_list(x->x_outlet2, NULL, 2, list);

	/* channels */
	SETSYM(list, ps_channels);
	SETLONG(list + 1, (int)x->x_frames[x->x_frameout].tag.channels);
	outlet_list(x->x_outlet2, NULL, 2, list);

	/* framesize */
	SETSYM(list, ps_framesize);
	SETLONG(list + 1, (int)x->x_frames[x->x_frameout].tag.framesize);
	outlet_list(x->x_outlet2, NULL, 2, list);

	/* bitrate */
	SETSYM(list, ps_bitrate);
	SETFLOAT(list + 1, (t_float)bitrate);
	outlet_list(x->x_outlet2, NULL, 2, list);

	/* --- internal info (buffer and network) --- */
	/* overflow */
	SETSYM(list, ps_overflow);
	SETLONG(list + 1, (int)x->x_overflow);
	outlet_list(x->x_outlet2, NULL, 2, list);

	/* underflow */
	SETSYM(list, ps_underflow);
	SETLONG(list + 1, (int)x->x_underflow);
	outlet_list(x->x_outlet2, NULL, 2, list);

	/* queuesize */
	SETSYM(list, ps_queuesize);
	SETLONG(list + 1, (int)QUEUESIZE);
	outlet_list(x->x_outlet2, NULL, 2, list);

	/* average queuesize */
	SETSYM(list, ps_average);
	SETFLOAT(list + 1, (t_float)((t_float)avg / (t_float)DEFAULT_AVERAGE_NUMBER));
	outlet_list(x->x_outlet2, NULL, 2, list);

	if (x->x_tcp)
	{
		/* IP address */
		SETSYM(list, (t_symbol *)ps_hostname);
		SETSYM(list + 1, x->x_hostname);
		outlet_list(x->x_outlet2, NULL, 2, list);
	}
#endif
}



static void netreceive_tilde_print(t_netreceive_tilde* x)
{
	int i, avg = 0;
	for (i = 0; i < DEFAULT_AVERAGE_NUMBER; i++)
		avg += x->x_average[i];
	post("netreceive~: last size = %d, avg size = %g, %d underflows, %d overflows", QUEUESIZE, (float)((float)avg / (float)DEFAULT_AVERAGE_NUMBER), x->x_underflow, x->x_overflow);
	post("netreceive~: channels = %d, framesize = %d, packets = %d", x->x_frames[x->x_framein].tag.channels, x->x_frames[x->x_framein].tag.framesize, x->x_counter);
}



#ifdef PD
static void *netreceive_tilde_new(t_floatarg fportno, t_floatarg outlets, t_floatarg prot)
#else
static void *netreceive_tilde_new(long fportno, long outlets, long prot)
#endif
{
	t_netreceive_tilde *x;
	int i;

	if (fportno == 0) fportno = DEFAULT_PORT;

#ifdef PD
	x = (t_netreceive_tilde *)pd_new(netreceive_tilde_class);
    if (x)
    { 
        for (i = sizeof(t_object); i < (int)sizeof(t_netreceive_tilde); i++)  
                ((char *)x)[i] = 0; 
	}

	x->x_noutlets = CLIP((int)outlets, 1, DEFAULT_AUDIO_CHANNELS);
	for (i = 0; i < x->x_noutlets; i++)
		outlet_new(&x->x_obj, &s_signal);
	if (!prot)
		x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);	/* outlet for connection state (TCP/IP) */
	x->x_outlet2 = outlet_new(&x->x_obj, &s_anything);
#else
	x = (t_netreceive_tilde *)newobject(netreceive_tilde_class);
    if (x)
    { 
        for (i = sizeof(t_pxobject); i < (int)sizeof(t_netreceive_tilde); i++)  
                ((char *)x)[i] = 0; 
	}

	dsp_setup((t_pxobject *)x, 0);	/* no signal inlets */
	x->x_noutlets = CLIP((int)outlets, 1, DEFAULT_AUDIO_CHANNELS);
	x->x_outlet2 = listout(x);	/* outlet for info list */
	if (!prot)
		x->x_outlet1 = listout(x);	/* outlet for connection state (TCP/IP) */
	for (i = 0 ; i < x->x_noutlets; i++)
		outlet_new(x, "signal");
	x->x_connectpoll = clock_new(x, (method)netreceive_tilde_connectpoll);
	x->x_datapoll = clock_new(x, (method)netreceive_tilde_datapoll);
#endif

	x->x_myvec = (t_int **)t_getbytes(sizeof(t_int *) * (x->x_noutlets + 3));
	if (!x->x_myvec)
	{
		error("netreceive~: out of memory");
		return NULL;
	}

#ifdef USE_FAAC
	x->x_faac_decoder = NULL;
	x->x_faac_init = 0;
#endif

	x->x_connectsocket = -1;
	x->x_socket = -1;
	x->x_tcp = 1;
	x->x_nconnections = 0;
	x->x_ndrops = 0;
	x->x_underflow = 0;
	x->x_overflow = 0;
	x->x_hostname = ps_nothing;

	for (i = 0; i < DEFAULT_AUDIO_BUFFER_FRAMES; i++)
	{
		x->x_frames[i].data = (char *)t_getbytes(DEFAULT_AUDIO_BUFFER_SIZE * x->x_noutlets * sizeof(t_float));
	}
	x->x_framein = 0;
	x->x_frameout = 0;
	x->x_maxframes = DEFAULT_QUEUE_LENGTH;
	x->x_vecsize = 64;	/* we'll update this later */
	x->x_blocksize = DEFAULT_AUDIO_BUFFER_SIZE;	/* LATER make this dynamic */
	x->x_blockssincerecv = 0;
	x->x_blocksperrecv = x->x_blocksize / x->x_vecsize;

	if (prot)
		x->x_tcp = 0;

	if (!netreceive_tilde_createsocket(x, (int)fportno))
	{
		error("netreceive~: failed to create listening socket");
		return (NULL);
	}

	return (x);
}



static void netreceive_tilde_free(t_netreceive_tilde *x)
{
	int i;

	if (x->x_connectsocket != -1)
	{
#ifdef PD
		sys_rmpollfn(x->x_connectsocket);
#else
		clock_unset(x->x_connectpoll);
#endif
		CLOSESOCKET(x->x_connectsocket);
	}
	if (x->x_socket != -1)
	{
#ifdef PD
		sys_rmpollfn(x->x_socket);
#else
		clock_unset(x->x_datapoll);
#endif
		CLOSESOCKET(x->x_socket);
	}

#ifndef PD
	dsp_free((t_pxobject *)x);	/* free the object */
	clock_free(x->x_connectpoll);
	clock_free(x->x_datapoll);
#endif

#ifdef USE_FAAC
	netreceive_tilde_faac_close(x);
#endif

	/* free memory */
	t_freebytes(x->x_myvec, sizeof(t_int *) * (x->x_noutlets + 3));
	for (i = 0; i < DEFAULT_AUDIO_BUFFER_FRAMES; i++)
	{
		 t_freebytes(x->x_frames[i].data, DEFAULT_AUDIO_BUFFER_SIZE * x->x_noutlets * sizeof(t_float));
	}
}



#ifdef PD
void netreceive_tilde_setup(void)
{
	netreceive_tilde_class = class_new(gensym("netreceive~"), 
		(t_newmethod) netreceive_tilde_new, (t_method) netreceive_tilde_free,
		sizeof(t_netreceive_tilde),  0, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);

	class_addmethod(netreceive_tilde_class, nullfn, gensym("signal"), 0);
	class_addbang(netreceive_tilde_class, (t_method)netreceive_tilde_bang);
	class_addmethod(netreceive_tilde_class, (t_method)netreceive_tilde_dsp, gensym("dsp"), A_CANT, 0);
	class_addmethod(netreceive_tilde_class, (t_method)netreceive_tilde_print, gensym("print"), 0);
	class_addmethod(netreceive_tilde_class, (t_method)netreceive_tilde_kick, gensym("kick"), 0);
	class_addmethod(netreceive_tilde_class, (t_method)netreceive_tilde_reset, gensym("reset"), A_DEFFLOAT, 0);
	class_addmethod(netreceive_tilde_class, (t_method)netreceive_tilde_reset, gensym("buffer"), A_DEFFLOAT, 0);
	class_sethelpsymbol(netreceive_tilde_class, gensym("netsend~"));
	post("netreceive~ v%s, (c) 2004 Olaf Matthes", VERSION);

	ps_format = gensym("format");
	ps_channels = gensym("channels");
	ps_framesize = gensym("framesize");
	ps_bitrate = gensym("bitrate");
	ps_overflow = gensym("overflow");
	ps_underflow = gensym("underflow");
	ps_queuesize = gensym("queuesize");
	ps_average = gensym("average");
	ps_hostname = gensym("ipaddr");
	ps_sf_float = gensym("_float_");
	ps_sf_16bit = gensym("_16bit_");
	ps_sf_8bit = gensym("_8bit_");
	ps_sf_mp3 = gensym("_mp3_");
	ps_sf_aac = gensym("_aac_");
	ps_sf_unknown = gensym("_unknown_");
	ps_nothing = gensym("");
}

#else

void netreceive_tilde_assist(t_netreceive_tilde *x, void *b, long m, long a, char *s)
{
	switch (m)
	{
		case 1: /* inlet */
			sprintf(s, "(Anything) Control Messages");
			break;
		case 2: /* outlets */
			sprintf(s, "(Signal) Audio Channel %d", (int)(a + 1));
			break;
		break;
	}

}

void main()
{
#ifdef _WINDOWS
    short version = MAKEWORD(2, 0);
    WSADATA nobby;
#endif	/* _WINDOWS */

	setup((t_messlist **)&netreceive_tilde_class, (method)netreceive_tilde_new, (method)netreceive_tilde_free, 
		  (short)sizeof(t_netreceive_tilde), 0L, A_DEFLONG, A_DEFLONG, A_DEFLONG, 0);
	addmess((method)netreceive_tilde_dsp, "dsp", A_CANT, 0);
	addmess((method)netreceive_tilde_assist, "assist", A_CANT, 0);
	addmess((method)netreceive_tilde_print, "print", 0);
	addmess((method)netreceive_tilde_kick, "kick", 0);
	addmess((method)netreceive_tilde_reset, "reset", A_DEFFLOAT, 0);
	addmess((method)netreceive_tilde_reset, "buffer", A_DEFFLOAT, 0);
	addbang((method)netreceive_tilde_bang);
	dsp_initclass();
	finder_addclass("System", "netreceive~");
	post("netreceive~ v%s, © 2004 Olaf Matthes", VERSION);

	ps_format = gensym("format");
	ps_channels = gensym("channels");
	ps_framesize = gensym("framesize");
	ps_bitrate = gensym("bitrate");
	ps_overflow = gensym("overflow");
	ps_underflow = gensym("underflow");
	ps_queuesize = gensym("queuesize");
	ps_average = gensym("average");
	ps_hostname = gensym("ipaddr");
	ps_sf_float = gensym("_float_");
	ps_sf_16bit = gensym("_16bit_");
	ps_sf_8bit = gensym("_8bit_");
	ps_sf_mp3 = gensym("_mp3_");
	ps_sf_aac = gensym("_aac_");
	ps_sf_unknown = gensym("_unknown_");
	ps_nothing = gensym("");

#ifdef _WINDOWS
    if (WSAStartup(version, &nobby)) error("netreceive~: WSAstartup failed");
#endif	/* _WINDOWS */
}
#endif	/* PD */
