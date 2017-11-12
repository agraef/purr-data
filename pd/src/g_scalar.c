/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This file defines the "scalar" object, which is not a text object, just a
"gobj".  Scalars have templates which describe their structures, which
can contain numbers, sublists, and arrays.

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>      /* for read/write to files */
#include "m_pd.h"
#include "g_canvas.h"

t_class *scalar_class;

void pd_doloadbang(void);

extern t_symbol *canvas_field_templatesym; /* for "canvas" data type */
extern t_word *canvas_field_vec;           /* for "canvas" data type */
extern t_gpointer *canvas_field_gp;        /* parent for "canvas" data type */

void word_init(t_word *data, t_template *template, t_gpointer *gp)
{
    int i, nitems = template->t_n;
    t_dataslot *datatypes = template->t_vec;
    t_word *wp = data;
    for (i = 0; i < nitems; i++, datatypes++, wp++)
    {
        int type = datatypes->ds_type;
        if (type == DT_FLOAT)
            wp->w_float = 0; 
        else if (type == DT_SYMBOL)
            wp->w_symbol = &s_symbol;
        else if (type == DT_ARRAY)
        {
            wp->w_array = array_new(datatypes->ds_fieldtemplate, gp);
        }
        else if (type == DT_LIST)
        {
            /* we feed these values to global vars so that we can 
               read them from inside canvas_new.  This is very hacky, 
               but I couldn't figure out a better way to do it. */
            canvas_field_templatesym = template->t_sym;
            /* this is bad-- we're storing a reference to a position in
               a dynamically allocated byte array when realloc can potentially
               move this data.  Essentially, we're depending on gcc to never
               move it, which is a bad assumption.  Unfortunately gpointers
               do the same thing, and I haven't heard back from Miller yet
               on how he plans to deal with this problem. Hopefully that same
               solution will be usable here. */
            canvas_field_vec = data;
            /* Here too we're being dangerous-- I'm copying the gpointer
               without recounting, and I'm not unsetting the one that's 
               part of the _glist struct (t_gpointer gl_gp). */
            canvas_field_gp = gp;

            /* copied from glob_evalfile... */
            t_pd *x = 0;
            /* even though binbuf_evalfile appears to take care of dspstate,
            we have to do it again here, because canvas_startdsp() assumes
            that all toplevel canvases are visible.  LATER check if this
            is still necessary -- probably not. */
            int dspstate = canvas_suspend_dsp();
            // this needs to be set to sane symbols,
            // possibly stored in the dataslot...
            glob_setfilename(0, gensym("foo"), gensym("bar"));
            t_pd *boundx = s__X.s_thing;
            s__X.s_thing = 0;       /* don't save #X; we'll need to leave it
                                       bound for the caller to grab it. */

            /* copied from binbuf_evalfile... we need to refactor at some
               point... */
            /* save bindings of symbols #N, #A (and restore afterward) */
            t_pd *bounda = gensym("#A")->s_thing, *boundn = s__N.s_thing;
            gensym("#A")->s_thing = 0;
            s__N.s_thing = &pd_canvasmaker;
            binbuf_eval(datatypes->ds_binbuf, 0, 0, 0);
            gensym("#A")->s_thing = bounda;
            s__N.s_thing = boundn;
            glob_setfilename(0, &s_, &s_);

            wp->w_list = canvas_getcurrent();
            wp->w_list->gl_templatesym = template->t_sym;
            /* make the parent glist the parent of our canvas field */
            wp->w_list->gl_owner = gp->gp_stub->gs_un.gs_glist;

            while ((x != s__X.s_thing) && s__X.s_thing) 
            {
                x = s__X.s_thing;
                vmess(x, gensym("pop"), "i", 0);
            }
            /* oops, can't actually do a loadbang here.
               Why?
               Consider getting the value of a fielddesc that hasn't
               been init'd yet... */
            /* pd_doloadbang(); */
            canvas_resume_dsp(dspstate);

            s__X.s_thing = boundx;
            post("eval'd a canvas with addy x%lx", (long unsigned int)
                wp->w_list);
        }
        else if (type == DT_TEXT)
        {
            // Miller's [text] object addition
            wp->w_binbuf = binbuf_new();
        }
    }
}

void scalar_doloadbang(t_scalar *x)
{
    t_template *template = template_findbyname(x->sc_template);
    t_dataslot *datatypes = template->t_vec;
    t_word *wp = x->sc_vec;
    int i, nitems = template->t_n;
    for (i = 0; i < nitems; i++, datatypes++, wp++)
    {
        if (datatypes->ds_type == DT_LIST)
        {
            t_canvas *c = wp->w_list;
            pd_vmess((t_pd *)c, gensym("loadbang"), "");
        }
    }
}

void word_restore(t_word *wp, t_template *template,
    int argc, t_atom *argv)
{
    int i, nitems = template->t_n;
    t_dataslot *datatypes = template->t_vec;
    for (i = 0; i < nitems; i++, datatypes++, wp++)
    {
        int type = datatypes->ds_type;
        if (type == DT_FLOAT)
        {
            t_float f;
            if (argc)
            {
                f =  atom_getfloat(argv);
                argv++, argc--;
            }
            else f = 0;
            wp->w_float = f; 
        }
        else if (type == DT_SYMBOL)
        {
            t_symbol *s;
            if (argc)
            {
                s =  atom_getsymbol(argv);
                argv++, argc--;
            }
            else s = &s_;
            wp->w_symbol = s;
        }
    }
    if (argc)
        post("warning: word_restore: extra arguments");
}

void word_free(t_word *wp, t_template *template)
{
    int i;
    t_dataslot *dt;
    for (dt = template->t_vec, i = 0; i < template->t_n; i++, dt++)
    {
        if (dt->ds_type == DT_ARRAY)
            array_free(wp[i].w_array);
        else if (dt->ds_type == DT_LIST)
            canvas_free(wp[i].w_list);
        else if (dt->ds_type == DT_TEXT)
            binbuf_free(wp[i].w_binbuf);
    }
}

/* some of this code is used in a function in g_canvas.c...
   need to modularize it */
