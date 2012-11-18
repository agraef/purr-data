#include <stdio.h>
#include <string.h>
#include <m_pd.h>
#include "g_canvas.h"

#define DEBUG(x)

static t_class *patch_name_class;
static t_canvas *canvas;

typedef struct _patch_name
{
    t_object x_obj;
    t_atom x_atom;
	t_canvas *x_canvas;

    t_symbol *x_patch_name;
	t_symbol *x_patch_path;
    t_symbol *x_remote_name;

	t_outlet *x_outlet_path;
	t_outlet *x_outlet_name;
} t_patch_name;

static void patch_name_bang(t_patch_name *x)
{
    char buf[MAXPDSTRING];

    snprintf(buf, MAXPDSTRING, "%s", x->x_canvas->gl_name->s_name);
    x->x_patch_name = gensym(buf);

	snprintf(buf, MAXPDSTRING, "%s", canvas_getdir(x->x_canvas)->s_name);
	x->x_patch_path = gensym(buf);

    outlet_symbol(x->x_outlet_name, x->x_patch_name);
	outlet_symbol(x->x_outlet_path,x->x_patch_path);
}

static void *patch_name_new(t_symbol *s, int argc, t_atom *argv)
{
    t_atom a;
    if (argc == 0)
    {
        argc = 1;
        SETFLOAT(&a, 0);
        argv = &a;
    }
    t_patch_name *x = (t_patch_name *)pd_new(patch_name_class);
    x->x_atom = *argv;
	t_glist *glist=(t_glist *)canvas_getcurrent(); 
	x->x_canvas=(t_canvas *)glist_getcanvas(glist);

    if (argv->a_type == A_FLOAT)
    {
        int depth=(int)atom_getint(&x->x_atom);

        if(depth<0)depth=0;
        while(depth && x->x_canvas->gl_owner) {
          x->x_canvas=x->x_canvas->gl_owner;
          depth--;
        }
    }
    else
    {
        x->x_remote_name = (t_symbol *)atom_getsymbol(&x->x_atom);
    }
    
    x->x_outlet_path = outlet_new(&x->x_obj, &s_symbol);
	x->x_outlet_name = outlet_new(&x->x_obj, &s_symbol);

    return(x);
}

void patch_name_setup(void)
{
    patch_name_class = class_new(gensym("patch_name"), 
        (t_newmethod)patch_name_new, NULL, 
        sizeof(t_patch_name), 0, A_GIMME, 0);

    class_addbang(patch_name_class, (t_method)patch_name_bang);
}
