/* tcpserver.c Martin Peach 20100508 */
/* with contributions from Ivica Ico Bukvic <ico@bukvic.net> */
/* and IOhannes Zmoelnig zmoelnig@iem.at */
/* tcpserver.c is based on netserver by Olaf Matthes <olaf.matthes@gmx.de>: */
/* --------------------------  netserver  ------------------------------------- */
/*                                                                              */
/* A server for bidirectional communication from within Pd.                     */
/* Allows to send back data to specific clients connected to the server.        */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>                                */
/* Get source at http://www.akustische-kunst.org/puredata/maxlib                */
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
/*                                                                              */
/* ---------------------------------------------------------------------------- */
//define DEBUG

#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"

#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/ioctl.h> /* linux has the SIOCOUTQ ioctl */
#define SOCKET_ERROR -1
#else
#include <winsock2.h>
#include <ws2tcpip.h> /* for socklen_t */
#endif

/* support older Pd versions without sys_open(), sys_fopen(), sys_fclose() */
#if PD_MAJOR_VERSION == 0 && PD_MINOR_VERSION < 44
#define sys_open open
#define sys_fopen fopen
#define sys_fclose fclose
#endif

#ifdef _MSC_VER
#define snprintf sprintf_s
#endif

#define MAX_CONNECT 1024 /* maximum number of connections to reserve space for */
#define INBUFSIZE 65536L /* was 4096: size of receiving data buffer */
#define MAX_UDP_RECEIVE 65536L /* longer than data in maximum UDP packet */

/* ----------------------------- tcpserver ------------------------- */

static t_class *tcpserver_class;
static char objName[] = "tcpserver";

typedef void (*t_tcpserver_socketnotifier)(void *x);
typedef void (*t_tcpserver_socketreceivefn)(void *x, t_binbuf *b);

typedef struct _tcpserver_socketreceiver
{
    t_symbol                    *sr_host;
    int                         sr_fd;
    int                         sr_fdbuf;
    u_long                      sr_addr;
    unsigned char               *sr_inbuf;
    int                         sr_inhead;
    int                         sr_intail;
    void                        *sr_owner;
    t_tcpserver_socketnotifier  sr_notifier;
    t_tcpserver_socketreceivefn sr_socketreceivefn;
} t_tcpserver_socketreceiver;

typedef struct _tcpserver_send_params
{
    int     client;
    int     sockfd;
    char    *byte_buf;
    size_t  length;
    int     verbosity;
} t_tcpserver_send_params;

typedef struct _tcpserver
{
    t_object                    x_obj;
    t_outlet                    *x_msgout;
    t_outlet                    *x_connectout;
    t_outlet                    *x_sockout;
    t_outlet                    *x_addrout;
    t_outlet                    *x_status_outlet;
    int                         x_dump; // 1 = hexdump received bytes

    t_tcpserver_socketreceiver  *x_sr[MAX_CONNECT];

    t_atom                      x_addrbytes[4];
    int                         x_sock_fd;
    int                         x_connectsocket;
    int                         x_port;
    int                         x_nconnections;
    int                         x_blocked;
    int                         x_verbosity;
    t_atom                      x_msgoutbuf[MAX_UDP_RECEIVE];
    char                        x_msginbuf[MAX_UDP_RECEIVE];
} t_tcpserver;

typedef struct _tcpserver_broadcast_params
{
    t_tcpserver *x;
    int         argc;
    t_atom      argv[MAX_UDP_RECEIVE];
} t_tcpserver_broadcast_params;

static t_tcpserver_socketreceiver *tcpserver_socketreceiver_new(void *owner, t_tcpserver_socketnotifier notifier,
    t_tcpserver_socketreceivefn socketreceivefn);
static int tcpserver_socketreceiver_doread(t_tcpserver_socketreceiver *x);
static void tcpserver_socketreceiver_read(t_tcpserver_socketreceiver *x, int fd);
static void tcpserver_socketreceiver_free(t_tcpserver_socketreceiver *x);
static void tcpserver_send(t_tcpserver *x, t_symbol *s, int argc, t_atom *argv);
static void tcpserver_send_bytes(int sockfd, t_tcpserver *x, int argc, t_atom *argv);
#ifdef SIOCOUTQ
static int tcpserver_send_buffer_avaliable_for_client(t_tcpserver *x, int client);
#endif
static void *tcpserver_send_buf_thread(void *arg);
static void *tcpserver_broadcast_thread(void *arg);
static void tcpserver_client_send(t_tcpserver *x, t_symbol *s, int argc, t_atom *argv);
static void tcpserver_output_client_state(t_tcpserver *x, int client);
static int tcpserver_get_socket_send_buf_size(int sockfd);
static int tcpserver_set_socket_send_buf_size(int sockfd, int size);
static void tcpserver_buf_size(t_tcpserver *x, t_symbol *s, int argc, t_atom *argv);
static void tcpserver_disconnect(t_tcpserver *x);
static void tcpserver_client_disconnect(t_tcpserver *x, t_floatarg fclient);
static void tcpserver_socket_disconnect(t_tcpserver *x, t_floatarg fsocket);
static void tcpserver_verbosity(t_tcpserver *x, t_floatarg verbosity_level);
static void tcpserver_broadcast(t_tcpserver *x, t_symbol *s, int argc, t_atom *argv);
static void tcpserver_notify(t_tcpserver *x);
static void tcpserver_connectpoll(t_tcpserver *x);
static void tcpserver_print(t_tcpserver *x);
static void tcpserver_unblock(t_tcpserver *x);
static void *tcpserver_new(t_floatarg fportno);
static void tcpserver_port(t_tcpserver *x, t_floatarg fportno);
static void tcpserver_free(t_tcpserver *x);
void tcpserver_setup(void);
static void tcpserver_dump(t_tcpserver *x, t_float dump);
static void tcpserver_hexdump(unsigned char *buf, long len, int verbosity);


static void tcpserver_verbosity(t_tcpserver *x, t_floatarg verbosity_level)
{
    x->x_verbosity = verbosity_level;
    if (x->x_verbosity > 0)  post("tcpserver_verbosity now %d", x->x_verbosity);
}

static void tcpserver_dump(t_tcpserver *x, t_float dump)
{
    x->x_dump = (dump == 0)?0:1;
}

