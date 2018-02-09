/* Copyright (c) 1997-2001 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This file deals with the behavior of glists as either "text objects" or
"graphs" inside another glist.  LATER move the inlet/outlet code of g_canvas.c 
to this file... */

#include <stdlib.h>
#include "m_pd.h"
#include "m_imp.h"
#include "g_canvas.h"
#include "g_all_guis.h" /* for canvas handle freeing */
#include "s_stuff.h"    /* for sys_hostfontsize */
#include <stdio.h>
#include <string.h>

extern int array_joc;
int garray_joc(t_garray *x);

/* ---------------------- forward definitions ----------------- */

static void graph_vis(t_gobj *gr, t_glist *unused_glist, int vis);
void graph_graphrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2);
static void graph_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2);
void graph_checkgop_rect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2);

extern t_template *template_findbydrawcommand(t_gobj *g);

extern int do_not_redraw;
int gop_redraw = 0;

/* -------------------- maintaining the list -------------------- */

void canvas_drawredrect(t_canvas *x, int doit);

int canvas_isgroup(t_canvas *x)
{
    //t_binbuf *b = x->gl_obj.te_binbuf;
    //if (!b)
    //{
    //    bug("canvas_isgroup");
    //    return 0;
    //}
    //t_atom *argv = binbuf_getvec(x->gl_obj.te_binbuf);
    //if (argv[0].a_type == A_SYMBOL &&
    //    argv[0].a_w.w_symbol == gensym("g"))
    //    return 1;
    //else
    //    return 0;
    if (x->gl_svg)
        return 1;
    else
        return 0;
}

extern t_template *canvas_findtemplate(t_canvas *c);
extern t_canvas *canvas_templatecanvas_forgroup(t_canvas *c);

void glist_add(t_glist *x, t_gobj *y)
{
    //fprintf(stderr,"glist_add %lx %d\n", (t_int)x, (x->gl_editor ? 1 : 0));    
    t_object *ob;
    y->g_next = 0;
    int index = 0;

    if (!x->gl_list) x->gl_list = y;
    else
    {
        t_gobj *y2;
        for (y2 = x->gl_list; y2->g_next; y2 = y2->g_next)
            index++;
        y2->g_next = y;
    }
    if (x->gl_editor && (ob = pd_checkobject(&y->g_pd)))
    {
        rtext_new(x, ob);
        //let's now set up create undo
        //glist_select(x, y);
        //canvas_setundo(x, canvas_undo_create,
        //    canvas_undo_set_create(x, index), "create");
        //glist_noselect(x);
    }
    if (x->gl_editor && x->gl_isgraph && !x->gl_goprect
        && pd_checkobject(&y->g_pd))
    {
        x->gl_goprect = 1;
        canvas_drawredrect(x, 1);
    }
    if (glist_isvisible(x))
        gobj_vis(y, x, 1);
    if (class_isdrawcommand(y->g_pd)) 
    {
        t_template *tmpl = template_findbydrawcommand(y);
        canvas_redrawallfortemplate(tmpl, 0);
    }
    if (pd_class(&y->g_pd) == canvas_class &&
        canvas_isgroup((t_canvas *)y))
    {
        t_canvas *templatecanvas =
            canvas_templatecanvas_forgroup((t_canvas *)y);
        t_template *tmpl = canvas_findtemplate(templatecanvas);
        canvas_redrawallfortemplate(tmpl, 0);
    }
}

    /* this is to protect against a hairy problem in which deleting
    a sub-canvas might delete an inlet on a box, after the box had
    been invisible-ized, so that we have to protect against redrawing it! */
int canvas_setdeleting(t_canvas *x, int flag)
{
    int ret = x->gl_isdeleting;
    x->gl_isdeleting = flag;
    return (ret);
}

    /* check if canvas has an array and return 1, otherwise return 0
    this is used to prevent creation of new objects in an array window */
int canvas_hasarray(t_canvas *x)
{
    t_gobj *g = x->gl_list;
    int hasarray = 0;
    while (g)
    {
        if (pd_class(&g->g_pd) == garray_class) hasarray = 1;
        g = g->g_next;
    }
    return(hasarray);
}

/* JMZ: emit a closebang message */
void canvas_closebang(t_canvas *x);

    /* delete an object from a glist and free it */
void glist_delete(t_glist *x, t_gobj *y)
{
    //fprintf(stderr,"glist_delete y=%lx x=%lx glist_getcanvas=%lx\n", y, x, glist_getcanvas(x));
    if (x->gl_list)
    {
        //fprintf(stderr,"glist_delete YES\n");
        t_gobj *g;
        t_object *ob;
        t_template *tmpl = NULL;
        t_gotfn chkdsp = zgetfn(&y->g_pd, gensym("dsp"));
        t_canvas *canvas = glist_getcanvas(x);
        int drawcommand = class_isdrawcommand(y->g_pd);
        int wasdeleting;
        t_rtext *rt = NULL;
        int late_rtext_free = 0;

        if (pd_class(&y->g_pd) == canvas_class)
        {
          /* JMZ: send a closebang to the canvas */
          canvas_closebang((t_canvas *)y);
          /* and this little hack so drawing commands can tell
             if a [group] is deleting them (and thus suppress
             their own redraws) */
          ((t_canvas *)y)->gl_unloading = 1;
          /* if we are a group, let's call ourselves a drawcommand */
          if (((t_canvas *)y)->gl_svg)
              drawcommand = 1;
        }
     
        wasdeleting = canvas_setdeleting(canvas, 1);
        if (x->gl_editor)
        {
            if (x->gl_editor->e_grab == y) x->gl_editor->e_grab = 0;
            if (glist_isselected(x, y)) glist_deselect(x, y);

                /* HACK -- we had phantom outlets not getting erased on the
                screen because the canvas_setdeleting() mechanism is too
                crude.  LATER carefully set up rules for when the rtexts
                should exist, so that they stay around until all the
                steps of becoming invisible are done.  In the meantime, just
                zap the inlets and outlets here... */
            if (pd_class(&y->g_pd) == canvas_class)
            {
                if (glist_isvisible(x))
                {
                    t_glist *gl = (t_glist *)y;
                    if (gl->gl_isgraph)
                    {
                        char tag[80];
                        //sprintf(tag, "graph%lx", (t_int)gl);
                        //t_glist *yy = (t_glist *)y;
                        sprintf(tag, "%s",
                            rtext_gettag(glist_findrtext(x, &gl->gl_obj)));
                        glist_eraseiofor(x, &gl->gl_obj, tag);
                        text_eraseborder(&gl->gl_obj, x,
                            rtext_gettag(glist_findrtext(x, &gl->gl_obj)));
                    }
                    else
                    {
                        text_eraseborder(&gl->gl_obj, x,
                            rtext_gettag(glist_findrtext(x, &gl->gl_obj)));
                    }
                }
            }
        }
            /* if we're a drawing command, erase all scalars that
                       belong to our template, before deleting
               it; we'll redraw them once it's deleted below. */
        if (drawcommand)
        {
            tmpl = template_findbydrawcommand(y);
            if (!(canvas_isgroup(canvas) && canvas->gl_unloading))
            {
                canvas_redrawallfortemplate(tmpl, 2);
            }
        }
        if (glist_isvisible(canvas))
        {
            //fprintf(stderr,"...deleting %lx %lx\n", x, glist_getcanvas(x));
            gobj_vis(y, x, 0);
        }
        if (x->gl_editor && (ob = pd_checkobject(&y->g_pd)))
        {
            //rtext_new(x, ob);
            rt = glist_findrtext(x, ob);
            if (rt)
                late_rtext_free = 1;
        }
        if (x->gl_list == y)
        {
            if (y->g_next)
                x->gl_list = y->g_next;
            else
                x->gl_list = NULL;
        }
        else for (g = x->gl_list; g; g = g->g_next)
        {
            if (g->g_next == y)
            {
                if (y->g_next)
                    g->g_next = y->g_next;
                else g->g_next = NULL;
                break;
            }
        }
        gobj_delete(y, x);
        pd_free(&y->g_pd);
        if (chkdsp) canvas_update_dsp();
        if (drawcommand)
        {
            if (tmpl != NULL && !(canvas_isgroup(canvas) && canvas->gl_unloading))
            {
                canvas_redrawallfortemplate(tmpl, 1);
            }
        }
        canvas_setdeleting(canvas, wasdeleting);
        x->gl_valid = ++glist_valid;
        if (late_rtext_free)
        {
            //fprintf(stderr,"glist_delete late_rtext_free\n");
            rtext_free(rt);
        }
    }
}

    /* remove every object from a glist.  Experimental. */
