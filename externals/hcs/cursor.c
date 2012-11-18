/* TODO add reset method for cursor icons, this should probably be done in
pd.tk, or cursor reset method could be done in help patch */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <m_pd.h>
#include "g_canvas.h"

#define POLLTIME 10

static t_symbol *button_symbol;
static t_symbol *mousewheel_symbol;
static t_symbol *motion_symbol;
static t_symbol *x_symbol;
static t_symbol *y_symbol;
static t_symbol *cursor_receive_symbol;

t_int cursor_instances_polling;

static t_class *cursor_class;

typedef struct _cursor
{
    t_object x_obj;
    t_int    am_polling;
    t_symbol *receive_symbol;
    t_canvas *parent_canvas;
    t_outlet *data_outlet;
//    t_outlet *status_outlet; // not used (yet?)
} t_cursor;

static void create_namespace(void)
{
    sys_gui("if { [namespace exists ::hcs_cursor_class]} {\n");
    sys_gui("  puts stderr {WARNING: ::hcs_cursor_class namespace exists!}\n");
    sys_gui("} else {\n");
    sys_gui("  namespace eval ::hcs_cursor_class {\n");
    sys_gui("    variable continue_pollmotion 0\n");
    sys_gui("    variable last_x 0\n");
    sys_gui("    variable last_y 0\n");
    sys_gui("  }\n");
    sys_gui("}\n");
}

static void create_proc_test(void)
{
    sys_gui ("if {[info commands ::hcs_cursor_class::proc_test] eq {::hcs_cursor_class::proc_test}} {");
    sys_gui("  puts stderr {WARNING: ::hcs_cursor_class::proc_test exists!}\n");
    sys_gui("} else {\n");
    sys_gui("  proc ::hcs_cursor_class::proc_test {proc_name} {\n");
    sys_gui("    if {[info commands ::hcs_cursor_class::$proc_name] eq $proc_name} {\n");
    sys_gui("      puts stderr {WARNING: ::hcs_cursor_class::$proc_name exists!}\n");
    sys_gui("      return 1\n");
    sys_gui("    } else {\n");
    sys_gui("      return 0\n");
    sys_gui("    }\n");
    sys_gui("  }\n");
    sys_gui("}\n");
}

/* in Pd 0.43, the internal proc changed from 'pd' to 'pdsend' */
static void create_legacy_pd (void)
{
    post("creating legacy 'pdsend' using legacy 'pd' proc");
    sys_gui("if {[info commands pdsend] ne {pdsend}} {\n");
    sys_gui("  proc pdsend {message} {pd $message}\n");
    sys_gui("}\n");
}

/* idea from #tcl for a Tcl unbind */
static void create_unbind (void)
{
    sys_gui("if { ! [::hcs_cursor_class::proc_test unbind]} {");
    sys_gui("  proc ::hcs_cursor_class::unbind {tag event script} {\n");
    sys_gui("    set bind {}\n");
    sys_gui("    foreach x [split [bind $tag $event] \"\n\"] {\n");
    sys_gui("      if {$x != $script} {\n");
    sys_gui("        lappend bind $x\n");
    sys_gui("      }\n");
    sys_gui("    }\n");
    sys_gui("    bind $tag $event {}\n");
    sys_gui("    foreach x $bind {bind $tag $event $x}\n");
    sys_gui("  }\n");
    sys_gui("}\n");
}

static void create_button_proc(void)
{
    sys_gui ("if { ! [::hcs_cursor_class::proc_test button]} {");
    sys_gui ("  proc ::hcs_cursor_class::button {button state} {\n");
    sys_vgui("    pd [concat %s button $button $state \\;]\n",
             cursor_receive_symbol->s_name);
    sys_gui ("  }\n");
    sys_gui ("}\n");
}

static void create_mousewheel_proc(void)
{
    sys_gui ("if { ! [::hcs_cursor_class::proc_test mousewheel]} {");
    sys_gui ("  proc ::hcs_cursor_class::mousewheel {delta} {\n");
    sys_vgui("    pd [concat %s mousewheel $delta \\;]\n",
             cursor_receive_symbol->s_name);
    sys_gui ("  }\n");
    sys_gui ("}\n");
}

