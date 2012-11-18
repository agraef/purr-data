/* routeOSC.c 20060424 by Martin Peach, based on OSCroute and OSC-pattern-match.c. */
/* OSCroute.c header follows: */
/*
Written by Adrian Freed, The Center for New Music and Audio Technologies,
University of California, Berkeley.  Copyright (c) 1992,93,94,95,96,97,98,99,2000,01,02,03,04
The Regents of the University of California (Regents).

Permission to use, copy, modify, distribute, and distribute modified versions
of this software and its documentation without fee and without a signed
licensing agreement, is hereby granted, provided that the above copyright
notice, this paragraph and the following two paragraphs appear in all copies,
modifications, and distributions.

IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


The OSC webpage is http://cnmat.cnmat.berkeley.edu/OpenSoundControl
*/

/* OSC-route.c
  Max object for OSC-style dispatching

  To-do:

  	Match a pattern against a pattern?
      [Done: Only Slash-Star is allowed, see MyPatternMatch.]
  	Declare outlet types / distinguish leaf nodes from other children
  	More sophisticated (2-pass?) allmessages scheme
  	set message?


	pd
	-------------
		-- tweaks for Win32    www.zeggz.com/raf	13-April-2002

*/
/* OSC-pattern-match.c header follows: */
/*
Copyright © 1998. The Regents of the University of California (Regents).
All Rights Reserved.

Written by Matt Wright, The Center for New Music and Audio Technologies,
University of California, Berkeley.

Permission to use, copy, modify, distribute, and distribute modified versions
of this software and its documentation without fee and without a signed
licensing agreement, is hereby granted, provided that the above copyright
notice, this paragraph and the following two paragraphs appear in all copies,
modifications, and distributions.

IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

The OpenSound Control WWW page is
    http://www.cnmat.berkeley.edu/OpenSoundControl
*/



/*
    OSC-pattern-match.c
    Matt Wright, 3/16/98
    Adapted from oscpattern.c, by Matt Wright and Amar Chaudhury
*/

/* the required include files */
#include "m_pd.h"

#define MAX_NUM 128 // maximum number of paths (prefixes) we can route

typedef struct _routeOSC
{
    t_object    x_obj; // required header
    t_int       x_num; // Number of prefixes we store
    char        *x_prefixes[MAX_NUM];
    void        *x_outlets[MAX_NUM+1]; // one for each prefix plus one for everything else
} t_routeOSC;

/* prototypes  */

void routeOSC_setup(void);
static int MyPatternMatch (const char *pattern, const char *test);
static void routeOSC_doanything(t_routeOSC *x, t_symbol *s, int argc, t_atom *argv);
static void routeOSC_list(t_routeOSC *x, t_symbol *s, int argc, t_atom *argv);
static void *routeOSC_new(t_symbol *s, int argc, t_atom *argv);
static void routeOSC_set(t_routeOSC *x, t_symbol *s, int argc, t_atom *argv);
static char *NextSlashOrNull(char *p);
static void StrCopyUntilSlash(char *target, const char *source);

/* from
    OSC-pattern-match.c
*/
static const char *theWholePattern;	/* Just for warning messages */
static int MatchBrackets (const char *pattern, const char *test);
static int MatchList (const char *pattern, const char *test);
static int PatternMatch (const char *  pattern, const char * test);

static t_class *routeOSC_class;
t_symbol *ps_list, *ps_complain, *ps_emptySymbol;

static int MyPatternMatch (const char *pattern, const char *test)
{
    // This allows the special case of "routeOSC /* " to be an outlet that
    // matches anything; i.e., it always outputs the input with the first level
    // of the address stripped off.

    if (test[0] == '*' && test[1] == '\0') return 1;
    else return PatternMatch(pattern, test);
}

static void routeOSC_free(t_routeOSC *x)
{
}

/* initialization routine */
// setup
void routeOSC_setup(void)
{
    routeOSC_class = class_new(gensym("routeOSC"), (t_newmethod)routeOSC_new,
        (t_method)routeOSC_free,sizeof(t_routeOSC), 0, A_GIMME, 0);
    class_addlist(routeOSC_class, routeOSC_list);
    class_addanything(routeOSC_class, routeOSC_doanything);
    class_addmethod(routeOSC_class, (t_method)routeOSC_set, gensym("set"), A_GIMME, 0);

    ps_emptySymbol = gensym("");

//    post("routeOSC object version 1.0 by Martin Peach, based on OSCroute by Matt Wright. pd: jdl Win32 raf.");
//    post("OSCroute Copyright © 1999 Regents of the Univ. of California. All Rights Reserved.");
}