static void tcpserver_hexdump(unsigned char *buf, long len, int verbosity)
{
#define BYTES_PER_LINE 16
    char hexStr[(3*BYTES_PER_LINE)+1];
    char ascStr[BYTES_PER_LINE+1];
    long i, j, k = 0L;

    if (verbosity > 0)  post("tcpserver_hexdump of %d bytes", len);
    while (k < len)
    {
        for (i = j = 0; i < BYTES_PER_LINE; ++i, ++k, j+=3)
        {
            if (k < len)
            {
                snprintf(&hexStr[j], 4, "%02X ", buf[k]);
                snprintf(&ascStr[i], 2, "%c", ((buf[k] >= 32) && (buf[k] <= 126))? buf[k]: '.');
            }
            else
            { // the last line
                snprintf(&hexStr[j], 4, "   ");
                snprintf(&ascStr[i], 2, " ");
            }
        }
        post ("%s%s", hexStr, ascStr);
    }
}

static t_tcpserver_socketreceiver *tcpserver_socketreceiver_new(void *owner, t_tcpserver_socketnotifier notifier,
    t_tcpserver_socketreceivefn socketreceivefn)
{
    t_tcpserver_socketreceiver *x = (t_tcpserver_socketreceiver *)getbytes(sizeof(*x));
    if (!x)
    {
        error("%s_socketreceiver: unable to allocate %zu bytes", objName, sizeof(*x));
    }
    else
    {
        x->sr_inhead = x->sr_intail = 0;
        x->sr_owner = owner;
        x->sr_notifier = notifier;
        x->sr_socketreceivefn = socketreceivefn;
        if (!(x->sr_inbuf = malloc(INBUFSIZE)))
        {
            freebytes(x, sizeof(*x));
            x = NULL;
            error("%s_socketreceiver: unable to allocate %ld bytes", objName, INBUFSIZE);
        }
    }
    return (x);
}

/* this is in a separately called subroutine so that the buffer isn't
   sitting on the stack while the messages are getting passed. */
static int tcpserver_socketreceiver_doread(t_tcpserver_socketreceiver *x)
{
    char            messbuf[INBUFSIZE];
    char            *bp = messbuf;
    int             indx, i;
    int             inhead = x->sr_inhead;
    int             intail = x->sr_intail;
    unsigned char   c;
    t_tcpserver     *y = x->sr_owner;
    unsigned char   *inbuf = x->sr_inbuf;

    if (intail == inhead) return (0);
    if (y->x_verbosity > 1) post ("%s_socketreceiver_doread: intail=%d inhead=%d", objName, intail, inhead);

    for (indx = intail, i = 0; indx != inhead; indx = (indx+1)&(INBUFSIZE-1), ++i)
    {
        c = *bp++ = inbuf[indx];
        y->x_msgoutbuf[i].a_w.w_float = (float)c;
    }

    if (y->x_dump)tcpserver_hexdump(&inbuf[intail], i, y->x_verbosity);

    if (i > 1) outlet_list(y->x_msgout, &s_list, i, y->x_msgoutbuf);
    else outlet_float(y->x_msgout, y->x_msgoutbuf[0].a_w.w_float);

 //   intail = (indx+1)&(INBUFSIZE-1);
    x->sr_inhead = inhead;
    x->sr_intail = indx;//intail;
    return (1);
}

static void tcpserver_socketreceiver_read(t_tcpserver_socketreceiver *x, int fd)
{
    int         readto = (x->sr_inhead >= x->sr_intail ? INBUFSIZE : x->sr_intail-1);
    int         ret, i;
    t_tcpserver *y = x->sr_owner;

    y->x_sock_fd = fd;
    /* the input buffer might be full.  If so, drop the whole thing */
    if (readto == x->sr_inhead)
    {
        if (y->x_verbosity > 0)  post("%s: dropped message", objName);
        x->sr_inhead = x->sr_intail = 0;
        readto = INBUFSIZE;
    }
    else
    {
        ret = recv(fd, x->sr_inbuf + x->sr_inhead,
            readto - x->sr_inhead, 0);
        if (ret < 0)
        {
            sys_sockerror("tcpserver: recv");
            if (x->sr_notifier) (*x->sr_notifier)(x->sr_owner);
            sys_rmpollfn(fd);
            sys_closesocket(fd);
        }
        else if (ret == 0)
        {
            if (y->x_verbosity > 0) post("%s: connection closed on socket %d", objName, fd);
            if (x->sr_notifier) (*x->sr_notifier)(x->sr_owner);
            sys_rmpollfn(fd);
            sys_closesocket(fd);
        }
        else
        {
            if (y->x_verbosity > 1) post ("%s_socketreceiver_read: received %d byte%s", objName, ret, (ret==1)?"":"s");
            x->sr_inhead += ret;
            if (x->sr_inhead >= INBUFSIZE) x->sr_inhead = 0;
            /* output client's IP and socket no. */
            for(i = 0; i < y->x_nconnections; i++)	/* search for corresponding IP */
            {
                if(y->x_sr[i]->sr_fd == y->x_sock_fd)
                {
                    /* find sender's ip address and output it */
                    y->x_addrbytes[0].a_w.w_float = (y->x_sr[i]->sr_addr & 0xFF000000)>>24;
                    y->x_addrbytes[1].a_w.w_float = (y->x_sr[i]->sr_addr & 0x0FF0000)>>16;
                    y->x_addrbytes[2].a_w.w_float = (y->x_sr[i]->sr_addr & 0x0FF00)>>8;
                    y->x_addrbytes[3].a_w.w_float = (y->x_sr[i]->sr_addr & 0x0FF);
                    outlet_list(y->x_addrout, &s_list, 4L, y->x_addrbytes);
                    break;
                }
            }
            outlet_float(y->x_sockout, y->x_sock_fd);	/* the socket number */
            tcpserver_socketreceiver_doread(x);
        }
    }
}

static void tcpserver_socketreceiver_free(t_tcpserver_socketreceiver *x)
{
    t_tcpserver *y = x->sr_owner;

    if (y->x_verbosity > 1) post("tcpserver_socketreceiver_free x=%p", x);
    if (x != NULL)
    {
        if (NULL != x->sr_inbuf) 
        {
            if (y->x_verbosity > 1) post("tcpserver_socketreceiver_free freeing x->sr_inbuf %p", x->sr_inbuf);
            free(x->sr_inbuf);
            x->sr_inbuf = NULL;
        }
        freebytes(x, sizeof(*x));
    }
}

/* ---------------- main tcpserver (send) stuff --------------------- */