static void create_motion_proc(void)
{
    sys_gui("if { ![::hcs_cursor_class::proc_test motion]} {\n");
    sys_gui ("  proc ::hcs_cursor_class::motion {x y} {\n");
    sys_gui ("    if { $x != $::hcs_cursor_class::last_x \\\n");
    sys_gui ("      || $y != $::hcs_cursor_class::last_y} {\n");
    sys_vgui("        pd [concat %s motion $x $y \\;]\n",
             cursor_receive_symbol->s_name);
    sys_gui ("        set ::hcs_cursor_class::last_x $x\n");
    sys_gui ("        set ::hcs_cursor_class::last_y $y\n");
    sys_gui ("    }\n");
    sys_gui ("  }\n");
    sys_gui ("}\n");
}

static void create_pollmotion_proc(void)
{
    sys_gui ("if { ![::hcs_cursor_class::proc_test pollmotion]} {\n");
    sys_gui ("  proc ::hcs_cursor_class::pollmotion {} {\n");
    sys_vgui("    ::hcs_cursor_class::motion [winfo pointerx .] [winfo pointery .]\n");
    sys_gui ("    if {$::hcs_cursor_class::continue_pollmotion != 0} { \n");
    sys_gui ("      after 10 ::hcs_cursor_class::pollmotion\n");
    sys_gui ("    }\n");
    sys_gui ("  }\n");
    sys_gui ("}\n");
}

static void cursor_setmethod(t_cursor *x, t_symbol *s, int argc, t_atom *argv)
{
    sys_vgui("set cursor_%s \"%s\"\n", s->s_name, atom_getsymbol(argv)->s_name);
    canvas_setcursor(x->parent_canvas, 0); /* hack to refresh the cursor */
}
 
static void cursor_bang(t_cursor *x)
{
    sys_vgui("pd [concat %s motion [winfo pointerxy .] \\;]\n",
             x->receive_symbol->s_name);
}

static void cursor_float(t_cursor *x, t_float f)
{
    /* "bind all <Motion> only gives data when over windows, so its commented
     * out. See the cursor_bang function to see the pointer x,y data getting */
    if(f == 0) {
        if (x->am_polling) {
            x->am_polling = 0;
            cursor_instances_polling--;
            /* if no more objects are listening, stop sending the events */
            if (cursor_instances_polling == 0) {
                sys_gui("set ::hcs_cursor_class::continue_pollmotion 0 \n");
                sys_gui("::hcs_cursor_class::unbind all <ButtonPress> {::hcs_cursor_class::button %b 1}\n");
                sys_gui("::hcs_cursor_class::unbind all <ButtonRelease> {::hcs_cursor_class::button %b 0}\n");
                sys_gui("::hcs_cursor_class::unbind all <MouseWheel> {::hcs_cursor_class::mousewheel %D}\n");
            }
            pd_unbind(&x->x_obj.ob_pd, cursor_receive_symbol);
        }
    } else {
        if ( ! x->am_polling) {
            x->am_polling = 1;
            pd_bind(&x->x_obj.ob_pd, cursor_receive_symbol);
            cursor_instances_polling++;
            /* if this is the first instance to start, set up Tcl binding and polling */
            if (cursor_instances_polling == 1) {
                sys_gui("set ::hcs_cursor_class::continue_pollmotion 1 \n");
                sys_gui("::hcs_cursor_class::pollmotion \n");
                sys_gui("bind all <ButtonPress> {+::hcs_cursor_class::button %b 1}\n");
                sys_gui("bind all <ButtonRelease> {+::hcs_cursor_class::button %b 0}\n");
                sys_gui("bind all <MouseWheel> {+::hcs_cursor_class::mousewheel %D}\n");
            }
        }
    }
}

static void cursor_button_callback(t_cursor *x, t_float button, t_float state)
{
    t_atom output_atoms[2];
    
    SETFLOAT(output_atoms, button);
    SETFLOAT(output_atoms + 1, state);
    outlet_anything(x->data_outlet, button_symbol, 2, output_atoms);
}

