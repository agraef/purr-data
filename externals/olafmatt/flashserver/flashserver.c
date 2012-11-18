/* --------------------------  flashserver  ----------------------------------- */
/*                                                                              */
/* A server for bidirectional communication between Flash and Pd.               */
/* Use XMLsocket() functions in Macromedia Flash 5.0 to connect to it.          */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>                                */
/* Get source at http://www.akustische-kunst.org/puredata/flash/                */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
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
/* Inspired by work done by Eric Skogen (http://www.cyburrs.com/eskogen/).      */
/*                                                                              */
/* ---------------------------------------------------------------------------- */

#include "m_pd.h"
#include "s_stuff.h"        /* for socket / networking stuff borought from Pd */
#include "m_imp.h"          /* we need more than just m_pd.h */

#include <sys/types.h>
#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>        /* needs pthread library on win! */
#ifdef UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sched.h>
#include <sys/mman.h>
#define SOCKET_ERROR -1
#else
#include <io.h>
#include <fcntl.h>
#include <winsock.h>
#endif

#define MAX_CONNECT  256	/* maximum number of connected Flash clients */
#define INBUFSIZE    4096   /* size of receiving data buffer */
#define DEFSTRSIZE   256    /* default size for strings */

static char *version = "flashserver v0.2h :: bidirectional communication between Flash and Pd\n"
                       "                     written by Olaf Matthes <olaf.matthes@gmx.de>";

/* ----------------------------- flashserver ------------------------- */

static t_class *flashserver_class;
static t_binbuf *b;                 /* a binbuf used to parse output data */
static char databuf[INBUFSIZE];     /* the string we received from Flash client */

typedef void (*t_flashserver_socketnotifier)(void *x);
typedef void (*t_flashserver_socketreceivefn)(void *x, char *buf);

typedef struct _flashserver
{
    t_object x_obj;
    t_outlet *x_msgout;				/* output for the received message */
    t_outlet *x_connectout;         /* output for number of connections (0 - MAX_CONNECT) */
	t_outlet *x_clientno;           /* output for client number (0 - MAX_CONNECT) */
	t_outlet *x_clientsock;         /* output for socket number */
	t_outlet *x_connectionip;       /* outlet for the IP we just got data from */
	t_canvas *x_canvas;             /* needed to get current directory */
	t_symbol *x_host[MAX_CONNECT];  /* list of all hostnames */
	t_int    x_fd[MAX_CONNECT];     /* list of all socket numbers */
	t_int    x_client[MAX_CONNECT]; /* the client number assigned to each client */
	t_int    x_clientlist[MAX_CONNECT];	    /* list of client numbers already in use */
	t_int    x_sock_fd;             /* the socket number we just got data from */
    t_int    x_connectsocket;       /* number of socket we are listening on */
    t_int    x_nconnections;        /* total number of open connections */
	t_int    x_maxconnect;          /* user setable maximum number of connections */
	t_int    x_prepend;             /* prepend client number to output data */
	t_int    x_verbose;
} t_flashserver;

typedef struct _flashserver_socketreceiver
{
    char *sr_inbuf;                 /* data we got from clients, a plain character string */
    int sr_inhead;                  /* writing position in above receive buffer */
    int sr_intail;                  /* reading position in above receive buffer */
    void *sr_owner;                 /* we'll put the correcponding t_flashserver in here */
    t_flashserver_socketnotifier sr_notifier;
    t_flashserver_socketreceivefn sr_socketreceivefn;
} t_flashserver_socketreceiver;

	/* some prototypes (as needed) */
static void flashserver_notify(t_flashserver *x);	/* notify us in case a client disconnected */
static void flashserver_client_remove(t_flashserver *x, int sockfd);/* remove client from list */
static int flashserver_get_client(t_flashserver *x, int sockno);	/* find client number */
static int flashserver_get_socket(t_flashserver *x, int client);	/* find socket */
static int flashserver_get_entry(t_flashserver *x, int sockno);		/* find number of entry */

/* ----------- socket utility functions ----------- */
	/* set SO_KEEPALIVE option for socket */
static int flashserver_socket_set_keepalive(int sockfd)
{
    int keepalive = 1;
    return setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(int));
}

	/* set SO_SNDBUF to 0 -> disable send buffer */
static int flashserver_socket_disable_buffer(int sockfd)
{
    int zero = 0;	/* the buffer size */
    return setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&zero, sizeof(zero));
}

