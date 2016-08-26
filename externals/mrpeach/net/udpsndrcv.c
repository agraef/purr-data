/* udpsndrcv.c 20140221.  Dennis Engdahl did it based on: 20060424. Martin Peach did it based on x_net.c. x_net.c header follows: */
/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* network */

#include "m_pd.h"
#include "s_stuff.h"

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h> // for SIOCGIFCONF
#include <net/if.h> // for SIOCGIFCONF
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#endif // _WIN32

/* support older Pd versions without sys_open(), sys_fopen(), sys_fclose() */
#if PD_MAJOR_VERSION == 0 && PD_MINOR_VERSION < 44
#define sys_open open
#define sys_fopen fopen
#define sys_fclose fclose
#endif

static t_class *udpsndrcv_class;

#define MAX_UDP_RECEIVE 65536L // longer than data in maximum UDP packet

typedef struct _udpsndrcv
{
    t_object        x_obj;
    int             x_fd; /* the socket */
    t_outlet        *x_msgout;
    t_outlet        *x_addrout;
    long            x_total_received;
    t_atom          x_addrbytes[5];
    t_atom          x_msgoutbuf[MAX_UDP_RECEIVE];
    char            x_msginbuf[MAX_UDP_RECEIVE];
    char            x_addr_name[256]; // a multicast address or 0
} t_udpsndrcv;

void udpsndrcv_setup(void);
static void udpsndrcv_free(t_udpsndrcv *x);
static void udpsndrcv_send(t_udpsndrcv *x, t_symbol *s, int argc, t_atom *argv);
static void udpsndrcv_disconnect(t_udpsndrcv *x);
static void udpsndrcv_connect(t_udpsndrcv *x, t_symbol *hostname, t_floatarg fportno, t_floatarg ffromport);
static void udpsndrcv_sock_err(t_udpsndrcv *x, char *err_string);
static void *udpsndrcv_new(void);
static void udpsndrcv_sock_err(t_udpsndrcv *x, char *err_string);
static void udpsndrcv_status(t_udpsndrcv *x);
static void udpsndrcv_read(t_udpsndrcv *x, int sockfd);

static void *udpsndrcv_new(void)
{
    t_udpsndrcv        *x;
    int                 i;

    x = (t_udpsndrcv *)pd_new(udpsndrcv_class); /* if something fails we return 0 instead of x. Is this OK? */
    if (NULL == x) return x;
    x->x_addr_name[0] = '\0';
    /* convert the bytes in the buffer to floats in a list */
    for (i = 0; i < MAX_UDP_RECEIVE; ++i)
    {
        x->x_msgoutbuf[i].a_type = A_FLOAT;
        x->x_msgoutbuf[i].a_w.w_float = 0;
    }
    for (i = 0; i < 5; ++i)
    {
        x->x_addrbytes[i].a_type = A_FLOAT;
        x->x_addrbytes[i].a_w.w_float = 0;
    }

    outlet_new(&x->x_obj, &s_float);
    x->x_msgout = outlet_new(&x->x_obj, &s_anything);
    x->x_addrout = outlet_new(&x->x_obj, &s_anything);

    x->x_fd = -1;
    return (x);
}

static void udpsndrcv_read(t_udpsndrcv *x, int sockfd)
{
    int                 i, read = 0;
    struct sockaddr_in  from;
    socklen_t           fromlen = sizeof(from);
    t_atom              output_atom;
    long                addr;
    unsigned short      port;

    read = recvfrom(sockfd, x->x_msginbuf, MAX_UDP_RECEIVE, 0, (struct sockaddr *)&from, &fromlen);
#ifdef DEBUG
    post("udpsndrcv_read: read %lu x->x_fd = %d",
        read, x->x_fd);
#endif
    /* get the sender's ip */
    addr = ntohl(from.sin_addr.s_addr);
    port = ntohs(from.sin_port);

    x->x_addrbytes[0].a_w.w_float = (addr & 0xFF000000)>>24;
    x->x_addrbytes[1].a_w.w_float = (addr & 0x0FF0000)>>16;
    x->x_addrbytes[2].a_w.w_float = (addr & 0x0FF00)>>8;
    x->x_addrbytes[3].a_w.w_float = (addr & 0x0FF);
    x->x_addrbytes[4].a_w.w_float = port;
    outlet_anything(x->x_addrout, gensym("from"), 5L, x->x_addrbytes);

    if (read < 0)
    {
        udpsndrcv_sock_err(x, "udpsndrcv_read");
        sys_closesocket(x->x_fd);
        return;
    }
    if (read > 0)
    {
        for (i = 0; i < read; ++i)
        {
            /* convert the bytes in the buffer to floats in a list */
            x->x_msgoutbuf[i].a_w.w_float = (float)(unsigned char)x->x_msginbuf[i];
        }
        x->x_total_received += read;
        SETFLOAT(&output_atom, read);
        outlet_anything(x->x_addrout, gensym("received"), 1, &output_atom);
        /* send the list out the outlet */
        if (read > 1) outlet_list(x->x_msgout, &s_list, read, x->x_msgoutbuf);
        else outlet_float(x->x_msgout, x->x_msgoutbuf[0].a_w.w_float);
    }
}