static void cursor_motion_callback(t_cursor *x, t_float x_position, t_float y_position)
{
    t_atom output_atoms[2];
    
    SETSYMBOL(output_atoms, x_symbol);
    SETFLOAT(output_atoms + 1, x_position);
    outlet_anything(x->data_outlet, motion_symbol, 2, output_atoms);
    SETSYMBOL(output_atoms, y_symbol);
    SETFLOAT(output_atoms + 1, y_position);
    outlet_anything(x->data_outlet, motion_symbol, 2, output_atoms);
}
 
static void cursor_mousewheel_callback(t_cursor *x, t_float f)
{
    t_atom output_atom;
    
    SETFLOAT(&output_atom, f);
    outlet_anything(x->data_outlet, mousewheel_symbol, 1, &output_atom);
}

static void cursor_free(t_cursor *x)
{
    cursor_float(x, 0);
    pd_unbind(&x->x_obj.ob_pd, x->receive_symbol);
}

static void *cursor_new(void)
{
    char buf[MAXPDSTRING];
    t_cursor *x = (t_cursor *)pd_new(cursor_class);

    x->parent_canvas = canvas_getcurrent();

    sprintf(buf, "#%lx", (t_int)x);
    x->receive_symbol = gensym(buf);
    pd_bind(&x->x_obj.ob_pd, x->receive_symbol);
	x->data_outlet = outlet_new(&x->x_obj, 0);
	//x->status_outlet = outlet_new(&x->x_obj, 0);

    x->am_polling = 0;

    return(x);
}

void cursor_setup(void)
{
    cursor_class = class_new(gensym("cursor"),
        (t_newmethod)cursor_new, (t_method)cursor_free,
        sizeof(t_cursor), 0, 0);

    class_addbang(cursor_class, (t_method)cursor_bang);
    class_addfloat(cursor_class, (t_method)cursor_float);

    button_symbol = gensym("button");
    mousewheel_symbol = gensym("mousewheel");
    motion_symbol = gensym("motion");
    x_symbol = gensym("x");
    y_symbol = gensym("y");
    //status_symbol = gensym("status");
    cursor_receive_symbol = gensym("#hcs_cursor_class_receive");

    class_addmethod(cursor_class, (t_method)cursor_button_callback, 
                    button_symbol, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(cursor_class, (t_method)cursor_motion_callback, 
                    motion_symbol, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(cursor_class, (t_method)cursor_mousewheel_callback, 
                    mousewheel_symbol, A_DEFFLOAT, 0);

    /* methods for setting the cursor icon */
    class_addmethod(cursor_class, (t_method)cursor_setmethod, 
                    gensym("runmode_nothing"), A_GIMME, 0);
    class_addmethod(cursor_class, (t_method)cursor_setmethod, 
                    gensym("runmode_clickme"), A_GIMME, 0);
    class_addmethod(cursor_class, (t_method)cursor_setmethod, 
                    gensym("runmode_thicken"), A_GIMME, 0);
    class_addmethod(cursor_class, (t_method)cursor_setmethod, 
                    gensym("runmode_addpoint"), A_GIMME, 0);
    class_addmethod(cursor_class, (t_method)cursor_setmethod, 
                    gensym("editmode_nothing"), A_GIMME, 0);
    class_addmethod(cursor_class, (t_method)cursor_setmethod, 
                    gensym("editmode_connect"), A_GIMME, 0);
    class_addmethod(cursor_class, (t_method)cursor_setmethod, 
                    gensym("editmode_disconnect"), A_GIMME, 0);

    create_namespace();
    create_proc_test();
/* TODO figure this out once 0.43 is released */
/*    if(PD_MAJOR_VERSION == 0 && PD_MINOR_VERSION < 43)
        create_legacy_pd();*/
    create_unbind();
    create_motion_proc();
    create_pollmotion_proc();
    create_mousewheel_proc();
    create_button_proc();
}
