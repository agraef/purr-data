/* Copyright (c) 1997-2001 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* this file defines the "glist" class, also known as "canvas" (the two used
to be different but are now unified except for some fossilized names.) */

#include <stdlib.h>
#include <stdio.h>
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include "g_magicglass.h"
#include "g_canvas.h"
#include "g_all_guis.h"
#include <string.h>
#include <math.h>

t_garray *array_garray;
t_class *preset_hub_class;
t_class *preset_node_class;
int array_joc;

extern int do_not_redraw;
extern void canvas_drawconnection(t_canvas *x, int lx1, int ly1, int lx2, int ly2, t_int tag, int issignal);
extern void canvas_updateconnection(t_canvas *x, int lx1, int ly1, int lx2, int ly2, t_int tag);

    /* LATER consider adding font size to this struct (see glist_getfont()) */
struct _canvasenvironment
{
    t_symbol *ce_dir;      /* directory patch lives in */
    int ce_argc;           /* number of "$" arguments */
    t_atom *ce_argv;       /* array of "$" arguments */
    int ce_dollarzero;     /* value of "$0" */
    t_namelist *ce_path;   /* search path */
};

#define GLIST_DEFCANVASWIDTH 450
#define GLIST_DEFCANVASHEIGHT 300

#ifdef __APPLE__
#define GLIST_DEFCANVASYLOC 22
#else
#define GLIST_DEFCANVASYLOC 0
#endif

/* ---------------------- variables --------------------------- */

extern t_pd *newest;
t_class *canvas_class;
int canvas_dspstate;                /* whether DSP is on or off */  
t_canvas *canvas_editing;           /* last canvas to start text edting */ 
t_canvas *canvas_whichfind;         /* last canvas we did a find in */ 
//t_canvas *canvas_list;              /* list of all root canvases */

/* ------------------ forward function declarations --------------- */
static void canvas_start_dsp(void);
static void canvas_stop_dsp(void);
static void canvas_drawlines(t_canvas *x);
void canvas_dosetbounds(t_canvas *x, int x1, int y1, int x2, int y2);
void canvas_reflecttitle(t_canvas *x);
static void canvas_addtolist(t_canvas *x);
static void canvas_takeofflist(t_canvas *x);
static void canvas_pop(t_canvas *x, t_floatarg fvis);
static int canvas_should_bind(t_canvas *x);
static void canvas_bind(t_canvas *x);
static void canvas_unbind(t_canvas *x);
void canvas_declare(t_canvas *x, t_symbol *s, int argc, t_atom *argv);

/* --------- functions to handle the canvas environment ----------- */

static t_symbol *canvas_newfilename = &s_;
static t_symbol *canvas_newdirectory = &s_;
static int canvas_newargc;
static t_atom *canvas_newargv;

static t_ab_definition *canvas_newabsource = 0;

    /* maintain the list of visible toplevels for the GUI's "windows" menu */
void canvas_updatewindowlist( void)
{
    t_canvas *x;
    if (glist_amreloadingabstractions)  /* not if we're in a reload */
        return;
    gui_start_vmess("gui_set_toplevel_window_list", "s", "dummy");
    gui_start_array();
    for (x = pd_this->pd_canvaslist; x; x = x->gl_next)
    {
        if (!x->gl_owner)
        {
            /* this is a canvas; if we have a window, put on "windows" list */
            if (x->gl_havewindow)
            {
                gui_s(x->gl_name->s_name);
                gui_x((long unsigned int)x);
            }
        }
    }
    gui_end_array();
    gui_end_vmess();
}

    /* add a glist the list of "root" canvases (toplevels without parents.) */
static void canvas_addtolist(t_canvas *x)
{
    x->gl_next = pd_this->pd_canvaslist;
    pd_this->pd_canvaslist = x;
}

static void canvas_takeofflist(t_canvas *x)
{
        /* take it off the window list */
    if (x == pd_this->pd_canvaslist) pd_this->pd_canvaslist = x->gl_next;
    else
    {
        t_canvas *z;
        for (z = pd_this->pd_canvaslist; z->gl_next != x; z = z->gl_next)
            if (!z->gl_next) return;
        z->gl_next = x->gl_next;
    }
}


void canvas_setargs(int argc, t_atom *argv)
{
        /* if there's an old one lying around free it here.  This
        happens if an abstraction is loaded but never gets as far
        as calling canvas_new(). */
    if (canvas_newargv)
        freebytes(canvas_newargv, canvas_newargc * sizeof(t_atom));
    canvas_newargc = argc;
    canvas_newargv = copybytes(argv, argc * sizeof(t_atom));
}

void glob_setfilename(void *dummy, t_symbol *filesym, t_symbol *dirsym)
{
    canvas_newfilename = filesym;
    canvas_newdirectory = dirsym;
}

/* set the source for the next canvas, it will be an ab instance */
void canvas_setabsource(t_ab_definition *abdef)
{
    canvas_newabsource = abdef;
}

t_canvas *canvas_getcurrent(void)
{
    return ((t_canvas *)pd_findbyclass(&s__X, canvas_class));
}

void canvas_setcurrent(t_canvas *x)
{
    pd_pushsym(&x->gl_pd);
}

void canvas_unsetcurrent(t_canvas *x)
{
    pd_popsym(&x->gl_pd);
}

t_canvasenvironment *canvas_getenv(t_canvas *x)
{
    if (!x) bug("canvas_getenv");
    while (!x->gl_env)
        if (!(x = x->gl_owner))
            bug("t_canvasenvironment");
    return (x->gl_env);
}

extern t_class *messresponder_class;
extern t_glist *messresponder_getglist(t_pd *x);

int canvas_getdollarzero(t_pd *x)
{
    t_canvas *cnv;
    /* binbuf_eval can send us a NULL target... */
    if (x && pd_class(x) == messresponder_class)
    {
        cnv = messresponder_getglist(x);
    }
    else
        cnv = canvas_getcurrent();
    t_canvasenvironment *env = (cnv ? canvas_getenv(cnv) : 0);
    if (env)
        return (env->ce_dollarzero);
    else return (0);
}

void canvas_getargs(int *argcp, t_atom **argvp)
{
    t_canvasenvironment *e = canvas_getenv(canvas_getcurrent());
    *argcp = e->ce_argc;
    *argvp = e->ce_argv;
}

void canvas_getargs_after_creation(t_canvas *c, int *argcp, t_atom **argvp)
{
    t_canvasenvironment *e = canvas_getenv(c);
    *argcp = e->ce_argc;
    *argvp = e->ce_argv;
}

t_symbol *canvas_realizedollar(t_canvas *x, t_symbol *s)
{
    t_symbol *ret;
    char *name = s->s_name;
    if (strchr(name, '$'))
    {
        t_canvasenvironment *env = canvas_getenv(x);
        canvas_setcurrent(x);
        ret = binbuf_realizedollsym(s, env->ce_argc, env->ce_argv, 1);
        canvas_unsetcurrent(x);
    }
    else ret = s;
    return (ret);
}

t_symbol *canvas_getcurrentdir(void)
{
    return (canvas_getdir(canvas_getcurrent()));
}

/* ** refactored function, check for errors */
t_symbol *canvas_getdir(t_canvas *x)
{
    x = canvas_getrootfor(x);
    /* in the case the root is an ab instance, we borrow the
        dir from the main root canvas (where the definition is stored) */
    if(x->gl_isab) x = x->gl_absource->ad_owner;
    t_canvasenvironment *e = canvas_getenv(x);
    return (e->ce_dir);
}

void canvas_makefilename(t_canvas *x, char *file, char *result, int resultsize)
{
    char interim[FILENAME_MAX];
    sys_expandpathelems(file, interim);
    //fprintf(stderr,"interim = <%s>\n", interim);
    char *dir = canvas_getenv(x)->ce_dir->s_name;
    if (interim[0] == '/' || (interim[0] && interim[1] == ':') || !*dir)
    {
        //fprintf(stderr,"root file\n");
        strncpy(result, interim, resultsize);
        result[resultsize-1] = 0;
    }
    else
    {
        //fprintf(stderr,"relative file\n");
        int nleft;
        strncpy(result, dir, resultsize);
        result[resultsize-1] = 0;
        nleft = resultsize - strlen(result) - 1;
        if (nleft <= 0) return;
        strcat(result, "/");
        strncat(result, interim, nleft);
        result[resultsize-1] = 0;
    } 
    //fprintf(stderr,"resulting file = <%s>\n", result);          
}

void canvas_rename(t_canvas *x, t_symbol *s, t_symbol *dir)
{
    canvas_unbind(x);
    x->gl_name = s;
    canvas_bind(x);
    if (dir && dir != &s_)
    {
        t_canvasenvironment *e = canvas_getenv(x);
        e->ce_dir = dir;
    }
    if (glist_isvisible(x))
        if (x->gl_havewindow) //was glist_isvisible(x)
            canvas_reflecttitle(x);
}

/* --------------- traversing the set of lines in a canvas ----------- */

int canvas_getindex(t_canvas *x, t_gobj *y)
{
    t_gobj *y2;
    int indexno;
    for (indexno = 0, y2 = x->gl_list; y2 && y2 != y; y2 = y2->g_next)
        indexno++;
    return (indexno);
}

void linetraverser_start(t_linetraverser *t, t_canvas *x)
{
    t->tr_ob = 0;
    t->tr_x = x;
    t->tr_nextoc = 0;
    t->tr_nextoutno = t->tr_nout = 0;
}

t_outconnect *linetraverser_next(t_linetraverser *t)
{
    t_outconnect *rval = t->tr_nextoc;
    int outno;
    while (!rval)
    {
        outno = t->tr_nextoutno;
        while (outno == t->tr_nout)
        {
            t_gobj *y;
            t_object *ob = 0;
            if (!t->tr_ob) y = t->tr_x->gl_list;
            else y = t->tr_ob->ob_g.g_next;
            for (; y; y = y->g_next)
                if (ob = pd_checkobject(&y->g_pd)) break;
            if (!ob) return (0);
            t->tr_ob = ob;
            t->tr_nout = obj_noutlets(ob);
            outno = 0;
            if (glist_isvisible(t->tr_x))
                gobj_getrect(y, t->tr_x,
                    &t->tr_x11, &t->tr_y11, &t->tr_x12, &t->tr_y12);
            else t->tr_x11 = t->tr_y11 = t->tr_x12 = t->tr_y12 = 0;
        }
        t->tr_nextoutno = outno + 1;
        rval = obj_starttraverseoutlet(t->tr_ob, &t->tr_outlet, outno);
        t->tr_outno = outno;
    }
    t->tr_nextoc = obj_nexttraverseoutlet(rval, &t->tr_ob2,
        &t->tr_inlet, &t->tr_inno);
    t->tr_nin = obj_ninlets(t->tr_ob2);
    if (!t->tr_nin) bug("drawline");
    if (glist_isvisible(t->tr_x))
    {
        int inplus = (t->tr_nin == 1 ? 1 : t->tr_nin - 1);
        int outplus = (t->tr_nout == 1 ? 1 : t->tr_nout - 1);
        gobj_getrect(&t->tr_ob2->ob_g, t->tr_x,
            &t->tr_x21, &t->tr_y21, &t->tr_x22, &t->tr_y22);
        t->tr_lx1 = t->tr_x11 +
            ((t->tr_x12 - t->tr_x11 - IOWIDTH) * t->tr_outno) /
                outplus + IOMIDDLE;
        t->tr_ly1 = t->tr_y12;
        t->tr_lx2 = t->tr_x21 +
            ((t->tr_x22 - t->tr_x21 - IOWIDTH) * t->tr_inno)/inplus +
                IOMIDDLE;
        t->tr_ly2 = t->tr_y21;
    }
    else
    {
        t->tr_x21 = t->tr_y21 = t->tr_x22 = t->tr_y22 = 0;
        t->tr_lx1 = t->tr_ly1 = t->tr_lx2 = t->tr_ly2 = 0;
    }
    return (rval);
}

void linetraverser_skipobject(t_linetraverser *t)
{
    t->tr_nextoc = 0;
    t->tr_nextoutno = t->tr_nout;
}

/* -------------------- the canvas object -------------------------- */
int glist_valid = 10000;

void canvasgop__clickhook(t_scalehandle *sh, int newstate);
void canvasgop__motionhook(t_scalehandle *sh,t_floatarg f1, t_floatarg f2);
extern void glist_setlastxy(t_glist *gl, int xval, int yval);

void glist_init(t_glist *x)
{
        /* zero out everyone except "pd" field */
    memset(((char *)x) + sizeof(x->gl_pd), 0, sizeof(*x) - sizeof(x->gl_pd));
    x->gl_stub = gstub_new(x, 0);
    x->gl_valid = ++glist_valid;
    x->gl_xlabel = (t_symbol **)t_getbytes(0);
    x->gl_ylabel = (t_symbol **)t_getbytes(0);

    //dpsaha@vt.edu gop resize (refactored by mathieu)
    x->x_handle = scalehandle_new((t_object *)x, x, 1,
        canvasgop__clickhook, canvasgop__motionhook);
    x->x_mhandle = scalehandle_new((t_object *)x, x, 0,
        canvasgop__clickhook, canvasgop__motionhook);
}

/* These globals are used to set state for the "canvas" field in a
   struct. We try below to make sure they only get set for the toplevel
   canvas, and then set them to NULL for everything else. */
t_symbol *canvas_field_templatesym; /* for "canvas" data type */
t_word *canvas_field_vec;           /* for "canvas" data type */
t_gpointer *canvas_field_gp;        /* parent for "canvas" data type */

static int calculate_zoom(t_float zoom_hack)
{
  // This gives back the zoom level stored in the patch (cf. zoom_hack
  // in g_readwrite.c). Make sure to round this value to handle any rounding
  // errors due to the limited float precision in Pd's binbuf writer.
  int zoom = round(zoom_hack*32);
  // This is a 5 bit number in 2's complement, so we need to extend the sign.
  zoom = zoom << 27 >> 27;
  // To be on the safe side, we finally clamp the result to the range -7..8
  // which are the zoom levels supported by Purr Data right now.
  if (zoom < -7) zoom = -7;
  if (zoom > 8) zoom = 8;
  //post("read zoom level: %d", zoom);
  return zoom;
}

int canvas_dirty_broadcast_all(t_symbol *name, t_symbol *dir, int mess);
int canvas_dirty_broadcast_ab_all(t_ab_definition *abdef, int mess);

    /* make a new glist.  It will either be a "root" canvas or else
    it appears as a "text" object in another window (canvas_getcurrent() 
    tells us which.) */