static t_object *template_getstruct(t_template *template)
{
    if (template)
    {
        t_gobj *y;
        t_canvas *c;
        if (c = template_findcanvas(template))
        {
            t_symbol *s1 = gensym("struct");
            for (y = c->gl_list; y; y = y->g_next)
            {
                t_object *ob = pd_checkobject(&y->g_pd);
                t_atom *argv;
                if (!ob || ob->te_type != T_OBJECT ||
                    binbuf_getnatom(ob->te_binbuf) < 2)
                    continue;
                argv = binbuf_getvec(ob->te_binbuf);
                if (argv[0].a_w.w_symbol != s1)
                    continue;
                if (canvas_makebindsym(argv[1].a_w.w_symbol) == template->t_sym)
                    return (ob);
            }
        }
    }
    return (0);
}

int template_hasxy(t_template *template)
{
    t_symbol *zz;
    int xonset, yonset, xtype, ytype, gotx, goty;
    if (!template)
    {
        error("struct: couldn't find template %s", template->t_sym->s_name);
        return 0;
    }
    gotx = template_find_field(template, gensym("x"), &xonset, &xtype, &zz);
    goty = template_find_field(template, gensym("y"), &yonset, &ytype, &zz);
    if ((gotx && (xtype == DT_FLOAT)) &&
        (goty && (ytype == DT_FLOAT)) &&
        (xonset == 0) && (yonset == sizeof(t_word)))
    {
        return 1;
    }
    else
        return 0;
}

int template_check_array_fields(t_symbol *structname, t_template *template)
{
    /* We're calling this from template_cancreate as well as
       gtemplate_cancreate. With gtemplate_cancreate, the t_template doesn't
       exist yet. So we send the struct name to see if it matches the name of
       any array field templates that our gtemplate will depend on. If we find
       a match we will refuse to create the gtemplate. However,
       template_cancreate starts from the struct name for a template that
       already exists.  On the first time through this recursive function,
       there isn't a containing structname yet. So we suppress this conditional
       the first time through the recursive loop, and then set the structname
       for all subsequent iterations through the loop.
       
       1 = success
       0 = circular dependency
       -1 = non-existant array elemtemplate found */
    if (structname && structname == template->t_sym)
    {
        t_object *ob = template_getstruct(template);
        pd_error(ob, "%s: circular dependency",
            template->t_sym->s_name);
       return (0);
    }
    int i, nitems = template->t_n;
    t_dataslot *datatypes = template->t_vec;
    t_template *elemtemplate;
    for (i = 0; i < nitems; i++, datatypes++)
    {
        if (datatypes->ds_type == DT_ARRAY)
        {
            elemtemplate = template_findbyname(datatypes->ds_fieldtemplate);
            if (!(elemtemplate))
            {
                t_object *ob = template_getstruct(template);
                pd_error(ob, "%s: no such template",
                    datatypes->ds_fieldtemplate->s_name);
                return (-1);
            }
            else if (elemtemplate->t_sym == structname)
            {
                t_object *ob = template_getstruct(template);
                pd_error(ob, "%s: circular dependency",
                    datatypes->ds_fieldtemplate->s_name);
                return (0);
            }
            else
            {
                if (!structname)
                {
                    structname = template->t_sym;
                }
                return (template_check_array_fields(structname, elemtemplate));
            }
        }
    }
    return 1;
}

int template_cancreate(t_template *template)
{
    /* we send "0" for the structname since there is no container struct */
    return (template_check_array_fields(0, template) == 1);
}

    /* get the first canvas field for a scalar */
t_canvas *scalar_getcanvasfield(t_scalar *x)
{
    t_template *template = template_findbyname(x->sc_template);
    if (template)
    {
        int i, nitems = template->t_n;
        t_dataslot *datatypes = template->t_vec;
        for (i = 0; i < nitems; i++, datatypes++)
        {
            if (datatypes->ds_type == DT_LIST)
                return x->sc_vec[i].w_list;
        }
    }
    return 0;
}

    /* make a new scalar and add to the glist.  We create a "gp" here which
    will be used for array items to point back here.  This gp doesn't do
    reference counting or "validation" updates though; the parent won't go away
    without the contained arrays going away too.  The "gp" is copied out
    by value in the word_init() routine so we can throw our copy away. */
t_scalar *scalar_new(t_glist *owner, t_symbol *templatesym)
{
    t_scalar *x;
    t_template *template;
    t_gpointer gp;
    gpointer_init(&gp);
    template = template_findbyname(templatesym);
    if (!template)
    {
        error("scalar: couldn't find template %s", templatesym->s_name);
        return (0);
    }
    if (!template_cancreate(template))
        return (0);
    x = (t_scalar *)getbytes(sizeof(t_scalar) +
        (template->t_n - 1) * sizeof(*x->sc_vec));
    x->sc_gobj.g_pd = scalar_class;
    x->sc_template = templatesym;
    gpointer_setglist(&gp, owner, &x->sc_gobj);
    word_init(x->sc_vec, template, &gp);
    return (x);
}

    /* Pd method to create a new scalar, add it to a glist, and initialize
    it from the message arguments. */

int canvas_readscalar(t_glist *x, int natoms, t_atom *vec,
    int *p_nextmsg, int selectit);

void glist_scalar(t_glist *glist,
    t_symbol *classname, t_int argc, t_atom *argv)
{
    t_symbol *templatesym =
        canvas_makebindsym(atom_getsymbolarg(0, argc, argv));
    t_binbuf *b;
    int natoms, nextmsg = 0;
    t_atom *vec;
    if (!template_findbyname(templatesym))
    {
        pd_error(glist, "%s: no such template",
            atom_getsymbolarg(0, argc, argv)->s_name);
        return;
    }

    b = binbuf_new();
    binbuf_restore(b, argc, argv);
    natoms = binbuf_getnatom(b);
    vec = binbuf_getvec(b);
    
    canvas_readscalar(glist, natoms, vec, &nextmsg, 0);
    binbuf_free(b);
}

    /* search template fields recursively to see if the template
       depends on elemtemplate */