static void tcpserver_send_bytes(int client, t_tcpserver *x, int argc, t_atom *argv)
{
    static char             byte_buf[MAX_UDP_RECEIVE];// arbitrary maximum similar to max IP packet size
    int                     i, j, d;
    unsigned char           c;
    float                   f, e;
    int                     length;
    size_t                  flen = 0;
    int                     sockfd = x->x_sr[client]->sr_fd;
    char                    fpath[FILENAME_MAX];
    FILE                    *fptr;
    t_atom                  output_atom[3];
    t_tcpserver_send_params *ttsp;
    pthread_t               sender_thread;
    pthread_attr_t          sender_attr = {{0}};
    int                     sender_thread_result;

    /* process & send data */
    if(sockfd >= 0)
    {
        if (x->x_blocked > 0) goto failed;
        /* sender thread should start out detached so its resouces will be freed when it is done */
        if (0!= (sender_thread_result = pthread_attr_init(&sender_attr)))
        {
            error("%s: pthread_attr_init failed: %d", objName, sender_thread_result);
            goto failed;
        }
        if(0!= (sender_thread_result = pthread_attr_setdetachstate(&sender_attr, PTHREAD_CREATE_DETACHED)))
        {
            error("%s: pthread_attr_setdetachstate failed: %d", objName, sender_thread_result);
            goto failed;
        }
        for (i = j = 0; i < argc; ++i)
        {
            if (argv[i].a_type == A_FLOAT)
            { /* load floats into buffer as long as they are integers on [0..255]*/
                f = argv[i].a_w.w_float;
                d = (int)f;
                e = f - d;
                if (x->x_verbosity > 2) post("%s: argv[%d]: float:%f int:%d delta:%f", objName, i, f, d, e);
                if (e != 0)
                {
                    error("%s: item %d (%f) is not an integer", objName, i, f);
                    goto failed;
                }
                if ((d < 0) || (d > 255))
                {
                    error("%s: item %d (%f) is not between 0 and 255", objName, i, f);
                    goto failed;
                }
                c = (unsigned char)d; /* make sure it doesn't become negative; this only matters for post() */
                if (x->x_verbosity > 2) post("%s: argv[%d]: %d", objName, i, c);
                byte_buf[j++] = c;
                if (j >= MAX_UDP_RECEIVE)
                { /* if the argument list is longer than our buffer, send the buffer whenever it's full */
#ifdef SIOCOUTQ
                    if (tcpserver_send_buffer_avaliable_for_client(x, client) < j)
                    {
                        error("%s: buffer too small for client(%d)", objName, client);
                        goto failed;
                    }
#endif // SIOCOUTQ
                    ttsp = (t_tcpserver_send_params *)getbytes(sizeof(t_tcpserver_send_params));
                    if (ttsp == NULL)
                    {
                        error("%s: unable to allocate %zu bytes for t_tcpserver_send_params", objName, sizeof(t_tcpserver_send_params));
                        goto failed;
                    }
                    ttsp->client = client;
                    ttsp->sockfd = sockfd;
                    ttsp->byte_buf = byte_buf;
                    ttsp->length = j;
                    ttsp->verbosity = x->x_verbosity;
                    if (0 != (sender_thread_result = pthread_create(&sender_thread, &sender_attr, tcpserver_send_buf_thread, (void *)ttsp)))
                    {
                        ++x->x_blocked;
                        error("%s: couldn't create sender thread (%d)", objName, sender_thread_result);
                        if (NULL != ttsp)
                        {
                            freebytes (ttsp, sizeof (t_tcpserver_send_params));
                            ttsp = NULL;
                        }
                        goto failed;
                    }
                    flen += j;
                    j = 0;
                }
            }
            else if (argv[i].a_type == A_SYMBOL)
            { /* symbols are interpreted to be file names; attempt to load the file and send it */

                atom_string(&argv[i], fpath, FILENAME_MAX);
                if (x->x_verbosity > 2) post ("%s: fname: %s", objName, fpath);
                fptr = sys_fopen(fpath, "rb");
                if (fptr == NULL)
                {
                    error("%s: unable to open \"%s\"", objName, fpath);
                    goto failed;
                }
                rewind(fptr);
                while ((d = fgetc(fptr)) != EOF)
                {
                    if (x->x_verbosity > 2) post("%s: d is %d", objName, d);
                    byte_buf[j++] = (char)(d & 0x0FF);
                    if (x->x_verbosity > 2)  post("%s: byte_buf[%d] = %d", objName, j-1, byte_buf[j-1]);
                    if (j >= MAX_UDP_RECEIVE)
                    { /* if the file is longer than our buffer, send the buffer whenever it's full */
                        /* this might be better than allocating huge amounts of memory */
#ifdef SIOCOUTQ
                        if (tcpserver_send_buffer_avaliable_for_client(x, client) < j)
                        {
                            error("%s: buffer too small for client(%d)", objName, client);
                            goto failed;
                        }
#endif // SIOCOUTQ
                        ttsp = (t_tcpserver_send_params *)getbytes(sizeof(t_tcpserver_send_params));
                        if (ttsp == NULL)
                        {
                            error("%s: unable to allocate %zu bytes for t_tcpserver_send_params", objName, sizeof(t_tcpserver_send_params));
                            goto failed;
                        }
                        ttsp->client = client;
                        ttsp->sockfd = sockfd;
                        ttsp->byte_buf = byte_buf;
                        ttsp->length = j;
                        ttsp->verbosity = x->x_verbosity;
                        if ( 0!= (sender_thread_result = pthread_create(&sender_thread, &sender_attr, tcpserver_send_buf_thread, (void *)ttsp)))
                        {
                            ++x->x_blocked;
                            error("%s: couldn't create sender thread (%d)", objName, sender_thread_result);
                            if (NULL != ttsp)
                            {
                                freebytes (ttsp, sizeof (t_tcpserver_send_params));
                                ttsp = NULL;
                            }
                            goto failed;
                        }
                        flen += j;
                        j = 0;
                    }
                }
                flen += j;
                sys_fclose(fptr);
                fptr = NULL;
                if (x->x_verbosity > 0) post("%s: read \"%s\" length %d byte%s", objName, fpath, flen, ((d==1)?"":"s"));
            }
            else
            { /* arg was neither a float nor a valid file name */
                error("%s: item %d is not a float or a file name", objName, i);
                goto failed;
            }
        }
        length = j;
        if (length > 0)
        { /* send whatever remains in our buffer */
#ifdef SIOCOUTQ
            if (tcpserver_send_buffer_avaliable_for_client(x, client) < length)
            {
                error("%s: buffer too small for client(%d)", objName, client);
                goto failed;
            }
#endif // SIOCOUTQ
            ttsp = (t_tcpserver_send_params *)getbytes(sizeof(t_tcpserver_send_params));
            if (ttsp == NULL)
            {
                error("%s: unable to allocate %zu bytes for t_tcpserver_send_params", objName, sizeof(t_tcpserver_send_params));
                goto failed;
            }
            ttsp->client = client;
            ttsp->sockfd = sockfd;
            ttsp->byte_buf = byte_buf;
            ttsp->length = length;
            ttsp->verbosity = x->x_verbosity;
            if ( 0!= (sender_thread_result = pthread_create(&sender_thread, &sender_attr, tcpserver_send_buf_thread, (void *)ttsp)))
            {
                ++x->x_blocked;
                error("%s: couldn't create sender thread (%d)", objName, sender_thread_result);
                if (NULL != ttsp) 
                {
                    freebytes (ttsp, sizeof (t_tcpserver_send_params));
                    ttsp = NULL;
                }
                goto failed;
            }
            flen += length;
        }
    }
    else post("%s: not a valid socket number (%d)", objName, sockfd);
failed:
    if (0!= (sender_thread_result = pthread_attr_destroy(&sender_attr)))
    {
        error("%s_broadcast: pthread_attr_destroy failed: %d", objName, sender_thread_result);
    }

    SETFLOAT(&output_atom[0], client+1);
    SETFLOAT(&output_atom[1], flen);
    SETFLOAT(&output_atom[2], sockfd);
    if(x->x_blocked) outlet_anything( x->x_status_outlet, gensym("blocked"), 3, output_atom);
    else outlet_anything( x->x_status_outlet, gensym("sent"), 3, output_atom);
}