t_canvas *canvas_new(void *dummy, t_symbol *sel, int argc, t_atom *argv)
{
    /* DEBUG
    int d;
    for (d=0; d < argc; d++)
    {
        if (argv[d].a_type == A_FLOAT)
            fprintf(stderr, " %g ", argv[d].a_w.w_float);
        else fprintf(stderr, " %s ", argv[d].a_w.w_symbol->s_name);
    }
    fprintf(stderr,"\n");*/
    t_canvas *x = (t_canvas *)pd_new(canvas_class);
    t_canvas *owner = canvas_getcurrent();
    t_symbol *s = &s_;
    int vis = 0, width = GLIST_DEFCANVASWIDTH, height = GLIST_DEFCANVASHEIGHT;
    int xloc = 0, yloc = GLIST_DEFCANVASYLOC;
    int font = (owner ? owner->gl_font : sys_defaultfont);
    int zoom = 0;
    extern int sys_zoom;

    glist_init(x);
    //x->gl_magic_glass = magicGlass_new(x);
    x->gl_obj.te_type = T_OBJECT;
    if (!owner)
        canvas_addtolist(x);
    /* post("canvas %lx, owner %lx", x, owner); */

    if (argc == 5)  /* toplevel: x, y, w, h, font */
    {
        t_float zoom_hack = atom_getfloatarg(3, argc, argv);
        xloc = atom_getintarg(0, argc, argv);
        yloc = atom_getintarg(1, argc, argv);
        width = atom_getintarg(2, argc, argv);
        height = atom_getintarg(3, argc, argv);
        font = atom_getintarg(4, argc, argv);
        zoom_hack -= height;
        if (sys_zoom && zoom_hack > 0)
            zoom = calculate_zoom(zoom_hack);
    }
    else if (argc == 6)  /* subwindow: x, y, w, h, name, vis */
    {
        t_float zoom_hack = atom_getfloatarg(3, argc, argv);
        xloc = atom_getintarg(0, argc, argv);
        yloc = atom_getintarg(1, argc, argv);
        width = atom_getintarg(2, argc, argv);
        height = atom_getintarg(3, argc, argv);
        s = atom_getsymbolarg(4, argc, argv);
        vis = atom_getintarg(5, argc, argv);
        zoom_hack -= height;
        if (sys_zoom && zoom_hack > 0)
            zoom = calculate_zoom(zoom_hack);
    }
        /* (otherwise assume we're being created from the menu.) */

    if (canvas_newdirectory->s_name[0])
    {
        static int dollarzero = 1000;
        t_canvasenvironment *env = x->gl_env =
            (t_canvasenvironment *)getbytes(sizeof(*x->gl_env));
        if (!canvas_newargv)
            canvas_newargv = getbytes(0);
        env->ce_dir = canvas_newdirectory;
        env->ce_argc = canvas_newargc;
        env->ce_argv = canvas_newargv;
        env->ce_dollarzero = dollarzero++;
        env->ce_path = 0;
        canvas_newdirectory = &s_;
        canvas_newargc = 0;
        canvas_newargv = 0;
    }
    else x->gl_env = 0;

    x->gl_abdefs = 0;
    /* if canvas_newabsource is set means that
        this canvas is going to be an ab instance */
    if(canvas_newabsource)
    {
        x->gl_isab = 1;
        x->gl_absource = canvas_newabsource;
        canvas_newabsource = 0;
    }
    else x->gl_isab = 0;

    x->gl_subdirties = 0;
    x->gl_dirties = 0;

    if (yloc < GLIST_DEFCANVASYLOC)
        yloc = GLIST_DEFCANVASYLOC;
    if (xloc < 0)
        xloc = 0;
    x->gl_x1 = 0;
    x->gl_y1 = 0;
    x->gl_x2 = 1;
    x->gl_y2 = 1;
    canvas_dosetbounds(x, xloc, yloc, xloc + width, yloc + height);
    x->gl_owner = owner;
    x->gl_isclone = 0;
    x->gl_name = (*s->s_name ? s : 
        (canvas_newfilename ? canvas_newfilename : gensym("Pd")));
    canvas_bind(x);
    x->gl_loading = 1;
    x->gl_unloading = 0;
    //fprintf(stderr,"loading = 1 .x%lx owner=.x%lx\n", (t_int)x, (t_int)x->gl_owner);
    x->gl_goprect = 0;      /* no GOP rectangle unless it's turned on later */
        /* cancel "vis" flag if we're a subpatch of an
         abstraction inside another patch.  A separate mechanism prevents
         the toplevel abstraction from showing up. */
    if (vis && gensym("#X")->s_thing && 
        ((*gensym("#X")->s_thing) == canvas_class))
    {
        t_canvas *zzz = (t_canvas *)(gensym("#X")->s_thing);
        while (zzz && !zzz->gl_env)
            zzz = zzz->gl_owner;
        if (zzz && canvas_isabstraction(zzz) && zzz->gl_owner)
            vis = 0;
    }
    x->gl_willvis = vis;
    x->gl_edit = !strncmp(x->gl_name->s_name, "Untitled", 8);
    x->gl_font = sys_nearestfontsize(font);
    x->gl_zoom = zoom;
    pd_pushsym(&x->gl_pd);

    x->u_queue = canvas_undo_init(x);
    //glist_setlastxy(x, 20, 20);

    x->gl_templatesym = canvas_field_templatesym;
    x->gl_vec = canvas_field_vec;
    if (canvas_field_gp) gpointer_copy(canvas_field_gp, &x->gl_gp);

    // unset the globals in case this was a canvas field
    canvas_field_templatesym = NULL;
    canvas_field_vec = NULL;
    canvas_field_gp = NULL;

    /* in the case it is an abstraction (gl_env is not null)
        get the number of dirty instances of this same abstraction */
    if(x->gl_env)
    {
        if(!x->gl_isab)
            x->gl_dirties = canvas_dirty_broadcast_all(x->gl_name,
                                canvas_getdir(x), 0);
        else
            x->gl_dirties = canvas_dirty_broadcast_ab_all(x->gl_absource, 0);
    }

    return(x);
}

void canvas_setgraph(t_glist *x, int flag, int nogoprect);

static void canvas_coords(t_glist *x, t_symbol *s, int argc, t_atom *argv)
{
    //IB: first delete the graph in case we are downsizing the object size via script
    canvas_setgraph(x, 0, 0);

    x->gl_x1 = atom_getfloatarg(0, argc, argv);
    x->gl_y1 = atom_getfloatarg(1, argc, argv);
    x->gl_x2 = atom_getfloatarg(2, argc, argv);
    x->gl_y2 = atom_getfloatarg(3, argc, argv);
    x->gl_pixwidth = atom_getintarg(4, argc, argv);
    x->gl_pixheight = atom_getintarg(5, argc, argv);
    if (argc <= 7)
        canvas_setgraph(x, atom_getintarg(6, argc, argv), 1);
    else
    {
        x->gl_xmargin = atom_getintarg(7, argc, argv);
        x->gl_ymargin = atom_getintarg(8, argc, argv);
        canvas_setgraph(x, atom_getintarg(6, argc, argv), 0);
    }
}

    /* make a new glist and add it to this glist.  It will appear as
    a "graph", not a text object.  */
t_glist *glist_addglist(t_glist *g, t_symbol *sym,
    t_float x1, t_float y1, t_float x2, t_float y2,
    t_float px1, t_float py1, t_float px2, t_float py2)
{
    static int gcount = 0;
    int zz;
    int menu = 0;
    char *str;
    t_glist *x = (t_glist *)pd_new(canvas_class);
    glist_init(x);
    x->gl_obj.te_type = T_OBJECT;
    if (!*sym->s_name)
    {
        char buf[40];
        sprintf(buf, "graph%d", ++gcount);
        sym = gensym(buf);
        menu = 1;
    }
    else if (!strncmp((str = sym->s_name), "graph", 5)
        && (zz = atoi(str + 5)) > gcount)
            gcount = zz;
        /* in 0.34 and earlier, the pixel rectangle and the y bounds were
        reversed; this would behave the same, except that the dialog window
        would be confusing.  The "correct" way is to have "py1" be the value
        that is higher on the screen. */
    if (py2 < py1)
    {
        t_float zz;
        zz = y2;
        y2 = y1;
        y1 = zz;
        zz = py2;
        py2 = py1;
        py1 = zz;
    }
    if (x1 == x2 || y1 == y2)
        x1 = 0, x2 = 100, y1 = 1, y2 = -1;
    if (px1 != 0 && px2 == 0) px2 = px1 + GLIST_DEFGRAPHWIDTH;
    if (py1 != 0 && py2 == py1) py2 = py1 + GLIST_DEFGRAPHHEIGHT;
    if (px1 >= px2 || py1 >= py2)
        px1 = 100, py1 = 20, px2 = 100 + GLIST_DEFGRAPHWIDTH,
            py2 = 20 + GLIST_DEFGRAPHHEIGHT;

    x->gl_name = sym;
    x->gl_x1 = x1;
    x->gl_x2 = x2;
    x->gl_y1 = y1;
    x->gl_y2 = y2;
    x->gl_obj.te_xpix = px1;
    x->gl_obj.te_ypix = py1;
    x->gl_pixwidth = px2 - px1;
    x->gl_pixheight = py2 - py1;
    x->gl_font =  (canvas_getcurrent() ?
        canvas_getcurrent()->gl_font : sys_defaultfont);
    x->gl_zoom = 0;
    x->gl_screenx1 = x->gl_screeny1 = 0;
    x->gl_screenx2 = 450;
    x->gl_screeny2 = 300;
    x->gl_owner = g;
    canvas_bind(x);
    x->gl_isgraph = 1;
    x->gl_goprect = 0;
    x->gl_obj.te_binbuf = binbuf_new();
    binbuf_addv(x->gl_obj.te_binbuf, "s", gensym("graph"));
    if (!menu)
        pd_pushsym(&x->gl_pd);
    glist_add(g, &x->gl_gobj);
    if (!do_not_redraw) scrollbar_update(glist_getcanvas(g));
    //fprintf(stderr,"    ... %f %f\n", x->gl_x1, x->gl_x2);
    return (x);
}

extern int we_are_undoing;

    /* call glist_addglist from a Pd message */
void glist_glist(t_glist *g, t_symbol *s, int argc, t_atom *argv)
{
    if (canvas_hasarray(g)) return;
    pd_vmess(&g->gl_pd, gensym("editmode"), "i", 1);
    t_symbol *sym = atom_getsymbolarg(0, argc, argv);
    /* if we wish to put a graph where the mouse is we need to replace bogus name */
    if (!strcmp(sym->s_name, "NULL")) sym = &s_;  
    t_float x1 = atom_getfloatarg(1, argc, argv);  
    t_float y1 = atom_getfloatarg(2, argc, argv);  
    t_float x2 = atom_getfloatarg(3, argc, argv);  
    t_float y2 = atom_getfloatarg(4, argc, argv);  
    t_float px1 = atom_getfloatarg(5, argc, argv);  
    t_float py1 = atom_getfloatarg(6, argc, argv);  
    t_float px2 = atom_getfloatarg(7, argc, argv);  
    t_float py2 = atom_getfloatarg(8, argc, argv);
    glist_addglist(g, sym, x1, y1, x2, y2, px1, py1, px2, py2);
    if (!we_are_undoing)
        canvas_undo_add(glist_getcanvas(g), 9, "create",
            (void *)canvas_undo_set_create(glist_getcanvas(g)));
}

    /* return true if the glist should appear as a graph on parent;
    otherwise it appears as a text box. */
int glist_isgraph(t_glist *x)
{
    // testing to see if we have an array and force hiding text (later update GUI accordingly)
    // we likely need this to silently update legacy arrays
    // (no regressions are expected but this needs to be tested)
    t_gobj *g = x->gl_list;
    int hasarray = 0;
    while (g) {
        if (pd_class(&g->g_pd) == garray_class) hasarray = 1;
        g = g->g_next;
    }
    if (hasarray)  {
        x->gl_isgraph = 1;
        x->gl_hidetext = 1;
    }
    return (x->gl_isgraph|(x->gl_hidetext<<1));
}

/* bounds-setting for patch/subpatch windows */
void canvas_dosetbounds(t_canvas *x, int x1, int y1, int x2, int y2)
{
    //fprintf(stderr,"canvas_setbounds %d %d %d %d\n", x1, y1, x2, y2);

    int heightwas = y2 - y1;
    int heightchange = y2 - y1 - (x->gl_screeny2 - x->gl_screeny1);
    if (x->gl_screenx1 == x1 && x->gl_screeny1 == y1 &&
        x->gl_screenx2 == x2 && x->gl_screeny2 == y2)
            return;
    x->gl_screenx1 = x1;
    x->gl_screeny1 = y1;
    x->gl_screenx2 = x2;
    x->gl_screeny2 = y2;
    if (!glist_isgraph(x) && (x->gl_y2 < x->gl_y1)) 
    {
            /* if it's flipped so that y grows upward,
            fix so that zero is bottom edge and redraw.  This is
            only appropriate if we're a regular "text" object on the
            parent. */
        t_float diff = x->gl_y1 - x->gl_y2;
        t_gobj *y;
        x->gl_y1 = heightwas * diff;
        x->gl_y2 = x->gl_y1 - diff;
            /* and move text objects accordingly; they should stick
            to the bottom, not the top. */
        for (y = x->gl_list; y; y = y->g_next)
            if (pd_checkobject(&y->g_pd))
                gobj_displace(y, x, 0, heightchange);
        canvas_redraw(x);
    }
}

    /* public method to set the bounds for a patch/subpatch window from the
       GUI. */
static void canvas_setbounds(t_canvas *x, t_float left, t_float top,
                             t_float right, t_float bottom)
{
    canvas_dosetbounds(x, (int)left, (int)top, (int)right, (int)bottom);
}

t_symbol *canvas_makebindsym(t_symbol *s)
{
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, "pd-%s", s->s_name);
    buf[MAXPDSTRING-1] = 0;
    return (gensym(buf));
}

t_symbol *canvas_makebindsym_ab(t_symbol *s)
{
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, "ab-%s", s->s_name);
    buf[MAXPDSTRING-1] = 0;
    return (gensym(buf));
}

int garray_getname(t_garray *x, t_symbol **namep);

void canvas_args_to_string(char *namebuf, t_canvas *x)
{
    t_canvasenvironment *env = canvas_getenv(x);
    if (env->ce_argc)
    {
        int i;
        strcpy(namebuf, " (");
        for (i = 0; i < env->ce_argc; i++)
        {
            if (strlen(namebuf) > MAXPDSTRING / 2 - 5)
                break;
            if (i != 0)
                strcat(namebuf, " ");
            atom_string(&env->ce_argv[i], namebuf + strlen(namebuf),
                MAXPDSTRING / 2);
        }
        strcat(namebuf, ")");
    }
    else
    {
        namebuf[0] = 0;
        t_gobj *g = NULL;
        t_symbol *arrayname;
        int found = 0;
        for (g = x->gl_list; g; g = g->g_next)
        {

            if (pd_class(&g->g_pd) == garray_class)
            {
                garray_getname((t_garray *)g, &arrayname);
                if (found)
                {
                    strcat(namebuf, " ");
                }
                strcat(namebuf, arrayname->s_name);
                found++;
                //post("found=%d %s %s", found, arrayname->s_name, namebuf);
            }
        }
    }
}

void canvas_reflecttitle(t_canvas *x)
{
    char namebuf[MAXPDSTRING];
    canvas_args_to_string(namebuf, x);
    gui_vmess("gui_canvas_set_title", "xsssi",
        x, x->gl_name->s_name,
        namebuf, canvas_getdir(x)->s_name, x->gl_dirty);
}

/* --------------------- */

/* the following functions are used to broadcast messages to all instances of
    a specific abstraction (either file-based or ab).
    the state of these instances change accoring to the message sent. */

void clone_iterate(t_pd *z, t_canvas_iterator it, void* data);
int clone_match(t_pd *z, t_symbol *name, t_symbol *dir);
int clone_isab(t_pd *z);
int clone_matchab(t_pd *z, t_ab_definition *source);

static void canvas_dirty_common(t_canvas *x, int mess)
{
    if(mess == 2)
    {
        if(x->gl_dirty)
        {
            if(!x->gl_havewindow) canvas_vis(x, 1);
            gui_vmess("gui_canvas_emphasize", "x", x);
        }
    }
    else
    {
        x->gl_dirties += mess;
        if(x->gl_havewindow)
            canvas_warning(x, (x->gl_dirties > 1 ?
                                (x->gl_dirty ? 2 : 1)
                                : (x->gl_dirties ? !x->gl_dirty : 0)));
    }
}

/* packed data passing structure for canvas_dirty_broadcast */
typedef struct _dirty_broadcast_data
{
    t_symbol *name;
    t_symbol *dir;
    int mess;
    int *res;   /* return value */
} t_dirty_broadcast_data;

static void canvas_dirty_deliver_packed(t_canvas *x, t_dirty_broadcast_data *data)
{
    *data->res += (x->gl_dirty > 0);
    canvas_dirty_common(x, data->mess);
}

static int canvas_dirty_broadcast_packed(t_canvas *x, t_dirty_broadcast_data *data);

