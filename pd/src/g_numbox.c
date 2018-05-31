/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* my_numbox.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "g_all_guis.h"
#include <math.h>
#define IEM_GUI_COLOR_EDITED 0xff0000

#if PD_FLOATSIZE == 32
#define MY_NUMBOX_FLOAT_SPECIFIER "%.6g"
#elif PD_FLOATSIZE == 64
#define MY_NUMBOX_FLOAT_SPECIFIER "%.14lg"
#endif

extern int gfxstub_haveproperties(void *key);
static void my_numbox_draw_select(t_my_numbox *x, t_glist *glist);
static void my_numbox_key(void *z, t_floatarg fkey);
static void my_numbox_draw_update(t_gobj *client, t_glist *glist);
t_widgetbehavior my_numbox_widgetbehavior;
/*static*/ t_class *my_numbox_class;

static void my_numbox_tick_reset(t_my_numbox *x)
{
    //printf("tick_reset\n");
    if(x->x_gui.x_change && x->x_gui.x_glist)
    {
        //printf("    success\n");
        x->x_gui.x_change = 0;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
    }
}

static void my_numbox_tick_wait(t_my_numbox *x)
{
    //printf("tick_wait\n");
    sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
}

void my_numbox_clip(t_my_numbox *x)
{
    if(x->x_val < x->x_min)
        x->x_val = x->x_min;
    if(x->x_val > x->x_max)
        x->x_val = x->x_max;
}

int my_numbox_calc_fontwidth2(t_my_numbox *x, int w, int h, int fontsize)
{
    int f=31;
    if     (x->x_gui.x_font_style == 1) f = 27;
    else if(x->x_gui.x_font_style == 2) f = 25;
    return (fontsize * f * w) / 36 + (h / 2) + 4;
}

int my_numbox_calc_fontwidth(t_my_numbox *x)
{
    return my_numbox_calc_fontwidth2(x,x->x_gui.x_w,x->x_gui.x_h,
        x->x_gui.x_fontsize);
}

void my_numbox_ftoa(t_my_numbox *x)
{
    double f=x->x_val;
    int bufsize, is_exp=0, i, idecimal;

    sprintf(x->x_buf, MY_NUMBOX_FLOAT_SPECIFIER, f);
    bufsize = strlen(x->x_buf);
    if(bufsize >= 5)/* if it is in exponential mode */
    {
        i = bufsize - 4;
        if((x->x_buf[i] == 'e') || (x->x_buf[i] == 'E'))
            is_exp = 1;
    }
    if(bufsize > x->x_gui.x_w)/* if to reduce */
    {
        if(is_exp)
        {
            if(x->x_gui.x_w <= 5)
            {
                x->x_buf[0] = (f < 0.0 ? '-' : '+');
                x->x_buf[1] = 0;
            }
            i = bufsize - 4;
            for(idecimal=0; idecimal < i; idecimal++)
                if(x->x_buf[idecimal] == '.')
                    break;
            if(idecimal > (x->x_gui.x_w - 4))
            {
                x->x_buf[0] = (f < 0.0 ? '-' : '+');
                x->x_buf[1] = 0;
            }
            else
            {
                int new_exp_index=x->x_gui.x_w-4, old_exp_index=bufsize-4;

                for(i=0; i < 4; i++, new_exp_index++, old_exp_index++)
                    x->x_buf[new_exp_index] = x->x_buf[old_exp_index];
                x->x_buf[x->x_gui.x_w] = 0;
            }

        }
        else
        {
            for(idecimal=0; idecimal < bufsize; idecimal++)
                if(x->x_buf[idecimal] == '.')
                    break;
            if(idecimal > x->x_gui.x_w)
            {
                x->x_buf[0] = (f < 0.0 ? '-' : '+');
                x->x_buf[1] = 0;
            }
            else
                x->x_buf[x->x_gui.x_w] = 0;
        }
    }
}