#ifdef SIOCOUTQ
/* SIOCOUTQ exists only(?) on linux, returns remaining space in the socket's output buffer  */
static int tcpserver_send_buffer_avaliable_for_client(t_tcpserver *x, int client)
{
    int sockfd = x->x_sr[client].sr_fd;
    int result = 0L;

    ioctl(sockfd, SIOCOUTQ, &result);
    return result;
}
#endif // SIOCOUTQ

// send a buffer in its own thread
static void *tcpserver_send_buf_thread(void *arg)
{
    t_tcpserver_send_params *ttsp = (t_tcpserver_send_params *)arg;
    int                     result;
    
    result = send(ttsp->sockfd, ttsp->byte_buf, ttsp->length, 0);
    if (result <= 0)
    {
        sys_sockerror("tcpserver: send");
        if (ttsp->verbosity > 0) post("%s_send_buf: could not send data to client %d", objName, ttsp->client+1);
    }
    if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_send_params));
    return NULL;
}

/* send message to client using socket number */
static void tcpserver_send(t_tcpserver *x, t_symbol *s, int argc, t_atom *argv)
{
    int     i, sockfd;
    int     client = -1;

    if(x->x_nconnections <= 0)
    {
        if (x->x_verbosity > 0) post("%s_send: no clients connected", objName);
        return;
    }
    if(argc == 0) /* no socket specified: output state of all sockets */
    {
        tcpserver_output_client_state(x, client);
        return;
    }
    /* get socket number of connection (first element in list) */
    if(argv[0].a_type == A_FLOAT)
    {
        sockfd = atom_getfloatarg(0, argc, argv);
        for(i = 0; i < x->x_nconnections; i++) /* check if connection exists */
        {
            if(x->x_sr[i]->sr_fd == sockfd)
            {
                client = i; /* the client we're sending to */
                break;
            }
        }
        if(client == -1)
        {
            if (x->x_verbosity > 0) post("%s_send: no connection on socket %d", objName, sockfd);
            return;
        }
    }
    else
    {
        if (x->x_verbosity > 0) post("%s_send: no socket specified", objName);
        return;
    }
    if (argc < 2) /* nothing to send: output state of this socket */
    {
        tcpserver_output_client_state(x, client+1);
        return;
    }
    tcpserver_send_bytes(client, x, argc-1, &argv[1]);
}

/* disconnect the client at x_sock_fd */
static void tcpserver_disconnect(t_tcpserver *x)
{
    int i, fd;
    t_tcpserver_socketreceiver *y;

    if (x->x_sock_fd >= 0)
    {
        /* find the socketreceiver for this socket */
        for(i = 0; i < x->x_nconnections; i++)
        {
            if(x->x_sr[i]->sr_fd == x->x_sock_fd)
            {
                y = x->x_sr[i];
                fd = y->sr_fd;
                if (y->sr_notifier) (*y->sr_notifier)(x);
                sys_rmpollfn(fd);
                sys_closesocket(fd);
                x->x_sock_fd = -1;
                return;
            }
        }
    }
    if (x->x_verbosity > 0) post("%s__disconnect: no connection on socket %d", objName, x->x_sock_fd);
}

/* disconnect a client by socket */
static void tcpserver_socket_disconnect(t_tcpserver *x, t_floatarg fsocket)
{
    int sock = (int)fsocket;

    if(x->x_nconnections <= 0)
    {
        if (x->x_verbosity > 0) post("%s_socket_disconnect: no clients connected", objName);
        return;
    }
    x->x_sock_fd = sock;
    tcpserver_disconnect(x);
}

/* disconnect a client by number */
static void tcpserver_client_disconnect(t_tcpserver *x, t_floatarg fclient)
{
    int client = (int)fclient;

    if(x->x_nconnections <= 0)
    {
        if (x->x_verbosity > 0) post("%s_client_disconnect: no clients connected", objName);
        return;
    }
    if (!((client > 0) && (client < MAX_CONNECT)))
    {
        if (x->x_verbosity > 0) post("%s: client %d out of range [1..%d]", objName, client, MAX_CONNECT);
        return;
    }
    --client;/* zero based index*/
    x->x_sock_fd = x->x_sr[client]->sr_fd;
    tcpserver_disconnect(x);
}


/* send message to client using client number
   note that the client numbers might change in case a client disconnects! */
/* clients start at 1 but our index starts at 0 */
static void tcpserver_client_send(t_tcpserver *x, t_symbol *s, int argc, t_atom *argv)
{
    int     client = -1;

    if(x->x_nconnections <= 0)
    {
        if (x->x_verbosity > 0) post("%s_client_send: no clients connected", objName);
        return;
    }
    if(argc > 0)
    {
        /* get number of client (first element in list) */
        if(argv[0].a_type == A_FLOAT)
            client = atom_getfloatarg(0, argc, argv);
        else
        {
            if (x->x_verbosity > 0) post("%s_client_send: specify client by number", objName);
            return;
        }
        if (!((client > 0) && (client < MAX_CONNECT)))
        {
            if (x->x_verbosity > 0) post("%s_client_send: client %d out of range [1..%d]", objName, client, MAX_CONNECT);
            return;
        }
    }
    if (argc > 1)
    {
        --client;/* zero based index*/
        tcpserver_send_bytes(client, x, argc-1, &argv[1]);
        return;
    }
    tcpserver_output_client_state(x, client);
}

