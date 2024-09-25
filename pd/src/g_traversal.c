/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This file defines Text objects which traverse data contained in scalars
and arrays:

pointer - point to an object belonging to a template
get -     get numeric fields
set -     change numeric fields
element - get an array element
getsize - get the size of an array
setsize - change the size of an array
append -  add an element to a list
sublist - get a pointer into a list which is an element of another scalar
field -   get numeric/symbolic field while within a canvas field

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>      /* for read/write to files */
#include "m_pd.h"
#include "g_canvas.h"

/* ------------- gstubs and gpointers - safe pointing --------------- */

/* create a gstub which is "owned" by a glist (gl) or an array ("a"). */

t_gstub *gstub_new(t_glist *gl, t_array *a)
{
    t_gstub *gs = t_getbytes(sizeof(*gs));
    if (gl)
    {
        gs->gs_which = GP_GLIST;
        gs->gs_un.gs_glist = gl;
    }
    else
    {
        gs->gs_which = GP_ARRAY;
        gs->gs_un.gs_array = a;
    }
    gs->gs_refcount = 0;
    return (gs);
}

/* when a "gpointer" is set to point to this stub (so we can later chase
down the owner) we increase a reference count.  The following routine is called
whenever a gpointer is unset from pointing here.  If the owner is
gone and the refcount goes to zero, we can free the gstub safely. */

static void gstub_dis(t_gstub *gs)
{
    int refcount = --gs->gs_refcount;
    if ((!refcount) && gs->gs_which == GP_NONE)
        t_freebytes(gs, sizeof (*gs));
    else if (refcount < 0) bug("gstub_dis");
}

/* this routing is called by the owner to inform the gstub that it is
being deleted.  If no gpointers are pointing here, we can free the gstub;
otherwise we wait for the last gstub_dis() to free it. */

void gstub_cutoff(t_gstub *gs)
{
    gs->gs_which = GP_NONE;
    if (gs->gs_refcount < 0) bug("gstub_cutoff");
    if (!gs->gs_refcount) t_freebytes(gs, sizeof (*gs));
}

int gpointer_docheck(const t_gpointer *gp, int headok, int gobjok)
{
    t_gstub *gs = gp->gp_stub;
    if (!gs) return (0);
    if (gs->gs_which == GP_ARRAY)
    {
        if (gs->gs_un.gs_array->a_valid != gp->gp_valid) return (0);
        else return (1);
    }
    else if (gs->gs_which == GP_GLIST)
    {
        if (!headok && !((t_scalar *)(gp->gp_un.gp_gobj)))
            return (0);
        else if (!gobjok && gp->gp_un.gp_gobj &&
                 pd_class(&gp->gp_un.gp_gobj->g_pd) != scalar_class)
            return (0);
        else if (gs->gs_un.gs_glist->gl_valid != gp->gp_valid)
            return (0);
        else
            return (1);
    }
    else return (0);
}

/* call this to verify that a pointer is fresh, i.e., that it either
points to real data or to the head of a list, and that in either case
the object hasn't disappeared since this pointer was generated. 
Unless "headok" is set,  the routine also fails for the head of a list.*/

int gpointer_check(const t_gpointer *gp, int headok)
{
    return (gpointer_docheck(gp, headok, 0));
}

/* more general form for checking for pointers to gobjs */
int gpointer_check_gobj(const t_gpointer *gp)
{
    return (gpointer_docheck(gp, 0, 1));
}

/* get the template for the object pointer to.  Assumes we've already checked
freshness.  Returns 0 if head of list. */

static t_symbol *gpointer_gettemplatesym(const t_gpointer *gp)
{
    t_gstub *gs = gp->gp_stub;
    if (gs->gs_which == GP_GLIST)
    {
        t_scalar *sc = (t_scalar *)(gp->gp_un.gp_gobj);
        if (sc)
            return (sc->sc_template);
        else return (0);
    }
    else
    {
        t_array *a = gs->gs_un.gs_array;
        return (a->a_templatesym);
    }
}

    /* copy a pointer to another, assuming the second one hasn't yet been
    initialized.  New gpointers should be initialized either by this
    routine or by gpointer_init below. */
void gpointer_copy(const t_gpointer *gpfrom, t_gpointer *gpto)
{
    *gpto = *gpfrom;
    if (gpto->gp_stub)
        gpto->gp_stub->gs_refcount++;
    else bug("gpointer_copy");
}

    /* clear a gpointer that was previously set, releasing the associated
    gstub if this was the last reference to it. */
void gpointer_unset(t_gpointer *gp)
{
    t_gstub *gs;
    if (gs = gp->gp_stub)
    {
        gstub_dis(gs);
        gp->gp_stub = 0;
    }
}

void gpointer_setglist(t_gpointer *gp, t_glist *glist, t_gobj *x)
{
    t_gstub *gs;
    if (gs = gp->gp_stub) gstub_dis(gs);
    gp->gp_stub = gs = glist->gl_stub;
    gp->gp_valid = glist->gl_valid;
    gp->gp_un.gp_gobj = x;
    gs->gs_refcount++;
}

void gpointer_setarray(t_gpointer *gp, t_array *array, t_word *w)
{
    t_gstub *gs;
    if (gs = gp->gp_stub) gstub_dis(gs);
    gp->gp_stub = gs = array->a_stub;
    gp->gp_valid = array->a_valid;
    gp->gp_un.gp_w = w;
    gs->gs_refcount++;
}

void gpointer_init(t_gpointer *gp)
{
    gp->gp_stub = 0;
    gp->gp_valid = 0;
    gp->gp_un.gp_gobj = 0;
}

/*********  random utility function to find a binbuf in a datum */

t_binbuf *pointertobinbuf(t_pd *x, t_gpointer *gp, t_symbol *s,
    const char *fname)
{
    t_symbol *templatesym = gpointer_gettemplatesym(gp), *arraytype;
    t_template *template;
    int onset, type;
    t_gstub *gs = gp->gp_stub;
    t_word *vec;
    if (!templatesym)
    {
        pd_error(x, "%s: bad pointer", fname);
        return (0);
    }
    if (!(template = template_findbyname(templatesym)))
    {
        pd_error(x, "%s: couldn't find template %s", fname,
            templatesym->s_name);
        return (0);
    }
    if (!template_find_field(template, s, &onset, &type, &arraytype))
    {
        pd_error(x, "%s: %s.%s: no such field", fname,
            templatesym->s_name, s->s_name);
        return (0);
    }
    if (type != DT_TEXT)
    {
        pd_error(x, "%s: %s.%s: not a list", fname,
            templatesym->s_name, s->s_name);
        return (0);
    }
    if (gs->gs_which == GP_ARRAY)
        vec = gp->gp_un.gp_w;
    else vec = ((t_scalar *)(gp->gp_un.gp_gobj))->sc_vec;
    return (vec[onset].w_binbuf);
}

    /* templates are named using the name-bashing by which canvases bind
    thenselves, with a leading "pd-".  LATER see if we can have templates
    occupy their real names.  Meanwhile, if a template has an empty name
    or is named "-" (as when passed as a "-" argument to "get", etc.), just
    return &s_; objects should check for this and allow it as a wild
    card when appropriate. */