/* ----------- socket receiver funtions ----------- */
static t_flashserver_socketreceiver *flashserver_socketreceiver_new(void *owner, t_flashserver_socketnotifier notifier,
    t_flashserver_socketreceivefn socketreceivefn)
{
    t_flashserver_socketreceiver *x = (t_flashserver_socketreceiver *)getbytes(sizeof(*x));
    x->sr_inhead = x->sr_intail = 0;
    x->sr_owner = owner;
    x->sr_notifier = notifier;
    x->sr_socketreceivefn = socketreceivefn;
    if (!(x->sr_inbuf = malloc(INBUFSIZE))) bug("t_flashserver_socketreceiver");
    return (x);
}

    /* this is in a separately called subroutine so that the buffer isn't
    sitting on the stack while the messages are getting passed. */
static int flashserver_socketreceiver_doread(t_flashserver_socketreceiver *x)
{
    char messbuf[INBUFSIZE], *bp = messbuf;
    int indx;
    int inhead = x->sr_inhead;
    int intail = x->sr_intail;
    char *inbuf = x->sr_inbuf;
    if (intail == inhead) return (0);
    for (indx = intail; indx != inhead; indx = (indx+1)&(INBUFSIZE-1))
    {
    	char c = *bp++ = inbuf[indx];
			/* we split the buffer at terminating null character */
    	if (((c == '\0' || c == '\n') && (!indx || inbuf[indx-1] != '\\')))
    	{
    	    intail = (indx+1)&(INBUFSIZE-1);
				/* copy data into buffer we pass on to flashserver_doit */
			strncpy(databuf, messbuf, bp - messbuf);
    	    x->sr_inhead = inhead;
    	    x->sr_intail = intail;
    	    return (1);
    	}
    }
    return (0);
}

static void flashserver_socketreceiver_read(t_flashserver_socketreceiver *x, int fd)
{
	char *semi;
	int readto = (x->sr_inhead >= x->sr_intail ? INBUFSIZE : x->sr_intail-1);
	int ret;

	t_flashserver *y = x->sr_owner;

	y->x_sock_fd = fd;

    		/* the input buffer might be full.  If so, drop the whole thing */
	if (readto == x->sr_inhead)
	{
    		post("flashserver: dropped message");
    		x->sr_inhead = x->sr_intail = 0;
    		readto = INBUFSIZE;
	}
	else
	{
		ret = recv(fd, x->sr_inbuf + x->sr_inhead,
	    	readto - x->sr_inhead, 0);
		if (ret < 0)
		{
			sys_sockerror("recv");
                    if (x->sr_notifier) (*x->sr_notifier)(x->sr_owner);
                    sys_rmpollfn(fd);
                    sys_closesocket(fd);
		}
		else if (ret == 0)
		{
	    	post("flashserver: connection closed on socket %d", fd);
			if (x->sr_notifier) (*x->sr_notifier)(x->sr_owner);
	    	sys_rmpollfn(fd);
	    	sys_closesocket(fd);
		}
		else
		{
    		x->sr_inhead += ret;
    		if (x->sr_inhead >= INBUFSIZE) x->sr_inhead = 0;
    		while (flashserver_socketreceiver_doread(x))
			{
				outlet_setstacklim();
				if (x->sr_socketreceivefn)
		    		(*x->sr_socketreceivefn)(x->sr_owner, databuf);
    			// else binbuf_eval(inbinbuf, 0, 0, 0);
 	    	}
		}
	}
}

static void flashserver_socketreceiver_free(t_flashserver_socketreceiver *x)
{
    free(x->sr_inbuf);
    freebytes(x, sizeof(*x));
}

/* ------------------ main flashserver send stuff ----------------------- */

/* flashserver_send_doit() - send data to client, might return an error in
   case client disconnected
   LATER put this into a seperate thread because send might block, e.g.
   in case a client disconnects during send or crashes...                 */

