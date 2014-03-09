/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>  /* for matrix transforms */

#include "m_pd.h"
#include "s_stuff.h"    /* for sys_hostfontsize */
#include "g_canvas.h"

void array_redraw(t_array *a, t_glist *glist);
void graph_graphrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2);
/*
This file contains text objects you would put in a canvas to define a
template.  Templates describe objects of type "array" (g_array.c) and
"scalar" (g_scalar.c).
*/

    /* the structure of a "struct" object (also the obsolete "gtemplate"
    you get when using the name "template" in a box.) */

struct _gtemplate
{
    t_object x_obj;
    t_template *x_template;
    t_canvas *x_owner;
    t_symbol *x_sym;
    struct _gtemplate *x_next;
    int x_argc;
    t_atom *x_argv;
};

/* ---------------- forward definitions ---------------- */

static void template_conformarray(t_template *tfrom, t_template *tto,
    int *conformaction, t_array *a);
static void template_conformglist(t_template *tfrom, t_template *tto,
    t_glist *glist,  int *conformaction);

/* ---------------------- storage ------------------------- */

static t_class *gtemplate_class;
static t_class *template_class;

/* there's a pre-defined "float" template.  LATER should we bind this
to a symbol such as "pd-float"??? */

    /* return true if two dataslot definitions match */
static int dataslot_matches(t_dataslot *ds1, t_dataslot *ds2,
    int nametoo)
{
    return ((!nametoo || ds1->ds_name == ds2->ds_name) &&
        ds1->ds_type == ds2->ds_type &&
            (ds1->ds_type != DT_ARRAY ||
                ds1->ds_arraytemplate == ds2->ds_arraytemplate));
}

/* -- templates, the active ingredient in gtemplates defined below. ------- */

t_template *template_new(t_symbol *templatesym, int argc, t_atom *argv)
{
    t_template *x = (t_template *)pd_new(template_class);
    x->t_n = 0;
    x->t_vec = (t_dataslot *)t_getbytes(0);
    while (argc > 0)
    {
        int newtype, oldn, newn;
        t_symbol *newname, *newarraytemplate = &s_, *newtypesym;
        if (argc < 2 || argv[0].a_type != A_SYMBOL ||
            argv[1].a_type != A_SYMBOL)
                goto bad;
        newtypesym = argv[0].a_w.w_symbol;
        newname = argv[1].a_w.w_symbol;
        if (newtypesym == &s_float)
            newtype = DT_FLOAT;
        else if (newtypesym == &s_symbol)
            newtype = DT_SYMBOL;
        else if (newtypesym == &s_list)
            newtype = DT_LIST;
        else if (newtypesym == gensym("array"))
        {
            if (argc < 3 || argv[2].a_type != A_SYMBOL)
            {
                pd_error(x, "array lacks element template or name");
                goto bad;
            }
            newarraytemplate = canvas_makebindsym(argv[2].a_w.w_symbol);
            newtype = DT_ARRAY;
            argc--;
            argv++;
        }
        else
        {
            pd_error(x, "%s: no such type", newtypesym->s_name);
            goto bad;
        }
        newn = (oldn = x->t_n) + 1;
        x->t_vec = (t_dataslot *)t_resizebytes(x->t_vec,
            oldn * sizeof(*x->t_vec), newn * sizeof(*x->t_vec));
        x->t_n = newn;
        x->t_vec[oldn].ds_type = newtype;
        x->t_vec[oldn].ds_name = newname;
        x->t_vec[oldn].ds_arraytemplate = newarraytemplate;
    bad: 
        argc -= 2; argv += 2;
    }
    if (*templatesym->s_name)
    {
        x->t_sym = templatesym;
        pd_bind(&x->t_pdobj, x->t_sym);
    }
    else x->t_sym = templatesym;
    return (x);
}

int template_size(t_template *x)
{
    return (x->t_n * sizeof(t_word));
}

int template_find_field(t_template *x, t_symbol *name, int *p_onset,
    int *p_type, t_symbol **p_arraytype)
{
    t_template *t;
    int i, n;
    if (!x)
    {
        bug("template_find_field");
        return (0);
    }
    n = x->t_n;
    for (i = 0; i < n; i++)
        if (x->t_vec[i].ds_name == name)
    {
        *p_onset = i * sizeof(t_word);
        *p_type = x->t_vec[i].ds_type;
        *p_arraytype = x->t_vec[i].ds_arraytemplate;
        return (1);
    }
    return (0);
}

t_float template_getfloat(t_template *x, t_symbol *fieldname, t_word *wp,
    int loud)
{
    int onset, type;
    t_symbol *arraytype;
    t_float val = 0;
    if (template_find_field(x, fieldname, &onset, &type, &arraytype))
    {
        if (type == DT_FLOAT)
            val = *(t_float *)(((char *)wp) + onset);
        else if (loud) error("%s.%s: not a number",
            x->t_sym->s_name, fieldname->s_name);
    }
    else if (loud) error("%s.%s: no such field",
        x->t_sym->s_name, fieldname->s_name);
    return (val);
}

void template_setfloat(t_template *x, t_symbol *fieldname, t_word *wp, 
    t_float f, int loud)
{
    int onset, type;
    t_symbol *arraytype;
    if (template_find_field(x, fieldname, &onset, &type, &arraytype))
     {
        if (type == DT_FLOAT)
            *(t_float *)(((char *)wp) + onset) = f;
        else if (loud) error("%s.%s: not a number",
            x->t_sym->s_name, fieldname->s_name);
    }
    else if (loud) error("%s.%s: no such field",
        x->t_sym->s_name, fieldname->s_name);
}

t_symbol *template_getsymbol(t_template *x, t_symbol *fieldname, t_word *wp,
    int loud)
{
    int onset, type;
    t_symbol *arraytype;
    t_symbol *val = &s_;
    if (template_find_field(x, fieldname, &onset, &type, &arraytype))
    {
        if (type == DT_SYMBOL)
            val = *(t_symbol **)(((char *)wp) + onset);
        else if (loud) error("%s.%s: not a symbol",
            x->t_sym->s_name, fieldname->s_name);
    }
    else if (loud) error("%s.%s: no such field",
        x->t_sym->s_name, fieldname->s_name);
    return (val);
}

void template_setsymbol(t_template *x, t_symbol *fieldname, t_word *wp, 
    t_symbol *s, int loud)
{
    int onset, type;
    t_symbol *arraytype;
    if (template_find_field(x, fieldname, &onset, &type, &arraytype))
     {
        if (type == DT_SYMBOL)
            *(t_symbol **)(((char *)wp) + onset) = s;
        else if (loud) error("%s.%s: not a symbol",
            x->t_sym->s_name, fieldname->s_name);
    }
    else if (loud) error("%s.%s: no such field",
        x->t_sym->s_name, fieldname->s_name);
}

    /* stringent check to see if a "saved" template, x2, matches the current
        one (x1).  It's OK if x1 has additional scalar elements but not (yet)
        arrays or lists.  This is used for reading in "data files". */
int template_match(t_template *x1, t_template *x2)
{
    int i;
    if (x1->t_n < x2->t_n)
        return (0);
    for (i = x2->t_n; i < x1->t_n; i++)
    {
        if (x1->t_vec[i].ds_type == DT_ARRAY || 
            x1->t_vec[i].ds_type == DT_LIST)
                return (0);
    }
    if (x2->t_n > x1->t_n)
        post("add elements...");
    for (i = 0; i < x2->t_n; i++)
        if (!dataslot_matches(&x1->t_vec[i], &x2->t_vec[i], 1))
            return (0);
    return (1);
}

/* --------------- CONFORMING TO CHANGES IN A TEMPLATE ------------ */

/* the following routines handle updating scalars to agree with changes
in their template.  The old template is assumed to be the "installed" one
so we can delete old items; but making new ones we have to avoid scalar_new
which would make an old one whereas we will want a new one (but whose array
elements might still be old ones.)
    LATER deal with graphics updates too... */

    /* conform the word vector of a scalar to the new template */    
static void template_conformwords(t_template *tfrom, t_template *tto,
    int *conformaction, t_word *wfrom, t_word *wto)
{
    int nfrom = tfrom->t_n, nto = tto->t_n, i;
    for (i = 0; i < nto; i++)
    {
        if (conformaction[i] >= 0)
        {
                /* we swap the two, in case it's an array or list, so that
                when "wfrom" is deleted the old one gets cleaned up. */
            t_word wwas = wto[i];
            wto[i] = wfrom[conformaction[i]];
            wfrom[conformaction[i]] = wwas;
        }
    }
}

    /* conform a scalar, recursively conforming sublists and arrays  */
static t_scalar *template_conformscalar(t_template *tfrom, t_template *tto,
    int *conformaction, t_glist *glist, t_scalar *scfrom)
{
    t_scalar *x;
    t_gpointer gp;
    int nto = tto->t_n, nfrom = tfrom->t_n, i;
    t_template *scalartemplate;
    /* post("conform scalar"); */
        /* possibly replace the scalar */
    if (scfrom->sc_template == tfrom->t_sym)
    {
            /* see scalar_new() for comment about the gpointer. */
        gpointer_init(&gp);
        x = (t_scalar *)getbytes(sizeof(t_scalar) +
            (tto->t_n - 1) * sizeof(*x->sc_vec));
        x->sc_gobj.g_pd = scalar_class;
        x->sc_template = tfrom->t_sym;
        gpointer_setglist(&gp, glist, x);
            /* Here we initialize to the new template, but array and list
            elements will still belong to old template. */
        word_init(x->sc_vec, tto, &gp);

        template_conformwords(tfrom, tto, conformaction,
            scfrom->sc_vec, x->sc_vec);
            
            /* replace the old one with the new one in the list */
        if (glist->gl_list == &scfrom->sc_gobj)
        {
            glist->gl_list = &x->sc_gobj;
            x->sc_gobj.g_next = scfrom->sc_gobj.g_next;
        }
        else
        {
            t_gobj *y, *y2;
            for (y = glist->gl_list; y2 = y->g_next; y = y2)
                if (y2 == &scfrom->sc_gobj)
            {
                x->sc_gobj.g_next = y2->g_next;
                y->g_next = &x->sc_gobj;
                goto nobug;
            }
            bug("template_conformscalar");
        nobug: ;
        }
            /* burn the old one */
        pd_free(&scfrom->sc_gobj.g_pd);
        scalartemplate = tto;
    }
    else
    {
        x = scfrom;
        scalartemplate = template_findbyname(x->sc_template);
    }
        /* convert all array elements and sublists */
    for (i = 0; i < scalartemplate->t_n; i++)
    {
        t_dataslot *ds = scalartemplate->t_vec + i;
        if (ds->ds_type == DT_LIST)
        {
            t_glist *gl2 = x->sc_vec[i].w_list;
            template_conformglist(tfrom, tto, gl2, conformaction);
        }
        else if (ds->ds_type == DT_ARRAY)
        {
            template_conformarray(tfrom, tto, conformaction, 
                x->sc_vec[i].w_array);
        }
    }
    return (x);
}

    /* conform an array, recursively conforming sublists and arrays  */
static void template_conformarray(t_template *tfrom, t_template *tto,
    int *conformaction, t_array *a)
{
    int i, j;
    t_template *scalartemplate = 0;
    if (a->a_templatesym == tfrom->t_sym)
    {
        /* the array elements must all be conformed */
        int oldelemsize = sizeof(t_word) * tfrom->t_n,
            newelemsize = sizeof(t_word) * tto->t_n;
        char *newarray = getbytes(newelemsize * a->a_n);
        char *oldarray = a->a_vec;
        if (a->a_elemsize != oldelemsize)
            bug("template_conformarray");
        for (i = 0; i < a->a_n; i++)
        {
            t_word *wp = (t_word *)(newarray + newelemsize * i);
            word_init(wp, tto, &a->a_gp);
            template_conformwords(tfrom, tto, conformaction,
                (t_word *)(oldarray + oldelemsize * i), wp);
            word_free((t_word *)(oldarray + oldelemsize * i), tfrom);
        }
        scalartemplate = tto;
        a->a_vec = newarray;
        freebytes(oldarray, oldelemsize * a->a_n);
    }
    else scalartemplate = template_findbyname(a->a_templatesym);
        /* convert all arrays and sublist fields in each element of the array */
    for (i = 0; i < a->a_n; i++)
    {
        t_word *wp = (t_word *)(a->a_vec + sizeof(t_word) * a->a_n * i);
        for (j = 0; j < scalartemplate->t_n; j++)
        {
            t_dataslot *ds = scalartemplate->t_vec + j;
            if (ds->ds_type == DT_LIST)
            {
                t_glist *gl2 = wp[j].w_list;
                template_conformglist(tfrom, tto, gl2, conformaction);
            }
            else if (ds->ds_type == DT_ARRAY)
            {
                template_conformarray(tfrom, tto, conformaction, 
                    wp[j].w_array);
            }
        }
    }
}

    /* this routine searches for every scalar in the glist that belongs
    to the "from" template and makes it belong to the "to" template.  Descend
    glists recursively.
    We don't handle redrawing here; this is to be filled in LATER... */

t_array *garray_getarray(t_garray *x);

static void template_conformglist(t_template *tfrom, t_template *tto,
    t_glist *glist,  int *conformaction)
{
    t_gobj *g;
    /* post("conform glist %s", glist->gl_name->s_name); */
    for (g = glist->gl_list; g; g = g->g_next)
    {
        if (pd_class(&g->g_pd) == scalar_class)
            g = &template_conformscalar(tfrom, tto, conformaction,
                glist, (t_scalar *)g)->sc_gobj;
        else if (pd_class(&g->g_pd) == canvas_class)
            template_conformglist(tfrom, tto, (t_glist *)g, conformaction);
        else if (pd_class(&g->g_pd) == garray_class)
            template_conformarray(tfrom, tto, conformaction,
                garray_getarray((t_garray *)g));
    }
}

    /* globally conform all scalars from one template to another */ 
void template_conform(t_template *tfrom, t_template *tto)
{
    int nto = tto->t_n, nfrom = tfrom->t_n, i, j,
        *conformaction = (int *)getbytes(sizeof(int) * nto),
        *conformedfrom = (int *)getbytes(sizeof(int) * nfrom), doit = 0;
    for (i = 0; i < nto; i++)
        conformaction[i] = -1;
    for (i = 0; i < nfrom; i++)
        conformedfrom[i] = 0;
    for (i = 0; i < nto; i++)
    {
        t_dataslot *dataslot = &tto->t_vec[i];
        for (j = 0; j < nfrom; j++)
        {
            t_dataslot *dataslot2 = &tfrom->t_vec[j];
            if (dataslot_matches(dataslot, dataslot2, 1))
            {
                conformaction[i] = j;
                conformedfrom[j] = 1;
            }
        }
    }
    for (i = 0; i < nto; i++)
        if (conformaction[i] < 0)
    {
        t_dataslot *dataslot = &tto->t_vec[i];
        for (j = 0; j < nfrom; j++)
            if (!conformedfrom[j] &&
                dataslot_matches(dataslot, &tfrom->t_vec[j], 0))
        {
            conformaction[i] = j;
            conformedfrom[j] = 1;
        }
    }
    if (nto != nfrom)
        doit = 1;
    else for (i = 0; i < nto; i++)
        if (conformaction[i] != i)
            doit = 1;

    if (doit)
    {
        t_glist *gl;
        post("conforming template '%s' to new structure",
            tfrom->t_sym->s_name);
        for (i = 0; i < nto; i++)
            post("... %d", conformaction[i]);
        for (gl = canvas_list; gl; gl = gl->gl_next)
            template_conformglist(tfrom, tto, gl, conformaction);
    }
    freebytes(conformaction, sizeof(int) * nto);
    freebytes(conformedfrom, sizeof(int) * nfrom);
}

t_template *template_findbyname(t_symbol *s)
{
    return ((t_template *)pd_findbyclass(s, template_class));
}

t_canvas *template_findcanvas(t_template *template)
{
    t_gtemplate *gt;
    if (!template) {
        bug("template_findcanvas");
		return (0);
	}
    if (!(gt = template->t_list))
        return (0);
    return (gt->x_owner);
    /* return ((t_canvas *)pd_findbyclass(template->t_sym, canvas_class)); */
}

void template_notify(t_template *template, t_symbol *s, int argc, t_atom *argv)
{
    if (template->t_list)
        outlet_anything(template->t_list->x_obj.ob_outlet, s, argc, argv);
}

    /* bash the first of (argv) with a pointer to a scalar, and send on
    to template as a notification message */
void template_notifyforscalar(t_template *template, t_glist *owner,
    t_scalar *sc, t_symbol *s, int argc, t_atom *argv)
{
    t_gpointer gp;
    gpointer_init(&gp);
    gpointer_setglist(&gp, owner, sc);
    SETPOINTER(argv, &gp);
    template_notify(template, s, argc, argv);
    gpointer_unset(&gp);
}

    /* call this when reading a patch from a file to declare what templates
    we'll need.  If there's already a template, check if it matches.
    If it doesn't it's still OK as long as there are no "struct" (gtemplate)
    objects hanging from it; we just conform everyone to the new template.
    If there are still struct objects belonging to the other template, we're
    in trouble.  LATER we'll figure out how to conform the new patch's objects
    to the pre-existing struct. */
static void *template_usetemplate(void *dummy, t_symbol *s,
    int argc, t_atom *argv)
{
    t_template *x;
    t_symbol *templatesym =
        canvas_makebindsym(atom_getsymbolarg(0, argc, argv));
    if (!argc)
        return (0);
    argc--; argv++;
            /* check if there's already a template by this name. */
    if ((x = (t_template *)pd_findbyclass(templatesym, template_class)))
    {
        t_template *y = template_new(&s_, argc, argv), *y2;
            /* If the new template is the same as the old one,
            there's nothing to do.  */
        if (!template_match(x, y))
        {
                /* Are there "struct" objects upholding this template? */
            if (x->t_list)
            {
                    /* don't know what to do here! */
                error("%s: template mismatch",
                    templatesym->s_name);
            }
            else
            {
                    /* conform everyone to the new template */
                template_conform(x, y);
                pd_free(&x->t_pdobj);
                y2 = template_new(templatesym, argc, argv);
                y2->t_list = 0;
            }
        }
        pd_free(&y->t_pdobj);
    }
        /* otherwise, just make one. */
    else template_new(templatesym, argc, argv);
    return (0);
}

    /* here we assume someone has already cleaned up all instances of this. */
void template_free(t_template *x)
{
    if (*x->t_sym->s_name)
        pd_unbind(&x->t_pdobj, x->t_sym);
    t_freebytes(x->t_vec, x->t_n * sizeof(*x->t_vec));
}

static void template_setup(void)
{
    template_class = class_new(gensym("template"), 0, (t_method)template_free,
        sizeof(t_template), CLASS_PD, 0);
    class_addmethod(pd_canvasmaker, (t_method)template_usetemplate,
        gensym("struct"), A_GIMME, 0);
        
}

/* ---------------- gtemplates.  One per canvas. ----------- */

/* "Struct": an object that searches for, and if necessary creates, 
a template (above).  Other objects in the canvas then can give drawing
instructions for the template.  The template doesn't go away when the
"struct" is deleted, so that you can replace it with
another one to add new fields, for example. */

static void *gtemplate_donew(t_symbol *sym, int argc, t_atom *argv)
{
    t_gtemplate *x = (t_gtemplate *)pd_new(gtemplate_class);
    t_template *t = template_findbyname(sym);
    int i;
    t_symbol *sx = gensym("x");
    x->x_owner = canvas_getcurrent();
    x->x_next = 0;
    x->x_sym = sym;
    x->x_argc = argc;
    x->x_argv = (t_atom *)getbytes(argc * sizeof(t_atom));
    for (i = 0; i < argc; i++)
        x->x_argv[i] = argv[i];

        /* already have a template by this name? */
    if (t)
    {
        x->x_template = t;
            /* if it's already got a "struct" object we
            just tack this one to the end of the list and leave it
            there. */
        if (t->t_list)
        {
            t_gtemplate *x2, *x3;
            for (x2 = x->x_template->t_list; x3 = x2->x_next; x2 = x3)
                ;
            x2->x_next = x;
            post("template %s: warning: already exists.", sym->s_name);
        }
        else
        {
                /* if there's none, we just replace the template with
                our own and conform it. */
            t_template *y = template_new(&s_, argc, argv);
            canvas_redrawallfortemplate(t, 2);
                /* Unless the new template is different from the old one,
                there's nothing to do.  */
            if (!template_match(t, y))
            {
                    /* conform everyone to the new template */
                template_conform(t, y);
                pd_free(&t->t_pdobj);
                t = template_new(sym, argc, argv);
            }
            pd_free(&y->t_pdobj);
            t->t_list = x;
            canvas_redrawallfortemplate(t, 1);
        }
    }
    else
    {
            /* otherwise make a new one and we're the only struct on it. */
        x->x_template = t = template_new(sym, argc, argv);
        t->t_list = x;
    }
    outlet_new(&x->x_obj, 0);
    return (x);
}

static void *gtemplate_new(t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *sym = atom_getsymbolarg(0, argc, argv);
    if (argc >= 1)
        argc--; argv++;
    return (gtemplate_donew(canvas_makebindsym(sym), argc, argv));
}

    /* old version (0.34) -- delete 2003 or so */
static void *gtemplate_new_old(t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *sym = canvas_makebindsym(canvas_getcurrent()->gl_name);
    static int warned;
    if (!warned)
    {
        post("warning -- 'template' (%s) is obsolete; replace with 'struct'",
            sym->s_name);
        warned = 1;
    }
    return (gtemplate_donew(sym, argc, argv));
}

t_template *gtemplate_get(t_gtemplate *x)
{
    return (x->x_template);
}

static void gtemplate_free(t_gtemplate *x)
{
        /* get off the template's list */
    t_template *t = x->x_template;
    t_gtemplate *y;
    if (x == t->t_list)
    {
        canvas_redrawallfortemplate(t, 2);
        if (x->x_next)
        {
                /* if we were first on the list, and there are others on
                the list, make a new template corresponding to the new
                first-on-list and replace the existing template with it. */
            t_template *z = template_new(&s_,
                x->x_next->x_argc, x->x_next->x_argv);
            template_conform(t, z);
            pd_free(&t->t_pdobj);
            pd_free(&z->t_pdobj);
            z = template_new(x->x_sym, x->x_next->x_argc, x->x_next->x_argv);
            z->t_list = x->x_next;
            for (y = z->t_list; y ; y = y->x_next)
                y->x_template = z;
        }
        else t->t_list = 0;
        canvas_redrawallfortemplate(t, 1);
    }
    else
    {
        t_gtemplate *x2, *x3;
        for (x2 = t->t_list; x3 = x2->x_next; x2 = x3)
        {
            if (x == x3)
            {
                x2->x_next = x3->x_next;
                break;
            }
        }
    }
    freebytes(x->x_argv, sizeof(t_atom) * x->x_argc);
}

static void gtemplate_setup(void)
{
    gtemplate_class = class_new(gensym("struct"),
        (t_newmethod)gtemplate_new, (t_method)gtemplate_free,
        sizeof(t_gtemplate), CLASS_NOINLET, A_GIMME, 0);
    class_addcreator((t_newmethod)gtemplate_new_old, gensym("template"),
        A_GIMME, 0);
}

/* ---------------  FIELD DESCRIPTORS ---------------------- */

/* a field descriptor can hold a constant or a variable; if a variable,
it's the name of a field in the template we belong to.  LATER, we might
want to cache the offset of the field so we don't have to search for it
every single time we draw the object.
*/

struct _fielddesc
{
    char fd_type;       /* LATER consider removing this? */
    char fd_var;
    union
    {
        t_float fd_float;       /* the field is a constant float */
        t_symbol *fd_symbol;    /* the field is a constant symbol */
        t_symbol *fd_varsym;    /* the field is variable and this is the name */
    } fd_un;
    float fd_v1;        /* min and max values */
    float fd_v2;
    float fd_screen1;   /* min and max screen values */
    float fd_screen2;
    float fd_quantum;   /* quantization in value */ 
};

static void fielddesc_setfloat_const(t_fielddesc *fd, t_float f)
{
    fd->fd_type = A_FLOAT;
    fd->fd_var = 0;
    fd->fd_un.fd_float = f;
    fd->fd_v1 = fd->fd_v2 = fd->fd_screen1 = fd->fd_screen2 =
        fd->fd_quantum = 0;
}

static void fielddesc_setsymbol_const(t_fielddesc *fd, t_symbol *s)
{
    fd->fd_type = A_SYMBOL;
    fd->fd_var = 0;
    fd->fd_un.fd_symbol = s;
    fd->fd_v1 = fd->fd_v2 = fd->fd_screen1 = fd->fd_screen2 =
        fd->fd_quantum = 0;
}

static void fielddesc_setfloat_var(t_fielddesc *fd, t_symbol *s)
{
    char *s1, *s2, *s3, strbuf[MAXPDSTRING];
    int i;
    fd->fd_type = A_FLOAT;
    fd->fd_var = 1;
    if (!(s1 = strchr(s->s_name, '(')) || !(s2 = strchr(s->s_name, ')'))
        || (s1 > s2))
    {
        fd->fd_un.fd_varsym = s;
        fd->fd_v1 = fd->fd_v2 = fd->fd_screen1 = fd->fd_screen2 =
            fd->fd_quantum = 0;
    }
    else
    {
        int cpy = s1 - s->s_name, got;
        if (cpy > MAXPDSTRING-5)
            cpy = MAXPDSTRING-5;
        strncpy(strbuf, s->s_name, cpy);
        strbuf[cpy] = 0;
        fd->fd_un.fd_varsym = gensym(strbuf);
        got = sscanf(s1, "(%f:%f)(%f:%f)(%f)",
            &fd->fd_v1, &fd->fd_v2, &fd->fd_screen1, &fd->fd_screen2,
                &fd->fd_quantum);
        if (got < 2)
            goto fail;
        if (got == 3 || (got < 4 && strchr(s2, '(')))
            goto fail;
        if (got < 5 && (s3 = strchr(s2, '(')) && strchr(s3+1, '('))
            goto fail;
        if (got == 4)
            fd->fd_quantum = 0;
        else if (got == 2)
        {
            fd->fd_quantum = 0;
            fd->fd_screen1 = fd->fd_v1;
            fd->fd_screen2 = fd->fd_v2;
        }
        return;
    fail:
        post("parse error: %s", s->s_name);
        fd->fd_v1 = fd->fd_screen1 = fd->fd_v2 = fd->fd_screen2 =
            fd->fd_quantum = 0;
    }
}

#define CLOSED 1
#define BEZ 2
#define NOMOUSE 4
#define BBOX 8          /* pair of coords for rectangles and ellipses */
#define A_ARRAY 55      /* LATER decide whether to enshrine this in m_pd.h */