static t_symbol *template_getbindsym(t_symbol *s)
{
    if (!*s->s_name || !strcmp(s->s_name, "-"))
        return (&s_);
    else return (canvas_makebindsym(s));
}

/* ---------------------- pointers ----------------------------- */

static t_class *ptrobj_class;

typedef struct 
{
    t_symbol *to_type;
    t_outlet *to_outlet;
} t_typedout;

typedef struct _ptrobj
{
    t_object x_obj;
    t_gpointer x_gp;
    t_typedout *x_typedout;
    int x_ntypedout;
    t_outlet *x_otherout;
    t_outlet *x_bangout;
} t_ptrobj;

static void *ptrobj_new(t_symbol *classname, int argc, t_atom *argv)
{
    t_ptrobj *x = (t_ptrobj *)pd_new(ptrobj_class);
    t_typedout *to;
    int n;
    gpointer_init(&x->x_gp);
    x->x_typedout = to = (t_typedout *)getbytes(argc * sizeof (*to));
    x->x_ntypedout = n = argc;
    for (; n--; to++)
    {
        to->to_outlet = outlet_new(&x->x_obj, &s_pointer);
        to->to_type = template_getbindsym(atom_getsymbol(argv++));
    }
    x->x_otherout = outlet_new(&x->x_obj, &s_pointer);
    x->x_bangout = outlet_new(&x->x_obj, &s_bang);
    pointerinlet_new(&x->x_obj, &x->x_gp);
    return (x);
}

static void ptrobj_traverse(t_ptrobj *x, t_symbol *s)
{
    t_glist *glist = (t_glist *)pd_findbyclass(s, canvas_class);
    if (glist) gpointer_setglist(&x->x_gp, glist, 0);
    else pd_error(x, "pointer: list '%s' not found", s->s_name);
}

static void ptrobj_vnext(t_ptrobj *x, t_float f)
{
    t_gobj *gobj;
    t_gpointer *gp = &x->x_gp;
    t_gstub *gs = gp->gp_stub;
    t_glist *glist;
    int wantselected = (f != 0);

    if (!gs)
    {
        pd_error(x, "ptrobj_next: no current pointer");
        return;
    }
    if (gs->gs_which != GP_GLIST)
    {
        pd_error(x, "ptrobj_next: lists only, not arrays");
        return;
    }
    glist = gs->gs_un.gs_glist;
    if (glist->gl_valid != gp->gp_valid)
    {
        pd_error(x, "ptrobj_next: stale pointer");
        return;
    }
    if (wantselected && !glist_isvisible(glist))
    {
        pd_error(x,
            "ptrobj_vnext: next-selected only works for a visible window");
        return;
    }
    gobj = gp->gp_un.gp_gobj;
    
    if (!gobj) gobj = glist->gl_list;
    else gobj = gobj->g_next;
    while (gobj && ((pd_class(&gobj->g_pd) != scalar_class) ||
        (wantselected && !glist_isselected(glist, gobj))))
            gobj = gobj->g_next;
    
    if (gobj)
    {
        t_typedout *to;
        int n;
        t_scalar *sc = (t_scalar *)gobj;
        t_symbol *templatesym = sc->sc_template;

        gp->gp_un.gp_gobj = &sc->sc_gobj; 
        for (n = x->x_ntypedout, to = x->x_typedout; n--; to++)
        {
            if (to->to_type == templatesym)
            {
                outlet_pointer(to->to_outlet, &x->x_gp);
                return;
            }
        }
        outlet_pointer(x->x_otherout, &x->x_gp);
    }
    else
    {
        gpointer_unset(gp);
        outlet_bang(x->x_bangout);
    }
}

static void ptrobj_next(t_ptrobj *x)
{
    ptrobj_vnext(x, 0);
}

static void ptrobj_delete(t_ptrobj *x)
{
    t_gobj *gobj, *old;
    t_gpointer *gp = &x->x_gp;
    t_gstub *gs = gp->gp_stub;
    t_glist *glist;
    if (!gs)
    {
        pd_error(x, "pointer delete: no current pointer");
        return;
    }
    if (gs->gs_which != GP_GLIST)
    {
        pd_error(x, "pointer delete: lists only, not arrays");
        return;
    }
    glist = gs->gs_un.gs_glist;
    if (glist->gl_valid != gp->gp_valid)
    {
        pd_error(x, "pointer delete: stale pointer");
        return;
    }
    if (!gp->gp_un.gp_gobj)
    {
        pd_error(x, "pointer delete: pointing to head");
        return;
    }
    if (((t_scalar*)gp->gp_un.gp_gobj)->sc_template == gensym("pd-text"))
    {
        pd_error(x, "pointer delete: can't delete 'pd-text' scalar");
        return;
    }
    if (((t_scalar*)gp->gp_un.gp_gobj)->sc_template == gensym("pd-float-array"))
    {
        pd_error(x, "pointer delete: can't delete 'pd-float-array' scalar");
        return;
    }

    old = gp->gp_un.gp_gobj;
    gobj = old->g_next;
    while (gobj && (pd_class(&gobj->g_pd) != scalar_class))
        gobj = gobj->g_next;
    glist_delete(glist, old);
    gp->gp_valid = glist->gl_valid;
    if (gobj)
    {
        t_typedout *to;
        int n;
        t_scalar *sc = (t_scalar *)gobj;
        t_symbol *templatesym = sc->sc_template;

        gp->gp_un.gp_gobj = gobj;
        for (n = x->x_ntypedout, to = x->x_typedout; n--; to++)
        {
            if (to->to_type == templatesym)
            {
                outlet_pointer(to->to_outlet, &x->x_gp);
                return;
            }
        }
        outlet_pointer(x->x_otherout, &x->x_gp);
    }
    else
    {
        gpointer_unset(gp);
        outlet_bang(x->x_bangout);
    }
}

t_symbol *gpointer_gettemplatesym(const t_gpointer *gp);