int template_has_elemtemplate(t_template *t, t_template *elemtemplate)
{
    int returnval = 0;
    if (t && elemtemplate)
    {
        int i;
        t_dataslot *d = t->t_vec;
        for (i = 0; i < t->t_n; i++, d++)
        {
            if (d->ds_type == DT_ARRAY)
            {
                if (d->ds_fieldtemplate == elemtemplate->t_sym)
                {
                    returnval = 1;
                    break;
                }
                else
                {
                    returnval = template_has_elemtemplate(
                        template_findbyname(d->ds_fieldtemplate),
                        elemtemplate);
                }
            }
        }
    }
    return (returnval);
}

/* -------------------- widget behavior for scalar ------------ */
void scalar_getbasexy(t_scalar *x, t_float *basex, t_float *basey)
{
    t_template *template = template_findbyname(x->sc_template);
    *basex = template_getfloat(template, gensym("x"), x->sc_vec, 0);
    *basey = template_getfloat(template, gensym("y"), x->sc_vec, 0);
}

extern int array_joc;

extern void template_notifyforscalar(t_template *template, t_glist *owner,
    t_scalar *sc, t_symbol *s, int argc, t_atom *argv);

extern void scalar_getinnersvgrect(t_gobj *z, t_glist *owner, t_word *data,
    t_template *template, t_float basex, t_float basey,
    int *xp1, int *yp1, int *xp2, int *yp2);

extern t_symbol *group_gettype(t_glist *glist);
static void scalar_getgrouprect(t_glist *owner, t_glist *groupcanvas,
    t_word *data, t_template *template, int basex, int basey,
    int *x1, int *x2, int *y1, int *y2)
{
    t_gobj *y;
    for (y = groupcanvas->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == canvas_class &&
            ((t_canvas *)y)->gl_svg)
        {
            /* todo: accumulate basex and basey for correct offset */
            if (group_gettype((t_canvas *)y) == gensym("g"))
                scalar_getgrouprect(owner, (t_glist *)y, data, template,
                    basex, basey, x1, x2, y1, y2);
            else /* inner svg */
                scalar_getinnersvgrect(y, owner, data, template, basex, basey,
                    x1, y1, x2, y2);
        }
        else
        {
            t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
            int nx1, ny1, nx2, ny2;
            if (!wb) continue;
            (*wb->w_parentgetrectfn)(y, owner,
                data, template, basex, basey,
                &nx1, &ny1, &nx2, &ny2);
            if (nx1 < *x1) *x1 = nx1;
            if (ny1 < *y1) *y1 = ny1;
            if (nx2 > *x2) *x2 = nx2;
            if (ny2 > *y2) *y2 = ny2;
            //fprintf(stderr,"====scalar_getrect x1 %d y1 %d x2 %d y2 %d\n",
            //    x1, y1, x2, y2);
        }
    }
}
 
static void scalar_getrect(t_gobj *z, t_glist *owner,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    //fprintf(stderr,"scalar_getrect %d\n", array_joc);
    t_scalar *x = (t_scalar *)z;

    t_template *template = template_findbyname(x->sc_template);
    t_canvas *templatecanvas = template_findcanvas(template);
    int x1 = 0x7fffffff, x2 = -0x7fffffff, y1 = 0x7fffffff, y2 = -0x7fffffff;
    t_float basex, basey;
    t_float screenx1, screeny1, screenx2, screeny2;

    // EXPERIMENTAL: we assume that entire canvas is within
    // the rectangle--this is for arrays
    // with "jump on click" enabled TODO: test for other regressions
    // (there should not be any
    // provided the global variable array_joc is properly maintained)
    if (glist_istoplevel(owner) && array_joc)
    {
        x1 = -0x7fffffff, y1 = -0x7fffffff, x2 = 0x7fffffff, y2 = 0x7fffffff;
    }
    else
    {
        scalar_getbasexy(x, &basex, &basey);
            /* if someone deleted the template canvas, we're just a point */
        if (!templatecanvas)
        {
            //fprintf(stderr,"...point\n");
            x1 = x2 = glist_xtopixels(owner, basex);
            y1 = y2 = glist_ytopixels(owner, basey);
        }
        else
        {
            /* todo: bad flow with internal return here. make it cleaner */
            if (x->sc_bboxcache && 0)
            {
                screenx1 = glist_xtopixels(owner, x->sc_x1);
                screeny1 = glist_ytopixels(owner, x->sc_y1);
                screenx2 = glist_xtopixels(owner, x->sc_x2);
                screeny2 = glist_ytopixels(owner, x->sc_y2);

                *xp1 = (int)(screenx1 < screenx2 ? screenx1 : screenx2);
                *yp1 = (int)(screeny1 < screeny2 ? screeny1 : screeny2);
                *xp2 = (int)(screenx2 > screenx1 ? screenx2 : screenx1);
                *yp2 = (int)(screeny2 > screeny1 ? screeny2 : screeny1);

                //fprintf(stderr,"CACHED FINAL scalar_getrect "
                //               "x1 %g y1 %g x2 %g y2 %g\n",
                //    screenx1,
                //    screeny1,
                //    screenx2,
                //    screeny2);

                return;
            }
            x1 = y1 = 0x7fffffff;
            x2 = y2 = -0x7fffffff;
            scalar_getgrouprect(owner, templatecanvas, x->sc_vec, template,
                basex, basey, &x1, &x2, &y1, &y2);
            if (x2 < x1 || y2 < y1)
                x1 = y1 = x2 = y2 = 0;
        }
    }
    screenx1 = glist_xtopixels(owner, x1);
    screeny1 = glist_ytopixels(owner, y1);
    screenx2 = glist_xtopixels(owner, x2);
    screeny2 = glist_ytopixels(owner, y2);

    // Values for screen bounding box
    *xp1 = (int)(screenx1 < screenx2 ? screenx1 : screenx2);
    *xp2 = (int)(screenx2 > screenx1 ? screenx2 : screenx1);
    *yp1 = (int)(screeny1 < screeny2 ? screeny1 : screeny2);
    *yp2 = (int)(screeny2 > screeny1 ? screeny2 : screeny1);

    // Cache without glist_topixel (in case a gop is moved, for example)
    x->sc_x1 = x1;
    x->sc_x2 = x2;
    x->sc_y1 = y1;
    x->sc_y2 = y2;

    //fprintf(stderr,"COMPUTED FINAL scalar_getrect "
    //    "x1 %g y1 %g x2 %g y2 %g\n",
    //    screenx1,
    //    screeny1,
    //    screenx2,
    //    screeny2);

    x->sc_bboxcache = 1; // We now have cached values for the next call
}

