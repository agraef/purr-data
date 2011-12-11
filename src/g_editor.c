/* Copyright (c) 1997-2001 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <stdlib.h>
#include <stdio.h>
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include "g_magicglass.h"
#include "g_canvas.h"
#include "g_undo.h"
#include <string.h>

void glist_readfrombinbuf(t_glist *x, t_binbuf *b, char *filename,
    int selectem);

void open_via_helppath(const char *name, const char *dir);
char *class_gethelpdir(t_class *c);

//static int toggle_moving = 0; //global variable

/* ------------------ forward declarations --------------- */
static void canvas_doclear(t_canvas *x);
static void glist_setlastxy(t_glist *gl, int xval, int yval);
static void glist_donewloadbangs(t_glist *x);
static t_binbuf *canvas_docopy(t_canvas *x);
static void canvas_dopaste(t_canvas *x, t_binbuf *b);
static void canvas_paste(t_canvas *x);
static void canvas_clearline(t_canvas *x);
static t_binbuf *copy_binbuf;
//static char *canvas_textcopybuf;
//static int canvas_textcopybufsize;
static t_glist *glist_finddirty(t_glist *x);
static void canvas_reselect(t_canvas *x);
static void canvas_cut(t_canvas *x);
static void canvas_undo(t_canvas *x);
static int paste_xyoffset = 0; /* a counter of pastes to make x,y offsets */
static void canvas_mouseup_gop(t_canvas *x, t_gobj *g);
static void canvas_done_popup(t_canvas *x, t_float which, t_float xpos, t_float ypos);
static void canvas_doarrange(t_canvas *x, t_float which, t_gobj *oldy, t_gobj *oldy_prev, t_gobj *oldy_next);
static void canvas_paste_xyoffset(t_canvas *x);
void canvas_setgraph(t_glist *x, int flag, int nogoprect);
static char canvas_cnct_inlet_tag[4096];
static char canvas_cnct_outlet_tag[4096];
static int outlet_issignal = 0;
static int inlet_issignal = 0;
static int last_inlet_filter = 0;
static int last_outlet_filter = 0;
static int copyfromexternalbuffer = 0;
static int screenx1;            /* screen coordinates when doing copyfromexternalbuffer */
static int screeny1;
static int screenx2;
static int screeny2;
static int copiedfont;
static void canvas_dofont(t_canvas *x, t_floatarg font, t_floatarg xresize,
    t_floatarg yresize);
extern void canvas_setbounds(t_canvas *x, int x1, int y1, int x2, int y2);
struct _outlet
{
    t_object *o_owner;
    struct _outlet *o_next;
    t_outconnect *o_connections;
    t_symbol *o_sym;
};

/* used for new duplicate behavior where we can "duplicate" into new window */
static t_canvas *c_selection;

/* iemgui uses black inlets and outlets while default objects use gray ones
   add here more as necessary */
int gobj_filter_highlight_behavior(t_rtext *y) {

	char *buf;
	char name[4];
	int bufsize, i;
	rtext_gettext(y, &buf, &bufsize);
	for (i = 0; i < 3; i++) {
		name[i] = buf[i];
	}
	name[3]='\0';
	//fprintf(stderr,"object name = >%s<\n", name);
	if (!strcmp(name, "bng") ||
		!strcmp(name, "nbx") ||
		!strcmp(name, "hdl") ||
		!strcmp(name, "hsl") ||
		!strcmp(name, "tgl") ||
		!strcmp(name, "vdl") ||
		!strcmp(name, "vsl") ||
		!strcmp(name, "vu ") ||
		/* alternative names for hradio and vradio when invoked from the menu */
		!strcmp(name, "hra") ||
		!strcmp(name, "vra")
		)
		return 1;

	return 0;
}

/* ---------------- generic widget behavior ------------------------- */

void gobj_getrect(t_gobj *x, t_glist *glist, int *x1, int *y1,
    int *x2, int *y2)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_getrectfn)
        (*x->g_pd->c_wb->w_getrectfn)(x, glist, x1, y1, x2, y2);
}

void gobj_displace(t_gobj *x, t_glist *glist, int dx, int dy)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_displacefn)
        (*x->g_pd->c_wb->w_displacefn)(x, glist, dx, dy);
}

void gobj_displace_withtag(t_gobj *x, t_glist *glist, int dx, int dy)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_displacefnwtag)
        (*x->g_pd->c_wb->w_displacefnwtag)(x, glist, dx, dy);
}

void gobj_select(t_gobj *x, t_glist *glist, int state)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_selectfn)
        (*x->g_pd->c_wb->w_selectfn)(x, glist, state);
}

void gobj_activate(t_gobj *x, t_glist *glist, int state)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_activatefn)
        (*x->g_pd->c_wb->w_activatefn)(x, glist, state);
}

void gobj_delete(t_gobj *x, t_glist *glist)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_deletefn)
        (*x->g_pd->c_wb->w_deletefn)(x, glist);
}

int gobj_shouldvis(t_gobj *x, struct _glist *glist)
{
    t_object *ob;
	//fprintf(stderr,"shouldvis\n");
    if (!glist->gl_havewindow && glist->gl_isgraph && glist->gl_goprect &&
        glist->gl_owner && (pd_class(&glist->gl_pd) != garray_class))
    {
        /* if we're graphing-on-parent and the object falls outside the
        graph rectangle, don't draw it. */
        int x1, y1, x2, y2, gx1, gy1, gx2, gy2, m;
        gobj_getrect(&glist->gl_gobj, glist->gl_owner, &x1, &y1, &x2, &y2);
        if (x1 > x2)
            m = x1, x1 = x2, x2 = m;
        if (y1 > y2)
            m = y1, y1 = y2, y2 = m;
        gobj_getrect(x, glist, &gx1, &gy1, &gx2, &gy2);
        if (gx1 < x1 || gx1 > x2 || gx2 < x1 || gx2 > x2 ||
            gy1 < y1 || gy1 > y2 || gy2 < y1 || gy2 > y2)
                return (0);
		if (glist==glist_getcanvas(glist))
        	sys_vgui(".x%lx.c raise all_cords\n", glist_getcanvas(glist));
    }
    if (ob = pd_checkobject(&x->g_pd))
    {
        /* return true if the text box should be drawn.  We don't show text
        boxes inside graphs---except comments, if we're doing the new
        (goprect) style. */
        return (glist->gl_havewindow ||
            (ob->te_pd != canvas_class &&
                ob->te_pd->c_wb != &text_widgetbehavior) ||
            (ob->te_pd == canvas_class && (((t_glist *)ob)->gl_isgraph)) ||
            (glist->gl_goprect && (ob->te_type == T_TEXT)));
    }
    else return (1);
}

void gobj_vis(t_gobj *x, struct _glist *glist, int flag)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_visfn && gobj_shouldvis(x, glist))
        (*x->g_pd->c_wb->w_visfn)(x, glist, flag);
}

int gobj_click(t_gobj *x, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_clickfn)
        return ((*x->g_pd->c_wb->w_clickfn)(x,
            glist, xpix, ypix, shift, alt, dbl, doit));
    else return (0);
}

/* ------------------------ managing the selection ----------------- */

void glist_selectline(t_glist *x, t_outconnect *oc, int index1,
    int outno, int index2, int inno)
{
    if (x->gl_editor)
    {
        glist_noselect(x);
        x->gl_editor->e_selectedline = 1;
        x->gl_editor->e_selectline_index1 = index1;
        x->gl_editor->e_selectline_outno = outno;
        x->gl_editor->e_selectline_index2 = index2;
        x->gl_editor->e_selectline_inno = inno;
        x->gl_editor->e_selectline_tag = oc;
        sys_vgui(".x%lx.c itemconfigure l%lx -fill $select_color\n",
            x, x->gl_editor->e_selectline_tag);
        sys_vgui(".x%lx.c addtag selected withtag l%lx\n",
            glist_getcanvas(x), x->gl_editor->e_selectline_tag);
		c_selection = x;
    }
}

void glist_deselectline(t_glist *x)
{
    if (x->gl_editor)
    {
        t_linetraverser t;
        t_outconnect *oc;
        x->gl_editor->e_selectedline = 0;
        linetraverser_start(&t, glist_getcanvas(x));
        do {
            oc = linetraverser_next(&t);
        } while (oc && oc != x->gl_editor->e_selectline_tag);
        int issignal;
        if(outlet_getsymbol(t.tr_outlet) == &s_signal)
            issignal = 1;
        else
            issignal = 0;
        sys_vgui(".x%lx.c itemconfigure l%lx -fill %s\n",
            x, x->gl_editor->e_selectline_tag,
            (issignal ? "$signal_cord" : "$msg_cord"));
        sys_vgui(".x%lx.c dtag l%lx selected\n",
            glist_getcanvas(x), glist_getcanvas(x)->gl_editor->e_selectline_tag);
    }    
}

int glist_isselected(t_glist *x, t_gobj *y)
{
    if (x->gl_editor)
    {
        t_selection *sel;
        for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next)
            if (sel->sel_what == y) return (1);
    }
    return (0);
}

    /* call this for unselected objects only */
void glist_select(t_glist *x, t_gobj *y)
{
    if (x->gl_editor)
    {
		if (c_selection && c_selection != x)
			glist_noselect(c_selection);
        t_selection *sel = (t_selection *)getbytes(sizeof(*sel));
        if (x->gl_editor->e_selectedline)
            glist_deselectline(x);
            /* LATER #ifdef out the following check */
        if (glist_isselected(x, y)) bug("glist_select");
        sel->sel_next = x->gl_editor->e_selection;
        sel->sel_what = y;
        x->gl_editor->e_selection = sel;
        gobj_select(y, x, 1);
		c_selection = x;

		sys_vgui("pdtk_canvas_update_edit_menu .x%lx 1\n", x);
    }
}

    /* recursively deselect everything in a gobj "g", if it happens to be
    a glist, in preparation for deselecting g itself in glist_dselect() */
static void glist_checkanddeselectall(t_glist *gl, t_gobj *g)
{
    t_glist *gl2;
    t_gobj *g2;
    if (pd_class(&g->g_pd) != canvas_class)
        return;
    gl2 = (t_glist *)g;
    for (g2 = gl2->gl_list; g2; g2 = g2->g_next)
        glist_checkanddeselectall(gl2, g2);
    glist_noselect(gl2);
}

    /* call this for selected objects only */
void glist_deselect(t_glist *x, t_gobj *y)
{
	//fprintf(stderr, "deselect\n");
    int fixdsp = 0;
    static int reenter = 0;
    /* if (reenter) return; */
    reenter = 1;
    if (x->gl_editor)
    {
        t_selection *sel, *sel2;
        t_rtext *z = 0;
        if (!glist_isselected(x, y)) bug("glist_deselect");
        if (x->gl_editor->e_textedfor)
        {
			//fprintf(stderr, "e_textedfor\n");
            t_rtext *fuddy = glist_findrtext(x, (t_text *)y);
            if (x->gl_editor->e_textedfor == fuddy)
            {
				//fprintf(stderr, "e_textedfor == fuddy\n");
                if (x->gl_editor->e_textdirty)
                {
					//fprintf(stderr, "textdirty yes\n");
                    z = fuddy;
                    canvas_stowconnections(glist_getcanvas(x));
                    glist_checkanddeselectall(x, y);
                }
                gobj_activate(y, x, 0);
            }
            if (zgetfn(&y->g_pd, gensym("dsp")))
                fixdsp = canvas_suspend_dsp();
        }
        if ((sel = x->gl_editor->e_selection)->sel_what == y)
        {
            x->gl_editor->e_selection = x->gl_editor->e_selection->sel_next;
            gobj_select(sel->sel_what, x, 0);
            freebytes(sel, sizeof(*sel));
        }
        else
        {
            for (sel = x->gl_editor->e_selection; sel2 = sel->sel_next;
                sel = sel2)
            {
                if (sel2->sel_what == y)
                {
                    sel->sel_next = sel2->sel_next;
                    gobj_select(sel2->sel_what, x, 0);
                    freebytes(sel2, sizeof(*sel2));
                    break;
                }
            }
        }
        if (z)
        {
			//fprintf(stderr, "setto\n");
            char *buf;
            int bufsize;

            rtext_gettext(z, &buf, &bufsize);
            text_setto((t_text *)y, x, buf, bufsize);
            canvas_fixlinesfor(glist_getcanvas(x), (t_text *)y);
            x->gl_editor->e_textedfor = 0;
        }
        if (fixdsp)
            canvas_resume_dsp(1);
		if (!x->gl_editor->e_selection)
			sys_vgui("pdtk_canvas_update_edit_menu .x%lx 0\n", x);
    }
    reenter = 0;
}

void glist_noselect(t_glist *x)
{
    if (x->gl_editor)
    {
		if (x->gl_editor->e_selection) {
		    while (x->gl_editor->e_selection)
		        glist_deselect(x, x->gl_editor->e_selection->sel_what);
		}
        if (x->gl_editor->e_selectedline)
            glist_deselectline(x);
		if (c_selection == x)
			c_selection = NULL;
    }
}

void glist_selectall(t_glist *x)
{
    if (x->gl_editor)
    {
        glist_noselect(x);
        if (x->gl_list)
        {
            t_selection *sel = (t_selection *)getbytes(sizeof(*sel));
            t_gobj *y = x->gl_list;
            x->gl_editor->e_selection = sel;
            sel->sel_what = y;
            gobj_select(y, x, 1);
            while (y = y->g_next)
            {
                t_selection *sel2 = (t_selection *)getbytes(sizeof(*sel2));
                sel->sel_next = sel2;
                sel = sel2;
                sel->sel_what = y;
                gobj_select(y, x, 1);
            }
            sel->sel_next = 0;
			c_selection = x;
        }
    }
}

    /* get the index of a gobj in a glist.  If y is zero, return the
    total number of objects. */
int glist_getindex(t_glist *x, t_gobj *y)
{
    t_gobj *y2;
    int indx;

    for (y2 = x->gl_list, indx = 0; y2 && y2 != y; y2 = y2->g_next)
        indx++;
    return (indx);
}

    /* get the index of the object, among selected items, if "selected"
    is set; otherwise, among unselected ones.  If y is zero, just
    counts the selected or unselected objects. */
int glist_selectionindex(t_glist *x, t_gobj *y, int selected)
{
    t_gobj *y2;
    int indx;

    for (y2 = x->gl_list, indx = 0; y2 && y2 != y; y2 = y2->g_next)
        if (selected == glist_isselected(x, y2))
            indx++;
    return (indx);
}

static t_gobj *glist_nth(t_glist *x, int n)
{
    t_gobj *y;
    int indx;
    for (y = x->gl_list, indx = 0; y; y = y->g_next, indx++)
        if (indx == n)
            return (y);
    return (0);
}

/* ------------------- support for undo/redo  -------------------------- */

static t_undofn canvas_undo_fn;         /* current undo function if any */
static int canvas_undo_whatnext;        /* whether we can now UNDO or REDO */
static void *canvas_undo_buf;           /* data private to the undo function */
static t_canvas *canvas_undo_canvas;    /* which canvas we can undo on */
static const char *canvas_undo_name;

void canvas_setundo(t_canvas *x, t_undofn undofn, void *buf,
    const char *name)
{
	//fprintf(stderr,"canvas_setundo %s\n", name);

    int hadone = 0;
        /* blow away the old undo information.  In one special case the
        old undo info is re-used; if so we shouldn't free it here. */
    if (canvas_undo_fn && canvas_undo_buf && (buf != canvas_undo_buf))
    {
		//fprintf(stderr,"hadone canvas_setundo\n");
        (*canvas_undo_fn)(canvas_undo_canvas, canvas_undo_buf, UNDO_FREE);
        hadone = 1;
    }
    canvas_undo_canvas = x;
    canvas_undo_fn = undofn;
    canvas_undo_buf = buf;
    canvas_undo_whatnext = UNDO_UNDO;
    canvas_undo_name = name;
    //if (x && glist_isvisible(x) && glist_istoplevel(x))
	if (x)
    	// enable undo in menu
        sys_vgui("pdtk_undomenu .x%lx %s no\n", x, name);
    else if (hadone)
        sys_vgui("pdtk_undomenu nobody no no\n");
}

    /* clear undo if it happens to be for the canvas x.
     (but if x is 0, clear it regardless of who owns it.) */
void canvas_noundo(t_canvas *x)
{
    if (!x || (x == canvas_undo_canvas))
        canvas_setundo(0, 0, 0, "foo");
}

static void canvas_undo(t_canvas *x)
{
	//fprintf(stderr,"canvas_undo\n");
    if (x != canvas_undo_canvas)
        bug("canvas_undo 1");
    else if (canvas_undo_whatnext != UNDO_UNDO)
        bug("canvas_undo 2");
    else
    {
        //fprintf(stderr,"undo\n");
        (*canvas_undo_fn)(canvas_undo_canvas, canvas_undo_buf, UNDO_UNDO);
            /* enable redo in menu */
        if (glist_isvisible(x) && glist_istoplevel(x))
            sys_vgui("pdtk_undomenu .x%lx no %s\n", x, canvas_undo_name);
        canvas_undo_whatnext = UNDO_REDO;
		sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
    }
}

static void canvas_redo(t_canvas *x)
{
    if (x != canvas_undo_canvas)
        bug("canvas_undo 1");
    else if (canvas_undo_whatnext != UNDO_REDO)
        bug("canvas_undo 2");
    else
    {
        /* post("redo"); */
        (*canvas_undo_fn)(canvas_undo_canvas, canvas_undo_buf, UNDO_REDO);
            /* enable undo in menu */
        if (glist_isvisible(x) && glist_istoplevel(x))
            sys_vgui("pdtk_undomenu .x%lx %s no\n", x, canvas_undo_name);
        canvas_undo_whatnext = UNDO_UNDO;
		sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
    }
}

/* ------- specific undo methods: 1. connect and disconnect -------- */

typedef struct _undo_connect    
{
    int u_index1;
    int u_outletno;
    int u_index2;
    int u_inletno;
} t_undo_connect;

static void *canvas_undo_set_disconnect(t_canvas *x,
    int index1, int outno, int index2, int inno)
{
    t_undo_connect *buf = (t_undo_connect *)getbytes(sizeof(*buf));
    buf->u_index1 = index1;
    buf->u_outletno = outno;
    buf->u_index2 = index2;
    buf->u_inletno = inno;
    return (buf);
}

void canvas_disconnect(t_canvas *x,
    t_float index1, t_float outno, t_float index2, t_float inno)
{
    t_linetraverser t;
    t_outconnect *oc;
    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
    {
        int srcno = canvas_getindex(x, &t.tr_ob->ob_g);
        int sinkno = canvas_getindex(x, &t.tr_ob2->ob_g);
        if (srcno == index1 && t.tr_outno == outno &&
            sinkno == index2 && t.tr_inno == inno)
        {
            sys_vgui(".x%lx.c delete l%lx\n", x, oc);
            // jsarlo
			if(x->gl_magic_glass) {
            	magicGlass_unbind(x->gl_magic_glass);
            	magicGlass_hide(x->gl_magic_glass);
			}
            // end jsarlo
            obj_disconnect(t.tr_ob, t.tr_outno, t.tr_ob2, t.tr_inno);
            break;
        }
    }
}

static void canvas_undo_disconnect(t_canvas *x, void *z, int action)
{
    t_undo_connect *buf = z;
    if (action == UNDO_UNDO)
    {
        canvas_connect(x, buf->u_index1, buf->u_outletno,
            buf->u_index2, buf->u_inletno);
    }
    else if (action == UNDO_REDO)
    {
        canvas_disconnect(x, buf->u_index1, buf->u_outletno,
            buf->u_index2, buf->u_inletno);
    }
    else if (action == UNDO_FREE)
        t_freebytes(buf, sizeof(*buf));
}

    /* connect just calls disconnect actions backward... */