static int flashserver_send_doit(t_flashserver *x, int sockfd, char *buf, int length, int client)
{
	int sent;
	for (sent = 0; sent < length;)
	{
		static double lastwarntime;
		static double pleasewarn;
		double timebefore = clock_getlogicaltime();
#ifndef __linux__
    	int res = send(sockfd, buf+sent, length-sent, 0);
#else
		int res = send(sockfd, buf+sent, length-sent, MSG_DONTWAIT|MSG_NOSIGNAL);
#endif
    	double timeafter = clock_getlogicaltime();
    	int late = (timeafter - timebefore > 0.005);
    	if (late || pleasewarn)
    	{
    	    if (timeafter > lastwarntime + 2)
    	    {
    	    	 post("flashserver blocked %d msec",
    	    	    (int)(1000 * ((timeafter - timebefore) + pleasewarn)));
    	    	 pleasewarn = 0;
    	    	 lastwarntime = timeafter;
    	    }
    	    else if (late) pleasewarn += timeafter - timebefore;
    	}
    	if (res <= 0)
    	{
    		sys_sockerror("flashserver");
			post("flashserver: could not send data to cient #%d, disconnecting", client+1);
			flashserver_client_remove(x, sockfd);
			sys_rmpollfn(sockfd);
			sys_closesocket(sockfd);
			return -1;
		}
    	else
    	{
    		sent += res;
    	}
	}
	return sent;
}

	/* send XML message consisting of a symbol and a float to client using socket 
	   number, messages are terminated with a null character */
static void flashserver_sendXML(t_flashserver *x, t_symbol *s, int argc, t_atom *argv)
{
	int sockfd, client = -1, i;
	char XMLmsg[DEFSTRSIZE];
	if(x->x_nconnections < 0)
	{
		post("flashserver: no Flash clients connected");
		return;
	}
	if(argc < 2)
	{
		post("flashserver: nothing to send");
		return;
	}
		/* get socket number of connection (first element in list) */
	if(argv[0].a_type == A_FLOAT)
	{
		sockfd = atom_getfloatarg(0, argc, argv);
		client = flashserver_get_client(x, sockfd);
		if(client == -1)	/* no Flash client found ? */
		{
			post("flashserver: no Flash client on socket %d", sockfd);
			return;
		}
	}
	else
	{
		post("flashserver: no socket specified");
		return;
	}
		/* process & send data */
	if(sockfd > 0)
	{
		t_symbol *XMLsym;
		t_float XMLvalue;
		int length;
			/* get symbol and value from input */
		if(argv[1].a_type == A_SYMBOL)
		{
			XMLsym = atom_getsymbolarg(1, argc, argv);
			if(argv[2].a_type == A_FLOAT)
			{
				XMLvalue = atom_getfloatarg(2, argc, argv);
			}
			else 
			{
				post("flashserver: no value specified");
				return;
			}
		}
		else
		{
			post("flashserver: no symbol specified");
			return;
		}
			/* build up XML message */
		sprintf(XMLmsg, "<data symbol=\"%s\" value=\"%f\" />", XMLsym->s_name, XMLvalue);
	
		if(x->x_verbose)post("flashserver: sending data to client #%d on socket %d", client + 1, sockfd);

		length = strlen(XMLmsg) + 1;	/* lenght of string including terminating null character ! */

		if(x->x_verbose)post("flashserver: sending XML: \n%s", XMLmsg);
		flashserver_send_doit(x, sockfd, XMLmsg, length, client);

	}
	else post("flashserver: not a valid socket number (%d)", sockfd);
}

	/* send message to client using socket number, messages are terminated 
	   with a null character, not with A_SEMMI as it would be needed for Pd ! */
static void flashserver_send(t_flashserver *x, t_symbol *s, int argc, t_atom *argv)
{
	int sockfd, client = -1, i;
	if(x->x_nconnections < 0)
	{
		post("flashserver: no Flash clients connected");
		return;
	}
	if(argc < 2)
	{
		post("flashserver: nothing to send");
		return;
	}
		/* get socket number of connection (first element in list) */
	if(argv[0].a_type == A_FLOAT)
	{
		sockfd = atom_getfloatarg(0, argc, argv);
		for(i = 0; i < x->x_nconnections; i++)	/* check if connection exists */
		{
			if(x->x_fd[i] == sockfd)
			{
				client = x->x_client[i];	/* the client we're sending to */
				break;
			}
		}
		if(client == -1)	/* still no Flash client found ? */
		{
			post("flashserver: no client on socket %d", sockfd);
			return;
		}
	}
	else
	{
		post("flashserver: no socket specified");
		return;
	}
		/* process & send data */
	if(sockfd > 0)
	{
		t_binbuf *bb = binbuf_new();
		char *buf;
		int length, sent;
	
		if(x->x_verbose)post("flashserver: sending data to client #%d on socket %d", client+1, sockfd);

		binbuf_add(bb, argc - 1, argv + 1);	/* skip first element which is the socket no. */
		binbuf_gettext(bb, &buf, &length);  /* convert binbuf into string */
		buf[length] = '\0';                 /* append null termination '\0' */
		strcat(buf, ";");                   /* add A_SEMI at end of binbuf */
		length = strlen(buf) + 1;           /* we send one character more because we must 
											   include the '\0' for Flash ! */

		if(x->x_verbose)post("flashserver: sending \"%s\"", buf);
		// flashserver_check_socket(x, sockfd);
		flashserver_send_doit(x, sockfd, buf, length, client);
		t_freebytes(buf, length);
		binbuf_free(bb);
	}
	else post("flashserver: not a valid socket number (%d)", sockfd);
}

	/* send XML message to client using client number */