static int canvas_dirty_broadcast(t_canvas *x, t_symbol *name, t_symbol *dir, int mess)
{
    int res = 0;
    t_gobj *g;
    for (g = x->gl_list; g; g = g->g_next)
    {
        if(pd_class(&g->g_pd) == canvas_class)
        {
            if(canvas_isabstraction((t_canvas *)g) && !((t_canvas *)g)->gl_isab
                && ((t_canvas *)g)->gl_name == name
                && canvas_getdir((t_canvas *)g) == dir)
            {
                res += (((t_canvas *)g)->gl_dirty > 0);
                canvas_dirty_common((t_canvas *)g, mess);
            }
            else
                res += canvas_dirty_broadcast((t_canvas *)g, name, dir, mess);
        }
        else if(pd_class(&g->g_pd) == clone_class)
        {
            int cres = 0;
            t_dirty_broadcast_data data;
            data.name = name; data.dir = dir; data.mess = mess; data.res = &cres;
            if(clone_match(&g->g_pd, name, dir))
            {
                clone_iterate(&g->g_pd, canvas_dirty_deliver_packed, &data);
            }
            else
            {
                clone_iterate(&g->g_pd, canvas_dirty_broadcast_packed, &data);
            }
            res += cres;
        }
    }
    return (res);
}

static int canvas_dirty_broadcast_packed(t_canvas *x, t_dirty_broadcast_data *data)
{
    *data->res = canvas_dirty_broadcast(x, data->name, data->dir, data->mess);
}

int canvas_dirty_broadcast_all(t_symbol *name, t_symbol *dir, int mess)
{
    int res = 0;
    t_canvas *x;
    for (x = pd_this->pd_canvaslist; x; x = x->gl_next)
        res += canvas_dirty_broadcast(x, name, dir, mess);
    return (res);
}

/* same but for ab */

typedef struct _dirty_broadcast_ab_data
{
    t_ab_definition *abdef;
    int mess;
    int *res;
} t_dirty_broadcast_ab_data;

static void canvas_dirty_deliver_ab_packed(t_canvas *x, t_dirty_broadcast_ab_data *data)
{
    *data->res += (x->gl_dirty > 0);
    canvas_dirty_common(x, data->mess);
}

static int canvas_dirty_broadcast_ab_packed(t_canvas *x, t_dirty_broadcast_ab_data *data);

static int canvas_dirty_broadcast_ab(t_canvas *x, t_ab_definition *abdef, int mess)
{
    int res = 0;
    t_gobj *g;
    for (g = x->gl_list; g; g = g->g_next)
    {
        if(pd_class(&g->g_pd) == canvas_class)
        {
            if(canvas_isabstraction((t_canvas *)g) && ((t_canvas *)g)->gl_isab
                && ((t_canvas *)g)->gl_absource == abdef)
            {
                res += (((t_canvas *)g)->gl_dirty > 0);
                canvas_dirty_common((t_canvas *)g, mess);
            }
            else
                res += canvas_dirty_broadcast_ab((t_canvas *)g, abdef, mess);
        }
        else if(pd_class(&g->g_pd) == clone_class)
        {
            int cres = 0;
            t_dirty_broadcast_ab_data data;
            data.abdef = abdef; data.mess = mess; data.res = &cres;
            if(clone_matchab(&g->g_pd, abdef))
            {
                clone_iterate(&g->g_pd, canvas_dirty_deliver_ab_packed, &data);
            }
            else if(clone_isab(&g->g_pd))
            {
                clone_iterate(&g->g_pd, canvas_dirty_broadcast_ab_packed, &data);
            }
            res += cres;
        }
    }
    return (res);
}

static int canvas_dirty_broadcast_ab_packed(t_canvas *x, t_dirty_broadcast_ab_data *data)
{
    *data->res = canvas_dirty_broadcast_ab(x, data->abdef, data->mess);
}

int canvas_dirty_broadcast_ab_all(t_ab_definition *abdef, int mess)
{
    int res = 0;
    t_canvas *x;
    for (x = pd_this->pd_canvaslist; x; x = x->gl_next)
        res += canvas_dirty_broadcast_ab(x, abdef, mess);
    return (res);
}

/* --------------------- */

/* climbs up to the root canvas while enabling or disabling visual markings for dirtiness
    of traversed canvases */
void canvas_dirtyclimb(t_canvas *x, int n)
{
    if (x->gl_owner)
    {
        gobj_dirty(&x->gl_gobj, x->gl_owner,
            (n ? 1 : (x->gl_subdirties ? 2 : 0)));
        x = x->gl_owner;
        while(x->gl_owner)
        {
            x->gl_subdirties += ((unsigned)n ? 1 : -1);
            if(!x->gl_dirty)
                gobj_dirty(&x->gl_gobj, x->gl_owner, (x->gl_subdirties ? 2 : 0));
            x = x->gl_owner;
        }
    }
}

    /* mark a glist dirty or clean */
void canvas_dirty(t_canvas *x, t_floatarg n)
{
    t_canvas *x2 = canvas_getrootfor(x);
    if (glist_amreloadingabstractions)
        return;
    if ((unsigned)n != x2->gl_dirty)
    {
        x2->gl_dirty = n;
        if (x2->gl_havewindow)
            canvas_reflecttitle(x2);

        /* set dirtiness visual markings */
        canvas_dirtyclimb(x2, (unsigned)n);

        /* in the case it is an abstraction, we tell all other
            instances that there is eiher one more dirty instance or
            one less dirty instance */
        if(canvas_isabstraction(x2)
            && (x2->gl_owner || x2->gl_isclone))
        {
            if(!x2->gl_isab)
                canvas_dirty_broadcast_all(x2->gl_name, canvas_getdir(x2),
                    (x2->gl_dirty ? 1 : -1));
            else
                canvas_dirty_broadcast_ab_all(x2->gl_absource,
                    (x2->gl_dirty ? 1 : -1));
        }
    }
}

void draw_notify(t_canvas *x, t_symbol *s, int argc, t_atom *argv);

void canvas_scalar_event(t_canvas *x, t_symbol *s, int argc, t_atom *argv)
{
    /* These events only get sent when we're not in edit mode.  Once
       we get editmode status sync'd in the GUI we can just prevent
       sending any messages when in edit mode. */
    if (!x->gl_edit)
        draw_notify(x, s, argc, argv);
}

void canvas_show_scrollbars(t_canvas *x, t_floatarg f)
{
    x->gl_noscroll = (int)f;
    if (x->gl_mapped)
        gui_vmess("gui_canvas_set_scrollbars", "xi", x, (int)f);
}

void canvas_show_menu(t_canvas *x, t_floatarg f)
{
    x->gl_nomenu = (int)f;
}

extern void canvas_check_nlet_highlights(t_canvas *x);

/*********** dpsaha@vt.edu resize move hooks ****************/
void canvas_draw_gop_resize_hooks(t_canvas* x)
{
    t_scalehandle *sh = (t_scalehandle *)(x->x_handle);
    t_scalehandle *mh = (t_scalehandle *)(x->x_mhandle);
    //fprintf(stderr,"draw_gop_resize_hooks START\n");
    //in case we are an array which does not initialize its hooks
    if (!sh || !mh) return;
    if(x->gl_edit && glist_isvisible(x) && glist_istoplevel(x) &&
        x->gl_goprect && !x->gl_editor->e_selection)
    {
        //Drawing and Binding Resize_Blob for GOP
        //fprintf(stderr,"draw_gop_resize_hooks DRAW %lx %lx\n", (t_int)x, (t_int)glist_getcanvas(x));
        sprintf(sh->h_pathname, ".x%lx.h%lx", (t_int)x, (t_int)sh);
        sprintf(mh->h_pathname, ".x%lx.h%lx", (t_int)x, (t_int)mh);

        /* These are handled now in canvas_doclick */
        //scalehandle_draw_select(sh,
        //    -1-x->gl_obj.te_xpix+x->gl_xmargin + x->gl_pixwidth,
        //    -1-x->gl_obj.te_ypix+x->gl_ymargin + x->gl_pixheight/*,GOP_resblob*/);
        //scalehandle_draw_select(mh,
        //    2+SCALEHANDLE_WIDTH -x->gl_obj.te_xpix+x->gl_xmargin,
        //    2+SCALEHANDLE_HEIGHT-x->gl_obj.te_ypix+x->gl_ymargin /*,"GOP_movblob"*/);

        /* these constants don't actually reflect the actual size of the
           click rectangle-- we should probably change them... */

        scalehandle_draw_select(mh,
            SCALEHANDLE_WIDTH - 4, SCALEHANDLE_HEIGHT - 11);
    }
    else
    {
        //fprintf(stderr,"draw_gop_resize_hooks ERASE\n");
        scalehandle_draw_erase(sh);
        scalehandle_draw_erase(mh);
    }
    canvas_check_nlet_highlights(x);
}
/*****************************************************************************/

void canvas_drawredrect(t_canvas *x, int doit)
{
    if (doit)
    {
        int x1=x->gl_xmargin, y1=x->gl_ymargin + sys_legacy;
        int x2=x1+x->gl_pixwidth, y2=y1+x->gl_pixheight;
        gui_vmess("gui_canvas_drawredrect", "xiiii",
            glist_getcanvas(x),
            x1, y1, x2, y2);
        //dpsaha@vt.edu for drawing the GOP_blobs
        if (x->gl_goprect && x->gl_edit)
            canvas_draw_gop_resize_hooks(x);
    }
    else
    {
        gui_vmess("gui_canvas_deleteredrect", "x",
            glist_getcanvas(x));
    }
}

    /* the window becomes "mapped" (visible and not miniaturized) or
    "unmapped" (either miniaturized or just plain gone.)  This should be
    called from the GUI after the fact to "notify" us that we're mapped. */
void canvas_map(t_canvas *x, t_floatarg f)
{
    //fprintf(stderr,"canvas_map %lx %f\n", (t_int)x, f);
    int flag = (f != 0);
    t_gobj *y;
    if (flag)
    {
        if (!glist_isvisible(x)) {
            t_selection *sel;
            if (!x->gl_havewindow)
            {
                bug("canvas_map");
                canvas_vis(x, 1);
            }
            if (!x->gl_list) {
                //if there are no objects on the canvas
                canvas_create_editor(x);
            }
            else for (y = x->gl_list; y; y = y->g_next) {
                gobj_vis(y, x, 1);
            }
            if (x->gl_editor && x->gl_editor->e_selection)
                for (sel = x->gl_editor->e_selection; sel; sel = sel->sel_next)
                    gobj_select(sel->sel_what, x, 1);
            x->gl_mapped = 1;
            canvas_drawlines(x);
            if (x->gl_isgraph && x->gl_goprect)
                canvas_drawredrect(x, 1);
            scrollbar_update(x);
        }
    }
    else
    {
        //fprintf(stderr,"canvas_map 0\n");
        if (glist_isvisible(x))
        {
            /* Clear out scalars one by one. We need to do this
               in order to unbind the symbol we used to receive
               [draw] events from the GUI. (We could also just unbind
               here if this turns out to be a bottleneck... */
            t_gobj *y;
            for (y = x->gl_list; y; y = y->g_next)
            {
                if (pd_class(&y->g_pd) == scalar_class)
                    gobj_vis(y, x, 0);
            }
            /* now clear out the rest of the canvas in the GUI */
            gui_vmess("gui_canvas_erase_all_gobjs", "x", x);
            x->gl_mapped = 0;
        }
    }
}

void canvas_redraw(t_canvas *x)
{
    if (do_not_redraw) return;
    //fprintf(stderr,"canvas_redraw %lx\n", (t_int)x);
    if (glist_isvisible(x))
    {
        //fprintf(stderr,"canvas_redraw glist_isvisible=true\n");
        canvas_map(x, 0);
        canvas_map(x, 1);

        /* now re-highlight our selection */
        t_selection *y;
        if (x->gl_editor && x->gl_editor->e_selection)
            for (y = x->gl_editor->e_selection; y; y = y->sel_next)
                gobj_select(y->sel_what, x, 1);
    }
}

    /* we call this on a non-toplevel glist to "open" it into its
    own window. */
void glist_menu_open(t_glist *x)
{
    /* 20151230: moved to canvas_vis, so that scripted vis calls (e.g. via
       [send pd-abstraction-name.pd] do proper redraws of abstractions */
    /*if (glist_isvisible(x))
    {
        if (!glist_istoplevel(x))
        {
            t_glist *gl2 = x->gl_owner;
            if (!gl2) 
                bug("glist_menu_open"); // shouldn't happen but not dangerous
            else
            {
                // erase ourself in parent window
                gobj_vis(&x->gl_gobj, gl2, 0);
                // get rid of our editor (and subeditors)
                if (x->gl_editor)
                    canvas_destroy_editor(x);
                x->gl_havewindow = 1;
                // redraw ourself in parent window (blanked out this time)
                gobj_vis(&x->gl_gobj, gl2, 1);
            }
        }
        else
        {
            // Not sure if this needs to get ported... need to test
            //sys_vgui("focus .x%lx\n", (t_int)x);
        }
    }
    else
    {
        if (x->gl_editor)
        canvas_destroy_editor(x);
    }
    */
    canvas_vis(x, 1);
}

int glist_isvisible(t_glist *x)
{
    return ((!x->gl_loading) && ((x->gl_isgraph && glist_getcanvas(x)->gl_mapped) || (!x->gl_isgraph && x->gl_mapped)));
}

int glist_istoplevel(t_glist *x)
{
        /* we consider a graph "toplevel" if it has its own window
        or if it appears as a box in its parent window so that we
        don't draw the actual contents there. */
    return (x->gl_havewindow || !x->gl_isgraph);
}

int glist_getfont(t_glist *x)
{
    while (!x->gl_env)
        if (!(x = x->gl_owner))
            bug("t_canvasenvironment");
    return (x->gl_font);
}

extern void canvas_group_free(t_pd *x);
static void canvas_deregister_ab(t_canvas *x, t_ab_definition *a);

void canvas_free(t_canvas *x)
{
    //fprintf(stderr,"canvas_free %lx\n", (t_int)x);

    /* crude hack. in the case it was a clone instance, it shouldn't have an owner.
        For ab instances, we have set the owner inside clone_free because we need it
        in order to deregister the dependencies.
        here we set it to NULL again to prevent any error in the functions called bellow */
    t_canvas *aux = x->gl_owner;
    if(x->gl_isclone) x->gl_owner = 0;

    /* in the case it is a dirty abstraction, we tell all other
        instances that there is one less dirty instance */
    if(canvas_isabstraction(x) && x->gl_dirty
        && (x->gl_owner || x->gl_isclone))
    {
        if(!x->gl_isab)
            canvas_dirty_broadcast_all(x->gl_name, canvas_getdir(x), -1);
        else
            canvas_dirty_broadcast_ab_all(x->gl_absource, -1);
    }

    t_gobj *y;
    int dspstate = canvas_suspend_dsp();

    //canvas_noundo(x);
    canvas_undo_free(x);

    if (canvas_editing == x)
        canvas_editing = 0;
    if (canvas_whichfind == x)
        canvas_whichfind = 0;
    glist_noselect(x);
    x->gl_unloading = 1;
    while (y = x->gl_list)
        glist_delete(x, y);
    if (x == glist_getcanvas(x))
        canvas_vis(x, 0);
    if (x->gl_editor)
        canvas_destroy_editor(x);   /* bug workaround; should already be gone*/

    if (x->x_handle) scalehandle_free(x->x_handle);
    if (x->x_mhandle) scalehandle_free(x->x_mhandle);
    
    canvas_unbind(x);
    if (x->gl_env)
    {
        freebytes(x->gl_env->ce_argv, x->gl_env->ce_argc * sizeof(t_atom));
        freebytes(x->gl_env, sizeof(*x->gl_env));
    }
    canvas_resume_dsp(dspstate);
    freebytes(x->gl_xlabel, x->gl_nxlabels * sizeof(*(x->gl_xlabel)));
    freebytes(x->gl_ylabel, x->gl_nylabels * sizeof(*(x->gl_ylabel)));
    gstub_cutoff(x->gl_stub);
    gfxstub_deleteforkey(x);        /* probably unnecessary */
    if (!x->gl_owner && !x->gl_isclone)
        canvas_takeofflist(x);
    if (x->gl_svg)                   /* for groups, free the data */
        canvas_group_free(x->gl_svg);

    /* freeing an ab instance */
    if(x->gl_isab)
    {
        x->gl_absource->ad_numinstances--;
        canvas_deregister_ab((x->gl_isclone ? aux : x->gl_owner),
            x->gl_absource);
    }

    /* free stored ab definitions */
    t_ab_definition *d = x->gl_abdefs, *daux;
    while(d)
    {
        daux = d->ad_next;
        binbuf_free(d->ad_source);
        freebytes(d->ad_dep, sizeof(t_ab_definition*)*d->ad_numdep);
        freebytes(d->ad_deprefs, sizeof(int)*d->ad_numdep);
        freebytes(d, sizeof(t_ab_definition));
        d = daux;
    }
}

