/* Copyright (c) 1997-2002 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* 
Routines to read and write canvases to files:
canvas_savetofile() writes a root canvas to a "pd" file.  (Reading "pd" files
is done simply by passing the contents to the pd message interpreter.)
Alternatively, the  glist_read() and glist_write() routines read and write
"data" from and to files (reading reads into an existing canvas), using a
file format as in the dialog window for data.
*/

#include <stdlib.h>
#include <stdio.h>
#include "m_pd.h"
#include "g_canvas.h"
#include <string.h>

/* object to assist in saving state by abstractions */
static t_class *savestate_class;

typedef struct _savestate
{
    t_object x_obj;
    t_outlet *x_stateout;
    t_outlet *x_bangout;
    t_binbuf *x_savetobuf;
} t_savestate;

static void *savestate_new(void)
{
    t_savestate *x = (t_savestate *)pd_new(savestate_class);
    x->x_stateout = outlet_new(&x->x_obj, &s_list);
    x->x_bangout = outlet_new(&x->x_obj, &s_bang);
    x->x_savetobuf = 0;
    return (x);
}

    /* call this when the owning abstraction's parent patch is saved so we
     *     can add state-restoring messages to binbuf */
static void savestate_doit(t_savestate *x, t_binbuf *b)
{
    x->x_savetobuf = b;
    outlet_bang(x->x_bangout);
    x->x_savetobuf = 0;
}

    /* called by abstraction in response to savestate_doit(); lists received
       here are added to the parent patch's save buffer after the line that will
       create the abstraction, addressed to "#A" which will be this patch after
       it is recreated by reopening the parent patch, pasting, or "undo". */
static void savestate_list(t_savestate *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->x_savetobuf)
    {
        binbuf_addv(x->x_savetobuf, "ss", gensym("#A"), gensym("saved"));
        binbuf_add(x->x_savetobuf, argc, argv);
        binbuf_addv(x->x_savetobuf, ";");
    }
    else pd_error(x, "savestate: ignoring message sent when not saving parent");
}

static void savestate_setup(void)
{
    savestate_class = class_new(gensym("savestate"),
        (t_newmethod)savestate_new, 0, sizeof(t_savestate), 0, 0);
    class_addlist(savestate_class, savestate_list);
}

void canvas_statesavers_doit(t_glist *x, t_binbuf *b)
{
    t_gobj *g;
    for (g = x->gl_list; g; g = g->g_next)
    {
        if (g->g_pd == savestate_class)
        {
            savestate_doit((t_savestate *)g, b);
        }
        else if (g->g_pd == canvas_class &&
            !canvas_isabstraction((t_canvas *)g))
        {
            canvas_statesavers_doit((t_glist *)g, b);
        }
    }
}

void canvas_saved(t_glist *x, t_symbol *s, int argc, t_atom *argv)
{
    t_gobj *g;
    for (g = x->gl_list; g; g = g->g_next)
    {
        if (g->g_pd == savestate_class)
        {
            outlet_list(((t_savestate *)g)->x_stateout, 0, argc, argv);
        }
        else if (g->g_pd == canvas_class &&
            !canvas_isabstraction((t_canvas *)g))
        {
            canvas_saved((t_glist *)g, s, argc, argv);
        }
    }
}

void canvas_savedeclarationsto(t_canvas *x, t_binbuf *b);
void canvas_saveabdefinitionsto(t_canvas *x, t_binbuf *b);

    /* the following routines read "scalars" from a file into a canvas. */

static int canvas_scanbinbuf(int natoms, t_atom *vec, int *p_indexout,
    int *p_next)
{
    int i;
    int indexwas = *p_next;
    *p_indexout = indexwas;
    if (indexwas >= natoms)
        return (0);
    for (i = indexwas; i < natoms && vec[i].a_type != A_SEMI; i++)
        ;
    if (i >= natoms)
        *p_next = i;
    else *p_next = i + 1;
    return (i - indexwas);
}

int canvas_readscalar(t_glist *x, int natoms, t_atom *vec,
     int *p_nextmsg, int selectit);


static void canvas_readerror(int natoms, t_atom *vec, int message, 
    int nline, char *s)
{
    error("%s", s);
    startpost("line was:");
    postatom(nline, vec + message);
    endpost();
}

    /* fill in the contents of the scalar into the vector w. */