static void flashserver_client_sendXML(t_flashserver *x, t_symbol *s, int argc, t_atom *argv)
{
	int sock = -1, client, i;
	if(x->x_nconnections < 0)
	{
		post("flashserver: no clients connected");
		return;
	}
	if(argc < 2)
	{
		post("flashserver: nothing to send");
		return;
	}
		/* get number of client (first element in list) */
	if(argv[0].a_type == A_FLOAT)
		client = atom_getfloatarg(0, argc, argv);
	else
	{
		post("flashserver: no client specified");
		return;
	}
	if((sock = flashserver_get_socket(x, client-1)) < 0)
	{
		post("flashserver: client #%d does not exist", client);
		return;
	}
	argv[0].a_w.w_float = sock;		/* replace client # with socket number */
	flashserver_sendXML(x, NULL, argc, argv);	/* call send routine with socket number */
}

	/* send message to client using client number */
static void flashserver_client_send(t_flashserver *x, t_symbol *s, int argc, t_atom *argv)
{
	int sock = -1, client, i;
	if(x->x_nconnections < 0)
	{
		post("flashserver: no clients connected");
		return;
	}
	if(argc < 2)
	{
		post("flashserver: nothing to send");
		return;
	}
		/* get number of client (first element in list) */
	if(argv[0].a_type == A_FLOAT)
		client = atom_getfloatarg(0, argc, argv);
	else
	{
		post("flashserver: no client specified");
		return;
	}
	if((sock = flashserver_get_socket(x, client-1)) < 0)
	{
		post("flashserver: client #%d does not exist", client);
		return;
	}
	argv[0].a_w.w_float = sock;		/* replace client # with socket number */
	flashserver_send(x, NULL, argc, argv);	/* call send routine with socket number */
}

	/* broadcasts a XML message to all connected Flash clients */
static void flashserver_broadcastXML(t_flashserver *x, t_symbol *s, int argc, t_atom *argv)
{
	int i, c = x->x_nconnections;
	t_atom at[MAX_CONNECT];

	if(x->x_nconnections == 0) return;	/* check if there are any clients connected */

	for(i = 0; i < argc; i++)
	{
		at[i + 1] = argv[i];
	}
	argc++;
		/* enumerate through the clients and send each socket the message */
    while(c--)
	{
		SETFLOAT(at, x->x_fd[c]);	/* prepend number of client */
		flashserver_sendXML(x, s, argc, at);
    }
}

	/* broadcasts a message to all connected Flash clients */
static void flashserver_broadcast(t_flashserver *x, t_symbol *s, int argc, t_atom *argv)
{
	int i, c = x->x_nconnections;
	t_atom at[MAX_CONNECT];

	if(x->x_nconnections == 0) return;	/* check if there are any clients connected */

	for(i = 0; i < argc; i++)
	{
		at[i + 1] = argv[i];
	}
	argc++;
		/* enumerate through the clients and send each socket the message */
    while(c--)
	{
		SETFLOAT(at, x->x_fd[c]);	/* prepend number of client */
		flashserver_send(x, s, argc, at);
    }
}

/* ---------------- main flashserver receive stuff --------------------- */
static void flashserver_client_remove(t_flashserver *x, int sockfd)
{
	int i, k;
		/* remove connection from list */
	for(i = 0; i < x->x_nconnections; i++)
	{
			if(x->x_fd[i] == sockfd)
			{
				x->x_nconnections--;
				post("flashserver: client #%d (\"%s\") removed from list of clients", x->x_client[i] + 1, x->x_host[i]->s_name);
				x->x_host[i] = NULL;	/* delete entry */
				x->x_fd[i] = -1;
				x->x_clientlist[x->x_client[i]] = -1;	/* free client number in list */
					/* rearrange list now: move entries to close the gap */
				for(k = i; k < x->x_nconnections; k++)
				{
					x->x_host[k] = x->x_host[k + 1];
					x->x_fd[k] = x->x_fd[k + 1];
					x->x_client[k] = x->x_client[k + 1];
				}
			}
	}
    outlet_float(x->x_connectout, x->x_nconnections);
}
	/* gets called whenever a client disconnects and removes this 
	   client from the list of clients and reports new number of clients */
