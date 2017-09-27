/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* Pd side of the Pd/Pd-gui interface.  Also, some system interface routines
that didn't really belong anywhere. */

#include "config.h"

#include "m_pd.h"
#include "s_stuff.h"
#include "m_imp.h"
#include "g_canvas.h"   /* for GUI queueing stuff */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else // if isatty exists outside unistd, please add another #ifdef
static int isatty(int fd) {return 0;}
#endif

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#endif

#ifdef HAVE_BSTRING_H
#include <bstring.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif 

#ifdef _WIN32
#include <fcntl.h>
#include <process.h>
#include <winsock.h>
#include <windows.h>
# ifdef _MSC_VER
typedef int pid_t;
# endif
typedef int socklen_t;
#define EADDRINUSE WSAEADDRINUSE
#endif

#include <stdarg.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <glob.h>
#else
#include <stdlib.h>
#endif

#define DEBUG_MESSUP 1      /* messages up from pd to pd-gui */
#define DEBUG_MESSDOWN 2    /* messages down from pd-gui to pd */

#ifndef PDBINDIR
#define PDBINDIR "bin/"
#endif

#ifndef WISHAPP
#define WISHAPP "wish85.exe"
#endif

#ifdef __linux__
#define LOCALHOST "127.0.0.1"
#else
#define LOCALHOST "localhost"
#endif

#define X_SPECIFIER "x%.6lx"

static int stderr_isatty;

/* I don't see any other systems where this header (and backtrace) are
   available. */
#ifdef __linux__
#include <execinfo.h>
#endif

typedef struct _fdpoll
{
    int fdp_fd;
    t_fdpollfn fdp_fn;
    void *fdp_ptr;
} t_fdpoll;

#define INBUFSIZE 4096

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

extern char *pd_version;
extern int sys_guisetportnumber;

static int sys_nfdpoll;
static t_fdpoll *sys_fdpoll;
static int sys_maxfd;
static int sys_guisock;

static t_binbuf *inbinbuf;
static t_socketreceiver *sys_socketreceiver;
extern int sys_addhist(int phase);

/* ----------- functions for timing, signals, priorities, etc  --------- */

#ifdef MSW
static LARGE_INTEGER nt_inittime;
static double nt_freq = 0;

static void sys_initntclock(void)
{
    LARGE_INTEGER f1;
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (!QueryPerformanceFrequency(&f1))
    {
          fprintf(stderr, "pd: QueryPerformanceFrequency failed\n");
          f1.QuadPart = 1;
    }
    nt_freq = f1.QuadPart;
    nt_inittime = now;
}

#if 0
    /* this is a version you can call if you did the QueryPerformanceCounter
    call yourself.  Necessary for time tagging incoming MIDI at interrupt
    level, for instance; but we're not doing that just now. */

double nt_tixtotime(LARGE_INTEGER *dumbass)
{
    if (nt_freq == 0) sys_initntclock();
    return (((double)(dumbass->QuadPart - nt_inittime.QuadPart)) / nt_freq);
}
#endif
#endif /* MSW */

    /* get "real time" in seconds; take the
    first time we get called as a reference time of zero. */
double sys_getrealtime(void)    
{
#ifndef MSW
    static struct timeval then;
    struct timeval now;
    gettimeofday(&now, 0);
    if (then.tv_sec == 0 && then.tv_usec == 0) then = now;
    return ((now.tv_sec - then.tv_sec) +
        (1./1000000.) * (now.tv_usec - then.tv_usec));
#else
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (nt_freq == 0) sys_initntclock();
    return (((double)(now.QuadPart - nt_inittime.QuadPart)) / nt_freq);
#endif
}

extern int sys_nosleep;

static int sys_domicrosleep(int microsec, int pollem)
{
    struct timeval timout;
    int i, didsomething = 0;
    t_fdpoll *fp;
    timout.tv_sec = 0;
    timout.tv_usec = (sys_nosleep ? 0 : microsec);
    if (pollem)
    {
        fd_set readset, writeset, exceptset;
        FD_ZERO(&writeset);
        FD_ZERO(&readset);
        FD_ZERO(&exceptset);
        for (fp = sys_fdpoll, i = sys_nfdpoll; i--; fp++)
            FD_SET(fp->fdp_fd, &readset);
#ifdef MSW
        if (sys_maxfd == 0)
                Sleep(microsec/1000);
        else
#endif
        select(sys_maxfd+1, &readset, &writeset, &exceptset, &timout);
        for (i = 0; i < sys_nfdpoll; i++)
            if (FD_ISSET(sys_fdpoll[i].fdp_fd, &readset))
        {
#ifdef THREAD_LOCKING
            sys_lock();
#endif
            (*sys_fdpoll[i].fdp_fn)(sys_fdpoll[i].fdp_ptr, sys_fdpoll[i].fdp_fd);
#ifdef THREAD_LOCKING
            sys_unlock();
#endif
            didsomething = 1;
        }
        return (didsomething);
    }
    else
    {
#ifdef MSW
        if (sys_maxfd == 0)
              Sleep(microsec/1000);
        else
#endif
        select(0, 0, 0, 0, &timout);
        return (0);
    }
}

void sys_microsleep(int microsec)
{
    sys_domicrosleep(microsec, 1);
}

#ifdef HAVE_UNISTD_H
typedef void (*sighandler_t)(int);

static void sys_signal(int signo, sighandler_t sigfun)
{
    struct sigaction action;
    action.sa_flags = 0;
    action.sa_handler = sigfun;
    memset(&action.sa_mask, 0, sizeof(action.sa_mask));
#if 0  /* GG says: don't use that */
    action.sa_restorer = 0;
#endif
    if (sigaction(signo, &action, 0) < 0)
        perror("sigaction");
}

static void sys_exithandler(int n)
{
    static int trouble = 0;
    if (!trouble)
    {
        trouble = 1;
        fprintf(stderr, "Pd: signal %d\n", n);
        sys_bail(1);
    }
    else _exit(1);
}

static void sys_alarmhandler(int n)
{
    fprintf(stderr, "Pd: system call timed out\n");
}

static void sys_huphandler(int n)
{
    struct timeval timout;
    timout.tv_sec = 0;
    timout.tv_usec = 30000;
    select(1, 0, 0, 0, &timout);
}

void sys_setalarm(int microsec)
{
    struct itimerval gonzo;
    int sec = (int)(microsec/1000000);
    microsec %= 1000000;
#if 0
    fprintf(stderr, "timer %d:%d\n", sec, microsec);
#endif
    gonzo.it_interval.tv_sec = 0;
    gonzo.it_interval.tv_usec = 0;
    gonzo.it_value.tv_sec = sec;
    gonzo.it_value.tv_usec = microsec;
    if (microsec)
        sys_signal(SIGALRM, sys_alarmhandler);
    else sys_signal(SIGALRM, SIG_IGN);
    setitimer(ITIMER_REAL, &gonzo, 0);
}