void glist_clear(t_glist *x)
{
    t_gobj *y;
    int dspstate = 0, suspended = 0;
    t_symbol *dspsym = gensym("dsp");
    while (y = x->gl_list)
    {
            /* to avoid unnecessary DSP resorting, we suspend DSP
            only if we hit a patchable object. */
        if (!suspended && pd_checkobject(&y->g_pd) && zgetfn(&y->g_pd, dspsym))
        {
            dspstate = canvas_suspend_dsp();
            suspended = 1;
        }
            /* here's the real deletion. */
        glist_delete(x, y);
    }
    if (suspended)
        canvas_resume_dsp(dspstate);
}

void glist_retext(t_glist *glist, t_text *y)
{
        /* check that we have built rtexts yet.  LATER need a better test. */
    if (glist->gl_editor && glist->gl_editor->e_rtext)
    {
        t_rtext *rt = glist_findrtext(glist, y);
        if (rt)
            rtext_retext(rt);
    }
}

void glist_grab(t_glist *x, t_gobj *y, t_glistmotionfn motionfn,
    t_glistkeyfn keyfn, int xpos, int ypos)
{
    //fprintf(stderr,"glist_grab\n");
    t_glist *x2 = glist_getcanvas(x);
    if (motionfn)
        x2->gl_editor->e_onmotion = MA_PASSOUT;
    else x2->gl_editor->e_onmotion = 0;
    x2->gl_editor->e_grab = y;
    x2->gl_editor->e_motionfn = motionfn;
    x2->gl_editor->e_keyfn = keyfn;
    x2->gl_editor->e_xwas = xpos;
    x2->gl_editor->e_ywas = ypos;
}

t_canvas *glist_getcanvas(t_glist *x)
{
    //fprintf(stderr,"glist_getcanvas\n");
    while (x->gl_owner && !x->gl_havewindow && x->gl_isgraph)
    {
            //fprintf(stderr,"x=%lx x->gl_owner=%d x->gl_havewindow=%d "
            //               "x->gl_isgraph=%d gobj_shouldvis=%d\n", 
            //    x, (x->gl_owner ? 1:0), x->gl_havewindow, x->gl_isgraph,
            //    gobj_shouldvis(&x->gl_gobj, x->gl_owner));
            x = x->gl_owner;
            //fprintf(stderr,"+\n");
    }
    return((t_canvas *)x);
}

static t_float gobj_getxforsort(t_gobj *g)
{
    if (pd_class(&g->g_pd) == scalar_class)
    {
        t_float x1, y1;
        scalar_getbasexy((t_scalar *)g, &x1, &y1);
        return(x1);
    }
    else return (0);
}

static t_gobj *glist_merge(t_glist *x, t_gobj *g1, t_gobj *g2)
{
    t_gobj *g = 0, *g9 = 0;
    t_float f1 = 0, f2 = 0;
    if (g1)
        f1 = gobj_getxforsort(g1);
    if (g2)
        f2 = gobj_getxforsort(g2);
    while (1)
    {
        if (g1)
        {
            if (g2)
            {
                if (f1 <= f2)
                    goto put1;
                else goto put2;
            }
            else goto put1;     
        }
        else if (g2)
            goto put2;
        else break;
    put1:
        if (g9)
            g9->g_next = g1, g9 = g1;
        else g9 = g = g1;
        if (g1 = g1->g_next)
            f1 = gobj_getxforsort(g1);
        g9->g_next = 0;
        continue;
    put2:
        if (g9)
            g9->g_next = g2, g9 = g2;
        else g9 = g = g2;
        if (g2 = g2->g_next)
            f2 = gobj_getxforsort(g2);
        g9->g_next = 0;
        continue;
    }
    return (g);
}

static t_gobj *glist_dosort(t_glist *x,
    t_gobj *g, int nitems)
{
    if (nitems < 2)
        return (g);
    else
    {
        int n1 = nitems/2, n2 = nitems - n1, i;
        t_gobj *g2, *g3;
        for (g2 = g, i = n1-1; i--; g2 = g2->g_next)
            ;
        g3 = g2->g_next;
        g2->g_next = 0;
        g = glist_dosort(x, g, n1);
        g3 = glist_dosort(x, g3, n2);
        return (glist_merge(x, g, g3));
    }
}

void glist_sort(t_glist *x)
{
    int nitems = 0, foo = 0;
    t_float lastx = -1e37;
    t_gobj *g;
    for (g = x->gl_list; g; g = g->g_next)
    {
        t_float x1 = gobj_getxforsort(g);
        if (x1 < lastx)
            foo = 1;
        lastx = x1;
        nitems++;
    }
    if (foo)
        x->gl_list = glist_dosort(x, x->gl_list, nitems);
}

/* --------------- inlets and outlets  ----------- */


t_inlet *canvas_addinlet(t_canvas *x, t_pd *who, t_symbol *s)
{
    //fprintf(stderr,"canvas_addinlet %d %lx %d\n", x->gl_loading, x->gl_owner, glist_isvisible(x->gl_owner));
    t_inlet *ip = inlet_new(&x->gl_obj, who, s, 0);
    if (!x->gl_loading && x->gl_owner && glist_isvisible(x->gl_owner))
    {
        gobj_vis(&x->gl_gobj, x->gl_owner, 0);
        gobj_vis(&x->gl_gobj, x->gl_owner, 1);
        canvas_fixlinesfor(x->gl_owner, &x->gl_obj);
    }
    if (!x->gl_loading) canvas_resortinlets(x);
    return (ip);
}

void canvas_rminlet(t_canvas *x, t_inlet *ip)
{
    t_canvas *owner = x->gl_owner;
    int redraw = (owner && glist_isvisible(owner) && (!owner->gl_isdeleting)
        && glist_istoplevel(owner));
    
    if (owner) canvas_deletelinesforio(owner, &x->gl_obj, ip, 0);
    if (redraw)
        gobj_vis(&x->gl_gobj, x->gl_owner, 0);
    inlet_free(ip);
    if (redraw)
    {
        gobj_vis(&x->gl_gobj, x->gl_owner, 1);
        canvas_fixlinesfor(x->gl_owner, &x->gl_obj);
    }
}

