/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* interface objects */

#include "m_pd.h"
#include "m_imp.h"
#include "g_canvas.h"
#include "s_stuff.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* we need the following for [pdinfo] ... */

#define MAXNDEV 20
#define DEVDESCSIZE 80

/* -------------------------- print ------------------------------ */
t_class *print_class;

typedef struct _print
{
    t_object x_obj;
    t_symbol *x_sym;
} t_print;

static void *print_new(t_symbol *sel, int argc, t_atom *argv)
{
    t_print *x = (t_print *)pd_new(print_class);
    if (argc == 0)
        x->x_sym = gensym("print");
    else if (argc == 1 && argv->a_type == A_SYMBOL)
    {
        t_symbol *s = atom_getsymbolarg(0, argc, argv);
        if (!strcmp(s->s_name, "-n"))
            x->x_sym = &s_;
        else x->x_sym = s;
    }
    else
    {
        int bufsize;
        char *buf;
        t_binbuf *bb = binbuf_new();
        binbuf_add(bb, argc, argv);
        binbuf_gettext(bb, &buf, &bufsize);
        buf = resizebytes(buf, bufsize, bufsize+1);
        buf[bufsize] = 0;
        x->x_sym = gensym(buf);
        freebytes(buf, bufsize+1);
        binbuf_free(bb);
    }
    return (x);
}

static void print_bang(t_print *x)
{
    if (sys_nogui)
        post("%s%sbang", x->x_sym->s_name, (*x->x_sym->s_name ? ": " : ""));
    else
    {
        gui_start_vmess("gui_print", "xs", x, x->x_sym->s_name);
        gui_start_array();
        gui_s(s_bang.s_name);
        gui_end_array();
        gui_end_vmess();
    }
}

static void print_pointer(t_print *x, t_gpointer *gp)
{
    if (sys_nogui)
        post("%s%s(gpointer)", x->x_sym->s_name,
            (*x->x_sym->s_name ? ": " : ""));
    else
    {
        gui_start_vmess("gui_print", "xs", x, x->x_sym->s_name);
        gui_start_array();
        gui_s("(gpointer)");
        gui_end_array();
        gui_end_vmess();
    }
}

static void print_float(t_print *x, t_floatarg f)
{
    if (sys_nogui)
        post("%s%s%g", x->x_sym->s_name, (*x->x_sym->s_name ? ": " : ""), f);
    else
    {
        gui_start_vmess("gui_print", "xs", x, x->x_sym->s_name);
        gui_start_array();
        gui_f(f);
        gui_end_array();
        gui_end_vmess();
    }
}

static void print_symbol(t_print *x, t_symbol *s)
{
    if (sys_nogui)
        post("%s%s%s", x->x_sym->s_name, (*x->x_sym->s_name ? ": " : ""),
            s->s_name);
    else
    {
        gui_start_vmess("gui_print", "xs", x, x->x_sym->s_name);
        gui_start_array();
        gui_s(s_symbol.s_name);
        gui_s(s->s_name);
        gui_end_array();
        gui_end_vmess();
    }
}

static void print_anything(t_print *x, t_symbol *s, int argc, t_atom *argv)
{
    char buf[MAXPDSTRING];
    t_atom at;
    if (sys_nogui)
    {
        startpost("%s%s", x->x_sym->s_name, (*x->x_sym->s_name ? ":" : ""));
        if (s && (s != &s_list || (argc && argv->a_type != A_FLOAT)))
        {
            SETSYMBOL(&at, s);
            postatom(1, &at);
        }
        postatom(argc, argv);
        endpost();
    }
    else
    {
        gui_start_vmess("gui_print", "xs", x, x->x_sym->s_name);
        gui_start_array();
        if (s && (s != &s_list || (argc && argv->a_type != A_FLOAT)))
            gui_s(s->s_name);
        for(; argc; argv++, argc--)
        {
            atom_string(argv, buf, MAXPDSTRING);
            gui_s(buf);
        }
        gui_end_array();
        gui_end_vmess();
    }
}

static void print_setup(void)
{
    print_class = class_new(gensym("print"), (t_newmethod)print_new, 0,
        sizeof(t_print), 0, A_GIMME, 0);
    class_addbang(print_class, print_bang);
    class_addfloat(print_class, print_float);
    class_addpointer(print_class, print_pointer);
    class_addsymbol(print_class, print_symbol);
    class_addanything(print_class, print_anything);
}

/* --------------pdinfo, canvasinfo, classinfo, objectinfo --------------- */
static t_class *canvasinfo_class;

typedef struct _canvasinfo {
    t_object x_obj;
    t_canvas *x_canvas;
    t_float   x_depth;
} t_canvasinfo;

static t_class *pdinfo_class;

typedef struct _pdinfo {
    t_object x_obj;
} t_pdinfo;

static t_class *classinfo_class;

typedef struct _classinfo {
    t_object x_obj;
    t_outlet *x_out2;
    t_symbol *x_name;
} t_classinfo;

static t_class *objectinfo_class;

typedef struct _objectinfo {
    t_object x_obj;
    t_outlet *x_out2;
    t_gpointer x_gp;
    t_canvas *x_canvas;
} t_objectinfo;

/* used by all the *info objects */

static int info_to_console = 0;

void info_out(t_text *te, t_symbol *s, int argc, t_atom *argv)
{
    if (info_to_console)
    {
        startpost("%s:", s->s_name);
        if (argc > 0)
            postatom(argc, argv);
        endpost();
    }
    else
    {
        outlet_list(te->ob_outlet,
            &s_list, argc, argv);
    }
}

