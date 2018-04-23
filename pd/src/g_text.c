/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* changes by Thomas Musil IEM KUG Graz Austria 2001 */
/* the methods for calling the gui-objects from menu are implemented */
/* all changes are labeled with      iemlib      */

#include <stdlib.h>
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include "g_canvas.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "g_undo.h"
#include "x_preset.h"

#include "s_utf8.h"

t_class *text_class;
t_class *message_class;
static t_class *gatom_class;
static t_class *dropdown_class;
static void text_vis(t_gobj *z, t_glist *glist, int vis);
static void text_displace(t_gobj *z, t_glist *glist,
    int dx, int dy);
static void text_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2);
void canvas_howputnew(t_canvas *x, int *connectp, int *xpixp, int *ypixp,
    int *indexp, int *totalp);

void canvas_startmotion(t_canvas *x);
t_widgetbehavior text_widgetbehavior;

extern void canvas_displaceselection(t_canvas *x, int dx, int dy);
extern void canvas_apply_setundo(t_canvas *x, t_gobj *y);
extern void canvas_setundo(t_canvas *x, t_undofn undofn, void *buf,
    const char *name);
extern void *canvas_undo_set_create(t_canvas *x);
extern void canvas_undo_create(t_canvas *x, void *z, int action);
extern int we_are_undoing;
extern void glob_preset_node_list_check_loc_and_update(void);
extern void glob_preset_node_list_seek_hub(void);

/* ----------------- the "text" object.  ------------------ */

    /* add a "text" object (comment) to a glist.  While this one goes for any
    glist, the other 3 below are for canvases only.  (why?)  This is called
    without args if invoked from the GUI; otherwise at least x and y
    are provided.  */

void glist_text(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    if (canvas_hasarray(gl)) return;
    t_text *x = (t_text *)pd_new(text_class);
    t_atom at;
    x->te_width = 0;                            /* don't know it yet. */
    x->te_type = T_TEXT;
    x->te_iemgui = 0;
    x->te_binbuf = binbuf_new();
    if (argc > 1)
    {
        x->te_xpix = atom_getfloatarg(0, argc, argv);
        x->te_ypix = atom_getfloatarg(1, argc, argv);
        if (argc > 2)
        {
            binbuf_restore(x->te_binbuf, argc-2, argv+2);
        }
        else
        {
            SETSYMBOL(&at, gensym("comment"));
            binbuf_restore(x->te_binbuf, 1, &at);
        }
        glist_add(gl, &x->te_g);
    }
    else
    {
        //int xpix, ypix;
        int connectme, xpix, ypix, indx, nobj;
        canvas_howputnew(gl, &connectme, &xpix, &ypix, &indx, &nobj);
        pd_vmess((t_pd *)glist_getcanvas(gl), gensym("editmode"), "i", 1);
        SETSYMBOL(&at, gensym("comment"));
        glist_noselect(gl);
        //glist_getnextxy(gl, &xpix, &ypix);
        x->te_xpix = xpix;
        x->te_ypix = ypix;
        binbuf_restore(x->te_binbuf, 1, &at);
        glist_add(gl, &x->te_g);
        glist_noselect(gl);
        glist_select(gl, &x->te_g);
            /* it would be nice to "activate" here, but then the second,
            "put-me-down" click changes the text selection, which is quite
            irritating, so I took this back out.  It's OK in messages
            and objects though since there's no text in them at menu
            creation. */
            /* gobj_activate(&x->te_g, gl, 1); */
        if (!we_are_undoing)
            canvas_undo_add(glist_getcanvas(gl), 9, "create",
                (void *)canvas_undo_set_create(glist_getcanvas(gl)));
        if (connectme == 0)
        {
            canvas_displaceselection(glist_getcanvas(gl), -8, -8);
            canvas_startmotion(glist_getcanvas(gl));
        }
    }
    glob_preset_node_list_seek_hub();
    glob_preset_node_list_check_loc_and_update();
}

/* ----------------- the "object" object.  ------------------ */

extern t_pd *newest;
int scalar_in_a_box;
extern void glist_scalar(t_glist *canvas, t_symbol *s, int argc, t_atom *argv);
void canvas_getargs(int *argcp, t_atom **argvp);

static void canvas_objtext(t_glist *gl, int xpix, int ypix,
    int width, int selected, t_binbuf *b, int connectme)
{
    //fprintf(stderr,"canvas_objtext\n");
    t_text *x;
    int argc;
    t_atom *argv;

    // for hiding arguments
    t_atom *vec;
    int len, i, hidden;
    t_binbuf *hide;

    newest = 0;
    canvas_setcurrent((t_canvas *)gl);
    canvas_getargs(&argc, &argv);
    binbuf_eval(b, &pd_objectmaker, argc, argv);
    if (binbuf_getnatom(b))
    {
        if (!newest)
        {
            /* let's see if there's a scalar by this name... */
            t_atom *scalar_at = binbuf_getvec(b);
            if (scalar_at->a_type == A_SYMBOL)
            {
                t_symbol *templatesym = 
                    canvas_makebindsym(atom_getsymbol(scalar_at));
                t_template *tmpl = template_findbyname(templatesym);
                if (template_findbyname(templatesym) &&
                    template_cancreate(tmpl) &&
                    template_hasxy(tmpl))
                {
                    //post("Hmm, found a scalar from struct %s... ",
                    //    templatesym->s_name);
                    t_binbuf *scalarbuf = binbuf_new();
                    t_atom coords_at[2];
                    SETFLOAT(coords_at, (t_float)xpix);
                    SETFLOAT(coords_at+1, (t_float)ypix);
                    binbuf_add(scalarbuf, 1, scalar_at);
                    binbuf_add(scalarbuf, 2, coords_at);
                    binbuf_add(scalarbuf, binbuf_getnatom(b)-1, scalar_at+1);
                    t_atom *scalar_create_at = binbuf_getvec(scalarbuf);
                    glist_scalar(gl, gensym("scalar_from_canvas_objtext"),
                        binbuf_getnatom(b)+2, scalar_create_at);
                    binbuf_free(scalarbuf);
                    //binbuf_free(b);
                    canvas_unsetcurrent((t_canvas *)gl);
                    scalar_in_a_box = 1;
                    return;
                }
            }
            x = 0;
        }
        else if (!(x = pd_checkobject(newest)))
        {
            binbuf_print(b);
            post("... didn't return a patchable object");
        }
    }
    else x = 0;
    if (!x)
    {
        /* LATER make the color reflect this */
        x = (t_text *)pd_new(text_class);
        if (binbuf_getnatom(b))
        {
            int bufsize;
            char *buf;
            binbuf_gettext(b, &buf, &bufsize);
            buf = t_resizebytes(buf, bufsize, bufsize+1);
            buf[bufsize] = 0;
            pd_error(x, "couldn't create \"%s\"", buf);
            t_freebytes(buf, bufsize + 1);
        }
    }
    /* special case: an object, like preset_hub, hides its arguments
       beyond the first n, so we modify its binbuf here */
    vec = binbuf_getvec(b);
    len = binbuf_getnatom(b);
    hidden = 0;
    for (i = 0; i < len; i++)
    {
        if (!strcmp("%hidden%", atom_getsymbol(&vec[i])->s_name))
        {
            //fprintf(stderr,"found hidden %d %s\n",
            //    i, atom_getsymbol(&vec[i])->s_name);
            hidden = i;
            break;
        }
    }
    if (hidden)
    {
        hide = binbuf_new();
        binbuf_add(hide, hidden, vec);
        binbuf_free(b);
        b = hide;
    }
    /* done special case */

    x->te_binbuf = b;

    x->te_xpix = xpix;
    x->te_ypix = ypix;
    x->te_width = width;
    x->te_type = T_OBJECT;
    /* let's see if iemgui objects did not already set the value to 1,
       otherwise set it explicitly to 0 */
    if (x->te_iemgui != 1)
        x->te_iemgui = 0;
    glist_add(gl, &x->te_g);

    if (selected)
    {
            /* this is called if we've been created from the menu. we use
               connectme to be able to tell the GUI the difference between
               a newly created floating object and an autopatched one. */
        glist_select(gl, &x->te_g);
        gobj_activate(&x->te_g, gl,
            connectme ? 1 : 2); // <-- hack to set floating mode for new obj box
    }
    if (pd_class(&x->ob_pd) == vinlet_class)
        canvas_resortinlets(glist_getcanvas(gl));
    if (pd_class(&x->ob_pd) == voutlet_class)
        canvas_resortoutlets(glist_getcanvas(gl));
    canvas_unsetcurrent((t_canvas *)gl);

    glob_preset_node_list_seek_hub();
    glob_preset_node_list_check_loc_and_update();

    // here we recreate data buffer inside previously created undo snapshot
    //canvas_undo_create(glist_getcanvas(gl),
    //    glist_getcanvas(gl)->u_last->data, UNDO_FREE);
    //glist_getcanvas(gl)->u_last->data =
    //    canvas_undo_set_create(glist_getcanvas(gl));
    /*if (binbuf_getnatom(x->te_binbuf) && !we_are_undoing) {
        fprintf(stderr,"canvas_objtext calls create undo\n");
        //glist_select(gl, &x->te_g);
        canvas_undo_add(glist_getcanvas(gl), 9, "create",
            (void *)canvas_undo_set_create(glist_getcanvas(gl)));
    }*/
    if ( glist_isvisible( ((t_canvas *)gl) ) )
    {
        canvas_getscroll(glist_getcanvas(gl));
    }
}

extern int sys_noautopatch;
extern t_gobj *glist_nth(t_glist *x, int n);
extern int glist_getindex(t_glist *x, t_gobj *y);
    /* utility routine to figure out where to put a new text box from menu
    and whether to connect to it automatically */
void canvas_howputnew(t_canvas *x, int *connectp, int *xpixp, int *ypixp,
    int *indexp, int *totalp)
{
    int indx = 0, nobj = 0, n2, x1, x2, y1, y2;

    /*
    int connectme = 0;
    if (x->gl_editor->e_selection &&
        !x->gl_editor->e_selection->sel_next &&
        !sys_noautopatch)
    {
        selected = x->gl_editor->e_selection->sel_what;
        t_object *ob = pd_checkobject(&selected->g_pd);
        connectme = (obj_noutlets(ob) ? 1 : 0);
    }*/
    int connectme = (x->gl_editor->e_selection &&
        !x->gl_editor->e_selection->sel_next);
    if (connectme)
    {
        t_gobj *g, *selected = x->gl_editor->e_selection->sel_what;
        t_text *t = (t_text *)selected;
        // if selected object has not yet been activated we need to recreate it first
        if (pd_class(&t->te_pd) == text_class && t->te_type != T_TEXT)
        {
            glist_noselect(x); // we do this to explicitly activate object
            glist_select(x, glist_nth(x, glist_getindex(x, 0)-1)); // then reselect it
            selected = x->gl_editor->e_selection->sel_what;
        }
        for (g = x->gl_list, nobj = 0; g; g = g->g_next, nobj++)
            if (g == selected)
            {
                gobj_getrect(g, x, &x1, &y1, &x2, &y2);
                indx = nobj;
                *xpixp = x1;
                *ypixp = y2 + 5;
            }
        glist_noselect(x);
            /* search back for 'selected' and if it isn't on the list, 
                plan just to connect from the last item on the list. */
        for (g = x->gl_list, n2 = 0; g; g = g->g_next, n2++)
        {
            if (g == selected)
            {
                indx = n2;
                break;
            }
            else if (!g->g_next)
                indx = nobj-1;
        }
        x->gl_editor->e_onmotion = MA_NONE;
        canvas_getscroll(x);
    }
    else
    {
        glist_getnextxy(x, xpixp, ypixp);
        glist_noselect(x);
    }
    if (sys_noautopatch)
    {
        if (connectme == 1) connectme = -1;
        else connectme = 0;
    }
    *connectp = connectme;
    *indexp = indx;
    *totalp = nobj;
}

    /* object creation routine.  These are called without any arguments if
    they're invoked from the gui; when pasting or restoring from a file, we
    get at least x and y. */

EXTERN int connect_exception;

void canvas_obj(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    //fprintf(stderr,"canvas_obj\n");
    if (canvas_hasarray(gl)) return;
    if (argc >= 2)
    {
        t_binbuf *b = binbuf_new();
        binbuf_restore(b, argc-2, argv+2);
        canvas_objtext(gl, atom_getintarg(0, argc, argv),
            atom_getintarg(1, argc, argv), 0, 0, b, 0);
    }
        /* JMZ: don't go into interactive mode in a closed canvas */
    else if (!glist_isvisible(gl))
        post("unable to create stub object in closed canvas!");
    else
    {
            /* interactively create new obect */
        t_binbuf *b = binbuf_new();
        int connectme, xpix, ypix, indx, nobj;
        canvas_howputnew(gl, &connectme, &xpix, &ypix, &indx, &nobj);
        pd_vmess(&gl->gl_pd, gensym("editmode"), "i", 1);
        canvas_objtext(gl,
            connectme ? xpix : xpix - 8,
            connectme ? ypix : ypix - 8,
            0, 1, b, connectme);
        if (connectme == 1)
        {
            //fprintf(stderr,"canvas_obj calls canvas_connect\n");
            connect_exception = 1;
            canvas_connect(gl, indx, 0, nobj, 0);
            connect_exception = 0;
        }
        else if (connectme == 0)
        {
            //fprintf(stderr,"canvas_obj calls canvas_startmotion\n");
            //canvas_displaceselection(glist_getcanvas(gl), -8, -8);
            canvas_startmotion(glist_getcanvas(gl));
        }
        //canvas_setundo(glist_getcanvas(gl),
        //    canvas_undo_create, canvas_undo_set_create(gl), "create");
        if (!we_are_undoing)
            canvas_undo_add(glist_getcanvas(gl), 9, "create",
                (void *)canvas_undo_set_create(glist_getcanvas(gl)));
    }
}

