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
int sys_printtostderr;

/* escape characters for tcl/tk */
static char* strnescape(char *dest, const char *src, size_t len)
{
    int ptin = 0;
    unsigned ptout = 0;
    for(; ptout < len; ptin++, ptout++)
    {
        int c = src[ptin];
        if (c == '\\' || c == '{' || c == '}' || c == ';')
            dest[ptout++] = '\\';
        dest[ptout] = src[ptin];
        if (c==0) break;
    }

    if(ptout < len)
        dest[ptout]=0;
    else
        dest[len-1]=0;

    return dest;
}

static char* strnpointerid(char *dest, const void *pointer, size_t len)
{
    *dest=0;
    if (pointer)
        snprintf(dest, len, ".x%lx", (unsigned long)pointer);
    return dest;
}

static void doerror(const void *object, const char *s)
{
    char upbuf[MAXPDSTRING];
    upbuf[MAXPDSTRING-1]=0;

    // what about sys_printhook_error ?
    if (sys_printhook)
    {
        snprintf(upbuf, MAXPDSTRING-1, "error: %s", s);
        (*sys_printhook)(upbuf);
    }
    else if (sys_printtostderr)
        fprintf(stderr, "error: %s", s);
    else
    {
        char obuf[MAXPDSTRING];
        //sys_vgui("pdtk_posterror {%s} 1 {%s}\n",
        //    strnpointerid(obuf, object, MAXPDSTRING),
        //    strnescape(upbuf, s, MAXPDSTRING));
        gui_vmess("gui_post_error", "sis",
            strnpointerid(obuf, object, MAXPDSTRING),
            1,
            strnescape(upbuf, s, MAXPDSTRING));
    }
}

static void dologpost(const void *object, const int level, const char *s)
{
    char upbuf[MAXPDSTRING];
    upbuf[MAXPDSTRING-1]=0;

    // what about sys_printhook_verbose ?
    if (sys_printhook) 
    {
        snprintf(upbuf, MAXPDSTRING-1, "verbose(%d): %s", level, s);
        (*sys_printhook)(upbuf);
    }
    else if (sys_printtostderr) 
    {
        fprintf(stderr, "verbose(%d): %s", level, s);
    }
    else
    {
        //sys_vgui("::pdwindow::logpost {%s} %d {%s}\n", 
                 //strnpointerid(obuf, object, MAXPDSTRING), 
                 //level, strnescape(upbuf, s, MAXPDSTRING));
        //sys_vgui("pdtk_post {%s}\n", 
        //         strnescape(upbuf, s, MAXPDSTRING));
        gui_vmess("gui_post", "s",
                 strnescape(upbuf, s, MAXPDSTRING));
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
        char upbuf[MAXPDSTRING];
        int ptin = 0, ptout = 0, len = strlen(s);
        //static int heldcr = 0;
        //if (heldcr)
        //    upbuf[ptout++] = '\n', heldcr = 0;
        for (; ptin < len && ptout < MAXPDSTRING-3;
            ptin++, ptout++)
        {
            int c = s[ptin];
            if (c == '\\' || c == '{' || c == '}' || c == ';')
                upbuf[ptout++] = '\\';
            upbuf[ptout] = s[ptin];
        }
        //if (ptout && upbuf[ptout-1] == '\n')
        //    upbuf[--ptout] = 0, heldcr = 1;
        upbuf[ptout] = 0;
//        sys_vgui("pdtk_post {%s}\n", upbuf);
        gui_vmess("gui_post", "s", upbuf);
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
    dopost("verbose(");
    postfloat((t_float)level);
    dopost("):");
    
    va_start(ap, fmt);
    vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
    va_end(ap);
    strcat(buf, "\n");
    dopost(buf);
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
    long obj = 0;
    if (sscanf(s->s_name, ".x%lx", &obj))
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