extern t_inlet *vinlet_getit(t_pd *x);
extern void obj_moveinletfirst(t_object *x, t_inlet *i);

void canvas_resortinlets(t_canvas *x)
{
    int ninlets = 0, i, j, xmax;
    t_gobj *y, **vec, **vp, **maxp;
    
    for (ninlets = 0, y = x->gl_list; y; y = y->g_next)
        if (pd_class(&y->g_pd) == vinlet_class) ninlets++;

    if (ninlets < 2  && !(canvas_isgroup(x))) return;
    vec = (t_gobj **)getbytes(ninlets * sizeof(*vec));
    
    for (y = x->gl_list, vp = vec; y; y = y->g_next)
        if (pd_class(&y->g_pd) == vinlet_class) *vp++ = y;
    
    for (i = ninlets; i--;)
    {
        t_inlet *ip;
        for (vp = vec, xmax = -0x7fffffff, maxp = 0, j = ninlets;
            j--; vp++)
        {
            int x1, y1, x2, y2;
            t_gobj *g = *vp;
            if (!g) continue;
            gobj_getrect(g, x, &x1, &y1, &x2, &y2);
            if (x1 > xmax) xmax = x1, maxp = vp;
        }
        if (!maxp) break;
        y = *maxp;
        *maxp = 0;
        ip = vinlet_getit(&y->g_pd);
        
        obj_moveinletfirst(&x->gl_obj, ip);
    }
    freebytes(vec, ninlets * sizeof(*vec));
    if (x->gl_owner &&
        glist_isvisible(x->gl_owner) && glist_isvisible(x) &&
        !x->gl_owner->gl_loading && !x->gl_loading)
    {
        canvas_fixlinesfor(x->gl_owner, &x->gl_obj);
        //fprintf(stderr,"good place to fix redrawing of inlets "
        //               ".x%lx owner=.x%lx %d (parent)%d\n",
        //    x, x->gl_owner, x->gl_loading, x->gl_owner->gl_loading);

        /*
        t_object *ob = pd_checkobject(&y->g_pd);
        t_rtext *rt = glist_findrtext(x->gl_owner, (t_text *)&ob->ob_g);
        for (i = 0; i < ninlets; i++)
        {
            //sys_vgui(".x%x.c itemconfigure %si%d -fill %s -width 1\n",
            //    x, rtext_gettag(rt), i, 
            //    (obj_issignalinlet(ob, i) ?
            //        "$signal_nlet" : "$pd_colors_control_nlet)"));
            sprintf(xlet_tag, "%si%d", rtext_gettag(rt), i);
            char xlet_tag[MAXPDSTRING];
            gui_vmess("gui_gobj_configure_io", "xsiii",
                x,
                xlet_tag,
                0,
                obj_issignalinlet(ob, i),
                1);
        }
        */

        //glist_redraw(x);
        graph_vis(&x->gl_gobj, x->gl_owner, 0); 
        graph_vis(&x->gl_gobj, x->gl_owner, 1);
    }
}

t_outlet *canvas_addoutlet(t_canvas *x, t_pd *who, t_symbol *s)
{
    t_outlet *op = outlet_new(&x->gl_obj, s);
    if (!x->gl_loading && x->gl_owner && glist_isvisible(x->gl_owner))
    {
        gobj_vis(&x->gl_gobj, x->gl_owner, 0);
        gobj_vis(&x->gl_gobj, x->gl_owner, 1);
        canvas_fixlinesfor(x->gl_owner, &x->gl_obj);
    }
    if (!x->gl_loading) canvas_resortoutlets(x);
    return (op);
}

void canvas_rmoutlet(t_canvas *x, t_outlet *op)
{
    t_canvas *owner = x->gl_owner;
    int redraw = (owner && glist_isvisible(owner) && (!owner->gl_isdeleting)
        && glist_istoplevel(owner));
    
    if (owner) canvas_deletelinesforio(owner, &x->gl_obj, 0, op);
    if (redraw)
        gobj_vis(&x->gl_gobj, x->gl_owner, 0);

    outlet_free(op);
    if (redraw)
    {
        gobj_vis(&x->gl_gobj, x->gl_owner, 1);
        canvas_fixlinesfor(x->gl_owner, &x->gl_obj);
    }
}

extern t_outlet *voutlet_getit(t_pd *x);
extern void obj_moveoutletfirst(t_object *x, t_outlet *i);

void canvas_resortoutlets(t_canvas *x)
{
    int noutlets = 0, i, j, xmax;
    t_gobj *y, **vec, **vp, **maxp;
    
    for (noutlets = 0, y = x->gl_list; y; y = y->g_next)
        if (pd_class(&y->g_pd) == voutlet_class) noutlets++;

    if (noutlets < 2 && !(canvas_isgroup(x))) return;
    
    vec = (t_gobj **)getbytes(noutlets * sizeof(*vec));
    
    for (y = x->gl_list, vp = vec; y; y = y->g_next)
        if (pd_class(&y->g_pd) == voutlet_class) *vp++ = y;
    
    for (i = noutlets; i--;)
    {
        t_outlet *ip;
        for (vp = vec, xmax = -0x7fffffff, maxp = 0, j = noutlets;
            j--; vp++)
        {
            int x1, y1, x2, y2;
            t_gobj *g = *vp;
            if (!g) continue;
            gobj_getrect(g, x, &x1, &y1, &x2, &y2);
            if (x1 > xmax) xmax = x1, maxp = vp;
        }
        if (!maxp) break;
        y = *maxp;
        *maxp = 0;
        ip = voutlet_getit(&y->g_pd);
        
        obj_moveoutletfirst(&x->gl_obj, ip);
    }
    freebytes(vec, noutlets * sizeof(*vec));
    if (x->gl_owner &&
        glist_isvisible(x->gl_owner) && glist_isvisible(x) &&
        !x->gl_owner->gl_loading && !x->gl_loading)
    {
        canvas_fixlinesfor(x->gl_owner, &x->gl_obj);
        //fprintf(stderr,"good place to fix redrawing of outlets\n");
        //fprintf(stderr,"found it\n");
        //glist_redraw(x);
        graph_vis(&x->gl_gobj, x->gl_owner, 0); 
        graph_vis(&x->gl_gobj, x->gl_owner, 1);
    }
}

/* ----------calculating coordinates and controlling appearance --------- */


static void graph_bounds(t_glist *x, t_floatarg x1, t_floatarg y1,
    t_floatarg x2, t_floatarg y2)
{
    if (x1==x2 || y1==y2) {
        error("graph: empty bounds rectangle");
        x1 = y1 = 0;
        x2 = y2 = 1;
    }
    if (x->gl_x1!=x1 || x->gl_y1!=y1 || x->gl_x2!=x2 || x->gl_y2!=y2) {
        //printf("%f %f %f %f %f %f %f %f\n",x->gl_x1,x1,x->gl_y1,y1,x->gl_x2,x2,x->gl_y2,y2);
        x->gl_x1 = x1;
        x->gl_x2 = x2;
        x->gl_y1 = y1;
        x->gl_y2 = y2;
        if (!do_not_redraw)
            glist_redraw(x);
    }
}

static void graph_xticks(t_glist *x,
    t_floatarg point, t_floatarg inc, t_floatarg f)
{
    x->gl_xtick.k_point = point;
    x->gl_xtick.k_inc = inc;
    x->gl_xtick.k_lperb = f;
    glist_redraw(x);
}