static void glist_readatoms(t_glist *x, int natoms, t_atom *vec,
    int *p_nextmsg, t_symbol *templatesym, t_word *w, int argc, t_atom *argv)
{
    int message, n, i;

    t_template *template = template_findbyname(templatesym);
    if (!template)
    {
        error("%s: no such template", templatesym->s_name);
        *p_nextmsg = natoms;
        return;
    }
    word_restore(w, template, argc, argv);
    n = template->t_n;
    for (i = 0; i < n; i++)
    {
        if (template->t_vec[i].ds_type == DT_ARRAY)
        {
            t_array *a = w[i].w_array;
            int elemsize = a->a_elemsize, nitems = 0;
            t_symbol *arraytemplatesym = template->t_vec[i].ds_fieldtemplate;
            t_template *arraytemplate =
                template_findbyname(arraytemplatesym);
            if (!arraytemplate)
            {
                error("%s: no such template", arraytemplatesym->s_name);
            }
            else while (1)
            {
                t_word *element;
                int nline = canvas_scanbinbuf(natoms, vec, &message, p_nextmsg);
                    /* empty line terminates array */
                if (!nline)
                    break;
                array_resize(a, nitems + 1);
                element = (t_word *)(((char *)a->a_vec) +
                    nitems * elemsize);
                glist_readatoms(x, natoms, vec, p_nextmsg, arraytemplatesym,
                    element, nline, vec + message);
                nitems++;
            }
        }
        else if (template->t_vec[i].ds_type == DT_LIST)
        {
            /* nothing needs to happen here */
        }
        else if (template->t_vec[i].ds_type == DT_TEXT)
        {
            // Miller's addition for the [text] object
            t_binbuf *z = binbuf_new();
            int first = *p_nextmsg, last;
            for (last = first; last < natoms && vec[last].a_type != A_SEMI;
                last++);
            binbuf_restore(z, last-first, vec+first);
            binbuf_add(w[i].w_binbuf, binbuf_getnatom(z), binbuf_getvec(z));
            binbuf_free(z);
            last++;
            if (last > natoms) last = natoms;
            *p_nextmsg = last;
        }
    }
}

void scalar_doloadbang(t_scalar *x);

int canvas_readscalar(t_glist *x, int natoms, t_atom *vec,
    int *p_nextmsg, int selectit)
{
    int message, nline;
    t_template *template;
    t_symbol *templatesym;
    t_scalar *sc;
    int nextmsg = *p_nextmsg;
    int wasvis = glist_isvisible(x);

    if (nextmsg >= natoms || vec[nextmsg].a_type != A_SYMBOL)
    {
        if (nextmsg < natoms)
            post("stopping early: type %d", vec[nextmsg].a_type);
        *p_nextmsg = natoms;
        return (0);
    }
    templatesym = canvas_makebindsym(vec[nextmsg].a_w.w_symbol);
    *p_nextmsg = nextmsg + 1;
    
    if (!(template = template_findbyname(templatesym)))
    {
        error("canvas_read: %s: no such template", templatesym->s_name);
        *p_nextmsg = natoms;
        return (0);
    }
    sc = scalar_new(x, templatesym);
    if (!sc)
    {
        error("couldn't create scalar \"%s\"", templatesym->s_name);
        *p_nextmsg = natoms;
        return (0);
    }
    if (wasvis)
    {
            /* temporarily lie about vis flag while this is built */
        glist_getcanvas(x)->gl_mapped = 0;
    }
    glist_add(x, &sc->sc_gobj);
    
    nline = canvas_scanbinbuf(natoms, vec, &message, p_nextmsg);
    glist_readatoms(x, natoms, vec, p_nextmsg, templatesym, sc->sc_vec, 
        nline, vec + message);
    if (wasvis)
    {
            /* reset vis flag as before */
        glist_getcanvas(x)->gl_mapped = 1;
        gobj_vis(&sc->sc_gobj, x, 1);
    }
    if (selectit)
    {
        glist_select(x, &sc->sc_gobj);
    }
    /* send a loadbang for any canvas fields in this scalar */
    scalar_doloadbang(sc);
    return (1);
}

void glist_readfrombinbuf(t_glist *x, t_binbuf *b, char *filename, int selectem)
{
    int natoms, nline, message, nextmsg = 0;
    t_atom *vec;

    natoms = binbuf_getnatom(b);
    vec = binbuf_getvec(b);

    
            /* check for file type */
    nline = canvas_scanbinbuf(natoms, vec, &message, &nextmsg);
    if (nline != 1 && vec[message].a_type != A_SYMBOL &&
        strcmp(vec[message].a_w.w_symbol->s_name, "data"))
    {
        pd_error(x, "%s: file apparently of wrong type", filename);
        return;
    }
        /* read in templates and check for consistency */
    while (1)
    {
        t_template *newtemplate, *existtemplate;
        t_symbol *templatesym;
        t_atom *templateargs = getbytes(0);
        int ntemplateargs = 0, newnargs;
        nline = canvas_scanbinbuf(natoms, vec, &message, &nextmsg);
        if (nline < 2)
        {
            t_freebytes(templateargs, sizeof(*templateargs) * ntemplateargs);
            break;
        }
        else if (nline > 2)
            canvas_readerror(natoms, vec, message, nline,
                "extra items ignored");
        else if (vec[message].a_type != A_SYMBOL ||
            strcmp(vec[message].a_w.w_symbol->s_name, "template") ||
            vec[message + 1].a_type != A_SYMBOL)
        {
            canvas_readerror(natoms, vec, message, nline,
                "bad template header");
            continue;
        }
        templatesym = canvas_makebindsym(vec[message + 1].a_w.w_symbol);
        while (1)
        {
            nline = canvas_scanbinbuf(natoms, vec, &message, &nextmsg);
            if (nline != 2 && nline != 3)
                break;
            newnargs = ntemplateargs + nline;
            templateargs = (t_atom *)t_resizebytes(templateargs,
                sizeof(*templateargs) * ntemplateargs,
                sizeof(*templateargs) * newnargs);
            templateargs[ntemplateargs] = vec[message];
            templateargs[ntemplateargs + 1] = vec[message + 1];
            if (nline == 3)
                templateargs[ntemplateargs + 2] = vec[message + 2];
            ntemplateargs = newnargs;
        }
        if (!(existtemplate = template_findbyname(templatesym)))
        {
            error("%s: template not found in current patch",
                templatesym->s_name);
            t_freebytes(templateargs, sizeof (*templateargs) * ntemplateargs);
            return;
        }
        newtemplate = template_new(templatesym, ntemplateargs, templateargs);
        t_freebytes(templateargs, sizeof (*templateargs) * ntemplateargs);
        if (!template_match(existtemplate, newtemplate))
        {
            error("%s: template doesn't match current one",
                templatesym->s_name);
            pd_free(&newtemplate->t_pdobj);
            return;
        }
        pd_free(&newtemplate->t_pdobj);
    }
    while (nextmsg < natoms)
    {
        canvas_readscalar(x, natoms, vec, &nextmsg, selectem);
    }
}

