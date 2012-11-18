#include <stdio.h>
#include <string.h>
#include <m_pd.h>
#include "g_canvas.h"

#define DEBUG(x)

static t_class *sys_gui_class;

typedef struct _sys_gui
{
    t_object x_obj;
    char *send_buffer;
} t_sys_gui;

static void sys_gui_bang(t_sys_gui *x)
{
    sys_gui(x->send_buffer);
}

static void sys_gui_anything(t_sys_gui *x, t_symbol *s, int argc, t_atom *argv)
{
    DEBUG(post("sys_gui_anything"););
    int i = 0;
    char buf[MAXPDSTRING];

    snprintf(x->send_buffer, MAXPDSTRING, "%s ", s->s_name);
    for(i=0;i<argc;++i)
    {
        atom_string(argv + i, buf, MAXPDSTRING);
        strncat(x->send_buffer, buf, MAXPDSTRING - strlen(x->send_buffer));
        strncat(x->send_buffer, " ", MAXPDSTRING - strlen(x->send_buffer));
    }
    strncat(x->send_buffer, " ;\n", 3);
    sys_gui(x->send_buffer);
}

static void sys_gui_list(t_sys_gui *x, t_symbol *s, int argc, t_atom *argv)
{
    DEBUG(post("sys_gui_list"););
    int i = 0;
    char buf[MAXPDSTRING];

    for(i=0;i<argc;++i)
    {
        atom_string(argv + i, buf, MAXPDSTRING);
        strncat(x->send_buffer, buf, MAXPDSTRING - strlen(x->send_buffer));
        strncat(x->send_buffer, " ", MAXPDSTRING - strlen(x->send_buffer));
    }
    strncat(x->send_buffer, " ;\n", 3);
    sys_gui(x->send_buffer);
}

static void sys_gui_free(t_sys_gui *x)
{
    freebytes(x->send_buffer,MAXPDSTRING);
}

static void *sys_gui_new(t_symbol *s)
{
    t_sys_gui *x = (t_sys_gui *)pd_new(sys_gui_class);

	outlet_new(&x->x_obj, &s_anything);
    x->send_buffer = (char *)getbytes(MAXPDSTRING);

    return(x);
}

void sys_gui_setup(void)
{
    sys_gui_class = class_new(gensym("sys_gui"),
        (t_newmethod)sys_gui_new, (t_method)sys_gui_free,
        sizeof(t_sys_gui), 0, 0);

    class_addanything(sys_gui_class, (t_method)sys_gui_anything);
    class_addbang(sys_gui_class, (t_method)sys_gui_bang);
    class_addlist(sys_gui_class, (t_method)sys_gui_list);
}