static void graph_yticks(t_glist *x,
    t_floatarg point, t_floatarg inc, t_floatarg f)
{
    x->gl_ytick.k_point = point;
    x->gl_ytick.k_inc = inc;
    x->gl_ytick.k_lperb = f;
    glist_redraw(x);
}

static void graph_xlabel(t_glist *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    if (argc < 1) error("graph_xlabel: no y value given");
    else
    {
        x->gl_xlabely = atom_getfloat(argv);
        argv++; argc--;
        x->gl_xlabel = (t_symbol **)t_resizebytes(x->gl_xlabel, 
            x->gl_nxlabels * sizeof (t_symbol *), argc * sizeof (t_symbol *));
        x->gl_nxlabels = argc;
        for (i = 0; i < argc; i++) x->gl_xlabel[i] = atom_gensym(&argv[i]);
    }
    glist_redraw(x);
}
    
static void graph_ylabel(t_glist *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    if (argc < 1) error("graph_ylabel: no x value given");
    else
    {
        x->gl_ylabelx = atom_getfloat(argv);
        argv++; argc--;
        x->gl_ylabel = (t_symbol **)t_resizebytes(x->gl_ylabel, 
            x->gl_nylabels * sizeof (t_symbol *), argc * sizeof (t_symbol *));
        x->gl_nylabels = argc;
        for (i = 0; i < argc; i++) x->gl_ylabel[i] = atom_gensym(&argv[i]);
    }
    glist_redraw(x);
}

/****** routines to convert pixels to X or Y value and vice versa ******/

    /* convert an x pixel value to an x coordinate value */
t_float glist_pixelstox(t_glist *x, t_float xpix)
{
        /* if we appear as a text box on parent, our range in our
        coordinates (x1, etc.) specifies the coordinate range
        of a one-pixel square at top left of the window. */
    if (!x->gl_isgraph)
        return (x->gl_x1 + (x->gl_x2 - x->gl_x1) * xpix);

        /* if we're a graph when shown on parent, but own our own
        window right now, our range in our coordinates (x1, etc.) is spread
        over the visible window size, given by screenx1, etc. */  
    else if (x->gl_isgraph && x->gl_havewindow)
        return (x->gl_x1 + (x->gl_x2 - x->gl_x1) * 
            (xpix) / (x->gl_screenx2 - x->gl_screenx1));

        /* otherwise, we appear in a graph within a parent glist,
         so get our screen rectangle on parent and transform. */
    else 
    {
        int x1, y1, x2, y2;
        if (!x->gl_owner)
            bug("glist_pixelstox");         
        graph_graphrect(&x->gl_gobj, x->gl_owner, &x1, &y1, &x2, &y2);
        return (x->gl_x1 + (x->gl_x2 - x->gl_x1) * 
            (xpix - x1) / (x2 - x1));
    }
}

t_float glist_pixelstoy(t_glist *x, t_float ypix)
{
    if (!x->gl_isgraph)
        return (x->gl_y1 + (x->gl_y2 - x->gl_y1) * ypix);
    else if (x->gl_isgraph && x->gl_havewindow)
        return (x->gl_y1 + (x->gl_y2 - x->gl_y1) * 
                (ypix) / (x->gl_screeny2 - x->gl_screeny1));
    else 
    {
        int x1, y1, x2, y2;
        if (!x->gl_owner)
            bug("glist_pixelstoy");
        graph_graphrect(&x->gl_gobj, x->gl_owner, &x1, &y1, &x2, &y2);
        return (x->gl_y1 + (x->gl_y2 - x->gl_y1) * 
            (ypix - y1) / (y2 - y1));
    }
}

    /* convert an x coordinate value to an x pixel location in window */
t_float glist_xtopixels(t_glist *x, t_float xval)
{
    if (!x->gl_isgraph)
        return ((xval - x->gl_x1) / (x->gl_x2 - x->gl_x1));
    else if (x->gl_isgraph && x->gl_havewindow)
        return (x->gl_screenx2 - x->gl_screenx1) * 
            (xval - x->gl_x1) / (x->gl_x2 - x->gl_x1);
    else
    {
        int x1, y1, x2, y2;
        if (!x->gl_owner)
            bug("glist_pixelstox");
        graph_graphrect(&x->gl_gobj, x->gl_owner, &x1, &y1, &x2, &y2);
        return (x1 + (x2 - x1) * (xval - x->gl_x1) / (x->gl_x2 - x->gl_x1));
    }
}

t_float glist_ytopixels(t_glist *x, t_float yval)
{
    if (!x->gl_isgraph)
        return ((yval - x->gl_y1) / (x->gl_y2 - x->gl_y1));
    else if (x->gl_isgraph && x->gl_havewindow)
        return (x->gl_screeny2 - x->gl_screeny1) * 
                (yval - x->gl_y1) / (x->gl_y2 - x->gl_y1);
    else 
    {
        int x1, y1, x2, y2;
        if (!x->gl_owner)
            bug("glist_pixelstoy");
        graph_graphrect(&x->gl_gobj, x->gl_owner, &x1, &y1, &x2, &y2);
        return (y1 + (y2 - y1) * (yval - x->gl_y1) / (x->gl_y2 - x->gl_y1));
    }
}

    /* convert an X screen distance to an X coordinate increment.
      This is terribly inefficient;
      but probably not a big enough CPU hog to warrant optimizing. */
t_float glist_dpixtodx(t_glist *x, t_float dxpix)
{ 
    return (dxpix * (glist_pixelstox(x, 1) - glist_pixelstox(x, 0)));
}

t_float glist_dpixtody(t_glist *x, t_float dypix)
{
    return (dypix * (glist_pixelstoy(x, 1) - glist_pixelstoy(x, 0)));
}

    /* get the window location in pixels of a "text" object.  The
    object's x and y positions are in pixels when the glist they're
    in is toplevel.  Otherwise, if it's a new-style graph-on-parent
    (so gl_goprect is set) we use the offset into the framing subrectangle
    as an offset into the parent rectangle.  Finally, it might be an old,
    proportional-style GOP.  In this case we do a coordinate transformation. */
int text_xpix(t_text *x, t_glist *glist)
{
    int xpix = 0; 
    if (glist->gl_havewindow || !glist->gl_isgraph)
        xpix = x->te_xpix; 
    else if (glist->gl_goprect)
        xpix = glist_xtopixels(glist, glist->gl_x1) +
            x->te_xpix - glist->gl_xmargin;
    else xpix = (glist_xtopixels(glist, 
            glist->gl_x1 + (glist->gl_x2 - glist->gl_x1) * 
                x->te_xpix / (glist->gl_screenx2 - glist->gl_screenx1)));
    if (x->te_iemgui == 1)
        xpix += ((t_iemgui *)x)->legacy_x*sys_legacy;
    return(xpix);
}

int text_ypix(t_text *x, t_glist *glist)
{
    int ypix = 0; 
    if (glist->gl_havewindow || !glist->gl_isgraph)
        ypix = x->te_ypix; 
    else if (glist->gl_goprect)
        ypix = glist_ytopixels(glist, glist->gl_y1) +
            x->te_ypix - glist->gl_ymargin;
    else ypix = (glist_ytopixels(glist, 
            glist->gl_y1 + (glist->gl_y2 - glist->gl_y1) * 
                x->te_ypix / (glist->gl_screeny2 - glist->gl_screeny1)));
    if (x->te_iemgui == 1)
        ypix += ((t_iemgui *)x)->legacy_y*sys_legacy;
    return(ypix);
}