#endif

#ifdef __linux

#if defined(_POSIX_PRIORITY_SCHEDULING) || defined(_POSIX_MEMLOCK)
#include <sched.h>
#endif

void sys_set_priority(int higher) 
{
#ifdef _POSIX_PRIORITY_SCHEDULING
    struct sched_param par;
    int p1 ,p2, p3;
    p1 = sched_get_priority_min(SCHED_FIFO);
    p2 = sched_get_priority_max(SCHED_FIFO);
#ifdef USEAPI_JACK    
    p3 = (higher ? p1 + 7 : p1 + 5);
#else
    p3 = (higher ? p2 - 1 : p2 - 3);
#endif
    par.sched_priority = p3;
    if (sched_setscheduler(0,SCHED_FIFO,&par) != -1)
       fprintf(stderr, "priority %d scheduling enabled.\n", p3);
#endif

#ifdef REALLY_POSIX_MEMLOCK /* this doesn't work on Fedora 4, for example. */
#ifdef _POSIX_MEMLOCK
    /* tb: force memlock to physical memory { */
    {
        struct rlimit mlock_limit;
        mlock_limit.rlim_cur=0;
        mlock_limit.rlim_max=0;
        setrlimit(RLIMIT_MEMLOCK,&mlock_limit);
    }
    /* } tb */
    if (mlockall(MCL_FUTURE) != -1) 
        fprintf(stderr, "memory locking enabled.\n");
#endif
#endif
}

#endif /* __linux__ */

#ifdef IRIX             /* hack by <olaf.matthes@gmx.de> at 2003/09/21 */

#if defined(_POSIX_PRIORITY_SCHEDULING) || defined(_POSIX_MEMLOCK)
#include <sched.h>
#endif

void sys_set_priority(int higher)
{
#ifdef _POSIX_PRIORITY_SCHEDULING
    struct sched_param par;
        /* Bearing the table found in 'man realtime' in mind, I found it a */
        /* good idea to use 192 as the priority setting for Pd. Any thoughts? */
    if (higher)
                par.sched_priority = 250;       /* priority for watchdog */
    else
                par.sched_priority = 192;       /* priority for pd (DSP) */

    if (sched_setscheduler(0, SCHED_FIFO, &par) != -1)
        fprintf(stderr, "priority %d scheduling enabled.\n", par.sched_priority);
#endif

#ifdef _POSIX_MEMLOCK
    if (mlockall(MCL_FUTURE) != -1) 
        fprintf(stderr, "memory locking enabled.\n");
#endif
}
/* end of hack */
#endif /* IRIX */

/* ------------------ receiving incoming messages over sockets ------------- */

void sys_sockerror(char *s)
{
#ifdef MSW
    int err = WSAGetLastError();
    if (err == 10054) return;
    else if (err == 10044)
    {
        fprintf(stderr,
            "Warning: you might not have TCP/IP \"networking\" turned on\n");
        fprintf(stderr, "which is needed for Pd to talk to its GUI layer.\n");
    }
#else
    int err = errno;
#endif
    fprintf(stderr, "%s: %s (%d)\n", s, strerror(err), err);
}

void sys_addpollfn(int fd, t_fdpollfn fn, void *ptr)
{
    int nfd = sys_nfdpoll;
    int size = nfd * sizeof(t_fdpoll);
    t_fdpoll *fp;
    sys_fdpoll = (t_fdpoll *)t_resizebytes(sys_fdpoll, size,
        size + sizeof(t_fdpoll));
    fp = sys_fdpoll + nfd;
    fp->fdp_fd = fd;
    fp->fdp_fn = fn;
    fp->fdp_ptr = ptr;
    sys_nfdpoll = nfd + 1;
    if (fd >= sys_maxfd) sys_maxfd = fd + 1;
}

void sys_rmpollfn(int fd)
{
    int nfd = sys_nfdpoll;
    int i, size = nfd * sizeof(t_fdpoll);
    t_fdpoll *fp;
    for (i = nfd, fp = sys_fdpoll; i--; fp++)
    {
        if (fp->fdp_fd == fd)
        {
            while (i--)
            {
                fp[0] = fp[1];
                fp++;
            }
            sys_fdpoll = (t_fdpoll *)t_resizebytes(sys_fdpoll, size,
                size - sizeof(t_fdpoll));
            sys_nfdpoll = nfd - 1;
            return;
        }
    }
    post("warning: %d removed from poll list but not found", fd);
}

t_socketreceiver *socketreceiver_new(void *owner, t_socketnotifier notifier,
    t_socketreceivefn socketreceivefn, int udp)
{
    t_socketreceiver *x = (t_socketreceiver *)getbytes(sizeof(*x));
    x->sr_inhead = x->sr_intail = 0;
    x->sr_owner = owner;
    x->sr_notifier = notifier;
    x->sr_socketreceivefn = socketreceivefn;
    x->sr_udp = udp;
    if (!(x->sr_inbuf = malloc(INBUFSIZE))) bug("t_socketreceiver");;
    return (x);
}

void socketreceiver_free(t_socketreceiver *x)
{
    free(x->sr_inbuf);
    freebytes(x, sizeof(*x));
}

    /* this is in a separately called subroutine so that the buffer isn't
    sitting on the stack while the messages are getting passed. */
static int socketreceiver_doread(t_socketreceiver *x)
{
    char messbuf[INBUFSIZE], *bp = messbuf;
    int indx, first = 1;
    int inhead = x->sr_inhead;
    int intail = x->sr_intail;
    char *inbuf = x->sr_inbuf;
    for (indx = intail; first || (indx != inhead);
        first = 0, (indx = (indx+1)&(INBUFSIZE-1)))
    {
            /* if we hit a semi that isn't preceeded by a \, it's a message
            boundary.  LATER we should deal with the possibility that the
            preceeding \ might itself be escaped! */
        char c = *bp++ = inbuf[indx];
        if (c == ';' && (!indx || inbuf[indx-1] != '\\'))
        {
            intail = (indx+1)&(INBUFSIZE-1);
            binbuf_text(inbinbuf, messbuf, bp - messbuf);
            if (sys_debuglevel & DEBUG_MESSDOWN) {
                if (stderr_isatty)
                    fprintf(stderr,"\n<- \e[0;1;36m%.*s\e[0m", bp - messbuf, messbuf);
                else
                    fprintf(stderr,"\n<- %.*s", bp - messbuf, messbuf);
            }
            x->sr_inhead = inhead;
            x->sr_intail = intail;
            return (1);
        }
    }
    return (0);
}

