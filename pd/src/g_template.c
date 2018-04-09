/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>  /* for path bbox calculations */

#include "m_pd.h"
#include "m_imp.h"
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
t_canvas *canvas_templatecanvas_forgroup(t_canvas *c);
static int drawimage_getindex(void *z, t_template *template, t_word *data);

/* ---------------------- storage ------------------------- */

t_class *gtemplate_class;
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
                ds1->ds_fieldtemplate == ds2->ds_fieldtemplate));
}

/* -- templates, the active ingredient in gtemplates defined below. ------- */

t_template *template_new(t_symbol *templatesym, int argc, t_atom *argv)
{
    t_template *x = (t_template *)pd_new(template_class);
    //fprintf(stderr,"template_new %lx\n", x);
    x->t_n = 0;
    x->t_transformable = 0;
    x->t_vec = (t_dataslot *)t_getbytes(0);
    while (argc > 0)
    {
        int newtype, oldn, newn;
        t_symbol *newname, *newarraytemplate = &s_, *newtypesym;
        t_binbuf *newbinbuf = NULL;
        if (argc < 2 || argv[0].a_type != A_SYMBOL ||
            argv[1].a_type != A_SYMBOL)
                goto bad;
        newtypesym = argv[0].a_w.w_symbol;
        newname = argv[1].a_w.w_symbol;
        if (newtypesym == &s_float)
            newtype = DT_FLOAT;
        else if (newtypesym == &s_symbol)
            newtype = DT_SYMBOL;
        //else if (newtypesym == &s_list)
        //    newtype = DT_LIST;
        else if (newtypesym == gensym("canvas"))
        {
            char filename[MAXPDSTRING+3];
            t_binbuf *b = binbuf_new();
            if (argc < 3 || argv[2].a_type != A_SYMBOL)
            {
                pd_error(x, "canvas lacks template or name");
                goto bad;
            }
            /* We're abusing newarraytemplate here to store the name of
               the abstraction (minus the .pd extension) */
            newarraytemplate = argv[2].a_w.w_symbol;
            sprintf(filename, "%s.pd", argv[2].a_w.w_symbol->s_name);
            if (binbuf_read_via_canvas(b, filename, canvas_getcurrent(), 0))
                post("warning: abstraction %s not found", filename);
            else
                newbinbuf = b;
            newtype = DT_LIST;
            argc--;
            argv++;
        }
        else if (newtypesym == gensym("text") || newtypesym == &s_list)
        {
            newtype = DT_TEXT;
        }
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
        x->t_vec[oldn].ds_fieldtemplate = newarraytemplate;
        x->t_vec[oldn].ds_binbuf = newbinbuf;
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
        *p_arraytype = x->t_vec[i].ds_fieldtemplate;
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
            x1->t_vec[i].ds_type == DT_LIST ||
            x1->t_vec[i].ds_type == DT_TEXT)
                return (0);
    }
    //if (x2->t_n > x1->t_n)
    //    post("add elements...");
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
    int nto = tto->t_n, i;
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
    int i;
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
        gpointer_setglist(&gp, glist, &x->sc_gobj);
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
        if (ds->ds_type == DT_TEXT)
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
            if (ds->ds_type == DT_TEXT)
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
    We don't do a redraw here, instead it happens when we change the
    template (in glist_add). */

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
        //for (i = 0; i < nto; i++)
        //    post("... %d", conformaction[i]);
        for (gl = pd_this->pd_canvaslist; gl; gl = gl->gl_next)
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
    if (!template)
    {
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
    gpointer_setglist(&gp, owner, &sc->sc_gobj);
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

static void gtemplate_free(t_gtemplate *x);

static void *gtemplate_donew(t_symbol *sym, int argc, t_atom *argv)
{
    t_canvas *cur = canvas_getcurrent();
    t_gobj *gob = cur->gl_list;
    while (gob)
    {
        if (pd_class(&gob->g_pd) == gtemplate_class)
        {
            error("%s: only one struct allowed per canvas.",
                cur->gl_name->s_name);
            return(0);
        }
        gob = gob->g_next;
    }
    t_gtemplate *x = (t_gtemplate *)pd_new(gtemplate_class);
    t_template *t = template_findbyname(sym);

    int i;
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
                x->x_template = t = template_new(sym, argc, argv);
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

int template_check_array_fields(t_symbol *structname, t_template *template);

/* probably duplicating some code from template_new here... */
int gtemplate_cancreate(t_symbol *templatename, int argc, t_atom *argv)
{
    while (argc > 1)
    {
        t_symbol *typesym = argv[0].a_w.w_symbol;
        if (typesym == &s_float || typesym == &s_symbol ||
            typesym == gensym("text"))
        {
            argc -= 2;
            argv += 2; 
        }
        else if (typesym == gensym("array"))
        {
            if (argc > 2 && argv[2].a_type == A_SYMBOL)
            {
                /* check for cancreation here */
                t_template *elemtemplate =
                    template_findbyname(canvas_makebindsym(argv[2].a_w.w_symbol));
                if (!elemtemplate)
                {
                    post("warning: template %s does not exist",
                        argv[2].a_w.w_symbol->s_name);
                }
                else if (template_check_array_fields(
                        canvas_makebindsym(templatename), elemtemplate) == 0)
                {
                    return 0;
                }
                argc -= 3;
                argv += 3;
            }
            else
            {
                error("struct: bad array field arguments");
                return 0;
            }
        }
        else if (typesym == gensym("canvas"))
        {
            if (argc > 2)
            {
                argc -= 3;
                argv += 3;
            }
            else
            {
                error("struct: bad canvas field arguments");
                return 0;
            }
        }
        else
        {
                error("struct: bad field type");
                return 0;
        }
    }
    if (argc)
    {
        error("struct: extra argument");
        return 0;
    }
    return 1;
}

static void *gtemplate_new(t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *sym = atom_getsymbolarg(0, argc, argv);
    if (argc >= 1)
        argc--, argv++;
    if (gtemplate_cancreate(sym, argc, argv))
    {
        return (gtemplate_donew(canvas_makebindsym(sym), argc, argv));
    }
    else
        return (0);
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
    t_float coord, extreme, div;
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
draw objects belong to templates and describe how the data in the template
are to be drawn.  The coordinates of the draw (and other display features)
can be attached to fields in the template.

todo: some better way than drawcurve has for defining click widgetbehaviors
    (just checking for field variables and moving joints is too simplistic)
*/

t_class *draw_class;

t_class *drawimage_class;

t_class *drawarray_class;

t_class *svg_class;

/* this is a wrapper around t_fielddesc-- it adds a flag for two reasons:
   1) tkpath defaults to inheriting values for options from the parent until
      you specify them-- after that, there's no way to get back to "inherit".
      We set the flag to "0" to mean "inherit"-- that way we don't send a tcl
      string for that option.  There's still the bug that the user can't set
      an attribute back to "inherit" after they've set something explicitly,
      however.
   2) This might be generally useful even with other GUI toolkits, so that if
      the user calls a method for an attribute without any arguments, Qt goes
      back to inheriting the value from the parent.  That gives us a way to
      differentiate "inherit" from fielddescriptors, the value "none" and
      "".  (Similar to the way "set" resets a message box.)
*/
typedef struct _svg_attr
{
    int a_flag;
    t_fielddesc a_attr;
} t_svg_attr;

typedef enum
{
    CT_NULL,  /* nothing specified */
    CT_SYM,   /* t_fielddesc #abcdef or color name "blue", etc. */
    CT_RGB,   /* fielddesc: R G B */
    CT_HSL,   /*            H S L */
    CT_HCL,   /*            H C L */
    CT_LAB    /*            L A B */
} t_colortype;

/* events on which to output a notification from the outlet of [draw] */
typedef struct _svg_event
{
    t_svg_attr e_focusin,
               e_focusout,
               e_activate,
               e_click,
               e_mousedown,
               e_mouseup,
               e_mouseover,
               e_mousemove,
               e_mouseout,
               e_mouseenter,
               e_mouseleave,
               e_drag; /* not in the svg spec, but should have been */
} t_svg_event;

/* svg attributes */
typedef struct _svg
{
    t_pd x_pd;
    void *x_parent; /* parent object-- [group], [draw] or [draw image] */
    int x_flags;
    t_symbol *x_type;
    t_fielddesc x_fill[3];
    t_colortype x_filltype;
    t_svg_attr x_fillopacity;
    t_svg_attr x_fillrule;
    t_fielddesc x_stroke[3];
    t_colortype x_stroketype;
    int x_ndash;
    t_fielddesc *x_strokedasharray; /* array of lengths */
    t_svg_attr x_strokedashoffset;
    t_svg_event x_events;
    t_fielddesc x_drag; /* convenience event, not part of the svg spec */
    t_svg_attr x_opacity;
    t_svg_attr x_pointerevents;
    t_svg_attr x_strokelinecap;
    t_svg_attr x_strokelinejoin;
    t_svg_attr x_strokemiterlimit;
    t_svg_attr x_strokeopacity;
    t_svg_attr x_strokewidth;
    t_svg_attr x_rx; /* for rounded rectangles, ellipse,
                        and abused for
                        circle (aka 'r')
                        line (aka 'x2') */
    t_svg_attr x_ry; /* abused for line (aka 'y2') */
    int x_transform_n;
    t_fielddesc *x_transform;
    t_svg_attr x_x; /* doubles as 'cx' and 'x1' */
    t_svg_attr x_y; /* doubles as 'cy' and 'y1' */
    t_svg_attr x_width;
    t_svg_attr x_height;
    t_svg_attr x_vis;
    t_svg_attr x_viewbox[4];
    t_fielddesc x_bbox; /* turn bbox calculation on or off */
    int x_pathrect_cache; /* 0 to recalc on next draw_getrect call
                             1 for cached
                            -1 to turn off caching */
    int x_x1; /* for cached bbox */
    int x_y1;
    int x_x2;
    int x_y2;
    int x_nargs;
    t_fielddesc *x_vec;
    int *x_nargs_per_cmd;      /* points per each path command */
    int x_npathcmds;
    char *x_pathcmds; /* for path commands */
} t_svg;

typedef struct _draw
{
    t_object x_obj;
    t_symbol *x_drawtype;
    t_canvas *x_canvas;
    t_pd *x_attr;
} t_draw;

typedef struct _drawimage
{
    t_object x_obj;
    t_fielddesc x_index;
    t_fielddesc x_vis;
    t_symbol *x_img;
    t_float x_w;
    t_float x_h;
    int x_flags;
    t_canvas *x_canvas;
    t_pd *x_attr;
} t_drawimage;

typedef struct _drawarray
{
    t_object x_obj;
    t_canvas *x_canvas;
    t_fielddesc x_data;
    t_pd *x_attr;
} t_drawarray;

extern t_outlet *obj_rightmost_outlet(t_object *x);

void draw_notifyforscalar(t_object *x, t_glist *owner, t_array *a,
    t_word *data, t_scalar *sc, t_symbol *s, int argc, t_atom *argv)
{
    t_gpointer gp;
    t_binbuf *b = binbuf_new();
    t_atom at[1];
    gpointer_init(&gp);
    if (a)
        gpointer_setarray(&gp, a, data);
    else
        gpointer_setglist(&gp, owner, &sc->sc_gobj);
    SETPOINTER(at, &gp);
    binbuf_add(b, 1, at);
    binbuf_add(b, argc, argv);
    if (x)
    {
        t_outlet *out;
        if (pd_class(&x->te_pd) == canvas_class)
            out = obj_rightmost_outlet(x);
        else
            out = x->ob_outlet;
        outlet_anything(out, s, binbuf_getnatom(b),
            binbuf_getvec(b));
    }
    gpointer_unset(&gp);
    binbuf_free(b);
}

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
        if (argv[i].a_type == A_SYMBOL &&
            is_svgpath_cmd(atom_getsymbol(argv+i)))
            j++;
    }
    return j;
}

void svg_attr_setfloatarg(t_svg_attr *a, int argc, t_atom *argv)
{
    fielddesc_setfloatarg(&a->a_attr, argc, argv);
    a->a_flag = 1;
}

void svg_attr_setfloat_const(t_svg_attr *a, t_float n)
{
    fielddesc_setfloat_const(&a->a_attr, n);
    a->a_flag = 0;
}

void *svg_new(t_pd *parent, t_symbol *s, int argc, t_atom *argv)
{
    t_fielddesc *fd;
    int i, flags = 0;
    t_svg *x = (t_svg *)pd_new(svg_class);
    t_symbol *type = x->x_type = s;
    x->x_flags = flags;
    x->x_parent = (void *)parent;
    fielddesc_setfloat_const(&x->x_vis.a_attr, 1);
    /* let it inherit visibility from parent group, if present */
    x->x_vis.a_flag = 0;
    /* the following should be set in method space */
    /* flags |= NOMOUSE; */
    if (type == gensym("rect") ||
        type == gensym("circle") ||
        type == gensym("ellipse") ||
        type == gensym("line") ||
        type == gensym("svg"))
    {
        if (type == gensym("rect") || type == gensym("svg"))
        {
            if (argc) svg_attr_setfloatarg(&x->x_width, argc--, argv++);
            else svg_attr_setfloat_const(&x->x_width, 0);
            if (argc) svg_attr_setfloatarg(&x->x_height, argc--, argv++);
            else svg_attr_setfloat_const(&x->x_height, 0);
        }
        else if (type != gensym("line"))
        {
            if (argc) svg_attr_setfloatarg(&x->x_rx, argc--, argv++);
            else svg_attr_setfloat_const(&x->x_rx, 0);
            if (type != gensym("circle"))
            {
                if (argc) svg_attr_setfloatarg(&x->x_ry, argc--, argv++);
                else svg_attr_setfloat_const(&x->x_ry, 0);
            }
        }
        if (argc) svg_attr_setfloatarg(&x->x_x, argc--, argv++);
        else svg_attr_setfloat_const(&x->x_x, 0);
        if (argc) svg_attr_setfloatarg(&x->x_y, argc--, argv++);
        else svg_attr_setfloat_const(&x->x_x, 0);
        /* Just reuse rx and ry for x2 and y2 of "line" */
        if (type == gensym("line"))
        {
            if (argc) svg_attr_setfloatarg(&x->x_rx, argc--, argv++);
            else svg_attr_setfloat_const(&x->x_rx, 0);
            if (argc) svg_attr_setfloatarg(&x->x_ry, argc--, argv++);
            else svg_attr_setfloat_const(&x->x_ry, 0);
        }
    }
    if (x->x_type == gensym("path"))
    {
        int ncmds = x->x_npathcmds = path_ncmds(argc, argv);
        x->x_pathcmds = (char *)t_getbytes(ncmds * sizeof(char));
        x->x_nargs_per_cmd = (int *)t_getbytes(ncmds * sizeof(int));
        for (i = 0; i < ncmds; i++) x->x_nargs_per_cmd[i] = 0;
        x->x_nargs = argc - ncmds;
    }
    else if (type == gensym("g") || type == gensym("svg"))
    {
        x->x_nargs = 0;
        x->x_vec = 0;
        /* Hack to get around the path parsing below, which should really
           be split out... */
        argc = 0;
    }
    else
    {
        x->x_nargs = argc;
    }
    x->x_vec = (t_fielddesc *)t_getbytes(x->x_nargs * sizeof(t_fielddesc));
    if (argc && x->x_type == gensym("path"))
    {
        if (argv->a_type != A_SYMBOL ||
            (atom_getsymbol(argv) != gensym("M") &&
             atom_getsymbol(argv) != gensym("m")))
        {
            /* this should probably be the parent instead of "x" */
            pd_error(x->x_parent, "draw path: path data must start "
                        "with a moveto command (M or m)");
            return 0;
        }
    }
    int cmdn = -1; /* hack */
    for (i = 0, fd = x->x_vec; i < argc; i++, argv++)
    {
        if (x->x_type == gensym("path") &&
            argv->a_type == A_SYMBOL &&
            is_svgpath_cmd(atom_getsymbol(argv)))
        {
                x->x_pathcmds[++cmdn] = *(atom_getsymbol(argv)->s_name);
        }
        else
        {
            fielddesc_setfloatarg(fd++, 1, argv);
            /* post("got a coord"); */
            if (x->x_type == gensym("path"))
            {
                (x->x_nargs_per_cmd[cmdn])++;
                /* if we get a field variable, just
                   turn off the get_rect caching */
                if (argv->a_type == A_SYMBOL)
                    x->x_pathrect_cache = -1;
            }
        }
    }
    fielddesc_setfloat_const(&x->x_bbox, 1);
    fielddesc_setfloat_const(&x->x_drag, 0);
    x->x_filltype = CT_NULL;
    x->x_fillopacity.a_flag = 0;
    x->x_fillrule.a_flag = 0;
    fielddesc_setfloat_const(&x->x_pointerevents.a_attr, 1);
    x->x_pointerevents.a_flag = 0;
    x->x_stroketype = 0;
    x->x_strokelinecap.a_flag = 0;
    x->x_strokelinejoin.a_flag = 0;
    x->x_strokemiterlimit.a_flag = 0;
    x->x_strokeopacity.a_flag = 0;
    x->x_strokewidth.a_flag = 0;
    /* set the flag of the first array element... */
    x->x_viewbox->a_flag = 0;
    x->x_x1 = 0;
    x->x_x2 = 0;
    x->x_y1 = 0;
    x->x_y2 = 0;

    x->x_ndash = 0;
    x->x_strokedasharray =
        (t_fielddesc *)t_getbytes(x->x_ndash * sizeof(t_fielddesc));
    x->x_transform_n = 0;
    x->x_strokedashoffset.a_flag = 0;
    x->x_transform = (t_fielddesc *)t_getbytes(x->x_transform_n *
        sizeof(t_fielddesc));
    /* initialize events */
    fielddesc_setfloat_const(&x->x_events.e_focusin.a_attr, 0);
    x->x_events.e_focusin.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_focusout.a_attr, 0);
    x->x_events.e_focusout.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_activate.a_attr, 0);
    x->x_events.e_activate.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_click.a_attr, 0);
    x->x_events.e_click.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_mousedown.a_attr, 0);
    x->x_events.e_mousedown.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_mouseup.a_attr, 0);
    x->x_events.e_mouseup.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_mouseover.a_attr, 0);
    x->x_events.e_mouseover.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_mousemove.a_attr, 0);
    x->x_events.e_mousemove.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_mouseout.a_attr, 0);
    x->x_events.e_mouseout.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_mouseenter.a_attr, 0);
    x->x_events.e_mouseenter.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_mouseleave.a_attr, 0);
    x->x_events.e_mouseleave.a_flag = 0;
    fielddesc_setfloat_const(&x->x_events.e_drag.a_attr, 0);
    x->x_events.e_drag.a_flag = 0;

    char buf[50];
    // Here we bind the parent object to the addy for
    // the svg. This way both [group] and [draw] will have
    // the same binding symbol
    sprintf(buf, "x%lx", (long unsigned int)x);
    pd_bind(parent, gensym(buf));

    return (x);
}

t_pd *svg_header(t_pd *x)
{
    return (&(((t_svg *)x)->x_pd));
}

static int symbol_isdrawtype(t_symbol *s)
{
    if (s == gensym("circle")  || s == gensym("ellipse")  ||
        s == gensym("line")    || s == gensym("path")     ||
        s == gensym("polygon") || s == gensym("polyline") ||
        s == gensym("rect")    || s == gensym("image")    ||
        s == gensym("sprite")  || s == gensym("g")        ||
        s == gensym("svg")   || s == gensym("array"))
    {
        return 1;
    }
    else
        return 0;
}

static void *drawimage_new(t_symbol *classsym, int argc, t_atom *argv);
static void *drawarray_new(t_symbol *s, int argc, t_atom *argv);
extern void *group_new(t_symbol *type, int argc, t_atom *argv);

static void *draw_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    t_symbol *type;
    if (argc && argv->a_type == A_SYMBOL &&
        symbol_isdrawtype(argv[0].a_w.w_symbol))
    {
        type = atom_getsymbolarg(0, argc--, argv++);
        /* sprite and image have their own widgetbehavior, and so does
           array. They also have their own classes and constructors... */
        if (type == gensym("sprite") || type == gensym("image"))
            return (drawimage_new(type, argc, argv));
        else if (type == gensym("array"))
            return (drawarray_new(type, argc, argv));
        else if (type == gensym("g") || type == gensym("svg"))
            return (group_new(type, argc, argv));
    }
    else
    {
        type = gensym("rect");
        post("warning: draw: no shape specified, defaulting to 'rect'");
    }

    t_draw *x = (t_draw *)pd_new(draw_class);

    /* create a proxy for drawing/svg attributes */
    if (!(x->x_attr = (t_pd *)svg_new((t_pd *)x, type, argc, argv)))
    {
        pd_error(x, "draw: path data must start with 'm' or 'M'");
        return (0);
    }
    /* now that we have our proxy, make an inlet for it.
       The inlet will belong to our draw object, but the
       svg_class will actually receive the messages
       to the inlet. */
    t_svg *sa = (t_svg *)x->x_attr;
    inlet_new(&x->x_obj, &sa->x_pd, 0, 0);

    /* outlet for event notifications */
    outlet_new(&x->x_obj, &s_anything);

    /* x_canvas can stay here */
    x->x_canvas = canvas_getcurrent();


    return (x);
}

t_canvas *svg_parentcanvas(t_svg *x)
{
    /* this is probably a better interface... I am
       returning c sometimes because it works for my
       use case, but that doesn't seem like a sensible
       interface in general. */
    t_canvas *ret = 0;
    if (x->x_type == gensym("g") || x->x_type == gensym("svg"))
    {
        t_canvas *c = (t_canvas *)x->x_parent;
        if (c->gl_owner)
            ret = c->gl_owner;
        else
            ret = c;
    }
    else if (x->x_type == gensym("sprite") ||
             x->x_type == gensym("image"))
    {
        t_drawimage *d = (t_drawimage *)x->x_parent;
        ret = d->x_canvas;
    }
    else if (x->x_type == gensym("array"))
    {
        t_drawarray *d = (t_drawarray *)x->x_parent;
        ret = d->x_canvas;
    }
    else
    {
        t_draw *d = (t_draw *)x->x_parent;
        ret = d->x_canvas;
    }
    return (ret);
}

static int rgb_to_int(int r, int g, int b)
{
    int r1 = r < 0 ? 0 : r;
    if (r1 > 255) r1 = 255;
    int g1 = g < 0 ? 0 : g;
    if (g1 > 255) g1 = 255;
    int b1 = b < 0 ? 0 : b;
    if (b1 > 255) b1 = 255;
    return ((r1 << 16) + (g1 << 8) + b1);
}

/* hsl implementation from (3-clause BSD-licensed) d3 */
static t_float hsl_v(t_float m1, t_float m2, t_float h)
{
    if (h > 360) h -= 360;
    else if (h < 0) h += 360;
    if (h < 60) return (m1 + (m2 - m1) * h / 60.);
    if (h < 180) return m2;
    if (h < 240) return (m1 + (m2 - m1) * (240 - h) / 60.);
    return m1;
}

static int hsl_to_int(t_float h, t_float s, t_float lightness)
{
    t_float m1, m2;

    h = ((int)h) % 360;
    h = h < 0 ? h + 360 : h;
    s = s < 0 ? 0 : s > 1 ? 1 : s;
    lightness = lightness < 0 ? 0 : lightness > 1 ? 1 : lightness;

    m2 = lightness <= 0.5 ?
        lightness * (1 + s) :
        lightness + s - lightness * s;
    m1 = 2 * lightness - m2;

    return (rgb_to_int(
        (int)(hsl_v(m1, m2, h + 120) * 255 + 0.5),
        (int)(hsl_v(m1, m2, h) * 255 + 0.5),
        (int)(hsl_v(m1, m2, h - 120) * 255 + 0.5)));
}

t_float lab_xyz(t_float x)
{
    return (x > 0.206893034 ? x * x * x : (x - 4. / 29.) / 7.787037);
}

int xyz_rgb(t_float r)
{
    return ((int)(0.5 + (255 * (r <= 0.00304 ?
        12.92 * r :
        1.055 * pow(r, 1. / 2.4) - 0.055))));
}

#define LAB_X 0.950470
#define LAB_Y 1.
#define LAB_Z 1.088830
static int lab_to_int(t_float lightness, t_float a, t_float b)
{
    t_float y, x, z;
    y = (lightness + 16) / 116.;
    x = y + a / 500.;
    z = y - b / 200.;

    x = lab_xyz(x) * LAB_X;
    y = lab_xyz(y) * LAB_Y;
    z = lab_xyz(z) * LAB_Z;

    return rgb_to_int(
        xyz_rgb( 3.2404542 * x - 1.5371385 * y - 0.4985314 * z),
        xyz_rgb(-0.9692660 * x + 1.8760108 * y + 0.0415560 * z),
        xyz_rgb( 0.0556434 * x - 0.2040259 * y + 1.0572252 * z));
}