extern void canvas_updateconnection(t_canvas *x, int lx1, int ly1, int lx2, int ly2, t_int tag);

    /* redraw all the items in a glist.  We construe this to mean
    redrawing in its own window and on parent, as needed in each case.
    This is too conservative -- for instance, when you draw an "open"
    rectangle on the parent, you shouldn't have to redraw the window!  */
void glist_redraw(t_glist *x)
{
    if (glist_isvisible(x))
    {
            /* LATER fix the graph_vis() code to handle both cases */
        if (glist_istoplevel(x) && x->gl_havewindow)
        {
            t_gobj *g;
            t_linetraverser t;
            t_outconnect *oc;
            for (g = x->gl_list; g; g = g->g_next)
            {
                gobj_vis(g, x, 0);
                gobj_vis(g, x, 1);
            }
                /* redraw all the lines */
            linetraverser_start(&t, x);
            while (oc = linetraverser_next(&t))
                canvas_updateconnection(glist_getcanvas(x), t.tr_lx1, t.tr_ly1, t.tr_lx2, t.tr_ly2, (t_int)oc);
                //sys_vgui(".x%lx.c coords l%lx %d %d %d %d\n",
                //    glist_getcanvas(x), oc,
                //        t.tr_lx1, t.tr_ly1, t.tr_lx2, t.tr_ly2);
            canvas_drawredrect(x, 0);
            if (x->gl_goprect)
            {
                //post("draw it");
                /* update gop rect size on toplevel in case font has
                changed and we are showing text */
                /*if (!x->gl_hidetext) {
                    int x1, y1, x2, y2;
                    graph_getrect((t_gobj *)x, x, &x1, &y1, &x2, &y2);
                    if (x2-x1 > x->gl_pixwidth) x->gl_pixwidth = x2-x1;
                    if (y2-y1 > x->gl_pixheight) x->gl_pixheight = y2-y1;
                }*/
                canvas_drawredrect(x, 1);
            }
        }
    }
    if (x->gl_owner && glist_isvisible(x->gl_owner))
    {
        graph_vis(&x->gl_gobj, x->gl_owner, 0); 
        graph_vis(&x->gl_gobj, x->gl_owner, 1);
    }
}

/* --------------------------- widget behavior  ------------------- */

int garray_getname(t_garray *x, t_symbol **namep);


    /* Note that some code in here would also be useful for drawing
    graph decorations in toplevels... */