void scalar_drawselectrect(t_scalar *x, t_glist *glist, int state)
{
    char tagbuf[MAXPDSTRING];
    sprintf(tagbuf, "scalar%lx", (long unsigned int)x->sc_vec);

    //fprintf(stderr,"scalar_drawselecterect%d\n", state);
    if (state)
    {
        int x1, y1, x2, y2;
        t_float basex, basey;
        scalar_getbasexy(x, &basex, &basey);

        scalar_getrect(&x->sc_gobj, glist, &x1, &y1, &x2, &y2);
        x1--; x2++; y1--; y2++;
        if (glist_istoplevel(glist))
        {
            t_float xorig = glist_xtopixels(glist, 0);
            t_float yorig = glist_ytopixels(glist, 0);
            t_float xscale = glist_xtopixels(glist, 1) - xorig;
            t_float yscale = glist_ytopixels(glist, 1) - yorig;
            // unscaled x/y coordinates
            t_float u1 = (x1 - xorig) / xscale;
            t_float v1 = (y1 - yorig) / yscale;
            t_float u2 = (x2 - xorig) / xscale;
            t_float v2 = (y2 - yorig) / yscale;
            // make sure that these are in the right order,
            // gui_scalar_draw_select_rect expects them that way
            if (u2 < u1) {
                t_float u = u2;
                u2 = u1; u1 = u;
            }
            if (v2 < v1) {
                t_float v = v2;
                v2 = v1; v1 = v;
            }
            gui_vmess("gui_scalar_draw_select_rect", "xsiffffff",
                glist_getcanvas(glist), tagbuf,
                state,
                u1, v1, u2, v2,
                basex,
                basey);
        }
    }
    else
    {
        if (glist_istoplevel(glist))
        {
            gui_vmess("gui_scalar_draw_select_rect", "xsiiiiiii",
                glist_getcanvas(glist), tagbuf,
                state,
                0, 0, 0, 0, 0, 0);
        }
    }
}

/* This is greatly simplified with Node-Webkit-- we just need to get the
   basex/basey for the scalar, in addition to the bbox of the scalar.

   This can be simplified further by using a single function on the GUI
   side, and sending it the "state" parameter.
*/
void scalar_select(t_gobj *z, t_glist *owner, int state)
{
    //fprintf(stderr,"scalar_select %d\n", state);
    t_scalar *x = (t_scalar *)z;

    char tagbuf[MAXPDSTRING];
    sprintf(tagbuf, "scalar%lx", (long unsigned int)x->sc_vec);

    t_template *tmpl;
    t_symbol *templatesym = x->sc_template;
    t_atom at;
    //t_canvas *templatecanvas = NULL;
    t_gpointer gp;
    gpointer_init(&gp);
    gpointer_setglist(&gp, owner, &x->sc_gobj);
    SETPOINTER(&at, &gp);
    if (tmpl = template_findbyname(templatesym))
    {
        template_notify(tmpl, (state ? gensym("select") : gensym("deselect")),
            1, &at);
        //templatecanvas = template_findcanvas(tmpl);
    }
    gpointer_unset(&gp);
    if (state)
    {
        x->sc_selected = owner;
        if (glist_isvisible(owner))
            gui_vmess("gui_gobj_select", "xs",
                glist_getcanvas(owner), tagbuf);
    }
    else
    {
        x->sc_selected = 0;
        if (glist_isvisible(owner))
            gui_vmess("gui_gobj_deselect", "xs",
                glist_getcanvas(owner), tagbuf);
    }
    //sys_vgui("pdtk_select_all_gop_widgets .x%lx %lx %d\n",
    //    glist_getcanvas(owner), owner, state);
    scalar_drawselectrect(x, owner, state);
}

static void scalar_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    //fprintf(stderr,"scalar_displace\n");
    t_scalar *x = (t_scalar *)z;
    t_symbol *templatesym = x->sc_template;
    t_template *template = template_findbyname(templatesym);
    t_symbol *zz;
    t_atom at[3];
    t_gpointer gp;
    int xonset, yonset, xtype, ytype, gotx, goty;
    if (!template)
    {
        error("scalar: couldn't find template %s", templatesym->s_name);
        return;
    }
    gotx = template_find_field(template, gensym("x"), &xonset, &xtype, &zz);
    if ((gotx && (xtype != DT_FLOAT)) || x->sc_selected != glist)
        gotx = 0;
    goty = template_find_field(template, gensym("y"), &yonset, &ytype, &zz);
    if ((goty && (ytype != DT_FLOAT)) || x->sc_selected != glist)
        goty = 0;
    if (gotx)
    {
        *(t_float *)(((char *)(x->sc_vec)) + xonset) +=
            dx * (glist_pixelstox(glist, 1) - glist_pixelstox(glist, 0));
        x->sc_x1 += dx;
        x->sc_x2 += dx;
    }
    if (goty)
    {
        *(t_float *)(((char *)(x->sc_vec)) + yonset) +=
            dy * (glist_pixelstoy(glist, 1) - glist_pixelstoy(glist, 0));
        x->sc_y1 += dy;
        x->sc_y2 += dy;
    }
    gpointer_init(&gp);
    gpointer_setglist(&gp, glist, &x->sc_gobj);
    SETPOINTER(&at[0], &gp);
    SETFLOAT(&at[1], (t_float)dx);
    SETFLOAT(&at[2], (t_float)dy);
    template_notify(template, gensym("displace"), 2, at);
    scalar_redraw(x, glist);
}