extern void glist_setlastxy(t_glist *gl, int xval, int yval);

/* invoked from tcl/tk: abstraction_name x_offset y_offset */
void canvas_obj_abstraction_from_menu(t_glist *gl, t_symbol *s,
    int argc, t_atom *argv)
{
    if (canvas_hasarray(gl)) return;
    //fprintf(stderr,"canvas_abstraction_from_menu\n");
    //t_text *x;
    t_gobj *y;

    t_binbuf *b = binbuf_new();
    binbuf_restore(b, 2, argv);
    int connectme, xpix, ypix, indx, nobj;
    canvas_howputnew(gl, &connectme, &xpix, &ypix, &indx, &nobj);
    pd_vmess(&gl->gl_pd, gensym("editmode"), "i", 1);
#ifdef PDL2ORK
    if (sys_k12_mode)
        pd_vmess (&gl->gl_pd, gensym("tooltips"), "i", 1);
#endif
    canvas_objtext(gl, xpix+atom_getintarg(1, argc, argv),
        ypix+atom_getintarg(2, argc, argv), 0, 1, b, 0);

    // the object is now the last on the glist so we locate it
    // and send it loadbang
    // we know we have at least one object since we just created one
    // so we don't check for y being valid
    y = gl->gl_list;
    while (y->g_next)
        y = y->g_next;
    canvas_loadbang((t_canvas *)y);

    if (connectme == 1)
    {
        canvas_connect(gl, indx, 0, nobj, 0);
    }
    else if (connectme == 0)
    {
        //glist_setlastxy(glist_getcanvas(gl), xpix, ypix);
        canvas_startmotion(glist_getcanvas(gl));
    }
    canvas_undo_add(glist_getcanvas(gl), 9, "create",
        (void *)canvas_undo_set_create(glist_getcanvas(gl)));
}

/* make an object box for an object that's already there. */

/* iemlib */
void canvas_iemguis(t_glist *gl, t_symbol *guiobjname)
{
    if (canvas_hasarray(gl)) return;
    //fprintf(stderr,"canvas_iemguis\n");
    t_atom at;
    t_binbuf *b = binbuf_new();
    //int xpix, ypix;

    if(!strcmp(guiobjname->s_name, "cnv"))
        glist_noselect(gl);

    int connectme, xpix, ypix, indx, nobj;

    canvas_howputnew(gl, &connectme, &xpix, &ypix, &indx, &nobj);

    /* NOT NECESSARY ANY MORE: compensate for the iemgui sliders' xyoffset
       in case of autopatch
    if (connectme)
    {
        if(!strcmp(guiobjname->s_name, "hsl"))
            xpix = xpix + 3;
        else if(!strcmp(guiobjname->s_name, "vsl"))
            ypix = ypix + 2;
        else if(!strcmp(guiobjname->s_name, "vu"))
        {
            xpix = xpix + 1;
            ypix = ypix + 2;
        }
    }*/
    
    pd_vmess(&gl->gl_pd, gensym("editmode"), "i", 1);
    glist_noselect(gl);
    SETSYMBOL(&at, guiobjname);
    binbuf_restore(b, 1, &at);
    canvas_objtext(gl, xpix, ypix, 0, 1, b, 0);
    if (connectme == 1)
        canvas_connect(gl, indx, 0, nobj, 0);
    //glist_getnextxy(gl, &xpix, &ypix);
    //canvas_objtext(gl, xpix, ypix, 1, b, 0);
    else if (connectme == 0)
    {
        canvas_displaceselection(glist_getcanvas(gl), -8, -8);
        canvas_startmotion(glist_getcanvas(gl));
    }
    //canvas_setundo(glist_getcanvas(gl),
    //    canvas_undo_create, canvas_undo_set_create(gl), "create");
    canvas_undo_add(glist_getcanvas(gl), 9, "create",
        (void *)canvas_undo_set_create(glist_getcanvas(gl)));
}

void canvas_bng(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("bng"));
}

void canvas_toggle(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("tgl"));
}

void canvas_vslider(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("vsl"));
}

void canvas_hslider(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("hsl"));
}

void canvas_hdial(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("hdl"));
}

void canvas_vdial(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("vdl"));
}

void canvas_hradio(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("hradio"));
}

void canvas_vradio(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("vradio"));
}

void canvas_vumeter(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("vu"));
}

void canvas_mycnv(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("cnv"));
}

void canvas_numbox(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("nbx"));
}

/* iemlib */

void canvas_objfor(t_glist *gl, t_text *x, int argc, t_atom *argv)
{
    x->te_width = 0;                            /* don't know it yet. */
    x->te_type = T_OBJECT;
    x->te_binbuf = binbuf_new();
    x->te_xpix = atom_getfloatarg(0, argc, argv);
    x->te_ypix = atom_getfloatarg(1, argc, argv);
    if (argc > 2) binbuf_restore(x->te_binbuf, argc-2, argv+2);
    glist_add(gl, &x->te_g);
}

/* ---------------------- the "message" text item ------------------------ */

typedef struct _messresponder
{
    t_pd mr_pd;
    t_glist *mr_glist;
    t_outlet *mr_outlet;
} t_messresponder;

typedef struct _message
{
    t_text m_text;
    t_messresponder m_messresponder;
    t_glist *m_glist;
    t_clock *m_clock;
} t_message;

t_class *messresponder_class;

static void messresponder_bang(t_messresponder *x)
{
    outlet_bang(x->mr_outlet);
}

static void messresponder_float(t_messresponder *x, t_float f)
{
    outlet_float(x->mr_outlet, f);
}

static void messresponder_symbol(t_messresponder *x, t_symbol *s)
{
    outlet_symbol(x->mr_outlet, s);
}

/* MP 20070107 blob type */
//static void messresponder_blob(t_messresponder *x, t_blob *st)
//{
//    outlet_blob(x->mr_outlet, st);
//}

static void messresponder_list(t_messresponder *x, 
    t_symbol *s, int argc, t_atom *argv)
{
    outlet_list(x->mr_outlet, s, argc, argv);
}

static void messresponder_anything(t_messresponder *x,
    t_symbol *s, int argc, t_atom *argv)
{
    outlet_anything(x->mr_outlet, s, argc, argv);
}

/* get the glist cached in a message responder */
t_glist *messresponder_getglist(t_pd *x)
{
    return ((t_messresponder *)x)->mr_glist;
}

static void message_bang(t_message *x)
{
    /*  we do canvas_setcurrent/unsetcurrent to substitute canvas
        instance number for $0 */
    binbuf_eval(x->m_text.te_binbuf, &x->m_messresponder.mr_pd, 0, 0);
}

static void message_float(t_message *x, t_float f)
{
    t_atom at;
    SETFLOAT(&at, f);
    /*  we do canvas_setcurrent/unsetcurrent to substitute canvas
        instance number for $0 */
    binbuf_eval(x->m_text.te_binbuf, &x->m_messresponder.mr_pd, 1, &at);
}

static void message_symbol(t_message *x, t_symbol *s)
{
    t_atom at;
    SETSYMBOL(&at, s);
    /*  we do canvas_setcurrent/unsetcurrent to substitute canvas
        instance number for $0 */
    binbuf_eval(x->m_text.te_binbuf, &x->m_messresponder.mr_pd, 1, &at);
}

static void message_blob(t_message *x, t_blob *st)
{
    t_atom at;
    SETBLOB(&at, st);
    /*  we do canvas_setcurrent/unsetcurrent to substitute canvas
        instance number for $0 */
    binbuf_eval(x->m_text.te_binbuf, &x->m_messresponder.mr_pd, 1, &at);
}

static void message_list(t_message *x, t_symbol *s, int argc, t_atom *argv)
{
    // TODO: here and elsewhere in the message, do we want $0 to be parsed
    // into canvas instance? Makes sense since there is no such argument,
    // but will this break anything?
    /*  we do canvas_setcurrent/unsetcurrent to substitute canvas
        instance number for $0 */
    binbuf_eval(x->m_text.te_binbuf, &x->m_messresponder.mr_pd, argc, argv);
}

static void message_set(t_message *x, t_symbol *s, int argc, t_atom *argv)
{
    binbuf_clear(x->m_text.te_binbuf);
    binbuf_add(x->m_text.te_binbuf, argc, argv);
    glist_retext(x->m_glist, &x->m_text);
    if (glist_isvisible(glist_getcanvas(x->m_glist)))
        canvas_getscroll(x->m_glist);
}

static void message_add2(t_message *x, t_symbol *s, int argc, t_atom *argv)
{
    binbuf_add(x->m_text.te_binbuf, argc, argv);
    glist_retext(x->m_glist, &x->m_text);
    if (glist_isvisible(glist_getcanvas(x->m_glist)))
        canvas_getscroll(x->m_glist);
}

static void message_add(t_message *x, t_symbol *s, int argc, t_atom *argv)
{
    binbuf_add(x->m_text.te_binbuf, argc, argv);
    binbuf_addsemi(x->m_text.te_binbuf);
    glist_retext(x->m_glist, &x->m_text);
    if (glist_isvisible(glist_getcanvas(x->m_glist)))
        canvas_getscroll(x->m_glist);
}

static void message_addcomma(t_message *x)
{
    t_atom a;
    SETCOMMA(&a);
    binbuf_add(x->m_text.te_binbuf, 1, &a);
    glist_retext(x->m_glist, &x->m_text);
    if (glist_isvisible(glist_getcanvas(x->m_glist)))
        canvas_getscroll(x->m_glist);
}

static void message_addsemi(t_message *x)
{
    message_add(x, 0, 0, 0);
    if (glist_isvisible(glist_getcanvas(x->m_glist)))
        canvas_getscroll(x->m_glist);
}

static void message_adddollar(t_message *x, t_floatarg f)
{
    t_atom a;
    int n = f;
    if (n < 0)
        n = 0;
    SETDOLLAR(&a, n);
    binbuf_add(x->m_text.te_binbuf, 1, &a);
    glist_retext(x->m_glist, &x->m_text);
    if (glist_isvisible(glist_getcanvas(x->m_glist)))
        canvas_getscroll(x->m_glist);
}

static void message_adddollsym(t_message *x, t_symbol *s)
{
    t_atom a;
    char buf[MAXPDSTRING];
    buf[0] = '$';
    strncpy(buf+1, s->s_name, MAXPDSTRING-2);
    buf[MAXPDSTRING-1] = 0;
    SETDOLLSYM(&a, gensym(buf));
    binbuf_add(x->m_text.te_binbuf, 1, &a);
    glist_retext(x->m_glist, &x->m_text);
    if (glist_isvisible(glist_getcanvas(x->m_glist)))
        canvas_getscroll(x->m_glist);
}

static void message_click(t_message *x,
    t_floatarg xpos, t_floatarg ypos, t_floatarg shift,
        t_floatarg ctrl, t_floatarg alt)
{
    message_float(x, 0);
    if (glist_isvisible(x->m_glist))
    {
        t_rtext *y = glist_findrtext(x->m_glist, &x->m_text);
        gui_vmess("gui_message_flash", "xsi",
            glist_getcanvas(x->m_glist), rtext_gettag(y), 1);
        clock_delay(x->m_clock, 120);
    }
}

static void message_tick(t_message *x)
{
    if (glist_isvisible(x->m_glist))
    {
        t_rtext *y = glist_findrtext(x->m_glist, &x->m_text);
        gui_vmess("gui_message_flash", "xsi",
            glist_getcanvas(x->m_glist), rtext_gettag(y), 0);
    }
}

static void message_free(t_message *x)
{
    clock_free(x->m_clock);
}

t_pd *pd_mess_from_responder(t_pd *x)
{
    if (pd_class(x) == messresponder_class)
    {
        /* do pointer math to try to get to the container message struct */
        void *tmp = (void *)x - sizeof(t_text);
        /* if it looks like a message, it must be a message */
        if(((t_text *)tmp)->te_pd == message_class)
            return ((t_pd *)tmp);
    }
    return x;
}

