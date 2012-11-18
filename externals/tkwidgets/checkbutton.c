/* [checkbutton] object for dislaying a check box

   Copyright (C) 2007 Hans-Christoph Steiner <hans@at.or.at>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   This is part of the tkwidgets library for Pd.

*/

/* TODO rectify char and pixel widths/heights ug */
/* TODO bind to <Configure> so that things are redrawn when the label changes */
/* TODO add float method to set state based on == 0 and everything else */

#include "tkwidgets.h"

#define DEBUG(x) x

#define TOTAL_INLETS            1
#define TOTAL_OUTLETS           2

/* ------------------------ class variables --------------------------------- */

static t_class *checkbutton_class;
static t_widgetbehavior checkbutton_widgetbehavior;

typedef struct _checkbutton
{
    t_object    x_obj;
    t_canvas*   x_canvas;      /* canvas this widget is currently drawn in */
    t_glist*    x_glist;       /* glist that owns this widget */
    t_binbuf*   options_binbuf;/* binbuf to save options state in */

    int         width;
    int         height;

    int         x_resizing;
    int         x_selected;
    
    /* IDs for Tk widgets */
	t_symbol*   tcl_namespace;       
    t_symbol*   receive_name;  /* name to bind to to receive callbacks */
	t_symbol*   canvas_id;  
	t_symbol*   widget_id;        
	t_symbol*   handle_id;      
    t_symbol*   window_tag;
	t_symbol*   iolets_tag;
	t_symbol*   all_tag;
    
    t_outlet*   x_data_outlet;
    t_outlet*   x_status_outlet;
} t_checkbutton;

static char *checkbutton_tk_options[] = {
    "activebackground",
    "activeforeground",
    "anchor",
    "background",
    "bitmap",
    "borderwidth",
//    "command", /* problematic since it uses names based on pointers */
    "compound",
    "cursor",
    "disabledforeground",
    "font",
    "foreground",
    "height",
    "highlightbackground",
    "highlightcolor",
    "highlightthickness",
    "image",
    "indicatoron",
    "justify",
    "offrelief",
    "offvalue",
    "onvalue",
    "overrelief",
    "padx",
    "pady",
    "relief",
    "selectcolor",
    "selectimage",
    "state",
    "takefocus",
    "text",
    "textvariable",
    "underline",
//    "variable", /* problematic since it uses names based on pointers */
    "width",
    "wraplength"
};

/* -------------------- common symbols to preload --------------------------- */

static t_symbol *id_symbol;
static t_symbol *query_callback_symbol;
static t_symbol *size_symbol;

/* -------------------- function prototypes --------------------------------- */

static void checkbutton_query_callback(t_checkbutton *x, t_symbol *s, int argc, t_atom *argv);
static void checkbutton_set(t_checkbutton* x,  t_float f);

/* -------------------- widget helper functions------------------------------ */

static void set_tkwidgets_ids(t_checkbutton* x, t_canvas* canvas)
{
    x->x_canvas = canvas;
    x->canvas_id = tkwidgets_gen_canvas_id(x->x_canvas);
    x->widget_id = tkwidgets_gen_widget_id((t_object*)x, x->canvas_id);
    x->window_tag = tkwidgets_gen_window_tag((t_object*)x, x->canvas_id);
    x->handle_id = tkwidgets_gen_handle_id((t_object *)x, x->canvas_id);
}

static void drawme(t_checkbutton *x, t_glist *glist)
{
    set_tkwidgets_ids(x,glist_getcanvas(glist));
    sys_vgui("destroy %s\n", x->widget_id->s_name); /* just in case it exists */
    sys_vgui("namespace eval %s {} \n", x->tcl_namespace->s_name);
    sys_vgui("set ::%s::state 0\n", x->tcl_namespace->s_name);
    sys_vgui("checkbutton %s -variable ::%s::state \
-command {pd [concat %s output $::%s::state \\;]}\n", 
             x->widget_id->s_name, x->tcl_namespace->s_name, 
             x->receive_name->s_name, x->tcl_namespace->s_name);
    if(x->x_glist == x->x_canvas) // if GOP, don't draw inlets
        tkwidgets_draw_iolets((t_object*)x, glist, 
                              x->canvas_id, x->iolets_tag, x->all_tag,
                              x->width, x->height);
    sys_vgui("%s create window %d %d -anchor nw -window %s -tags {%s %s}\n", 
             x->canvas_id->s_name, 
             text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
             x->widget_id->s_name,
             x->window_tag->s_name, x->all_tag->s_name);
    tkwidgets_bind_key_events(x->canvas_id, x->widget_id);
    tkwidgets_bind_mouse_events(x->canvas_id, x->widget_id);
}