/* Kind of complicated at the moment. If a scalar is in a gop canvas, then
   we don't need to update its x/y fields (if it even has them) when displacing
   it.  Otherwise we do.  The member sc_selected is used to store the canvas
   where the selection exists-- if it matches the glist parameter below then we
   know the scalar is directly selected.  If not it's in a gop canvas.
*/
static void scalar_displace_withtag(t_gobj *z, t_glist *glist, int dx, int dy)
{
    //fprintf(stderr,"scalar_displace_withtag %lx %d %d\n", (t_int)z, dx, dy);
    t_scalar *x = (t_scalar *)z;
    t_symbol *templatesym = x->sc_template;
    t_template *template = template_findbyname(templatesym);
    t_symbol *zz;
    t_atom at[3];
    t_gpointer gp;
    int xonset, yonset, xtype, ytype, gotx, goty;
    t_float basex = 0, basey = 0;
    if (!template)
    {
        error("scalar: couldn't find template %s", templatesym->s_name);
        return;
    }
    gotx = template_find_field(template, gensym("x"), &xonset, &xtype, &zz);
    if ((gotx && (xtype != DT_FLOAT)) || x->sc_selected != glist)
        gotx = 0;
    goty = template_find_field(template, gensym("y"), &yonset, &ytype, &zz);
    if ((goty && (ytype != DT_FLOAT)) || x->sc_selected != glist)
        goty = 0;
    if (gotx)
    {
        *(t_float *)(((char *)(x->sc_vec)) + xonset) +=
            dx * (glist_pixelstox(glist, 1) - glist_pixelstox(glist, 0));
        x->sc_x1 += dx;
        x->sc_x2 += dx;
    }
    if (goty)
    {
        *(t_float *)(((char *)(x->sc_vec)) + yonset) +=
            dy * (glist_pixelstoy(glist, 1) - glist_pixelstoy(glist, 0));
        x->sc_y1 += dy;
        x->sc_y2 += dy;
    }
    //fprintf(stderr,"gotx=%d goty=%d\n", gotx, goty);
    scalar_getbasexy(x, &basex, &basey);
    gpointer_init(&gp);
    gpointer_setglist(&gp, glist, &x->sc_gobj);
    SETPOINTER(&at[0], &gp);
    SETFLOAT(&at[1], (t_float)dx);
    SETFLOAT(&at[2], (t_float)dy);
    template_notify(template, gensym("displace"), 2, at);

    canvas_getscroll(glist);
     
    /* Apparently this is no longer needed, so it is commented out.  But if
       we merge garrays back into this code we may need it... */
    /*
    if (template->t_sym != gensym("_float_array"))
    {
        t_gobj *y;
        t_canvas *templatecanvas = template_findcanvas(template);
        for (y = templatecanvas->gl_list; y; y = y->g_next)
            {
                t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
                if (!wb) continue;
                (*wb->w_parentdisplacefn)(y, glist, x->sc_vec, template,
                    basex, basey, dx, dy);
            }
    }
    */

    //scalar_redraw(x, glist);
}

static void scalar_activate(t_gobj *z, t_glist *owner, int state)
{
    /* post("scalar_activate %d", state); */
    /* later */
}

static void scalar_delete(t_gobj *z, t_glist *glist)
{
    /* nothing to do */
}

extern void svg_grouptogui(t_glist *g, t_template *template, t_word *data);

extern void svg_parentwidgettogui(t_gobj *z, t_scalar *sc, t_glist *owner,
    t_word *data, t_template *template);

extern void svg_register_events(t_gobj *z, t_canvas *c, t_scalar *sc,
    t_template *template, t_word *data, t_array *parentarray);

static void scalar_group_configure(t_scalar *x, t_glist *owner,
    t_template *template, t_word *data, t_glist *gl, t_glist *parent,
    t_array *parentarray)
{
    t_gobj *y;
    char tagbuf[MAXPDSTRING];
    sprintf(tagbuf, "dgroup%lx.%lx", (long unsigned int)gl,
        (long unsigned int)data);
    char parentbuf[MAXPDSTRING];
    sprintf(parentbuf, "dgroup%lx.%lx",
        (long unsigned int)parent,
        (long unsigned int)data);
    gui_start_vmess("gui_draw_configure_all", "xs",
        glist_getcanvas(owner), tagbuf);
    svg_grouptogui(gl, template, data);
    gui_end_vmess();
    svg_register_events((t_gobj *)gl, owner, x, template, data, parentarray);
    for (y = gl->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == canvas_class &&
            ((t_glist *)y)->gl_svg)
        {
            scalar_group_configure(x, owner, template, data, (t_glist *)y, gl,
                0);
        }
        t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
        if (!wb) continue;
        //(*wb->w_parentvisfn)(y, owner, gl, x, data, template,
        //   0, 0, 0, vis);
        svg_parentwidgettogui(y, x, owner, data, template);
        svg_register_events(y, owner, x, template, data, parentarray);
    }
}

void scalar_doconfigure(t_gobj *xgobj, t_glist *owner)
{
    t_scalar *x = (t_scalar *)xgobj;
    int vis = glist_isvisible(owner);
    if (vis)
    {
        //fprintf(stderr,"scalar_vis %d %lx\n", vis, (t_int)z);
        x->sc_bboxcache = 0;

        t_template *template = template_findbyname(x->sc_template);
        t_canvas *templatecanvas = template_findcanvas(template);
        t_gobj *y;
        t_float basex, basey;
        scalar_getbasexy(x, &basex, &basey);
            /* if we don't know how to draw it, make a small rectangle */

        t_float xscale = glist_xtopixels(owner, 1) - glist_xtopixels(owner, 0);
        t_float yscale = glist_ytopixels(owner, 1) - glist_ytopixels(owner, 0);

        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "scalar%lx", (long unsigned int)x->sc_vec);
        gui_vmess("gui_scalar_configure_gobj", "xsiffffii",
            glist_getcanvas(owner), 
            tagbuf,
            glist_isselected(owner, &x->sc_gobj),
            xscale, 0.0, 0.0, yscale,
            (int)glist_xtopixels(owner, basex),
            (int)glist_ytopixels(owner, basey));

        for (y = templatecanvas->gl_list; y; y = y->g_next)
        {
            t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
            if (!wb)
            {
                /* check subpatches for more drawing commands.
                   (Optimized to only search [group] subpatches) */
                if (pd_class(&y->g_pd) == canvas_class &&
                    ((t_glist *)y)->gl_svg)
                {
                    scalar_group_configure(x, owner, template, x->sc_vec,
                        (t_glist *)y, templatecanvas, 0);
                }
                continue;
            }
            //(*wb->w_parentvisfn)(y, owner, 0, x, x->sc_vec, template,
            //    basex, basey, 0, vis);
            svg_parentwidgettogui(y, x, owner, x->sc_vec, template);
            svg_register_events(y, owner, x, template, x->sc_vec, 0);
        }
        if (glist_isselected(owner, &x->sc_gobj))
        {
            // we removed this because it caused infinite recursion
            // in the scalar-help.pd example
            //scalar_select(z, owner, 1);
            scalar_drawselectrect(x, owner, 0);
            scalar_drawselectrect(x, owner, 1);
        }
    }
}

