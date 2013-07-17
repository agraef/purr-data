#include "m_pd.h"
//#include "m_imp.h"
#include "s_stuff.h"

#include <sys/types.h>
#include <string.h>
#ifdef MSW
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#define SOCKET_ERROR -1
#endif

#define DISIS_INBUFSIZE 4096

typedef struct _msgqueue
{
	t_binbuf *msg_binbuf;
	t_atom msg_addrbytes[5];
	struct _msgqueue *msg_next;
} t_msgqueue;

struct _socketreceiver
{
    char *sr_inbuf;
    int sr_inhead;
    int sr_intail;
    void *sr_owner;
    int sr_udp;
    t_socketnotifier sr_notifier;
    t_socketreceivefn sr_socketreceivefn;
};

static t_class *disis_netreceive_class;
static void disis_socketreceiver_getudp(t_socketreceiver *x, int fd);

typedef struct _disis_netreceive
{
    t_object x_obj;
    t_outlet *x_msgout;
    t_outlet *x_connectout;
    t_outlet *x_addrout;
    t_atom x_addrbytes[5];
    int x_connectsocket;
	int x_acceptsocket;
    int x_nconnections;
	int x_oldnconnections;
    int x_udp;

	//to ensure arriving messages will be processed in sync
	t_clock *x_clock;
	int x_isdeleting;
	t_msgqueue *x_end;
	t_msgqueue *x_start;
} t_disis_netreceive;

static void disis_netreceive_init_msgqueue(t_disis_netreceive *x)
{
	//no msgqueue exists
	if (x->x_start == NULL) {
		x->x_start = (t_msgqueue *)t_getbytes(sizeof *x->x_start);
		x->x_end = x->x_start;
	}
	//there is one or more
	else {
		t_msgqueue *tmp = (t_msgqueue *)t_getbytes(sizeof *x->x_start);
		x->x_end->msg_next = tmp;
		x->x_end = tmp;
		//fprintf(stderr,"queue larger than 1\n");
	}
	//init internal values
	x->x_end->msg_binbuf = NULL;
	x->x_end->msg_next = NULL;
}