static void eraseme(t_checkbutton* x)
{
    sys_vgui("destroy %s\n", x->widget_id->s_name);
    sys_vgui("%s delete %s\n", x->canvas_id->s_name, x->all_tag->s_name);
}

/* --------------------- query functions ------------------------------------ */

static void query_id(t_checkbutton *x)
{
    t_atom id[2];
    t_symbol *widget_id = x->widget_id;
    SETSYMBOL(id, id_symbol);
    SETSYMBOL(id + 1, widget_id);
    checkbutton_query_callback(x, query_callback_symbol, 2, id);
}

static void query_size(t_checkbutton *x)
{
    t_atom coords[3];
    SETSYMBOL(coords, size_symbol);
    SETFLOAT(coords + 1, (t_float)x->width);
    SETFLOAT(coords + 2, (t_float)x->height);
    checkbutton_query_callback(x, query_callback_symbol, 3, coords);
}

static void checkbutton_query(t_checkbutton *x, t_symbol *s)
{
    post("checkbutton_query %s", s->s_name);
    if(s == &s_)
    {
        tkwidgets_query_options(x->receive_name, x->widget_id, 
                                sizeof(checkbutton_tk_options)/sizeof(char *), 
                                checkbutton_tk_options);
        query_id(x);
        query_size(x);
    }
    else if(s == id_symbol)
        query_id(x);
    else if(s == size_symbol)
        query_size(x);
    else
        tkwidgets_query_options(x->receive_name, x->widget_id, 1, &(s->s_name));
}


/* --------------------- checkbutton widgetbehaviour ------------------------ */
static void checkbutton_getrect(t_gobj *z, t_glist *glist,
                          int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_checkbutton* x = (t_checkbutton*)z;

    *xp1 = text_xpix(&x->x_obj, glist);
    *yp1 = text_ypix(&x->x_obj, glist);
    *xp2 = text_xpix(&x->x_obj, glist) + x->width;
    *yp2 = text_ypix(&x->x_obj, glist) + x->height;
}

static void checkbutton_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_checkbutton *x = (t_checkbutton *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    if (glist_isvisible(glist))
    {
        sys_vgui("%s move %s %d %d\n", 
                 x->canvas_id->s_name, x->all_tag->s_name, dx, dy);
        sys_vgui("%s move RSZ %d %d\n", x->canvas_id->s_name, dx, dy);
        canvas_fixlinesfor(glist_getcanvas(glist), (t_text*) x);
    }
}

static void checkbutton_select(t_gobj *z, t_glist *glist, int state)
{
    t_checkbutton *x = (t_checkbutton *)z;
    DEBUG(post("checkbutton_select: canvas %lx glist %lx state %d", x->x_canvas, glist, state););
    
    if( (state) && (!x->x_selected))
    {
        sys_vgui("set ::%s::bg [%s cget -bg]\n", 
                 x->tcl_namespace->s_name, x->widget_id->s_name);
        sys_vgui("%s configure -bg %s\n",
                 x->widget_id->s_name, TKW_SELECTION_COLOR);
        x->x_selected = 1;
    }
    else if (!state)
    {
        sys_vgui("%s configure -bg $::%s::bg\n",
                 x->widget_id->s_name, x->tcl_namespace->s_name);
        /* activatefn never gets called with 0, so destroy handle here */
        sys_vgui("destroy %s\n", x->handle_id->s_name);
        x->x_selected = 0;
    }
}
static void checkbutton_delete(t_gobj *z, t_glist *glist)
{
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist_getcanvas(glist), x);
}
       
static void checkbutton_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_checkbutton* s = (t_checkbutton*)z;
    if (vis)
        drawme(s, glist);
    else
        eraseme(s);
}

static void checkbutton_save(t_gobj *z, t_binbuf *b)
{
    t_checkbutton *x = (t_checkbutton *)z;
    binbuf_addv(b, "ssiisii", &s__X, gensym("obj"),
                x->x_obj.te_xpix, x->x_obj.te_ypix, 
                atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)),
                x->width, x->height);
// TODO    binbuf_addbinbuf(b, x->options_binbuf);
    binbuf_addv(b, ";");
}

/* -------------------- methods for atoms ----------------------------------- */

/* this function uses the selector as the Tk option and applies the whole
 * message directly to the Tk widget itself using Tk's "configure".  This
 * function is called when "anything" is received. */
