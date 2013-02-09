/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* interface objects */

#include "m_pd.h"
#include "g_canvas.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* -------------------------- print ------------------------------ */
t_class *print_class;

typedef struct _print
{
    t_object x_obj;
    t_symbol *x_sym;
} t_print;

static void *print_new(t_symbol *s, int argc, t_atom *argv)
{
    int bufsize;
    char *buf;
    t_print *x = (t_print *)pd_new(print_class);
    if (argc)
    {
        t_binbuf *bb = binbuf_new();
        binbuf_add(bb, argc, argv);
        binbuf_gettext(bb, &buf, &bufsize);
        buf[bufsize] = 0;
        x->x_sym = gensym(buf);
        binbuf_free(bb);
    }
    else 
    {
        x->x_sym = gensym("print");
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
    if (argc && argv->a_type != A_SYMBOL) startpost("%s:", x->x_sym->s_name);
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

/* -----------------canvasinfo, pdinfo, classinfo ------------------------------ */
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
    t_float   x_depth;
} t_classinfo;

/* used by all the *info objects */
void info_out(t_text *te, t_int all, t_symbol *s, int argc, t_atom *argv)
{
    if (all)
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

/* -------------------------- canvasinfo ------------------------------ */
t_canvas *canvasinfo_dig(t_canvasinfo *x)
{
  int depth = (int)x->x_depth;
  t_canvas *c = x->x_canvas;
  if(depth<0) depth = 0;

  while(depth && c->gl_owner) {
    c = c->gl_owner;
    depth--;
  }
  return c;
}

void canvasinfo_get(t_canvasinfo *x, t_symbol *s, t_int all)
{
    t_canvas *c = canvasinfo_dig(x);
    if(s == gensym("args"))
    {
        int argc = 0;
        t_atom *argv = 0;
        t_binbuf *b;
        if(!c) return;
        c = canvas_getrootfor(c);
        b = c->gl_obj.te_binbuf;

        if(!b)
        {
            info_out((t_text *)x, all, s, 0, 0);
        }
        else
        {
            argc = binbuf_getnatom(b);
            argv = binbuf_getvec(b);
            info_out((t_text *)x, all, s, argc-1, argv+1);
        }
    }
    else if(s == gensym("coords"))
    {
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
        info_out((t_text *)x, all, s, 9, at);
    }
    else if(s == gensym("dir"))
    {
        c = canvas_getrootfor(c);
        t_atom at[1];
        SETSYMBOL(at, canvas_getdir(c));
        info_out((t_text *)x, all, s, 1, at);
    }
    else if (s == gensym("dirty"))
    {
        t_atom at[1];
        SETFLOAT(at, c->gl_dirty);
        info_out((t_text *)x, all, s, 1, at);
    }
    else if (s == gensym("dollarzero"))
    {
        c = canvas_getrootfor(c);
        t_symbol *d = gensym("$0");
        t_symbol *ret = canvas_realizedollar(c, d);
        float f = (float)strtod(ret->s_name,NULL);
        t_atom at[1];
        SETFLOAT(at, f);
        info_out((t_text *)x, all, s, 1, at);
    }
    else if (s == gensym("editmode"))
    {
        t_atom at[1];
        SETFLOAT(at, c->gl_edit);
        info_out((t_text *)x, all, s, 1, at);
    }
    else if (s == gensym("filename"))
    {
        c = canvas_getrootfor(c);
        t_atom at[1];
        SETSYMBOL(at, c->gl_name);
        info_out((t_text *)x, all, s, 1, at);
    }
    else if (s == gensym("name"))
    {
        char buf[MAXPDSTRING];
        snprintf(buf, MAXPDSTRING, ".x%lx", (long unsigned int)c);
        t_atom at[1];
        SETSYMBOL(at, gensym(buf));
        info_out((t_text *)x, all, s, 1, at);
    }
    else if (s == gensym("parent"))
    {
        t_atom at[1];
        if (c->gl_owner)
        {
            t_gpointer gp;
            gpointer_init(&gp);
            gpointer_setglist(&gp, c->gl_owner, 0);
            SETPOINTER(at, &gp);
            info_out((t_text *)x, all, s, 1, at);
            gpointer_unset(&gp);
        }
        else
        {
            SETFLOAT(at, 0);
            info_out((t_text *)x, all, s, 1, at);
        }
    }
    else if (s == gensym("posonparent"))
    {
        t_atom at[2];
        SETFLOAT(at, c->gl_obj.te_xpix);
        SETFLOAT(at+1, c->gl_obj.te_ypix);
        info_out((t_text *)x, all, s, 2, at);
    }
    else if (s == gensym("screenpos"))
    {
        t_atom at[4];
        SETFLOAT(at, c->gl_screenx1);
        SETFLOAT(at+1, c->gl_screeny1);
        SETFLOAT(at+2, c->gl_screenx2);
        SETFLOAT(at+3, c->gl_screeny2);
        info_out((t_text *)x, all, s, 4, at);
    }
    else if (s == gensym("self"))
    {
        t_atom at[1];
        t_gpointer gp;
        gpointer_init(&gp);
        gpointer_setglist(&gp, c, 0);
        SETPOINTER(at, &gp);
        info_out((t_text *)x, all, s, 1, at);
        gpointer_unset(&gp);
    }
    else if (s == gensym("vis"))
    {
        t_atom at[1];
        SETFLOAT(at, glist_isvisible(c));
        info_out((t_text *)x, all, s, 1, at);
    }
    else
    {
        pd_error(x, "canvasinfo: no %s property", s->s_name);
    }
}

void canvasinfo_symbol(t_canvasinfo *x, t_symbol *s)
{
    canvasinfo_get(x, s, 0);
}

void canvasinfo_bang(t_canvasinfo *x)
{
    canvasinfo_get(x, gensym("args"), 1);
    canvasinfo_get(x, gensym("coords"), 1);
    canvasinfo_get(x, gensym("dir"), 1);
    canvasinfo_get(x, gensym("dollarzero"), 1);
    canvasinfo_get(x, gensym("editmode"), 1);
    canvasinfo_get(x, gensym("filename"), 1);
    canvasinfo_get(x, gensym("name"), 1);
    canvasinfo_get(x, gensym("parent"), 1);
    canvasinfo_get(x, gensym("posonparent"), 1);
    canvasinfo_get(x, gensym("screenpos"), 1);
    canvasinfo_get(x, gensym("self"), 1);
    canvasinfo_get(x, gensym("vis"), 1);
}

void canvasinfo_pointer(t_canvasinfo *x, t_gpointer *gp)
{
    pd_error(x, "canvasinfo: no method for pointer");
}

void canvasinfo_float(t_canvasinfo *x, t_float f)
{
    pd_error(x, "canvasinfo: no method for float");
}

static void canvasinfo_list(t_canvasinfo *x, t_symbol *s, int ac, t_atom *av)
{
    obj_list(&x->x_obj, 0, ac, av);
}

static void canvasinfo_anything(t_canvasinfo *x, t_symbol *s, int ac, t_atom *av)
{
    t_atom *av2 = (t_atom *)getbytes((ac + 1) * sizeof(t_atom));
    int i;
    for (i = 0; i < ac; i++)
        av2[i + 1] = av[i];
    SETSYMBOL(av2, s);
    obj_list(&x->x_obj, 0, ac+1, av2);
    freebytes(av2, (ac + 1) * sizeof(t_atom));
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
    class_addpointer(canvasinfo_class, canvasinfo_pointer);
    class_addfloat(canvasinfo_class, canvasinfo_float);
    class_addsymbol(canvasinfo_class, canvasinfo_symbol);
    class_addlist(canvasinfo_class, canvasinfo_list);
    class_addanything(canvasinfo_class, canvasinfo_anything);
}

/* -------------------------- pdinfo ------------------------------ */
static t_class *pdinfo_class;

void pdinfo_get(t_pdinfo *x, t_symbol *s, t_int all)
{
    if(s == gensym("dsp"))
    {
        t_atom at[1];
        SETFLOAT(at, (t_float)canvas_dspstate);
        info_out((t_text *)x, all, s, 1, at);
    }
    else if(s == gensym("version"))
    {
        int major=0, minor=0, bugfix=0;
        sys_getversion(&major, &minor, &bugfix);
        t_atom at[3];
        SETFLOAT(at, (t_float)major);
        SETFLOAT(at+1, (t_float)minor);
        SETFLOAT(at+2, (t_float)bugfix);
        info_out((t_text *)x, all, s, 3, at);
    }
}

void pdinfo_symbol(t_pdinfo *x, t_symbol *s)
{
    pdinfo_get(x, s, 0);
}

void pdinfo_bang(t_pdinfo *x)
{
    pdinfo_get(x, gensym("dsp"), 1);
    pdinfo_get(x, gensym("version"), 1);
}

static void pdinfo_anything(t_pdinfo *x, t_symbol *s, int ac, t_atom *av)
{
    if(av->a_type == A_FLOAT)
    {
        pd_error(x, "pdinfo: no method for float");
        return;
    }
    else if(av->a_type == A_POINTER)
    {
        pd_error(x, "pdinfo: no method for pointer");
        return;
    }
    pdinfo_get(x, s, 0);
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

    class_addbang(pdinfo_class, pdinfo_bang);
    class_addsymbol(pdinfo_class, pdinfo_symbol);
    class_addanything(pdinfo_class, pdinfo_anything);
}

/* -------------------------- classinfo ------------------------------ */
void classinfo_get(t_classinfo *x, t_symbol *s, t_int all)
{
    t_atom at[1];
    SETFLOAT(at, (zgetfn(&pd_objectmaker, s)) ? 1 : 0);
    info_out((t_text *)x, all, s, 1, at);
}

void classinfo_symbol(t_classinfo *x, t_symbol *s)
{
    classinfo_get(x, s, 0);
}

void classinfo_bang(t_classinfo *x)
{
     
}

static void classinfo_anything(t_classinfo *x, t_symbol *s, int ac, t_atom *av)
{
    if(av->a_type == A_FLOAT)
    {
        pd_error(x, "classinfo: no method for float");
        return;
    }
    else if(av->a_type == A_POINTER)
    {
        pd_error(x, "classinfo: no method for pointer");
        return;
    }
    classinfo_get(x, s, 0);
}

void *classinfo_new(t_symbol *s, t_int argc, t_atom *argv)
{
    t_classinfo *x = (t_classinfo *)pd_new(classinfo_class);

    outlet_new(&x->x_obj, &s_anything);    
    return (void *)x;
}

void classinfo_setup(void)
{
    classinfo_class = class_new(gensym("classinfo"),
        (t_newmethod)classinfo_new, 0,
        sizeof(t_classinfo),
        CLASS_DEFAULT, 0);

    class_addbang(classinfo_class, classinfo_bang);
    class_addanything(classinfo_class, classinfo_anything);
    class_addsymbol(classinfo_class, classinfo_symbol);
}

void x_interface_setup(void)
{
    print_setup();
    canvasinfo_setup();
    pdinfo_setup();
    classinfo_setup();
}