static void fielddesc_setfloatarg(t_fielddesc *fd, int argc, t_atom *argv)
{
        if (argc <= 0) fielddesc_setfloat_const(fd, 0);
        else if (argv->a_type == A_SYMBOL)
            fielddesc_setfloat_var(fd, argv->a_w.w_symbol);
        else fielddesc_setfloat_const(fd, argv->a_w.w_float);
}

static void fielddesc_setsymbolarg(t_fielddesc *fd, int argc, t_atom *argv)
{
        if (argc <= 0) fielddesc_setsymbol_const(fd, &s_);
        else if (argv->a_type == A_SYMBOL)
        {
            fd->fd_type = A_SYMBOL;
            fd->fd_var = 1;
            fd->fd_un.fd_varsym = argv->a_w.w_symbol;
            fd->fd_v1 = fd->fd_v2 = fd->fd_screen1 = fd->fd_screen2 =
                fd->fd_quantum = 0;
        }
        else fielddesc_setsymbol_const(fd, &s_);
}

static void fielddesc_setarrayarg(t_fielddesc *fd, int argc, t_atom *argv)
{
        if (argc <= 0) fielddesc_setfloat_const(fd, 0);
        else if (argv->a_type == A_SYMBOL)
        {
            fd->fd_type = A_ARRAY;
            fd->fd_var = 1;
            fd->fd_un.fd_varsym = argv->a_w.w_symbol;
        }
        else fielddesc_setfloat_const(fd, argv->a_w.w_float);
}

    /* getting and setting values via fielddescs -- note confusing names;
    the above are setting up the fielddesc itself. */
static t_float fielddesc_getfloat(t_fielddesc *f, t_template *template,
    t_word *wp, int loud)
{
    if (f->fd_type == A_FLOAT)
    {
        if (f->fd_var)
            return (template_getfloat(template, f->fd_un.fd_varsym, wp, loud));
        else return (f->fd_un.fd_float);
    }
    else
    {
        if (loud)
            error("symbolic data field used as number");
        return (0);
    }
}

    /* convert a variable's value to a screen coordinate via its fielddesc */
t_float fielddesc_cvttocoord(t_fielddesc *f, t_float val)
{
    t_float coord, pix, extreme, div;
    if (f->fd_v2 == f->fd_v1)
        return (val);
    div = (f->fd_screen2 - f->fd_screen1)/(f->fd_v2 - f->fd_v1);
    coord = f->fd_screen1 + (val - f->fd_v1) * div;
    extreme = (f->fd_screen1 < f->fd_screen2 ?
        f->fd_screen1 : f->fd_screen2);
    if (coord < extreme)
        coord = extreme;
    extreme = (f->fd_screen1 > f->fd_screen2 ? 
        f->fd_screen1 : f->fd_screen2);
    if (coord > extreme)
        coord = extreme;
    return (coord);
}

    /* read a variable via fielddesc and convert to screen coordinate */
t_float fielddesc_getcoord(t_fielddesc *f, t_template *template,
    t_word *wp, int loud)
{
    if (f->fd_type == A_FLOAT)
    {
        if (f->fd_var)
        {
            t_float val = template_getfloat(template,
                f->fd_un.fd_varsym, wp, loud);
            return (fielddesc_cvttocoord(f, val));
        }
        else return (f->fd_un.fd_float);
    }
    else
    {
        if (loud)
            error("symbolic data field used as number");
        return (0);
    }
}

static t_symbol *fielddesc_getsymbol(t_fielddesc *f, t_template *template,
    t_word *wp, int loud)
{
    if (f->fd_type == A_SYMBOL)
    {
        if (f->fd_var)
            return(template_getsymbol(template, f->fd_un.fd_varsym, wp, loud));
        else return (f->fd_un.fd_symbol);
    }
    else
    {
        if (loud)
            error("numeric data field used as symbol");
        return (&s_);
    }
}

    /* convert from a screen coordinate to a variable value */
t_float fielddesc_cvtfromcoord(t_fielddesc *f, t_float coord)
{
    t_float val;
    if (f->fd_screen2 == f->fd_screen1)
        val = coord;
    else
    {
        t_float div = (f->fd_v2 - f->fd_v1)/(f->fd_screen2 - f->fd_screen1);
        t_float extreme;
        val = f->fd_v1 + (coord - f->fd_screen1) * div;
        if (f->fd_quantum != 0)
            val = ((int)((val/f->fd_quantum) + 0.5)) *  f->fd_quantum;
        extreme = (f->fd_v1 < f->fd_v2 ?
            f->fd_v1 : f->fd_v2);
        if (val < extreme) val = extreme;
        extreme = (f->fd_v1 > f->fd_v2 ?
            f->fd_v1 : f->fd_v2);
        if (val > extreme) val = extreme;
    }
    return (val);
 }

void fielddesc_setcoord(t_fielddesc *f, t_template *template,
    t_word *wp, t_float coord, int loud)
{
    if (f->fd_type == A_FLOAT && f->fd_var)
    {
        t_float val = fielddesc_cvtfromcoord(f, coord);
        template_setfloat(template,
                f->fd_un.fd_varsym, wp, val, loud);
    }
    else
    {
        if (loud)
            error("attempt to set constant or symbolic data field to a number");
    }
}

/* ---------------- draw svg shapes and paths ---------------- */

/*
draws belong to templates and describe how the data in the template are to
be drawn.  The coordinates of the draw (and other display features) can
be attached to fields in the template.

todo: draw_click doesn't work with paths yet
todo: some better way than drawcurve for defining click widgetbehaviors (just
      checking for field variables and moving joints is too simplistic)
todo: make pathgetrect
*/

t_class *draw_class;

typedef struct _draw
{
    t_object x_obj;
    int x_flags;            /* CLOSED and/or BEZ and/or NOMOUSE */
    t_symbol *x_drawtype;
    t_symbol *x_fill;
    t_fielddesc x_fill_r;
    t_fielddesc x_fill_g;
    t_fielddesc x_fill_b;
    t_fielddesc x_fillopacity;
    t_fielddesc x_fillrule;
    t_symbol *x_stroke;
    t_fielddesc x_stroke_r;
    t_fielddesc x_stroke_g;
    t_fielddesc x_stroke_b;
    int x_ndash;
    t_fielddesc *x_strokedasharray; /* array of lengths */
    t_fielddesc x_strokelinecap;
    t_fielddesc x_strokelinejoin;
    t_fielddesc x_strokemiterlimit;
    t_fielddesc x_strokeopacity;
    t_fielddesc x_strokewidth;
    t_fielddesc x_rx; /* for rounded rectangles */
    t_fielddesc x_ry;
    int x_transform_n;
    t_fielddesc *x_transform;
    t_fielddesc x_width;
    t_fielddesc x_vis;
    int x_pathrect_cache; /* 0 to recalc on next draw_getrect call
                             1 for cached
                            -1 to turn off caching */
    int x_x1; /* use these for caching path bbox */
    int x_y1;
    int x_x2;
    int x_y2;
    int x_nargs;
    t_fielddesc *x_vec;
    int *x_nargs_per_cmd;      /* points per each path command */
    int x_npathcmds;
    char *x_pathcmds; /* for path commands */
    t_canvas *x_canvas;
} t_draw;

static int is_svgpath_cmd(t_symbol *s)
{
    /* 1 for absolute cmd, 2 for relative */
    if (s == gensym("M") || s == gensym("Z") || s == gensym("L") ||
        s == gensym("H") || s == gensym("V") || s == gensym("C") ||
        s == gensym("S") || s == gensym("Q") || s == gensym("T") ||
        s == gensym("A"))
        return 1;
    else if (s == gensym("m") || s == gensym("z") || s == gensym("l") ||
        s == gensym("h") || s == gensym("v") || s == gensym("c") ||
        s == gensym("s") || s == gensym("q") || s == gensym("t") ||
        s == gensym("a"))
        return 2;
    else return 0;
}

static int path_ncmds(int argc, t_atom *argv)
{
    int i, j = 0;
    for(i = 0; i < argc; i++)
    {
        if (argv[i].a_type == A_SYMBOL && is_svgpath_cmd(atom_getsymbol(argv+i)))
            j++;
    }
    return j;
}

t_draw *draw_getgroup(t_draw *x)
{
    t_gobj *g;
    t_draw *dgroup = 0;
    t_template *templ;
    t_symbol *s1 = gensym("draw");
    t_symbol *s2 = gensym("group");
    for(g = x->x_canvas->gl_list; g; g = g->g_next)
    {
        t_object *ob = pd_checkobject(&g->g_pd);
        t_atom *argv;
        if (!ob || ob->te_type != T_OBJECT ||
            binbuf_getnatom(ob->te_binbuf) < 2)
            continue;
        argv = binbuf_getvec(ob->te_binbuf);
        if (argv[0].a_type != A_SYMBOL || argv[1].a_type != A_SYMBOL
            || argv[0].a_w.w_symbol != s1 || argv[1].a_w.w_symbol != s2)
            continue;
        dgroup = (t_draw *)g;
        break;
    }
    return (dgroup);
}

static void *draw_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    t_draw *x = (t_draw *)pd_new(draw_class);
   /* not sure about classname here... */
    if (argc && argv->a_type == A_SYMBOL)
        x->x_drawtype = atom_getsymbolarg(0, argc--, argv++);
    else
    {
        pd_error(x, "draw: need an svg shape (rect, circle, path, etc.)");
    }
    int flags = 0;
    int nxy, i;
    t_fielddesc *fd;
    x->x_canvas = canvas_getcurrent();
    fielddesc_setfloat_const(&x->x_vis, 1);
    while (1)
    {
        t_symbol *firstarg = atom_getsymbolarg(0, argc, argv);
        if (!strcmp(firstarg->s_name, "-v") && argc > 1)
        {
            fielddesc_setfloatarg(&x->x_vis, 1, argv+1);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(firstarg->s_name, "-x"))
        {
            flags |= NOMOUSE;
            argc -= 1; argv += 1;
        }
        else break;
    }
    x->x_flags = flags;
    if (argc < 0) argc = 0;
    if (x->x_drawtype == gensym("path"))
    {
        int ncmds = x->x_npathcmds = path_ncmds(argc, argv);
        x->x_pathcmds = (char *)t_getbytes(ncmds * sizeof(char));
        x->x_nargs_per_cmd = (int *)t_getbytes(ncmds * sizeof(int));
        for (i = 0; i < ncmds; i++) x->x_nargs_per_cmd[i] = 0;
        nxy = x->x_nargs = argc - ncmds;
    }
    else if (x->x_drawtype == gensym("group"))
    {
        nxy = 0;
        x->x_nargs = 0;
        x->x_vec = 0;
        t_draw *dgroup = draw_getgroup(x);
        if (dgroup)
        {
            pd_error(x, "draw group: only one group per canvas allowed");
            return (0);
        }
    }
    else /* all other shapes */
    {
        nxy =  (argc + (argc & 1));
        x->x_nargs = nxy;
    }
    x->x_vec = (t_fielddesc *)t_getbytes(nxy * sizeof(t_fielddesc));

    if (argc && x->x_drawtype == gensym("path"))
    {
        if (argv->a_type != A_SYMBOL ||
            (atom_getsymbol(argv) != gensym("M") &&
             atom_getsymbol(argv) != gensym("m")))
        {
            pd_error(x, "draw path: path data must start "
                        "with a moveto command (M or m)");
            return 0;
        }
    }

    int cmdn = -1; /* hack */
    for (i = 0, fd = x->x_vec; i < argc; i++, argv++)
    {
        if (x->x_drawtype == gensym("path") &&
            argv->a_type == A_SYMBOL &&
            is_svgpath_cmd(atom_getsymbol(argv)))
        {
                x->x_pathcmds[++cmdn] = *(atom_getsymbol(argv)->s_name);
        }
        else
        {
            fielddesc_setfloatarg(fd++, 1, argv);
            /* post("got a coord"); */
            if (x->x_drawtype == gensym("path"))
            {
                (x->x_nargs_per_cmd[cmdn])++;
                /* if we get a field variable, just
                   turn off the get_rect caching */
                if (argv->a_type == A_SYMBOL)
                    x->x_pathrect_cache = -1;
            }
        }
    }
    if (argc & 1 && x->x_drawtype != gensym("path")) fielddesc_setfloat_const(fd, 0);
    x->x_fill = gensym("\"\"");
    fielddesc_setfloat_const(&x->x_fillopacity, 1);
    fielddesc_setfloat_const(&x->x_fillrule, 0);
    x->x_stroke = gensym("black");
    fielddesc_setfloat_const(&x->x_strokelinecap, 0);
    fielddesc_setfloat_const(&x->x_strokelinejoin, 0);
    fielddesc_setfloat_const(&x->x_strokemiterlimit, 0);
    fielddesc_setfloat_const(&x->x_strokeopacity, 1);
    fielddesc_setfloat_const(&x->x_strokewidth, 1);
    x->x_x1 = 0;
    x->x_x2 = 0;
    x->x_y1 = 0;
    x->x_y2 = 0;

    x->x_ndash = 0;
    x->x_strokedasharray = (t_fielddesc *)t_getbytes(1 * sizeof(t_fielddesc));
    x->x_transform_n = 0;
    x->x_transform = (t_fielddesc *)t_getbytes(1 * sizeof(t_fielddesc));

    char buf[50];
    sprintf(buf, ".x%lx", (long unsigned int)x);

    pd_bind(&x->x_obj.ob_pd, gensym(buf));
    
    return (x);
}

void draw_float(t_draw *x, t_floatarg f)
{
    int viswas;
    if (x->x_vis.fd_type != A_FLOAT || x->x_vis.fd_var)
    {
        pd_error(x, "global vis/invis for a template with variable visibility");
        return;
    }
    viswas = (x->x_vis.fd_un.fd_float != 0);

    if ((f != 0 && viswas) || (f == 0 && !viswas))
        return;
    canvas_redrawallfortemplatecanvas(x->x_canvas, 2);
    fielddesc_setfloat_const(&x->x_vis, (f != 0));
    canvas_redrawallfortemplatecanvas(x->x_canvas, 1);
}

static char *rgb_to_hex(int r, int g, int b)
{
    static char hexc[10];
    int r1 = r < 0 ? 0 : r;
    if (r1 > 255) r1 = 255;
    int g1 = g < 0 ? 0 : g;
    if (g1 > 255) g1 = 255;
    int b1 = b < 0 ? 0 : b;
    if (b1 > 255) b1 = 255;
    sprintf(hexc, "#%.6x", (r1 << 16) + (g1 << 8) + b1);
    return hexc;
}

char *get_strokelinecap(int a)
{
    static char strokelinecap[15];
    if (a == 0) sprintf(strokelinecap, "butt");
    else if (a == 1) sprintf(strokelinecap, "round");
    else if (a == 2) sprintf(strokelinecap, "projecting");
    else sprintf(strokelinecap, "butt");
    return (strokelinecap);
}

char *get_strokelinejoin(int a)
{
    static char strokelinejoin[8];
    if (a == 0) sprintf(strokelinejoin, "miter");
    else if (a == 1) sprintf(strokelinejoin, "round");
    else if (a == 2) sprintf(strokelinejoin, "bevel");
    else sprintf(strokelinejoin, "miter");
    return (strokelinejoin);
}

void draw_doupdate(t_draw *x, t_canvas *c, t_symbol *s)
{
    t_gobj *g;
    t_template *template;
    t_canvas *visible = c;
    while(visible->gl_isgraph && visible->gl_owner)
        visible = visible->gl_owner;
    int isgroup = (x->x_drawtype == gensym("group"));
    for (g = c->gl_list; g; g = g->g_next)
    {
        if (glist_isvisible(c) && g->g_pd == scalar_class &&
            x->x_canvas ==
            template_findcanvas(template = (template_findbyname(
                (((t_scalar *)g)->sc_template))))
           )
        {
            t_word *data = ((t_scalar *)g)->sc_vec;
            char str[MAXPDSTRING];
            if (s == gensym("fill"))
            {
                char *fill;
                if (x->x_fill)
                    fill = x->x_fill->s_name;
                else
                {
                    fill = rgb_to_hex(
                        (int)fielddesc_getfloat(&x->x_fill_r,
                            template, data, 1),
                        (int)fielddesc_getfloat(&x->x_fill_g,
                            template, data, 1),
                        (int)fielddesc_getfloat(&x->x_fill_b,
                            template, data, 1));
                }
                sprintf(str, "-fill %s", fill);
            }
            else if (s == gensym("stroke"))
            {
                char *stroke;
                if (x->x_stroke)
                    stroke = x->x_stroke->s_name;
                else
                {
                    stroke = rgb_to_hex(
                        (int)fielddesc_getfloat(&x->x_stroke_r,
                            template, data, 1),
                        (int)fielddesc_getfloat(&x->x_stroke_g,
                            template, data, 1),
                        (int)fielddesc_getfloat(&x->x_stroke_b,
                            template, data, 1));
                }
                sprintf(str, "-stroke %s", stroke);
            }
            else if (s == gensym("fill-opacity"))
                sprintf(str, "-fillopacity %g",
                    fielddesc_getcoord(&x->x_fillopacity, template, data, 1));
            else if (s == gensym("fill-rule"))
                sprintf(str, "-fillrule %s", (int)fielddesc_getcoord(
                    &x->x_fillrule, template, data, 1) ?
                       "evenodd" : "nonzero");
            else if (s == gensym("stroke-linecap"))
                sprintf(str, "-strokelinecap %s", get_strokelinecap(
                    (int)fielddesc_getcoord(&x->x_strokelinecap,
                        template, data, 1)));
            else if (s == gensym("stroke-linejoin"))
                sprintf(str, "-strokelinejoin %s", get_strokelinejoin(
                    (int)fielddesc_getcoord(&x->x_strokelinejoin,
                        template, data, 1)));
            else if (s == gensym("stroke-miterlimit"))
                sprintf(str, "-strokemiterlimit %g", fielddesc_getcoord(
                    &x->x_strokemiterlimit, template, data, 1));
            else if (s == gensym("stroke-opacity"))
                sprintf(str, "-strokeopacity %g", fielddesc_getcoord(
                    &x->x_strokeopacity, template, data, 1));
            else if (s == gensym("stroke-width"))
                sprintf(str, "-strokewidth %g", fielddesc_getcoord(
                    &x->x_strokewidth, template, data, 1));
            else if (s == gensym("rx"))
                sprintf(str, "-rx %g", fielddesc_getcoord(
                    &x->x_rx, template, data, 1));
            else if (s == gensym("ry"))
                sprintf(str, "-ry %g", fielddesc_getcoord(
                    &x->x_ry, template, data, 1));
            else if (s == gensym("stroke-dasharray"))
            {
                if (x->x_ndash)
                {
                    t_fielddesc *fd = x->x_strokedasharray;
                    int i;
                    char *cur = str, * const end = str + sizeof str;
                    cur += snprintf(cur, end-cur, "-strokedasharray {");
                    for (i = 0; i < x->x_ndash; i++)
                        if (cur < end)
                            cur += snprintf(cur, end-cur, " %g ",
                                fielddesc_getcoord(fd+i, template, data, 1));
                    cur += snprintf(cur, end-cur, "}\n");
                }
                else return;
            }
            if (x->x_drawtype == gensym("group"))
            {
                sys_vgui(".x%lx.c itemconfigure .dgroup%lx %s\n",
                   visible, data, str);
            }
            else
            {
                sys_vgui(".x%lx.c itemconfigure .draw%lx.%lx %s\n",
                   visible, x, data, str);
            }
            sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", visible);
        }
        if (g->g_pd == canvas_class) {
            draw_doupdate(x, (t_glist *)g, s);
        }
    }
}

extern t_canvas *canvas_list;
void draw_update(t_draw *x, t_symbol *s)
{
    t_canvas *c;
    for (c = canvas_list; c; c = c->gl_next)
        draw_doupdate(x, c, s);

}

void draw_fillopacity(t_draw *x, t_symbol *s, t_int argc, t_atom *argv)
{
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&x->x_fillopacity, argc, argv);
        draw_update(x, s);
    }
}

void draw_strokeopacity(t_draw *x, t_symbol *s, t_int argc, t_atom *argv)
{
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&x->x_strokeopacity, argc, argv);
        draw_update(x, s);
    }
}

void draw_strokedasharray(t_draw *x, t_symbol *s, int argc, t_atom *argv)
{
    t_fielddesc *fd;
    x->x_strokedasharray = (t_fielddesc *)t_resizebytes(x->x_strokedasharray,
        x->x_ndash * sizeof(*x->x_strokedasharray),
        argc * sizeof(*x->x_strokedasharray));
    x->x_ndash = argc;
    fd = x->x_strokedasharray;
    while (argc)
        fielddesc_setfloatarg(fd++, argc--, argv++);
    draw_update(x, s);
}

void draw_fill(t_draw *x, t_symbol *s, t_int argc, t_atom *argv)
{
    if (argc == 1 && argv->a_type == A_SYMBOL)
        x->x_fill = atom_getsymbolarg(0, argc, argv);
    else if (argc > 2)
    {
        int i, var = 0;
        /* if there's a color variable field we have to recalculate
           it each redraw in draw_vis */
        for(i = 0; i < argc; i++)
            var = (argv[i].a_type == A_SYMBOL) ? 1 : var;
        if (var)
        {
            fielddesc_setfloatarg(&x->x_fill_r, argc--, argv++);
            fielddesc_setfloatarg(&x->x_fill_g, argc--, argv++);
            fielddesc_setfloatarg(&x->x_fill_b, argc--, argv++);
            x->x_fill = 0;
        }
        else
        {
            int r = (int)atom_getfloatarg(0, argc--, argv++);
            int g = (int)atom_getfloatarg(0, argc--, argv++);
            int b = (int)atom_getfloatarg(0, argc--, argv++);
            x->x_fill = gensym(rgb_to_hex(r, g, b));
        }
        if (argc && (argv->a_type == A_FLOAT || argv->a_type == A_SYMBOL))
        {
            draw_fillopacity(x, gensym("fill-opacity"), argc, argv);
        }
    }
    draw_update(x, s);
}

void draw_stroke(t_draw *x, t_symbol *s, t_int argc, t_atom *argv)
{
    if (argc == 1 && argv->a_type == A_SYMBOL)
        x->x_stroke = atom_getsymbolarg(0, argc, argv);
    else if (argc > 2)
    {
        int var = 0, i;
        for(i = 0; i < argc; i++)
            var = (argv[i].a_type == A_SYMBOL) ? 1 : var;
        if (var)
        {
            fielddesc_setfloatarg(&x->x_stroke_r, argc--, argv++);
            fielddesc_setfloatarg(&x->x_stroke_g, argc--, argv++);
            fielddesc_setfloatarg(&x->x_stroke_b, argc--, argv++);
            x->x_stroke = 0;
        }
        else
        {
            /* if no variables, then precompute x_stroke so it doesn't
               have to happen every call to draw_vis */
            int r = (int)atom_getfloatarg(0, argc--, argv++);
            int g = (int)atom_getfloatarg(0, argc--, argv++);
            int b = (int)atom_getfloatarg(0, argc--, argv++);
            x->x_stroke = gensym(rgb_to_hex(r, g, b));
        }
        if (argc && (argv->a_type == A_FLOAT || argv->a_type == A_SYMBOL))
        {
            draw_strokeopacity(x, s, argc, argv);
            return;
        }
    }
    draw_update(x, s);
}

void draw_strokelinecap(t_draw *x, t_symbol *s, t_int argc, t_atom *argv)
{
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&x->x_strokelinecap, argc, argv);
        draw_update(x, s);
    }
}

void draw_strokelinejoin(t_draw *x, t_symbol *s, t_int argc, t_atom *argv)
{
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&x->x_strokelinejoin, argc, argv);
        draw_update(x, s);
    }
}

void draw_strokemiterlimit(t_draw *x, t_symbol *s, t_int argc, t_atom *argv)
{
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&x->x_strokemiterlimit, argc, argv);
        draw_update(x, s);
    }
}

void draw_strokewidth(t_draw *x, t_symbol *s, t_int argc, t_atom *argv)
{
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&x->x_strokewidth, argc, argv);
        draw_update(x, s);
    }
}

void draw_fillrule(t_draw *x, t_symbol *s, t_int argc, t_atom *argv)
{
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&x->x_fillrule, argc, argv);
        draw_update(x, s);
    }
}

void draw_rx(t_draw *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_drawtype != gensym("rect"))
    {
        pd_error(x, "draw: %s: no method for 'rx'", x->x_drawtype->s_name);
        return;
    }
    if (!argc || argv->a_type != A_FLOAT)
    {
        pd_error(x, "draw: rect: bad arguments for method 'rx'");
        return;
    }
    fielddesc_setfloatarg(&x->x_rx, argc, argv);
    draw_update(x, s);
}

void draw_ry(t_draw *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_drawtype != gensym("rect"))
    {
        pd_error(x, "draw: %s: no method for 'ry'", x->x_drawtype->s_name);
        return;
    }
    if (!argc || argv->a_type != A_FLOAT)
    {
        pd_error(x, "draw: rect: bad arguments for method 'ry'");
        return;
    }
    fielddesc_setfloatarg(&x->x_ry, argc, argv);
    draw_update(x, s);
}