static void graph_vis(t_gobj *gr, t_glist *parent_glist, int vis)
{
    t_glist *x = (t_glist *)gr;
    //fprintf(stderr,"graph vis canvas=%lx gobj=%lx %d\n",
    //    (t_int)parent_glist, (t_int)gr, vis);
    //fprintf(stderr, "graph_vis gr=.x%lx parent_glist=.x%lx "
    //                "glist_getcanvas(x->gl_owner)=.x%lx vis=%d\n",
    //    (t_int)gr, (t_int)parent_glist,
    //    (t_int)glist_getcanvas(x->gl_owner), vis);  
    char tag[50];
    t_gobj *g;
    int x1, y1, x2, y2;
        /* ordinary subpatches: just act like a text object */
    if (!x->gl_isgraph)
    {
        text_widgetbehavior.w_visfn(gr, parent_glist, vis);
        return;
    }

    // weird exception
    //int exception = 0;
    //t_canvas* tgt = glist_getcanvas(x->gl_owner);
    //if (parent_glist->gl_owner && !parent_glist->gl_mapped &&
    //        parent_glist->gl_owner->gl_mapped) {
    //    tgt = parent_glist;
    //    exception = 1;
    //}
    //fprintf(stderr,"tgt=.x%lx %d\n", (t_int)tgt, exception);

    sprintf(tag, "%s", rtext_gettag(glist_findrtext(parent_glist, &x->gl_obj)));

    if (vis & gobj_shouldvis(gr, parent_glist))
    {
        int xpix, ypix;
        xpix = text_xpix(&x->gl_obj, parent_glist);
        ypix = text_ypix(&x->gl_obj, parent_glist);
        gui_vmess("gui_gobj_new", "xssiii",
            glist_getcanvas(x->gl_owner),
            tag, "graph", xpix, ypix, 1);
        if (canvas_showtext(x))
            rtext_draw(glist_findrtext(parent_glist, &x->gl_obj));
    }

    //sprintf(tag, "%s", rtext_gettag(glist_findrtext(parent_glist, &x->gl_obj)));

    // need the rect to create the gobj, so this should perhaps be above the
    // conditional
    graph_getrect(gr, parent_glist, &x1, &y1, &x2, &y2);
    //fprintf(stderr,"%d %d %d %d\n", x1, y1, x2, y2);
    if (sys_legacy == 1)
    {
        //fprintf(stderr,"legacy  gop\n");
        y1 += 1;
        y2 += 1;
    }

    if (!vis)
        rtext_erase(glist_findrtext(parent_glist, &x->gl_obj));

    //sprintf(tag, "graph%lx", (t_int)x);
    //fprintf(stderr, "gettag=%s, tag=graph%lx\n",
    //    rtext_gettag(glist_findrtext(parent_glist, &x->gl_obj)),(t_int)x);
    /* if we look like a graph but have been moved to a toplevel,
       just show the bounding rectangle */
    if (x->gl_havewindow)
    {
        if (vis && gobj_shouldvis(gr, parent_glist))
        {
            gui_vmess("gui_text_draw_border", "xssiii",
                glist_getcanvas(x->gl_owner),
                tag,
                "none",
                0,
                x2 - x1,
                y2 - y1);
            glist_noselect(x->gl_owner);
            gui_vmess("gui_graph_fill_border", "xsi",
                glist_getcanvas(x->gl_owner),
                tag);
        }
        else if (gobj_shouldvis(gr, parent_glist))
        {
            gui_vmess("gui_gobj_erase", "xs",
                glist_getcanvas(x->gl_owner),
                tag);
        }
        return;
    }
        /* otherwise draw (or erase) us as a graph inside another glist. */
    if (vis)
    {
        int i;
        t_float f;
        t_gobj *g;
        t_symbol *arrayname;
            /* draw a rectangle around the graph */
        char *ylabelanchor =
            (x->gl_ylabelx > 0.5*(x->gl_x1 + x->gl_x2) ? "w" : "e");
        char *xlabelanchor =
            (x->gl_xlabely > 0.5*(x->gl_y1 + x->gl_y2) ? "s" : "n");
        char tagbuf[MAXPDSTRING];
        sprintf(tagbuf, "%sR", tag);
        gui_vmess("gui_text_draw_border", "xssiii",
            glist_getcanvas(x->gl_owner),
            tag,
            "none",
            0,
            x2 - x1,
            y2 - y1);
            /* write garrays' names along the top */
        for (i = 0, g = x->gl_list; g; g = g->g_next, i++)
        {
            //fprintf(stderr,".\n");
            //if (g->g_pd == garray_class)
            //    fprintf(stderr,"garray_getname=%d\n",garray_getname((t_garray *)g, &arrayname));
            if (g->g_pd == garray_class &&
                !garray_getname((t_garray *)g, &arrayname))
            {
                gui_vmess("gui_graph_label", "xsiissisi",
                    glist_getcanvas(x),
                    tag,
                    i,
                    sys_fontheight(glist_getfont(x)),
                    arrayname->s_name,
                    sys_font,
                    sys_hostfontsize(glist_getfont(x)),
                    sys_fontweight,
                    glist_isselected(x, gr));
            }
        }
            /* draw ticks on horizontal borders.  If lperb field is
            zero, this is disabled. */
        if (x->gl_xtick.k_lperb)
        {
            t_float upix, lpix;
            if (y2 < y1)
                upix = y1, lpix = y2;
            else upix = y2, lpix = y1;
            for (i = 0, f = x->gl_xtick.k_point;
                f < 0.99 * x->gl_x2 + 0.01*x->gl_x1; i++,
                    f += x->gl_xtick.k_inc)
            {
                int tickpix = (i % x->gl_xtick.k_lperb ? 2 : 4);
                gui_vmess("gui_graph_vtick", "xsiiiiii",
                    glist_getcanvas(x->gl_owner),
                    tag,
                    (int)glist_xtopixels(x, f),
                    (int)upix,
                    (int)lpix,
                    (int)tickpix,
                    x1,
                    y1);
            }
            for (i = 1, f = x->gl_xtick.k_point - x->gl_xtick.k_inc;
                f > 0.99 * x->gl_x1 + 0.01*x->gl_x2;
                    i++, f -= x->gl_xtick.k_inc)
            {
                int tickpix = (i % x->gl_xtick.k_lperb ? 2 : 4);
                gui_vmess("gui_graph_vtick", "xsiiiiii",
                    glist_getcanvas(x->gl_owner),
                    tag,
                    (int)glist_xtopixels(x, f),
                    (int)upix,
                    (int)lpix,
                    (int)tickpix,
                    x1,
                    y1);
            }
        }

            /* draw ticks in vertical borders*/
        if (x->gl_ytick.k_lperb)
        {
            t_float ubound, lbound;
            if (x->gl_y2 < x->gl_y1)
                ubound = x->gl_y1, lbound = x->gl_y2;
            else ubound = x->gl_y2, lbound = x->gl_y1;
            for (i = 0, f = x->gl_ytick.k_point;
                f < 0.99 * ubound + 0.01 * lbound;
                    i++, f += x->gl_ytick.k_inc)
            {
                int tickpix = (i % x->gl_ytick.k_lperb ? 2 : 4);
                gui_vmess("gui_graph_htick", "xsiiiiii",
                    glist_getcanvas(x->gl_owner),
                    tag,
                    (int)glist_ytopixels(x, f),
                    x1,
                    x2,
                    (int)tickpix,
                    x1,
                    y1);
            }
            for (i = 1, f = x->gl_ytick.k_point - x->gl_ytick.k_inc;
                f > 0.99 * lbound + 0.01 * ubound;
                    i++, f -= x->gl_ytick.k_inc)
            {
                int tickpix = (i % x->gl_ytick.k_lperb ? 2 : 4);
                gui_vmess("gui_graph_htick", "xsiiiiii",
                    glist_getcanvas(x->gl_owner),
                    tag,
                    (int)glist_ytopixels(x, f),
                    x1,
                    x2,
                    (int)tickpix,
                    x1,
                    y1);
            }
        }
            /* draw x labels */
        for (i = 0; i < x->gl_nxlabels; i++)
        {
            gui_vmess("gui_graph_tick_label", "xsiissisiis",
                glist_getcanvas(x),
                tag,
                (int)glist_xtopixels(x, atof(x->gl_xlabel[i]->s_name)),
                (int)glist_ytopixels(x, x->gl_xlabely),
                x->gl_xlabel[i]->s_name,
                sys_font, 
                sys_hostfontsize(glist_getfont(x)),
                sys_fontweight,
                x1,
                y1,
                xlabelanchor);
        }

            /* draw y labels */
        for (i = 0; i < x->gl_nylabels; i++)
        {
            gui_vmess("gui_graph_tick_label", "xsiissisiis",
                glist_getcanvas(x),
                tag,
                (int)glist_xtopixels(x, x->gl_ylabelx),
                (int)glist_ytopixels(x, atof(x->gl_ylabel[i]->s_name)),
                x->gl_ylabel[i]->s_name,
                sys_font, 
                sys_hostfontsize(glist_getfont(x)),
                sys_fontweight,
                x1,
                y1,
                ylabelanchor);
        }

            /* draw contents of graph as glist */
        for (g = x->gl_list; g; g = g->g_next)
        {
            gop_redraw = 1;
            //fprintf(stderr,"drawing gop objects\n");
            gobj_vis(g, x, 1);
            //fprintf(stderr,"done\n");
            gop_redraw = 0;
        }
        /* reselect it upon redrawing if it was selected before */
        glist_drawiofor(parent_glist, &x->gl_obj, 1,
            tag, x1, y1, x2, y2);
        if (glist_isselected(parent_glist, gr))
            gobj_select(gr, parent_glist, 1);
        // here we check for changes in scrollbar because of legacy
        // objects that can fall outside gop window, e.g. scalars
        canvas_getscroll(glist_getcanvas(x->gl_owner));
        //fprintf(stderr,"******************graph_vis SELECT\n");
    }
    else
    {
        glist_eraseiofor(parent_glist, &x->gl_obj, tag);
        for (g = x->gl_list; g; g = g->g_next)
            gobj_vis(g, x, 0);

        gui_vmess("gui_gobj_erase", "xs",
            glist_getcanvas(x->gl_owner),
            tag);

        // here we check for changes in scrollbar because of legacy
        // objects that can fall outside gop window, e.g. scalars
        canvas_getscroll(glist_getcanvas(x->gl_owner));
    }
}

    /* get the graph's rectangle, not counting extra swelling for controls
    to keep them inside the graph.  This is the "logical" pixel size. */

void graph_graphrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_glist *x = (t_glist *)z;
    int x1 = text_xpix(&x->gl_obj, glist);
    int y1 = text_ypix(&x->gl_obj, glist);
    int x2, y2;
    x2 = x1 + x->gl_pixwidth;
    y2 = y1 + x->gl_pixheight;

    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

    /* check if the gop size needs to change due to gop's text
    in case hidetext is not enabled */