static void glist_doread(t_glist *x, t_symbol *filename, t_symbol *format,
    int clearme)
{
    t_binbuf *b = binbuf_new();
    t_canvas *canvas = glist_getcanvas(x);
    int wasvis = glist_isvisible(canvas);
    int cr = 0;

    if (!strcmp(format->s_name, "cr"))
        cr = 1;
    else if (*format->s_name)
        error("qlist_read: unknown flag: %s", format->s_name);
    
    if (binbuf_read_via_canvas(b, filename->s_name, canvas, cr))
    {
        pd_error(x, "read failed");
        binbuf_free(b);
        return;
    }
    if (wasvis)
        canvas_vis(canvas, 0);
    if (clearme)
        glist_clear(x);
    glist_readfrombinbuf(x, b, filename->s_name, 0);
    if (wasvis)
        canvas_vis(canvas, 1);
    binbuf_free(b);
}

void glist_read(t_glist *x, t_symbol *filename, t_symbol *format)
{
    glist_doread(x, filename, format, 1);
}

void glist_mergefile(t_glist *x, t_symbol *filename, t_symbol *format)
{
    glist_doread(x, filename, format, 0);
}

    /* read text from a "properties" window, called from a gfxstub set
    up in scalar_properties().  We try to restore the object; if successful
    we delete the scalar and put the new thing in its place on the list. */
void canvas_dataproperties(t_canvas *x, t_scalar *sc, t_binbuf *b)
{
    int ntotal, nnew, scindex;
    t_gobj *y, *y2 = 0, *newone, *oldone = 0;
    t_template *template;
    for (y = x->gl_list, ntotal = 0, scindex = -1; y; y = y->g_next)
    {
        if (y == &sc->sc_gobj)
            scindex = ntotal, oldone = y;
        ntotal++;
    }
    
    if (scindex == -1)
    {
        error("data_properties: scalar disappeared");
        return;
    }
    glist_readfrombinbuf(x, b, "properties dialog", 0);
    newone = 0;
        /* take the new object off the list */
    if (ntotal)
    {
        for (y = x->gl_list, nnew = 1; y2 = y->g_next;
            y = y2, nnew++)
                if (nnew == ntotal)
        {
            newone = y2;
            gobj_vis(newone, x, 0);
            y->g_next = y2->g_next;
            break;    
        }
    }
    else gobj_vis((newone = x->gl_list), x, 0), x->gl_list = newone->g_next;
    if (!newone)
        error("couldn't update properties (perhaps a format problem?)");
    else if (!oldone)
        bug("data_properties: couldn't find old element");
    else if (newone->g_pd == scalar_class && oldone->g_pd == scalar_class
        && ((t_scalar *)newone)->sc_template ==
            ((t_scalar *)oldone)->sc_template 
        && (template = template_findbyname(((t_scalar *)newone)->sc_template)))
    {
            /* copy new one to old one and delete new one */
        int i;
        /* swap out the sc_vec field. That way we'll keep the one from
           the new scalar, and the old one will get freed by word_free (which
           gets called from pd_free below)
        */
        for (i = 0; i < template->t_n; i++)
        {
            t_word w = ((t_scalar *)newone)->sc_vec[i];
            ((t_scalar *)newone)->sc_vec[i] = ((t_scalar *)oldone)->sc_vec[i];
            ((t_scalar *)oldone)->sc_vec[i] = w;
        }
        pd_free(&newone->g_pd);
        if (glist_isvisible(x))
        {
            gobj_vis(oldone, x, 0);
            gobj_vis(oldone, x, 1);
        }
    }
    else
    {
            /* delete old one; put new one where the old one was on glist */
        glist_delete(x, oldone);
        if (scindex > 0)
        {
            for (y = x->gl_list, nnew = 1; y;
                y = y->g_next, nnew++)
                    if (nnew == scindex || !y->g_next)
            {
                newone->g_next = y->g_next;
                y->g_next = newone;
                goto didit;
            }
            bug("data_properties: can't reinsert");
        }
        else newone->g_next = x->gl_list, x->gl_list = newone;
    }
    // here we check for changes in scrollbar due to potential repositioning
    canvas_getscroll(x);
didit:
    ;
}

    /* ----------- routines to write data to a binbuf ----------- */