static void checkbutton_set_option(t_checkbutton *x, t_symbol *s, int argc, t_atom *argv)
{
    if(s != &s_list)
    {
        t_binbuf *argument_binbuf = binbuf_new();
        char *argument_buffer;
        int buffer_length;
        
        binbuf_add(argument_binbuf, argc, argv);
        binbuf_gettext(argument_binbuf, &argument_buffer, &buffer_length);
        binbuf_free(argument_binbuf);
        argument_buffer[buffer_length] = 0;
        post("%s configure -%s {%s} \n", 
                 x->widget_id->s_name, s->s_name, argument_buffer);
        sys_vgui("%s configure -%s {%s} \n", 
                 x->widget_id->s_name, s->s_name, argument_buffer);
        tkwidgets_store_options(x->receive_name, x->tcl_namespace, x->widget_id, 
                                sizeof(checkbutton_tk_options)/sizeof(char *), 
                                (char **)&checkbutton_tk_options);
    }
}

/* Pass the contents of the text widget onto the checkbutton_output_callback
 * fuction above */
static void checkbutton_bang_output(t_checkbutton* x)
{
    sys_vgui("%s invoke\n", x->widget_id->s_name);
}

/* Pass the contents of the text widget onto the checkbutton_output_callback
 * fuction above */
static void checkbutton_float_output(t_checkbutton* x, t_float f)
{
    checkbutton_set(x, f);
    outlet_float(x->x_data_outlet, f);
}

/* --------------------------- methods for pd space ------------------------- */

static void checkbutton_options(t_checkbutton *x)
{
    tkwidgets_list_options(x->x_status_outlet,
                           sizeof(checkbutton_tk_options)/sizeof(char *), 
                           (char **)&checkbutton_tk_options);
}

static void checkbutton_set(t_checkbutton* x,  t_float f)
{
    /* TODO figure out which matches make sense, in terms of offvalue/onvalue */
    sys_vgui("if {%g != 0} {%s select} else {%s deselect}\n",
             f, x->widget_id->s_name, x->widget_id->s_name, x->widget_id->s_name);
}

static void checkbutton_size(t_checkbutton *x, t_float width, t_float height)
{
    DEBUG(post("checkbutton_size"););
    x->height = height;
    x->width = width;
    if(glist_isvisible(x->x_glist))
    {
        sys_vgui("%s itemconfigure %s -width %d -height %d\n",
                 x->canvas_id->s_name, x->window_tag->s_name, x->width, x->height);
        canvas_fixlinesfor(x->x_glist, (t_text *)x);  // 2nd inlet
    }
}

/* --------------------------- callback functions --------------------------- */

static void checkbutton_output_callback(t_checkbutton* x, t_float f)
{
    outlet_float(x->x_data_outlet, f);
}

static void checkbutton_query_callback(t_checkbutton *x, t_symbol *s, 
                                       int argc, t_atom *argv)
{
    t_symbol *tmp_symbol = atom_getsymbolarg(0, argc, argv);
    if(tmp_symbol != &s_)
        outlet_anything(x->x_status_outlet, tmp_symbol, argc - 1, argv + 1);
    else
        post("checkbutton_query_callback %s %d", s->s_name, argc);
}

static void checkbutton_store_callback(t_checkbutton *x, t_symbol *s, 
                                       int argc, t_atom *argv)
{
    if(s != &s_)
    {
        binbuf_clear(x->options_binbuf);
        binbuf_restore(x->options_binbuf, argc, argv);
    }
    else
        post("ERROR: does this ever happen?");
}

static void checkbutton_resize_click_callback(t_checkbutton *x, t_floatarg f)
{
    t_canvas *canvas = (glist_isvisible(x->x_glist) ? x->x_canvas : 0);
    int button_state = (int)f;
    if(x->x_resizing && !button_state && canvas)
    {
        tkwidgets_draw_iolets((t_object*)x, canvas,
                              x->canvas_id, x->iolets_tag, x->all_tag,
                              x->width, x->height);
        canvas_fixlinesfor(x->x_glist, (t_text *)x);  // 2nd inlet
    }
    else if(!x->x_resizing && button_state)
    {
        tkwidgets_erase_iolets(x->canvas_id, x->iolets_tag);
    }
    x->x_resizing = button_state;
}