static void my_numbox_draw_update(t_gobj *client, t_glist *glist)
{
    t_my_numbox *x = (t_my_numbox *)client;

    // we cannot ignore this call even if there is no change
    // since that will mess up number highlighting while editing
    // the code is left here as it is similar to other iemguis
    // but should not be used for this reason
    // if (!x->x_gui.x_changed) return;

    x->x_gui.x_changed = 0;
    if (!glist_isvisible(glist)) return;
    if(x->x_gui.x_change && x->x_buf[0])
    {
        //printf("draw_update 1\n");
        char *cp=x->x_buf;
        int sl = strlen(x->x_buf);
        x->x_buf[sl] = '>';
        x->x_buf[sl+1] = 0;
        if(sl >= x->x_gui.x_w)
            cp += sl - x->x_gui.x_w + 1;
        gui_vmess("gui_text_set", "xxs",
            glist_getcanvas(glist),
            x,
            cp);
        x->x_buf[sl] = 0;
    }
    else
    {
        my_numbox_ftoa(x); /* mmm... side-effects */
        gui_vmess("gui_text_set", "xxs",
            glist_getcanvas(glist),
            x,
            x->x_buf);
        x->x_buf[0] = 0; /* mmm... more side-effects... no clue why we'd need
                            to mutate a struct member in order to draw stuff */
    }
}

static void my_numbox_draw_new(t_my_numbox *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    char cbuf[8];
    sprintf(cbuf, "#%6.6x", x->x_gui.x_bcol);
    int half=x->x_gui.x_h/2, d=1+x->x_gui.x_h/34;
    int x1=text_xpix(&x->x_gui.x_obj, glist), x2=x1+x->x_numwidth;
    int y1=text_ypix(&x->x_gui.x_obj, glist), y2=y1+x->x_gui.x_h;

    gui_vmess("gui_numbox_new", "xxsiiiii",
        canvas,
        x,
        cbuf,
        x1,
        y1,
        x2 - x1,
        y2 - y1,
        glist_istoplevel(glist));
    /* Not sure when it is necessary to hide the frame... perhaps for
       k12? */
    if (!x->x_hide_frame || x->x_hide_frame == 2)
    {
        //sys_vgui(".x%lx.c create polyline %d %d %d %d %d %d -stroke #%6.6x "
        //    "-tags {%lxBASE2 x%lx text iemgui}\n",
        //    canvas, x1, y1, x1 + half, y1 + half, x1, y2,
        //    x->x_gui.x_fcol, x, x);
    }
    my_numbox_ftoa(x);
    sprintf(cbuf, "#%6.6x", x->x_gui.x_fcol);
    gui_vmess("gui_numbox_draw_text", "xxsisiiii",
        canvas,
        x,
        x->x_buf,
        x->x_gui.x_fontsize,
        cbuf,
        x1+half+2, y1+half+d, x1, y1);
}

/* Not sure that this is needed anymore */
static void my_numbox_draw_move(t_my_numbox *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    if (!glist_isvisible(canvas)) return;
    int half = x->x_gui.x_h/2, d=1+x->x_gui.x_h/34;
    int x1=text_xpix(&x->x_gui.x_obj, glist), x2=x1+x->x_numwidth;
    int y1=text_ypix(&x->x_gui.x_obj, glist), y2=y1+x->x_gui.x_h;

    iemgui_base_draw_move(&x->x_gui);

    if (x->x_hide_frame <= 1)
        iemgui_io_draw_move(&x->x_gui);
    if (!x->x_hide_frame || x->x_hide_frame == 2)
    {
        //sys_vgui(".x%lx.c coords %lxBASE2 %d %d %d %d %d %d\n",
        //    canvas, x, x1, y1, x1 + half, y1 + half, x1, y2);
    }

    gui_vmess("gui_numbox_coords", "xxii",
        canvas,
        x,
        x2 - x1,
        y2 - y1);

    gui_vmess("gui_numbox_update_text_position", "xxii",
        canvas,
        x,
        half + 2,
        half + d);
}

static void my_numbox_draw_config(t_my_numbox* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    char fg[8], bg[8];
    sprintf(fg, "#%6.6x",  x->x_gui.x_fcol);
    sprintf(bg, "#%6.6x",  x->x_gui.x_bcol);
    gui_vmess("gui_numbox_update", "xxsssii",
        canvas,
        x,
        fg,
        bg,
        iemgui_typeface((t_iemgui *)x),
        x->x_gui.x_fontsize,
        sys_fontweight);
}