void canvas_doaddtemplate(t_symbol *templatesym, 
    int *p_ntemplates, t_symbol ***p_templatevec)
{
    int n = *p_ntemplates, i;
    t_symbol **templatevec = *p_templatevec;
    for (i = 0; i < n; i++)
        if (templatevec[i] == templatesym)
            return;
    templatevec = (t_symbol **)t_resizebytes(templatevec,
        n * sizeof(*templatevec), (n+1) * sizeof(*templatevec));
    templatevec[n] = templatesym;
    *p_templatevec = templatevec;
    *p_ntemplates = n+1;
}

static void glist_writelist(t_gobj *y, t_binbuf *b);

void binbuf_savetext(t_binbuf *bfrom, t_binbuf *bto);

void canvas_writescalar(t_symbol *templatesym, t_word *w, t_binbuf *b,
    int amarrayelement)
{
    t_template *template = template_findbyname(templatesym);
    t_atom *a = (t_atom *)t_getbytes(0);
    int i, n = template->t_n, natom = 0;
    if (!amarrayelement)
    {
        t_atom templatename;
        SETSYMBOL(&templatename, gensym(templatesym->s_name + 3));
        binbuf_add(b, 1, &templatename);
    }
    if (!template)
        bug("canvas_writescalar");
        /* write the atoms (floats and symbols) */
    for (i = 0; i < n; i++)
    {
        if (template->t_vec[i].ds_type == DT_FLOAT ||
            template->t_vec[i].ds_type == DT_SYMBOL)
        {
            a = (t_atom *)t_resizebytes(a,
                natom * sizeof(*a), (natom + 1) * sizeof (*a));
            if (template->t_vec[i].ds_type == DT_FLOAT)
                SETFLOAT(a + natom, w[i].w_float);
            else SETSYMBOL(a + natom,  w[i].w_symbol);
            natom++;
        }
    }
        /* array elements have to have at least something */
    if (natom == 0 && amarrayelement)
        SETSYMBOL(a + natom,  &s_bang), natom++;
    binbuf_add(b, natom, a);
    binbuf_addsemi(b);
    t_freebytes(a, natom * sizeof(*a));
    for (i = 0; i < n; i++)
    {
        if (template->t_vec[i].ds_type == DT_ARRAY)
        {
            int j;
            t_array *a = w[i].w_array;
            int elemsize = a->a_elemsize, nitems = a->a_n;
            t_symbol *arraytemplatesym = template->t_vec[i].ds_fieldtemplate;
            for (j = 0; j < nitems; j++)
                canvas_writescalar(arraytemplatesym,
                    (t_word *)(((char *)a->a_vec) + elemsize * j), b, 1);
            binbuf_addsemi(b);
        }
        else if (template->t_vec[i].ds_type == DT_LIST)
        {
            /* This needs to be revisited if we want to keep the
               canvas field API */
            //glist_writelist(w->w_list->gl_list, b);
            binbuf_addsemi(b);
        }
        else if (template->t_vec[i].ds_type == DT_TEXT)
        {
            // Miller's addition for the implementation of the [text] object
            binbuf_savetext(w[i].w_binbuf, b);
        }
    }
}

/*
static void glist_writelist(t_gobj *y, t_binbuf *b)
{
    for (; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == scalar_class)
        {
            canvas_writescalar(((t_scalar *)y)->sc_template,
                ((t_scalar *)y)->sc_vec, b, 0);
        }
    }
}
*/

    /* ------------ routines to write out templates for data ------- */

static void canvas_addtemplatesforlist(t_gobj *y,
    int  *p_ntemplates, t_symbol ***p_templatevec);

static void canvas_addtemplatesforscalar(t_symbol *templatesym,
    t_word *w, int *p_ntemplates, t_symbol ***p_templatevec)
{
    t_dataslot *ds;
    int i;
    t_template *template = template_findbyname(templatesym);
    canvas_doaddtemplate(templatesym, p_ntemplates, p_templatevec);
    if (!template)
        bug("canvas_addtemplatesforscalar");
    else for (ds = template->t_vec, i = template->t_n; i--; ds++, w++)
    {
        if (ds->ds_type == DT_ARRAY)
        {
            int j;
            t_array *a = w->w_array;
            int elemsize = a->a_elemsize, nitems = a->a_n;
            t_symbol *arraytemplatesym = ds->ds_fieldtemplate;
            canvas_doaddtemplate(arraytemplatesym, p_ntemplates, p_templatevec);
            for (j = 0; j < nitems; j++)
                canvas_addtemplatesforscalar(arraytemplatesym,
                    (t_word *)(((char *)a->a_vec) + elemsize * j), 
                        p_ntemplates, p_templatevec);
        }
        else if (ds->ds_type == DT_LIST)
        {
            /* This needs to be revisited if we want to keep the
               canvas field list */
            //canvas_addtemplatesforlist(w->w_list->gl_list,
            //    p_ntemplates, p_templatevec);
        }
        else if (ds->ds_type == DT_TEXT)
        {
            //canvas_addtemplatesforlist(w->w_list->gl_list,
            //    p_ntemplates, p_templatevec);
        }
    }
}