/* ----------------- lines ---------- */

static void canvas_drawlines(t_canvas *x)
{
    t_linetraverser t;
    t_outconnect *oc;
    int issignal;
    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
    {
        issignal = (outlet_getsymbol(t.tr_outlet) == &s_signal ? 1 : 0);
        if (!(pd_class(&t.tr_ob2->ob_g.g_pd) == preset_node_class &&
              pd_class(&t.tr_ob->ob_g.g_pd) != message_class))
            canvas_drawconnection(glist_getcanvas(x), t.tr_lx1, t.tr_ly1,
                t.tr_lx2, t.tr_ly2, (t_int)oc, issignal);
    }
}

void canvas_fixlinesfor(t_canvas *x, t_text *text)
{
    t_linetraverser t;
    t_outconnect *oc;

    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
    {
        if (t.tr_ob == text || t.tr_ob2 == text)
        {
            canvas_updateconnection(x, t.tr_lx1, t.tr_ly1, t.tr_lx2, t.tr_ly2,
                (t_int)oc);
        }
    }
}

    /* kill all lines for the object */
void canvas_deletelinesfor(t_canvas *x, t_text *text)
{
    t_linetraverser t;
    t_outconnect *oc;
    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
    {
        if (t.tr_ob == text || t.tr_ob2 == text)
        {
            if (x->gl_editor && glist_isvisible(glist_getcanvas(x)))
            {
                /* Still don't see any place where this gets used. Maybe it's
                   used by older externals? Need to test... */
                //sys_vgui(".x%lx.c delete l%lx\n",
                //    glist_getcanvas(x), oc);
                /* probably need a gui_vmess here */
            }
            obj_disconnect(t.tr_ob, t.tr_outno, t.tr_ob2, t.tr_inno);
        }
    }
}

    /*  delete all lines for the object 
        for efficient redrawing of connections */
void canvas_eraselinesfor(t_canvas *x, t_text *text)
{
    t_linetraverser t;
    t_outconnect *oc;
    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
    {
        if (t.tr_ob == text || t.tr_ob2 == text)
        {
            if (x->gl_editor)
            {
                char tagbuf[MAXPDSTRING];
                sprintf(tagbuf, "l%lx", (long unsigned int)oc);
                gui_vmess("gui_canvas_delete_line", "xs",
                    glist_getcanvas(x), tagbuf);
            }
        }
    }
}


    /* kill all lines for one inlet or outlet */
void canvas_deletelinesforio(t_canvas *x, t_text *text,
    t_inlet *inp, t_outlet *outp)
{
    t_linetraverser t;
    t_outconnect *oc;
    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
    {
        if ((t.tr_ob == text && t.tr_outlet == outp) ||
            (t.tr_ob2 == text && t.tr_inlet == inp))
        {
            if (x->gl_editor)
            {
                char buf[MAXPDSTRING];
                sprintf(buf, "l%lx", (long unsigned int)oc);
                gui_vmess("gui_canvas_delete_line", "xs",
                    glist_getcanvas(x),
                    buf);
            }
            obj_disconnect(t.tr_ob, t.tr_outno, t.tr_ob2, t.tr_inno);
        }
    }
}

static void canvas_pop(t_canvas *x, t_floatarg fvis)
{
    if (fvis != 0)
        canvas_vis(x, 1);
    pd_popsym(&x->gl_pd);
    canvas_resortinlets(x);
    canvas_resortoutlets(x);
    x->gl_loading = 0;
    //fprintf(stderr,"loading = 0 .x%lx owner=.x%lx\n", x, x->gl_owner);
}

extern void *svg_new(t_pd *x, t_symbol *s, int argc, t_atom *argv);
extern t_pd *svg_header(t_pd *x);

static void group_svginit(t_glist *gl, t_symbol *type, int argc, t_atom *argv)
{
    gl->gl_svg = (t_pd *)(svg_new((t_pd *)gl, type, argc, argv));
    t_pd *proxy = svg_header(gl->gl_svg);
    inlet_new(&gl->gl_obj, proxy, 0, 0);
    outlet_new(&gl->gl_obj, &s_anything);
}

void canvas_objfor(t_glist *gl, t_text *x, int argc, t_atom *argv);

void canvas_restore(t_canvas *x, t_symbol *s, int argc, t_atom *argv)
{
    t_pd *z;
    int is_draw_command = 0;
    //fprintf(stderr,"canvas_restore %lx\n", x);
    /* for [draw g] and [draw svg] we add an inlet to the svg attr proxy */
    if (atom_getsymbolarg(2, argc, argv) == gensym("draw"))
    {
        t_symbol *type = (atom_getsymbolarg(3, argc, argv) == gensym("svg")) ?
            gensym("svg") : gensym("g");
        group_svginit(x, type,
            (type == gensym("svg") && argc > 4) ? argc-4 : 0,
            (type == gensym("svg") && argc > 4) ? argv+4 : 0);
        is_draw_command = 1;
    }
    if (argc > 3 || (is_draw_command && argc > 4))
    {
        int offset = is_draw_command ? 4 : 3;
        t_atom *ap=argv+offset;
        if (ap->a_type == A_SYMBOL)
        {
            t_canvasenvironment *e = canvas_getenv(canvas_getcurrent());
            canvas_rename(x, binbuf_realizedollsym(ap->a_w.w_symbol,
                e->ce_argc, e->ce_argv, 1), 0);
        }
    }
    canvas_pop(x, x->gl_willvis);

    if (!(z = gensym("#X")->s_thing)) error("canvas_restore: out of context");
    else if (*z != canvas_class) error("canvas_restore: wasn't a canvas");
    else
    {
        t_canvas *x2 = (t_canvas *)z;
        x->gl_owner = x2;
        canvas_objfor(x2, &x->gl_obj, argc, argv);
    }
}

void canvas_loadbangsubpatches(t_canvas *x, t_symbol *s)
{
    t_gobj *y;
    //t_symbol *s = gensym("loadbang");
    for (y = x->gl_list; y; y = y->g_next)
        if (pd_class(&y->g_pd) == canvas_class)
        {
            if (!canvas_isabstraction((t_canvas *)y))
            {
            //fprintf(stderr,"%lx s:canvas_loadbangsubpatches %s\n",
            //    x, s->s_name);
            canvas_loadbangsubpatches((t_canvas *)y, s);
            }
        }
    for (y = x->gl_list; y; y = y->g_next)
        if ((pd_class(&y->g_pd) != canvas_class) &&
            zgetfn(&y->g_pd, s))
        {
            //fprintf(stderr,"%lx s:obj_loadbang %s\n",x,s->s_name);
            pd_vmess(&y->g_pd, s, "f", (t_floatarg)LB_LOAD);
        }
}

static void canvas_loadbangabstractions(t_canvas *x, t_symbol *s)
{
    t_gobj *y;
    //t_symbol *s = gensym("loadbang");
    for (y = x->gl_list; y; y = y->g_next)
        if (pd_class(&y->g_pd) == canvas_class)
        {
            if (canvas_isabstraction((t_canvas *)y))
            {
                //fprintf(stderr,"%lx a:canvas_loadbang %s\n",x,s->s_name);
                canvas_loadbangabstractions((t_canvas *)y, s);
                canvas_loadbangsubpatches((t_canvas *)y, s);
            }
            else
            {
                //fprintf(stderr,"%lx a:canvas_loadbangabstractions %s\n",
                //    x, s->s_name);
                canvas_loadbangabstractions((t_canvas *)y, s);
            }
        }
}

void canvas_loadbang(t_canvas *x)
{
    //t_gobj *y;
    // first loadbang preset hubs and nodes
    //fprintf(stderr,"%lx 0\n", x);
    canvas_loadbangabstractions(x, gensym("pre-loadbang"));
    canvas_loadbangsubpatches(x, gensym("pre-loadbang"));
    //fprintf(stderr,"%lx 1\n", x);
    // then do the regular loadbang
    canvas_loadbangabstractions(x, gensym("loadbang"));
    canvas_loadbangsubpatches(x, gensym("loadbang"));
    //fprintf(stderr,"%lx 2\n", x);
}

/* JMZ/MSP:
 * initbang is emitted after a canvas is read from a file, but before the
   parent canvas is finished loading.  This is apparently used so that
   abstractions can create inlets/outlets as a function of creation arguments.
   This practice is quite ugly but there's no other way to do it so far.
*/
void canvas_initbang(t_canvas *x)
{
    t_gobj *y;
    t_symbol *s = gensym("loadbang");
    /* run "initbang" for all subpatches, but NOT for the child abstractions */
    for (y = x->gl_list; y; y = y->g_next)
        if (pd_class(&y->g_pd) == canvas_class &&
            !canvas_isabstraction((t_canvas *)y))
                canvas_initbang((t_canvas *)y);

    /* call the initbang()-method for objects that have one */
    for (y = x->gl_list; y; y = y->g_next)
        if ((pd_class(&y->g_pd) != canvas_class) && zgetfn(&y->g_pd, s))
            pd_vmess(&y->g_pd, s, "f", (t_floatarg)LB_INIT);
}

/* JMZ:
 * closebang is emitted before the canvas is destroyed
 * and BEFORE subpatches/abstractions in this canvas are destroyed
 */
void canvas_closebang(t_canvas *x)
{
    t_gobj *y;
    t_symbol *s = gensym("loadbang");

    /* call the closebang()-method for objects that have one
     * but NOT for subpatches/abstractions: these are called separately
     * from g_graph:glist_delete()
     */
    for (y = x->gl_list; y; y = y->g_next)
        if ((pd_class(&y->g_pd) != canvas_class) && zgetfn(&y->g_pd, s))
            pd_vmess(&y->g_pd, s, "f", (t_floatarg)LB_CLOSE);
}

// we use this function to check if the canvas that has sent out the <config>
// signal, meaning it has been resized, if we have scalars in there and
// the canvas has gop enabled, we need to redraw the window to make sure
// scalars scale with the window
void canvas_checkconfig(t_canvas *x)
{
    //fprintf(stderr,"canvas_checkconfig\n");
    t_gobj *y;
    if (x->gl_isgraph)
    {
        for (y = x->gl_list; y; y = y->g_next)
        {
            if (pd_class(&y->g_pd) == scalar_class)
            {
                //fprintf(stderr,"...redrawing\n");
                canvas_redraw(x);
                break;
            }
        }
    }
}

/* needed for readjustment of garrays */
extern t_array *garray_getarray(t_garray *x);
extern void garray_fittograph(t_garray *x, int n, int flag);
extern t_rtext *glist_findrtext(t_glist *gl, t_text *who);
extern void rtext_gettext(t_rtext *x, char **buf, int *bufsize);

static void canvas_relocate(t_canvas *x, t_symbol *canvasgeom,
    t_symbol *topgeom)
{
    int cxpix, cypix, cw, ch, txpix, typix, tw, th;
    if (sscanf(canvasgeom->s_name, "%dx%d+%d+%d", &cw, &ch, &cxpix, &cypix)
        < 4 ||
        sscanf(topgeom->s_name, "%dx%d+%d+%d", &tw, &th, &txpix, &typix) < 4)
        bug("canvas_relocate");
    /* for some reason this is initially called with cw=ch=1 so
    we just suppress that here. */
    if (cw > 5 && ch > 5)
        canvas_dosetbounds(x, txpix, typix,
            txpix + cw, typix + ch);
    /* readjust garrays (if any) */
    t_gobj *g;
    t_garray *ga = NULL;
    t_array *a = NULL;
    int  num_elem = 0;

    //int found_garray = 0;

    for (g = x->gl_list; g; g = g->g_next)
    {
        //fprintf(stderr, "searching\n");
        //post("searching");
        //for subpatch garrays
        if (pd_class(&g->g_pd) == garray_class)
        {
            //fprintf(stderr,"found ya\n");
            //post("found ya");
            ga = (t_garray *)g;
            if (ga)
            {
                a = garray_getarray(ga);
                num_elem = a->a_n;
                garray_fittograph(ga, num_elem, 1);
                //found_garray = 1;
            }
        }
    }
    canvas_checkconfig(x);

    // ico@vt.edu:
    // Here we update only scrollbars to avoid race condition
    // caused by doing gui_canvas_get_scroll which in turn
    // calls canvas_relocate to ensure garrays in subpatches
    // are properly updated when those windows are resized.
    // given that the scroll update will happen likely faster
    // than the return value from the backend, we do this to
    // get rid of the stale scrollbars, e.g. when making the
    // window smaller (at first the scrollbars are there because
    // the garray has not been redrawn yet, and then we update
    // scrollbars once again here below.
    //if (found_garray == 1) {
        //post("found garray");
    gui_vmess("do_getscroll", "xi", x, 0);
    //}
}

void canvas_popabstraction(t_canvas *x)
{
    newest = &x->gl_pd;
    gensym("#A")->s_thing = 0;
    pd_bind(newest, gensym("#A"));
    pd_popsym(&x->gl_pd);
    //x->gl_loading = 1;
    //fprintf(stderr,"loading = 1 .x%lx owner=.x%lx\n", x, x->gl_owner);
    canvas_resortinlets(x);
    canvas_resortoutlets(x);
    x->gl_loading = 0;
    //fprintf(stderr,"loading = 0 .x%lx owner=.x%lx\n", x, x->gl_owner);
}

void canvas_logerror(t_object *y)
{
#ifdef LATER
    canvas_vis(x, 1);
    if (!glist_isselected(x, &y->ob_g))
        glist_select(x, &y->ob_g);
#endif
}

/* -------------------------- subcanvases ---------------------- */

static void *subcanvas_new(t_symbol *s)
{
    t_atom a[6];
    t_canvas *x, *z = canvas_getcurrent();
    //fprintf(stderr,"subcanvas_new current canvas .x%lx\n", (t_int)z);
    if (!*s->s_name) s = gensym("/SUBPATCH/");
    SETFLOAT(a, 0);
    SETFLOAT(a+1, GLIST_DEFCANVASYLOC);
    SETFLOAT(a+2, GLIST_DEFCANVASWIDTH);
    SETFLOAT(a+3, GLIST_DEFCANVASHEIGHT);
    SETSYMBOL(a+4, s);
    SETFLOAT(a+5, 1);
    x = canvas_new(0, 0, 6, a);
    x->gl_owner = z;
    canvas_pop(x, 1);
    return (x);
}

void *group_new(t_symbol *type, int argc, t_atom *argv)
{
    t_symbol *groupname;
    if (type == gensym("g"))
        groupname = atom_getsymbolarg(0, argc, argv);
    else /* no name for inner svg */
        groupname = &s_;
    t_canvas *x = subcanvas_new(groupname);
    group_svginit(x, type, argc, argv);
    return (x);
}

static void canvas_click(t_canvas *x,
    t_floatarg xpos, t_floatarg ypos,
        t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    canvas_vis(x, 1);
}


    /* find out from subcanvas contents how much to fatten the box */
void canvas_fattensub(t_canvas *x,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    *xp2 += 50;     /* fake for now */
    *yp2 += 50;
}