static void udpsndrcv_connect(t_udpsndrcv *x, t_symbol *hostname, t_floatarg fportno, t_floatarg ffromport)
{
    struct sockaddr_in  server;
    struct hostent      *hp;
    int                 sockfd;
    int                 portno = fportno;
    int                 fromport = ffromport;

    if (x->x_fd >= 0)
    {
        pd_error(x, "udpsndrcv: already connected");
        return;
    }

    /* create a socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef DEBUG
    post("udpsndrcv_connect: socket %d port %d fromport %d", sockfd, portno, fromport);
#endif
    if (sockfd < 0)
    {
        udpsndrcv_sock_err(x, "udpsndrcv socket");
        return;
    }

    /* assign client port number */
    if (fromport == 0) fromport = portno;
    server.sin_family = AF_INET;
    server.sin_port = htons((u_short)fromport);

    /* bind the socket to INADDR_ANY and port as specified */
    server.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr *) &server, sizeof (server)) < 0)
    {
        udpsndrcv_sock_err(x, "udpsndrcv: bind");
        sys_closesocket(sockfd);
        return;
    }

    /* connect socket using hostname provided in command line */
    hp = gethostbyname(hostname->s_name);
    if (hp == 0)
    {
        post("udpsndrcv: bad host?\n");
        return;
    }
    memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);
    server.sin_port = htons((u_short)portno);

    post("udpsndrcv: connecting to port %d from %d", portno, fromport);
    /* try to connect. */
    if (connect(sockfd, (struct sockaddr *) &server, sizeof (server)) < 0)
    {
        udpsndrcv_sock_err(x, "udpsndrcv connect");
#ifdef _WIN32
        closesocket(sockfd);
#else
        close(sockfd);
#endif
        return;
    }
    x->x_fd = sockfd;
    x->x_total_received = 0L;
    sys_addpollfn(x->x_fd, (t_fdpollfn)udpsndrcv_read, x);
    outlet_float(x->x_obj.ob_outlet, 1);
    return;
}

static void udpsndrcv_sock_err(t_udpsndrcv *x, char *err_string)
{
/* prints the last error from errno or WSAGetLastError() */
#ifdef _WIN32
    void            *lpMsgBuf;
    unsigned long   errornumber = WSAGetLastError();
    int             len = 0, i;
    char            *cp;

    if ((len = FormatMessageA((FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS),
        NULL, errornumber, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL)))
    {
        cp = (char *)lpMsgBuf;
        for(i = 0; i < len; ++i)
        {
            if (cp[i] < 0x20)
            { /* end string at first weird character */
                cp[i] = 0;
                break;
            }
        }
        pd_error(x, "%s: %s (%ld)", err_string, (char *)lpMsgBuf, errornumber);
        LocalFree(lpMsgBuf);
    }
#else
    pd_error(x, "%s: %s (%d)", err_string, strerror(errno), errno);
#endif
}

static void udpsndrcv_status(t_udpsndrcv *x)
{
    t_atom output_atom;

    SETFLOAT(&output_atom, x->x_total_received);
    outlet_anything(x->x_addrout, gensym("total"), 1, &output_atom);
}

static void udpsndrcv_disconnect(t_udpsndrcv *x)
{
    if (x->x_fd >= 0)
    {
        post("udpsndrcv: disconnecting.");
#ifdef _WIN32
        closesocket(x->x_fd);
#else
        close(x->x_fd);
#endif
        sys_rmpollfn(x->x_fd);
        x->x_fd = -1;
        outlet_float(x->x_obj.ob_outlet, 0);
    }
}