static char *svg_get_color(t_fielddesc *fd, t_colortype ct,
    t_template *template, t_word *data)
{
    static char str[10];
    if (ct == CT_NULL)
        sprintf(str, "none");
    else if (ct == CT_SYM)
        sprintf(str, "%s", fielddesc_getsymbol(fd, template, data, 0)->s_name);
    else
    {
        t_float c1, c2, c3;
        int result = 0;
        c1 = fielddesc_getcoord(fd, template, data, 0),
        c2 = fielddesc_getcoord(fd+1, template, data, 0),
        c3 = fielddesc_getcoord(fd+2, template, data, 0);
        if (ct == CT_RGB)
            result = rgb_to_int((int)c1, (int)c2, (int)c3);
        else if (ct == CT_HSL)
            result = hsl_to_int(c1, c2, c3);
        else if (ct == CT_LAB)
            result = lab_to_int(c1, c2, c3);
        sprintf(str, "#%.6x", result);
    }
    return str;
}

char *get_strokelinecap(int a)
{
    static char strokelinecap[15];
    if (a == 0) sprintf(strokelinecap, "butt");
    else if (a == 1) sprintf(strokelinecap, "round");
    else if (a == 2) sprintf(strokelinecap, "square");
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

t_svg_attr *svg_getattr(t_svg *x, t_symbol *s)
{
    if (s == gensym("fill-opacity")) return &x->x_fillopacity;
    else if (s == gensym("fill-rule")) return &x->x_fillrule;
    else if (s == gensym("height")) return &x->x_height;
    else if (s == gensym("opacity")) return &x->x_opacity;
    else if (s == gensym("pointer-events")) return &x->x_pointerevents;
    else if (s == gensym("rx") || s == gensym("r") || s == gensym("x2"))
        return &x->x_rx;
    else if (s == gensym("ry") || s == gensym("y2"))
        return &x->x_ry;
    else if (s == gensym("stroke-opacity")) return &x->x_strokeopacity;
    else if (s == gensym("stroke-dashoffset")) return &x->x_strokedashoffset;
    else if (s == gensym("stroke-linecap")) return &x->x_strokelinecap;
    else if (s == gensym("stroke-linejoin")) return &x->x_strokelinejoin;
    else if (s == gensym("stroke-miterlimit")) return &x->x_strokemiterlimit;
    else if (s == gensym("stroke-width")) return &x->x_strokewidth;
    else if (s == gensym("vis")) return &x->x_vis;
    else if (s == gensym("width")) return &x->x_width;
    else if (s == gensym("x") || s == gensym("cx") || s == gensym("x1"))
        return &x->x_x;
    else if (s == gensym("y") || s == gensym("cy") || s == gensym("y1"))
        return &x->x_y;
    return 0;
}

t_symbol *group_gettype(t_glist *glist)
{
    return ((t_svg *)glist->gl_svg)->x_type;
}

void svg_parsetransform(t_svg *x, t_template *template, t_word *data,
    t_float *mp1, t_float *mp2, t_float *mp3,
    t_float *mp4, t_float *mp5, t_float *mp6);

void svg_group_pathrect_cache(t_svg *x, int state);

extern void scalar_drawselectrect(t_scalar *x, t_glist *glist, int state);

void svg_sendupdate(t_svg *x, t_canvas *c, t_symbol *s,
    t_template *template, t_word *data, int *predraw_bbox, void *parent,
    t_scalar *sc, t_array *array)
{
   /* todo-- I'm mixing "c" with glist_getcanvas(c) too freely...
      need to experiment with gop scalars to make sure I'm not breaking
      anything */ 
    char tag[MAXPDSTRING];
    int index;
    if (array)
    {
        int elemsize = array->a_elemsize;
        if (!elemsize) elemsize = 1;
        index = (((char *)data) - array->a_vec) / elemsize;
    }
    else
        index = -1;
    if (x->x_type == gensym("g") || x->x_type == gensym("svg"))
    {
        sprintf(tag, "dgroup%lx.%lx",
            (long unsigned int)x->x_parent,
            (long unsigned int)data);
    }
    else
    {
        sprintf(tag, "draw%lx.%lx",
            (long unsigned int)x->x_parent,
            (long unsigned int)data);
    }
    if (s == gensym("bbox"))
        *predraw_bbox = 1;
    else if (s == gensym("fill"))
    {
        gui_vmess("gui_draw_configure", "xsss",
            glist_getcanvas(c), tag, s->s_name,
                svg_get_color(x->x_fill, x->x_filltype, template, data));
    }
    else if (s == gensym("stroke"))
    {
        gui_vmess("gui_draw_configure", "xsss",
            glist_getcanvas(c), tag, s->s_name,
                svg_get_color(x->x_stroke, x->x_stroketype, template, data));
    }
    else if (s == gensym("fill-rule"))
        gui_vmess("gui_draw_configure", "xssi",
            glist_getcanvas(c), tag, "fill-rule",
            (int)fielddesc_getcoord(
                &x->x_fillrule.a_attr, template, data, 0) ?
                    "evenodd" : "nonzero");
    else if (s == gensym("fill-opacity") ||
             s == gensym("opacity") ||
             s == gensym("stroke-dashoffset") ||
             s == gensym("stroke-miterlimit") ||
             s == gensym("stroke-opacity"))
        gui_vmess("gui_draw_configure", "xssf",
            glist_getcanvas(c), tag, s->s_name,
            fielddesc_getcoord(
                &(svg_getattr(x, s)->a_attr), template, data, 0));
    else if (s == gensym("pointer-events"))
        *predraw_bbox = 1;
    else if (s == gensym("stroke-linecap"))
        gui_vmess("gui_draw_configure", "xsss",
            glist_getcanvas(c), tag, "stroke-linecap",
                get_strokelinecap(
                    (int)fielddesc_getcoord(&x->x_strokelinecap.a_attr,
                        template, data, 0)));
    else if (s == gensym("stroke-linejoin"))
        gui_vmess("gui_draw_configure", "xsss",
            glist_getcanvas(c), tag, "stroke-linejoin",
                get_strokelinejoin(
                    (int)fielddesc_getcoord(&x->x_strokelinejoin.a_attr,
                        template, data, 0)));
    else if (s == gensym("stroke-width") ||
             s == gensym("rx") || s == gensym("r") || s == gensym("x2") ||
             s == gensym("ry") || s == gensym("y2") ||
             s == gensym("x") || s == gensym("cx") || s == gensym("x1") ||
             s == gensym("y") || s == gensym("cy") || s == gensym("y1") ||
             s == gensym("height") || s == gensym("width"))
    {
        gui_vmess("gui_draw_configure", "xssf",
            glist_getcanvas(c), tag, s->s_name,
            fielddesc_getcoord(
                &(svg_getattr(x, s)->a_attr), template, data, 0));
        *predraw_bbox = 1;
    }
    else if (s == gensym("transform"))
    {
        t_float m1, m2, m3, m4, m5, m6;
        /* we'll probably get a different bbox now, so we will calculate a
           new one the next time we call draw_getrect for this draw command.
           For g we need to do it for all of the draw commands inside it.

           For inner svgs, however, we can ignore the inner content since
           it will never appear outside the width/height specified for the
           container. (At least not for now, since we don't allow the user
           to set an overflow style of "visible.") This allows us to get
           a speedup when interacting with content inside an inner svg, as
           Pd doesn't have to do any complex bbox calculations.

           Unfortunately, the HTML5 getBBox() method calculates the boundaries
           _without_ regard to clipping, so clipped content in inner svg will
           still trigger scrollbars.
        */
        if (x->x_type == gensym("g"))
            svg_group_pathrect_cache(x, 0);
        else if (x->x_pathrect_cache != -1)
        {
            x->x_pathrect_cache = 0;
        }
        svg_parsetransform(x, template, data, &m1, &m2, &m3, &m4, &m5, &m6);
        char mbuf[MAXPDSTRING];
        sprintf(mbuf, "matrix(%g %g %g %g %g %g)", m1, m2, m3, m4, m5, m6);
        gui_vmess("gui_draw_configure", "xsss",
            glist_getcanvas(c), tag, s->s_name,
            mbuf);
        *predraw_bbox = 1;
    }
    else if (s == gensym("mouseover"))
    {
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tag, sc, x, "mouseover", array, index,
            (int)fielddesc_getcoord(
                &x->x_events.e_mouseover.a_attr, template, data, 0));
    }
    else if (s == gensym("mouseout"))
    {
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tag, sc, x, "mouseout", array, index,
            (int)fielddesc_getcoord(
                &x->x_events.e_mouseout.a_attr, template, data, 0));
    }
    else if (s == gensym("mousemove"))
    {
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tag, sc, x, "mousemove", array, index,
            (int)fielddesc_getcoord(
                &x->x_events.e_mousemove.a_attr, template, data, 0));
    }
    else if (s == gensym("mouseup"))
    {
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tag, sc, x, "mouseup", array, index,
            (int)fielddesc_getcoord(
                &x->x_events.e_mouseup.a_attr, template, data, 0));
    }
    else if (s == gensym("mousedown"))
    {
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tag, sc, x, "mousedown", array, index,
            (int)fielddesc_getcoord(
                &x->x_events.e_mousedown.a_attr, template, data, 0));
    }
    else if (s == gensym("mouseenter"))
    {
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tag, sc, x, "mouseenter", array, index,
            (int)fielddesc_getcoord(
                &x->x_events.e_mouseenter.a_attr, template, data, 0));
    }
    else if (s == gensym("mouseleave"))
    {
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tag, sc, x, "mouseleave", array, index,
            (int)fielddesc_getcoord(
                &x->x_events.e_mouseleave.a_attr, template, data, 0));
    }
    else if (s == gensym("drag"))
    {
        gui_vmess("gui_draw_drag_event", "xsxxsxii",
            glist_getcanvas(c), tag, sc, x, "drag", array, index,
            (int)fielddesc_getcoord(
                &x->x_events.e_drag.a_attr, template, data, 0));
    }
    else if (s == gensym("vis"))
    {
        gui_vmess("gui_draw_configure", "xsss",
            glist_getcanvas(c), tag, "visibility",
            (int)fielddesc_getcoord(
                &x->x_vis.a_attr, template, data, 0) ? "visible" : "hidden");
        *predraw_bbox = 1;
    }
    else if (s == gensym("stroke-dasharray"))
    {
        if (x->x_ndash)
        {
            t_fielddesc *fd = x->x_strokedasharray;
            int i;
            gui_start_vmess("gui_draw_configure", "xss",
                glist_getcanvas(c), tag, s->s_name);
            gui_start_array();
            for (i = 0; i < x->x_ndash; i++)
                gui_f(fielddesc_getcoord(fd+i, template, data, 0));
            gui_end_array();
            gui_end_vmess();
        }
    }
    else if (s == gensym("d"))
    {
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "draw%lx.%lx",
            (long unsigned int)parent, (long unsigned int)data);
        gui_start_vmess("gui_draw_configure", "xss",
            glist_getcanvas(c), tagbuf, "d"); 
        /* let's turn off bbox caching so we can recalculate the bbox */
        if (x->x_pathrect_cache != -1)
           x->x_pathrect_cache = 0;
        *predraw_bbox = 1;
        int i;
        char *cmd;
        t_fielddesc *f;
        int totalpoints = 0; /* running tally */
        /* start an array parameter */
        char cmdbuf[2];
        gui_start_array();
        /* path parser: no error checking yet */
        for (i = 0, cmd = x->x_pathcmds; i < x->x_npathcmds; i++, cmd++)
        {
            int j;
            f = (x->x_vec)+totalpoints;
            sprintf(cmdbuf, "%c", *(cmd));
            gui_s(cmdbuf);
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
                gui_f(fielddesc_getcoord(
                    f+j, template, data, 0));
            totalpoints += x->x_nargs_per_cmd[i];
        }
        gui_end_array();
        gui_end_vmess();
    }
    else if (s == gensym("index"))
    {
        gui_vmess("gui_drawimage_index", "xxxi",
            glist_getcanvas(c), parent, data, drawimage_getindex(parent, template, data));
    }
    else if (s == gensym("image_xy"))
    {
        /* Hack to deal with x/y for sprite/image. They have an extra
           container so that we don't have to deal with changing each
           image in the sequence here. */
        gui_vmess("gui_drawimage_xy", "xxxff",
            glist_getcanvas(c), parent, data,
            x->x_x.a_flag ?
                fielddesc_getcoord(&x->x_x.a_attr, template, data, 0) : 0,
            x->x_y.a_flag ?
                fielddesc_getcoord(&x->x_y.a_attr, template, data, 0) : 0);
    }
    else if (s == gensym("viewbox"))
    {
        gui_start_vmess("gui_draw_viewbox", "xss",
            glist_getcanvas(c), tag, "viewBox");
        gui_start_array();
        if (x->x_viewbox->a_flag)
        {
            gui_f(fielddesc_getcoord(&x->x_viewbox[0].a_attr,
                template, data, 0));
            gui_f(fielddesc_getcoord(&x->x_viewbox[1].a_attr,
                template, data, 0));
            gui_f(fielddesc_getcoord(&x->x_viewbox[2].a_attr,
                template, data, 0));
            gui_f(fielddesc_getcoord(&x->x_viewbox[3].a_attr,
                template, data, 0));
        }
        gui_end_array();
        gui_end_vmess();
    }
    else if (s == gensym("points"))
    {
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "draw%lx.%lx",
            (long unsigned int)parent, (long unsigned int)data);
        gui_start_vmess("gui_draw_coords", "xss",
            glist_getcanvas(c), tagbuf, x->x_type->s_name);
        gui_start_array();
        /* let's turn off bbox caching so we can recalculate the bbox */
        if (x->x_pathrect_cache != -1)
           x->x_pathrect_cache = 0;
        *predraw_bbox = 1;
        int i;
        t_fielddesc *f = x->x_vec;
        if (x->x_type == gensym("rect") ||
            x->x_type == gensym("ellipse"))
        {
            t_float xval, yval, w, h;
            xval = fielddesc_getcoord(f, template, data, 0);
            yval = fielddesc_getcoord(f+1, template, data, 0);
            if (x->x_type == gensym("rect"))
            {
                w = xval + (fielddesc_getcoord(f+2, template, data, 0));
                h = yval + (fielddesc_getcoord(f+3, template, data, 0));
                gui_f(xval);
                gui_f(yval);
                gui_f(w);
                gui_f(h);
            }
            else
            {
                gui_f(xval);
                gui_f(yval);
            }
        }
        else
        {
            int n = (x->x_type == gensym("circle")) ? 2 : x->x_nargs;
            for (i = 0; i < n; i++)
                gui_f(fielddesc_getcoord(f+i, template, data, 0));
        }
        gui_end_array();
        gui_end_vmess();
    }
}

void svg_updatevec(t_canvas *c, t_word *data, t_template *template,
    t_template *target, void *parent, t_symbol *s, t_svg *x, t_scalar *sc,
    int *predraw_bbox)
{
    int i, j;
    for (i = 0; i < template->t_n; i++)
    {
        if (template->t_vec[i].ds_type == DT_ARRAY)
        {
            int arrayonset, type, elemsize;
            t_symbol *targetsym = target->t_sym, *elemtemplatesym;
            t_array *array;
            char *elem;
            if (!template_find_field(template, ((template->t_vec)+i)->ds_name,
                &arrayonset, &type, &elemtemplatesym))
            {
                error("draw: problem finding array...");
                return;
            }

            t_template *elemtemplate = template_findbyname(elemtemplatesym);
            /* this is a doozy... */
            array = *(t_array **)(((char *)data) + arrayonset);
            elem = (char *)array->a_vec;

            if (elemtemplatesym == targetsym)
            {
                elemsize = array->a_elemsize;
                for (j = 0; j < array->a_n; j++)
                {
                    svg_sendupdate(x, glist_getcanvas(c), s,
                        elemtemplate, (t_word *)(elem + elemsize * j),
                        predraw_bbox, parent, sc, array);
                }
            }
            svg_updatevec(c, (t_word *)elem, elemtemplate, target, parent,
                s, x, sc, predraw_bbox);
        }
    }
}

/* todo: i think svg_togui and this are unnecessarily duplicating code... */
extern void scalar_select(t_gobj *z, t_glist *owner, int state);
t_template *canvas_findtemplate(t_canvas *c);
void svg_doupdate(t_svg *x, t_canvas *c, t_symbol *s)
{
    t_gobj *g;
    t_template *template;
    t_canvas *visible = c;
    void *parent = x->x_parent;
    t_canvas *parentcanvas =
        canvas_templatecanvas_forgroup(svg_parentcanvas(x));
    t_template *parenttemplate = canvas_findtemplate(parentcanvas);
    int redraw_bbox = 0;
     /* "visible" is here because we could be drawn in a gop, in
       which case we must "climb" out to the parent canvas on
       which we are drawn. There's probably a function somewhere
       that abstracts this away... */
    while(visible->gl_isgraph && visible->gl_owner)
        visible = visible->gl_owner;
    for (g = c->gl_list; g; g = g->g_next)
    {
        if (glist_isvisible(c) && g->g_pd == scalar_class)
        {
            template = template_findbyname((((t_scalar *)g)->sc_template));
            /* still kinda hacky-- redrawbbox side-effect is weird */
            t_word *data = ((t_scalar *)g)->sc_vec;
            if (parenttemplate == template)
            { 
                svg_sendupdate(x, visible, s, template,
                    data, &redraw_bbox, parent, (t_scalar *)g, 0);
            }
            else
            {
                if (template_has_elemtemplate(template, parenttemplate))
                {
                    svg_updatevec(c, ((t_scalar *)g)->sc_vec, template,
                        parenttemplate, parent, s, x, (t_scalar *)g,
                        &redraw_bbox);
                }
            }
            if (redraw_bbox)
            {
                /* uncache the scalar's bbox */
                ((t_scalar *)g)->sc_bboxcache = 0;
                /* only get the scroll if we had to redraw the bbox */
                canvas_getscroll(visible);
                if (glist_isselected(c, &((t_scalar *)g)->sc_gobj))
                {
                    scalar_drawselectrect((t_scalar *)g, c, 0);
                    scalar_drawselectrect((t_scalar *)g, c, 1);
                }
            }
        }
        if (g->g_pd == canvas_class)
        {
            svg_doupdate(x, (t_glist *)g, s);
        }
    }
}

void svg_update(t_svg *x, t_symbol *s)
{
    t_canvas *c;
    for (c = pd_this->pd_canvaslist; c; c = c->gl_next)
        svg_doupdate(x, c, s);
}

/* not sure if this will work with array elements */
void svg_register_events(t_gobj *z, t_canvas *c, t_scalar *sc,
    t_template *template, t_word *data, t_array *a)
{
    t_svg *svg;
    int index;
    if (a)
    {
        int elemsize = a->a_elemsize; 
        /* avoid divide-by-zero in case our elemsize is 0. Currently
           there's no practical use case for an array of elements which
           have zero size as there's no way to differentiate their
           visualization. However if [draw array] changes in the future
           to allow spacing out element groups or something like that,
           we will have to revisit this workaround we're using to get
           the element's index. */
        if (!elemsize) elemsize = 1;
        index = (((char *)data) - a->a_vec) / elemsize;
    }
    else
        index = -1;
    char tagbuf[MAXPDSTRING];
    if (pd_class(&z->g_pd) == canvas_class)
    {
        svg = (t_svg *)((t_glist *)z)->gl_svg;
        sprintf(tagbuf, "dgroup%lx.%lx", (long unsigned int)z,
            (long unsigned int)data);
    }
    else if (pd_class(&z->g_pd) == draw_class)
    {
        svg = (t_svg *)((t_draw *)z)->x_attr;
        sprintf(tagbuf, "draw%lx.%lx", (long unsigned int)z,
            (long unsigned int)data);
    }
    else if (pd_class(&z->g_pd) == drawimage_class)
    {
        svg = (t_svg *)((t_drawimage *)z)->x_attr;
        sprintf(tagbuf, "draw%lx.%lx", (long unsigned int)z,
            (long unsigned int)data);
    }
    else /* legacy drawing commands: curve, drawnumber, etc. */
    {
        return;
    }
    if (svg->x_events.e_mouseover.a_flag)
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tagbuf, sc, svg, "mouseover", a, index,
            (int)fielddesc_getcoord(&svg->x_events.e_mouseover.a_attr, template,
            data, 1));
    if (svg->x_events.e_mouseout.a_flag)
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tagbuf, sc, svg, "mouseout", a, index,
            (int)fielddesc_getcoord(&svg->x_events.e_mouseout.a_attr, template,
            data, 1));
    if (svg->x_events.e_mousemove.a_flag)
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tagbuf, sc, svg, "mousemove", a, index,
            (int)fielddesc_getcoord(&svg->x_events.e_mousemove.a_attr, template,
            data, 1));
    if (svg->x_events.e_mousedown.a_flag)
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tagbuf, sc, svg, "mousedown", a, index,
            (int)fielddesc_getcoord(&svg->x_events.e_mousedown.a_attr, template,
            data, 1));
    if (svg->x_events.e_mouseup.a_flag)
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tagbuf, sc, svg, "mouseup", a, index,
            (int)fielddesc_getcoord(&svg->x_events.e_mouseup.a_attr, template,
            data, 1));
    if (svg->x_events.e_mouseenter.a_flag)
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tagbuf, sc, svg, "mouseenter", a, index,
            (int)fielddesc_getcoord(&svg->x_events.e_mouseenter.a_attr,
            template, data, 1));
    if (svg->x_events.e_mouseleave.a_flag)
        gui_vmess("gui_draw_event", "xsxxsxii",
            glist_getcanvas(c), tagbuf, sc, svg, "mouseleave", a, index,
            (int)fielddesc_getcoord(&svg->x_events.e_mouseleave.a_attr,
            template, data, 1));
    if (svg->x_events.e_drag.a_flag)
        gui_vmess("gui_draw_drag_event", "xsxxsxii",
            glist_getcanvas(c), tagbuf, sc, svg, "drag", a, index,
            (int)fielddesc_getcoord(&svg->x_events.e_drag.a_attr, template,
            data, 1));
}

void svg_setattr(t_svg *x, t_symbol *s, t_int argc, t_atom *argv)
{
    t_svg_attr *attr = svg_getattr(x, s);
    if (!attr)
    {
        pd_error(x, "draw: can't find attribute %s", s->s_name);
        return;
    }
    if (argc < 1)
        attr->a_flag = 0;
    else if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&attr->a_attr, argc, argv);
        attr->a_flag = 1;
        svg_update(x, s);
    }
}

/* Currently used just to update the arguments when the user changes
   the arguments in an existing [draw svg] object. */
void svg_update_args(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    /* "g" doesn't take any args, so check for "svg" arg */
    if (atom_getsymbolarg(0, argc, argv) == gensym("svg"))
    {
        argc--, argv++;
        if (argc) svg_setattr(x, gensym("width"), argc--, argv++), post("did width");
        if (argc) svg_setattr(x, gensym("height"), argc--, argv++);
        if (argc) svg_setattr(x, gensym("x"), argc--, argv++);
        if (argc) svg_setattr(x, gensym("y"), argc--, argv++);
    }
}

void svg_vis(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&x->x_vis.a_attr, argc, argv);
        x->x_vis.a_flag = 1;
        svg_update(x, s);
    }
}

/* resize x_vec et al for path or shape coordinate data */
void svg_resizecoords(t_svg *x, int argc, t_atom *argv)
{
    //if (x->x_type != gensym("path")) return;
    int oldn = x->x_npathcmds;
    /* for polyline and polygon, we don't want any path commands */
    x->x_npathcmds = (x->x_type == gensym("path")) ?
        path_ncmds(argc, argv) : 0;
    x->x_pathcmds = (char *)t_resizebytes(x->x_pathcmds,
        oldn * sizeof(char),
        x->x_npathcmds * sizeof(char));

    x->x_nargs_per_cmd = (int *)t_resizebytes(x->x_nargs_per_cmd,
        oldn * sizeof(*x->x_nargs_per_cmd),
        x->x_npathcmds * sizeof(*x->x_nargs_per_cmd));

    oldn = x->x_nargs;
    x->x_nargs = argc - x->x_npathcmds;

    x->x_vec = (t_fielddesc *)t_resizebytes(x->x_vec,
        oldn * sizeof(*x->x_vec),
        x->x_nargs * sizeof(*x->x_vec));
}

void svg_data(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc)
    {
        /* only paths and polys */
        if (x->x_type != gensym("path") && x->x_type != gensym("polyline") &&
            x->x_type != gensym("polygon"))
            return;
        /* "points" for polys... */
        if (s == gensym("d") && x->x_type != gensym("path"))
            return;
        /* and "d" for paths */
        if (x->x_type == gensym("path"))
        {
            if (s == gensym("points")) return;
            if (argv->a_type == A_SYMBOL &&
                !is_svgpath_cmd(atom_getsymbol(argv)) ||
                argv->a_type != A_SYMBOL)
                return;
        }
    }
    /* resize the path data fields to fit the incoming data */
    svg_resizecoords(x, argc, argv);
    /* todo: loop is copy/pasted from draw_new-- break it out */
    t_fielddesc *fd;
    int i, cmdn = -1; /* hack */
    for (i = 0, fd = x->x_vec; i < argc; i++, argv++)
    {
        if (x->x_type == gensym("path") &&
            argv->a_type == A_SYMBOL &&
            is_svgpath_cmd(atom_getsymbol(argv)))
        {
                x->x_pathcmds[++cmdn] = *(atom_getsymbol(argv)->s_name);
                x->x_nargs_per_cmd[cmdn] = 0;
        }
        else
        {
            fielddesc_setfloatarg(fd++, 1, argv);
            /* post("got a coord"); */
            if (x->x_type == gensym("path"))
            {
                (x->x_nargs_per_cmd[cmdn])++;
                /* if we get a field variable, just
                   turn off the get_rect caching */
                if (argv->a_type == A_SYMBOL)
                    x->x_pathrect_cache = -1;
            }
        }
    }
    svg_update(x, s);
}