static void checkbutton_resize_motion_callback(t_checkbutton *x, 
                                               t_floatarg f1, t_floatarg f2)
{
    DEBUG(post("checkbutton_resize_motion_callback"););
    if (x->x_resizing)
    {
        int dx = (int)f1, dy = (int)f2;
        if (glist_isvisible(x->x_glist))
        {
            x->width += dx;
            x->height += dy;
            sys_vgui("%s itemconfigure %s -width %d -height %d\n",
                     x->canvas_id->s_name, x->window_tag->s_name, 
                     x->width, x->height);
            sys_vgui("%s move RESIZE %d %d\n",
                     x->canvas_id->s_name, dx, dy);
            canvas_fixlinesfor(x->x_glist, (t_text *)x);
        }
    }
}

/* --------------------------- standard class functions --------------------- */

static void checkbutton_free(t_checkbutton *x)
{
    pd_unbind(&x->x_obj.ob_pd, x->receive_name);
}

static void *checkbutton_new(t_symbol* s, int argc, t_atom *argv)
{
    t_checkbutton *x = (t_checkbutton *)pd_new(checkbutton_class);

    x->options_binbuf = binbuf_new();

    x->width = 30;
    x->height = 30;

    if(argc > 0) x->width = atom_getint(argv);
    if(argc > 1) x->height = atom_getint(argv + 1);
    if(argc > 2) binbuf_add(x->options_binbuf, argc - 2, argv + 2);

    x->tcl_namespace = tkwidgets_gen_tcl_namespace((t_object*)x, s);
    x->receive_name = tkwidgets_gen_callback_name(x->tcl_namespace);
    pd_bind(&x->x_obj.ob_pd, x->receive_name);

    x->x_glist = canvas_getcurrent();
    x->iolets_tag = tkwidgets_gen_iolets_tag((t_object*)x);
    x->all_tag = tkwidgets_gen_all_tag((t_object*)x);

    x->x_data_outlet = outlet_new(&x->x_obj, &s_float);
    x->x_status_outlet = outlet_new(&x->x_obj, &s_anything);

    return (x);
}

void checkbutton_setup(void)
{
    checkbutton_class = class_new(gensym("checkbutton"), 
                                  (t_newmethod)checkbutton_new, 
                                  (t_method)checkbutton_free, 
                                  sizeof(t_checkbutton), 0, A_GIMME,0);
/* methods for atoms */
	class_addanything(checkbutton_class, (t_method)checkbutton_set_option);
	class_addbang(checkbutton_class, (t_method)checkbutton_bang_output);
	class_addfloat(checkbutton_class, (t_method)checkbutton_float_output);

/* common symbols to preload */
	id_symbol = gensym("id");
    query_callback_symbol = gensym("query_callback");
    size_symbol = gensym("size");
    
/* methods for pd space */
	class_addmethod(checkbutton_class, (t_method)checkbutton_options,
                    gensym("options"), 0);
    class_addmethod(checkbutton_class, (t_method)checkbutton_query,
                    gensym("query"), A_DEFSYMBOL, 0);
	class_addmethod(checkbutton_class, (t_method)checkbutton_set,
                    gensym("set"), A_DEFFLOAT, 0);
    class_addmethod(checkbutton_class, (t_method)checkbutton_size,
                    gensym("size"), A_DEFFLOAT, A_DEFFLOAT, 0);

/* callbacks */
	class_addmethod(checkbutton_class, (t_method)checkbutton_output_callback,
                    gensym("output"), A_DEFFLOAT, 0);
    class_addmethod(checkbutton_class, (t_method)checkbutton_query_callback,
                    query_callback_symbol, A_GIMME, 0);
    class_addmethod(checkbutton_class, (t_method)checkbutton_store_callback,
                    gensym("store_callback"), A_GIMME, 0);
    class_addmethod(checkbutton_class, (t_method)checkbutton_resize_click_callback,
                    gensym("resize_click"), A_FLOAT, 0);
    class_addmethod(checkbutton_class, (t_method)checkbutton_resize_motion_callback,
                    gensym("resize_motion"), A_FLOAT, A_FLOAT, 0);

/* widget behavior */
    checkbutton_widgetbehavior.w_getrectfn  = checkbutton_getrect;
    checkbutton_widgetbehavior.w_displacefn = checkbutton_displace;
    checkbutton_widgetbehavior.w_selectfn   = checkbutton_select;
    checkbutton_widgetbehavior.w_activatefn = NULL;
    checkbutton_widgetbehavior.w_deletefn   = checkbutton_delete;
    checkbutton_widgetbehavior.w_visfn      = checkbutton_vis;
    checkbutton_widgetbehavior.w_clickfn    = NULL;
    class_setwidget(checkbutton_class, &checkbutton_widgetbehavior);
    class_setsavefn(checkbutton_class, &checkbutton_save);
}