void info_print(t_text *te)
{
    t_class *c = classtable_findbyname(te->ob_pd->c_name);
    if(c == NULL)
    {
        pd_error(te, "s: can't find entry in classtable");
        return;
    }
    info_to_console = 1;
    t_int i;
    t_methodentry *m;
    for(i = c->c_nmethod, m = c->c_methods; i; i--, m++)
        if(m->me_name != gensym("print"))
            (m->me_fun)(te, m->me_name, 0, 0);
    /* not sure why this doesn't work... */
    /* pd_forwardmess(te->te_pd, 0, 0); */
    info_to_console = 0;
}

/* -------------------------- canvasinfo ------------------------------ */
/* climb up to a parent canvas */
t_canvas *canvas_climb(t_canvas *c, int level)
{
  if(level <= 0)
      return c;
  else
  {
      t_canvas *ret = c;
      while(level && ret->gl_owner)
      {
        ret = ret->gl_owner;
        level--;
      }
      return ret;
  }
}

void canvas_getargs_after_creation(t_canvas *c, int *argcp, t_atom **argvp);

void canvasinfo_args(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    int n = 0;
    t_atom *a = 0;
    t_binbuf *b;
    if(!c) return;
    if (s == gensym("args")) c = canvas_getrootfor(c);
    b = c->gl_obj.te_binbuf;
    if(!b)
    {
        info_out((t_text *)x, s, 0, 0);
    }
    else
    {
        if (s == gensym("args"))
        {
            canvas_getargs_after_creation(c, &n, &a);
            info_out((t_text *)x, s, n, a);
        }
        else
        {
            /* For "boxtext" we have to convert semi, comma, dollar, and
               dollsym atoms to symbol atoms. Otherwise we could end up
               outputting a message containing stray semis/commas/etc. which
               might cause trouble.

               We sent the atoms through binbuf_addbinbuf which does the
               conversion to symbols for us. That way the user will get
               expected output-- e.g., special characters will be properly
               escaped when printing.
            */
            t_binbuf *escaped = binbuf_new();
            binbuf_addbinbuf(escaped, b);
            n = binbuf_getnatom(escaped);
            a = binbuf_getvec(escaped);
            info_out((t_text *)x, s, n, a);
            binbuf_free(escaped);
        }
    }
}

void canvasinfo_coords(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    t_int gop = c->gl_isgraph + c->gl_hidetext;
    t_atom at[9];
    SETFLOAT(at, (c->gl_isgraph) ? c->gl_x1 : 0);
    SETFLOAT(at+1, (c->gl_isgraph) ? c->gl_y1 : 0);
    SETFLOAT(at+2, (c->gl_isgraph) ? c->gl_x2 : 0);
    SETFLOAT(at+3, (c->gl_isgraph) ? c->gl_y2 : 0);
    SETFLOAT(at+4, (c->gl_isgraph) ? c->gl_pixwidth : 0);
    SETFLOAT(at+5, (c->gl_isgraph) ? c->gl_pixheight : 0);
    SETFLOAT(at+6, (c->gl_isgraph) ? gop : 0);
    SETFLOAT(at+7, (c->gl_isgraph) ? c->gl_xmargin : 0);
    SETFLOAT(at+8, (c->gl_isgraph) ? c->gl_ymargin : 0);
    info_out((t_text *)x, s, 9, at);
}

void canvasinfo_dir(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    c = canvas_getrootfor(c);
    t_atom at[1];
    SETSYMBOL(at, canvas_getdir(c));
    info_out((t_text *)x, s, 1, at);
}

void canvasinfo_dirty(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    t_atom at[1];
    SETFLOAT(at, c->gl_dirty);
    info_out((t_text *)x, s, 1, at);
}

void canvasinfo_dollarzero(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    c = canvas_getrootfor(c);
    t_symbol *d = gensym("$0");
    t_symbol *ret = canvas_realizedollar(c, d);
    float f = (float)strtod(ret->s_name,NULL);
    t_atom at[1];
    SETFLOAT(at, f);
    info_out((t_text *)x, s, 1, at);
}

void canvasinfo_editmode(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    t_atom at[1];
    SETFLOAT(at, c->gl_edit);
    info_out((t_text *)x, s, 1, at);
}

void canvasinfo_filename(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    c = canvas_getrootfor(c);
    t_atom at[1];
    SETSYMBOL(at, c->gl_name);
    info_out((t_text *)x, s, 1, at);
}

int binbuf_match(t_binbuf *inbuf, t_binbuf *searchbuf, int wholeword);

void canvasinfo_find(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    int i, match = 0;
    t_atom at[1], *ap;
    t_gpointer *gp = (t_gpointer *)t_getbytes(500 * sizeof(*gp));
    t_gobj *y;
    t_binbuf *searchbuf = binbuf_new();
    t_binbuf *outbuf = binbuf_new();
    binbuf_add(searchbuf, argc, argv);
    for (y = c->gl_list; y && match < 500; y = y->g_next)
    {
        t_binbuf *objbuf;
        /* if it's not t_object (e.g., a scalar), or if it is but
           does not have any binbuf content, send a bang... */
        if (pd_checkobject(&y->g_pd) &&
            (objbuf = ((t_text *)y)->te_binbuf) &&
            binbuf_match(objbuf, searchbuf, 1))
        {
            match++;
            gpointer_init(gp+match-1);
            gpointer_setglist(gp+match-1, c, y);
            SETPOINTER(at, gp+match-1);
            binbuf_add(outbuf, 1, at); 
        }
    }
    if (match >= 500)
        post("canvasinfo: warning: find is currently limited to 500 results. "
             "Truncating the output to 500 elements...");
    info_out((t_text *)x, s, binbuf_getnatom(outbuf), binbuf_getvec(outbuf));
    for (i = 0, ap = binbuf_getvec(outbuf); i < binbuf_getnatom(outbuf); i++)
    {
        t_gpointer *gp = (ap+i)->a_w.w_gpointer;
        gpointer_unset(gp);
    }
    binbuf_free(outbuf);
    binbuf_free(searchbuf);
    freebytes(gp, 500 * sizeof(*gp));
}