static int draw_minv(t_float a[][3], t_float b[][3])
{
    t_float tmp[3][3], determinant = 0;
    int i, j;
    for(i = 0; i < 3; i++)
        determinant = determinant +
            (a[0][i]*(a[1][(i+1)%3]*a[2][(i+2)%3] -
            a[1][(i+2)%3]*a[2][(i+1)%3]));
    if (!determinant) return 0;
    for(i = 0; i < 3; i++)
    {
      for(j=0;j<3;j++)
           tmp[j][i] = ((a[(i+1)%3][(j+1)%3] * a[(i+2)%3][(j+2)%3]) - (a[(i+1)%3][(j+2)%3]*a[(i+2)%3][(j+1)%3]))/ determinant;
    }
    for(i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            b[i][j] = tmp[i][j];
    return 1;
}

/* multiply matrix a by b and put result in c. if desired, c
   can point to a or b. */
void draw_mmult(t_float a[][3], t_float b[][3], t_float c[][3])
{
    t_float tmp[3][3] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
    int i, j, k;
    for(i = 0; i < 3; i++)
        for(j = 0; j < 3; j++)
        {
            t_float sum = 0;
            for(k = 0; k < 3; k++)
                sum = sum + a[i][k] * b[k][j];
            tmp[i][j] = sum;
        }
    for(i = 0; i < 3; i++)
        for(j = 0; j < 3; j++)
            c[i][j] = tmp[i][j];
}

void draw_mset(t_float mtx[][3], t_float m1, t_float m2, t_float m3,
    t_float m4, t_float m5, t_float m6)
{
    mtx[0][0] = m1; mtx[1][0] = m2; mtx[0][1] = m3;
    mtx[1][1] = m4; mtx[0][2] = m5; mtx[1][2] = m6;
    mtx[2][0] = 0;  mtx[2][1] = 0;  mtx[2][2] = 1;
}

/* not sure if this is useful... */
void draw_mget(t_float mtx[][3], t_float *mp1, t_float *mp2, t_float *mp3,
    t_float *mp4, t_float *mp5, t_float *mp6)
{
    *mp1 = mtx[0][0]; *mp2 = mtx[1][0]; *mp3 = mtx[0][1];
    *mp4 = mtx[1][1]; *mp5 = mtx[0][2]; *mp6 = mtx[1][2];
}

void draw_parsetransform(t_draw *x, t_template *template, t_word *data,
    t_float *mp1, t_float *mp2, t_float *mp3,
    t_float *mp4, t_float *mp5, t_float *mp6)
{
    /* parse the args */
    t_symbol *type;
    int i, j;
    t_float m[3][3];
    t_float m2[3][3];
    draw_mset(m, 1, 0, 0, 1, 0, 0); /* init to the identity matrix... */
    draw_mset(m2, 0, 0, 0, 0, 0, 0);
    int argc = x->x_transform_n;
    t_fielddesc *fd = x->x_transform;
    /* should probably change this to argc > 0 since a screwup
       could land us in negativeland */
    while (argc)
    {
        if (fd->fd_un.fd_varsym)
            type = fd->fd_un.fd_varsym;
        else
        {
            pd_error(x, "draw: bad args to 'transform' method");
            return;
        }
        fd++;
        argc --;
        if (type == gensym("translate"))
        {
           t_float tx = fielddesc_getfloat(fd++, template, data, 0);
           argc--;
           t_float ty = fielddesc_getfloat(fd++, template, data, 0);
           argc--;
           draw_mset(m2, 1, 0, 0, 1, tx, ty);
           draw_mmult(m, m2, m);
        }
        else if (type == gensym("scale"))
        {
           t_float sx = fielddesc_getfloat(fd++, template, data, 0);
           argc--;
           t_float sy = sx;
           if (argc && fd->fd_type == A_FLOAT)
           {
               sy = fielddesc_getfloat(fd++, template, data, 0);
               argc--;
           }
           draw_mset(m2, sx, 0, 0, sy, 0, 0);
           draw_mmult(m, m2, m);
        }
        /* cx and cy are optional */
        /* this doesn't jibe with glist_xtopixels, ytopixels, etc. */
        else if (type == gensym("rotate"))
        {
            t_float a = fielddesc_getfloat(fd++, template, data, 0);
            argc--;
            t_float cx = 0, cy = 0;
            if (argc && fd->fd_type == A_FLOAT)
            {
                cx = fielddesc_getfloat(fd++, template, data, 0);
                argc--;
            }
            if (argc && fd->fd_type == A_FLOAT)
            {
                cy = fielddesc_getfloat(fd++, template, data, 0);
                argc--;
            }
            draw_mset(m2, cos(a), sin(a), sin(a) * -1, cos(a),
                      sin(a) * cy + cx * -1 * cos(a) + cx,
                      sin(a) * cx * -1 + cos(a) * cy * -1 + cy);
            draw_mmult(m, m2, m);
        }
        else if (type == gensym("skewx"))
        {
            t_float a = fielddesc_getfloat(fd++, template, data, 0);
            argc--;
            draw_mset(m2, 1, 0, tan(a), 1, 0, 0);
            draw_mmult(m, m2, m);
        }
        else if (type == gensym("skewy"))
        {
            t_float a = fielddesc_getfloat(fd++, template, data, 0);
            argc--;
            draw_mset(m2, 1, tan(a), 0, 1, 0, 0);
            draw_mmult(m, m2, m);
        }
        else if (type == gensym("matrix"))
        {
            t_float a, b, c, d, e, f;
            a = fielddesc_getfloat(fd++, template, data, 0); argc--;
            b = fielddesc_getfloat(fd++, template, data, 0); argc--;
            c = fielddesc_getfloat(fd++, template, data, 0); argc--;
            d = fielddesc_getfloat(fd++, template, data, 0); argc--;
            e = fielddesc_getfloat(fd++, template, data, 0); argc--;
            f = fielddesc_getfloat(fd++, template, data, 0); argc--;
            draw_mset(m2, a, b, c, d, e, f);
            draw_mmult(m, m2, m);
        }
    }
    t_float a1, a2, a3, a4, a5, a6;
    draw_mget(m, &a1, &a2, &a3, &a4, &a5, &a6);
    *mp1 = a1;
    *mp2 = a2;
    *mp3 = a3;
    *mp4 = a4;
    *mp5 = a5;
    *mp6 = a6;
}

void draw_group_pathrect_cache(t_draw *x, int state)
{
    t_gobj *y;
    for (y = x->x_canvas->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == draw_class &&
            ((t_draw *)y)->x_pathrect_cache != -1)
                ((t_draw *)y)->x_pathrect_cache = state;
    } 
}

extern void scalar_select(t_gobj *z, t_glist *owner, int state);
extern void scalar_drawselectrect(t_scalar *x, t_glist *glist, int state);
void draw_doupdatetransform(t_draw *x, t_canvas *c)
{
    t_float mtx1[3][3];
    t_float mtx2[3][3];
    t_float mtx3[3][3];
    draw_mset(mtx1, 0, 0, 0, 0, 0, 0);
    draw_mset(mtx2, 0, 0, 0, 0, 0, 0);
    t_gobj *g;
    t_template *template;
    t_canvas *visible = c;
    while(visible->gl_isgraph && visible->gl_owner)
        visible = visible->gl_owner;

    /* we'll probably get a different bbox now, so we
       will calculate a new one the next time we call
       draw_getrect for this draw command. For groups
       we need to do it for all of the draw commands.
    */
    if (x->x_drawtype == gensym("group"))
        draw_group_pathrect_cache(x, 0);
    else if (x->x_pathrect_cache != -1)
        x->x_pathrect_cache = 0;
    for (g = c->gl_list; g; g = g->g_next)
    {
        if (glist_isvisible(c) && g->g_pd == scalar_class &&
            x->x_canvas ==
            template_findcanvas((template = template_findbyname(
                (((t_scalar *)g)->sc_template))))
           )
        {
            t_float m1, m2, m3, m4, m5, m6;
            draw_parsetransform(x, template, ((t_scalar *)g)->sc_vec,
                &m1, &m2, &m3, &m4, &m5, &m6);
            if (x->x_drawtype == gensym("group"))
                sys_vgui(".x%lx.c itemconfigure .dgroup%lx -matrix "
                    "{ {%g %g} {%g %g} {%g %g} }\n",
                    visible, ((t_scalar *)g)->sc_vec, m1, m2, m3, m4, m5, m6);
            else
                sys_vgui(".x%lx.c itemconfigure .draw%lx.%lx -matrix "
                    "{ {%g %g} {%g %g} {%g %g} }\n",
                    visible, x, ((t_scalar*)g)->sc_vec,
                    m1, m2, m3, m4, m5, m6);
            /* uncache the scalar's bbox */
            ((t_scalar *)g)->sc_bboxcache = 0;
            if (glist_isselected(c, &((t_scalar *)g)->sc_gobj))
            {
                //scalar_select(g, c, 1);
                scalar_drawselectrect((t_scalar *)g, c, 0);
                scalar_drawselectrect((t_scalar *)g, c, 1);
            }
            sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", visible);
        }
        if (g->g_pd == canvas_class) {
            draw_doupdatetransform(x, (t_glist *)g);
        }
    }
}

void draw_queueupdatetransform(t_gobj *g, t_glist *glist)
{
    t_draw *x = (t_draw *)g;
    draw_doupdatetransform(x, glist);
}

extern t_canvas *canvas_list;
void draw_updatetransform(t_draw *x)
{
    t_canvas *c;
    int i;
    for (c = canvas_list; c; c = c->gl_next)
        draw_doupdatetransform(x, c);
    /* Attempted to use sys_queuegui above, but
       it actually ended up being slightly less
       efficient. Maybe I was using it wrong...
       sys_queuegui((t_gobj *)x, c, draw_queueupdatetransform);
    */
}

void draw_transform(t_draw *x, t_symbol *s, int argc, t_atom *argv)
{
    /* probably need to do error checking here */
    t_fielddesc *fd;
    x->x_transform = (t_fielddesc *)t_resizebytes(x->x_transform,
        x->x_ndash * sizeof(*x->x_transform), argc * sizeof(*x->x_transform));
    x->x_transform_n = argc;
    fd = x->x_transform;
    while (argc)
    {
        if (argv->a_type == A_SYMBOL
            && (argv->a_w.w_symbol == gensym("translate")
                || argv->a_w.w_symbol == gensym("rotate")
                || argv->a_w.w_symbol == gensym("scale")
                || argv->a_w.w_symbol == gensym("skewx")
                || argv->a_w.w_symbol == gensym("skewy")))
        {
            fielddesc_setsymbolarg(fd++, argc--, argv++);
        }
        else
        {
            if (argv->a_type == A_SYMBOL &&
                x->x_drawtype == gensym("path"))
                    x->x_pathrect_cache = -1;
            fielddesc_setfloatarg(fd++, argc--, argv++);
        }
    }
    draw_updatetransform(x);
}

/* -------------------- widget behavior for draw ------------ */

/* from Raphael.js lib */
static void draw_q2c(t_float x1, t_float y1, t_float *cx1, t_float *cy1,
    t_float *cx2, t_float *cy2, t_float *cx, t_float *cy)
{
    t_float _13 = 1.0 / 3.0;
    t_float _23 = 2.0 / 3.0;
    t_float ax = *cx1, ay = *cy1, x2 = *cx2, y2 = *cy2;

    *cx1 = _13 * x1 + _23 * ax;
    *cy1 = _13 * y1 + _23 * ay;
    *cx2 = _13 * x2 + _23 * ax;
    *cy2 = _13 * y2 + _23 * ay;
    *cx = x2;
    *cy = y2;
}

/* from Raphael.js lib */
static void draw_findDotAtSegment(t_float p1x, t_float p1y,
    t_float c1x, t_float c1y, t_float c2x, t_float c2y,
    t_float p2x, t_float p2y, t_float t, t_float *dotx, t_float *doty)
{
    t_float t1 = 1 - t;
    *dotx = pow(t1, 3) * p1x +
        pow(t1, 2) * 3 * t * c1x +
        t1 * 3 * t * t * c2x +
        pow(t, 3) * p2x;
    *doty = pow(t1, 3) * p1y +
        pow(t1, 2) * 3 * t * c1y +
        t1 * 3 * t * t * c2y +
        pow(t, 3) * p2y;
}

/* from Raphael.js lib */
static void draw_curvedim(t_float p1x, t_float p1y,
    t_float c1x, t_float c1y, t_float c2x, t_float c2y,
    t_float p2x, t_float p2y, t_float *xmin, t_float *ymin,
    t_float *xmax, t_float *ymax, t_float mtx1[][3])
{
    t_float mtx2[3][3];
    //post("inside curvedim...");
    int i;
    t_float a = (c2x - 2 * c1x + p1x) - (p2x - 2 * c2x + c1x);
    t_float        b = 2 * (c1x - p1x) - 2 * (c2x - c1x),
            c = p1x - c1x;
    //post("a is %g", a);
    //post("b is %g", b);
    //post("c is %g", c);
    t_float        syntax_sanity_check = 42,
            t1 = (a ? ((-b + sqrt(abs(b * b - 4 * a * c))) / 2.0 / a) : 0),
            t2 = (a ? ((-b - sqrt(abs(b * b - 4 * a * c))) / 2.0 / a) : 0);
    //post("t1 is %g", t1);
    //post("t2 is %g", t2);
    //post("syntax_sanity_check %g", syntax_sanity_check);
    t_float xy[12];
    xy[0] = p1x; xy[1] = p1y; xy[2] = p2x; xy[3] = p2y;

/* mtx mult */
    draw_mset(mtx2, p1x, p1y, p2x, p2y, 0, 0);
    mtx2[2][0] = 1; mtx2[2][1] = 1;
    draw_mmult(mtx1, mtx2, mtx2);
    xy[0] = mtx2[0][0]; xy[1] = mtx2[1][0]; xy[2] = mtx2[0][1]; xy[3] = mtx2[1][1];
    int xyc = 4;
    t_float dotx, doty;
    t_float dot;
    if (abs(t1) > 1e12) t1 = 0.5;
    if (abs(t2) > 1e12) t2 = 0.5;
    if (t1 > 0 && t1 < 1)
    {
    //post("found a dot");
        draw_findDotAtSegment(p1x, p1y, c1x, c1y, c2x, c2y, p2x, p2y, t1, &dotx, &doty);
        draw_mset(mtx2, dotx, doty, 0, 0, 0, 0);
        mtx2[2][0] = 1;
        draw_mmult(mtx1, mtx2, mtx2);
        dotx = mtx2[0][0];
        doty = mtx2[1][0];
        xy[xyc++] = dotx;
        xy[xyc++] = doty;
    }
    if (t2 > 0 && t2 < 1)
    {
    //post("found a second dot");
        draw_findDotAtSegment(p1x, p1y, c1x, c1y, c2x, c2y, p2x, p2y, t2, &dotx, &doty);
        draw_mset(mtx2, dotx, doty, 0, 0, 0, 0);
        mtx2[2][0] = 1;
        draw_mmult(mtx1, mtx2, mtx2);
        dotx = mtx2[0][0];
        doty = mtx2[1][0];
        xy[xyc++] = dotx;
        xy[xyc++] = doty;
    }
    a = (c2y - 2 * c1y + p1y) - (p2y - 2 * c2y + c1y);
    b = 2 * (c1y - p1y) - 2 * (c2y - c1y);
    c = p1y - c1y;
    //post("a is %g and b is %g and c is %g", a, b, c);
    t1 = (a ? ((-b + sqrt(abs(b * b - 4 * a * c))) / 2.0 / a) : 0);
    t2 = (a ? ((-b - sqrt(abs(b * b - 4 * a * c))) / 2.0 / a) : 0);
    //post("t1 is %g and t2 is %g", t1, t2);
    if (abs(t1) > 1e12) t1 = 0.5;
    if (abs(t2) > 1e12) t2 = 0.5;
    if (t1 > 0 && t1 < 1)
    {
    //post("found 3rd dot");
        draw_findDotAtSegment(p1x, p1y, c1x, c1y, c2x, c2y, p2x, p2y, t1, &dotx, &doty);
        draw_mset(mtx2, dotx, doty, 0, 0, 0, 0);
        mtx2[2][0] = 1;
        draw_mmult(mtx1, mtx2, mtx2);
        dotx = mtx2[0][0];
        doty = mtx2[1][0];
        xy[xyc++] = dotx;
        xy[xyc++] = doty;
    }
    if (t2 > 0 && t2 < 1)
    {
    //post("found 4th dot");
        draw_findDotAtSegment(p1x, p1y, c1x, c1y, c2x, c2y, p2x, p2y, t2, &dotx, &doty);
        draw_mset(mtx2, dotx, doty, 0, 0, 0, 0);
        mtx2[2][0] = 1;
        draw_mmult(mtx1, mtx2, mtx2);
        dotx = mtx2[0][0];
        doty = mtx2[1][0];
        xy[xyc++] = dotx;
        xy[xyc++] = doty;
    }

    *xmin = *ymin = 0x7fffffff;
    *xmax = *ymax = -0x7fffffff;
    for (i = 0; i < xyc; i+=2)
    {
    //post("final points:");
    //post("x %g", xy[i]);
    //post("y %g", xy[i+1]);
        if (xy[i] < *xmin) *xmin = xy[i];
        if (xy[i] > *xmax) *xmax = xy[i];
        if (xy[i+1] < *ymin) *ymin = xy[i+1];
        if (xy[i+1] > *ymax) *ymax = xy[i+1];
    }
    //post("end of curvedim:");
    //post("xmin %g ymin %g xmax %g ymax %g", *xmin, *ymin, *xmax, *ymax);
}

static t_float draw_getangle(t_float bx, t_float by)
{
  t_float pi = (t_float)3.14159265358979323846;
  t_float divisor = sqrt(bx * bx + by * by);
  return fmod(2*pi + (by > 0.0 ? 1.0 : -1.0) *
              acos( divisor? (bx / divisor) : 0 ), 2*pi);
}

void draw_arc2bbox(t_float x1, t_float y1, t_float rx, t_float ry,
   t_float phi, t_float large_arc, t_float sweep,
   t_float x2, t_float y2, t_float *xmin, t_float *ymin,
   t_float *xmax, t_float *ymax)
{
    t_float pi = (t_float)3.14159265358979323846;

    if (rx == 0 || ry == 0)
    {
        *xmin = (x1 < x2 ? x1 : x2);
        *xmax = (x1 > x2 ? x1 : x2);
        *ymin = (y1 < y2 ? y1 : y2);
        *ymax = (y1 > y2 ? y1 : y2);
        return;
    }

    if (rx < 0) rx *= -1;
    if (ry < 0) ry *= -1;

    const double x1prime = cos(phi)*(x1 - x2)/2 + sin(phi)*(y1 - y2)/2;
    const double y1prime = -sin(phi)*(x1 - x2)/2 + cos(phi)*(y1 - y2)/2;

    double radicant = (rx*rx*ry*ry - rx*rx*y1prime*y1prime - ry*ry*x1prime*x1prime);

    double divisor = (rx*rx*y1prime*y1prime + ry*ry*x1prime*x1prime);
    radicant = divisor ? radicant / divisor : 0;
    t_float cxprime = 0.0;
    t_float cyprime = 0.0;
    if (radicant < 0.0)
    {
        t_float ratio =  (ry ? rx/ry : 0);
        t_float radicant = y1prime*y1prime +
            (ratio ? x1prime*x1prime/(ratio*ratio) : 0);
        if (radicant < 0.0)
        {
            *xmin = (x1 < x2 ? x1 : x2);
            *xmax = (x1 > x2 ? x1 : x2);
            *ymin = (y1 < y2 ? y1 : y2);
            *ymax = (y1 > y2 ? y1 : y2);
            return;
        }
        ry = sqrt(radicant);
        rx = ratio * ry;
    }
    else
    {
        t_float factor = (large_arc==sweep ? -1.0 : 1.0)*sqrt(radicant);

        cxprime = factor*rx*y1prime/ry;
        cyprime = -factor*ry*x1prime/rx;
    }

    t_float cx = cxprime*cos(phi) - cyprime*sin(phi) + (x1 + x2)/2;
    t_float cy = cxprime*sin(phi) + cyprime*cos(phi) + (y1 + y2)/2;

    t_float txmin, txmax, tymin, tymax;

    if (phi == 0 || phi == pi)
    {
        *xmin = cx - rx;
        txmin = draw_getangle(-rx, 0);
        *xmax = cx + rx;
        txmax = draw_getangle(rx, 0);
        *ymin = cy - ry;
        tymin = draw_getangle(0, -ry);
        *ymax = cy + ry;
        tymax = draw_getangle(0, ry);
    }
    else if (phi == pi / 2.0 || phi == 3.0*pi/2.0)
    {
        *xmin = cx - ry;
        txmin = draw_getangle(-ry, 0);
        *xmax = cx + ry;
        txmax = draw_getangle(ry, 0);
        *ymin = cy - rx;
        tymin = draw_getangle(0, -rx);
        *ymax = cy + rx;
        tymax = draw_getangle(0, rx);
    }
    else
    {
        txmin = -atan(ry*tan(phi)/rx);
        txmax = pi - atan (ry*tan(phi)/rx);
        *xmin = cx + rx*cos(txmin)*cos(phi) - ry*sin(txmin)*sin(phi);
        *xmax = cx + rx*cos(txmax)*cos(phi) - ry*sin(txmax)*sin(phi);
        if (*xmin > *xmax)
        {
            t_float tmp = *xmax;
            *xmax = *xmin;
            *xmin = tmp;
            tmp = txmax;
            txmax = txmin;
            txmin = tmp;
        }
        t_float tmpY = cy + rx*cos(txmin)*sin(phi) + ry*sin(txmin)*cos(phi);
        txmin = draw_getangle(*xmin - cx, tmpY - cy);
        tmpY = cy + rx*cos(txmax)*sin(phi) + ry*sin(txmax)*cos(phi);
        txmax = draw_getangle(*xmax - cx, tmpY - cy);

        tymin = atan(ry/(tan(phi)*rx));
        tymax = atan(ry/(tan(phi)*rx))+pi;
        *ymin = cy + rx*cos(tymin)*sin(phi) + ry*sin(tymin)*cos(phi);
        *ymax = cy + rx*cos(tymax)*sin(phi) + ry*sin(tymax)*cos(phi);
        if (*ymin > *ymax)
        {
            t_float tmp = *ymax;
            *ymax = *ymin;
            *ymin = tmp;
            tmp = tymax;
            tymax = tymin;
            tymin = tmp;
        }
        t_float tmpX = cx + rx*cos(tymin)*cos(phi) - ry*sin(tymin)*sin(phi);
        tymin = draw_getangle(tmpX - cx, *ymin - cy);
        tmpX = cx + rx*cos(tymax)*cos(phi) - ry*sin(tymax)*sin(phi);
        tymax = draw_getangle(tmpX - cx, *ymax - cy);
    }

    t_float angle1 = draw_getangle(x1 - cx, y1 - cy);
    t_float angle2 = draw_getangle(x2 - cx, y2 - cy);

    if (!sweep)
    {
        t_float tmp = angle1;
        angle1 = angle2;
        angle2 = tmp;
    }

    int other_arc = 0;
    if (angle1 > angle2)
    {
        t_float tmp = angle1;
        angle1 = angle2;
        angle2 = tmp;
        other_arc = 1;
    }

    if ((!other_arc && (angle1 > txmin || angle2 < txmin)) || (other_arc && !(angle1 > txmin || angle2 < txmin)))
    *xmin = x1 < x2 ? x1 : x2;
  if ((!other_arc && (angle1 > txmax || angle2 < txmax)) || (other_arc && !(angle1 > txmax || angle2 < txmax)))
    *xmax = x1 > x2 ? x1 : x2;
  if ((!other_arc && (angle1 > tymin || angle2 < tymin)) || (other_arc && !(angle1 > tymin || angle2 < tymin)))
    *ymin = y1 < y2 ? y1 : y2;
  if ((!other_arc && (angle1 > tymax || angle2 < tymax)) || (other_arc && !(angle1 > tymax || angle2 < tymax)))
    *ymax = y1 > y2 ? y1 : y2;
}

/* get bbox for a path, based very roughly on
   Raphael.js "pathbbox" function.  Too complex to finish
   here, but maybe this could eventually get merged in to
   tkpath-- it will probably give a more accurate result... */