static void my_numbox_draw_select(t_my_numbox *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int issel = x->x_gui.x_selected == canvas && x->x_gui.x_glist == canvas;
    if(x->x_gui.x_selected && x->x_gui.x_change)
    {
        x->x_gui.x_change = 0;
        clock_unset(x->x_clock_reset);
        x->x_buf[0] = 0;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
    }
    char fcol[8]; sprintf(fcol, "#%6.6x", x->x_gui.x_fcol);
    char bcol[8]; sprintf(bcol, "#%6.6x", x->x_gui.x_bcol);
    // The logic in these sys_vgui calls is being taken care
    // of in the gui now...
    //sys_vgui(".x%lx.c itemconfigure %lxBASE1 -stroke %s\n", canvas, x,
    //    issel ? selection_color : x->x_hide_frame <= 1 ? "$pd_colors(iemgui_border)" : bcol);
    //sys_vgui(".x%lx.c itemconfigure %lxBASE2 -stroke %s\n", canvas, x,
    //    issel ? selection_color : fcol);
    //sys_vgui(".x%lx.c itemconfigure %lxNUMBER -fill %s\n", canvas, x,
    //    issel ? selection_color : fcol);
    if(issel) 
        scalehandle_draw_select2(&x->x_gui);
    else
        scalehandle_draw_erase2(&x->x_gui);
}

static void my_numbox__clickhook(t_scalehandle *sh, int newstate)
{
    t_my_numbox *x = (t_my_numbox *)(sh->h_master);
    if (newstate)
    {
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
        if (!sh->h_scale)
            scalehandle_click_label(sh);
    }
    /* not sure if we need this */
    sh->h_dragon = newstate;
}

static void my_numbox__motionhook(t_scalehandle *sh,
    t_floatarg mouse_x, t_floatarg mouse_y)
{
    if (sh->h_scale)
    {
        t_my_numbox *x = (t_my_numbox *)(sh->h_master);
        //int dx = (int)mouse_x - sh->h_offset_x;
        int dy = (int)mouse_y - sh->h_offset_y;

        /* first calculate y */
        int newy = maxi(x->x_gui.x_obj.te_ypix + x->x_gui.x_h +
            dy, x->x_gui.x_obj.te_ypix + SCALE_NUM_MINHEIGHT);

        /* then readjust fontsize */
        x->x_tmpfontsize = maxi((newy - x->x_gui.x_obj.te_ypix) * 0.8,
            IEM_FONT_MINSIZE);

        int f = 31;
        if     (x->x_gui.x_font_style == 1) f = 27;
        else if(x->x_gui.x_font_style == 2) f = 25;
        int char_w = (x->x_tmpfontsize * f) / 36;

        /* get the new total width */
        int new_total_width = x->x_numwidth + (int)mouse_x -
            (text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist) + x->x_numwidth);

        /* now figure out what does this translate into in terms of
           character length */
        int new_char_len = (new_total_width -
            ((newy - x->x_gui.x_obj.te_ypix) / 2) - 4) / char_w;
        if (new_char_len < SCALE_NUM_MINWIDTH)
            new_char_len = SCALE_NUM_MINWIDTH;

        x->x_scalewidth = new_char_len;
        x->x_scaleheight = newy - x->x_gui.x_obj.te_ypix;

        int numwidth = my_numbox_calc_fontwidth2(x,new_char_len,
            x->x_scaleheight,x->x_tmpfontsize);
        sh->h_dragx = numwidth - x->x_numwidth;
        sh->h_dragy = dy;
        //printf("dx=%-4d dy=%-4d scalewidth=%-4d scaleheight=%-4d numwidth=%-4d dragx=%-4d\n",
        //    dx,dy,x->x_scalewidth,x->x_scaleheight,numwidth,sh->h_dragx);
        scalehandle_drag_scale(sh);

        x->x_gui.x_fontsize = x->x_tmpfontsize;
        x->x_gui.x_w = new_char_len;
        x->x_gui.x_h = x->x_scaleheight;
        x->x_numwidth = my_numbox_calc_fontwidth(x);
        canvas_dirty(x->x_gui.x_glist, 1);

        if (glist_isvisible(x->x_gui.x_glist))
        {
            my_numbox_draw_move(x, x->x_gui.x_glist);
            my_numbox_draw_config(x, x->x_gui.x_glist);
            my_numbox_draw_update((t_gobj*)x, x->x_gui.x_glist);
            /* These were just a crude way to guarantee correct
               redrawing when testing, but it looks like the calls
               above take care of it. */
            //gobj_vis(x, x->x_gui.x_glist, 0);
            //gobj_vis(x, x->x_gui.x_glist, 1);
            scalehandle_unclick_scale(sh);
        }
        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            properties_set_field_int(properties,"width",x->x_scalewidth);
            properties_set_field_int(properties,"height",x->x_scaleheight);
            properties_set_field_int(properties,"font_size",x->x_tmpfontsize);
        }
    }
    scalehandle_dragon_label(sh,mouse_x, mouse_y);
}