void svg_strokedasharray(t_svg *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_fielddesc *fd;
    x->x_strokedasharray =
        (t_fielddesc *)t_resizebytes(x->x_strokedasharray,
            x->x_ndash * sizeof(*x->x_strokedasharray),
            argc * sizeof(*x->x_strokedasharray));
    x->x_ndash = argc;
    fd = x->x_strokedasharray;
    while (argc)
        fielddesc_setfloatarg(fd++, argc--, argv++);
    svg_update(x, s);
}

static int svg_set_color(t_svg *x, t_colortype *type, t_fielddesc *cfield,
    t_symbol *s, int argc, t_atom *argv)
{
    if (argc >= 3)
    {
        if (argc == 3)
            *type = CT_RGB;
        else
        {
            if (argv->a_type == A_SYMBOL)
            {
                t_symbol *cs = atom_getsymbolarg(0, argc, argv);
                if (cs == gensym("rgb")) *type = CT_RGB;
                else if (cs == gensym("hsl")) *type = CT_HSL;
                else if (cs == gensym("hcl")) *type = CT_HCL;
                else if (cs == gensym("lab")) *type = CT_LAB;
                else
                {
                    pd_error(x, "draw: error: color type %s not defined",
                        cs->s_name);
                    return 0;
                }
                argc--, argv++;
            }
            else
            {
                pd_error(x, "draw: error: no color type specified");
                return 0;
            }
        }
    }
    if (argc <= 0)
        *type = CT_NULL;
    else if (argc == 1)
    {
        if (argv->a_type == A_SYMBOL)
        {
            *type = CT_SYM;
            fielddesc_setsymbol_const(cfield,
                atom_getsymbolarg(0, argc, argv));
        }
        else
        {
            pd_error(x, "draw: error: bad argument for fill color");
            return 0;
        }
    }
    else if (argc == 2)
    {
        if (argv->a_type == A_SYMBOL &&
            argv[1].a_type == A_SYMBOL &&
            atom_getsymbolarg(0, argc, argv) == &s_symbol)
        {
            *type = CT_SYM;
            argc--, argv++;
            fielddesc_setsymbolarg(cfield, argc, argv);
        }
        else
        {
            pd_error(x, "draw: error: bad arguments for fill color");
            return 0;
        }
    }
    else if (argc == 3)
    {
        int i, var = 0;
        t_fielddesc *fd = cfield;
        /* if there's a color variable field we have to recalculate
           it each redraw in draw_vis */
        for (i = 0; i < argc; i++)
            var = (argv[i].a_type == A_SYMBOL) ? 1 : var;
        /* go ahead and set the fields */
        fielddesc_setfloatarg(fd, argc--, argv++);
        fielddesc_setfloatarg(fd+1, argc--, argv++);
        fielddesc_setfloatarg(fd+2, argc--, argv++);
        if (!var)
        {
            /* if all fields are constants, we can go ahead
               and cache the hex string here. A bit of a hack
               since svg_get_color expects a t_template* 
               and a t_word*. But those aren't used for
               constants so we can get away with it... */
            char *col = svg_get_color(fd, *type, 0, 0);
            fielddesc_setsymbol_const(cfield, gensym(col));
            *type = CT_SYM;
        }
    }
    else
    {
        pd_error(x, "draw: error: bad arguments for fill color");
        return 0;
    }
    return 1;
}

void svg_fill(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    svg_set_color(x, &x->x_filltype, x->x_fill, s, argc, argv);
    svg_update(x, s);
}

void svg_stroke(t_svg *x, t_symbol *s, t_int argc, t_atom *argv)
{
    svg_set_color(x, &x->x_stroketype, x->x_stroke, s, argc, argv);
    svg_update(x, s);
}

/* "drag" is a convenience method-- to use it the user must turn on the
   "mousedown" event, too. */
void svg_drag(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc > 0 && (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL))
    {
        fielddesc_setfloatarg(&x->x_drag, argc, argv);
    }
}

void svg_event(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc > 0 && (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL))
    {
        if (s == gensym("focusin"))
        {
            fielddesc_setfloatarg(&x->x_events.e_focusin.a_attr, argc, argv);
            x->x_events.e_focusin.a_flag = 1;
        }
        else if (s == gensym("focusout"))
        {
            fielddesc_setfloatarg(&x->x_events.e_focusout.a_attr, argc, argv);
            x->x_events.e_focusout.a_flag = 1;
        }
        else if (s == gensym("activate"))
        {
            fielddesc_setfloatarg(&x->x_events.e_activate.a_attr, argc, argv);
            x->x_events.e_activate.a_flag = 1;
        }
        else if (s == gensym("click"))
        {
            fielddesc_setfloatarg(&x->x_events.e_click.a_attr, argc, argv);
            x->x_events.e_click.a_flag = 1;
        }
        else if (s == gensym("mousedown"))
        {
            fielddesc_setfloatarg(&x->x_events.e_mousedown.a_attr, argc, argv);
            x->x_events.e_mousedown.a_flag = 1;
        }
        else if (s == gensym("mouseup"))
        {
            fielddesc_setfloatarg(&x->x_events.e_mouseup.a_attr, argc, argv);
            x->x_events.e_mouseup.a_flag = 1;
        }
        else if (s == gensym("mouseover"))
        {
            fielddesc_setfloatarg(&x->x_events.e_mouseover.a_attr, argc, argv);
            x->x_events.e_mouseover.a_flag = 1;
        }
        else if (s == gensym("mousemove"))
        {
            fielddesc_setfloatarg(&x->x_events.e_mousemove.a_attr, argc, argv);
            x->x_events.e_mousemove.a_flag = 1;
        }
        else if (s == gensym("mouseout"))
        {
            fielddesc_setfloatarg(&x->x_events.e_mouseout.a_attr, argc, argv);
            x->x_events.e_mouseout.a_flag = 1;
        }
        else if (s == gensym("mouseenter"))
        {
            fielddesc_setfloatarg(&x->x_events.e_mouseenter.a_attr, argc, argv);
            x->x_events.e_mouseenter.a_flag = 1;
        }
        else if (s == gensym("mouseleave"))
        {
            fielddesc_setfloatarg(&x->x_events.e_mouseleave.a_attr, argc, argv);
            x->x_events.e_mouseleave.a_flag = 1;
        }
        else if (s == gensym("drag"))
        {
            fielddesc_setfloatarg(&x->x_events.e_drag.a_attr, argc, argv);
            x->x_events.e_drag.a_flag = 1;
        }
        svg_update(x, s);
    }
}

void svg_r(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_type != gensym("circle"))
    {
        pd_error(x, "draw: %s: no method for 'r'", x->x_type->s_name);
        return;
    }
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        t_fielddesc *fd = x->x_vec;
        fielddesc_setfloatarg(fd+2, argc, argv);
        svg_update(x, s);
    }
}

/* The svg spec actually says that the rect shouldn't be rendered if
   height or width = 0. Current behavior is to draw a straight line,
   and only fail to draw if both are 0. Also, svg spec says to handle
   negative values as an error-- most of Pd doesn't do that so instead
   I just bash to zero. */
void svg_rectpoints(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_type == gensym("rect"))
    {
        if (argc)
        {
            int i;
            t_atomtype type = argv[0].a_type;
            if (s == gensym("x")) i = 0;
            else if (s == gensym("y")) i = 1;
            else if (s == gensym("width")) i = 2;
            else i = 3; /* height */
            if (type == A_FLOAT || type == A_SYMBOL)
            {
                t_fielddesc *fd = x->x_vec;
                /* don't allow negative height/width */
                if (type == A_FLOAT && i > 1 && argv[0].a_w.w_float < 0)
                    fielddesc_setfloat_const(fd + i, 0);
                else
                    fielddesc_setfloatarg(fd + i, argc, argv);
                /* just piggyback on the "points" message */
                svg_update(x, gensym("points"));
            }
        }
    }
    else
    {
        pd_error(x, "draw: %s: no  for '%s'",
            x->x_type->s_name, s->s_name);
        return;
    }
}

/* selectively do bbox calculation: 0 = off, 1 = on, instance variable per
   instance */
void svg_bbox(t_svg *x, t_symbol *s, t_int argc, t_atom *argv)
{
    if (argc > 0 && (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL))
    {
        fielddesc_setfloatarg(&x->x_bbox, argc, argv);
        svg_update(x, s);
    }
}

void svg_ellipsepoints(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_type == gensym("circle") || x->x_type == gensym("ellipse"))
    {
        if (argc)
        {
            int i;
            if (s == gensym("cx")) i = 0;
            else i = 1;
            if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
            {
                t_fielddesc *fd = x->x_vec;
                fielddesc_setfloatarg(fd + i, argc, argv);
                /* just piggybacking on the "points" message */
                svg_update(x, gensym("points"));
            }
        }
    }
    else
    {
        pd_error(x, "draw: %s: no method for '%s'",
            x->x_type->s_name, s->s_name);
        return;
    }
}

void svg_linepoints(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    int i = 0;
    if (x->x_type == gensym("line"))
    {
        if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
        {
            if (s == gensym("x1")) i = 0;
            else if (s == gensym("y1")) i = 1;
            else if (s == gensym("x2")) i = 2;
            else if (s == gensym("y2")) i = 3;
            t_fielddesc *fd = x->x_vec;
            fielddesc_setfloatarg(fd + i, argc, argv);
            /* just simulate the "points" method */
            svg_update(x, gensym("points"));
        }
    }
    else
    {
        pd_error(x, "draw: %s: no method for '%s'",
            x->x_type->s_name, s->s_name);
        return;
    }
}

void svg_viewbox(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc)
    {
        t_svg_attr *vbx = x->x_viewbox;
        svg_attr_setfloatarg(vbx++, argc--, argv++);

        if (argc) svg_attr_setfloatarg(vbx++, argc--, argv++);
        else svg_attr_setfloat_const(vbx++, 0);

        if (argc) svg_attr_setfloatarg(vbx++, argc--, argv++);
        else svg_attr_setfloat_const(vbx++, 0);

        if (argc) svg_attr_setfloatarg(vbx, argc, argv);
        else svg_attr_setfloat_const(vbx, 0);
    }
    else
    {
        x->x_viewbox->a_flag = 0;
    }
    svg_update(x, gensym("viewbox"));
}

static int minv(t_float a[][3], t_float b[][3])
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
static void mmult(t_float a[][3], t_float b[][3], t_float c[][3])
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

static void mset(t_float mtx[][3], t_float m1, t_float m2, t_float m3,
    t_float m4, t_float m5, t_float m6)
{
    mtx[0][0] = m1; mtx[1][0] = m2; mtx[0][1] = m3;
    mtx[1][1] = m4; mtx[0][2] = m5; mtx[1][2] = m6;
    mtx[2][0] = 0;  mtx[2][1] = 0;  mtx[2][2] = 1;
}

/* not sure if this is useful... */
static void mget(t_float mtx[][3], t_float *mp1, t_float *mp2, t_float *mp3,
    t_float *mp4, t_float *mp5, t_float *mp6)
{
    *mp1 = mtx[0][0]; *mp2 = mtx[1][0]; *mp3 = mtx[0][1];
    *mp4 = mtx[1][1]; *mp5 = mtx[0][2]; *mp6 = mtx[1][2];
}

void svg_parsetransform(t_svg *x, t_template *template, t_word *data,
    t_float *mp1, t_float *mp2, t_float *mp3,
    t_float *mp4, t_float *mp5, t_float *mp6)
{
    /* parse the args */
    t_symbol *type;
    t_float m[3][3];
    t_float m2[3][3];
    mset(m, 1, 0, 0, 1, 0, 0); /* init to the identity matrix... */
    mset(m2, 0, 0, 0, 0, 0, 0);
    int argc = x->x_transform_n;
    t_fielddesc *fd = x->x_transform;
    /* should probably change this to argc > 0 since a screwup
       could land us in negativeland */
    while (argc > 0)
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
           t_float tx = fielddesc_getcoord(fd++, template, data, 0);
           argc--;
           t_float ty = fielddesc_getcoord(fd++, template, data, 0);
           argc--;
           mset(m2, 1, 0, 0, 1, tx, ty);
           mmult(m, m2, m);
        }
        else if (type == gensym("scale"))
        {
           t_float sx = fielddesc_getcoord(fd++, template, data, 0);
           argc--;
           t_float sy = sx;
           if (argc && fd->fd_type == A_FLOAT)
           {
               sy = fielddesc_getcoord(fd++, template, data, 0);
               argc--;
           }
           mset(m2, sx, 0, 0, sy, 0, 0);
           mmult(m, m2, m);
        }
        /* cx and cy are optional */
        /* this doesn't jibe with glist_xtopixels, ytopixels, etc. */
        else if (type == gensym("rotate"))
        {
            /* we need to convert degrees to radians for the matrix */
            t_float a = (fielddesc_getcoord(fd++, template, data, 0)) *
                3.14159 / 180;
            argc--;
            t_float cx = 0, cy = 0;
            if (argc && fd->fd_type == A_FLOAT)
            {
                cx = fielddesc_getcoord(fd++, template, data, 0);
                argc--;
            }
            if (argc && fd->fd_type == A_FLOAT)
            {
                cy = fielddesc_getcoord(fd++, template, data, 0);
                argc--;
            }
            mset(m2, cos(a), sin(a), sin(a) * -1, cos(a),
                      sin(a) * cy + cx * -1 * cos(a) + cx,
                      sin(a) * cx * -1 + cos(a) * cy * -1 + cy);
            mmult(m, m2, m);
        }
        else if (type == gensym("skewx"))
        {
            t_float a = fielddesc_getcoord(fd++, template, data, 0) *
                3.14159 / 180;
            argc--;
            mset(m2, 1, 0, tan(a), 1, 0, 0);
            mmult(m, m2, m);
        }
        else if (type == gensym("skewy"))
        {
            t_float a = fielddesc_getcoord(fd++, template, data, 0) *
                3.14159 / 180;
            argc--;
            mset(m2, 1, tan(a), 0, 1, 0, 0);
            mmult(m, m2, m);
        }
        else if (type == gensym("matrix"))
        {
            t_float a, b, c, d, e, f;
            a = fielddesc_getcoord(fd++, template, data, 0); argc--;
            b = fielddesc_getcoord(fd++, template, data, 0); argc--;
            c = fielddesc_getcoord(fd++, template, data, 0); argc--;
            d = fielddesc_getcoord(fd++, template, data, 0); argc--;
            e = fielddesc_getcoord(fd++, template, data, 0); argc--;
            f = fielddesc_getcoord(fd++, template, data, 0); argc--;
            mset(m2, a, b, c, d, e, f);
            mmult(m, m2, m);
        }
    }
    t_float a1, a2, a3, a4, a5, a6;
    mget(m, &a1, &a2, &a3, &a4, &a5, &a6);
    *mp1 = a1;
    *mp2 = a2;
    *mp3 = a3;
    *mp4 = a4;
    *mp5 = a5;
    *mp6 = a6;
}

static void svg_dogroupmtx(t_canvas *c, t_template *template, t_word *data,
    t_float mtx[][3])
{
    t_float mtx2[3][3];
    t_float m1, m2, m3, m4, m5, m6;
    t_canvas *parent_group = c->gl_owner;
    if (parent_group && parent_group->gl_svg)
        svg_dogroupmtx(parent_group, template, data, mtx);
    t_svg *svg = (t_svg *)c->gl_svg;
    if (svg->x_transform)
    {
        svg_parsetransform(svg, template, data, &m1, &m2, &m3, &m4, &m5, &m6);
        mset(mtx2, m1, m2, m3, m4, m5, m6);
    }
    else
    {
        mset(mtx2, 1, 0, 0, 1, 0, 0);
    }
    mmult(mtx, mtx2, mtx);
}

/* should this return an int to signal something? */
static void svg_groupmtx(t_svg *x, t_template *template, t_word *data,
    t_float mtx[][3])
{
    t_canvas *c = svg_parentcanvas(x);
    mset(mtx, 1, 0, 0, 1, 0, 0);
    /* if the t_svg isn't inside a group,
       then we're done.  Otherwise we do
       matrix multiplications from the
       outermost group down. */
    if (c->gl_owner && c->gl_svg)
        svg_dogroupmtx(c, template, data, mtx);
}

void svg_group_pathrect_cache(t_svg *x, int state)
{
    t_gobj *y;
    t_canvas *c = (t_canvas *)x->x_parent;
    for (y = c->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == canvas_class &&
            ((t_canvas *)y)->gl_svg)
        {
            svg_group_pathrect_cache((t_svg *)(((t_canvas *)y)->gl_svg), state);
        }
        /* todo: probably need to recurse down into subgroups */
        if (pd_class(&y->g_pd) == draw_class)
        {
            t_svg *a = (t_svg *)(((t_draw *)y)->x_attr);
            if (a->x_pathrect_cache != -1)
                a->x_pathrect_cache = state;
        }
    } 
}

void svg_transform(t_svg *x, t_symbol *s, int argc, t_atom *argv)
{
    /* probably need to do error checking here */
    t_fielddesc *fd;
    x->x_transform = (t_fielddesc *)t_resizebytes(x->x_transform,
        x->x_ndash * sizeof(*x->x_transform),
        argc * sizeof(*x->x_transform));
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
                x->x_type == gensym("path"))
                    x->x_pathrect_cache = -1;
            fielddesc_setfloatarg(fd++, argc--, argv++);
        }
    }
    svg_update(x, s);
}

/* -------------------- widget behavior for draw ------------ */