static void ptrobj_equal(t_ptrobj *x, t_gpointer *gp)
{
    t_symbol *templatesym;
    int n, result;
    t_typedout *to;
    if (!gpointer_check(&x->x_gp, 1))
    {
        pd_error(x, "pointer equal: empty pointer");
        return;
    }
    /* we don't care for the actual type in the union because they are all pointers */
    result = (gp->gp_stub->gs_un.gs_glist == x->x_gp.gp_stub->gs_un.gs_glist) &&
        (gp->gp_un.gp_gobj == x->x_gp.gp_un.gp_gobj);
    if (!result)
    {
        outlet_bang(x->x_bangout);
        return;
    }
    templatesym = gpointer_gettemplatesym(&x->x_gp);
    for (n = x->x_ntypedout, to = x->x_typedout; n--; to++)
    {
        if (to->to_type == templatesym)
        {
            outlet_pointer(to->to_outlet, &x->x_gp);
            return;
        }
    }
    outlet_pointer(x->x_otherout, &x->x_gp);
}

    /* send a message to the window containing the object pointed to */ 
static void ptrobj_sendwindow(t_ptrobj *x, t_symbol *s, int argc, t_atom *argv)
{
    t_glist *glist;
    t_pd *canvas;
    t_gstub *gs;
    if (!gpointer_check(&x->x_gp, 1))
    {
        pd_error(x, "ptrobj_bang: empty pointer");
        return;
    }
    gs = x->x_gp.gp_stub;
    if (gs->gs_which == GP_GLIST)
        glist = gs->gs_un.gs_glist;  
    else
    {
        t_array *owner_array = gs->gs_un.gs_array;
        while (owner_array->a_gp.gp_stub->gs_which == GP_ARRAY)
            owner_array = owner_array->a_gp.gp_stub->gs_un.gs_array;
        glist = owner_array->a_gp.gp_stub->gs_un.gs_glist;  
    }
    canvas = (t_pd *)glist_getcanvas(glist);
    if (argc && argv->a_type == A_SYMBOL)
        pd_typedmess(canvas, argv->a_w.w_symbol, argc-1, argv+1);
    else pd_error(x, "send-window: no message?");
}

    /* send the pointer to the named object */
static void ptrobj_send(t_ptrobj *x, t_symbol *s)
{
    if (!s->s_thing)
        pd_error(x, "%s: no such object", s->s_name);
    else if (!gpointer_check(&x->x_gp, 1))
        pd_error(x, "pointer_send: empty pointer");
    else pd_pointer(s->s_thing, &x->x_gp);
}

static void ptrobj_bang(t_ptrobj *x)
{
    t_symbol *templatesym;
    int n;
    t_typedout *to;
    if (!gpointer_check(&x->x_gp, 1))
    {
        pd_error(x, "ptrobj_bang: empty pointer");
        return;
    }
    templatesym = gpointer_gettemplatesym(&x->x_gp);
    for (n = x->x_ntypedout, to = x->x_typedout; n--; to++)
    {
        if (to->to_type == templatesym)
        {
            outlet_pointer(to->to_outlet, &x->x_gp);
            return;
        }
    }
    outlet_pointer(x->x_otherout, &x->x_gp);
}


static void ptrobj_pointer(t_ptrobj *x, t_gpointer *gp)
{
    gpointer_unset(&x->x_gp);
    gpointer_copy(gp, &x->x_gp);
    ptrobj_bang(x);
}


static void ptrobj_rewind(t_ptrobj *x)
{
    t_glist *glist;
    t_gstub *gs;
    if (!gpointer_check(&x->x_gp, 1))
    {
        pd_error(x, "pointer_rewind: empty pointer");
        return;
    }
    gs = x->x_gp.gp_stub;
    if (gs->gs_which != GP_GLIST)
    {
        pd_error(x, "pointer_rewind: sorry, unavailable for arrays");
        return;
    }
    glist = gs->gs_un.gs_glist;  
    gpointer_setglist(&x->x_gp, glist, 0);
    ptrobj_bang(x);
}

static void ptrobj_free(t_ptrobj *x)
{
    freebytes(x->x_typedout, x->x_ntypedout * sizeof (*x->x_typedout));
    gpointer_unset(&x->x_gp);
}

static void ptrobj_setup(void)
{
    ptrobj_class = class_new(gensym("pointer"), (t_newmethod)ptrobj_new,
        (t_method)ptrobj_free, sizeof(t_ptrobj), 0, A_GIMME, 0);
    class_addmethod(ptrobj_class, (t_method)ptrobj_next, gensym("next"), 0); 
    class_addmethod(ptrobj_class, (t_method)ptrobj_send, gensym("send"),
        A_SYMBOL, 0);
    class_addmethod(ptrobj_class, (t_method)ptrobj_traverse, gensym("traverse"),
        A_SYMBOL, 0); 
    class_addmethod(ptrobj_class, (t_method)ptrobj_vnext, gensym("vnext"), 
        A_DEFFLOAT, 0); 
    class_addmethod(ptrobj_class, (t_method)ptrobj_delete, gensym("delete"), 0);
    class_addmethod(ptrobj_class, (t_method)ptrobj_equal, gensym("equal"), A_POINTER, 0);
    class_addmethod(ptrobj_class, (t_method)ptrobj_sendwindow,
        gensym("send-window"), A_GIMME, 0); 
    class_addmethod(ptrobj_class, (t_method)ptrobj_rewind,
        gensym("rewind"), 0); 
    class_addpointer(ptrobj_class, ptrobj_pointer); 
    class_addbang(ptrobj_class, ptrobj_bang); 
}

/* ---------------------- get ----------------------------- */

static t_class *get_class;

typedef struct _getvariable
{
    t_symbol *gv_sym;
    t_outlet *gv_outlet;
} t_getvariable;

typedef struct _get
{
    t_object x_obj;
    t_symbol *x_templatesym;
    int x_nout;
    t_getvariable *x_variables;
} t_get;

static void *get_new(t_symbol *why, int argc, t_atom *argv)
{
    t_get *x = (t_get *)pd_new(get_class);
    int varcount, i;
    t_atom at, *varvec;
    t_getvariable *sp;

    x->x_templatesym = canvas_makebindsym(atom_getsymbolarg(0, argc, argv));
    if (argc < 2) /* have at least one outlet */
    {
        varcount = 1;
        varvec = &at;
        SETSYMBOL(&at, &s_); /* set field to empty symbol */
    }
    else varcount = argc - 1, varvec = argv + 1;
    x->x_variables
        = (t_getvariable *)getbytes(varcount * sizeof (*x->x_variables));
    x->x_nout = varcount;
    for (i = 0, sp = x->x_variables; i < varcount; i++, sp++)
    {
        sp->gv_sym = atom_getsymbolarg(i, varcount, varvec);
        sp->gv_outlet = outlet_new(&x->x_obj, 0);
            /* LATER connect with the template and set the outlet's type
            correctly.  We can't yet guarantee that the template is there
            before we hit this routine. */
    }
    return (x);
}