static void flashserver_notify(t_flashserver *x)
{
	flashserver_client_remove(x, x->x_sock_fd);
}

	/* find the entry number for a given socket */
static int flashserver_get_entry(t_flashserver *x, int sockno)
{
	int i;
		/* look at all clients... */
	for(i = 0; i < x->x_nconnections; i++)
	{
			if(x->x_fd[i] == sockno)
			{
				return i;
			}
	}
	return -1;	/* return an error in case there is no match */
}

	/* find the client for a given socket number */
static int flashserver_get_client(t_flashserver *x, int sockno)
{
	int i;
		/* look at all clients... */
	for(i = 0; i < x->x_nconnections; i++)	/* search for specified socket number */
	{
		if(x->x_fd[i] == sockno)
		{
			return i;	/* return the result */
		}
	}
	return -1;	/* return an error in case there is no match */
}

	/* find the socket for a given client */
static int flashserver_get_socket(t_flashserver *x, int client)
{
	int i;
		/* look at all entries... */
	for(i = 0; i < x->x_nconnections; i++)	/* search for specified client number */
	{
		if(x->x_client[i] == client)
		{
			return x->x_fd[i];	/* return the socket */
		}
	}
	return -1;	/* return an error in case there is no match */
}

	/* kick client number f */
static void flashserver_client_kick(t_flashserver *x, t_floatarg f)
{
	int client = (int)f;
	int sockfd = flashserver_get_socket(x, client-1);	/* find socket */
	int i  = flashserver_get_entry(x, sockfd);			/* find the entry */
	if(sockfd == -1)
	{
		post("flashserver: client #%d does not exist", client);
		return;
	}
	post("flashserver: kicking client #%d (\"%s\") on socket %d", client, x->x_host[i]->s_name, sockfd);
	flashserver_client_remove(x, sockfd);
	sys_rmpollfn(sockfd);
	sys_closesocket(sockfd);
}

	/* send out socket number and client allocation number */
static void flashserver_identify(t_flashserver *x)
{
	int i = flashserver_get_client(x, x->x_sock_fd);
	if(i == -1)
	{
		post("flashserver: internal error");
		return;
	}
	outlet_symbol(x->x_connectionip, x->x_host[i]);		/* client's IP address */
	outlet_float(x->x_clientsock, x->x_sock_fd);		/* the socket number */
	outlet_float(x->x_clientno, x->x_client[i] + 1);	/* the client's allocation number */
}