static void draw_getpathrect(t_draw *x, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    /* todo: revisit the attr from Raphael for processPath. */ 

    t_float path2_vec[x->x_nargs];
    char path2cmds[x->x_npathcmds];
    t_float mtx1[3][3];
    t_float mtx2[3][3];
    t_float m1, m2, m3, m4, m5, m6;
    t_draw *dgroup = draw_getgroup(x);
    if (dgroup)
    {
        draw_parsetransform(dgroup, template, data, &m1, &m2, &m3,
            &m4, &m5, &m6);
        draw_mset(mtx1, m1, m2, m3, m4, m5, m6);
    }
    else
        draw_mset(mtx1, 1, 0, 0, 1, 0, 0);
    draw_parsetransform(x, template, data, &m1, &m2, &m3,
        &m4, &m5, &m6);
    draw_mset(mtx2, m1, m2, m3, m4, m5, m6);
    draw_mmult(mtx1, mtx2, mtx1);
    
    /* first let's get absolute path */

    /* check if we even have path data-- if not just return with
       some default bbox */
    /* to be filled in ... */ 
    t_float xx = 0, yy = 0, mx = 0, my = 0;
    int start = 0, tally = 0;
    if (x->x_pathcmds[0] == 'M' && x->x_nargs_per_cmd[0] == 2)
    {
        xx = fielddesc_getcoord(x->x_vec, template, data, 1);
        yy = fielddesc_getcoord((x->x_vec+1), template, data, 1);
        mx = xx;
        my = yy;
        start++;
        tally = 2;
        path2cmds[0] = 'M';
        path2_vec[0] = xx;
        path2_vec[1] = yy;
    }
    
    /* loop through and get absolute values. This is more
       complicated than it needs to be. If there were explicit
       single-letter cmds for each subset of path data then I
       could just use the default below for C, S, Q, T, and L. */
    int i = 0;
    for(i = start; i < x->x_npathcmds; i++)
    {
        int j;
        char cmd = x->x_pathcmds[i];
        t_float *ia = path2_vec+tally;
        t_fielddesc *fd = x->x_vec + tally;
        int rel = cmd > 96 && cmd < 123;
        if (rel) cmd -= 32;
        path2cmds[i] = cmd;
        switch (cmd)
        {
        case 'A':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j += 7)
            {
                *ia = fielddesc_getcoord(fd, template, data, 1);
                *(ia+1) = fielddesc_getcoord(fd+1, template, data, 1);
                *(ia+2) = fielddesc_getfloat(fd+2, template, data, 1);
                *(ia+3) = fielddesc_getfloat(fd+3, template, data, 1);
                *(ia+4) = fielddesc_getfloat(fd+4, template, data, 1);
                xx = *(ia+5) = fielddesc_getcoord(fd+5, template, data, 1)
                    + (rel? xx : 0);
                yy = *(ia+6) = fielddesc_getcoord(fd+6, template, data, 1)
                    + (rel? yy : 0);
            }
            break;
        case 'V':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
                yy = *ia = fielddesc_getcoord(fd, template, data, 1)
                    + (rel? yy : 0);
            break;
        case 'H':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
                xx = *ia = fielddesc_getcoord(fd, template, data, 1)
                    + (rel? xx : 0);
            break;
        case 'L':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
            {
                if (j%2 == 0)
                    xx = *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? xx : 0);
                else
                    yy = *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? yy : 0);
            }      
            break;
        case 'C':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
            {
                if (j%6 == 4)
                    xx = *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? xx : 0);
                else if (j%6 == 5)
                    yy = *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? yy : 0);
                else if (j%2 == 0)
                    *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? xx : 0);
                else
                    *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? yy : 0);
            }
            break;
        case 'Q':
        case 'S':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
            {
                if (j%4 == 2)
                    xx = *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? xx : 0);
                else if (j%4 == 3)
                    yy = *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? yy : 0);
                else if (j%2 == 0)
                    *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? xx : 0);
                else
                    *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? yy : 0);
            }
            break;
        case 'M':
            mx = fielddesc_getcoord(fd, template, data, 1) + (rel? xx : 0);
            my = fielddesc_getcoord(fd+1, template, data, 1) + (rel? yy : 0);
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
            {
                if (j%2 == 0)
                    xx = *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? xx : 0);
                else
                    yy = *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1)
                        + (rel? yy : 0);
            }
            break;
        default:
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
            {
                if (j%2 == 0)
                {
                    xx = *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1) + (rel? xx : 0);
                }
                else
                {
                   yy = *(ia+j) = fielddesc_getcoord(fd+j, template, data, 1) + (rel? yy : 0);
                }
            }
            break;
        }
        tally += x->x_nargs_per_cmd[i];
        switch (cmd)
        {
        case 'Z':
            xx = mx;
            yy = my;
            break;
        case 'H':
            xx = *ia;
            break;
        case 'V':
            yy = *ia;
            break;
        case 'M':
            mx = *(ia+(x->x_nargs_per_cmd[i] - 2));
            my = *(ia+(x->x_nargs_per_cmd[i] - 1));
        default:
            xx = *(ia+(x->x_nargs_per_cmd[i] - 2));
            yy = *(ia+(x->x_nargs_per_cmd[i] - 1));
        }
    }
    /* need to normalize everything to curves next ... */
    /* but for now, a cheap hack... */
    int finalx1 = 0x7fffffff, finaly1 = 0x7fffffff,
        finalx2 = -0x7fffffff, finaly2 = -0x7fffffff;
    tally = 0;
    int j;
    t_float tmpx, tmpy;
    t_float xprev = 0, yprev = 0, qxprev = 0, qyprev = 0, nx = 0, ny = 0,
        bx = 0, by = 0;
    t_float x1, x2, y1, y2;
    char pcmd = 'X';
    for (i = 0; i < x->x_npathcmds; i++)
    {
        char cmd = path2cmds[i];
        t_float *ia = path2_vec+tally;
        switch (cmd)
        {
        case 'A':
            draw_arc2bbox(xprev, yprev, *ia, *(ia+1), *(ia+2), *(ia+3),
                *(ia+4), *(ia+5), *(ia+6), &x1, &y1, &x2, &y2);
            xprev = *(ia+5);
            yprev = *(ia+6);
            draw_mset(mtx2, x1, y1, x2, y2, 0, 0);
            mtx2[2][0] = 1; mtx2[2][1] = 1;
            draw_mmult(mtx1, mtx2, mtx2);
            tmpx = mtx2[0][0]; tmpy = mtx2[1][0];
            finalx1 = tmpx < finalx1 ? tmpx : finalx1;
            finalx2 = tmpx > finalx2 ? tmpx : finalx2;
            finaly1 = tmpy < finaly1 ? tmpy : finaly1;
            finaly2 = tmpy > finaly2 ? tmpy : finaly2;
            tmpx = mtx2[0][1]; tmpy = mtx2[1][1];
            finalx1 = tmpx < finalx1 ? tmpx : finalx1;
            finalx2 = tmpx > finalx2 ? tmpx : finalx2;
            finaly1 = tmpy < finaly1 ? tmpy : finaly1;
            finaly2 = tmpy > finaly2 ? tmpy : finaly2;
            draw_mset(mtx2, x1, y2, x2, y1, 0, 0);
            mtx2[2][0] = 1; mtx2[2][1] = 1;
            draw_mmult(mtx1, mtx2, mtx2);
            tmpx = mtx2[0][0]; tmpy = mtx2[1][0];
            finalx1 = tmpx < finalx1 ? tmpx : finalx1;
            finalx2 = tmpx > finalx2 ? tmpx : finalx2;
            finaly1 = tmpy < finaly1 ? tmpy : finaly1;
            finaly2 = tmpy > finaly2 ? tmpy : finaly2;
            tmpx = mtx2[0][1]; tmpy = mtx2[1][1];
            finalx1 = tmpx < finalx1 ? tmpx : finalx1;
            finalx2 = tmpx > finalx2 ? tmpx : finalx2;
            finaly1 = tmpy < finaly1 ? tmpy : finaly1;
            finaly2 = tmpy > finaly2 ? tmpy : finaly2;
            break;
        case 'S':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j += 4)
            {
                    /* hack */
                    if ((j + 3) >= x->x_nargs_per_cmd[i])
                        break;
                if (pcmd == 'C' || pcmd == 'S')
                {
                    /* this is wrong... we need a
                       "bx" variable at the end instead
                       of nx... see Raphael... */
                    nx = xprev * 2 - bx;
                    ny = yprev * 2 - by;
                }
                else
                {
                    nx = xprev;
                    ny = yprev;
                }
                draw_curvedim(xprev, yprev,
                    nx, ny, *ia, *(ia+1), *(ia+2), *(ia+3),
                    &x1, &y1, &x2, &y2, mtx1);
                xprev = *(ia+2);
                yprev = *(ia+3);
                bx = *ia;
                by = *(ia + 1);

                if (x1 == 0x7fffffff && y1 == 0x7fffffff &&
                x2 == -0x7fffffff && y2 == -0x7fffffff)
                    break;
                finalx1 = x1 < finalx1 ? x1 : finalx1;
                finalx2 = x2 > finalx2 ? x2 : finalx2;
                finaly1 = y1 < finaly1 ? y1 : finaly1;
                finaly2 = y2 > finaly2 ? y2 : finaly2;
                pcmd = cmd;
            }
            break;
        case 'T':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j += 2)
            {
                    /* hack */
                    if ((j + 1) >= x->x_nargs_per_cmd[i])
                        break;
                if (pcmd == 'Q' || pcmd == 'T')
                {
                    qxprev = xprev * 2 - qxprev;
                    qyprev = yprev * 2 - qyprev;
                }
                else
                {
                    qxprev = xprev;
                    qyprev = yprev;
                }
                t_float cx1 = qxprev, cy1 = qyprev, cx2 = *ia, cy2 = *(ia+1),
                    cx, cy;
                draw_q2c(xprev, yprev, &cx1, &cy1, &cx2, &cy2, &cx, &cy);
                draw_curvedim(xprev, yprev, cx1, cy1, cx2, cy2, cx, cy,
                    &x1, &y1, &x2, &y2, mtx1);
                xprev = *ia;
                yprev = *(ia+1);

                if (x1 == 0x7fffffff && y1 == 0x7fffffff &&
                x2 == -0x7fffffff && y2 == -0x7fffffff)
                    break;
                finalx1 = x1 < finalx1 ? x1 : finalx1;
                finalx2 = x2 > finalx2 ? x2 : finalx2;
                finaly1 = y1 < finaly1 ? y1 : finaly1;
                finaly2 = y2 > finaly2 ? y2 : finaly2;
                pcmd = cmd;
            }
            break;
        case 'Q':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j += 4)
            {
                    /* hack */
                    if ((j + 3) >= x->x_nargs_per_cmd[i])
                        break;
                t_float cx1 = *ia, cy1 = *(ia+1),
                    cx2 = *(ia+2), cy2 = *(ia+3), cx, cy;
                draw_q2c(xprev, yprev, &cx1, &cy1, &cx2, &cy2, &cx, &cy);
                draw_curvedim(xprev, yprev, cx1, cy1, cx2, cy2, cx, cy,
                    &x1, &y1, &x2, &y2, mtx1);
                xprev = *(ia+2);
                yprev = *(ia+3);

                if (x1 == 0x7fffffff && y1 == 0x7fffffff &&
                    x2 == -0x7fffffff && y2 == -0x7fffffff)
                        break;
                finalx1 = x1 < finalx1 ? x1 : finalx1;
                finalx2 = x2 > finalx2 ? x2 : finalx2;
                finaly1 = y1 < finaly1 ? y1 : finaly1;
                finaly2 = y2 > finaly2 ? y2 : finaly2;
            }
            break;
        case 'C':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j += 6)
            {
                /* hack */
                if ((j + 5) >= x->x_nargs_per_cmd[i])
                    break;
                draw_curvedim(xprev, yprev, *ia, *(ia+1), *(ia+2), *(ia+3),
                    *(ia+4), *(ia+5), &x1, &y1, &x2, &y2, mtx1);
                xprev = *(ia+4);
                yprev = *(ia+5);
                bx = *(ia+2);
                by = *(ia+3);

                if (x1 == 0x7fffffff && y1 == 0x7fffffff &&
                    x2 == -0x7fffffff && y2 == -0x7fffffff)
                        break;
                finalx1 = x1 < finalx1 ? x1 : finalx1;
                finalx2 = x2 > finalx2 ? x2 : finalx2;
                finaly1 = y1 < finaly1 ? y1 : finaly1;
                finaly2 = y2 > finaly2 ? y2 : finaly2;
            }
/*
            draw_mset(mtx2, x1, y1, x2, y2, 0, 0);
            mtx2[2][0] = 1; mtx2[2][1] = 1;
            draw_mmult(mtx1, mtx2, mtx2);
            tmpx = mtx2[0][0]; tmpy = mtx2[1][0];
            finalx1 = tmpx < finalx1 ? tmpx : finalx1;
            finalx2 = tmpx > finalx2 ? tmpx : finalx2;
            finaly1 = tmpy < finaly1 ? tmpy : finaly1;
            finaly2 = tmpy > finaly2 ? tmpy : finaly2;
            tmpx = mtx2[0][1]; tmpy = mtx2[1][1];
            finalx1 = tmpx < finalx1 ? tmpx : finalx1;
            finalx2 = tmpx > finalx2 ? tmpx : finalx2;
            finaly1 = tmpy < finaly1 ? tmpy : finaly1;
            finaly2 = tmpy > finaly2 ? tmpy : finaly2;
            draw_mset(mtx2, x1, y2, x2, y1, 0, 0);
            mtx2[2][0] = 1; mtx2[2][1] = 1;
            draw_mmult(mtx1, mtx2, mtx2);
            tmpx = mtx2[0][0]; tmpy = mtx2[1][0];
            finalx1 = tmpx < finalx1 ? tmpx : finalx1;
            finalx2 = tmpx > finalx2 ? tmpx : finalx2;
            finaly1 = tmpy < finaly1 ? tmpy : finaly1;
            finaly2 = tmpy > finaly2 ? tmpy : finaly2;
            tmpx = mtx2[0][1]; tmpy = mtx2[1][1];
            finalx1 = tmpx < finalx1 ? tmpx : finalx1;
            finalx2 = tmpx > finalx2 ? tmpx : finalx2;
            finaly1 = tmpy < finaly1 ? tmpy : finaly1;
            finaly2 = tmpy > finaly2 ? tmpy : finaly2;
*/
            break;
        case 'V':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
            {
                draw_mset(mtx2, xprev, *ia, 0, 0, 0, 0);
                mtx2[2][0] = 1;
                draw_mmult(mtx1, mtx2, mtx2);
                tmpy = mtx2[1][0];
                finaly1 = tmpy < finaly1 ? tmpy : finaly1;
                finaly2 = tmpy > finaly2 ? tmpy : finaly2;
                yprev = *ia;
            }
            break;
        case 'H':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
            {
                draw_mset(mtx2, *ia, yprev, 0, 0, 0, 0);
                mtx2[2][0] = 1;
                draw_mmult(mtx1, mtx2, mtx2);
                tmpx = mtx2[0][0];  
                finalx1 = tmpx < finalx1 ? tmpx : finalx1;
                finalx2 = tmpx > finalx2 ? tmpx : finalx2;
                xprev = *ia;
            }
            break;
        default:
            for (j = 0; j < x->x_nargs_per_cmd[i]; j += 2)
            {
                    /* hack */
                    if ((j + 1) >= x->x_nargs_per_cmd[i])
                        break;
                    draw_mset(mtx2, *(ia+j), *(ia+j+1), 0, 0, 0, 0);
                    mtx2[2][0] = 1;
                    draw_mmult(mtx1, mtx2, mtx2);
                    tmpx = mtx2[0][0]; tmpy = mtx2[1][0];
                    finalx1 = tmpx < finalx1 ? tmpx : finalx1;
                    finalx2 = tmpx > finalx2 ? tmpx : finalx2;
                    finaly1 = tmpy < finaly1 ? tmpy : finaly1;
                    finaly2 = tmpy > finaly2 ? tmpy : finaly2;
                    xprev = *(ia+j); yprev = *(ia+j+1);
            }
    }
        tally += x->x_nargs_per_cmd[i];
        pcmd = cmd;
    }
    x->x_x1 = finalx1;
    x->x_x2 = finalx2;
    x->x_y1 = finaly1;
    x->x_y2 = finaly2;
    if (x->x_pathrect_cache != -1)
        x->x_pathrect_cache = 1;
    finalx1 = glist_xtopixels(glist, basex + finalx1);
    finalx2 = glist_xtopixels(glist, basex + finalx2);
    finaly1 = glist_ytopixels(glist, basey + finaly1);
    finaly2 = glist_ytopixels(glist, basey + finaly2);
    *xp1 = (int)finalx1;
    *xp2 = (int)finalx2;
    *yp1 = (int)finaly1;
    *yp2 = (int)finaly2;
}

static void draw_setrect(t_draw *x, t_floatarg x1,
    t_floatarg y1, t_floatarg x2, t_floatarg y2)
{
    /* todo: grab from getpathrect to allow caching the
       auto-calculation */
    x->x_x1 = x1 < 0 ? 0 : x1;
    x->x_x2 = x2 < 0 ? 0 : x2;
    x->x_y1 = y1 < 0 ? 0 : y1;
    x->x_y2 = y2 < 0 ? 0 : y2;
}

static int call_from_vis = 0;
static void draw_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_draw *x = (t_draw *)z;
    if (x->x_pathrect_cache == 1)
    {
        *xp1 = glist_xtopixels(glist, basex + x->x_x1);
        *xp2 = glist_xtopixels(glist, basex + x->x_x2);
        *yp1 = glist_ytopixels(glist, basey + x->x_y1);
        *yp2 = glist_ytopixels(glist, basey + x->x_y2);
        return;
    }

    t_float mtx1[3][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
    t_float mtx2[3][3] = { {1, 0, 0}, {0, 1, 0}, {1, 0, 1} };
    t_float m1, m2, m3, m4, m5, m6;
    
    //fprintf(stderr,"draw_getrect\n");
    t_draw *dgroup = draw_getgroup(x);
    int i, n = x->x_nargs;
    t_fielddesc *f = x->x_vec;
    int x1 = 0x7fffffff, x2 = -0x7fffffff, y1 = 0x7fffffff, y2 = -0x7fffffff;
    if (!fielddesc_getfloat(&x->x_vis, template, data, 0) ||
        (x->x_flags & NOMOUSE) || (x->x_drawtype == gensym("group")))
    {
        *xp1 = *yp1 = 0x7fffffff;
        *xp2 = *yp2 = -0x7fffffff;
        return;
    }

    if (dgroup)
    {
        draw_parsetransform(dgroup, template, data, &m1, &m2,
            &m3, &m4, &m5, &m6);
        draw_mset(mtx1, m1, m2, m3, m4, m5, m6);
    }
    else
        draw_mset(mtx1, 1, 0, 0, 1, 0, 0);

    if (x->x_drawtype == gensym("path"))
    {
        /* this could get damned expensive with complex paths 
        which is why there's a caching mechanism */
        draw_getpathrect(x, glist, data, template, basex, basey,
            &x1, &y1, &x2, &y2);
    }
    else if (x->x_drawtype == gensym("polyline") ||
             x->x_drawtype == gensym("line") ||
             x->x_drawtype == gensym("polygon"))
    {
        int nxy = n >> 1;
        draw_parsetransform(x, template, data, &m1, &m2, &m3,
            &m4, &m5, &m6);
        draw_mset(mtx2, m1, m2, m3, m4, m5, m6);
        draw_mmult(mtx1, mtx2, mtx1);

        for (i = 0, f = x->x_vec; i < n; i+=2, f+=2)
        {
            t_float xloc = fielddesc_getcoord(f, template, data, 0);
            t_float yloc = fielddesc_getcoord(f+1, template, data, 0);

            draw_mset(mtx2, xloc, yloc, 0, 0, 0, 0);
            mtx2[2][0] = 1;
            draw_mmult(mtx1, mtx2, mtx2);
            xloc = mtx2[0][0];
            yloc = mtx2[1][0];

            if (xloc < x1) x1 = xloc;
            if (xloc > x2) x2 = xloc;
            if (yloc < y1) y1 = yloc;
            if (yloc > y2) y2 = yloc;
        }
        if (n)
        {
            x1 = glist_xtopixels(glist, basex + x1);
            x2 = glist_xtopixels(glist, basex + x2);
            y1 = glist_ytopixels(glist, basey + y1);
            y2 = glist_ytopixels(glist, basey + y2);
        }
    }
    else if (x->x_drawtype == gensym("rect") ||
             x->x_drawtype == gensym("circle") ||
             x->x_drawtype == gensym("ellipse"))
    {
        t_float m1, m2, m3, m4, m5, m6; /* matrix */
        t_float xx1, yy1, xx2, yy2;
        t_float tx1, ty1, tx2, ty2, t5, t6; /* transformed points */
        if (x->x_drawtype == gensym("rect"))
        {
            xx1 = fielddesc_getcoord(x->x_vec, template, data, 0);
            yy1 = fielddesc_getcoord(x->x_vec+1, template, data, 0);
            t_float rwidth = fielddesc_getcoord(x->x_vec+2, template, data, 0);
            t_float rheight = fielddesc_getcoord(x->x_vec+3, template, data, 0);
            xx2 = xx1 + rwidth;
            yy2 = yy1 + rheight;
        }
        else
        {
            /* Yes, I realize this isn't the tightest-fitting bbox but it's
               late and I'm losing steam... */
            t_float cx = fielddesc_getcoord(x->x_vec, template, data, 0);
            t_float cy = fielddesc_getcoord(x->x_vec+1, template, data, 0);
            t_float rx = fielddesc_getcoord(x->x_vec+2, template, data, 0);
            t_float ry = fielddesc_getcoord(x->x_vec +
                (x->x_drawtype == gensym("ellipse")? 3 : 2), template,
                data, 0);
            xx1 = cx - rx;
            yy1 = cy - ry;
            xx2 = cx + rx;
            yy2 = cy + ry;
        }
        draw_parsetransform(x, template, data, &m1, &m2, &m3, &m4, &m5, &m6);
        draw_mset(mtx2, m1, m2, m3, m4, m5, m6);
        draw_mmult(mtx1, mtx2, mtx1);
        /* There's probably a much easier way to do this.  I'm just
           setting the first two columns to x/y points to get them
           transformed.  Since the shapes could be crazy skewed/rotated
           I have to check each coordinate of the rect, so I do it again
           below. */
        draw_mset(mtx2, xx1, yy1, xx2, yy2, 0, 0);
        mtx2[2][0] = 1; mtx2[2][1] = 1;
        draw_mmult(mtx1, mtx2, mtx2);
        draw_mget(mtx2, &tx1, &ty1, &tx2, &ty2, &t5, &t6);
        if (tx1 < x1) x1 = tx1;
        if (tx2 < x1) x1 = tx2;
        if (ty1 < y1) y1 = ty1;
        if (ty2 < y1) y1 = ty2;
        if (tx1 > x2) x2 = tx1;
        if (tx2 > x2) x2 = tx2;
        if (ty1 > y2) y2 = ty1;
        if (ty2 > y2) y2 = ty2;
        draw_mset(mtx2, xx1, yy2, xx2, yy1, 0, 0);
        mtx2[2][0] = 1; mtx2[2][1] = 1;
        draw_mmult(mtx1, mtx2, mtx2);
        draw_mget(mtx2, &tx1, &ty1, &tx2, &ty2, &t5, &t6);
        if (tx1 < x1) x1 = tx1;
        if (tx2 < x1) x1 = tx2;
        if (ty1 < y1) y1 = ty1;
        if (ty2 < y1) y1 = ty2;
        if (tx1 > x2) x2 = tx1;
        if (tx2 > x2) x2 = tx2;
        if (ty1 > y2) y2 = ty1;
        if (ty2 > y2) y2 = ty2;
        x1 = glist_xtopixels(glist, basex + x1);
        x2 = glist_xtopixels(glist, basex + x2);
        y1 = glist_ytopixels(glist, basey + y1);
        y2 = glist_ytopixels(glist, basey + y2);
    }
    if (fielddesc_getfloat(&x->x_strokewidth, template, data, 0))
    {
        int padding = fielddesc_getcoord(&x->x_strokewidth,
            template, data, 0) * 0.5;
        x1 -= padding;
        y1 -= padding;
        x2 += padding;
        y2 += padding;
    }
    //fprintf(stderr,"FINAL draw_getrect %d %d %d %d\n", x1, y1, x2, y2);
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2; 
}

static void draw_displace(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int dx, int dy)
{
    /* refuse */
}

static void draw_select(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    /* fill in later */
}

static void draw_activate(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    /* fill in later */
}

/* todo: create the group somewhere in here..., and use the template
tag on it so that it can get deleted on unvising */
static void draw_vis(t_gobj *z, t_glist *glist, t_scalar *sc,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int vis)
{
    t_draw *x = (t_draw *)z;
    /* As a quick hack we are sending the group matrix to the
       gui. This is inefficient since the group matrix is the
       same for all the other drawing instructions. It should
       instead be sent once inside scalar_vis, but that means
       searching the templatecanvas for a [draw group] object
       rather than just a single loop to find all the drawing
       command parentvisfn functions which it does currently.

       Really, [draw group] needs to be a separate class, but
       that means putting all the options-- stroke, etc.-- in
       a separate struct. It would also mean that draw_update
       and draw_updatetransform need to be made more general.
    */

    if (x->x_drawtype == gensym("group"))
    {
        t_float m1, m2, m3, m4, m5, m6;
        draw_parsetransform(x, template, data, &m1, &m2, &m3, &m4, &m5, &m6);
        sys_vgui(".x%lx.c itemconfigure .dgroup%lx -matrix { {%g %g} {%g %g} {%g %g} }\n", glist, data, m1, m2, m3, m4, m5, m6);
        return;
    }

    //post("number of points in draw_vis is %d", x->x_nargs);
    int i, n = x->x_nargs;
    t_float mtx1[3][3] =  { { 0, 0, 0}, {0, 0, 0}, {0, 0, 1} };
    t_float mtx2[3][3] =  { { 0, 0, 0}, {0, 0, 0}, {0, 0, 1} };
    /* need to scale some attributes like radii, widths, etc. */
    t_float xscale = glist_xtopixels(glist, 1) - glist_xtopixels(glist, 0);
    t_float yscale = glist_ytopixels(glist, 1) - glist_ytopixels(glist, 0);
    t_fielddesc *f = x->x_vec;

    /*// get the universal tag for all nested objects
    t_canvas *tag = x->x_canvas;
    while (tag->gl_owner) {
        tag = tag->gl_owner;
    }*/
    
        /* see comment in plot_vis() */
    if (vis && !fielddesc_getfloat(&x->x_vis, template, data, 0))
        return;
    if (vis)
    {
        if (n > 2)
        {
            /* Hack to figure out whether we're inside an
               array. See curve_vis comment for more info
               and feel free to revise this to make it a
               more sane approach.
            */
            int in_array = (sc->sc_vec == data) ? 0 : 1;
            if (in_array)
                sys_vgui(".x%lx.c create group -tags {.scelem%lx} "
                    "-parent .dgroup%lx -matrix { {1 0} {0 1} {%g %g} }\n",
                    glist_getcanvas(glist), data, sc->sc_vec, basex, basey);
            int flags = x->x_flags;
            char *outline;
            char *fill;
            char *stroke;
            char *strokelinecap = get_strokelinecap(
                (int)fielddesc_getfloat(&x->x_strokelinecap,
                    template, data, 1));
            char *strokelinejoin = get_strokelinejoin(
                (int)fielddesc_getfloat(&x->x_strokelinejoin,
                    template, data, 1));
            if (x->x_fill)
                fill = x->x_fill->s_name;
            else
            {
                fill = rgb_to_hex(
                    (int)fielddesc_getfloat(&x->x_fill_r, template, data, 1),
                    (int)fielddesc_getfloat(&x->x_fill_g, template, data, 1),
                    (int)fielddesc_getfloat(&x->x_fill_b, template, data, 1));
            }
            if (x->x_stroke)
                stroke = x->x_stroke->s_name;
            else
            {
                stroke = rgb_to_hex(
                    (int)fielddesc_getfloat(&x->x_stroke_r, template, data, 1),
                    (int)fielddesc_getfloat(&x->x_stroke_g, template, data, 1),
                    (int)fielddesc_getfloat(&x->x_stroke_b, template, data, 1));
            }
            t_float pix[200];
            if (n > 200)
                n = 200;
                /* calculate the pixel values before we start printing
                out the TK message so that "error" printout won't be
                interspersed with it.  Only show up to 100 points so we don't
                have to allocate memory here. */
            if (x->x_drawtype != gensym("path"))
            {
                int nxy = n >> 1;
                for (i = 0, f = x->x_vec; i < nxy; i++, f += 2)
                {
                    pix[2*i] = fielddesc_getcoord(f, template, data, 1);
                    pix[2*i+1] = fielddesc_getcoord(f+1, template, data, 1);
                    if (x->x_drawtype == gensym("circle")) break;
                }
            }
            /* begin the gui drawing command */
            if (x->x_drawtype == gensym("rect"))
                sys_vgui(".x%lx.c create prect\\\n", glist_getcanvas(glist));
            else if (x->x_drawtype == gensym("ellipse"))
                sys_vgui(".x%lx.c create ellipse\\\n", glist_getcanvas(glist));
            else if (x->x_drawtype == gensym("line"))
                sys_vgui(".x%lx.c create pline\\\n", glist_getcanvas(glist));
            else if (x->x_drawtype == gensym("polyline"))
                sys_vgui(".x%lx.c create polyline\\\n", glist_getcanvas(glist));
            else if (x->x_drawtype == gensym("polygon"))
                sys_vgui(".x%lx.c create ppolygon\\\n", glist_getcanvas(glist));
            else if (x->x_drawtype == gensym("path"))
                sys_vgui(".x%lx.c create path {\\\n", glist_getcanvas(glist));
            else if (x->x_drawtype == gensym("circle"))
                sys_vgui(".x%lx.c create ellipse\\\n", glist_getcanvas(glist));
            /* next send the gui drawing arguments: commands and points
               for paths, points for everything else */
            if (x->x_drawtype == gensym("path"))
            {
                /* let's turn off bbox caching so we can recalculate
                   the bbox */
                if (x->x_pathrect_cache != -1)
                    x->x_pathrect_cache = 0;
                char *cmd;
                int totalpoints = 0; /* running tally */
                /* path parser: no error checking yet */
                for (i = 0, cmd = x->x_pathcmds; i < x->x_npathcmds; i++, cmd++)
                {
                    int j;
                    int cargs = x->x_nargs_per_cmd[i];
                    f = (x->x_vec)+totalpoints;
                    sys_vgui("%c\\\n", *(cmd));
                    for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
                        sys_vgui("%g\\\n", fielddesc_getcoord(
                            f+j, template, data, 1));
                    totalpoints += x->x_nargs_per_cmd[i];
                }
                sys_gui("}\\\n");
            }
            else /* all other shapes */
            {
                int nxy = n >> 1;
                for (i = 0; i < nxy; i++)
                {
                    sys_vgui("%g %g\\\n", pix[2*i], pix[2*i+1]);
                    if ((x->x_drawtype == gensym("ellipse") ||
                         x->x_drawtype == gensym("circle")) && n > 1)
                    {
                        sys_vgui("-rx %d -ry %d\\\n",
                            (int)(fielddesc_getcoord(x->x_vec+2,
                                template, data, 1)),
                            (int)(fielddesc_getcoord(x->x_vec +
                                (x->x_drawtype == gensym("ellipse")?
                                 3 : 2),
                                template, data, 1)));
                        break;
                    }
                    else if (x->x_drawtype == gensym("rect") && n > 1)
                    {
                        sys_vgui("%g %g\\\n",
                            fielddesc_getcoord(x->x_vec,
                                template, data, 1) +
                                fielddesc_getcoord(x->x_vec+2,
                                template, data, 1),
                            fielddesc_getcoord(x->x_vec+1,
                                template, data, 1) +
                                fielddesc_getcoord(x->x_vec+3,
                                template, data, 1));
                        break;
                    }
                    else if (x->x_drawtype == gensym("circle"))
                    {
                        sys_vgui("-r %d\\\n",
                            (t_int)fielddesc_getcoord(x->x_vec+2,
                                template, data, 1));
                        break;
                    }
                }
            }
            sys_vgui("-strokewidth %g\\\n", fielddesc_getfloat(&x->x_strokewidth, template, data, 1));
            if (x->x_drawtype != gensym("line"))
                sys_vgui("-fill %s -fillopacity %g -fillrule %s\\\n",
                    fill,
                    fielddesc_getfloat(&x->x_fillopacity, template, data, 1),
                    (int)fielddesc_getfloat(
                    &x->x_fillrule, template, data, 1) ? "evenodd" : "nonzero");
            sys_vgui("-stroke %s -strokeopacity %g -strokelinecap %s -strokelinejoin %s -strokemiterlimit %g\\\n",
                stroke,
                fielddesc_getfloat(&x->x_strokeopacity, template, data, 1),
                strokelinecap, strokelinejoin,
                fielddesc_getfloat(&x->x_strokemiterlimit, template, data, 1)
            );
            if (x->x_ndash)
            {
                int i;
                t_fielddesc *fd;
                sys_gui(" -strokedasharray {\\\n");
                for (i = 0, fd = x->x_strokedasharray; i < x->x_ndash; i++)
                {
                    sys_vgui("%d\\\n", (int)fielddesc_getfloat(fd+i,
                        template, data, 1));
                }
                sys_gui("}\\\n");
            }
            if (x->x_transform_n > 0)
            {
                /* todo: premultiply this */
                t_float m1, m2, m3, m4, m5, m6;
                draw_parsetransform(x, template, data, &m1, &m2, &m3,
                    &m4, &m5, &m6);
                sys_vgui("-matrix { {%g %g} {%g %g} {%g %g} }\\\n",
                    m1, m2, m3, m4, m5, m6);
            }
            //if ((flags & BEZ) && !(flags & BBOX)) sys_vgui("-smooth 1\\\n"); //this doesn't work with tkpath
            if (in_array)
                sys_vgui(" -parent .scelem%lx \\\n", data);
            else
                sys_vgui(" -parent .dgroup%lx \\\n", sc->sc_vec);
            /* tags - one for this scalar (not sure why the double glist thingy)
              one for this specific draw item
            */
            sys_vgui("-tags {.x%lx.x%lx.template%lx .draw%lx.%lx}\n",
                glist_getcanvas(glist), glist, data, x, data);
            if (!glist_istoplevel(glist))
            {
                t_canvas *gl = glist_getcanvas(glist);
                //glist_noselect(gl);
                //glist_select(gl, (t_gobj *)glist);
                char objtag[64];
                sprintf(objtag, ".x%lx.x%lx.template%lx",
                    (t_int)gl, (t_int)glist, (t_int)data);
                canvas_restore_original_position(gl, (t_gobj *)glist,
                    objtag, -1);
            }
        }
        else post("warning: draws need at least two points to be graphed");
    }
    else
    {
        if (n > 1)

/* if in_array then delete the container group */

sys_vgui(".x%lx.c delete .x%lx.x%lx.template%lx\n",
            glist_getcanvas(glist), glist_getcanvas(glist), glist,
            data);      
    }
}