static void socketreceiver_getudp(t_socketreceiver *x, int fd)
{
    char buf[INBUFSIZE+1];
    int ret = recv(fd, buf, INBUFSIZE, 0);
    if (ret < 0)
    {
        sys_sockerror("recv");
        sys_rmpollfn(fd);
        sys_closesocket(fd);
    }
    else if (ret > 0)
    {
        buf[ret] = 0;
#if 0
        post("%s", buf);
#endif
        if (buf[ret-1] != '\n')
        {
#if 0
            buf[ret] = 0;
            error("dropped bad buffer %s\n", buf);
#endif
        }
        else
        {
            char *semi = strchr(buf, ';');
            if (semi) 
                *semi = 0;
            binbuf_text(inbinbuf, buf, strlen(buf));
            outlet_setstacklim();
            if (x->sr_socketreceivefn)
                (*x->sr_socketreceivefn)(x->sr_owner, inbinbuf);
            else bug("socketreceiver_getudp");
        }
    }
}

void sys_exit(void);

void socketreceiver_read(t_socketreceiver *x, int fd)
{
    if (x->sr_udp)   /* UDP ("datagram") socket protocol */
        socketreceiver_getudp(x, fd);
    else  /* TCP ("streaming") socket protocol */
    {
        int readto =
            (x->sr_inhead >= x->sr_intail ? INBUFSIZE : x->sr_intail-1);
        int ret;

            /* the input buffer might be full.  If so, drop the whole thing */
        if (readto == x->sr_inhead)
        {
            fprintf(stderr, "pd: dropped message from gui\n");
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
                if (x == sys_socketreceiver) sys_bail(1);
                else
                {
                    if (x->sr_notifier) (*x->sr_notifier)(x->sr_owner);
                    sys_rmpollfn(fd);
                    sys_closesocket(fd);
                }
            }
            else if (ret == 0)
            {
                if (x == sys_socketreceiver)
                {
                    fprintf(stderr, "pd: exiting\n");
                    sys_exit();
                    return;
                }
                else
                {
                    post("EOF on socket %d\n", fd);
                    if (x->sr_notifier) (*x->sr_notifier)(x->sr_owner);
                    sys_rmpollfn(fd);
                    sys_closesocket(fd);
                }
            }
            else
            {
                x->sr_inhead += ret;
                if (x->sr_inhead >= INBUFSIZE) x->sr_inhead = 0;
                while (socketreceiver_doread(x))
                {
                    outlet_setstacklim();
                    if (x->sr_socketreceivefn)
                        (*x->sr_socketreceivefn)(x->sr_owner, inbinbuf);
                    else binbuf_eval(inbinbuf, 0, 0, 0);
                    if (x->sr_inhead == x->sr_intail)
                        break;
                }
            }
        }
    }
}

void sys_closesocket(int fd)
{
#ifdef HAVE_UNISTD_H
    close(fd);
#endif
#ifdef MSW
    closesocket(fd);
#endif
}

/* ---------------------- sending messages to the GUI ------------------ */
#define GUI_ALLOCCHUNK 8192
#define GUI_UPDATESLICE 512 /* how much we try to do in one idle period */
#define GUI_BYTESPERPING 1024 /* how much we send up per ping */
//#define GUI_BYTESPERPING 0x7fffffff /* as per Miller's suggestion to disable the flow control */

typedef struct _guiqueue
{
    void *gq_client;
    t_glist *gq_glist;
    t_guicallbackfn gq_fn;
    struct _guiqueue *gq_next;
} t_guiqueue;

static t_guiqueue *sys_guiqueuehead;
static char *sys_guibuf;
static int sys_guibufhead;
static int sys_guibuftail;
static int sys_guibufsize;
static int sys_waitingforping;
static int sys_bytessincelastping;

static void sys_trytogetmoreguibuf(int newsize)
{
    char *newbuf = realloc(sys_guibuf, newsize);
#if 0
    static int sizewas;
    if (newsize > 70000 && sizewas < 70000)
    {
        int i;
        for (i = sys_guibuftail; i < sys_guibufhead; i++)
            fputc(sys_guibuf[i], stderr);
    }
    sizewas = newsize;
#endif
#if 0
    fprintf(stderr, "new size %d (head %d, tail %d)\n",
        newsize, sys_guibufhead, sys_guibuftail);
#endif

        /* if realloc fails, make a last-ditch attempt to stay alive by
        synchronously writing out the existing contents.  LATER test
        this by intentionally setting newbuf to zero */
    if (!newbuf)
    {
        int bytestowrite = sys_guibuftail - sys_guibufhead;
        int written = 0;
        while (1)
        {
            int res = send(sys_guisock,
                sys_guibuf + sys_guibuftail + written, bytestowrite, 0);
            if (res < 0)
            {
                perror("pd output pipe");
                sys_bail(1);
            }
            else
            {
                written += res;
                if (written >= bytestowrite)
                    break;
            }
        }
        sys_guibufhead = sys_guibuftail = 0;
    }
    else
    {
        sys_guibufsize = newsize;
        sys_guibuf = newbuf;
    }
}

void blargh(void) {
#ifndef __linux__
  fprintf(stderr,"unhandled exception\n");
#else
  int i;
  void *array[25];
  int nSize = backtrace(array, 25);
  char **symbols = backtrace_symbols(array, nSize);
  for (i=0; i<nSize; i++) fprintf(stderr,"%d: %s\n",i,symbols[i]);
  free(symbols);
#endif
}

static int lastend = -1;
void sys_vvgui(const char *fmt, va_list ap) {
	va_list aq;
	va_copy(aq,ap);
    int msglen;

    if (sys_nogui)
        return;
    if (!sys_guibuf)
    {
        if (!(sys_guibuf = malloc(GUI_ALLOCCHUNK)))
        {
            fprintf(stderr, "Pd: couldn't allocate GUI buffer\n");
            sys_bail(1);
        }
        sys_guibufsize = GUI_ALLOCCHUNK;
        sys_guibufhead = sys_guibuftail = 0;
    }
    if (sys_guibufhead > sys_guibufsize - (GUI_ALLOCCHUNK/2))
        sys_trytogetmoreguibuf(sys_guibufsize + GUI_ALLOCCHUNK);
    msglen = vsnprintf(sys_guibuf + sys_guibufhead,
        sys_guibufsize - sys_guibufhead, fmt, ap);
    va_end(ap);
    if(msglen < 0) 
    {
        fprintf(stderr, "Pd: buffer space wasn't sufficient for long GUI string\n");
        return;
    }
    if (msglen >= sys_guibufsize - sys_guibufhead)
    {
        int msglen2, newsize = sys_guibufsize + 1 +
            (msglen > GUI_ALLOCCHUNK ? msglen : GUI_ALLOCCHUNK);
        sys_trytogetmoreguibuf(newsize);

        va_copy(ap,aq);
        msglen2 = vsnprintf(sys_guibuf + sys_guibufhead,
            sys_guibufsize - sys_guibufhead, fmt, ap);
        va_end(ap);
        if (msglen2 != msglen)
            bug("sys_vgui");
        if (msglen >= sys_guibufsize - sys_guibufhead)
            msglen = sys_guibufsize - sys_guibufhead;
    }
    if (sys_debuglevel & DEBUG_MESSUP) {
        //blargh();
        //int begin = lastend=='\n' || lastend=='\r' || lastend==-1;
        int begin = lastend=='\x1f' || lastend==-1;
        if (stderr_isatty)
            fprintf(stderr, "%s\e[0;1;35m%s\e[0m",
                begin ? "\n-> " : "", sys_guibuf + sys_guibufhead);
        else
            fprintf(stderr, "%s%s",
                begin ? "\n-> " : "", sys_guibuf + sys_guibufhead);
    }
    sys_guibufhead += msglen;
    sys_bytessincelastping += msglen;
    if (sys_guibufhead>0) lastend=sys_guibuf[sys_guibufhead-1];
    if (sys_guibufhead>1 && strcmp(sys_guibuf+sys_guibufhead-2,"\\\n")==0)
        lastend=' ';
}
#undef sys_vgui
#undef sys_gui
void sys_vgui(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    sys_vvgui(fmt,ap);
}

