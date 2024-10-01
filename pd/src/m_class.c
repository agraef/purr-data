/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "config.h"

#define PD_CLASS_DEF
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "m_private_utils.h"

t_symbol *class_loadsym;     /* name under which an extern is invoked */
static void pd_defaultfloat(t_pd *x, t_float f);
static void pd_defaultblob(t_pd *x, t_blob *st); /* MP20061226 blob type */
static void pd_defaultlist(t_pd *x, t_symbol *s, int argc, t_atom *argv);
t_pd pd_objectmaker;    /* factory for creating "object" boxes */
t_pd pd_canvasmaker;    /* factory for creating canvases */

typedef struct _classtable
{
    t_class *ct_class;
    struct _classtable *ct_next;
} t_classtable;

t_classtable *ct;

static t_symbol *class_extern_dir = &s_;

int symbol_can_float(t_symbol *s, t_float *f);

    /* try to give the user some help with the uglier cases of unexpected
       atom types in messages.

       dostof flag controls whether we want to try to convert a symbol
       to a float.

       for [float]'s symbol conversion we even check if we received an
       out of range error, but here for the general case we don't do that. */
char *type_hint(t_symbol *s, int argc, t_atom *argv, int dostof)
{
    static char hint[MAXPDSTRING];
    t_float f = 0;

        /* Null selectors-- the user typically shouldn't encounter these,
           especially one that triggers an unknown method error. But we
           check for them anyway. */
    if (!s)
    {
        sprintf(hint, " (Note: null selector detected)");
        return hint;
    }

        /* Empty symbol selector is also rare. But a user can easily generate
           one with [symbol( or [symbol] then send to [list trim]. */
    if (s == &s_)
    {
        sprintf(hint, " (Note: empty symbol selector detected)");
        return hint;
    }

        /* More commonly, the user may have a "symbol" message where the
           payload is an empty symbol. This can cause confusion for debugging
           since the empty symbol doesn't print out anything. */
    if (s && s == &s_symbol && argc && argv->a_type == A_SYMBOL
            && argv->a_w.w_symbol == &s_)
    {
        sprintf(hint, " (Note: symbol message with empty payload detected)");
        return hint;
    }

        /* Somewhat common edge case-- [makefilename] and other objects can
           output a symbol message with a payload that looks numeric but is
           indeed a symbol atom. In fact, Pd's text parser would interpret
           such a string as a float if you sent it through the parser.

           This can cause problems if the user tries to serialize the data
           and read it back-- say, by saving the symbol message "symbol 123"
           in a file. The next time they load it, "123" will be parsed as
           a float atom, and any symbol methods will read from the wrong
           union field and (probably) interpret it as an empty symbol. */
    if (dostof)
    {
        if (symbol_can_float(atom_getsymbolarg(0, argc, argv), &f))
        {
            if (s == &s_symbol)
                sprintf(hint, " (Warning: symbol message with numeric payload "
                "detected. This data cannot be saved properly in a patch.");
            else
                sprintf(hint, " (Note: '%s' is actually a symbol atom, not "
                    "a float)",
                    argv->a_w.w_symbol->s_name);
            return hint;
        }
        else if (f == -1 || f == 1)
        {
                /* For values which would overflow, give a hint but don't
                   suggest float type */
            if (s == &s_symbol)
                sprintf(hint, " (Note: this symbol message has an %s floatlike "
                    "payload which cannot be saved properly.",
                    f == 1 ? "overflowing" : "underflowing");
            else
                sprintf(hint, " (Note: '%s' is actually a symbol atom. If you "
                    "save it Pd will parse it as a float and cause an %s "
                    "error.",
                    argv->a_w.w_symbol->s_name,
                    f == 1 ? "overflow" : "underflow");
            return hint;
        }
    }

        /* Rather uncommon case where the selector itself is a symbol atom
           that would normally have been parsed as a float. */
    if (dostof)
    {
        if (symbol_can_float(s, &f))
        {
            sprintf(hint, " (Note: %s looks like a float but is actually a "
                "symbol atom which cannot be saved properly)", s->s_name);
            return hint;
        }
        else if (f == -1 || f == 1)
        {
                /* For values which would overflow, give a hint but don't
                   suggest float type */
            sprintf(hint, " (Note: the symbol atom '%s' has an %s floatlike "
                "payload which cannot be saved properly.",
                s->s_name,
                f == 1 ? "overflowing" : "underflowing");
            return hint;
        }
    }

        /* Now that we've checked for symbols that could be floats, let's
           catch the generic case where a user entered a symbol payload for a
           "float" message. This can be typed into a messsage box, for
           example. We also check for empty symbol payload here, and other
           odd atom types */
    if (s && s == &s_float && argc && argv->a_type != A_FLOAT)
    {
        if (argv->a_type == A_SYMBOL && argv->a_w.w_symbol == &s_)
            sprintf(hint, " (Expected a float argument but got empty symbol)");
        else if (argv->a_type == A_SYMBOL)
            sprintf(hint, " (Expected a float argument but got '%s')",
                argv->a_w.w_symbol->s_name);
        else if (argv->a_type == A_POINTER)
            sprintf(hint, " (Expected a float argument but got a gpointer)");
        else
            sprintf(hint, " (Note: got an argument that's not a float)");
        return hint;
    }

    hint[0] = '\0';
    return hint;
}