void canvas_msg(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    if (canvas_hasarray(gl)) return;
    /*fprintf(stderr,"canvas_msg\n");
    int i = 0;
    while(i < argc)
    {
        if (argv[i].a_type == A_FLOAT)
            fprintf(stderr," %f", atom_getfloatarg(i, argc, argv));
        else
            fprintf(stderr," %s", atom_getsymbolarg(i, argc, argv)->s_name);
        i++;
    }
    fprintf(stderr,"\n");*/
    t_message *x = (t_message *)pd_new(message_class);
    x->m_messresponder.mr_pd = messresponder_class;
    x->m_messresponder.mr_outlet = outlet_new(&x->m_text, &s_float);
    x->m_messresponder.mr_glist = gl;
    x->m_text.te_width = 0;                             /* don't know it yet. */
    x->m_text.te_type = T_MESSAGE;
    x->m_text.te_iemgui = 0;
    x->m_text.te_binbuf = binbuf_new();
    x->m_glist = gl;
    x->m_clock = clock_new(x, (t_method)message_tick);
    if (argc > 1)
    {
        x->m_text.te_xpix = atom_getfloatarg(0, argc, argv);
        x->m_text.te_ypix = atom_getfloatarg(1, argc, argv);
        if (argc > 2) binbuf_restore(x->m_text.te_binbuf, argc-2, argv+2);
        glist_add(gl, &x->m_text.te_g);
    }
    else if (!glist_isvisible(gl))
        post("unable to create stub message in closed canvas!");
    else
    {
        int connectme, xpix, ypix, indx, nobj;
        canvas_howputnew(gl, &connectme, &xpix, &ypix, &indx, &nobj);
        
        pd_vmess(&gl->gl_pd, gensym("editmode"), "i", 1);
        x->m_text.te_xpix = xpix;
        x->m_text.te_ypix = ypix;
        glist_add(gl, &x->m_text.te_g);
        glist_noselect(gl);
        glist_select(gl, &x->m_text.te_g);
        gobj_activate(&x->m_text.te_g, gl,
            connectme ? 1 : 2); // <-- hack to signal we're a new message box
        if (connectme == 1)
            canvas_connect(gl, indx, 0, nobj, 0);
        else if (connectme == 0)
        {
            canvas_displaceselection(glist_getcanvas(gl), -8, -8);
            canvas_startmotion(glist_getcanvas(gl));
        }
        //canvas_setundo(glist_getcanvas(gl),
        //    canvas_undo_create, canvas_undo_set_create(gl), "create");
        canvas_undo_add(glist_getcanvas(gl), 9, "create",
            (void *)canvas_undo_set_create(glist_getcanvas(gl)));
    }
}

/* ---------------------- the "atom" text item ------------------------ */

#define ATOMBUFSIZE 40
#define ATOM_LABELLEFT 0
#define ATOM_LABELRIGHT 1
#define ATOM_LABELUP 2
#define ATOM_LABELDOWN 3

typedef struct _gatom
{
    t_text a_text;
    t_atom a_atom;          /* this holds the value and the type */
    t_glist *a_glist;       /* owning glist */
    t_float a_toggle;       /* value to toggle to */
    t_float a_draghi;       /* high end of drag range */
    t_float a_draglo;       /* low end of drag range */
    t_symbol *a_label;      /* symbol to show as label next to box */
    t_symbol *a_symfrom;    /* "receive" name -- bind ourselvs to this */
    t_symbol *a_symto;      /* "send" name -- send to this on output */
    char a_buf[ATOMBUFSIZE];/* string buffer for typing */
    char a_shift;           /* was shift key down when dragging started? */
    char a_wherelabel;      /* 0-3 for left, right, above, below */
    t_symbol *a_expanded_to; /* a_symto after $0, $1, ...  expansion */
} t_gatom;

    /* prepend "-" as necessary to avoid empty strings, so we can
    use them in Pd messages.  A more complete solution would be
    to introduce some quoting mechanism; but then we'd be much more
    complicated. */
static t_symbol *gatom_escapit(t_symbol *s)
{
    if (!*s->s_name)
        return (gensym("-"));
    else if (*s->s_name == '-')
    {
        char shmo[100];
        shmo[0] = '-';
        strncpy(shmo+1, s->s_name, 99);
        shmo[99] = 0;
        return (gensym(shmo));
    }
    else return (iemgui_dollar2raute(s));
}

    /* undo previous operation: strip leading "-" if found. */
static t_symbol *gatom_unescapit(t_symbol *s)
{
    if (*s->s_name == '-')
        return (gensym(s->s_name+1));
    else return (iemgui_raute2dollar(s));
}

static void gatom_redraw(t_gobj *client, t_glist *glist)
{
    t_gatom *x = (t_gatom *)client;
    glist_retext(x->a_glist, &x->a_text);
}

    /* recolor option offers     0 ignore recolor
                                1 recolor */
static void gatom_retext(t_gatom *x, int senditup, int recolor)
{
    t_canvas *canvas = glist_getcanvas(x->a_glist);
    t_rtext *y = glist_findrtext(x->a_glist, &x->a_text);
    if (recolor)
    {
        gui_vmess("gui_gatom_activate", "xsi",
            canvas, rtext_gettag(y), 0);
    }
    binbuf_clear(x->a_text.te_binbuf);
    binbuf_add(x->a_text.te_binbuf, 1, &x->a_atom);
    if (senditup && glist_isvisible(x->a_glist))
        sys_queuegui(x, x->a_glist, gatom_redraw);
}

static void gatom_set(t_gatom *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom oldatom = x->a_atom;
    int changed = 0;
    if (!argc) return;
    if (x->a_atom.a_type == A_FLOAT)
        x->a_atom.a_w.w_float = atom_getfloat(argv),
            changed = (x->a_atom.a_w.w_float != oldatom.a_w.w_float);
    else if (x->a_atom.a_type == A_SYMBOL)
        x->a_atom.a_w.w_symbol = atom_getsymbol(argv),
            changed = (x->a_atom.a_w.w_symbol != oldatom.a_w.w_symbol);
    if (changed)
    {
        if (x->a_atom.a_type == A_FLOAT && x->a_text.te_width == 1)
            gatom_retext(x, 1, 1);
        else
            gatom_retext(x, 1, 0);
    }
    x->a_buf[0] = 0;
}

static void gatom_bang(t_gatom *x)
{
    if (x->a_atom.a_type == A_FLOAT)
    {
        if (x->a_text.te_outlet)
            outlet_float(x->a_text.te_outlet, x->a_atom.a_w.w_float);
        if (*x->a_expanded_to->s_name && x->a_expanded_to->s_thing)
        {
            if (x->a_symto == x->a_symfrom)
                pd_error(x,
                    "%s: atom with same send/receive name (infinite loop)",
                        x->a_symto->s_name);
            else pd_float(x->a_expanded_to->s_thing, x->a_atom.a_w.w_float);
        }
    }
    else if (x->a_atom.a_type == A_SYMBOL)
    {
        if (x->a_text.te_outlet)
            outlet_symbol(x->a_text.te_outlet, x->a_atom.a_w.w_symbol);
        if (*x->a_symto->s_name && x->a_expanded_to->s_thing)
        {
            if (x->a_symto == x->a_symfrom)
                pd_error(x,
                    "%s: atom with same send/receive name (infinite loop)",
                        x->a_symto->s_name);
            else pd_symbol(x->a_expanded_to->s_thing, x->a_atom.a_w.w_symbol);
        }
    }
}

static void gatom_float(t_gatom *x, t_float f)
{
    t_atom at;
    SETFLOAT(&at, f);
    gatom_set(x, 0, 1, &at);
    gatom_bang(x);
}

static void gatom_clipfloat(t_gatom *x, t_float f)
{
    if (x->a_draglo != 0 || x->a_draghi != 0)
    {
        if (f < x->a_draglo)
            f = x->a_draglo;
        if (f > x->a_draghi)
            f = x->a_draghi;
    }
    gatom_float(x, f);
}

static void gatom_symbol(t_gatom *x, t_symbol *s)
{
    t_atom at;
    SETSYMBOL(&at, s);
    gatom_set(x, 0, 1, &at);
    gatom_bang(x);
}

    /* We need a list method because, since there's both an "inlet" and a
    "nofirstin" flag, the standard list behavior gets confused. */
static void gatom_list(t_gatom *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!argc)
        gatom_bang(x);
    else if (argv->a_type == A_FLOAT)
        gatom_float(x, argv->a_w.w_float);
    else if (argv->a_type == A_SYMBOL)
        gatom_symbol(x, argv->a_w.w_symbol);
    else pd_error(x, "gatom_list: need float or symbol");
}

static void gatom_motion(void *z, t_floatarg dx, t_floatarg dy)
{
    t_gatom *x = (t_gatom *)z;
    if (dy == 0) return;
    if (x->a_atom.a_type == A_FLOAT)
    {
        if (x->a_shift)
        {
            double nval = x->a_atom.a_w.w_float - 0.01 * dy;
            double trunc = 0.01 * (floor(100. * nval + 0.5));
            if (trunc < nval + 0.0001 && trunc > nval - 0.0001) nval = trunc;
            gatom_clipfloat(x, nval);
        }
        else
        {
            double nval = x->a_atom.a_w.w_float - dy;
            double trunc = 0.01 * (floor(100. * nval + 0.5));
            if (trunc < nval + 0.0001 && trunc > nval - 0.0001) nval = trunc;
            trunc = floor(nval + 0.5);
            if (trunc < nval + 0.001 && trunc > nval - 0.001) nval = trunc;
            gatom_clipfloat(x, nval);
        }
    }
}

static void gatom_key(void *z, t_floatarg f)
{
    //fprintf(stderr,"gatom_key %f\n", f);
    t_gatom *x = (t_gatom *)z;
    int c = f;
    int len = strlen(x->a_buf);
    t_atom at;
    char sbuf[ATOMBUFSIZE + 4];
    if (c == 0)
    {
        /* we're being notified that no more keys will come for this grab */
        if (x->a_buf[0])
            gatom_retext(x, 1, 1);
        else
            gatom_retext(x, 0, 1);
        return;
    }
    else if (c == '\b')
    {
        if (len > 0)
        x->a_buf[len-1] = 0;
        goto redraw;
    }
    else if (c == '\n')
    {
        if (x->a_atom.a_type == A_FLOAT)
            x->a_atom.a_w.w_float = atof(x->a_buf);
        else if (x->a_atom.a_type == A_SYMBOL)
            x->a_atom.a_w.w_symbol = gensym(x->a_buf);
        else bug("gatom_key");
        gatom_bang(x);
        gatom_retext(x, 1, 0);
        x->a_buf[0] = 0;
        /* We want to keep grabbing the keyboard after hitting "Enter", so
           we're commenting the following out */
        //glist_grab(x->a_glist, 0, 0, 0, 0, 0);
    }
    else if (len < (ATOMBUFSIZE-1))
    {
            /* for numbers, only let reasonable characters through */
        if ((x->a_atom.a_type == A_SYMBOL) ||
            (c >= '0' && c <= '9' || c == '.' || c == '-'
                || c == 'e' || c == 'E'))
        {
            /* the wchar could expand to up to 4 bytes, which
             * which might overrun our a_buf;
             * therefore we first expand into a temporary buffer, 
             * and only if the resulting utf8 string fits into a_buf
             * we apply it
             */
            char utf8[UTF8_MAXBYTES];
            int utf8len = u8_wc_toutf8(utf8, c);
            if((len+utf8len) < (ATOMBUFSIZE-1))
            {
                int j=0;
                for(j=0; j<utf8len; j++)
                    x->a_buf[len+j] = utf8[j];
                 
                x->a_buf[len+utf8len] = 0;
            }
            goto redraw;
        }
    }
    return;
redraw:
        /* LATER figure out how to avoid creating all these symbols! */
    sprintf(sbuf, "%s...", x->a_buf);
    SETSYMBOL(&at, gensym(sbuf));
    binbuf_clear(x->a_text.te_binbuf);
    binbuf_add(x->a_text.te_binbuf, 1, &at);
    glist_retext(x->a_glist, &x->a_text);
}

static void gatom_click(t_gatom *x,
    t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl,
    t_floatarg alt)
{
    if (x->a_text.te_width == 1)
    {
        if (x->a_atom.a_type == A_FLOAT)
            gatom_float(x, (x->a_atom.a_w.w_float == 0));
    }
    else
    {
        if (alt)
        {
            if (x->a_atom.a_type != A_FLOAT) return;
            if (x->a_atom.a_w.w_float != 0)
            {
                x->a_toggle = x->a_atom.a_w.w_float;
                gatom_float(x, 0);
            }
            else gatom_float(x, x->a_toggle);
            gatom_retext(x, 0, 1);
            return;
        }
        x->a_shift = shift;
        x->a_buf[0] = 0;
        glist_grab(x->a_glist, &x->a_text.te_g, gatom_motion, gatom_key,
            xpos, ypos);
    }
}

EXTERN int glist_getindex(t_glist *x, t_gobj *y);
EXTERN int canvas_apply_restore_original_position(t_canvas *x, int pos);

    /* message back from dialog window */
