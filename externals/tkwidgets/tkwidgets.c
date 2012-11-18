/* 
    Copyright (C) 2007 Free Software Foundation
    written by Hans-Christoph Steiner <hans@at.or.at>

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

    This is the shared library for the tkwidgets library for Pd.

*/

/* TODO apply store logic to query so query doesn't send blank options */

#include "tkwidgets.h"
#include <stdio.h>
#include <string.h>

#define DEBUG(x)
//#define DEBUG(x) x

/* -------------------- options handling ------------------------------------ */

void tkwidgets_query_options(t_symbol *receive_name, t_symbol *widget_id, 
                             int argc, char** argv)
{
    int i;
    for(i = 0; i < argc; i++)
        sys_vgui("pd [concat %s query_callback %s [%s cget -%s] \\;]\n",
                 receive_name->s_name, argv[i], widget_id->s_name, argv[i]);
}

/* this queries the widget for each option listed in the tk_options struct,
 * builts a list in Tcl-space, then send that list to the store_callback */
void tkwidgets_store_options(t_symbol *receive_name, t_symbol *tcl_namespace,
                             t_symbol *widget_id, int argc, char **argv)
{
    int i;
    for(i = 0; i < argc; i++)
    {
        sys_vgui("set ::%s::tmp [%s cget -%s]\n",
                 tcl_namespace->s_name, widget_id->s_name, argv[i]);
        sys_vgui("if {[string length $::%s::tmp] > 0} {\n",
                 tcl_namespace->s_name);
        sys_vgui("lappend ::%s::list -%s; lappend ::%s::list $::%s::tmp}\n", 
                 tcl_namespace->s_name, argv[i], 
                 tcl_namespace->s_name, tcl_namespace->s_name);
    }
    sys_vgui("pd [concat %s store_callback $::%s::list \\;]\n",
             receive_name->s_name, tcl_namespace->s_name);
    sys_vgui("unset ::%s::list \n", tcl_namespace->s_name);  
}

void tkwidgets_restore_options(t_symbol *widget_id, t_binbuf *options_binbuf)
{
    int length;
    char *options;
    binbuf_gettext(options_binbuf, &options, &length);
    options[length] = 0; //binbuf_gettext() doesn't put a null, so we do
    sys_vgui("%s configure %s\n", widget_id->s_name, options);
}

/* output a list of available options for this widget */ 
void tkwidgets_list_options(t_outlet *status_outlet, int argc, char** argv)
{
    int i;
    t_binbuf *bb = binbuf_new();
    for(i = 0; i < argc; ++i)
    {
        binbuf_addv(bb, "s", gensym(argv[i]));
    }
    outlet_anything(status_outlet, gensym("options"), 
                    binbuf_getnatom(bb), binbuf_getvec(bb));
}


/* -------------------- generate names for various elements ----------------- */

t_symbol* tkwidgets_gen_tcl_namespace(t_object* x, t_symbol* widget_name)
{
    char buf[MAXPDSTRING];
    sprintf(buf,"%s%lx", widget_name->s_name, (long unsigned int)x);
    return gensym(buf);
}

t_symbol* tkwidgets_gen_callback_name(t_symbol* tcl_namespace)
{
    char buf[MAXPDSTRING];
    sprintf(buf,"#%s", tcl_namespace->s_name);
    return gensym(buf);
}

t_symbol* tkwidgets_gen_canvas_id(t_canvas* canvas)
{
    char buf[MAXPDSTRING];
    sprintf(buf,".x%lx.c", (long unsigned int) canvas);
    return gensym(buf);
}

t_symbol* tkwidgets_gen_frame_id(t_object* x, t_symbol* canvas_id)
{
    char buf[MAXPDSTRING];
    sprintf(buf,"%s.frame%lx", canvas_id->s_name, (long unsigned int)x);
    return gensym(buf);    
}

t_symbol* tkwidgets_gen_widget_id(t_object* x, t_symbol* parent_id)
{
    char buf[MAXPDSTRING];
    sprintf(buf,"%s.widget%lx", parent_id->s_name, (long unsigned int)x);
    return gensym(buf);
}

t_symbol* tkwidgets_gen_handle_id(t_object *x, t_symbol* canvas_id)
{
    char buf[MAXPDSTRING];
    sprintf(buf,"%s.handle%lx", canvas_id->s_name, (long unsigned int)x);
    return gensym(buf);
}

t_symbol* tkwidgets_gen_scrollbar_id(t_object *x, t_symbol* frame_id)
{
    char buf[MAXPDSTRING];
    sprintf(buf,"%s.scrollbar%lx", frame_id->s_name, (long unsigned int)x);
    return gensym(buf);
}

t_symbol* tkwidgets_gen_window_tag(t_object* x, t_symbol* canvas_id)
{
    char buf[MAXPDSTRING];
    sprintf(buf,"%s.window%lx", canvas_id->s_name, (long unsigned int)x);
    return gensym(buf);
}

t_symbol* tkwidgets_gen_iolets_tag(t_object* x)
{
    char buf[MAXPDSTRING];
    sprintf(buf,"iolets%lx", (long unsigned int)x);
    return gensym(buf);
}

t_symbol* tkwidgets_gen_all_tag(t_object *x)
{
    char buf[MAXPDSTRING];
    sprintf(buf,"all%lx", (long unsigned int)x);
    return gensym(buf);
}

/* -------------------- inlets/outlets -------------------------------------- */
 
static int calculate_onset(int x_location, int width,
                           int current_iolet, int total_iolets)
{
    DEBUG(post("calculate_onset"););
    return(x_location + (width - IOWIDTH)                               \
           * current_iolet / (total_iolets == 1 ? 1 : total_iolets - 1));
}

