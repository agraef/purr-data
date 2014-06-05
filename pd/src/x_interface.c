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
    post("%s: bang", x->x_sym->s_name);
}

static void print_pointer(t_print *x, t_gpointer *gp)
{
    post("%s: (gpointer)", x->x_sym->s_name);
}

static void print_float(t_print *x, t_float f)
{
    post("%s: %g", x->x_sym->s_name, f);
}

static void print_list(t_print *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    if (argc && argv->a_type != A_SYMBOL)
		startpost("%s%s%g", x->x_sym->s_name,
            (*x->x_sym->s_name ? ": " : ""),
            atom_getfloatarg(0, argc--, argv++));
    else startpost("%s: %s", x->x_sym->s_name,
        (argc > 1 ? s_list.s_name : (argc == 1 ? s_symbol.s_name :
            s_bang.s_name)));
    postatom(argc, argv);
    endpost();
}

static void print_anything(t_print *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    startpost("%s: %s", x->x_sym->s_name, s->s_name);
    postatom(argc, argv);
    endpost();
}

static void print_setup(void)
{
    print_class = class_new(gensym("print"), (t_newmethod)print_new, 0,
        sizeof(t_print), 0, A_GIMME, 0);
    class_addbang(print_class, print_bang);
    class_addfloat(print_class, print_float);
    class_addpointer(print_class, print_pointer);
    class_addlist(print_class, print_list);
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
    t_canvas *x_canvas;
    t_float x_index;
    t_float x_depth;
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
    else {
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
      return c;
  }
}

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
        n = binbuf_getnatom(b);
        a = binbuf_getvec(b);
        if (s == gensym("args"))
            info_out((t_text *)x, s, n-1, a+1);
        else
            info_out((t_text *)x, s, n, a);
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

void canvasinfo_hitbox(t_canvasinfo *x, t_floatarg xpos, t_floatarg ypos)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    int x1, y1, x2, y2, indexno;
    t_gobj *ob = canvas_findhitbox(c, xpos, ypos, &x1, &y1, &x2, &y2);
    if (ob)
    {
        t_gobj *y;
        for (indexno = 0, y = c->gl_list; y && y != ob; y = y->g_next)
            indexno++;
        t_atom at[6];
        char *classname = class_getname(ob->g_pd);
        SETSYMBOL(at, gensym(classname));
        SETFLOAT(at+1, (t_float)indexno);
        SETFLOAT(at+2, (t_float)x1);
        SETFLOAT(at+3, (t_float)y1);
        SETFLOAT(at+4, (t_float)x2);
        SETFLOAT(at+5, (t_float)y2);
        info_out((t_text *)x, gensym("hitbox"), 6, at);
    }
    else
        info_out((t_text *)x, gensym("hitbox"), 0, 0);
}

void canvasinfo_name(t_canvasinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING, ".x%lx", (long unsigned int)c);
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