static void *canvas_undo_set_connect(t_canvas *x,
    int index1, int outno, int index2, int inno)
{
    return (canvas_undo_set_disconnect(x, index1, outno, index2, inno));
}

static void canvas_undo_connect(t_canvas *x, void *z, int action)
{
    int myaction;
    if (action == UNDO_UNDO)
        myaction = UNDO_REDO;
    else if (action == UNDO_REDO)
        myaction = UNDO_UNDO;
    else myaction = action;
    canvas_undo_disconnect(x, z, myaction);
}

/* ---------- ... 2. cut, clear, and typing into objects: -------- */

#define UCUT_CUT 1          /* operation was a cut */
#define UCUT_CLEAR 2        /* .. a clear */
#define UCUT_TEXT 3         /* text typed into a box */

typedef struct _undo_cut        
{
    t_binbuf *u_objectbuf;      /* the object cleared or typed into */
    t_binbuf *u_reconnectbuf;   /* connections into and out of object */
    t_binbuf *u_redotextbuf;    /* buffer to paste back for redo if TEXT */
    int u_mode;                 /* from flags above */
	int n_obj;					/* number of selected objects to be cut */
	int p_a[1];					/* array of original glist positions of selected objects */
								/* at least one object is selected, we dynamically resize it later */
} t_undo_cut;

static void *canvas_undo_set_cut(t_canvas *x, int mode)
{
    t_undo_cut *buf;
    t_gobj *y;
    t_linetraverser t;
    t_outconnect *oc;
    int nnotsel= glist_selectionindex(x, 0, 0);
	int nsel = glist_selectionindex(x, 0, 1);
    buf = (t_undo_cut *)getbytes(sizeof(*buf) + sizeof(buf->p_a[0]) * (nsel - 1));
	buf->n_obj = nsel;
    buf->u_mode = mode;
    buf->u_redotextbuf = 0;

        /* store connections into/out of the selection */
    buf->u_reconnectbuf = binbuf_new();
    linetraverser_start(&t, x);
	//if (linetraverser_next(&t)) {
	while (oc = linetraverser_next(&t))
	{
	    int issel1 = glist_isselected(x, &t.tr_ob->ob_g);
	    int issel2 = glist_isselected(x, &t.tr_ob2->ob_g);
	    if (issel1 != issel2)
	    {
	        binbuf_addv(buf->u_reconnectbuf, "ssiiii;",
	            gensym("#X"), gensym("connect"),
	            (issel1 ? nnotsel : 0)
	                + glist_selectionindex(x, &t.tr_ob->ob_g, issel1),
	            t.tr_outno,
	            (issel2 ? nnotsel : 0) +
	                glist_selectionindex(x, &t.tr_ob2->ob_g, issel2),
	            t.tr_inno);
	    }
	}
	//}
    if (mode == UCUT_TEXT)
    {
        buf->u_objectbuf = canvas_docopy(x);
    }
    else if (mode == UCUT_CUT)
    {
        buf->u_objectbuf = 0;
    }
    else if (mode == UCUT_CLEAR)
    {
        buf->u_objectbuf = canvas_docopy(x);
    }

	//instantiate num_obj and fill array of positions of selected objects
	if (mode == UCUT_CUT || mode == UCUT_CLEAR)
	{
		int i = 0, j = 0;
		if (x->gl_list) {
			for (y = x->gl_list; y; y = y->g_next)
			{
				if (glist_isselected(x, y)) {
					buf->p_a[i] = j;
					i++; 
				}
				j++;
			}
		}
		//for (i = 0; i < buf->n_obj; i++)
		//	fprintf(stderr,"%d position = %d\n", i, buf->p_a[i]);
	}

    return (buf);
}

static void canvas_undo_cut(t_canvas *x, void *z, int action)
{
	//fprintf(stderr, "canvas_undo_cut canvas=%d buf=%d action=%d\n", (int)x, (int)z, action);
    t_undo_cut *buf = z;
    int mode = buf->u_mode;
    if (action == UNDO_UNDO)
    {
		//fprintf(stderr,"UNDO_UNDO\n");
        if (mode == UCUT_CUT) {
			//fprintf(stderr, "UCUT_CUT\n");
            canvas_dopaste(x, copy_binbuf);
		}
        else if (mode == UCUT_CLEAR) {
			//fprintf(stderr, "UCUT_CLEAR\n");
            canvas_dopaste(x, buf->u_objectbuf);
		}
        else if (mode == UCUT_TEXT)
        {
			//fprintf(stderr, "UCUT_TEXT\n");
            t_gobj *y1, *y2;
            glist_noselect(x);
            for (y1 = x->gl_list; y2 = y1->g_next; y1 = y2)
                ;
            if (y1)
            {
                if (!buf->u_redotextbuf)
                {
                    glist_noselect(x);
                    glist_select(x, y1);
                    buf->u_redotextbuf = canvas_docopy(x);
                    glist_noselect(x);
                }
                glist_delete(x, y1);
            }
            canvas_dopaste(x, buf->u_objectbuf);
        }
        pd_bind(&x->gl_pd, gensym("#X"));
        binbuf_eval(buf->u_reconnectbuf, 0, 0, 0);
        pd_unbind(&x->gl_pd, gensym("#X"));

		//now reposition objects to their original locations
		if (mode == UCUT_CUT || mode == UCUT_CLEAR) {
			//fprintf(stderr,"reordering\n");
			int i = 0;
			int paste_pos = glist_getindex(x,0) - buf->n_obj; //location of the first newly pasted object
			//fprintf(stderr,"paste_pos %d\n", paste_pos);
			t_gobj *y_prev, *y, *y_next;
			for (i = 0; i < buf->n_obj; i++) {
				//first check if we are in the same position already
				if (paste_pos+i != buf->p_a[i]) {
					//fprintf(stderr,"not in the right place\n");
					y_prev = glist_nth(x, paste_pos-1+i);
					y = glist_nth(x, paste_pos+i);
					y_next = glist_nth(x, paste_pos+1+i);
					//if the object is supposed to be first in of gl_list
					if (buf->p_a[i] == 0) {
						if (y_prev && y_next) {
							y_prev->g_next = y_next;
						}
						else if (y_prev && !y_next)
							y_prev->g_next = NULL;
						//now put the moved object at the beginning of the cue
						y->g_next = glist_nth(x, 0);
						x->gl_list = y;
					}
					//if the object is supposed to be at the current end of gl_list	
					//can this ever happen???
					/*else if (!glist_nth(x,buf->p_a[i])) {

					}*/
					//if the object is supposed to be in the middle of gl_list
					else {
						if (y_prev && y_next) {
							y_prev->g_next = y_next;
						}
						else if (y_prev && !y_next) {
							y_prev->g_next = NULL;
						}
						//now put the moved object in its right place
						y_prev = glist_nth(x, buf->p_a[i]-1);
						y_next = glist_nth(x, buf->p_a[i]);

						y_prev->g_next = y;
						y->g_next = y_next;
					}
				}
				canvas_redraw(x);
			}
		}
    }
    else if (action == UNDO_REDO)
    {
		//fprintf(stderr,"UNDO_REDO\n");
        if (mode == UCUT_CUT || mode == UCUT_CLEAR) {
			//we can't just blindly do clear here when the user may have
			//unselected things between undo and redo, so first let's select
			//the right stuff
			glist_noselect(x);
			int i = 0;
			for (i = 0; i < buf->n_obj; i++)
            	glist_select(x, glist_nth(x, buf->p_a[i]));
            canvas_doclear(x);
		}
        else if (mode == UCUT_TEXT)
        {
            t_gobj *y1, *y2;
            for (y1 = x->gl_list; y2 = y1->g_next; y1 = y2)
                ;
            if (y1)
                glist_delete(x, y1);
            canvas_dopaste(x, buf->u_redotextbuf);
            pd_bind(&x->gl_pd, gensym("#X"));
            binbuf_eval(buf->u_reconnectbuf, 0, 0, 0);
            pd_unbind(&x->gl_pd, gensym("#X"));
        }
    }
    else if (action == UNDO_FREE)
    {
		//fprintf(stderr,"UNDO_FREE\n");
        if (buf->u_objectbuf)
            binbuf_free(buf->u_objectbuf);
        if (buf->u_reconnectbuf)
            binbuf_free(buf->u_reconnectbuf);
        if (buf->u_redotextbuf)
            binbuf_free(buf->u_redotextbuf);
        if (buf != NULL) t_freebytes(buf, sizeof(*buf) + sizeof(buf->p_a[0]) * (buf->n_obj-1));
    }
}

/* --------- 3. motion, including "tidy up" and stretching ----------- */

typedef struct _undo_move_elem  
{
    int e_index;
    int e_xpix;
    int e_ypix;
} t_undo_move_elem;

typedef struct _undo_move       
{
    t_undo_move_elem *u_vec;
    int u_n;
} t_undo_move;

static int canvas_undo_already_set_move;

static void *canvas_undo_set_move(t_canvas *x, int selected)
{
    int x1, y1, x2, y2, i, indx;
    t_gobj *y;
    t_undo_move *buf =  (t_undo_move *)getbytes(sizeof(*buf));
    buf->u_n = selected ? glist_selectionindex(x, 0, 1) : glist_getindex(x, 0);
    buf->u_vec = (t_undo_move_elem *)getbytes(sizeof(*buf->u_vec) *
        (selected ? glist_selectionindex(x, 0, 1) : glist_getindex(x, 0)));
    if (selected)
    {
        for (y = x->gl_list, i = indx = 0; y; y = y->g_next, indx++)
            if (glist_isselected(x, y))
        {
            gobj_getrect(y, x, &x1, &y1, &x2, &y2);
            buf->u_vec[i].e_index = indx;
            buf->u_vec[i].e_xpix = x1;
            buf->u_vec[i].e_ypix = y1;
            i++;
        }
    }
    else
    {
        for (y = x->gl_list, indx = 0; y; y = y->g_next, indx++)
        {
            gobj_getrect(y, x, &x1, &y1, &x2, &y2);
            buf->u_vec[indx].e_index = indx;
            buf->u_vec[indx].e_xpix = x1;
            buf->u_vec[indx].e_ypix = y1;
        }
    }
    canvas_undo_already_set_move = 1;
    return (buf);
}

static void canvas_undo_move(t_canvas *x, void *z, int action)
{
    t_undo_move *buf = z;
	t_class *cl;
	int resortin = 0, resortout = 0;
    if (action == UNDO_UNDO || action == UNDO_REDO)
    {
        int i;
        for (i = 0; i < buf->u_n; i++)
        {
            int x1, y1, x2, y2, newx, newy;
            t_gobj *y;
            newx = buf->u_vec[i].e_xpix;
            newy = buf->u_vec[i].e_ypix;
            y = glist_nth(x, buf->u_vec[i].e_index);
            if (y)
            {
                gobj_getrect(y, x, &x1, &y1, &x2, &y2);
                gobj_displace(y, x, newx-x1, newy - y1);
                buf->u_vec[i].e_xpix = x1;
                buf->u_vec[i].e_ypix = y1;
				cl = pd_class(&y->g_pd);
		        if (cl == vinlet_class) resortin = 1;
		        else if (cl == voutlet_class) resortout = 1;
            }
        }
		if (resortin) canvas_resortinlets(x);
		if (resortout) canvas_resortoutlets(x);
    }
    else if (action == UNDO_FREE)
    {
        t_freebytes(buf->u_vec, buf->u_n * sizeof(*buf->u_vec));
        t_freebytes(buf, sizeof(*buf));
    }
}

/* --------- 4. paste (also duplicate) ----------- */

typedef struct _undo_paste      
{
    int u_index;    /* index of first object pasted */  
} t_undo_paste;

static void *canvas_undo_set_paste(t_canvas *x)
{
    t_undo_paste *buf =  (t_undo_paste *)getbytes(sizeof(*buf));
    buf->u_index = glist_getindex(x, 0);
    return (buf);
}

static void canvas_undo_paste(t_canvas *x, void *z, int action)
{
    t_undo_paste *buf = z;
    if (action == UNDO_UNDO)
    {
        t_gobj *y;
        glist_noselect(x);
        for (y = glist_nth(x, buf->u_index); y; y = y->g_next)
            glist_select(x, y);
        canvas_doclear(x);
    }
    else if (action == UNDO_REDO)
    {
        t_selection *sel;
        canvas_dopaste(x, copy_binbuf);
            /* if it was "duplicate" have to re-enact the displacement. */
        if (canvas_undo_name && canvas_undo_name[0] == 'd')
            //for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next)
            //    gobj_displace(sel->sel_what, x, 10, 10);
			canvas_paste_xyoffset(x);
    }
else if (action == UNDO_FREE)
        t_freebytes(buf, sizeof(*buf));
}

    /* recursively check for abstractions to reload as result of a save. 
    Don't reload the one we just saved ("except") though. */
    /*  LATER try to do the same trick for externs. */
static void glist_doreload(t_glist *gl, t_symbol *name, t_symbol *dir,
    t_gobj *except)
{
	//fprintf(stderr,"doreload\n");
    t_gobj *g;
    int i, nobj = glist_getindex(gl, 0);  /* number of objects */
    int hadwindow = gl->gl_havewindow;
    for (g = gl->gl_list, i = 0; g && i < nobj; i++)
    {
        if (g != except && pd_class(&g->g_pd) == canvas_class &&
            canvas_isabstraction((t_canvas *)g) &&
                ((t_canvas *)g)->gl_name == name &&
                    canvas_getdir((t_canvas *)g) == dir)
        {
                /* we're going to remake the object, so "g" will go stale.
                Get its index here, and afterward restore g.  Also, the
                replacement will be at the end of the list, so we don't
                do g = g->g_next in this case. */
			//fprintf(stderr, "rebuildlicious\n");
            int j = glist_getindex(gl, g);
            if (!gl->gl_havewindow)
                canvas_vis(glist_getcanvas(gl), 1);
            glist_noselect(gl);
            glist_select(gl, g);
            canvas_setundo(gl, canvas_undo_cut,
                canvas_undo_set_cut(gl, UCUT_CLEAR), "clear");
            canvas_doclear(gl);
            canvas_undo(gl);
            glist_noselect(gl);
            g = glist_nth(gl, j);
        }
        else
        {
            if (g != except && pd_class(&g->g_pd) == canvas_class)
                glist_doreload((t_canvas *)g, name, dir, except);
             	g = g->g_next;
        }
    }
    if (!hadwindow && gl->gl_havewindow)
        canvas_vis(glist_getcanvas(gl), 0);
}

    /* this flag stops canvases from being marked "dirty" if we have to touch
    them to reload an abstraction; also suppress window list update */
int glist_amreloadingabstractions = 0;

    /* call canvas_doreload on everyone */
void canvas_reload(t_symbol *name, t_symbol *dir, t_gobj *except)
{
    t_canvas *x;
    int dspwas = canvas_suspend_dsp();
    glist_amreloadingabstractions = 1;
        /* find all root canvases */
    for (x = canvas_list; x; x = x->gl_next)
        glist_doreload(x, name, dir, except);
    glist_amreloadingabstractions = 0;
    canvas_resume_dsp(dspwas);
}

/* --------- 5. apply  ----------- */

typedef struct _undo_apply        
{
    t_binbuf *u_objectbuf;      /* the object cleared or typed into */
    t_binbuf *u_reconnectbuf;   /* connections into and out of object */
	int u_index;				/* index of the previous object */
} t_undo_apply;

static void *canvas_undo_set_apply(t_canvas *x, t_gobj *obj)
{
    t_undo_apply *buf;
    t_gobj *y;
    t_linetraverser t;
    t_outconnect *oc;
	/* enable editor (in case it is disabled) and select the object we are working on */
	if (!x->gl_edit)
		canvas_editmode(x, 1);
	if (!glist_isselected(x, obj))
		glist_select(x, obj);
    int nnotsel= glist_selectionindex(x, 0, 0); /* get number of all items for the offset below */
    buf = (t_undo_apply *)getbytes(sizeof(*buf));

    /* store connections into/out of the selection */
    buf->u_reconnectbuf = binbuf_new();
    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
    {
        int issel1 = glist_isselected(x, &t.tr_ob->ob_g);
        int issel2 = glist_isselected(x, &t.tr_ob2->ob_g);
        if (issel1 != issel2)
        {
            binbuf_addv(buf->u_reconnectbuf, "ssiiii;",
                gensym("#X"), gensym("connect"),
                (issel1 ? nnotsel : 0)
                    + glist_selectionindex(x, &t.tr_ob->ob_g, issel1),
                t.tr_outno,
                (issel2 ? nnotsel : 0) +
                    glist_selectionindex(x, &t.tr_ob2->ob_g, issel2),
                t.tr_inno);
        }
    }
	/* copy object in its current state */
    buf->u_objectbuf = canvas_docopy(x);

	/* store index of the currently selected object */
	buf->u_index = glist_getindex(x, obj);

    return (buf);
}

static void canvas_undo_apply(t_canvas *x, void *z, int action)
{
    t_undo_apply *buf = z;
    if (action == UNDO_UNDO || action == UNDO_REDO)
    {
		/* find current instance */
		glist_noselect(x);
		glist_select(x, glist_nth(x, buf->u_index));

		/* copy it for the new undo/redo */
		t_binbuf *tmp = canvas_docopy(x);

		/* delete current instance */
		canvas_doclear(x);

		/* replace it with previous instance */
        canvas_dopaste(x, buf->u_objectbuf);

		/* change previous instance with current one */
		buf->u_objectbuf = tmp;
		buf->u_index = glist_selectionindex(x, 0, 0);

		/* connections should stay the same */
        pd_bind(&x->gl_pd, gensym("#X"));
        binbuf_eval(buf->u_reconnectbuf, 0, 0, 0);
        pd_unbind(&x->gl_pd, gensym("#X"));
    }
    else if (action == UNDO_FREE)
    {
        if (buf->u_objectbuf)
            binbuf_free(buf->u_objectbuf);
        if (buf->u_reconnectbuf)
            binbuf_free(buf->u_reconnectbuf);
        t_freebytes(buf, sizeof(*buf));
    }
}

void canvas_apply_setundo(t_canvas *x, t_gobj *y)
{
	canvas_setundo(x, canvas_undo_apply, canvas_undo_set_apply(x, y), "apply");
}

/* --------- 6. arrange (to front/back)  ----------- */

typedef struct _undo_arrange       
{
	int u_previndex;			/* old index */
	int u_newindex;				/* new index */
} t_undo_arrange;

static void *canvas_undo_set_arrange(t_canvas *x, t_gobj *obj, int newindex)
{
	// newindex tells us is the new index at the beginning (0) or the end (1)

    t_undo_arrange *buf;
    t_gobj *y;
	/* enable editor (in case it is disabled) and select the object we are working on */
	if (!x->gl_edit)
		canvas_editmode(x, 1);

	// select the object
	if (!glist_isselected(x, obj))
		glist_select(x, obj);

    buf = (t_undo_arrange *)getbytes(sizeof(*buf));

	// set the u_newindex appropriately
	if (newindex == 0) buf->u_newindex = 0;
	else buf->u_newindex = glist_getindex(x, 0) - 1;

	/* store index of the currently selected object */
	buf->u_previndex = glist_getindex(x, obj);

	//fprintf(stderr,"undo_set_arrange %d %d\n", buf->u_previndex, buf->u_newindex);		

    return (buf);
}