static void disis_netreceive_output(t_disis_netreceive *x)
{
	t_msgqueue *tmp;

    //second outlet for tcp connections
	if (!x->x_udp && x->x_oldnconnections != x->x_nconnections) {
		x->x_oldnconnections = x->x_nconnections;
		outlet_float(x->x_connectout, x->x_nconnections);
	}

	//now navigate the msgqueue
	if (x->x_start) {
		while (x->x_start) {
			//second outlet for udp connections
			if (x->x_udp)
				outlet_list(x->x_addrout, &s_list, 5L, x->x_start->msg_addrbytes);

			if (x->x_start->msg_binbuf)	{		
				//first outlet
				int msg, natom = binbuf_getnatom(x->x_start->msg_binbuf);
				t_atom *at = binbuf_getvec(x->x_start->msg_binbuf);

				for (msg = 0; msg < natom;)
				{
					int emsg;
					for (emsg = msg; emsg < natom && at[emsg].a_type != A_COMMA
						&& at[emsg].a_type != A_SEMI; emsg++)
						    ;
					if (emsg > msg)
					{
						int i;
						for (i = msg; i < emsg; i++)
						    if (at[i].a_type == A_DOLLAR || at[i].a_type == A_DOLLSYM)
						{
						    pd_error(x, "disis_netreceive: got dollar sign in message");
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
			}

			//point to the next msgqueue and use tmp to destruct the parsed one
			tmp = x->x_start;
			x->x_start = x->x_start->msg_next;

			//destruct the parsed one
			if (tmp->msg_binbuf)
				binbuf_free(tmp->msg_binbuf);

			//deallocate msgqueue tmp is pointing to
			freebytes(tmp, sizeof(*tmp));
		}
		//once done with entire queue make sure disis_netsend pointers point to NULL
		x->x_start = NULL;
		x->x_end = NULL;
	}
}

static void disis_netreceive_notify(t_disis_netreceive *x)
{
	x->x_oldnconnections = x->x_nconnections;
    --x->x_nconnections;
	if (!x->x_isdeleting)
		clock_delay(x->x_clock, 0);
}

static void disis_netreceive_doit(void *z, t_binbuf *b)
{ 
	//fprintf(stderr,"doit\n");
	t_disis_netreceive *x = (t_disis_netreceive *)z;
	
	//for tcp packets we create new msgqueue here
	if (!x->x_udp)
		disis_netreceive_init_msgqueue(x);

	x->x_end->msg_binbuf = binbuf_duplicate(b);
	clock_delay(x->x_clock, 0);

	/*
    //t_atom messbuf[1024];
    int msg, natom = binbuf_getnatom(b);
    t_atom *at = binbuf_getvec(b);

    for (msg = 0; msg < natom;)
    {
        int emsg;
        for (emsg = msg; emsg < natom && at[emsg].a_type != A_COMMA
            && at[emsg].a_type != A_SEMI; emsg++)
                ;
        if (emsg > msg)
        {
            int i;
            for (i = msg; i < emsg; i++)
                if (at[i].a_type == A_DOLLAR || at[i].a_type == A_DOLLSYM)
            {
                pd_error(x, "disis_netreceive: got dollar sign in message");
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
	*/

}

static void disis_netreceive_connectpoll(t_disis_netreceive *x)
{
    int fd = accept(x->x_connectsocket, 0, 0);
    if (fd < 0) post("disis_netreceive: accept failed");
    else
    {
        t_socketreceiver *y = socketreceiver_new((void *)x, 
            (t_socketnotifier)disis_netreceive_notify,
                (x->x_msgout ? disis_netreceive_doit : 0), 0);
        sys_addpollfn(fd, (t_fdpollfn)socketreceiver_read, y);
        //outlet_float(x->x_connectout, ++x->x_nconnections);
		x->x_oldnconnections = x->x_nconnections;
		++x->x_nconnections;
		x->x_acceptsocket = fd;
		clock_delay(x->x_clock, 0);
    }
}

static void disis_netreceive_setport(t_disis_netreceive *x, t_floatarg fportno)
{
	if (!fportno)
		return;

	//fprintf(stderr,"disis_netreceive_setport\n");
	struct sockaddr_in server;
	int sockfd, intarg, portno = fportno;

	x->x_isdeleting = 1;

    if (x->x_connectsocket >= 0)
    {
        sys_rmpollfn(x->x_connectsocket);
		if (!x->x_udp && x->x_acceptsocket >= 0)
			sys_rmpollfn(x->x_acceptsocket);
        sys_closesocket(x->x_connectsocket);
    }

	clock_unset(x->x_clock);
	clock_free(x->x_clock);

	//delete the msgqueue (if any)
	if (x->x_start != NULL) {
		t_msgqueue *tmp;
		while (x->x_start) {
			tmp = x->x_start;
			x->x_start = x->x_start->msg_next;

			//destruct the parsed one
			if (tmp->msg_binbuf)
				binbuf_free(tmp->msg_binbuf);

			//deallocate msgqueue tmp is pointing to
			freebytes(tmp, sizeof(*tmp));
		}
	}

	x->x_start = NULL;
	x->x_end = NULL;

	// now recreate connection with the new portno

        /* create a socket */
    sockfd = socket(AF_INET, (x->x_udp ? SOCK_DGRAM : SOCK_STREAM), 0);

    if (sockfd < 0)
    {
        sys_sockerror("socket");
        return;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    intarg = 1;

		/* ask OS to allow another Pd to repoen this port after we close it. */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
        (char *)&intarg, sizeof(intarg)) < 0)
            post("setsockopt (SO_REUSEADDR) failed\n");

        /* Stream (TCP) sockets are set NODELAY */
    if (!x->x_udp)
    {
        intarg = 1;
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
            (char *)&intarg, sizeof(intarg)) < 0)
                post("setsockopt (TCP_NODELAY) failed\n");
    }
        /* assign server port number */
    server.sin_port = htons((u_short)portno);

        /* name the socket */
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        sys_sockerror("bind");
        sys_closesocket(sockfd);
        return;
    }

	x->x_acceptsocket = -1;

    if (x->x_udp)        /* datagram protocol */
    {
        t_socketreceiver *y = socketreceiver_new((void *)x, 
            (t_socketnotifier)disis_netreceive_notify,
                (x->x_msgout ? disis_netreceive_doit : 0), 1);
        sys_addpollfn(sockfd, (t_fdpollfn)disis_socketreceiver_getudp, y);
        x->x_connectout = 0;
    }
    else        /* streaming protocol */
    {
        if (listen(sockfd, 5) < 0)
        {
            sys_sockerror("listen");
            sys_closesocket(sockfd);
            sockfd = -1;
        }
        else
        {
            sys_addpollfn(sockfd, (t_fdpollfn)disis_netreceive_connectpoll, x);
        }
    }
    x->x_connectsocket = sockfd;
	//post("connectsocket = %d", x->x_connectsocket);
    x->x_nconnections = 0;
	x->x_isdeleting = 0;

	x->x_clock = clock_new(x, (t_method)disis_netreceive_output);
	clock_delay(x->x_clock, 0);
}