void pdinfo_dsp(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)canvas_dspstate);
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_audio_api(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    t_atom at[1];
    t_symbol *api = getapiname(sys_audioapi);
    SETSYMBOL(at, api);
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_audioin(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
//        char i
}

void pdinfo_audio_api_list_raw(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    t_atom at[7];
    int i;
    for(i = 0; i < 7; i++)
        SETSYMBOL(at+i, getapiname(i));
    info_out((t_text *)x, s, i, at);
}

void pdinfo_audio_apilist(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
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
    t_atom at[4];
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

void pdinfo_audio_dev(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    int devno;
    if (argc) devno = (int)atom_getfloatarg(0, argc, arg);
    else devno = 0;
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int rate, advance, callback, blocksize;
    sys_get_audio_params(&naudioindev, audioindev, chindev,
        &naudiooutdev, audiooutdev, choutdev, &rate, &advance, &callback);
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

void pdinfo_midi_api(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
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

void pdinfo_midi_apilist(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
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
    t_atom at[4];
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

void pdinfo_midi_dev(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    int devno, nmidiindev, midiindev[MAXMIDIINDEV],
        nmidioutdev, midioutdev[MAXMIDIOUTDEV];
    int *dev, *chan, ndev;
    t_atom at[4];
    if (argc) devno = (int)atom_getfloatarg(0, argc, arg);
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

void pdinfo_audio_outdev(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    int devno;
    if (argc) devno = (int)atom_getfloatarg(0, argc, arg);
    else devno = 0;
    int naudioindev, audioindev[MAXAUDIOINDEV], chindev[MAXAUDIOINDEV];
    int naudiooutdev, audiooutdev[MAXAUDIOOUTDEV], choutdev[MAXAUDIOOUTDEV];
    int rate, advance, callback, blocksize;
    sys_get_audio_params(&naudioindev, audioindev, chindev,
        &naudiooutdev, audiooutdev, choutdev, &rate, &advance, &callback);
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

void pdinfo_audio_inchannels(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)sys_get_inchannels());
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_audio_outchannels(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)sys_get_outchannels());
    info_out((t_text *)x, s, 1, at);
}


void pdinfo_audio_samplerate(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)sys_getsr());
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_audio_blocksize(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    t_atom at[1];
    SETFLOAT(at, (t_float)sys_getblksize());
    info_out((t_text *)x, s, 1, at);
}

void pdinfo_version(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    int major=0, minor=0, bugfix=0;
    sys_getversion(&major, &minor, &bugfix);
    t_atom at[3];
    SETFLOAT(at, (t_float)major);
    SETFLOAT(at+1, (t_float)minor);
    SETFLOAT(at+2, (t_float)bugfix);
    info_out((t_text *)x, s, 3, at);
}

void pdinfo_pi(t_pdinfo *x, t_symbol *s, int argc, t_atom *arg)
{
    t_atom at[1];
    const t_float Pi = 3.141592653589793;
    SETFLOAT(at, Pi);
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

    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_api,
        gensym("audio-api"), A_DEFFLOAT, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_apilist,
        gensym("audio-apilist"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_api_list_raw,
        gensym("audio-apilist-raw"), A_GIMME, 0);
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
    class_addmethod(pdinfo_class, (t_method)pdinfo_dir,
        gensym("dir"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_dsp,
        gensym("dsp-status"), A_GIMME, 0);
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
    class_addmethod(pdinfo_class, (t_method)pdinfo_pi,
        gensym("pi"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_audio_samplerate,
        gensym("samplerate"), A_GIMME, 0);
    class_addmethod(pdinfo_class, (t_method)pdinfo_version,
        gensym("version"), A_GIMME, 0);

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

t_gobj *objectinfo_getobject(t_canvas *c, int index)
{
    int i = index;
    t_gobj *y = c->gl_list;
    while(i-- && y)
        y = y->g_next;
    return y;
}

void objectinfo_float(t_floatarg f)
{

}

void objectinfo_boxtext(t_objectinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    t_gobj *ob;
   
    if(ob = objectinfo_getobject(c, x->x_index))
    {
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
            n = binbuf_getnatom(b);
            a = binbuf_getvec(b);
            info_out((t_text *)x, s, n, a);
        }
    }
    else
        outlet_bang(x->x_out2);
}

void objectinfo_bbox(t_objectinfo *x, t_symbol *s, int argc, t_atom *argv)
{
    t_gobj *ob;
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    int x1, y1, x2, y2;
    if(ob = objectinfo_getobject(c, x->x_index))
    {
        /* check for a getrectfn */
        if (ob->g_pd->c_wb && ob->g_pd->c_wb->w_getrectfn)
        {
            t_atom at[4];
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
    t_atom at[1];
    t_gobj *ob;
    t_canvas *c = canvas_climb(x->x_canvas, x->x_depth);
    if(ob = objectinfo_getobject(c, x->x_index))
    {
        char *classname = class_getname(ob->g_pd);
        SETSYMBOL(at, gensym(classname));
        info_out((t_text *)x, s, 1, at);
    }
    else
        outlet_bang(x->x_out2);
}

void objectinfo_print(t_classinfo *x)
{
    info_print((t_text *)x);
}

void *objectinfo_new(t_floatarg f)
{
    t_objectinfo *x = (t_objectinfo *)pd_new(objectinfo_class);
    t_glist *glist = (t_glist *)canvas_getcurrent();
    x->x_canvas = (t_canvas*)glist_getcanvas(glist);
    x->x_index = f;
    floatinlet_new(&x->x_obj, &x->x_index);
    floatinlet_new(&x->x_obj, &x->x_depth);
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

    class_addfloat(objectinfo_class, objectinfo_float);
    class_addmethod(objectinfo_class, (t_method)objectinfo_bbox,
        gensym("bbox"), A_GIMME, 0);
    class_addmethod(objectinfo_class, (t_method)objectinfo_boxtext,
        gensym("boxtext"), A_GIMME, 0);
    class_addmethod(objectinfo_class, (t_method)objectinfo_classname,
        gensym("class"), A_GIMME, 0);
    class_addmethod(objectinfo_class, (t_method)objectinfo_print,
        gensym("print"), 0);

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