static void canvas_undo_arrange(t_canvas *x, void *z, int action)
{
    t_undo_arrange *buf = z;
	t_gobj *y=NULL, *prev=NULL, *next=NULL;

	if (!x->gl_edit)
		canvas_editmode(x, 1);

    if (action == UNDO_UNDO)
    {
		// this is our object
		y = glist_nth(x, buf->u_newindex);

		//fprintf(stderr,"canvas_undo_arrange UNDO_UNDO %d %d\n", buf->u_previndex, buf->u_newindex);		

		/* select object */
		glist_noselect(x);
		glist_select(x, y);

		if (buf->u_newindex) {
			// if it is the last object
			
			// first previous object should point to nothing
			prev = glist_nth(x, buf->u_newindex - 1);
			prev->g_next = NULL;	

			/* now we reuse vars for the follwoing:
			   old index should be right before the object previndex
			   is pointing to as the object was moved to the end */

			/* old position is not first */
			if (buf->u_previndex) {
				prev = glist_nth(x, buf->u_previndex - 1);
				next = prev->g_next;

				// now readjust pointers
				prev->g_next = y;
				y->g_next = next;
			}
			/* old position is first */
			else {
				prev = NULL;
				next = x->gl_list;

				// now readjust pointers
				y->g_next = next;
				x->gl_list = y;
			}

			// and finally redraw canvas
			canvas_redraw(x);
		}
		else {
			// if it is the first object

			/* old index should be right after the object previndex
			   is pointing to as the object was moved to the end */
			prev = glist_nth(x, buf->u_previndex);

			// next may be NULL and that is ok
			next = prev->g_next;

			//first glist pointer needs to point to the second object
			x->gl_list = y->g_next;

			//now readjust pointers
			prev->g_next = y;
			y->g_next = next;

			// and finally redraw canvas
			canvas_redraw(x);
		}
    }
	else if (action == UNDO_REDO) {
		// find our object
		y = glist_nth(x, buf->u_previndex);

		//fprintf(stderr,"canvas_undo_arrange UNDO_REDO %d %d\n", buf->u_previndex, buf->u_newindex);	

		/* select object */
		glist_noselect(x);
		glist_select(x, y);

		int action;
		if (!buf->u_newindex) action = 4;
		else action = 3;

		t_gobj *oldy_prev=NULL, *oldy_next=NULL;

		// if there is an object before ours (in other words our index is > 0)
		if (glist_getindex(x,y))
			oldy_prev = glist_nth(x, buf->u_previndex - 1);
			
		// if there is an object after ours
		if (y->g_next)
			oldy_next = y->g_next;

		canvas_doarrange(x, action, y, oldy_prev, oldy_next);
	}
    else if (action == UNDO_FREE)
    {
        t_freebytes(buf, sizeof(*buf));
    }
}

void canvas_arrange_setundo(t_canvas *x, t_gobj *obj, int newindex)
{
	canvas_setundo(x, canvas_undo_arrange, canvas_undo_set_arrange(x, obj, newindex), "arrange");
}

/* --------- 7. apply on canvas ----------- */

typedef struct _undo_canvas_properties      
{
    int gl_pixwidth;            /* width in pixels (on parent, if a graph) */
    int gl_pixheight;
    t_float gl_x1;                /* bounding rectangle in our own coordinates */
    t_float gl_y1;
    t_float gl_x2;
    t_float gl_y2;
    int gl_screenx1;            /* screen coordinates when toplevel */
    int gl_screeny1;
    int gl_screenx2;
    int gl_screeny2;
    int gl_xmargin;                /* origin for GOP rectangle */
    int gl_ymargin;

    unsigned int gl_goprect:1;      /* draw rectangle for graph-on-parent */
    unsigned int gl_isgraph:1;      /* show as graph on parent */
    unsigned int gl_hidetext:1;     /* hide object-name + args when doing graph on parent */
} t_undo_canvas_properties;

t_undo_canvas_properties global_buf;

static void *canvas_undo_set_canvas(t_canvas *x)
{
	/* enable editor (in case it is disabled) */
	//if (x->gl_havewindow && !x->gl_edit)
	//	canvas_editmode(x, 1);

	global_buf.gl_pixwidth = x->gl_pixwidth;
	global_buf.gl_pixheight = x->gl_pixheight;
	global_buf.gl_x1 = x->gl_x1;
	global_buf.gl_y1 = x->gl_y1;
	global_buf.gl_x2 = x->gl_x2;
	global_buf.gl_y2 = x->gl_y2;
	global_buf.gl_screenx1 = x->gl_screenx1;
	global_buf.gl_screeny1 = x->gl_screeny1;
	global_buf.gl_screenx2 = x->gl_screenx2;
	global_buf.gl_screeny2 = x->gl_screeny2;
	global_buf.gl_xmargin = x->gl_xmargin;
	global_buf.gl_ymargin = x->gl_ymargin;
	global_buf.gl_goprect = x->gl_goprect;
	global_buf.gl_isgraph = x->gl_isgraph;
	global_buf.gl_hidetext = x->gl_hidetext;
	
    return (&global_buf);
}

extern int gfxstub_haveproperties(void *key);

static void canvas_undo_canvas_apply(t_canvas *x, void *z, int action)
{
    t_undo_canvas_properties *buf = z;
    t_undo_canvas_properties tmp;

	if (!x->gl_edit)
		canvas_editmode(x, 1);

	if (action == UNDO_UNDO || action == UNDO_REDO)
	{
		/*//close properties window first
		t_int properties = gfxstub_haveproperties((void *)x);
		if (properties) {
			sys_vgui("destroy .gfxstub%lx\n", properties);
		}*/

		//store current canvas values into temporary data holder
		tmp.gl_pixwidth = x->gl_pixwidth;
		tmp.gl_pixheight = x->gl_pixheight;
		tmp.gl_x1 = x->gl_x1;
		tmp.gl_y1 = x->gl_y1;
		tmp.gl_x2 = x->gl_x2;
		tmp.gl_y2 = x->gl_y2;
		tmp.gl_screenx1 = x->gl_screenx1;
		tmp.gl_screeny1 = x->gl_screeny1;
		tmp.gl_screenx2 = x->gl_screenx2;
		tmp.gl_screeny2 = x->gl_screeny2;
		tmp.gl_xmargin = x->gl_xmargin;
		tmp.gl_ymargin = x->gl_ymargin;
		tmp.gl_goprect = x->gl_goprect;
		tmp.gl_isgraph = x->gl_isgraph;
		tmp.gl_hidetext = x->gl_hidetext;

		//change canvas values with the ones from the undo buffer
		x->gl_pixwidth = buf->gl_pixwidth;
		x->gl_pixheight = buf->gl_pixheight;
		x->gl_x1 = buf->gl_x1;
		x->gl_y1 = buf->gl_y1;
		x->gl_x2 = buf->gl_x2;
		x->gl_y2 = buf->gl_y2;
		x->gl_screenx1 = buf->gl_screenx1;
		x->gl_screeny1 = buf->gl_screeny1;
		x->gl_screenx2 = buf->gl_screenx2;
		x->gl_screeny2 = buf->gl_screeny2;
		x->gl_xmargin = buf->gl_xmargin;
		x->gl_ymargin = buf->gl_ymargin;
		x->gl_goprect = buf->gl_goprect;
		x->gl_isgraph = buf->gl_isgraph;
		x->gl_hidetext = buf->gl_hidetext;

		//copy data values from the temporary data to the undo buffer
		buf->gl_pixwidth = tmp.gl_pixwidth;
		buf->gl_pixheight = tmp.gl_pixheight;
		buf->gl_x1 = tmp.gl_x1;
		buf->gl_y1 = tmp.gl_y1;
		buf->gl_x2 = tmp.gl_x2;
		buf->gl_y2 = tmp.gl_y2;
		buf->gl_screenx1 = tmp.gl_screenx1;
		buf->gl_screeny1 = tmp.gl_screeny1;
		buf->gl_screenx2 = tmp.gl_screenx2;
		buf->gl_screeny2 = tmp.gl_screeny2;
		buf->gl_xmargin = tmp.gl_xmargin;
		buf->gl_ymargin = tmp.gl_ymargin;
		buf->gl_goprect = tmp.gl_goprect;
		buf->gl_isgraph = tmp.gl_isgraph;
		buf->gl_hidetext = tmp.gl_hidetext;

		//redraw
		canvas_setgraph(x, x->gl_isgraph + 2*x->gl_hidetext, 0);
		canvas_dirty(x, 1);
		if (x->gl_havewindow) {
		    canvas_redraw(x);
		}
		if (x->gl_owner && glist_isvisible(x->gl_owner))
		{
			glist_noselect(x);
		    gobj_vis(&x->gl_gobj, x->gl_owner, 0);
		    gobj_vis(&x->gl_gobj, x->gl_owner, 1);
			canvas_redraw(x->gl_owner);
		}
		//update scrollbars when GOP potentially exceeds window size
		t_canvas *canvas=(t_canvas *)glist_getcanvas(x);

		//if gop is being disabled go one level up
		if (!x->gl_isgraph && x->gl_owner) {
			canvas=canvas->gl_owner;
			canvas_redraw(canvas);
		}

		//if properties window is open, update the properties with the previous window properties		
		t_int properties = gfxstub_haveproperties((void *)x);
		if (properties) {
			sys_vgui(".gfxstub%lx.xrange.entry3 delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.xrange.entry3 insert 0 %d\n", properties, x->gl_pixwidth);
			sys_vgui(".gfxstub%lx.yrange.entry3 delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.yrange.entry3 insert 0 %d\n", properties, x->gl_pixheight);
			sys_vgui(".gfxstub%lx.xrange.entry4 delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.xrange.entry4 insert 0 %d\n", properties, x->gl_xmargin);
			sys_vgui(".gfxstub%lx.yrange.entry4 delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.yrange.entry4 insert 0 %d\n", properties, x->gl_ymargin);
		}

		sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", (t_int)x);
		sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", (t_int)canvas);
	}

    else if (action == UNDO_FREE)
    {
		//do nothing since undo apply uses a global_buf struct rather than a pointer
    }
}

void canvas_canvas_setundo(t_canvas *x)
{
	canvas_setundo(x, canvas_undo_canvas_apply, canvas_undo_set_canvas(x), "apply");
}

/* --------- 8. create ----------- */

typedef struct _undo_create      
{
    int u_index;    /* index of the created object object */
    t_binbuf *u_objectbuf;      /* the object cleared or typed into */
    t_binbuf *u_reconnectbuf;   /* connections into and out of object */
} t_undo_create;

void *canvas_undo_set_create(t_canvas *x)
{
    t_gobj *y, *last;
    t_linetraverser t;
    t_outconnect *oc;

    t_undo_create *buf = (t_undo_create *)getbytes(sizeof(*buf));
    buf->u_index = glist_getindex(x, 0) - 1;
    int nnotsel= glist_selectionindex(x, 0, 0);

    buf->u_objectbuf = binbuf_new();
	if (x->gl_list) {
		for (y = x->gl_list; y; y = y->g_next)
		{
		    if (glist_isselected(x, y)) {
		        gobj_save(y, buf->u_objectbuf);
			}
		}
	}
    buf->u_reconnectbuf = binbuf_new();
    linetraverser_start(&t, x);
	if (linetraverser_next(&t)) {
		while (oc = linetraverser_next(&t))
		{
		    int issel1 = glist_isselected(x, &t.tr_ob->ob_g);
		    int issel2 = glist_isselected(x, &t.tr_ob2->ob_g);
		    if (issel1 != issel2)
		    {
		        binbuf_addv(buf->u_reconnectbuf, "ssiiii;",
		            gensym("#X"), gensym("connect"),
		            (issel1 ? nnotsel : 0)
		                + glist_selectionindex(x, &t.tr_ob->ob_g, issel1),
		            t.tr_outno,
		            (issel2 ? nnotsel : 0) +
		                glist_selectionindex(x, &t.tr_ob2->ob_g, issel2),
		            t.tr_inno);
		    }
		}
	}
    return (buf);
}

void canvas_undo_create(t_canvas *x, void *z, int action)
{
    t_undo_create *buf = z;
    t_gobj *y;

	//fprintf(stderr,"canvas = %lx buf->u_index = %d\n", (t_int)x, buf->u_index);

    if (action == UNDO_UNDO)
    {
        glist_noselect(x);
        y = glist_nth(x, buf->u_index);
        glist_select(x, y);
        canvas_doclear(x);
    }
    else if (action == UNDO_REDO)
    {
        pd_bind(&x->gl_pd, gensym("#X"));
   		binbuf_eval(buf->u_objectbuf, 0, 0, 0);
    	pd_unbind(&x->gl_pd, gensym("#X"));
        pd_bind(&x->gl_pd, gensym("#X"));
   		binbuf_eval(buf->u_reconnectbuf, 0, 0, 0);
    	pd_unbind(&x->gl_pd, gensym("#X"));
        y = glist_nth(x, buf->u_index);
        glist_select(x, y);
    }
	else if (action == UNDO_FREE) {
		binbuf_free(buf->u_objectbuf);
		binbuf_free(buf->u_reconnectbuf);
        t_freebytes(buf, sizeof(*buf));
	}
}

/* ------------------------ event handling ------------------------ */

static char *cursorlist[] = {
    "$cursor_runmode_nothing",
    "$cursor_runmode_clickme",
    "$cursor_runmode_thicken",
    "$cursor_runmode_addpoint",
    "$cursor_editmode_nothing",
    "$cursor_editmode_connect",
    "$cursor_editmode_disconnect"
};

void canvas_setcursor(t_canvas *x, unsigned int cursornum)
{
    static t_canvas *xwas;
    static unsigned int cursorwas;
    if (cursornum >= sizeof(cursorlist)/sizeof *cursorlist)
    {
	bug("canvas_setcursor");
        return;
    }
    if (xwas != x || cursorwas != cursornum)
    {
        sys_vgui("catch {.x%lx configure -cursor %s}\n", x, cursorlist[cursornum]);
        xwas = x;
        cursorwas = cursornum;
    }
}

    /* check if a point lies in a gobj.  */
int canvas_hitbox(t_canvas *x, t_gobj *y, int xpos, int ypos,
    int *x1p, int *y1p, int *x2p, int *y2p)
{
    int x1, y1, x2, y2;
    t_text *ob;
    if (!gobj_shouldvis(y, x))
        return (0);
    gobj_getrect(y, x, &x1, &y1, &x2, &y2);
    if (xpos >= x1 && xpos <= x2 && ypos >= y1 && ypos <= y2)
    {
        *x1p = x1;
        *y1p = y1;
        *x2p = x2;
        *y2p = y2;
        return (1);
    }
    else return (0);
}

    /* find the last gobj, if any, containing the point. */
static t_gobj *canvas_findhitbox(t_canvas *x, int xpos, int ypos,
    int *x1p, int *y1p, int *x2p, int *y2p)
{
    t_gobj *y, *rval = 0;
    int x1, y1, x2, y2;
    *x1p = -0x7fffffff;
    for (y = x->gl_list; y; y = y->g_next)
    {
        if (canvas_hitbox(x, y, xpos, ypos, &x1, &y1, &x2, &y2)
            && (x1 > *x1p))
                *x1p = x1, *y1p = y1, *x2p = x2, *y2p = y2, rval = y; 
    }
        /* if there are at least two selected objects, we'd prefer
        to find a selected one (never mind which) to the one we got. */
    if (x->gl_editor && x->gl_editor->e_selection &&
        x->gl_editor->e_selection->sel_next && !glist_isselected(x, y))
    {
        t_selection *sel;
        for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next)
            if (canvas_hitbox(x, sel->sel_what, xpos, ypos, &x1, &y1, &x2, &y2))
                *x1p = x1, *y1p = y1, *x2p = x2, *y2p = y2,
                    rval = sel->sel_what; 
    }
    return (rval);
}

    /* right-clicking on a canvas object pops up a menu. */
static void canvas_rightclick(t_canvas *x, int xpos, int ypos, t_gobj *y)
{
    int canprop, canopen, isobject;
	/* abstractions should only allow for properties inside them 
	   otherwise they end-up being dirty without visible notification
	   besides, why would one mess with their properties without
	   seeing what is inside them? CURRENTLY DISABLED */
    canprop = (!y || (y && class_getpropertiesfn(pd_class(&y->g_pd))) /*&& !canvas_isabstraction( ((t_glist*)y) )*/ );
    canopen = (y && zgetfn(&y->g_pd, gensym("menu-open")));
	if (y || x->gl_editor->e_selection) {
		isobject = 1;
	}
	else isobject = 0;
    sys_vgui("pdtk_canvas_popup .x%lx %d %d %d %d %d\n",
        x, xpos, ypos, canprop, canopen, isobject);
}

/* ----  editors -- perhaps this and "vis" should go to g_editor.c ------- */

static t_editor *editor_new(t_glist *owner)
{
    char buf[40];
    t_editor *x = (t_editor *)getbytes(sizeof(*x));
    x->e_connectbuf = binbuf_new();
    x->e_deleted = binbuf_new();
    x->e_glist = owner;
    sprintf(buf, ".x%lx", (t_int)owner);
    x->e_guiconnect = guiconnect_new(&owner->gl_pd, gensym(buf));
    return (x);
}

static void editor_free(t_editor *x, t_glist *y)
{
    glist_noselect(y);
    guiconnect_notarget(x->e_guiconnect, 1000);
    binbuf_free(x->e_connectbuf);
    binbuf_free(x->e_deleted);
    freebytes((void *)x, sizeof(*x));
}

    /* recursively create or destroy all editors of a glist and its 
    sub-glists, as long as they aren't toplevels. */
void canvas_create_editor(t_glist *x)
{
    t_gobj *y;
    t_object *ob;
    if (!x->gl_editor)
    {
        x->gl_editor = editor_new(x);
        for (y = x->gl_list; y; y = y->g_next)
            if (ob = pd_checkobject(&y->g_pd))
                rtext_new(x, ob);
    }
}

void canvas_destroy_editor(t_glist *x)
{
    t_gobj *y;
    t_object *ob;
	glist_noselect(x);
    if (x->gl_editor)
    {
		if (x->gl_list) {
        	for (y = x->gl_list; y; y = y->g_next)
        	    if (ob = pd_checkobject(&y->g_pd))
        	        rtext_free(glist_findrtext(x, ob));
		}
		//if (x->gl_editor) {
		editor_free(x->gl_editor, x);
		x->gl_editor = 0;
		//}
	}
}

void canvas_reflecttitle(t_canvas *x);
void canvas_map(t_canvas *x, t_floatarg f);

    /* we call this when we want the window to become visible, mapped, and
    in front of all windows; or with "f" zero, when we want to get rid of
    the window. */
//extern t_array *garray_getarray(t_garray *x);
//extern void garray_fittograph(t_garray *x, int n);
//extern t_rtext *glist_findrtext(t_glist *gl, t_text *who);
//extern void rtext_gettext(t_rtext *x, char **buf, int *bufsize);