static int draw_motion_field;
static t_float draw_motion_xcumulative;
static t_float draw_motion_xbase;
static t_float draw_motion_xper;
static t_float draw_motion_ycumulative;
static t_float draw_motion_ybase;
static t_float draw_motion_yper;
static t_glist *draw_motion_glist;
static t_scalar *draw_motion_scalar;
static t_array *draw_motion_array;
static t_word *draw_motion_wp;
static t_template *draw_motion_template;
static t_gpointer draw_motion_gpointer;

    /* LATER protect against the template changing or the scalar disappearing
    probably by attaching a gpointer here ... */

static void draw_motion(void *z, t_floatarg dx, t_floatarg dy)
{
    //fprintf(stderr,"draw_motion\n");
    t_draw *x = (t_draw *)z;
    t_float mtx1[3][3];
    t_float mtx2[3][3];
    t_float m1, m2, m3, m4, m5, m6, tdx, tdy;
    t_draw *g = draw_getgroup(x);
    if (g)
    {
        draw_parsetransform(g, draw_motion_template, draw_motion_wp,
            &m1, &m2, &m3, &m4, &m5, &m6);
        draw_mset(mtx1, m1, m2, m3, m4, m5, m6);
    }
    else
        draw_mset(mtx1, 1, 0, 0, 1, 0, 0);
    draw_parsetransform(x, draw_motion_template, draw_motion_wp,
        &m1, &m2, &m3, &m4, &m5, &m6);
    draw_mset(mtx2, m1, m2, m3, m4, m5, m6);
    draw_mmult(mtx1, mtx2, mtx1);
    draw_minv(mtx1, mtx1);
    /* get rid of translation so it doesn't factor
       in to our deltas */
    mtx1[0][2] = 0;
    mtx1[1][2] = 0;
    draw_mset(mtx2, dx, dy, 0, 0, 0, 0);
    mtx2[2][0] = 1;
    draw_mmult(mtx1, mtx2, mtx2);
    tdx = mtx2[0][0];
    tdy = mtx2[1][0];
    t_fielddesc *f = x->x_vec + draw_motion_field;
    t_atom at;
    if (!gpointer_check(&draw_motion_gpointer, 0))
    {
        post("draw_motion: scalar disappeared");
        return;
    }
    draw_motion_xcumulative += tdx;
    draw_motion_ycumulative += tdy;
    if (f->fd_var && (tdx != 0))
    {
        fielddesc_setcoord(f, draw_motion_template, draw_motion_wp,
            draw_motion_xbase + draw_motion_xcumulative * draw_motion_xper,
                1); 
    }
    if ((f+1)->fd_var && (tdy != 0))
    {
        fielddesc_setcoord(f+1, draw_motion_template, draw_motion_wp,
            draw_motion_ybase + draw_motion_ycumulative * draw_motion_yper,
                1); 
    }
        /* LATER figure out what to do to notify for an array? */
    if (draw_motion_scalar)
        template_notifyforscalar(draw_motion_template, draw_motion_glist, 
            draw_motion_scalar, gensym("change"), 1, &at);
    if (draw_motion_scalar)
        scalar_redraw(draw_motion_scalar, draw_motion_glist);
    if (draw_motion_array)
        array_redraw(draw_motion_array, draw_motion_glist);
}

static int draw_click(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, t_scalar *sc, t_array *ap,
    t_float basex, t_float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    //fprintf(stderr,"draw_click %f %f %d %d %lx\n", basex, basey, xpix, ypix, (t_int)data);
    t_draw *x = (t_draw *)z;
    t_float mtx1[3][3];
    t_float mtx2[3][3];
    t_float m1, m2, m3, m4, m5, m6;
    t_draw *g = draw_getgroup(x);
    if (g)
    {
        draw_parsetransform(g, template, data, &m1, &m2, &m3, &m4, &m5, &m6);
        draw_mset(mtx1, m1, m2, m3, m4, m5, m6);
    }
    else
        draw_mset(mtx1, 1, 0, 0, 1, 0, 0);
    draw_parsetransform(x, template, data, &m1, &m2, &m3, &m4, &m5, &m6);
    draw_mset(mtx2, m1, m2, m3, m4, m5, m6);
    draw_mmult(mtx1, mtx2, mtx1);
    int i, n = x->x_nargs;
    int bestn = -1;
    int besterror = 0x7fffffff;
    t_fielddesc *f;
    if (!fielddesc_getfloat(&x->x_vis, template, data, 0))
        return (0);
    int nxy = n >> 1;
    for (i = 0, f = x->x_vec; i < nxy; i++, f += 2)
    {
        t_float xval = fielddesc_getcoord(f, template, data, 0);
        t_float yval = fielddesc_getcoord(f+1, template, data, 0);
        draw_mset(mtx2, xval, yval, 0, 0, 0, 0);
        mtx2[2][0] = 1;
        draw_mmult(mtx1, mtx2, mtx2);
        t_float txval = mtx2[0][0];
        t_float tyval = mtx2[1][0];
        int xloc = glist_xtopixels(glist, basex + txval);
        int yloc = glist_ytopixels(glist, basey + tyval);
        int xerr = xloc - xpix, yerr = yloc - ypix;
        if (!f->fd_var && !(f+1)->fd_var)
            continue;
        if (xerr < 0)
            xerr = -xerr;
        if (yerr < 0)
            yerr = -yerr;
        if (yerr > xerr)
            xerr = yerr;
        if (xerr < besterror)
        {
            draw_motion_xbase = xval;
            draw_motion_ybase = yval;
            besterror = xerr;
            bestn = i;
        }
    }
    if (besterror > 6)
        return (0);
    if (doit)
    {
        draw_motion_xper = glist_pixelstox(glist, 1)
            - glist_pixelstox(glist, 0);
        draw_motion_yper = glist_pixelstoy(glist, 1)
            - glist_pixelstoy(glist, 0);
        draw_motion_xcumulative = 0;
        draw_motion_ycumulative = 0;
        draw_motion_glist = glist;
        draw_motion_scalar = sc;
        draw_motion_array = ap;
        draw_motion_wp = data;
        draw_motion_field = 2*bestn;
        draw_motion_template = template;
        if (draw_motion_scalar)
            gpointer_setglist(&draw_motion_gpointer, draw_motion_glist,
                draw_motion_scalar);
        else gpointer_setarray(&draw_motion_gpointer,
                draw_motion_array, draw_motion_wp);
        glist_grab(glist, z, draw_motion, 0, xpix, ypix);
    }
    return (1);
}

t_parentwidgetbehavior draw_widgetbehavior =
{
    draw_getrect,
    draw_displace,
    draw_select,
    draw_activate,
    draw_vis,
    draw_click,
};

static void draw_free(t_draw *x)
{
    /* [draw group] has no pts in x_vec, but it looks like
       t_freebytes allocates a single byte so freeing it
       should be fine */
    t_freebytes(x->x_vec, x->x_nargs * sizeof(*x->x_vec));
    t_freebytes(x->x_strokedasharray,
        x->x_ndash * sizeof(*x->x_strokedasharray));
    t_freebytes(x->x_transform,
        x->x_transform_n * sizeof(*x->x_transform));
    if (x->x_drawtype == gensym("path"))
    {
        t_freebytes(x->x_pathcmds, x->x_npathcmds * sizeof(*x->x_pathcmds));
        t_freebytes(x->x_nargs_per_cmd, x->x_npathcmds * sizeof(*x->x_nargs_per_cmd));
    }
    char buf[50];
    sprintf(buf, ".x%lx", (long unsigned int)x);
    pd_unbind(&x->x_obj.ob_pd, gensym(buf));
}