static void canvas_rename_method(t_canvas *x, t_symbol *s, int ac, t_atom *av)
{
    /* special case for [draw g] where the 3rd arg is the receiver name */
    if (x->gl_svg)
    {
        if (atom_getsymbolarg(0, ac, av) == gensym("g") && ac > 1)
            ac--, av++;
        else
            ac = 0;
    }
    if (ac && av->a_type == A_SYMBOL)
        canvas_rename(x, av->a_w.w_symbol, 0);
    else if (ac && av->a_type == A_DOLLSYM)
    {
        t_canvasenvironment *e = canvas_getenv(x);
        canvas_setcurrent(x);
        canvas_rename(x, binbuf_realizedollsym(av->a_w.w_symbol,
            e->ce_argc, e->ce_argv, 1), 0); 
        canvas_unsetcurrent(x);
    }
    else canvas_rename(x, gensym("Pd"), 0);
}

static int forwardmess_recurse = 0;

static void canvas_forwardmess(t_canvas *x, t_symbol *s, int ac, t_atom *av)
{
    if (av[0].a_type != A_FLOAT)
    {
        pd_error(x, "error: canvas: forwardmess: need object index");
        return;
    }
    t_int indexno = (t_int)atom_getfloatarg(0, ac--, av++);
    if (indexno < 0) indexno = 0;
    t_gobj *y;
    t_int i;
    for (i = 0, y = x->gl_list; y && i < indexno; i++)
        y = y->g_next;
    if (!y) return;
    if (forwardmess_recurse++ < 1)
        pd_forwardmess((t_pd *)y, ac, av);
    else
        pd_error(y, "error: canvas: forwardmess can't be in a recursive loop");
    forwardmess_recurse = 0;
}

/* ------------------ table ---------------------------*/

static int tabcount = 0;

static void *table_new(t_symbol *s, t_floatarg f)
{
    t_atom a[9];
    t_atom ga[4];
    t_glist *gl;
    t_canvas *x, *z = canvas_getcurrent();
    if (s == &s_)
    {
         char  tabname[255];
         t_symbol *t = gensym("table"); 
         sprintf(tabname, "%s%d", t->s_name, tabcount++);
         s = gensym(tabname); 
    }
    if (f <= 1)
        f = 100;
    SETFLOAT(a, 0);
    SETFLOAT(a+1, GLIST_DEFCANVASYLOC);
    SETFLOAT(a+2, 600);
    SETFLOAT(a+3, 400);
    SETSYMBOL(a+4, s);
    SETFLOAT(a+5, 0);
    x = canvas_new(0, 0, 6, a);

    x->gl_owner = z;

        /* create a graph for the table */
    gl = glist_addglist((t_glist*)x, &s_, 0, -1, (f > 1 ? f-1 : 1), 1,
        50, 350, 550, 50);

    SETSYMBOL(ga, s);
    SETFLOAT(ga+1, f);
    SETSYMBOL(ga+2, &s_float);
    SETFLOAT(ga+3, 0);
    graph_array(gl, gensym("array"), 4, ga);

    canvas_pop(x, 0); 

    return (x);
}

    /* return true if the "canvas" object is an abstraction (so we don't
    save its contents, fogr example.)  */
int canvas_isabstraction(t_canvas *x)
{
    return (x->gl_env != 0);
}

    /* return true if the "canvas" object should be bound to a name */
static int canvas_should_bind(t_canvas *x)
{
        /* FIXME should have a "backwards compatible" mode */
        /* not named "Pd" && (is top level || is subpatch) */
    return strcmp(x->gl_name->s_name, "Pd"); // && (!x->gl_owner || !x->gl_env);
}

static void canvas_bind(t_canvas *x)
{
    if (x->gl_isab) /* if it is an ab instance, we bind it to symbol 'ab-<name>' */
        pd_bind(&x->gl_pd, canvas_makebindsym_ab(x->gl_name));
    else if (canvas_should_bind(x))
        pd_bind(&x->gl_pd, canvas_makebindsym(x->gl_name));
}

static void canvas_unbind(t_canvas *x)
{
    if (x->gl_isab)
        pd_unbind(&x->gl_pd, canvas_makebindsym_ab(x->gl_name));
    else if (canvas_should_bind(x))
        pd_unbind(&x->gl_pd, canvas_makebindsym(x->gl_name));
}

    /* return true if the "canvas" object should be treated as a text
    object.  This is true for abstractions but also for "table"s... */
/* JMZ: add a flag to gop-abstractions to hide the title */
int canvas_showtext(t_canvas *x)
{
    t_atom *argv = (x->gl_obj.te_binbuf? binbuf_getvec(x->gl_obj.te_binbuf):0);
    int argc = (x->gl_obj.te_binbuf? binbuf_getnatom(x->gl_obj.te_binbuf) : 0);
    int isarray = (argc && argv[0].a_type == A_SYMBOL &&
        argv[0].a_w.w_symbol == gensym("graph"));
    if(x->gl_hidetext)
      return 0;
    else
      return (!isarray);
}

    /* get the document containing this canvas */
t_canvas *canvas_getrootfor(t_canvas *x)
{
    if ((!x->gl_owner) || canvas_isabstraction(x))
        return (x);
    else return (canvas_getrootfor(x->gl_owner));
}

/* ------------------------- DSP chain handling ------------------------- */

EXTERN_STRUCT _dspcontext;
#define t_dspcontext struct _dspcontext

void ugen_start(void);
void ugen_stop(void);

t_dspcontext *ugen_start_graph(int toplevel, t_signal **sp,
    int ninlets, int noutlets);
void ugen_add(t_dspcontext *dc, t_object *x);
void ugen_connect(t_dspcontext *dc, t_object *x1, int outno,
    t_object *x2, int inno);
void ugen_done_graph(t_dspcontext *dc);

    /* schedule one canvas for DSP.  This is called below for all "root"
    canvases, but is also called from the "dsp" method for sub-
    canvases, which are treated almost like any other tilde object.  */

void canvas_dodsp(t_canvas *x, int toplevel, t_signal **sp)
{
    t_linetraverser t;
    t_outconnect *oc;
    t_gobj *y;
    t_object *ob;
    t_symbol *dspsym = gensym("dsp");
    t_dspcontext *dc;    

    /* create a new "DSP graph" object to use in sorting this canvas.
       If we aren't toplevel, there are already other dspcontexts around. */

    dc = ugen_start_graph(toplevel, sp,
        obj_nsiginlets(&x->gl_obj),
        obj_nsigoutlets(&x->gl_obj));

    /* find all the "dsp" boxes and add them to the graph */

    if (x->gl_editor)
    {
        ob = &x->gl_editor->gl_magic_glass->x_obj;
        if (ob && magicGlass_bound(x->gl_editor->gl_magic_glass))
        {
            //fprintf(stderr,"adding cord inspector to dsp %d\n",
            //    magicGlass_bound(x->gl_magic_glass));
            // this t_canvas could be an array, hence no gl_magic_glass
            ugen_add(dc, ob);
        }
    }
    
    for (y = x->gl_list; y; y = y->g_next)
        if ((ob = pd_checkobject(&y->g_pd)) && zgetfn(&y->g_pd, dspsym))
            ugen_add(dc, ob);

    /* ... and all dsp interconnections */
    linetraverser_start(&t, x);
    while (oc = linetraverser_next(&t))
        if (obj_issignaloutlet(t.tr_ob, t.tr_outno))
            ugen_connect(dc, t.tr_ob, t.tr_outno, t.tr_ob2, t.tr_inno);

    /* finally, sort them and add them to the DSP chain */
    ugen_done_graph(dc);
}

static void canvas_dsp(t_canvas *x, t_signal **sp)
{
    canvas_dodsp(x, 0, sp);
}

    /* this routine starts DSP for all root canvases. */
static void canvas_dostart_dsp(void)
{
    t_canvas *x;
    if (pd_this->pd_dspstate)
        ugen_stop();
    else
        gui_vmess("gui_pd_dsp", "i", 1);
    ugen_start();

    for (x = pd_getcanvaslist(); x; x = x->gl_next)
        canvas_dodsp(x, 1, 0);
    
    canvas_dspstate = pd_this->pd_dspstate = 1;
    if (gensym("pd-dsp-started")->s_thing)
        pd_bang(gensym("pd-dsp-started")->s_thing);
}

static void canvas_start_dsp(void)
{
    pd_this->pd_dspstate_user = 1;
    canvas_dostart_dsp();
}

static void canvas_dostop_dsp(void)
{
    if (pd_this->pd_dspstate)
    {
        ugen_stop();
        gui_vmess("gui_pd_dsp", "i", 0);
        canvas_dspstate = pd_this->pd_dspstate = 0;
        if (gensym("pd-dsp-stopped")->s_thing)
            pd_bang(gensym("pd-dsp-stopped")->s_thing);
    }
}

static void canvas_stop_dsp(void)
{
    pd_this->pd_dspstate_user = 0;
    canvas_dostop_dsp();
}

    /* DSP can be suspended before, and resumed after, operations which
    might affect the DSP chain.  For example, we suspend before loading and
    resume afterward, so that DSP doesn't get resorted for every DSP object
    in the patch. */

int canvas_suspend_dsp(void)
{
    //fprintf(stderr,"canvas_suspend_dsp %d\n", rval);
    int rval = pd_this->pd_dspstate;
    if (rval) canvas_dostop_dsp();
    return (rval);
}

void canvas_resume_dsp(int oldstate)
{
    //fprintf(stderr,"canvas_resume_dsp %d\n", oldstate);
    if (oldstate) canvas_dostart_dsp();
}

    /* this is equivalent to suspending and resuming in one step. */
void canvas_update_dsp(void)
{
    if (pd_this->pd_dspstate) canvas_start_dsp();
}

/* the "dsp" message to pd starts and stops DSP computation, and, if
appropriate, also opens and closes the audio device.  On exclusive-access
APIs such as ALSA, MMIO, and ASIO (I think) it's appropriate to close the
audio devices when not using them; but jack behaves better if audio I/O
simply keeps running.  This is wasteful of CPU cycles but we do it anyway
and can perhaps regard this is a design flaw in jack that we're working around
here.  The function audio_shouldkeepopen() is provided by s_audio.c to tell
us that we should elide the step of closing audio when DSP is turned off.*/

void glob_dsp(void *dummy, t_symbol *s, int argc, t_atom *argv)
{
    int newstate;
    if (argc)
    {
        newstate = atom_getintarg(0, argc, argv);
        if (newstate && !pd_this->pd_dspstate)
        {
            sys_set_audio_state(1);
            canvas_start_dsp();
        }
        else if (!newstate && pd_this->pd_dspstate)
        {
            canvas_stop_dsp();
            sys_set_audio_state(0);
        }
    }
    else post("dsp state %d", pd_this->pd_dspstate);
}

void *canvas_getblock(t_class *blockclass, t_canvas **canvasp)
{
    t_canvas *canvas = *canvasp;
    t_gobj *g;
    void *ret = 0;
    for (g = canvas->gl_list; g; g = g->g_next)
    {
        if (g->g_pd == blockclass)
            ret = g;
    }
    *canvasp = canvas->gl_owner;
    return(ret);
}
    
/******************* redrawing  data *********************/

    /* redraw all "scalars" (do this if a drawing command is changed.) 
    LATER we'll use the "template" information to select which ones we
    redraw.   Action = 0 for redraw, 1 for draw only, 2 for erase. */
static void glist_redrawall(t_template *template, t_glist *gl, int action)
{
    //fprintf(stderr,"glist_redrawall\n");
    t_gobj *g;
    int vis = glist_isvisible(gl);
    for (g = gl->gl_list; g; g = g->g_next)
    {
        if (vis && g->g_pd == scalar_class &&
            ((template == template_findbyname(((t_scalar *)g)->sc_template))
            || template_has_elemtemplate(
                   template_findbyname(((t_scalar *)g)->sc_template),
                       template)))
        {
            if (action == 1)
            {
                if (glist_isvisible(gl))
                    gobj_vis(g, gl, 1);
            }
            else if (action == 2)
            {
                if (glist_isvisible(gl))
                    gobj_vis(g, gl, 0);
            }
            else scalar_redraw((t_scalar *)g, gl);
        }
        else if (g->g_pd == canvas_class)
            glist_redrawall(template, (t_glist *)g, action);
    }
    if (glist_isselected(glist_getcanvas(gl), (t_gobj *)gl))
    {
        /* Haven't tested scalars inside gop yet, but we
           probably need a gui_vmess here */
        sys_vgui("pdtk_select_all_gop_widgets .x%lx %lx %d\n",
            glist_getcanvas(gl), gl, 1);
    }
}

    /* public interface for above. */
void canvas_redrawallfortemplate(t_template *template, int action)
{
    t_canvas *x;
        /* find all root canvases */
    for (x = pd_this->pd_canvaslist; x; x = x->gl_next)
        glist_redrawall(template, x, action);
}

    /* find the template defined by a canvas, and redraw all elements
    for that */
extern t_canvas *canvas_templatecanvas_forgroup(t_canvas *x);

void canvas_redrawallfortemplatecanvas(t_canvas *x, int action)
{
    //fprintf(stderr,"canvas_redrawallfortemplatecanvas\n");
    t_gobj *g;
    t_template *tmpl;
    t_symbol *s1 = gensym("struct");
    for (g = x->gl_list; g; g = g->g_next)
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
        tmpl = template_findbyname(canvas_makebindsym(argv[1].a_w.w_symbol));
        canvas_redrawallfortemplate(tmpl, action);
    }
    canvas_redrawallfortemplate(0, action);
}

/* ------------------------------- ab ------------------------ */

static char ab_templatecanvas[] = "#N canvas;\n";

/* create an ab instance from its source */
static t_pd *do_create_ab(t_ab_definition *abdef, int argc, t_atom *argv)
{
    canvas_setargs(argc, argv);
    int dspstate = canvas_suspend_dsp();
    glob_setfilename(0, abdef->ad_name, gensym("[ab]"));

    /* set ab source, next canvas is going to be a private abstraction */
    canvas_setabsource(abdef);
    binbuf_eval(abdef->ad_source, 0, 0, 0);
    canvas_initbang((t_canvas *)(s__X.s_thing));

    glob_setfilename(0, &s_, &s_);
    canvas_resume_dsp(dspstate);
    canvas_popabstraction((t_canvas *)(s__X.s_thing));
    canvas_setargs(0, 0);

    /* open the canvas if we are creating it for the first time */
    canvas_vis((t_canvas *)newest, !glist_amreloadingabstractions
                                    && !abdef->ad_numinstances
                                    && binbuf_getnatom(abdef->ad_source) == 3);

    return(newest);
}

/* get root canvas crossing ab boundaries, where ab definitions are stored */
t_canvas *canvas_getrootfor_ab(t_canvas *x)
{
    if ((!x->gl_owner && !x->gl_isclone) || (canvas_isabstraction(x) && !x->gl_isab))
        return (x);
    else if (x->gl_isab) /* shortcut + workaround for clones (since they haven't owner)*/
        return (x->gl_absource->ad_owner);
    else
        return (canvas_getrootfor_ab(x->gl_owner));
}

/* check if the dependency graph has a cycle, assuming an new edge between parent and
    current nodes if there is a cycle, a visual scheme of the cycle is stored in 'res' */
static int ab_check_cycle(t_ab_definition *current, t_ab_definition *parent, int pathlen,
    char *path, char *res)
{
    if(current == parent)
    {
        sprintf(path+pathlen, "[ab %s]", current->ad_name->s_name);
        strcpy(res, path);
        return (1);
    }
    else
    {
        /* if it is a local private abstraction, get rid of classmember-like names (only used internally) */
        char *hash = strrchr(current->ad_name->s_name, '#');
        if(!hash) hash = current->ad_name->s_name;
        else hash += 1;
        int len = strlen(hash);
        sprintf(path+pathlen, "[ab %s]<-", hash);
        pathlen += (len+7);
        int i, cycle = 0;
        for(i = 0; !cycle && i < current->ad_numdep; i++)
        {
            cycle = ab_check_cycle(current->ad_dep[i], parent, pathlen, path, res);
        }
        pathlen -= (len+7);
        return (cycle);
    }
}