void canvas_vis(t_canvas *x, t_floatarg f)
{
	//fprintf(stderr,"canvas_vis .x%lx %f\n", (t_int)x, f);
    char buf[30];
    int flag = (f != 0);
    if (x != glist_getcanvas(x) && glist_isvisible(glist_getcanvas(x))) {
        bug("canvas_vis");
		fprintf(stderr,"canvas_vis .x%lx .x%lx %f\n", (t_int)x, (t_int)glist_getcanvas(x), f);
	}
    if (flag)
    {
        /* post("havewindow %d, isgraph %d, isvisible %d  editor %d",
            x->gl_havewindow, x->gl_isgraph, glist_isvisible(x),
                (x->gl_editor != 0)); */
            /* test if we're already visible and toplevel */
        if (x->gl_editor)
        {           /* just put us in front */
#ifdef MSW
            canvas_vis(x, 0);
            canvas_vis(x, 1);
#else
            sys_vgui("raise .x%lx\n", x);
            sys_vgui("focus .x%lx.c\n", x);
            sys_vgui("wm deiconify .x%lx\n", x);
#endif
        }
        else
        {
			//fprintf(stderr,"new window\n");
            canvas_create_editor(x);
            sys_vgui("catch {pdtk_canvas_new .x%lx %d %d +%d+%d %d}\n", x,
                (int)(x->gl_screenx2 - x->gl_screenx1),
                (int)(x->gl_screeny2 - x->gl_screeny1),
                (int)(x->gl_screenx1), (int)(x->gl_screeny1),
                x->gl_edit);
            canvas_reflecttitle(x);
            x->gl_havewindow = 1;

/*
			//newly opened arrays created prior to pd-l2ork require fittograph
			t_gobj *g, *gg = NULL;
			t_garray *ga = NULL;
			t_array *a = NULL;
			int  num_elem = 0;
			t_text *t = NULL;
			int objnamesize;
			char *objname;

			for (g = x->gl_list; g; g = g->g_next) {
				//fprintf(stderr, "searching\n");

				//for subpatch garrays
				if (pd_class(&g->g_pd) == garray_class) {
					//fprintf(stderr,"found ya\n");
					ga = (t_garray *)g;
					if (ga) {
						a = garray_getarray(ga);
						num_elem = a->a_n;
						garray_fittograph(ga, num_elem);
					}
				}
				//for garrays in gop inside a newly opened patch window
				else {
					t_text *t = (t_text *)g;
					t_rtext *y = glist_findrtext(x, t);
					if (y) {
						rtext_gettext(y, &objname, &objnamesize);
						//fprintf(stderr,"objname %s\n", objname);					
						if (!strcmp(objname, "graph ")) {
							//fprintf(stderr,"got a graph\n");
							for (gg = ((t_glist *)g)->gl_list; gg; gg = gg->g_next) {
								//fprintf(stderr, "sub-searching\n");
								if (pd_class(&gg->g_pd) == garray_class) {
									//fprintf(stderr,"sub found ya\n");
									ga = (t_garray *)gg;
									if (ga) {
										a = garray_getarray(ga);
										num_elem = a->a_n;
										garray_fittograph(ga, num_elem);
										canvas_dirty(x, 2);
									}
								}
							}					
						}
						//else fprintf(stderr,"fail %d >%s< >graph<\n", strcmp(objname, "graph"), objname);
					}
				}
			}
*/
            canvas_updatewindowlist();
        }
    }
    else    /* make invisible */
    {
        int i;
        t_canvas *x2;
        if (!x->gl_havewindow)
        {
                /* bug workaround -- a graph in a visible patch gets "invised"
                when the patch is closed, and must lose the editor here.  It's
                probably not the natural place to do this.  Other cases like
                subpatches fall here too but don'd need the editor freed, so
                we check if it exists. */
            if (x->gl_editor)
                canvas_destroy_editor(x);
            return;
        }
        sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
        glist_noselect(x);
        if (glist_isvisible(x))
            canvas_map(x, 0);
        canvas_destroy_editor(x);
        sys_vgui("destroy .x%lx\n", x);
        for (i = 1, x2 = x; x2; x2 = x2->gl_next, i++)
            ;
        //sys_vgui(".mbar.find delete %d\n", i);
            /* if we're a graph on our parent, and if the parent exists
               and is visible, show ourselves on parent. */
        if (glist_isgraph(x) && x->gl_owner)
        {
            t_glist *gl2 = x->gl_owner;
            if (glist_isvisible(gl2))
                gobj_vis(&x->gl_gobj, gl2, 0);
            x->gl_havewindow = 0;
            if (glist_isvisible(gl2))
                gobj_vis(&x->gl_gobj, gl2, 1);
        }
        else x->gl_havewindow = 0;
        canvas_updatewindowlist();
    }
}

    /* set a canvas up as a graph-on-parent.  Set reasonable defaults for
    any missing paramters and redraw things if necessary. */
void canvas_setgraph(t_glist *x, int flag, int nogoprect)
{
	//fprintf(stderr,"flag=%d\n",flag);
    if (!flag && glist_isgraph(x))
    {
        int hadeditor = (x->gl_editor != 0);
        if (x->gl_owner && !x->gl_loading && glist_isvisible(x->gl_owner))
            gobj_vis(&x->gl_gobj, x->gl_owner, 0);
        if (hadeditor)
            canvas_destroy_editor(x);
        x->gl_isgraph = 0;
		x->gl_hidetext = 0;
        if (x->gl_owner && !x->gl_loading && glist_isvisible(x->gl_owner))
        {
            gobj_vis(&x->gl_gobj, x->gl_owner, 1);
            canvas_fixlinesfor(x->gl_owner, &x->gl_obj);
        }
		x->gl_goprect = 0;
    }
    else if (flag)
    {
        if (x->gl_pixwidth <= 0)
            x->gl_pixwidth = GLIST_DEFGRAPHWIDTH;

        if (x->gl_pixheight <= 0)
            x->gl_pixheight = GLIST_DEFGRAPHHEIGHT;

        if (x->gl_owner && !x->gl_loading && glist_isvisible(x->gl_owner))
		{
            gobj_vis(&x->gl_gobj, x->gl_owner, 0);
		}
        x->gl_isgraph = 1;
        x->gl_hidetext = !(!(flag&2));
        if (!nogoprect && !x->gl_goprect)
        {
			/* Ivica Ico Bukvic 5/16/10 <ico@bukvic.net> */
			x->gl_goprect = 1;
        }
        if (glist_isvisible(x) && x->gl_goprect) {
            glist_redraw(x);
		}
        if (x->gl_owner && !x->gl_loading && glist_isvisible(x->gl_owner))
        {
            gobj_vis(&x->gl_gobj, x->gl_owner, 1);
            canvas_fixlinesfor(x->gl_owner, &x->gl_obj);
        }
    }
}

void garray_properties(t_garray *x, t_glist *canvas);

    /* tell GUI to create a properties dialog on the canvas.  We tell
    the user the negative of the "pixel" y scale to make it appear to grow
    naturally upward, whereas pixels grow downward. */
void canvas_properties(t_glist *x)
{
    t_gobj *y;
    char graphbuf[200];
    if (glist_isgraph(x) != 0)
        sprintf(graphbuf,
            "pdtk_canvas_dialog %%s %g %g %d %g %g %g %g %d %d %d %d\n",
                0., 0.,
                glist_isgraph(x) ,//1,
                x->gl_x1, x->gl_y1, x->gl_x2, x->gl_y2, 
                (int)x->gl_pixwidth, (int)x->gl_pixheight,
                (int)x->gl_xmargin, (int)x->gl_ymargin);
    else sprintf(graphbuf,
            "pdtk_canvas_dialog %%s %g %g %d %g %g %g %g %d %d %d %d\n",
                glist_dpixtodx(x, 1), -glist_dpixtody(x, 1),
                0,
                0., -1., 1., 1., 
                (int)x->gl_pixwidth, (int)x->gl_pixheight,
                (int)x->gl_xmargin, (int)x->gl_ymargin);
    gfxstub_new(&x->gl_pd, x, graphbuf);
        /* if any arrays are in the graph, put out their dialogs too */
    for (y = x->gl_list; y; y = y->g_next)
        if (pd_class(&y->g_pd) == garray_class) 
            garray_properties((t_garray *)y, x);
}

    /* called from the gui when "OK" is selected on the canvas properties
        dialog.  Again we negate "y" scale. */
static void canvas_donecanvasdialog(t_glist *x,
    t_symbol *s, int argc, t_atom *argv)
{
    t_float xperpix, yperpix, x1, y1, x2, y2, xpix, ypix, xmargin, ymargin;
	int rx1=0, ry1=0, rx2=0, ry2=0; //for getrect
    int graphme, redraw = 0;

    xperpix = atom_getfloatarg(0, argc, argv);
    yperpix = atom_getfloatarg(1, argc, argv);
    graphme = (int)(atom_getfloatarg(2, argc, argv));
	//fprintf(stderr,"graphme=%d\n", graphme);
    x1 = atom_getfloatarg(3, argc, argv);
    y1 = atom_getfloatarg(4, argc, argv);
    x2 = atom_getfloatarg(5, argc, argv);
    y2 = atom_getfloatarg(6, argc, argv);
    xpix = atom_getfloatarg(7, argc, argv);
    ypix = atom_getfloatarg(8, argc, argv);
    xmargin = atom_getfloatarg(9, argc, argv);
    ymargin = atom_getfloatarg(10, argc, argv);

	/* parent windows are treated differently than applies to individual objects */
	if (glist_getcanvas(x) != x && !canvas_isabstraction(x)) {
		canvas_apply_setundo(glist_getcanvas(x), (t_gobj *)x);
	}
	else /*if (x1!=x->gl_x1 || x2!=x->gl_x2 || y1!=x->gl_y1 || y2!=x->gl_y2 ||
			graphme!=(x->gl_isgraph+2*x->gl_hidetext) || x->gl_pixwidth!=xpix ||
			x->gl_pixheight!=ypix || x->gl_xmargin!=xmargin || x->gl_ymargin!=ymargin) {*/
	{	
		canvas_canvas_setundo(x);
		//fprintf(stderr,"canvas_apply_undo\n");
	}

    x->gl_pixwidth = xpix;
    x->gl_pixheight = ypix;
    x->gl_xmargin = xmargin;
    x->gl_ymargin = ymargin;

    yperpix = -yperpix;
    if (xperpix == 0)
        xperpix = 1;
    if (yperpix == 0)
        yperpix = 1;

    if (graphme)
    {
        if (x1 != x2)
            x->gl_x1 = x1, x->gl_x2 = x2;
        else x->gl_x1 = 0, x->gl_x2 = 1;
        if (y1 != y2)
            x->gl_y1 = y1, x->gl_y2 = y2;
        else x->gl_y1 = 0, x->gl_y2 = 1;
    }
    else
    {
        if (xperpix != glist_dpixtodx(x, 1) || yperpix != glist_dpixtody(x, 1))
            redraw = 1;
        if (xperpix > 0)
        {
            x->gl_x1 = 0;
            x->gl_x2 = xperpix;
        }
        else
        {
            x->gl_x1 = -xperpix * (x->gl_screenx2 - x->gl_screenx1);
            x->gl_x2 = x->gl_x1 + xperpix;
        }
        if (yperpix > 0)
        {
            x->gl_y1 = 0;
            x->gl_y2 = yperpix;
        }
        else
        {
            x->gl_y1 = -yperpix * (x->gl_screeny2 - x->gl_screeny1);
            x->gl_y2 = x->gl_y1 + yperpix;
        }
    }

        /* LATER avoid doing 2 redraws here (possibly one inside setgraph) */
    canvas_setgraph(x, graphme, 0);
    canvas_dirty(x, 1);

	// make sure gop is never smaller than its text
	// if one wants smaller gop window, make sure to disable text
	if (x->gl_isgraph && !x->gl_hidetext && x->gl_owner) {
		//fprintf(stderr, "check size\n");
		gobj_getrect((t_gobj*)x, x->gl_owner, &rx1, &ry1, &rx2, &ry2);
		//fprintf(stderr,"%d %d %d %d\n", rx1, rx2, ry1, ry2);
		if (rx2-rx1 > x->gl_pixwidth) {
			x->gl_pixwidth = rx2-rx1;
			//fprintf(stderr,"change width\n");
		}
		if (ry2-ry1 > x->gl_pixheight) {
			x->gl_pixheight = ry2-ry1;
			//fprintf(stderr,"change height\n");
		}
	}

    if (x->gl_havewindow) {
		//fprintf(stderr,"donecanvasdialog canvas_redraw\n");
        canvas_redraw(x);
	}
    else if (x->gl_owner && glist_isvisible(x->gl_owner))
    {
		glist_noselect(x);
        gobj_vis(&x->gl_gobj, x->gl_owner, 0);
        gobj_vis(&x->gl_gobj, x->gl_owner, 1);
		canvas_redraw(x->gl_owner);
    }
	//ico@bukvic.net 100518 update scrollbars when GOP potentially exceeds window size
    t_canvas *canvas=(t_canvas *)glist_getcanvas(x);
	//if gop is being disabled go one level up (if u can)
	if (!graphme && canvas->gl_owner) canvas=canvas->gl_owner;
	sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", (t_int)canvas);
}

/* called by undo/redo arrange and done_canvas_popup. only done_canvas_popup
   checks if it is a valid action and activates undo option */
static void canvas_doarrange(t_canvas *x, t_float which, t_gobj *oldy, t_gobj *oldy_prev, t_gobj *oldy_next)
{
	t_gobj *y_begin = x->gl_list;
	t_gobj *y_end = glist_nth(x, glist_getindex(x,0) - 1);

	if (which == 3) /* to front */
	{
		//put the object at the end of the cue
		y_end->g_next = oldy;
		oldy->g_next = NULL;

		// now fix links in the hole made in the list due to moving of the oldy
		// (we know there is oldy_next as y_end != oldy in canvas_done_popup)
		if (oldy_prev) //there is indeed more before the oldy position
			oldy_prev->g_next = oldy_next;
		else x->gl_list = oldy_next;

		// and finally redraw
		canvas_redraw(x);	
	}
	if (which == 4) /* to back */
	{
		x->gl_list = oldy; //put it to the beginning of the cue
		oldy->g_next = y_begin; //make it point to the old beginning

		// now fix links in the hole made in the list due to moving of the oldy
		// (we know there is oldy_prev as y_begin != oldy in canvas_done_popup)
		if (oldy_next) //there is indeed more after oldy position
			oldy_prev->g_next = oldy_next;
		else oldy_prev->g_next = NULL; //oldy was the last in the cue

		// and finally redraw
		canvas_redraw(x);
	}
	canvas_dirty(x, 1);
}

    /* called from the gui when a popup menu comes back with "properties,"
        "open," or "help." */
	/* Ivica Ico Bukvic <ico@bukvic.net> 2010-11-17
	   also added "To Front" and "To Back" */
static void canvas_done_popup(t_canvas *x, t_float which, t_float xpos, t_float ypos)
{
    char pathbuf[FILENAME_MAX], namebuf[FILENAME_MAX];
    t_gobj *y=NULL, *oldy=NULL, *oldy_prev=NULL, *oldy_next=NULL, *y_begin, *y_end=NULL;
	int x1, y1, x2, y2;

	// first deselect any objects that may be already selected if doing action 3 or 4
	if (which == 3 || which == 4) {
		if (x->gl_editor->e_selection && x->gl_editor->e_selection->sel_next)
			glist_noselect(x);
	}
	else glist_noselect(x);

	// mark the beginning of the glist for front/back
	y_begin = x->gl_list;

	if (which == 3 || which == 4) {
		// if no object has been selected for to-front/back action
		if (!x->gl_editor->e_selection) {
			//fprintf(stderr,"doing hitbox\n");
			for (y = x->gl_list; y; y = y->g_next) {
				if (canvas_hitbox(x, y, xpos, ypos, &x1, &y1, &x2, &y2)) {
					if (!x->gl_edit)
						canvas_editmode(x, 1);
					if (!glist_isselected(x, y))
						glist_select(x, y);
				}
			}
			// this was a bogus/unsupported call--get me out of here!
			if (!x->gl_editor->e_selection) {
				post("To front/back action could not be performed because multiple items were selected...");
				return;
			}
		}
	}
	
    for (y = x->gl_list; y; y = y->g_next)
    {
		if (which == 3 || which == 4) /* to-front or to-back */
		{
			if (!x->gl_edit)
				canvas_editmode(x, 1);

			// if next one is the one selected for moving
			if (y->g_next && glist_isselected(x, y->g_next)) {
				oldy_prev = y;
				oldy = y->g_next;
				//if there is more after the selected object
				if (oldy->g_next)
					oldy_next = oldy->g_next;
			}
			else if (glist_isselected(x, y) && oldy == NULL) {
				//selected obj is the first in the cue
				oldy = y;
				if (y->g_next)
					oldy_next = y->g_next;
			}
		}
        if (canvas_hitbox(x, y, xpos, ypos, &x1, &y1, &x2, &y2))
        {
            if (which == 0)     /* properties */
            {
                if (!class_getpropertiesfn(pd_class(&y->g_pd)))
                    continue;
				else {
					if (!x->gl_edit)
						canvas_editmode(x, 1);
					if (!glist_isselected(x, y))
						glist_select(x, y);
                	(*class_getpropertiesfn(pd_class(&y->g_pd)))(y, x);
				}
                return;
            }
            else if (which == 1)    /* open */
            {
                if (!zgetfn(&y->g_pd, gensym("menu-open")))
                    continue;
                vmess(&y->g_pd, gensym("menu-open"), "");
                return;
            }
            else if (which == 2)   /* help */
            {
                char *dir;
                if (pd_class(&y->g_pd) == canvas_class &&
                    canvas_isabstraction((t_canvas *)y))
                {
                    t_object *ob = (t_object *)y;
                    int ac = binbuf_getnatom(ob->te_binbuf);
                    t_atom *av = binbuf_getvec(ob->te_binbuf);
                    if (ac < 1)
                        return;
                    atom_string(av, namebuf, FILENAME_MAX);
                    dir = canvas_getdir((t_canvas *)y)->s_name;
                }
                else
                {
                    strcpy(namebuf, class_gethelpname(pd_class(&y->g_pd)));
                    dir = class_gethelpdir(pd_class(&y->g_pd));
                }
                if (strlen(namebuf) < 4 ||
                    strcmp(namebuf + strlen(namebuf) - 3, ".pd"))
                        strcat(namebuf, ".pd");
                open_via_helppath(namebuf, dir);
                return;
            }
        }
    }

	y_end = glist_nth(x, glist_getindex(x,0) - 1);

	if (which == 3 && y_end != oldy) /* to front */
	{
		/* create appropriate undo action */
		canvas_arrange_setundo(x, oldy, 1);

		canvas_doarrange(x, which, oldy, oldy_prev, oldy_next);
	}
	if (which == 4 && y_begin != oldy) /* to back */
	{
		/* create appropriate undo action */
		canvas_arrange_setundo(x, oldy, 0);

		canvas_doarrange(x, which, oldy, oldy_prev, oldy_next);
	}
    if (which == 0) {
		if (!x->gl_edit)
			canvas_editmode(x, 1);
        canvas_properties(x);
	}
    else if (which == 2)
        open_via_helppath("intro.pd", canvas_getdir((t_canvas *)x)->s_name);
}

#define NOMOD 0
#define SHIFTMOD 1
#define CTRLMOD 2
#define ALTMOD 4
#define RIGHTCLICK 8

static double canvas_upclicktime;
static int canvas_upx, canvas_upy;
#define DCLICKINTERVAL 0.25

    /* mouse click */