static void get_set(t_get *x, t_symbol *templatesym, t_symbol *field)
{
    if (x->x_nout != 1)
        pd_error(x, "get: cannot set multiple fields.");
    else
    {
       x->x_templatesym = template_getbindsym(templatesym); 
       x->x_variables->gv_sym = field;
    }
}

static void get_pointer(t_get *x, t_gpointer *gp)
{
    int nitems = x->x_nout, i;
    t_symbol *templatesym;
    t_template *template;
    t_gstub *gs = gp->gp_stub;
    t_word *vec;
    t_getvariable *vp;

    if (!gpointer_check(gp, 0))
    {
        pd_error(x, "get: stale or empty pointer");
        return;
    }
    if (*x->x_templatesym->s_name)
    {
        if ((templatesym = x->x_templatesym) != gpointer_gettemplatesym(gp))
        {
            pd_error(x, "get %s: got wrong template (%s)",
                templatesym->s_name, gpointer_gettemplatesym(gp)->s_name);
            return;
        }
    }
    else templatesym = gpointer_gettemplatesym(gp);
    if (!(template = template_findbyname(templatesym)))
    {
        pd_error(x, "get: couldn't find template %s", templatesym->s_name);
        return;
    }
    if (gs->gs_which == GP_ARRAY) vec = gp->gp_un.gp_w;
    else vec = ((t_scalar *)(gp->gp_un.gp_gobj))->sc_vec;
    for (i = nitems - 1, vp = x->x_variables + i; i >= 0; i--, vp--)
    {
        int onset, type;
        t_symbol *arraytype;
        if (template_find_field(template, vp->gv_sym, &onset, &type, &arraytype))
        {
            if (type == DT_FLOAT)
                outlet_float(vp->gv_outlet,
                    *(t_float *)(((char *)vec) + onset));
            else if (type == DT_SYMBOL)
                outlet_symbol(vp->gv_outlet,
                    *(t_symbol **)(((char *)vec) + onset));
            else pd_error(x, "get: %s.%s is not a number or symbol",
                    template->t_sym->s_name, vp->gv_sym->s_name);
        }
        else pd_error(x, "get: %s.%s: no such field",
            template->t_sym->s_name, vp->gv_sym->s_name);
    }
}

static void get_free(t_get *x)
{
    freebytes(x->x_variables, x->x_nout * sizeof (*x->x_variables));
}

static void get_setup(void)
{
    get_class = class_new(gensym("get"), (t_newmethod)get_new,
        (t_method)get_free, sizeof(t_get), 0, A_GIMME, 0);
    class_addpointer(get_class, get_pointer); 
    class_addmethod(get_class, (t_method)get_set, gensym("set"),
        A_SYMBOL, A_SYMBOL, 0); 
}

/* ---------------------- set ----------------------------- */

static t_class *set_class;

typedef struct _setvariable
{
    t_symbol *gv_sym;
    union word gv_w;
} t_setvariable;

typedef struct _set
{
    t_object x_obj;
    t_gpointer x_gp;
    t_symbol *x_templatesym;
    int x_nin;
    int x_issymbol;
    t_setvariable *x_variables;
} t_set;

static void *set_new(t_symbol *why, int argc, t_atom *argv)
{
    t_set *x = (t_set *)pd_new(set_class);
    int i, varcount;
    t_setvariable *sp;
    t_atom at, *varvec;
    if (argc && (argv[0].a_type == A_SYMBOL) &&
        !strcmp(argv[0].a_w.w_symbol->s_name, "-symbol"))
    {
        x->x_issymbol = 1;
        argc--;
        argv++;
    }
    else x->x_issymbol = 0;
    x->x_templatesym = template_getbindsym(atom_getsymbolarg(0, argc, argv));
    if (argc < 2)
    {
        varcount = 1;
        varvec = &at;
        SETSYMBOL(&at, &s_);
    }
    else varcount = argc - 1, varvec = argv + 1;
    x->x_variables
        = (t_setvariable *)getbytes(varcount * sizeof (*x->x_variables));
    x->x_nin = varcount;
    for (i = 0, sp = x->x_variables; i < varcount; i++, sp++)
    {
        sp->gv_sym = atom_getsymbolarg(i, varcount, varvec);
        if (x->x_issymbol)
            sp->gv_w.w_symbol = &s_;
        else sp->gv_w.w_float = 0;
        if (i)
        {
            if (x->x_issymbol)
                symbolinlet_new(&x->x_obj, &sp->gv_w.w_symbol);
            else floatinlet_new(&x->x_obj, &sp->gv_w.w_float);
        }
    }
    pointerinlet_new(&x->x_obj, &x->x_gp);
    gpointer_init(&x->x_gp);
    return (x);
}

static void set_set(t_set *x, t_symbol *templatesym, t_symbol *field)
{
    if (x->x_nin != 1)
        pd_error(x, "set: cannot set multiple fields.");
    else
    {
       x->x_templatesym = template_getbindsym(templatesym); 
       x->x_variables->gv_sym = field;
       if (x->x_issymbol)
           x->x_variables->gv_w.w_symbol = &s_;
       else
           x->x_variables->gv_w.w_float = 0;
    }
}

extern void scalar_configure(t_scalar *x, t_glist *owner);
extern void array_configure(t_scalar *x, t_glist *owner, t_array *a,
    t_word *data);

static void set_bang(t_set *x)
{
    int nitems = x->x_nin, i;
    t_symbol *templatesym;
    t_template *template;
    t_setvariable *vp;
    t_gpointer *gp = &x->x_gp;
    t_gstub *gs = gp->gp_stub;
    t_word *vec;
    if (!gpointer_check(gp, 0))
    {
        pd_error(x, "set: empty pointer");
        return;
    }
    if (*x->x_templatesym->s_name)
    {
        if ((templatesym = x->x_templatesym) != gpointer_gettemplatesym(gp))
        {
            pd_error(x, "set %s: got wrong template (%s)",
                templatesym->s_name, gpointer_gettemplatesym(gp)->s_name);
            return;
        }
    }
    else templatesym = gpointer_gettemplatesym(gp);
    if (!(template = template_findbyname(templatesym)))
    {
        pd_error(x, "set: couldn't find template %s", templatesym->s_name);
        return;
    }
    if (!nitems)
        return;
    if (gs->gs_which == GP_ARRAY)
        vec = gp->gp_un.gp_w;
    else vec = ((t_scalar *)(gp->gp_un.gp_gobj))->sc_vec;
    if (x->x_issymbol)
        for (i = 0, vp = x->x_variables; i < nitems; i++, vp++)
            template_setsymbol(template, vp->gv_sym, vec, vp->gv_w.w_symbol, 1);
    else for (i = 0, vp = x->x_variables; i < nitems; i++, vp++)
        template_setfloat(template, vp->gv_sym, vec, vp->gv_w.w_float, 1);
    if (gs->gs_which == GP_GLIST)
        scalar_configure((t_scalar *)(gp->gp_un.gp_gobj), gs->gs_un.gs_glist);
    else
    {
        t_array *owner_array = gs->gs_un.gs_array;
        t_array *top_array = owner_array;
        while (top_array->a_gp.gp_stub->gs_which == GP_ARRAY)
            top_array = top_array->a_gp.gp_stub->gs_un.gs_array;
        array_configure((t_scalar *)(top_array->a_gp.gp_un.gp_gobj),
            top_array->a_gp.gp_stub->gs_un.gs_glist, owner_array,
            vec);
    }
}