/* try to register a new dependency into the dependency graph,
    returns 0 and the scheme in 'res' if a dependency issue is found */
static int canvas_register_ab(t_canvas *x, t_ab_definition *a, char *res)
{
    /* climb to closest ab */
    while(x && !x->gl_isab)
        x = x->gl_owner;

    if(x && x->gl_isab)
    {
        t_ab_definition *f = x->gl_absource;

        int i, found = 0;
        for(i = 0; !found && i < a->ad_numdep; i++)
            found = (a->ad_dep[i] == f);

        if(!found)
        {
            char path[MAXPDSTRING];
            sprintf(path, "[ab %s]<-", a->ad_name->s_name);
            if(!ab_check_cycle(f, a, strlen(path), path, res))
            {
                /* no dependency issues found so we add the new dependency */
                a->ad_dep =
                    (t_ab_definition **)resizebytes(a->ad_dep, sizeof(t_ab_definition *)*a->ad_numdep,
                            sizeof(t_ab_definition *)*(a->ad_numdep+1));
                a->ad_deprefs =
                    (int *)resizebytes(a->ad_deprefs, sizeof(int)*a->ad_numdep,
                            sizeof(int)*(a->ad_numdep+1));
                a->ad_dep[a->ad_numdep] = f;
                a->ad_deprefs[a->ad_numdep] = 1;
                a->ad_numdep++;
            }
            else return (0);
        }
        else
        {
            a->ad_deprefs[i-1]++;
        }
    }
    return (1);
}

static void canvas_deregister_ab(t_canvas *x, t_ab_definition *a)
{
    /* climb to closest ab */
    while(x && !x->gl_isab)
        x = x->gl_owner;

    if(x && x->gl_isab)
    {
        t_ab_definition *f = x->gl_absource;

        int i, found = 0;
        for(i = 0; !found && i < a->ad_numdep; i++)
            found = (a->ad_dep[i] == f);

        if(found)
        {
            a->ad_deprefs[i-1]--;

            if(!a->ad_deprefs[i-1])
            {
                /* we can delete the dependency since there are no instances left */
                t_ab_definition **ad =
                        (t_ab_definition **)getbytes(sizeof(t_ab_definition *) * (a->ad_numdep - 1));
                int *adr = (int *)getbytes(sizeof(int) * (a->ad_numdep - 1));
                memcpy(ad, a->ad_dep, sizeof(t_ab_definition *) * (i-1));
                memcpy(ad+(i-1), a->ad_dep+i, sizeof(t_ab_definition *) * (a->ad_numdep - i));
                memcpy(adr, a->ad_deprefs, sizeof(int) * (i-1));
                memcpy(adr+(i-1), a->ad_deprefs+i, sizeof(int) * (a->ad_numdep - i));
                freebytes(a->ad_dep, sizeof(t_ab_definition *) * a->ad_numdep);
                freebytes(a->ad_deprefs, sizeof(int) * a->ad_numdep);
                a->ad_numdep--;
                a->ad_dep = ad;
                a->ad_deprefs = adr;
            }
        }
        else bug("canvas_deregister_ab");
    }
}

/* tries to find an ab definition given its name */
static t_ab_definition *canvas_find_ab(t_canvas *x, t_symbol *name)
{
    t_canvas *c = canvas_getrootfor_ab(x);
    t_ab_definition* d;
    for (d = c->gl_abdefs; d; d = d->ad_next)
    {
        if (d->ad_name == name)
            return d;
    }
    return 0;
}

/* tries to add a new ab definition. returns the definition if it has been added, 0 otherwise */
static t_ab_definition *canvas_add_ab(t_canvas *x, t_symbol *name, t_binbuf *source)
{
    if(!canvas_find_ab(x, name))
    {
        t_canvas *c = canvas_getrootfor_ab(x);
        t_ab_definition *abdef = (t_ab_definition *)getbytes(sizeof(t_ab_definition));

        abdef->ad_name = name;
        abdef->ad_source = source;
        abdef->ad_numinstances = 0;
        abdef->ad_owner = c;
        abdef->ad_numdep = 0;
        abdef->ad_dep = (t_ab_definition **)getbytes(0);
        abdef->ad_deprefs = (int *)getbytes(0);
        abdef->ad_visflag = 0;

        abdef->ad_next = c->gl_abdefs;
        c->gl_abdefs = abdef;
        return (abdef);
    }
    return (0);
}

static int canvas_del_ab(t_canvas *x, t_symbol *name)
{
    t_canvas *c = canvas_getrootfor_ab(x);
    t_ab_definition *abdef, *abdefpre;
    for(abdef = c->gl_abdefs, abdefpre = 0; abdef; abdefpre = abdef, abdef = abdef->ad_next)
    {
        if(abdef->ad_name == name && !abdef->ad_numinstances)
        {
            if(abdefpre) abdefpre->ad_next = abdef->ad_next;
            else c->gl_abdefs = abdef->ad_next;
            binbuf_free(abdef->ad_source);
            freebytes(abdef->ad_dep, sizeof(t_ab_definition*)*abdef->ad_numdep);
            freebytes(abdef->ad_deprefs, sizeof(int)*abdef->ad_numdep);
            freebytes(abdef, sizeof(t_ab_definition));
            return (1);
        }
    }
    return (0);
}

/* given the ab definitions list, returns its topological ordering */

static int currvisflag = 0;

static void ab_topological_sort_rec(t_ab_definition *a, t_ab_definition **stack, int *head)
{
    a->ad_visflag = currvisflag;

    int i;
    for(i = 0; i < a->ad_numdep; i++)
    {
        if(a->ad_dep[i]->ad_visflag != currvisflag)
            ab_topological_sort_rec(a->ad_dep[i], stack, head);
    }

    stack[*head] = a;
    (*head)++;
}

static void ab_topological_sort(t_ab_definition *abdefs, t_ab_definition **stack, int *head)
{
    currvisflag++;
    t_ab_definition *abdef;
    for(abdef = abdefs; abdef; abdef = abdef->ad_next)
    {
        if(abdef->ad_visflag != currvisflag)
            ab_topological_sort_rec(abdef, stack, head);
    }
}

/* saves all ab definition within the scope into the b binbuf,
    they are sorted topollogially before saving in order to get exactly the
    same state (ab objects that can't be instantiated due dependencies) when reloading the file */
void canvas_saveabdefinitionsto(t_canvas *x, t_binbuf *b)
{
    if(!x->gl_abdefs)
        return;

    int numabdefs = 0;
    t_ab_definition *abdef;
    for(abdef = x->gl_abdefs; abdef; abdef = abdef->ad_next)
        numabdefs++;

    t_ab_definition **stack =
        (t_ab_definition **)getbytes(sizeof(t_ab_definition *) * numabdefs);
    int head = 0;
    ab_topological_sort(x->gl_abdefs, stack, &head);

    int i, fra = 0;
    for(i = 0; i < head; i++)
    {
        if(stack[i]->ad_numinstances)
        {
            if(!fra)
            {
                binbuf_addv(b, "ssi;", gensym("#X"), gensym("abframe"), 1);
                fra = 1;
            }

            binbuf_add(b, binbuf_getnatom(stack[i]->ad_source), binbuf_getvec(stack[i]->ad_source));
            binbuf_addv(b, "sss", gensym("#X"), gensym("abpush"), stack[i]->ad_name);

            int j;
            for(j = 0; j < stack[i]->ad_numdep; j++)
                binbuf_addv(b, "s", stack[i]->ad_dep[j]->ad_name);
            binbuf_addsemi(b);
        }
    }
    if(fra) binbuf_addv(b, "ssi;", gensym("#X"), gensym("abframe"), 0);

    freebytes(stack, sizeof(t_ab_definition *) * numabdefs);
}

/* saves last canvas as an ab definition */
static void canvas_abpush(t_canvas *x, t_symbol *s, int argc, t_atom *argv)
{
    canvas_pop(x, 0);

    t_canvas *c = canvas_getcurrent();
    t_symbol *name = argv[0].a_w.w_symbol;
    t_binbuf *source = binbuf_new();
    x->gl_env = 0xF1A6; //to save it as a root canvas
    mess1(&((t_text *)x)->te_pd, gensym("saveto"), source);
    x->gl_env = 0;

    t_ab_definition *n;
    if(!(n = canvas_add_ab(c, name, source)))
    {
        error("canvas_abpush: ab definition for '%s' already exists, skipping",
                name->s_name);
    }
    else
    {
        if(argc > 1)
        {
            /* restore all dependencies, to get exactly the
                same state (ab objects that can't be instantiated due dependencies) as before */
            n->ad_numdep = argc-1;
            n->ad_dep =
                (t_ab_definition **)resizebytes(n->ad_dep, 0, sizeof(t_ab_definition *)*n->ad_numdep);
            n->ad_deprefs =
                (int *)resizebytes(n->ad_deprefs, 0, sizeof(int)*n->ad_numdep);

            int i;
            for(i = 1; i < argc; i++)
            {
                t_symbol *abname = argv[i].a_w.w_symbol;
                t_ab_definition *absource = canvas_find_ab(c, abname);
                if(!absource) { bug("canvas_abpush"); return; }
                n->ad_dep[i-1] = absource;
                n->ad_deprefs[i-1] = 0;
            }
        }
    }

    pd_free(&x->gl_pd);
}

/* extends the name for a local ab, using a classmember-like format */
static t_symbol *ab_extend_name(t_canvas *x, t_symbol *s)
{
    char res[MAXPDSTRING];
    t_canvas *next = canvas_getrootfor(x);
    if(next->gl_isab)
        sprintf(res, "%s#%s", next->gl_absource->ad_name->s_name, s->s_name);
    else
        strcpy(res, s->s_name);
    return gensym(res);
}

static int abframe = 0;
static void canvas_abframe(t_canvas *x, t_float val)
{
    abframe = val;
}

extern t_class *text_class;

/* creator for "ab" objects */
static void *ab_new(t_symbol *s, int argc, t_atom *argv)
{
    if(abframe)
        /* return dummy text object so that creator
            does not throw an error */
        return pd_new(text_class);

    t_canvas *c = canvas_getcurrent();

    if (argc && argv[0].a_type != A_SYMBOL)
    {
        error("ab_new: ab name must be a symbol");
        newest = 0;
    }
    else
    {
        t_symbol *name = (argc ? argv[0].a_w.w_symbol : gensym("(ab)"));
        t_ab_definition *source;

        if(name->s_name[0] == '@') /* is local ab */
            name = ab_extend_name(c, name);

        if(!(source = canvas_find_ab(c, name)))
        {
            t_binbuf *b = binbuf_new();
            binbuf_text(b, ab_templatecanvas, strlen(ab_templatecanvas));
            source = canvas_add_ab(c, name, b);
        }

        char res[MAXPDSTRING];
        if(canvas_register_ab(c, source, res))
        {
            newest = do_create_ab(source, (argc ? argc-1 : 0), (argc ? argv+1 : 0));
            source->ad_numinstances++;
        }
        else
        {
            error("ab_new: can't insantiate ab within itself\n cycle: %s", res);
            newest = 0;
        }
    }
    return (newest);
}

static void canvas_getabstractions(t_canvas *x)
{
    t_canvas *c = canvas_getrootfor_ab(x),
             *r = canvas_getrootfor(x);
    gfxstub_deleteforkey(x);
    char *gfxstub = gfxstub_new2(&x->gl_pd, &x->gl_pd);
    t_ab_definition *abdef;
    gui_start_vmess("gui_abstractions_dialog", "xs", x, gfxstub);
    gui_start_array();
    gui_end_array();
    gui_start_array();
    for(abdef = c->gl_abdefs; abdef; abdef = abdef->ad_next)
    {
        char *hash = strrchr(abdef->ad_name->s_name, '#');
        if(!hash)
        {
            if(abdef->ad_name->s_name[0] != '@' || !r->gl_isab)
            {
                gui_s(abdef->ad_name->s_name);
                gui_i(abdef->ad_numinstances);
            }
        }
        else
        {
            *hash = '\0';
            if(r->gl_isab &&
                gensym(abdef->ad_name->s_name) == r->gl_absource->ad_name)
            {
                gui_s(hash+1);
                gui_i(abdef->ad_numinstances);
            }
            *hash = '#';
        }
    }
    gui_end_array();
    gui_end_vmess();
}

static void canvas_delabstractions(t_canvas *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *name;
    int i;
    for(i = 0; i < argc; i++)
    {
        name = atom_getsymbol(argv++);
        if(name->s_name[0] == '@')
            name = ab_extend_name(x, name);
        if(!canvas_del_ab(x, name))
            bug("canvas_delabstractions");
    }
    startpost("info: a total of [%d] ab definitions have been deleted\n      > ", argc);
    postatom(argc, argv-argc);
    endpost();
}

/* --------- */

static t_class *abdefs_class;

typedef struct _abdefs
{
    t_object x_obj;
    t_canvas *x_canvas;
} t_abdefs;

static void *abdefs_new(void)
{
    t_abdefs *x = (t_abdefs *)pd_new(abdefs_class);
    x->x_canvas = canvas_getcurrent();
    outlet_new(&x->x_obj, &s_list);
    return (x);
}

static void abdefs_get(t_abdefs *x);
static void abdefs_bang(t_abdefs *x)
{
    abdefs_get(x);
}

static void abdefs_get(t_abdefs *x)
{
    t_canvas *r = canvas_getrootfor_ab(x->x_canvas);
    t_binbuf *out = binbuf_new();
    t_ab_definition *abdef;
    for(abdef = r->gl_abdefs; abdef; abdef = abdef->ad_next)
    {
        binbuf_addv(out, "si", abdef->ad_name, abdef->ad_numinstances);
    }
    outlet_list(x->x_obj.ob_outlet, &s_list,
        binbuf_getnatom(out), binbuf_getvec(out));
    binbuf_free(out);
}

static void abdefs_instances(t_abdefs *x, t_symbol *s)
{
    t_ab_definition *abdef;
    if((abdef = canvas_find_ab(x->x_canvas, s)))
    {
        t_atom at[1];
        SETFLOAT(at, abdef->ad_numinstances);
        outlet_list(x->x_obj.ob_outlet, &s_list, 1, at);
    }
    else
        error("abdefs: couldn't find definition for '%s'", s->s_name);
}

static void abdefs_menuopen(t_abdefs *x)
{
    char buf[MAXPDSTRING];
    t_canvas *c = canvas_getrootfor_ab(x->x_canvas);
    t_ab_definition *abdef;
    char *gfx_tag = gfxstub_new2(&x->x_obj.ob_pd, x);
    gui_start_vmess("gui_external_dialog", "s", gfx_tag);
    gui_s("[ab] definitions");
    gui_start_array();
    if(!c->gl_abdefs)
    {
        gui_s("Ø_hidden"); gui_i(0);
    }
    for(abdef = c->gl_abdefs; abdef; abdef = abdef->ad_next)
    {
        sprintf(buf, "%s (%d)_hidden", abdef->ad_name->s_name, abdef->ad_numinstances);
        gui_s(buf); gui_i(0);
    }
    gui_end_array();
    gui_end_vmess();
}

static void abdefs_dialog(t_abdefs *x, t_symbol *s, int argc, t_atom *argv)
{
    gfxstub_deleteforkey(x);
}

/* --------- */

static void canvas_showdirty(t_canvas *x)
{
    if(!x->gl_isab)
        canvas_dirty_broadcast_all(x->gl_name, canvas_getdir(x), 2);
    else
        canvas_dirty_broadcast_ab_all(x->gl_absource, 2);
}

/* ------------------------------- declare ------------------------ */

