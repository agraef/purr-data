/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <stdio.h>
#include <string.h>

#define DOLLARALL -0x7fffffff /* defined in m_binbuf.c, too. Consider merging */


#if PD_FLOATSIZE == 32
#define M_ATOM_FLOAT_SPECIFIER "%.6g"
#elif PD_FLOATSIZE == 64
#define M_ATOM_FLOAT_SPECIFIER "%.14lg"
#endif

    /* convenience routines for checking and getting values of
        atoms.  There's no "pointer" version since there's nothing
        safe to return if there's an error. */

t_float atom_getfloat(t_atom *a)
{
    if (a->a_type == A_FLOAT) return (a->a_w.w_float);
    else return (0);
}

t_int atom_getint(t_atom *a)
{
    return (atom_getfloat(a));
}

t_symbol *atom_getsymbol(t_atom *a)  /* LATER think about this more carefully */
{
    if (a->a_type == A_SYMBOL) return (a->a_w.w_symbol);
    else return (&s_float);
}

t_blob *atom_getblob(t_atom *a)  /* MP 20070108 */
{
    static unsigned char c = 0;/* a default blob to avoid null pointers. This should be somewhere else...? */
    static t_blob st = {1L, &c};
    if (a->a_type == A_BLOB) return (a->a_w.w_blob);
    else return (&st);
}

t_symbol *atom_gensym(t_atom *a)  /* this works better for graph labels */
{
    char buf[30];
    if (a->a_type == A_SYMBOL) return (a->a_w.w_symbol);
    else if (a->a_type == A_FLOAT)
        sprintf(buf, "%g", a->a_w.w_float);
    else strcpy(buf, "???");
    return (gensym(buf));
}

t_float atom_getfloatarg(int which, int argc, t_atom *argv)
{
    if (argc <= which) return (0);
    argv += which;
    if (argv->a_type == A_FLOAT) return (argv->a_w.w_float);
    else return (0);
}

t_int atom_getintarg(int which, int argc, t_atom *argv)
{
    return (atom_getfloatarg(which, argc, argv));
}

t_symbol *atom_getsymbolarg(int which, int argc, t_atom *argv)
{
    if (argc <= which) return (&s_);
    argv += which;
    if (argv->a_type == A_SYMBOL) return (argv->a_w.w_symbol);
    else return (&s_);
}

/* convert an atom into a string, in the reverse sense of binbuf_text (q.v.)
* special attention is paid to symbols containing the special characters
* ';', ',', '$', and '\'; these are quoted with a preceding '\', except that
* the '$' only gets quoted if followed by a digit.
*/

// ag 20240916 XXXFIXME: Option to suppress escaping spaces. This flag is
// normally 1 (enable backslash-quoting of space characters), but at present
// we need to disable it for listboxes because these use spaces as a
// delimiter. It's a kludge to have this as a global variable, but turning it
// into a parameter would be even clumsier and require changes in public APIs,
// which we don't want. Thus, this will have to do until we fix our listbox
// implementation to make it work with symbols containing spaces.
int atom_quote_spaces = 1;

void atom_string(t_atom *a, char *buf, unsigned int bufsize)
{
    char tbuf[30];
    switch(a->a_type)
    {
    case A_SEMI: strcpy(buf, ";"); break;
    case A_COMMA: strcpy(buf, ","); break;
    case A_POINTER:
        strcpy(buf, "(pointer)");
        break;
    case A_FLOAT:
        sprintf(tbuf, M_ATOM_FLOAT_SPECIFIER, a->a_w.w_float);
        if (strlen(tbuf) < bufsize-1) strcpy(buf, tbuf);
        else if (a->a_w.w_float < 0) strcpy(buf, "-");
        else  strcat(buf, "+");
        break;
    case A_SYMBOL:
    case A_DOLLSYM:
    {
        char *sp;
        unsigned int len;
        int quote, a_sym = a->a_type == A_SYMBOL;
        if(a_sym && !strcmp(a->a_w.w_symbol->s_name, "$@")) /* JMZ: #@ quoting */
            quote=1;
        else
        {
            for (sp = a->a_w.w_symbol->s_name, len = 0, quote = 0; *sp; sp++, len++)
                if (*sp == ';' || *sp == ',' || *sp == '\\' ||
                    (atom_quote_spaces && *sp == ' ') ||
                    (a_sym && *sp == '$' && sp[1] >= '0' && sp[1] <= '9'))
                    quote = 1;
        }
        if (quote)
        {
            char *bp = buf, *ep = buf + (bufsize-2);
            sp = a->a_w.w_symbol->s_name;
            while (bp < ep && *sp)
            {
                if (*sp == ';' || *sp == ',' || *sp == '\\' ||
                    (atom_quote_spaces && *sp == ' ') ||
                    (a_sym && *sp == '$' && ((sp[1] >= '0' && sp[1] <= '9') || sp[1]=='@')))
                        *bp++ = '\\';
                *bp++ = *sp++;
            }
            if (*sp) *bp++ = '*';
            *bp = 0;
            /* post("quote %s -> %s", a->a_w.w_symbol->s_name, buf); */
        }
        else
        {
            if (len < bufsize-1) strcpy(buf, a->a_w.w_symbol->s_name);
            else
            {
                strncpy(buf, a->a_w.w_symbol->s_name, bufsize - 2);
                strcpy(buf + (bufsize - 2), "*");
            }
        }
    }
        break;
    case A_DOLLAR:
        if(a->a_w.w_index == DOLLARALL)
        {
            /* JMZ: $@ expansion */
            sprintf(buf, "$@");
        }
        else
        {
            sprintf(buf, "$%d", a->a_w.w_index);
        }
        break;
    default:
        bug("atom_string");
    }
}