void canvas_doclick(t_canvas *x, int xpos, int ypos, int which,
    int mod, int doit)
{
	/* here we make global array_garray pointer defined in g_canvas.h
	   point back to nothing--we use this pointer to pass information
	   to array_motion so that we can update corresponding send when
	   the array has been changed */
	array_garray = NULL;

    t_gobj *y;
    int shiftmod, runmode, altmod, doublemod = 0, rightclick;
    int x1=0, y1=0, x2=0, y2=0, clickreturned = 0;

	//fprintf(stderr,"canvas_doclick\n");
    
    if (!x->gl_editor)
    {
        bug("editor");
        return;
    }
    
    shiftmod = (mod & SHIFTMOD);
    runmode = ((mod & CTRLMOD) || (!x->gl_edit));
    altmod = (mod & ALTMOD);
    rightclick = (mod & RIGHTCLICK);

    canvas_undo_already_set_move = 0;

            /* if keyboard was grabbed, notify grabber and cancel the grab */
    if (doit && x->gl_editor->e_grab && x->gl_editor->e_keyfn)
    {
        (* x->gl_editor->e_keyfn) (x->gl_editor->e_grab, 0);
        glist_grab(x, 0, 0, 0, 0, 0);
    }

    if (doit && !runmode && xpos == canvas_upx && ypos == canvas_upy &&
        sys_getrealtime() - canvas_upclicktime < DCLICKINTERVAL)
            doublemod = 1;
    x->gl_editor->e_lastmoved = 0;
    if (doit)
    {
		if (x->gl_editor->e_onmotion == MA_MOVE) {		
			//fprintf(stderr,"letting go of objects\n");
        	sys_vgui(".x%lx.c raise all_cords\n", x);
			sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
		}
        x->gl_editor->e_grab = 0;
        x->gl_editor->e_onmotion = MA_NONE;
    }
    /* post("click %d %d %d %d", xpos, ypos, which, mod); */
    
    if (x->gl_editor->e_onmotion != MA_NONE) {
		//fprintf(stderr,"onmotion != MA_NONE\n");
        return;
	}

    x->gl_editor->e_xwas = xpos;
    x->gl_editor->e_ywas = ypos;
	//fprintf(stderr,"mouse %d %d\n", xpos, ypos);

    if (runmode && !rightclick)
    {
        for (y = x->gl_list; y; y = y->g_next)
        {
                /* check if the object wants to be clicked */
            if (canvas_hitbox(x, y, xpos, ypos, &x1, &y1, &x2, &y2)
                && (clickreturned = gobj_click(y, x, xpos, ypos,
                    shiftmod, ((mod & CTRLMOD) && (!x->gl_edit)) || altmod,
                        0, doit)))
                            break;
        }
        if (!doit)
        {
            if (y)
                canvas_setcursor(x, clickreturned);
            else canvas_setcursor(x, CURSOR_RUNMODE_NOTHING);
        }
        return;
    }
        /* if not a runmode left click, fall here. */
    if (y = canvas_findhitbox(x, xpos, ypos, &x1, &y1, &x2, &y2))
    {
        t_object *ob = pd_checkobject(&y->g_pd);
            /* check you're in the rectangle */
        ob = pd_checkobject(&y->g_pd);
        if (rightclick)
            canvas_rightclick(x, xpos, ypos, y);
        else if (shiftmod)
        {
            if (doit)
            {
                t_rtext *rt;
                if (ob && (rt = x->gl_editor->e_textedfor) &&
                    rt == glist_findrtext(x, ob))
                {
                    rtext_mouse(rt, xpos - x1, ypos - y1, RTEXT_SHIFT);
                    x->gl_editor->e_onmotion = MA_DRAGTEXT;
                    x->gl_editor->e_xwas = x1;
                    x->gl_editor->e_ywas = y1;
                }
                else
                {
                    if (glist_isselected(x, y))
                        glist_deselect(x, y);
                    else glist_select(x, y);
                }
            }
        }
        else
        {
                /* look for an outlet */
            int noutlet;
            if (ob && (noutlet = obj_noutlets(ob)) && ypos >= y2-4)
            {
                int width = x2 - x1;
                int nout1 = (noutlet > 1 ? noutlet - 1 : 1);
                int closest = ((xpos-x1) * (nout1) + width/2)/width;
                int hotspot = x1 +
                    (width - IOWIDTH) * closest / (nout1);
                if (closest < noutlet &&
                    xpos >= (hotspot-1) && xpos <= hotspot + (IOWIDTH+1))
                {
                    if (doit)
                    {
                        int issignal = obj_issignaloutlet(ob, closest);
                        x->gl_editor->e_onmotion = MA_CONNECT;
                        x->gl_editor->e_xwas = xpos;
                        x->gl_editor->e_ywas = ypos;
                        sys_vgui(
                          ".x%lx.c create line %d %d %d %d -width %d -tags x\n",
                                x, xpos, ypos, xpos, ypos,
                                    (issignal ? 2 : 1));
                    }   
    	    	    else
                    // jsarlo
                    {
               	        t_rtext *y = glist_findrtext(x, (t_text *)&ob->ob_g);

                        if (canvas_cnct_outlet_tag[0] != 0)
                        {
                            sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
                                   	x, canvas_cnct_outlet_tag,
									(last_outlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
									(outlet_issignal ? "$signal_nlet" : "$msg_nlet"));
                        }
                        if (y)
                        {
							last_outlet_filter = gobj_filter_highlight_behavior(y);
                            sprintf(canvas_cnct_outlet_tag, 
                                    "%so%d",
                                    rtext_gettag(y),
                                    closest);
                            sys_vgui(".x%x.c itemconfigure %s -outline $select_nlet_color -width $highlight_width\n",
                                     x,
                                     canvas_cnct_outlet_tag);
                            //sys_vgui(".x%x.c raise %s\n",
                            //         x,
                            //         canvas_cnct_outlet_tag);
							outlet_issignal = obj_issignaloutlet(ob,closest);
                        }
                        // jsarlo
						if(x->gl_magic_glass) {
	                        magicGlass_unbind(x->gl_magic_glass);
	                        magicGlass_hide(x->gl_magic_glass);
						}
                        // end jsarlo
                        canvas_setcursor(x, CURSOR_EDITMODE_CONNECT);
                    }
                    // end jsarlo
                }
                else if (doit)
                    goto nooutletafterall;
            }
                /* not in an outlet; select and move */
            else if (doit)
            {
                t_rtext *rt;
                    /* check if the box is being text edited */
            nooutletafterall:
                if (ob && (rt = x->gl_editor->e_textedfor) &&
                    rt == glist_findrtext(x, ob))
                {
                    rtext_mouse(rt, xpos - x1, ypos - y1,
                        (doublemod ? RTEXT_DBL : RTEXT_DOWN));
                    x->gl_editor->e_onmotion = MA_DRAGTEXT;
                    x->gl_editor->e_xwas = x1;
                    x->gl_editor->e_ywas = y1;
                }
                else
                {
                        /* otherwise select and drag to displace */
                    if (!glist_isselected(x, y))
                    {
                        glist_noselect(x);
                        glist_select(x, y);
                    }
					//toggle_moving = 1;
					//sys_vgui("pdtk_update_xy_tooltip .x%lx %d %d\n", x, (int)xpos, (int)ypos);
					//sys_vgui("pdtk_toggle_xy_tooltip .x%lx %d\n", x, 1);
                    x->gl_editor->e_onmotion = MA_MOVE;
                }
            }
    	    else
            // jsarlo 
            {
                if (canvas_cnct_outlet_tag[0] != 0)
                {
                    sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
                           	x, canvas_cnct_outlet_tag,
							(last_outlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
							(outlet_issignal ? "$signal_nlet" : "$msg_nlet"));
                    canvas_cnct_outlet_tag[0] = 0;                  
                }
				if(x->gl_magic_glass) {              
                	magicGlass_unbind(x->gl_magic_glass);
                	magicGlass_hide(x->gl_magic_glass);
				}
                canvas_setcursor(x, CURSOR_EDITMODE_NOTHING); 
    	    }
            // end jsarlo
        }
        return;
    }
        /* if right click doesn't hit any boxes, call rightclick
            routine anyway */
    if (rightclick)
        canvas_rightclick(x, xpos, ypos, 0);

        /* if not an editing action, and if we didn't hit a
        box, set cursor and return */
    if (runmode || rightclick)
    {
        canvas_setcursor(x, CURSOR_RUNMODE_NOTHING);
        return;
    }
        /* having failed to find a box, we try lines now. */
    if (!runmode && !altmod && !shiftmod)
    {
        t_linetraverser t;
        t_outconnect *oc;
        t_float fx = xpos, fy = ypos;
        t_glist *glist2 = glist_getcanvas(x);
        linetraverser_start(&t, glist2);
        while (oc = linetraverser_next(&t))
        {
            // jsarlo
            int parseOutno;
            t_object *parseOb = NULL;
            t_outlet *parseOutlet = NULL;
            // end jsarlo
            t_float lx1 = t.tr_lx1, ly1 = t.tr_ly1,
                lx2 = t.tr_lx2, ly2 = t.tr_ly2;
            t_float area = (lx2 - lx1) * (fy - ly1) -
                (ly2 - ly1) * (fx - lx1);
            t_float dsquare = (lx2-lx1) * (lx2-lx1) + (ly2-ly1) * (ly2-ly1);
            if (area * area >= 50 * dsquare) continue;
            if ((lx2-lx1) * (fx-lx1) + (ly2-ly1) * (fy-ly1) < 0) continue;
            if ((lx2-lx1) * (lx2-fx) + (ly2-ly1) * (ly2-fy) < 0) continue;
            if (doit)
            {
                glist_selectline(glist2, oc, 
                    canvas_getindex(glist2, &t.tr_ob->ob_g), t.tr_outno,
                    canvas_getindex(glist2, &t.tr_ob2->ob_g), t.tr_inno);
            }
            // jsarlo
            parseOutno = t.tr_outno;
            parseOb = t.tr_ob;
            for (parseOutlet = parseOb->ob_outlet; 
                 parseOutlet && parseOutno; 
                 parseOutlet = parseOutlet->o_next, parseOutno--);
            if (parseOutlet && magicGlass_isOn(x->gl_magic_glass))
            {
                magicGlass_bind(x->gl_magic_glass,
                                t.tr_ob,
                                t.tr_outno); 
                magicGlass_setDsp(x->gl_magic_glass,
                                  obj_issignaloutlet(t.tr_ob, t.tr_outno));
            }
            magicGlass_moveText(x->gl_magic_glass, xpos, ypos); 
            if (magicGlass_isOn(x->gl_magic_glass))
                magicGlass_show(x->gl_magic_glass);
            if (canvas_cnct_inlet_tag[0] != 0)
            {
				sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
		       			x, canvas_cnct_inlet_tag,
						(last_inlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
						(inlet_issignal ? "$signal_nlet" : "$msg_nlet"));
                canvas_cnct_inlet_tag[0] = 0;                  
            }
            if (canvas_cnct_outlet_tag[0] != 0)
            {
                sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
                       	x, canvas_cnct_outlet_tag,
						(last_outlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
						(outlet_issignal ? "$signal_nlet" : "$msg_nlet"));
                canvas_cnct_outlet_tag[0] = 0;                  
            }
            // end jsarlo
            canvas_setcursor(x, CURSOR_EDITMODE_DISCONNECT);
            return;
        }
    }
    // jsarlo
    if (canvas_cnct_outlet_tag[0] != 0)
    {
        sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
               	x, canvas_cnct_outlet_tag,
				(last_outlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
				(outlet_issignal ? "$signal_nlet" : "$msg_nlet"));
        canvas_cnct_outlet_tag[0] = 0;                  
    }
	if(x->gl_magic_glass) {
    	magicGlass_unbind(x->gl_magic_glass);
    	magicGlass_hide(x->gl_magic_glass);
	}
    // end jsarlo
    canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
    if (doit)
    {
        if (!shiftmod) glist_noselect(x);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags x -outline $select_color\n",
              x, xpos, ypos, xpos, ypos);
        x->gl_editor->e_xwas = xpos;
        x->gl_editor->e_ywas = ypos;
        x->gl_editor->e_onmotion = MA_REGION;
    }
}

void canvas_mousedown(t_canvas *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg which, t_floatarg mod)
{
    canvas_doclick(x, xpos, ypos, which, mod, 1);
}

int canvas_isconnected (t_canvas *x, t_text *ob1, int n1,
    t_text *ob2, int n2)
{
    t_linetraverser t;
    t_outconnect *oc;
    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
        if (t.tr_ob == ob1 && t.tr_outno == n1 &&
            t.tr_ob2 == ob2 && t.tr_inno == n2) 
                return (1);
    return (0);
}

void canvas_doconnect(t_canvas *x, int xpos, int ypos, int which, int doit)
{
    int x11=0, y11=0, x12=0, y12=0;
    t_gobj *y1;
    int x21=0, y21=0, x22=0, y22=0;
    t_gobj *y2;
    int xwas = x->gl_editor->e_xwas,
        ywas = x->gl_editor->e_ywas;
    if (doit) sys_vgui(".x%lx.c delete x\n", x);
    else sys_vgui(".x%lx.c coords x %d %d %d %d\n",
            x, x->gl_editor->e_xwas,
                x->gl_editor->e_ywas, xpos, ypos);

    if ((y1 = canvas_findhitbox(x, xwas, ywas, &x11, &y11, &x12, &y12))
        && (y2 = canvas_findhitbox(x, xpos, ypos, &x21, &y21, &x22, &y22)))
    {
        t_object *ob1 = pd_checkobject(&y1->g_pd);
        t_object *ob2 = pd_checkobject(&y2->g_pd);
        int noutlet1, ninlet2;
        if (ob1 && ob2 && ob1 != ob2 &&
            (noutlet1 = obj_noutlets(ob1))
            && (ninlet2 = obj_ninlets(ob2)))
        {
            int width1 = x12 - x11, closest1, hotspot1;
            int width2 = x22 - x21, closest2, hotspot2;
            int lx1, lx2, ly1, ly2;
            t_outconnect *oc;

            if (noutlet1 > 1)
            {
                closest1 = ((xwas-x11) * (noutlet1-1) + width1/2)/width1;
                hotspot1 = x11 +
                    (width1 - IOWIDTH) * closest1 / (noutlet1-1);
            }
            else closest1 = 0, hotspot1 = x11;

            if (ninlet2 > 1)
            {
                closest2 = ((xpos-x21) * (ninlet2-1) + width2/2)/width2;
                hotspot2 = x21 +
                    (width2 - IOWIDTH) * closest2 / (ninlet2-1);
            }
            else closest2 = 0, hotspot2 = x21;

            if (closest1 >= noutlet1)
                closest1 = noutlet1 - 1;
            if (closest2 >= ninlet2)
                closest2 = ninlet2 - 1;

            if (canvas_isconnected (x, ob1, closest1, ob2, closest2))
            {
                // jsarlo
				if(x->gl_magic_glass) {                
					magicGlass_unbind(x->gl_magic_glass);
                	magicGlass_hide(x->gl_magic_glass);
				}
                // end jsarlo
                canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
                return;
            }
            if (obj_issignaloutlet(ob1, closest1) &&
                !obj_issignalinlet(ob2, closest2))
            {
                if (doit)
                    error("can't connect signal outlet to control inlet");
                // jsarlo
               	if(x->gl_magic_glass) {
					magicGlass_unbind(x->gl_magic_glass);
	                magicGlass_hide(x->gl_magic_glass);
				}
                // end jsarlo
                canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
                return;
            }
            if (doit)
            {
                int issignal = obj_issignaloutlet(ob1, closest1);
                oc = obj_connect(ob1, closest1, ob2, closest2);
                lx1 = x11 + (noutlet1 > 1 ?
                        ((x12-x11-IOWIDTH) * closest1)/(noutlet1-1) : 0)
                             + IOMIDDLE;
                ly1 = y12;
                lx2 = x21 + (ninlet2 > 1 ?
                        ((x22-x21-IOWIDTH) * closest2)/(ninlet2-1) : 0)
                            + IOMIDDLE;
                ly2 = y21;
                sys_vgui(".x%lx.c create line %d %d %d %d -fill %s -width %d -tags {l%lx all_cords}\n",
                    glist_getcanvas(x),
                        lx1, ly1, lx2, ly2,
                    (issignal ? "$signal_cord" : "$msg_cord"),
                    (issignal ? 2 : 1), 
                    oc);
                if (canvas_cnct_inlet_tag[0] != 0)
                {
                    sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
                           	x, canvas_cnct_inlet_tag,
							(last_inlet_filter ? "black" : (obj_issignaloutlet(ob1, closest1) ? "$signal_cord" : "$msg_cord")),
							(inlet_issignal ? "$signal_nlet" : "$msg_nlet"));
                    canvas_cnct_inlet_tag[0] = 0;                  
                }
                if (canvas_cnct_outlet_tag[0] != 0)
                {
                    sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
                           	x, canvas_cnct_outlet_tag,
							(last_outlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
							(outlet_issignal ? "$signal_nlet" : "$msg_nlet"));
                    canvas_cnct_outlet_tag[0] = 0;                  
                }
                // end jsarlo
                canvas_dirty(x, 1);
                canvas_setundo(x, canvas_undo_connect,
                    canvas_undo_set_connect(x, 
                        canvas_getindex(x, &ob1->ob_g), closest1,
                        canvas_getindex(x, &ob2->ob_g), closest2),
                        "connect");
            }
    	    else 
            // jsarlo
            {
         		t_rtext *y = glist_findrtext(x, (t_text *)&ob2->ob_g);
                if (canvas_cnct_inlet_tag[0] != 0)
                {
                    sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
                           	x, canvas_cnct_inlet_tag,
							(last_inlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
							(inlet_issignal ? "$signal_nlet" : "$msg_nlet"));                
                }
                if (y)
                {
					last_inlet_filter = gobj_filter_highlight_behavior(y);
                    sprintf(canvas_cnct_inlet_tag, 
                            "%si%d",
                            rtext_gettag(y),
                            closest2);
                    sys_vgui(".x%x.c itemconfigure %s -outline $select_nlet_color -width $highlight_width\n",
                             x,
                             canvas_cnct_inlet_tag);
                    //sys_vgui(".x%x.c raise %s\n",
                    //         x,
                    //         canvas_cnct_inlet_tag);
					inlet_issignal = obj_issignalinlet(ob2, closest2);
                }
                canvas_setcursor(x, CURSOR_EDITMODE_CONNECT);
            }
            // end jsarlo
            return;
    	}
    }
    // jsarlo
    if (canvas_cnct_inlet_tag[0] != 0)
    {
        sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
               	x, canvas_cnct_inlet_tag,
				(last_inlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
				(inlet_issignal ? "$signal_nlet" : "$msg_nlet"));               
    }
	if(x->gl_magic_glass) {
    	magicGlass_unbind(x->gl_magic_glass);
    	magicGlass_hide(x->gl_magic_glass);
	}
    // end jsarlo
    canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
}

void canvas_selectinrect(t_canvas *x, int lox, int loy, int hix, int hiy)
{
    t_gobj *y;
    for (y = x->gl_list; y; y = y->g_next)
    {
        int x1, y1, x2, y2;
        gobj_getrect(y, x, &x1, &y1, &x2, &y2);
        if (hix >= x1 && lox <= x2 && hiy >= y1 && loy <= y2) {
			if (!glist_isselected(x, y))
		    	glist_select(x, y);
			else glist_deselect(x, y);
		}
    }
}

static void canvas_doregion(t_canvas *x, int xpos, int ypos, int doit)
{
    if (doit)
    {
        int lox, loy, hix, hiy;
        if (x->gl_editor->e_xwas < xpos)
            lox = x->gl_editor->e_xwas, hix = xpos;
        else hix = x->gl_editor->e_xwas, lox = xpos;
        if (x->gl_editor->e_ywas < ypos)
            loy = x->gl_editor->e_ywas, hiy = ypos;
        else hiy = x->gl_editor->e_ywas, loy = ypos;
        canvas_selectinrect(x, lox, loy, hix, hiy);
        sys_vgui(".x%lx.c delete x\n", x);
        x->gl_editor->e_onmotion = MA_NONE;
    }
    else sys_vgui(".x%lx.c coords x %d %d %d %d\n",
            x, x->gl_editor->e_xwas,
                x->gl_editor->e_ywas, xpos, ypos);
}

/*
static void canvas_mouseup_gop(t_canvas *x, t_gobj *g) {

	//simulate clearing and recreating object
	gobj_activate(g, x, 1);
	t_object *ob = pd_checkobject(&g->g_pd);
	t_rtext *yyyy = glist_findrtext(x, (t_text *)&ob->ob_g);
	//copy current text
    char *buf;
    int bufsize;
    rtext_gettext(yyyy, &buf, &bufsize);
	//fprintf(stderr, ">%s<\n", buf);
	rtext_key(yyyy, 127, NULL);

	//recreate object with no args
	glist_deselect(x, g);

	//object was recreated, so now it is latest in the queue
	t_gobj *z = x->gl_list;

	while (z->g_next) {
		z = z->g_next;
	}

	glist_select(x, z);	
	g = z;	
	ob = pd_checkobject(&g->g_pd);
	yyyy = glist_findrtext(x, (t_text *)&ob->ob_g);

	//redo the old text
	int i;
	for (i = 0; i < bufsize; i++) {
		rtext_key(yyyy, (int)buf[i], NULL);
	}
	gobj_activate(z, x, 1);
	x->gl_editor->e_textdirty = 1;
}
*/