static void flashserver_doit(void *z, char *buf)
{
    t_atom messbuf[1024];
    t_flashserver *x = (t_flashserver *)z;
    int msg, natom;
    t_atom *at;
	int i, length;

		/* check for valid data in this string */
	if(strstr(buf, "GET / HTTP/1.0"))	/* oops, got a HTTP request */
	{
		post("flashserver: HTTP requests not supported");
		return;
	}
	else if(strstr(buf, "<data"))		/* got XML style message */
	{
		t_symbol *XMLsym;
		t_float XMLvalue;
		char *s;
		char *t;
		t_atom list[3];		/* a list containing the symbol and value we got */
		t_int stringlength;

		flashserver_identify(x);	/* output client info */
			/* parse the XML string:
			   this is a very simple and silly XML parser for XML messages of
			   the following style: <data symbol="<symbol>" value="<float>" /> */
		if(t = strstr(buf, "symbol=\""))
		{
			stringlength = strcspn(t+8, "\"");
			s = getbytes(stringlength + 1);
			strncpy(s, t+8, stringlength);	/* copy up to next 'inverted commas' */
			XMLsym = gensym(s);
			if(t = strstr(t+8, "value=\""))
			{
				XMLvalue = atof(t+7);
					/* prepare list for output */
				if(x->x_prepend)SETFLOAT(list, x->x_client[flashserver_get_client(x, x->x_sock_fd)] + 1);
				SETSYMBOL(list+x->x_prepend, XMLsym);
				SETFLOAT(list+x->x_prepend+1, XMLvalue);
				outlet_list(x->x_msgout, 0, 2+x->x_prepend, list); 
			}
			else post("flashserver: got corrupted XML data");
			freebytes(s, stringlength + 1);
		}
		else post("flashserver: got corrupted XML data");
	}
	else if(strstr(buf, ";\0"))	    /* (probably) got Pd conform data */
	{                               /* with semi and null termination at the end */
				/* prepend client number if user chose to do so */
		if(x->x_prepend)
		{
			char buf2[DEFSTRSIZE];
			strcpy(buf2, buf);
			sprintf(buf, "%d ", x->x_client[flashserver_get_client(x, x->x_sock_fd)] + 1);
			strcat(buf, buf2);
		}
			/* convert the string buffer we received into a binbuf */
		binbuf_text(b, buf, strlen(buf));
			/* get content out of binbuf */
		natom = binbuf_getnatom(b);
		at = binbuf_getvec(b);
			/* check again for valid data */
		if(natom < 2)	/* need at least 2 atoms: something (float, symbol...) and the semi */
			return;

		flashserver_identify(x);	/* output client info */

			/* process data */
		for (msg = 0; msg < natom;)
		{
    		int emsg;
			for (emsg = msg; emsg < natom && at[emsg].a_type != A_COMMA
				&& at[emsg].a_type != A_SEMI; emsg++);

			if (emsg > msg)
			{
				int ii;
				for (ii = msg; ii < emsg; ii++)
	    			if (at[ii].a_type == A_DOLLAR || at[ii].a_type == A_DOLLSYM)
					{
	    				pd_error(x, "flashserver: got dollar sign in message");
						goto nodice;
					}
				if (at[msg].a_type == A_FLOAT)
				{
	    			if (emsg > msg + 1)
						outlet_list(x->x_msgout, 0, emsg-msg, at + msg);
					else outlet_float(x->x_msgout, at[msg].a_w.w_float);
				}
				else if (at[msg].a_type == A_SYMBOL)
	    			outlet_anything(x->x_msgout, at[msg].a_w.w_symbol,
					emsg-msg-1, at + msg + 1);
			}
			nodice:
    			msg = emsg + 1;
		}
	}	/* endif Pd conform data */
}

static void flashserver_connectpoll(t_flashserver *x)
{
    struct sockaddr_in incomer_address;
    int sockaddrl = (int) sizeof( struct sockaddr );
    int fd = accept(x->x_connectsocket, (struct sockaddr*)&incomer_address, &sockaddrl);
	int i;
    if(fd < 0)										/* f...ed up socket number */
	{
		 post("flashserver: accept failed");
		 return;
	}
	else if(x->x_nconnections == x->x_maxconnect)	/* too many clients connected */
	{
		post("flashserver: refused connection from %s on socket %d, too many clients", 
			inet_ntoa(incomer_address.sin_addr), fd);
		sys_closesocket(fd);
		return;
	}
    else											/* everything looks fine, go on... */
    {
    	t_flashserver_socketreceiver *y = flashserver_socketreceiver_new((void *)x, 
    	    (t_flashserver_socketnotifier)flashserver_notify,
	    	(x->x_msgout ? flashserver_doit : 0));
		if(flashserver_socket_set_keepalive(fd))		/* set socket to SO_KEEPALIVE */
			error("flashserver: flashserver_socket_set_keepalive() failed");
		// if(flashserver_socket_disable_buffer(fd))		/* disable send buffer */
		//	error("flashserver: flashserver_socket_disable_buffer() failed");
    	sys_addpollfn(fd, (t_fdpollfn)flashserver_socketreceiver_read, y);	/* add poll-function */
		x->x_nconnections++;
		x->x_host[x->x_nconnections - 1] = gensym(inet_ntoa(incomer_address.sin_addr));
		x->x_fd[x->x_nconnections - 1] = fd;
			/* search for free client allocation number */
		for(i = 0; i < x->x_maxconnect; i++)
		{
			if(x->x_clientlist[i] == -1)	/* this is the first unused number ! */
			{
				x->x_client[x->x_nconnections - 1] = i;
				x->x_clientlist[i] = 1;		/* mark number as 'used' */
				break;
			}
		}

		post("flashserver: accepted connection from %s on socket %d", 
			x->x_host[x->x_nconnections - 1]->s_name, x->x_fd[x->x_nconnections - 1]);
    	outlet_float(x->x_connectout, x->x_nconnections);
    }
}

	/* open the Flash projektor file, code in parts stolen from Joseph A. Sarlo's GriPD */