void my_numbox_draw(t_my_numbox *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)      sys_queuegui(x, glist, my_numbox_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)   my_numbox_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)    my_numbox_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT) my_numbox_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG) my_numbox_draw_config(x, glist);
}

/* ------------------------ nbx widgetbehaviour----------------------------- */


static void my_numbox_getrect(t_gobj *z, t_glist *glist,
                              int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_my_numbox* x = (t_my_numbox*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_numwidth;
    *yp2 = *yp1 + x->x_gui.x_h;
    iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);
}

static void my_numbox_save(t_gobj *z, t_binbuf *b)
{
    t_my_numbox *x = (t_my_numbox *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    if(x->x_gui.x_change)
    {
        x->x_gui.x_change = 0;
        clock_unset(x->x_clock_reset);
        x->x_gui.x_changed = 1;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
    }
    binbuf_addv(b, "ssiisiiffiisssiiiiiiifii;", gensym("#X"),gensym("obj"),
        (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
        gensym("nbx"), x->x_gui.x_w, x->x_gui.x_h,
        (t_float)x->x_min, (t_float)x->x_max,
        x->x_lin0_log1, iem_symargstoint(&x->x_gui),
        srl[0], srl[1], srl[2], x->x_gui.x_ldx, x->x_gui.x_ldy,
        iem_fstyletoint(&x->x_gui), x->x_gui.x_fontsize,
        bflcol[0], bflcol[1], bflcol[2],
        x->x_val, x->x_log_height, x->x_hide_frame);
}

int my_numbox_check_minmax(t_my_numbox *x, double min, double max)
{
    int ret=0;

    if(x->x_lin0_log1)
    {
        if((min == 0.0)&&(max == 0.0))
            max = 1.0;
        if(max > 0.0)
        {
            if(min <= 0.0)
                min = 0.01*max;
        }
        else
        {
            if(min > 0.0)
                max = 0.01*min;
        }
    }
    x->x_min = min;
    x->x_max = max;
    if(x->x_val < x->x_min)
    {
        x->x_val = x->x_min;
        ret = 1;
    }
    if(x->x_val > x->x_max)
    {
        x->x_val = x->x_max;
        ret = 1;
    }
    if(x->x_lin0_log1)
        x->x_k = exp(log(x->x_max/x->x_min)/(double)(x->x_log_height));
    else
        x->x_k = 1.0;
    return(ret);
}

static void my_numbox_properties(t_gobj *z, t_glist *owner)
{
    t_my_numbox *x = (t_my_numbox *)z;
    char buf[800], *gfx_tag;
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    if(x->x_gui.x_change)
    {
        x->x_gui.x_change = 0;
        clock_unset(x->x_clock_reset);
        x->x_gui.x_changed = 1;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);

    }
    sprintf(buf, "pdtk_iemgui_dialog %%s |nbx| \
        -------dimensions(digits)(pix):------- %d %d width: %d %d height: \
        -----------output-range:----------- %g min: %g max: %d \
        %d lin log %d %d log-height: %d {%s} {%s} {%s} %d %d %d %d %d %d %d\n",
        x->x_gui.x_w, 1, x->x_gui.x_h, 8, x->x_min, x->x_max,
        x->x_hide_frame, /*EXCEPTION: x_hide_frame instead of schedule*/
        x->x_lin0_log1, x->x_gui.x_loadinit, -1,
        x->x_log_height, /*no multi, but iem-characteristic*/
        srl[0]->s_name, srl[1]->s_name, srl[2]->s_name,
        x->x_gui.x_ldx, x->x_gui.x_ldy,
        x->x_gui.x_font_style, x->x_gui.x_fontsize,
        0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol,
        0xffffff & x->x_gui.x_lcol);
    //gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
    gfx_tag = gfxstub_new2(&x->x_gui.x_obj.ob_pd, x);

    gui_start_vmess("gui_iemgui_dialog", "s", gfx_tag);
    gui_start_array();
    gui_s("type"); gui_s("nbx");
    gui_s("width");  gui_i(x->x_gui.x_w);
    gui_s("height"); gui_i(x->x_gui.x_h);
    gui_s("minimum_range"); gui_f(x->x_min);
    gui_s("maximum_range"); gui_f(x->x_max);
    gui_s("log_scaling"); gui_i(x->x_lin0_log1);
    gui_s("log_height"); gui_i(x->x_log_height);
    gui_s("init"); gui_i(x->x_gui.x_loadinit);
    gui_s("send_symbol");      gui_s(srl[0]->s_name);
    gui_s("receive_symbol");   gui_s(srl[1]->s_name);
    gui_s("label");            gui_s(srl[2]->s_name);
    gui_s("x_offset");         gui_i(x->x_gui.x_ldx);
    gui_s("y_offset");         gui_i(x->x_gui.x_ldy);
    gui_s("font_style");       gui_i(x->x_gui.x_font_style);
    gui_s("font_size");        gui_i(x->x_gui.x_fontsize);
    gui_s("background_color"); gui_i(0xffffff & x->x_gui.x_bcol);
    gui_s("foreground_color"); gui_i(0xffffff & x->x_gui.x_fcol);
    gui_s("label_color");      gui_i(0xffffff & x->x_gui.x_lcol);
    gui_s("hide_frame");       gui_i(x->x_hide_frame);
    gui_end_array();
    gui_end_vmess();
}

static void my_numbox_bang(t_my_numbox *x)
{
    iemgui_out_float(&x->x_gui, 0, 0, x->x_val);
}

static void my_numbox_dialog(t_my_numbox *x, t_symbol *s, int argc,
    t_atom *argv)
{
    if (atom_getintarg(19, argc, argv))
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
    x->x_gui.x_w = maxi(atom_getintarg(0, argc, argv),1);
    x->x_gui.x_h = maxi(atom_getintarg(1, argc, argv),8);
    double min = atom_getfloatarg(2, argc, argv);
    double max = atom_getfloatarg(3, argc, argv);
    x->x_lin0_log1 = !!atom_getintarg(4, argc, argv);
    x->x_log_height = maxi(atom_getintarg(6, argc, argv),10);
    if (argc > 17)
        x->x_hide_frame = (int)atom_getintarg(18, argc, argv);
    iemgui_dialog(&x->x_gui, argc, argv);
    x->x_numwidth = my_numbox_calc_fontwidth(x);

    my_numbox_check_minmax(x, min, max);
    // normally, you'd do move+config, but here you have to do erase+new
    // because iemgui_draw_io does not support changes to x_hide_frame.
    iemgui_draw_erase(&x->x_gui);
    iemgui_draw_new(&x->x_gui);
    //iemgui_draw_move(&x->x_gui);
    //iemgui_draw_config(&x->x_gui);
    scalehandle_draw(&x->x_gui);
    if (x->x_gui.x_selected)
        iemgui_select((t_gobj *)x,x->x_gui.x_glist,1);
    //canvas_restore_original_position(x->x_gui.x_glist, (t_gobj *)x,"bogus",-1);
    scrollbar_update(x->x_gui.x_glist);
}

static void my_numbox_motion(t_my_numbox *x, t_floatarg dx, t_floatarg dy)
{
    double k2=1.0;
    int old = x->x_val;

    if(x->x_gui.x_finemoved)
        k2 = 0.01;
    if(x->x_lin0_log1)
        x->x_val *= pow(x->x_k, -k2*dy);
    else
        x->x_val -= k2*dy;
    my_numbox_clip(x);
    if (old != x->x_val)
    {
        x->x_gui.x_changed = 1;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
        my_numbox_bang(x);
    }
    clock_unset(x->x_clock_reset);
}

static void my_numbox_click(t_my_numbox *x, t_floatarg xpos, t_floatarg ypos,
                            t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
        (t_glistmotionfn)my_numbox_motion, my_numbox_key, xpos, ypos);
}