static void pd_defaultanything(t_pd *x, t_symbol *s, int argc, t_atom *argv)
{
    pd_error(x, "%s: no method for '%s'%s",
        (*x)->c_name->s_name, s->s_name, type_hint(s, argc, argv,
            *(*x)->c_floatmethod != pd_defaultfloat ? 1 : 0));
}

static void pd_defaultbang(t_pd *x)
{
    if (*(*x)->c_listmethod != pd_defaultlist)
        (*(*x)->c_listmethod)(x, &s_bang, 0, 0);
    else (*(*x)->c_anymethod)(x, &s_bang, 0, 0);
}

static void pd_defaultblob(t_pd *x, t_blob *st) /* MP 20061226 blob type */
{ /* for now just reject it, later convert to symbol/float/list */
    pd_error(x, "%s: no method for blob so far...", (*x)->c_name->s_name);
}

static void pd_defaultpointer(t_pd *x, t_gpointer *gp)
{
    if (*(*x)->c_listmethod != pd_defaultlist)
    {
        t_atom at;
        SETPOINTER(&at, gp);
        (*(*x)->c_listmethod)(x, &s_pointer, 1, &at);
    }
    else
    {
        t_atom at;
        SETPOINTER(&at, gp);
        (*(*x)->c_anymethod)(x, &s_pointer, 1, &at);
    }
}

static void pd_defaultfloat(t_pd *x, t_float f)
{
    if (*(*x)->c_listmethod != pd_defaultlist)
    {
        t_atom at;
        SETFLOAT(&at, f);
        (*(*x)->c_listmethod)(x, &s_float, 1, &at);
    }
    else
    {
        t_atom at;
        SETFLOAT(&at, f);
        (*(*x)->c_anymethod)(x, &s_float, 1, &at);
    }
}

static void pd_defaultsymbol(t_pd *x, t_symbol *s)
{
    if (*(*x)->c_listmethod != pd_defaultlist)
    {
        t_atom at;
        SETSYMBOL(&at, s);
        (*(*x)->c_listmethod)(x, &s_symbol, 1, &at);
    }
    else
    {
        t_atom at;
        SETSYMBOL(&at, s);
        (*(*x)->c_anymethod)(x, &s_symbol, 1, &at);
    }
}

void obj_list(t_object *x, t_symbol *s, int argc, t_atom *argv);
static void class_nosavefn(t_gobj *z, t_binbuf *b);

    /* handle "list" messages to Pds without explicit list methods defined. */
static void pd_defaultlist(t_pd *x, t_symbol *s, int argc, t_atom *argv)
{
            /* a list with no elements is handled by the 'bang' method if
            one exists. */
    if (argc == 0 && *(*x)->c_bangmethod != pd_defaultbang)
    {
        (*(*x)->c_bangmethod)(x);
        return;
    }
            /* a list with one element which is a number can be handled by a
            "float" method if any is defined; same for "symbol", "pointer". */
    if (argc == 1)
    {
        if (argv->a_type == A_FLOAT &&
        *(*x)->c_floatmethod != pd_defaultfloat)
        {
            (*(*x)->c_floatmethod)(x, argv->a_w.w_float);
            return;
        }
        else if (argv->a_type == A_SYMBOL &&
            *(*x)->c_symbolmethod != pd_defaultsymbol)
        {
            (*(*x)->c_symbolmethod)(x, argv->a_w.w_symbol);
            return;
        }
        else if (argv->a_type == A_POINTER &&
            *(*x)->c_pointermethod != pd_defaultpointer)
        {
            (*(*x)->c_pointermethod)(x, argv->a_w.w_gpointer);
            return;
        }
    }
        /* Next try for an "anything" method */
    if ((*x)->c_anymethod != pd_defaultanything)
        (*(*x)->c_anymethod)(x, &s_list, argc, argv);

        /* if the object is patchable (i.e., can have proper inlets)
            send it on to obj_list which will unpack the list into the inlets */
    else if (argc>0 && (*x)->c_patchable)
        obj_list((t_object *)x, s, argc, argv);
            /* otherwise give up and complain. */
    else pd_defaultanything(x, &s_list, argc, argv);
}

    /* for now we assume that all "gobjs" are text unless explicitly
    overridden later by calling class_setbehavior().  I'm not sure
    how to deal with Pds that aren't gobjs; shouldn't there be a
    way to check that at run time?  Perhaps the presence of a "newmethod"
    should be our cue, or perhaps the "tiny" flag.  */

    /* another matter.  This routine does two unrelated things: it creates
    a Pd class, but also adds a "new" method to create an instance of it.
    These are combined for historical reasons and for brevity in writing
    objects.  To avoid adding a "new" method send a null function pointer.
    To add additional ones, use class_addcreator below.  Some "classes", like
    "select", are actually two classes of the same name, one for the single-
    argument form, one for the multiple one; see select_setup() to find out
    how this is handled.  */

extern void text_save(t_gobj *z, t_binbuf *b);

