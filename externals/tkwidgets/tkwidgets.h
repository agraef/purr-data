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

#ifndef __TKWIDGETS_H
#define __TKWIDGETS_H

#include "m_pd.h"
#include "m_imp.h"
#include "g_canvas.h"

/* I don't know what these do, but they seem to be everywhere */
#ifdef _MSC_VER
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

/* so far, all of the tkwidgets objects have the same inlets/outlets */
#define TOTAL_INLETS            1
#define TOTAL_OUTLETS           2

#define TKW_SELECTION_COLOR     "#bdbddd"

/* size and offset for the resizing handle */
#define TKW_HANDLE_HEIGHT       15
#define TKW_HANDLE_WIDTH        15
#define TKW_HANDLE_INSET        -2

/* sketch for a common struct */
typedef struct _tkwidgets
{
    t_canvas* canvas;        /* canvas/glist this widget is currently drawn in*/
    t_glist*  glist;         /* glist that owns this widget */
    t_binbuf* options_binbuf;/* binbuf to save options state in */
    t_symbol* receive_name;  /* name to bind to, to receive callbacks */
    t_symbol* tcl_namespace; /* namespace to prevent name collisions */
    t_symbol* canvas_id;     /* the canvas that is showing this widget */
    t_symbol* frame_id;      /* the frame around the widget and supporters */
    t_symbol* window_tag;     /* the window that contains the widget */
    t_symbol* widget_id;     /* the core widget */
    t_symbol* handle_id;     /* the resizing handle */
    t_symbol* all_tag;       /* the tag for moving/deleting everything */
    int       resizing;      /* flag to tell when being resized */
    int       selected;      /* flag for when widget is selected */
} t_tkwidgets;   

/* handle options */
void tkwidgets_store_options(t_symbol *receive_name, t_symbol *tcl_namespace,
                             t_symbol *widget_id, int argc, char **argv);
void tkwidgets_restore_options(t_symbol *widget_id, t_binbuf *options_binbuf);
void tkwidgets_query_options(t_symbol *receive_name, t_symbol *widget_id, 
                             int argc, char** argv);
void tkwidgets_list_options(t_outlet *status_outlet, int argc, char** argv);

/* generate ID and tag names for using in Tcl/Tk space */
t_symbol* tkwidgets_gen_tcl_namespace(t_object* x, t_symbol* widget_name);
t_symbol* tkwidgets_gen_callback_name(t_symbol* tcl_namespace);
t_symbol* tkwidgets_gen_canvas_id(t_canvas* canvas);
t_symbol* tkwidgets_gen_frame_id(t_object* x, t_symbol* canvas_id);
t_symbol* tkwidgets_gen_widget_id(t_object* x, t_symbol* parent_id);
t_symbol* tkwidgets_gen_handle_id(t_object *x, t_symbol* canvas_id);
t_symbol* tkwidgets_gen_scrollbar_id(t_object *x, t_symbol* frame_id);
t_symbol* tkwidgets_gen_window_tag(t_object* x, t_symbol* canvas_id);
t_symbol* tkwidgets_gen_iolets_tag(t_object* x);
t_symbol* tkwidgets_gen_all_tag(t_object *x);


// TODO perhaps I should try to use glist_drawiofor() from g_text.c
void tkwidgets_draw_iolets(t_object *x, t_glist *glist, t_symbol *canvas_id,
                           t_symbol *iolets_tag, t_symbol *all_tag,
                           int width, int height);
void tkwidgets_erase_iolets(t_symbol* canvas_id, t_symbol* iolets_tag);
void tkwidgets_draw_y_scrollbar(t_symbol *widget_id, t_symbol *scrollbar_id);
void tkwidgets_erase_y_scrollbar(t_symbol *widget_id, t_symbol *scrollbar_id);

void tkwidgets_draw_handle(); // TODO draw resize handle when selected in editmode
void tkwidgets_draw_resize_window(); // TODO draw the resize window while resizing

/* selection */



/* bind this widget to Cmd/Ctrl keys and mouse events to support things like
 * then standard keys and right-click to bring up the Properties/Open/Help
 * menu when the Tk widgets have focus */
void tkwidgets_bind_key_events(t_symbol *canvas_id, t_symbol *widget_id);
void tkwidgets_bind_mouse_events(t_symbol *canvas_id, t_symbol *widget_id);



#endif /* NOT __TK_WIDGETS_H */