void canvas_mouseup(t_canvas *x,
    t_floatarg fxpos, t_floatarg fypos, t_floatarg fwhich)
{
	//if (toggle_moving == 1) {
	//	toggle_moving = 0;
	//	sys_vgui("pdtk_toggle_xy_tooltip .x%lx %d\n", x, 0);
	//}
    int xpos = fxpos, ypos = fypos, which = fwhich;
    /* post("mouseup %d %d %d", xpos, ypos, which); */
    if (!x->gl_editor)
    {
        bug("editor");
        return;
    }

    canvas_upclicktime = sys_getrealtime();
    canvas_upx = xpos;
    canvas_upy = ypos;

    if (x->gl_editor->e_onmotion == MA_CONNECT)
        canvas_doconnect(x, xpos, ypos, which, 1);
    else if (x->gl_editor->e_onmotion == MA_REGION)
        canvas_doregion(x, xpos, ypos, 1);
    else if (x->gl_editor->e_onmotion == MA_MOVE)
    {
            /* after motion, if there's only one item selected, activate it */
        if (x->gl_editor->e_selection &&
            !(x->gl_editor->e_selection->sel_next))
        {
            t_gobj *g = x->gl_editor->e_selection->sel_what;
            t_glist *gl2;
                /* first though, check we aren't an abstraction with a
                dirty sub-patch that would be discarded if we edit this. */
            if (pd_class(&g->g_pd) == canvas_class &&
                canvas_isabstraction((t_glist *)g) &&
                    (gl2 = glist_finddirty((t_glist *)g)))
            {
                vmess(&gl2->gl_pd, gensym("menu-open"), "");
                x->gl_editor->e_onmotion = MA_NONE;
                sys_vgui(
					"pdtk_check .x%lx {Discard changes to '%s'?} {.x%lx dirty 0;\n} no\n",
                    canvas_getrootfor(gl2),
                    canvas_getrootfor(gl2)->gl_name->s_name, gl2);
                return;
            }
            /* OK, activate it */

			/*
			// but before we do, check if this is GOP and adjust accordingly
			//fprintf(stderr,"activate...");
			if (pd_class(&g->g_pd) == canvas_class && 
				((t_glist *)g)->gl_isgraph &&
				canvas_isabstraction((t_glist *)g))
			{
				//fprintf(stderr,"gop...");
				// if mouse has not moved AND this object does not have its text hidden (otherwise we only translate the selection)
				if (!(((t_glist *)g)->gl_hidetext) && !x->gl_editor->e_lastmoved) {
					//fprintf(stderr,"yes\n");
					canvas_mouseup_gop(x, x->gl_editor->e_selection->sel_what);
				}
				//else fprintf(stderr,"no\n");
			}
			// else if it is a regular object
			*/
			else {
				//fprintf(stderr,"reg_obj\n");
				gobj_activate(x->gl_editor->e_selection->sel_what, x, 1);
			}
        }
		sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
    }

	if (canvas_cnct_outlet_tag[0] != 0)
	{
		sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
		       	x, canvas_cnct_outlet_tag,
				(last_outlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
				(outlet_issignal ? "$signal_nlet" : "$msg_nlet"));
	}
	if (canvas_cnct_inlet_tag[0] != 0)
	{
		sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
	   			x, canvas_cnct_inlet_tag,
				(last_inlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
				(inlet_issignal ? "$signal_nlet" : "$msg_nlet"));
		canvas_cnct_inlet_tag[0] = 0;                  
	}
    
    x->gl_editor->e_onmotion = MA_NONE;
}

    /* displace the selection by (dx, dy) pixels */
static void canvas_displaceselection(t_canvas *x, int dx, int dy)
{
    t_selection *y;
    int resortin = 0, resortout = 0;
    if (!canvas_undo_already_set_move)
    {
        canvas_setundo(x, canvas_undo_move, canvas_undo_set_move(x, 1),
            "motion");
        canvas_undo_already_set_move = 1;
    }
    for (y = x->gl_editor->e_selection; y; y = y->sel_next)
    {
		/* for the time being let's discern from vanilla objects and those that don't conform */
		t_glist *yglist = (t_glist *)(y->sel_what);
		if (y->sel_what->g_pd->c_wb && y->sel_what->g_pd->c_wb->w_displacefnwtag) {
			/* this is a vanilla object */
			gobj_displace_withtag(y->sel_what, x, dx, dy);
		}
		else {
			/* we will move the non-conforming objects the old way */
			gobj_displace(y->sel_what, x, dx, dy);
		}
        t_class *cl = pd_class(&y->sel_what->g_pd);
        if (cl == vinlet_class) resortin = 1;
        else if (cl == voutlet_class) resortout = 1;
    }
	if (dx || dy) {
		sys_vgui(".x%lx.c move selected %d %d\n", x, dx, dy);
	    if (resortin) canvas_resortinlets(x);
	    if (resortout) canvas_resortoutlets(x);
	    //sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
	    if (x->gl_editor->e_selection)
	        canvas_dirty(x, 1);
	}
}

    /* this routine is called whenever a key is pressed or released.  "x"
    may be zero if there's no current canvas.  The first argument is true or
    false for down/up; the second one is either a symbolic key name (e.g.,
    "Right" or an Ascii key number.  The third is the shift key. */
void canvas_key(t_canvas *x, t_symbol *s, int ac, t_atom *av)
{
    static t_symbol *keynumsym, *keyupsym, *keynamesym;
    int keynum, fflag;
    t_symbol *gotkeysym;
        
    int down, shift;
    
    if (ac < 3)
        return;
    if (!x || !x->gl_editor)
        return;
    canvas_undo_already_set_move = 0;
    down = (atom_getfloat(av) != 0);  /* nonzero if it's a key down */
    shift = (atom_getfloat(av+2) != 0);  /* nonzero if shift-ed */
    if (av[1].a_type == A_SYMBOL) {
        gotkeysym = av[1].a_w.w_symbol;
	}
    else if (av[1].a_type == A_FLOAT)
    {
        char buf[3];
        switch((int)(av[1].a_w.w_float))
        {
        case 8:  gotkeysym = gensym("BackSpace"); break;
        case 9:  gotkeysym = gensym("Tab"); break;
        case 10: gotkeysym = gensym("Return"); break;
        case 27: gotkeysym = gensym("Escape"); break;
        case 32: gotkeysym = gensym("Space"); break;
        case 127:gotkeysym = gensym("Delete"); break;
        default:
            sprintf(buf, "%c", (int)(av[1].a_w.w_float));
            gotkeysym = gensym(buf);
        }
    }
    else gotkeysym = gensym("?");
    fflag = (av[0].a_type == A_FLOAT ? av[0].a_w.w_float : 0);
    keynum = (av[1].a_type == A_FLOAT ? av[1].a_w.w_float : 0);
    if (keynum == '\\' || keynum == '{' || keynum == '}')
    {
        post("keycode %d: dropped", (int)keynum);
        return;
    }
#if 0
    post("keynum %d, down %d, gotkeysym %s", (int)keynum, down, gotkeysym->s_name);
#endif
    if (keynum == '\r') keynum = '\n';
    if (av[1].a_type == A_SYMBOL &&
        !strcmp(av[1].a_w.w_symbol->s_name, "Return"))
            keynum = '\n';
    if (!keynumsym)
    {
        keynumsym = gensym("#key");
        keyupsym = gensym("#keyup");
        keynamesym = gensym("#keyname");
    }
#ifdef __APPLE__
        if (keynum == 30)
            keynum = 0, gotkeysym = gensym("Up");
        else if (keynum == 31)
            keynum = 0, gotkeysym = gensym("Down");
        else if (keynum == 28)
            keynum = 0, gotkeysym = gensym("Left");
        else if (keynum == 29)
            keynum = 0, gotkeysym = gensym("Right");
#endif
    if (keynumsym->s_thing && down)
        pd_float(keynumsym->s_thing, (t_float)keynum);
    if (keyupsym->s_thing && !down)
        pd_float(keyupsym->s_thing, (t_float)keynum);
    if (keynamesym->s_thing)
    {
        t_atom at[2];
        at[0] = av[0];
        SETFLOAT(at, down);
        SETSYMBOL(at+1, gotkeysym);
        pd_list(keynamesym->s_thing, 0, 2, at);
    }
    if (!x->gl_editor)  /* if that 'invis'ed the window, we'd better stop. */
        return;
    if (x && down)
    {
        t_object *ob;
            /* cancel any dragging action */
        if (x->gl_editor->e_onmotion == MA_MOVE)
            x->gl_editor->e_onmotion = MA_NONE;
            /* if an object has "grabbed" keys just send them on */
        if (x->gl_editor->e_grab
            && x->gl_editor->e_keyfn && keynum)
                (* x->gl_editor->e_keyfn)
                    (x->gl_editor->e_grab, (t_float)keynum);
            /* if a text editor is open send the key on, as long as
            it is either "real" (has a key number) or else is an arrow key. */
        else if (x->gl_editor->e_textedfor && (keynum
            || !strcmp(gotkeysym->s_name, "Up")
            || !strcmp(gotkeysym->s_name, "Down")
            || !strcmp(gotkeysym->s_name, "Left")
            || !strcmp(gotkeysym->s_name, "Right")
			|| !strcmp(gotkeysym->s_name, "CtrlHome")
			|| !strcmp(gotkeysym->s_name, "CtrlEnd")
			|| !strcmp(gotkeysym->s_name, "Home")
			|| !strcmp(gotkeysym->s_name, "End")))
        {
                /* send the key to the box's editor */
            if (!x->gl_editor->e_textdirty)
            {
                canvas_setundo(x, canvas_undo_cut,
                    canvas_undo_set_cut(x, UCUT_TEXT), "typing");
            }
            rtext_key(x->gl_editor->e_textedfor,
                (int)keynum, gotkeysym);
			canvas_fixlinesfor(x, (t_text *)(x->gl_editor->e_selection->sel_what));
            if (x->gl_editor->e_textdirty)
                canvas_dirty(x, 1);
        }
            /* check for backspace or clear */
        else if (keynum == 8 || keynum == 127)
        {
            if (x->gl_editor->e_selectedline)
                canvas_clearline(x);
            else if (x->gl_editor->e_selection)
            {
                canvas_setundo(x, canvas_undo_cut,
                    canvas_undo_set_cut(x, UCUT_CLEAR), "clear");
                canvas_doclear(x);
            }
        }
                /* check for arrow keys */
		else if (x->gl_editor->e_selection) {
		    if (!strcmp(gotkeysym->s_name, "Up")) {
		        canvas_displaceselection(x, 0, shift ? -10 : -1);
				sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
			}
		    else if (!strcmp(gotkeysym->s_name, "Down")) {
		        canvas_displaceselection(x, 0, shift ? 10 : 1);
				sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
			}
		    else if (!strcmp(gotkeysym->s_name, "Left")) {
		        canvas_displaceselection(x, shift ? -10 : -1, 0);
				sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
			}
		    else if (!strcmp(gotkeysym->s_name, "Right")) {
		        canvas_displaceselection(x, shift ? 10 : 1, 0);
				sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
			}
		}
    }

        /* if control key goes up or down, and if we're in edit mode, change
        cursor to indicate how the click action changes */
    if (x && keynum == 0 && x->gl_edit &&
        !strncmp(gotkeysym->s_name, "Control", 7))
            canvas_setcursor(x, down ?
                CURSOR_RUNMODE_NOTHING : CURSOR_EDITMODE_NOTHING);
}

void canvas_motion(t_canvas *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg fmod)
{ 
    //fprintf(stderr,"motion %d %d\n", (int)xpos, (int)ypos);
    int mod = fmod;
    if (!x->gl_editor)
    {
        bug("editor");
        return;
    }
    glist_setlastxy(x, xpos, ypos);
    if (x->gl_editor->e_onmotion == MA_MOVE)
    {
        canvas_displaceselection(x, 
            xpos - x->gl_editor->e_xwas, ypos - x->gl_editor->e_ywas);
        x->gl_editor->e_xwas = xpos;
        x->gl_editor->e_ywas = ypos;    
    }
    else if (x->gl_editor->e_onmotion == MA_REGION)
        canvas_doregion(x, xpos, ypos, 0);
    else if (x->gl_editor->e_onmotion == MA_CONNECT) {
		//fprintf(stderr,"MA_CONNECT\n");
        canvas_doconnect(x, xpos, ypos, 0, 0);
	}
    else if (x->gl_editor->e_onmotion == MA_PASSOUT)
    {
        if (!x->gl_editor->e_motionfn)
            bug("e_motionfn");
        (*x->gl_editor->e_motionfn)(&x->gl_editor->e_grab->g_pd,
            xpos - x->gl_editor->e_xwas,
            ypos - x->gl_editor->e_ywas);
        x->gl_editor->e_xwas = xpos;
        x->gl_editor->e_ywas = ypos;
    }
    else if (x->gl_editor->e_onmotion == MA_DRAGTEXT)
    {
        t_rtext *rt = x->gl_editor->e_textedfor;
        if (rt)
            rtext_mouse(rt, xpos - x->gl_editor->e_xwas,
                ypos - x->gl_editor->e_ywas, RTEXT_DRAG);
    }
    else canvas_doclick(x, xpos, ypos, 0, mod, 0);
	//if (toggle_moving == 1) {
	//	sys_vgui("pdtk_update_xy_tooltip .x%lx %d %d\n", x, (int)xpos, (int)ypos);
	//}
    x->gl_editor->e_lastmoved = 1;
}

void canvas_startmotion(t_canvas *x)
{
	//fprintf(stderr,"canvas_startmotion\n");
    int xval, yval;
    if (!x->gl_editor) return;
    glist_getnextxy(x, &xval, &yval);
    if (xval == 0 && yval == 0) return;
    x->gl_editor->e_onmotion = MA_MOVE;
    x->gl_editor->e_xwas = xval;
    x->gl_editor->e_ywas = yval; 
}

/* ----------------------------- window stuff ----------------------- */
extern int sys_perf;

void canvas_print(t_canvas *x, t_symbol *s)
{
    if (*s->s_name) sys_vgui(".x%lx.c postscript -file %s\n", x, s->s_name);
    else sys_vgui(".x%lx.c postscript -file x.ps\n", x);
}

    /* find a dirty sub-glist, if any, of this one (including itself) */
static t_glist *glist_finddirty(t_glist *x)
{
    t_gobj *g;
    t_glist *g2;
    if (x->gl_env && x->gl_dirty)
        return (x);
    for (g = x->gl_list; g; g = g->g_next)
        if (pd_class(&g->g_pd) == canvas_class &&
            (g2 = glist_finddirty((t_glist *)g)))
                return (g2);
    return (0);
}

    /* quit, after calling glist_finddirty() on all toplevels and verifying
    the user really wants to discard changes  */
void glob_verifyquit(void *dummy, t_floatarg f)
{
    t_glist *g, *g2;
        /* find all root canvases */
    for (g = canvas_list; g; g = g->gl_next)
        if (g2 = glist_finddirty(g))
        {
			/* first open window */
			if (!glist_istoplevel(g2) && g2->gl_env) {
				/* if this is an abstraction */
            	vmess(&g2->gl_pd, gensym("menu-open"), "");
			} else {
				/* is this even necessary? */
	            canvas_vis(g2, 1);
			}
			if (!glist_istoplevel(g2) && g->gl_env) {
				/* if this is an abstraction */
            	sys_vgui("pdtk_canvas_menuclose .x%lx {.x%lx menuclose 3;}\n",
                     g2, g2);
			} else {
            	sys_vgui("pdtk_canvas_menuclose .x%lx {.x%lx menuclose 3;}\n",
                     canvas_getrootfor(g2), g2);
			}
            //canvas_vis(g2, 1);
            //sys_vgui("pdtk_canvas_menuclose .x%lx {.x%lx menuclose 3;\n}\n",
            //         canvas_getrootfor(g2), g2);
        return;
    }
    if (f == 0 && sys_perf)
        sys_vgui("pdtk_check . {really quit?} {pd quit;\n} yes\n");
    else glob_quit(0);
}

//void canvas_dofree(t_gobj *dummy, t_glist *x)
//{
	//int dspstate = canvas_suspend_dsp();
	//sys_flushqueue();
	//pd_free(&x->gl_pd);
	//canvas_resume_dsp(dspstate);
//}

    /* close a window (or possibly quit Pd), checking for dirty flags.
    The "force" parameter is interpreted as follows:
        0 - request from GUI to close, verifying whether clean or dirty
        1 - request from GUI to close, no verification
        2 - verified - mark this one clean, then continue as in 1
        3 - verified - mark this one clean, then verify-and-quit
    */
void canvas_menuclose(t_canvas *x, t_floatarg fforce)
{
    int force = fforce;
    t_glist *g;
	if (x->gl_owner && (force == 0 || force == 1))
        canvas_vis(x, 0);   /* if subpatch, just invis it */
    else if (force == 0)    
    {
        g = glist_finddirty(x);
        if (g)
        {
			/* first open window */
			if (!glist_istoplevel(g) && g->gl_env) {
				/* if this is an abstraction */
            	vmess(&g->gl_pd, gensym("menu-open"), "");
			} else {
				/* is this even necessary? */
	            canvas_vis(g, 1);
			}
			if (!glist_istoplevel(g) && g->gl_env) {
				/* if this is an abstraction */
            	sys_vgui("pdtk_canvas_menuclose .x%lx {.x%lx menuclose 2;}\n",
                     g, g);
			} else {
            	sys_vgui("pdtk_canvas_menuclose .x%lx {.x%lx menuclose 2;}\n",
                     canvas_getrootfor(g), g);
			}
            return;
        }
/*
        else if (sys_perf)
        {
            sys_vgui(
"pdtk_check .x%lx {Close '%s'?} {.x%lx menuclose 1;\n} yes\n",
                canvas_getrootfor(x), canvas_getrootfor(x)->gl_name->s_name, x);
        }
*/
        else pd_free(&x->gl_pd);
			//sys_queuegui(x, x, canvas_dofree);
			//clock_delay(x->gl_destroy, 0);
    }
    else if (force == 1) {
		//sys_vgui("pd {.x%lx menuclose -1;}\n", x);
		//sys_vgui("menu_close .x%lx\n", x);
		//sys_queuegui(x, x, canvas_dofree);
		//canvas_vis(x, 0);
		//canvas_free(x);
		pd_free(&x->gl_pd);
		//fprintf(stderr,"pd_free queued------------\n");
		//clock_delay(x->gl_destroy, 0);
	}
    else if (force == 2)
    {
        canvas_dirty(x, 0);
        while (x->gl_owner)
            x = x->gl_owner;
        g = glist_finddirty(x);
        if (g)
        {
            vmess(&g->gl_pd, gensym("menu-open"), "");
			if (!glist_istoplevel(g) && g->gl_env) {
				/* if this is an abstraction */
            	sys_vgui("pdtk_canvas_menuclose .x%lx {.x%lx menuclose 2;}\n",
                     g, g);
			} else {
            	sys_vgui("pdtk_canvas_menuclose .x%lx {.x%lx menuclose 2;}\n",
                     canvas_getrootfor(g), g);
			}
            //sys_vgui("pdtk_canvas_menuclose .x%lx {.x%lx menuclose 2;\n}\n",
            //         canvas_getrootfor(x), g);
            return;
        }
        else pd_free(&x->gl_pd);
			//sys_vgui("pd {.x%lx menuclose -1;}\n", x);
			//sys_queuegui(x, x, canvas_dofree);
			//clock_delay(x->gl_destroy, 0);
    }
    else if (force == 3)
    {
        canvas_dirty(x, 0);
        glob_verifyquit(0, 1);
    }
}

    /* put up a dialog which may call canvas_font back to do the work */