static void tcpserver_output_client_state(t_tcpserver *x, int client)
{
    t_atom  output_atom[4];

    if (client == -1)
    {
        /* output parameters of all connections via status outlet */
        for(client = 0; client < x->x_nconnections; client++)
        {
            x->x_sr[client]->sr_fdbuf = tcpserver_get_socket_send_buf_size(x->x_sr[client]->sr_fd);
            SETFLOAT(&output_atom[0], client+1);
            SETFLOAT(&output_atom[1], x->x_sr[client]->sr_fd);
            output_atom[2].a_type = A_SYMBOL;
            output_atom[2].a_w.w_symbol = x->x_sr[client]->sr_host;
            SETFLOAT(&output_atom[3], x->x_sr[client]->sr_fdbuf);
            outlet_anything( x->x_status_outlet, gensym("client"), 4, output_atom);
        }
    }
    else
    {
        client -= 1;/* zero-based client index conflicts with 1-based user index !!! */

        if (client < x->x_nconnections)
        { // only if there is such a client
            /* output client parameters via status outlet */
            x->x_sr[client]->sr_fdbuf = tcpserver_get_socket_send_buf_size(x->x_sr[client]->sr_fd);
            SETFLOAT(&output_atom[0], client+1);/* user sees client 0 as 1 */
            SETFLOAT(&output_atom[1], x->x_sr[client]->sr_fd);
            output_atom[2].a_type = A_SYMBOL;
            output_atom[2].a_w.w_symbol = x->x_sr[client]->sr_host;
            SETFLOAT(&output_atom[3], x->x_sr[client]->sr_fdbuf);
            outlet_anything( x->x_status_outlet, gensym("client"), 4, output_atom);
        }
        else if (x->x_verbosity > 0) post("%s: no such client (%d)", objName, client+1);
    }
}

/* Return the send buffer size of socket */
static int tcpserver_get_socket_send_buf_size(int sockfd)
{
    int                 optVal = 0;
    unsigned int        optLen = sizeof(int);
#ifdef _WIN32
    if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, &optLen) == SOCKET_ERROR)
        post("%_get_socket_send_buf_size: getsockopt returned %d\n", objName, WSAGetLastError());
#else
    if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, &optLen) == -1)
        post("%_get_socket_send_buf_size: getsockopt returned %d\n", objName, errno);
#endif
    return  optVal;
}

/* Set the send buffer size of socket, returns actual size */
static int tcpserver_set_socket_send_buf_size(int sockfd, int size)
{
    int                 optVal = size;
    int                 optLen = sizeof(int);
#ifdef _WIN32
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, optLen) == SOCKET_ERROR)
    {
        post("%s_set_socket_send_buf_size: setsockopt returned %d\n", objName, WSAGetLastError());
#else
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, optLen) == -1)
    {
        post("%s_set_socket_send_buf_size: setsockopt returned %d\n", objName, errno);
#endif
        return 0;
    }
    else return (tcpserver_get_socket_send_buf_size(sockfd));
}

/* Get/set the send buffer of client socket */
static void tcpserver_buf_size(t_tcpserver *x, t_symbol *s, int argc, t_atom *argv)
{
    int     client = -1;
    float   buf_size = 0;
//    t_atom  output_atom[3];

    if(x->x_nconnections <= 0)
    {
        if (x->x_verbosity > 0)  post("%s_buf_size: no clients connected", objName);
        return;
    }
    /* get number of client (first element in list) */
    if (argc > 0)
    {
        if (argv[0].a_type == A_FLOAT)
            client = atom_getfloatarg(0, argc, argv);
        else
        {
            if (x->x_verbosity > 0) post("%s_buf_size: specify client by number", objName);
            return;
        }
        if (!((client > 0) && (client < MAX_CONNECT)))
        {
            if (x->x_verbosity > 0) post("%s__buf_size: client %d out of range [1..%d]", objName, client, MAX_CONNECT);
            return;
        }
    }
    if (argc > 1)
    {
        if (argv[1].a_type != A_FLOAT)
        {
            if (x->x_verbosity > 0) post("%s_buf_size: specify buffer size with a float", objName);
            return;
        }
        buf_size = atom_getfloatarg(1, argc, argv);
        --client;/* zero based index*/
        x->x_sr[client]->sr_fdbuf = tcpserver_set_socket_send_buf_size(x->x_sr[client]->sr_fd, (int)buf_size);
        if (x->x_verbosity > 0) post("%s_buf_size: client %d set to %d", objName, client+1, x->x_sr[client]->sr_fdbuf);
        return;
    }
    if (x->x_verbosity > 0) post("%s_buf_size: specify client and buffer size", objName);
    return;
}