static void gatom_param(t_gatom *x, t_symbol *sel, int argc, t_atom *argv)
{
    /* Check if we need to set an undo point. This happens if the user
       clicks the "Ok" button, but not when clicking "Apply" or "Cancel" */
    if (atom_getintarg(7, argc, argv))
        canvas_apply_setundo(x->a_glist, (t_gobj *)x);

    t_float width = atom_getfloatarg(0, argc, argv);
    t_float draglo = atom_getfloatarg(1, argc, argv);
    t_float draghi = atom_getfloatarg(2, argc, argv);
    t_symbol *label = gatom_unescapit(atom_getsymbolarg(3, argc, argv));
    t_float wherelabel = atom_getfloatarg(4, argc, argv);
    t_symbol *symfrom = gatom_unescapit(atom_getsymbolarg(5, argc, argv));
    t_symbol *symto = gatom_unescapit(atom_getsymbolarg(6, argc, argv));

    gobj_vis(&x->a_text.te_g, x->a_glist, 0);
    if (!*symfrom->s_name && *x->a_symfrom->s_name)
        inlet_new(&x->a_text, &x->a_text.te_pd, 0, 0);
    else if (*symfrom->s_name && !*x->a_symfrom->s_name && x->a_text.te_inlet)
    {
        canvas_deletelinesforio(x->a_glist, &x->a_text,
            x->a_text.te_inlet, 0);
        inlet_free(x->a_text.te_inlet);
    }
    if (!*symto->s_name && *x->a_symto->s_name)
        outlet_new(&x->a_text, 0);
    else if (*symto->s_name && !*x->a_symto->s_name && x->a_text.te_outlet)
    {
        canvas_deletelinesforio(x->a_glist, &x->a_text,
            0, x->a_text.te_outlet);
        outlet_free(x->a_text.te_outlet);
    }
    if (draglo >= draghi)
        draglo = draghi = 0;
    x->a_draglo = draglo;
    x->a_draghi = draghi;
    if (width < 0)
        width = 4;
    else if (width > 80)
        width = 80;
    x->a_text.te_width = width;
    x->a_wherelabel = ((int)wherelabel & 3);
    x->a_label = label;
    if (*x->a_symfrom->s_name)
        pd_unbind(&x->a_text.te_pd,
            canvas_realizedollar(x->a_glist, x->a_symfrom));
    x->a_symfrom = symfrom;
    if (*x->a_symfrom->s_name)
        pd_bind(&x->a_text.te_pd,
            canvas_realizedollar(x->a_glist, x->a_symfrom));
    x->a_symto = symto;
    x->a_expanded_to = canvas_realizedollar(x->a_glist, x->a_symto);
    gobj_vis(&x->a_text.te_g, x->a_glist, 1);
    gobj_select(&x->a_text.te_g, x->a_glist, 1);
    canvas_dirty(x->a_glist, 1);
    canvas_getscroll(x->a_glist);
    /* glist_retext(x->a_glist, &x->a_text); */
}

    /* ---------------- gatom-specific widget functions --------------- */

static void gatom_getwherelabel(t_gatom *x, t_glist *glist, int *xp, int *yp)
{
    int x1, y1, x2, y2;
    text_getrect(&x->a_text.te_g, glist, &x1, &y1, &x2, &y2);
    if (x->a_wherelabel == ATOM_LABELLEFT)
    {
        *xp = -3 -
            strlen(canvas_realizedollar(x->a_glist, x->a_label)->s_name) *
            sys_fontwidth(glist_getfont(glist));
        *yp = y2 - y1 - 4;
    }
    else if (x->a_wherelabel == ATOM_LABELRIGHT)
    {
        *xp = x2 - x1 + 3;
        *yp = y2 - y1 - 4;
    }
    else if (x->a_wherelabel == ATOM_LABELUP)
    {
        *xp = -1;
        *yp = -3;
    }
    else
    {
        *xp = -1;
        *yp = y2 - y1 + sys_fontheight(glist_getfont(glist));
    }
}

static void gatom_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    text_displace(z, glist, dx, dy);
}

/* for gatom's label */
static void gatom_vis(t_gobj *z, t_glist *glist, int vis)
{
    //fprintf(stderr,"gatom_vis\n");
    t_gatom *x = (t_gatom*)z;
    text_vis(z, glist, vis);
    if (*x->a_label->s_name)
    {
        if (vis)
        {
            int x1, y1;
            t_rtext *y = glist_findrtext(x->a_glist, &x->a_text);
            gatom_getwherelabel(x, glist, &x1, &y1);
            gui_vmess("gui_text_new", "xssiiisi",
                glist_getcanvas(glist),
                rtext_gettag(y),
                "gatom",
                0,
                x1, // left margin
                y1, // top margin
                canvas_realizedollar(x->a_glist, x->a_label)->s_name,
                sys_hostfontsize(glist_getfont(glist))
            );
        }
        else
        {
            /* We're just deleting the parent gobj in the GUI, which takes
               care of removing all the children. So we don't need to send
               a message here */
            //sys_vgui(".x%lx.c delete %lx.l\n", glist_getcanvas(glist), x);
        }
    }
    if (!vis)
        sys_unqueuegui(x);
}

void canvas_atom(t_glist *gl, t_atomtype type,
    t_symbol *s, int argc, t_atom *argv)
{
    if (canvas_hasarray(gl)) return;
    //fprintf(stderr,"canvas_atom\n");
    t_gatom *x = (t_gatom *)pd_new(gatom_class);
    t_atom at;
    x->a_text.te_width = 0;                        /* don't know it yet. */
    x->a_text.te_type = T_ATOM;
    x->a_text.te_iemgui = 0;
    x->a_text.te_binbuf = binbuf_new();
    x->a_glist = gl;
    x->a_atom.a_type = type;
    x->a_toggle = 1;
    x->a_draglo = 0;
    x->a_draghi = 0;
    x->a_wherelabel = 0;
    x->a_label = &s_;
    x->a_symfrom = &s_;
    x->a_symto = x->a_expanded_to = &s_;
    if (type == A_FLOAT)
    {
        x->a_atom.a_w.w_float = 0;
        x->a_text.te_width = 5;
        SETFLOAT(&at, 0);
    }
    else
    {
        x->a_atom.a_w.w_symbol = &s_symbol;
        x->a_text.te_width = 10;
        SETSYMBOL(&at, &s_symbol);
    }
    binbuf_add(x->a_text.te_binbuf, 1, &at);
    if (argc > 1)
        /* create from file. x, y, width, low-range, high-range, flags,
            label, receive-name, send-name */
    {
        x->a_text.te_xpix = atom_getfloatarg(0, argc, argv);
        x->a_text.te_ypix = atom_getfloatarg(1, argc, argv);
        x->a_text.te_width = atom_getintarg(2, argc, argv);
            /* sanity check because some very old patches have trash in this
            field... remove this in 2003 or so: */
        if (x->a_text.te_width < 0 || x->a_text.te_width > 500)
            x->a_text.te_width = 4;
        x->a_draglo = atom_getfloatarg(3, argc, argv);
        x->a_draghi = atom_getfloatarg(4, argc, argv);
        x->a_wherelabel = (((int)atom_getfloatarg(5, argc, argv)) & 3);
        x->a_label = gatom_unescapit(atom_getsymbolarg(6, argc, argv));
        x->a_symfrom = gatom_unescapit(atom_getsymbolarg(7, argc, argv));
        if (*x->a_symfrom->s_name)
            pd_bind(&x->a_text.te_pd,
                canvas_realizedollar(x->a_glist, x->a_symfrom));

        x->a_symto = gatom_unescapit(atom_getsymbolarg(8, argc, argv));
        x->a_expanded_to = canvas_realizedollar(x->a_glist, x->a_symto);
        if (x->a_symto == &s_)
            outlet_new(&x->a_text,
                x->a_atom.a_type == A_FLOAT ? &s_float: &s_symbol);
        if (x->a_symfrom == &s_)
            inlet_new(&x->a_text, &x->a_text.te_pd, 0, 0);
        glist_add(gl, &x->a_text.te_g);
    }
    else
    {
        int connectme, xpix, ypix, indx, nobj;
        canvas_howputnew(gl, &connectme, &xpix, &ypix, &indx, &nobj);
        outlet_new(&x->a_text,
            x->a_atom.a_type == A_FLOAT ? &s_float: &s_symbol);
        inlet_new(&x->a_text, &x->a_text.te_pd, 0, 0);
        pd_vmess(&gl->gl_pd, gensym("editmode"), "i", 1);
        x->a_text.te_xpix = xpix;
        x->a_text.te_ypix = ypix;
        glist_add(gl, &x->a_text.te_g);
        glist_noselect(gl);
        glist_select(gl, &x->a_text.te_g);
        if (connectme == 1)
            canvas_connect(gl, indx, 0, nobj, 0);
        else if (connectme == 0)
        {
            canvas_displaceselection(glist_getcanvas(gl), -8, -8);
            canvas_startmotion(glist_getcanvas(gl));
        }
        //canvas_setundo(glist_getcanvas(gl),
        //    canvas_undo_create, canvas_undo_set_create(gl), "create");
        canvas_undo_add(glist_getcanvas(gl), 9, "create",
            (void *)canvas_undo_set_create(glist_getcanvas(gl)));
    }
    glob_preset_node_list_seek_hub();
    glob_preset_node_list_check_loc_and_update();
}

void canvas_floatatom(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_atom(gl, A_FLOAT, s, argc, argv);
}

void canvas_symbolatom(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_atom(gl, A_SYMBOL, s, argc, argv);
}

static void gatom_free(t_gatom *x)
{
    if (*x->a_symfrom->s_name)
        pd_unbind(&x->a_text.te_pd,
            canvas_realizedollar(x->a_glist, x->a_symfrom));
    gfxstub_deleteforkey(x);
}

static void gatom_properties(t_gobj *z, t_glist *owner)
{
    t_gatom *x = (t_gatom *)z;
    //char buf[200];
    //sprintf(buf, "pdtk_gatom_dialog %%s %d %g %g %d {%s} {%s} {%s}\n",
    //    x->a_text.te_width, x->a_draglo, x->a_draghi,
    //        x->a_wherelabel, gatom_escapit(x->a_label)->s_name,
    //            gatom_escapit(x->a_symfrom)->s_name,
    //                gatom_escapit(x->a_symto)->s_name);
    //gfxstub_new(&x->a_text.te_pd, x, buf);
    gui_start_vmess("gui_gatom_dialog", "s",
        gfxstub_new2(&x->a_text.te_pd, x));
    gui_start_array();
    gui_s("name");     gui_s("atom");
    gui_s("width");    gui_i(x->a_text.te_width);
    gui_s("draglo");   gui_f(x->a_draglo);
    gui_s("draghi");   gui_f(x->a_draghi);
    gui_s("labelpos"); gui_i(x->a_wherelabel);
    gui_s("label");    gui_s(gatom_escapit(x->a_label)->s_name);
    gui_s("receive_symbol");  gui_s(gatom_escapit(x->a_symfrom)->s_name);
    gui_s("send_symbol");     gui_s(gatom_escapit(x->a_symto)->s_name);
    gui_end_array();
    gui_end_vmess();
}

/* ---------------------- the "dropdown" text item ------------------------ */

typedef struct _dropdown
{
    t_text a_text;
    t_binbuf *a_names;      /* names to be displayed */
    int a_maxnamewidth;     /* when width = 0 this is used */
    int a_index;            /* index */
    t_glist *a_glist;       /* owning glist */
    t_float a_dummy;        /* dummy value */
    int a_outtype;           /* 0 = index, 1 = value */
    int a_output;           /* 0 = index, 1 = value */
    t_symbol *a_label;      /* symbol to show as label next to box */
    t_symbol *a_symfrom;    /* "receive" name -- bind ourselvs to this */
    t_symbol *a_symto;      /* "send" name -- send to this on output */
    char a_wherelabel;      /* 0-3 for left, right, above, below */
    t_symbol *a_expanded_to; /* a_symto after $0, $1, ...  expansion */
} t_dropdown;

int is_dropdown(t_text *x)
{
    return (x->te_type == T_ATOM && pd_class(&x->te_pd) == dropdown_class);
}

static void dropdown_redraw(t_gobj *client, t_glist *glist)
{
    t_dropdown *x = (t_dropdown *)client;
    glist_retext(x->a_glist, &x->a_text);
}

    /* recolor option offers    0 ignore recolor
                                1 recolor */
static void dropdown_retext(t_dropdown *x, int senditup, int recolor)
{
    if (recolor)
    {
        /* not sure if we need to activate dropdown */
        //t_rtext *y = glist_findrtext(x->a_glist, &x->a_text);
        //t_canvas *canvas = glist_getcanvas(x->a_glist);
        //gui_vmess("gui_gatom_activate", "xsi",
        //    canvas, rtext_gettag(y), 0);
        post("note: dropdown is being activated!");
    }
    binbuf_clear(x->a_text.te_binbuf);
    binbuf_add(x->a_text.te_binbuf, 1, binbuf_getvec(x->a_names) + x->a_index);

    if (senditup && glist_isvisible(x->a_glist))
        sys_queuegui(x, x->a_glist, dropdown_redraw);
}