void classtable_register(t_class *c)
{
    t_classtable *t;
    for(t = ct; t; t = t->ct_next)
        if (t->ct_class == c) post("already registered %s", c->c_name->s_name);
    t = (t_classtable *)t_getbytes(sizeof(*t));
    t->ct_class = c;
    t->ct_next = ct;
    ct = t;
}

int classtable_size(void) {
    t_classtable *t;
    int i;
    for(t = ct, i = 0; t; t = t->ct_next)
        i++;
    return i;
}

void classtable_tovec(int size, t_atom *vec)
{
    t_classtable *t;
    int i;
    for(t = ct, i = 0; t && i < size; t = t->ct_next, i++)
        if (!t->ct_class->c_name)
        {
            SETSYMBOL(vec+i, gensym("anonymous-class"));
        }
        else
            SETSYMBOL(vec+i, t->ct_class->c_name);
}

// todo-- make accessors so m_imp.h isn't needed by x_interface.c

t_class *classtable_findbyname(t_symbol *s)
{
    t_classtable *t;
    for (t = ct; t; t = t->ct_next)
        if (t->ct_class->c_name == s)
            return t->ct_class;
    return NULL;
}

t_class *class_new(t_symbol *s, t_newmethod newmethod, t_method freemethod,
    size_t size, int flags, t_atomtype type1, ...)
{
    va_list ap;
    t_atomtype vec[MAXPDARG+1], *vp = vec;
    int count = 0;
    t_class *c;
    int typeflag = flags & CLASS_TYPEMASK;
    if (!typeflag) typeflag = CLASS_PATCHABLE;
    *vp = type1;

    va_start(ap, type1);
    while (*vp)
    {
        if (count == MAXPDARG)
        {
            error("class %s: sorry: only %d args typechecked; use A_GIMME",
                s->s_name, MAXPDARG);
            break;
        }
        vp++;
        count++;
        *vp = va_arg(ap, t_atomtype);
    }
    va_end(ap);
    if (pd_objectmaker && newmethod)
    {
            /* add a "new" method by the name specified by the object */
        class_addmethod(pd_objectmaker, (t_method)newmethod, s,
            vec[0], vec[1], vec[2], vec[3], vec[4], vec[5]);
        if (class_loadsym)
        {
                /* if we're loading an extern it might have been invoked by a
                longer file name; in this case, make this an admissible name
                too. */
            char *loadstring = class_loadsym->s_name;
            int l1 = strlen(s->s_name), l2 = strlen(loadstring);
            if (l2 > l1 && !strcmp(s->s_name, loadstring + (l2 - l1)))
                class_addmethod(pd_objectmaker, (t_method)newmethod,
                    class_loadsym,
                    vec[0], vec[1], vec[2], vec[3], vec[4], vec[5]);
        }
    }
    c = (t_class *)t_getbytes(sizeof(*c));
    c->c_name = c->c_helpname = s;
    c->c_size = size;
    c->c_methods = t_getbytes(0);
    c->c_nmethod = 0;
    c->c_freemethod = (t_method)freemethod;
    c->c_bangmethod = pd_defaultbang;
    c->c_pointermethod = pd_defaultpointer;
    c->c_floatmethod = pd_defaultfloat;
    c->c_symbolmethod = pd_defaultsymbol;
    c->c_blobmethod = pd_defaultblob; /* MP 20061226 blob type */
    c->c_listmethod = pd_defaultlist;
    c->c_anymethod = pd_defaultanything;
    /* set default widget behavior.  Things like IEM GUIs override
        this; they're patchable but have bespoke widget behaviors */
    c->c_wb = (typeflag == CLASS_PATCHABLE ? &text_widgetbehavior : 0);
    c->c_pwb = 0;
    c->c_firstin = ((flags & CLASS_NOINLET) == 0);
    c->c_patchable = (typeflag == CLASS_PATCHABLE);
    c->c_gobj = (typeflag >= CLASS_GOBJ);
    c->c_multichannel = (flags & CLASS_MULTICHANNEL) != 0;
    c->c_nopromotesig = (flags & CLASS_NOPROMOTESIG) != 0;
    c->c_nopromoteleft = (flags & CLASS_NOPROMOTELEFT) != 0;
    c->c_drawcommand = 0;
    c->c_floatsignalin = 0;
    c->c_externdir = class_extern_dir;
    c->c_savefn = (typeflag == CLASS_PATCHABLE ? text_save : class_nosavefn);
#if 0 
    post("class: %s", c->c_name->s_name);
#endif
    classtable_register(c);
//    post("class: %s", c->c_name->s_name);
    return (c);
}

    /* add a creation method, which is a function that returns a Pd object
    suitable for putting in an object box.  We presume you've got a class it
    can belong to, but this won't be used until the newmethod is actually
    called back (and the new method explicitly takes care of this.) */