static void *disis_netreceive_new(t_symbol *compatflag,
    t_floatarg fportno, t_floatarg udpflag)
{
    t_disis_netreceive *x;
    struct sockaddr_in server;
    int sockfd, portno = fportno, udp = (udpflag != 0);
    int old = !strcmp(compatflag->s_name , "old");
    int intarg;
        /* create a socket */
    sockfd = socket(AF_INET, (udp ? SOCK_DGRAM : SOCK_STREAM), 0);
#if 0
    fprintf(stderr, "receive socket %d\n", sockfd);
#endif
    if (sockfd < 0)
    {
        sys_sockerror("socket");
        return (0);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

#if 1
        /* ask OS to allow another Pd to repoen this port after we close it. */
    intarg = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
        (char *)&intarg, sizeof(intarg)) < 0)
            post("setsockopt (SO_REUSEADDR) failed\n");
#endif
#if 0
    intarg = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,
        &intarg, sizeof(intarg)) < 0)
            post("setsockopt (SO_RCVBUF) failed\n");
#endif
        /* Stream (TCP) sockets are set NODELAY */
    if (!udp)
    {
        intarg = 1;
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
            (char *)&intarg, sizeof(intarg)) < 0)
                post("setsockopt (TCP_NODELAY) failed\n");
    }
        /* assign server port number */
    server.sin_port = htons((u_short)portno);

        /* name the socket */
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        sys_sockerror("bind");
        sys_closesocket(sockfd);
        return (0);
    }
    x = (t_disis_netreceive *)pd_new(disis_netreceive_class);
    if (old)
    {
        /* old style, nonsecure version */
        x->x_msgout = 0;
        x->x_addrout = 0;
    }
    else 
    {
        x->x_msgout = outlet_new(&x->x_obj, &s_anything);
    }

	x->x_acceptsocket = -1;

    if (udp)        /* datagram protocol */
    {
        t_socketreceiver *y = socketreceiver_new((void *)x, 
            (t_socketnotifier)disis_netreceive_notify,
                (x->x_msgout ? disis_netreceive_doit : 0), 1);
        sys_addpollfn(sockfd, (t_fdpollfn)disis_socketreceiver_getudp, y);

		x->x_addrout = outlet_new(&x->x_obj, &s_list);
        x->x_connectout = 0;
    }
    else        /* streaming protocol */
    {
        if (listen(sockfd, 5) < 0)
        {
            sys_sockerror("listen");
            sys_closesocket(sockfd);
            sockfd = -1;
        }
        else
        {
            sys_addpollfn(sockfd, (t_fdpollfn)disis_netreceive_connectpoll, x);
            x->x_connectout = outlet_new(&x->x_obj, &s_float);
        }
    }
    x->x_connectsocket = sockfd;
	//post("connectsocket = %d", x->x_connectsocket);
    x->x_nconnections = 0;
    x->x_udp = udp;
	x->x_isdeleting = 0;

	x->x_clock = clock_new(x, (t_method)disis_netreceive_output);

    return (x);
}