/* This was used by the old command interface, but is no longer used. */
void sys_vvguid(const char *file, int line, const char *fmt, va_list ap) {
    if ((sys_debuglevel&4) && /*line>=0 &&*/ (lastend=='\n' || lastend=='\r' || lastend==-1)) {
        sys_vgui("AT %s:%d; ",file,line);
    }
    sys_vvgui(fmt,ap);
}

void sys_gui(const char *s)
{
    sys_vgui("%s", s);
}

static void escape_double_quotes(const char *src) {
    int dq = 0, len = 0;
    const char *s = src;
    while(*s)
    {
        len++;
        if (*s == '\"' || *s == '\\')
        {
            dq++;
        }
        s++;
    }
    if (!dq)
        sys_vgui("\"%s\"", src);
    else
    {
        char *dest = (char *)t_getbytes((len+dq+1)*sizeof(*dest));
        char *tmp = dest;
        s = src;
        while(*s)
        {
            if (*s == '\"' || *s == '\\')
            {
                *tmp++ = '\\';
                *tmp++ = *s;
            }
            else
            {
                *tmp++ = *s;
            }
            s++;
        }
        *tmp = '\0'; /* null terminate */
        sys_vgui("\"%s\"", dest);
        t_freebytes(dest, (len+dq+1)*sizeof(*dest));
    }
}

void gui_end_vmess(void)
{
    sys_gui("\x1f"); /* hack: use unit separator to delimit messages */
}

static int gui_array_head;
static int gui_array_tail;
/* this is a bug fix for the edge case of a message to the gui
   with a single array. This is necessary because I'm using a space
   delimiter between the first and second arg, and commas between
   the rest.
   While I think gui_vmess and gui_start_vmess interfaces work well,
   the actual format of the message as sent to the GUI can definitely
   be changed if need be. I threw it together hastily just to get something
   easy to parse in Javascript on the GUI side.
   -jw */
static void set_leading_array(int state) {
    if (state)
    {
        gui_array_head = 1;
        gui_array_tail = 0;
    }
    else
    {
        gui_array_head = 0;
        gui_array_tail = 0;
    }
}

/* quick hack to send a parameter list for use as a function call in nw.js */
void gui_do_vmess(const char *sel, char *fmt, int end, va_list ap)
{
    //va_list ap;
    int nargs = 0;
    char *fp = fmt;
    //va_start(ap, end);
    sys_vgui("%s ", sel);
    while (*fp)
    {
        // stop-gap-- increase to 20 until we find a way to pass a list or array
        if (nargs >= 20)
        {
            error("sys_gui_vmess: only 10 named parameters allowed");
            break;
        }
        if (nargs > 0) sys_gui(",");
        switch(*fp++)
        {
        case 'f': sys_vgui("%g", va_arg(ap, double)); break;
        case 's': escape_double_quotes(va_arg(ap, const char *)); break;
        case 'i': sys_vgui("%d", va_arg(ap, int)); break;
        case 'x': sys_vgui("\"" X_SPECIFIER "\"",
            va_arg(ap, long unsigned int));
            break;
        //case 'p': SETPOINTER(at, va_arg(ap, t_gpointer *)); break;
        default: goto done;
        }
        nargs++;
    }
done:
    va_end(ap);
    if (nargs) set_leading_array(0);
    if (end)
        gui_end_vmess();
}

void gui_vmess(const char *sel, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    gui_do_vmess(sel, fmt, 1, ap);
}

void gui_start_vmess(const char *sel, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    set_leading_array(1);
    gui_do_vmess(sel, fmt, 0, ap);
}

void gui_start_array(void)
{
    if (gui_array_head && !gui_array_tail)
        sys_gui("[");
    else
        sys_gui(",[");
    gui_array_head = 1;
    gui_array_tail = 0;
}

void gui_f(t_float f)
{
    if (gui_array_head && !gui_array_tail)
    {
        sys_vgui("%g", f);
    }
    else
        sys_vgui(",%g", f);
    gui_array_head = 0;
    gui_array_tail = 0;
}

void gui_i(int i)
{
    if (gui_array_head && !gui_array_tail)
    {
        sys_vgui("%d", i);
    }
    else
        sys_vgui(",%d", i);
    gui_array_head = 0;
    gui_array_tail = 0;
}

void gui_s(const char *s) 
{
    if (gui_array_head && !gui_array_tail)
    {
        escape_double_quotes(s);
    }
    else
    {
        sys_vgui(",");
        escape_double_quotes(s);
    }
    gui_array_head = 0;
    gui_array_tail = 0;
}

void gui_x(long unsigned int i)
{
    if (gui_array_head && !gui_array_tail)
        sys_vgui("\"x%.6lx\"", i);
    else
        sys_vgui(",\"x%.6lx\"", i);
    gui_array_head = 0;
    gui_array_tail = 0;
}

void gui_end_array(void)
{
    sys_gui("]");
    gui_array_tail = 1;
}

/* Old GUI command interface... */
void sys_vguid(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    char buf[MAXPDSTRING], *bufp, *endp;
    va_start(ap, fmt);
    vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
    va_end(ap);
    /* Just display the first non-empty line, to remove console clutter. -ag */
    for (bufp = buf; *bufp && isspace(*bufp); bufp++) ;
    if ((endp = strchr(bufp, '\n')))
    {
        *endp = 0;
        if (endp[1])
        {
            /* Indicate that more stuff follows with an ellipsis. */
            strncat(bufp, "...", MAXPDSTRING);
        }
    }
    gui_vmess("gui_legacy_tcl_command", "sis",
        file, line, bufp);
    //sys_vvguid(file,line,fmt,ap);
}