static void udpsndrcv_send(t_udpsndrcv *x, t_symbol *s, int argc, t_atom *argv)
{
#define BYTE_BUF_LEN 65536 // arbitrary maximum similar to max IP packet size
    static char    byte_buf[BYTE_BUF_LEN];
    int            d;
    int            i, j;
    unsigned char  c;
    float          f, e;
    char           *bp;
    int            length, sent;
    int            result;
    static double  lastwarntime;
    static double  pleasewarn;
    double         timebefore;
    double         timeafter;
    int            late;
    char           fpath[FILENAME_MAX];
    FILE           *fptr;

#ifdef DEBUG
    post("s: %s", s->s_name);
    post("argc: %d", argc);
#endif
    for (i = j = 0; i < argc; ++i)
    {
        if (argv[i].a_type == A_FLOAT)
        {
            f = argv[i].a_w.w_float;
            d = (int)f;
            e = f - d;
            if (e != 0)
            {
                pd_error(x, "udpsndrcv_send: item %d (%f) is not an integer", i, f);
                return;
            }
            c = (unsigned char)d;
            if (c != d)
            {
                pd_error(x, "udpsndrcv_send: item %d (%f) is not between 0 and 255", i, f);
                return;
            }
#ifdef DEBUG
            post("udpsndrcv_send: argv[%d]: %d", i, c);
#endif
            byte_buf[j++] = c;
        }
        else if (argv[i].a_type == A_SYMBOL)
        {

            atom_string(&argv[i], fpath, FILENAME_MAX);
#ifdef DEBUG
            post ("udpsndrcv fname: %s", fpath);
#endif
            fptr = sys_fopen(fpath, "rb");
            if (fptr == NULL)
            {
                post("udpsndrcv: unable to open \"%s\"", fpath);
                return;
            }
            rewind(fptr);
            while ((d = fgetc(fptr)) != EOF)
            {
#ifdef DEBUG
                post("udpsndrcv: d is %d", d);
#endif
                byte_buf[j++] = (char)(d & 0x0FF);
#ifdef DEBUG
                post("udpsndrcv: byte_buf[%d] = %d", j-1, byte_buf[j-1]);
#endif
                if (j >= BYTE_BUF_LEN)
                {
                    post ("udpsndrcv: file too long, truncating at %lu", BYTE_BUF_LEN);
                    break;
                }
            }
            fclose(fptr);
            fptr = NULL;
            post("udpsndrcv: read \"%s\" length %d byte%s", fpath, j, ((d==1)?"":"s"));
        }
        else
        {
            pd_error(x, "udpsndrcv_send: item %d is not a float or a file name", i);
            return;
        }
    }

    length = j;
    if ((x->x_fd >= 0) && (length > 0))
    {
        for (bp = byte_buf, sent = 0; sent < length;)
        {
            timebefore = sys_getrealtime();
            result = send(x->x_fd, byte_buf, length-sent, 0);
            timeafter = sys_getrealtime();
            late = (timeafter - timebefore > 0.005);
            if (late || pleasewarn)
            {
                if (timeafter > lastwarntime + 2)
                {
                    post("udpsndrcv blocked %d msec",
                        (int)(1000 * ((timeafter - timebefore) + pleasewarn)));
                    pleasewarn = 0;
                    lastwarntime = timeafter;
                }
                else if (late) pleasewarn += timeafter - timebefore;
            }
            if (result <= 0)
            {
                udpsndrcv_sock_err(x, "udpsndrcv send");
                udpsndrcv_disconnect(x);
                break;
            }
            else
            {
                sent += result;
                bp += result;
            }
        }
    }
    else pd_error(x, "udpsndrcv: not connected");
}

static void udpsndrcv_free(t_udpsndrcv *x)
{
    udpsndrcv_disconnect(x);
}

void udpsndrcv_setup(void)
{
    udpsndrcv_class = class_new(gensym("udpsndrcv"), (t_newmethod)udpsndrcv_new, (t_method)udpsndrcv_free, sizeof(t_udpsndrcv), 0, 0);
    class_addmethod(udpsndrcv_class, (t_method)udpsndrcv_connect, gensym("connect"), A_SYMBOL, A_FLOAT, A_FLOAT, 0);
    class_addmethod(udpsndrcv_class, (t_method)udpsndrcv_disconnect, gensym("disconnect"), 0);
    class_addmethod(udpsndrcv_class, (t_method)udpsndrcv_send, gensym("send"), A_GIMME, 0);
    class_addmethod(udpsndrcv_class, (t_method)udpsndrcv_status,
        gensym("status"), 0);
    class_addlist(udpsndrcv_class, (t_method)udpsndrcv_send);
}

/* end udpsndrcv.c*/