static void disis_socketreceiver_getudp(t_socketreceiver *x, int fd)
{
	//fprintf(stderr,"getudp\n");
    t_binbuf *inbinbuf;
    inbinbuf = binbuf_new();
	t_disis_netreceive *rec = x->sr_owner;

	//for udp packets we create new msgqueue here
	disis_netreceive_init_msgqueue(rec);

    struct sockaddr_in from;
    socklen_t length = sizeof(from);
    long addr;
    unsigned short port;
	char msginbuf[DISIS_INBUFSIZE];

    int ret = recvfrom(fd, msginbuf, DISIS_INBUFSIZE, 0, (struct sockaddr *)&from, &length);

    addr = ntohl(from.sin_addr.s_addr);
    port = ntohs(from.sin_port);

    //t_disis_netreceive *rec = x->sr_owner;

    int i;
    for (i = 0; i < 5; ++i)
    {
        rec->x_end->msg_addrbytes[i].a_type = A_FLOAT;
        rec->x_end->msg_addrbytes[i].a_w.w_float = 0;
    }

    rec->x_end->msg_addrbytes[0].a_w.w_float = (addr & 0xFF000000)>>24;
    rec->x_end->msg_addrbytes[1].a_w.w_float = (addr & 0x0FF0000)>>16;
    rec->x_end->msg_addrbytes[2].a_w.w_float = (addr & 0x0FF00)>>8;
    rec->x_end->msg_addrbytes[3].a_w.w_float = (addr & 0x0FF);
    rec->x_end->msg_addrbytes[4].a_w.w_float = port;
    //outlet_list(rec->x_addrout, &s_list, 5L, rec->x_addrbytes);

    if (ret < 0)
    {
		//post("lgth=%d received %s", ret, msginbuf);
        sys_sockerror("recvfrom");
        sys_rmpollfn(fd);
        sys_closesocket(fd);
    }
    else if (ret > 0)
    {
        msginbuf[ret] = 0;
#if 0
        post("%s", buf);
#endif
	    if (msginbuf[ret-1] != '\n')
	    {
#if 0
	        buf[ret] = 0;
	        error("dropped bad buffer %s\n", buf);
#endif
	    }
	    else
	    {
	        char *semi = strchr(msginbuf, ';');
	        if (semi) 
	            *semi = 0;
	        binbuf_text(inbinbuf, msginbuf, strlen(msginbuf));
	        //outlet_setstacklim();
	        if (x->sr_socketreceivefn)
	            (*x->sr_socketreceivefn)(x->sr_owner, inbinbuf);
	        else bug("disis_socketreceiver_getudp");
	    }
    }
}

static void disis_netreceive_free(t_disis_netreceive *x)
{
	x->x_isdeleting = 1;

    if (x->x_connectsocket >= 0)
    {
        sys_rmpollfn(x->x_connectsocket);
		if (!x->x_udp && x->x_acceptsocket >= 0)
			sys_rmpollfn(x->x_acceptsocket);
        sys_closesocket(x->x_connectsocket);
    }

	clock_unset(x->x_clock);
	clock_free(x->x_clock);

	//delete the msgqueue (if any)
	if (x->x_start != NULL) {
		t_msgqueue *tmp;
		while (x->x_start) {
			tmp = x->x_start;
			x->x_start = x->x_start->msg_next;

			//destruct the parsed one
			if (tmp->msg_binbuf)
				binbuf_free(tmp->msg_binbuf);

			//deallocate msgqueue tmp is pointing to
			freebytes(tmp, sizeof(*tmp));
		}
	}
}

void disis_netreceive_setup(void)
{
    disis_netreceive_class = class_new(gensym("disis_netreceive"),
        (t_newmethod)disis_netreceive_new, (t_method)disis_netreceive_free,
        sizeof(t_disis_netreceive), 0, A_DEFFLOAT, A_DEFFLOAT, 
            A_DEFSYM, 0);

	class_addmethod(disis_netreceive_class, (t_method)disis_netreceive_setport, gensym("port"), A_DEFFLOAT, 0);
}