/* put "declare" objects in a patch to tell it about the environment in
which objects should be created in this canvas.  This includes directories to
search ("-path", "-stdpath") and object libraries to load
("-lib" and "-stdlib").  These must be set before the patch containing
the "declare" object is filled in with its contents; so when the patch is
saved,  we throw early messages to the canvas to set the environment
before any objects are created in it. */

static t_class *declare_class;

typedef struct _declare
{
    t_object x_obj;
    t_canvas *x_canvas;
    int x_useme;
} t_declare;

static void *declare_new(t_symbol *s, int argc, t_atom *argv)
{
    t_declare *x = (t_declare *)pd_new(declare_class);
    x->x_useme = 1;
    x->x_canvas = canvas_getcurrent();
        /* LATER update environment and/or load libraries */
    if (!x->x_canvas->gl_loading)
    {
        /* the object is created by the user (not by loading a patch),
         * so update canvas's properties on the fly */
        canvas_declare(x->x_canvas, s, argc, argv);
    }
    return (x);
}

static void declare_free(t_declare *x)
{
    x->x_useme = 0;
        /* LATER update environment */
}

void canvas_savedeclarationsto(t_canvas *x, t_binbuf *b)
{
    t_gobj *y;

    for (y = x->gl_list; y; y = y->g_next)
    {
        if (pd_class(&y->g_pd) == declare_class)
        {
            binbuf_addv(b, "s", gensym("#X"));
            binbuf_addbinbuf(b, ((t_declare *)y)->x_obj.te_binbuf);
            binbuf_addv(b, ";");
        }
            /* before 0.47 we also allowed abstractions to write out to the
            parent's declarations; now we only allow non-abstraction subpatches
            to do so. */
        else if (pd_checkglist(&y->g_pd) &&
            (pd_compatibilitylevel < 47 || !canvas_isabstraction((t_canvas *)y)))
                canvas_savedeclarationsto((t_canvas *)y, b);
    }
}

static void canvas_completepath(char *from, char *to, int bufsize)
{
    if (sys_isabsolutepath(from))
    {
        to[0] = '\0';
    }
    else
    {   // if not absolute path, append Pd lib dir
        strncpy(to, sys_libdir->s_name, bufsize-10);
        to[bufsize-9] = '\0';
        strcat(to, "/extra/");
    }
    strncat(to, from, bufsize-strlen(to));
    to[bufsize-1] = '\0';
}

/* maybe we should rename check_exists() to sys_access() and move it to s_path */
#ifdef _WIN32
static int check_exists(const char*path)
{
    char pathbuf[MAXPDSTRING];
    wchar_t ucs2path[MAXPDSTRING];
    sys_bashfilename(path, pathbuf);
    u8_utf8toucs2(ucs2path, MAXPDSTRING, pathbuf, MAXPDSTRING-1);
    return (0 ==  _waccess(ucs2path, 0));
}
#else
#include <unistd.h>
static int check_exists(const char*path)
{
    char pathbuf[MAXPDSTRING];
    sys_bashfilename(path, pathbuf);
    return (0 == access(pathbuf, 0));
}
#endif

//extern t_namelist *sys_staticpath;

static void canvas_stdpath(t_canvasenvironment *e, char *stdpath)
{
    t_namelist*nl;
    char strbuf[MAXPDSTRING];
    if (sys_isabsolutepath(stdpath))
    {
        e->ce_path = namelist_append(e->ce_path, stdpath, 0);
        return;
    }
    /* strip    "extra/"-prefix */
    if (!strncmp("extra/", stdpath, 6))
        stdpath+=6;

        /* prefix full pd-path (including extra) */
    canvas_completepath(stdpath, strbuf, MAXPDSTRING);
    if (check_exists(strbuf))
    {
        e->ce_path = namelist_append(e->ce_path, strbuf, 0);
        return;
    }
    /* check whether the given subdir is in one of the standard-paths */
    for (nl=pd_extrapath; nl; nl=nl->nl_next)
    {
        snprintf(strbuf, MAXPDSTRING-1, "%s/%s/", nl->nl_string, stdpath);
        strbuf[MAXPDSTRING-1]=0;
        if (check_exists(strbuf))
        {
            e->ce_path = namelist_append(e->ce_path, strbuf, 0);
            return;
        }
    }
}
static void canvas_stdlib(t_canvasenvironment *e, char *stdlib)
{
    t_namelist*nl;
    char strbuf[MAXPDSTRING];
    if (sys_isabsolutepath(stdlib))
    {
        sys_load_lib(0, stdlib);
        return;
    }

        /* strip    "extra/"-prefix */
    if (!strncmp("extra/", stdlib, 6))
        stdlib+=6;

        /* prefix full pd-path (including extra) */
    canvas_completepath(stdlib, strbuf, MAXPDSTRING);
    if (sys_load_lib(0, strbuf))
        return;

    /* check whether the given library is located in one of the standard-paths */
    for (nl=pd_extrapath; nl; nl=nl->nl_next)
    {
        snprintf(strbuf, MAXPDSTRING-1, "%s/%s", nl->nl_string, stdlib);
        strbuf[MAXPDSTRING-1]=0;
        if (sys_load_lib(0, strbuf))
            return;
    }
}

extern t_symbol *class_loadsym;     /* name under which an extern is invoked */

void canvas_declare(t_canvas *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_canvasenvironment *e = canvas_getenv(x);
#if 0
    startpost("declare:: %s", s->s_name);
    postatom(argc, argv);
    endpost();
#endif
    for (i = 0; i < argc; i++)
    {
        char *flag = atom_getsymbolarg(i, argc, argv)->s_name;
        if ((argc > i+1) && !strcmp(flag, "-path"))
        {
            e->ce_path = namelist_append(e->ce_path,
                atom_getsymbolarg(i+1, argc, argv)->s_name, 0);
            i++;
        }
        else if ((argc > i+1) && !strcmp(flag, "-stdpath"))
        {
            canvas_stdpath(e, atom_getsymbolarg(i+1, argc, argv)->s_name);
            i++;
        }
        else if ((argc > i+1) && !strcmp(flag, "-lib"))
        {
            /* set class_loadsym in case we're loading a library by
               absolute or namespace-prefixed path. Not sure yet
               exactly how stdlib works so I haven't touched that
               one... */
            class_loadsym = atom_getsymbolarg(i+1, argc, argv);
            sys_load_lib(x, class_loadsym->s_name);
            class_loadsym = NULL;
            i++;
        }
        else if ((argc > i+1) && !strcmp(flag, "-stdlib"))
        {
            canvas_stdlib(e, atom_getsymbolarg(i+1, argc, argv)->s_name);
            i++;
        }
        else post("declare: %s: unknown declaration", flag);
    }
}

typedef struct _canvasopen
{
    const char *name;
    const char *ext;
    char *dirresult;
    char **nameresult;
    unsigned int size;
    int bin;
    int fd;
} t_canvasopen;

static int canvas_open_iter(const char *path, t_canvasopen *co)
{
    int fd;
    if ((fd = sys_trytoopenone(path, co->name, co->ext,
        co->dirresult, co->nameresult, co->size, co->bin)) >= 0)
    {
        co->fd = fd;
        return 0;
    }
    return 1;
}

    /* utility function to read a file, looking first down the canvas's search
    path (set with "declare" objects in the patch and recursively in calling
    patches), then down the system one.  The filename is the concatenation of
    "name" and "ext".  "Name" may be absolute, or may be relative with
    slashes.  If anything can be opened, the true directory
    is put in the buffer dirresult (provided by caller), which should
    be "size" bytes.  The "nameresult" pointer will be set somewhere in
    the interior of "dirresult" and will give the file basename (with
    slashes trimmed).  If "bin" is set a 'binary' open is
    attempted, otherwise ASCII (this only matters on Microsoft.)
    If "x" is zero, the file is sought in the directory "." or in the
    global path.*/
int canvas_open(t_canvas *x, const char *name, const char *ext,
    char *dirresult, char **nameresult, unsigned int size, int bin)
{
    int fd = -1;
    char final_name[FILENAME_MAX];
    t_canvasopen co;

    sys_expandpathelems(name, final_name);

        /* first check if "name" is absolute (and if so, try to open) */
    if (sys_open_absolute(final_name, ext, dirresult, nameresult, size, bin, &fd))
        return (fd);
        /* otherwise "name" is relative; iterate over all the search-paths */
    co.name = final_name;
    co.ext = ext;
    co.dirresult = dirresult;
    co.nameresult = nameresult;
    co.size = size;
    co.bin = bin;
    co.fd = -1;

    canvas_path_iterate(x, (t_canvas_path_iterator)canvas_open_iter, &co);

    return (co.fd);
}

int canvas_path_iterate(t_canvas*x, t_canvas_path_iterator fun, void *user_data)
{
    t_canvas *y = 0;
    t_namelist *nl = 0;
    int count = 0;
    if (!fun)
        return 0;
        /* iterate through canvas-local paths */
    for (y = x; y; y = y->gl_owner)
        if (y->gl_env)
    {
        t_canvas *x2 = x;
        char *dir;
        while (x2 && x2->gl_owner)
            x2 = x2->gl_owner;
        dir = (x2 ? canvas_getdir(x2)->s_name : ".");
        for (nl = y->gl_env->ce_path; nl; nl = nl->nl_next)
        {
            char realname[MAXPDSTRING];
            if (sys_isabsolutepath(nl->nl_string))
                realname[0] = '\0';
            else
            {   /* if not absolute path, append Pd lib dir */
                strncpy(realname, dir, MAXPDSTRING);
                realname[MAXPDSTRING-3] = 0;
                strcat(realname, "/");
            }
            strncat(realname, nl->nl_string, MAXPDSTRING-strlen(realname));
            realname[MAXPDSTRING-1] = 0;
            if (!fun(realname, user_data))
                return count+1;
            count++;
        }
    }
    /* try canvas dir */
    if (!fun((x ? canvas_getdir(x)->s_name : "."), user_data))
        return count+1;
    count++;

    /* now iterate through the global paths */
    for (nl = sys_searchpath; nl; nl = nl->nl_next)
    {
        if (!fun(nl->nl_string, user_data))
            return count+1;
        count++;
    }
    /* and the default paths */
    if (sys_usestdpath)
        for (nl = pd_extrapath; nl; nl = nl->nl_next)
        {
            if (!fun(nl->nl_string, user_data))
                return count+1;
            count++;
        }

    return count;
}

/*int canvas_open(t_canvas *x, const char *name, const char *ext,
    char *dirresult, char **nameresult, unsigned int size, int bin)
{
    int fd = -1;
    int result = 0;
    t_canvas *y;
    char final_name[FILENAME_MAX];

    // first check for @pd_extra (and later possibly others)
    // and ~/ and replace
    sys_expandpathelems(name, final_name);

    // first check if "name" is absolute (and if so, try to open)
    if (sys_open_absolute(final_name, ext, dirresult, nameresult, size, bin, &fd))
        return (fd);
    
    // otherwise "name" is relative; start trying in directories named
    // in this and parent environments
    for (y = x; y; y = y->gl_owner)
        if (y->gl_env)
        {
            t_namelist *nl;
            t_canvas *x2 = x;
            char *dir;
            while (x2 && x2->gl_owner)
                x2 = x2->gl_owner;
            dir = (x2 ? canvas_getdir(x2)->s_name : ".");
            for (nl = y->gl_env->ce_path; nl; nl = nl->nl_next)
            {
                char realname[FILENAME_MAX];
                if (sys_isabsolutepath(nl->nl_string))
                {
                    realname[0] = '\0';
                }
                else
                {   // if not absolute path, append Pd lib dir
                    strncpy(realname, dir, FILENAME_MAX);
                    realname[FILENAME_MAX-3] = 0;
                    strcat(realname, "/");
                }
                strncat(realname, nl->nl_string, FILENAME_MAX-strlen(realname));
                realname[FILENAME_MAX-1] = 0;
                if ((fd = sys_trytoopenone(realname, final_name, ext,
                    dirresult, nameresult, size, bin)) >= 0)
                        return (fd);
            }
        }
    result = open_via_path((x ? canvas_getdir(x)->s_name : "."),
        final_name, ext, dirresult, nameresult, size, bin);
    return(result);
}*/

extern t_symbol *last_typedmess; // see m_class.c for details on this ugly hack
extern t_pd *last_typedmess_pd; // same as above

static void canvas_f(t_canvas *x, t_symbol *s, int argc, t_atom *argv)
{
    static int warned_future_version;
    static int warned_old_syntax;
    //fprintf(stderr,"canvas_f %lx %d current=%lx %s\n",
    //    (t_int)x, argc, canvas_getcurrent(),
    //    last_typedmess != NULL ? last_typedmess->s_name : "none");
    t_canvas *xp = x; //parent window for a special case dealing with subpatches
    t_gobj *g, *g2;
    t_object *ob;
    if (argc > 1 && !warned_future_version)
    {
        post("** ignoring width or font settings from future Pd version **");
        warned_future_version = 1;
    }
    // if we are part of a restore message
    // of a subpatch in the form "#X restore..., f 123456789+;"
    if (!x->gl_list || !strcmp(last_typedmess->s_name, "restore"))
    {
        if (x->gl_owner && !x->gl_isgraph)
        {
            // this means that we are a canvas that
            // just finished restoring and that our width applies to our
            // appearance on our parent.
            if (!warned_old_syntax)
            {
                post("warning: old box-width syntax detected. Please save the "
                     "patch to switch to the new syntax.");
                warned_old_syntax = 1;
            }
            xp = x->gl_owner;
            g = &x->gl_gobj;
        }
        else return;
    }
    else
    {
        for (g = x->gl_list; g2 = g->g_next; g = g2)
            ;
        //fprintf(stderr,"same canvas .x%lx .x%lx\n", (t_int)g, (t_int)x);
    }
    if ((ob = pd_checkobject(&g->g_pd)) || pd_class(&g->g_pd) == canvas_class)
    {
        //fprintf(stderr,"f received\n");
        ob->te_width = atom_getfloatarg(0, argc, argv);
        if (glist_isvisible(xp))
        {
            gobj_vis(g, xp, 0);
            gobj_vis(g, xp, 1);
        }
    }
}

/* Not sure if this is still used... */
void canvasgop_draw_move(t_canvas *x, int doit)
{
    //delete the earlier GOP window so that when dragging 
    //there is only one GOP window present on parent
    /* don't think we need this anymore */
    //sys_vgui(".x%lx.c delete GOP\n",  x);
        
    //redraw the GOP
    canvas_setgraph(x, x->gl_isgraph+2*x->gl_hidetext, 0);
    canvas_dirty(x, 1);
    if (x->gl_havewindow)
    {
        canvas_redraw(x);
    }

    if (x->gl_owner && glist_isvisible(x->gl_owner))
    {
        glist_noselect(x);
        //vmess(&x->gl_owner->gl_obj.te_pd, gensym("menu-open"), "");
        gobj_vis(&x->gl_gobj, x->gl_owner, 0);
        gobj_vis(&x->gl_gobj, x->gl_owner, 1);
        //canvas_redraw(x->gl_owner);
    }
    
    //update scrollbars when GOP potentially exceeds window size
    t_canvas *canvas=(t_canvas *)glist_getcanvas(x);
    
    //if gop is being disabled go one level up
    if (!x->gl_isgraph && x->gl_owner && glist_isvisible(x->gl_owner))
    {
        canvas=canvas->gl_owner;
        //canvas_redraw(canvas);
    }
    scrollbar_update(x);
    if (x->gl_owner && glist_isvisible(x->gl_owner))
        scrollbar_update(x->gl_owner);
}

extern int gfxstub_haveproperties(void *key);
extern void canvas_canvas_setundo(t_canvas *x);
extern void graph_checkgop_rect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2);

/* we use the following function after saving to file
   gop-enabled canvas with hidetext disabled to check
   whether the requested size fits the newfound canvas
   name (as reflected in its new filename)

   LATER: use this for __clickhook below and possibly
   other places as well */