static void canvas_addtemplatesforstruct(t_template *template,
    int *p_ntemplates, t_symbol ***p_templatevec)
{
    t_dataslot *ds;
    int i;
    canvas_doaddtemplate(template->t_sym, p_ntemplates, p_templatevec);
    if (!template)
        bug("canvas_addtemplatesforscalar");
    else for (ds = template->t_vec, i = template->t_n; i--; ds++)
    {
        if (ds->ds_type == DT_ARRAY)
        {
            t_symbol *arraytemplatesym = ds->ds_fieldtemplate;
            t_template *arraytemplate = template_findbyname(arraytemplatesym);
            if (arraytemplate)
            {
                canvas_doaddtemplate(arraytemplatesym, p_ntemplates,
                    p_templatevec);
                canvas_addtemplatesforstruct(arraytemplate,
                        p_ntemplates, p_templatevec);
            }
        }
        /* not sure about this part-- all the sublist datatype seems
           to do is crash */
        //else if (ds->ds_type == DT_LIST)
        //    canvas_addtemplatesforlist(w->w_list->gl_list,
        //        p_ntemplates, p_templatevec);
    }
}

/*
static void canvas_addtemplatesforlist(t_gobj *y,
    int  *p_ntemplates, t_symbol ***p_templatevec)
{
    for (; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == scalar_class)
        {
            canvas_addtemplatesforscalar(((t_scalar *)y)->sc_template,
                ((t_scalar *)y)->sc_vec, p_ntemplates, p_templatevec);
        }
    }
}
*/

    /* write all "scalars" in a glist to a binbuf. */
t_binbuf *glist_writetobinbuf(t_glist *x, int wholething)
{
    int i;
    t_symbol **templatevec = getbytes(0);
    int ntemplates = 0;
    t_gobj *y;
    t_binbuf *b = binbuf_new();

    for (y = x->gl_list; y; y = y->g_next)
    {
        if ((pd_class(&y->g_pd) == scalar_class) &&
            (wholething || glist_isselected(x, y)))
        {
            canvas_addtemplatesforscalar(((t_scalar *)y)->sc_template,
                ((t_scalar *)y)->sc_vec,  &ntemplates, &templatevec);
        }
    }
    binbuf_addv(b, "s;", gensym("data"));
    for (i = 0; i < ntemplates; i++)
    {
        t_template *template = template_findbyname(templatevec[i]);
        int j, m = template->t_n;
            /* drop "pd-" prefix from template symbol to print it: */
        binbuf_addv(b, "ss;", gensym("template"),
            gensym(templatevec[i]->s_name + 3));
        for (j = 0; j < m; j++)
        {
            t_symbol *type;
            switch (template->t_vec[j].ds_type)
            {
                case DT_FLOAT: type = &s_float; break;
                case DT_SYMBOL: type = &s_symbol; break;
                case DT_ARRAY: type = gensym("array"); break;
                case DT_LIST: type = gensym("canvas"); break;
                case DT_TEXT: type = &s_list; break;
                default: type = &s_float; bug("canvas_write");
            }
            if (template->t_vec[j].ds_type == DT_ARRAY ||
                template->t_vec[j].ds_type == DT_LIST)
                binbuf_addv(b, "sss;", type, template->t_vec[j].ds_name,
                    gensym(template->t_vec[j].ds_fieldtemplate->s_name + 3));
            else binbuf_addv(b, "ss;", type, template->t_vec[j].ds_name);
        }
        binbuf_addsemi(b);
    }
    binbuf_addsemi(b);
        /* now write out the objects themselves */
    for (y = x->gl_list; y; y = y->g_next)
    {
        if ((pd_class(&y->g_pd) == scalar_class) &&
            (wholething || glist_isselected(x, y)))
        {
            canvas_writescalar(((t_scalar *)y)->sc_template,
                ((t_scalar *)y)->sc_vec,  b, 0);
        }
    }
    t_freebytes(templatevec, ntemplates * sizeof(*templatevec));
    return (b);
}

static void glist_write(t_glist *x, t_symbol *filename, t_symbol *format)
{
    int cr = 0;
    t_binbuf *b;
    char buf[MAXPDSTRING];
    t_canvas *canvas = glist_getcanvas(x);
    canvas_makefilename(canvas, filename->s_name, buf, MAXPDSTRING);
    if (!strcmp(format->s_name, "cr"))
        cr = 1;
    else if (*format->s_name)
        error("qlist_read: unknown flag: %s", format->s_name);
    
    b = glist_writetobinbuf(x, 1);
    if (b)
    {
        if (binbuf_write(b, buf, "", cr))
            error("%s: write failed", filename->s_name);
        binbuf_free(b);
    }
}