/* from Raphael.js lib */
static void svg_q2c(t_float x1, t_float y1, t_float *cx1, t_float *cy1,
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
static void svg_findDotAtSegment(t_float p1x, t_float p1y,
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
static void svg_curvedim(t_float p1x, t_float p1y,
    t_float c1x, t_float c1y, t_float c2x, t_float c2y,
    t_float p2x, t_float p2y, t_float *xmin, t_float *ymin,
    t_float *xmax, t_float *ymax, t_float mtx1[][3])
{
    t_float mtx2[3][3];
    int i;
    t_float a = (c2x - 2 * c1x + p1x) - (p2x - 2 * c2x + c1x);
    t_float b = 2 * (c1x - p1x) - 2 * (c2x - c1x),
            c = p1x - c1x;
    t_float t1 = (a ? ((-b + sqrt(abs(b * b - 4 * a * c))) / 2.0 / a) : 0),
            t2 = (a ? ((-b - sqrt(abs(b * b - 4 * a * c))) / 2.0 / a) : 0);
    t_float xy[12];
    xy[0] = p1x; xy[1] = p1y; xy[2] = p2x; xy[3] = p2y;

    /* mtx mult */
    mset(mtx2, p1x, p1y, p2x, p2y, 0, 0);
    mtx2[2][0] = 1; mtx2[2][1] = 1;
    mmult(mtx1, mtx2, mtx2);
    xy[0] = mtx2[0][0]; xy[1] = mtx2[1][0]; xy[2] = mtx2[0][1]; xy[3] = mtx2[1][1];
    int xyc = 4;
    t_float dotx, doty;
    if (abs(t1) > 1e12) t1 = 0.5;
    if (abs(t2) > 1e12) t2 = 0.5;
    if (t1 > 0 && t1 < 1)
    {
        svg_findDotAtSegment(p1x, p1y, c1x, c1y, c2x, c2y, p2x, p2y, t1, &dotx, &doty);
        mset(mtx2, dotx, doty, 0, 0, 0, 0);
        mtx2[2][0] = 1;
        mmult(mtx1, mtx2, mtx2);
        dotx = mtx2[0][0];
        doty = mtx2[1][0];
        xy[xyc++] = dotx;
        xy[xyc++] = doty;
    }
    if (t2 > 0 && t2 < 1)
    {
        svg_findDotAtSegment(p1x, p1y, c1x, c1y, c2x, c2y, p2x, p2y, t2, &dotx, &doty);
        mset(mtx2, dotx, doty, 0, 0, 0, 0);
        mtx2[2][0] = 1;
        mmult(mtx1, mtx2, mtx2);
        dotx = mtx2[0][0];
        doty = mtx2[1][0];
        xy[xyc++] = dotx;
        xy[xyc++] = doty;
    }
    a = (c2y - 2 * c1y + p1y) - (p2y - 2 * c2y + c1y);
    b = 2 * (c1y - p1y) - 2 * (c2y - c1y);
    c = p1y - c1y;
    t1 = (a ? ((-b + sqrt(abs(b * b - 4 * a * c))) / 2.0 / a) : 0);
    t2 = (a ? ((-b - sqrt(abs(b * b - 4 * a * c))) / 2.0 / a) : 0);
    if (abs(t1) > 1e12) t1 = 0.5;
    if (abs(t2) > 1e12) t2 = 0.5;
    if (t1 > 0 && t1 < 1)
    {
        svg_findDotAtSegment(p1x, p1y, c1x, c1y, c2x, c2y, p2x, p2y, t1, &dotx, &doty);
        mset(mtx2, dotx, doty, 0, 0, 0, 0);
        mtx2[2][0] = 1;
        mmult(mtx1, mtx2, mtx2);
        dotx = mtx2[0][0];
        doty = mtx2[1][0];
        xy[xyc++] = dotx;
        xy[xyc++] = doty;
    }
    if (t2 > 0 && t2 < 1)
    {
        svg_findDotAtSegment(p1x, p1y, c1x, c1y, c2x, c2y, p2x, p2y, t2, &dotx, &doty);
        mset(mtx2, dotx, doty, 0, 0, 0, 0);
        mtx2[2][0] = 1;
        mmult(mtx1, mtx2, mtx2);
        dotx = mtx2[0][0];
        doty = mtx2[1][0];
        xy[xyc++] = dotx;
        xy[xyc++] = doty;
    }

    *xmin = *ymin = 0x7fffffff;
    *xmax = *ymax = -0x7fffffff;
    for (i = 0; i < xyc; i+=2)
    {
        if (xy[i] < *xmin) *xmin = xy[i];
        if (xy[i] > *xmax) *xmax = xy[i];
        if (xy[i+1] < *ymin) *ymin = xy[i+1];
        if (xy[i+1] > *ymax) *ymax = xy[i+1];
    }
}

static t_float svg_getangle(t_float bx, t_float by)
{
  t_float pi = (t_float)3.14159265358979323846;
  t_float divisor = sqrt(bx * bx + by * by);
  return fmod(2*pi + (by > 0.0 ? 1.0 : -1.0) *
              acos( divisor? (bx / divisor) : 0 ), 2*pi);
}

void svg_arc2bbox(t_float x1, t_float y1, t_float rx, t_float ry,
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
    double radicant = (rx*rx*ry*ry - rx*rx*y1prime*y1prime -
        ry*ry*x1prime*x1prime);
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
        txmin = svg_getangle(-rx, 0);
        *xmax = cx + rx;
        txmax = svg_getangle(rx, 0);
        *ymin = cy - ry;
        tymin = svg_getangle(0, -ry);
        *ymax = cy + ry;
        tymax = svg_getangle(0, ry);
    }
    else if (phi == pi / 2.0 || phi == 3.0*pi/2.0)
    {
        *xmin = cx - ry;
        txmin = svg_getangle(-ry, 0);
        *xmax = cx + ry;
        txmax = svg_getangle(ry, 0);
        *ymin = cy - rx;
        tymin = svg_getangle(0, -rx);
        *ymax = cy + rx;
        tymax = svg_getangle(0, rx);
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
        txmin = svg_getangle(*xmin - cx, tmpY - cy);
        tmpY = cy + rx*cos(txmax)*sin(phi) + ry*sin(txmax)*cos(phi);
        txmax = svg_getangle(*xmax - cx, tmpY - cy);
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
        tymin = svg_getangle(tmpX - cx, *ymin - cy);
        tmpX = cx + rx*cos(tymax)*cos(phi) - ry*sin(tymax)*sin(phi);
        tymax = svg_getangle(tmpX - cx, *ymax - cy);
    }
    t_float angle1 = svg_getangle(x1 - cx, y1 - cy);
    t_float angle2 = svg_getangle(x2 - cx, y2 - cy);
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
    if ((!other_arc && (angle1 > txmin || angle2 < txmin)) ||
        (other_arc && !(angle1 > txmin || angle2 < txmin)))
        *xmin = x1 < x2 ? x1 : x2;
    if ((!other_arc && (angle1 > txmax || angle2 < txmax)) ||
        (other_arc && !(angle1 > txmax || angle2 < txmax)))
        *xmax = x1 > x2 ? x1 : x2;
    if ((!other_arc && (angle1 > tymin || angle2 < tymin))
        || (other_arc && !(angle1 > tymin || angle2 < tymin)))
        *ymin = y1 < y2 ? y1 : y2;
    if ((!other_arc && (angle1 > tymax || angle2 < tymax)) ||
        (other_arc && !(angle1 > tymax || angle2 < tymax)))
        *ymax = y1 > y2 ? y1 : y2;
}

    /* get bbox for a path, based very roughly on
       Raphael.js "pathbbox" function.  Too complex to finish
       here, but maybe this could eventually get merged in to
       tkpath-- it will probably give a more accurate result... */
static void svg_getpathrect(t_svg *x, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    /* todo: revisit the attr from Raphael for processPath. */ 
    t_float path2_vec[x->x_nargs];
    char path2cmds[x->x_npathcmds];
    t_float mtx1[3][3];
    t_float mtx2[3][3];
    t_float m1, m2, m3, m4, m5, m6;
    svg_groupmtx(x, template, data, mtx1);
    svg_parsetransform(x, template, data, &m1, &m2, &m3,
        &m4, &m5, &m6);
    mset(mtx2, m1, m2, m3, m4, m5, m6);
    mmult(mtx1, mtx2, mtx1);
    
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
                if (x->x_nargs_per_cmd[i] < 7)
                    break;
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
            if (x->x_nargs_per_cmd[i] > 0)
                mx = fielddesc_getcoord(fd, template, data, 1) + (rel? xx : 0);
            if (x->x_nargs_per_cmd[i] > 1)
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
            if (x->x_nargs_per_cmd[i])
                xx = *ia;
            break;
        case 'V':
            if (x->x_nargs_per_cmd[i])
                yy = *ia;
            break;
        case 'M':
            if (x->x_nargs_per_cmd[i] > 1)
            {
                mx = *(ia);
                my = *(ia+1);
            }
        default:
            if (x->x_nargs_per_cmd[i] > 1)
            {
                xx = *(ia+(x->x_nargs_per_cmd[i] - 2));
                yy = *(ia+(x->x_nargs_per_cmd[i] - 1));
            }
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
            for (j = 0; j < x->x_nargs_per_cmd[i]; j += 7)
            {
                if (x->x_nargs_per_cmd[i] < 7)
                    break;
                svg_arc2bbox(xprev, yprev, *ia, *(ia+1), *(ia+2), *(ia+3),
                    *(ia+4), *(ia+5), *(ia+6), &x1, &y1, &x2, &y2);
                xprev = *(ia+5);
                yprev = *(ia+6);
                mset(mtx2, x1, y1, x2, y2, 0, 0);
                mtx2[2][0] = 1; mtx2[2][1] = 1;
                mmult(mtx1, mtx2, mtx2);
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
                mset(mtx2, x1, y2, x2, y1, 0, 0);
                mtx2[2][0] = 1; mtx2[2][1] = 1;
                mmult(mtx1, mtx2, mtx2);
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
            }
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
                svg_curvedim(xprev, yprev,
                    nx, ny, *ia+j, *(ia+j+1), *(ia+j+2), *(ia+j+3),
                    &x1, &y1, &x2, &y2, mtx1);
                xprev = *(ia+j+2);
                yprev = *(ia+j+3);
                bx = *ia+j;
                by = *(ia + j + 1);

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
                t_float cx1 = qxprev, cy1 = qyprev, cx2 = *ia+j,
                    cy2 = *(ia+j+1), cx, cy;
                svg_q2c(xprev, yprev, &cx1, &cy1, &cx2, &cy2, &cx, &cy);
                svg_curvedim(xprev, yprev, cx1, cy1, cx2, cy2, cx, cy,
                    &x1, &y1, &x2, &y2, mtx1);
                xprev = *ia+j;
                yprev = *(ia+j+1);

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
                t_float cx1 = *ia+j, cy1 = *(ia+j+1),
                    cx2 = *(ia+j+2), cy2 = *(ia+j+3), cx, cy;
                svg_q2c(xprev, yprev, &cx1, &cy1, &cx2, &cy2, &cx, &cy);
                svg_curvedim(xprev, yprev, cx1, cy1, cx2, cy2, cx, cy,
                    &x1, &y1, &x2, &y2, mtx1);
                xprev = *(ia+j+2);
                yprev = *(ia+j+3);

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
                svg_curvedim(xprev, yprev, *(ia+j), *(ia+j+1), *(ia+j+2),
                    *(ia+j+3), *(ia+j+4), *(ia+j+5), &x1, &y1, &x2, &y2, mtx1);
                xprev = *(ia+j+4);
                yprev = *(ia+j+5);
                bx = *(ia+j+2);
                by = *(ia+j+3);

                if (x1 == 0x7fffffff && y1 == 0x7fffffff &&
                    x2 == -0x7fffffff && y2 == -0x7fffffff)
                        break;
                finalx1 = x1 < finalx1 ? x1 : finalx1;
                finalx2 = x2 > finalx2 ? x2 : finalx2;
                finaly1 = y1 < finaly1 ? y1 : finaly1;
                finaly2 = y2 > finaly2 ? y2 : finaly2;
            }
            break;
        case 'V':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
            {
                mset(mtx2, xprev, *ia+j, 0, 0, 0, 0);
                mtx2[2][0] = 1;
                mmult(mtx1, mtx2, mtx2);
                tmpy = mtx2[1][0];
                finaly1 = tmpy < finaly1 ? tmpy : finaly1;
                finaly2 = tmpy > finaly2 ? tmpy : finaly2;
                yprev = *ia+j;
            }
            break;
        case 'H':
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
            {
                mset(mtx2, *ia+j, yprev, 0, 0, 0, 0);
                mtx2[2][0] = 1;
                mmult(mtx1, mtx2, mtx2);
                tmpx = mtx2[0][0];  
                finalx1 = tmpx < finalx1 ? tmpx : finalx1;
                finalx2 = tmpx > finalx2 ? tmpx : finalx2;
                xprev = *ia+j;
            }
            break;
        default:
            for (j = 0; j < x->x_nargs_per_cmd[i]; j += 2)
            {
                    /* hack */
                    if ((j + 1) >= x->x_nargs_per_cmd[i])
                        break;
                    mset(mtx2, *(ia+j), *(ia+j+1), 0, 0, 0, 0);
                    mtx2[2][0] = 1;
                    mmult(mtx1, mtx2, mtx2);
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
    //finalx1 = glist_xtopixels(glist, basex + finalx1);
    //finalx2 = glist_xtopixels(glist, basex + finalx2);
    //finaly1 = glist_ytopixels(glist, basey + finaly1);
    //finaly2 = glist_ytopixels(glist, basey + finaly2);
    *xp1 = (int)(finalx1 + basex);
    *xp2 = (int)(finalx2 + basex);
    *yp1 = (int)(finaly1 + basey);
    *yp2 = (int)(finaly2 + basey);
}

static void svg_getrectrect(t_svg *x, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_float width, height, xoff, yoff;
    t_float x1, y1, x2, y2;
    x1 = y1 = 0x7fffffff;
    x2 = y2 = -0x7fffffff;

    t_float mtx1[3][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
    t_float mtx2[3][3] = { {1, 0, 0}, {0, 1, 0}, {1, 0, 1} };
    t_float tx1, ty1, tx2, ty2, t5, t6;
    if (!fielddesc_getfloat(&x->x_bbox, template, data, 0) ||
        (x->x_vis.a_flag && !fielddesc_getfloat(&x->x_vis.a_attr,
            template, data, 0)))
    {
        *xp1 = *yp1 = 0x7fffffff;
        *xp2 = *yp2 = -0x7fffffff;
        return;
    }

    svg_groupmtx(x, template, data, mtx1);
    width = fielddesc_getcoord(&x->x_width.a_attr, template, data, 0);
    height = fielddesc_getcoord(&x->x_height.a_attr, template, data, 0);
    xoff = fielddesc_getcoord(&x->x_x.a_attr, template, data, 0);
    yoff = fielddesc_getcoord(&x->x_y.a_attr, template, data, 0);
 
    mset(mtx2, xoff, yoff, xoff + width, yoff + height, 0, 0);
    mtx2[2][0] = 1; mtx2[2][1] = 1;
    mmult(mtx1, mtx2, mtx2);
    mget(mtx2, &tx1, &ty1, &tx2, &ty2, &t5, &t6);
    if (tx1 < x1) x1 = tx1;
    if (tx2 < x1) x1 = tx2;
    if (ty1 < y1) y1 = ty1;
    if (ty2 < y1) y1 = ty2;
    if (tx1 > x2) x2 = tx1;
    if (tx2 > x2) x2 = tx2;
    if (ty1 > y2) y2 = ty1;
    if (ty2 > y2) y2 = ty2;
    mset(mtx2, xoff, yoff + height, xoff + width, yoff, 0, 0);
    mtx2[2][0] = 1; mtx2[2][1] = 1;
    mmult(mtx1, mtx2, mtx2);
    mget(mtx2, &tx1, &ty1, &tx2, &ty2, &t5, &t6);
    if (tx1 < x1) x1 = tx1;
    if (tx2 < x1) x1 = tx2;
    if (ty1 < y1) y1 = ty1;
    if (ty2 < y1) y1 = ty2;
    if (tx1 > x2) x2 = tx1;
    if (tx2 > x2) x2 = tx2;
    if (ty1 > y2) y2 = ty1;
    if (ty2 > y2) y2 = ty2;
    //x1 = glist_xtopixels(glist, basex + x1);
    //x2 = glist_xtopixels(glist, basex + x2);
    //y1 = glist_ytopixels(glist, basey + y1);
    //y2 = glist_ytopixels(glist, basey + y2);

    x1 = basex + x1;
    x2 = basex + x2;
    y1 = basey + y1;
    y2 = basey + y2;

    /* todo: put these up top */
    if (!fielddesc_getfloat(&x->x_vis.a_attr, template, data, 0))
    {
        *xp1 = *yp1 = 0x7fffffff;
        *xp2 = *yp2 = -0x7fffffff;
        return;
    }
    *xp1 = (int)x1;
    *yp1 = (int)y1;
    *xp2 = (int)x2;
    *yp2 = (int)y2;
}

void scalar_getinnersvgrect(t_gobj *z, t_glist *owner, t_word *data,
    t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_canvas *c = (t_canvas *)z;
    svg_getrectrect((t_svg *)c->gl_svg,
        owner, data, template, basex, basey, xp1, yp1, xp2, yp2);
}

static void draw_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_draw *x = (t_draw *)z;
    t_svg *sa = (t_svg *)x->x_attr;

    /* So in the svg spec, the "display" attribute doesn't actually
       calculate a bbox, whereas the "visibility" still calcs the bbox.
       tkpath doesn't have a function for "display" so currently "vis"
       is filling in for it

    */
    if (!fielddesc_getfloat(&sa->x_vis.a_attr, template, data, 0) ||
        !fielddesc_getfloat(&sa->x_bbox, template, data, 0) ||
        (sa->x_type == gensym("g")))
    {
        *xp1 = *yp1 = 0x7fffffff;
        *xp2 = *yp2 = -0x7fffffff;
        return;
    }

    if (sa->x_pathrect_cache == 1)
    {
        //*xp1 = glist_xtopixels(glist, basex + sa->x_x1);
        //*xp2 = glist_xtopixels(glist, basex + sa->x_x2);
        //*yp1 = glist_ytopixels(glist, basey + sa->x_y1);
        //*yp2 = glist_ytopixels(glist, basey + sa->x_y2);

        *xp1 = basex + sa->x_x1;
        *xp2 = basex + sa->x_x2;
        *yp1 = basey + sa->x_y1;
        *yp2 = basey + sa->x_y2;

        return;
    }

    t_float mtx1[3][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
    t_float mtx2[3][3] = { {1, 0, 0}, {0, 1, 0}, {1, 0, 1} };
    t_float m1, m2, m3, m4, m5, m6;
    
    //fprintf(stderr,"draw_getrect\n");
    int i, n = sa->x_nargs;
    t_fielddesc *f = sa->x_vec;
    int x1 = 0x7fffffff, x2 = -0x7fffffff, y1 = 0x7fffffff, y2 = -0x7fffffff;

    svg_groupmtx(sa, template, data, mtx1);
    if (sa->x_type == gensym("path"))
    {
        /* this could get very expensive with complex paths--
        that is why there's a caching mechanism */
        svg_getpathrect(sa, glist, data, template, basex, basey,
            &x1, &y1, &x2, &y2);
    }
    else if (sa->x_type == gensym("polyline") ||
             sa->x_type == gensym("polygon"))
    {
        svg_parsetransform(sa, template, data, &m1, &m2, &m3,
            &m4, &m5, &m6);
        mset(mtx2, m1, m2, m3, m4, m5, m6);
        mmult(mtx1, mtx2, mtx1);

        for (i = 0, f = sa->x_vec; i < n; i+=2, f+=2)
        {
            t_float xloc = fielddesc_getcoord(f, template, data, 0);
            /* 0 for y coordinate if user forgot to supply one */
            t_float yloc = 
                i+1 < n ? fielddesc_getcoord(f+1, template, data, 0) : 0;

            mset(mtx2, xloc, yloc, 0, 0, 0, 0);
            mtx2[2][0] = 1;
            mmult(mtx1, mtx2, mtx2);
            xloc = mtx2[0][0];
            yloc = mtx2[1][0];

            if (xloc < x1) x1 = xloc;
            if (xloc > x2) x2 = xloc;
            if (yloc < y1) y1 = yloc;
            if (yloc > y2) y2 = yloc;
        }
        if (n)
        {
            //x1 = glist_xtopixels(glist, basex + x1);
            //x2 = glist_xtopixels(glist, basex + x2);
            //y1 = glist_ytopixels(glist, basey + y1);
            //y2 = glist_ytopixels(glist, basey + y2);

            x1 = basex + x1;
            x2 = basex + x2;
            y1 = basey + y1;
            y2 = basey + y2;
        }
    }
    else if (sa->x_type == gensym("rect") ||
             sa->x_type == gensym("circle") ||
             sa->x_type == gensym("ellipse") ||
             sa->x_type == gensym("line"))
    {
        t_float m1, m2, m3, m4, m5, m6; /* matrix */
        t_float xx1, yy1, xx2, yy2;
        t_float tx1, ty1, tx2, ty2, t5, t6; /* transformed points */
        if (sa->x_type == gensym("rect") || sa->x_type == gensym("line"))
        {
            xx1 = fielddesc_getcoord(&sa->x_x.a_attr, template, data, 0);
            yy1 = fielddesc_getcoord(&sa->x_y.a_attr, template, data, 0);
            if (sa->x_type == gensym("rect"))
            {
                t_float rwidth = fielddesc_getcoord(&sa->x_width.a_attr,
                    template, data, 0);
                t_float rheight = fielddesc_getcoord(&sa->x_height.a_attr,
                    template, data, 0);
                xx2 = xx1 + rwidth;
                yy2 = yy1 + rheight;
            }
            else
            {
                xx2 = fielddesc_getcoord(&sa->x_rx.a_attr,
                    template, data, 0);
                yy2 = fielddesc_getcoord(&sa->x_ry.a_attr,
                    template, data, 0);
            }
        }
        else
        {
            /* Yes, I realize this isn't a correct bbox but it's
               late and I'm losing steam... Need to just port Raphael's
               path bbox method to c, then convert ellipses to paths... */
            t_float cx = fielddesc_getcoord(&sa->x_x.a_attr,
                template, data, 0);
            t_float cy = fielddesc_getcoord(&sa->x_y.a_attr, template, data, 0);
            t_float rx = fielddesc_getcoord(&sa->x_rx.a_attr, template, data, 0);
            t_float ry;
            if (sa->x_type == gensym("circle"))
                ry = rx;
            else
                ry = fielddesc_getcoord(&sa->x_ry.a_attr, template,
                    data, 0);
            xx1 = cx;
            yy1 = cy;
            xx2 = rx;
            yy2 = ry;
        }
        svg_parsetransform(sa, template, data, &m1, &m2, &m3, &m4, &m5, &m6);
        mset(mtx2, m1, m2, m3, m4, m5, m6);
        mmult(mtx1, mtx2, mtx1);
        /* There's probably a much easier way to do this.  I'm just
           setting the first two columns to x/y points to get them
           transformed.  Since the shapes could be crazy skewed/rotated
           I have to check each coordinate of the rect, so I do it again
           below. */
        if (sa->x_type == gensym("rect") || sa->x_type == gensym("line"))
            mset(mtx2, xx1, yy1, xx2, yy1, 0, 0);
        else
            mset(mtx2, xx1, yy1 + yy2, xx1 + xx2, yy1, 0, 0);
        mtx2[2][0] = 1; mtx2[2][1] = 1;
        mmult(mtx1, mtx2, mtx2);
        mget(mtx2, &tx1, &ty1, &tx2, &ty2, &t5, &t6);
        if (tx1 < x1) x1 = tx1;
        if (tx2 < x1) x1 = tx2;
        if (ty1 < y1) y1 = ty1;
        if (ty2 < y1) y1 = ty2;
        if (tx1 > x2) x2 = tx1;
        if (tx2 > x2) x2 = tx2;
        if (ty1 > y2) y2 = ty1;
        if (ty2 > y2) y2 = ty2;
        if (sa->x_type == gensym("rect") || sa->x_type == gensym("line"))
            mset(mtx2, xx2, yy2, xx1, yy2, 0, 0);
        else
            mset(mtx2, xx1, yy1 - yy2, xx1 - xx2, yy1, 0, 0);
        mtx2[2][0] = 1; mtx2[2][1] = 1;
        mmult(mtx1, mtx2, mtx2);
        mget(mtx2, &tx1, &ty1, &tx2, &ty2, &t5, &t6);
        if (tx1 < x1) x1 = tx1;
        if (tx2 < x1) x1 = tx2;
        if (ty1 < y1) y1 = ty1;
        if (ty2 < y1) y1 = ty2;
        if (tx1 > x2) x2 = tx1;
        if (tx2 > x2) x2 = tx2;
        if (ty1 > y2) y2 = ty1;
        if (ty2 > y2) y2 = ty2;
        //x1 = glist_xtopixels(glist, basex + x1);
        //x2 = glist_xtopixels(glist, basex + x2);
        //y1 = glist_ytopixels(glist, basey + y1);
        //y2 = glist_ytopixels(glist, basey + y2);

        x1 = basex + x1;
        x2 = basex + x2;
        y1 = basey + y1;
        y2 = basey + y2;
    }
    if (fielddesc_getfloat(&sa->x_strokewidth.a_attr, template, data, 0))
    {
        int padding = fielddesc_getcoord(&sa->x_strokewidth.a_attr,
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

static void svg_togui(t_svg *x, t_template *template, t_word *data)
{
    // Hack to send parameters to the GUI. Not sure yet if
    // we want to generalize that...
    gui_start_array();
    if (x->x_filltype != CT_NULL)
    {
        gui_s("fill");
        gui_s(svg_get_color(x->x_fill, x->x_filltype, template, data));
    }
    if (x->x_fillopacity.a_flag)
    {
        gui_s("fill-opacity");
        gui_f(fielddesc_getcoord(&x->x_fillopacity.a_attr, template, data, 0));
    }
    if (x->x_fillrule.a_flag)
    {
        gui_s("fill-rule");
        gui_s((int)fielddesc_getfloat(
            &x->x_fillrule.a_attr, template, data, 0) ?
                "evenodd" : "nonzero");
    }
    if (x->x_type == gensym("path"))
    {
        int i;
        /* let's turn off bbox caching so we can recalculate
           the bbox */
        if (x->x_pathrect_cache != -1)
            x->x_pathrect_cache = 0;
        t_fielddesc *f;
        char *cmd, cmdbuf[2];
        int totalpoints = 0; /* running tally */

        gui_s("d");
        gui_start_array();
        /* path parser: no error checking yet */
        for (i = 0, cmd = x->x_pathcmds; i < x->x_npathcmds; i++, cmd++)
        {
            int j;
            f = (x->x_vec)+totalpoints;
            sprintf(cmdbuf, "%c", *(cmd));
            gui_s(cmdbuf);
            for (j = 0; j < x->x_nargs_per_cmd[i]; j++)
                gui_f(fielddesc_getcoord(f+j, template, data, 0));
            totalpoints += x->x_nargs_per_cmd[i];
        }
        gui_end_array();
    }
    if (x->x_stroketype != CT_NULL)
    {
        gui_s("stroke");
        gui_s(svg_get_color(x->x_stroke, x->x_stroketype, template, data));
    }
    if (x->x_strokewidth.a_flag)
    {
        gui_s("stroke-width");
        gui_f(fielddesc_getcoord(&x->x_strokewidth.a_attr, template, data, 0));
    }
    if (x->x_rx.a_flag)
    {
        gui_s(x->x_type == gensym("circle") ? "r" :
            x->x_type == gensym("line") ? "x2" : "rx");
        gui_f(fielddesc_getcoord(&x->x_rx.a_attr, template, data, 0));
    }
    if (x->x_ry.a_flag)
    {
            gui_s(x->x_type == gensym("ellipse") ? "ry" : "y2");
            gui_f(fielddesc_getcoord(&x->x_ry.a_attr, template, data, 0));
    }
    if (x->x_x.a_flag)
    {
        gui_s(x->x_type == gensym("rect") || x->x_type == gensym("svg") ? "x" :
            x->x_type == gensym("line") ? "x1" : "cx");
        gui_f(fielddesc_getcoord(&x->x_x.a_attr, template, data, 0));
    }
    if (x->x_y.a_flag)
    {
        gui_s(x->x_type == gensym("rect") || x->x_type == gensym("svg") ? "y" :
            x->x_type == gensym("line") ? "y1" : "cy");
        gui_f(fielddesc_getcoord(&x->x_y.a_attr, template, data, 0));
    }
    if (x->x_width.a_flag)
    {
        gui_s("width");
        gui_f(fielddesc_getcoord(&x->x_width.a_attr, template, data, 0));
    }
    if (x->x_height.a_flag)
    {
        gui_s("height");
        gui_f(fielddesc_getcoord(&x->x_height.a_attr, template, data, 0));
    }
    if (x->x_type == gensym("line"))
    {
        if (x->x_nargs > 0)
        {
            gui_s("x1");
            gui_f(fielddesc_getcoord(&x->x_vec[0], template, data, 0));
        }
        if (x->x_nargs > 1)
        {
            gui_s("y1");
            gui_f(fielddesc_getcoord(&x->x_vec[1], template, data, 0));
        }
        if (x->x_nargs > 2)
        {
            gui_s("x2");
            gui_f(fielddesc_getcoord(&x->x_vec[2], template, data, 0));
        }
        if (x->x_nargs > 3)
        {
            gui_s("y2");
            gui_f(fielddesc_getcoord(&x->x_vec[3], template, data, 0));
        }
    }
    if (x->x_opacity.a_flag)
    {
        gui_s("opacity");
        gui_f(fielddesc_getcoord(&x->x_opacity.a_attr, template, data, 0));
    }
    if (x->x_type == gensym("polyline") ||
        x->x_type == gensym("polygon"))
    {
        int i;
        if (x->x_nargs)
        {
            gui_s("points");
            gui_start_array();
            for (i = 0; i < x->x_nargs; i++)
                gui_f(fielddesc_getcoord(&x->x_vec[i], template, data, 0));
            gui_end_array();
        }
    }
    if (x->x_strokeopacity.a_flag)
    {
        gui_s("stroke-opacity");
        gui_f(fielddesc_getcoord(&x->x_strokeopacity.a_attr, template, data, 0));
    }
    if (x->x_strokedashoffset.a_flag)
    {
        gui_s("stroke-dashoffset");
        gui_f(fielddesc_getcoord(&x->x_strokedashoffset.a_attr, template, data, 0));
    }
    if (x->x_strokelinecap.a_flag)
    {
        gui_s("stroke-linecap");
        gui_s(get_strokelinecap(
            (int)fielddesc_getcoord(&x->x_strokelinecap.a_attr,
            template, data, 0)));
    }
    if (x->x_strokelinejoin.a_flag)
    {
        gui_s("stroke-linejoin");
        gui_s(get_strokelinejoin(
            (int)fielddesc_getfloat(&x->x_strokelinejoin.a_attr,
        template, data, 0)));
    }
    if (x->x_strokemiterlimit.a_flag)
    {
        gui_s("stroke-miterlimit");
        gui_f(fielddesc_getcoord(&x->x_strokemiterlimit.a_attr,
            template, data, 0));
    }
    if (x->x_ndash)
    {
        int i;
        t_fielddesc *fd;
        gui_s("stroke-dasharray");
        gui_start_array();
        for (i = 0, fd = x->x_strokedasharray; i < x->x_ndash; i++)
        {
            gui_f(fielddesc_getcoord(fd+i, template, data, 0));
        }
        gui_end_array();
    }
    if (x->x_transform_n > 0)
    {
        /* todo: premultiply this */
        t_float m1, m2, m3, m4, m5, m6;
        svg_parsetransform(x, template, data, &m1, &m2, &m3,
            &m4, &m5, &m6);
        gui_s("transform");
        char transbuf[MAXPDSTRING];
        sprintf(transbuf, "matrix(%g,%g,%g,%g,%g,%g)",
            m1, m2, m3, m4, m5, m6);
        gui_s(transbuf);
    }
    if (x->x_vis.a_flag) 
    { 
        gui_s("visibility");
        gui_s(fielddesc_getfloat(&x->x_vis.a_attr, 
            template, data, 0) ? "visible" : "hidden");
    }
    if (x->x_rx.a_flag)
    {
        gui_s("rx");
        gui_f(fielddesc_getcoord(&x->x_rx.a_attr,
            template, data, 0));
    }
    if (x->x_ry.a_flag)
    {
        gui_s("ry");
        gui_f(fielddesc_getcoord(&x->x_ry.a_attr,
            template, data, 0));
    }
    if (x->x_viewbox->a_flag)
    {
        gui_s("viewBox");
        gui_start_array();
        gui_f(fielddesc_getcoord(&x->x_viewbox[0].a_attr,
            template, data, 0));
        gui_f(fielddesc_getcoord(&x->x_viewbox[1].a_attr,
            template, data, 0));
        gui_f(fielddesc_getcoord(&x->x_viewbox[2].a_attr,
            template, data, 0));
        gui_f(fielddesc_getcoord(&x->x_viewbox[3].a_attr,
            template, data, 0));
        gui_end_array();
    }
    // Not sure why display attr is here...
    gui_s("display");
    gui_s("inline");
    gui_end_array();
}

void svg_grouptogui(t_glist *g, t_template *template, t_word *data)
{
    t_svg *x = (t_svg *)g->gl_svg;
    svg_togui(x, template, data);
}

static void draw_vis(t_gobj *z, t_glist *glist, t_glist *parentglist,
    t_scalar *sc, t_word *data, t_template *template,
    t_float basex, t_float basey, t_array *parentarray, int vis)
{
    t_draw *x = (t_draw *)z;
    t_svg *sa = (t_svg *)x->x_attr;

    int n = sa->x_nargs;
    //t_float mtx1[3][3] =  { { 0, 0, 0}, {0, 0, 0}, {0, 0, 1} };
    //t_float mtx2[3][3] =  { { 0, 0, 0}, {0, 0, 0}, {0, 0, 1} };
    /* need to scale some attributes like radii, widths, etc. */
    //t_float xscale = glist_xtopixels(glist, 1) - glist_xtopixels(glist, 0);
    //t_float yscale = glist_ytopixels(glist, 1) - glist_ytopixels(glist, 0);

    /*// get the universal tag for all nested objects
    t_canvas *tag = x->x_canvas;
    while (tag->gl_owner)
    {
        tag = tag->gl_owner;
    }*/
    
        /* see comment in plot_vis() */
    /*if (vis && !fielddesc_getfloat(&sa->x_vis.a_attr, template, data, 0))
        return; */
    if (vis)
    {
        /* Hack to figure out whether we're inside an
           array. See curve_vis comment for more info
           and feel free to revise this to make it a
           more sane approach.

           [plot] doesn't work completely with [group]s yet. But
           as far as I can tell, all the old ds tutorials and
           patches work and look fine.
        */

        int in_array = (sc->sc_vec == data) ? 0 : 1;
        /* This was originally used to limit the number of points to 500.
           We're not doing that anymore, but "n" is used below so I want to
           investigate further before removing it. */
        if (n > 500)
            n = 500;
        /* begin the gui drawing command */
        gui_start_vmess("gui_draw_vis", "xs", glist_getcanvas(glist),
            sa->x_type->s_name);

        /* next send the gui drawing arguments: commands and points
           for paths, points for everything else */

        svg_togui(sa, template, data);

        gui_start_array();
        char parent_tagbuf[MAXPDSTRING];
        sprintf(parent_tagbuf, "dgroup%lx.%lx",
            (in_array ?
                (long unsigned int)parentglist :
                (long unsigned int)x->x_canvas),
            (long unsigned int)data);
        gui_s(parent_tagbuf);
        /* tags - one for this scalar (not sure why the double glist thingy)
          one for this specific draw item
        */
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "draw%lx.%lx",
            (long unsigned int)x,
            (long unsigned int)data);
        gui_s(tagbuf);
        gui_end_array();
        gui_end_vmess();

        /* need to investigate this further-- it apparently handles
           the z order for gop scalars */
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

        /* register events */
        svg_register_events(z, glist, sc, template, data, parentarray);
    }
    else
    {
        if (n > 1)
        {
            char itemtagbuf[MAXPDSTRING];
            sprintf(itemtagbuf, "draw%lx.%lx", (long unsigned int)x,
                (long unsigned int)data);
            gui_vmess("gui_draw_erase_item", "xs", glist_getcanvas(glist),
                itemtagbuf);
        }
                
    }
}

//static int draw_motion_field;
static t_float draw_motion_xcumulative;
//static t_float draw_motion_xbase;
static t_float draw_motion_xper;
static t_float draw_motion_ycumulative;
//static t_float draw_motion_ybase;
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
    t_draw *x = (t_draw *)z;
    t_svg *sa = (t_svg *)x->x_attr;
    t_atom at[4];
    SETFLOAT(at, (t_float)dx);
    SETFLOAT(at+1, (t_float)dy);
    t_float mtx1[3][3];
    t_float mtx2[3][3];
    t_float m1, m2, m3, m4, m5, m6, tdx, tdy;

    /* might use this to output the ctm */
    svg_groupmtx(sa, draw_motion_template, draw_motion_wp, mtx1);
    svg_parsetransform(sa, draw_motion_template, draw_motion_wp,
        &m1, &m2, &m3, &m4, &m5, &m6);
    mset(mtx2, m1, m2, m3, m4, m5, m6);
    mmult(mtx1, mtx2, mtx1);
    minv(mtx1, mtx1);
    /* get rid of translation so it doesn't factor
       in to our deltas */
    mtx1[0][2] = 0;
    mtx1[1][2] = 0;
    mset(mtx2, dx, dy, 0, 0, 0, 0);
    mtx2[2][0] = 1;
    mmult(mtx1, mtx2, mtx2);
    tdx = mtx2[0][0];
    tdy = mtx2[1][0];
    SETFLOAT(at+2, tdx);
    SETFLOAT(at+3, tdy);
    if (!gpointer_check(&draw_motion_gpointer, 0))
    {
        post("draw_motion: scalar disappeared");
        return;
    }
    draw_motion_xcumulative += dx;
    draw_motion_ycumulative += dy;
    //if (f->fd_var && (tdx != 0))
    //{
    //    fielddesc_setcoord(f, draw_motion_template, draw_motion_wp,
    //        draw_motion_xbase + draw_motion_xcumulative * draw_motion_xper,
    //            1); 
    //}
    //if ((f+1)->fd_var && (tdy != 0))
    //{
    //    fielddesc_setcoord(f+1, draw_motion_template, draw_motion_wp,
    //        draw_motion_ybase + draw_motion_ycumulative * draw_motion_yper,
    //            1); 
    //}
        /* LATER figure out what to do to notify for an array? */
    if (draw_motion_scalar)
    {
        draw_notifyforscalar(&x->x_obj, draw_motion_glist, 0, 0,
            draw_motion_scalar, gensym("drag"), 4, at);
        template_notifyforscalar(draw_motion_template, draw_motion_glist, 
            draw_motion_scalar, gensym("change"), 1, at);
    }
    //if (draw_motion_scalar)
    //    scalar_redraw(draw_motion_scalar, draw_motion_glist);
    //if (draw_motion_array)
    //    array_redraw(draw_motion_array, draw_motion_glist);
}

/*
static void draw_motion(void *z, t_floatarg dx, t_floatarg dy)
{
    t_draw *x = (t_draw *)z;
    t_svg *sa = (t_svg *)x->x_attr;

    t_float mtx1[3][3];
    t_float mtx2[3][3];
    t_float m1, m2, m3, m4, m5, m6, tdx, tdy;

    svg_groupmtx(sa, draw_motion_template, draw_motion_wp, mtx1);
    svg_parsetransform(sa, draw_motion_template, draw_motion_wp,
        &m1, &m2, &m3, &m4, &m5, &m6);
    mset(mtx2, m1, m2, m3, m4, m5, m6);
    mmult(mtx1, mtx2, mtx1);
    minv(mtx1, mtx1);
    */
    /* get rid of translation so it doesn't factor
       in to our deltas */
/* 
    mtx1[0][2] = 0;
    mtx1[1][2] = 0;
    mset(mtx2, dx, dy, 0, 0, 0, 0);
    mtx2[2][0] = 1;
    mmult(mtx1, mtx2, mtx2);
    tdx = mtx2[0][0];
    tdy = mtx2[1][0];
    t_fielddesc *f = sa->x_vec + draw_motion_field;
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
    */
        /* LATER figure out what to do to notify for an array? */
    /*
    if (draw_motion_scalar)
        template_notifyforscalar(draw_motion_template, draw_motion_glist, 
            draw_motion_scalar, gensym("change"), 1, &at);
    if (draw_motion_scalar)
        scalar_redraw(draw_motion_scalar, draw_motion_glist);
    if (draw_motion_array)
        array_redraw(draw_motion_array, draw_motion_glist);
}
*/

static int draw_click(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, t_scalar *sc, t_array *ap,
    t_float basex, t_float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    //fprintf(stderr,"draw_click %f %f %d %d %g %g %lx\n",
    //    basex, basey, xpix, ypix, glist_xtopixels(glist, basex),
    //    glist_ytopixels(glist, basey), (t_int)data);
    t_draw *x = (t_draw *)z;
    t_svg *sa = (t_svg *)x->x_attr;
    int x1, y1, x2, y2;
    /* don't register a click if we don't have an event listener, or
       if our pointer-event is "none" */
    if (!fielddesc_getfloat(&sa->x_pointerevents.a_attr, template, data, 1) ||
         (!fielddesc_getfloat(&sa->x_events.e_mousedown.a_attr, template, data, 1) &&
          !fielddesc_getfloat(&sa->x_drag, template, data, 1)))
        return 0;
    draw_getrect(z, glist, data, template, basex, basey,
        &x1, &y1, &x2, &y2);
    if (xpix >= x1 && xpix <= x2 && ypix >= y1 && ypix <= y2)
    {
        if (doit)
        {
            t_atom at[5];
            SETFLOAT(at+1, xpix - glist_xtopixels(glist, basex));
            SETFLOAT(at+2, ypix - glist_ytopixels(glist, basey));
            t_float mtx1[3][3];
            t_float mtx2[3][3];
            t_float m1, m2, m3, m4, m5, m6;

            t_svg *sa = (t_svg *)x->x_attr;
            /* might use this to output the ctm */
            svg_groupmtx(sa, template, data, mtx1);
            svg_parsetransform(sa, template, data,
                &m1, &m2, &m3, &m4, &m5, &m6);
            mset(mtx2, m1, m2, m3, m4, m5, m6);
            mmult(mtx1, mtx2, mtx1);
            minv(mtx1, mtx1);
            /* get rid of translation so it doesn't factor
               in to our deltas */
            //mtx1[0][2] = 0;
            //mtx1[1][2] = 0;
            /* maybe needs units per pixel here? */
            mset(mtx2, xpix - glist_xtopixels(glist, basex),
                ypix - glist_ytopixels(glist, basey), 0, 0, 0, 0);
            mtx2[2][0] = 1;
            mmult(mtx1, mtx2, mtx2);
            SETFLOAT(at+3, mtx2[0][0]); /* user-coordinate x */
            SETFLOAT(at+4, mtx2[1][0]); /* user-coordinate y */

            if (fielddesc_getfloat(&sa->x_drag, template, data, 1))
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
                //draw_motion_field = 2*bestn;
                draw_motion_template = template;
                if (draw_motion_scalar)
                    gpointer_setglist(&draw_motion_gpointer, draw_motion_glist,
                        &draw_motion_scalar->sc_gobj);
                else gpointer_setarray(&draw_motion_gpointer,
                        draw_motion_array, draw_motion_wp);
                glist_grab(glist, z, draw_motion, 0, xpix, ypix);
                //outlet_anything(x->x_obj.ob_outlet, gensym("click"), 0, 0);
            }
            //draw_notifyforscalar(x, glist, sc, gensym("mousedown"), 5, at);
        }
        return (1);
    }
    return (0);
}

/* given symbol "x123456", search the fields of a scalar's template for
   a t_array that matches that addy. For now we only search the toplevel.
   We can probably also search arbitrarily deep by making this recursive.
   But the one person I know who uses nested ds arrays hasn't asked for
   that, so let's see if we can just make it to the end of this software's
   life before that happens. */
static void scalar_spelunkforword(void* word_candidate, t_template* template,
    t_word *data, int word_index, t_array **arrayp, t_word **datap)
{
    int i, nitems = template->t_n;
    t_dataslot *datatypes = template->t_vec;
    t_word *wp = data;
    for (i = 0; i < nitems; i++, datatypes++, wp++)
    {
        if (datatypes->ds_type == DT_ARRAY &&
            ((void *)wp->w_array) == (void *)word_candidate)
        {
                /* Make sure we're in range, as the array could have been
                   resized. In that case simply return */
            if (word_index >= wp->w_array->a_n) return;
            *arrayp = wp->w_array;
            *datap = ((t_word *)(wp->w_array->a_vec +
                word_index * wp->w_array->a_elemsize));
            return;
        }
    }
        /* Now swoop through the headers again and recursively search
           any arrays for nested data. We do this as a second step so
           that toplevel arrays don't take a performance hit from deeply
           nested arrays. (Probably not a big deal for click events, but
           for complex data structures it could be noticeable for drag
           events */
    wp = data;
    datatypes = template->t_vec;
    for (i = 0; i < nitems; i++, datatypes++, wp++)
        if (datatypes->ds_type == DT_ARRAY)
        {
            t_template* t = template_findbyname(wp->w_array->a_templatesym);
            if (t)
            {
                int i, elemsize = wp->w_array->a_elemsize;
                for(i = 0; i < wp->w_array->a_n; i++)
                    scalar_spelunkforword(word_candidate, t,
                        (t_word *)(wp->w_array->a_vec + i * elemsize),
                            word_index, arrayp, datap);
            }
        }
}

void draw_notify(t_canvas *x, t_symbol *s, int argc, t_atom *argv)
{
    char canvas_field_namebuf[20];
    t_symbol *canvas_field_event;
    t_symbol *scalarsym = atom_getsymbolarg(0, argc--, argv++);
    t_symbol *drawcommand_sym = atom_getsymbolarg(0, argc--, argv++);
    t_symbol *array_sym = atom_getsymbolarg(0, argc--, argv++);
    t_array *a = 0;
    t_word *data = 0;
    int index = atom_getintarg(0, argc--, argv++);
    t_scalar *sc;
    t_object *ob = 0;
    if (scalarsym->s_thing)
        sc = (t_scalar *)scalarsym->s_thing;
    else
    {
        error("draw_notify: can't get scalar from symbol");
        return;
    }

    /* Now that we have our scalar, check if this callback is for an
       array element that was drawn with [draw array]. If the index is zero
       or greater then that's what we have */
    if (index > -1)
    {
        t_template *template = template_findbyname(sc->sc_template);
        if (!template)
        {
            pd_error(sc, "scalar: template disappeared before notification "
                        "from gui arrived");
            return;
        }
        long word_candidate = 0; /* from glob_findinstance */
        if (!sscanf(array_sym->s_name, "x%lx", &word_candidate))
        {
            pd_error(sc, "scalar: couldn't read array datum from GUI");
            return;
        }
        scalar_spelunkforword((void *)word_candidate, template, sc->sc_vec,
            index, &a, &data);
        if (!data)
        {
            pd_error(x, "scalar: couldn't get array data for event callback");
            return;
        }
    }

    /* Generate the symbol that would be bound by any [event] inside
       a canvas field.  If there's any in existence, forward the event
       notification. pd_bind takes care of the details of this-- if 
       there are multiple [event] objects it will dispatch to each */
    sprintf(canvas_field_namebuf, "%lx_event", (long unsigned int)sc->sc_vec);
    canvas_field_event = gensym(canvas_field_namebuf);
    t_pd *target = canvas_field_event->s_thing;
    if (target)
        pd_forwardmess(target, argc, argv);

    /* need to revisit popping this off the args, it's a little confusing... */
    t_symbol *event_name = atom_getsymbolarg(0, argc--, argv++);

    if (drawcommand_sym->s_thing)
    {
        t_pd *drawcommand = (t_pd *)drawcommand_sym->s_thing;
        if (pd_class(drawcommand) == draw_class)
        {
            t_draw *d = (t_draw *)drawcommand;
            ob = &d->x_obj;
        }
        else
        {
            t_canvas *group = (t_canvas *)drawcommand;
            ob = &group->gl_obj;
        }
    }
    else
    {
        error("draw_notify: can't get draw object from symbol");
        return;
    }
    if (ob)
        draw_notifyforscalar(ob, x, a, data, sc, event_name, argc, argv);
}


/*
static int draw_click(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, t_scalar *sc, t_array *ap,
    t_float basex, t_float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    post("hello?");
    //fprintf(stderr,"draw_click %f %f %d %d %lx\n",
    //    basex, basey, xpix, ypix, (t_int)data);
    t_draw *x = (t_draw *)z;
    t_svg *sa = (t_svg *)x->x_attr;
    t_float mtx1[3][3];
    t_float mtx2[3][3];
    t_float m1, m2, m3, m4, m5, m6;

    svg_groupmtx(sa, template, data, mtx1);
    svg_parsetransform(sa, template, data, &m1, &m2, &m3, &m4, &m5, &m6);
    mset(mtx2, m1, m2, m3, m4, m5, m6);
    mmult(mtx1, mtx2, mtx1);
    int i, n = sa->x_nargs;
    int bestn = -1;
    int besterror = 0x7fffffff;
    t_fielddesc *f;
    if (!fielddesc_getfloat(&sa->x_vis.a_attr, template, data, 0))
        return (0);
    int nxy = n >> 1;
    for (i = 0, f = sa->x_vec; i < nxy; i++, f += 2)
    {
        t_float xval = fielddesc_getcoord(f, template, data, 0);
        t_float yval = fielddesc_getcoord(f+1, template, data, 0);
        mset(mtx2, xval, yval, 0, 0, 0, 0);
        mtx2[2][0] = 1;
        mmult(mtx1, mtx2, mtx2);
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
                &draw_motion_scalar->sc_gobj);
        else gpointer_setarray(&draw_motion_gpointer,
                draw_motion_array, draw_motion_wp);
        glist_grab(glist, z, draw_motion, 0, xpix, ypix);
    }
    post("we got clicked");
    outlet_anything(x->x_obj.ob_outlet, gensym("click"), 0, 0);
    return (1);
}
*/

t_parentwidgetbehavior draw_widgetbehavior =
{
    draw_getrect,
    draw_displace,
    draw_select,
    draw_activate,
    draw_vis,
    draw_click,
};

static void svg_free_events(t_svg *x)
{
    /* Right now all the events except for "drag" get automatically
       garbage collected in the GUI.
       The reason "drag" does not is that it's a kind of "meta-event"--
       we keep a reference to the "draggable" object and check for it
       on clicking the canvas. The benefit is there's a single, centralized
       set of canvas events instead of event listeners for each scalar.
       Drawback is that we have to manage destroying the "drag" references
       in the GUI. But eventually all scalar events should be handled this
       way... */
    if (x->x_events.e_drag.a_flag == 1)
    {
        fielddesc_setfloat_const(&x->x_events.e_drag.a_attr, 0);
        svg_update(x, gensym("drag"));
    }
}

static void svg_free(t_svg *x)
{
    /* free any events we have registered with the GUI */
    svg_free_events(x);
    /* [group] has no pts in x_attr->a_vec, but it looks like
       t_freebytes allocates a single byte so freeing it
       should be fine */
    t_freebytes(x->x_vec,
        x->x_nargs * sizeof(*x->x_vec));
    t_freebytes(x->x_strokedasharray,
        x->x_ndash * sizeof(*x->x_strokedasharray));
    t_freebytes(x->x_transform,
        x->x_transform_n * sizeof(*x->x_transform));
    if (x->x_type == gensym("path"))
    {
        t_freebytes(x->x_pathcmds, x->x_npathcmds * sizeof(*x->x_pathcmds));
        t_freebytes(x->x_nargs_per_cmd, x->x_npathcmds * sizeof(*x->x_nargs_per_cmd));
    }
    char buf[50];
    sprintf(buf, "x%lx", (long unsigned int)x);
    pd_unbind((t_pd *)x->x_parent, gensym(buf));
}

void canvas_group_free(t_pd *x)
{
    t_svg *svg = (t_svg *)x;
    svg_free(svg);
}

static void draw_free(t_draw *x)
{
    t_svg *sa = (t_svg *)x->x_attr;
    svg_free(sa);
}


static void draw_setup(void)
{
    draw_class = class_new(gensym("draw"), (t_newmethod)draw_new,
        (t_method)draw_free, sizeof(t_draw), CLASS_NOINLET, A_GIMME, 0);
    /* proxy inlet for [draw] and [group] */
    svg_class = class_new(gensym("_svg"), 0, 0,
          sizeof(struct _svg), CLASS_PD, 0);
    class_setdrawcommand(draw_class);
    class_setparentwidget(draw_class, &draw_widgetbehavior);
    /* methods for svg_class-- these will be accessible
       from the inlet of [draw] and the (rightmost) inlet of
       [group] */
    /* I don't find anything in the spec that will render the
       shape while turning off the bbox calculations-- i.e., something
       like the "-n" flag of [drawcurve].  So I've introduced the
       "bbox" method for this */
    /* future method for animation */
    //class_addmethod(svg_class, (t_method)svg_animate,
    //    gensym("animate"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_bbox,
        gensym("bbox"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("cx"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("cy"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_data,
        gensym("d"), A_GIMME, 0);
    //class_addmethod(svg_class, (t_method)svg_drag,
    //    gensym("drag"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_event,
        gensym("drag"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_fill,
        gensym("fill"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("fill-opacity"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("fill-rule"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("height"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_event,
        gensym("mousedown"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_event,
        gensym("mousemove"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_event,
        gensym("mouseover"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_event,
        gensym("mouseout"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_event,
        gensym("mouseup"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_event,
        gensym("mouseenter"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_event,
        gensym("mouseleave"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("opacity"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("pointer-events"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_data,
        gensym("points"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("r"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("rx"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("ry"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_stroke,
        gensym("stroke"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_strokedasharray,
        gensym("stroke-dasharray"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("stroke-opacity"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("stroke-dashoffset"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("stroke-linecap"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("stroke-linejoin"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("stroke-miterlimit"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("stroke-width"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_transform,
        gensym("transform"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_viewbox,
        gensym("viewBox"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("vis"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("width"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("x"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("x1"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("x2"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("y"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("y1"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_setattr,
        gensym("y2"), A_GIMME, 0);
    class_addmethod(svg_class, (t_method)svg_update_args,
        gensym("update_svg"), A_GIMME, 0);
}

/* ------------------------------ event --------------------------------- */
/* This is a very simple class used to dispatch events inside a
   canvas field. */

t_class *event_class;

typedef struct _event
{
    t_object x_obj;
    t_symbol *x_bindsym;
} t_event;

static void event_anything(t_event *x, t_symbol *s, int argc, t_atom *argv)
{
    outlet_anything(x->x_obj.ob_outlet, s, argc, argv);
}

static void *event_new(void)
{
    char namebuf[20];
    t_event *x = (t_event *)pd_new(event_class);
    t_canvas *c = canvas_getrootfor(canvas_getcurrent());
    if (c->gl_vec)
    {
        sprintf(namebuf, "%lx_event", (long unsigned int)c->gl_vec);
        x->x_bindsym = gensym(namebuf);
        pd_bind(&x->x_obj.ob_pd, x->x_bindsym);
    }
    else
    {
        x->x_bindsym = 0;
    }
    outlet_new(&x->x_obj, &s_anything);
    return (x);
}

static void event_free(t_event *x)
{
    if (x->x_bindsym)
        pd_unbind(&x->x_obj.ob_pd, x->x_bindsym);
}

void event_setup(void)
{
    event_class = class_new(gensym("event"), (t_newmethod)event_new,
        (t_method)event_free, sizeof(t_event), 0, 0);
    class_addanything(event_class, event_anything);
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
    t_fielddesc x_outlinecolor;
    t_fielddesc x_width;
    t_fielddesc x_vis;
    int x_npoints;
    t_fielddesc *x_vec;
    t_canvas *x_canvas;
} t_curve;

/* We use this to prevent the user from creating plot and other legacy
   drawing commands inside a [group]. (However, they can still sneak in
   when loading a file.) The reason is that they have not had their
   motionfn revised to respect the parent affine transformations.  If they
   did then we could allow them inside a [group]. */
extern int canvas_isgroup(t_canvas *x);

static int legacy_draw_in_group(t_canvas *c)
{
    if (canvas_isgroup(c))
    {
        pd_error(c, "group: can't contain curve or plot");
        return (1);
    }
    else
        return (0);
}

typedef struct {int x,y;} intxy;

/* this is a conversion from tk's smooth method to an svg path command.
   It is here for backwards-compabitility with the legacy data structure
   drawing commands. A description of tk's curve implementation may be
   found here:
   http://www.tcl.tk/cgi-bin/tct/tip/168.html */
void curve_smooth_to_q(int *pix, int n, int closed)
{
    //fprintf(stderr,"curve_smooth_to_q closed=%d\n", closed);
    intxy *p = (intxy *)pix;
    int i;
    int overlap = 0;
    // >>1 is used instead of /2 or *0.5, because of slight difference in rounding.

    gui_s("d");
    gui_start_array();

    if (closed)
    {
        int a=0, b=0;
        // if first point and last point are the same skip the first point
        if (p[0].x == p[n-1].x && p[0].y == p[n-1].y)
        {
            if (n-1 != 0)
            {
                // protects below in the for loop to not exceed the point array size
                overlap = 1;
            }
            a = (p[1].x+p[0].x)>>1;
            b = (p[1].y+p[0].y)>>1;
        }
        else
        {
            a = (p[0].x+p[n-1].x)>>1;
            b = (p[0].y+p[n-1].y)>>1;
        }
        gui_s("M");
        gui_i(a);
        gui_i(b);
    }
    else // need to test non-closed smooth curves
    {
        gui_s("M");
        gui_i(p[0].x);
        gui_i(p[0].y);
    }
    intxy o = closed ? p[0+overlap] : p[1];
    int n2 = (closed?n:n-1); // need to test this for non-closed smooth curves
    for (i = (closed?(1+overlap):2); i < n2; i++)
    {
        gui_s("Q");
        gui_i(o.x);
        gui_i(o.y);
        gui_i(((o.x + p[i].x)>>1));
        gui_i(((o.y + p[i].y)>>1));

        o = p[i];
    }
    if (closed)
    {
        // here we repurpose overlap for an additional check
        overlap = (p[0].x == p[n-1].x && p[0].y == p[n-1].y && n-1 != 0);
        gui_s("Q");
        gui_i(p[n-1].x);
        gui_i(p[n-1].y);
        gui_i((p[n-1].x+p[0+overlap].x)>>1);
        gui_i((p[n-1].y+p[0+overlap].y)>>1);
    }
    else
    {
        // need ot test this for non-closed smooth curves
        gui_s("Q");
        gui_i(p[n-2].x);
        gui_i(p[n-2].y);
        gui_i(p[n-1].x);
        gui_i(p[n-1].y);
    }
    gui_end_array();
}

static void *curve_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
    if (legacy_draw_in_group(canvas_getcurrent()))
        return 0;
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
    if (classname[0] == 'c') flags |= BEZ;
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
        int xloc = basex + fielddesc_getcoord(f, template, data, 0);
        int yloc = basey + fielddesc_getcoord(f+1, template, data, 0);
        if (xloc < x1) x1 = xloc;
        if (xloc > x2) x2 = xloc;
        if (yloc < y1) y1 = yloc;
        if (yloc > y2) y2 = yloc;
    }
    //fprintf(stderr,"FINAL curve_getrect %d %d %d %d\n", x1, y1, x2, y2);
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

static void curve_vis(t_gobj *z, t_glist *glist, t_glist *parentglist,
    t_scalar *sc, t_word *data, t_template *template,
    t_float basex, t_float basey, t_array *parentarray, int vis)
{
    t_curve *x = (t_curve *)z;
    int i, n = x->x_npoints;
    t_fielddesc *f = x->x_vec;

    /*// get the universal tag for all nested objects
    t_canvas *tag = x->x_canvas;
    while (tag->gl_owner)
    {
        tag = tag->gl_owner;
    }*/
    
        /* see comment in plot_vis() */
    /* Note: we can probably get a slight acceleration here-- if vis
       is -1 and x->x_vis is 0, we can just send a "visibility:0" attribute
       instead of the coords and stuff */
    //if (vis && !fielddesc_getfloat(&x->x_vis, template, data, 0))
    //    return;
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
            int flags = x->x_flags;
            t_float width = fielddesc_getfloat(&x->x_width, template, data, 1);
            char outline[20], fill[20];
            int pix[200];
            char type[9];
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
                if (flags & BEZ)
                    sprintf(type, "path");
                else
                    sprintf(type, "polygon");
            }
            else
            {
                if(flags & BEZ)
                    sprintf(type, "path");
                else
                    sprintf(type, "polyline");
            }
            if (vis != -1) /* draw for the first time */
                gui_start_vmess("gui_draw_vis", "xs",
                    glist_getcanvas(glist), type);
            else /* just update the attributes for an existing object */
                gui_start_vmess("gui_draw_configure_old_command", "xs",
                    glist_getcanvas(glist), type);
            // Attributes array
            gui_start_array();

            // visibility
            gui_s("visibility");
            gui_s((int)fielddesc_getfloat(&x->x_vis, template, data, 0) ?
                "normal" : "hidden");

            // points data
            if (flags & BEZ)
            {
                curve_smooth_to_q(pix, n, (flags & CLOSED));
            }
            else
            {
                gui_s("points");
                gui_start_array();
                for (i = 0; i < n; i++)
                {
                    gui_i(pix[2*i]);
                    gui_i(pix[2*i+1]);
                }
                gui_end_array();
            }
            gui_s("stroke-width");
            gui_f(width);
            if (flags & CLOSED)
            {
                gui_s("fill");
                gui_s(fill);
                gui_s("stroke");
                gui_s(outline);
            }
            else
            {
                gui_s("stroke");
                gui_s(outline);
                gui_s("fill");
                gui_s("none");
            }
            /* We add an attribute to keep the stroke from scaling, to keep
               backwards compatibility with Pd Vanilla for curve.  For the
               newer [draw] objects we stick with the svg spec (and should
               probably add a method for this attribute...) */
            gui_s("vector-effect");
            gui_s("non-scaling-stroke");
            gui_end_array();

            // Tags Array
            gui_start_array();
            char parent_tagbuf[MAXPDSTRING];
 
            sprintf(parent_tagbuf, "dgroup%lx.%lx",
                (in_array ? (long unsigned int)parentglist :
                            (long unsigned int)x->x_canvas),
                (long unsigned int)data);
            gui_s(parent_tagbuf);
            char tagbuf[MAXPDSTRING];
            sprintf(tagbuf, "curve%lx.%lx", (long unsigned int)x,
                (long unsigned int)data);
            gui_s(tagbuf);
            gui_end_array(); /* end of tags array */
            gui_end_vmess();
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
        else post("warning: curves need at least two points to be graphed");
    }
    else
    {
        if (n > 1)
        {
            char itemtagbuf[MAXPDSTRING];
            sprintf(itemtagbuf, "curve%lx.%lx", (long unsigned int)x,
                (long unsigned int)data);
        }
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
    //fprintf(stderr,"curve_click %f %f %d %d %lx\n", basex, basey,
    //    xpix, ypix, (t_int)data);
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
                &curve_motion_scalar->sc_gobj);
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
    class_setparentwidget(curve_class, &curve_widgetbehavior);
    class_addfloat(curve_class, curve_float);
}

/* --------- plots for showing arrays --------------- */

static t_class *plot_class;

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

int is_plot_class(t_gobj *y)
{
    return (pd_class(&y->g_pd) == plot_class);
}

static void *plot_new(t_symbol *classsym, t_int argc, t_atom *argv)
{
     if (legacy_draw_in_group(canvas_getcurrent()))
        return 0;

    t_plot *x = (t_plot *)pd_new(plot_class);
    int defstyle = PLOTSTYLE_POLY;
    x->x_canvas = canvas_getcurrent();
    //fprintf(stderr,"plot new %s\n",
    //    (canvas_makebindsym(x->x_canvas->gl_name))->s_name);
    t_template *t = template_findbyname(
        canvas_makebindsym(x->x_canvas->gl_name));
    if (t)
    {
        /* increment variable of the template
           to prevent transform as that would
           make arrays break their hitboxes
           and all kinds of other bad stuff */
        t->t_transformable++;
    }

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
    int elemsize, yonset, wonset, xonset, type;
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

static void plot_getgrouprect(t_glist *glist, t_template *elemtemplate,
    t_canvas *groupcanvas, int elemsize,
    t_array *array, int i, t_float usexloc, t_float useyloc,
    int *x1, int *y1, int *x2, int *y2)
{
    t_gobj *y;
    for (y = groupcanvas->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == canvas_class &&
            ((t_canvas *)y)->gl_svg)
        {
            plot_getgrouprect(glist, elemtemplate, (t_canvas *)y,
                elemsize, array, i, usexloc, useyloc, x1, y1, x2, y2);
        }
        //fprintf(stderr,".-.-. usexloc %f useyloc %f "
        //               "(alt %f %f)\n",
        //  usexloc, useyloc,
        //  basex + xloc +
        //  fielddesc_cvttocoord(xfielddesc,
        //      *(t_float *)(((char *)(array->a_vec) + elemsize * i)
        //      + xonset)),
        //  *(t_float *)(((char *)(array->a_vec) + elemsize * i) +
        //  yonset));
        int xx1, xx2, yy1, yy2;
        t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
        if (!wb) continue;
        (*wb->w_parentgetrectfn)(y, glist,
            (t_word *)((char *)(array->a_vec) + elemsize * i),
                elemtemplate, usexloc, useyloc, 
                    &xx1, &yy1, &xx2, &yy2);
        //fprintf(stderr,"  .....plot_getrect %d %d %d %d\n",
        //    xx1, yy1, xx2, yy2); 
        if (xx1 < *x1)
            *x1 = xx1;
        if (yy1 < *y1)
            *y1 = yy1;
        if (xx2 > *x2)
            *x2 = xx2;
        if (yy2 > *y2)
            *y2 = yy2;
        //fprintf(stderr,"  ....plot_getrect %d %d %d %d\n",
        //    x1, y1, x2, y2); 
    }
} 

static void plot_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    //fprintf(stderr,"plot_getrect\n");
    t_plot *x = (t_plot *)z;
    t_float mtx1[3][3], mtx2[3][3];

    /* todo: svg_groupmtx should be changed so we
       can just call it here.  But it takes an
       t_svg and t_plot doesn't have one */
    mset(mtx1, 1, 0, 0, 1, 0, 0);
    if (x->x_canvas->gl_owner && x->x_canvas->gl_svg)
        svg_dogroupmtx(x->x_canvas, template, data, mtx1);
    //post("plot_getrect matrix: %g %g %g %g %g %g",
    //  mtx1[0][0], mtx1[1][0], mtx1[0][1], mtx1[1][1], mtx1[0][2], mtx1[1][2]);
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
    t_float xpix1, xpix2, ypix, ypix2, wpix;
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
                /* get the coords of the point proper */
            array_getcoordinate(glist, (char *)(array->a_vec) + i * elemsize,
                xonset, yonset, wonset, i, basex + xloc, basey + yloc, xinc,
                xfielddesc, yfielddesc, wfielddesc,
                &xpix1, &xpix2, &ypix, &wpix, 0);
            //fprintf(stderr,"elemsize%d yonset%d wonset%d xonset%d "
            //               "i%d basex%f xloc%f basey%f yloc%f xinc%f "
            //               "xpix%f ypix%f wpix%f\n",
            //    elemsize, yonset, wonset, xonset, i, basex, xloc, basey, yloc,
            //    xinc, xpix, ypix, wpix);
            mset(mtx2, xpix1 - basex, ypix - basey,
                       xpix2 - basex, ypix - basey, 0, 0);
            mtx2[2][0] = 1; mtx2[2][1] = 1;
            mmult(mtx1, mtx2, mtx2);
            xpix1 = mtx2[0][0] + basex;
            xpix2 = mtx2[0][1] + basex;
            ypix = mtx2[1][0] + basey;
            ypix2 = mtx2[1][1] + basey;
            if (xpix1 < x1)
                x1 = xpix1;
            if (xpix1 > x2)
                x2 = xpix1;
            if (xpix2 < x1)
                x1 = xpix2;
            if (xpix2 > x2)
                x2 = xpix2;
            if (ypix - wpix < y1)
                y1 = ypix - wpix;
            if (ypix + wpix > y2)
                y2 = ypix + wpix;
            if (ypix2 - wpix < y1)
                y1 = ypix2 - wpix;
            if (ypix2 + wpix > y2)
                y2 = ypix2 + wpix;

            //fprintf(stderr,"plot_getrect %d %d %d %d\n", x1, y1, x2, y2);
            if (scalarvis != 0)
            {
                    /* check also the drawing instructions for the scalar */ 
                if (xonset >= 0)
                    usexloc = basex + xloc + fielddesc_cvttocoord(xfielddesc, 
                        *(t_float *)(((char *)(array->a_vec) + elemsize * i)
                            + xonset));
                //else usexloc = x1; //usexloc = basex + xsum, xsum += xinc;
                usexloc = xloc + basex + xsum, xsum += xinc;
                if (yonset >= 0)
                    yval = *(t_float *)(((char *)(array->a_vec) + elemsize * i)
                        + yonset);
                else yval = 0;
                //useyloc = (y1+y2)/2; //basey + yloc + fielddesc_cvttocoord(yfielddesc, yval);
                useyloc = basey + yloc + fielddesc_cvttocoord(yfielddesc, yval);

                plot_getgrouprect(glist, elemtemplate, elemtemplatecanvas,
                    elemsize, array, i, usexloc, useyloc, &x1, &y1, &x2, &y2);
/*
                for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
                {
                    //fprintf(stderr,".-.-. usexloc %f useyloc %f "
                    //               "(alt %f %f)\n",
                    //  usexloc, useyloc,
                    //  basex + xloc +
                    //  fielddesc_cvttocoord(xfielddesc,
                    //      *(t_float *)(((char *)(array->a_vec) + elemsize * i)
                    //      + xonset)),
                    //  *(t_float *)(((char *)(array->a_vec) + elemsize * i) +
                    //  yonset));
                    int xx1, xx2, yy1, yy2;
                    t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
                    if (!wb) continue;
                    (*wb->w_parentgetrectfn)(y, glist,
                        (t_word *)((char *)(array->a_vec) + elemsize * i),
                            elemtemplate, usexloc, useyloc, 
                                &xx1, &yy1, &xx2, &yy2);
                    //fprintf(stderr,"  .....plot_getrect %d %d %d %d\n",
                    //    xx1, yy1, xx2, yy2); 
                    if (xx1 < x1)
                        x1 = xx1;
                    if (yy1 < y1)
                        y1 = yy1;
                     if (xx2 > x2)
                        x2 = xx2;
                    if (yy2 > y2)
                        y2 = yy2;
                    //fprintf(stderr,"  ....plot_getrect %d %d %d %d\n",
                    //    x1, y1, x2, y2); 
                }
*/
            }
            //fprintf(stderr,"  >====plot_getrect %d %d %d %d\n",
            //    x1, y1, x2, y2);
        }
    }
    //fprintf(stderr,"FINAL plot_getrect %d %d %d %d\n", x1, y1, x2, y2);
    //fprintf(stderr,"basex %g basey %g\n", basex, basey);
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

static void plot_groupvis(t_scalar *x, t_glist *owner, t_word *data,
    t_template *template,
    t_glist *groupcanvas, t_glist *parent, t_float basex, t_float basey,
    t_array *parentarray)
{
    t_gobj *y;
    char tagbuf[MAXPDSTRING], parent_tagbuf[MAXPDSTRING];
    sprintf(tagbuf, "dgroup%lx.%lx", (long unsigned int)groupcanvas,
        (long unsigned int)data);
    sprintf(parent_tagbuf, "dgroup%lx.%lx", (long unsigned int)parent,
        (long unsigned int)data);
    gui_start_vmess("gui_scalar_draw_group", "xsss",
        glist_getcanvas(owner),
        tagbuf,
        parent_tagbuf,
        "g");
    svg_grouptogui(groupcanvas, template, data);
    gui_end_vmess();
    for (y = groupcanvas->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == canvas_class &&
            ((t_glist *)y)->gl_svg)
        {
            plot_groupvis(x, owner, data, template, (t_glist *)y, groupcanvas,
                basex, basey, parentarray);
        }
        t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
        if (!wb) continue;
        (*wb->w_parentvisfn)(y, owner, groupcanvas, x, data, template,
            basex, basey, parentarray, 1);
    }
}

/* see if the elements we're plotting have any drawing commands */
int plot_has_drawcommand(t_canvas *elemtemplatecanvas)
{
    t_gobj *y;
    for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == canvas_class && ((t_glist *)y)->gl_svg)
            return 1;
        else if (class_isdrawcommand(y->g_pd))
            return 1;
    }
    return 0;
}

static void plot_vis(t_gobj *z, t_glist *glist, t_glist *parentglist,
    t_scalar *sc, t_word *data, t_template *template,
    t_float basex, t_float basey, t_array *parentarray, int tovis)
{
    t_plot *x = (t_plot *)z;
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
        int in_array = (sc->sc_vec == data) ? 0 : 1;
        int draw_scalars = plot_has_drawcommand(elemtemplatecanvas);
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

        t_float xscale = glist_xtopixels(glist, 1) - glist_xtopixels(glist, 0);
        t_float yscale = glist_ytopixels(glist, 1) - glist_ytopixels(glist, 0);
        t_float x_inverse = 1 / xscale;
        t_float y_inverse = 1 / yscale; /* for the stroke-width */

        if (style == PLOTSTYLE_POINTS || style == PLOTSTYLE_BARS)
        {

            symfill = (style == PLOTSTYLE_POINTS ? symoutline : symfill);
            t_float minyval = 1e20, maxyval = -1e20;
            int ndrawn = 0;

            gui_start_vmess("gui_plot_vis", "xii",
                glist_getcanvas(glist),
                basex,
                basey);
                
            gui_start_array();

            for (xsum = xloc, i = 0; i < nelem; i++)
            {
                t_float yval;
                int ixpix, inextx, render;

                if (xonset >= 0)
                {
                    usexloc = xloc +
                        *(t_float *)((elem + elemsize * i) + xonset);
                    ixpix = fielddesc_cvttocoord(xfielddesc, usexloc);
                    inextx = ixpix + 2;
                    /* we use 'render' as a stopgap to choose whether
                       or not to draw this point in the trace. For
                       templates that have an x field we always render */
                    render = 1;
                }
                else
                {
                    usexloc = xsum;
                    xsum += xinc;
                    ixpix = (int)(glist_xtopixels(glist,
                            fielddesc_cvttocoord(xfielddesc, usexloc)));
                    inextx = (int)(glist_xtopixels(glist,
                            fielddesc_cvttocoord(xfielddesc, xsum)));

                    /* For y-only templates, we only render the point
                       if its at a different x-coordinate than the
                       previous one. (For example, if you try to fit
                       a 44,100 point array into a 100 pixel wide
                       graph.) We're doing the scaling on the GUI side,
                       but we must still use glist_xtopixels in order
                       to test for a new x-pixel value. */
                    render = ixpix != inextx;
                }

                if (yonset >= 0)
                    yval = yloc + *(t_float *)((elem + elemsize * i) + yonset);
                else yval = 0;
                if (yval > maxyval)
                    maxyval = yval;
                if (yval < minyval)
                    minyval = yval;
                if (i == nelem-1 || render)
                {
                    int py2 = 0;
                    if (style == PLOTSTYLE_POINTS)
                        py2 = (int)fielddesc_cvttocoord(yfielddesc, maxyval)
                                + linewidth - 1;
                    else
                    {
                        if (glist->gl_isgraph && !glist->gl_havewindow)
                        {
                            py2 = glist->gl_y2;
                        }
                        else
                        {
                            /* case for open window displaying the array */
                            /* tbd */
                        }
                    }
                    int mex1 = fielddesc_cvttocoord(xfielddesc, usexloc);
                    //t_float mey1 = fielddesc_cvttocoord(yfielddesc, minyval) - 1;
                    int mex2 = fielddesc_cvttocoord(xfielddesc, xsum + x_inverse);

                    t_float mey2 = style == PLOTSTYLE_POINTS ?
                        yval + y_inverse * linewidth : py2;

                    gui_s("M");
                    gui_i(mex1);
                    gui_f(yval);

                    gui_s("H");
                    gui_i(mex2);

                    gui_s("V");
                    gui_f(mey2);

                    gui_s("H");
                    gui_i(mex1);

                    gui_s("z");

                    ndrawn++;
                    minyval = 1e20;
                    maxyval = -1e20;
                }
                if (ndrawn > 2000 || ixpix >= 3000) break;
            }

            gui_end_array();

            /* stroke and fill */
            gui_start_array();
            gui_s("fill");
            gui_s(symfill->s_name);

            gui_s("stroke");
            gui_s(symoutline->s_name);

            gui_s("stroke-width");
            gui_f(style == PLOTSTYLE_POINTS ? 0 : 1);
            gui_s("vector-effect");
            gui_s("non-scaling-stroke");
            gui_end_array();

            /* tags */
            gui_start_array();
            char pbuf[MAXPDSTRING];
            char tbuf[MAXPDSTRING];
            sprintf(pbuf, "dgroup%lx.%lx",
                (long unsigned int)x->x_canvas,
                (long unsigned int)data);
            sprintf(tbuf, ".x%lx.x%lx.template%lx",
                (long unsigned int)glist_getcanvas(glist),
                (long unsigned int)glist,
                (long unsigned int)data);
            gui_s(pbuf);
            gui_s(pbuf);
            gui_end_array();

            gui_end_vmess();
        }
        else /* polygon style */
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

                gui_start_vmess("gui_plot_vis", "xii",
                    glist_getcanvas(glist),
                    basex,
                    basey);

                gui_start_array();
                gui_s("M");

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
                    xpix = fielddesc_cvttocoord(xfielddesc, usexloc);

                    ixpix = xpix + 0.5;
                    if (xonset >= 0 || ixpix != lastpixel)
                    {
                        gui_i(ixpix);
                        gui_f(fielddesc_cvttocoord(yfielddesc, 
                                  yloc + yval) -
                                      fielddesc_cvttocoord(wfielddesc,wval));
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
                    xpix = fielddesc_cvttocoord(xfielddesc, usexloc);

                    ixpix = xpix + 0.5;
                    if (xonset >= 0 || ixpix != lastpixel)
                    {
                        gui_i(ixpix);
                        gui_f(yloc + fielddesc_cvttocoord(yfielddesc,
                                  yval) +
                                      fielddesc_cvttocoord(wfielddesc, wval));
                        ndrawn++;
                    }
                    lastpixel = ixpix;
                    if (ndrawn >= 1000) goto ouch;
                }
                    /* TK will complain if there aren't at least 3 points.
                    There should be at least two already. */
                if (ndrawn < 4)
                {
                    gui_i(ixpix + 10);
                    gui_f(yloc + fielddesc_cvttocoord(yfielddesc,
                              yval) +
                                  fielddesc_cvttocoord(wfielddesc, wval));
                    gui_i(ixpix + 10);
                    gui_f(yloc + fielddesc_cvttocoord(yfielddesc,
                              yval) -
                                  fielddesc_cvttocoord(wfielddesc, wval));
                }
                gui_end_array();
            ouch:
                gui_start_array();

                gui_s("stroke-width");
                gui_f(1);
                gui_s("vector-effect");
                gui_s("non-scaling-stroke");
                gui_s("stroke");
                gui_s(symoutline->s_name);
                gui_s("fill");
                gui_s(symoutline->s_name);

                gui_end_array();

                /* tags */
                gui_start_array();
                char pbuf[MAXPDSTRING];
                char tbuf[MAXPDSTRING];
                sprintf(pbuf, "dgroup%lx.%lx",
                    (long unsigned int)x->x_canvas,
                    (long unsigned int)data);
                sprintf(tbuf, ".x%lx.x%lx.template%lx",
                    (long unsigned int)glist_getcanvas(glist),
                    (long unsigned int)glist,
                    (long unsigned int)data);
                gui_s(pbuf);
                gui_s(pbuf);
                gui_end_array();
                gui_end_vmess();
            }
            else if (linewidth > 0)
            {
                    /* no "w" field.  If the linewidth is positive, draw a
                    segmented line with the requested width; otherwise don't
                    draw the trace at all. */
                gui_start_vmess("gui_plot_vis", "xii",
                    glist_getcanvas(glist),
                    basex,
                    basey);

                gui_start_array();
                gui_s("M");

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

                    xpix = fielddesc_cvttocoord(xfielddesc, usexloc);

                    ixpix = xpix + 0.5;

                    int render;

                    render = (int)(glist_xtopixels(glist, ixpix)) !=
                             (int)(glist_xtopixels(glist, lastpixel));

                    if (xonset >= 0 || render)
                    {
                        gui_i(ixpix);
                        gui_f(yloc + fielddesc_cvttocoord(yfielddesc,
                                  yval));

                        ndrawn++;
                    }
                    lastpixel = ixpix;
                    if (ndrawn >= 1000) break;
                }
                    /* TK will complain if there aren't at least 2 points... */
                //if (ndrawn == 0) sys_vgui("0 0 0 0 \\\n");
                if (ndrawn == 1)
                {
                    gui_i(ixpix + 10);
                    gui_f(yloc + fielddesc_cvttocoord(yfielddesc, yval));
                }
                gui_end_array();

                gui_start_array();
                gui_s("stroke-width");
                gui_f(linewidth);
                gui_s("vector-effect");
                gui_s("non-scaling-stroke");
                gui_s("stroke");
                gui_s(symoutline->s_name);
                gui_s("fill");
                gui_s("none");
                gui_end_array();

                /* tags */
                gui_start_array();
                char pbuf[MAXPDSTRING];
                char tbuf[MAXPDSTRING];
                sprintf(pbuf, "dgroup%lx.%lx",
                    (long unsigned int)x->x_canvas,
                    (long unsigned int)data);
                sprintf(tbuf, ".x%lx.x%lx.template%lx",
                    (long unsigned int)glist_getcanvas(glist),
                    (long unsigned int)glist,
                    (long unsigned int)data);
                gui_s(pbuf);
                gui_s(pbuf);
                gui_end_array();
                gui_end_vmess();
            }
        }

        /* make sure the array drawings are behind the graph */
        /* not doing this yet with the GUI port... */
        //sys_vgui(".x%lx.c lower plot%lx graph%lx\n", glist_getcanvas(glist),
        //    data, glist);

            /* We're done with the outline; now draw all the points.
            This code is inefficient since the template has to be
            searched for drawing instructions for every last point. */
        if (scalarvis != 0 && draw_scalars)
        {
            //t_float xoffset = in_array ? basex: 0;
            //t_float yoffset = in_array ? basey: 0;
            t_float xoffset = 0;
            t_float yoffset = 0;

            for (xsum = xloc, i = 0; i < nelem; i++)
            {
                t_float usexloc, useyloc;
                t_gobj *y;
                if (xonset >= 0)
                    usexloc = xloc + xoffset +
                        *(t_float *)((elem + elemsize * i) + xonset);
                else usexloc = xoffset + xsum, xsum += xinc;
                if (yonset >= 0)
                    yval = *(t_float *)((elem + elemsize * i) + yonset);
                else yval = 0;
                useyloc = yloc + yoffset +
                    fielddesc_cvttocoord(yfielddesc, yval);
                 /* We're setting up a special group that will get set as
                   the parent by array elements */

                   /* todo: need to check if plot itself is in an array */
                char tagbuf[MAXPDSTRING];
                sprintf(tagbuf, "dgroup%lx.%lx",
                    (long unsigned int)elemtemplatecanvas,
                    (long unsigned int)((t_word *)(elem + elemsize * i)));
                char parent_tagbuf[MAXPDSTRING];
                sprintf(parent_tagbuf, "dgroup%lx.%lx",
                    (in_array ? (long unsigned int)parentglist :
                                (long unsigned int)x->x_canvas),
                    (long unsigned int)data);
                char transform_buf[MAXPDSTRING];
                sprintf(transform_buf, "translate(%g,%g)", usexloc, useyloc);

                gui_start_vmess("gui_draw_vis", "xs",
                    glist_getcanvas(glist), "g");
                gui_start_array();
                gui_s("transform");
                gui_s(transform_buf);
                gui_end_array();
                gui_start_array();
                gui_s(parent_tagbuf);
                gui_s(tagbuf);
                gui_end_array();
                gui_end_vmess();

                for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
                {
                    if (pd_class(&y->g_pd) == canvas_class &&
                        ((t_glist *)y)->gl_svg)
                    {
                        plot_groupvis(sc, glist,
                            (t_word *)(elem + elemsize * i),
                        template, (t_glist *)y, 
                            elemtemplatecanvas, usexloc, useyloc, array);
                    }
                    t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
                    if (!wb) continue;
                    (*wb->w_parentvisfn)(y, glist, elemtemplatecanvas, sc,
                        (t_word *)(elem + elemsize * i),
                            elemtemplate, usexloc, useyloc, array, tovis);
                }
            }
        }
        if (!glist_istoplevel(glist))
        {
            t_canvas *gl = glist_getcanvas(glist);
            char objtag[64];
            sprintf(objtag, ".x%lx.x%lx.template%lx",
                (t_int)gl, (t_int)glist, (t_int)data);
            canvas_restore_original_position(gl, (t_gobj *)glist, objtag, -1);
        }
        /*
        sys_vgui(".x%lx.c lower .x%lx.x%lx.plot%lx %s\n",
            glist_getcanvas(glist), glist_getcanvas(glist), glist, data,
            rtext_gettag(glist_findrtext(glist_getcanvas(glist),
            &glist->gl_obj)));
        sys_vgui(".x%lx.c raise .x%lx.x%lx.plot%lx %s\n",
            glist_getcanvas(glist), glist_getcanvas(glist), glist, data,
            rtext_gettag(glist_findrtext(glist_getcanvas(glist),
            &glist->gl_obj)));
         */
    }
    else
    {
        /* un-draw the individual points */
        //fprintf(stderr,"plot_vis UNVIS\n");

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
                    (*wb->w_parentvisfn)(y, glist, elemtemplatecanvas, sc,
                        (t_word *)(elem + elemsize * i), elemtemplate,
                            0, 0, parentarray, 0);
                }
            }
        }

            /* and then the trace */
        //sys_vgui(".x%lx.c delete .x%lx.x%lx.template%lx\n",
        //    glist_getcanvas(glist), glist_getcanvas(glist), glist, data);      
    }
}

static int plot_click(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, t_scalar *sc, t_array *ap,
    t_float basex, t_float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    //fprintf(stderr,"plot_click %lx %lx %f %f %d %d\n",
    //    (t_int)z, (t_int)glist, basex, basey, xpix, ypix);
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
    //fprintf(stderr,"  ->array_doclick\n");
        return (array_doclick(array, glist, sc, ap,
            elemtemplatesym,
            linewidth, basex + xloc, xinc, basey + yloc, scalarvis,
            xfielddesc, yfielddesc, wfielddesc,
            xpix, ypix, shift, alt, dbl, doit));
    }
    else return (0);
}

static void plot_free(t_plot *x)
{
    //fprintf(stderr,"plot_free\n");
    //sys_queuegui(x->x_canvas, 0, canvas_redrawallfortemplatecanvas);
    /* decrement variable of the template
       to prevent transform as that would
       make arrays break their hitboxes
       and all kinds of other bad stuff */
    t_template *t = template_findbyname(
        canvas_makebindsym(x->x_canvas->gl_name)
    );
    if (t)
    {
        t->t_transformable--;
        //fprintf(stderr,"plot_free > template:%lx(%s) transform:%d\n",
        //    (t_int)t, canvas_makebindsym(x->x_canvas->gl_name)->s_name,
        //    t->t_transformable);
    }
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
    plot_class = class_new(gensym("plot"), (t_newmethod)plot_new,
        (t_method)plot_free, sizeof(t_plot), 0, A_GIMME, 0);
    class_setdrawcommand(plot_class);
    class_addfloat(plot_class, plot_float);
    class_setparentwidget(plot_class, &plot_widgetbehavior);
}

/* --------- generic draw command for showing arrays --------------- */

static void *drawarray_new(t_symbol *s, int argc, t_atom *argv)
{
    t_drawarray *x = (t_drawarray *)pd_new(drawarray_class);

    /* We need a t_svg to associate with it */
    x->x_attr = (t_pd *)svg_new((t_pd *)x, s, 0, 0);
    t_svg *sa = (t_svg *)x->x_attr;

    x->x_canvas = canvas_getcurrent();
    //t_template *t = template_findbyname(
    //    canvas_makebindsym(x->x_canvas->gl_name));

    if (argc) fielddesc_setarrayarg(&x->x_data, argc--, argv++);
    else fielddesc_setfloat_const(&x->x_data, 1);

    /* Default dimensions for SVG: 150x100 */
    if (argc) svg_attr_setfloatarg(&sa->x_width, argc--, argv++);
    else svg_attr_setfloat_const(&sa->x_width, 150);
    if (argc) svg_attr_setfloatarg(&sa->x_height, argc--, argv++);
    else svg_attr_setfloat_const(&sa->x_height, 100);

    return (x);
}

void drawarray_float(t_drawarray *x, t_floatarg f)
{
    /* toggle visibility on and off */
}

void drawarray_transform(t_drawarray *x, t_floatarg f)
{
    /* draw array uses an inner svg as the container, which
       has no transform attribute */
    pd_error(x, "draw array: no method for 'transform'");
}

static void drawarray_anything(t_drawarray *x, t_symbol *s, int argc,
    t_atom *argv)
{
    /* forward to t_svg thingy */
    pd_typedmess(x->x_attr, s, argc, argv);
}

/* -------------------- widget behavior for drawarray ------------ */


    /* get everything we'll need from the owner template of the array being
    drawn. Not used for garrays, but see below */
static int drawarray_readownertemplate(t_drawarray *x,
    t_word *data, t_template *ownertemplate, 
    t_symbol **elemtemplatesymp, t_array **arrayp)
{
    int arrayonset, type;
    t_symbol *elemtemplatesym;
    t_array *array;

        /* find the data and verify it's an array */
    if (x->x_data.fd_type != A_ARRAY || !x->x_data.fd_var)
    {
        error("draw array: needs an array field");
        return (-1);
    }
    if (!template_find_field(ownertemplate, x->x_data.fd_un.fd_varsym,
        &arrayonset, &type, &elemtemplatesym))
    {
        error("draw array: %s: no such field", x->x_data.fd_un.fd_varsym->s_name);
        return (-1);
    }
    if (type != DT_ARRAY)
    {
        error("draw array: %s: not an array", x->x_data.fd_un.fd_varsym->s_name);
        return (-1);
    }
    array = *(t_array **)(((char *)data) + arrayonset);
    *elemtemplatesymp = elemtemplatesym;
    *arrayp = array;

    return (0);
}

static void drawarray_getrect(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    /* For now we just exclude drawarray from the bbox calculation.
       Otherwise it gets too expensive and interferes with realtime audio.

       If the user wants bbox they can add a [draw rect] for an anchor
       or manual bbox, or they can nest this in a [draw svg] for a viewport.

       If users really want a bbox for this in the future we can just use the
       same expensive algorithm as plot_getrect and suggest nesting in an
       [draw svg] for performance. But for now I don't think we need that. */
    *xp1 = *yp1 = 0x7fffffff;
    *xp2 = *yp2 = -0x7fffffff;
}

static void drawarray_displace(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int dx, int dy)
{
        /* not yet */
}

static void drawarray_select(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
    //fprintf(stderr,"drawarray_select %d\n", state);
    /* not yet */
}

static void drawarray_activate(t_gobj *z, t_glist *glist,
    t_word *data, t_template *template, t_float basex, t_float basey,
    int state)
{
        /* not yet */
}

static void drawarray_groupvis(t_scalar *x, t_glist *owner, t_word *data,
    t_template *template,
    t_glist *groupcanvas, t_glist *parent, t_float basex, t_float basey,
    t_array *parentarray)
{
    t_gobj *y;
    char tagbuf[MAXPDSTRING], parent_tagbuf[MAXPDSTRING];
    sprintf(tagbuf, "dgroup%lx.%lx", (long unsigned int)groupcanvas,
        (long unsigned int)data);
    sprintf(parent_tagbuf, "dgroup%lx.%lx", (long unsigned int)parent,
        (long unsigned int)data);
    gui_start_vmess("gui_scalar_draw_group", "xsss",
        glist_getcanvas(owner),
        tagbuf,
        parent_tagbuf,
        "g");
    svg_grouptogui(groupcanvas, template, data);
    gui_end_vmess();
    for (y = groupcanvas->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == canvas_class &&
            ((t_glist *)y)->gl_svg)
        {
            drawarray_groupvis(x, owner, data, template, (t_glist *)y,
                groupcanvas, basex, basey, parentarray);
        }
        t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
        if (!wb) continue;
        (*wb->w_parentvisfn)(y, owner, groupcanvas, x, data, template,
            basex, basey, parentarray, 1);
    }
}

/* todo: merge this with plot_has_drawcommand */
/* see if the elements we're plotting have any drawing commands */
int drawarray_has_drawcommand(t_canvas *elemtemplatecanvas)
{
    t_gobj *y;
    for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == canvas_class && ((t_glist *)y)->gl_svg)
            return 1;
        else if (class_isdrawcommand(y->g_pd))
            return 1;
    }
    return 0;
}

static void drawarray_vis(t_gobj *z, t_glist *glist, t_glist *parentglist,
    t_scalar *sc, t_word *data, t_template *template,
    t_float basex, t_float basey, t_array *parentarray, int tovis)
{
    t_drawarray *x = (t_drawarray *)z;
    int elemsize, yonset, wonset, xonset, i;
    t_canvas *elemtemplatecanvas;
    t_template *elemtemplate;
    t_symbol *elemtemplatesym;
    /* Let's just set constant increment values and see how they
       play out before adding xinc and yinc to the public interface... */
    t_float xinc = 40, xsum, yval;
    //t_float yinc = 40;
    t_array *array;
    int nelem;
    char *elem;
        
    if (drawarray_readownertemplate(x, data, template, 
        &elemtemplatesym, &array)
            || array_getfields(elemtemplatesym, &elemtemplatecanvas,
                &elemtemplate, &elemsize, 0, 0, 0,
                &xonset, &yonset, &wonset))
                    return;
    nelem = array->a_n;
    elem = (char *)array->a_vec;

    /* id for the the viewport-- we prefix it with "draw" to be
       compatible with the other svg-based drawcommands */
    char viewport_tagbuf[MAXPDSTRING];
    sprintf(viewport_tagbuf, "draw%lx.%lx",
        (long unsigned int)x, (long unsigned int)data);

    if (tovis)
    {
        int in_array = (sc->sc_vec == data) ? 0 : 1;
        int draw_scalars = plot_has_drawcommand(elemtemplatecanvas);

        /* make sure the array drawings are behind the graph */
        /* not doing this yet with the GUI port... */
        //sys_vgui(".x%lx.c lower plot%lx graph%lx\n", glist_getcanvas(glist),
        //    data, glist);

        /* 1. Set up the main <g> for this widget */
        char parent_tagbuf[MAXPDSTRING];
        sprintf(parent_tagbuf, "dgroup%lx.%lx",
            (in_array ? (long unsigned int)parentglist :
                        (long unsigned int)x->x_canvas),
            (long unsigned int)data);
        t_svg *sa = (t_svg *)x->x_attr;
        gui_start_vmess("gui_draw_vis", "xs",
            glist_getcanvas(glist), "g");
        svg_togui(sa, template, data);

        gui_start_array();
        gui_s(parent_tagbuf);
        gui_s(viewport_tagbuf);
        gui_end_array();
        gui_end_vmess();

            /* 2. Draw the individual elements */
            /* This code is inefficient since the template has to be
            searched for drawing instructions for every last point. */
        if (draw_scalars)
        {
            //t_float xoffset = in_array ? basex: 0;
            //t_float yoffset = in_array ? basey: 0;
            t_float xoffset = 0;
            t_float yoffset = 0;

            for (xsum = 0, i = 0; i < nelem; i++)
            {
                t_float usexloc, useyloc;
                t_gobj *y;
                if (xonset >= 0)
                    usexloc = xoffset +
                        *(t_float *)((elem + elemsize * i) + xonset);
                else usexloc = xoffset + xsum, xsum += xinc;
                if (yonset >= 0)
                    yval = *(t_float *)((elem + elemsize * i) + yonset);
                else yval = 0;
                useyloc = yoffset + yval;
                /*    fielddesc_cvttocoord(yfielddesc, yval); */
                 /* We're setting up a special group that will get set as
                   the parent by array elements */

                   /* todo: need to check if drawarray itself is in an array */
                char tagbuf[MAXPDSTRING];
                sprintf(tagbuf, "dgroup%lx.%lx",
                    (long unsigned int)elemtemplatecanvas,
                    (long unsigned int)((t_word *)(elem + elemsize * i)));
                char transform_buf[MAXPDSTRING];
                sprintf(transform_buf, "translate(%g,%g)", usexloc, useyloc);

                gui_start_vmess("gui_draw_vis", "xs",
                    glist_getcanvas(glist), "g");
                gui_start_array();
                /* For now we're not controlling x/y spacing at all. In the
                   future we might want to add a method to help define grid
                   spacing or something, but I can't currently think of a nice
                   interface for that. (The [plot] object just uses a flag to
                   reference fields from the element's template, but that is
                   too complicated and gets in the way of efficient redrawing.
                */
                //gui_s("transform");
                //gui_s(transform_buf);
                gui_end_array();
                gui_start_array();
                gui_s(viewport_tagbuf);
                gui_s(tagbuf);
                gui_end_array();
                gui_end_vmess();

                for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
                {
                    if (pd_class(&y->g_pd) == canvas_class &&
                        ((t_glist *)y)->gl_svg)
                    {
                        drawarray_groupvis(sc, glist,
                            (t_word *)(elem + elemsize * i),
                        template, (t_glist *)y, 
                            elemtemplatecanvas, usexloc, useyloc, array);
                    }
                    t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
                    if (!wb) continue;
                    (*wb->w_parentvisfn)(y, glist, elemtemplatecanvas, sc,
                        (t_word *)(elem + elemsize * i),
                            elemtemplate, usexloc, useyloc, array, tovis);
                }
            }
        }
        if (!glist_istoplevel(glist))
        {
            t_canvas *gl = glist_getcanvas(glist);
            char objtag[64];
            sprintf(objtag, ".x%lx.x%lx.template%lx",
                (t_int)gl, (t_int)glist, (t_int)data);
            canvas_restore_original_position(gl, (t_gobj *)glist, objtag, -1);
        }
    }
    else
    {
        /* un-draw the individual points */
        //fprintf(stderr,"drawarray_vis UNVIS\n");

        int i;
        for (i = 0; i < nelem; i++)
        {
            t_gobj *y;
            for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
            {
                t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
                if (!wb) continue;
                (*wb->w_parentvisfn)(y, glist, elemtemplatecanvas, sc,
                    (t_word *)(elem + elemsize * i), elemtemplate,
                        0, 0, array, 0);
            }
        }
        /* Now remove our drawarray svg container */
        gui_vmess("gui_draw_erase_item", "xs", glist_getcanvas(glist),
            viewport_tagbuf);
    }
}

static int drawarray_click(t_gobj *z, t_glist *glist, 
    t_word *data, t_template *template, t_scalar *sc, t_array *ap,
    t_float basex, t_float basey,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
/* Let's hold off on this for a bit...
    //fprintf(stderr,"drawarray_click %lx %lx %f %f %d %d\n",
    //    (t_int)z, (t_int)glist, basex, basey, xpix, ypix);
    t_drawarray *x = (t_drawarray *)z;
    t_symbol *elemtemplatesym;
    t_float linewidth, xloc, xinc, yloc, style, vis, scalarvis;
    t_array *array;
    t_fielddesc *xfielddesc, *yfielddesc, *wfielddesc;
    t_symbol *symfillcolor;
    t_symbol *symoutlinecolor;

    if (!drawarray_readownertemplate(x, data, template, 
        &elemtemplatesym, &array)
        && (vis != 0))
    {
    //fprintf(stderr,"  ->array_doclick\n");
        return (array_doclick(array, glist, sc, ap,
            elemtemplatesym,
            linewidth, basex + xloc, xinc, basey + yloc, scalarvis,
            xfielddesc, yfielddesc, wfielddesc,
            xpix, ypix, shift, alt, dbl, doit));
    }
    else return (0);
*/
    return 0;
}

static void drawarray_free(t_drawarray *x)
{
    //sys_queuegui(x->x_canvas, 0, canvas_redrawallfortemplatecanvas);
    /* decrement variable of the template
       to prevent transform as that would
       make arrays break their hitboxes
       and all kinds of other bad stuff */
    t_template *t = template_findbyname(
        canvas_makebindsym(x->x_canvas->gl_name)
    );
    if (t)
    {
        t->t_transformable--;
        //fprintf(stderr,"drawarray_free > template:%lx(%s) transform:%d\n",
        //    (t_int)t, canvas_makebindsym(x->x_canvas->gl_name)->s_name,
        //    t->t_transformable);
    }
}

t_parentwidgetbehavior drawarray_widgetbehavior =
{
    drawarray_getrect,
    drawarray_displace,
    drawarray_select,
    drawarray_activate,
    drawarray_vis,
    drawarray_click,
};

static void drawarray_setup(void)
{
    drawarray_class = class_new(gensym("drawarray"),
        0,
        (t_method)drawarray_free, sizeof(t_drawarray), 0, A_GIMME, 0);
    class_setdrawcommand(drawarray_class);
    class_addfloat(drawarray_class, drawarray_float);
    class_addmethod(drawarray_class, (t_method)drawarray_transform,
        gensym("transform"), A_GIMME, 0);
    class_addanything(drawarray_class, drawarray_anything);
    class_setparentwidget(drawarray_class, &drawarray_widgetbehavior);
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
    if (legacy_draw_in_group(canvas_getcurrent()))
        return 0;

    t_drawnumber *x = (t_drawnumber *)pd_new(drawnumber_class);
    char *classname = classsym->s_name;
    int flags = 0;
    int got_font_size = 0;
    
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

    if (argc == 2)
    {
        fielddesc_setfloatarg(&x->x_fontsize, argc--, argv++);
        got_font_size = 1;
    }
    if (argc)
    {
        if (argv->a_type == A_SYMBOL || got_font_size)
        {
            x->x_label = atom_getsymbolarg(0, argc, argv);
            if (!got_font_size) 
                fielddesc_setfloatarg(&x->x_fontsize, 0, NULL);         
        }
        else if (argv->a_type == A_FLOAT)
        {
            fielddesc_setfloatarg(&x->x_fontsize, argc, argv);
            x->x_label = &s_;
        }
    } else {
        fielddesc_setfloatarg(&x->x_fontsize, 0, NULL);
        x->x_label = &s_;
    }

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

/*#define DRAWNUMBER_BUFSIZE 80
static void drawnumber_sprintf(t_drawnumber *x, char *buf, t_atom *ap)
{
    int nchars;
    strncpy(buf, x->x_label->s_name, DRAWNUMBER_BUFSIZE);
    buf[DRAWNUMBER_BUFSIZE - 1] = 0;
    nchars = strlen(buf);
    atom_string(ap, buf + nchars, DRAWNUMBER_BUFSIZE - nchars);
}*/

static int drawnumber_gettype(t_drawnumber *x, t_word *data,
    t_template *template, int *onsetp)
{
    int type;
    t_symbol *arraytype;
    if (template_find_field(template, /*x->x_fieldname*/ x->x_value.fd_un.fd_varsym, onsetp, &type,
        &arraytype) && type != DT_ARRAY)
            return (type);
    else return (-1);
}

#define DRAWNUMBER_BUFSIZE 1024
static void drawnumber_getbuf(t_drawnumber *x, t_word *data,
    t_template *template, char *buf)
{
    int nchars, onset, type = drawnumber_gettype(x, data, template, &onset);
    if (type < 0)
        buf[0] = 0;
    else
    {
        strncpy(buf, x->x_label->s_name, DRAWNUMBER_BUFSIZE);
        buf[DRAWNUMBER_BUFSIZE - 1] = 0;
        nchars = strlen(buf);
        if (type == DT_TEXT)
        {
            char *buf2;
            int size2, ncopy;
            binbuf_gettext(((t_word *)((char *)data + onset))->w_binbuf,
                &buf2, &size2);
            ncopy = (size2 > DRAWNUMBER_BUFSIZE-1-nchars ?
                DRAWNUMBER_BUFSIZE-1-nchars: size2);
            memcpy(buf+nchars, buf2, ncopy);
            buf[nchars+ncopy] = 0;
            if (nchars+ncopy == DRAWNUMBER_BUFSIZE-1)
                strcpy(buf+(DRAWNUMBER_BUFSIZE-4), "...");
            t_freebytes(buf2, size2);
        }
        else
        {
            t_atom at;
            if (type == DT_FLOAT)
                SETFLOAT(&at, ((t_word *)((char *)data + onset))->w_float);
            else SETSYMBOL(&at, ((t_word *)((char *)data + onset))->w_symbol);
            atom_string(&at, buf + nchars, DRAWNUMBER_BUFSIZE - nchars);
        }
    }
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
    /* hack to keep the font scaling with the gop */
    t_float xscale = glist_xtopixels(glist, 1) - glist_xtopixels(glist, 0);
    t_float yscale = glist_ytopixels(glist, 1) - glist_ytopixels(glist, 0);

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
    //drawnumber_sprintf(x, buf, &at);
    drawnumber_getbuf(x, data, template, buf);
    *xp1 = xloc;
    *yp1 = yloc;
    // Ico 20140830: another regression from the 20140731 where getrect is not accurate
    // this, in addition to the vis call fix makes things work right again
    // namely, this fixes the getrect inconsistency, while the one in the vis
    // function fixes sizing problems
    *xp2 = xloc + (fontwidth * strlen(buf) * xscale);
    *yp2 = yloc + (fontheight * yscale);
    //*xp2 = xloc + (fontwidth * strlen(buf));
    //*yp2 = yloc + (fontheight);
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

static void drawnumber_vis(t_gobj *z, t_glist *glist, t_glist *parentglist,
    t_scalar *sc, t_word *data, t_template *template,
    t_float basex, t_float basey, t_array *parentarray, int vis)
{
    //fprintf(stderr,"drawnumber_vis %d\n", vis);
    t_drawnumber *x = (t_drawnumber *)z;

    /*// get the universal tag for all nested objects
    t_canvas *tag = x->x_canvas;
    while (tag->gl_owner)
    {
        tag = tag->gl_owner;
    }*/
    
        /* see comment in plot_vis() */
    //if (vis && !fielddesc_getfloat(&x->x_vis, template, data, 0))
    //    return;
    if (vis)
    {
        t_atom at;
        int in_array = (sc->sc_vec == data) ? 0 : 1;
        // Ico: why are we using scale here? For group transforms? I thought
        // that drawsymbol was not eligible for group transforms since it is 
        // a legacy object? keepin xscale and yscale 1.0 makes things look good
        // again on the disis_wiimote-help.pd patch
        t_float xscale = 1.0;
        t_float yscale = 1.0;
        /*t_float xscale = glist_xtopixels(glist, 1) - glist_xtopixels(glist, 0);
        t_float yscale = glist_ytopixels(glist, 1) - glist_ytopixels(glist, 0);
        if (xscale != 0) xscale = 1.0 / xscale;
        if (yscale != 0) yscale = 1.0 / yscale;*/
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
        drawnumber_getbuf(x, data, template, buf);
        //drawnumber_sprintf(x, buf, &at);

        char parent_tagbuf[MAXPDSTRING];
        sprintf(parent_tagbuf, "dgroup%lx.%lx",
            (in_array ? (long unsigned int)parentglist :
                        (long unsigned int)x->x_canvas),
            (long unsigned int)data);
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "drawnumber%lx.%lx", (long unsigned int)x, (long unsigned int)data);
        gui_vmess("gui_drawnumber_vis", "xssiiffsissii",
            glist_getcanvas(glist),
            parent_tagbuf,
            tagbuf,
            xloc,
            yloc, // Wrong-- we need to take font height into account
            xscale,
            yscale,
            sys_font,
            fontsize,
            colorstring,
            buf,
            vis,
            (int)fielddesc_getfloat(&x->x_vis, template, data, 0));
    }
    else
    {
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "drawnumber%lx.%lx", (long unsigned int)x, (long unsigned int)data);
        gui_vmess("gui_draw_erase_item", "xs", glist_getcanvas(glist),
            tagbuf);
    }
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
                    drawnumber_motion_glist, &drawnumber_motion_scalar->sc_gobj);
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
    if (legacy_draw_in_group(canvas_getcurrent()))
        return 0;

    t_drawsymbol *x = (t_drawsymbol *)pd_new(drawsymbol_class);
    char *classname = classsym->s_name;
    int flags = 0;
    int got_font_size = 0;
    
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

    if (argc == 2)
    {
        fielddesc_setfloatarg(&x->x_fontsize, argc--, argv++);
        got_font_size = 1;
    }
    if (argc)
    {
        if (argv->a_type == A_SYMBOL || got_font_size)
        {
            x->x_label = atom_getsymbolarg(0, argc, argv);
            if (!got_font_size) 
                fielddesc_setfloatarg(&x->x_fontsize, 0, NULL);         
        }
        else if (argv->a_type == A_FLOAT)
        {
            fielddesc_setfloatarg(&x->x_fontsize, argc, argv);
            x->x_label = &s_;
        }
    } else {
        fielddesc_setfloatarg(&x->x_fontsize, 0, NULL);
        x->x_label = &s_;
    }

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

/*#define DRAWSYMBOL_BUFSIZE 80
static void drawsymbol_sprintf(t_drawsymbol *x, char *buf, t_atom *ap)
{
    int nchars;
    strncpy(buf, x->x_label->s_name, DRAWSYMBOL_BUFSIZE);
    buf[DRAWSYMBOL_BUFSIZE - 1] = 0;
    nchars = strlen(buf);
    atom_string(ap, buf + nchars, DRAWSYMBOL_BUFSIZE - nchars);
    //fprintf(stderr,"drawsymbol_sprintf %s\n", buf);
}*/

static int drawsymbol_gettype(t_drawsymbol *x, t_word *data,
    t_template *template, int *onsetp)
{
    int type;
    t_symbol *arraytype;
    if (template_find_field(template, /*x->x_fieldname*/ x->x_value.fd_un.fd_varsym, onsetp, &type,
        &arraytype) && type != DT_ARRAY)
            return (type);
    else return (-1);
}

#define DRAWSYMBOL_BUFSIZE 1024
static void drawsymbol_getbuf(t_drawsymbol *x, t_word *data,
    t_template *template, char *buf)
{
    int nchars, onset, type = drawsymbol_gettype(x, data, template, &onset);
    if (type < 0)
        buf[0] = 0;
    else
    {
        strncpy(buf, x->x_label->s_name, DRAWSYMBOL_BUFSIZE);
        buf[DRAWSYMBOL_BUFSIZE - 1] = 0;
        nchars = strlen(buf);
        if (type == DT_TEXT)
        {
            char *buf2;
            int size2, ncopy;
            binbuf_gettext(((t_word *)((char *)data + onset))->w_binbuf,
                &buf2, &size2);
            ncopy = (size2 > DRAWSYMBOL_BUFSIZE-1-nchars ?
                DRAWSYMBOL_BUFSIZE-1-nchars: size2);
            memcpy(buf+nchars, buf2, ncopy);
            buf[nchars+ncopy] = 0;
            if (nchars+ncopy == DRAWSYMBOL_BUFSIZE-1)
                strcpy(buf+(DRAWSYMBOL_BUFSIZE-4), "...");
            t_freebytes(buf2, size2);
        }
        else
        {
            t_atom at;
            if (type == DT_FLOAT)
                SETFLOAT(&at, ((t_word *)((char *)data + onset))->w_float);
            else SETSYMBOL(&at, ((t_word *)((char *)data + onset))->w_symbol);
            atom_string(&at, buf + nchars, DRAWSYMBOL_BUFSIZE - nchars);
        }
    }
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
    /* hack to keep the font scaling with the gop */
    t_float xscale = glist_xtopixels(glist, 1) - glist_xtopixels(glist, 0);
    t_float yscale = glist_ytopixels(glist, 1) - glist_ytopixels(glist, 0);

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
    //drawsymbol_sprintf(x, buf, &at);
    drawsymbol_getbuf(x, data, template, buf);
    *xp1 = xloc;
    *yp1 = yloc;
    // Ico 20140830: another regression from the 20140731 where getrect is not accurate
    // this, in addition to the vis call fix makes things work right again
    // namely, this fixes the getrect inconsistency, while the one in the vis
    // function fixes sizing problems
    *xp2 = xloc + (fontwidth * strlen(buf) * xscale);
    *yp2 = yloc + (fontheight * yscale);
    //*xp2 = xloc + (fontwidth * strlen(buf));
    //*yp2 = yloc + (fontheight);
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

static void drawsymbol_vis(t_gobj *z, t_glist *glist, t_glist *parentglist,
    t_scalar *sc, t_word *data, t_template *template,
    t_float basex, t_float basey, t_array *parentarray, int vis)
{
    t_drawsymbol *x = (t_drawsymbol *)z;

    /*// get the universal tag for all nested objects
    t_canvas *tag = x->x_canvas;
    while (tag->gl_owner)
    {
        tag = tag->gl_owner;
    }*/
    
        /* see comment in plot_vis() */
    //if (vis && !fielddesc_getfloat(&x->x_vis, template, data, 0))
    //    return;
    if (vis)
    {
        t_atom at;
        int in_array = (sc->sc_vec == data) ? 0 : 1;
        // Ico: why are we using scale here? For group transforms? I thought
        // that drawsymbol was not eligible for group transforms since it is 
        // a legacy object? keeping xscale and yscale 1.0 makes things look good
        // again on the disis_wiimote-help.pd patch
        t_float xscale = 1.0;
        t_float yscale = 1.0;
        /*t_float xscale = glist_xtopixels(glist, 1) - glist_xtopixels(glist, 0);
        t_float yscale = glist_ytopixels(glist, 1) - glist_ytopixels(glist, 0);
        if (xscale != 0) xscale = 1.0 / xscale;
        if (yscale != 0) yscale = 1.0 / yscale;*/

        int fontsize = fielddesc_getfloat(&x->x_fontsize, template, data, 0);
        if (!fontsize) fontsize = glist_getfont(glist);
        /*int xloc = glist_xtopixels(glist,
            basex + fielddesc_getcoord(&x->x_xloc, template, data, 0));
        int yloc = glist_ytopixels(glist,
            basey + fielddesc_getcoord(&x->x_yloc, template, data, 0)); */
        int xloc = fielddesc_getcoord(&x->x_xloc, template, data, 0);
        int yloc = fielddesc_getcoord(&x->x_yloc, template, data, 0);

        char colorstring[20], buf[DRAWSYMBOL_BUFSIZE];
        numbertocolor(fielddesc_getfloat(&x->x_color, template, data, 1),
            colorstring);
        if (x->x_flags & DRAW_SYMBOL)
            SETSYMBOL(&at, fielddesc_getsymbol(&x->x_value, template, data, 0));
        else SETFLOAT(&at, fielddesc_getfloat(&x->x_value, template, data, 0));
        //drawsymbol_sprintf(x, buf, &at);
        drawsymbol_getbuf(x, data, template, buf);

        char parent_tagbuf[MAXPDSTRING];
        sprintf(parent_tagbuf, "dgroup%lx.%lx",
            (in_array ? (long unsigned int)parentglist :
                        (long unsigned int)x->x_canvas),
            (long unsigned int)data);
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "drawnumber%lx.%lx", (long unsigned int)x, (long unsigned int)data);

        gui_vmess("gui_drawnumber_vis", "xssiiffsissii",
            glist_getcanvas(glist),
            parent_tagbuf,
            tagbuf,
            xloc,
            yloc, // Wrong-- we need to take font height into account
            xscale,
            yscale,
            sys_font,
            fontsize,
            colorstring,
            buf,
            vis,
            (int)fielddesc_getfloat(&x->x_vis, template, data, 0));
    }
    else
    {
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "drawnumber%lx.%lx", (long unsigned int)x, (long unsigned int)data);
        gui_vmess("gui_draw_erase_item", "xs", glist_getcanvas(glist),
            tagbuf);
    }
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
                    drawsymbol_motion_glist, &drawsymbol_motion_scalar->sc_gobj);
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


#define DRAW_SPRITE 1

/* t_drawimage defined above */

static void *drawimage_new(t_symbol *classsym, int argc, t_atom *argv)
{
    t_drawimage *x = (t_drawimage *)pd_new(drawimage_class);

    /* we need a t_svg to associate with it */
    t_svg *sa = (t_svg *)svg_new((t_pd *)x, classsym, 0, 0);
    
    char *classname = classsym->s_name;
    char buf[50];
    sprintf(buf, "x%lx", (t_int)x);
    pd_bind(&x->x_obj.ob_pd, gensym(buf));
    int flags = 0;
    
    if (classname[0] == 's')
        flags |= DRAW_SPRITE;
    x->x_flags = flags;
    x->x_attr = (t_pd *)sa;
    fielddesc_setfloat_const(&x->x_vis, 1);
    fielddesc_setfloat_const(&x->x_index, 1);
    x->x_canvas = canvas_getcurrent();
    t_symbol *dir = canvas_getdir(x->x_canvas);
    if (argc && argv->a_type == A_SYMBOL)
        x->x_img = atom_getsymbolarg(0, argc--, argv++);
    else x->x_img = &s_;
    if (argc) svg_attr_setfloatarg(&sa->x_x, argc--, argv++);
    else svg_attr_setfloat_const(&sa->x_x, 0);
    if (argc) svg_attr_setfloatarg(&sa->x_y, argc--, argv++);
    else svg_attr_setfloat_const(&sa->x_y, 0);

    /* outlet for event notifications */
    outlet_new(&x->x_obj, &s_anything);

    /* [drawimage] allocates memory for an image or image sequence in
       the GUI. When we "vis" the scalar we fetch the data from the
       loaded images in the sequence. Then we can change the image shown
       by just sending an index to the GUI where it displays that particular
       image in the sequence without having to touch I/O or even parse
       image data and render a new image.
    */
    gui_vmess("gui_drawimage_new", "xssi",
        x,
        x->x_img->s_name,
        dir->s_name,
        x->x_flags);
    return (x);
}

void drawimage_size(t_drawimage *x, t_float w, t_float h)
{
    x->x_w = w;
    x->x_h = h;
}

static int drawimage_getindex(void *z, t_template *template, t_word *data)
{
    t_drawimage *x = (t_drawimage *)z;
    int index = (int)fielddesc_getcoord(&x->x_index, template, data, 1);
    return (index);
}

static void drawimage_index(t_drawimage *x, t_symbol *s, int argc,
    t_atom *argv)
{
    //t_canvas *c = canvas_templatecanvas_forgroup(x->x_canvas);
    if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        if (!(x->x_flags & DRAW_SPRITE))
            post("drawimage warning: sequence variable is only "
                 "used with drawsprite");
        fielddesc_setfloatarg(&x->x_index, argc, argv);
        svg_update((t_svg *)x->x_attr, gensym("index"));
    }
}

void drawimage_float(t_drawimage *x, t_floatarg f)
{
    t_atom at[1];
    SETFLOAT(at, f);
    /* note: no symbol set here */
    drawimage_index(x, 0, 1, at); 
}

void drawimage_symbol(t_drawimage *x, t_symbol *s)
{
    t_atom at[1];
    SETSYMBOL(at, s);
    /* note: no symbol set here */
    drawimage_index(x, 0, 1, at); 
}

/* With the current drawimage/sprite implementation we can't easily support
   the x and y attributes. The reason is that we're currently just applying
   attributes to the parent <g> for convenience, but <g> has no x/y atty.

   We could just forward everything to the child <image> element, but that
   could get clunky when dealing with large image sequences.

   So for now we hack around this by adding a <g> above our parent <g>, then
   converting the x/y in the GUI to a transform for that <g>. This is ugly
   and gets in the way of the interface built here, but it should work.
*/
static void drawimage_xy(t_drawimage *x, t_symbol *s, int argc,
    t_atom *argv)
{
    t_svg *sa = (t_svg *)x->x_attr;
    t_svg_attr *attr = svg_getattr(sa, s);
    if (!attr)
    {
        pd_error(x, "draw: can't find attribute %s", s->s_name);
        return;
    }
    if (argc < 1)
        attr->a_flag = 0;
    else if (argv[0].a_type == A_FLOAT || argv[0].a_type == A_SYMBOL)
    {
        fielddesc_setfloatarg(&attr->a_attr, argc, argv);
        attr->a_flag = 1;
        svg_update(sa, gensym("image_xy"));
    }
}

static void drawimage_anything(t_drawimage *x, t_symbol *s, int argc,
    t_atom *argv)
{
    pd_typedmess(x->x_attr, s, argc, argv);
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
    int x1, y1, x2, y2;
    x1 = y1 = 0x7fffffff;
    x2 = y2 = -0x7fffffff;

    //char buf[DRAWNUMBER_BUFSIZE];
    t_float mtx1[3][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
    t_float mtx2[3][3] = { {1, 0, 0}, {0, 1, 0}, {1, 0, 1} };
    t_float m1, m2, m3, m4, m5, m6,
            tx1, ty1, tx2, ty2, t5, t6;
    t_svg *sa = (t_svg *)x->x_attr;
    if (!fielddesc_getfloat(&sa->x_bbox, template, data, 0) ||
        (sa->x_vis.a_flag && !fielddesc_getfloat(&sa->x_vis.a_attr,
            template, data, 0)))
    {
        *xp1 = *yp1 = 0x7fffffff;
        *xp2 = *yp2 = -0x7fffffff;
        return;
    }

    svg_groupmtx(sa, template, data, mtx1);
    svg_parsetransform(sa, template, data, &m1, &m2, &m3,
        &m4, &m5, &m6);
    mset(mtx2, m1, m2, m3, m4, m5, m6);
    mmult(mtx1, mtx2, mtx1);
    xloc = fielddesc_getcoord(&sa->x_x.a_attr, template, data, 0);
    yloc = fielddesc_getcoord(&sa->x_y.a_attr, template, data, 0);
 
    mset(mtx2, xloc, yloc, xloc + x->x_w, yloc + x->x_h, 0, 0);
    mtx2[2][0] = 1; mtx2[2][1] = 1;
    mmult(mtx1, mtx2, mtx2);
    mget(mtx2, &tx1, &ty1, &tx2, &ty2, &t5, &t6);
    if (tx1 < x1) x1 = tx1;
    if (tx2 < x1) x1 = tx2;
    if (ty1 < y1) y1 = ty1;
    if (ty2 < y1) y1 = ty2;
    if (tx1 > x2) x2 = tx1;
    if (tx2 > x2) x2 = tx2;
    if (ty1 > y2) y2 = ty1;
    if (ty2 > y2) y2 = ty2;
    mset(mtx2, xloc, yloc + x->x_h, xloc + x->x_w, yloc, 0, 0);
    mtx2[2][0] = 1; mtx2[2][1] = 1;
    mmult(mtx1, mtx2, mtx2);
    mget(mtx2, &tx1, &ty1, &tx2, &ty2, &t5, &t6);
    if (tx1 < x1) x1 = tx1;
    if (tx2 < x1) x1 = tx2;
    if (ty1 < y1) y1 = ty1;
    if (ty2 < y1) y1 = ty2;
    if (tx1 > x2) x2 = tx1;
    if (tx2 > x2) x2 = tx2;
    if (ty1 > y2) y2 = ty1;
    if (ty2 > y2) y2 = ty2;
    //x1 = glist_xtopixels(glist, basex + x1);
    //x2 = glist_xtopixels(glist, basex + x2);
    //y1 = glist_ytopixels(glist, basey + y1);
    //y2 = glist_ytopixels(glist, basey + y2);

    x1 = basex + x1;
    x2 = basex + x2;
    y1 = basey + y1;
    y2 = basey + y2;

    /* todo: put these up top */
    if (!fielddesc_getfloat(&x->x_vis, template, data, 0))
    {
        *xp1 = *yp1 = 0x7fffffff;
        *xp2 = *yp2 = -0x7fffffff;
        return;
    }
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
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

static void drawimage_vis(t_gobj *z, t_glist *glist, t_glist *parentglist,
    t_scalar *sc, t_word *data, t_template *template,
    t_float basex, t_float basey, t_array *parentarray, int vis)
{
    t_drawimage *x = (t_drawimage *)z;
    t_svg *svg = (t_svg *)x->x_attr;
    t_canvas *parent = svg_parentcanvas(svg);
        /* see comment in plot_vis() */
    if (vis && !fielddesc_getfloat(&x->x_vis, template, data, 0))
        return;
    if (vis)
    {
        int in_array = (sc->sc_vec == data) ? 0: 1;
        /*int xloc = glist_xtopixels(glist,
            basex + fielddesc_getcoord(&svg->x_x.a_attr, template, data, 0));
        int yloc = glist_ytopixels(glist,
            basey + fielddesc_getcoord(&svg->x_y.a_attr, template, data, 0));
        sys_vgui("pdtk_drawimage_vis .x%lx.c %d %d .x%lx .x%lx.i %d ",*/
        t_float xloc = fielddesc_getcoord(&svg->x_x.a_attr, template, data, 0);
        t_float yloc = fielddesc_getcoord(&svg->x_y.a_attr, template, data, 0);

        char tagbuf[MAXPDSTRING];
        char parent_tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "draw%lx.%lx",
            (long unsigned int)x, (long unsigned int)data);
        sprintf(parent_tagbuf,"dgroup%lx.%lx",
            in_array ? (long unsigned int)parentglist : (long unsigned int)parent,
            (long unsigned int)data);

        gui_vmess("gui_drawimage_vis", "xffxxis",
            glist_getcanvas(glist),
            xloc,
            yloc,
            x,
            data,
            (int)fielddesc_getfloat(&x->x_index, template, data, 0),
            parent_tagbuf);

        gui_start_vmess("gui_draw_configure_all", "xs",
            glist_getcanvas(glist), tagbuf);
        svg_togui(svg, template, data);
        gui_end_vmess();
    }
    else
    {
        /* We don't actually need this-- the image should get destroyed
           automatically. */
        //sys_vgui("pdtk_drawimage_unvis .x%lx.c .x%lx.i\n",
        //    glist_getcanvas(glist), data);
    }
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
    t_svg *sa = (t_svg *)x->x_attr;
    t_fielddesc *f = &x->x_index;
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
    {
        //scalar_configure(drawimage_motion_scalar, drawimage_motion_glist);
        svg_update(sa, gensym("index"));
    }
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
        && x->x_index.fd_var &&
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
                fielddesc_getfloat(&x->x_index, template, data, 0);
            drawimage_motion_sprite = ((x->x_flags & DRAW_SPRITE) != 0);
            if (drawimage_motion_scalar)
                gpointer_setglist(&drawimage_motion_gpointer, 
                    drawimage_motion_glist, &drawimage_motion_scalar->sc_gobj);
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
    sprintf(buf, "x%lx", (long unsigned int)x);
    pd_unbind(&x->x_obj.ob_pd, gensym(buf));
    gui_vmess("gui_image_free", "x", x);
}

static void drawimage_setup(void)
{
    /* we need drawimage_class in order to get a different set of
       widget behavior than draw_class. But we also want to use the
       [draw shape] syntax for consistency. So for class_new we set
       the constructor to zero and call drawimage_new from inside
       draw_new. This way the user has to type "draw image" or "draw sprite"
       to create the objects. */
    drawimage_class = class_new(gensym("drawimage"),
        0, (t_method)drawimage_free,
        sizeof(t_drawimage), 0, A_GIMME, 0);
    class_setdrawcommand(drawimage_class);
    class_addfloat(drawimage_class, drawimage_float);
    class_addsymbol(drawimage_class, drawimage_symbol);
    class_addmethod(drawimage_class, (t_method)drawimage_size,
        gensym("size"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(drawimage_class, (t_method)drawimage_index,
        gensym("index"), A_GIMME, 0);
    class_addmethod(drawimage_class, (t_method)drawimage_xy,
        gensym("x"), A_GIMME, 0);
    class_addmethod(drawimage_class, (t_method)drawimage_xy,
        gensym("y"), A_GIMME, 0);
    class_addanything(drawimage_class, drawimage_anything);
    class_setparentwidget(drawimage_class, &drawimage_widgetbehavior);
}

/* ------------- convenience functions for all drawcommands --------------*/

t_template *canvas_findtemplate(t_canvas *c)
{
    t_gobj *g;
    for (g = c->gl_list; g; g = g->g_next)
    {
        if (pd_class(&g->g_pd) == gtemplate_class)
            return ((t_gtemplate *)g)->x_template;
    }
    return 0;
}

t_canvas *canvas_templatecanvas_forgroup(t_canvas *c)
{
    t_canvas *templatecanvas = c;
    if (!c->gl_owner)
        return templatecanvas;

   /* warning: this needs to be carefully considered-- seems like
      canvas's struct may not be initialized before the objects within
      it. */
    t_binbuf *b = c->gl_obj.te_binbuf;
    if (!b)
        return c;
    t_atom *argv = binbuf_getvec(b);
    if (binbuf_getnatom(b) > 1 &&
        atom_getsymbol(argv) == gensym("draw") &&
        (atom_getsymbol(argv+1) == gensym("g") ||
         atom_getsymbol(argv+1) == gensym("svg")))
    {
        templatecanvas = canvas_templatecanvas_forgroup(c->gl_owner);
    }
    return templatecanvas;
}

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
    else if (g->g_pd == plot_class)
        c = ((t_plot *)g)->x_canvas;
    else if (g->g_pd == drawarray_class)
        c = ((t_drawarray *)g)->x_canvas;
    else if (g->g_pd == canvas_class)
        c = (t_canvas *)g;
    else return (0);
    c = canvas_templatecanvas_forgroup(c);
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

/* set attributes for a scalar's parent draw command. This is handy
   because it saves having to erase and redraw the object. */
void svg_parentwidgettogui(t_gobj *z, t_scalar *sc, t_glist *owner,
    t_word *data, t_template *template)
{
    char tagbuf[MAXPDSTRING];
    if (pd_class(&z->g_pd) == draw_class)
    {
        t_draw *x = (t_draw *)z;
        sprintf(tagbuf, "draw%lx.%lx",
            (long unsigned int)x,
            (long unsigned int)data);
        gui_start_vmess("gui_draw_configure_all", "xs",
            glist_getcanvas(owner), tagbuf);
        svg_togui((t_svg *)x->x_attr, template, data);
        gui_end_vmess();
    }
    else if (pd_class(&z->g_pd) == drawimage_class)
    {
        t_drawimage *x = (t_drawimage *)z;
        sprintf(tagbuf, "draw%lx.%lx",
            (long unsigned int)x,
            (long unsigned int)data);
        gui_start_vmess("gui_draw_configure_all", "xs",
            glist_getcanvas(owner), tagbuf);
        svg_togui((t_svg *)x->x_attr, template, data);
        gui_end_vmess();
        gui_vmess("gui_drawimage_index", "xxxi",
            glist_getcanvas(owner), x, data,
            drawimage_getindex(x, template, data));
        gui_vmess("gui_drawimage_xy", "xxxff",
            glist_getcanvas(owner), x, data,
            fielddesc_getcoord(&((t_svg *)x->x_attr)->x_x.a_attr,
                template, data, 0),
            fielddesc_getcoord(&((t_svg *)x->x_attr)->x_y.a_attr,
                template, data, 0));
    }
    else if (pd_class(&z->g_pd) == curve_class)
    {
        /* For old commands we call the visfn with a flag of -1 to signal
           changing attributes instead of creating a new one.
           Not sure what to do with arrays yet-- we'll probably
           need a parentglist below instead of a "0" */
        curve_vis(z, owner, 0, sc, data, template, 0, 0, 0, -1);
    }
    else if (pd_class(&z->g_pd) == drawnumber_class)
        drawnumber_vis(z, owner, 0, sc, data, template, 0, 0, 0, -1);
    else if (pd_class(&z->g_pd) == drawsymbol_class)
        drawsymbol_vis(z, owner, 0, sc, data, template, 0, 0, 0, -1);
}

/* ---------------------- setup function ---------------------------- */

void g_template_setup(void)
{
    template_setup();
    gtemplate_setup();
    curve_setup();
    draw_setup();
    event_setup();
    plot_setup();
    drawnumber_setup();
    drawsymbol_setup();
    drawimage_setup();
    drawarray_setup();
}