static int my_numbox_newclick(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_my_numbox* x = (t_my_numbox *)z;

    if(doit)
    {
        //printf("newclick doit\n");
        my_numbox_click( x, (t_floatarg)xpix, (t_floatarg)ypix,
            (t_floatarg)shift, 0, (t_floatarg)alt);
        if(shift)
            x->x_gui.x_finemoved = 1;
        else
            x->x_gui.x_finemoved = 0;
        if(!x->x_gui.x_change)
        {
            //printf("    change=0\n");
            clock_delay(x->x_clock_wait, 50);
            x->x_gui.x_change = 1;
            clock_delay(x->x_clock_reset, 3000);

            x->x_buf[0] = 0;
        }
        else
        {
            //printf("    change=1\n");
            x->x_gui.x_change = 0;
            clock_unset(x->x_clock_reset);
            x->x_buf[0] = 0;
            x->x_gui.x_changed = 1;
            sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
        }
    }
    return (1);
}

static void my_numbox_set(t_my_numbox *x, t_floatarg f)
{
    if (x->x_val != f)
    {
        x->x_val = f;
        my_numbox_clip(x);
        x->x_gui.x_changed = 1;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
    }
}

static void my_numbox_log_height(t_my_numbox *x, t_floatarg lh)
{
    if(lh < 10.0)
        lh = 10.0;
    x->x_log_height = (int)lh;
    if(x->x_lin0_log1)
        x->x_k = exp(log(x->x_max/x->x_min)/(double)(x->x_log_height));
    else
        x->x_k = 1.0;
}