void class_addcreator(t_newmethod newmethod, t_symbol *s, 
    t_atomtype type1, ...)
{
    va_list ap;
    t_atomtype vec[MAXPDARG+1], *vp = vec;
    int count = 0;
    *vp = type1;

    va_start(ap, type1);
    while (*vp)
    {
        if (count == MAXPDARG)
        {
            error("class %s: sorry: only %d creation args allowed",
                s->s_name, MAXPDARG);
            break;
        }
        vp++;
        count++;
        *vp = va_arg(ap, t_atomtype);
    } 
    va_end(ap);
    class_addmethod(pd_objectmaker, (t_method)newmethod, s,
        vec[0], vec[1], vec[2], vec[3], vec[4], vec[5]);
    if (class_loadsym)
    {
            /* if we're loading an extern it might have been invoked by a
            longer file name; in this case, make this an admissible name
            too. */
        char *loadstring = class_loadsym->s_name,
            l1 = strlen(s->s_name), l2 = strlen(loadstring);
        if (l2 > l1 && !strcmp(s->s_name, loadstring + (l2 - l1)))
            class_addmethod(pd_objectmaker, (t_method)newmethod,
                class_loadsym,
                vec[0], vec[1], vec[2], vec[3], vec[4], vec[5]);
    }
}

void class_addmethod(t_class *c, t_method fn, t_symbol *sel,
    t_atomtype arg1, ...)
{
    va_list ap;
    t_methodentry *m;
    t_atomtype argtype = arg1;
    int nargs;
    
    if (!c)
    {
        bug("class_addmethod");
        return;
    }
    va_start(ap, arg1);
        /* "signal" method specifies that we take audio signals but
        that we don't want automatic float to signal conversion.  This
        is obsolete; you should now use the CLASS_MAINSIGNALIN macro. */
    if (sel == &s_signal)
    {
        if (c->c_floatsignalin)
            post("warning: signal method overrides class_mainsignalin");
        c->c_floatsignalin = -1;
    }
        /* check for special cases.  "Pointer" is missing here so that
        pd_objectmaker's pointer method can be typechecked differently.  */
    if (sel == &s_bang)
    {
        if (argtype) goto phooey;
        class_addbang(c, fn);
    }
    else if (sel == &s_float)
    {
        if (argtype != A_FLOAT || va_arg(ap, t_atomtype)) goto phooey;
        class_doaddfloat(c, fn);
    }
    else if (sel == &s_symbol)
    {
        if (argtype != A_SYMBOL || va_arg(ap, t_atomtype)) goto phooey;
        class_addsymbol(c, fn);
    }
    else if (sel == &s_blob) /* MP 20070106 blob type */
    {
        post("class_addmethod: %p", fn);
        if (argtype != A_BLOB || va_arg(ap, t_atomtype)) goto phooey;
        class_addblob(c, fn);
    }
    else if (sel == &s_list)
    {
        if (argtype != A_GIMME) goto phooey;
        class_addlist(c, fn);
    }
    else if (sel == &s_anything)
    {
        if (argtype != A_GIMME) goto phooey;
        class_addanything(c, fn);
    }
    else
    {
        /* Pd-extended doesn't use the aliasing automagic
        int i;
        for (i = 0; i < c->c_nmethod; i++)
            if (c->c_methods[i].me_name == sel)
        {
            char nbuf[80];
            snprintf(nbuf, 80, "%s_aliased", sel->s_name);
            c->c_methods[i].me_name = gensym(nbuf);
            if (c == pd_objectmaker)
                post("warning: class '%s' overwritten; old one renamed '%s'",
                    sel->s_name, nbuf);
            else post("warning: old method '%s' for class '%s' renamed '%s'",
                sel->s_name, c->c_name->s_name, nbuf);
        }
        */
        c->c_methods = t_resizebytes(c->c_methods,
            c->c_nmethod * sizeof(*c->c_methods),
            (c->c_nmethod + 1) * sizeof(*c->c_methods));
        m = c->c_methods +  c->c_nmethod;
        c->c_nmethod++;
        m->me_name = sel;
        m->me_fun = (t_gotfn)fn;
        nargs = 0;
        while (argtype != A_NULL && nargs < MAXPDARG)
        {
            m->me_arg[nargs++] = argtype;
            argtype = va_arg(ap, t_atomtype);
        }
        if (argtype != A_NULL)
            error("%s_%s: only 5 arguments are typecheckable; use A_GIMME",
                c->c_name->s_name, sel->s_name);
        m->me_arg[nargs] = A_NULL;
    }
    va_end(ap);
    return;
phooey:
    bug("class_addmethod: %s_%s: bad argument types\n",
        c->c_name->s_name, sel->s_name);
}

    /* Instead of these, see the "class_addfloat", etc.,  macros in m_pd.h */
void class_addbang(t_class *c, t_method fn)
{
    if (!c)
    {
        bug("class_addbang");
        return;
    }
    c->c_bangmethod = (t_bangmethod)fn;
}

void class_addpointer(t_class *c, t_method fn)
{
    if (!c)
    {
        bug("class_addpointer");
        return;
    }
    c->c_pointermethod = (t_pointermethod)fn;
}

void class_doaddfloat(t_class *c, t_method fn)
{
    if (!c)
    {
        bug("class_doaddfloat");
        return;
    }
    c->c_floatmethod = (t_floatmethod)fn;
}

void class_addsymbol(t_class *c, t_method fn)
{
    if (!c)
    {
        bug("class_addsymbol");
        return;
    }
    c->c_symbolmethod = (t_symbolmethod)fn;
}