void scalar_configure(t_scalar *x, t_glist *owner)
{
    sys_queuegui(x, owner, scalar_doconfigure);
}

extern int is_plot_class(t_gobj *y);
void array_configure(t_scalar *x, t_glist *owner, t_array *a, t_word *data)
{
    t_template *template = template_findbyname(x->sc_template);
    t_template *elemtemplate = template_findbyname(a->a_templatesym);
    t_canvas *templatecanvas = template_findcanvas(template);
    t_canvas *elemtemplatecanvas = template_findcanvas(elemtemplate);
    t_gobj *y;

    for (y = templatecanvas->gl_list; y; y = y->g_next)
    {
        t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
        if (wb && is_plot_class(y))
        {
            scalar_redraw(x, owner);
            return;
        }
    }
        /* If no plot widgets, it is now safe to just configure the individual
           array elements. */
    for (y = elemtemplatecanvas->gl_list; y; y = y->g_next)
    {
        t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
        if (!wb)
        {
            /* check subpatches for more drawing commands.
               (Optimized to only search [group] subpatches) */
            if (pd_class(&y->g_pd) == canvas_class &&
                ((t_glist *)y)->gl_svg)
            {
                scalar_group_configure(x, owner, template, data,
                    (t_glist *)y, elemtemplatecanvas, a);
            }
            continue;
        }
        svg_parentwidgettogui(y, x, owner, data, elemtemplate);
        svg_register_events(y, owner, x, elemtemplate, data, a);
    }

}

static void scalar_groupvis(t_scalar *x, t_glist *owner, t_template *template,
    t_glist *gl, t_glist *parent, int vis)
{
    t_gobj *y;
    if (vis)
    {
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "dgroup%lx.%lx", (long unsigned int)gl,
            (long unsigned int)x->sc_vec);
        char parentbuf[MAXPDSTRING];
        sprintf(parentbuf, "dgroup%lx.%lx", (long unsigned int)parent,
            (long unsigned int)x->sc_vec);
        gui_start_vmess("gui_scalar_draw_group", "xsss",
            glist_getcanvas(owner), tagbuf, parentbuf,
            group_gettype(gl)->s_name);
        svg_grouptogui(gl, template, x->sc_vec);
        gui_end_vmess();

        /* register events */
        svg_register_events((t_gobj *)gl, owner, x, template, x->sc_vec, 0);
    }
    for (y = gl->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == canvas_class &&
            ((t_glist *)y)->gl_svg)
        {
            scalar_groupvis(x, owner, template, (t_glist *)y, gl, vis);
        }
        t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
        if (!wb) continue;
        (*wb->w_parentvisfn)(y, owner, gl, x, x->sc_vec, template,
            0, 0, 0, vis);
    }
}

/* At present, scalars have a three-level hierarchy in the gui,
   with two levels accessible by the user from within Pd:
   scalar - ".scalar%lx", x->sc_vec
     |      <g> with matrix derived from x/y fields,
     |      gop basexy, and gop scaling values. This group is
     |      not configurable by the user. This means that the
     |      a [draw g] below can ignore basexy and gop junk
     |      when computing the transform matrix.
     v
   dgroup - ".dgroup%lx.%lx", templatecanvas, x->sc_vec
     |      group used as parent for all the toplevel drawing
     |      commands of the scalar (i.e., the ones located on
     |      the same canvas as the [struct]).  Its matrix and
     |      options aren't accessible by the user.
     v
   (draw  - ".draw%lx.%lx", (t_draw *ptr), x->sc_vec
     |      the actual drawing command: rectangle, path, g, etc. 
     or     Each has its own matrix and options which can set
   dgroup   with messages to the corresponding [draw] object.
     |      Also, ds arrays have an additional group for the sake of
     |      convenience.
     |      Anything with "dgroup" is either [draw g] or [draw svg]
     v
    etc.

   The tag "blankscalar" is for scalars that don't have a visual
   representation, but maybe this can just be merged with "scalar"
*/
static void scalar_vis(t_gobj *z, t_glist *owner, int vis)
{
    //fprintf(stderr,"scalar_vis %d %lx\n", vis, (t_int)z);
    t_scalar *x = (t_scalar *)z;
    char buf[50];
    sprintf(buf, "x%lx", (long unsigned int)x);

    x->sc_bboxcache = 0;

    t_template *template = template_findbyname(x->sc_template);
    t_canvas *templatecanvas = template_findcanvas(template);
    t_gobj *y;
    t_float basex, basey;
    scalar_getbasexy(x, &basex, &basey);
        /* if we don't know how to draw it, make a small rectangle */
    if (!templatecanvas)
    {
        if (vis)
        {
            //int x1 = glist_xtopixels(owner, basex);
            //int y1 = glist_ytopixels(owner, basey);
            /* Let's just not create anything to visualize scalars that
               don't have a template. Pd Vanilla draws a single pixel to 
               represent them, so later we might want to do a simple
               shape for them... */
            //sys_vgui(".x%lx.c create prect %d %d %d %d "
            //         "-tags {blankscalar%lx %s}\n",
            //    glist_getcanvas(owner), x1-1, y1-1, x1+1, y1+1, x,
            //    (glist_isselected(owner, &x->sc_gobj) ?
            //        "scalar_selected" : ""));
        }
        else
        {
            /* No need to delete if we don't draw anything... */
            //sys_vgui(".x%lx.c delete blankscalar%lx\n",
            //    glist_getcanvas(owner), x);
        }
        return;
    }
    //else sys_vgui(".x%lx.c delete blankscalar%lx\n",
    //    glist_getcanvas(owner), x);

    if (vis)
    {
        t_float xscale = glist_xtopixels(owner, 1) - glist_xtopixels(owner, 0);
        t_float yscale = glist_ytopixels(owner, 1) - glist_ytopixels(owner, 0);
        /* we translate the .scalar%lx group to displace it on the tk side.
           This is the outermost group for the scalar, something like a
           poor man's viewport.
           Also:
             * the default stroke is supposed to be "none"
             * default fill is supposed to be black.
             * stroke-linejoin should be "miter", not "round"  
           To fix these, we set the correct fill/stroke/strokelinjoin options
           here on the .scalar%lx group. (Notice also that tkpath doesn't
           understand "None"-- instead we must send an empty symbol.) */
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "scalar%lx", (long unsigned int)x->sc_vec);
        gui_vmess("gui_scalar_new", "xsiffffiii",
            glist_getcanvas(owner), 
            tagbuf,
            glist_isselected(owner, &x->sc_gobj),
            xscale, 0.0, 0.0, yscale,
            (int)glist_xtopixels(owner, basex),
            (int)glist_ytopixels(owner, basey),
            glist_istoplevel(owner));
        char groupbuf[MAXPDSTRING];
        // Quick hack to make gui_scalar_draw_group more general (so we
        // don't have to tack on "gobj" manually)
        sprintf(tagbuf, "scalar%lxgobj", (long unsigned int)x->sc_vec);
        sprintf(groupbuf, "dgroup%lx.%lx", (long unsigned int)templatecanvas,
            (long unsigned int)x->sc_vec);
        gui_vmess("gui_scalar_draw_group", "xsss",
            glist_getcanvas(owner), groupbuf, tagbuf, "g");
        pd_bind(&x->sc_gobj.g_pd, gensym(buf));
    }

    /* warning: don't need--- have recursive func. */
    for (y = templatecanvas->gl_list; y; y = y->g_next)
    {
        t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
        if (!wb)
        {
            /* check subpatches for more drawing commands.
               (Optimized to only search [group] subpatches) */
            if (pd_class(&y->g_pd) == canvas_class &&
                ((t_glist *)y)->gl_svg)
            {
                scalar_groupvis(x, owner, template,
                    (t_glist *)y, templatecanvas, vis);
            }
            continue;
        }
        (*wb->w_parentvisfn)(y, owner, 0, x, x->sc_vec, template,
            basex, basey, 0, vis);
    }
    if (!vis)
    {
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "scalar%lx", (long unsigned int)x->sc_vec);
        gui_vmess("gui_scalar_erase", "xs",
            glist_getcanvas(owner), tagbuf);
        if (gensym(buf)->s_thing)
            pd_unbind(&x->sc_gobj.g_pd, gensym(buf));
    }

    sys_unqueuegui(x);
    if (vis && glist_isselected(owner, &x->sc_gobj))
    {
        // we removed this because it caused infinite recursion
        // in the scalar-help.pd example
        //scalar_select(z, owner, 1);
        scalar_drawselectrect(x, owner, 0);
        scalar_drawselectrect(x, owner, 1);
    }
}