static void canvas_menufont(t_canvas *x)
{
    char buf[80];
    t_canvas *x2 = canvas_getrootfor(x);
    gfxstub_deleteforkey(x2);
    sprintf(buf, "pdtk_canvas_dofont %%s %d\n", x2->gl_font);
    gfxstub_new(&x2->gl_pd, &x2->gl_pd, buf);
}

static int canvas_find_index1, canvas_find_index2, canvas_find_wholeword;
static t_binbuf *canvas_findbuf;
int binbuf_match(t_binbuf *inbuf, t_binbuf *searchbuf, int wholeword);

    /* find an atom or string of atoms */
static int canvas_dofind(t_canvas *x, int *myindex1p)
{
    t_gobj *y;
    int myindex1 = *myindex1p, myindex2;
    if (myindex1 >= canvas_find_index1)
    {
        for (y = x->gl_list, myindex2 = 0; y;
            y = y->g_next, myindex2++)
        {
            t_object *ob = 0;
            if (ob = pd_checkobject(&y->g_pd))
            {
                if (binbuf_match(ob->ob_binbuf, canvas_findbuf,
                    canvas_find_wholeword))
                {
                    if (myindex1 > canvas_find_index1 ||
                        myindex1 == canvas_find_index1 &&
                            myindex2 > canvas_find_index2)
                    {
                        canvas_find_index1 = myindex1;
                        canvas_find_index2 = myindex2;
                        glist_noselect(x);
                        vmess(&x->gl_pd, gensym("menu-open"), "");
                        canvas_editmode(x, 1.);
                        glist_select(x, y);
                        return (1);
                    }
                }
            }
        }
    }
    for (y = x->gl_list, myindex2 = 0; y; y = y->g_next, myindex2++)
    {
        if (pd_class(&y->g_pd) == canvas_class)
        {
            (*myindex1p)++;
            if (canvas_dofind((t_canvas *)y, myindex1p))
                return (1);
        }
    }
    return (0);
}

static void canvas_find(t_canvas *x, t_symbol *s, t_floatarg wholeword)
{
    int myindex1 = 0;
    t_symbol *decodedsym = sys_decodedialog(s);
    if (!canvas_findbuf)
        canvas_findbuf = binbuf_new();
    binbuf_text(canvas_findbuf, decodedsym->s_name, strlen(decodedsym->s_name));
    canvas_find_index1 = 0;
    canvas_find_index2 = -1;
    canvas_find_wholeword = wholeword;
    canvas_whichfind = x;
    if (!canvas_dofind(x, &myindex1))
    {
        binbuf_print(canvas_findbuf);
        post("... couldn't find");
    }
}

static void canvas_find_again(t_canvas *x)
{
    int myindex1 = 0;
    if (!canvas_findbuf || !canvas_whichfind)
        return;
    if (!canvas_dofind(canvas_whichfind, &myindex1))
    {
        binbuf_print(canvas_findbuf);
        post("... couldn't find");
    }
}

static void canvas_find_parent(t_canvas *x)
{
    if (x->gl_owner)
        canvas_vis(glist_getcanvas(x->gl_owner), 1);
}

static int glist_dofinderror(t_glist *gl, void *error_object)
{
    t_gobj *g;
    for (g = gl->gl_list; g; g = g->g_next)
    {
        if ((void *)g == error_object)
        {
            /* got it... now show it. */
            glist_noselect(gl);
            canvas_vis(glist_getcanvas(gl), 1);
            canvas_editmode(glist_getcanvas(gl), 1.);
            glist_select(gl, g);
            return (1);
        }
        else if (g->g_pd == canvas_class)
        {
            if (glist_dofinderror((t_canvas *)g, error_object))
                return (1);
        }
    }
    return (0);
}

void canvas_finderror(void *error_object)
{
    t_canvas *x;
        /* find all root canvases */
    for (x = canvas_list; x; x = x->gl_next)
    {
        if (glist_dofinderror(x, error_object))
            return;
    }
    post("... sorry, I couldn't find the source of that error.");
}

void canvas_stowconnections(t_canvas *x)
{
    t_gobj *selhead = 0, *seltail = 0, *nonhead = 0, *nontail = 0, *y, *y2;
    t_linetraverser t;
    t_outconnect *oc;
    if (!x->gl_editor) return;
        /* split list to "selected" and "unselected" parts */ 
    for (y = x->gl_list; y; y = y2)
    {
        y2 = y->g_next;
        if (glist_isselected(x, y))
        {
            if (seltail)
            {
                seltail->g_next = y;
                seltail = y;
                y->g_next = 0;
            }
            else
            {
                selhead = seltail = y;
                seltail->g_next = 0;
            }
        }
        else
        {
            if (nontail)
            {
                nontail->g_next = y;
                nontail = y;
                y->g_next = 0;
            }
            else
            {
                nonhead = nontail = y;
                nontail->g_next = 0;
            }
        }
    }
        /* move the selected part to the end */
    if (!nonhead) x->gl_list = selhead;
    else x->gl_list = nonhead, nontail->g_next = selhead;

        /* add connections to binbuf */
    binbuf_clear(x->gl_editor->e_connectbuf);
    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
    {
        int s1 = glist_isselected(x, &t.tr_ob->ob_g);
        int s2 = glist_isselected(x, &t.tr_ob2->ob_g);
        if (s1 != s2)
            binbuf_addv(x->gl_editor->e_connectbuf, "ssiiii;",
                gensym("#X"), gensym("connect"),
                    glist_getindex(x, &t.tr_ob->ob_g), t.tr_outno,
                        glist_getindex(x, &t.tr_ob2->ob_g), t.tr_inno);
    }
}

void canvas_restoreconnections(t_canvas *x)
{
    pd_bind(&x->gl_pd, gensym("#X"));
    binbuf_eval(x->gl_editor->e_connectbuf, 0, 0, 0);
    pd_unbind(&x->gl_pd, gensym("#X"));
}

static t_binbuf *canvas_docopy(t_canvas *x)
{
	//fprintf(stderr,"canvas_docopy\n");
    t_gobj *y, *last;
    t_linetraverser t;
    t_outconnect *oc;
    t_binbuf *b = binbuf_new();
    for (y = x->gl_list; y; y = y->g_next)
    {
        if (glist_isselected(x, y)) {
			//fprintf(stderr,"saving object\n");
            gobj_save(y, b);
		}
    }
    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
    {
		//fprintf(stderr,"found some lines %d %d\n", glist_isselected(x, &t.tr_ob->ob_g), glist_isselected(x, &t.tr_ob2->ob_g));
        if (glist_isselected(x, &t.tr_ob->ob_g)
            && glist_isselected(x, &t.tr_ob2->ob_g))
        {
			//fprintf(stderr,"saving lines leading into selected object\n");
            binbuf_addv(b, "ssiiii;", gensym("#X"), gensym("connect"),
                glist_selectionindex(x, &t.tr_ob->ob_g, 1), t.tr_outno,
                glist_selectionindex(x, &t.tr_ob2->ob_g, 1), t.tr_inno);
        }
    }
    return (b);
}

static void canvas_copyfromexternalbuffer(t_canvas *x, t_symbol *s, int ac, t_atom *av)
{
	if (!x->gl_editor)
		return;

	if (ac == 0) {
		//fprintf(stderr,"init\n");
		copyfromexternalbuffer = 1;
		screenx1 = 0;
		screeny1 = 0;
		screenx2 = 0;
		screeny2 = 0;
		copiedfont = 0;
		binbuf_free(copy_binbuf);
		copy_binbuf = binbuf_new();
	} else if (copyfromexternalbuffer) {
		//fprintf(stderr,"fill %d\n", ac);
		if (av[0].a_type == A_SYMBOL && strcmp(av[0].a_w.w_symbol->s_name, "#N")) {
			binbuf_add(copy_binbuf, ac, av);
			binbuf_addsemi(copy_binbuf);
		} else if (ac == 7) {
			int check = 0;
			//if the canvas is empty resize window size and position here...
			//fprintf(stderr,"copying canvas properties for copyfromexternalbuffer\n");
			if (av[2].a_type == A_FLOAT) {
				screenx1 = av[2].a_w.w_float;
				check++;
			}
			if (av[3].a_type == A_FLOAT) {
				screeny1 = av[3].a_w.w_float;
				check++;
			}
			if (av[4].a_type == A_FLOAT) {
				screenx2 = av[4].a_w.w_float;
				check++;
			}
			if (av[5].a_type == A_FLOAT) {
				screeny2 = av[5].a_w.w_float;
				check++;
			}
			if (av[5].a_type == A_FLOAT) {
				copiedfont = av[6].a_w.w_float;
				check++;
			}
			if (check != 5) {
				post("error copying: copyfromexternalbuffer: canvas info has invalid data\n");
				copyfromexternalbuffer = 0;
			}
		}
	}
}

static void canvas_copy(t_canvas *x)
{
    if (!x->gl_editor || !x->gl_editor->e_selection)
        return;
	copyfromexternalbuffer = 0;
	screenx1 = 0;
	screeny1 = 0;
	screenx2 = 0;
	screeny2 = 0;
	copiedfont = 0;
    binbuf_free(copy_binbuf);
	//fprintf(stderr, "canvas_copy\n");
    copy_binbuf = canvas_docopy(x);
	if (!x->gl_editor->e_selection)
		sys_vgui("pdtk_canvas_update_edit_menu .x%lx 0\n", x);
	else
		sys_vgui("pdtk_canvas_update_edit_menu .x%lx 1\n", x);
    paste_xyoffset = 1;
    if (x->gl_editor->e_textedfor)
    {
        char *buf;
        int bufsize;
        rtext_getseltext(x->gl_editor->e_textedfor, &buf, &bufsize);

//#if defined(MSW) || defined(__APPLE__)
//            /* for Mac or Windows, copy the text to the clipboard here */
        sys_vgui("clipboard clear\n", bufsize, buf);
        sys_vgui("clipboard append {%.*s}\n", bufsize, buf);
//#else
            /* in X windows the selection already went to the
            clipboard when it was made; here we "copy" it to our own buffer
            as well, because, annoyingly, the clipboard will usually be 
            destroyed by the time the user asks to "paste". */
        /*if (canvas_textcopybuf)
            t_freebytes(canvas_textcopybuf, canvas_textcopybufsize);
        canvas_textcopybuf = (char *)getbytes(bufsize);
        memcpy(canvas_textcopybuf, buf, bufsize);
        canvas_textcopybufsize = bufsize;*/
//#endif
    }
}

static void canvas_clearline(t_canvas *x)
{
    if (x->gl_editor->e_selectedline)
    {
        canvas_disconnect(x, x->gl_editor->e_selectline_index1,
             x->gl_editor->e_selectline_outno,
             x->gl_editor->e_selectline_index2,
             x->gl_editor->e_selectline_inno);
        canvas_dirty(x, 1);
        canvas_setundo(x, canvas_undo_disconnect,
            canvas_undo_set_disconnect(x,
                x->gl_editor->e_selectline_index1,
                x->gl_editor->e_selectline_outno,
                x->gl_editor->e_selectline_index2,
                x->gl_editor->e_selectline_inno),
            "disconnect");
    }
}

extern t_pd *newest;
static void canvas_doclear(t_canvas *x)
{
    t_gobj *y, *y2;
    int dspstate;

    dspstate = canvas_suspend_dsp();
    if (x->gl_editor->e_selectedline)
    {
        canvas_disconnect(x, x->gl_editor->e_selectline_index1,
             x->gl_editor->e_selectline_outno,
             x->gl_editor->e_selectline_index2,
             x->gl_editor->e_selectline_inno);
        canvas_setundo(x, canvas_undo_disconnect,
            canvas_undo_set_disconnect(x,
                x->gl_editor->e_selectline_index1,
                x->gl_editor->e_selectline_outno,
                x->gl_editor->e_selectline_index2,
                x->gl_editor->e_selectline_inno),
            "disconnect");
    }
        /* if text is selected, deselecting it might remake the
        object. So we deselect it and hunt for a "new" object on
        the glist to reselect. */
    if (x->gl_editor->e_textedfor)
    {
        newest = 0;
        glist_noselect(x);
        if (newest)
        {
            for (y = x->gl_list; y; y = y->g_next)
                if (&y->g_pd == newest) glist_select(x, y);
        }
    }
    while (1)   /* this is pretty wierd...  should rewrite it */
    {
        for (y = x->gl_list; y; y = y2)
        {
            y2 = y->g_next;
            if (glist_isselected(x, y))
            {
				/* if it is a graph and the window is open, destroy it first
				   this will avoid leaving stale gop rectangle and name */
		        if (pd_class(&y->g_pd) == canvas_class &&
		        	((t_glist *)y)->gl_havewindow)
				{
					canvas_menuclose((t_glist *)y, 0);
				}

				/* now destroy the object */
                glist_delete(x, y);
#if 0
                if (y2) post("cut 5 %lx %lx", y2, y2->g_next);
                else post("cut 6");
#endif
                goto next;
            }
        }
        goto restore;
    next: ;
    }
restore:
    canvas_dirty(x, 1);
	canvas_redraw(x);
	//sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
    canvas_resume_dsp(dspstate);
}

static void canvas_cut(t_canvas *x)
{
    if (x->gl_editor && x->gl_editor->e_selectedline)
        canvas_clearline(x);
	/* if we are cutting text */
    else if (x->gl_editor && x->gl_editor->e_textedfor)
    {
        char *buf;
        int bufsize;
        rtext_getseltext(x->gl_editor->e_textedfor, &buf, &bufsize);
        if (!bufsize)
            return;
        canvas_copy(x);
        rtext_key(x->gl_editor->e_textedfor, 127, &s_);
        canvas_dirty(x, 1);
    }
	/* else we are cutting objects */
    else if (x->gl_editor && x->gl_editor->e_selection)
    {
        canvas_setundo(x, canvas_undo_cut,
            canvas_undo_set_cut(x, UCUT_CUT), "cut");
        canvas_copy(x);
        canvas_doclear(x);
        paste_xyoffset = 0;
        sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
    }
}

static int paste_onset;
static t_canvas *paste_canvas;

static void glist_donewloadbangs(t_glist *x)
{
    if (x->gl_editor)
    {
        t_selection *sel;
        for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next)
            if (pd_class(&sel->sel_what->g_pd) == canvas_class)
                canvas_loadbang((t_canvas *)(&sel->sel_what->g_pd));
    }
}

static void canvas_paste_xyoffset(t_canvas *x)
{
    t_selection *sel;
	t_class *cl;
    int resortin = 0;
    int resortout = 0;

    for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next) {
        gobj_displace(sel->sel_what, x, paste_xyoffset*10, paste_xyoffset*10);
		cl = pd_class(&sel->sel_what->g_pd);
        if (cl == vinlet_class) resortin = 1;
        if (cl == voutlet_class) resortout = 1;
	}

    if (resortin) canvas_resortinlets(x);
    if (resortout) canvas_resortoutlets(x);

	// alternative one-line implementation that
	// replaces the entire function
	//canvas_displaceselection(x, 10, 10);

    //paste_xyoffset++; //a part of original way
}

static void canvas_paste_atmouse(t_canvas *x)
{
    t_selection *sel;
	/* use safe values for x1 and y1 which are essentially the same as xyoffset */
	int x1 = x->gl_editor->e_xwas+10, y1 = x->gl_editor->e_ywas+10, init = 0;
	t_glist *g;
	t_text *t;

	/* find the initial offset--we use leftmost object as our reference */
    for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next) {
		g = (t_glist *)sel->sel_what;
		t = (t_text *)g;
		if (!init) {
			x1 = t->te_xpix;
			y1 = t->te_ypix;
			init = 1;
		} else if ( t->te_xpix < x1 ) {
			x1 = t->te_xpix;
		}
	}
	/* redraw objects */
    canvas_displaceselection(x, (x->gl_editor->e_xwas)+5-x1, (x->gl_editor->e_ywas)-y1);
	canvas_startmotion(x);
}

