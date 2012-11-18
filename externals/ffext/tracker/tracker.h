/* Copyright (c) 2007 Federico Ferri
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifndef __G_GRID_H
#define __G_GRID_H

#include <ctype.h>
#include "m_pd.h"
#include "m_imp.h"
#include "g_canvas.h"
#include "s_stuff.h"

#include <unistd.h>
#include <stdio.h>

#define MAX_BUF 10

typedef struct _tracker
{
    t_object x_obj;
    t_glist *x_glist;
    t_int x_selected;      // selected flag (in edit mode)
    t_int x_columns;       // # of cols
    t_int x_rows;          // # of rows
    t_int x_active_row;    // selected item row
    t_int x_active_column; // selected item col
    t_int x_ndigits;       // cell width (+1 of margin added automatically)
    t_int x_cursor_pos;    // visual cursor position (row indicator)
    t_int b_cursor;        // track cursor?
    t_int b_save_data;     // save data?
    char x_buf[MAX_BUF+1]; // edit buffer
    t_int x_cell_width;    // cell width in pixels  (auto computed)
    t_int x_cell_height;   // cell height in pixels (auto computed)
    t_float** x_data;      // data (2d matrix)
    t_outlet* outlet0;     // outlet
    t_symbol* s_send;
    t_symbol* s_recv;
} t_tracker;

static void tracker_draw_new(t_tracker* x, t_glist* glist);
static void tracker_draw_update(t_tracker* x);
static void tracker_draw_move(t_tracker* x, t_glist* glist);
static void tracker_draw_erase(t_tracker* x, t_glist* glist);
static void tracker_draw_select(t_tracker* x, t_glist* glist);
static void tracker_getrect(t_gobj* z, t_glist* owner, int* xp1, int* yp1, int* xp2, int* yp2);
static void tracker_save(t_gobj* z, t_binbuf* b);
static void tracker_properties(t_gobj* z, t_glist* owner);
static void tracker_dialog(t_tracker* x, t_symbol* s, int argc, t_atom* argv);
static void tracker_select(t_gobj* z, t_glist* glist, int selected);
static void tracker_vis(t_gobj* z, t_glist* glist, int vis);
static void tracker_delete(t_gobj* z, t_glist* glist);
static void tracker_displace(t_gobj* z, t_glist* glist, int dx, int dy);
static void tracker_resize(t_tracker* x, int newrows, int newcols);
static void tracker_free(t_tracker* x);
static void tracker_getrow(t_tracker* x, t_float row);
static void tracker_setrow(t_tracker* x, t_symbol* s, int argc, t_atom* argv);
static void tracker_getitem(t_tracker* x, t_float row, t_float col);
static void tracker_setitem(t_tracker* x, t_float row, t_float col, t_float val);
static void tracker_motion(t_tracker* x, t_floatarg dx, t_floatarg dy);
static void tracker_select_item(t_tracker* x, int r, int c);
static void tracker_select_movecur(t_tracker* x, int dx, int dy);
static void tracker_changenotify(t_tracker* x, t_floatarg row, t_floatarg col);
void tracker_setup(void);

#endif // __G_GRID_H