/* ------ routines to save and restore canvases (patches) recursively. ----*/

static double zoom_hack(int x, int zoom)
{
  // AG: This employs an interesting little hack made possible by a loophole
  // in Pd's patch parser (binbuf_eval), which will happily read a float value
  // where an int is expected, and cast it to an int anyway. Applied to the #N
  // header of a canvas, this lets us store the zoom level of a patch in the
  // fractional part of a window geometry argument x, so that we can restore
  // it when the patch is reloaded. Since vanilla and other Pd flavors won't
  // even notice the extra data, the same patch can still be loaded by any
  // other Pd version without any problems.

  // Encoding of the zoom level: Purr Data's zoom levels go from -7 to 8 so
  // that we can encode them as 4 bit numbers in two's complement. However, it
  // makes sense to leave 1 extra bit for future extensions, so our encoding
  // actually uses nonnegative 5 bit integer multiples of 2^-5 = 0.03125 in a
  // way that leaves the integer part of the height parameter intact. Note
  // that using 2's complement here has the advantage that a zero zoom level
  // maps to a zero fraction so that we don't need an extra bit to detect
  // whether there's a zoom level when reading the patch.
  return x + (zoom & 31) * 0.03125;
}

    /* save to a binbuf, called recursively; cf. canvas_savetofile() which
    saves the document, and is only called on root canvases. */
static void canvas_saveto(t_canvas *x, t_binbuf *b)
{
    t_gobj *y;
    t_linetraverser t;
    t_outconnect *oc;
    extern int sys_zoom;
        /* subpatch */
    if (x->gl_owner && !x->gl_env)
    {
        /* have to go to original binbuf to find out how we were named. */
        //fprintf(stderr,"saving subpatch\n");
        t_binbuf *bz = binbuf_new();
        t_symbol *patchsym, *selector;
        int name_index;
        binbuf_addbinbuf(bz, x->gl_obj.ob_binbuf);
        selector = atom_getsymbolarg(0, binbuf_getnatom(bz), binbuf_getvec(bz));
        /* For [draw group] we save the name as the third argument. This
           is rather obscure but it might be handy if people want to
           dynamically create objects inside a [draw group] */
        if (selector == gensym("draw")) name_index = 2;
        else name_index = 1;
        patchsym = atom_getsymbolarg(name_index,
            binbuf_getnatom(bz), binbuf_getvec(bz));
        binbuf_free(bz);
        // look up the enclosing root or abstraction for the zoomflag value
        // (this is where 'declare -zoom' stores it)
        t_glist *gl = x;
        while (!gl->gl_env && gl->gl_owner) {
          gl = gl->gl_owner;
        }
        if (gl->gl_zoomflag && x->gl_zoom != 0) {
          // This uses the hack described above to store the zoom factor in
          // the fractional part of the windows height parameter. Note that
          // any of the other canvas geometry parameters would do just as
          // well, as they are all measured in pixels and thus unlikely to
          // become fractional in the future.
          binbuf_addv(b, "ssiiifsi;", gensym("#N"), gensym("canvas"),
                      (int)(x->gl_screenx1),
                      (int)(x->gl_screeny1),
                      (int)(x->gl_screenx2 - x->gl_screenx1),
                      zoom_hack(x->gl_screeny2 - x->gl_screeny1, x->gl_zoom),
                      (patchsym != &s_ ? patchsym: gensym("(subpatch)")),
                      x->gl_mapped);
        } else {
          // This is the standard (vanilla) code. This is also used in the
          // case of a zero zoom level, so unzoomed patches will always
          // produce a standard vanilla patch file when saved.
          binbuf_addv(b, "ssiiiisi;", gensym("#N"), gensym("canvas"),
                      (int)(x->gl_screenx1),
                      (int)(x->gl_screeny1),
                      (int)(x->gl_screenx2 - x->gl_screenx1),
                      (int)(x->gl_screeny2 - x->gl_screeny1),
                      (patchsym != &s_ ? patchsym: gensym("(subpatch)")),
                      x->gl_mapped);
        }
        /* Not sure what the following commented line was doing... */
        //t_text *xt = (t_text *)x;
    }
        /* root or abstraction */
    else 
    {
        // See above.
        if (x->gl_zoomflag && x->gl_zoom != 0) {
          binbuf_addv(b, "ssiiifi;", gensym("#N"), gensym("canvas"),
                      (int)(x->gl_screenx1),
                      (int)(x->gl_screeny1),
                      (int)(x->gl_screenx2 - x->gl_screenx1),
                      zoom_hack(x->gl_screeny2 - x->gl_screeny1, x->gl_zoom),
                      (int)x->gl_font);
        } else {
          binbuf_addv(b, "ssiiiii;", gensym("#N"), gensym("canvas"),
                      (int)(x->gl_screenx1),
                      (int)(x->gl_screeny1),
                      (int)(x->gl_screenx2 - x->gl_screenx1),
                      (int)(x->gl_screeny2 - x->gl_screeny1),
                      (int)x->gl_font);
        }
        canvas_savedeclarationsto(x, b);
    }

    canvas_saveabdefinitionsto(x, b);

    for (y = x->gl_list; y; y = y->g_next)
        gobj_save(y, b);

    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
    {
        if (outconnect_visible(oc))
        {
            int srcno = canvas_getindex(x, &t.tr_ob->ob_g);
            int sinkno = canvas_getindex(x, &t.tr_ob2->ob_g);
            binbuf_addv(b, "ssiiii;", gensym("#X"), gensym("connect"),
                srcno, t.tr_outno, sinkno, t.tr_inno);
        }
    }
        /* unless everything is the default (as in ordinary subpatches)
        print out a "coords" message to set up the coordinate systems */
    if (x->gl_isgraph || x->gl_x1 || x->gl_y1 ||
        x->gl_x2 != 1 ||  x->gl_y2 != 1 || x->gl_pixwidth || x->gl_pixheight)
    {
        if (x->gl_isgraph && x->gl_goprect)
                /* if we have a graph-on-parent rectangle, we're new style.
                The format is arranged so
                that old versions of Pd can at least do something with it. */
            binbuf_addv(b, "ssfffffffff;", gensym("#X"), gensym("coords"),
                x->gl_x1, x->gl_y1,
                x->gl_x2, x->gl_y2,
                (t_float)x->gl_pixwidth, (t_float)x->gl_pixheight,
                (t_float)((x->gl_hidetext)?2.:1.),
                (t_float)x->gl_xmargin, (t_float)x->gl_ymargin); 
                    /* otherwise write in 0.38-compatible form */
        else binbuf_addv(b, "ssfffffff;", gensym("#X"), gensym("coords"),
                x->gl_x1, x->gl_y1,
                x->gl_x2, x->gl_y2,
                (t_float)x->gl_pixwidth, (t_float)x->gl_pixheight,
                (t_float)x->gl_isgraph);
    }
        /* save a message if scrollbars are disabled-- otherwise do nothing
           for the sake of backwards compatibility. */
    if (x->gl_noscroll)
        binbuf_addv(b, "ssi;", gensym("#X"), gensym("scroll"), x->gl_noscroll);
        /* same for menu */
    if (x->gl_nomenu)
        binbuf_addv(b, "ssi;", gensym("#X"), gensym("menu"), x->gl_nomenu);
}