static void draw_setup(void)
{
    draw_class = class_new(gensym("draw"), (t_newmethod)draw_new,
        (t_method)draw_free, sizeof(t_draw), 0, A_GIMME, 0);
    class_setdrawcommand(draw_class);
    class_setparentwidget(draw_class, &draw_widgetbehavior);
    class_addfloat(draw_class, draw_float);
    class_addmethod(draw_class, (t_method)draw_fill,
        gensym("fill"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_fillopacity,
        gensym("fill-opacity"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_fillrule,
        gensym("fill-rule"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_stroke,
        gensym("stroke"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_strokedasharray,
        gensym("stroke-dasharray"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_strokeopacity,
        gensym("stroke-opacity"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_strokelinecap,
        gensym("stroke-linecap"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_strokelinejoin,
        gensym("stroke-linejoin"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_strokemiterlimit,
        gensym("stroke-miterlimit"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_strokewidth,
        gensym("stroke-width"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_transform,
        gensym("transform"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_rx,
        gensym("rx"), A_GIMME, 0);
    class_addmethod(draw_class, (t_method)draw_ry,
        gensym("ry"), A_GIMME, 0);
}

/* ---------------- curves and polygons (joined segments) ---------------- */

/*
curves belong to templates and describe how the data in the template are to
be drawn.  The coordinates of the curve (and other display features) can
be attached to fields in the template.
*/

t_class *curve_class;

typedef struct _curve
{
    t_object x_obj;
    int x_flags;            /* CLOSED and/or BEZ and/or NOMOUSE */
    t_fielddesc x_fillcolor;
    t_fielddesc x_fillopacity;
    t_fielddesc x_fillrule;
    t_fielddesc x_outlinecolor;
    t_fielddesc *x_strokedasharray; /* array of lengths */
    t_fielddesc x_strokelinecap;
    t_fielddesc x_strokelinejoin;
    t_fielddesc x_strokemiterlimit;
    t_fielddesc x_strokeopacity;
    t_fielddesc x_strokewidth;
    t_fielddesc *x_matrix;
    t_fielddesc x_width;
    t_fielddesc x_vis;
    int x_npoints;
    t_fielddesc *x_vec;
    t_canvas *x_canvas;
} t_curve;

static void *curve_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    t_curve *x = (t_curve *)pd_new(curve_class);
    char *classname = classsym->s_name;
    int flags = 0;
    int nxy, i;
    t_fielddesc *fd;
    x->x_canvas = canvas_getcurrent();
    if (classname[0] == 'f')
    {
        classname += 6;
        flags |= CLOSED;
    }
    else classname += 4;
    if (classname[0] == 'c' || classname[0] == 'e') flags |= BEZ;
    if (classname[0] == 'e' || classname[0] == 'r') flags |= BBOX;
    fielddesc_setfloat_const(&x->x_vis, 1);
    while (1)
    {
        t_symbol *firstarg = atom_getsymbolarg(0, argc, argv);
        if (!strcmp(firstarg->s_name, "-v") && argc > 1)
        {
            fielddesc_setfloatarg(&x->x_vis, 1, argv+1);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(firstarg->s_name, "-x"))
        {
            flags |= NOMOUSE;
            argc -= 1; argv += 1;
        }
        else break;
    }
    x->x_flags = flags;
    if ((flags & CLOSED) && argc)
        fielddesc_setfloatarg(&x->x_fillcolor, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_fillcolor, 0); 
    if (argc) fielddesc_setfloatarg(&x->x_outlinecolor, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_outlinecolor, 0);
    if (argc) fielddesc_setfloatarg(&x->x_width, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_width, 1);
    if (argc < 0) argc = 0;
    nxy =  (argc + (argc & 1));
    x->x_npoints = (nxy>>1);
    x->x_vec = (t_fielddesc *)t_getbytes(nxy * sizeof(t_fielddesc));
    for (i = 0, fd = x->x_vec; i < argc; i++, fd++, argv++)
        fielddesc_setfloatarg(fd, 1, argv);
    if (argc & 1) fielddesc_setfloat_const(fd, 0);
    fielddesc_setfloat_const(&x->x_fillopacity, 1);
    return (x);
}

void curve_float(t_curve *x, t_floatarg f)
{
    int viswas;
    if (x->x_vis.fd_type != A_FLOAT || x->x_vis.fd_var)
    {
        pd_error(x, "global vis/invis for a template with variable visibility");
        return;
    }
    viswas = (x->x_vis.fd_un.fd_float != 0);
    
    if ((f != 0 && viswas) || (f == 0 && !viswas))
        return;
    canvas_redrawallfortemplatecanvas(x->x_canvas, 2);
    fielddesc_setfloat_const(&x->x_vis, (f != 0));
    canvas_redrawallfortemplatecanvas(x->x_canvas, 1);
}

void curve_fillopacity(t_curve *x, t_symbol *s, t_int argc, t_atom *argv)
{
    char *classname = s->s_name;
    if (classname[0] == 'd' || argc < 1) return;
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&x->x_fillopacity, argc, argv);
        canvas_redrawallfortemplatecanvas(x->x_canvas, 0);
    }
}

/* -------------------- widget behavior for curve ------------ */

static void curve_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
	//fprintf(stderr,">>>>>>>>>>>>>>>>>>>>>>curve_getrect %lx\n", (t_int)z);
    t_curve *x = (t_curve *)z;
    int i, n = x->x_npoints;
    t_fielddesc *f = x->x_vec;
    int x1 = 0x7fffffff, x2 = -0x7fffffff, y1 = 0x7fffffff, y2 = -0x7fffffff;
    if (!fielddesc_getfloat(&x->x_vis, template, data, 0) ||
        (x->x_flags & NOMOUSE))
    {
        *xp1 = *yp1 = 0x7fffffff;
        *xp2 = *yp2 = -0x7fffffff;
        return;
    }
    for (i = 0, f = x->x_vec; i < n; i++, f += 2)
    {
        int xloc = glist_xtopixels(glist,
            basex + fielddesc_getcoord(f, template, data, 0));
        int yloc = glist_ytopixels(glist,
            basey + fielddesc_getcoord(f+1, template, data, 0));
        if (xloc < x1) x1 = xloc;
        if (xloc > x2) x2 = xloc;
        if (yloc < y1) y1 = yloc;
        if (yloc > y2) y2 = yloc;
    }
    if ((x->x_flags & BEZ) && (x->x_flags & BBOX))
    {
        int cx = glist_xtopixels(glist,
            basex + fielddesc_getcoord(x->x_vec, template,
                data, 0));
        int cy = glist_ytopixels(glist,
            basey + fielddesc_getcoord(x->x_vec+1, template,
                data, 0));
        int rx = fielddesc_getfloat(x->x_vec+2, template,
            data, 0);
        int ry = fielddesc_getfloat(x->x_vec+3, template,
            data, 0);
        x1 = cx - rx;
        y1 = cy - ry;
        x2 = cx + rx;
        y2 = cy + ry;
    }
	//fprintf(stderr,"FINAL curve_getrect %d %d %d %d\n", x1, y1, x2, y2);
    //sys_vgui(".x%lx.c create prect %d %d %d %d -stroke red -tags blah\n",
    //        glist_getcanvas(glist), x1, y1, x2, y2);
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2; 
}

static void curve_displace(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int dx, int dy)
{
    /* refuse */
}

static void curve_select(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    //fprintf(stderr,"curve_select %d\n", state);
    /* fill in later */
}

static void curve_activate(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    /* fill in later */
}

#if 0
static int rangecolor(int n)    /* 0 to 9 in 5 steps */
{
    int n2 = n/2;               /* 0 to 4 */
    int ret = (n2 << 6);        /* 0 to 256 in 5 steps */
    if (ret > 255) ret = 255;
    return (ret);
}
#endif

static int rangecolor(int n)    /* 0 to 9 in 5 steps */
{
    int n2 = (n == 9 ? 8 : n);               /* 0 to 8 */
    int ret = (n2 << 5);        /* 0 to 256 in 9 steps */
    if (ret > 255) ret = 255;
    return (ret);
}

static void numbertocolor(int n, char *s)
{
    int red, blue, green;
    if (n < 0) n = 0;
    red = n / 100;
    blue = ((n / 10) % 10);
    green = n % 10;
    sprintf(s, "#%2.2x%2.2x%2.2x", rangecolor(red), rangecolor(blue),
        rangecolor(green));
}

static void curve_vis(t_gobj *z, t_glist *glist, t_scalar *sc, 
    t_word *data, t_template *template, t_float basex, t_float basey,
    int vis)
{
    t_curve *x = (t_curve *)z;
    int i, n = x->x_npoints;
    t_fielddesc *f = x->x_vec;

	/*// get the universal tag for all nested objects
	t_canvas *tag = x->x_canvas;
	while (tag->gl_owner) {
		tag = tag->gl_owner;
	}*/
    
        /* see comment in plot_vis() */
    if (vis && !fielddesc_getfloat(&x->x_vis, template, data, 0))
        return;
    if (vis)
    {
        if (n > 1)
        {
            /* The first variable here is an obscure hack. If
               "sc->sc_vec" is not the same address as "data"
               then we're drawing this curve as an element of
               an array. It's because plot_vis cycles through
               an array's char *a_vec, casts each element to
               a t_word* and send it to us as the "data" param.
               But we send the same "sc" scalar each time so
               we can use it to do this check. This should be
               revised so it's done in a more sane fashion
            */
            int in_array = (sc->sc_vec == data) ? 0 : 1;
            int flags = x->x_flags, closed = (flags & CLOSED);
            t_float width = fielddesc_getfloat(&x->x_width, template, data, 1);
            char outline[20], fill[20];
            int pix[200];
            if (n > 100)
                n = 100;
                /* calculate the pixel values before we start printing
                out the TK message so that "error" printout won't be
                interspersed with it.  Only show up to 100 points so we don't
                have to allocate memory here. */
            for (i = 0, f = x->x_vec; i < n; i++, f += 2)
            {
                //pix[2*i] = glist_xtopixels(glist,
                //    basex + fielddesc_getcoord(f, template, data, 1));
                //pix[2*i+1] = glist_ytopixels(glist,
                //    basey + fielddesc_getcoord(f+1, template, data, 1));
                pix[2*i] = fielddesc_getcoord(f, template, data, 1);
                pix[2*i+1] = fielddesc_getcoord(f+1, template, data, 1);
            }
            if (width < 1) width = 1;
            numbertocolor(
                fielddesc_getfloat(&x->x_outlinecolor, template, data, 1),
                outline);
            if (flags & CLOSED)
            {
                numbertocolor(
                    fielddesc_getfloat(&x->x_fillcolor, template, data, 1),
                    fill);
                if (flags & CLOSED && !(flags & BBOX))
                {
                    sys_vgui(".x%lx.c create ppolygon \\\n",
                        glist_getcanvas(glist));
                }
                else if (flags & BBOX) /* rectangles and ellipses */
                {
                    n = 2; /* silently truncate extra coordinates */
                    if(flags & BEZ)
                        sys_vgui(".x%lx.c create ellipse \\\n",
                            glist_getcanvas(glist));
                    else
                        sys_vgui(".x%lx.c create prect \\\n",
                        glist_getcanvas(glist));
                }
            }
            else
            {
                if(flags & BBOX)
                {
                    if(flags & BEZ)
                        sys_vgui(".x%lx.c create ellipse \\\n", glist_getcanvas(glist));
                    else
                        sys_vgui(".x%lx.c create prect \\\n", glist_getcanvas(glist));
                } else
                    sys_vgui(".x%lx.c create polyline \\\n", glist_getcanvas(glist));
            }
            for (i = 0; i < n; i++)
            {
                //sys_vgui("%d %d \\\n", pix[2*i], pix[2*i+1]);
                sys_vgui("%d %d \\\n",
                    pix[2*i] + (in_array ? (int)basex : 0),
                    pix[2*i+1] + (in_array ? (int)basey : 0));
                if ((flags & BEZ) && (flags & BBOX))
                {
                    sys_vgui("-rx %d -ry %d \\\n",
                        (t_int)fielddesc_getfloat(x->x_vec+2,
                            template, data, 1),
                        (t_int)fielddesc_getfloat(x->x_vec+3,
                            template, data, 1));
                    break;
                }
            }
            sys_vgui("-strokewidth %f \\\n", width);
		if (flags & CLOSED) sys_vgui("-fill %s -stroke %s -fillopacity %g \\\n",
                fill, outline, fielddesc_getfloat(&x->x_fillopacity, template, data, 1));
            else if(flags & BBOX) sys_vgui("-stroke %s \\\n", outline);
            else sys_vgui("-stroke %s \\\n", outline);
            sys_vgui("-parent .dgroup%lx \\\n", sc->sc_vec);
            //if ((flags & BEZ) && !(flags & BBOX)) sys_vgui("-smooth 1 \\\n"); //this doesn't work with tkpath
            sys_vgui("-tags {.x%lx.x%lx.template%lx scalar%lx}\n", glist_getcanvas(glist), glist,
				data, sc);
			if (!glist_istoplevel(glist)) {
				t_canvas *gl = glist_getcanvas(glist);
				//glist_noselect(gl);
				//glist_select(gl, (t_gobj *)glist);
				char objtag[64];
				sprintf(objtag, ".x%lx.x%lx.template%lx", (t_int)gl, (t_int)glist, (t_int)data);
				canvas_restore_original_position(gl, (t_gobj *)glist, objtag, -1);
			}
        }
        else post("warning: curves need at least two points to be graphed");
    }
    else
    {
        if (n > 1) sys_vgui(".x%lx.c delete .x%lx.x%lx.template%lx\n",
            glist_getcanvas(glist), glist_getcanvas(glist), glist,
			data);      
    }
}

static int curve_motion_field;
static t_float curve_motion_xcumulative;
static t_float curve_motion_xbase;
static t_float curve_motion_xper;
static t_float curve_motion_ycumulative;
static t_float curve_motion_ybase;
static t_float curve_motion_yper;
static t_glist *curve_motion_glist;
static t_scalar *curve_motion_scalar;
static t_array *curve_motion_array;
static t_word *curve_motion_wp;
static t_template *curve_motion_template;
static t_gpointer curve_motion_gpointer;

    /* LATER protect against the template changing or the scalar disappearing
    probably by attaching a gpointer here ... */

static void curve_motion(void *z, t_floatarg dx, t_floatarg dy)
{
	//fprintf(stderr,"curve_motion\n");
    t_curve *x = (t_curve *)z;
    t_fielddesc *f = x->x_vec + curve_motion_field;
    t_atom at;
    if (!gpointer_check(&curve_motion_gpointer, 0))
    {
        post("curve_motion: scalar disappeared");
        return;
    }
    curve_motion_xcumulative += dx;
    curve_motion_ycumulative += dy;
    if (f->fd_var && (dx != 0))
    {
        fielddesc_setcoord(f, curve_motion_template, curve_motion_wp,
            curve_motion_xbase + curve_motion_xcumulative * curve_motion_xper,
                1); 
    }
    if ((f+1)->fd_var && (dy != 0))
    {
        fielddesc_setcoord(f+1, curve_motion_template, curve_motion_wp,
            curve_motion_ybase + curve_motion_ycumulative * curve_motion_yper,
                1); 
    }
        /* LATER figure out what to do to notify for an array? */
    if (curve_motion_scalar)
        template_notifyforscalar(curve_motion_template, curve_motion_glist, 
            curve_motion_scalar, gensym("change"), 1, &at);
    if (curve_motion_scalar)
        scalar_redraw(curve_motion_scalar, curve_motion_glist);
    if (curve_motion_array)
        array_redraw(curve_motion_array, curve_motion_glist);
}

static int curve_click(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, t_scalar *sc, t_array *ap,
    t_float basex, t_float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
	//fprintf(stderr,"curve_click %f %f %d %d %lx\n", basex, basey, xpix, ypix, (t_int)data);
    t_curve *x = (t_curve *)z;
    int i, n = x->x_npoints;
    int bestn = -1;
    int besterror = 0x7fffffff;
    t_fielddesc *f;
    if (!fielddesc_getfloat(&x->x_vis, template, data, 0))
        return (0);
    for (i = 0, f = x->x_vec; i < n; i++, f += 2)
    {
        int xval = fielddesc_getcoord(f, template, data, 0),
            xloc = glist_xtopixels(glist, basex + xval);
        int yval = fielddesc_getcoord(f+1, template, data, 0),
            yloc = glist_ytopixels(glist, basey + yval);
        int xerr = xloc - xpix, yerr = yloc - ypix;
        if (!f->fd_var && !(f+1)->fd_var)
            continue;
        if (xerr < 0)
            xerr = -xerr;
        if (yerr < 0)
            yerr = -yerr;
        if (yerr > xerr)
            xerr = yerr;
        if (xerr < besterror)
        {
            curve_motion_xbase = xval;
            curve_motion_ybase = yval;
            besterror = xerr;
            bestn = i;
        }
    }
    if (besterror > 6)
        return (0);
    if (doit)
    {
        curve_motion_xper = glist_pixelstox(glist, 1)
            - glist_pixelstox(glist, 0);
        curve_motion_yper = glist_pixelstoy(glist, 1)
            - glist_pixelstoy(glist, 0);
        curve_motion_xcumulative = 0;
        curve_motion_ycumulative = 0;
        curve_motion_glist = glist;
        curve_motion_scalar = sc;
        curve_motion_array = ap;
        curve_motion_wp = data;
        curve_motion_field = 2*bestn;
        curve_motion_template = template;
        if (curve_motion_scalar)
            gpointer_setglist(&curve_motion_gpointer, curve_motion_glist,
                curve_motion_scalar);
        else gpointer_setarray(&curve_motion_gpointer,
                curve_motion_array, curve_motion_wp);
        glist_grab(glist, z, curve_motion, 0, xpix, ypix);
    }
    return (1);
}

t_parentwidgetbehavior curve_widgetbehavior =
{
    curve_getrect,
    curve_displace,
    curve_select,
    curve_activate,
    curve_vis,
    curve_click,
};

static void curve_free(t_curve *x)
{
    t_freebytes(x->x_vec, 2 * x->x_npoints * sizeof(*x->x_vec));
}

static void curve_setup(void)
{
    curve_class = class_new(gensym("drawpolygon"), (t_newmethod)curve_new,
        (t_method)curve_free, sizeof(t_curve), 0, A_GIMME, 0);
    class_setdrawcommand(curve_class);
    class_addcreator((t_newmethod)curve_new, gensym("drawcurve"),
        A_GIMME, 0);
    class_addcreator((t_newmethod)curve_new, gensym("filledpolygon"),
        A_GIMME, 0);
    class_addcreator((t_newmethod)curve_new, gensym("filledcurve"),
        A_GIMME, 0);
    class_addcreator((t_newmethod)curve_new, gensym("drawrectangle"),
        A_GIMME, 0);
    class_addcreator((t_newmethod)curve_new, gensym("filledrectangle"),
        A_GIMME, 0);
    class_addcreator((t_newmethod)curve_new, gensym("drawellipse"),
        A_GIMME, 0);
    class_addcreator((t_newmethod)curve_new, gensym("filledellipse"),
        A_GIMME, 0);
    class_setparentwidget(curve_class, &curve_widgetbehavior);
    class_addfloat(curve_class, curve_float);
    class_addmethod(curve_class, (t_method)curve_fillopacity,
        gensym("fillopacity"), A_GIMME, 0);
}

/* --------- plots for showing arrays --------------- */

t_class *plot_class;

typedef struct _plot
{
    t_object x_obj;
    t_canvas *x_canvas;
    t_fielddesc x_outlinecolor;
    t_fielddesc x_width;
    t_fielddesc x_xloc;
    t_fielddesc x_yloc;
    t_fielddesc x_xinc;
    t_fielddesc x_style;
    t_fielddesc x_data;
    t_fielddesc x_xpoints;
    t_fielddesc x_ypoints;
    t_fielddesc x_wpoints;
    t_fielddesc x_vis;          /* visible */
    t_fielddesc x_scalarvis;    /* true if drawing the scalar at each point */
    t_fielddesc x_symoutlinecolor; /* color as hex symbol */
    t_fielddesc x_symfillcolor;    /* fill color as hex symbol */
} t_plot;

static void *plot_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    t_plot *x = (t_plot *)pd_new(plot_class);
    int defstyle = PLOTSTYLE_POLY;
    x->x_canvas = canvas_getcurrent();

    fielddesc_setfloat_var(&x->x_xpoints, gensym("x"));
    fielddesc_setfloat_var(&x->x_ypoints, gensym("y"));
    fielddesc_setfloat_var(&x->x_wpoints, gensym("w"));
    
    fielddesc_setfloat_const(&x->x_vis, 1);
    fielddesc_setfloat_const(&x->x_scalarvis, 1);
    while (1)
    {
        t_symbol *firstarg = atom_getsymbolarg(0, argc, argv);
        if (!strcmp(firstarg->s_name, "curve") ||
            !strcmp(firstarg->s_name, "-c"))
        {
            defstyle = PLOTSTYLE_BEZ;
            argc--, argv++;
        }
        else if (!strcmp(firstarg->s_name, "-v") && argc > 1)
        {
            fielddesc_setfloatarg(&x->x_vis, 1, argv+1);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(firstarg->s_name, "-vs") && argc > 1)
        {
            fielddesc_setfloatarg(&x->x_scalarvis, 1, argv+1);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(firstarg->s_name, "-x") && argc > 1)
        {
            fielddesc_setfloatarg(&x->x_xpoints, 1, argv+1);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(firstarg->s_name, "-y") && argc > 1)
        {
            fielddesc_setfloatarg(&x->x_ypoints, 1, argv+1);
            argc -= 2; argv += 2;
        }
        else if (!strcmp(firstarg->s_name, "-w") && argc > 1)
        {
            fielddesc_setfloatarg(&x->x_wpoints, 1, argv+1);
            argc -= 2; argv += 2;
        }
        else break;
    }
    if (argc) fielddesc_setarrayarg(&x->x_data, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_data, 1);
    if (argc) fielddesc_setfloatarg(&x->x_outlinecolor, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_outlinecolor, 0);
    if (argc) fielddesc_setfloatarg(&x->x_width, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_width, 1);
    if (argc) fielddesc_setfloatarg(&x->x_xloc, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_xloc, 1);
    if (argc) fielddesc_setfloatarg(&x->x_yloc, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_yloc, 1);
    if (argc) fielddesc_setfloatarg(&x->x_xinc, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_xinc, 1);
    if (argc) fielddesc_setfloatarg(&x->x_style, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_style, defstyle);
    if (argc) fielddesc_setsymbolarg(&x->x_symfillcolor, argc--, argv++);
    else argc--, argv++;
    if (argc) fielddesc_setsymbolarg(&x->x_symoutlinecolor, argc--, argv++);

    return (x);
}

void plot_float(t_plot *x, t_floatarg f)
{
    int viswas;
    if (x->x_vis.fd_type != A_FLOAT || x->x_vis.fd_var)
    {
        pd_error(x, "global vis/invis for a template with variable visibility");
        return;
    }
    viswas = (x->x_vis.fd_un.fd_float != 0);
    
    if ((f != 0 && viswas) || (f == 0 && !viswas))
        return;
    canvas_redrawallfortemplatecanvas(x->x_canvas, 2);
    fielddesc_setfloat_const(&x->x_vis, (f != 0));
    canvas_redrawallfortemplatecanvas(x->x_canvas, 1);
}

/* -------------------- widget behavior for plot ------------ */


    /* get everything we'll need from the owner template of the array being
    plotted. Not used for garrays, but see below */
static int plot_readownertemplate(t_plot *x,
    t_word *data, t_template *ownertemplate, 
    t_symbol **elemtemplatesymp, t_array **arrayp,
    t_float *linewidthp, t_float *xlocp, t_float *xincp, t_float *ylocp, t_float *stylep,
    t_float *visp, t_float *scalarvisp,
    t_fielddesc **xfield, t_fielddesc **yfield, t_fielddesc **wfield, t_symbol **fillcolorp,
    t_symbol **outlinecolorp)
{
    int arrayonset, type;
    t_symbol *elemtemplatesym;
    t_array *array;

        /* find the data and verify it's an array */
    if (x->x_data.fd_type != A_ARRAY || !x->x_data.fd_var)
    {
        error("plot: needs an array field");
        return (-1);
    }
    if (!template_find_field(ownertemplate, x->x_data.fd_un.fd_varsym,
        &arrayonset, &type, &elemtemplatesym))
    {
        error("plot: %s: no such field", x->x_data.fd_un.fd_varsym->s_name);
        return (-1);
    }
    if (type != DT_ARRAY)
    {
        error("plot: %s: not an array", x->x_data.fd_un.fd_varsym->s_name);
        return (-1);
    }
    array = *(t_array **)(((char *)data) + arrayonset);
    *linewidthp = fielddesc_getfloat(&x->x_width, ownertemplate, data, 1);
    *xlocp = fielddesc_getfloat(&x->x_xloc, ownertemplate, data, 1);
    *xincp = fielddesc_getfloat(&x->x_xinc, ownertemplate, data, 1);
    *ylocp = fielddesc_getfloat(&x->x_yloc, ownertemplate, data, 1);
    *stylep = fielddesc_getfloat(&x->x_style, ownertemplate, data, 1);
    *visp = fielddesc_getfloat(&x->x_vis, ownertemplate, data, 1);
    *scalarvisp = fielddesc_getfloat(&x->x_scalarvis, ownertemplate, data, 1);
    *elemtemplatesymp = elemtemplatesym;
    *arrayp = array;
    *xfield = &x->x_xpoints;
    *yfield = &x->x_ypoints;
    *wfield = &x->x_wpoints;
    *fillcolorp = fielddesc_getsymbol(&x->x_symfillcolor, ownertemplate,
        data, 0);
    *outlinecolorp = fielddesc_getsymbol(&x->x_symoutlinecolor, ownertemplate,
        data, 0);

    return (0);
}

    /* get everything else you could possibly need about a plot,
    either for plot's own purposes or for plotting a "garray" */
int array_getfields(t_symbol *elemtemplatesym,
    t_canvas **elemtemplatecanvasp,
    t_template **elemtemplatep, int *elemsizep,
    t_fielddesc *xfielddesc, t_fielddesc *yfielddesc, t_fielddesc *wfielddesc, 
    int *xonsetp, int *yonsetp, int *wonsetp)
{
    int arrayonset, elemsize, yonset, wonset, xonset, type;
    t_template *elemtemplate;
    t_symbol *dummy, *varname;
    t_canvas *elemtemplatecanvas = 0;

        /* the "float" template is special in not having to have a canvas;
        template_findbyname is hardwired to return a predefined 
        template. */

    if (!(elemtemplate =  template_findbyname(elemtemplatesym)))
    {
        error("plot: %s: no such template", elemtemplatesym->s_name);
        return (-1);
    }
    if (!((elemtemplatesym == &s_float) ||
        (elemtemplatecanvas = template_findcanvas(elemtemplate))))
    {
        error("plot: %s: no canvas for this template", elemtemplatesym->s_name);
        return (-1);
    }
    elemsize = elemtemplate->t_n * sizeof(t_word);
    if (yfielddesc && yfielddesc->fd_var)
        varname = yfielddesc->fd_un.fd_varsym;
    else varname = gensym("y");
    if (!template_find_field(elemtemplate, varname, &yonset, &type, &dummy)
        || type != DT_FLOAT)    
            yonset = -1;
    if (xfielddesc && xfielddesc->fd_var)
        varname = xfielddesc->fd_un.fd_varsym;
    else varname = gensym("x");
    if (!template_find_field(elemtemplate, varname, &xonset, &type, &dummy)
        || type != DT_FLOAT) 
            xonset = -1;
    if (wfielddesc && wfielddesc->fd_var)
        varname = wfielddesc->fd_un.fd_varsym;
    else varname = gensym("w");
    if (!template_find_field(elemtemplate, varname, &wonset, &type, &dummy)
        || type != DT_FLOAT) 
            wonset = -1;

        /* fill in slots for return values */
    *elemtemplatecanvasp = elemtemplatecanvas;
    *elemtemplatep = elemtemplate;
    *elemsizep = elemsize;
    *xonsetp = xonset;
    *yonsetp = yonset;
    *wonsetp = wonset;
    return (0);
}

static void plot_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
	//fprintf(stderr,"plot_getrect\n");
    t_plot *x = (t_plot *)z;
    int elemsize, yonset, wonset, xonset;
    t_canvas *elemtemplatecanvas;
    t_template *elemtemplate;
    t_symbol *elemtemplatesym;
    t_symbol *symfillcolor;
    t_symbol *symoutlinecolor;
    t_float linewidth, xloc, xinc, yloc, style, xsum, yval, vis, scalarvis;
    t_array *array;
    int x1 = 0x7fffffff, y1 = 0x7fffffff, x2 = -0x7fffffff, y2 = -0x7fffffff;
    int i;
    t_float xpix1, xpix2, ypix, wpix;
    t_fielddesc *xfielddesc, *yfielddesc, *wfielddesc;
        /* if we're the only plot in the glist claim the whole thing */
    /*if (glist->gl_list && !glist->gl_list->g_next)
    {
        *xp1 = *yp1 = -0x7fffffff;
        *xp2 = *yp2 = 0x7fffffff;
        return;
    }*/
    if (!plot_readownertemplate(x, data, template, 
        &elemtemplatesym, &array, &linewidth, &xloc, &xinc, &yloc, &style,
        &vis, &scalarvis, &xfielddesc, &yfielddesc, &wfielddesc,
        &symfillcolor, &symoutlinecolor) &&
                (vis != 0) &&
            !array_getfields(elemtemplatesym, &elemtemplatecanvas,
                &elemtemplate, &elemsize, 
                xfielddesc, yfielddesc, wfielddesc,
                &xonset, &yonset, &wonset))
    {
            /* if it has more than 2000 points, just check 1000 of them. */
        int incr = (array->a_n <= 2000 ? 1 : array->a_n / 1000);
        for (i = 0, xsum = 0; i < array->a_n; i += incr)
        {
            t_float usexloc, useyloc;
            t_gobj *y;
                /* get the coords of the point proper */
            array_getcoordinate(glist, (char *)(array->a_vec) + i * elemsize,
                xonset, yonset, wonset, i, basex + xloc, basey + yloc, xinc,
                xfielddesc, yfielddesc, wfielddesc, &xpix1, &xpix2, &ypix, &wpix);
			//fprintf(stderr,"		!!!!!!!!elemsize%d yonset%d wonset%d xonset%d i%d basex%f xloc%f basey%f yloc%f xinc%f xpix%f ypix%f wpix%f\n", elemsize, yonset, wonset, xonset, i, basex, xloc, basey, yloc, xinc, xpix, ypix, wpix);
            if (xpix1 < x1)
                x1 = xpix1;
            if (xpix2 > x2)
                x2 = xpix2;
            if (ypix - wpix < y1)
                y1 = ypix - wpix;
            if (ypix + wpix > y2)
                y2 = ypix + wpix;

			//fprintf(stderr,"	/////plot_getrect %d %d %d %d\n", x1, y1, x2, y2);
            
            if (scalarvis != 0)
            {
                    /* check also the drawing instructions for the scalar */ 
                if (xonset >= 0)
                    usexloc = basex + xloc + fielddesc_cvttocoord(xfielddesc, 
                        *(t_float *)(((char *)(array->a_vec) + elemsize * i)
                            + xonset));
                //else usexloc = x1; //usexloc = basex + xsum, xsum += xinc;
                usexloc = basex + xsum, xsum += xinc;
                if (yonset >= 0)
                    yval = *(t_float *)(((char *)(array->a_vec) + elemsize * i)
                        + yonset);
                else yval = 0;
                //useyloc = (y1+y2)/2; //basey + yloc + fielddesc_cvttocoord(yfielddesc, yval);
                useyloc = basey + yloc + fielddesc_cvttocoord(yfielddesc, yval);
                for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
                {
					//fprintf(stderr,".-.-. usexloc %f useyloc %f (alt %f %f)\n", usexloc, useyloc, basex + xloc + fielddesc_cvttocoord(xfielddesc, *(t_float *)(((char *)(array->a_vec) + elemsize * i) + xonset)), *(t_float *)(((char *)(array->a_vec) + elemsize * i) + yonset));
                    int xx1, xx2, yy1, yy2;
                    t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
                    if (!wb) continue;
                    (*wb->w_parentgetrectfn)(y, glist,
                        (t_word *)((char *)(array->a_vec) + elemsize * i),
                            elemtemplate, usexloc, useyloc, 
                                &xx1, &yy1, &xx2, &yy2);
					//fprintf(stderr,"	.....plot_getrect %d %d %d %d\n", xx1, yy1, xx2, yy2); 
                    if (xx1 < x1)
                        x1 = xx1;
                    if (yy1 < y1)
                        y1 = yy1;
                     if (xx2 > x2)
                        x2 = xx2;
                    if (yy2 > y2)
                        y2 = yy2;
					//fprintf(stderr,"	.....plot_getrect %d %d %d %d\n", x1, y1, x2, y2); 
                }
            }
			//fprintf(stderr,"	>====plot_getrect %d %d %d %d\n", x1, y1, x2, y2);
        }
    }
	//fprintf(stderr,"FINAL plot_getrect %d %d %d %d\n", x1, y1, x2, y2);

    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

static void plot_displace(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int dx, int dy)
{
        /* not yet */
}

static void plot_select(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    //fprintf(stderr,"plot_select %d\n", state);
    /* not yet */
}

static void plot_activate(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
        /* not yet */
}

static void plot_vis(t_gobj *z, t_glist *glist, t_scalar *sc, 
    t_word *data, t_template *template, t_float basex, t_float basey,
    int tovis)
{
    t_plot *x = (t_plot *)z;
	/*// get the universal tag for all nested objects
	t_canvas *tag = x->x_canvas;
	while (tag->gl_owner) {
		tag = tag->gl_owner;
	}*/

	//fprintf(stderr,"===============plot %lx glist %lx glist_getcanvas %lx plot->x_obj %lx plot->x_canvas %lx glist_getcanvas(plot->x_canvas) %lx\n", (t_int)x, (t_int)glist, (t_int)glist_getcanvas(glist), (t_int)&x->x_obj, (t_int)x->x_canvas, (t_int)x->x_canvas->gl_owner);
	int draw_me = 1;	//used for experimental disabling of drawing outside GOP bounds
    int elemsize, yonset, wonset, xonset, i;
    t_canvas *elemtemplatecanvas;
    t_template *elemtemplate;
    t_symbol *elemtemplatesym;
    t_float linewidth, xloc, xinc, yloc, style, usexloc, xsum, yval, vis,
        scalarvis;
    t_symbol *symfill;
    t_symbol *symoutline;
    char outline[20];
    numbertocolor(fielddesc_getfloat(&x->x_outlinecolor, template,
        data, 1), outline);
    t_array *array;
    int nelem;
    char *elem;
    t_fielddesc *xfielddesc, *yfielddesc, *wfielddesc;
        /* even if the array is "invisible", if its visibility is
        set by an instance variable you have to explicitly erase it,
        because the flag could earlier have been on when we were getting
        drawn.  Rather than look to try to find out whether we're
        visible we just do the erasure.  At the TK level this should
        cause no action because the tag matches nobody.  LATER we
        might want to optimize this somehow.  Ditto the "vis()" routines
        for other drawing instructions. */
        
    if (plot_readownertemplate(x, data, template, 
        &elemtemplatesym, &array, &linewidth, &xloc, &xinc, &yloc, &style,
        &vis, &scalarvis, &xfielddesc, &yfielddesc, &wfielddesc, &symfill,
        &symoutline) ||
            ((vis == 0) && tovis) /* see above for 'tovis' */
            || array_getfields(elemtemplatesym, &elemtemplatecanvas,
                &elemtemplate, &elemsize, xfielddesc, yfielddesc, wfielddesc,
                &xonset, &yonset, &wonset))
                    return;
    nelem = array->a_n;
    elem = (char *)array->a_vec;

    if (tovis)
    {
        /* check if old 3-digit color field is being used... */
        int dscolor = fielddesc_getfloat(&x->x_outlinecolor, template, data, 1);
        if (dscolor != 0)
        {
            char outline[20];
            numbertocolor(dscolor, outline);
            symoutline = gensym(outline);
        }
        if (symoutline == &s_) symoutline = gensym("#000000");
        if (symfill == &s_) symfill = gensym("#000000");
        if (style == PLOTSTYLE_POINTS || style == PLOTSTYLE_BARS)
        {
            symfill = style == PLOTSTYLE_POINTS ? symoutline : symfill;
            t_float minyval = 1e20, maxyval = -1e20;
            int ndrawn = 0;
            sys_vgui(".x%lx.c create path { \\\n", glist_getcanvas(glist));
            for (xsum = basex + xloc, i = 0; i < nelem; i++)
            {
                t_float yval, xpix, ypix, nextxloc;
                int ixpix, inextx;

                if (xonset >= 0)
                {
                    usexloc = basex + xloc +
                        *(t_float *)((elem + elemsize * i) + xonset);
                    ixpix = glist_xtopixels(glist, 
                        fielddesc_cvttocoord(xfielddesc, usexloc));
                    inextx = ixpix + 2;
                }
                else
                {
                    usexloc = xsum;
                    xsum += xinc;
                    ixpix = glist_xtopixels(glist,
                        fielddesc_cvttocoord(xfielddesc, usexloc));
                    inextx = glist_xtopixels(glist,
                        fielddesc_cvttocoord(xfielddesc, xsum));
                }

                if (yonset >= 0)
                    yval = yloc + *(t_float *)((elem + elemsize * i) + yonset);
                else yval = 0;
                if (yval > maxyval)
                    maxyval = yval;
                if (yval < minyval)
                    minyval = yval;
                if (i == nelem-1 || inextx != ixpix)
                {
                    int py2 = 0;
                    int border = 0;
                    if(style == PLOTSTYLE_POINTS)
                        py2 = (int)(glist_ytopixels(glist,
                            basey + fielddesc_cvttocoord(yfielddesc, maxyval))
                                + linewidth) - 1;
                    else
                    {
                        /* this should probably be changed to anchor to the
                           y-minimum instead of the bottom of the graph. That
                           way the user can invert the y min/max to get a graph
                           anchored from the top */

                        if(glist->gl_isgraph && !glist->gl_havewindow)
                        {
                            int x1, y1, x2, y2;
                            graph_graphrect(&glist->gl_gobj, glist->gl_owner,
                                &x1, &y1, &x2, &y2);
                            py2 = y2;
                            border = 1;
                        }
                    }
                //fprintf(stderr,"%f %f %f %f %f\n", basey, minyval, maxyval,glist->gl_y2,glist->gl_y1);
                // with the following experimental code we can prevent drawing outside the gop window (preferred but needs to be further tested)
                /*if (glist->gl_y2 > glist->gl_y1) {
                   if (minyval >= glist->gl_y1 && maxyval <= glist->gl_y2) draw_me = 1;
                   else draw_me = 0; 
                } else {
                if (minyval >= glist->gl_y2 && maxyval <= glist->gl_y1) draw_me = 1;
                else draw_me = 0; 
                }
                if (draw_me) {*/
                //we subtract 1 from y to keep it in sync with the rest of the types of templates
                        /* This is the old, inefficient code that creates a separate canvas item for each element... 
                sys_vgui(
                        ".x%lx.c create prect %d %d %d %d -fill %s -stroke %s -strokewidth %d -tags {.x%lx.x%lx.template%lx array}\n",
                    glist_getcanvas(glist),
                    ixpix, (int)glist_ytopixels(glist, 
                            basey + fielddesc_cvttocoord(yfielddesc, minyval)) - 1,
                            inextx, py2, symfill->s_name, symoutline->s_name,
                            border, glist_getcanvas(glist), glist, data);
                        */

                    /* For efficiency, we make a single path item for the trace or bargraph */
                    int mex1 = ixpix;
                    int mey1 = (int)glist_ytopixels(glist, basey + fielddesc_cvttocoord(yfielddesc, minyval)) - 1;
                    int mex2 = inextx;
                    int mey2 = py2;
                    sys_vgui("M %d %d H %d V %d H %d z \\\n",
                        mex1, mey1, mex2, mey2, mex1);
               //} //part of experimental code above
                    ndrawn++;
                    minyval = 1e20;
                    maxyval = -1e20;
                }
                if (ndrawn > 2000 || ixpix >= 3000) break;
            }
            /* end of the path item from above */
            sys_vgui("} -fill %s -stroke %s -strokewidth %d -tags {.x%lx.x%lx.template%lx array}\n",
                        symfill->s_name, symoutline->s_name,
                        style == PLOTSTYLE_POINTS ? 0 : 1,
                        glist_getcanvas(glist), glist, data);

        }
        else
        {
            //char outline[20];
            int lastpixel = -1, ndrawn = 0;
            t_float yval = 0, wval = 0, xpix;
            int ixpix = 0;
                /* draw the trace */
            //numbertocolor(fielddesc_getfloat(&x->x_outlinecolor, template,
            //    data, 1), outline);
            if (wonset >= 0)
            {
                    /* found "w" field which controls linewidth.  The trace is
                    a filled polygon with 2n points. */
                sys_vgui(".x%lx.c create ppolygon \\\n",
                    glist_getcanvas(glist));

                for (i = 0, xsum = xloc; i < nelem; i++)
                {
                    if (xonset >= 0)
                        usexloc = xloc + *(t_float *)((elem + elemsize * i)
                            + xonset);
                    else usexloc = xsum, xsum += xinc;
                    if (yonset >= 0)
                        yval = *(t_float *)((elem + elemsize * i) + yonset);
                    else yval = 0;
                    wval = *(t_float *)((elem + elemsize * i) + wonset);
                    xpix = glist_xtopixels(glist,
                        basex + fielddesc_cvttocoord(xfielddesc, usexloc));
                    ixpix = xpix + 0.5;
                    if (xonset >= 0 || ixpix != lastpixel)
                    {
                        sys_vgui("%d %f \\\n", ixpix,
                            glist_ytopixels(glist,
                                basey + fielddesc_cvttocoord(yfielddesc, 
                                    yloc + yval) -
                                        fielddesc_cvttocoord(wfielddesc,wval)));
                        ndrawn++;
                    }
                    lastpixel = ixpix;
                    if (ndrawn >= 1000) goto ouch;
                }
                lastpixel = -1;
                for (i = nelem-1; i >= 0; i--)
                {
                    t_float usexloc;
                    if (xonset >= 0)
                        usexloc = xloc + *(t_float *)((elem + elemsize * i)
                            + xonset);
                    else xsum -= xinc, usexloc = xsum;
                    if (yonset >= 0)
                        yval = *(t_float *)((elem + elemsize * i) + yonset);
                    else yval = 0;
                    wval = *(t_float *)((elem + elemsize * i) + wonset);
                    xpix = glist_xtopixels(glist,
                        basex + fielddesc_cvttocoord(xfielddesc, usexloc));
                    ixpix = xpix + 0.5;
                    if (xonset >= 0 || ixpix != lastpixel)
                    {
                        sys_vgui("%d %f \\\n", ixpix, glist_ytopixels(glist,
                            basey + yloc + fielddesc_cvttocoord(yfielddesc,
                                yval) +
                                    fielddesc_cvttocoord(wfielddesc, wval)));
                        ndrawn++;
                    }
                    lastpixel = ixpix;
                    if (ndrawn >= 1000) goto ouch;
                }
                    /* TK will complain if there aren't at least 3 points.
                    There should be at least two already. */
                if (ndrawn < 4)
                {
                    sys_vgui("%d %f \\\n", ixpix + 10, glist_ytopixels(glist,
                        basey + yloc + fielddesc_cvttocoord(yfielddesc,
                            yval) +
                                fielddesc_cvttocoord(wfielddesc, wval)));
                    sys_vgui("%d %f \\\n", ixpix + 10, glist_ytopixels(glist,
                        basey + yloc + fielddesc_cvttocoord(yfielddesc,
                            yval) -
                                fielddesc_cvttocoord(wfielddesc, wval)));
                }
            ouch:
                sys_vgui(" -strokewidth 1 -fill %s -stroke %s \\\n",
                    symfill->s_name, symoutline->s_name);
                //if (style == PLOTSTYLE_BEZ) sys_vgui("-smooth 1 \\\n"); //this doesn't work with tkpath

                sys_vgui("-tags {.x%lx.x%lx.template%lx scalar%lx}\n", glist_getcanvas(glist), glist,
					 data, sc);
            }
            else if (linewidth > 0)
            {
                    /* no "w" field.  If the linewidth is positive, draw a
                    segmented line with the requested width; otherwise don't
                    draw the trace at all. */
                sys_vgui(".x%lx.c create polyline \\\n", glist_getcanvas(glist));

                for (xsum = xloc, i = 0; i < nelem; i++)
                {
                    t_float usexloc;
                    if (xonset >= 0)
                        usexloc = xloc + *(t_float *)((elem + elemsize * i) +
                            xonset);
                    else usexloc = xsum, xsum += xinc;
                    if (yonset >= 0)
                        yval = *(t_float *)((elem + elemsize * i) + yonset);
                    else yval = 0;
                    xpix = glist_xtopixels(glist,
                        basex + fielddesc_cvttocoord(xfielddesc, usexloc));
                    ixpix = xpix + 0.5;
                    if (xonset >= 0 || ixpix != lastpixel)
                    {
                        sys_vgui("%d %f \\\n", ixpix,
                            glist_ytopixels(glist,
                                basey + yloc + fielddesc_cvttocoord(yfielddesc,
                                    yval)));
                        ndrawn++;
                    }
                    lastpixel = ixpix;
                    if (ndrawn >= 1000) break;
                }
                    /* TK will complain if there aren't at least 2 points... */
                if (ndrawn == 0) sys_vgui("0 0 0 0 \\\n");
                else if (ndrawn == 1) sys_vgui("%d %f \\\n", ixpix + 10,
                    glist_ytopixels(glist, basey + yloc + 
                        fielddesc_cvttocoord(yfielddesc, yval)));

                //sys_vgui("-strokewidth %f \\\n", linewidth);
                //sys_vgui("-fill %s \\\n", outline);
                sys_vgui("-strokewidth %f -stroke %s \\\n", linewidth, symoutline->s_name);
                //sys_vgui("-fill %s \\\n", symoutline->s_name);
                //if (style == PLOTSTYLE_BEZ) sys_vgui("-smooth 1 \\\n"); //this doesn't work with tkpath
 
                sys_vgui("-tags {.x%lx.x%lx.template%lx scalar%lx}\n", glist_getcanvas(glist), glist, data,sc);
            }
        }
        /* make sure the array drawings are behind the graph */
        sys_vgui(".x%lx.c lower plot%lx graph%lx\n", glist_getcanvas(glist),
            data, glist);

            /* We're done with the outline; now draw all the points.
            This code is inefficient since the template has to be
            searched for drawing instructions for every last point. */
        if (scalarvis != 0)
        {
            for (xsum = xloc, i = 0; i < nelem; i++)
            {
                t_float usexloc, useyloc;
                t_gobj *y;
                if (xonset >= 0)
                    usexloc = /* basex */ + xloc +
                        *(t_float *)((elem + elemsize * i) + xonset);
                else usexloc = /* basex */ + xsum, xsum += xinc;
                if (yonset >= 0)
                    yval = *(t_float *)((elem + elemsize * i) + yonset);
                else yval = 0;
                useyloc = /* basey */ + yloc +
                    fielddesc_cvttocoord(yfielddesc, yval);
                for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
                {
                    t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
                    if (!wb) continue;
                    (*wb->w_parentvisfn)(y, glist, sc,
                        (t_word *)(elem + elemsize * i),
                            elemtemplate, usexloc, useyloc, tovis);
                }
            }
        }
		if (!glist_istoplevel(glist)) {
			t_canvas *gl = glist_getcanvas(glist);
			char objtag[64];
			sprintf(objtag, ".x%lx.x%lx.template%lx", (t_int)gl, (t_int)glist, (t_int)data);
			canvas_restore_original_position(gl, (t_gobj *)glist, objtag, -1);
		}
			/*
			sys_vgui(".x%lx.c lower .x%lx.x%lx.plot%lx %s\n", glist_getcanvas(glist), glist_getcanvas(glist), glist, data, rtext_gettag(glist_findrtext(glist_getcanvas(glist), &glist->gl_obj)));
			sys_vgui(".x%lx.c raise .x%lx.x%lx.plot%lx %s\n", glist_getcanvas(glist), glist_getcanvas(glist), glist, data, rtext_gettag(glist_findrtext(glist_getcanvas(glist), &glist->gl_obj)));
		}*/
    }
    else
    {
            /* un-draw the individual points */
        if (scalarvis != 0)
        {
            int i;
            for (i = 0; i < nelem; i++)
            {
                t_gobj *y;
                for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
                {
                    t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
                    if (!wb) continue;
                    (*wb->w_parentvisfn)(y, glist, sc,
                        (t_word *)(elem + elemsize * i), elemtemplate,
                            0, 0, 0);
                }
            }
        }
            /* and then the trace */
        sys_vgui(".x%lx.c delete .x%lx.x%lx.template%lx\n",
            glist_getcanvas(glist), glist_getcanvas(glist), glist, data);      
    }
}

static int plot_click(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, t_scalar *sc, t_array *ap,
    t_float basex, t_float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
	//fprintf(stderr,"plot_click %lx %lx %f %f %d %d\n", (t_int)z, (t_int)glist, basex, basey, xpix, ypix);
    t_plot *x = (t_plot *)z;
    t_symbol *elemtemplatesym;
    t_float linewidth, xloc, xinc, yloc, style, vis, scalarvis;
    t_array *array;
    t_fielddesc *xfielddesc, *yfielddesc, *wfielddesc;
    t_symbol *symfillcolor;
    t_symbol *symoutlinecolor;

    if (!plot_readownertemplate(x, data, template, 
        &elemtemplatesym, &array, &linewidth, &xloc, &xinc, &yloc, &style,
        &vis, &scalarvis,
        &xfielddesc, &yfielddesc, &wfielddesc, &symfillcolor, &symoutlinecolor)
        && (vis != 0))
    {
		//fprintf(stderr,"	->array_doclick\n");
        return (array_doclick(array, glist, sc, ap,
            elemtemplatesym,
            linewidth, basex + xloc, xinc, basey + yloc, scalarvis,
            xfielddesc, yfielddesc, wfielddesc,
            xpix, ypix, shift, alt, dbl, doit));
    }
    else return (0);
}

t_parentwidgetbehavior plot_widgetbehavior =
{
    plot_getrect,
    plot_displace,
    plot_select,
    plot_activate,
    plot_vis,
    plot_click,
};

static void plot_setup(void)
{
    plot_class = class_new(gensym("plot"), (t_newmethod)plot_new, 0,
        sizeof(t_plot), 0, A_GIMME, 0);
    class_setdrawcommand(plot_class);
    class_addfloat(plot_class, plot_float);
    class_setparentwidget(plot_class, &plot_widgetbehavior);
}

/* ---------------- drawnumber: draw a number (or symbol) ---------------- */

/*
    drawnumbers draw numeric fields at controllable locations, with
    controllable color and label.  invocation:
    (drawnumber|drawsymbol) [-v <visible>] variable x y color label
*/

t_class *drawnumber_class;

#define DRAW_SYMBOL 1

typedef struct _drawnumber
{
    t_object x_obj;
    t_fielddesc x_value;
    t_fielddesc x_xloc;
    t_fielddesc x_yloc;
    t_fielddesc x_color;
    t_fielddesc x_vis;
	t_fielddesc x_fontsize;
    t_symbol *x_label;
    int x_flags;
    t_canvas *x_canvas;
} t_drawnumber;

static void *drawnumber_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    t_drawnumber *x = (t_drawnumber *)pd_new(drawnumber_class);
    char *classname = classsym->s_name;
    int flags = 0;
    
    if (classname[4] == 's')
        flags |= DRAW_SYMBOL;
    x->x_flags = flags;
    fielddesc_setfloat_const(&x->x_vis, 1);
    x->x_canvas = canvas_getcurrent();
    while (1)
    {
        t_symbol *firstarg = atom_getsymbolarg(0, argc, argv);
        if (!strcmp(firstarg->s_name, "-v") && argc > 1)
        {
            fielddesc_setfloatarg(&x->x_vis, 1, argv+1);
            argc -= 2; argv += 2;
        }
        else break;
    }
    if (flags & DRAW_SYMBOL)
    {
        if (argc) fielddesc_setsymbolarg(&x->x_value, argc--, argv++);
        else fielddesc_setsymbol_const(&x->x_value, &s_);
    }
    else
    {
        if (argc) fielddesc_setfloatarg(&x->x_value, argc--, argv++);
        else fielddesc_setfloat_const(&x->x_value, 0);
    }
    if (argc) fielddesc_setfloatarg(&x->x_xloc, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_xloc, 0);
    if (argc) fielddesc_setfloatarg(&x->x_yloc, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_yloc, 0);
    if (argc) fielddesc_setfloatarg(&x->x_color, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_color, 1);
	if (argc == 2) fielddesc_setfloatarg(&x->x_fontsize, argc--, argv++);
	else fielddesc_setfloatarg(&x->x_fontsize, 0, NULL);
    if (argc)
        x->x_label = atom_getsymbolarg(0, argc, argv);
    else x->x_label = &s_;

    return (x);
}

void drawnumber_float(t_drawnumber *x, t_floatarg f)
{
    int viswas;
    if (x->x_vis.fd_type != A_FLOAT || x->x_vis.fd_var)
    {
        pd_error(x, "global vis/invis for a template with variable visibility");
        return;
    }
    viswas = (x->x_vis.fd_un.fd_float != 0);
    
    if ((f != 0 && viswas) || (f == 0 && !viswas))
        return;
    canvas_redrawallfortemplatecanvas(x->x_canvas, 2);
    fielddesc_setfloat_const(&x->x_vis, (f != 0));
    canvas_redrawallfortemplatecanvas(x->x_canvas, 1);
}

/* -------------------- widget behavior for drawnumber ------------ */

#define DRAWNUMBER_BUFSIZE 80
static void drawnumber_sprintf(t_drawnumber *x, char *buf, t_atom *ap)
{
    int nchars;
    strncpy(buf, x->x_label->s_name, DRAWNUMBER_BUFSIZE);
    buf[DRAWNUMBER_BUFSIZE - 1] = 0;
    nchars = strlen(buf);
    atom_string(ap, buf + nchars, DRAWNUMBER_BUFSIZE - nchars);
}

static void drawnumber_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_drawnumber *x = (t_drawnumber *)z;
    t_atom at;
        int xloc, yloc, font, fontwidth, fontheight;
    char buf[DRAWNUMBER_BUFSIZE];

    if (!fielddesc_getfloat(&x->x_vis, template, data, 0))
    {
        *xp1 = *yp1 = 0x7fffffff;
        *xp2 = *yp2 = -0x7fffffff;
        return;
    }
    xloc = glist_xtopixels(glist,
        basex + fielddesc_getcoord(&x->x_xloc, template, data, 0));
    yloc = glist_ytopixels(glist,
        basey + fielddesc_getcoord(&x->x_yloc, template, data, 0));
    font = fielddesc_getfloat(&x->x_fontsize, template, data, 0);
	if (!font) font = glist_getfont(glist);
    fontwidth = sys_fontwidth(font);
        fontheight = sys_fontheight(font);
    if (x->x_flags & DRAW_SYMBOL)
        SETSYMBOL(&at, fielddesc_getsymbol(&x->x_value, template, data, 0));
    else SETFLOAT(&at, fielddesc_getfloat(&x->x_value, template, data, 0));
    drawnumber_sprintf(x, buf, &at);
    *xp1 = xloc;
    *yp1 = yloc;
    *xp2 = xloc + fontwidth * strlen(buf);
    *yp2 = yloc + fontheight;
}

static void drawnumber_displace(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int dx, int dy)
{
    /* refuse */
}

static void drawnumber_select(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    //fprintf(stderr,"drawnumber_select %d", state);
    /* fill in later */
}

static void drawnumber_activate(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    //post("drawnumber_activate %d", state);
}

static void drawnumber_vis(t_gobj *z, t_glist *glist, t_scalar *sc,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int vis)
{
	//fprintf(stderr,"drawnumber_vis %d\n", vis);
    t_drawnumber *x = (t_drawnumber *)z;

	/*// get the universal tag for all nested objects
	t_canvas *tag = x->x_canvas;
	while (tag->gl_owner) {
		tag = tag->gl_owner;
	}*/
    
        /* see comment in plot_vis() */
    if (vis && !fielddesc_getfloat(&x->x_vis, template, data, 0))
        return;
    if (vis)
    {
        t_atom at;
		int fontsize = fielddesc_getfloat(&x->x_fontsize, template, data, 0);
		if (!fontsize) fontsize = glist_getfont(glist);
        /*int xloc = glist_xtopixels(glist,
            basex + fielddesc_getcoord(&x->x_xloc, template, data, 0));
        int yloc = glist_ytopixels(glist,
            basey + fielddesc_getcoord(&x->x_yloc, template, data, 0));*/
        int xloc = fielddesc_getcoord(&x->x_xloc, template, data, 0);
        int yloc = fielddesc_getcoord(&x->x_yloc, template, data, 0);

        char colorstring[20], buf[DRAWNUMBER_BUFSIZE];
        numbertocolor(fielddesc_getfloat(&x->x_color, template, data, 1),
            colorstring);
        if (x->x_flags & DRAW_SYMBOL)
            SETSYMBOL(&at, fielddesc_getsymbol(&x->x_value, template, data, 0));
        else SETFLOAT(&at, fielddesc_getfloat(&x->x_value, template, data, 0));
        drawnumber_sprintf(x, buf, &at);
        /*sys_vgui(".x%lx.c create text %d %d -anchor nw -fill %s -text {%s}",
                glist_getcanvas(glist), xloc, yloc, colorstring, buf);
        sys_vgui(" -font {{%s} -%d %s}", sys_font,
				 sys_hostfontsize(fontsize), sys_fontweight);*/
        sys_vgui(".x%lx.c create ptext %d [expr {[font metrics {{%s} %d} -ascent] + %d}] -textanchor start -fill %s -text {%s}\\\n",
                glist_getcanvas(glist), xloc, sys_font, sys_hostfontsize(fontsize), yloc, colorstring, buf);
        /* have to remove fontweight for the time being... */
        sys_vgui(" -fontfamily {%s} -fontsize %d", sys_font,
                sys_hostfontsize(fontsize));
        sys_vgui(" -parent .scalar%lx", data);
        sys_vgui(" -tags {.x%lx.x%lx.template%lx scalar%lx}\n", 
			glist_getcanvas(glist), glist, data, sc);
    }
    else sys_vgui(".x%lx.c delete .x%lx.x%lx.template%lx\n", glist_getcanvas(glist), 
		glist_getcanvas(glist), glist, data);
}

static t_float drawnumber_motion_ycumulative;
static t_glist *drawnumber_motion_glist;
static t_scalar *drawnumber_motion_scalar;
static t_array *drawnumber_motion_array;
static t_word *drawnumber_motion_wp;
static t_template *drawnumber_motion_template;
static t_gpointer drawnumber_motion_gpointer;
static int drawnumber_motion_symbol;
static int drawnumber_motion_firstkey;

    /* LATER protect against the template changing or the scalar disappearing
    probably by attaching a gpointer here ... */

static void drawnumber_motion(void *z, t_floatarg dx, t_floatarg dy)
{
    t_drawnumber *x = (t_drawnumber *)z;
    t_fielddesc *f = &x->x_value;
    t_atom at;
    if (!gpointer_check(&drawnumber_motion_gpointer, 0))
    {
        post("drawnumber_motion: scalar disappeared");
        return;
    }
    if (drawnumber_motion_symbol)
    {
        post("drawnumber_motion: symbol");
        return;
    }
    drawnumber_motion_ycumulative -= dy;
    template_setfloat(drawnumber_motion_template,
        f->fd_un.fd_varsym,
            drawnumber_motion_wp, 
            drawnumber_motion_ycumulative,
                1);
    if (drawnumber_motion_scalar)
        template_notifyforscalar(drawnumber_motion_template,
            drawnumber_motion_glist, drawnumber_motion_scalar,
                gensym("change"), 1, &at);

    if (drawnumber_motion_scalar)
        scalar_redraw(drawnumber_motion_scalar, drawnumber_motion_glist);
    if (drawnumber_motion_array)
        array_redraw(drawnumber_motion_array, drawnumber_motion_glist);
}

static void drawnumber_key(void *z, t_floatarg fkey)
{
    t_drawnumber *x = (t_drawnumber *)z;
    t_fielddesc *f = &x->x_value;
    int key = fkey;
    char sbuf[MAXPDSTRING];
    t_atom at;
    if (!gpointer_check(&drawnumber_motion_gpointer, 0))
    {
        post("drawnumber_motion: scalar disappeared");
        return;
    }
    if (key == 0)
        return;
    if (drawnumber_motion_symbol)
    {
            /* key entry for a symbol field */
        if (drawnumber_motion_firstkey)
            sbuf[0] = 0;
        else strncpy(sbuf, template_getsymbol(drawnumber_motion_template,
            f->fd_un.fd_varsym, drawnumber_motion_wp, 1)->s_name,
                MAXPDSTRING);
        sbuf[MAXPDSTRING-1] = 0;
        if (key == '\b')
        {
            if (*sbuf)
                sbuf[strlen(sbuf)-1] = 0;
        }
        else
        {
            sbuf[strlen(sbuf)+1] = 0;
            sbuf[strlen(sbuf)] = key;
        }
    }
    else
    {
            /* key entry for a numeric field.  This is just a stopgap. */
        float newf;
        if (drawnumber_motion_firstkey)
            sbuf[0] = 0;
        else sprintf(sbuf, "%g", template_getfloat(drawnumber_motion_template,
            f->fd_un.fd_varsym, drawnumber_motion_wp, 1));
        drawnumber_motion_firstkey = (key == '\n');
        if (key == '\b')
        {
            if (*sbuf)
                sbuf[strlen(sbuf)-1] = 0;
        }
        else
        {
            sbuf[strlen(sbuf)+1] = 0;
            sbuf[strlen(sbuf)] = key;
        }
        if (sscanf(sbuf, "%g", &newf) < 1)
            newf = 0;
        template_setfloat(drawnumber_motion_template,
            f->fd_un.fd_varsym, drawnumber_motion_wp, newf, 1);
        if (drawnumber_motion_scalar)
            template_notifyforscalar(drawnumber_motion_template,
                drawnumber_motion_glist, drawnumber_motion_scalar,
                    gensym("change"), 1, &at);
        if (drawnumber_motion_scalar)
            scalar_redraw(drawnumber_motion_scalar, drawnumber_motion_glist);
        if (drawnumber_motion_array)
            array_redraw(drawnumber_motion_array, drawnumber_motion_glist);
    }
}

static int drawnumber_click(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, t_scalar *sc, t_array *ap,
    t_float basex, t_float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_drawnumber *x = (t_drawnumber *)z;
    int x1, y1, x2, y2;
    drawnumber_getrect(z, glist,
        data, template, basex, basey,
        &x1, &y1, &x2, &y2);
    if (xpix >= x1 && xpix <= x2 && ypix >= y1 && ypix <= y2
        && x->x_value.fd_var &&
            fielddesc_getfloat(&x->x_vis, template, data, 0))
    {
        if (doit)
        {
            drawnumber_motion_glist = glist;
            drawnumber_motion_wp = data;
            drawnumber_motion_template = template;
            drawnumber_motion_scalar = sc;
            drawnumber_motion_array = ap;
            drawnumber_motion_firstkey = 1;
            drawnumber_motion_ycumulative =
                fielddesc_getfloat(&x->x_value, template, data, 0);
            drawnumber_motion_symbol = ((x->x_flags & DRAW_SYMBOL) != 0);
            if (drawnumber_motion_scalar)
                gpointer_setglist(&drawnumber_motion_gpointer, 
                    drawnumber_motion_glist, drawnumber_motion_scalar);
            else gpointer_setarray(&drawnumber_motion_gpointer,
                    drawnumber_motion_array, drawnumber_motion_wp);
           glist_grab(glist, z, drawnumber_motion, drawnumber_key,
                xpix, ypix);
        }
        return (1);
    }
    else return (0);
}

t_parentwidgetbehavior drawnumber_widgetbehavior =
{
    drawnumber_getrect,
    drawnumber_displace,
    drawnumber_select,
    drawnumber_activate,
    drawnumber_vis,
    drawnumber_click,
};

static void drawnumber_free(t_drawnumber *x)
{
}

static void drawnumber_setup(void)
{
    drawnumber_class = class_new(gensym("drawnumber"),
        (t_newmethod)drawnumber_new, (t_method)drawnumber_free,
        sizeof(t_drawnumber), 0, A_GIMME, 0);
    class_setdrawcommand(drawnumber_class);
    class_addfloat(drawnumber_class, drawnumber_float);
    class_setparentwidget(drawnumber_class, &drawnumber_widgetbehavior);
}

/* ---------------------- drawsymbol -------------------------------- */

t_class *drawsymbol_class;

typedef struct _drawsymbol
{
    t_object x_obj;
    t_fielddesc x_value;
    t_fielddesc x_xloc;
    t_fielddesc x_yloc;
    t_fielddesc x_color;
    t_fielddesc x_vis;
	t_fielddesc x_fontsize;
    t_symbol *x_label;
    int x_flags;
    t_canvas *x_canvas;
} t_drawsymbol;

static void *drawsymbol_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    t_drawsymbol *x = (t_drawsymbol *)pd_new(drawsymbol_class);
    char *classname = classsym->s_name;
    int flags = 0;
    
    if (classname[4] == 's')
        flags |= DRAW_SYMBOL;
    x->x_flags = flags;
    fielddesc_setfloat_const(&x->x_vis, 1);
    x->x_canvas = canvas_getcurrent();
    while (1)
    {
        t_symbol *firstarg = atom_getsymbolarg(0, argc, argv);
        if (!strcmp(firstarg->s_name, "-v") && argc > 1)
        {
            fielddesc_setfloatarg(&x->x_vis, 1, argv+1);
            argc -= 2; argv += 2;
        }
        else break;
    }
    if (flags & DRAW_SYMBOL)
    {
        if (argc) fielddesc_setsymbolarg(&x->x_value, argc--, argv++);
        else fielddesc_setsymbol_const(&x->x_value, &s_);
    }
    else
    {
        if (argc) fielddesc_setfloatarg(&x->x_value, argc--, argv++);
        else fielddesc_setfloat_const(&x->x_value, 0);
    }
    if (argc) fielddesc_setfloatarg(&x->x_xloc, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_xloc, 0);
    if (argc) fielddesc_setfloatarg(&x->x_yloc, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_yloc, 0);
    if (argc) fielddesc_setfloatarg(&x->x_color, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_color, 1);
	if (argc == 2) fielddesc_setfloatarg(&x->x_fontsize, argc--, argv++);
	else fielddesc_setfloatarg(&x->x_fontsize, 0, NULL);
    if (argc)
        x->x_label = atom_getsymbolarg(0, argc, argv);
    else x->x_label = &s_;

    return (x);
}

void drawsymbol_float(t_drawsymbol *x, t_floatarg f)
{
    int viswas;
    if (x->x_vis.fd_type != A_FLOAT || x->x_vis.fd_var)
    {
        pd_error(x, "global vis/invis for a template with variable visibility");
        return;
    }
    viswas = (x->x_vis.fd_un.fd_float != 0);
    
    if ((f != 0 && viswas) || (f == 0 && !viswas))
        return;
    canvas_redrawallfortemplatecanvas(x->x_canvas, 2);
    fielddesc_setfloat_const(&x->x_vis, (f != 0));
    canvas_redrawallfortemplatecanvas(x->x_canvas, 1);
}

/* -------------------- widget behavior for drawsymbol ------------ */

#define DRAWSYMBOL_BUFSIZE 80
static void drawsymbol_sprintf(t_drawsymbol *x, char *buf, t_atom *ap)
{
    //int nchars;
    //strncpy(buf, x->x_label->s_name, DRAWSYMBOL_BUFSIZE);
    //buf[DRAWSYMBOL_BUFSIZE - 1] = 0;
    //nchars = strlen(buf);
    atom_string(ap, buf, DRAWSYMBOL_BUFSIZE);
}

static void drawsymbol_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_drawsymbol *x = (t_drawsymbol *)z;
    t_atom at;
        int xloc, yloc, font, fontwidth, fontheight;
    char buf[DRAWSYMBOL_BUFSIZE];

    if (!fielddesc_getfloat(&x->x_vis, template, data, 0))
    {
        *xp1 = *yp1 = 0x7fffffff;
        *xp2 = *yp2 = -0x7fffffff;
        return;
    }
    xloc = glist_xtopixels(glist,
        basex + fielddesc_getcoord(&x->x_xloc, template, data, 0));
    yloc = glist_ytopixels(glist,
        basey + fielddesc_getcoord(&x->x_yloc, template, data, 0));
    font = fielddesc_getfloat(&x->x_fontsize, template, data, 0);
	if (!font) font = glist_getfont(glist);
    fontwidth = sys_fontwidth(font);
        fontheight = sys_fontheight(font);
    if (x->x_flags & DRAW_SYMBOL)
        SETSYMBOL(&at, fielddesc_getsymbol(&x->x_value, template, data, 0));
    else SETFLOAT(&at, fielddesc_getfloat(&x->x_value, template, data, 0));
    drawsymbol_sprintf(x, buf, &at);
    *xp1 = xloc;
    *yp1 = yloc;
    *xp2 = xloc + fontwidth * strlen(buf);
    *yp2 = yloc + fontheight;
}

static void drawsymbol_displace(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int dx, int dy)
{
    /* refuse */
}

static void drawsymbol_select(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    //fprintf(stderr,"drawsymbol_select %d", state);
    /* fill in later */
}

static void drawsymbol_activate(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    //post("drawsymbol_activate %d", state);
}

static void drawsymbol_vis(t_gobj *z, t_glist *glist, t_scalar *sc,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int vis)
{
    t_drawsymbol *x = (t_drawsymbol *)z;

	/*// get the universal tag for all nested objects
	t_canvas *tag = x->x_canvas;
	while (tag->gl_owner) {
		tag = tag->gl_owner;
	}*/
    
        /* see comment in plot_vis() */
    if (vis && !fielddesc_getfloat(&x->x_vis, template, data, 0))
        return;
    if (vis)
    {
        t_atom at;
		int fontsize = fielddesc_getfloat(&x->x_fontsize, template, data, 0);
		if (!fontsize) fontsize = glist_getfont(glist);
        /*int xloc = glist_xtopixels(glist,
            basex + fielddesc_getcoord(&x->x_xloc, template, data, 0));
        int yloc = glist_ytopixels(glist,
            basey + fielddesc_getcoord(&x->x_yloc, template, data, 0));*/
        int xloc = fielddesc_getcoord(&x->x_xloc, template, data, 0);
        int yloc = fielddesc_getcoord(&x->x_yloc, template, data, 0);

        char colorstring[20], buf[DRAWSYMBOL_BUFSIZE];
        numbertocolor(fielddesc_getfloat(&x->x_color, template, data, 1),
            colorstring);
        if (x->x_flags & DRAW_SYMBOL)
            SETSYMBOL(&at, fielddesc_getsymbol(&x->x_value, template, data, 0));
        else SETFLOAT(&at, fielddesc_getfloat(&x->x_value, template, data, 0));
        drawsymbol_sprintf(x, buf, &at);
        /*sys_vgui(".x%lx.c create text %d %d -anchor nw -fill %s -text {%s}",
                glist_getcanvas(glist), xloc, yloc, colorstring, buf);
        sys_vgui(" -font {{%s} -%d %s}", sys_font,
				 sys_hostfontsize(fontsize), sys_fontweight);*/
        sys_vgui(".x%lx.c create ptext %d [expr {[font metrics {{%s} %d} -ascent] + %d}] -textanchor start -fill %s -text {%s}\\\n",
                glist_getcanvas(glist), xloc, sys_font, sys_hostfontsize(fontsize), yloc, colorstring, buf);
        sys_vgui(" -fontfamily {%s} -fontsize %d ", sys_font,
                sys_hostfontsize(fontsize));
        sys_vgui(" -parent .scalar%lx", data);
        sys_vgui(" -tags {.x%lx.x%lx.template%lx scalar%lx}\n", 
			glist_getcanvas(glist), glist, data, sc);
    }
    else sys_vgui(".x%lx.c delete .x%lx.x%lx.template%lx\n", glist_getcanvas(glist), 
		glist_getcanvas(glist), glist, data);
}

static t_float drawsymbol_motion_ycumulative;
static t_glist *drawsymbol_motion_glist;
static t_scalar *drawsymbol_motion_scalar;
static t_array *drawsymbol_motion_array;
static t_word *drawsymbol_motion_wp;
static t_template *drawsymbol_motion_template;
static t_gpointer drawsymbol_motion_gpointer;
static int drawsymbol_motion_symbol;
static int drawsymbol_motion_firstkey;

    /* LATER protect against the template changing or the scalar disappearing
    probably by attaching a gpointer here ... */

static void drawsymbol_motion(void *z, t_floatarg dx, t_floatarg dy)
{
    t_drawsymbol *x = (t_drawsymbol *)z;
    t_fielddesc *f = &x->x_value;
    t_atom at;
    if (!gpointer_check(&drawsymbol_motion_gpointer, 0))
    {
        post("drawsymbol_motion: scalar disappeared");
        return;
    }
    if (drawsymbol_motion_symbol)
    {
        post("drawsymbol_motion: symbol");
        return;
    }
    drawsymbol_motion_ycumulative -= dy;
    template_setfloat(drawsymbol_motion_template,
        f->fd_un.fd_varsym,
            drawsymbol_motion_wp, 
            drawsymbol_motion_ycumulative,
                1);
    if (drawsymbol_motion_scalar)
        template_notifyforscalar(drawsymbol_motion_template,
            drawsymbol_motion_glist, drawsymbol_motion_scalar,
                gensym("change"), 1, &at);

    if (drawsymbol_motion_scalar)
        scalar_redraw(drawsymbol_motion_scalar, drawsymbol_motion_glist);
    if (drawsymbol_motion_array)
        array_redraw(drawsymbol_motion_array, drawsymbol_motion_glist);
}

static void drawsymbol_key(void *z, t_floatarg fkey)
{
    t_drawsymbol *x = (t_drawsymbol *)z;
    t_fielddesc *f = &x->x_value;
    int key = fkey;
    char sbuf[MAXPDSTRING];
    t_atom at;
    if (!gpointer_check(&drawsymbol_motion_gpointer, 0))
    {
        post("drawsymbol_motion: scalar disappeared");
        return;
    }
    if (key == 0)
        return;
    if (drawsymbol_motion_symbol)
    {
            /* key entry for a symbol field */
        if (drawsymbol_motion_firstkey)
            sbuf[0] = 0;
        else strncpy(sbuf, template_getsymbol(drawsymbol_motion_template,
            f->fd_un.fd_varsym, drawsymbol_motion_wp, 1)->s_name,
                MAXPDSTRING);
        sbuf[MAXPDSTRING-1] = 0;
        if (key == '\b')
        {
            if (*sbuf)
                sbuf[strlen(sbuf)-1] = 0;
        }
        else
        {
            sbuf[strlen(sbuf)+1] = 0;
            sbuf[strlen(sbuf)] = key;
        }
    }
    else
    {
            /* key entry for a numeric field.  This is just a stopgap. */
        float newf;
        if (drawsymbol_motion_firstkey)
            sbuf[0] = 0;
        else sprintf(sbuf, "%g", template_getfloat(drawsymbol_motion_template,
            f->fd_un.fd_varsym, drawsymbol_motion_wp, 1));
        drawsymbol_motion_firstkey = (key == '\n');
        if (key == '\b')
        {
            if (*sbuf)
                sbuf[strlen(sbuf)-1] = 0;
        }
        else
        {
            sbuf[strlen(sbuf)+1] = 0;
            sbuf[strlen(sbuf)] = key;
        }
        if (sscanf(sbuf, "%g", &newf) < 1)
            newf = 0;
        template_setfloat(drawsymbol_motion_template,
            f->fd_un.fd_varsym, drawsymbol_motion_wp, newf, 1);
        if (drawsymbol_motion_scalar)
            template_notifyforscalar(drawsymbol_motion_template,
                drawsymbol_motion_glist, drawsymbol_motion_scalar,
                    gensym("change"), 1, &at);
        if (drawsymbol_motion_scalar)
            scalar_redraw(drawsymbol_motion_scalar, drawsymbol_motion_glist);
        if (drawsymbol_motion_array)
            array_redraw(drawsymbol_motion_array, drawsymbol_motion_glist);
    }
}

static int drawsymbol_click(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, t_scalar *sc, t_array *ap,
    t_float basex, t_float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_drawsymbol *x = (t_drawsymbol *)z;
    int x1, y1, x2, y2;
    drawsymbol_getrect(z, glist,
        data, template, basex, basey,
        &x1, &y1, &x2, &y2);
    if (xpix >= x1 && xpix <= x2 && ypix >= y1 && ypix <= y2
        && x->x_value.fd_var &&
            fielddesc_getfloat(&x->x_vis, template, data, 0))
    {
        if (doit)
        {
            drawsymbol_motion_glist = glist;
            drawsymbol_motion_wp = data;
            drawsymbol_motion_template = template;
            drawsymbol_motion_scalar = sc;
            drawsymbol_motion_array = ap;
            drawsymbol_motion_firstkey = 1;
            drawsymbol_motion_ycumulative =
                fielddesc_getfloat(&x->x_value, template, data, 0);
            drawsymbol_motion_symbol = ((x->x_flags & DRAW_SYMBOL) != 0);
            if (drawsymbol_motion_scalar)
                gpointer_setglist(&drawsymbol_motion_gpointer, 
                    drawsymbol_motion_glist, drawsymbol_motion_scalar);
            else gpointer_setarray(&drawsymbol_motion_gpointer,
                    drawsymbol_motion_array, drawsymbol_motion_wp);
           glist_grab(glist, z, drawsymbol_motion, drawsymbol_key,
                xpix, ypix);
        }
        return (1);
    }
    else return (0);
}

t_parentwidgetbehavior drawsymbol_widgetbehavior =
{
    drawsymbol_getrect,
    drawsymbol_displace,
    drawsymbol_select,
    drawsymbol_activate,
    drawsymbol_vis,
    drawsymbol_click,
};

static void drawsymbol_free(t_drawsymbol *x)
{
}

static void drawsymbol_setup(void)
{
    drawsymbol_class = class_new(gensym("drawsymbol"),
        (t_newmethod)drawsymbol_new, (t_method)drawsymbol_free,
        sizeof(t_drawsymbol), 0, A_GIMME, 0);
    class_setdrawcommand(drawsymbol_class);
    class_addfloat(drawsymbol_class, drawsymbol_float);
    class_setparentwidget(drawsymbol_class, &drawsymbol_widgetbehavior);
}

/* ---------------- drawimage: draw an image ---------------- */
/* ---------------- drawsprite: draw a sprite ---------------- */

/*
    drawimage draws an image (gif) at controllable locations.
    invocation:
    (drawimage|drawsprite) [-v <visible>] variable x y directory
*/

t_class *drawimage_class;

#define DRAW_SPRITE 1

typedef struct _drawimage
{
    t_object x_obj;
    t_fielddesc x_value;
    t_fielddesc x_xloc;
    t_fielddesc x_yloc;
    t_fielddesc x_vis;
    t_symbol *x_img;
    t_float x_w;
    t_float x_h;
    int x_flags;
    int x_deleteme;
    t_canvas *x_canvas;
} t_drawimage;

static void *drawimage_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    t_drawimage *x = (t_drawimage *)pd_new(drawimage_class);
    x->x_deleteme = 0;
    char *classname = classsym->s_name;
    char buf[50];
    sprintf(buf, ".x%lx", (t_int)x);
    pd_bind(&x->x_obj.ob_pd, gensym(buf));
    int flags = 0;
    
    if (classname[4] == 's')
        flags |= DRAW_SPRITE;
    x->x_flags = flags;
    fielddesc_setfloat_const(&x->x_vis, 1);
    x->x_canvas = canvas_getcurrent();
    t_symbol *dir = canvas_getdir(x->x_canvas);
    while (1)
    {
        t_symbol *firstarg = atom_getsymbolarg(0, argc, argv);
        if (!strcmp(firstarg->s_name, "-v") && argc > 1)
        {
            fielddesc_setfloatarg(&x->x_vis, 1, argv+1);
            argc -= 2; argv += 2;
        }
        else break;
    }
    if (argc && argv->a_type == A_SYMBOL)
        x->x_img = atom_getsymbolarg(0, argc--, argv++);
    else x->x_img = &s_;
    if (argc) fielddesc_setfloatarg(&x->x_xloc, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_xloc, 0);
    if (argc) fielddesc_setfloatarg(&x->x_yloc, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_yloc, 0);
    if (argc)
    {
        fielddesc_setfloatarg(&x->x_value, argc--, argv++);
        if (!(x->x_flags & DRAW_SPRITE))
            post("drawimage warning: sequence variable is only "
                 "used with drawsprite");
    }
    else fielddesc_setfloat_const(&x->x_value, 0);

    /* [drawimage] allocates memory for an image or image sequence
       while the object is creating. The corresponding scalar gets
       drawn as a canvas image item using the "parent" tk image as
       the source. ".x%lx" is the name for the parent tk image and
       ".x%lx.i" is the tag given to a scalar's canvas image item.
    */
    sys_vgui("pdtk_drawimage_new .x%lx {%s} {%s} %d\n", (t_int)x,
        x->x_img->s_name, dir->s_name, x->x_flags);
    post("deleteme is %d", x->x_deleteme);
    return (x);
}

void drawimage_float(t_drawimage *x, t_floatarg f)
{
    int viswas;
    if (x->x_vis.fd_type != A_FLOAT || x->x_vis.fd_var)
    {
        pd_error(x, "global vis/invis for a template with variable visibility");
        return;
    }
    viswas = (x->x_vis.fd_un.fd_float != 0);
    
    if ((f != 0 && viswas) || (f == 0 && !viswas))
        return;
    canvas_redrawallfortemplatecanvas(x->x_canvas, 2);
    fielddesc_setfloat_const(&x->x_vis, (f != 0));
    canvas_redrawallfortemplatecanvas(x->x_canvas, 1);
}

void drawimage_size(t_drawimage *x, t_float w, t_float h)
{
    x->x_w = w;
    x->x_h = h;
}

/* -------------------- widget behavior for drawimage ------------ */


/*
static void drawimage_sprintf(t_drawimage *x, char *buf, t_atom *ap)
{
    int nchars;
    strncpy(buf, x->x_label->s_name, MAXPDSTRING);
    buf[DRAWNUMBER_BUFSIZE - 1] = 0;
    nchars = strlen(buf);
    atom_string(ap, buf + nchars, DRAWNUMBER_BUFSIZE - nchars);
}
*/


static void drawimage_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_drawimage *x = (t_drawimage *)z;
    int xloc, yloc;
//    char buf[DRAWNUMBER_BUFSIZE];

    if (!fielddesc_getfloat(&x->x_vis, template, data, 0))
    {
        *xp1 = *yp1 = 0x7fffffff;
        *xp2 = *yp2 = -0x7fffffff;
        return;
    }
    xloc = glist_xtopixels(glist,
        basex + fielddesc_getcoord(&x->x_xloc, template, data, 0));
    yloc = glist_ytopixels(glist,
        basey + fielddesc_getcoord(&x->x_yloc, template, data, 0));
    *xp1 = xloc;
    *yp1 = yloc;

    *xp2 = xloc + x->x_w;
    *yp2 = yloc + x->x_h;
}