/* broadcasts messages to all clients */
/* Ivica Ico Bukvic <ico@bukvic.net> 5/6/10 rewrote broadcast to be xrun-free */
static void *tcpserver_broadcast_thread(void *arg)
{
    t_tcpserver_broadcast_params    *ttbp = (t_tcpserver_broadcast_params *)arg;
    int                             client;
    static char                     byte_buf[MAX_UDP_RECEIVE];// arbitrary maximum similar to max IP packet size
    int                             i, j, d;
    unsigned char                   c;
    float                           f, e;
    int                             length;
    size_t                          flen = 0;
    int                             sockfd = 0;
    char                            fpath[FILENAME_MAX];
    FILE                            *fptr;
//    t_atom                          output_atom[3];
//    t_tcpserver_send_params         *ttsp;
//    pthread_t                       sender_thread;
//    pthread_attr_t                  sender_attr;
//    int                             sender_thread_result;
    int                             result;

    for (i = j = 0; i < ttbp->argc; ++i)
    {
        if (ttbp->argv[i].a_type == A_FLOAT)
        { /* load floats into buffer as long as they are integers on [0..255]*/
            f = ttbp->argv[i].a_w.w_float;
            d = (int)f;
            e = f - d;
            if (ttbp->x->x_verbosity > 1) post("%s: argv[%d]: float:%f int:%d delta:%f", objName, i, f, d, e);
            if (e != 0)
            {
                error("%s_broadcast_thread: item %d (%f) is not an integer", objName, i, f);
                if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
                return NULL;
            }
            if ((d < 0) || (d > 255))
            {
                error("%s_broadcast_thread: item %d (%f) is not between 0 and 255", objName, i, f);
                if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
                return NULL;
            }
            c = (unsigned char)d; /* make sure it doesn't become negative; this only matters for post() */
            if (ttbp->x->x_verbosity > 1) post("%s_broadcast_thread: argv[%d]: %d", objName, i, c);
            byte_buf[j++] = c;
            if (j >= MAX_UDP_RECEIVE)
            { /* if the argument list is longer than our buffer, send the buffer whenever it's full */
//LOOP
                /* enumerate through the clients and send each the message */
                for(client = 0; client < ttbp->x->x_nconnections; client++)	/* check if connection exists */
                {
#ifdef SIOCOUTQ
                    if (tcpserver_send_buffer_avaliable_for_client(ttbp->x, client) < j)
                    {
                        error("%s_broadcast_thread: buffer too small for client(%d)", objName, client);
                        if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
                        return NULL;
                    }
#endif // SIOCOUTQ
                    if(ttbp->x->x_sr[client]->sr_fd >= 0)
                    { /* socket exists for this client */
                        sockfd = ttbp->x->x_sr[client]->sr_fd;
                        result = send(sockfd, byte_buf, j, 0);
                        if (result <= 0)
                        {
                            sys_sockerror("tcpserver_broadcast_thread: send");
                            if (ttbp->x->x_verbosity > 0) post("%s_broadcast_thread: could not send data to client %d", objName, client+1);
                            if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
                            return NULL;
                        }
                    }
                }
                flen += j;
                j = 0;
            }
        }
        else if (ttbp->argv[i].a_type == A_SYMBOL)
        { /* symbols are interpreted to be file names; attempt to load the file and send it */
            atom_string(&ttbp->argv[i], fpath, FILENAME_MAX);
            if (ttbp->x->x_verbosity > 1) post ("%s_broadcast_thread: fname: %s", objName, fpath);
            fptr = sys_fopen(fpath, "rb");
            if (fptr == NULL)
            {
                error("%s_broadcast_thread: unable to open \"%s\"", objName, fpath);
                if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
                return NULL;
            }
            rewind(fptr);
            while ((d = fgetc(fptr)) != EOF)
            {
                if (ttbp->x->x_verbosity > 1) post("%s_broadcast_thread: d is %d", objName, d);
                byte_buf[j++] = (char)(d & 0x0FF);
                if (ttbp->x->x_verbosity > 1) post("%s_broadcast_thread: byte_buf[%d] = %d", objName, j-1, byte_buf[j-1]);
                if (j >= MAX_UDP_RECEIVE)
                { /* if the file is longer than our buffer, send the buffer whenever it's full */
//LOOP
                    /* enumerate through the clients and send each the message */
                    for(client = 0; client < ttbp->x->x_nconnections; client++)	/* check if connection exists */
                    {
                        /* this might be better than allocating huge amounts of memory */
#ifdef SIOCOUTQ
                        if (tcpserver_send_buffer_avaliable_for_client(x, client) < j)
                        {
                            error("%s_broadcast_thread: buffer too small for client(%d)", objName, client);
                            if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
                            return NULL;
                        }
#endif // SIOCOUTQ
                        if(ttbp->x->x_sr[client]->sr_fd >= 0)
                        { /* socket exists for this client */
                            sockfd = ttbp->x->x_sr[client]->sr_fd;
                            result = send(sockfd, byte_buf, j, 0);
                            if (result <= 0)
                            {
                                sys_sockerror("tcpserver_broadcast_thread: send");
                                if (ttbp->x->x_verbosity > 1) post("%s_broadcast_thread: could not send data to client %d", objName, client+1);
                                if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
                                return NULL;
                            }
                        }
                    }
                    flen += j;
                    j = 0;
                }
            }
            flen += j;
            sys_fclose(fptr);
            fptr = NULL;
            if (ttbp->x->x_verbosity > 1) post("%s_broadcast_thread: read \"%s\" length %d byte%s", objName, fpath, flen, ((d==1)?"":"s"));
        }
        else
        { /* arg was neither a float nor a valid file name */
            error("%s_broadcast_thread: item %d is not a float or a file name", objName, i);
            if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
            return NULL;
        }
    }
    length = j;
    if (length > 0)
    { /* send whatever remains in our buffer */
        /* enumerate through the clients and send each the message */
        for(client = 0; client < ttbp->x->x_nconnections; client++)	/* check if connection exists */
        {
#ifdef SIOCOUTQ
            if (tcpserver_send_buffer_avaliable_for_client(x, client) < length)
            {
                error("%s_broadcast_thread: buffer too small for client(%d)", objName, client);
                if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
                return NULL;
            }
#endif // SIOCOUTQ
            if(ttbp->x->x_sr[client]->sr_fd >= 0)
            { /* socket exists for this client */
                sockfd = ttbp->x->x_sr[client]->sr_fd;
                result = send(sockfd, byte_buf, length, 0);
                if (result <= 0)
                {
                    sys_sockerror("tcpserver_broadcast_thread: send");
                    if (ttbp->x->x_verbosity > 0) post("%s_broadcast_thread: could not send data to client %d", objName, client+1);
                    if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
                    return NULL;
                }
            }
        }
        flen += length;
    }
    else  if (ttbp->x->x_verbosity > 0) post("%s_broadcast_thread: not a valid socket number (%d)", objName, sockfd);
    if (NULL != arg) freebytes (arg, sizeof (t_tcpserver_broadcast_params));
    return NULL;
}

static void tcpserver_broadcast(t_tcpserver *x, t_symbol *s, int argc, t_atom *argv)
/* initializes separate thread that broadcasts to all clients */
/* Ivica Ico Bukvic <ico@bukvic.net> 5/6/10 rewrote broadcast to be xrun-free */
{
    int                             i;
    t_tcpserver_broadcast_params    *ttbp;
    pthread_t                       broadcast_thread;
    pthread_attr_t                  broadcast_attr = {{0}};
    int                             broadcast_thread_result;

    if (x->x_nconnections != 0)
    {
        /* sender thread should start out detached so its resouces will be freed when it is done */
        if (0!= (broadcast_thread_result = pthread_attr_init(&broadcast_attr)))
        {
            error("%s_broadcast: pthread_attr_init failed: %d", objName, broadcast_thread_result);
        }
        else
        {
            if(0 != (broadcast_thread_result = pthread_attr_setdetachstate(&broadcast_attr, PTHREAD_CREATE_DETACHED)))
            {
                error("%s_broadcast: pthread_attr_setdetachstate failed: %d", objName, broadcast_thread_result);
            }
            else
            {
                ttbp = (t_tcpserver_broadcast_params *)getbytes(sizeof(t_tcpserver_broadcast_params));
                if (ttbp == NULL)
                {
                    error("%s_broadcast: unable to allocate %zu bytes for t_tcpserver_broadcast_params", objName, sizeof(t_tcpserver_broadcast_params));
                }
                else
                {
                    ttbp->x = x;
                    ttbp->argc = argc;
                    for (i = 0; i < argc; i++) ttbp->argv[i] = argv[i];
                    if (0 != (broadcast_thread_result = pthread_create(&broadcast_thread, &broadcast_attr, tcpserver_broadcast_thread, (void *)ttbp)))
                    {
                        error("%s_broadcast: couldn't create broadcast thread (%d)", objName, broadcast_thread_result);
                        if (NULL != ttbp)
                        {
                            freebytes (ttbp, sizeof (t_tcpserver_broadcast_params));
                            ttbp = NULL;
                        }
                    }
                }
            }
            if (0!= (broadcast_thread_result = pthread_attr_destroy(&broadcast_attr)))
            {
                error("%s_broadcast: pthread_attr_destroy failed: %d", objName, broadcast_thread_result);
            }
        }
    }
}

