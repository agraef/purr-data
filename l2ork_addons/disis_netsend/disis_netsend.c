#include "m_pd.h"
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
#define SOCKET_ERROR -1
#endif

static t_class *disis_netsend_class;

typedef struct _binbuf_queue
{
	t_binbuf *b;
	struct _binbuf_queue *next;
} t_binbuf_queue;

typedef struct _disis_netsend
{
    t_object x_obj;
	t_binbuf *queue;
    int x_fd;
    int x_protocol;
	t_binbuf_queue *start;
	t_binbuf_queue *end;
} t_disis_netsend;

t_binbuf_queue *binbuf_queue_new(void)
{
    t_binbuf_queue *y = (t_binbuf_queue *)t_getbytes(sizeof(*y));
	y->b = binbuf_new();
    return (y);
}

void binbuf_queue_free(t_binbuf_queue *x)
{
    binbuf_free(x->b);
    t_freebytes(x,  sizeof(*x));
}

static void *disis_netsend_new(t_floatarg udpflag)
{
    t_disis_netsend *x = (t_disis_netsend *)pd_new(disis_netsend_class);
    outlet_new(&x->x_obj, &s_float);
    x->x_fd = -1;
    x->x_protocol = (udpflag != 0 ? SOCK_DGRAM : SOCK_STREAM);
    t_binbuf_queue *b = binbuf_queue_new();
	x->start = b;
	x->end = b;
    return (x);
}

static void disis_netsend_enqueue(t_disis_netsend *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_fd >= 0)
    {
        t_binbuf_queue *bq = binbuf_queue_new();
		binbuf_add(bq->b, argc, argv);
		x->end->next = bq;
		x->end = bq;
	}
}

static void disis_netsend_connect(t_disis_netsend *x, t_symbol *hostname,
    t_floatarg fportno)
{
    struct sockaddr_in server;
    struct hostent *hp;
    int sockfd;
    int portno = fportno;
    int broadcast = 1;/* nonzero is true */
    int intarg;
    if (x->x_fd >= 0)
    {
        error("disis_netsend_connect: already connected");
        return;
    }

        /* create a socket */
    sockfd = socket(AF_INET, x->x_protocol, 0);
#if 0
    fprintf(stderr, "send socket %d\n", sockfd);
#endif
    if (sockfd < 0)
    {
        sys_sockerror("socket");
        return;
    }

/* Based on zmoelnig's patch 2221504:
Enable sending of broadcast messages (if hostname is a broadcast address)*/
#ifdef SO_BROADCAST
    if( 0 != setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const void *)&broadcast, sizeof(broadcast)))
    {
        pd_error(x, "couldn't switch to broadcast mode");
    }
#endif /* SO_BROADCAST */

    /* connect socket using hostname provided in command line */
    server.sin_family = AF_INET;
    hp = gethostbyname(hostname->s_name);
    if (hp == 0)
    {
        post("bad host?\n");
        return;
    }
#if 0
    intarg = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,
        &intarg, sizeof(intarg)) < 0)
            post("setsockopt (SO_RCVBUF) failed\n");
#endif
        /* for stream (TCP) sockets, specify "nodelay" */
    if (x->x_protocol == SOCK_STREAM)
    {
        intarg = 1;
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
            (char *)&intarg, sizeof(intarg)) < 0)
                post("setsockopt (TCP_NODELAY) failed\n");
    }
    memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);

    /* assign client port number */
    server.sin_port = htons((u_short)portno);

    post("connecting to port %d", portno);
        /* try to connect.  LATER make a separate thread to do this
        because it might block */
    if (connect(sockfd, (struct sockaddr *) &server, sizeof (server)) < 0)
    {
        sys_sockerror("connecting stream socket");
        sys_closesocket(sockfd);
        return;
    }
    x->x_fd = sockfd;
    outlet_float(x->x_obj.ob_outlet, 1);
}

static void disis_netsend_disconnect(t_disis_netsend *x)
{
    if (x->x_fd >= 0)
    {
        sys_closesocket(x->x_fd);
        x->x_fd = -1;
        outlet_float(x->x_obj.ob_outlet, 0);
    }
}