static void my_numbox_hide_frame(t_my_numbox *x, t_floatarg lh)
{
    if(lh < 0.0)
        lh = 0.0;
    if (lh > 3.0)
        lh = 3.0;
    x->x_hide_frame = (int)lh;
    my_numbox_draw(x, x->x_gui.x_glist, 4);
    my_numbox_draw(x, x->x_gui.x_glist, 2);  
}

static void my_numbox_float(t_my_numbox *x, t_floatarg f)
{
    my_numbox_set(x, f);
    if(x->x_gui.x_put_in2out)
        my_numbox_bang(x);
}

static void my_numbox_size(t_my_numbox *x, t_symbol *s, int ac, t_atom *av)
{
    int h, w;

    w = (int)atom_getintarg(0, ac, av);
    if(w < 1)
        w = 1;
    x->x_gui.x_w = w;
    if(ac > 1)
    {
        h = (int)atom_getintarg(1, ac, av);
        if(h < 8)
            h = 8;
        x->x_gui.x_h = h;
    }
    x->x_numwidth = my_numbox_calc_fontwidth(x);
    iemgui_size(&x->x_gui);
}

static void my_numbox_range(t_my_numbox *x, t_symbol *s, int ac, t_atom *av)
{
    if(my_numbox_check_minmax(x, (double)atom_getfloatarg(0, ac, av),
                              (double)atom_getfloatarg(1, ac, av)))
    {
        x->x_gui.x_changed = 1;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
        /*my_numbox_bang(x);*/
    }
}

static void my_numbox_log(t_my_numbox *x)
{
    x->x_lin0_log1 = 1;
    if(my_numbox_check_minmax(x, x->x_min, x->x_max))
    {
        x->x_gui.x_changed = 1;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
        /*my_numbox_bang(x);*/
    }
}