/* yuck, wish I didn't have to do this... */
extern t_class *gtemplate_class;

    /* call this recursively to collect all the template names for
    a canvas or for the selection. */
static void canvas_collecttemplatesfor(t_canvas *x, int *ntemplatesp,
    t_symbol ***templatevecp, int wholething)
{
    t_gobj *y;

    for (y = x->gl_list; y; y = y->g_next)
    {
        if ((pd_class(&y->g_pd) == scalar_class) &&
            (wholething || glist_isselected(x, y)))
                canvas_addtemplatesforscalar(((t_scalar *)y)->sc_template,
                    ((t_scalar *)y)->sc_vec,  ntemplatesp, templatevecp);
        else if ((pd_class(&y->g_pd) == gtemplate_class) &&
            (wholething || glist_isselected(x, y)))
                canvas_addtemplatesforstruct(gtemplate_get((t_gtemplate *)y),
                    ntemplatesp, templatevecp);
        else if ((pd_class(&y->g_pd) == canvas_class) &&
            (wholething || glist_isselected(x, y)))
                canvas_collecttemplatesfor((t_canvas *)y,
                    ntemplatesp, templatevecp, 1);
    }
}

    /* save the templates needed by a canvas to a binbuf. */
static void canvas_savetemplatesto(t_canvas *x, t_binbuf *b, int wholething)
{
    t_symbol **templatevec = getbytes(0);
    int i, ntemplates = 0;
    canvas_collecttemplatesfor(x, &ntemplates, &templatevec, wholething);
    for (i = 0; i < ntemplates; i++)
    {
        t_template *template = template_findbyname(templatevec[i]);
        int j, m = template->t_n;
        if (!template)
        {
            bug("canvas_savetemplatesto");
            continue;
        }
            /* drop "pd-" prefix from template symbol to print */
        binbuf_addv(b, "sss", &s__N, gensym("struct"),
            gensym(templatevec[i]->s_name + 3));
        for (j = 0; j < m; j++)
        {
            t_symbol *type;
            switch (template->t_vec[j].ds_type)
            {
                case DT_FLOAT: type = &s_float; break;
                case DT_SYMBOL: type = &s_symbol; break;
                case DT_ARRAY: type = gensym("array"); break;
                case DT_LIST: type = gensym("canvas"); break;
                case DT_TEXT: type = gensym("text"); break; //&s_list; break;
                default: type = &s_float; bug("canvas_write");
            }
            if (template->t_vec[j].ds_type == DT_LIST)
                binbuf_addv(b, "sss", type, template->t_vec[j].ds_name,
                    gensym(template->t_vec[j].ds_fieldtemplate->s_name));
            else if (template->t_vec[j].ds_type == DT_ARRAY)
                binbuf_addv(b, "sss", type, template->t_vec[j].ds_name,
                    gensym(template->t_vec[j].ds_fieldtemplate->s_name + 3));
            else binbuf_addv(b, "ss", type, template->t_vec[j].ds_name);
        }
        binbuf_addsemi(b);
    }
}