void graph_checkgop_rect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{

    //fprintf(stderr,"graph_checkgop_rect\n");
    t_glist *x = (t_glist *)z;
    t_gobj *g;
    int fw = sys_fontwidth(x->gl_font);
    int fh = sys_fontheight(x->gl_font);

    if (!x->gl_hidetext)
    {
        int x21, y21, x22, y22;
        text_widgetbehavior.w_getrectfn(z, glist, &x21, &y21, &x22, &y22);
        if (x22 > *xp2)
            *xp2 = x22;
        if (y22 > *yp2) 
            *yp2 = y22;
        // WARNING: ugly hack trying to replicate rtext_senditup
        // if we have no parent. Later consider fixing hardwired values
        int tcols = strlen(x->gl_name->s_name) - 3;
        int th = fh + fh * (tcols/60) + 4;
        if (tcols > 60) tcols = 60;
        int tw = fw * tcols + 4;
        if (tw + *xp1 > *xp2)
            *xp2 = tw + *xp1;
        if (th + *yp1 > *yp2)
            *yp2 = th + *yp1;
    }

    // check if the gop has array members and if so,
    // make its minimum size based on array names size
    t_symbol *arrayname;
    int cols_tmp = 0;
    int arrayname_cols = 0;
    int arrayname_rows = 0;
    for (g = x->gl_list; g; g = g->g_next)
    {
        if (pd_class(&g->g_pd) == garray_class &&
            !garray_getname((t_garray *)g, &arrayname))
        {
            arrayname_rows += 1;
            cols_tmp = strlen(arrayname->s_name);
            if(cols_tmp > arrayname_cols) arrayname_cols = cols_tmp;
        }
    }
    if (arrayname_rows)
    {
        int fontwidth = sys_fontwidth(x->gl_font);
        int fontheight = sys_fontheight(x->gl_font);
        if ((arrayname_rows * fontheight - 1) > (*yp2 - *yp1))
            *yp2 = *yp1 + (arrayname_rows * fontheight - 1);
        if ((arrayname_cols * fontwidth + 2) > (*xp2 - *xp1))
            *xp2 = *xp1 + (arrayname_cols * fontwidth + 2);
    }

    // failsafe where we cannot have a gop that is smaller than 1x1 pixels
    // regardless whether the text is hidden
    int in = obj_ninlets(pd_checkobject(&z->g_pd));
    int out = obj_noutlets(pd_checkobject(&z->g_pd));
    int max_xlets = in >= out ? in : out;
    int minhsize = (max_xlets * IOWIDTH) +
        ((max_xlets > 1 ? max_xlets - 1 : 0) * IOWIDTH);
    if (minhsize < SCALE_GOP_MINWIDTH) minhsize = SCALE_GOP_MINWIDTH;
    int minvsize = ((in > 0 ? 1 : 0) + (out > 0 ? 1 : 0)) * 2;
    if (minvsize < SCALE_GOP_MINHEIGHT) minvsize = SCALE_GOP_MINHEIGHT;
    if (*xp2 < *xp1+minhsize) *xp2 = *xp1+minhsize;
    if (*yp2 < *yp1+minvsize) *yp2 = *yp1+minvsize;
}

    /* get the rectangle, enlarged to contain all the "contents" --
    meaning their formal bounds rectangles. */
static void graph_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    int x1 = 0x7fffffff, y1 = 0x7fffffff, x2 = -0x7fffffff, y2 = -0x7fffffff;
    t_glist *x = (t_glist *)z;
    //fprintf(stderr,"graph_getrect %d\n", x->gl_isgraph);
    if (x->gl_isgraph)
    {
        int hadwindow;
        t_gobj *g;
        int x21, y21, x22, y22;
        graph_graphrect(z, glist, &x1, &y1, &x2, &y2);
        //fprintf(stderr,"%d %d %d %d\n", x1, y1, x2, y2);

        if (canvas_showtext(x))
        {
            text_widgetbehavior.w_getrectfn(z, glist, &x21, &y21, &x22, &y22);
            if (x22 > x2) 
                x2 = x22;
            if (y22 > y2) 
                y2 = y22;
            //fprintf(stderr,"canvas_showtext %d %d %d %d\n", x1, y1, x2, y2);
        }
        if (!x->gl_goprect)
        {
            /* expand the rectangle to fit in text objects; this applies only
            to the old (0.37) graph-on-parent behavior. */
            /* lie about whether we have our own window to affect gobj_getrect
            calls below.  */
            hadwindow = x->gl_havewindow;
            x->gl_havewindow = 0;
            for (g = x->gl_list; g; g = g->g_next)
            {
                    /* don't do this for arrays, just let them hang outside the
                    box. */
                if (pd_class(&g->g_pd) == garray_class ||
                    pd_class(&g->g_pd) == scalar_class)
                    continue;
                gobj_getrect(g, x, &x21, &y21, &x22, &y22);
                if (x22 > x2) 
                    x2 = x22;
                if (y22 > y2) 
                    y2 = y22;
            }
            x->gl_havewindow = hadwindow;
        }

        //fprintf(stderr,"%d %d %d %d\n", x1, y1, x2, y2);

        // check if the text is not hidden and if so use that as the
        // limit of the gop's size (we check for hidden flag inside
        // the function we point to)
        graph_checkgop_rect(z, glist, &x1, &y1, &x2, &y2);

        /* fix visibility of edge items for garrays */
        /*
        int has_garray = 0;
        for (g = x->gl_list; g; g = g->g_next)
        {
            if (pd_class(&g->g_pd) == garray_class)
            {
                has_garray = 1;
            }
        }
        if (has_garray) {
            x1 -= 1;
            y1 -= 2;
            //x2 += 1;
            y2 += 1;
        }*/
    }
    else text_widgetbehavior.w_getrectfn(z, glist, &x1, &y1, &x2, &y2);

    if (sys_legacy == 1)
    {
        //fprintf(stderr,"legacy  gop\n");
        y1 += 1;
        y2 += 1;
    }
    //fprintf(stderr,"    post %d %d %d %d\n", x1, y1, x2, y2); 

    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

static void graph_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    //fprintf(stderr,"graph_displace %d %d\n", dx, dy);
    t_glist *x = (t_glist *)z;
    if (!x->gl_isgraph)
        text_widgetbehavior.w_displacefn(z, glist, dx, dy);
    else
    {
        x->gl_obj.te_xpix += dx;
        x->gl_obj.te_ypix += dy;
        /*char tag[80];
        sprintf(tag, "%s",
            rtext_gettag(
                glist_findrtext((x->gl_owner ? x->gl_owner: x), &x->gl_obj)));
        sys_vgui(".x%lx.c move %s %d %d\n",
            glist_getcanvas(x->gl_owner), tag, dx, dy);
        sys_vgui(".x%lx.c move %sR %d %d\n",
            glist_getcanvas(x->gl_owner), tag, dx, dy);*/
        if (!do_not_redraw)
        {
            //fprintf(stderr,"graph_displace redraw\n");
            glist_redraw(glist_getcanvas(glist));
            //gobj_select(z, glist, 1);
            canvas_fixlinesfor(glist_getcanvas(glist), &x->gl_obj);
        }
    }
}

extern int old_displace; //from g_editor.c for legacy drawing

static void graph_displace_scalars(t_glist *x, t_glist *glist, int dx, int dy)
{
    t_gobj *g;
    for (g = x->gl_list; g; g = g->g_next)
    {
        if (pd_class((t_pd *)g) == scalar_class &&
            g->g_pd->c_wb->w_displacefnwtag != NULL)
        {
            (*(g->g_pd->c_wb->w_displacefnwtag))(g, glist, dx, dy);
        }
        else if (pd_class(&g->g_pd) == canvas_class &&
                 ((t_glist *)g)->gl_isgraph)
        {
            graph_displace_scalars((t_glist *)g, glist, dx, dy); 
        }
    }
}

static void graph_displace_withtag(t_gobj *z, t_glist *glist, int dx, int dy)
{
    //fprintf(stderr,"graph_displace_withtag %d %d\n", dx, dy);
    t_glist *x = (t_glist *)z;
    if (!x->gl_isgraph)
        text_widgetbehavior.w_displacefnwtag(z, glist, dx, dy);
    else
    {
        // first check for legacy objects that don't offer displacefnwtag
        // and fallback on the old way of doing things
        t_gobj *g;
        /* special case for scalars, which have a group for
           the transform matrix */
        graph_displace_scalars(x, glist, dx, dy); 
        for (g = x->gl_list; g; g = g->g_next)
        {
            //fprintf(stderr,"shouldvis %d %d\n",
            //    gobj_shouldvis(g, glist), gobj_shouldvis(g, x));
            if (g && gobj_shouldvis(g, x) &&
                g->g_pd->c_wb->w_displacefnwtag == NULL &&
                pd_class((t_pd *)g) != garray_class)
            {
                //fprintf(stderr,"old way\n");
                old_displace = 1;
                graph_displace(z, glist, dx, dy);
                return;
            }
        }
        // else we do things the new and more elegant way
        //fprintf(stderr,"new way\n");

        
        x->gl_obj.te_xpix += dx;
        x->gl_obj.te_ypix += dy;
        canvas_fixlinesfor(glist_getcanvas(glist), &x->gl_obj);
    }
}