void canvasinfo_gobjs(t_canvasinfo *x, t_float xpos, t_float ypos,
    int all)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    int x1, y1, x2, y2, i, atom_count = 0;
    /* hack to avoid memory allocation. Maybe there's a way to use the
       XLIST_ATOMS_ALLOCA macro? */
    t_atom at[500];
    t_gpointer *gp, *gvec;
    gp = gvec = (t_gpointer *)t_getbytes(500 * sizeof (*gvec));
    t_gobj *y;
    for (y = c->gl_list, i = 0; y && atom_count < 500; y = y->g_next, i++)
    {
        if (all || canvas_hitbox(c, y, xpos, ypos, &x1, &y1, &x2, &y2))
        {
            gpointer_init(gp);
            gpointer_setglist(gp, c, y);
            SETPOINTER(at+atom_count, gp);
            atom_count++;
            gp++;
        }
    }
    if (atom_count >= 500)
        post("canvasinfo: warning: hitbox is currently limited to 500 objects. "
             "Truncating the output to 500 elements...");
    info_out((t_text *)x, gensym("hitbox"), atom_count, at);
    for (i = 0, gp = gvec; i < atom_count; i++, gp++)
        gpointer_unset(gp);
    freebytes(gvec, 500 * sizeof(*gvec));
}

void canvasinfo_bang(t_canvasinfo *x)
{
    canvasinfo_gobjs(x, 0, 0, 1);
}

void canvasinfo_hitbox(t_canvasinfo *x, t_floatarg xpos, t_floatarg ypos)
{
    canvasinfo_gobjs(x, xpos, ypos, 0);
}

void canvasinfo_name(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING, "x%lx", (long unsigned int)c);
    t_atom at[1];
    SETSYMBOL(at, gensym(buf));
    info_out((t_text *)x, s, 1, at);
}

void canvasinfo_pointer(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    t_atom at[1];
    t_gpointer gp;
    gpointer_init(&gp);
    gpointer_setglist(&gp, c, 0);
    SETPOINTER(at, &gp);
    info_out((t_text *)x, s, 1, at);
    gpointer_unset(&gp);
}

void canvasinfo_posonparent(t_canvasinfo *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    t_atom at[2];
    SETFLOAT(at, c->gl_obj.te_xpix);
    SETFLOAT(at+1, c->gl_obj.te_ypix);
    info_out((t_text *)x, s, 2, at);
}

void canvasinfo_screenpos(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    t_atom at[4];
    SETFLOAT(at, c->gl_screenx1);
    SETFLOAT(at+1, c->gl_screeny1);
    SETFLOAT(at+2, c->gl_screenx2);
    SETFLOAT(at+3, c->gl_screeny2);
    info_out((t_text *)x, s, 4, at);
}

void canvasinfo_toplevel(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    t_float f = c->gl_owner ? 0 : 1;
    t_atom at[1];
    SETFLOAT(at, f);
    info_out((t_text *)x, s, 1, at);
}

void canvasinfo_vis(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    t_atom at[1];
    SETFLOAT(at, glist_isvisible(c));
    info_out((t_text *)x, s, 1, at);
}

void canvasinfo_print(t_canvasinfo *x)
{
    info_print((t_text *)x);
}

void *canvasinfo_new(t_floatarg f)
{
    t_canvasinfo *x = (t_canvasinfo *)pd_new(canvasinfo_class);
    t_glist *glist = (t_glist *)canvas_getcurrent();
    t_canvas *c = (t_canvas*)glist_getcanvas(glist);
    x->x_canvas = c;
    floatinlet_new(&x->x_obj, &x->x_depth);
    outlet_new(&x->x_obj, &s_list);
    x->x_depth = f;
    return (void *)x;
}