/* instance creation routine */
static void *routeOSC_new(t_symbol *s, int argc, t_atom *argv)
{

    t_routeOSC *x = (t_routeOSC *)pd_new(routeOSC_class);   // get memory for a new object & initialize
    int i;

    if (argc > MAX_NUM)
    {
        error("* routeOSC: too many arguments: %d (max %d)", argc, MAX_NUM);
        return 0;
    }
    x->x_num = 0;
    for (i = 0; i < argc; ++i)
    {
        if (argv[i].a_type == A_SYMBOL)
        {
            if (argv[i].a_w.w_symbol->s_name[0] == '/')
            { /* Now that's a nice prefix */
                x->x_prefixes[i] = argv[i].a_w.w_symbol->s_name;
                ++(x->x_num);
            }
        }
        else if (argv[i].a_type == A_FLOAT)
        {
            error("* routeOSC: float arguments are not OK.");
            return 0;
        }
        else
        {
            error("* routeOSC: unrecognized argument type!");
            return 0;
        }
    }
    /* Have to create the outlets in reverse order */
    /* well, not in pd ? */
    for (i = 0; i <= x->x_num; i++)
    {
        x->x_outlets[i] = outlet_new(&x->x_obj, &s_list);
    }
    return (x);
}

static void routeOSC_set(t_routeOSC *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;

	if (argc > x->x_num)
    {
        pd_error (x, "routeOSC: too many paths");
        return;
    }
    for (i = 0; i < argc; ++i)
    {
        if (argv[i].a_type != A_SYMBOL)
        {
            pd_error (x, "routeOSC: path %d not a symbol", i);
            return;
        }
        if (argv[i].a_w.w_symbol->s_name[0] != '/')
        {
            pd_error (x, "routeOSC: path %d doesn't start with /", i);
            return;
        }
    }
    for (i = 0; i < argc; ++i)
    {
        if (argv[i].a_w.w_symbol->s_name[0] == '/')
        { /* Now that's a nice prefix */
            x->x_prefixes[i] = argv[i].a_w.w_symbol->s_name;
        }
    }
}

static void routeOSC_list(t_routeOSC *x, t_symbol *s, int argc, t_atom *argv)
{
    if(argc < 1)
    {
      pd_error(x, "* routeOSC: ignoring empty list...");
      return;
    }
    if (argv[0].a_type == A_SYMBOL)
    { 
        /* Ignore the fact that this is a "list" */
        routeOSC_doanything(x, argv[0].a_w.w_symbol, argc-1, argv+1);
    }
    else
    {
        // pd_error(x, "* OSC-route: invalid list beginning with a number");
        // output on unmatched outlet jdl 20020908
        if (argv[0].a_type == A_FLOAT)
        {
          outlet_float(x->x_outlets[x->x_num], atom_getfloat(argv));
        }
        else
        {
          pd_error(x, "* routeOSC: unrecognized atom type!");
        }
    }
}

static void routeOSC_doanything(t_routeOSC *x, t_symbol *s, int argc, t_atom *argv)
{
    char *pattern, *nextSlash;
    int i;
    int matchedAnything;
    // post("*** routeOSC_anything(s %s, argc %ld)", s->s_name, (long) argc);

    pattern = s->s_name;
    if (pattern[0] != '/')
    {
        pd_error(x, "* routeOSC: invalid message pattern %s does not begin with /", s->s_name);
        outlet_anything(x->x_outlets[x->x_num], s, argc, argv);
        return;
    }
    matchedAnything = 0;

	nextSlash = NextSlashOrNull(pattern+1);
    if (*nextSlash == '\0')
    {
        /* last level of the address, so we'll output the argument list */
    
        for (i = 0; i < x->x_num; ++i)
        {
            if (MyPatternMatch(pattern+1, x->x_prefixes[i]+1))
            {
                ++matchedAnything;
                // I hate stupid Max lists with a special first element
                if (argc == 0)
                {
                    outlet_bang(x->x_outlets[i]);
                }
                else if (argv[0].a_type == A_SYMBOL)
                {
                    // Promote the symbol that was argv[0] to the special symbol
                    outlet_anything(x->x_outlets[i], argv[0].a_w.w_symbol, argc-1, argv+1);
                }
                else if (argc > 1)
                {
                    // Multiple arguments starting with a number, so naturally we have
                    // to use a special function to output this "list", since it's what
                    // Max originally meant by "list".
                    outlet_list(x->x_outlets[i], 0L, argc, argv);
                }
                else
                {
                    // There was only one argument, and it was a number, so we output it
                    // not as a list
                    if (argv[0].a_type == A_FLOAT)
                    {
                        outlet_float(x->x_outlets[i], argv[0].a_w.w_float);
                    }
                    else
                    {
                        pd_error(x, "* routeOSC: unrecognized atom type!");
                    }
                }
            }
        }
    }
    else
    {
        /* There's more address after this part, so our output list will begin with
           the next slash.  */
        t_symbol *restOfPattern = 0; /* avoid the gensym unless we have to output */
        char patternBegin[1000];

        /* Get the first level of the incoming pattern to match against all our prefixes */
        StrCopyUntilSlash(patternBegin, pattern+1);

        for (i = 0; i < x->x_num; ++i)
        {
            if (MyPatternMatch(patternBegin, x->x_prefixes[i]+1))
            {
                ++matchedAnything;
                if (restOfPattern == 0) restOfPattern = gensym(nextSlash);
                outlet_anything(x->x_outlets[i], restOfPattern, argc, argv);
            }
        }
    }

    if (!matchedAnything)
    {
        // output unmatched data on rightmost outlet a la normal 'route' object, jdl 20020908
        outlet_anything(x->x_outlets[x->x_num], s, argc, argv);
	}
}

