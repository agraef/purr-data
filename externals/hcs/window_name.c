#include <stdio.h>
#include <string.h>
#include <m_pd.h>
#include "g_canvas.h"

#define DEBUG(x)

static t_class *window_name_class;
static t_canvas *canvas;

typedef struct _window_name
{
    t_object x_obj;
    t_atom x_atom;
    t_symbol *x_window_name;
    t_symbol *x_remote_name;
} t_window_name;

static void window_name_bang(t_window_name *x)
{
    if (x->x_atom.a_type == A_SYMBOL)
    {
        canvas=(t_canvas *)pd_findbyclass(x->x_remote_name, canvas_class);
        char buf[MAXPDSTRING];

        snprintf(buf, MAXPDSTRING, ".x%lx", (long unsigned int)canvas);
        x->x_window_name = gensym(buf);
    }
    outlet_symbol(x->x_obj.ob_outlet,x->x_window_name);
}

static void *window_name_new(t_symbol *s, int argc, t_atom *argv)
{
    t_atom a;
    if (argc == 0)
    {
        argc = 1;
        SETFLOAT(&a, 0);
        argv = &a;
    }
    t_window_name *x = (t_window_name *)pd_new(window_name_class);
    x->x_atom = *argv;
    if (argv->a_type == A_FLOAT)
    { // thx to IOhannes's iemguts:
        t_glist *glist=(t_glist *)canvas_getcurrent(); 
        canvas=(t_canvas *)glist_getcanvas(glist);
        int depth=(int)atom_getint(&x->x_atom);

        if(depth<0)depth=0;
        while(depth && canvas->gl_owner) {
          canvas=canvas->gl_owner;
          depth--;
        }
        char buf[MAXPDSTRING];

        snprintf(buf, MAXPDSTRING, ".x%lx", (long unsigned int)canvas);
        x->x_window_name = gensym(buf);
    }
    else
    {
        x->x_remote_name = (t_symbol *)atom_getsymbol(&x->x_atom);
    }
    
    outlet_new(&x->x_obj, &s_symbol);

    return(x);
}

void window_name_setup(void)
{
    window_name_class = class_new(gensym("window_name"), 
        (t_newmethod)window_name_new, NULL, 
        sizeof(t_window_name), 0, A_GIMME, 0);

    class_addbang(window_name_class, (t_method)window_name_bang);
}