static void flashserver_open(t_flashserver *x, t_symbol *sym, int argc, t_atom *argv)
{
    char flashExec[DEFSTRSIZE], filename[DEFSTRSIZE], dirbuf[DEFSTRSIZE], *nameptr;
#ifdef NT
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    int i;
#else	/* not NT */
    int i, pid;
#endif
		/* get the filename from message box */
    if (argc > 0)
    {		
        strcpy(filename, argv[0].a_w.w_symbol->s_name);
    }
    else
	{
		post("flashserver: no filename specified");
		return;
	}
		/* now search for the correct path */
	i = open_via_path(canvas_getdir(x->x_canvas)->s_name, filename, "", dirbuf, &nameptr, DEFSTRSIZE, 0);
	if(i != -1)
	{
		close(i);	/* open_via_path() has opened the file, we don't want that, so we close it */
		strcpy(flashExec, filename);
		if(!strstr(filename, "/") && !strstr(filename, "\\"))	/* refabricate the pathname if needed */
			sprintf(filename, "%s/%s", dirbuf, flashExec);
		sys_bashfilename(filename, filename);		/* convert filename into system conform spelling */
		post("flashserver: going to open \"%s\"", filename);
	}
	else
	{
		post("flashserver: \"%s\" not found", filename);
		return;
	}

#ifdef NT
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

	if(strstr(filename, ".swf"))	/* is it a .swf file ? */
		sprintf(flashExec, "\"FlashPla.exe %s\"", filename);	/* use FlashPlayer -> does not work */
	else
		sprintf(flashExec, "\"%s\"", filename);	/* add quotation marks */
    if(!CreateProcess(NULL, flashExec, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) != 0)
	{
        post("flashserver: failed to execute %s", flashExec);
    }
#else
    pid = fork();
    if (pid == 0)
    {
        struct sched_param par;
        int p1;

        /* Lose setuid priveliges */
        seteuid(getuid());
        /* set lowest priority, SCHED_OTHER policy, unlock mem*/
#if (_POSIX_PRIORITY_SCHEDULING - 0) >=  200112L
        p1 = sched_get_priority_min(SCHED_OTHER);
        par.sched_priority = p1;
        if (sched_setscheduler(0,SCHED_OTHER, &par) == -1)
            post("flashserver: unable to set priority %d scheduling.", p1);
#endif
#ifdef _POSIX_MEMLOCK
        if((munlockall() == -1) && (!getuid()))
    	    post("flashserver: unable to unlock memory.");
#endif
        if((execlp(flashExec, flashExec, filename, 0, 0, (char *)0)) == -1)
        {
            post("flashserver: error launching \"%s\"", flashExec);
            exit(1);
        }
    }
#endif
}

	/* set prepend mode (prepend client allocation number to data) */
static void flashserver_prepend(t_flashserver *x, t_floatarg f)
{
	if(f)
	{
		if(!x->x_prepend)post("flashserver~: switching to prepend mode");
		x->x_prepend = 1;
	}
	else
	{
		if(x->x_prepend)post("flashserver~: turning off prepend mode");
		x->x_prepend = 0;
	}
}

static void flashserver_print(t_flashserver *x)
{
	int i;
	if(x->x_nconnections > 0)
	{
		post("flashserver: %d open connections:", x->x_nconnections);

		for(i = 0; i < x->x_nconnections; i++)
		{
			post("             client #%d: \"%s\" on socket %d", 
				x->x_client[i] + 1, x->x_host[i]->s_name, x->x_fd[i]);
		}
	} else post("flashserver: no open connections");
}

static void *flashserver_new(t_floatarg fportno, t_floatarg fclients, t_floatarg fprep, t_floatarg fverbose)
{
    t_flashserver *x;
	int i, keepalive = 1;
    struct sockaddr_in server;
    int sockfd, portno = (int)fportno;
#ifdef UNIX_SIGPIPE
	struct sigaction sa;
		/* add our own sigpipe handler to avoid calling Pd's handler */
	sa.sa_handler = SIG_IGN;
  /*  sa.sa_mask = 0;
  sa.sa_flags = 0;
  */
	sigaction(SIGPIPE, &sa, (struct sigaction *)NULL);
#endif
    	/* create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#if 0
    post("flashserver: receive socket %d", sockfd);
#endif
    if (sockfd < 0)
    {
    	sys_sockerror("socket");
    	return (0);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

#ifdef IRIX
    	/* this seems to work only in IRIX but is unnecessary in
	Linux.  Not sure what NT needs in place of this. */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 0, 0) < 0)
    	post("setsockopt failed\n");