/* use this to keep the index within the correct range */
static int dropdown_clipindex(t_dropdown *x, int i)
{
    int ret = i, len = binbuf_getnatom(x->a_names);
    if (ret < 0) ret = 0;
    if (ret > len - 1) ret = len - 1;
    return ret;
}

static void dropdown_set(t_dropdown *x, t_symbol *s, int argc, t_atom *argv)
{
    int oldindex = x->a_index;
    if (!argc) return;
    x->a_index = dropdown_clipindex(x, (int)atom_getfloat(argv));
    if (oldindex != x->a_index)
        dropdown_retext(x, 1, 0);
}

static int dropdown_names_getmaxwidth(t_dropdown *x) {
    char *buf;
    t_binbuf *names = x->a_names, *b = binbuf_new();
    int len = binbuf_getnatom(x->a_names), maxwidth = 0;
    while (len--)
    {
        int width;
        binbuf_clear(b);
        binbuf_add(b, 1, binbuf_getvec(names) + len);
        binbuf_gettext(b, &buf, &width);
        if (width > maxwidth) maxwidth = width;
    }
    return maxwidth;
}

static void dropdown_names(t_dropdown *x, t_symbol *s, int argc, t_atom *argv)
{
    t_rtext *y = glist_findrtext(x->a_glist, &x->a_text);
    binbuf_clear(x->a_names);
    if (argc)
        binbuf_add(x->a_names, argc, argv);
    else
        binbuf_addv(x->a_names, "s", &s_);
    /* nudge a_index back into range */
    x->a_index = dropdown_clipindex(x, x->a_index);
    x->a_maxnamewidth = dropdown_names_getmaxwidth(x);
    //dropdown_max_namelength(x);
    dropdown_retext(x, 1, 0);
    /* Now redraw the border */
    text_drawborder(&x->a_text, x->a_glist, rtext_gettag(y),
        rtext_width(y), rtext_height(y), 0);
}

static void dropdown_bang(t_dropdown *x)
{
    t_atom at;
    if (x->a_outtype == 0)
        SETFLOAT(&at, (t_float)x->a_index);
    else
    {
        t_atom *atfrom = binbuf_getvec(x->a_names) + x->a_index;
        if (atfrom->a_type == A_FLOAT)
            SETFLOAT(&at, atom_getfloat(atfrom));
        else if (atfrom->a_type == A_SYMBOL)
            SETSYMBOL(&at, atom_getsymbol(atfrom));
        else pd_error(x, "only float and symbol output supported");
    }
    if (x->a_text.te_outlet)
        outlet_list(x->a_text.te_outlet, &s_list, 1, &at);
    if (*x->a_expanded_to->s_name && x->a_expanded_to->s_thing)
    {
        if (x->a_symto == x->a_symfrom)
            pd_error(x,
                "%s: atom with same send/receive name (infinite loop)",
                    x->a_symto->s_name);
        else pd_list(x->a_expanded_to->s_thing, &s_list, 1, &at);
    }
}

static void dropdown_float(t_dropdown *x, t_float f)
{
    /* this should output the atom at the relevant index
       Let's do it js-style, negative indices for wrapping
       back around and bring numbers greater than last index
       down to last index */
    t_atom at;
    SETFLOAT(&at, f);
    dropdown_set(x, 0, 1, &at);
    dropdown_bang(x);
}

static void dropdown_symbol(t_dropdown *x, t_symbol *s)
{
    t_atom at;
    SETSYMBOL(&at, s);
    dropdown_set(x, 0, 1, &at);
    dropdown_bang(x);
}

    /* We need a list method because, since there's both an "inlet" and a
    "nofirstin" flag, the standard list behavior gets confused. */
static void dropdown_list(t_dropdown *x, t_symbol *s, int argc, t_atom *argv)
{
    if (!argc)
        dropdown_bang(x);
    else if (argv->a_type == A_FLOAT)
        dropdown_float(x, argv->a_w.w_float);
    else if (argv->a_type == A_SYMBOL)
        dropdown_symbol(x, argv->a_w.w_symbol);
    else pd_error(x, "dropdown_list: need float or symbol");
}

/* this should send a message to the GUI triggering the dropdown
   <div> to be displayed and its event listeners activated */
static int dropdown_click(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_dropdown *x = (t_dropdown *)z;
    t_canvas *canvas = glist_getcanvas(glist);
    t_rtext *y = glist_findrtext(glist, (t_text *)x);
    if (doit)
    {
        int i, len = binbuf_getnatom(x->a_names);
        t_atom *at = binbuf_getvec(x->a_names);
        /* for gatom we turn the text red to indicate it as editable.
           For dropdown we instead have the GUI create a menu with which
           the user can choose an option */
        gui_start_vmess("gui_dropdown_activate", "xxsiii",
            canvas,
            x,
            rtext_gettag(y),
            x->a_index,
            sys_hostfontsize(glist_getfont(glist)),
            1);
        gui_start_array();
        for (i = 0; i < len; i++)
        {
            if (at[i].a_type == A_FLOAT)
                gui_f(at[i].a_w.w_float);
            else if (at[i].a_type == A_SYMBOL)
                gui_s(at[i].a_w.w_symbol->s_name);
            else
                gui_s("(pointer)");
        }
        gui_end_array();
        gui_end_vmess();
    }
    return (1);
}

    /* message back from dialog window */
static void dropdown_param(t_dropdown *x, t_symbol *sel, int argc, t_atom *argv)
{
    /* Check if we need to set an undo point. This happens if the user
       clicks the "Ok" button, but not when clicking "Apply" or "Cancel" */
    if (atom_getintarg(7, argc, argv))
        canvas_apply_setundo(x->a_glist, (t_gobj *)x);

    t_float width = atom_getfloatarg(0, argc, argv);

    int output = (int)atom_getfloatarg(1, argc, argv);
    t_float dummy = atom_getfloatarg(2, argc, argv);
    t_symbol *label = gatom_unescapit(atom_getsymbolarg(3, argc, argv));
    t_float wherelabel = atom_getfloatarg(4, argc, argv);
    t_symbol *symfrom = gatom_unescapit(atom_getsymbolarg(5, argc, argv));
    t_symbol *symto = gatom_unescapit(atom_getsymbolarg(6, argc, argv));

    gobj_vis(&x->a_text.te_g, x->a_glist, 0);
    if (!*symfrom->s_name && *x->a_symfrom->s_name)
        inlet_new(&x->a_text, &x->a_text.te_pd, 0, 0);
    else if (*symfrom->s_name && !*x->a_symfrom->s_name && x->a_text.te_inlet)
    {
        canvas_deletelinesforio(x->a_glist, &x->a_text,
            x->a_text.te_inlet, 0);
        inlet_free(x->a_text.te_inlet);
    }
    if (!*symto->s_name && *x->a_symto->s_name)
        outlet_new(&x->a_text, 0);
    else if (*symto->s_name && !*x->a_symto->s_name && x->a_text.te_outlet)
    {
        canvas_deletelinesforio(x->a_glist, &x->a_text,
            0, x->a_text.te_outlet);
        outlet_free(x->a_text.te_outlet);
    }
    x->a_outtype = output;
    x->a_dummy = dummy;
    if (width < 0)
        width = 4;
    else if (width > 80)
        width = 80;
    x->a_text.te_width = width;
    x->a_wherelabel = ((int)wherelabel & 3);
    x->a_label = label;
    if (*x->a_symfrom->s_name)
        pd_unbind(&x->a_text.te_pd,
            canvas_realizedollar(x->a_glist, x->a_symfrom));
    x->a_symfrom = symfrom;
    if (*x->a_symfrom->s_name)
        pd_bind(&x->a_text.te_pd,
            canvas_realizedollar(x->a_glist, x->a_symfrom));
    x->a_symto = symto;
    x->a_expanded_to = canvas_realizedollar(x->a_glist, x->a_symto);
    gobj_vis(&x->a_text.te_g, x->a_glist, 1);
    gobj_select(&x->a_text.te_g, x->a_glist, 1);
    canvas_dirty(x->a_glist, 1);
    canvas_getscroll(x->a_glist);
    /* glist_retext(x->a_glist, &x->a_text); */
}

    /* ---------------- dropdown-specific widget functions --------------- */

/* this can be combined with gatom_getwherelabel */
static void dropdown_getwherelabel(t_dropdown *x, t_glist *glist, int *xp, int *yp)
{
    int x1, y1, x2, y2;
    text_getrect(&x->a_text.te_g, glist, &x1, &y1, &x2, &y2);
    if (x->a_wherelabel == ATOM_LABELLEFT)
    {
        *xp = -3 -
            strlen(canvas_realizedollar(x->a_glist, x->a_label)->s_name) *
            sys_fontwidth(glist_getfont(glist));
        *yp = y2 - y1 - 4;
    }
    else if (x->a_wherelabel == ATOM_LABELRIGHT)
    {
        *xp = x2 - x1 + 3;
        *yp = y2 - y1 - 4;
    }
    else if (x->a_wherelabel == ATOM_LABELUP)
    {
        *xp = -1;
        *yp = -3;
    }
    else
    {
        *xp = -1;
        *yp = y2 - y1 + sys_fontheight(glist_getfont(glist));
    }
}

/* for dropdown's label */
static void dropdown_vis(t_gobj *z, t_glist *glist, int vis)
{
    //fprintf(stderr,"dropdown_vis\n");
    t_dropdown *x = (t_dropdown *)z;
    text_vis(z, glist, vis);
    if (*x->a_label->s_name)
    {
        if (vis)
        {
            int x1, y1;
            t_rtext *y = glist_findrtext(x->a_glist, &x->a_text);
            dropdown_getwherelabel(x, glist, &x1, &y1);
            gui_vmess("gui_text_new", "xssiiisi",
                glist_getcanvas(glist),
                rtext_gettag(y),
                "dropdown",
                0,
                x1, // left margin
                y1, // top margin
                canvas_realizedollar(x->a_glist, x->a_label)->s_name,
                sys_hostfontsize(glist_getfont(glist))
            );
        }
        else
        {
            /* We're just deleting the parent gobj in the GUI, which takes
               care of removing all the children. So we don't need to send
               a message here */
            //sys_vgui(".x%lx.c delete %lx.l\n", glist_getcanvas(glist), x);
        }
    }
    if (!vis)
        sys_unqueuegui(x);
}

/* a lot of this is duplicated from canvas_atom-- we should factor out the
   common stuff from the copy/pasta here */
void canvas_dropdown(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    char tagbuf[MAXPDSTRING];
    if (canvas_hasarray(gl)) return;
    //fprintf(stderr,"canvas_atom\n");
    t_dropdown *x = (t_dropdown *)pd_new(dropdown_class);
    x->a_text.te_width = 0;                        /* don't know it yet. */
    x->a_text.te_type = T_ATOM;
    x->a_text.te_iemgui = 0;
    x->a_text.te_binbuf = binbuf_new();
    x->a_glist = gl;
    x->a_dummy = 0;
    x->a_outtype = 1; /* output value by default */
    x->a_wherelabel = 0;
    x->a_label = &s_;
    x->a_symfrom = &s_;
    x->a_symto = x->a_expanded_to = &s_;
    x->a_index = 0;
    x->a_names = binbuf_new();
    binbuf_addv(x->a_names, "ssss", &s_symbol, &s_float, &s_bang, &s_list);
    x->a_maxnamewidth = dropdown_names_getmaxwidth(x);
    x->a_text.te_width = 6;

    /* bind symbol for sending index updates from the GUI */
    sprintf(tagbuf, "x%lx", (long unsigned int)x);
    pd_bind(&x->a_text.te_pd, gensym(tagbuf));

    binbuf_add(x->a_text.te_binbuf, 1, binbuf_getvec(x->a_names));
    if (argc > 1)
        /* create from file. x, y, width, low-range, high-range, flags,
            label, receive-name, send-name */
    {
        x->a_text.te_xpix = atom_getfloatarg(0, argc, argv);
        x->a_text.te_ypix = atom_getfloatarg(1, argc, argv);
        x->a_text.te_width = atom_getintarg(2, argc, argv);
            /* sanity check because some very old patches have trash in this
            field... remove this in 2003 or so: */
        if (x->a_text.te_width < 0 || x->a_text.te_width > 500)
            x->a_text.te_width = 4;
        x->a_outtype = (int)atom_getfloatarg(3, argc, argv);
        x->a_dummy = atom_getfloatarg(4, argc, argv);
        x->a_wherelabel = (((int)atom_getfloatarg(5, argc, argv)) & 3);
        x->a_label = gatom_unescapit(atom_getsymbolarg(6, argc, argv));
        x->a_symfrom = gatom_unescapit(atom_getsymbolarg(7, argc, argv));
        if (*x->a_symfrom->s_name)
            pd_bind(&x->a_text.te_pd,
                canvas_realizedollar(x->a_glist, x->a_symfrom));

        x->a_symto = gatom_unescapit(atom_getsymbolarg(8, argc, argv));
        x->a_expanded_to = canvas_realizedollar(x->a_glist, x->a_symto);
        if (x->a_symto == &s_)
            outlet_new(&x->a_text, &s_float);
        if (x->a_symfrom == &s_)
            inlet_new(&x->a_text, &x->a_text.te_pd, 0, 0);
        glist_add(gl, &x->a_text.te_g);
    }
    else
    {
        int connectme, xpix, ypix, indx, nobj;
        canvas_howputnew(gl, &connectme, &xpix, &ypix, &indx, &nobj);
        outlet_new(&x->a_text, &s_float);
        inlet_new(&x->a_text, &x->a_text.te_pd, 0, 0);
        pd_vmess(&gl->gl_pd, gensym("editmode"), "i", 1);
        x->a_text.te_xpix = xpix;
        x->a_text.te_ypix = ypix;
        glist_add(gl, &x->a_text.te_g);
        glist_noselect(gl);
        glist_select(gl, &x->a_text.te_g);
        if (connectme == 1)
            canvas_connect(gl, indx, 0, nobj, 0);
        else if (connectme == 0)
        {
            canvas_displaceselection(glist_getcanvas(gl), -8, -8);
            canvas_startmotion(glist_getcanvas(gl));
        }
        //canvas_setundo(glist_getcanvas(gl),
        //    canvas_undo_create, canvas_undo_set_create(gl), "create");
        canvas_undo_add(glist_getcanvas(gl), 9, "create",
            (void *)canvas_undo_set_create(glist_getcanvas(gl)));
    }
    glob_preset_node_list_seek_hub();
    glob_preset_node_list_check_loc_and_update();
}