static void drawimage_displace(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int dx, int dy)
{
    /* refuse */
}

static void drawimage_select(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    //fprintf(stderr,"drawimage_select %d", state);
    /* fill in later */
}

static void drawimage_activate(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    //post("drawimage_activate %d", state);
}

static void drawimage_vis(t_gobj *z, t_glist *glist, t_scalar *sc, 
    t_word *data, t_template *template, t_float basex, t_float basey,
    int vis)
{
    t_drawimage *x = (t_drawimage *)z;
    
        /* see comment in plot_vis() */
    if (vis && !fielddesc_getfloat(&x->x_vis, template, data, 0))
        return;
    if (vis)
    {
        t_atom at;
        /*int xloc = glist_xtopixels(glist,
            basex + fielddesc_getcoord(&x->x_xloc, template, data, 0));
        int yloc = glist_ytopixels(glist,
            basey + fielddesc_getcoord(&x->x_yloc, template, data, 0));
        sys_vgui("pdtk_drawimage_vis .x%lx.c %d %d .x%lx .x%lx.i %d ",*/
        int xloc = fielddesc_getcoord(&x->x_xloc, template, data, 0);
        int yloc = fielddesc_getcoord(&x->x_yloc, template, data, 0);
        sys_vgui("pdtk_drawimage_vis .x%lx.c %d %d .x%lx .x%lx.i %d \\\n",
            glist_getcanvas(glist), xloc, yloc, x, data,
            (int)fielddesc_getfloat(&x->x_value, template, data, 0));
        //sys_vgui(".x%lx.x%lx.template%lx scalar%lx\n", glist_getcanvas(glist),
        //    glist, data, sc);
        sys_vgui(".x%lx.x%lx.template%lx scalar%lx .scalar%lx\n", glist_getcanvas(glist),
            glist, data, sc, data);
    }
    else sys_vgui("pdtk_drawimage_unvis .x%lx.c .x%lx.i\n",
        glist_getcanvas(glist), data);
}