static void my_numbox_lin(t_my_numbox *x)
{
    x->x_lin0_log1 = 0;
}

static void my_numbox_loadbang(t_my_numbox *x, t_floatarg action)
{
    if (action == LB_LOAD && x->x_gui.x_loadinit)
    {
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
        my_numbox_bang(x);
    }
}

static void my_numbox_key(void *z, t_floatarg fkey)
{
    t_my_numbox *x = z;
    char c=fkey;
    char buf[3];
    buf[1] = 0;

    if (c == 0)
    {
        x->x_gui.x_change = 0;
        clock_unset(x->x_clock_reset);
        x->x_gui.x_changed = 1;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
        return;
    }
    if(((c>='0')&&(c<='9'))||(c=='.')||(c=='-')||
        (c=='e')||(c=='+')||(c=='E'))
    {
        if(strlen(x->x_buf) < (IEMGUI_MAX_NUM_LEN-2))
        {
            buf[0] = c;
            strcat(x->x_buf, buf);
            x->x_gui.x_changed = 1;
            sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
        }
    }
    else if((c=='\b')||(c==127))
    {
        int sl=strlen(x->x_buf)-1;

        if(sl < 0)
            sl = 0;
        x->x_buf[sl] = 0;
        x->x_gui.x_changed = 1;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
    }
    else if((c=='\n')||(c==13))
    {
        x->x_val = atof(x->x_buf);
        x->x_buf[0] = 0;
        x->x_gui.x_change = 0;
        clock_unset(x->x_clock_reset);
        my_numbox_clip(x);
        my_numbox_bang(x);
        x->x_gui.x_changed = 1;
        sys_queuegui(x, x->x_gui.x_glist, my_numbox_draw_update);
    }
    clock_delay(x->x_clock_reset, 3000);
}

static void my_numbox_list(t_my_numbox *x, t_symbol *s, int ac, t_atom *av)
{
    if (IS_A_FLOAT(av,0))
    {
        my_numbox_set(x, atom_getfloatarg(0, ac, av));
        my_numbox_bang(x);
    }
}

static void *my_numbox_new(t_symbol *s, int argc, t_atom *argv)
{
    t_my_numbox *x = (t_my_numbox *)pd_new(my_numbox_class);
    int bflcol[]={-262144, -1, -1};
    int w=5, h=14;
    int lilo=0, ldx=0, ldy=-8;
    int fs=10;
    int log_height=256;
    double min=-1.0e+37, max=1.0e+37,v=0.0;

    if((argc >= 17)&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
       &&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,3)
       &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&(IS_A_SYMBOL(argv,7)||IS_A_FLOAT(argv,7))
       &&(IS_A_SYMBOL(argv,8)||IS_A_FLOAT(argv,8))
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)
       &&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13)
       &&IS_A_FLOAT(argv,14)&&IS_A_FLOAT(argv,15)&&IS_A_FLOAT(argv,16))
    {
        w = maxi(atom_getintarg(0, argc, argv),1);
        h = maxi(atom_getintarg(1, argc, argv),8);
        min = atom_getfloatarg(2, argc, argv);
        max = atom_getfloatarg(3, argc, argv);
        lilo = !!atom_getintarg(4, argc, argv);
        iem_inttosymargs(&x->x_gui, atom_getintarg(5, argc, argv));
        iemgui_new_getnames(&x->x_gui, 6, argv);
        ldx = atom_getintarg(9, argc, argv);
        ldy = atom_getintarg(10, argc, argv);
        iem_inttofstyle(&x->x_gui, atom_getintarg(11, argc, argv));
        fs = maxi(atom_getintarg(12, argc, argv),4);
        bflcol[0] = atom_getintarg(13, argc, argv);
        bflcol[1] = atom_getintarg(14, argc, argv);
        bflcol[2] = atom_getintarg(15, argc, argv);
        v = atom_getfloatarg(16, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 6, 0);
    if((argc == 18)&&IS_A_FLOAT(argv,17))
        log_height = maxi(atom_getintarg(17, argc, argv),10);
    x->x_hide_frame = 0; // default behavior
    if((argc == 19)&&IS_A_FLOAT(argv,18))
        x->x_hide_frame = (int)atom_getintarg(18, argc, argv);
    x->x_gui.x_draw = (t_iemfunptr)my_numbox_draw;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    x->x_val = x->x_gui.x_loadinit ? v : 0.0;
    x->x_lin0_log1 = lilo;
    x->x_log_height = log_height;
    if (x->x_gui.x_font_style<0 || x->x_gui.x_font_style>2) x->x_gui.x_font_style=0;
    if (iemgui_has_rcv(&x->x_gui))
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = w;
    x->x_gui.x_h = h;
    x->x_buf[0] = 0;
    x->x_numwidth = my_numbox_calc_fontwidth(x);
    my_numbox_check_minmax(x, min, max);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    x->x_clock_reset = clock_new(x, (t_method)my_numbox_tick_reset);
    x->x_clock_wait = clock_new(x, (t_method)my_numbox_tick_wait);
    x->x_gui.x_change = 0;
    outlet_new(&x->x_gui.x_obj, &s_float);

    x->x_gui.x_handle = scalehandle_new((t_object *)x,x->x_gui.x_glist,1,my_numbox__clickhook,my_numbox__motionhook);
    x->x_gui.x_lhandle = scalehandle_new((t_object *)x,x->x_gui.x_glist,0,my_numbox__clickhook,my_numbox__motionhook);
    x->x_scalewidth = 0;
    x->x_scaleheight = 0;
    x->x_tmpfontsize = 0;
    x->x_gui.x_obj.te_iemgui = 1;
    x->x_gui.x_changed = 0;

    x->x_gui.legacy_x = 0;
    x->x_gui.legacy_y = 1;

    return (x);
}