static void dropdown_free(t_dropdown *x)
{
    char tagbuf[MAXPDSTRING];
    sprintf(tagbuf, "x%lx", (long unsigned int)x);
    pd_unbind(&x->a_text.te_pd, gensym(tagbuf));

    if (*x->a_symfrom->s_name)
        pd_unbind(&x->a_text.te_pd,
            canvas_realizedollar(x->a_glist, x->a_symfrom));
    gfxstub_deleteforkey(x);
}

static void dropdown_properties(t_gobj *z, t_glist *owner)
{
    t_dropdown *x = (t_dropdown *)z;
    //char buf[200];
    //sprintf(buf, "pdtk_dropdown_dialog %%s %d %g %g %d {%s} {%s} {%s}\n",
    //    x->a_text.te_width, x->a_draglo, x->a_draghi,
    //        x->a_wherelabel, gatom_escapit(x->a_label)->s_name,
    //            gatom_escapit(x->a_symfrom)->s_name,
    //                gatom_escapit(x->a_symto)->s_name);
    //gfxstub_new(&x->a_text.te_pd, x, buf);
    gui_start_vmess("gui_dropdown_dialog", "s",
        gfxstub_new2(&x->a_text.te_pd, x));
    gui_start_array();
    gui_s("name");     gui_s("dropdown");
    gui_s("width");    gui_i(x->a_text.te_width);
    gui_s("outtype");   gui_f(x->a_outtype);
    gui_s("labelpos"); gui_i(x->a_wherelabel);
    gui_s("label");    gui_s(gatom_escapit(x->a_label)->s_name);
    gui_s("receive_symbol");  gui_s(gatom_escapit(x->a_symfrom)->s_name);
    gui_s("send_symbol");     gui_s(gatom_escapit(x->a_symto)->s_name);
    gui_end_array();
    gui_end_vmess();
}

/* -------------------- widget behavior for text objects ------------ */

/* variant of the glist_findrtext found in g_rtext.c 
   that does not throw a consistency check */
extern t_rtext *glist_tryfindrtext(t_glist *gl, t_text *who);

static void text_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    //fprintf(stderr,"text_getrect\n");
    t_text *x = (t_text *)z;
    int width = 0, height = 0, iscomment = (x->te_type == T_TEXT);
    t_float x1, y1, x2, y2;

        /* for number boxes, we know width and height a priori, and should
        report them here so that graphs can get swelled to fit. */
    
    if (x->te_type == T_ATOM && x->te_width > 0)
    {
        //fprintf(stderr,"    T_ATOM\n");
        int font = glist_getfont(glist);
        int fontwidth = sys_fontwidth(font), fontheight = sys_fontheight(font);
        width = (x->te_width > 0 ? x->te_width : 6) * fontwidth + 2;
        /* add an extra two characters for the dropdown box's arrow */
        if (is_dropdown(x)) width += fontwidth * 2;
        height = fontheight + 3; /* borrowed from TMARGIN, etc, in g_rtext.c */
    }
    // jsarlo
    else if (strcmp("magicGlass", class_getname(x->ob_pd)) == 0)
    {
        width = 0;
        height = 0;
    }
    // end jsarlo
    else if (x->te_type == T_TEXT)
    {
        //fprintf(stderr,"    T_TEXT\n");
        t_rtext *y = glist_findrtext(glist, x);
        if (y)
        {
            width = rtext_width(y);
            height = rtext_height(y);
        }
        else
        {
            width = height = 10;
        }
        //fprintf(stderr,"T_TEXT width=%d height=%d\n", width, height);
    }
        /* if we're invisible we don't know our size so we just lie about
        it.  This is called on invisible boxes to establish order of inlets
        and possibly other reasons.
           To find out if the box is visible we can't just check the "vis"
        flag because we might be within the vis() routine and not have set
        that yet.  So we check directly whether the "rtext" list has been
        built.  LATER reconsider when "vis" flag should be on and off? */
    else if (glist->gl_editor && glist->gl_editor->e_rtext)
    {
        t_rtext *y = glist_tryfindrtext(glist, x);
        if (y)
        {
            width = rtext_width(y);
            if (is_dropdown(x))
            {
                int font = glist_getfont(glist);
                int fontwidth = sys_fontwidth(font);
                //width += fontwidth * 2;
                width = fontwidth * (((t_dropdown *)x)->a_maxnamewidth + 2);
            }
            height = rtext_height(y) - (iscomment << 1);
        }

        //fprintf(stderr,"rtext width=%d height=%d\n", width, height);

        /*  now find if we have more inlets or outlets than
            what can comfortably fit and adjust accordingly
            NB: textless GOPs are unaffected and are treated
            as GUI objects
        */

        //fprintf(stderr,"isgraph %d\n", ((t_glist *)z)->gl_isgraph);
        //if (!((t_glist *)z)->gl_isgraph) {

        t_object *ob = pd_checkobject(&x->te_pd);
        int no = obj_noutlets(ob);
        int ni = obj_ninlets(ob);

        /*
        // debug chunk
        char *bufdbg;
        int bufsizedbg;
        if (y) rtext_gettext(y, &bufdbg, &bufsizedbg);
        if (!strncmp(bufdbg, "gate", 4) || strlen(bufdbg) == 0)
            fprintf(stderr,"text_getrect nlets %d %d <%s>\n", ni, no, ( y ? bufdbg : "null" ));
        */

        int m = ( ni > no ? ni : no);
        //let's see if the object has more nlets than
        //its text width and resize them accordingly
        //UNLESS we are gop in which case it is user's choice
        //how big/small they want the object
        if (pd_class(&z->g_pd) != canvas_class || 
                pd_class(&z->g_pd) == canvas_class &&
                    !((t_glist *)z)->gl_isgraph)
        {
            if (width < (IOWIDTH * m) * 2 - IOWIDTH)
            {
                //we have to resize the object
                width = (IOWIDTH * m) * 2 - IOWIDTH;
            }
        }
        height = rtext_height(y) - (iscomment << 1);
    }
    else {
        width = height = 10;
        //fprintf(stderr,"    default\n");
    }
    x1 = text_xpix(x, glist);
    y1 = text_ypix(x, glist);
    x2 = x1 + width;
    y2 = y1 + height;
    //x1 += iscomment*2;
    //y1 += iscomment*6;
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

static void text_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    t_text *x = (t_text *)z;
    x->te_xpix += dx;
    x->te_ypix += dy;
    if (glist_isvisible(glist))
    {
        t_rtext *y = glist_findrtext(glist, x);
        gui_vmess("gui_text_displace", "xsii",
            glist,
            rtext_gettag(y),
            dx,
            dy);
        canvas_fixlinesfor(glist_getcanvas(glist), x);
    }
}

static void text_displace_withtag(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    t_text *x = (t_text *)z;
    x->te_xpix += dx;
    x->te_ypix += dy;
    if (glist_isvisible(glist))
        canvas_fixlinesfor(glist_getcanvas(glist), x);
}

static void text_select(t_gobj *z, t_glist *glist, int state)
{
    t_text *x = (t_text *)z;
    t_rtext *y = glist_findrtext(glist, x);
    rtext_select(y, state);

    // text_class is either a comment or an object that failed to create
    // so we distinguish between it and comment using T_TEXT type check
    if (gobj_shouldvis(&x->te_g, glist))
    {
        if (z->g_pd->c_wb && z->g_pd->c_wb->w_displacefnwtag)
        {
            if (state)
            {
                gui_vmess("gui_gobj_select", "xs",
                    glist_getcanvas(glist), rtext_gettag(y));
            }
            else
            {
                gui_vmess("gui_gobj_deselect", "xs",
                    glist_getcanvas(glist),
                    rtext_gettag(y));
            }
        }
    }
}

/* state:
   0. deactivate
   1. activate text
   2. activate "floating" text (i.e., a new empty obj that follows the mouse) */
static void text_activate(t_gobj *z, t_glist *glist, int state)
{
    t_text *x = (t_text *)z;
    t_rtext *y = glist_findrtext(glist, x);
    if (z->g_pd != gatom_class && z->g_pd != dropdown_class)
        rtext_activate(y, state);
}

static void text_delete(t_gobj *z, t_glist *glist)
{
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist, x);
}

static void text_get_typestring(int type, char *buf)
{
    if (type == T_OBJECT)
        sprintf(buf, "%s", "obj");
    else if (type == T_MESSAGE)
        sprintf(buf, "%s", "msg");
    else if (type == T_TEXT)
        sprintf(buf, "%s", "comment");
    else
        sprintf(buf, "%s", "atom");
}

static void text_vis(t_gobj *z, t_glist *glist, int vis)
{
    //fprintf(stderr,"text_vis %d\n", vis);
    t_text *x = (t_text *)z;
    int x1, y1, x2, y2;
    char type[8];
    text_get_typestring(x->te_type, type);
#ifdef PDL2ORK
    //if we are in k12 mode and this is hub with level 1 (global)
    //don't draw it and make its width/height 0
    int exception = 0;
    if (pd_class(&x->te_pd) == preset_hub_class && sys_k12_mode)
    {
        //fprintf(stderr,"text_vis reports preset_hub_class detected\n");
        t_preset_hub *h = (t_preset_hub *)z;
        if (h->ph_invis)
        {
            exception = 1;
            x->te_width = 0;
        }
    }
    if (!exception)
    {
#endif
        if (vis)
        {
            if (gobj_shouldvis(&x->te_g, glist))
            {
                //fprintf(stderr,"    draw it\n");
                t_rtext *y = glist_findrtext(glist, x);
                // make a group
                text_getrect(&x->te_g, glist, &x1, &y1, &x2, &y2);
                gui_vmess("gui_gobj_new", "xssiii",
                    glist_getcanvas(glist),
                    rtext_gettag(y),
                    type,
                    x1,
                    y1,
                    glist_istoplevel(glist));
                if (x->te_type == T_ATOM)
                    glist_retext(glist, x);
                text_drawborder(x, glist, rtext_gettag(y),
                    rtext_width(y), rtext_height(y), 1);
                rtext_draw(y);
            }
        }
        else
        {
            t_rtext *y = glist_findrtext(glist, x);
            if (gobj_shouldvis(&x->te_g, glist))
            {
                //fprintf(stderr,"    erase it %lx %lx\n", x, glist);
                text_erase_gobj(x, glist, rtext_gettag(y));
                //text_eraseborder(x, glist, rtext_gettag(y));
                //rtext_erase(y);
            }
        }
#ifdef PDL2ORK
    }
#endif
}

static int text_click(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_text *x = (t_text *)z;
    if (x->te_type == T_OBJECT)
    {
        t_symbol *clicksym = gensym("click");
        if (zgetfn(&x->te_pd, clicksym))
        {
            if (doit)
                pd_vmess(&x->te_pd, clicksym, "fffff",
                    (double)xpix, (double)ypix,
                        (double)shift, (double)0, (double)alt);
            return (1);
        }
        else return (0);
    }
    else if (x->te_type == T_ATOM)
    {
        /* Note: dropdown has its own click handler */
        t_canvas *canvas = glist_getcanvas(glist);
        t_rtext *y = glist_findrtext(glist, x);
        if (doit)
        {
            //fprintf(stderr,"atom click\n");
            /* Change the gatom blue when it's clicked? Need to test... */
            gui_vmess("gui_gatom_activate", "xsi",
                canvas, rtext_gettag(y), 1);
            gatom_click((t_gatom *)x, (t_floatarg)xpix, (t_floatarg)ypix,
                (t_floatarg)shift, (t_floatarg)0, (t_floatarg)alt);
        }
        return (1);
    }
    else if (x->te_type == T_MESSAGE)
    {
        if (doit)
            message_click((t_message *)x, (t_floatarg)xpix, (t_floatarg)ypix,
                (t_floatarg)shift, (t_floatarg)0, (t_floatarg)alt);
        return (1);
    }
    else return (0);
}