/* ---------------- main tcpserver (receive) stuff --------------------- */
/* tcpserver_notify is called when socket has closed */
static void tcpserver_notify(t_tcpserver *x)
{
    int     i, k;

    /* remove connection from list */
    for(i = 0; i < x->x_nconnections; i++)
    {
        if(x->x_sr[i]->sr_fd == x->x_sock_fd)
        {
            x->x_nconnections--;
/* Ivica Ico Bukvic <ico@bukvic.net>:*/
            x->x_addrbytes[0].a_w.w_float = (x->x_sr[i]->sr_addr & 0xFF000000)>>24;
            x->x_addrbytes[1].a_w.w_float = (x->x_sr[i]->sr_addr & 0x0FF0000)>>16;
            x->x_addrbytes[2].a_w.w_float = (x->x_sr[i]->sr_addr & 0x0FF00)>>8;
            x->x_addrbytes[3].a_w.w_float = (x->x_sr[i]->sr_addr & 0x0FF);
            outlet_list(x->x_addrout, &s_list, 4L, x->x_addrbytes);
            outlet_float(x->x_sockout, x->x_sr[i]->sr_fd); /* the socket number */
            outlet_float(x->x_connectout, x->x_nconnections);
/* /Ivica Ico Bukvic */
            if (x->x_verbosity > 0) post("%s: \"%s\" removed from list of clients", objName, x->x_sr[i]->sr_host->s_name);
            tcpserver_socketreceiver_free(x->x_sr[i]);
            x->x_sr[i] = NULL;

            /* rearrange list now: move entries to close the gap */
            for(k = i; k < x->x_nconnections; k++)
            {
                x->x_sr[k] = x->x_sr[k + 1];
            }
        }
    }
// ico    outlet_float(x->x_connectout, x->x_nconnections);
}

static void tcpserver_connectpoll(t_tcpserver *x)
{
    struct sockaddr_in  incomer_address;
    unsigned int        sockaddrl = sizeof( struct sockaddr );
    int                 fd;
    int                 i;
    int                 optVal;
    unsigned int        optLen = sizeof(int);

    if (MAX_CONNECT <= x->x_nconnections)
    { /* no more free space for t_tcpserver_socketreceiver pointers, maybe increase MAX_CONNECT? */
        if (x->x_verbosity > 0) post("%s: too many connections (MAX_CONNECT is %d)", objName, x->x_nconnections);
        return;
    }
    fd = accept(x->x_connectsocket, (struct sockaddr*)&incomer_address, &sockaddrl);
    if ((fd < 0)&&(x->x_verbosity > 1)) post("%s: accept failed", objName);
    else
    {
        t_tcpserver_socketreceiver *y = tcpserver_socketreceiver_new((void *)x,
            (t_tcpserver_socketnotifier)tcpserver_notify, NULL);/* MP tcpserver_doit isn't used I think...*/
        if (!y)
        {
#ifdef _WIN32
            closesocket(fd);
#else
            close(fd);
#endif
            return;
        }
        sys_addpollfn(fd, (t_fdpollfn)tcpserver_socketreceiver_read, y);
        x->x_nconnections++;
        i = x->x_nconnections - 1;
        x->x_sr[i] = y;
        x->x_sr[i]->sr_host = gensym(inet_ntoa(incomer_address.sin_addr));
        x->x_sr[i]->sr_fd = fd;
        if (x->x_verbosity > 0) post("%s: accepted connection from %s on socket %d (client %d)",
            objName, x->x_sr[i]->sr_host->s_name, x->x_sr[i]->sr_fd, i+1);
/* see how big the send buffer is on this socket */
        x->x_sr[i]->sr_fdbuf = 0;
#ifdef _WIN32
        if (getsockopt(x->x_sr[i]->sr_fd, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, &optLen) != SOCKET_ERROR)
        {
            /* post("%s_connectpoll: send buffer is %ld\n", objName, optVal); */
            x->x_sr[i]->sr_fdbuf = optVal;
        }
        else if (x->x_verbosity > 1) post("%s_connectpoll: getsockopt returned %d\n", objName, WSAGetLastError());
#else
        if (getsockopt(x->x_sr[i]->sr_fd, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, &optLen) == 0)
        {
            /* post("%s_connectpoll: send buffer is %ld\n", objName, optVal); */
            x->x_sr[i]->sr_fdbuf = optVal;
        }
        else if (x->x_verbosity > 1) post("%s_connectpoll: getsockopt returned %d\n", objName, errno);
#endif
        x->x_sr[i]->sr_addr = ntohl(incomer_address.sin_addr.s_addr);
        x->x_addrbytes[0].a_w.w_float = (x->x_sr[i]->sr_addr & 0xFF000000)>>24;
        x->x_addrbytes[1].a_w.w_float = (x->x_sr[i]->sr_addr & 0x0FF0000)>>16;
        x->x_addrbytes[2].a_w.w_float = (x->x_sr[i]->sr_addr & 0x0FF00)>>8;
        x->x_addrbytes[3].a_w.w_float = (x->x_sr[i]->sr_addr & 0x0FF);
        outlet_list(x->x_addrout, &s_list, 4L, x->x_addrbytes);
        outlet_float(x->x_sockout, x->x_sr[i]->sr_fd);	/* the socket number */
        outlet_float(x->x_connectout, x->x_nconnections);
    }
}

static void tcpserver_print(t_tcpserver *x)
{
    int     i;

    if(x->x_nconnections > 0)
    {
        post("%s: %d open connections:", objName, x->x_nconnections);
        for(i = 0; i < x->x_nconnections; i++)
        {
            post("        \"%s\" on socket %d",
                x->x_sr[i]->sr_host->s_name, x->x_sr[i]->sr_fd);
        }
    }
    else post("%s: no open connections", objName);
}

static void tcpserver_unblock(t_tcpserver *x)
{
    x->x_blocked = 0;
}