void canvas_reload(t_symbol *name, t_symbol *dir, t_gobj *except);
extern void canvasgop_checksize(t_canvas *x);

    /* save a "root" canvas to a file; cf. canvas_saveto() which saves the
    body (and which is called recursively.) */
static void canvas_savetofile(t_canvas *x, t_symbol *filename, t_symbol *dir,
    t_floatarg fdestroy)
{
    t_binbuf *b = binbuf_new();
    canvas_savetemplatesto(x, b, 1);
    canvas_saveto(x, b);
    if (binbuf_write(b, filename->s_name, dir->s_name, 0)) sys_ouch();
    else
    {
            /* if not an abstraction, reset title bar and directory */ 
        if (!x->gl_owner)
        {
            canvas_rename(x, filename, dir);
            /* update window list in case Save As changed the window name */
            canvas_updatewindowlist(); 
        }
        post("saved to: %s/%s", dir->s_name, filename->s_name);
        canvas_dirty(x, 0);
        if (x->gl_isgraph)
            canvasgop_checksize(x);
        canvas_reload(filename, dir, &x->gl_gobj);
        if (fdestroy != 0)
            vmess(&x->gl_pd, gensym("menuclose"), "f", 1.);
    }
    binbuf_free(b);
}

void canvas_reload_ab(t_canvas *x);

/* updates the shared ab definition and reloads all instances */
static void canvas_save_ab(t_canvas *x, t_floatarg fdestroy)
{
    if(!x->gl_absource) bug("canvas_save_ab");

    t_binbuf *b = binbuf_new();
    canvas_savetemplatesto(x, b, 1);
    canvas_saveto(x, b);

    binbuf_free(x->gl_absource->ad_source);
    x->gl_absource->ad_source = b;

    canvas_dirty(x, 0);
    canvas_reload_ab(x);

    if (fdestroy != 0) //necessary?
        vmess(&x->gl_pd, gensym("menuclose"), "f", 1.);
}

static void canvas_menusaveas(t_canvas *x, t_floatarg fdestroy)
{
    t_canvas *x2 = canvas_getrootfor(x);
    if(!x->gl_isab)
        gui_vmess("gui_canvas_saveas", "xssi",
            x2,
            (strncmp(x2->gl_name->s_name, "Untitled", 8) ?
                x2->gl_name->s_name : "title"),
            canvas_getdir(x2)->s_name,
            fdestroy != 0);
    else if(x->gl_dirty)
        canvas_save_ab(x2, fdestroy);
}

static void canvas_menusave(t_canvas *x, t_floatarg fdestroy)
{
    t_canvas *x2 = canvas_getrootfor(x);
    char *name = x2->gl_name->s_name;
    if(!x->gl_isab)
    {
        if (*name && strncmp(name, "Untitled", 8)
                && (strlen(name) < 4 || strcmp(name + strlen(name)-4, ".pat")
                    || strcmp(name + strlen(name)-4, ".mxt")))
                canvas_savetofile(x2, x2->gl_name, canvas_getdir(x2), fdestroy);
        else canvas_menusaveas(x2, fdestroy);
    }
    else if(x->gl_dirty)
        canvas_save_ab(x2, fdestroy);
}

static void canvas_menuprint(t_canvas *x)
{
    t_canvas *x2 = canvas_getrootfor(x);
    gui_vmess("gui_canvas_print", "xss", x, x->gl_name->s_name, canvas_getdir(x2)->s_name);
}

void g_readwrite_setup(void)
{
    savestate_setup();
    class_addmethod(canvas_class, (t_method)glist_write,
        gensym("write"), A_SYMBOL, A_DEFSYM, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_read,
        gensym("read"), A_SYMBOL, A_DEFSYM, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_mergefile,
        gensym("mergefile"), A_SYMBOL, A_DEFSYM, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_savetofile,
        gensym("savetofile"), A_SYMBOL, A_SYMBOL, A_DEFFLOAT, 0);
    class_addmethod(canvas_class, (t_method)canvas_saveto,
        gensym("saveto"), A_CANT, 0);
    class_addmethod(canvas_class, (t_method)canvas_saved,
        gensym("saved"), A_GIMME, 0);
/* ------------------ from the menu ------------------------- */
    class_addmethod(canvas_class, (t_method)canvas_menusave,
        gensym("menusave"), A_DEFFLOAT, 0);
    class_addmethod(canvas_class, (t_method)canvas_menusaveas,
        gensym("menusaveas"), A_DEFFLOAT, 0);
    class_addmethod(canvas_class, (t_method)canvas_menuprint,
        gensym("menuprint"), 0);
}

void canvas_readwrite_for_class(t_class *c)
{
    class_addmethod(c, (t_method)canvas_menusave,
        gensym("menusave"), 0);
    class_addmethod(c, (t_method)canvas_menusaveas,
        gensym("menusaveas"), 0);
    class_addmethod(c, (t_method)canvas_menuprint,
        gensym("menuprint"), 0);
}