void class_addblob(t_class *c, t_method fn) /* MP 20061226 blob type */
{
    if (!c)
    {
        bug("class_addblob");
        return;
    }
    c->c_blobmethod = (t_blobmethod)fn;
}

void class_addlist(t_class *c, t_method fn)
{
    if (!c)
    {
        bug("class_addlist");
        return;
    }
    c->c_listmethod = (t_listmethod)fn;
}

void class_addanything(t_class *c, t_method fn)
{
    if (!c)
    {
        bug("class_addanything");
        return;
    }
    c->c_anymethod = (t_anymethod)fn;
}

void class_setwidget(t_class *c, t_widgetbehavior *w)
{
    if (!c)
    {
        bug("class_setwidget");
        return;
    }
    c->c_wb = w;
}

void class_setparentwidget(t_class *c, t_parentwidgetbehavior *pw)
{
    if (!c)
    {
        bug("class_setparentwidget");
        return;
    }
    c->c_pwb = pw;
}

char *class_getname(t_class *c)
{
    if (!c)
    {
        bug("class_getname");
        return 0;
    }
    return (c->c_name->s_name);
}

char *class_gethelpname(t_class *c)
{
    if (!c)
    {
        bug("class_gethelpname");
        return 0;
    }
    return (c->c_helpname->s_name);
}

void class_sethelpsymbol(t_class *c, t_symbol *s)
{
    if (!c)
    {
        bug("class_sethelpsymbol");
        return;
    }
    c->c_helpname = s;
}

t_parentwidgetbehavior *pd_getparentwidget(t_pd *x)
{
    return ((*x)->c_pwb);
}

void class_setdrawcommand(t_class *c)
{
    if (!c)
    {
        bug("class_setdrawcommand");
        return;
    }
    c->c_drawcommand = 1;
}

int class_isdrawcommand(t_class *c)
{
    if (!c)
    {
        bug("class_isdrawcommand");
        return 0;
    }
    return (c->c_drawcommand);
}

static void pd_floatforsignal(t_pd *x, t_float f)
{
    int offset = (*x)->c_floatsignalin;
    if (offset > 0)
        *(t_float *)(((char *)x) + offset) = f;
    else
        pd_error(x, "%s: float unexpected for signal input",
            (*x)->c_name->s_name);
}

void class_domainsignalin(t_class *c, int onset)
{
    if (!c)
    {
        bug("class_domainsignalin");
        return;
    }
    if (onset <= 0) onset = -1;
    else
    {
        if (c->c_floatmethod != pd_defaultfloat)
            post("warning: %s: float method overwritten", c->c_name->s_name);
        c->c_floatmethod = (t_floatmethod)pd_floatforsignal;
    }
    c->c_floatsignalin = onset;
}

void class_set_extern_dir(t_symbol *s)
{
    class_extern_dir = s;
}

char *class_gethelpdir(t_class *c)
{
    if (!c)
    {
        bug("class_gethelpdir");
        return 0;
    }
    return (c->c_externdir->s_name);
}

static void class_nosavefn(t_gobj *z, t_binbuf *b)
{
    bug("save function called but not defined");
}

void class_setsavefn(t_class *c, t_savefn f)
{
    if (!c)
    {
        bug("class_setsavefn");
        return;
    }
    c->c_savefn = f;
}

t_savefn class_getsavefn(t_class *c)
{
    if (!c)
    {
        bug("class_getsavefn");
        return 0;
    }
    return (c->c_savefn);
}

void class_setpropertiesfn(t_class *c, t_propertiesfn f)
{
    if (!c)
    {
        bug("class_setpropertiesfn");
        return;
    }
    c->c_propertiesfn = f;
}

t_propertiesfn class_getpropertiesfn(t_class *c)
{
    if (!c)
    {
        bug("class_getpropertiesfn");
        return 0;
    }
    return (c->c_propertiesfn);
}

/* ---------------- the symbol table ------------------------ */

#define HASHSIZE 1024

static t_symbol *symhash[HASHSIZE];

t_symbol *dogensym(const char *s, t_symbol *oldsym)
{
    t_symbol **sym1, *sym2;
    unsigned int hash1 = 0,  hash2 = 0;
    int length = 0;
    const char *s2 = s;
    while (*s2)
    {
        hash1 += *s2;
        hash2 += hash1;
        length++;
        s2++;
    }
    sym1 = symhash + (hash2 & (HASHSIZE-1));
    while (sym2 = *sym1)
    {
        if (!strcmp(sym2->s_name, s)) return(sym2);
        sym1 = &sym2->s_next;
    }
    if (oldsym) sym2 = oldsym;
    else
    {
        sym2 = (t_symbol *)t_getbytes(sizeof(*sym2));
        sym2->s_name = t_getbytes(length+1);
        sym2->s_next = 0;
        sym2->s_thing = 0;
        strcpy(sym2->s_name, s);
    }
    *sym1 = sym2;
    return (sym2);
}

t_symbol *gensym(const char *s)
{
    return(dogensym(s, 0));
}

#define MAXOBJDEPTH 1000
static int tryingalready;