static void tcpserver_port(t_tcpserver*x, t_floatarg fportno)
{
    t_atom              output_atom;
    int                 portno = fportno;
    struct sockaddr_in  server;
    socklen_t           serversize = sizeof(server);
    int                 sockfd = x->x_connectsocket;
    int                 optVal = 1;
    int                 optLen = sizeof(int);


    if(x->x_port == portno)  return;

    if(x->x_nconnections > 0)
    {
        if (x->x_verbosity > 0) post ("%s: can't change port (connection count is %d)", objName, x->x_nconnections);
        return;
    }

    SETFLOAT(&output_atom, -1);

    /* shut down the old socket */
    if (sockfd >= 0)
    {
        sys_rmpollfn(sockfd);
        sys_closesocket(sockfd);
        x->x_connectsocket = -1;
        x->x_port = -1;
    }

    /* create a whole new socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (x->x_verbosity > 1) post("%s: receive socket %d", objName, sockfd);
    if (sockfd < 0)
    {
        sys_sockerror("tcpserver: socket");
        return;
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    /* enable reuse of local address */
#ifdef _WIN32
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&optVal, optLen) == SOCKET_ERROR)
#else
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&optVal, optLen) == -1)
#endif
        sys_sockerror("tcpserver: setsockopt SO_REUSEADDR");

    /* assign server port number */
    server.sin_port = htons((u_short)portno);
    /* name the socket */
    if (bind(sockfd, (struct sockaddr *)&server, serversize) < 0)
    {
        sys_sockerror("tcpserver: bind");
        sys_closesocket(sockfd);
        outlet_anything(x->x_status_outlet, gensym("port"), 1, &output_atom);
        return;
    }

    /* streaming protocol */
    if (listen(sockfd, 5) < 0)
    {
        sys_sockerror("tcpserver: listen");
        sys_closesocket(sockfd);
        sockfd = -1;
        outlet_anything(x->x_status_outlet, gensym("port"), 1, &output_atom);
        return;
    }
    else
    {
        sys_addpollfn(sockfd, (t_fdpollfn)tcpserver_connectpoll, x); // wait for new connections 
    }

    x->x_connectsocket = sockfd;
    x->x_nconnections = 0;
    x->x_blocked = 0;


    // find out which port is actually used (useful when assigning "0")
    if(!getsockname(sockfd, (struct sockaddr *)&server, &serversize))
    {
        x->x_port=ntohs(server.sin_port);
    }

    SETFLOAT(&output_atom, x->x_port);
    outlet_anything(x->x_status_outlet, gensym("port"), 1, &output_atom);
}

static void *tcpserver_new(t_floatarg fportno)
{
    t_tcpserver         *x;
    int                 i;
    struct sockaddr_in  server;
    int                 sockfd, portno = fportno;
    int                 optVal = 1;
    int                 optLen = sizeof(int);

    /* create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        sys_sockerror("tcpserver: socket");
        return (0);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    /* enable reuse of local address */
#ifdef _WIN32
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&optVal, optLen) == SOCKET_ERROR)
#else
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&optVal, optLen) == -1)
#endif
        sys_sockerror("tcpserver: setsockopt SO_REUSEADDR");

    /* assign server port number */
    server.sin_port = htons((u_short)portno);
    /* name the socket */
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        sys_sockerror("tcpserver: bind");
        sys_closesocket(sockfd);
        return (0);
    }
    x = (t_tcpserver *)pd_new(tcpserver_class);
    x->x_msgout = outlet_new(&x->x_obj, &s_anything); /* 1st outlet for received data */
   /* streaming protocol */
    if (listen(sockfd, 5) < 0)
    {
        sys_sockerror("tcpserver: listen");
        sys_closesocket(sockfd);
        sockfd = -1;
    }
    else
    {
        sys_addpollfn(sockfd, (t_fdpollfn)tcpserver_connectpoll, x);
        x->x_connectout = outlet_new(&x->x_obj, &s_float); /* 2nd outlet for number of connected clients */
        x->x_sockout = outlet_new(&x->x_obj, &s_float); /* 3rd outlet for socket number of current client */
        x->x_addrout = outlet_new(&x->x_obj, &s_list); /* 4th outlet for ip address of current client */
        x->x_status_outlet = outlet_new(&x->x_obj, &s_anything);/* 5th outlet for everything else */
    }
    x->x_connectsocket = sockfd;
    x->x_nconnections = 0;
    x->x_blocked = 0;
    for(i = 0; i < MAX_CONNECT; i++)
    {
        x->x_sr[i] = NULL;
    }
    /* prepare to convert the bytes in the buffer to floats in a list */
    for (i = 0; i < MAX_UDP_RECEIVE; ++i)
    {
        x->x_msgoutbuf[i].a_type = A_FLOAT;
        x->x_msgoutbuf[i].a_w.w_float = 0;
    }
    for (i = 0; i < 4; ++i)
    {
        x->x_addrbytes[i].a_type = A_FLOAT;
        x->x_addrbytes[i].a_w.w_float = 0;
    }
    x->x_port = portno;
    post ("tcpserver listening on port %d, socket %d", x->x_port, sockfd);
    x->x_verbosity = 0; // stop talking to console unless we're asked
    return (x);
}


static void tcpserver_free(t_tcpserver *x)
{
    int     i;

    if (x->x_verbosity > 1)  post("tcp_server_free...");
    for(i = 0; i < MAX_CONNECT; i++)
    {
        if (x->x_sr[i] != NULL) 
        {
            if (x->x_verbosity > 1)  post("tcp_server_free...freeing client %d", i+1);
            if (x->x_sr[i]->sr_fd >= 0)
            {
                if (x->x_verbosity > 1)  post("tcp_server_free...freeing fd %d", x->x_sr[i]->sr_fd);
                sys_rmpollfn(x->x_sr[i]->sr_fd); // sometimes is not found as poll function was already removed
                sys_closesocket(x->x_sr[i]->sr_fd);
            }
            tcpserver_socketreceiver_free(x->x_sr[i]);
            x->x_sr[i] = NULL;
        }
    }
    if (x->x_connectsocket >= 0)
    {
        if (x->x_verbosity > 1)  post("tcp_server_free...freeing connectsocket %d", x->x_connectsocket);
        sys_rmpollfn(x->x_connectsocket);
        sys_closesocket(x->x_connectsocket);
    }
    if (x->x_verbosity > 1)  post("tcp_server_free end");
}

void tcpserver_setup(void)
{
    tcpserver_class = class_new(gensym(objName),(t_newmethod)tcpserver_new, (t_method)tcpserver_free,
        sizeof(t_tcpserver), 0, A_DEFFLOAT, 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_print, gensym("print"), 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_send, gensym("send"), A_GIMME, 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_client_send, gensym("client"), A_GIMME, 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_buf_size, gensym("clientbuf"), A_GIMME, 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_client_disconnect, gensym("disconnectclient"), A_DEFFLOAT, 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_socket_disconnect, gensym("disconnectsocket"), A_DEFFLOAT, 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_dump, gensym("dump"), A_FLOAT, 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_unblock, gensym("unblock"), 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_broadcast, gensym("broadcast"), A_GIMME, 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_port, gensym("port"), A_FLOAT, 0);
    class_addmethod(tcpserver_class, (t_method)tcpserver_verbosity, gensym("verbosity"), A_DEFFLOAT, 0);
    class_addlist(tcpserver_class, (t_method)tcpserver_send);
}

/* end of tcpserver.c */