extern void canvas_obj(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_howputnew(t_canvas *x, int *connectp, int *xpixp, int *ypixp,
    int *indexp, int *totalp);

static void canvas_dopaste(t_canvas *x, t_binbuf *b)
{
	//fprintf(stderr,"start dopaste\n");
	
    t_gobj *newgobj, *last, *g2;
    int dspstate = canvas_suspend_dsp(), nbox, count;
	int canvas_empty = 0;

	//first let's see if we are pasting into an empty canvas
	//this will be used below when pasting from copyfromexternalbuffer, usually text editor
	if (!x->gl_list) canvas_empty = 1;

	//autopatching variables
	int connectme, xpix, ypix, indx, nobj;
	connectme = 0;

    canvas_editmode(x, 1.);
	/*	abolish potential displacing of object that may have been
		created with the first new object on canvas, but now we are
		pasting and therefore MA_MOVE should not apply to new objects
	*/
	x->gl_editor->e_onmotion = MA_NONE;

	if (copyfromexternalbuffer && canvas_empty) {
		if (screenx2 && screeny2 && copiedfont) {
			x->gl_screenx1 = screenx1;
			x->gl_screenx2 = screenx1 + screenx2;
			x->gl_screeny1 = screeny1;
			x->gl_screeny2 = screeny1 + screeny2;
			//canvas_setbounds(x, screenx1, screeny1, screenx1+screenx2, screeny1+screeny2);
			sys_vgui("wm geometry .x%lx =%dx%d+%d+%d\n", x,
                (int)(x->gl_screenx2 - x->gl_screenx1),
                (int)(x->gl_screeny2 - x->gl_screeny1),
                (int)(x->gl_screenx1), (int)(x->gl_screeny1));
			//hardwired stretchval and whichstretch until we figure out proper resizing
			canvas_dofont(x, copiedfont, 100, 1);
			//sys_vgui("pdtk_canvas_checkgeometry .x%lx\n", x);
			canvas_redraw(x);
		}
	}

	//if we have something selected in another canvas
	if (c_selection && c_selection != x)
		glist_noselect(c_selection);
	//else is we are pasting see if we can autopatch
	else if (canvas_undo_name && !strcmp(canvas_undo_name, "paste")) {
		canvas_howputnew(x, &connectme, &xpix, &ypix, &indx, &nobj);
    	//glist_noselect(x);
	}
	//else we are duplicating
	else glist_noselect(x);

    for (g2 = x->gl_list, nbox = 0; g2; g2 = g2->g_next) nbox++;
    
	/* found the end of the queue */
    paste_onset = nbox;
    paste_canvas = x;

    pd_bind(&x->gl_pd, gensym("#X"));
    binbuf_eval(b, 0, 0, 0);
    pd_unbind(&x->gl_pd, gensym("#X"));

	/* select newly created objects */
    for (g2 = x->gl_list, count = 0; g2; g2 = g2->g_next, count++)
		if (count >= nbox)
            glist_select(x, g2);

    paste_canvas = 0;
    canvas_resume_dsp(dspstate);

	//if we are pasting only one object autoposition it below our selection
	if (count == nbox+1 && connectme) {
    	canvas_connect(x, indx, 0, nobj, 0);

		//is this universally safe? I think so
		t_text *z = (t_text *)x->gl_editor->e_selection->sel_what;
		//fprintf(stderr,"%d %d %d %d\n", z->te_xpix, z->te_ypix, xpix, ypix);

		//calculate delta (since displace is always relative)
		int delta_x = xpix - z->te_xpix;
		int delta_y = ypix - z->te_ypix;

		//now displace it but without undo
		//(by spoofing canvas_undo_already_set_move)
		canvas_undo_already_set_move = 1;
 		canvas_displaceselection(x, delta_x, delta_y);
		//reset canvas_undo_already_set_move
		canvas_undo_already_set_move = 0;
	}
	//if we are pasting into a new window and this is not copied from external buffer OR
	//if we are copying from external buffer and the current canvas is not empty
	else if (canvas_undo_name && !strcmp(canvas_undo_name, "paste") && !copyfromexternalbuffer ||
		copyfromexternalbuffer && !canvas_empty) {
		canvas_paste_atmouse(x);
		//fprintf(stderr,"doing a paste\n");
	}

    canvas_dirty(x, 1);
	/*if (!canvas_undo_name || canvas_undo_name[0] != 'd') {
		canvas_redraw(x);
	}*/
    sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
    glist_donewloadbangs(x);
}

static void canvas_paste(t_canvas *x)
{
    if (!x->gl_editor)
        return;
    if (x->gl_editor->e_textedfor)
    {
            /* simulate keystrokes as if the copy buffer were typed in. */
//#if defined(MSW) || defined(__APPLE__)
            /* for Mac or Windows,  ask the GUI to send the clipboard down */
        sys_gui("pdtk_pastetext\n");
//#else
            /* in X windows we kept the text in our own copy buffer */
/*        int i;
        for (i = 0; i < canvas_textcopybufsize; i++)
        {
            pd_vmess(&x->gl_gobj.g_pd, gensym("key"), "iii",
                1, canvas_textcopybuf[i]&0xff, 0);
        }*/
//#endif
    }
    else
    {
        canvas_setundo(x, canvas_undo_paste, canvas_undo_set_paste(x),
            "paste");
        canvas_dopaste(x, copy_binbuf);
        //canvas_paste_xyoffset(x);
    }
}

static void canvas_duplicate(t_canvas *x)
{
	//if (x->gl_editor->e_onmotion == MA_NONE && x->gl_editor->e_selection)
	if (x->gl_editor->e_onmotion == MA_NONE && c_selection && c_selection->gl_editor->e_selection)
    {
        //canvas_copy(x);
        //canvas_setundo(x, canvas_undo_paste, canvas_undo_set_paste(x),
        //    "duplicate");
        //canvas_dopaste(x, copy_binbuf);
        //canvas_paste_xyoffset(x);
        //canvas_dirty(x, 1);
        canvas_copy(c_selection);
		if (c_selection == x) {
			/* we are in the same window */
		    canvas_setundo(x, canvas_undo_paste, canvas_undo_set_paste(x),
		        "duplicate");
		    canvas_dopaste(x, copy_binbuf);
		    canvas_paste_xyoffset(x);
		    canvas_dirty(x, 1);
		} else {
		    canvas_setundo(x, canvas_undo_paste, canvas_undo_set_paste(x),
		        "duplicate");
		    canvas_dopaste(x, copy_binbuf);
		    //canvas_paste_xyoffset(x);
		    canvas_dirty(x, 1);
		}
    }
}

static void canvas_selectall(t_canvas *x)
{
    t_gobj *y;
    if (!x->gl_edit)
        canvas_editmode(x, 1);

    if (x->gl_editor && x->gl_editor->e_textedfor)
    {
        /* only do this if exactly one item is selected. */
        if (x->gl_editor->e_selection->sel_what &&
            !x->gl_editor->e_selection->sel_next)
        {
			t_gobj *z = x->gl_editor->e_selection->sel_what;
			/* reactivate */
			gobj_activate(z, x, 1);
			/* make canvas dirty in case this was done at creation time */
			x->gl_editor->e_textdirty = 1;
		}
	}
	else {

		/* if everyone is already selected deselect everyone */
		if (!glist_selectionindex(x, 0, 0))
		    glist_noselect(x);
		else for (y = x->gl_list; y; y = y->g_next)
		{
		    if (!glist_isselected(x, y))
		        glist_select(x, y);
		}
	}
}

static void canvas_reselect(t_canvas *x)
{
    t_gobj *g, *gwas;
    t_selection *sel;
    t_object *ob;
        /* if someone is text editing, and if only one object is 
        selected,  deselect everyone and reselect.  */
    if (x->gl_editor->e_textedfor)
    {
            /* only do this if exactly one item is selected. */
        if ((gwas = x->gl_editor->e_selection->sel_what) &&
            !x->gl_editor->e_selection->sel_next)
        {
            int nobjwas = glist_getindex(x, 0),
                indx = canvas_getindex(x, x->gl_editor->e_selection->sel_what);
            glist_noselect(x);
            for (g = x->gl_list; g; g = g->g_next)
                if (g == gwas)
            {
                glist_select(x, g);
                return;
            }
                /* "gwas" must have disappeared; just search to the last
                object and select it */
            for (g = x->gl_list; g; g = g->g_next)
                if (!g->g_next)
                    glist_select(x, g);
        }
    }
    else if (x->gl_editor->e_selection &&
        !x->gl_editor->e_selection->sel_next)
            /* otherwise activate first item in selection */
            gobj_activate(x->gl_editor->e_selection->sel_what, x, 1);
}

extern t_class *text_class;

void canvas_connect(t_canvas *x, t_floatarg fwhoout, t_floatarg foutno,
    t_floatarg fwhoin, t_floatarg finno)
{
	if (!x->gl_list) {
		post("paste error: no objects to connect, probably incomplete clipboard copy from an external source (e.g. from a text editor)");
		return;		
	}
    int whoout = fwhoout, outno = foutno, whoin = fwhoin, inno = finno;
    t_gobj *src = 0, *sink = 0;
    t_object *objsrc, *objsink;
    t_outconnect *oc;
    int nin = whoin, nout = whoout;
    if (paste_canvas == x) whoout += paste_onset, whoin += paste_onset;
    for (src = x->gl_list; whoout; src = src->g_next, whoout--)
        if (!src->g_next) goto bad; /* bug fix thanks to Hannes */
    for (sink = x->gl_list; whoin; sink = sink->g_next, whoin--)
        if (!sink->g_next) goto bad;
    
        /* check they're both patchable objects */
    if (!(objsrc = pd_checkobject(&src->g_pd)) ||
        !(objsink = pd_checkobject(&sink->g_pd)))
            goto bad;
    
        /* if object creation failed, make dummy inlets or outlets
        as needed */ 
    if (pd_class(&src->g_pd) == text_class && objsrc->te_type == T_OBJECT)
        while (outno >= obj_noutlets(objsrc))
            outlet_new(objsrc, 0);
    if (pd_class(&sink->g_pd) == text_class && objsink->te_type == T_OBJECT)
        while (inno >= obj_ninlets(objsink))
            inlet_new(objsink, &objsink->ob_pd, 0, 0);

    if (!(oc = obj_connect(objsrc, outno, objsink, inno))) goto bad;
    if (glist_isvisible(x))
    {
        sys_vgui(".x%lx.c create line %d %d %d %d -width %d -fill %s -tags {l%lx all_cords}\n",
            glist_getcanvas(x), 0, 0, 0, 0,
            (obj_issignaloutlet(objsrc, outno) ? 2 : 1),
            (obj_issignaloutlet(objsrc, outno) ? "$signal_cord" : "$msg_cord"), oc);
        canvas_fixlinesfor(x, objsrc);
    }
    return;

bad:
    post("%s %d %d %d %d (%s->%s) connection failed", 
        x->gl_name->s_name, nout, outno, nin, inno,
            (src? class_getname(pd_class(&src->g_pd)) : "???"),
            (sink? class_getname(pd_class(&sink->g_pd)) : "???"));
}

#define XTOLERANCE 20
#define YTOLERANCE 20
#define NHIST 15

    /* LATER might have to speed this up */
static void canvas_tidy(t_canvas *x)
{
    t_gobj *y, *y2, *y3;
    int ax1, ay1, ax2, ay2, bx1, by1, bx2, by2;
    int histogram[NHIST], *ip, i, besthist, bestdist;
        /* if nobody is selected, this means do it to all boxes;
        othewise just the selection */
    int all = (x->gl_editor ? (x->gl_editor->e_selection == 0) : 1);

    canvas_setundo(x, canvas_undo_move, canvas_undo_set_move(x, !all),
        "motion");

        /* tidy horizontally */
    for (y = x->gl_list; y; y = y->g_next)
        if (all || glist_isselected(x, y))
    {
        gobj_getrect(y, x, &ax1, &ay1, &ax2, &ay2);

        for (y2 = x->gl_list; y2; y2 = y2->g_next)
            if (all || glist_isselected(x, y2))
        {
            gobj_getrect(y2, x, &bx1, &by1, &bx2, &by2);
            if (by1 <= ay1 + YTOLERANCE && by1 >= ay1 - YTOLERANCE &&
                bx1 < ax1)
                    goto nothorizhead;
        }

        for (y2 = x->gl_list; y2; y2 = y2->g_next)
            if (all || glist_isselected(x, y2))
        {
            gobj_getrect(y2, x, &bx1, &by1, &bx2, &by2);
            if (by1 <= ay1 + YTOLERANCE && by1 >= ay1 - YTOLERANCE
                && by1 != ay1)
                    gobj_displace(y2, x, 0, ay1-by1);
        }
    nothorizhead: ;
    }
        /* tidy vertically.  First guess the user's favorite vertical spacing */
    for (i = NHIST, ip = histogram; i--; ip++) *ip = 0;
    for (y = x->gl_list; y; y = y->g_next)
        if (all || glist_isselected(x, y))
    {
        gobj_getrect(y, x, &ax1, &ay1, &ax2, &ay2);
        for (y2 = x->gl_list; y2; y2 = y2->g_next)
            if (all || glist_isselected(x, y2))
        {
            gobj_getrect(y2, x, &bx1, &by1, &bx2, &by2);
            if (bx1 <= ax1 + XTOLERANCE && bx1 >= ax1 - XTOLERANCE)
            {
                int distance = by1-ay2;
                if (distance >= 0 && distance < NHIST)
                    histogram[distance]++;
            }
        }
    }
    for (i = 1, besthist = 0, bestdist = 4, ip = histogram + 1;
        i < (NHIST-1); i++, ip++)
    {
        int hit = ip[-1] + 2 * ip[0] + ip[1];
        if (hit > besthist)
        {
            besthist = hit;
            bestdist = i;
        }
    }
    //post("best vertical distance %d", bestdist);
    for (y = x->gl_list; y; y = y->g_next)
        if (all || glist_isselected(x, y))
    {
        int keep = 1;
        gobj_getrect(y, x, &ax1, &ay1, &ax2, &ay2);
        for (y2 = x->gl_list; y2; y2 = y2->g_next)
            if (all || glist_isselected(x, y2))
        {
            gobj_getrect(y2, x, &bx1, &by1, &bx2, &by2);
            if (bx1 <= ax1 + XTOLERANCE && bx1 >= ax1 - XTOLERANCE &&
                ay1 >= by2 - 10 && ay1 < by2 + NHIST)
                    goto nothead;
        }
        while (keep)
        {
            keep = 0;
            for (y2 = x->gl_list; y2; y2 = y2->g_next)
                if (all || glist_isselected(x, y2))
            {
                gobj_getrect(y2, x, &bx1, &by1, &bx2, &by2);
                if (bx1 <= ax1 + XTOLERANCE && bx1 >= ax1 - XTOLERANCE &&
                    by1 > ay1 && by1 < ay2 + NHIST)
                {
                    int vmove = ay2 + bestdist - by1;
                    gobj_displace(y2, x, ax1-bx1, vmove);
                    ay1 = by1 + vmove;
                    ay2 = by2 + vmove;
                    keep = 1;
                    break;
                }
            }
        }
    nothead: ;
    }
    canvas_dirty(x, 1);
}

static void canvas_texteditor(t_canvas *x)
{
    t_rtext *foo;
    char *buf;
    int bufsize;
    if (foo = x->gl_editor->e_textedfor)
        rtext_gettext(foo, &buf, &bufsize);
    else buf = "", bufsize = 0;
    sys_vgui("pdtk_pd_texteditor {%.*s}\n", bufsize, buf);
    
}

void glob_key(void *dummy, t_symbol *s, int ac, t_atom *av)
{
        /* canvas_editing can be zero; canvas_key checks for that */
    canvas_key(canvas_editing, s, ac, av);
}

extern void canvas_draw_gop_resize_hooks(t_canvas *x);

void canvas_editmode(t_canvas *x, t_floatarg fyesplease)
{
	//fprintf(stderr,"canvas_editmode %f\n", fyesplease);
    int yesplease = fyesplease;
    if (yesplease && x->gl_edit) {
	    //if (x->gl_edit && glist_isvisible(x) && glist_istoplevel(x))
	    //    canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
        return;
	}
    x->gl_edit = !x->gl_edit;
    if (x->gl_edit && glist_isvisible(x) && glist_istoplevel(x)){
		//dpsaha@vt.edu add the resize blobs on GOP
		if (x->gl_goprect)	canvas_draw_gop_resize_hooks(x);
		canvas_setcursor(x, CURSOR_EDITMODE_NOTHING);
	}
    else
    {
		//fprintf(stderr,"we are out of edit\n");
        glist_noselect(x);
        if (glist_isvisible(x) && glist_istoplevel(x))
        {
            // jsarlo
            if (canvas_cnct_inlet_tag[0] != 0)
            {
                sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
                       	x, canvas_cnct_inlet_tag,
						(last_inlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
						(inlet_issignal ? "$signal_nlet" : "$msg_nlet")); 
                canvas_cnct_inlet_tag[0] = 0;                  
            }
            if (canvas_cnct_outlet_tag[0] != 0)
            {
                sys_vgui(".x%x.c itemconfigure %s -outline %s -fill %s -width 1\n",
                       	x, canvas_cnct_outlet_tag,
						(last_outlet_filter ? "black" : (outlet_issignal ? "$signal_cord" : "$msg_cord")),
						(outlet_issignal ? "$signal_nlet" : "$msg_nlet"));
                canvas_cnct_outlet_tag[0] = 0;                  
            }
			if(x->gl_magic_glass) {
            	magicGlass_unbind(x->gl_magic_glass);
            	magicGlass_hide(x->gl_magic_glass);
			}
            // end jsarlo
        }
		canvas_setcursor(x, CURSOR_RUNMODE_NOTHING);
    }
    sys_vgui("pdtk_canvas_editval .x%lx %d\n",
        glist_getcanvas(x), x->gl_edit);
	/*if (!x->gl_edit) {
		sys_vgui(".x%lx.m.edit entryconfigure \"Cord Inspector\" -indicatoron false -state disabled\n", glist_getcanvas(x));
	}
	else {
		sys_vgui(".x%lx.m.edit entryconfigure \"Cord Inspector\" -indicatoron false -state normal\n", glist_getcanvas(x));
	}*/
	//dpsaha@vt.edu called to delete the GOP_blob
	if (x->gl_goprect)		canvas_draw_gop_resize_hooks(x);
}

// jsarlo
void canvas_magicglass(t_canvas *x, t_floatarg fyesplease)
{
    int yesplease = fyesplease;
    if (yesplease && magicGlass_isOn(x->gl_magic_glass))
    	return;
    if (!magicGlass_isOn(x->gl_magic_glass)) {
		canvas_editmode(x, 1.);
        magicGlass_setOn(x->gl_magic_glass, 1);
        if (magicGlass_bound(x->gl_magic_glass))
        {
            magicGlass_show(x->gl_magic_glass);
        }
    }
    else {
        magicGlass_setOn(x->gl_magic_glass, 0);
        magicGlass_hide(x->gl_magic_glass);
    }
    sys_vgui("pdtk_canvas_magicglassval .x%x %d\n",
        glist_getcanvas(x), magicGlass_isOn(x->gl_magic_glass));
}
// end jsarlo

    /* called by canvas_font below */
static void canvas_dofont(t_canvas *x, t_floatarg font, t_floatarg xresize,
    t_floatarg yresize)
{
    t_gobj *y;
    x->gl_font = font;
    if (xresize != 1 || yresize != 1)
    {
        canvas_setundo(x, canvas_undo_move, canvas_undo_set_move(x, 0),
            "motion");
        for (y = x->gl_list; y; y = y->g_next)
        {
            int x1, x2, y1, y2, nx1, ny1;
            gobj_getrect(y, x, &x1, &y1, &x2, &y2);
            nx1 = x1 * xresize + 0.5;
            ny1 = y1 * yresize + 0.5;
            gobj_displace(y, x, nx1-x1, ny1-y1);
        }
    }
    if (glist_isvisible(x))
        glist_redraw(x);
    for (y = x->gl_list; y; y = y->g_next)
        if (pd_class(&y->g_pd) == canvas_class
            && !canvas_isabstraction((t_canvas *)y))
                canvas_dofont((t_canvas *)y, font, xresize, yresize);
	sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
}

    /* canvas_menufont calls up a TK dialog which calls this back */
static void canvas_font(t_canvas *x, t_floatarg font, t_floatarg resize,
    t_floatarg whichresize)
{
    t_float realresize, realresx = 1, realresy = 1;
    t_canvas *x2 = canvas_getrootfor(x);
    if (!resize) realresize = 1;
    else
    {
        if (resize < 20) resize = 20;
        if (resize > 500) resize = 500;
        realresize = resize * 0.01;
    }
    if (whichresize != 3) realresx = realresize;
    if (whichresize != 2) realresy = realresize;
    canvas_dofont(x2, font, realresx, realresy);
    sys_defaultfont = font;
}

static t_glist *canvas_last_glist;
static int canvas_last_glist_x, canvas_last_glist_y;

void glist_getnextxy(t_glist *gl, int *xpix, int *ypix)
{
    if (canvas_last_glist == gl)
        *xpix = canvas_last_glist_x, *ypix = canvas_last_glist_y;
    else *xpix = *ypix = 40;
}

static void glist_setlastxy(t_glist *gl, int xval, int yval)
{
    canvas_last_glist = gl;
    canvas_last_glist_x = xval;
    canvas_last_glist_y = yval;
}


void g_editor_setup(void)
{
/* ------------------------ events ---------------------------------- */
    class_addmethod(canvas_class, (t_method)canvas_mousedown, gensym("mouse"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_mouseup, gensym("mouseup"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_key, gensym("key"),
        A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_motion, gensym("motion"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);

/* ------------------------ menu actions ---------------------------- */
    class_addmethod(canvas_class, (t_method)canvas_menuclose,
        gensym("menuclose"), A_DEFFLOAT, 0);
    class_addmethod(canvas_class, (t_method)canvas_cut,
        gensym("cut"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_copy,
        gensym("copy"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_paste,
        gensym("paste"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_duplicate,
        gensym("duplicate"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_selectall,
        gensym("selectall"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_reselect,
        gensym("reselect"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_undo,
        gensym("undo"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_redo,
        gensym("redo"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_tidy,
        gensym("tidy"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_texteditor,
        gensym("texteditor"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_editmode,
        gensym("editmode"), A_DEFFLOAT, A_NULL);
    // jsarlo
    class_addmethod(canvas_class, (t_method)canvas_magicglass,
        gensym("magicglass"), A_DEFFLOAT, A_NULL);
    //end jsarlo
    class_addmethod(canvas_class, (t_method)canvas_print,
        gensym("print"), A_SYMBOL, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_menufont,
        gensym("menufont"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_font,
        gensym("font"), A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_find,
        gensym("find"), A_SYMBOL, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_find_again,
        gensym("findagain"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_find_parent,
        gensym("findparent"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_done_popup,
        gensym("done-popup"), A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_donecanvasdialog,
        gensym("donecanvasdialog"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_arraydialog,
        gensym("arraydialog"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_copyfromexternalbuffer,
        gensym("copyfromexternalbuffer"), A_GIMME, A_NULL);

/* -------------- connect method used in reading files ------------------ */
    class_addmethod(canvas_class, (t_method)canvas_connect,
        gensym("connect"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);

    class_addmethod(canvas_class, (t_method)canvas_disconnect,
        gensym("disconnect"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
/* -------------- copy buffer ------------------ */
    copy_binbuf = binbuf_new();
}