void canvas_popabstraction(t_canvas *x);
void canvas_initbang(t_canvas *x);

extern t_pd *newest;

t_symbol* pathsearch(t_symbol *s,char* ext);
int pd_setloadingabstraction(t_symbol *sym);

    /* this routine is called when a new "object" is requested whose class Pd
    doesn't know.  Pd tries to load it as an extern, then as an abstraction. */
void new_anything(void *dummy, t_symbol *s, int argc, t_atom *argv)
{
    if (tryingalready>MAXOBJDEPTH){
      error("maximum object loading depth %d reached", MAXOBJDEPTH);
      return;
    }
    newest = 0;
    class_loadsym = s;
    if (sys_load_lib(canvas_getcurrent(), s->s_name))
    {
        tryingalready++;
        typedmess(dummy, s, argc, argv);
        tryingalready--;
        return;
    }
    class_loadsym = 0;
}

t_symbol  s_pointer =   {"pointer", 0, 0};
t_symbol  s_float =     {"float", 0, 0};
t_symbol  s_symbol =    {"symbol", 0, 0};
t_symbol  s_bang =      {"bang", 0, 0};
t_symbol  s_list =      {"list", 0, 0};
t_symbol  s_anything =  {"anything", 0, 0};
t_symbol  s_signal =    {"signal", 0, 0};
t_symbol  s__N =        {"#N", 0, 0};
t_symbol  s__X =        {"#X", 0, 0};
t_symbol  s_x =         {"x", 0, 0};
t_symbol  s_y =         {"y", 0, 0};
t_symbol  s_ =          {"", 0, 0};
t_symbol  s_blob =      {"blob", 0, 0}; /* MP 20061223 blob type */

static t_symbol *symlist[] = { &s_pointer, &s_float, &s_symbol, &s_bang,
    &s_list, &s_anything, &s_signal, &s__N, &s__X, &s_x, &s_y, &s_, &s_blob}; /* MP 20061223 added s_blob */

void mess_init(void)
{
    t_symbol **sp;
    int i;

    if (pd_objectmaker) return;    
    for (i = sizeof(symlist)/sizeof(*symlist), sp = symlist; i--; sp++)
        (void) dogensym((*sp)->s_name, *sp);
    pd_objectmaker = class_new(gensym("objectmaker"), 0, 0, sizeof(t_pd),
        CLASS_DEFAULT, A_NULL);
    pd_canvasmaker = class_new(gensym("classmaker"), 0, 0, sizeof(t_pd),
        CLASS_DEFAULT, A_NULL);
    pd_bind(&pd_canvasmaker, &s__N);
    class_addanything(pd_objectmaker, (t_method)new_anything);
}

t_pd *newest;

/* This is externally available, but note that it might later disappear; the
whole "newest" thing is a hack which needs to be redesigned. */
t_pd *pd_newest(void)
{
    return (newest);
}

    /* horribly, we need prototypes for each of the artificial function
    calls in typedmess(), to keep the compiler quiet. */
typedef t_pd *(*t_newgimme)(t_symbol *s, int argc, t_atom *argv);
typedef void(*t_messgimme)(t_pd *x, t_symbol *s, int argc, t_atom *argv);

typedef t_pd *(*t_fun0)(
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun1)(t_int i1,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun2)(t_int i1, t_int i2,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun3)(t_int i1, t_int i2, t_int i3,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun4)(t_int i1, t_int i2, t_int i3, t_int i4,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun5)(t_int i1, t_int i2, t_int i3, t_int i4, t_int i5,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun6)(t_int i1, t_int i2, t_int i3, t_int i4, t_int i5, t_int i6,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);

/* needed for proper error reporting */
extern t_pd *pd_mess_from_responder(t_pd *x);

/* This is a hack for the messages in Pd files that follow a comma, such as:
     #X restore..., f 12
   In that case the "f 12" would get sent to the subcanvas where we actually
   want it to apply to the parent.

   Pd Vanilla apparently made a special case to handle this-- instead of using
   the comma after the "restore" message it starts a new "#X f 12;" which
   ensures that the #X is bound to the correct canvas. But that means it fails
   for the old syntax, and there are patches in the wild that use it.

   So we support _both_ of these styles in Purr Data by doing the following:
   1. Checking if the last typedmess was "restore" selector.
   2. Checking if the last t_pd was a canvas that matches the current one.
      If so, we're dealing with the old "#X..., f 12;" syntax. If not, it's
      the new Pd Vanilla syntax.
*/
t_symbol *last_typedmess;
t_pd *last_typedmess_pd;