static void set_float(t_set *x, t_float f)
{
    if (x->x_nin && !x->x_issymbol)
    {
        x->x_variables[0].gv_w.w_float = f;
        set_bang(x);
    }
    else pd_error(x, "type mismatch or no field specified");
}

static void set_symbol(t_set *x, t_symbol *s)
{
    if (x->x_nin && x->x_issymbol)
    {
        x->x_variables[0].gv_w.w_symbol = s;
        set_bang(x);
    }
    else pd_error(x, "type mismatch or no field specified");
}

static void set_free(t_set *x)
{
    freebytes(x->x_variables, x->x_nin * sizeof (*x->x_variables));
    gpointer_unset(&x->x_gp);
}

static void set_setup(void)
{
    set_class = class_new(gensym("set"), (t_newmethod)set_new,
        (t_method)set_free, sizeof(t_set), 0, A_GIMME, 0);
    class_addfloat(set_class, set_float); 
    class_addsymbol(set_class, set_symbol); 
    class_addbang(set_class, set_bang); 
    class_addmethod(set_class, (t_method)set_set, gensym("set"),
        A_SYMBOL, A_SYMBOL, 0); 
}

/* ---------------------- element ----------------------------- */

static t_class *elem_class;

typedef struct _elem
{
    t_object x_obj;
    t_symbol *x_templatesym;
    t_symbol *x_fieldsym;
    t_gpointer x_gp;
    t_gpointer x_gparent;
} t_elem;

static void *elem_new(t_symbol *templatesym, t_symbol *fieldsym)
{
    t_elem *x = (t_elem *)pd_new(elem_class);
    x->x_templatesym = template_getbindsym(templatesym);
    x->x_fieldsym = fieldsym;
    gpointer_init(&x->x_gp);
    gpointer_init(&x->x_gparent);
    pointerinlet_new(&x->x_obj, &x->x_gparent);
    outlet_new(&x->x_obj, &s_pointer);
    return (x);
}

static void elem_set(t_elem *x, t_symbol *templatesym, t_symbol *fieldsym)
{
    x->x_templatesym = template_getbindsym(templatesym);
    x->x_fieldsym = fieldsym;
}

static void elem_float(t_elem *x, t_float f)
{
    int indx = f, nitems, onset;
    t_symbol *templatesym, *fieldsym = x->x_fieldsym, *elemtemplatesym;
    t_template *template;
    t_template *elemtemplate;
    t_gpointer *gparent = &x->x_gparent;
    t_word *w;
    t_array *array;
    int elemsize, type;
    
    if (!gpointer_check(gparent, 0))
    {
        pd_error(x, "element: empty pointer");
        return;
    }
    if (*x->x_templatesym->s_name)
    {
        if ((templatesym = x->x_templatesym) !=
            gpointer_gettemplatesym(gparent))
        {
            pd_error(x, "elem %s: got wrong template (%s)",
                templatesym->s_name, gpointer_gettemplatesym(gparent)->s_name);
            return;
        }
    }
    else templatesym = gpointer_gettemplatesym(gparent);
    if (!(template = template_findbyname(templatesym)))
    {
        pd_error(x, "elem: couldn't find template %s", templatesym->s_name);
        return;
    }
    if (gparent->gp_stub->gs_which == GP_ARRAY) w = gparent->gp_un.gp_w;
    else w = ((t_scalar *)(gparent->gp_un.gp_gobj))->sc_vec;
    if (!template)
    {
        pd_error(x, "element: couldn't find template %s", templatesym->s_name);
        return;
    }
    if (!template_find_field(template, fieldsym,
        &onset, &type, &elemtemplatesym))
    {
        pd_error(x, "element: couldn't find array field %s", fieldsym->s_name);
        return;
    }
    if (type != DT_ARRAY)
    {
        pd_error(x, "element: field %s not of type array", fieldsym->s_name);
        return;
    }
    if (!(elemtemplate = template_findbyname(elemtemplatesym)))
    {
        pd_error(x, "element: couldn't find field template %s",
            elemtemplatesym->s_name);
        return;
    }

    elemsize = elemtemplate->t_n * sizeof(t_word);

    array = *(t_array **)(((char *)w) + onset);

    nitems = array->a_n;
    if (indx < 0) indx = 0;
    if (indx >= nitems) indx = nitems-1;

    gpointer_setarray(&x->x_gp, array, 
        (t_word *)((char *)(array->a_vec) + indx * elemsize));
    outlet_pointer(x->x_obj.ob_outlet, &x->x_gp);
}

static void elem_free(t_elem *x, t_gpointer *gp)
{
    gpointer_unset(&x->x_gp);
    gpointer_unset(&x->x_gparent);
}

static void elem_setup(void)
{
    elem_class = class_new(gensym("element"), (t_newmethod)elem_new,
        (t_method)elem_free, sizeof(t_elem), 0, A_DEFSYM, A_DEFSYM, 0);
    class_addfloat(elem_class, elem_float); 
    class_addmethod(elem_class, (t_method)elem_set, gensym("set"),
        A_SYMBOL, A_SYMBOL, 0); 
}

/* ---------------------- getsize ----------------------------- */

static t_class *getsize_class;

typedef struct _getsize
{
    t_object x_obj;
    t_symbol *x_templatesym;
    t_symbol *x_fieldsym;
} t_getsize;