static char *NextSlashOrNull(char *p)
{
    while (*p != '/' && *p != '\0') p++;
    return p;
}

static void StrCopyUntilSlash(char *target, const char *source)
{
    while (*source != '/' && *source != '\0')
    {
        *target = *source;
        ++target;
        ++source;
    }
    *target = 0;
}

/* from
    OSC-pattern-match.c
    Matt Wright, 3/16/98
    Adapted from oscpattern.c, by Matt Wright and Amar Chaudhury
*/

static int PatternMatch (const char *  pattern, const char * test)
{
    theWholePattern = pattern;
  
    if (pattern == 0 || pattern[0] == 0) return test[0] == 0;
  
    if (test[0] == 0)
    {
        if (pattern[0] == '*') return PatternMatch (pattern+1, test);
        return 0;
    }

    switch (pattern[0])
    {
        case 0:
            return test[0] == 0;
        case '?':
            return PatternMatch (pattern+1, test+1);
        case '*': 
            if (PatternMatch (pattern+1, test)) return 1;
            return PatternMatch (pattern, test+1);
        case ']':
        case '}':
			      error("routeOSC: Spurious %c in pattern \".../%s/...\"",pattern[0], theWholePattern);
            return 0;
        case '[':
            return MatchBrackets (pattern,test);
        case '{':
            return MatchList (pattern,test);
        case '\\':  
            if (pattern[1] == 0) return test[0] == 0;
            if (pattern[1] == test[0]) return PatternMatch (pattern+2,test+1);
            return 0;
        default:
            if (pattern[0] == test[0]) return PatternMatch (pattern+1,test+1);
            return 0;
    }
}

/* we know that pattern[0] == '[' and test[0] != 0 */

static int MatchBrackets (const char *pattern, const char *test) 
{
    int result;
    int negated = 0;
    const char *p = pattern;

    if (pattern[1] == 0) 
    {
        error("routeOSC: Unterminated [ in pattern \".../%s/...\"", theWholePattern);
        return 0;
    }
    if (pattern[1] == '!')
    {
        negated = 1;
        p++;
    }
    while (*p != ']')
    {
        if (*p == 0) 
        {
            error("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
            return 0;
        }
        if (p[1] == '-' && p[2] != 0) 
        {
            if (test[0] >= p[0] && test[0] <= p[2]) 
            {
                result = !negated;
                goto advance;
            }
        }
        if (p[0] == test[0]) 
        {
            result = !negated;
            goto advance;
        }
        p++;
    }
    result = negated;
advance:
    if (!result) return 0;
    while (*p != ']') 
    {
        if (*p == 0) 
        {
            error("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
            return 0;
        }
        p++;
    }
    return PatternMatch (p+1,test+1);
}

static int MatchList (const char *pattern, const char *test) 
{
    const char *restOfPattern, *tp = test;

    for(restOfPattern = pattern; *restOfPattern != '}'; restOfPattern++) 
    {
        if (*restOfPattern == 0) 
        {
            error("Unterminated { in pattern \".../%s/...\"", theWholePattern);
            return 0;
        }
    }
    restOfPattern++; /* skip close curly brace */
    pattern++; /* skip open curly brace */
    while (1)
	{
        if (*pattern == ',') 
		{
            if (PatternMatch (restOfPattern, tp)) return 1;
            tp = test;
            ++pattern;
		}
        else if (*pattern == '}') return PatternMatch (restOfPattern, tp);
        else if (*pattern == *tp) 
        {
            ++pattern;
            ++tp;
        }
        else 
		{
            tp = test;
            while (*pattern != ',' && *pattern != '}') pattern++;
            if (*pattern == ',') pattern++;
        }
    }
}

/* end of routeOSC.c */