void pd_typedmess(t_pd *x, t_symbol *s, int argc, t_atom *argv)
{
    t_class *c = *x;
    t_methodentry *m;
    t_atomtype *wp, wanttype;
    int i;
    t_int ai[MAXPDARG+1], *ap = ai;
    t_floatarg ad[MAXPDARG+1], *dp = ad;
    int narg = 0;
    t_pd *bonzo;
    
    //fprintf(stderr,"pd_typedmess: %s %d\n", s->s_name, c->c_nmethod);

        /* check for messages that are handled by fixed slots in the class
        structure.  We don't catch "pointer" though so that sending "pointer"
        to pd_objectmaker doesn't require that we supply a pointer value. */
    if (s == &s_float)
    {
        if (!argc) (*c->c_floatmethod)(x, 0.);
        else if (argv->a_type == A_FLOAT)
            (*c->c_floatmethod)(x, argv->a_w.w_float);
        else goto badarg;
        goto lastmess;
    }
    if (s == &s_bang)
    {
        (*c->c_bangmethod)(x);
        goto lastmess;
    }
    if (s == &s_list)
    {
        (*c->c_listmethod)(x, s, argc, argv);
        goto lastmess;
    }
    if (s == &s_symbol)
    {
        if (argc && argv->a_type == A_SYMBOL)
            (*c->c_symbolmethod)(x, argv->a_w.w_symbol);
        else
            (*c->c_symbolmethod)(x, &s_);
        goto lastmess;
    }
    if (s == &s_blob) /* MP 20061226 blob type */
    {
        /*post("pd_typedmess argc = %d\n", argc);*//* MP 20061226 debug */
        if (argc == 1) (*c->c_blobmethod)(x, argv->a_w.w_blob);
        else goto badarg;
        goto lastmess;
    }
    for (i = c->c_nmethod, m = c->c_methods; i--; m++)
    {
        //if (m->me_name == s)
        if (m && m->me_name == s)
        {
            //fprintf(stderr,"me_name %s\n", m->me_name);
            wp = m->me_arg;
            if (*wp == A_GIMME)
            {
                if (x == &pd_objectmaker)
                    newest = (*((t_newgimme)(m->me_fun)))(s, argc, argv);
                else (*((t_messgimme)(m->me_fun)))(x, s, argc, argv);
                goto lastmess;
            }
            if (argc > MAXPDARG) argc = MAXPDARG;
            if (x != &pd_objectmaker) *(ap++) = (t_int)x, narg++;
            while (wanttype = *wp++)
            {
                switch (wanttype)
                {
                case A_POINTER:
                    if (!argc) goto badarg;
                    else
                    {
                        if (argv->a_type == A_POINTER)
                            *ap = (t_int)(argv->a_w.w_gpointer);
                        else goto badarg;
                        argc--;
                        argv++;
                    }
                    narg++;
                    ap++;
                    break;
                case A_FLOAT:
                    if (!argc) goto badarg; /* falls through */
                case A_DEFFLOAT:
                    if (!argc) *dp = 0;
                    else
                    {
                        if (argv->a_type == A_FLOAT)
                            *dp = argv->a_w.w_float;
                        else goto badarg;
                        argc--;
                        argv++;
                    }
                    dp++;
                    break;
                case A_BLOB:/* MP 20070106 blob type */
                    /*post("pd_typedmess A_BLOB");*/
                    if (!argc) goto badarg;
                    if (argv->a_type == A_BLOB)
                    {
                        /*post("argv->a_type == A_BLOB, argc = %d, narg= %d",
                        //    argc, narg);*/
                        *ap = (t_int)(argv->a_w.w_blob);
                    }
                    argc--;
                    argv++;
                    narg++;
                    ap++;
                    break;
                case A_SYMBOL:
                    if (!argc) goto badarg; /* falls through */
                case A_DEFSYM:
                    if (!argc) *ap = (t_int)(&s_);
                    else
                    {
                        if (argv->a_type == A_SYMBOL)
                            *ap = (t_int)(argv->a_w.w_symbol);
                                /* if it's an unfilled "dollar" argument it
                                   appears as zero here; cheat and bash it
                                   to the null symbol.  Unfortunately, this
                                   lets real zeros pass as symbols too, which
                                   seems wrong... */
                        else if (x == &pd_objectmaker &&
                                 argv->a_type == A_FLOAT
                                 && argv->a_w.w_float == 0)
                            *ap = (t_int)(&s_);
                        else goto badarg;
                        argc--;
                        argv++;
                    }
                    narg++;
                    ap++;
                    break;
                default:
                    goto badarg;
                }
            }
            switch (narg)
            {
            case 0 : bonzo = (*(t_fun0)(m->me_fun))
                (ad[0], ad[1], ad[2], ad[3], ad[4]); break;
            case 1 : bonzo = (*(t_fun1)(m->me_fun))
                (ai[0], ad[0], ad[1], ad[2], ad[3], ad[4]); break;
            case 2 : bonzo = (*(t_fun2)(m->me_fun))
                (ai[0], ai[1], ad[0], ad[1], ad[2], ad[3], ad[4]); break;
            case 3 : bonzo = (*(t_fun3)(m->me_fun))
                (ai[0], ai[1], ai[2], ad[0], ad[1], ad[2], ad[3], ad[4]); break;
            case 4 : bonzo = (*(t_fun4)(m->me_fun))
                (ai[0], ai[1], ai[2], ai[3],
                    ad[0], ad[1], ad[2], ad[3], ad[4]); break;
            case 5 : bonzo = (*(t_fun5)(m->me_fun))
                (ai[0], ai[1], ai[2], ai[3], ai[4],
                    ad[0], ad[1], ad[2], ad[3], ad[4]); break;
            case 6 : bonzo = (*(t_fun6)(m->me_fun))
                (ai[0], ai[1], ai[2], ai[3], ai[4], ai[5],
                    ad[0], ad[1], ad[2], ad[3], ad[4]); break;
            default: bonzo = 0;
            }
            if (x == &pd_objectmaker)
                newest = bonzo;
            goto lastmess;
        }
    }
    (*c->c_anymethod)(x, s, argc, argv);
    goto lastmess;
badarg:
    /* if x is a messresponder class, tweak it to point to the
       message that contains it (so it can be selected when 'Find
       Error' is used). */
    x = pd_mess_from_responder(x);
    pd_error(x, "Bad arguments for message '%s' to object '%s'%s",
        s->s_name,
        c->c_name->s_name,
        type_hint(s, argc, argv, 1));
lastmess:
    last_typedmess = s;    
    last_typedmess_pd = x;
    return;
}

    /* convenience routine giving a stdarg interface to typedmess().  Only
    ten args supported; it seems unlikely anyone will need more since
    longer messages are likely to be programmatically generated anyway. */