void text_save(t_gobj *z, t_binbuf *b)
{
    //fprintf(stderr, "text_save\n");
    t_text *x = (t_text *)z;
    if (x->te_type == T_OBJECT)
    {
            /* if we have a "saveto" method, and if we don't happen to be
            a canvas that's an abstraction, the saveto method does the work */
        if (zgetfn(&x->te_pd, gensym("saveto")) &&
            !((pd_class(&x->te_pd) == canvas_class) && 
                (canvas_isabstraction((t_canvas *)x)
                    || canvas_istable((t_canvas *)x))))
        {  
            //fprintf(stderr, "saveto\n");
            mess1(&x->te_pd, gensym("saveto"), b);
            binbuf_addv(b, "ssii", gensym("#X"), gensym("restore"),
                (int)x->te_xpix, (int)x->te_ypix);
        }
        else    /* otherwise just save the text */
        {
            //fprintf(stderr, "is this gop?\n");
            binbuf_addv(b, "ssii", gensym("#X"), gensym("obj"),
                (int)x->te_xpix, (int)x->te_ypix);
        }
        //fprintf(stderr, "this must be it\n");
        binbuf_addbinbuf(b, x->te_binbuf);
        //fprintf(stderr, "DONE this must be it\n");
    }
    else if (x->te_type == T_MESSAGE)
    {
        //fprintf(stderr, "message\n");
        binbuf_addv(b, "ssii", gensym("#X"), gensym("msg"),
            (int)x->te_xpix, (int)x->te_ypix);
        binbuf_addbinbuf(b, x->te_binbuf);
    }
    else if (x->te_type == T_ATOM)
    {
        //fprintf(stderr, "atom\n");
        if (pd_class(&x->te_pd) == gatom_class)
        {
            t_atomtype t = ((t_gatom *)x)->a_atom.a_type;
            t_symbol *sel = (t == A_SYMBOL ? gensym("symbolatom") :
                (t == A_FLOAT ? gensym("floatatom") : gensym("intatom")));
            t_symbol *label = gatom_escapit(((t_gatom *)x)->a_label);
            t_symbol *symfrom = gatom_escapit(((t_gatom *)x)->a_symfrom);
            t_symbol *symto = gatom_escapit(((t_gatom *)x)->a_symto);
            binbuf_addv(b, "ssiiifffsss", gensym("#X"), sel,
                (int)x->te_xpix, (int)x->te_ypix, (int)x->te_width,
                (double)((t_gatom *)x)->a_draglo,
                (double)((t_gatom *)x)->a_draghi,
                (double)((t_gatom *)x)->a_wherelabel,
                label, symfrom, symto);
        }
        else
        {
            t_symbol *sel = gensym("dropdown");
            t_symbol *label = gatom_escapit(((t_dropdown *)x)->a_label);
            t_symbol *symfrom = gatom_escapit(((t_dropdown *)x)->a_symfrom);
            t_symbol *symto = gatom_escapit(((t_dropdown *)x)->a_symto);
            binbuf_addv(b, "ssiiiiffsss", gensym("#X"), sel,
                (int)x->te_xpix, (int)x->te_ypix, (int)x->te_width,
                (int)((t_dropdown *)x)->a_outtype,
                (double)((t_dropdown *)x)->a_dummy,
                (double)((t_dropdown *)x)->a_wherelabel,
                label, symfrom, symto);
        }
    }
    else    
    {
        //fprintf(stderr,"comment\n");
        int natom = binbuf_getnatom(x->te_binbuf);
        t_atom *a = binbuf_getvec(x->te_binbuf);
        int i;
        for (i = 0; i < natom; i++)
        {
            t_symbol *s;
            if(a[i].a_type == A_SYMBOL)
            {
                //fprintf(stderr,"%d is a symbol\n", i);
                s = a[i].a_w.w_symbol;
                if (s != NULL && s->s_name != NULL)
                {
                    //fprintf(stderr,"s != NULL\n");
                    char *c;
                    for(c = s->s_name; c != NULL && *c != '\0'; c++)
                    {
                        if(*c == '\n')
                        {
                            *c = '\v';
                            //fprintf(stderr,"n->v\n");
                        }
                    }
                }
            }
        }

        binbuf_addv(b, "ssii", gensym("#X"), gensym("text"),
            (int)x->te_xpix, (int)x->te_ypix);
        binbuf_addbinbuf(b, x->te_binbuf);
    }
    if (x->te_width)
        binbuf_addv(b, ",si", gensym("f"), (int)x->te_width);
    binbuf_addv(b, ";");
}

    /* this one is for everyone but "gatoms"; it's imposed in m_class.c */
t_widgetbehavior text_widgetbehavior =
{
    text_getrect,
    text_displace,
    text_select,
    text_activate,
    text_delete,
    text_vis,
    text_click,
    text_displace_withtag,
};

static t_widgetbehavior gatom_widgetbehavior =
{
    text_getrect,
    gatom_displace,
    text_select,
    text_activate,
    text_delete,
    gatom_vis,
    text_click,
    text_displace_withtag,
};

static t_widgetbehavior dropdown_widgetbehavior =
{
    text_getrect,
    text_displace,
    text_select,
    text_activate,
    text_delete,
    dropdown_vis,
    dropdown_click,
    text_displace_withtag,
};

/* -------------------- the "text" class  ------------ */

#define EXTRAPIX 2

    /* draw inlets and outlets for a text object or for a graph. */
void glist_drawiofor(t_glist *glist, t_object *ob, int firsttime,
    char *tag, int x1, int y1, int x2, int y2)
{
    t_rtext *y = glist_findrtext(glist, ob);
    //if this is a comment or we are drawing inside gop on one of
    //our parents return
    if (pd_class(&ob->te_pd) == text_class || glist_getcanvas(glist) != glist)
        return;
    //fprintf(stderr,"glist_drawiofor\n");
    int n = obj_noutlets(ob), nplus = (n == 1 ? 1 : n-1), i;
    int width = x2 - x1;
    int issignal;
    for (i = 0; i < n; i++)
    {
        int onset = x1 + (width - IOWIDTH) * i / nplus;
        if (firsttime)
        {
            //fprintf(stderr,"glist_drawiofor o firsttime\n");
            issignal = obj_issignaloutlet(ob,i);

            /* need to send issignal and is_iemgui here... */
            gui_vmess("gui_gobj_draw_io", "xssiiiiiisiii",
                glist_getcanvas(glist),
                rtext_gettag(y),
                tag,
                onset,
                y2 - 2,
                onset + IOWIDTH,
                y2,
                x1,
                y1,
                "o",
                i,
                issignal,
                0);
        }
        else
        {
            gui_vmess("gui_gobj_redraw_io", "xssiisiii",
                glist_getcanvas(glist),
                rtext_gettag(y),
                tag,
                onset,
                y2 - 2,
                "o",
                i,
                x1,
                y1);
        }
    }
    n = obj_ninlets(ob);
    nplus = (n == 1 ? 1 : n-1);
    for (i = 0; i < n; i++)
    {
        int onset = x1 + (width - IOWIDTH) * i / nplus;
        if (firsttime)
        {
            //fprintf(stderr,"glist_drawiofor i firsttime\n");
            issignal = obj_issignalinlet(ob,i);
            gui_vmess("gui_gobj_draw_io", "xssiiiiiisiii",
                glist_getcanvas(glist),
                rtext_gettag(y),
                tag,
                onset,
                y1,
                onset + IOWIDTH,
                y1 + EXTRAPIX,
                x1,
                y1,
                "i",
                i,
                issignal,
                0);
        }
        else
        {
            //fprintf(stderr,"glist_drawiofor i firsttime\n");
            gui_vmess("gui_gobj_redraw_io", "xssiisiii",
                glist_getcanvas(glist),
                rtext_gettag(y),
                tag,
                onset,
                y1,
                "i",
                i,
                x1,
                y1);
        }
    }
}

void text_drawborder(t_text *x, t_glist *glist,
    char *tag, int width2, int height2, int firsttime)
{
    t_object *ob;
    int x1, y1, x2, y2;
    int broken;

    /* if this is gop patcher, the getrect should be equal to gop-ed window
       rather than just the size of text */
    if (pd_class(&x->te_pd) == canvas_class &&
        ((t_glist *)x)->gl_isgraph &&
        ((t_glist *)x)->gl_goprect &&
        !(((t_glist *)x)->gl_havewindow) )
    {
        int gop_width = ((t_glist *)x)->gl_pixwidth;
        int gop_height = ((t_glist *)x)->gl_pixheight;
        text_getrect(&x->te_g, glist, &x1, &y1, &x2, &y2);
        /*fprintf(stderr,"text = %d %d %d %d     gop = %d %d\n",
            x1, y1, x2, y2, gop_width, gop_height); */
        if (gop_width > (x2-x1)) x2 = x1 + gop_width;
        if (gop_height > (y2-y1)) y2 = y1 + gop_height;
    }
    else
    {
        text_getrect(&x->te_g, glist, &x1, &y1, &x2, &y2);
    }

    if (x->te_type == T_OBJECT)
    {
        broken = (pd_class(&x->te_pd) == text_class) ? 1 : 0;
        if (firsttime)
        {
            gui_vmess("gui_text_draw_border", "xssiii",
                glist_getcanvas(glist),
                tag,
                "none",
                broken,
                x2 - x1,
                y2 - y1);
        }
        else
        {
            //fprintf(stderr, "redrawing rectangle? .x%lx.c %sR\n",
            //    (t_int)glist_getcanvas(glist), tag);
            gui_vmess("gui_text_redraw_border", "xsii",
                glist_getcanvas(glist),
                tag,
                x2 - x1,
                y2 - y1);
        }
    }
    else if (x->te_type == T_MESSAGE)
    {
        if (firsttime)
        {
            gui_vmess("gui_message_draw_border", "xsii",
                glist_getcanvas(glist),
                tag,
                x2 - x1,
                y2 - y1);
        }
        else
        {
            gui_vmess("gui_message_redraw_border", "xsii",
                glist_getcanvas(glist),
                tag,
                x2 - x1,
                y2 - y1);
        }
    }
    else if (x->te_type == T_ATOM)
    {
        if (firsttime)
        {
            gui_vmess("gui_atom_draw_border", "xsiii",
                glist_getcanvas(glist),
                tag,
                (is_dropdown(x) ? ((t_dropdown *)x)->a_outtype + 1 : 0),
                x2 - x1,
                y2 - y1);
        }
        else
        {
            /* doesn't look like this ever gets called... */
            gui_vmess("gui_atom_redraw_border", "xsiii",
                glist_getcanvas(glist),
                tag,
                pd_class(&x->te_pd) == dropdown_class ?
                    ((t_dropdown *)x)->a_outtype + 1 : 0,
                x2 - x1,
                y2 - y1);
        }
    }
        /* for comments a dotted rectangle is drawn in edit mode. Currently
        this is probably inefficient because Pd assumes it must send GUI
        updates for all comments when edit mode is toggled (due to the
        "commentbar" sizing line in Pd Vanilla). However in Purr Data, we
        let CSS do all that and thus don't require that inefficiency.
        Also,
        we do not draw these unless the comments in question are being drawn
        on top level--this avoids bugggy behavior where comment rectangles are
        drawn inside a GOP on another toplevel glist when the GOP subpatch is
        in edit mode */
    else if (x->te_type == T_TEXT && glist->gl_edit && glist_istoplevel(glist))
    {
        if (firsttime)
        {
            gui_vmess("gui_text_draw_border", "xssiii",
                glist_getcanvas(glist),
                tag,
                "none",
                0,
                x2 - x1,
                y2 - y1);
        }
        else
        {
            gui_vmess("gui_text_redraw_border", "xsii",
                glist_getcanvas(glist),
                tag,
                x2 - x1,
                y2 - y1);
        }
    }

    /* draw inlets/outlets */    
    if (ob = pd_checkobject(&x->te_pd))
    {
        glist_drawiofor(glist, ob, firsttime, tag, x1, y1, x2, y2);
    }
    /* raise cords over everything else */
    if (firsttime && glist==glist_getcanvas(glist))
        canvas_raise_all_cords(glist);
}

void glist_eraseiofor(t_glist *glist, t_object *ob, char *tag)
{
    //fprintf(stderr,"glist_eraseiofor\n");
    /* This whole function seems unnecessary now... xlets
       get erased with the parent gobj group */
    int i, n;
    n = obj_noutlets(ob);
    for (i = 0; i < n; i++)
    {
        //sys_vgui(".x%lx.c delete %so%d\n",
        //    glist_getcanvas(glist), tag, i);
    }
    n = obj_ninlets(ob);
    for (i = 0; i < n; i++)
    {
        //sys_vgui(".x%lx.c delete %si%d\n",
        //    glist_getcanvas(glist), tag, i);
    }
}