static void scalar_doredraw(t_gobj *client, t_glist *glist)
{
    if (glist_isvisible(glist))
    {
        scalar_vis(client, glist, 0);
        scalar_vis(client, glist, 1);
        if (glist_isselected(glist_getcanvas(glist), (t_gobj *)glist))
        {
            //fprintf(stderr,"yes\n");
            /* I still don't understand what this does... should probably
               do some scalar gop tests to see if it is actually needed... */
            //sys_vgui("pdtk_select_all_gop_widgets .x%lx %lx %d\n",
            //    glist_getcanvas(glist), glist, 1);
        }
        canvas_getscroll(glist_getcanvas(glist));
    }
}

void scalar_redraw(t_scalar *x, t_glist *glist)
{
    if (glist_isvisible(glist))
        scalar_doredraw((t_gobj *)x, glist);
        //sys_queuegui(x, glist, scalar_doredraw);
}

/* here we call the parentclickfns for drawing commands.
   We recurse all the way to the end of the glist (and, for the
   groups, as deep as they go) and then call the functions from the bottom up.
   This way the clicks can "bubble" up from the bottom.  This has the effect
   that the shape visually closest to the front gets called first.

   This is called "event bubbling" in the HTML5 DOM.  You can also call
   events in top down order in HTML5/SVG as well (called "capturing" or
   "trickling"). But because some old version of Internet Explorer only did
   bubbling, that is the most widely used model today, and the only one
   I decided to implement here.

   However, [group] doesn't yet have an outlet to report events. Only a
   single [draw] should report a click here atm.
*/
int scalar_groupclick(struct _glist *groupcanvas,
    t_word *data, t_template *template, t_scalar *sc,
    t_array *ap, struct _glist *owner,
    t_float xloc, t_float yloc, int xpix, int ypix,
    int shift, int alt, int dbl, int doit, t_float basex, t_float basey,
    t_gobj *obj)
{
    int hit = 0;
    t_gobj *nextobj = obj->g_next;
    /* let's skip over any objects that aren't drawing instructions
       or groups */
    while (nextobj &&
           !class_isdrawcommand(pd_class((t_pd *)nextobj)) &&
           !(pd_class(&nextobj->g_pd) == canvas_class &&
               ((t_glist *)nextobj)->gl_svg))
        nextobj = nextobj->g_next;
    /* If there's another link in the list go ahead and recurse with it */
    if (nextobj)
    {
        hit = (scalar_groupclick(groupcanvas, data, template, sc, ap,
            owner, xloc, yloc, xpix, ypix,
            shift, alt, dbl, doit, basex, basey, nextobj));
        if (hit) return hit;
    }
    /* recurse inside a [group] object to look for more objects */
    if (pd_class(&obj->g_pd) == canvas_class && ((t_glist *)obj)->gl_svg)
    {
            t_canvas *cnv = (t_canvas *)obj;
            obj = cnv->gl_list;
            if (obj)
                return (scalar_groupclick(cnv, data, template, sc, ap,
                owner, xloc, yloc, xpix, ypix,
                shift, alt, dbl, doit, basex, basey, obj));
    }
    else /* finally, try to call the parent click function ... */
    {
        t_parentwidgetbehavior *wb = pd_getparentwidget(&obj->g_pd);
        if (!wb) return hit;
        if ((*wb->w_parentclickfn)(obj, owner,
            data, template, sc, ap, basex + xloc, basey + yloc,
            xpix, ypix, shift, alt, dbl, doit))
                hit = 1;
    }
    return (hit);
}