/* standard method for drawing inlets and outlets.  Currently, the number of
 * inlets and outlets is set in tkwidgets.h since I think all of the
 * objectclasses will have the same ones.  If that needs to change, then this
 * function can use obj_ninlets() and obj_noutlets() */
void tkwidgets_draw_iolets(t_object *x, t_glist *glist, t_symbol *canvas_id,
                           t_symbol *iolets_tag, t_symbol *all_tag,
                           int width, int height)
{
    int i, onset;
    int x_location = text_xpix(x, glist);
    int y_location = text_ypix(x, glist);

/* TODO: make inlets draw on top of widget */
    for (i = 0; i < TOTAL_INLETS; i++)  /* inlets */
    {
        onset = calculate_onset(x_location, width, i, TOTAL_INLETS);
        sys_vgui("%s create rectangle %d %d %d %d -tags {%s %s}\n",
                 canvas_id->s_name, onset, y_location - 2,
                 onset + IOWIDTH, y_location,
                 iolets_tag->s_name, all_tag->s_name);
        sys_vgui("%s raise %s\n", canvas_id->s_name, iolets_tag->s_name);
    }
    for (i = 0; i < TOTAL_OUTLETS; i++) /* outlets */
    {
        onset = calculate_onset(x_location, width, i, TOTAL_OUTLETS);
        sys_vgui("%s create rectangle %d %d %d %d -tags {%s %s}\n",
                 canvas_id->s_name, onset, y_location + height,
                 onset + IOWIDTH, y_location + height + 2,
                 iolets_tag->s_name, all_tag->s_name);
        sys_vgui("%s raise %s\n", canvas_id->s_name, iolets_tag->s_name);
    }
}

void tkwidgets_erase_iolets(t_symbol* canvas_id, t_symbol* iolets_tag)
{
    sys_vgui("%s delete %s\n", canvas_id->s_name, iolets_tag->s_name); 
}

/* -------------------- scrollbars ------------------------------------------ */

void tkwidgets_draw_y_scrollbar(t_symbol *widget_id, t_symbol *scrollbar_id)
{
    sys_vgui("scrollbar %s -orient vertical -command {%s yview}\n",
             scrollbar_id->s_name, widget_id->s_name);
    sys_vgui("pack %s -side right -fill y -before %s \n",
             scrollbar_id->s_name, widget_id->s_name);
    sys_vgui("%s configure -yscrollcommand {%s set}\n",
             widget_id->s_name, scrollbar_id->s_name);
}

void tkwidgets_erase_y_scrollbar(t_symbol *widget_id, t_symbol *scrollbar_id)
{
    sys_vgui("%s configure -yscrollcommand {}\n", widget_id->s_name);
    sys_vgui("pack forget %s \n", scrollbar_id->s_name);
    sys_vgui("destroy %s \n", scrollbar_id->s_name);
}

/* -------------------- bind to keys and mouse events ----------------------- */

void tkwidgets_bind_key_events(t_symbol *canvas_id, t_symbol *widget_id)
{
#ifdef __APPLE__
    sys_vgui("bind %s <Mod1-Key> {pdtk_canvas_ctrlkey %s %%K 0}\n",
             widget_id->s_name, canvas_id->s_name);
    sys_vgui("bind %s <Mod1-Shift-Key> {pdtk_canvas_ctrlkey %s %%K 1}\n",
             widget_id->s_name, canvas_id->s_name);
#else
    sys_vgui("bind %s <Control-Key> {pdtk_canvas_ctrlkey %s %%K 0}\n",
             widget_id->s_name, canvas_id->s_name);
    sys_vgui("bind %s <Control-Shift-Key> {pdtk_canvas_ctrlkey %s %%K 1}\n",
             widget_id->s_name, canvas_id->s_name);
#endif
}

void tkwidgets_bind_mouse_events(t_symbol *canvas_id, t_symbol *widget_id)
{
    /* mouse buttons */
    sys_vgui("bind %s <Button> {pdtk_canvas_sendclick %s \
[expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b 0}\n",
             widget_id->s_name, canvas_id->s_name, 
             canvas_id->s_name, canvas_id->s_name);
    sys_vgui("bind %s <ButtonRelease> {pdtk_canvas_mouseup %s \
[expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b}\n",
             widget_id->s_name, canvas_id->s_name, 
             canvas_id->s_name, canvas_id->s_name);
    sys_vgui("bind %s <Shift-Button> {pdtk_canvas_click %s \
[expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b 1}\n",
             widget_id->s_name, canvas_id->s_name, 
             canvas_id->s_name, canvas_id->s_name);
    sys_vgui("bind %s <Button-2> {pdtk_canvas_rightclick %s \
[expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b}\n",
             widget_id->s_name, canvas_id->s_name, 
             canvas_id->s_name, canvas_id->s_name);
    sys_vgui("bind %s <Button-3> {pdtk_canvas_rightclick %s \
[expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b}\n",
             widget_id->s_name, canvas_id->s_name, 
             canvas_id->s_name, canvas_id->s_name);
    sys_vgui("bind %s <Control-Button> {pdtk_canvas_rightclick %s \
[expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b}\n",
             widget_id->s_name, canvas_id->s_name, 
             canvas_id->s_name, canvas_id->s_name);
    /* mouse motion */
    sys_vgui("bind %s <Motion> {pdtk_canvas_motion %s \
[expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] 0}\n",
             widget_id->s_name, canvas_id->s_name, 
             canvas_id->s_name, canvas_id->s_name);
}

/* -------------------- gui elements for resizing --------------------------- */

void tkwidgets_draw_handle()
{
    // TODO draw resize handle when selected in editmode
}

void tkwidgets_draw_resize_window()
{
    // TODO draw the resize window while resizing
}