static t_float drawimage_motion_ycumulative;
static t_glist *drawimage_motion_glist;
static t_scalar *drawimage_motion_scalar;
static t_array *drawimage_motion_array;
static t_word *drawimage_motion_wp;
static t_template *drawimage_motion_template;
static t_gpointer drawimage_motion_gpointer;
static int drawimage_motion_sprite;
static int drawimage_motion_firstkey;

    /* LATER protect against the template changing or the scalar disappearing
    probably by attaching a gpointer here ... */

static void drawimage_motion(void *z, t_floatarg dx, t_floatarg dy)
{
    t_drawimage *x = (t_drawimage *)z;
    t_fielddesc *f = &x->x_value;
    t_atom at;
    if (!gpointer_check(&drawimage_motion_gpointer, 0))
    {
        post("drawimage_motion: scalar disappeared");
        return;
    }
    if (!drawimage_motion_sprite)
    {
        /* post("drawimage_motion: image"); */
        return;
    }
    drawimage_motion_ycumulative -= dy;
    template_setfloat(drawimage_motion_template,
        f->fd_un.fd_varsym,
            drawimage_motion_wp, 
            drawimage_motion_ycumulative,
                1);
    if (drawimage_motion_scalar)
        template_notifyforscalar(drawimage_motion_template,
            drawimage_motion_glist, drawimage_motion_scalar,
                gensym("change"), 1, &at);

    if (drawimage_motion_scalar)
        scalar_redraw(drawimage_motion_scalar, drawimage_motion_glist);
    if (drawimage_motion_array)
        array_redraw(drawimage_motion_array, drawimage_motion_glist);
}

static void drawimage_key(void *z, t_floatarg fkey)
{
    return;
    t_drawnumber *x = (t_drawnumber *)z;
    t_fielddesc *f = &x->x_value;
    int key = fkey;
    char sbuf[MAXPDSTRING];
    t_atom at;
    if (!gpointer_check(&drawnumber_motion_gpointer, 0))
    {
        post("drawnumber_motion: scalar disappeared");
        return;
    }
    if (key == 0)
        return;
    if (drawnumber_motion_symbol)
    {
            /* key entry for a symbol field */
        if (drawnumber_motion_firstkey)
            sbuf[0] = 0;
        else strncpy(sbuf, template_getsymbol(drawnumber_motion_template,
            f->fd_un.fd_varsym, drawnumber_motion_wp, 1)->s_name,
                MAXPDSTRING);
        sbuf[MAXPDSTRING-1] = 0;
        if (key == '\b')
        {
            if (*sbuf)
                sbuf[strlen(sbuf)-1] = 0;
        }
        else
        {
            sbuf[strlen(sbuf)+1] = 0;
            sbuf[strlen(sbuf)] = key;
        }
    }
    else
    {
            /* key entry for a numeric field.  This is just a stopgap. */
        double newf;
        if (drawnumber_motion_firstkey)
            sbuf[0] = 0;
        else sprintf(sbuf, "%g", template_getfloat(drawnumber_motion_template,
            f->fd_un.fd_varsym, drawnumber_motion_wp, 1));
        drawnumber_motion_firstkey = (key == '\n');
        if (key == '\b')
        {
            if (*sbuf)
                sbuf[strlen(sbuf)-1] = 0;
        }
        else
        {
            sbuf[strlen(sbuf)+1] = 0;
            sbuf[strlen(sbuf)] = key;
        }
        if (sscanf(sbuf, "%lg", &newf) < 1)
            newf = 0;
        template_setfloat(drawnumber_motion_template,
            f->fd_un.fd_varsym, drawnumber_motion_wp, (t_float)newf, 1);
        if (drawnumber_motion_scalar)
            template_notifyforscalar(drawnumber_motion_template,
                drawnumber_motion_glist, drawnumber_motion_scalar,
                    gensym("change"), 1, &at);
        if (drawnumber_motion_scalar)
            scalar_redraw(drawnumber_motion_scalar, drawnumber_motion_glist);
        if (drawnumber_motion_array)
            array_redraw(drawnumber_motion_array, drawnumber_motion_glist);
    }
}

static int drawimage_click(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, t_scalar *sc, t_array *ap,
    t_float basex, t_float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_drawimage *x = (t_drawimage *)z;
    int x1, y1, x2, y2;
    drawimage_getrect(z, glist,
        data, template, basex, basey,
        &x1, &y1, &x2, &y2);
    if (xpix >= x1 && xpix <= x2 && ypix >= y1 && ypix <= y2
        && x->x_value.fd_var &&
            fielddesc_getfloat(&x->x_vis, template, data, 0))
    {
        if (doit)
        {
            drawimage_motion_glist = glist;
            drawimage_motion_wp = data;
            drawimage_motion_template = template;
            drawimage_motion_scalar = sc;
            drawimage_motion_array = ap;
            drawimage_motion_firstkey = 1;
            drawimage_motion_ycumulative =
                fielddesc_getfloat(&x->x_value, template, data, 0);
            drawimage_motion_sprite = ((x->x_flags & DRAW_SPRITE) != 0);
            if (drawimage_motion_scalar)
                gpointer_setglist(&drawimage_motion_gpointer, 
                    drawimage_motion_glist, drawimage_motion_scalar);
            else gpointer_setarray(&drawimage_motion_gpointer,
                    drawimage_motion_array, drawimage_motion_wp);
           glist_grab(glist, z, drawimage_motion, drawimage_key,
                xpix, ypix);
        }
        return (1);
    }
    else return (0);
}

t_parentwidgetbehavior drawimage_widgetbehavior =
{
    drawimage_getrect,
    drawimage_displace,
    drawimage_select,
    drawimage_activate,
    drawimage_vis,
    drawimage_click,
};

static void drawimage_free(t_drawimage *x)
{
    /* delete the parent image in the gui */
    char buf[50];
    //sprintf(buf, ".x%lx", (t_int)x);
    sprintf(buf, ".x%lx", (long unsigned int)x);
    pd_unbind(&x->x_obj.ob_pd, gensym(buf));
    sys_vgui("pdtk_drawimage_free .x%lx\n", (t_int)x);
}

static void drawimage_setup(void)
{
    drawimage_class = class_new(gensym("drawimage"),
        (t_newmethod)drawimage_new, (t_method)drawimage_free,
        sizeof(t_drawimage), 0, A_GIMME, 0);
    class_setdrawcommand(drawimage_class);
    class_addfloat(drawimage_class, drawimage_float);
    class_addmethod(drawimage_class, (t_method)drawimage_size,
        gensym("size"), A_FLOAT, A_FLOAT, 0);
    class_addcreator((t_newmethod)drawimage_new, gensym("drawsprite"),
        A_GIMME, 0);
    class_setparentwidget(drawimage_class, &drawimage_widgetbehavior);
}

/* ------------- convenience functions for all drawcommands --------------*/

/* works for [draw] and old style curves, drawnumber, etc. */
t_template *template_findbydrawcommand(t_gobj *g)
{
    t_canvas *c;
    if (g->g_pd == draw_class)
        c = ((t_draw *)g)->x_canvas;
    else if (g->g_pd == curve_class)
        c = ((t_curve *)g)->x_canvas;
    else if (g->g_pd == drawnumber_class)
        c = ((t_drawnumber *)g)->x_canvas;
    else if (g->g_pd == drawsymbol_class)
        c = ((t_drawsymbol *)g)->x_canvas;
    else if (g->g_pd == drawimage_class)
        c = ((t_drawimage *)g)->x_canvas;
    else return (0);
    t_template *tmpl;
    t_symbol *s1 = gensym("struct");
    for (g = c->gl_list; g; g = g->g_next)
    {
        t_object *ob = pd_checkobject(&g->g_pd);
        t_atom *argv;
        if (!ob || ob->te_type != T_OBJECT ||
            binbuf_getnatom(ob->te_binbuf) < 2)
            continue;
        argv = binbuf_getvec(ob->te_binbuf);
        if (argv[0].a_type != A_SYMBOL || argv[1].a_type != A_SYMBOL
            || argv[0].a_w.w_symbol != s1)
                continue;
        return (template_findbyname(canvas_makebindsym(argv[1].a_w.w_symbol)));
    }
    return (0);
}

/* ---------------------- setup function ---------------------------- */

void g_template_setup(void)
{
    template_setup();
    gtemplate_setup();
    curve_setup();
    draw_setup();
    plot_setup();
    drawnumber_setup();
    drawsymbol_setup();
    drawimage_setup();
}