int sys_flushtogui( void)
{
    int writesize = sys_guibufhead - sys_guibuftail, nwrote = 0;
    if (writesize > 0)
        nwrote = send(sys_guisock, sys_guibuf + sys_guibuftail, writesize, 0);

#if 0   
    if (writesize)
        fprintf(stderr, "wrote %d of %d\n", nwrote, writesize);
#endif

    if (nwrote < 0)
    {
        perror("pd-to-gui socket");
        sys_bail(1);
    }
    else if (!nwrote)
        return (0);
    else if (nwrote >= sys_guibufhead - sys_guibuftail)
         sys_guibufhead = sys_guibuftail = 0;
    else if (nwrote)
    {
        sys_guibuftail += nwrote;
        if (sys_guibuftail > (sys_guibufsize >> 2))
        {
            memmove(sys_guibuf, sys_guibuf + sys_guibuftail,
                sys_guibufhead - sys_guibuftail);
            sys_guibufhead = sys_guibufhead - sys_guibuftail;
            sys_guibuftail = 0;
        }
    }
    return (1);
}

void glob_ping(t_pd *dummy)
{
    sys_waitingforping = 0;
}

static int sys_flushqueue(void )
{
    int wherestop = sys_bytessincelastping + GUI_UPDATESLICE;
    if (wherestop + (GUI_UPDATESLICE >> 1) > GUI_BYTESPERPING)
        wherestop = 0x7fffffff;
    if (sys_waitingforping)
        return (0);
    if (!sys_guiqueuehead)
        return (0);
    while (1)
    {
        if (sys_bytessincelastping >= GUI_BYTESPERPING)
        {
            //sys_gui("pdtk_ping\n");
            gui_vmess("gui_ping", "");
            sys_bytessincelastping = 0;
            sys_waitingforping = 1;
            return (1);
        }
        if (sys_guiqueuehead)
        {
            t_guiqueue *headwas = sys_guiqueuehead;
            sys_guiqueuehead = headwas->gq_next;
            (*headwas->gq_fn)(headwas->gq_client, headwas->gq_glist);
            t_freebytes(headwas, sizeof(*headwas));
            if (sys_bytessincelastping >= wherestop)
                break;
        }
        else break;
    }
    sys_flushtogui();
    return (1);
}

    /* flush output buffer and update queue to gui in small time slices */
static int sys_poll_togui(void) /* returns 1 if did anything */
{
    if (sys_nogui)
        return (0);
        /* see if there is stuff still in the buffer, if so we
            must have fallen behind, so just try to clear that. */
    if (sys_flushtogui())
        return (1);
        /* if the flush wasn't complete, wait. */
    if (sys_guibufhead > sys_guibuftail)
        return (0);
    
        /* check for queued updates */
    if (sys_flushqueue())
        return (1);
    
    return (0);
}

    /* if some GUI object is having to do heavy computations, it can tell
    us to back off from doing more updates by faking a big one itself. */
void sys_pretendguibytes(int n)
{
    sys_bytessincelastping += n;
}

void sys_queuegui(void *client, t_glist *glist, t_guicallbackfn f)
{
    t_guiqueue **gqnextptr, *gq;
    if (!sys_guiqueuehead)
        gqnextptr = &sys_guiqueuehead;
    else
    {
        for (gq = sys_guiqueuehead; gq->gq_next; gq = gq->gq_next)
            if (gq->gq_client == client)
                return;
        if (gq->gq_client == client)
            return;
        gqnextptr = &gq->gq_next;
    }
    gq = t_getbytes(sizeof(*gq));
    gq->gq_next = 0;
    gq->gq_client = client;
    gq->gq_glist = glist;
    gq->gq_fn = f;
    gq->gq_next = 0;
    *gqnextptr = gq;
}

void sys_unqueuegui(void *client)
{
    t_guiqueue *gq, *gq2;
    while (sys_guiqueuehead && sys_guiqueuehead->gq_client == client)
    {
        gq = sys_guiqueuehead;
        sys_guiqueuehead = sys_guiqueuehead->gq_next;
        t_freebytes(gq, sizeof(*gq));
    }
    if (!sys_guiqueuehead)
        return;
    for (gq = sys_guiqueuehead; gq2 = gq->gq_next; gq = gq2)
        if (gq2->gq_client == client)
    {
        gq->gq_next = gq2->gq_next;
        t_freebytes(gq2, sizeof(*gq2));
        break;
    }
}

int sys_pollgui(void)
{
    // return (sys_domicrosleep(0, 1) || sys_poll_togui());
    // "fix" for sluggish gui proposed by Miller on 12/16/2012
    return (sys_domicrosleep(0, 1) + sys_poll_togui());
}



/* --------------------- starting up the GUI connection ------------- */

static int sys_watchfd;

#ifdef __linux__
void glob_watchdog(t_pd *dummy)
{
    if (write(sys_watchfd, "\n", 1) < 1)
    {
        fprintf(stderr, "pd: watchdog process died\n");
        sys_bail(1);
    }
}
#endif

#define FIRSTPORTNUM 5400

#define MAXFONTS 21
static int defaultfontshit[MAXFONTS] = {
    8, 5, 9, 10, 6, 10, 12, 7, 13, 14, 9, 17, 16, 10, 19, 24, 15, 28,
        24, 15, 28};
#define NDEFAULTFONT (sizeof(defaultfontshit)/sizeof(*defaultfontshit))

extern t_symbol *pd_getplatform(void);
extern void sys_expandpath(const char *from, char *to);

/* set the datadir for nwjs. We use the published nw.js
   defaults if there's only one instance of the GUI set to
   run. Otherwise, we append the port number so that
   nw.js will spawn a new instance for us.

   Currently, users can give a command line arg to force Pd
   to start with a new GUI instance. A new GUI must also get
   created when a user turns on audio and there is a [pd~] object
   on the canvas. The latter would actually be more usable within
   a single GUI instance, but that would require some way to
   visually distinguish patches that are associated with different Pd
   engine processes.
*/ 

static void set_datadir(char *buf, int new_gui_instance, int portno)
{
    char dir[FILENAME_MAX];
    t_symbol *platform = pd_getplatform(); 
    strcpy(buf, "--user-data-dir=");
    /* Let's go ahead and quote the string to handle spaces in
       paths, etc. */
    strcat(buf, "\"");
    if (platform == gensym("darwin"))
        sys_expandpath("~/Library/Application Support/", dir);
    else if (platform != gensym("win32"))/* bsd and linux */
        sys_expandpath("~/.config/", dir);
#ifdef _WIN32
        /* win32 has a more robust API for getting the
           value of %LOCALAPPDATA%, but this works on
           Windows 7 and is straightforward... */
        char *env = getenv("LOCALAPPDATA");
        strcpy(dir, env);
        strcat(dir, "\\");
#endif
    strcat(dir, "purr-data");
    if (new_gui_instance)
    {
        char portbuf[10];
        sprintf(portbuf, "-%d", portno);
        strcat(dir, portbuf);
    }
    strcat(buf, dir);
    /* Finally, close quote... */
    strcat(buf, "\"");
}

extern int sys_unique;