void pd_vmess(t_pd *x, t_symbol *sel, char *fmt, ...)
{
    va_list ap;
    t_atom arg[10], *at = arg;
    int nargs = 0;
    char *fp = fmt;

    va_start(ap, fmt);
    while (1)
    {
        if (nargs >= 10)
        {
            pd_error(x, "pd_vmess: only 10 allowed");
            break;
        }
        switch(*fp++)
        {
        case 'f': SETFLOAT(at, va_arg(ap, double)); break;
        case 's': SETSYMBOL(at, va_arg(ap, t_symbol *)); break;
        case 't':
            SETBLOB(at, va_arg(ap, t_blob *));
            /*post("pd_vmess: arg[0].a_w.w_blob = %p", arg[0].a_w.w_blob);*/
            break; /* MP 20061226 blob type */
        case 'i': SETFLOAT(at, va_arg(ap, t_int)); break;       
        case 'p': SETPOINTER(at, va_arg(ap, t_gpointer *)); break;
        default: goto done;
        }
        at++;
        nargs++;
    }
done:
    va_end(ap);
    typedmess(x, sel, nargs, arg);
}

void pd_forwardmess(t_pd *x, int argc, t_atom *argv)
{
    if (argc)
    {
        t_atomtype t = argv->a_type;
        if (t == A_SYMBOL) pd_typedmess(x, argv->a_w.w_symbol, argc-1, argv+1);
        else if (t == A_POINTER)
        {
            if (argc == 1) pd_pointer(x, argv->a_w.w_gpointer);
            else pd_list(x, &s_list, argc, argv);
        }
        else if (t == A_FLOAT)
        {
            if (argc == 1) pd_float(x, argv->a_w.w_float);
            else pd_list(x, &s_list, argc, argv);
        }
        else bug("pd_forwardmess");
    }

}

void nullfn(void) {}

t_gotfn getfn(t_pd *x, t_symbol *s)
{
    t_class *c = *x;
    t_methodentry *m;
    int i;

    for (i = c->c_nmethod, m = c->c_methods; i--; m++)
        if (m->me_name == s) return(m->me_fun);
    pd_error(x, "%s: no method for message '%s'%s",
            c->c_name->s_name,
            s->s_name,
            type_hint(s, 0, 0, 1));
    return((t_gotfn)nullfn);
}

t_gotfn zgetfn(t_pd *x, t_symbol *s)
{
    t_class *c = *x;
    t_methodentry *m;
    int i;

    for (i = c->c_nmethod, m = c->c_methods; i--; m++)
        if (m->me_name == s) return(m->me_fun);
    return(0);
}

t_gotfn zcheckgetfn(t_pd *x, t_symbol *s, t_atomtype arg1, ...)
{
    t_class *c = *x;
    t_methodentry *m;
    int i, j;

    /* get arg types */
    va_list ap;
    int nargs = 0;
    t_atomtype args[MAXPDARG+1], curr = arg1;
    va_start(ap, arg1);
    while (curr != A_NULL && nargs < MAXPDARG)
    {
        args[nargs++] = curr;
        curr = va_arg(ap, t_atomtype);
    }
    if (curr != A_NULL) error("zcheckgetfn: only 5 arguments are typecheckable");
    args[nargs] = A_NULL;
    va_end(ap);

    for (i = c->c_nmethod, m = c->c_methods; i--; m++)
    {
        if (m->me_name == s)
        {
            j = 0;
            /* both argtype lists are valid, dont need to check whether j < MAXDPARG */
            while(m->me_arg[j] != A_NULL && args[j] != A_NULL
                    && m->me_arg[j] == args[j]) j++;
            if(m->me_arg[j] == A_NULL && args[j] == A_NULL) return(m->me_fun);
        }
    }
    return(0);
}

/* this is privately shared with d_ugen.c */
int class_getdspflags(const t_class *c)
{
    return ((c->c_multichannel ? CLASS_MULTICHANNEL : 0) |
            (c->c_nopromotesig ? CLASS_NOPROMOTESIG : 0) |
            (c->c_nopromoteleft ? CLASS_NOPROMOTELEFT : 0) );
}