static void graph_select(t_gobj *z, t_glist *glist, int state)
{
    //fprintf(stderr,"graph_select .x%lx .x%lx %d...\n",
    //    (t_int)z, (t_int)glist, state);
    t_glist *x = (t_glist *)z;
    if (!x->gl_isgraph)
        text_widgetbehavior.w_selectfn(z, glist, state);
    else //if(glist_istoplevel(glist))
    {
        //fprintf(stderr,"...yes\n");
        //fprintf(stderr,"%lx %lx %lx\n", glist_getcanvas(glist), glist, x);
        t_rtext *y = glist_findrtext(glist, &x->gl_obj);
        if (canvas_showtext(x))
        {
            rtext_select(y, state);
        }

        t_glist *canvas;
        if (!glist_istoplevel(glist))
        {
            canvas = glist_getcanvas(glist);
        }
        else
        {
            canvas = glist;
        }
        if (glist_isvisible(glist) &&
                (glist_istoplevel(glist) ||
                 gobj_shouldvis(z, glist)))
        {
            if (state)
                gui_vmess("gui_gobj_select", "xs",
                    canvas, rtext_gettag(y));
            else
                gui_vmess("gui_gobj_deselect", "xs",
                    canvas, rtext_gettag(y));
        }

        t_gobj *g;
        //fprintf(stderr,"graph_select\n");
        if (x->gl_list && !glist_istoplevel(x))
        {
            for (g = x->gl_list; g; g = g->g_next)
            {
                //fprintf(stderr,"shouldvis %d\n",gobj_shouldvis(g, x));
                if ((g && gobj_shouldvis(g, x) &&
                    (g->g_pd->c_wb->w_displacefnwtag != NULL) ||
                    (g && pd_class((t_pd *)g) == garray_class)))
                {
                    gobj_select(g, x, state);
                }
            }
        }
        // Don't yet understand the purpose of this call, so not deleting
        // it just yet...
        //sys_vgui("pdtk_select_all_gop_widgets .x%lx %s %d\n",
        //    canvas, rtext_gettag(glist_findrtext(glist, &x->gl_obj)), state);
    }
}

static void graph_activate(t_gobj *z, t_glist *glist, int state)
{
    t_glist *x = (t_glist *)z;
    if (canvas_showtext(x))
        text_widgetbehavior.w_activatefn(z, glist, state);
}

#if 0
static void graph_delete(t_gobj *z, t_glist *glist)
{
    t_glist *x = (t_glist *)z;
    if (!x->gl_isgraph)
        text_widgetbehavior.w_deletefn(z, glist);
    else
    {
        t_gobj *y;
        while (y = x->gl_list) glist_delete(x, y);
#if 0       /* I think this was just wrong. */
        if (glist_isvisible(x))
            sys_vgui(".x%lx.c delete graph%lx\n", glist_getcanvas(glist), x);
#endif
    }
}
#endif

static void graph_delete(t_gobj *z, t_glist *glist)
{
    //fprintf(stderr,"graph_delete\n");
    t_glist *x = (t_glist *)z;
    t_gobj *y;
    text_widgetbehavior.w_deletefn(z, glist);
    while (y = x->gl_list)
    {
        glist_delete(x, y);
    }
    if (glist_istoplevel(glist) && glist_isvisible(glist))
        canvas_getscroll(glist);
}

extern t_class *my_canvas_class; // for ignoring runtime clicks

static int graph_click(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    //fprintf(stderr, "graph_click %d\n", doit);
    t_glist *x = (t_glist *)z;
    t_gobj *y, *clickme = NULL;
    int clickreturned = 0;
    //int tmpclickreturned = 0;
    if (!x->gl_isgraph)
        return (text_widgetbehavior.w_clickfn(z, glist,
            xpix, ypix, shift, alt, dbl, doit));
    else if (x->gl_havewindow)
        return (0);
    else
    {
        for (y = x->gl_list; y; y = y->g_next)
        {
            if(pd_class(&y->g_pd) == garray_class &&
               !y->g_next &&
               (array_joc = garray_joc((t_garray *)y)) &&
               (clickreturned =
                   gobj_click(y, x, xpix, ypix, shift, alt, 0, doit)))
            {
                break;
            }
            else
            {
                int x1, y1, x2, y2;
                t_object *ob;
                /* check if the object wants to be clicked and pick
                   the topmost with the exception of the text (comment)*/
                if (canvas_hitbox(x, y, xpix, ypix, &x1, &y1, &x2, &y2))
                {
                    ob = pd_checkobject(&y->g_pd);
                    /* do not give clicks to comments or cnv during runtime */
                    if (!ob || (ob->te_type != T_TEXT && ob->ob_pd != my_canvas_class)) 
                        clickme = y;
                    //fprintf(stderr,"    found clickable %d\n", clickreturned);
                }
            }
        }
        if (clickme)
        {
            //fprintf(stderr,"    clicking\n");
            clickreturned = gobj_click(clickme, x, xpix, ypix,
                    shift, alt, 0, doit);
        }
        if (!doit)
        {
            //fprintf(stderr,"    not clicking %lx %d\n",
            //    (t_int)clickme, clickreturned);
            if (clickme != NULL)
            {
                //fprintf(stderr,"    cursor %d\n", clickreturned);
                canvas_setcursor(glist_getcanvas(x), clickreturned);
            }
            else if (!array_joc)
            {
                //fprintf(stderr,"    cursor 0\n");
                canvas_setcursor(glist_getcanvas(x), CURSOR_RUNMODE_NOTHING);
            }
        }
        return (clickreturned); 
    }
}

t_widgetbehavior graph_widgetbehavior =
{
    graph_getrect,
    graph_displace,
    graph_select,
    graph_activate,
    graph_delete,
    graph_vis,
    graph_click,
    graph_displace_withtag,
};

    /* find the graph most recently added to this glist;
        if none exists, return 0. */

t_glist *glist_findgraph(t_glist *x)
{
    t_gobj *y = 0, *z;
    for (z = x->gl_list; z; z = z->g_next)
        if (pd_class(&z->g_pd) == canvas_class && ((t_glist *)z)->gl_isgraph)
            y = z;
    return ((t_glist *)y);
}

extern void canvas_menuarray(t_glist *canvas);

void g_graph_setup(void)
{
    class_setwidget(canvas_class, &graph_widgetbehavior);
    class_addmethod(canvas_class, (t_method)graph_bounds, gensym("bounds"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(canvas_class, (t_method)graph_xticks, gensym("xticks"),
        A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(canvas_class, (t_method)graph_xlabel, gensym("xlabel"),
        A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)graph_yticks, gensym("yticks"),
        A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(canvas_class, (t_method)graph_ylabel, gensym("ylabel"),
        A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)graph_array, gensym("array"),
        A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_menuarray,
        gensym("menuarray"), A_NULL);
    class_addmethod(canvas_class, (t_method)glist_sort,
        gensym("sort"), A_NULL);
}