void canvasinfo_setup(void)
{
    canvasinfo_class = class_new(gensym("canvasinfo"),
        (t_newmethod)canvasinfo_new, 0,
        sizeof(t_canvasinfo),
        CLASS_DEFAULT, A_DEFFLOAT, 0);

    class_addbang(canvasinfo_class, canvasinfo_bang);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_args,
        gensym("args"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_args,
        gensym("boxtext"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_coords,
        gensym("coords"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_dir,
        gensym("dir"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_dirty,
        gensym("dirty"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_dollarzero,
        gensym("dollarzero"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_editmode,
        gensym("editmode"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_filename,
        gensym("filename"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_find,
        gensym("find"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_hitbox,
        gensym("hitbox"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_name,
        gensym("name"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_pointer,
        gensym("pointer"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_posonparent,
        gensym("posonparent"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_screenpos,
        gensym("screenpos"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_toplevel,
        gensym("toplevel"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_vis,
        gensym("vis"), A_GIMME, 0);
    class_addmethod(canvasinfo_class, (t_method)canvasinfo_print,
        gensym("print"), 0);

    post("canvasinfo: v0.1");
    post("stable canvasinfo methods: args dir dirty editmode vis");

}

/* -------------------------- pdinfo ------------------------------ */
static t_class *pdinfo_class;

t_symbol *getapiname(int id)
{
    t_symbol *s = 0;
    switch (id)
    {
        case API_NONE: s = gensym("none"); break;
        case API_ALSA: s = gensym("ALSA"); break;
        case API_OSS: s = gensym("OSS"); break;
        case API_MMIO: s = gensym("MMIO"); break;
        case API_PORTAUDIO: s = gensym("PortAudio"); break;
        case API_JACK: s = gensym("JACK"); break;
        case API_SGI: s = gensym("SGI"); break;
        //case API_AUDIOUNIT: s = gensym("AudioUnit"); break;
        //case API_ESD: s = gensym("ESD"); break;
        //case API_DUMMY: s = gensym("dummy"); break;
    }
    return s;
}

void pdinfo_dir(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    t_symbol *dir = pd_getdirname();
    if (!dir)
    {
        pd_error(x, "pdinfo: can't find pd's directory");
        return;
    }
    SETSYMBOL(at, dir);
    info_out((t_text *)x, s, 1, at);
}

/* Instead of reporting the actual value for dsp when it's temporarily
   suspended, we report what it will be when dsp is resumed. This way the
   user can get a meaningful value at load time. */
void pdinfo_dsp(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)(pd_this->pd_dspstate_user));
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_audio_api(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    t_symbol *api = getapiname(sys_audioapi);
    SETSYMBOL(at, api);
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_canvaslist(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c;
    int j, i = 0;
    t_binbuf *outbuf = binbuf_new();
    t_atom at[1];
    for (c = pd_this->pd_canvaslist; c; c = c->gl_next)
        i++;
    t_gpointer *gp = (t_gpointer *)t_getbytes(i * sizeof(*gp));
    for (c = pd_this->pd_canvaslist, i = 0; c; c = c->gl_next, i++)
    {
        gpointer_init(gp+i);
        gpointer_setglist(gp+i, c, 0);
        SETPOINTER(at, gp+i);
        binbuf_add(outbuf, 1, at); 
    }
    info_out((t_text *)x, s, binbuf_getnatom(outbuf), binbuf_getvec(outbuf));
    binbuf_free(outbuf);
    for (j = 0; j < i; j++)
        gpointer_unset(gp+j);
    freebytes(gp, i * sizeof(*gp));
}

/* maybe this should be the bang method? */
void pdinfo_classlist(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    int size = classtable_size();
    if (info_to_console)
    {
        t_atom at[2];
        SETFLOAT(at, size);
        SETSYMBOL(at+1, gensym("classes loaded (\"classtable\" outputs "
                               "the full list)"));
        info_out((t_text *)x, s, 2, at);
    }
    else
    {
        t_atom at[size];
        classtable_tovec(size, at);
        info_out((t_text *)x, s, size, at);
    }
}

void pdinfo_audioin(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
//        char i
}

void pdinfo_audio_api_list_all(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[7];
    int i;
    for(i = 0; i < 7; i++)
        SETSYMBOL(at+i, getapiname(i));
    info_out((t_text *)x, s, i, at);
}

void pdinfo_audio_apilist(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[8];
    int n = 0;
#ifdef USEAPI_OSS
    SETSYMBOL(at+n, getapiname(API_OSS)); n++;
#endif
#ifdef USEAPI_MMIO
    SETSYMBOL(at+n, getapiname(API_MMIO)); n++;
#endif
#ifdef USEAPI_ALSA
    SETSYMBOL(at+n, getapiname(API_ALSA)); n++;
#endif
#ifdef USEAPI_PORTAUDIO
#ifdef MSW
    SETSYMBOL(at+n, getapiname(API_PORTAUDIO));
#else
#ifdef OSX
    SETSYMBOL(at+n, getapiname(API_PORTAUDIO));
#else
    SETSYMBOL(at+n, getapiname(API_PORTAUDIO));
#endif
#endif
    n++;
#endif
#ifdef USEAPI_JACK
    SETSYMBOL(at+n, getapiname(API_JACK)); n++;
#endif
#ifdef USEAPI_AUDIOUNIT
    SETSYMBOL(at+n, getapiname(API_AUDIOUNIT)); n++;
#endif
#ifdef USEAPI_ESD
    SETSYMBOL(at+n, getapiname(API_ESD)); n++;
#endif
#ifdef USEAPI_DUMMY
    SETSYMBOL(at+n, getapiname(API_DUMMY)); n++;
#endif
    info_out((t_text *)x, s, n, at);
}

void pdinfo_audio_listdevs(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    char indevlist[MAXNDEV*DEVDESCSIZE], outdevlist[MAXNDEV*DEVDESCSIZE];
    int nindevs = 0, noutdevs = 0, i, canmulti = 0, cancallback = 0;
    sys_get_audio_devs(indevlist, &nindevs,
            outdevlist, &noutdevs,
            &canmulti, &cancallback,
            MAXNDEV, DEVDESCSIZE);
    t_atom at[MAXNDEV];
    if (s == gensym("audio-multidev-support"))
    {
        SETFLOAT(at, canmulti);
        info_out((t_text *)x, s, 1, at); 
    }
    else if (s == gensym("audio-indevlist"))
    {
        for (i = 0; i < nindevs; i++)
           SETSYMBOL(at+i, gensym(indevlist + i * DEVDESCSIZE));
        info_out((t_text *)x, s, i, at);
    }
    else if (s == gensym("audio-outdevlist"))
    {
        for (i = 0; i < noutdevs; i++)
           SETSYMBOL(at+i, gensym(outdevlist + i * DEVDESCSIZE));
        info_out((t_text *)x, s, i, at);
    }
}

void pdinfo_audio_dev(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    int devno;
    if (argc) devno = (int)atom_getfloatarg(0, argc, argv);
    else devno = 0;
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int rate, advance, callback, blocksize;
    sys_get_audio_params(&naudioindev, audioindev, chindev,
        &naudiooutdev, audiooutdev, choutdev, &rate, &advance, &callback, &blocksize);
    int *dev, *chan, ndev;
    if (s == gensym("audio-indev"))
        dev = audioindev, chan = chindev, ndev = naudioindev;
    else
        dev = audiooutdev, chan = choutdev, ndev = naudiooutdev;
    if (devno >= 0 && devno < ndev)
    {
        t_atom at[2];
        SETFLOAT(at, (t_float)dev[devno]);
        SETFLOAT(at+1, (t_float)chan[devno]);
        info_out((t_text *)x, s, 2, at);
    }
    else
        info_out((t_text *)x, s, 0, 0);
}

void pdinfo_midi_api(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    t_symbol *api, *def = gensym("DEFAULT");
#ifdef USEAPI_OSS
    def = gensym("OSS");
#endif
    api = sys_midiapi ? gensym("ALSA") : def;
    SETSYMBOL(at, api);
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_midi_apilist(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[8];
    int n = 0;
    SETSYMBOL(at+n, gensym("DEFAULT"));
#ifdef USEAPI_OSS
    SETSYMBOL(at+n, gensym("OSS"));
#endif
    n++;
#ifdef USEAPI_ALSA
    SETSYMBOL(at+n, getapiname(API_ALSA)); n++;
#endif
    info_out((t_text *)x, s, n, at);
}

void pdinfo_midi_listdevs(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    char indevlist[MAXMIDIINDEV*DEVDESCSIZE],
        outdevlist[MAXMIDIOUTDEV*DEVDESCSIZE];
    int nindevs = 0, noutdevs = 0, i;
    sys_get_midi_devs(indevlist, &nindevs,
            outdevlist, &noutdevs,
            MAXNDEV, DEVDESCSIZE);
    t_atom at[MAXNDEV];
    if (s == gensym("midi-indevlist"))
    {
        for (i = 0; i < nindevs; i++)
           SETSYMBOL(at+i, gensym(indevlist + i * DEVDESCSIZE));
        info_out((t_text *)x, s, i, at);
    }
    else if (s == gensym("midi-outdevlist"))
    {
        for (i = 0; i < noutdevs; i++)
           SETSYMBOL(at+i, gensym(outdevlist + i * DEVDESCSIZE));
        info_out((t_text *)x, s, i, at);
    }
}

void pdinfo_midi_dev(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    int devno, nmidiindev, midiindev[MAXMIDIINDEV],
        nmidioutdev, midioutdev[MAXMIDIOUTDEV];
    int *dev, ndev;
    if (argc) devno = (int)atom_getfloatarg(0, argc, argv);
    else devno = 0;
    sys_get_midi_params(&nmidiindev, midiindev, &nmidioutdev, midioutdev);
    if (s == gensym("midi-indev"))
        dev = midiindev, ndev = nmidiindev;
    else
        dev = midioutdev, ndev = nmidioutdev;
    if (devno >= 0 && devno < ndev)
    {
        t_atom at[1];
        SETFLOAT(at, (t_float)dev[devno]);
        info_out((t_text *)x, s, 1, at);
    }
    else
        info_out((t_text *)x, s, 0, 0);
}

void pdinfo_audio_outdev(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    int devno;
    if (argc) devno = (int)atom_getfloatarg(0, argc, argv);
    else devno = 0;
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int rate, advance, callback, blocksize;
    sys_get_audio_params(&naudioindev, audioindev, chindev,
        &naudiooutdev, audiooutdev, choutdev, &rate, &advance, &callback, &blocksize);
    if (devno >= 0 && devno < naudioindev)
    {
        t_atom at[2];
        SETFLOAT(at, (t_float)audioindev[devno]);
        SETFLOAT(at+1, (t_float)chindev[devno]);
        info_out((t_text *)x, s, 2, at);
    }
    else
        info_out((t_text *)x, s, 0, 0);
}

void pdinfo_audio_inchannels(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)sys_get_inchannels());
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_audio_outchannels(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)sys_get_outchannels());
    info_out((t_text *)x, s, 1, at);
}


void pdinfo_audio_samplerate(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)sys_getsr());
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_audio_blocksize(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)sys_audio_get_blocksize());
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_gui(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)(!sys_nogui));
    info_out((t_text *)x, s, 1, at);
}

/* directory where extra and doc are found. Might also want to add
   another method to return a list of all paths searched for libs-- i.e.,
   "extrapath". */
void pdinfo_libdir(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    SETSYMBOL(at, sys_libdir);
    info_out((t_text *)x, s, 1, at);
}

t_symbol* pd_getplatform(void)
{
#ifdef __APPLE__
    return gensym("darwin");
#endif
#ifdef __FreeBSD__
    return gensym("freebsd");
#endif
#ifdef _WIN32
    return gensym("win32");
#endif
#ifdef __linux__
    return gensym("linux");
#endif
    /* don't know the platform... */
    return gensym("unknown");
}

void pdinfo_platform(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    SETSYMBOL(at, pd_getplatform());
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_arch(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom at[1];
    t_symbol *a = gensym("unknown");
#ifdef __i386__
    a = gensym("ia32");
#endif
#ifdef __x86_64__
    a = gensym("x64");
#endif
#ifdef __arm__
    a = gensym("arm");
#endif
    SETSYMBOL(at, a);
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_version(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    int major=0, minor=0, bugfix=0;
    sys_getversion(&major, &minor, &bugfix);
    t_atom at[3];
    SETFLOAT(at, (t_float)major);
    SETFLOAT(at+1, (t_float)minor);
    SETFLOAT(at+2, (t_float)bugfix);
    info_out((t_text *)x, s, 3, at);
}

void pdinfo_l2ork_version(t_pdinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    char buf[MAXPDSTRING];
    t_atom at[1];
    sprintf(buf, PD_L2ORK_VERSION " " PD_BUILD_VERSION);
    SETSYMBOL(at, gensym(buf));
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_print(t_pdinfo *x)
{
    info_print((t_text *)x);
}

void *pdinfo_new(t_symbol *s, t_int argc, t_atom *argv)
{
    t_pdinfo *x = (t_pdinfo *)pd_new(pdinfo_class);
    outlet_new(&x->x_obj, &s_list);    
    return (void *)x;
}

void pdinfo_setup(void)
{
    pdinfo_class = class_new(gensym("pdinfo"),
        (t_newmethod)pdinfo_new, 0,
        sizeof(t_pdinfo),
        CLASS_DEFAULT, 0);

    class_addmethod(pdinfo_class, (t_method)pdinfo_arch,
        gensym("arch"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_api,
        gensym("audio-api"), A_DEFFLOAT, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_apilist,
        gensym("audio-apilist"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_api_list_all,
        gensym("audio-apilist-all"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_inchannels,
        gensym("audio-inchannels"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_dev,
        gensym("audio-indev"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_listdevs,
        gensym("audio-indevlist"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_listdevs,
        gensym("audio-multidev-support"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_outchannels,
        gensym("audio-outchannels"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_dev,
        gensym("audio-outdev"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_listdevs,
        gensym("audio-outdevlist"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_blocksize,
        gensym("blocksize"), A_GIMME, 0);
    /* this needs a better name-- the user doesn't have to know the
       name used in the implementation */
    class_addmethod(pdinfo_class, (t_method)pdinfo_canvaslist,
        gensym("canvaslist"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_classlist,
        gensym("classlist"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_dir,
        gensym("dir"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_dsp,
        gensym("dsp-status"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_gui,
        gensym("gui"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_libdir,
        gensym("libdir"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_midi_api,
        gensym("midi-api"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_midi_apilist,
        gensym("midi-apilist"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_midi_dev,
        gensym("midi-indev"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_midi_listdevs,
        gensym("midi-indevlist"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_midi_dev,
        gensym("midi-outdev"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_midi_listdevs,
        gensym("midi-outdevlist"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_platform,
        gensym("platform"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_samplerate,
        gensym("samplerate"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_version,
        gensym("version"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_l2ork_version,
        gensym("l2ork_version"), A_GIMME, 0);

    class_addmethod(pdinfo_class, (t_method)pdinfo_print,
        gensym("print"), 0);

    post("pdinfo: v.0.1");
    post("stable pdinfo methods: dir dsp version");
}

/* -------------------------- classinfo ------------------------------ */
t_symbol *attosym(t_atomtype at)
{
    t_symbol *s;
    switch (at)
    {
        case A_FLOAT: s = gensym("A_FLOAT"); break;
        case A_SYMBOL: s = gensym("A_SYMBOL"); break;
        case A_POINTER: s = gensym("A_POINTER"); break;
        case A_DEFFLOAT: s = gensym("A_DEFFLOAT"); break;
        case A_DEFSYM: s = gensym("A_DEFSYM"); break;
        case A_GIMME: s = gensym("A_GIMME"); break;
        case A_CANT: s = gensym("A_CANT"); break;
        default: s = 0;
    }
    return s;
}

void classinfo_args(t_classinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_class *c;
    if(!(c = classtable_findbyname(x->x_name)))
    {
        outlet_bang(x->x_out2);
        return;
    }
    c = classtable_findbyname(gensym("objectmaker"));
    if (!c)
    {
        pd_error(x, "classinfo: no objectmaker.");
        return;
    }
    t_atom ap[MAXPDARG];
    t_int i;
    t_methodentry *m;
    for(i = c->c_nmethod, m = c->c_methods; i; i--, m++)
    {
        if(m->me_name == x->x_name)
            break;
    }
    /* We have to check if there was a match-- while the class
       table holds all classes, the objectmaker only creates a
       new method entry if the class has a t_newmethod defined
       in its setup routine. This misses a few objects such as 
       gatom which can't be typed into a box. Consequently I'm
       sending a bang to the reject outlet for those objects--
       unfortunately that means a "reject" bang could mean the
       object either doesn't exist, or it doesn't have its own
       t_newmethod. However, one can use the other [classinfo]
       methods to unambiguously check the class for existence.
    */
    if (i)
    {
        t_atomtype arg, *args = m->me_arg;
        for(i = 0; arg = *args; args++, i++)
        {
            t_symbol *sym = attosym(arg);
            if (!sym)
            {
                pd_error(x, "classinfo: %s: bad argtype", x->x_name->s_name);
                return;
            }
            SETSYMBOL(ap+i, sym);
        }
        info_out((t_text *)x, s, i, ap);
    }
    else
        outlet_bang(x->x_out2);
}

void classinfo_externdir(t_classinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_class *c;
    if(!(c = classtable_findbyname(x->x_name)))
    {
        outlet_bang(x->x_out2);
        return;
    }
    t_atom at[1];
    SETSYMBOL(at, c->c_externdir);
    info_out((t_text *)x, s, 1, at);
}

void classinfo_methods(t_classinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_class *c;
    if(!(c = classtable_findbyname(x->x_name)))
    {
        outlet_bang(x->x_out2);
        return;
    }
    t_atom at[1];
    SETFLOAT(at, c->c_nmethod);
    info_out((t_text *)x, s, 1, at);
}

void classinfo_size(t_classinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_class *c;
    if(!(c = classtable_findbyname(x->x_name)))
    {
        outlet_bang(x->x_out2);
        return;
    }
    t_atom at[1];
    SETFLOAT(at, (t_float)c->c_size);
    info_out((t_text *)x, s, 1, at);
} 
 
void classinfo_float(t_classinfo *x, t_float f)
{
    t_class *c;
    if(c = classtable_findbyname(x->x_name))
    {
        if(f >= 0 && (t_int)f < c->c_nmethod)
        {
            t_atom ap[MAXPDARG+1];
            if(c->c_nmethod)
            {
                t_methodentry *me = c->c_methods;
                SETSYMBOL(ap, (me+(t_int)f)->me_name);
                t_atomtype arg, *args = (me+(t_int)f)->me_arg;
                t_int n;
                for(n = 1; arg = *args; args++, n++)
                {
                    t_symbol *s = attosym(arg);
                    SETSYMBOL(ap+n, s);
                }
                info_out((t_text *)x, &s_list, n, ap);
            }
        }
        else
            info_out((t_text *)x, &s_bang, 0, 0);
    }
    else
        outlet_bang(x->x_out2);
}

void classinfo_print(t_classinfo *x)
{
    info_print((t_text *)x);
}

void *classinfo_new(t_symbol *s)
{
    t_classinfo *x = (t_classinfo *)pd_new(classinfo_class);
    x->x_name = s;
    symbolinlet_new(&x->x_obj, &x->x_name);
    outlet_new(&x->x_obj, &s_anything);
    x->x_out2 = outlet_new(&x->x_obj, &s_bang);
    return (void *)x;
}

void classinfo_setup(void)
{
    classinfo_class = class_new(gensym("classinfo"),
        (t_newmethod)classinfo_new, 0,
        sizeof(t_classinfo),
        CLASS_DEFAULT, A_DEFSYM, 0);

    class_addfloat(classinfo_class, classinfo_float);
    class_addmethod(classinfo_class, (t_method)classinfo_args, gensym("args"),
        A_GIMME, 0);
    class_addmethod(classinfo_class, (t_method)classinfo_externdir,
        gensym("externdir"), A_GIMME, 0);
    class_addmethod(classinfo_class, (t_method)classinfo_methods,
        gensym("methods"), A_GIMME, 0);
    class_addmethod(classinfo_class, (t_method)classinfo_size, gensym("size"),
        A_GIMME, 0);
    class_addmethod(classinfo_class, (t_method)classinfo_print,
        gensym("print"), 0);

    post("classinfo: v.0.1");
    post("stable classinfo methods: size");

/*
   todo: make an objectinfo class to get the kind of info that canvasinfo "hitbox"
   currently gives
*/
}

/* -------------------------- objectinfo ------------------------------ */

int gpointer_check_gobj(const t_gpointer *gp);

t_gobj *objectinfo_getobject(t_objectinfo *x)
{
//    if (gpointer_check_gobj(&x->x_gp))
//        post("we passed the check in getobject");
    /* needs to pass the check AND point to a gobj */
    if (gpointer_check_gobj(&x->x_gp) && x->x_gp.gp_stub->gs_which == GP_GLIST)
    {
//        post("we passed total check");
        return x->x_gp.gp_un.gp_gobj;
    }
    else
        return 0;
}

void objectinfo_bang(t_objectinfo *x)
{
    t_atom at[1];
    t_gpointer gp;
    gpointer_init(&gp);
    gpointer_setglist(&gp, x->x_canvas, (t_gobj *)x);
//    if (gpointer_check_gobj(&gp))
//        post("creating pointer passed the check");
//    else
//        post("didn't create right");
    SETPOINTER(at, &gp);
    info_out((t_text *)x, &s_pointer, 1, at);
    gpointer_unset(&gp);
}

void objectinfo_float(t_objectinfo *x, t_floatarg f)
{
    /*
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    t_gobj *obj = objectinfo_getobject(x);
    post("object is .%x", obj);
    x->x_test = obj;
    */
}

void objectinfo_parseargs(t_objectinfo *x, int argc, t_atom *argv)
{
    /* probably don't need to specify index number as an argument */
    /*
    if (argc)
    {
        if (argv->a_type == A_FLOAT)
        {
            x->x_index = atom_getfloatarg(0, argc, argv);
        }
        else
        {
            pd_error(x, "expected float but didn't get float");
        }
        argc--;
        argv++;
    }
    */
    /* another stopgap comment out...
    if (argc)
    {
        if (argv->a_type == A_FLOAT)
            x->x_index = atom_getfloatarg(0, argc, argv);
        else
            pd_error(x, "expected float but didn't get a float");
    }
    */
}

void objectinfo_boxtext(t_objectinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_gobj *ob = objectinfo_getobject(x);
    if (ob)
    {
//        post("it's an obj");
        int n = 0;
        t_atom *a = 0;
        t_binbuf *b;
        /* if it's not t_object (e.g., a scalar), or if it is but
           does not have any binbuf content, send a bang... */
        if (!pd_checkobject(&ob->g_pd) ||
            !(b = ((t_text *)ob)->te_binbuf))
        {
            info_out((t_text *)x, &s_bang, 0, 0);
        }
        else
        {
            /* We have to escape semi, comma, dollar, and dollsym atoms,
               which is what binbuf_addbinbuf does.  Otherwise the user
               could pass them around or save them unescaped, which might 
               cause trouble. */
            t_binbuf *escaped = binbuf_new();
            binbuf_addbinbuf(escaped, b);
            n = binbuf_getnatom(escaped);
            a = binbuf_getvec(escaped);
            info_out((t_text *)x, s, n, a);
            binbuf_free(escaped);
        }
    }
    else
        outlet_bang(x->x_out2);
}

void objectinfo_bbox(t_objectinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_gobj *ob = objectinfo_getobject(x);
    int x1, y1, x2, y2;
    if(ob)
    {
      /* check for a getrectfn */
        if (ob->g_pd->c_wb && ob->g_pd->c_wb->w_getrectfn)
        {
            t_atom at[4];
            /* objectinfo_getobject will only return a gobj* if the gstub
               is a GP_GLIST, so we can safely fetch the glist from our
               gpointer.  Not sure if gobj can ever be inside an array, but
               if so I'm excluding those cases here... */
            t_canvas *c = x->x_gp.gp_stub->gs_un.gs_glist;
            gobj_getrect(ob, c, &x1, &y1, &x2, &y2);
            SETFLOAT(at, (t_float)x1);
            SETFLOAT(at+1, (t_float)y1);
            SETFLOAT(at+2, (t_float)x2);
            SETFLOAT(at+3, (t_float)y2);
            info_out((t_text *)x, s, 4, at);
        }
        else
        {
            info_out((t_text *)x, &s_bang, 0, 0);
        }
    }
    else
        outlet_bang(x->x_out2);
}

void objectinfo_classname(t_objectinfo *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_gobj *ob = objectinfo_getobject(x);
    t_atom at[1];
    if(ob)
    {
        char *classname = class_getname(ob->g_pd);
        SETSYMBOL(at, gensym(classname));
        info_out((t_text *)x, s, 1, at);
    }
    else
        outlet_bang(x->x_out2);
}

void objectinfo_index(t_objectinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_gobj *ob = objectinfo_getobject(x);
    if(ob)
    {
        t_atom at[4];
        t_gobj *y;
        int i;
        /* objectinfo_getobject will only return a gobj* if the gstub
           is a GP_GLIST, so we can safely fetch the glist from our
           gpointer.  Not sure if gobj can ever be inside an array, but
           if so I'm excluding those cases here... */
        t_canvas *c = x->x_gp.gp_stub->gs_un.gs_glist;
        for (i = 0, y = c->gl_list; y; y = y->g_next, i++)
        {
            if (y == ob)
            {
                SETFLOAT(at, (t_float)i);
                info_out((t_text *)x, s, 1, at);
                return;
            }
        }
        info_out((t_text *)x, s, 0, at);
    }
    else
        outlet_bang(x->x_out2);
}

void objectinfo_xlets(t_objectinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_gobj *ob = objectinfo_getobject(x);
    t_atom at[1];
    if(ob)
    {
        /* we exclude scalars here because they are not patchable */
        if (pd_class(&ob->g_pd) != scalar_class)
        {
//            post("not a scalar...");
            t_object *o = (t_object *)ob;
            int n = (s == gensym("inlets") ? obj_ninlets(o) : obj_noutlets(o));
            SETFLOAT(at, (t_float)n);
            info_out((t_text *)x, s, 1, at);
        }
    }
    else
        outlet_bang(x->x_out2);
}

void objectinfo_print(t_objectinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    objectinfo_parseargs(x, argc, argv);
    info_print((t_text *)x);
}

void *objectinfo_new(t_floatarg f)
{
    t_objectinfo *x = (t_objectinfo *)pd_new(objectinfo_class);
    x->x_canvas = canvas_getcurrent();
    pointerinlet_new(&x->x_obj, &x->x_gp);
    outlet_new(&x->x_obj, &s_anything);
    x->x_out2 = outlet_new(&x->x_obj, &s_bang);
    return (void *)x;
}

void objectinfo_setup(void)
{
    objectinfo_class = class_new(gensym("objectinfo"),
        (t_newmethod)objectinfo_new, 0,
        sizeof(t_objectinfo),
        CLASS_DEFAULT, A_DEFFLOAT, 0);

    class_addbang(objectinfo_class, objectinfo_bang);
    class_addfloat(objectinfo_class, objectinfo_float);
    class_addmethod(objectinfo_class, (t_method)objectinfo_bbox,
        gensym("bbox"), A_GIMME, 0);
    class_addmethod(objectinfo_class, (t_method)objectinfo_boxtext,
        gensym("boxtext"), A_GIMME, 0);
    class_addmethod(objectinfo_class, (t_method)objectinfo_classname,
        gensym("class"), A_GIMME, 0);
    class_addmethod(objectinfo_class, (t_method)objectinfo_index,
        gensym("index"), A_GIMME, 0);
    class_addmethod(objectinfo_class, (t_method)objectinfo_xlets,
        gensym("inlets"), A_GIMME, 0);
    class_addmethod(objectinfo_class, (t_method)objectinfo_xlets,
        gensym("outlets"), A_GIMME, 0);


    class_addmethod(objectinfo_class, (t_method)objectinfo_print,
        gensym("print"), A_GIMME, 0);

    post("objectinfo: v.0.1");
    post("stable objectinfo methods: class");
}

void x_interface_setup(void)
{
    print_setup();
    canvasinfo_setup();
    pdinfo_setup();
    classinfo_setup();
    objectinfo_setup();
}