#endif
    	/* assign server port number */
    server.sin_port = htons((u_short)portno);

    	/* name the socket */
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
    	sys_sockerror("bind");
    	sys_closesocket(sockfd);
    	return (0);
    }
    x = (t_flashserver *)pd_new(flashserver_class);
    x->x_msgout = outlet_new(&x->x_obj, &s_anything);

		/* streaming protocol */
	if (listen(sockfd, 5) < 0)
	{
    	sys_sockerror("listen");
    	sys_closesocket(sockfd);
		sockfd = -1;
	}
    else
	{
		sys_addpollfn(sockfd, (t_fdpollfn)flashserver_connectpoll, x);
    	x->x_connectout = outlet_new(&x->x_obj, &s_float);
		x->x_clientno = outlet_new(&x->x_obj, &s_float);
		x->x_clientsock = outlet_new(&x->x_obj, &s_float);
		x->x_connectionip = outlet_new(&x->x_obj, &s_symbol);
		b = binbuf_new();
	}
    x->x_connectsocket = sockfd;
    x->x_nconnections = 0;
	x->x_verbose = 0;
	x->x_canvas = canvas_getcurrent();

	if(fclients)
	{
		x->x_maxconnect = (t_int)fclients;
		if(x->x_maxconnect > MAX_CONNECT)
			x->x_maxconnect = MAX_CONNECT;
		post("flashserver: set maximum number of clients to %d", x->x_maxconnect);
	}
	else x->x_maxconnect = MAX_CONNECT;
	for(i = 0; i < x->x_maxconnect; i++)
	{
		x->x_fd[i] = -1;
		x->x_client[i] = -1;
		x->x_clientlist[i] = -1;
	}

	if(fprep)
	{
		x->x_prepend = 1;
		post("flashserver: setting \"prepend mode\"");
	}
	else x->x_prepend = 0;

	if(fverbose)
	{
		x->x_verbose = 1;
		post("flashserver: debugging mode");
	}

    return (x);
}

static void flashserver_free(t_flashserver *x)
{
	int i;
	post("flashserver: closing open connections....");
	for(i = 0; i < x->x_nconnections; i++)
	{
		sys_rmpollfn(x->x_fd[i]);
    	sys_closesocket(x->x_fd[i]);
	}
    if (x->x_connectsocket >= 0)
    {
    	sys_rmpollfn(x->x_connectsocket);
    	sys_closesocket(x->x_connectsocket);
    }
	post("flashserver:   ...done");
	binbuf_free(b);
}

void flashserver_setup(void)
{
    flashserver_class = class_new(gensym("flashserver"),(t_newmethod)flashserver_new, (t_method)flashserver_free,
    	sizeof(t_flashserver), 0, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(flashserver_class, (t_method)flashserver_print, gensym("print"), 0);
	class_addmethod(flashserver_class, (t_method)flashserver_send, gensym("send"), A_GIMME, 0);
	class_addmethod(flashserver_class, (t_method)flashserver_sendXML, gensym("sendXML"), A_GIMME, 0);
	class_addmethod(flashserver_class, (t_method)flashserver_client_send, gensym("client"), A_GIMME, 0);
	class_addmethod(flashserver_class, (t_method)flashserver_client_sendXML, gensym("clientXML"), A_GIMME, 0);
	class_addmethod(flashserver_class, (t_method)flashserver_broadcast, gensym("broadcast"), A_GIMME, 0);
	class_addmethod(flashserver_class, (t_method)flashserver_broadcastXML, gensym("broadcastXML"), A_GIMME, 0);
	class_addmethod(flashserver_class, (t_method)flashserver_open, gensym("open"), A_GIMME, 0);
	class_addmethod(flashserver_class, (t_method)flashserver_prepend, gensym("prepend"), A_FLOAT, 0);
	class_addmethod(flashserver_class, (t_method)flashserver_client_kick, gensym("kick"), A_FLOAT, 0);
	class_sethelpsymbol(flashserver_class, gensym("help-flashserver.pd"));
    post(version);
}