static void disis_netsend_send(t_disis_netsend *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_fd >= 0)
    {
        t_binbuf *b = binbuf_new();
        char *buf, *bp;
        int length, sent;
        t_atom at;
        binbuf_add(b, argc, argv);
        SETSEMI(&at);
        binbuf_add(b, 1, &at);
        binbuf_gettext(b, &buf, &length);
        for (bp = buf, sent = 0; sent < length;)
        {
            //static double lastwarntime;
            //static double pleasewarn;
            //double timebefore = sys_getrealtime();
            int res = send(x->x_fd, bp, length-sent, 0);
            //double timeafter = sys_getrealtime();
            //int late = (timeafter - timebefore > 0.005);
            //if (late || pleasewarn)
            //{
            //    if (timeafter > lastwarntime + 2)
            //    {
            //         post("disis_netsend blocked %d msec",
            //            (int)(1000 * ((timeafter - timebefore) + pleasewarn)));
            //         pleasewarn = 0;
            //         lastwarntime = timeafter;
            //    }
            //    else if (late) pleasewarn += timeafter - timebefore;
            //}
            if (res <= 0)
            {
                sys_sockerror("disis_netsend");
                disis_netsend_disconnect(x);
                break;
            }
            else
            {
                sent += res;
                bp += res;
            }
        }
        t_freebytes(buf, length);
        binbuf_free(b);
    }
    //else error("disis_netsend: not connected");
}

static void disis_netsend_doBang(t_disis_netsend *x)
{
    if (x->x_fd >= 0)
    {
		t_binbuf_queue *current;
	    char *buf, *bp;
	    int length, sent;
	    t_atom at;
		int res;

		while (x->start != x->end) {
			current = x->start->next;
			binbuf_queue_free(x->start);
			x->start = current;
		    //t_binbuf *b = binbuf_new();
		    //binbuf_add(b, argc, argv);
		    SETSEMI(&at);
		    binbuf_add(current->b, 1, &at);
		    binbuf_gettext(current->b, &buf, &length);
			//post("buf=%s length=%d", buf, length);
		    for (bp = buf, sent = 0; sent < length;)
		    {
		        //static double lastwarntime;
		        //static double pleasewarn;
		        //double timebefore = sys_getrealtime();
		        res = send(x->x_fd, bp, length-sent, 0);
		        //double timeafter = sys_getrealtime();
		        //int late = (timeafter - timebefore > 0.005);
		        //if (late || pleasewarn)
		        //{
		        //    if (timeafter > lastwarntime + 2)
		        //    {
		        //         post("disis_netsend blocked %d msec",
		        //            (int)(1000 * ((timeafter - timebefore) + pleasewarn)));
		        //         pleasewarn = 0;
		        //         lastwarntime = timeafter;
		        //    }
		        //    else if (late) pleasewarn += timeafter - timebefore;
		        //}
		        if (res <= 0)
		        {
		            sys_sockerror("disis_netsend");
		            disis_netsend_disconnect(x);
		            break;
		        }
		        else
		        {
		            sent += res;
		            bp += res;
		        }
		    }
		    t_freebytes(buf, length);
		    //binbuf_free(b);
		}
    }
    //else error("disis_netsend: not connected");
}

static void disis_netsend_free(t_disis_netsend *x)
{
    disis_netsend_disconnect(x);
	if (x->start)
		binbuf_queue_free(x->start);
}

void disis_netsend_setup(void)
{
    disis_netsend_class = class_new(gensym("disis_netsend"), (t_newmethod)disis_netsend_new,
        (t_method)disis_netsend_free,
        sizeof(t_disis_netsend), 0, A_DEFFLOAT, 0);
    class_addmethod(disis_netsend_class, (t_method)disis_netsend_connect,
        gensym("connect"), A_SYMBOL, A_FLOAT, 0);
    class_addmethod(disis_netsend_class, (t_method)disis_netsend_disconnect,
        gensym("disconnect"), 0);
    class_addmethod(disis_netsend_class, (t_method)disis_netsend_send, gensym("send"),
        A_GIMME, 0);
    class_addmethod(disis_netsend_class, (t_method)disis_netsend_enqueue, gensym("enqueue"),
        A_GIMME, 0);
	class_addbang(disis_netsend_class, disis_netsend_doBang);
}