/*
int scalar_groupclick(struct _glist *groupcanvas,
    t_word *data, t_template *template, t_scalar *sc,
    t_array *ap, struct _glist *owner,
    t_float xloc, t_float yloc, int xpix, int ypix,
    int shift, int alt, int dbl, int doit, t_float basex, t_float basey)
{
    int hit = 0;
    t_gobj *y;
    for (y = groupcanvas->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == canvas_class &&
            ((t_glist *)y)->gl_svg)
        {
            if (hit = scalar_groupclick((t_glist *)y, data, template, sc, ap,
                owner, xloc, yloc, xpix, ypix,
                shift, alt, dbl, doit, basex, basey))
            {
                return (hit);
            }
        }
        t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
        if (!wb) continue;
        if (hit = (*wb->w_parentclickfn)(y, owner,
            data, template, sc, ap, basex + xloc, basey + yloc,
            xpix, ypix, shift, alt, dbl, doit))
        {
            return (hit);
        }
    }
    return 0;
}
*/

int scalar_doclick(t_word *data, t_template *template, t_scalar *sc,
    t_array *ap, struct _glist *owner,
    t_float xloc, t_float yloc, int xpix, int ypix,
    int shift, int alt, int dbl, int doit)
{
    int hit = 0;
    t_canvas *templatecanvas = template_findcanvas(template);
    t_atom at[2];
    t_gobj *obj;
    t_float basex = template_getfloat(template, gensym("x"), data, 0);
    t_float basey = template_getfloat(template, gensym("y"), data, 0);
    //fprintf(stderr,"=================scalar_doclick %f %f %f %f %d\n",
    //    basex, basey, xloc, yloc, doit);

    SETFLOAT(at, basex + xloc);
    SETFLOAT(at+1, basey + yloc);
    if (doit)
    {
        //fprintf(stderr,"    doit\n");
        template_notifyforscalar(template, owner, 
            sc, gensym("click"), 2, at);
    }

    // if we are nested ignore xloc and yloc, otherwise
    // nested objects get their hitbox miscalculated
    if (xloc != 0.0 || yloc != 0.0)
    {
        //fprintf(stderr,"ignoring\n");
        //basex = 0.0;
        //basey = 0.0;
    }

    if (templatecanvas)
    {
        if (!(obj = templatecanvas->gl_list)) return 0;
        hit = scalar_groupclick(templatecanvas, data, template, sc, ap,
                    owner, xloc, yloc, xpix, ypix,
                    shift, alt, dbl, doit, basex, basey, obj);
    }
    return hit;
}

/* Unfortunately, nested gops don't yet handle scalar clicks correctly. The
   nested scalar seems not to receive the click.  However, the enter/leave
   messages happen just fine since most of their logic is in tcl/tk. */
static int scalar_click(t_gobj *z, struct _glist *owner,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    //fprintf(stderr,"scalar_click %d %d\n", xpix, ypix);
    t_scalar *x = (t_scalar *)z;

    x->sc_bboxcache = 0;

    t_template *template = template_findbyname(x->sc_template);

    return (scalar_doclick(x->sc_vec, template, x, 0,
        owner, 0, 0, xpix, ypix, shift, alt, dbl, doit));
}

void canvas_writescalar(t_symbol *templatesym, t_word *w, t_binbuf *b,
    int amarrayelement);

static void scalar_save(t_gobj *z, t_binbuf *b)
{
    t_scalar *x = (t_scalar *)z;
    t_binbuf *b2 = binbuf_new();
    canvas_writescalar(x->sc_template, x->sc_vec, b2, 0);
    binbuf_addv(b, "ss", &s__X, gensym("scalar"));
    binbuf_addbinbuf(b, b2);
    binbuf_addsemi(b);
    binbuf_free(b2);
}

static void scalar_menuopen(t_scalar *x)
{
    t_canvas *c = scalar_getcanvasfield(x);
    canvas_vis(c, 1);
}

static void scalar_properties(t_gobj *z, struct _glist *owner)
{
    t_scalar *x = (t_scalar *)z;
    char *buf, *gfx_tag;
    int bufsize;
    t_binbuf *b;
    glist_noselect(owner);
    glist_select(owner, z);
    b = glist_writetobinbuf(owner, 0);
    binbuf_gettext(b, &buf, &bufsize);
    binbuf_free(b);
    buf = t_resizebytes(buf, bufsize, bufsize+1);
    buf[bufsize] = 0;
    gfx_tag = gfxstub_new2((t_pd *)owner, x);
    gui_vmess("gui_data_dialog", "ss", gfx_tag, buf);
    t_freebytes(buf, bufsize+1);
}

static t_widgetbehavior scalar_widgetbehavior =
{
    scalar_getrect,
    scalar_displace,
    scalar_select,
    scalar_activate,
    scalar_delete,
    scalar_vis,
    scalar_click,
    scalar_displace_withtag,
};

static void scalar_free(t_scalar *x)
{
    t_symbol *templatesym = x->sc_template;
    t_template *template = template_findbyname(templatesym);
    if (!template)
    {
        error("scalar: couldn't find template %s", templatesym->s_name);
        return;
    }
    word_free(x->sc_vec, template);
    gfxstub_deleteforkey(x);
        /* the "size" field in the class is zero, so Pd doesn't try to free
        us automatically (see pd_free()) */
    freebytes(x, sizeof(t_scalar) + (template->t_n - 1) * sizeof(*x->sc_vec));
}

/* ----------------- setup function ------------------- */

void g_scalar_setup(void)
{
    scalar_class = class_new(gensym("scalar"), 0, (t_method)scalar_free, 0,
        CLASS_GOBJ, 0);
    class_addmethod(scalar_class, (t_method)scalar_menuopen,
        gensym("menu-open"), 0);
    class_setwidget(scalar_class, &scalar_widgetbehavior);
    class_setsavefn(scalar_class, scalar_save);
    class_setpropertiesfn(scalar_class, scalar_properties);
}
