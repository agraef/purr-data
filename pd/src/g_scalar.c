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

void word_init(t_word *wp, t_template *template, t_gpointer *gp)
{
    int i, nitems = template->t_n;
    t_dataslot *datatypes = template->t_vec;
    for (i = 0; i < nitems; i++, datatypes++, wp++)
    {
        int type = datatypes->ds_type;
        if (type == DT_FLOAT)
            wp->w_float = 0; 
        else if (type == DT_SYMBOL)
            wp->w_symbol = &s_symbol;
        else if (type == DT_ARRAY)
        {
            wp->w_array = array_new(datatypes->ds_arraytemplate, gp);
        }
        else if (type == DT_LIST)
        {
                /* LATER test this and get it to work */
            wp->w_list = canvas_new(0, 0, 0, 0);
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
       gtemplate_cancreate.
       With gtemplate_cancreate, the t_template doesn't exist yet.
       So we send the struct name to see if it matches the name of any
       array field templates that our gtemplate will depend on. If we find
       a match we will refuse to create the gtemplate.
       However, template_cancreate starts from the struct name for a template
       that already exists.  So on the first time through this recursive
       function, there isn't a containing structname yet.
       So we suppress this conditional the first
       time through the recursive loop, and then set the structname for
       all subsequent iterations through the loop. */
    if (structname && structname == template->t_sym)
    {
        return 0;
    }
    int i, nitems = template->t_n;
    t_dataslot *datatypes = template->t_vec;
    t_template *elemtemplate;
    for (i = 0; i < nitems; i++, datatypes++)
    {
        if (datatypes->ds_type == DT_ARRAY)
        {
            elemtemplate = template_findbyname(datatypes->ds_arraytemplate);
            if (!(elemtemplate))
            {
                t_object *ob = template_getstruct(template);
                pd_error(ob, "%s: no such template",
                    datatypes->ds_arraytemplate->s_name);
                return (0);
            }
            else if (elemtemplate->t_sym == structname)
            {
                t_object *ob = template_getstruct(template);
                pd_error(ob, "%s: circular dependency",
                    datatypes->ds_arraytemplate->s_name);
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
    return (1);
}

int template_cancreate(t_template *template)
{
    /* we send "0" for the structname since there is no container struct */
    return (template_check_array_fields(0, template));
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
    gpointer_setglist(&gp, owner, x);
    word_init(x->sc_vec, template, &gp);
    char buf[50];
    sprintf(buf, ".x%lx", (long unsigned int)x);
    pd_bind(&x->sc_gobj.g_pd, gensym(buf));
    return (x);
}

    /* Pd method to create a new scalar, add it to a glist, and initialize
    it from the message arguments. */

int glist_readscalar(t_glist *x, int natoms, t_atom *vec,
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
    
    glist_readscalar(glist, natoms, vec, &nextmsg, 0);
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
                if (d->ds_arraytemplate == elemtemplate->t_sym)
                {
                    returnval = 1;
                    break;
                }
                else
                {
                    returnval = template_has_elemtemplate(
                        template_findbyname(d->ds_arraytemplate),
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

t_canvas *sc_mouseover_canvas;
static void scalar_mouseover(t_scalar *x, t_floatarg state)
{
    t_atom at[1];
    t_template *template = template_findbyname(x->sc_template);
    if (state)
        template_notifyforscalar(template, sc_mouseover_canvas,
            x, gensym("leave"), 1, at);
    else
        template_notifyforscalar(template, sc_mouseover_canvas,
            x, gensym("enter"), 1, at);
}

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
            scalar_getgrouprect(owner, (t_glist *)y, data, template,
                basex, basey, x1, x2, y1, y2);
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
            if (x->sc_bboxcache)
            {
                *xp1 = x->sc_x1;
                *yp1 = x->sc_y1;
                *xp2 = x->sc_x2;
                *yp2 = x->sc_y2;
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
    //fprintf(stderr,"FINAL scalar_getrect x1 %d y1 %d x2 %d y2 %d\n",
    //    x1, y1, x2, y2);
    *xp1 = x->sc_x1 = x1;
    *yp1 = x->sc_y1 = y1;
    *xp2 = x->sc_x2 = x2;
    *yp2 = x->sc_y2 = y2; 
    x->sc_bboxcache = 1;
}

void scalar_drawselectrect(t_scalar *x, t_glist *glist, int state)
{
    //fprintf(stderr,"scalar_drawselecterect%d\n", state);
    if (state)
    {
        int x1, y1, x2, y2;
       
        scalar_getrect(&x->sc_gobj, glist, &x1, &y1, &x2, &y2);
        x1--; x2++; y1--; y2++;
        if (glist_istoplevel(glist))
            sys_vgui(".x%lx.c create prect %d %d %d %d "
                     "-strokewidth 1 -stroke $pd_colors(selection) "
                     "-tags {select%lx selected}\n",
                    glist_getcanvas(glist), x1, y1, x2, y2,
                    x);
    }
    else
    {
        if (glist_istoplevel(glist))
            sys_vgui(".x%lx.c delete select%lx\n", glist_getcanvas(glist), x);
    }
}

/* This is a workaround.  Since scalars are contained within a tkpath
   group, and since tkpath groups don't have coords, we can't just use
   the same "selected" tag that is used to move all other Pd objects. That
   would move scalars in their local coordinate system, which is wrong
   for transformed objects. For example, if a rectangle is rotated 45 and
   we try to do a [canvas move 10 0] command on it, it would get moved to
   the northeast instead of to the right!

   Instead, we tag selected scalars with the "scalar_selected" tag. Then in
   the GUI we use that tag to loop through and change each scalar's group
   matrix, and add (dx,dy) to its current translation values. The scalar
   group matrix .scalar%lx isn't accessible by the user, so it will only
   ever contain these translation values.
*/
void scalar_select(t_gobj *z, t_glist *owner, int state)
{
    //fprintf(stderr,"scalar_select %d\n", state);
    t_scalar *x = (t_scalar *)z;
    t_template *tmpl;
    t_symbol *templatesym = x->sc_template;
    t_atom at;
    //t_canvas *templatecanvas = NULL;
    t_gpointer gp;
    gpointer_init(&gp);
    gpointer_setglist(&gp, owner, x);
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
        sys_vgui(".x%lx.c addtag selected withtag blankscalar%lx\n",
            glist_getcanvas(owner), x);
        sys_vgui(".x%lx.c addtag scalar_selected withtag {.scalar%lx}\n",
            glist_getcanvas(owner), x->sc_vec);
    }
    else
    {
        x->sc_selected = 0;
        sys_vgui(".x%lx.c dtag blankscalar%lx selected\n",
            glist_getcanvas(owner), x);
        sys_vgui(".x%lx.c dtag .scalar%lx scalar_selected\n",
            glist_getcanvas(owner), x->sc_vec);
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
    gpointer_setglist(&gp, glist, x);
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
    scalar_getbasexy(x, &basex, &basey);
    gpointer_init(&gp);
    gpointer_setglist(&gp, glist, x);
    SETPOINTER(&at[0], &gp);
    SETFLOAT(&at[1], (t_float)dx);
    SETFLOAT(&at[2], (t_float)dy);
    template_notify(template, gensym("displace"), 2, at);

    sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", glist);
     
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

static void scalar_groupvis(t_scalar *x, t_glist *owner, t_template *template,
    t_glist *gl, t_glist *parent, int vis)
{
    t_gobj *y;
    if (vis)
    {
        sys_vgui(".x%lx.c create group -tags {.dgroup%lx.%lx} "
                 "-parent {.dgroup%lx.%lx}\\\n",
            glist_getcanvas(owner), gl, x->sc_vec,
            parent, x->sc_vec);
        svg_grouptogui(gl, template, x->sc_vec);
        sys_gui("\n");

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
            0, 0, vis);
    }
}

/* At present, scalars have a three-level hierarchy in tkpath,
   with two levels accessible by the user from within Pd:
   scalar - ".scalar%lx", x->sc_vec
     |      tkpath group with matrix derived from x/y fields,
     |      gop basexy, and gop scaling values. This group is
     |      not configurable by the user. This means that the
     |      [draw group] below can ignore basexy and gop junk
     |      when computing the transform matrix.
     v
   group  - ".dgroup%lx.%lx", templatecanvas, x->sc_vec
     |      group used as parent for all the toplevel drawing
     |      commands of the scalar (i.e., the ones located on
     |      the same canvas as the [struct]).  Its matrix and
     |      options aren't yet accessible by the user.
     v
   (draw  - ".draw%lx.%lx", (t_draw *ptr), x->sc_vec
     |      the actual drawing command: rectangle, path, etc.
     or     Each has its own matrix and options which can set
   scelem   with messages to the corresponding [draw] object.
     or   - ds arrays can nest arbitrarily deep. Scelem is for
   group)   data structure arrays.  group is for more groups.
     |
     v
    etc.

   The tag "blankscalar" is for scalars that don't have a visual
   representation, but maybe this can just be merged with "scalar"
*/
static void scalar_vis(t_gobj *z, t_glist *owner, int vis)
{
    //fprintf(stderr,"scalar_vis %d %lx\n", vis, (t_int)z);
    t_scalar *x = (t_scalar *)z;

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
            int x1 = glist_xtopixels(owner, basex);
            int y1 = glist_ytopixels(owner, basey);
            sys_vgui(".x%lx.c create prect %d %d %d %d "
                     "-tags {blankscalar%lx %s}\n",
                glist_getcanvas(owner), x1-1, y1-1, x1+1, y1+1, x,
                (glist_isselected(owner, &x->sc_gobj) ?
                    "scalar_selected" : ""));
        }
        else sys_vgui(".x%lx.c delete blankscalar%lx\n",
            glist_getcanvas(owner), x);
        return;
    }
    //else sys_vgui(".x%lx.c delete blankscalar%lx\n",
    //    glist_getcanvas(owner), x);

    if (vis)
    {
        t_float xscale = glist_xtopixels(owner, 1) - glist_xtopixels(owner, 0);
        t_float yscale = glist_ytopixels(owner, 1) - glist_ytopixels(owner, 0);
        /* we could use the tag .template%lx for easy access from
           the draw_class, but that's not necessary at this point */
        sys_vgui(".x%lx.c create group -tags {.scalar%lx %s} "
            "-matrix { {%g %g} {%g %g} {%d %d} }\n",
            glist_getcanvas(owner), x->sc_vec,
            (glist_isselected(owner, &x->sc_gobj) ? "scalar_selected" : ""),
            xscale, 0.0, 0.0, yscale, (int)glist_xtopixels(owner, basex),
            (int)glist_ytopixels(owner, basey)
            );
        sys_vgui(".x%lx.c create group -tags {.dgroup%lx.%lx} "
                 "-parent {.scalar%lx}\n",
            glist_getcanvas(owner), templatecanvas, x->sc_vec, x->sc_vec);
        sys_vgui("pdtk_bind_scalar_mouseover "
                 ".x%lx.c .x%lx.x%lx.template%lx {.x%lx}\n",
            glist_getcanvas(owner), glist_getcanvas(owner),
            owner, x->sc_vec, x);
    }

    /* warning: don't need--- have recursive func. */
    for (y = templatecanvas->gl_list; y; y = y->g_next)
    {
        t_parentwidgetbehavior *wb = pd_getparentwidget(&y->g_pd);
        if (!wb)
        {
            /* check subpatches for more drawing commands.  This
               can be optimized to only search [group] subpatches */
            if (pd_class(&y->g_pd) == canvas_class &&
                ((t_glist *)y)->gl_svg)
            {
                scalar_groupvis(x, owner, template,
                    (t_glist *)y, templatecanvas, vis);
            }
            continue;
        }
        (*wb->w_parentvisfn)(y, owner, 0, x, x->sc_vec, template,
            basex, basey, vis);
    }
    if (!vis)
        sys_vgui(".x%lx.c delete .scalar%lx\n", glist_getcanvas(owner),
            x->sc_vec);

    sys_unqueuegui(x);
    if (glist_isselected(owner, &x->sc_gobj))
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
    scalar_vis(client, glist, 0);
    scalar_vis(client, glist, 1);
    if (glist_isselected(glist_getcanvas(glist), (t_gobj *)glist))
    {
        //fprintf(stderr,"yes\n");
        sys_vgui("pdtk_select_all_gop_widgets .x%lx %lx %d\n",
            glist_getcanvas(glist), glist, 1);
    }
    sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", glist_getcanvas(glist));
}

void scalar_redraw(t_scalar *x, t_glist *glist)
{
    if (glist_isvisible(glist))
        scalar_doredraw((t_gobj *)x, glist);
        //sys_queuegui(x, glist, scalar_doredraw);
}

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

int scalar_doclick(t_word *data, t_template *template, t_scalar *sc,
    t_array *ap, struct _glist *owner,
    t_float xloc, t_float yloc, int xpix, int ypix,
    int shift, int alt, int dbl, int doit)
{
    int hit = 0;
    t_canvas *templatecanvas = template_findcanvas(template);
    t_atom at[2];
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
        basex = 0.0;
        basey = 0.0;
    }

    hit = scalar_groupclick(templatecanvas, data, template, sc, ap,
                owner, xloc, yloc, xpix, ypix,
                shift, alt, dbl, doit, basex, basey);
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

static void scalar_properties(t_gobj *z, struct _glist *owner)
{
    t_scalar *x = (t_scalar *)z;
    char *buf, buf2[80];
    int bufsize;
    t_binbuf *b;
    glist_noselect(owner);
    glist_select(owner, z);
    b = glist_writetobinbuf(owner, 0);
    binbuf_gettext(b, &buf, &bufsize);
    binbuf_free(b);
    buf = t_resizebytes(buf, bufsize, bufsize+1);
    buf[bufsize] = 0;
    sprintf(buf2, "pdtk_data_dialog %%s {");
    gfxstub_new((t_pd *)owner, x, buf2);
    sys_gui(buf);
    sys_gui("}\n");
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
    char buf[50];
    sprintf(buf, ".x%lx", (long unsigned int)x);
    pd_unbind(&x->sc_gobj.g_pd, gensym(buf));
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
    class_addmethod(scalar_class, (t_method)scalar_mouseover,
        gensym("mouseover"), A_FLOAT, A_NULL);
    class_setwidget(scalar_class, &scalar_widgetbehavior);
    class_setsavefn(scalar_class, scalar_save);
    class_setpropertiesfn(scalar_class, scalar_properties);
}
