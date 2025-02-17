/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "s_stuff.h"
#ifdef _MSC_VER  /* This is only for Microsoft's compiler, not cygwin, e.g. */
#define snprintf sprintf_s
#endif

t_printhook sys_printhook;
t_printhook sys_printhook_error;
int sys_printtostderr;

#ifdef _WIN32

    /* NB: Unlike vsnprintf(), _vsnprintf() does *not* null-terminate
    the output if the resulting string is too large to fit into the buffer.
    Also, it just returns -1 instead of the required number of bytes.
    Strictly speaking, the UCRT in Windows 10 actually contains a standard-
    conforming vsnprintf() function that is not just an alias for _vsnprintf().
    However, MinGW traditionally links against the old msvcrt.dll runtime library.
    Recent versions of MinGW seem to have their own (standard-conformating)
    implementation of vsnprintf(), but to ensure portability we rather use our
    own implementation for all Windows builds. */
int pd_vsnprintf(char *buf, size_t size, const char *fmt, va_list argptr)
{
    int ret = _vsnprintf(buf, size, fmt, argptr);
    if (ret < 0)
    {
            /* null-terminate the buffer and get the required number of bytes. */
        ret = _vscprintf(fmt, argptr);
        buf[size - 1] = '\0';
    }
    return ret;
}

int pd_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = pd_vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return ret;
}

#else

int pd_vsnprintf(char *buf, size_t size, const char *fmt, va_list argptr)
{
    return vsnprintf(buf, size, fmt, argptr);
}

int pd_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return ret;
}

#endif

static char* strnpointerid(char *dest, const void *pointer, size_t len)
{
    *dest=0;
    if (pointer)
        snprintf(dest, len, ".x%zx", (t_uint)pointer);
    return dest;
}

static void doerror(const void *object, const char *s)
{
    char upbuf[MAXPDSTRING];
    upbuf[MAXPDSTRING-1]=0;

    if (sys_printhook || sys_printhook_error)
    {
        snprintf(upbuf, MAXPDSTRING-1, "error: %s", s);
        if (sys_printhook_error)
            (*sys_printhook_error)(upbuf);
        if (sys_printhook)
            (*sys_printhook)(upbuf);
    }
    else if (sys_printtostderr)
        fprintf(stderr, "error: %s", s);
    else
    {
        char obuf[MAXPDSTRING];
        gui_vmess("gui_post_error", "sis",
            strnpointerid(obuf, object, MAXPDSTRING), 1, s);
    }
}

static void dologpost(const void *object, const int level, const char *s)
{
    /* 1. s is at most MAXPDSTRING, but we're prepending a stupid header
       below. So for sanity, we first overallocate here to ensure the stupid
       header doesn't end up overflowing the buffer. */
    char upbuf[MAXPDSTRING];

    // what about sys_printhook_verbose ?
    if (sys_printhook) 
    {
        /* 2. The "n" in snprintf stands for "evil": we have to subtract one
           from total size so the null doesn't get truncated */ 
        snprintf(upbuf, MAXPDSTRING - 1, "verbose(%d): %s", level, s);
        /* 3. Finally, we add a null at UPBUFSZ-1 so that we end up with
           a string that fits inside MAXPDSTRING for use with t_symbol, etc.

           If anyone knows how I was *supposed* to do this safely within the
           constraints of C's stupid stdlib, please teach me... */
        upbuf[MAXPDSTRING-1]=0;
        (*sys_printhook)(upbuf);
    }
    else if (sys_printtostderr) 
    {
        fprintf(stderr, "verbose(%d): %s", level, s);
    }
    else
    {
        gui_vmess("gui_post", "s", s);
    }
}

static void dopost(const char *s)
{
    if (sys_printhook)
        (*sys_printhook)(s);
    else if (sys_printtostderr)
        fprintf(stderr, "%s", s);
    else
    {
        gui_vmess("gui_post", "s", s);
    }
}

void logpost(const void *object, const int level, const char *fmt, ...)
{
    char buf[MAXPDSTRING];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
    va_end(ap);
    strcat(buf, "\n");

    dologpost(object, level, buf);
}

void post(const char *fmt, ...)
{
    char buf[MAXPDSTRING];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
    va_end(ap);
    strcat(buf, "\n");
    dopost(buf);
}

void startpost(const char *fmt, ...)
{
    char buf[MAXPDSTRING];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
    va_end(ap);
    dopost(buf);
}

void poststring(const char *s)
{
    dopost(" ");
    dopost(s);
}

void postatom(int argc, t_atom *argv)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        char buf[MAXPDSTRING];
        atom_string(argv+i, buf, MAXPDSTRING);
        poststring(buf);
    }
}

void postfloat(t_float f)
{
    t_atom a;
    SETFLOAT(&a, f);
    postatom(1, &a);
}

void endpost(void)
{
    dopost("\n");
}

void error(const char *fmt, ...)
{
    char buf[MAXPDSTRING];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
    va_end(ap);
    strcat(buf, "\n");

    doerror(NULL, buf);
}

void verbose(int level, const char *fmt, ...)
{
    char buf[MAXPDSTRING];
    va_list ap;
    if(level>sys_verbose)return;
    
    va_start(ap, fmt);
    vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
    va_end(ap);
    strcat(buf, "\n");

        /* log levels for verbose() traditionally start at -3,
        so we have to adjust it before passing it on to dologpost() */
    dologpost(NULL, level + 3, buf);
}

    /* here's the good way to log errors -- keep a pointer to the
    offending or offended object around so the user can search for it
    later. */

static void *error_object;
static char error_string[256];
void canvas_finderror(void *object);

void pd_error(void *object, const char *fmt, ...)
{
    char buf[MAXPDSTRING];
    va_list ap;
    static int saidit;

    va_start(ap, fmt);
    vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
    va_end(ap);
    strcat(buf, "\n");

    doerror(object, buf);

    error_object = object;
    if (!saidit)
    {
        /* move this to a function in the GUI so that we can change the
           message without having to recompile */
        post("... click the link above to track it down, or click the 'Find Last Error' item in the Edit menu.");
        saidit = 1;
    }
}

void glob_finderror(t_pd *dummy)
{
    if (!error_object)
        post("no findable error yet.");
    else
    {
        post("last trackable error:");
        post("%s", error_string);
        canvas_finderror(error_object);
    }
}

void glob_findinstance(t_pd *dummy, t_symbol*s)
{
    // revert s to (potential) pointer to object
    t_int obj = 0;
    if (sscanf(s->s_name, ".x%zx", &obj))
    {
        if (obj)
        {
            canvas_finderror((void *)obj);
        }
    }
}

void bug(const char *fmt, ...)
{
    char buf[MAXPDSTRING];
    va_list ap;
    dopost("consistency check failed: ");
    va_start(ap, fmt);
    vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
    va_end(ap);
    strcat(buf, "\n");
    dopost(buf);
}

    /* this isn't worked out yet. */
static const char *errobject;
static const char *errstring;

void sys_logerror(const char *object, const char *s)
{
    errobject = object;
    errstring = s;
}

void sys_unixerror(const char *object)
{
    errobject = object;
    errstring = strerror(errno);
}

void sys_ouch(void)
{
    if (*errobject) error("%s: %s", errobject, errstring);
    else error("%s", errstring);
}