static void my_numbox_free(t_my_numbox *x)
{
    if(iemgui_has_rcv(&x->x_gui))
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    clock_free(x->x_clock_reset);
    clock_free(x->x_clock_wait);
    gfxstub_deleteforkey(x);

    if (x->x_gui. x_handle) scalehandle_free(x->x_gui. x_handle);
    if (x->x_gui.x_lhandle) scalehandle_free(x->x_gui.x_lhandle);
}

void g_numbox_setup(void)
{
    my_numbox_class = class_new(gensym("nbx"), (t_newmethod)my_numbox_new,
        (t_method)my_numbox_free, sizeof(t_my_numbox), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)my_numbox_new, gensym("my_numbox"),
        A_GIMME, 0);
    class_addbang(my_numbox_class,my_numbox_bang);
    class_addfloat(my_numbox_class,my_numbox_float);
    class_addlist(my_numbox_class, my_numbox_list);
    class_addmethod(my_numbox_class, (t_method)my_numbox_click,
        gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_motion,
        gensym("motion"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_dialog,
        gensym("dialog"), A_GIMME, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_loadbang,
        gensym("loadbang"), A_DEFFLOAT, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_set,
        gensym("set"), A_FLOAT, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_size,
        gensym("size"), A_GIMME, 0);
    iemgui_class_addmethods(my_numbox_class);
    class_addmethod(my_numbox_class, (t_method)my_numbox_range,
        gensym("range"), A_GIMME, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_log,
        gensym("log"), 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_lin,
        gensym("lin"), 0);
    class_addmethod(my_numbox_class, (t_method)iemgui_init,
        gensym("init"), A_FLOAT, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_log_height,
        gensym("log_height"), A_FLOAT, 0);
    class_addmethod(my_numbox_class, (t_method)my_numbox_hide_frame,
        gensym("hide_frame"), A_FLOAT, 0);

    wb_init(&my_numbox_widgetbehavior,my_numbox_getrect,my_numbox_newclick);
    class_setwidget(my_numbox_class, &my_numbox_widgetbehavior);
    class_sethelpsymbol(my_numbox_class, gensym("numbox2"));
    class_setsavefn(my_numbox_class, my_numbox_save);
    class_setpropertiesfn(my_numbox_class, my_numbox_properties);
}