// erase the whole gobj in the gui one go
void text_erase_gobj(t_text *x, t_glist *glist, char *tag)
{
    gui_vmess("gui_gobj_erase", "xs", glist_getcanvas(glist), tag);
}

/* Another function that's unnecessary... parent gobj group will
   erase this for us */
void text_eraseborder(t_text *x, t_glist *glist, char *tag)
{
    if (x->te_type == T_TEXT && !glist->gl_edit) return;
    //if (!glist_isvisible(glist)) return;
    glist_eraseiofor(glist, x, tag);
}

static int compare_subpatch_selectors(t_atom *a, t_atom *b)
{
    if (a[0].a_type == A_SYMBOL && b[0].a_type == A_SYMBOL)
    {
        return (!strcmp(a[0].a_w.w_symbol->s_name, "pd") &&
                !strcmp(b[0].a_w.w_symbol->s_name, "pd"))
               ||
               (!strcmp(a[0].a_w.w_symbol->s_name, "draw") &&
                !strcmp(b[0].a_w.w_symbol->s_name, "draw"));
    }
    else
        return 0;
}

extern t_class *scalar_class;

void text_checkvalidwidth(t_glist *glist)
{
    // readjust border in case the new object is invalid and it has more
    // connections than what the default width allows (this typically happens
    // when there is a valid object that has been replaced by an invalid one
    // and during recreation the new object has 0 inlets and outlets and is
    // therefore unaware of its possibly greater width)
    t_gobj *yg = glist->gl_list;
    if (yg)
    {
        while (yg->g_next)
            yg = yg->g_next;
        /* bugfix for scalars. Since they don't have an rtext associated with
           them the glist_findrtext call below will end with a failed
           consistency check. This can happen when undoing on a canvas that
           contains a scalar. */
        if (pd_class((t_pd *)yg) == scalar_class) return;
        t_text *newest_t = (t_text *)yg;
        t_rtext *yn = glist_findrtext(glist, newest_t);
        if (yn && pd_class(&newest_t->te_pd) == text_class &&
            newest_t->te_type != T_TEXT)
        {
            text_drawborder(newest_t, glist, rtext_gettag(yn),
                rtext_width(yn), rtext_height(yn), 0);
        }
    }
}

    /* change text; if T_OBJECT, remake it. */

void text_setto(t_text *x, t_glist *glist, char *buf, int bufsize, int pos)
{
    char *c1, *c2;
    int i1, i2;

    //fprintf(stderr,"text_setto %d\n", x->te_type);
    if (x->te_type == T_OBJECT)
    {
        //fprintf(stderr,"setto T_OBJECT\n");
        t_binbuf *b = binbuf_new();
        int natom1, natom2, widthwas = x->te_width;
        t_atom *vec1, *vec2;
        binbuf_text(b, buf, bufsize);
        natom1 = binbuf_getnatom(x->te_binbuf);
        vec1 = binbuf_getvec(x->te_binbuf);
        natom2 = binbuf_getnatom(b);
        vec2 = binbuf_getvec(b);
        /* special case: if pd subpatch is valid and its args change,
           and its new name is valid, just pass the message on. */
        if (x->te_pd == canvas_class && natom1 >= 1 && natom2 >= 1 &&
            compare_subpatch_selectors(vec1, vec2))
        {
            //fprintf(stderr,"setto canvas\n");
            //first check if the contents have changed to see if there is
            //any point of recreating the object
            binbuf_gettext(x->te_binbuf, &c1, &i1);
            binbuf_gettext(b, &c2, &i2);
            /* must remember that binbuf_gettext does *not*
               null-terminate, so we have to be careful here... */
            if (i1 != i2 || strncmp(c1, c2, i1))
            {
                //fprintf(stderr,"string differs\n");
                canvas_undo_add(glist_getcanvas(glist), 10, "recreate",
                    (void *)canvas_undo_set_recreate(glist_getcanvas(glist),
                    &x->te_g, pos));
                typedmess(&x->te_pd, gensym("rename"), natom2-1, vec2+1);
                // Special case for [draw svg] -- update the args
                if (((t_canvas *)x)->gl_svg)
                    typedmess(((t_canvas *)x)->gl_svg, gensym("update_svg"),
                        natom2-1, vec2+1);
                binbuf_free(x->te_binbuf);
                x->te_binbuf = b;
                glob_preset_node_list_seek_hub();
                glob_preset_node_list_check_loc_and_update();
                /* Crude hack-- rather than spend another hour rooting
                   through this awful spaghetti code to figure out where
                   the border is supposed to get redrawn, let's just add
                   this... */
                gobj_vis((t_gobj *)x, glist, 0);
                gobj_vis((t_gobj *)x, glist, 1);
                /* ...voila. If this ends up causing problems we can always
                   revisit it. */
                //canvas_apply_restore_original_position(glist_getcanvas(glist),
                //    pos);
            }
            else
            {
                //just retext it
                t_rtext *yr = glist_findrtext(glist, x);
                if (yr) rtext_retext(yr);
                binbuf_free(b);
                b = NULL;
            }
        }
        else  /* normally, just destroy the old one and make a new one. */
        {
            //first check if the contents have changed to see if there
            //is any point of recreating the object
            //fprintf(stderr,"setto not canvas\n");
            binbuf_gettext(x->te_binbuf, &c1, &i1);
            binbuf_gettext(b, &c2, &i2);
            /* It might be nice here to make another attempt at loading
               broken objects. For example, there may now be an abstraction
               in the canvas path-- if so, we should create it.

               However, doing this would require more thought-- for example,
               if the object indeed remains broken we wouldn't want to call
               canvas_restoreconnections as our broken object would now
               be at the end of the glist. (I imagine this is what the
               canvas_apply_restore_original_position call was meant to
               fix before it was removed.)

               So instead, we only check for new objects if the string
               has changed. If this function ever gets cleaned up to become
               more than just a series of nearly-incomprehensible
               side-effects, perhaps the issue may be revisited.
            */
            if (i1 != i2 || strncmp(c1, c2, i1))
            {
                //fprintf(stderr,"text_setto calls canvas_undo_add recreate\n");
                canvas_undo_add(glist_getcanvas(glist), 10, "recreate",
                    (void *)canvas_undo_set_recreate(glist_getcanvas(glist),
                    &x->te_g, pos));
                int xwas = x->te_xpix, ywas = x->te_ypix;
                canvas_eraselinesfor(glist, x);
                glist_delete(glist, &x->te_g);
                canvas_objtext(glist, xwas, ywas, widthwas, 0, b, 0);
                    /* if it's an abstraction loadbang it here */
                if (newest && pd_class(newest) == canvas_class)
                    canvas_loadbang((t_canvas *)newest);
                canvas_restoreconnections(glist_getcanvas(glist));
                //canvas_apply_restore_original_position(glist_getcanvas(glist),
                //    pos);
                /* this conditional is here because I'm creating scalars
                   inside of object boxes using canvas_objtext.  But scalars
                   aren't technically t_text, and checkvalidwidth expects
                   to find a new t_text at the end of the glist.
                 */
                if (!scalar_in_a_box)
                    text_checkvalidwidth(glist);
            }
            else
            {
                //fprintf(stderr,"just retext it\n");
                t_rtext *yr = glist_findrtext(glist, x);
                if (yr) rtext_retext(yr);
                binbuf_free(b);
                b = NULL;
            }
        }
            /* if we made a new "pd" or changed a window name,
                update window list */
        if (b && natom2 >= 1
            && vec2 && vec2[0].a_type == A_SYMBOL
            && !strcmp(vec2[0].a_w.w_symbol->s_name, "pd"))
        {
            canvas_updatewindowlist();
        }
        /* this is a quick bugfix-- we need to free the binbuf "b" if we
           created a scalar in canvas_objtext */
        if (scalar_in_a_box)
        {
            binbuf_free(b);   
            scalar_in_a_box = 0;
        }
    }
    else
    { // T_MESSAGE, T_TEXT, T_ATOM
        if (buf && x->te_type == T_TEXT)
        {
            char *c;
            int n;
            for(c = buf, n = 0; n < bufsize; n++, c++)
            {
                if(*c == '\n')
                {
                    *c = '\v';
                }
            }
        }
        binbuf_gettext(x->te_binbuf, &c1, &i1);
        t_binbuf *b = binbuf_new();
        binbuf_text(b, buf, bufsize);
        binbuf_gettext(b, &c2, &i2);
        if (!c1 || i1 != i2 || strncmp(c1, c2, i1))
        {
            canvas_undo_add(glist_getcanvas(glist), 10, "typing",
                (void *)canvas_undo_set_recreate(glist_getcanvas(glist),
                &x->te_g, pos));
            //fprintf(stderr,"blah |%s| |%s|\n", c1, buf);
        }
        binbuf_text(x->te_binbuf, buf, bufsize);
        binbuf_free(b);

        // we redraw retexted messages and comments so that they visually
        // match their stack position (namely, they are now on top)
        if (glist_istoplevel(glist))
        {
            gobj_vis(&x->te_g, glist, 0);
            gobj_vis(&x->te_g, glist, 1);
        }

        //probably don't need this here, but doesn't hurt to leave it in
        glob_preset_node_list_seek_hub();
        glob_preset_node_list_check_loc_and_update();
    }
}

    /* this gets called when a message gets sent to an object whose creation
    failed, presumably because of loading a patch with a missing extern or
    abstraction */
static void text_anything(t_text *x, t_symbol *s, int argc, t_atom *argv)
{
}

void g_text_setup(void)
{
    text_class = class_new(gensym("text"), 0, 0, sizeof(t_text),
        CLASS_NOINLET | CLASS_PATCHABLE, 0);
    class_addanything(text_class, text_anything);

    message_class = class_new(gensym("message"), 0, (t_method)message_free,
        sizeof(t_message), CLASS_PATCHABLE, 0);
    class_addbang(message_class, message_bang);
    class_addfloat(message_class, message_float);
    class_addsymbol(message_class, message_symbol);
    class_addblob(message_class, message_blob);
    class_addlist(message_class, message_list);
    class_addanything(message_class, message_list);

    class_addmethod(message_class, (t_method)message_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(message_class, (t_method)message_set, gensym("set"),
        A_GIMME, 0);
    class_addmethod(message_class, (t_method)message_add, gensym("add"),
        A_GIMME, 0);
    class_addmethod(message_class, (t_method)message_add2, gensym("add2"),
        A_GIMME, 0);
    class_addmethod(message_class, (t_method)message_addcomma,
        gensym("addcomma"), 0);
    class_addmethod(message_class, (t_method)message_addsemi,
        gensym("addsemi"), 0);
    class_addmethod(message_class, (t_method)message_adddollar,
        gensym("adddollar"), A_FLOAT, 0);
    class_addmethod(message_class, (t_method)message_adddollsym,
        gensym("adddollsym"), A_SYMBOL, 0);

    messresponder_class = class_new(gensym("messresponder"), 0, 0,
        sizeof(t_text), CLASS_PD, 0);
    class_addbang(messresponder_class, messresponder_bang);
    class_addfloat(messresponder_class, (t_method) messresponder_float);
    class_addsymbol(messresponder_class, messresponder_symbol);
    class_addlist(messresponder_class, messresponder_list);
    class_addanything(messresponder_class, messresponder_anything);

    gatom_class = class_new(gensym("gatom"), 0, (t_method)gatom_free,
        sizeof(t_gatom), CLASS_NOINLET | CLASS_PATCHABLE, 0);
    class_addbang(gatom_class, gatom_bang);
    class_addfloat(gatom_class, gatom_float);
    class_addsymbol(gatom_class, gatom_symbol);
    class_addlist(gatom_class, gatom_list);
    class_addmethod(gatom_class, (t_method)gatom_set, gensym("set"),
        A_GIMME, 0);
    class_addmethod(gatom_class, (t_method)gatom_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(gatom_class, (t_method)gatom_param, gensym("param"),
        A_GIMME, 0);
    class_setwidget(gatom_class, &gatom_widgetbehavior);
    class_setpropertiesfn(gatom_class, gatom_properties);

    dropdown_class = class_new(gensym("dropdown"), 0, (t_method)dropdown_free,
        sizeof(t_dropdown), CLASS_NOINLET | CLASS_PATCHABLE, 0);
    class_addbang(dropdown_class, dropdown_bang);
    class_addfloat(dropdown_class, dropdown_float);
    class_addsymbol(dropdown_class, dropdown_symbol);
    class_addlist(dropdown_class, dropdown_list);
    class_addmethod(dropdown_class, (t_method)dropdown_set, gensym("set"),
        A_GIMME, 0);
    class_addmethod(dropdown_class, (t_method)dropdown_names, gensym("names"),
        A_GIMME, 0);
    //class_addmethod(dropdown_class, (t_method)dropdown_click, gensym("click"),
    //  A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(dropdown_class, (t_method)dropdown_param, gensym("param"),
        A_GIMME, 0);
    class_setwidget(dropdown_class, &dropdown_widgetbehavior);
    class_setpropertiesfn(dropdown_class, dropdown_properties);
}