void canvasgop_checksize(t_canvas *x)
{
    if (x->gl_isgraph)
    {
        int x1=0, y1=0, x2=0, y2=0;
        int dirty=0;
        if (x->gl_owner)
        {
            gobj_getrect((t_gobj*)x, x->gl_owner,
                &x1, &y1, &x2, &y2);
        }
        else
        {
            graph_checkgop_rect((t_gobj*)x, x, &x1, &y1, &x2, &y2);
        }
        if (x2-x1 > x->gl_pixwidth)
        {
            x->gl_pixwidth = x2-x1;
            dirty = 1;
        }
        if (y2-y1 > x->gl_pixheight)
        { 
            x->gl_pixheight = y2-y1;
            dirty = 1;
        }

        if (dirty)
        {
            post("Warning: "
                 "Adjusting canvas graph-on-parent area to accomodate "
                 "its name. If you want to have a smaller graph-on-parent "
                 "window, please hide graph text.");
            canvas_dirty(x, 1);
            canvasgop_draw_move(x,1);
            canvas_fixlinesfor(x, (t_text *)x);
            scrollbar_update(x);
        }
    }
}

void canvasgop__clickhook(t_scalehandle *sh, int newstate)
{
    t_canvas *x = (t_canvas *)(sh->h_master);

    /* So ugly: if the user is dragging the bottom right-hand corner of
       a gop subcanvas on the parent, we already set an undo event for it.
       So we only add one here for resizing the gop red rectangle, or for
       moving it with the red scalehandle. */
    if (sh->h_scale != 1)
        canvas_undo_add(x, 8, "apply", canvas_undo_set_canvas(x));

    /* We're abusing h_scale to differentiate between clicking gop red
       rectangle and clicking the corner of a subcanvas on the parent */
    if (sh->h_scale == 1) /* clicking corner of gop subcanvas on parent */
    {
        /* This is for clicking on the bottom right-hand corner of the
           gop canvas when it's displayed on the parent canvas. */
        sh->h_adjust_x = sh->h_offset_x -
            (((t_object *)x)->te_xpix + x->gl_pixwidth);
        sh->h_adjust_y = sh->h_offset_y -
            (((t_object *)x)->te_ypix + x->gl_pixheight);
    }
    else if (sh->h_scale == 2) /* resize gop hook for (red) gop rect */
    {
        /* Store an adjustment for difference between the initial
           pointer position-- which is within five pixels or so-- and the
           bottom right-hand corner. Otherwise we'd get a "jump" from the
           the current dimensions to the pointer offset. Such a jump would
           become noticeable with constrained dragging, as the constrained
           dimension would initially jump to the pointer position.

           We could alternatively use dx/dy, but then the pointer position
           would stray when the user attempts to drag past the minimum
           width/height of the rectangle. */

        sh->h_adjust_x = sh->h_offset_x - (x->gl_xmargin + x->gl_pixwidth);
        sh->h_adjust_y = sh->h_offset_y - (x->gl_ymargin + x->gl_pixheight);

        /* We could port this, but it might be better to wait until we
           just move the scalehandle stuff directly to the GUI... */
        //sys_vgui("lower %s\n", sh->h_pathname);
    }
    else /* move_gop hook */
    {
        /* Same as above... */
        //sys_vgui("lower %s\n", sh->h_pathname);
    }
}

void canvasgop__motionhook(t_scalehandle *sh, t_floatarg mouse_x,
    t_floatarg mouse_y)
{
    t_canvas *x = (t_canvas *)(sh->h_master);
    int dx = (int)mouse_x - sh->h_offset_x,
        dy = (int)mouse_y - sh->h_offset_y;

    if (sh->h_scale == 1) /* resize the gop on the parent */
    {
        int tmpx1 = 0, tmpy1 = 0, tmpx2 = 0, tmpy2 = 0;
        int tmp_x_final = 0, tmp_y_final = 0;
        /* The member h_glist currently points our current glist x. I think
           that is being used to draw the gop red rect move anchor atm. So
           rather than muck around with that code, we just set a pointer to
           whatever our toplevel is here: */
        t_glist *owner = glist_getcanvas(x);
        /* Just unvis the object, then vis it once we've done our
           mutation and checks */
        gobj_vis((t_gobj *)x, owner, 0);
        /* struct _glist has its own member e_xnew for storing our offset.
           At some point we need to refactor since our t_scalehandle has
           members for storing offsets already. */
        x->gl_pixwidth = (sh->h_constrain == CURSOR_EDITMODE_RESIZE_Y) ?
            x->gl_pixwidth :
            (int)mouse_x - ((t_object *)x)->te_xpix - sh->h_adjust_x;
        x->gl_pixheight = (sh->h_constrain == CURSOR_EDITMODE_RESIZE_X) ?
            x->gl_pixheight :
            (int)mouse_y - ((t_object *)x)->te_ypix - sh->h_adjust_y;
        /* If the box text is not hidden then it sets a lower boundary for
           box size... */
        graph_checkgop_rect((t_gobj *)x, owner, &tmpx1, &tmpy1, &tmpx2,
            &tmpy2);
        tmpx1 = ((t_object *)x)->te_xpix;
        tmpy1 = ((t_object *)x)->te_ypix;
        //fprintf(stderr,"%d %d %d %d\n", tmpx1, tmpy1, tmpx2, tmpy2);
        if (!x->gl_hidetext)
        {
            /* ico@vt.edu: we add pixels to match minimum space
               on the right side of the text to that of the left side */
            tmp_x_final = tmpx2 - tmpx1 + 2;
            tmp_y_final = tmpy2 - tmpy1;
        }
        else
        {
            tmp_x_final = tmpx2;
            tmp_y_final = tmpy2;
        }
        if (tmp_x_final > x->gl_pixwidth)
            x->gl_pixwidth = tmp_x_final;
        if (tmp_y_final > x->gl_pixheight)
            x->gl_pixheight = tmp_y_final;
        owner->gl_editor->e_xnew = mouse_x;
        owner->gl_editor->e_ynew = mouse_y;
        canvas_fixlinesfor(owner, (t_text *)x);
        gobj_vis((t_gobj *)x, owner, 1);
        canvas_dirty(owner, 1);

        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            properties_set_field_int(properties,
                "x_pix",x->gl_pixwidth + sh->h_dragx);
            properties_set_field_int(properties,
                "y_pix",x->gl_pixheight + sh->h_dragy);
        }
    }
    else if (sh->h_scale == 2) /* resize_gop red rect hook */
    {
        int width = (sh->h_constrain == CURSOR_EDITMODE_RESIZE_Y) ?
            x->gl_pixwidth :
            (int)mouse_x - x->gl_xmargin - sh->h_adjust_x;
        int height = (sh->h_constrain == CURSOR_EDITMODE_RESIZE_X) ?
            x->gl_pixheight :
            (int)mouse_y - x->gl_ymargin - sh->h_adjust_y;
        x->gl_pixwidth = width = maxi(SCALE_GOP_MINWIDTH, width);
        x->gl_pixheight = height = maxi(SCALE_GOP_MINHEIGHT, height);
        gui_vmess("gui_canvas_redrect_coords", "xiiii",
            x,
            x->gl_xmargin,
            x->gl_ymargin,
            x->gl_xmargin + width,
            x->gl_ymargin + height);
        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            properties_set_field_int(properties,
                "x_pix",x->gl_pixwidth + sh->h_dragx);
            properties_set_field_int(properties,
                "y_pix",x->gl_pixheight + sh->h_dragy);
        }
    }
    else /* move_gop hook */
    {
        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            properties_set_field_int(properties,
                "x_margin",x->gl_xmargin + dx);
                properties_set_field_int(properties,
                "y_margin",x->gl_ymargin + dy);
        }
        if (sh->h_constrain != CURSOR_EDITMODE_RESIZE_Y)
            x->gl_xmargin += dx;
        if (sh->h_constrain != CURSOR_EDITMODE_RESIZE_X)
            x->gl_ymargin += dy;

        int x1 = x->gl_xmargin, x2 = x1 + x->gl_pixwidth;
        int y1 = x->gl_ymargin, y2 = y1 + x->gl_pixheight;

        gui_vmess("gui_canvas_redrect_coords", "xiiii",
            x, x1, y1, x2, y2);
        sh->h_dragx = dx;
        sh->h_dragy = dy;
    }
}

extern t_class *array_define_class;     /* LATER datum class too */

    /* check if a pd can be treated as a glist - true if we're of any of
    the glist classes, which all have 'glist' as the first item in struct */
t_glist *pd_checkglist(t_pd *x)
{
    if (*x == canvas_class || *x == array_define_class)
        return ((t_canvas *)x);
    else return (0);
}

/* ------------------------------- setup routine ------------------------ */

    /* why are some of these "glist" and others "canvas"? */
extern void glist_text(t_glist *x, t_symbol *s, int argc, t_atom *argv);
extern void canvas_obj(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_obj_abstraction_from_menu(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_bng(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_toggle(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_vslider(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_hslider(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_vdial(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
    /* old version... */
extern void canvas_hdial(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_hdial(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
    /* new version: */
extern void canvas_hradio(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_vradio(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_vumeter(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_mycnv(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_numbox(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_msg(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_floatatom(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_symbolatom(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void canvas_dropdown(t_glist *gl, t_symbol *s, int argc, t_atom *argv);
extern void glist_scalar(t_glist *canvas, t_symbol *s, int argc, t_atom *argv);

void g_graph_setup(void);
void g_editor_setup(void);
void g_readwrite_setup(void);
extern void canvas_properties(t_gobj *z, t_glist *dummy);

void g_canvas_setup(void)
{
        /* we prevent the user from typing "canvas" in an object box
        by sending 0 for a creator function. */
    canvas_class = class_new(gensym("canvas"), 0,
        (t_method)canvas_free, sizeof(t_canvas), CLASS_NOINLET, 0);
            /* here is the real creator function, invoked in patch files
            by sending the "canvas" message to #N, which is bound
            to pd_camvasmaker. */
    class_addmethod(pd_canvasmaker, (t_method)canvas_new, gensym("canvas"),
        A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)canvas_restore,
        gensym("restore"), A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)canvas_coords,
        gensym("coords"), A_GIMME, 0);

/* -------------------------- objects ----------------------------- */
    class_addmethod(canvas_class, (t_method)canvas_obj,
        gensym("obj"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_obj_abstraction_from_menu,
        gensym("obj_abstraction"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_msg,
        gensym("msg"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_floatatom,
        gensym("floatatom"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_symbolatom,
        gensym("symbolatom"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_dropdown,
        gensym("dropdown"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_text,
        gensym("text"), A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_glist, gensym("graph"),
        A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_scalar,
        gensym("scalar"), A_GIMME, A_NULL);

/* -------------- IEMGUI: button, toggle, slider, etc.  ------------ */
    class_addmethod(canvas_class, (t_method)canvas_bng, gensym("bng"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_toggle, gensym("toggle"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_vslider, gensym("vslider"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_hslider, gensym("hslider"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_hdial, gensym("hdial"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_vdial, gensym("vdial"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_hradio, gensym("hradio"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_vradio, gensym("vradio"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_vumeter, gensym("vumeter"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_mycnv, gensym("mycnv"),
                    A_GIMME, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_numbox, gensym("numbox"),
                    A_GIMME, A_NULL);

/* ------------------------ gui stuff --------------------------- */
    class_addmethod(canvas_class, (t_method)canvas_pop, gensym("pop"),
        A_DEFFLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_loadbang,
        gensym("loadbang"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_setbounds,
        gensym("setbounds"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_relocate,
        gensym("relocate"), A_SYMBOL, A_SYMBOL, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_vis,
        gensym("vis"), A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)glist_menu_open,
        gensym("menu-open"), A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_map,
        gensym("map"), A_FLOAT, A_NULL);
    class_addmethod(canvas_class, (t_method)canvas_dirty,
        gensym("dirty"), A_FLOAT, A_NULL);
    class_setpropertiesfn(canvas_class, (t_propertiesfn)canvas_properties);

    class_addmethod(canvas_class, (t_method)canvas_scalar_event,
        gensym("scalar_event"), A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)canvas_show_scrollbars,
        gensym("scroll"), A_FLOAT, 0);
    class_addmethod(canvas_class, (t_method)canvas_show_menu,
        gensym("menu"), A_FLOAT, 0);

/* ---------------------- list handling ------------------------ */
    class_addmethod(canvas_class, (t_method)glist_clear, gensym("clear"),
        A_NULL);

/* ----- subcanvases, which you get by typing "pd" in a box ---- */
    class_addcreator((t_newmethod)subcanvas_new, gensym("pd"), A_DEFSYMBOL, 0);
    class_addcreator((t_newmethod)subcanvas_new, gensym("page"),  A_DEFSYMBOL, 0);

    class_addmethod(canvas_class, (t_method)canvas_click,
        gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(canvas_class, (t_method)canvas_dsp, gensym("dsp"),
        A_CANT, 0);
    class_addmethod(canvas_class, (t_method)canvas_rename_method,
        gensym("rename"), A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)canvas_forwardmess,
        gensym("forwardmess"), A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)canvas_checkconfig,
        gensym("checkconfig"), A_NULL, 0);

/*---------------------------- tables -- GG ------------------- */

    class_addcreator((t_newmethod)table_new, gensym("table"),
        A_DEFSYM, A_DEFFLOAT, 0);

/*---------------------------- ab ------------------- */

    class_addcreator((t_newmethod)ab_new, gensym("ab"), A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)canvas_abpush,
        gensym("abpush"), A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)canvas_abframe,
        gensym("abframe"), A_FLOAT, 0);

    abdefs_class = class_new(gensym("abdefs"), (t_newmethod)abdefs_new,
        0, sizeof(t_abdefs), CLASS_DEFAULT, A_NULL);
    class_addbang(abdefs_class, (t_method)abdefs_bang);
    class_addmethod(abdefs_class, (t_method)abdefs_get, gensym("get"), 0);
    class_addmethod(abdefs_class, (t_method)abdefs_instances, gensym("instances"), A_SYMBOL, 0);
    class_addmethod(abdefs_class, (t_method)abdefs_menuopen, gensym("menu-open"), 0);
    class_addmethod(abdefs_class, (t_method)abdefs_dialog, gensym("dialog"), A_GIMME, 0);

    class_addmethod(canvas_class, (t_method)canvas_showdirty,
        gensym("showdirty"), 0);
    class_addmethod(canvas_class, (t_method)canvas_getabstractions,
        gensym("getabstractions"), 0);
    class_addmethod(canvas_class, (t_method)canvas_delabstractions,
        gensym("delabstractions"), A_GIMME, 0);
/*---------------------------- declare ------------------- */
    declare_class = class_new(gensym("declare"), (t_newmethod)declare_new,
        (t_method)declare_free, sizeof(t_declare), CLASS_NOINLET, A_GIMME, 0);
    class_addmethod(canvas_class, (t_method)canvas_declare,
        gensym("declare"), A_GIMME, 0);

/*--------------- future message to set formatting  -------------- */
    class_addmethod(canvas_class, (t_method)canvas_f,
        gensym("f"), A_GIMME, 0);
/* -------------- setups from other files for canvas_class ---------------- */
    g_graph_setup();
    g_editor_setup();
    g_readwrite_setup();
}

    /* functions to add basic gui (e.g., clicking but not editing) to things
    based on canvases that aren't editable, like "array define" object */
void canvas_editor_for_class(t_class *c);
void g_graph_setup_class(t_class *c);
void canvas_readwrite_for_class(t_class *c);

void canvas_add_for_class(t_class *c)
{
    class_addmethod(c, (t_method)canvas_restore,
        gensym("restore"), A_GIMME, 0);
    class_addmethod(c, (t_method)canvas_click,
        gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(c, (t_method)canvas_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(c, (t_method)canvas_map,
        gensym("map"), A_FLOAT, A_NULL);
    class_addmethod(c, (t_method)canvas_setbounds,
        gensym("setbounds"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_NULL);
    canvas_editor_for_class(c);
    canvas_readwrite_for_class(c);
    /* g_graph_setup_class(c); */
}