int sys_startgui(const char *guidir)
{
    pid_t childpid;
    char cwd[FILENAME_MAX];
    char cmdbuf[4*MAXPDSTRING];
    struct sockaddr_in server = {0};
    int len = sizeof(server);
    int ntry = 0, portno = FIRSTPORTNUM;
    int xsock = -1;
    stderr_isatty = isatty(2);
#ifdef MSW
    if (GetCurrentDirectory(FILENAME_MAX, cwd) == 0)
        strcpy(cwd, ".");
#endif
#ifdef HAVE_UNISTD_H
    if (!getcwd(cwd, FILENAME_MAX))
        strcpy(cwd, ".");
#endif
#ifdef MSW
    short version = MAKEWORD(2, 0);
    WSADATA nobby;
#endif
#ifdef HAVE_UNISTD_H
    int stdinpipe[2];
#endif
    /* create an empty FD poll list */
    sys_fdpoll = (t_fdpoll *)t_getbytes(0);
    sys_nfdpoll = 0;
    inbinbuf = binbuf_new();

#ifdef HAVE_UNISTD_H
    signal(SIGHUP, sys_huphandler);
    signal(SIGINT, sys_exithandler);
    signal(SIGQUIT, sys_exithandler);
    signal(SIGILL, sys_exithandler);
    signal(SIGIOT, sys_exithandler);
    signal(SIGFPE, SIG_IGN);
    /* signal(SIGILL, sys_exithandler);
    signal(SIGBUS, sys_exithandler);
    signal(SIGSEGV, sys_exithandler); */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
#if 0  /* GG says: don't use that */
    signal(SIGSTKFLT, sys_exithandler);
#endif
#endif
#ifdef MSW
    if (WSAStartup(version, &nobby)) sys_sockerror("WSAstartup");
#endif

    if (sys_nogui)
    {
            /* fake the GUI's message giving cwd and font sizes; then
            skip starting the GUI up. */
        t_atom zz[NDEFAULTFONT+2];
        int i;
        strcpy(cmdbuf, cwd);
        SETSYMBOL(zz, gensym(cmdbuf));
        for (i = 0; i < (int)NDEFAULTFONT; i++)
            SETFLOAT(zz+i+1, defaultfontshit[i]);
        SETFLOAT(zz+NDEFAULTFONT+1,0);
        glob_initfromgui(0, 0, 23, zz);
    }
    else if (sys_guisetportnumber)  /* GUI exists and sent us a port number */
    {
        struct sockaddr_in server = {0};
        struct hostent *hp;
        /* create a socket */
        sys_guisock = socket(AF_INET, SOCK_STREAM, 0);
        if (sys_guisock < 0)
            sys_sockerror("socket");

        /* connect socket using hostname provided in command line */
        server.sin_family = AF_INET;

        hp = gethostbyname(LOCALHOST);

        if (hp == 0)
        {
            fprintf(stderr,
                "localhost not found (inet protocol not installed?)\n");
            exit(1);
        }
        memcpy((char *)&server.sin_addr, (char *)hp->h_addr, hp->h_length);

        /* assign client port number */
        server.sin_port = htons((unsigned short)sys_guisetportnumber);

            /* try to connect */
        if (connect(sys_guisock, (struct sockaddr *) &server, sizeof (server))
            < 0)
        {
            sys_sockerror("connecting stream socket");
            exit(1);
        }
    }
    else    /* default behavior: start up the GUI ourselves. */
    {
#ifdef MSW
        char scriptbuf[MAXPDSTRING+30], wishbuf[MAXPDSTRING+30], portbuf[80];
        int spawnret;

#endif
#ifdef MSW
        char intarg;
#else
        int intarg;
#endif

        /* create a socket */
        xsock = socket(AF_INET, SOCK_STREAM, 0);
        if (xsock < 0) sys_sockerror("socket");
#if 0
        intarg = 0;
        if (setsockopt(xsock, SOL_SOCKET, SO_SNDBUF,
            &intarg, sizeof(intarg)) < 0)
                post("setsockopt (SO_RCVBUF) failed\n");
        intarg = 0;
        if (setsockopt(xsock, SOL_SOCKET, SO_RCVBUF,
            &intarg, sizeof(intarg)) < 0)
                post("setsockopt (SO_RCVBUF) failed\n");
#endif
        intarg = 1;
        if (setsockopt(xsock, IPPROTO_TCP, TCP_NODELAY,
            &intarg, sizeof(intarg)) < 0)
#ifndef MSW
                post("setsockopt (TCP_NODELAY) failed\n")
#endif
                    ;
        
        
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;

        /* assign server port number */
        server.sin_port =  htons((unsigned short)portno);
        /* name the socket */
        while (bind(xsock, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
#ifdef MSW
            int err = WSAGetLastError();
#else
            int err = errno;
#endif
            if ((ntry++ > 20) || (err != EADDRINUSE))
            {
                perror("bind");
                fprintf(stderr,
                    "Pd needs your machine to be configured with\n");
                fprintf(stderr,
                  "'networking' turned on (see Pd's html doc for details.)\n");
                exit(1);
            }
            portno++;
            server.sin_port = htons((unsigned short)(portno));
        }

        if (sys_verbose) fprintf(stderr, "port %d\n", portno);


#ifdef HAVE_UNISTD_H
        if (!sys_guicmd)
        {
#ifdef __APPLE__
            int i;
            struct stat statbuf;
            glob_t glob_buffer;
            char *homedir = getenv("HOME");
            char embed_glob[FILENAME_MAX];
            char embed_filename[FILENAME_MAX], home_filename[FILENAME_MAX];
            char *wish_paths[10] = {
                "(did not find an embedded wish)",
                "(did not find a home directory)",
                "/Applications/Utilities/Wish.app/Contents/MacOS/Wish",
                "/Applications/Utilities/Wish Shell.app/Contents/MacOS/Wish Shell",
                "/Applications/Wish.app/Contents/MacOS/Wish",
                "/Applications/Wish Shell.app/Contents/MacOS/Wish Shell",
                "/usr/local/bin/wish8.4",
                "/sw/bin/wish8.4"
                "/opt/bin/wish8.4"
                "/usr/bin/wish8.4"
                "/usr/local/bin/wish8.4",
                "/usr/local/bin/wish",
                "/usr/bin/wish"
            };
            /* this glob is needed so the Wish executable can have the same
             * filename as the Pd.app, i.e. 'Pd-0.42-3.app' should have a Wish
             * executable called 'Pd-0.42-3.app/Contents/MacOS/Pd-0.42-3' */
//            sprintf(embed_glob, "%s/../../MacOS/Pd*", guidir);
            sprintf(embed_glob, "%s/../../MacOS/nwjs", guidir);
            glob_buffer.gl_matchc = 1; /* we only need one match */
            glob(embed_glob, GLOB_LIMIT, NULL, &glob_buffer);
            if (glob_buffer.gl_pathc > 0) {
                strcpy(embed_filename, glob_buffer.gl_pathv[0]);
                wish_paths[0] = embed_filename;
            }
            sprintf(home_filename,
                    "%s/Applications/Wish.app/Contents/MacOS/Wish",homedir);
            wish_paths[1] = home_filename;
            for(i=0;i<10;i++) {
                if (sys_verbose)
                    fprintf(stderr, "Trying Wish at \"%s\"\n", wish_paths[i]);
                if (stat(wish_paths[i], &statbuf) >= 0)
                    break;
            }
            sprintf(cmdbuf,"\"%s\" %s/pd.tk %d\n",wish_paths[i],guidir,portno);
            char data_dir_flag[MAXPDSTRING];
            set_datadir(data_dir_flag, sys_unique, portno);
            /* Make a copy in case the user wants to change to the repo
               dir while developing... */
            char guidir2[MAXPDSTRING];
            /* Let's go ahead and quote the path in case there are spaces
               in it. This won't happen on a sane Linux/BSD setup. But it
               will happen under Windows, so we quote here, too, for
               the sake of consistency. */
            strcpy(guidir2, "\"");
            strcat(guidir2, guidir);
            strcat(guidir2, "\"");
            /* Uncomment the following line if you want to
               use the nw binary and GUI code from your local
               copy of the Purr Data repo. (Make sure to run
               tar_em_up.sh first to fetch the nw binary.) */
            //strcpy(guidir2, "\"/home/grieg/purr-data/pd/nw\"");
            sprintf(cmdbuf,
                "\"%s\" %s %s "
                "%d localhost %s %s " X_SPECIFIER,
                wish_paths[i],
                data_dir_flag,
                guidir2,
                portno,
                (sys_k12_mode ? "pd-l2ork-k12" : "pd-l2ork"),
                guidir2,
                (long unsigned int)pd_this);
#else
            sprintf(cmdbuf,
                "TCL_LIBRARY=\"%s/tcl/library\" TK_LIBRARY=\"%s/tk/library\" \
                 \"%s/pd-gui\" %d localhost %s\n",
                 sys_libdir->s_name, sys_libdir->s_name, guidir, portno, (sys_k12_mode ? "pd-l2ork-k12" : "pd-l2ork"));

            fprintf(stderr, "guidir is %s\n", guidir);

            /* For some reason, the nw binary doesn't give you access to
               the first argument-- this is the path to the directory where
               package.json lives (or the zip file if it's compressed). So
               we add it again as the last argument to make sure we can fetch
               it on the GUI side. */
            char data_dir_flag[MAXPDSTRING];
            set_datadir(data_dir_flag, sys_unique, portno);

            /* Make a copy in case the user wants to change to the repo
               dir while developing... */
            char guidir2[MAXPDSTRING];
            /* Let's go ahead and quote the path in case there are spaces
               in it. This won't happen on a sane Linux/BSD setup. But it
               will happen under Windows, so we quote here, too, for
               the sake of consistency. */
            strcpy(guidir2, "\"");
            strcat(guidir2, guidir);
            strcat(guidir2, "\"");
            /* Uncomment the following line if you want to
               use the nw binary and GUI code from your local
               copy of the Purr Data repo. (Make sure to run
               tar_em_up.sh first to fetch the nw binary.) */
            //strcpy(guidir2, "\"/home/grieg/purr-data/pd/nw\"");
            sprintf(cmdbuf,
                "%s/nw/nw %s %s "
                "%d localhost %s %s " X_SPECIFIER,
                guidir2,
                data_dir_flag,
                guidir2,
                portno,
                (sys_k12_mode ? "pd-l2ork-k12" : "pd-l2ork"),
                guidir2,
                (long unsigned int)pd_this);
#endif
            sys_guicmd = cmdbuf;
        }

        if (sys_verbose) 
            fprintf(stderr, "%s", sys_guicmd);

        childpid = fork();
        if (childpid < 0)
        {
            if (errno) perror("sys_startgui");
            else fprintf(stderr, "sys_startgui failed\n");
            return (1);
        }
        else if (!childpid)                     /* we're the child */
        {
            setuid(getuid());          /* lose setuid priveliges */
#ifndef __APPLE__
                /* the wish process in Unix will make a wish shell and
                    read/write standard in and out unless we close the
                    file descriptors.  Somehow this doesn't make the MAC OSX
                        version of Wish happy...*/
            if (pipe(stdinpipe) < 0)
                sys_sockerror("pipe");
            else
            {
                if (stdinpipe[0] != 0)
                {
                    close (0);
                    dup2(stdinpipe[0], 0);
                    close(stdinpipe[0]);
                }
            }

#endif
            execl("/bin/sh", "sh", "-c", sys_guicmd, (char*)0);
            perror("pd: exec");
            _exit(1);
       }
#endif /* HAVE_UNISTD_H */

#ifdef MSW
            /* in MSW land "guipath" is unused; we just do everything from
            the libdir. */
        /* fprintf(stderr, "%s\n", sys_libdir->s_name); */
        
        //strcpy(scriptbuf, "\"");
        //strcat(scriptbuf, sys_libdir->s_name);
        //strcat(scriptbuf, "/" PDBINDIR "pd.tk\"");
        //sys_bashfilename(scriptbuf, scriptbuf);

        char pd_this_string[80];
        sprintf(pd_this_string, X_SPECIFIER, (long unsigned int)pd_this);
        sprintf(scriptbuf, "\""); /* use quotes in case there are spaces */
        strcat(scriptbuf, sys_libdir->s_name);
        strcat(scriptbuf, "/" PDBINDIR);
        sys_bashfilename(scriptbuf, scriptbuf);
        /* PDBINDIR ends with a "\", which will unfortunately end up
           escaping our trailing double quote. So we replace the "\" with
           our double quote. nw.js seems to find the path in this case,
           so this _should_ work for all cases. */
        scriptbuf[strlen(scriptbuf)-1] = '"';

        sprintf(portbuf, "%d", portno);

        strcpy(wishbuf, sys_libdir->s_name);
        strcat(wishbuf, "/" PDBINDIR "nw/nw");
        sys_bashfilename(wishbuf, wishbuf);

        char data_dir_flag[MAXPDSTRING];
        set_datadir(data_dir_flag, sys_unique, portno);
        spawnret = _spawnl(P_NOWAIT, wishbuf, "pd-nw",
            data_dir_flag,
            scriptbuf,
            portbuf,
            "localhost",
            (sys_k12_mode ? "pd-l2ork-k12" : "pd-l2ork"),
            scriptbuf, pd_this_string, 0);
        if (spawnret < 0)
        {
            perror("spawnl");
            fprintf(stderr, "%s: couldn't load GUI\n", wishbuf);
            exit(1);
        }

#endif /* MSW */
    }

#if defined(__linux__) || defined(IRIX)
        /* now that we've spun off the child process we can promote
        our process's priority, if we can and want to.  If not specfied
        (-1), we assume real-time was wanted.  Afterward, just in case
        someone made Pd setuid in order to get permission to do this,
        unset setuid and lose root priveliges after doing this.  Starting
        in Linux 2.6 this is accomplished by putting lines like:
                @audio - rtprio 99
                @audio - memlock unlimited
        in the system limits file, perhaps /etc/limits.conf or
        /etc/security/limits.conf */
    if (sys_hipriority == -1)
        sys_hipriority = 1; //(!getuid() || !geteuid());
    
    sprintf(cmdbuf, "%s/pd-watchdog\n", guidir);
    if (sys_hipriority)
    {
            /* To prevent lockup, we fork off a watchdog process with
            higher real-time priority than ours.  The GUI has to send
            a stream of ping messages to the watchdog THROUGH the Pd
            process which has to pick them up from the GUI and forward
            them.  If any of these things aren't happening the watchdog
            starts sending "stop" and "cont" signals to the Pd process
            to make it timeshare with the rest of the system.  (Version
            0.33P2 : if there's no GUI, the watchdog pinging is done
            from the scheduler idle routine in this process instead.) */
        int pipe9[2], watchpid;

        if (pipe(pipe9) < 0)
        {
            setuid(getuid());      /* lose setuid priveliges */
            sys_sockerror("pipe");
            return (1);
        }
        watchpid = fork();
        if (watchpid < 0)
        {
            setuid(getuid());      /* lose setuid priveliges */
            if (errno)
                perror("sys_startgui");
            else fprintf(stderr, "sys_startgui failed\n");
            return (1);
        }
        else if (!watchpid)             /* we're the child */
        {
            sys_set_priority(1);
            setuid(getuid());      /* lose setuid priveliges */
            if (pipe9[1] != 0)
            {
                dup2(pipe9[0], 0);
                close(pipe9[0]);
            }
            close(pipe9[1]);

            if (sys_verbose) fprintf(stderr, "%s", cmdbuf);
            execl("/bin/sh", "sh", "-c", cmdbuf, (char*)0);
            perror("pd: exec");
            _exit(1);
        }
        else                            /* we're the parent */
        {
            sys_set_priority(0);
            setuid(getuid());      /* lose setuid priveliges */
            close(pipe9[0]);
            sys_watchfd = pipe9[1];
                /* We also have to start the ping loop in the GUI;
                this is done later when the socket is open. */
        }
    }

    setuid(getuid());          /* lose setuid priveliges */
#endif /* __linux__ */

#ifdef MSW
    if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
        fprintf(stderr, "pd: couldn't set high priority class\n");
#endif
#ifdef __APPLE__
    if (sys_hipriority)
    {
        struct sched_param param;
        int policy = SCHED_RR;
        int err;
        param.sched_priority = 80; /* adjust 0 : 100 */

        err = pthread_setschedparam(pthread_self(), policy, &param);
        if (err)
            post("warning: high priority scheduling failed\n");
    }
#endif /* __APPLE__ */

    if (!sys_nogui && !sys_guisetportnumber)
    {
        if (sys_verbose)
            fprintf(stderr, "Waiting for connection request... \n");
        if (listen(xsock, 5) < 0) sys_sockerror("listen");

        sys_guisock = accept(xsock, (struct sockaddr *) &server, 
            (socklen_t *)&len);
#ifdef OOPS
        sys_closesocket(xsock);
#endif
        if (sys_guisock < 0) sys_sockerror("accept");
        if (sys_verbose)
            fprintf(stderr, "... connected\n");
    }
    if (!sys_nogui)
    {
      char buf[256], buf2[256];
         sys_socketreceiver = socketreceiver_new(0, 0, 0, 0);
         sys_addpollfn(sys_guisock, (t_fdpollfn)socketreceiver_read,
             sys_socketreceiver);

            /* here is where we start the pinging. */
#if defined(__linux__) || defined(IRIX)
         if (sys_hipriority)
             gui_vmess("gui_watchdog", "");
#endif
         sys_get_audio_apis(buf);
         sys_get_midi_apis(buf2);
//         sys_vgui("pdtk_pd_startup {%s} %s %s {%s} %s\n", pd_version, buf, buf2, 
//                  sys_font, sys_fontweight); 

        t_binbuf *aapis = binbuf_new(), *mapis = binbuf_new();
        sys_get_audio_apis2(aapis);
        sys_get_midi_apis2(mapis);
        gui_start_vmess("gui_startup", "sss",
		  pd_version,
		  sys_font,
		  sys_fontweight);

        int i;
        gui_start_array(); // audio apis
        for (i = 0; i < binbuf_getnatom(aapis); i+=2)
        {
            gui_s(atom_getsymbol(binbuf_getvec(aapis)+i)->s_name);
            gui_i(atom_getint(binbuf_getvec(aapis)+i+1));
        }
        gui_end_array();

        gui_start_array(); // midi apis
        for (i = 0; i < binbuf_getnatom(mapis); i+=2)
        {
            gui_s(atom_getsymbol(binbuf_getvec(mapis)+i)->s_name);
            gui_i(atom_getint(binbuf_getvec(mapis)+i+1));
        }
        gui_end_array();

        gui_end_vmess();
        gui_vmess("gui_set_cwd", "xs", 0, cwd);
        binbuf_free(aapis);
        binbuf_free(mapis);
    }
    return (0);

}

extern void sys_exit(void);

/* This is called when something bad has happened, like a segfault.
Call glob_quit() below to exit cleanly.
LATER try to save dirty documents even in the bad case. */
void sys_bail(int n)
{
    static int reentered = 0;
    if (!reentered)
    {
        reentered = 1;
#ifndef __linux__  /* sys_close_audio() hangs if you're in a signal? */
        fprintf(stderr, "closing audio...\n");
        sys_close_audio();
        fprintf(stderr, "closing MIDI...\n");
        sys_close_midi();
        fprintf(stderr, "... done.\n");
#endif
        exit(n);
    }
    else _exit(1);
}

//extern t_pd *garray_arraytemplatecanvas;
//extern t_pd *garray_floattemplatecanvas;
extern void glob_closeall(void *dummy, t_floatarg fforce);

extern int do_not_redraw;

void glob_quit(void *dummy, t_floatarg status)
{
    /* If we're going to try to cleanly close everything here, we should
       do the same for all open patches and that is currently not the case,
       so for the time being, let's just leave OS to deal with freeing of all
       memory when the program exits... */

    /* Let's try to cleanly remove invisible template canvases */
    //if (garray_arraytemplatecanvas)
    //    canvas_free((t_canvas *)garray_arraytemplatecanvas);
    //if (garray_floattemplatecanvas)
    //    canvas_free( (t_canvas *)garray_floattemplatecanvas);
    canvas_suspend_dsp();
    do_not_redraw = 1;
    glob_closeall(0, 1);
    //sys_vgui("exit\n");
    gui_vmess("app_quit", "");
    if (!sys_nogui)
    {
        sys_closesocket(sys_guisock);
        sys_rmpollfn(sys_guisock);
    }
    sys_bail(status); 
}