static void *getsize_new(t_symbol *templatesym, t_symbol *fieldsym)
{
    t_getsize *x = (t_getsize *)pd_new(getsize_class);
    x->x_templatesym = template_getbindsym(templatesym);
    x->x_fieldsym = fieldsym;
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

static void getsize_set(t_getsize *x, t_symbol *templatesym, t_symbol *fieldsym)
{
    x->x_templatesym = template_getbindsym(templatesym);
    x->x_fieldsym = fieldsym;
}

static void getsize_pointer(t_getsize *x, t_gpointer *gp)
{
    int onset, type;
    t_symbol *templatesym, *fieldsym = x->x_fieldsym, *elemtemplatesym;
    t_template *template;
    t_word *w;
    t_array *array;
    t_gstub *gs = gp->gp_stub;
    if (!gpointer_check(gp, 0))
    {
        pd_error(x, "get: stale or empty pointer");
        return;
    }
    if (*x->x_templatesym->s_name)
    {
        if ((templatesym = x->x_templatesym) !=
            gpointer_gettemplatesym(gp))
        {
            pd_error(x, "elem %s: got wrong template (%s)",
                templatesym->s_name, gpointer_gettemplatesym(gp)->s_name);
            return;
        }
    }
    else templatesym = gpointer_gettemplatesym(gp);
    if (!(template = template_findbyname(templatesym)))
    {
        pd_error(x, "elem: couldn't find template %s", templatesym->s_name);
        return;
    }
    if (!template_find_field(template, fieldsym,
        &onset, &type, &elemtemplatesym))
    {
        pd_error(x, "getsize: couldn't find array field %s", fieldsym->s_name);
        return;
    }
    if (type != DT_ARRAY)
    {
        pd_error(x, "getsize: field %s not of type array", fieldsym->s_name);
        return;
    }
    if (gs->gs_which == GP_ARRAY) w = gp->gp_un.gp_w;
    else w = ((t_scalar *)(gp->gp_un.gp_gobj))->sc_vec;
    
    array = *(t_array **)(((char *)w) + onset);
    outlet_float(x->x_obj.ob_outlet, (t_float)(array->a_n));
}

static void getsize_setup(void)
{
    getsize_class = class_new(gensym("getsize"), (t_newmethod)getsize_new, 0,
        sizeof(t_getsize), 0, A_DEFSYM, A_DEFSYM, 0);
    class_addpointer(getsize_class, getsize_pointer); 
    class_addmethod(getsize_class, (t_method)getsize_set, gensym("set"),
        A_SYMBOL, A_SYMBOL, 0); 
}

/* ---------------------- setsize ----------------------------- */

static t_class *setsize_class;

typedef struct _setsize
{
    t_object x_obj;
    t_symbol *x_templatesym;
    t_symbol *x_fieldsym;
    t_gpointer x_gp;
} t_setsize;

static void *setsize_new(t_symbol *templatesym, t_symbol *fieldsym,
    t_floatarg newsize)
{
    t_setsize *x = (t_setsize *)pd_new(setsize_class);
    x->x_templatesym = template_getbindsym(templatesym);
    x->x_fieldsym = fieldsym;
    gpointer_init(&x->x_gp);
    
    pointerinlet_new(&x->x_obj, &x->x_gp);
    return (x);
}

static void setsize_set(t_setsize *x, t_symbol *templatesym, t_symbol *fieldsym)
{
    x->x_templatesym = template_getbindsym(templatesym);
    x->x_fieldsym = fieldsym;
}

static void setsize_float(t_setsize *x, t_float f)
{
    int nitems, onset, type;
    t_symbol *templatesym, *fieldsym = x->x_fieldsym, *elemtemplatesym;
    t_template *template;
    t_template *elemtemplate;
    t_word *w;
    t_array *array;
    int elemsize;
    int newsize = f;
    t_gpointer *gp = &x->x_gp;
    t_gstub *gs = gp->gp_stub;
    if (!gpointer_check(&x->x_gp, 0))
    {
        pd_error(x, "setsize: empty pointer");
        return;
    }
    if (*x->x_templatesym->s_name)
    {
        if ((templatesym = x->x_templatesym) !=
            gpointer_gettemplatesym(gp))
        {
            pd_error(x, "elem %s: got wrong template (%s)",
                templatesym->s_name, gpointer_gettemplatesym(gp)->s_name);
            return;
        }
    }
    else templatesym = gpointer_gettemplatesym(gp);
    if (!(template = template_findbyname(templatesym)))
    {
        pd_error(x, "elem: couldn't find template %s", templatesym->s_name);
        return;
    }
    if (!template_find_field(template, fieldsym,
        &onset, &type, &elemtemplatesym))
    {
        pd_error(x, "setsize: couldn't find array field %s", fieldsym->s_name);
        return;
    }
    if (type != DT_ARRAY)
    {
        pd_error(x,"setsize: field %s not of type array", fieldsym->s_name);
        return;
    }
    if (gs->gs_which == GP_ARRAY) w = gp->gp_un.gp_w;
    else w = ((t_scalar *)(gp->gp_un.gp_gobj))->sc_vec;

    if (!(elemtemplate = template_findbyname(elemtemplatesym)))
    {
        pd_error(x,"element: couldn't find field template %s",
            elemtemplatesym->s_name);
        return;
    }

    elemsize = elemtemplate->t_n * sizeof(t_word);

    array = *(t_array **)(((char *)w) + onset);

    if (elemsize != array->a_elemsize) bug("setsize_gpointer");

    nitems = array->a_n;
    if (newsize < 1) newsize = 1;
    if (newsize == nitems) return;
    
        /* erase the array before resizing it.  If we belong to a
        scalar it's easy, but if we belong to an element of another
        array we have to search back until we get to a scalar to erase.
        When graphics updates become queueable this may fall apart... */

    if (gs->gs_which == GP_GLIST)
    {
        if (glist_isvisible(gs->gs_un.gs_glist))
            gobj_vis(gp->gp_un.gp_gobj, gs->gs_un.gs_glist, 0);  
    }
    else
    {
        t_array *owner_array = gs->gs_un.gs_array;
        while (owner_array->a_gp.gp_stub->gs_which == GP_ARRAY)
            owner_array = owner_array->a_gp.gp_stub->gs_un.gs_array;
        if (glist_isvisible(owner_array->a_gp.gp_stub->gs_un.gs_glist))
            gobj_vis(owner_array->a_gp.gp_un.gp_gobj,
                owner_array->a_gp.gp_stub->gs_un.gs_glist, 0);  
    }
        /* if shrinking, free the scalars that will disappear */
    if (newsize < nitems)
    {
        char *elem;
        int count;
        for (elem = ((char *)array->a_vec) + newsize * elemsize,
            count = nitems = newsize; count--; elem += elemsize)
                word_free((t_word *)elem, elemtemplate);
    } 
        /* resize the array */
    array->a_vec = (char *)resizebytes(array->a_vec,
        elemsize * nitems, elemsize * newsize);
    array->a_n = newsize;
        /* if growing, initialize new scalars */
    if (newsize > nitems)
    {
        char *elem;
        int count;
        for (elem = ((char *)array->a_vec) + nitems * elemsize,
            count = newsize - nitems; count--; elem += elemsize)
                word_init((t_word *)elem, elemtemplate, gp);
    }

    /* redraw again. */
    if (gs->gs_which == GP_GLIST)
    {
        if (glist_isvisible(gs->gs_un.gs_glist))
            gobj_vis(gp->gp_un.gp_gobj, gs->gs_un.gs_glist, 1);
    }
    else
    {
        t_array *owner_array = gs->gs_un.gs_array;
        while (owner_array->a_gp.gp_stub->gs_which == GP_ARRAY)
            owner_array = owner_array->a_gp.gp_stub->gs_un.gs_array;
        if (glist_isvisible(owner_array->a_gp.gp_stub->gs_un.gs_glist))
            gobj_vis(owner_array->a_gp.gp_un.gp_gobj,
                owner_array->a_gp.gp_stub->gs_un.gs_glist, 1);
    }
}

static void setsize_free(t_setsize *x)
{
    gpointer_unset(&x->x_gp);
}

static void setsize_setup(void)
{
    setsize_class = class_new(gensym("setsize"), (t_newmethod)setsize_new,
        (t_method)setsize_free, sizeof(t_setsize), 0,
        A_DEFSYM, A_DEFSYM, A_DEFFLOAT, 0);
    class_addfloat(setsize_class, setsize_float);
    class_addmethod(setsize_class, (t_method)setsize_set, gensym("set"),
        A_SYMBOL, A_SYMBOL, 0); 
}

/* ---------------------- append ----------------------------- */

static t_class *append_class;

typedef struct _appendvariable
{
    t_symbol *gv_sym;
    t_float gv_f;
} t_appendvariable;

typedef struct _append
{
    t_object x_obj;
    t_gpointer x_gp;
    t_symbol *x_templatesym;
    int x_nin;
    t_appendvariable *x_variables;
} t_append;

static void *append_new(t_symbol *why, int argc, t_atom *argv)
{
    t_append *x = (t_append *)pd_new(append_class);
    int varcount, i;
    t_atom at, *varvec;
    t_appendvariable *sp;

    x->x_templatesym = template_getbindsym(atom_getsymbolarg(0, argc, argv));
    if (argc < 2)
    {
        varcount = 1;
        varvec = &at;
        SETSYMBOL(&at, &s_);
    }
    else varcount = argc - 1, varvec = argv + 1;
    x->x_variables
        = (t_appendvariable *)getbytes(varcount * sizeof (*x->x_variables));
    x->x_nin = varcount;
    for (i = 0, sp = x->x_variables; i < varcount; i++, sp++)
    {
        sp->gv_sym = atom_getsymbolarg(i, varcount, varvec);
        sp->gv_f = 0;
        if (i) floatinlet_new(&x->x_obj, &sp->gv_f);
    }
    pointerinlet_new(&x->x_obj, &x->x_gp);
    outlet_new(&x->x_obj, &s_pointer);
    gpointer_init(&x->x_gp);
    return (x);
}

static void append_set(t_append *x, t_symbol *templatesym, t_symbol *field)
{
    if (x->x_nin != 1)
    {
        pd_error(x, "set: cannot set multiple fields.");
    }
    else
    {
       x->x_templatesym = template_getbindsym(templatesym); 
       x->x_variables->gv_sym = field;
       x->x_variables->gv_f = 0;
    }
}

static void append_float(t_append *x, t_float f)
{
    int nitems = x->x_nin, i;
    t_symbol *templatesym = x->x_templatesym;
    t_template *template;
    t_appendvariable *vp;
    t_gpointer *gp = &x->x_gp;
    t_gstub *gs = gp->gp_stub;
    t_word *vec;
    t_scalar *sc, *oldsc;
    t_glist *glist;

    if (!templatesym->s_name)
    {
        pd_error(x, "append: no template supplied");
        return;
    }
    template = template_findbyname(templatesym);
    if (!template)
    {
        pd_error(x, "append: couldn't find template %s", templatesym->s_name);
        return;
    }
    if (!gs)
    {
        pd_error(x, "append: no current pointer");
        return;
    }
    if (gs->gs_which != GP_GLIST)
    {
        pd_error(x, "append: lists only, not arrays");
        return;
    }
    glist = gs->gs_un.gs_glist;
    if (glist->gl_valid != gp->gp_valid)
    {
        pd_error(x, "append: stale pointer");
        return;
    }
    if (!nitems) return;
    x->x_variables[0].gv_f = f;

    sc = scalar_new(glist, templatesym);
    if (!sc)
    {
        pd_error(x, "%s: couldn't create scalar", templatesym->s_name);
        return;
    }
    oldsc = (t_scalar *)(gp->gp_un.gp_gobj);
    
    if (oldsc)
    {
        sc->sc_gobj.g_next = oldsc->sc_gobj.g_next;
        oldsc->sc_gobj.g_next = &sc->sc_gobj;
    }
    else
    {
        sc->sc_gobj.g_next = glist->gl_list;
        glist->gl_list = &sc->sc_gobj;
    }

    gp->gp_un.gp_gobj = (t_gobj *)sc;
    vec = sc->sc_vec;
    for (i = 0, vp = x->x_variables; i < nitems; i++, vp++)
    {
        template_setfloat(template, vp->gv_sym, vec, vp->gv_f, 1);
    }
 
    if (glist_isvisible(glist_getcanvas(glist)))
        gobj_vis(&sc->sc_gobj, glist, 1);
    /*  scalar_redraw(sc, glist);  ... have to do 'vis' instead here because
    redraw assumes we're already visible??? ... */

    outlet_pointer(x->x_obj.ob_outlet, gp);
}

static void append_free(t_append *x)
{
    freebytes(x->x_variables, x->x_nin * sizeof (*x->x_variables));
    gpointer_unset(&x->x_gp);
}

static void append_setup(void)
{
    append_class = class_new(gensym("append"), (t_newmethod)append_new,
        (t_method)append_free, sizeof(t_append), 0, A_GIMME, 0);
    class_addfloat(append_class, append_float); 
    class_addmethod(append_class, (t_method)append_set, gensym("set"),
        A_SYMBOL, A_SYMBOL, 0); 
}

/* ---------------------- sublist ----------------------------- */

static t_class *sublist_class;

typedef struct _sublist
{
    t_object x_obj;
    t_symbol *x_templatesym;
    t_symbol *x_fieldsym;
    t_gpointer x_gp;
} t_sublist;

static void *sublist_new(t_symbol *templatesym, t_symbol *fieldsym)
{
    t_sublist *x = (t_sublist *)pd_new(sublist_class);
    x->x_templatesym = canvas_makebindsym(templatesym);
    x->x_fieldsym = fieldsym;
    gpointer_init(&x->x_gp);
    outlet_new(&x->x_obj, &s_pointer);
    return (x);
}

static void sublist_pointer(t_sublist *x, t_gpointer *gp)
{
    t_symbol *templatesym = x->x_templatesym, *dummy;
    t_template *template = template_findbyname(templatesym);
    t_gstub *gs = gp->gp_stub;
    int onset, type;
    t_word *w;

    if (!template)
    {
        pd_error(x, "sublist: couldn't find template %s", templatesym->s_name);
        return;
    }
    if (!gpointer_check(gp, 0))
    {
        pd_error(x, "get: stale or empty pointer");
        return;
    }
    if (!template_find_field(template, x->x_fieldsym,
        &onset, &type, &dummy))
    {
        pd_error(x, "sublist: couldn't find field %s", x->x_fieldsym->s_name);
        return;
    }
    if (type != DT_TEXT)
    {
        pd_error(x, "sublist: field %s not of type list", x->x_fieldsym->s_name);
        return;
    }
    if (gs->gs_which == GP_ARRAY) w = gp->gp_un.gp_w;
    else w = ((t_scalar *)(gp->gp_un.gp_gobj))->sc_vec;
    
    gpointer_setglist(&x->x_gp, *(t_glist **)(((char *)w) + onset), 0);

    outlet_pointer(x->x_obj.ob_outlet, &x->x_gp);
}

static void sublist_free(t_sublist *x, t_gpointer *gp)
{
    gpointer_unset(&x->x_gp);
}

static void sublist_setup(void)
{
    sublist_class = class_new(gensym("sublist"), (t_newmethod)sublist_new,
        (t_method)sublist_free, sizeof(t_sublist), 0, A_DEFSYM, A_DEFSYM, 0);
    class_addpointer(sublist_class, sublist_pointer); 
}

/* ---------------------- field ----------------------------- */

static t_class *field_class;

typedef struct _field
{
    t_object x_obj;
    t_symbol *x_s;
    t_template *x_template;
    t_canvas *x_canvas;
} t_field;

static void *field_new(t_symbol *s)
{
    t_field *x = (t_field *)pd_new(field_class);
    x->x_s = s;
    x->x_canvas = canvas_getrootfor(canvas_getcurrent());
    x->x_template = NULL;
    outlet_new(&x->x_obj, &s_list);
    return (x);
}

static void field_set(t_field *x, t_symbol *s)
{
    x->x_s = s;
}

    /* Contortions to get and possibly cache template after
       the first lookup. We can't get it in field_new because
       word_init only sets the template after we've been created. */
static t_template *field_gettemplate(t_field *x)
{
    if (x->x_template)
        return x->x_template;
    else
    {
        if (x->x_canvas->gl_templatesym)
        {
            x->x_template = template_findbyname(x->x_canvas->gl_templatesym);
            return x->x_template;
        }
        else
            return 0;
    }
}

static void field_bang(t_field *x)
{
    t_template *template = field_gettemplate(x);
    if (template)
    {
        t_word *vec = x->x_canvas->gl_vec;
        t_symbol *fieldsym = x->x_s;
        int onset, type;
        t_symbol *arraytype;
        if (template_find_field(template, fieldsym, &onset, &type, &arraytype))
        {
            if (type == DT_FLOAT)
                outlet_float(x->x_obj.ob_outlet,
                    *(t_float *)(((char *)vec) + onset));
            else if (type == DT_SYMBOL)
                outlet_symbol(x->x_obj.ob_outlet,
                    *(t_symbol **)(((char *)vec)+ onset));
            else pd_error(x, "field: %s.%s is not a number or symbol",
                    template->t_sym->s_name, fieldsym->s_name);
        }
        else pd_error(x, "field: %s.%s: no such field",
            template->t_sym->s_name, fieldsym->s_name);
    }
    else /* send a bang to signal that we're not in a scalar */
        outlet_bang(x->x_obj.ob_outlet);
}

static void field_setvalue(t_field *x, t_symbol *s, int argc, t_atom *argv)
{
    t_template *template = field_gettemplate(x);
    if (!template) return;
    t_word *vec = x->x_canvas->gl_vec;
    t_gpointer *gp = &x->x_canvas->gl_gp;
    t_gstub *gs = gp->gp_stub;
    /* not needed yet... but may be needed if we find a cleaner way to
       update graphics for array elements */
    // int onset, type;
    //t_symbol *arraytype;
    t_symbol *fieldsym = x->x_s;
    if (argc)
    {
        if (argv->a_type == A_FLOAT)
            template_setfloat(template, fieldsym, vec,
                atom_getfloatarg(0, argc, argv), 1);
        else if (argv->a_type == A_SYMBOL)
            template_setsymbol(template, fieldsym, vec,
                atom_getsymbolarg(0, argc, argv), 1);
        else pd_error(x, "field: incoming value must be float or symbol");
    }
    if (gp->gp_stub->gs_which == GP_GLIST)
        scalar_configure((t_scalar *)(gp->gp_un.gp_gobj), gs->gs_un.gs_glist);  
    else
    {
        t_array *owner_array = gs->gs_un.gs_array;
        while (owner_array->a_gp.gp_stub->gs_which == GP_ARRAY)
            owner_array = owner_array->a_gp.gp_stub->gs_un.gs_array;
        scalar_redraw((t_scalar *)(owner_array->a_gp.gp_un.gp_gobj),
            owner_array->a_gp.gp_stub->gs_un.gs_glist);  
    }
}

static void field_symbol(t_field *x, t_symbol *s)
{
    t_atom at[1];
    SETSYMBOL(at, s);
    field_setvalue(x, gensym("symbol"), 1, at);
}

static void field_float(t_field *x, t_floatarg f)
{
    t_atom at[1];
    SETFLOAT(at, f);
    field_setvalue(x, gensym("float"), 1, at);
}

static void field_setup(void)
{
    field_class = class_new(gensym("field"), (t_newmethod)field_new,
        0, sizeof(t_field), 0, A_DEFSYM, 0);
    class_addfloat(field_class, field_float); 
    class_addsymbol(field_class, field_symbol); 
    class_addbang(field_class, field_bang); 
    class_addmethod(field_class, (t_method)field_set, gensym("set"),
        A_SYMBOL, 0); 
}

/* ----------------- setup function ------------------- */

void g_traversal_setup(void)
{
    ptrobj_setup();
    get_setup();
    set_setup();
    elem_setup();
    getsize_setup();
    setsize_setup();
    append_setup();
    sublist_setup();
    field_setup();
}
