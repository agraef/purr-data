// g_slider.c
// written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 as hdial/vdial.
// thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja.
// Copyright (c) 2014 by Mathieu Bouchard. (rewrite)
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt", in this distribution.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "g_all_guis.h"
#include <math.h>

static t_class *hscalehandle_class;
static t_class *vscalehandle_class;
extern int gfxstub_haveproperties(void *key);
static void hslider_draw_select(t_slider *x, t_glist *glist);
static void vslider_draw_select(t_slider *x, t_glist *glist);
t_widgetbehavior vslider_widgetbehavior;
t_widgetbehavior hslider_widgetbehavior;
t_class *hslider_class;
t_class *vslider_class;

static void slider_draw_update(t_gobj *client, t_glist *glist)
{
    t_slider *x = (t_slider *)client;
    if (!x->x_gui.x_changed) return;
    x->x_gui.x_changed = 0;
    if (!glist_isvisible(glist)) return;
    t_canvas *canvas=glist_getcanvas(glist);
    int x1=text_xpix(&x->x_gui.x_obj, glist), x2=x1+x->x_gui.x_w;
    int y1=text_ypix(&x->x_gui.x_obj, glist), y2=y1+x->x_gui.x_h;
    if (x->x_orient) y2+=5; else x2+=5;
    int r;
    if (x->x_orient) {
        r=y2-3 - (x->x_val+50)/100;
        sys_vgui(".x%lx.c coords %lxKNOB %d %d %d %d\n",
            canvas, x, x1+2, r, x2-2, r);
    } else {
        r=x1+3 + (x->x_val+50)/100;
        sys_vgui(".x%lx.c coords %lxKNOB %d %d %d %d\n",
            canvas, x, r, y1+2, r, y2-2);
    }
    int t = x->x_thick;
    x->x_thick = x->x_val == x->x_center;
    if (t!=x->x_thick)
        sys_vgui(".x%lx.c itemconfigure %lxKNOB -strokewidth %d\n",
            canvas, x, 4*x->x_thick+3);
}

static void slider_draw_io(t_slider *x, t_glist *glist, int old_snd_rcv_flags)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_io_draw(&x->x_gui,canvas,old_snd_rcv_flags);
}
static void slider_draw_new(t_slider *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int x1=text_xpix(&x->x_gui.x_obj, glist), x2=x1+x->x_gui.x_w;
    int y1=text_ypix(&x->x_gui.x_obj, glist), y2=y1+x->x_gui.x_h;
    if (x->x_orient) y2+=5; else x2+=5;
    int r;
    if (x->x_orient) r = y2-3 - (x->x_val + 50)/100;
    else             r = x1+3 + (x->x_val + 50)/100;
    char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);
    iemgui_base_draw_new(&x->x_gui, canvas, nlet_tag);
    scalehandle_draw_new(x->x_gui. x_handle,canvas);
    scalehandle_draw_new(x->x_gui.x_lhandle,canvas);
    if (x->x_orient) {
        sys_vgui(".x%lx.c create polyline %d %d %d %d -strokewidth 3 "
            "-stroke #%6.6x -tags {%lxKNOB %lxOBJ %s text iemgui}\n",
            canvas, x1+2, r, x2-2, r, x->x_gui.x_fcol, x, x, nlet_tag);
    } else {
        sys_vgui(".x%lx.c create polyline %d %d %d %d -strokewidth 3 "
            "-stroke #%6.6x -tags {%lxKNOB %lxOBJ %s text iemgui}\n",
            canvas, r, y1+2, r, y2-2, x->x_gui.x_fcol, x, x, nlet_tag);
    }
    iemgui_label_draw_new(&x->x_gui,canvas,x1,y1,nlet_tag);
    slider_draw_io(x,glist,7);
}

static void slider_draw_move(t_slider *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    if (!glist_isvisible(canvas)) return;
    int x1=text_xpix(&x->x_gui.x_obj, glist), x2=x1+x->x_gui.x_w;
    int y1=text_ypix(&x->x_gui.x_obj, glist), y2=y1+x->x_gui.x_h;
    if (x->x_orient) y2+=5; else x2+=5;
    int r;
    if (x->x_orient) r = y2-3 - (x->x_val + 50)/100;
    else             r = x1+3 + (x->x_val + 50)/100;
    char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);
    iemgui_base_draw_move(&x->x_gui, canvas, nlet_tag);
    if (x->x_orient)
        sys_vgui(".x%lx.c coords %lxKNOB %d %d %d %d\n",
            canvas, x, x1+2, r, x2-2, r);
    else
        sys_vgui(".x%lx.c coords %lxKNOB %d %d %d %d\n",
            canvas, x, r, y1+2, r, y2-2);
    iemgui_label_draw_move(&x->x_gui,canvas,x1,y1);
    iemgui_io_draw_move(&x->x_gui,canvas,nlet_tag);
    if (x->x_gui.x_selected) {
        if (x->x_orient) vslider_draw_select(x, x->x_gui.x_glist);
        else             hslider_draw_select(x, x->x_gui.x_glist);
    }
    if (x->x_gui.x_selected) vslider_draw_select(x, x->x_gui.x_glist);
}

static void slider_draw_config(t_slider *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_label_draw_config(&x->x_gui,canvas);
    iemgui_base_draw_config(&x->x_gui,canvas);
    sys_vgui(".x%lx.c itemconfigure %lxKNOB -stroke #%6.6x\n",
        canvas, x, x->x_gui.x_fcol);
}

static void hslider_draw_select(t_slider *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_base_draw_config(&x->x_gui,canvas);
    if(x->x_gui.x_selected)
    {
        if (x->x_gui.x_glist == glist_getcanvas(glist))
            scalehandle_draw_select2(&x->x_gui,glist,
                x->x_gui.x_w+5-1,x->x_gui.x_h-1);
    }
    else scalehandle_draw_erase2(&x->x_gui,glist);
    iemgui_label_draw_select(&x->x_gui,canvas);
    iemgui_tag_selected(&x->x_gui,canvas);
}
static void vslider_draw_select(t_slider *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_base_draw_config(&x->x_gui,canvas);
    if(x->x_gui.x_selected)
    {
        if (x->x_gui.x_glist == glist_getcanvas(glist))
            scalehandle_draw_select2(&x->x_gui,glist,
                x->x_gui.x_w-1,x->x_gui.x_h+5-1);
    }
    else scalehandle_draw_erase2(&x->x_gui,glist);
    iemgui_label_draw_select(&x->x_gui,canvas);
    iemgui_tag_selected(&x->x_gui,canvas);
}

void slider_check_minmax(t_slider *x, double min, double max);
void hslider_check_width(t_slider *x, int w);
void vslider_check_height(t_slider *x, int w);

static void hslider__clickhook2(t_scalehandle *sh, t_slider *x) {
    double w_change_ratio = (double)(x->x_gui.x_w + sh->h_dragx)
        /(double)x->x_gui.x_w;
    x->x_val = x->x_val * w_change_ratio;
    hslider_check_width(x, x->x_gui.x_w + sh->h_dragx);
    x->x_gui.x_h += sh->h_dragy;
    slider_check_minmax(x, x->x_min, x->x_max);
}
static void vslider__clickhook2(t_scalehandle *sh, t_slider *x) {
    double h_change_ratio = (double)(x->x_gui.x_h + sh->h_dragy)
        /(double)x->x_gui.x_h;
    x->x_val = x->x_val * h_change_ratio;
    x->x_gui.x_w += sh->h_dragx;
    vslider_check_height(x, x->x_gui.x_h + sh->h_dragy);
    slider_check_minmax(x, x->x_min, x->x_max);
}

static void slider__clickhook(t_scalehandle *sh, t_floatarg f,
    t_floatarg xxx, t_floatarg yyy)
{
    int newstate = (int)f;
    if (sh->h_dragon && newstate == 0 && sh->h_scale)
    {
        t_slider *x = (t_slider *)(sh->h_master);
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
        if (sh->h_dragx || sh->h_dragy)
        {
            if (x->x_orient) vslider__clickhook2(sh,x);
            else             hslider__clickhook2(sh,x);
            canvas_dirty(x->x_gui.x_glist, 1);
        }
        if (glist_isvisible(x->x_gui.x_glist))
        {
            slider_draw_move(x, x->x_gui.x_glist);
            scalehandle_unclick_scale(sh);
        }
    }
    iemgui__clickhook3(sh,newstate);
}

static void hslider__motionhook(t_scalehandle *sh, t_floatarg f1, t_floatarg f2)
{
    if (sh->h_dragon && sh->h_scale)
    {
        t_slider *x = (t_slider *)(sh->h_master);
        int dx = (int)f1, dy = (int)f2;
        dx = maxi(dx, IEM_SL_MINSIZE-x->x_gui.x_w);
        dy = maxi(dy,IEM_GUI_MINSIZE-x->x_gui.x_h);
        sh->h_dragx = dx;
        sh->h_dragy = dy;
        scalehandle_drag_scale(sh);

        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            int new_w = x->x_gui.x_w + sh->h_dragx;
            int new_h = x->x_gui.x_h + sh->h_dragy;
            properties_set_field_int(properties,"dim.w_ent",new_w);
            properties_set_field_int(properties,"dim.h_ent",new_h);
        }
    }
    scalehandle_dragon_label(sh,f1,f2);
}
static void vslider__motionhook(t_scalehandle *sh, t_floatarg f1, t_floatarg f2)
{
    if (sh->h_dragon && sh->h_scale)
    {
        t_slider *x = (t_slider *)(sh->h_master);
        int dx = (int)f1, dy = (int)f2;
        dx = maxi(dx,IEM_GUI_MINSIZE-x->x_gui.x_w);
        dy = maxi(dy, IEM_SL_MINSIZE-x->x_gui.x_h);
        sh->h_dragx = dx;
        sh->h_dragy = dy;
        scalehandle_drag_scale(sh);

        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            int new_w = x->x_gui.x_w + sh->h_dragx;
            int new_h = x->x_gui.x_h + sh->h_dragy;
            properties_set_field_int(properties,"dim.w_ent",new_w);
            properties_set_field_int(properties,"dim.h_ent",new_h);
        }
    }
    scalehandle_dragon_label(sh,f1,f2);
}

void hslider_draw(t_slider *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui(x, glist, slider_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        slider_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
    {
        slider_draw_new(x, glist);
        sys_vgui(".x%lx.c raise all_cords\n", glist_getcanvas(glist));
    }
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        hslider_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        iemgui_draw_erase(&x->x_gui, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        slider_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        slider_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}
void vslider_draw(t_slider *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui(x, glist, slider_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        slider_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
    {
        slider_draw_new(x, glist);
        sys_vgui(".x%lx.c raise all_cords\n", glist_getcanvas(glist));
    }
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        vslider_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        iemgui_draw_erase(&x->x_gui, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        slider_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        slider_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

static void hslider_getrect(t_gobj *z, t_glist *glist,
                            int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_slider *x = (t_slider *)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w + 5;
    *yp2 = *yp1 + x->x_gui.x_h;

    iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);
}


static void vslider_getrect(t_gobj *z, t_glist *glist,
                            int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_slider *x = (t_slider *)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h + 5;

    iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);
}

static void hslider_save(t_gobj *z, t_binbuf *b)
{
    t_slider *x = (t_slider *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiffiisssiiiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("hsl"), x->x_gui.x_w, x->x_gui.x_h,
                (t_float)x->x_min, (t_float)x->x_max,
                x->x_lin0_log1, iem_symargstoint(&x->x_gui),
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2],
                x->x_val, x->x_steady);
    binbuf_addv(b, ";");
}


static void vslider_save(t_gobj *z, t_binbuf *b)
{
    t_slider *x = (t_slider *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiffiisssiiiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("vsl"), x->x_gui.x_w, x->x_gui.x_h, // diff
                (t_float)x->x_min, (t_float)x->x_max,
                x->x_lin0_log1, iem_symargstoint(&x->x_gui),
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2],
                x->x_val, x->x_steady);
    binbuf_addv(b, ";");
}

void hslider_check_width(t_slider *x, int w)
{
    if(w < IEM_SL_MINSIZE)
        w = IEM_SL_MINSIZE;
    x->x_gui.x_w = w;
    x->x_center = (x->x_gui.x_w-1)*50;
    if(x->x_val > (x->x_gui.x_w*100 - 100))
    {
        x->x_pos = x->x_gui.x_w*100 - 100;
        x->x_val = x->x_pos;
    }
    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_w - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_w - 1);
}

void vslider_check_height(t_slider *x, int h)
{
    if(h < IEM_SL_MINSIZE)
        h = IEM_SL_MINSIZE;
    x->x_gui.x_h = h;
    x->x_center = (x->x_gui.x_h-1)*50;
    if(x->x_val > (x->x_gui.x_h*100 - 100))
    {
        x->x_pos = x->x_gui.x_h*100 - 100;
        x->x_val = x->x_pos;
    }
    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_h - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

void slider_check_minmax(t_slider *x, double min, double max)
{
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
    x->x_gui.x_reverse = x->x_min > x->x_max;
    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_w - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_w - 1);
}

static void hslider_properties(t_gobj *z, t_glist *owner)
{
    t_slider *x = (t_slider *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |hsl| \
        --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
        -----------output-range:----------- %g left: %g right: %g \
        %d lin log %d %d empty %d {%s} {%s} {%s} %d %d %d %d %d %d %d\n",
        x->x_gui.x_w, IEM_SL_MINSIZE, x->x_gui.x_h, IEM_GUI_MINSIZE,
        x->x_min, x->x_max, 0.0,/*no_schedule*/
        x->x_lin0_log1, x->x_gui.x_loadinit, x->x_steady, -1,/*no multi, but iem-characteristic*/
        srl[0]->s_name, srl[1]->s_name,
        srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
        x->x_gui.x_font_style, x->x_gui.x_fontsize,
        0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol,
        0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}
static void vslider_properties(t_gobj *z, t_glist *owner)
{
    t_slider *x = (t_slider *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |vsl| \
        --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
        -----------output-range:----------- %g bottom: %g top: %g \
        %d lin log %d %d empty %d {%s} {%s} {%s} %d %d %d %d %d %d %d\n",
        x->x_gui.x_w, IEM_GUI_MINSIZE, x->x_gui.x_h, IEM_SL_MINSIZE,
        x->x_min, x->x_max, 0.0,/*no_schedule*/
        x->x_lin0_log1, x->x_gui.x_loadinit, x->x_steady, -1,/*no multi, but iem-characteristic*/
        srl[0]->s_name, srl[1]->s_name,
        srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
        x->x_gui.x_font_style, x->x_gui.x_fontsize,
        0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol,
        0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void slider_bang(t_slider *x)
{
    double out;

    if(x->x_lin0_log1)
        out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else {
        if (x->x_is_last_float && x->x_last <= x->x_max &&
            x->x_last >= x->x_min)
            out = x->x_last;
        else
            out = (double)(x->x_val)*0.01*x->x_k + x->x_min;
    }
    if((out < 1.0e-10)&&(out > -1.0e-10))
        out = 0.0;
    outlet_float(x->x_gui.x_obj.ob_outlet, out);
    if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
        pd_float(x->x_gui.x_snd->s_thing, out);
}

static void hslider_dialog(t_slider *x, t_symbol *s, int argc, t_atom *argv)
{
    canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    double min = (double)atom_getfloatarg(2, argc, argv);
    double max = (double)atom_getfloatarg(3, argc, argv);
    int lilo = (int)atom_getintarg(4, argc, argv);
    int steady = (int)atom_getintarg(17, argc, argv);
    int sr_flags;

    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady)
        x->x_steady = 1;
    else
        x->x_steady = 0;
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    x->x_gui.x_h = iemgui_clip_size(h); //diff
    int old_width = x->x_gui.x_w; //diff
    hslider_check_width(x, w); //diff
    if (x->x_gui.x_w != old_width) //diff
    {
        x->x_val = x->x_val * ((double)x->x_gui.x_w/(double)old_width); //diff
    }
    slider_check_minmax(x, min, max);
    x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    //x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    //canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
    iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_MOVE);

    if (x->x_gui.x_selected) hslider_draw_select(x, x->x_gui.x_glist); //diff
    scrollbar_update(x->x_gui.x_glist);
}
static void vslider_dialog(t_slider *x, t_symbol *s, int argc, t_atom *argv)
{
    canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    double min = (double)atom_getfloatarg(2, argc, argv);
    double max = (double)atom_getfloatarg(3, argc, argv);
    int lilo = (int)atom_getintarg(4, argc, argv);
    int steady = (int)atom_getintarg(17, argc, argv);
    int sr_flags;

    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady)
        x->x_steady = 1;
    else
        x->x_steady = 0;
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    x->x_gui.x_w = iemgui_clip_size(w);
    int old_height = x->x_gui.x_h;
    vslider_check_height(x, h);
    if (x->x_gui.x_h != old_height)
    {
        x->x_val = x->x_val * ((double)x->x_gui.x_h/(double)old_height);
    }
    slider_check_minmax(x, min, max);
    x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    //x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    //canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
    iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_MOVE);

    if (x->x_gui.x_selected) vslider_draw_select(x, x->x_gui.x_glist);
    scrollbar_update(x->x_gui.x_glist);
}

static void hslider_motion(t_slider *x, t_floatarg dx, t_floatarg dy)
{
    x->x_is_last_float = 0;
    int old = x->x_val;

    if(x->x_gui.x_finemoved)
        x->x_pos += (int)dx;
    else
        x->x_pos += 100*(int)dx;
    x->x_val = x->x_pos;
    if(x->x_val > (100*x->x_gui.x_w - 100))
    {
        x->x_val = 100*x->x_gui.x_w - 100;
        x->x_pos += 50;
        x->x_pos -= x->x_pos%100;
    }
    if(x->x_val < 0)
    {
        x->x_val = 0;
        x->x_pos -= 50;
        x->x_pos -= x->x_pos%100;
    }
    if(old != x->x_val)
    {
        x->x_gui.x_changed = 1;
        x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        slider_bang(x);
    }
}
static void vslider_motion(t_slider *x, t_floatarg dx, t_floatarg dy)
{
    x->x_is_last_float = 0;
    int old = x->x_val;

    if(x->x_gui.x_finemoved)
        x->x_pos -= (int)dy;
    else
        x->x_pos -= 100*(int)dy;
    x->x_val = x->x_pos;
    if(x->x_val > (100*x->x_gui.x_h - 100))
    {
        x->x_val = 100*x->x_gui.x_h - 100;
        x->x_pos += 50;
        x->x_pos -= x->x_pos%100;
    }
    if(x->x_val < 0)
    {
        x->x_val = 0;
        x->x_pos -= 50;
        x->x_pos -= x->x_pos%100;
    }
    if(old != x->x_val)
    {
        x->x_gui.x_changed = 1;
        x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        slider_bang(x);
    }
}

static void hslider_click(t_slider *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    if(!x->x_steady)
        x->x_val = (int)(100.0 * (xpos -
            text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist)));
    if(x->x_val > (100*x->x_gui.x_w - 100))
        x->x_val = 100*x->x_gui.x_w - 100;
    if(x->x_val < 0)
        x->x_val = 0;
    if (x->x_pos != x->x_val)
    {
        x->x_pos = x->x_val;
        x->x_gui.x_changed = 1;
        x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
    slider_bang(x);
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
        (t_glistmotionfn)hslider_motion, 0, xpos, ypos);
}
static void vslider_click(t_slider *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    if(!x->x_steady)
        x->x_val = (int)(100.0 * (x->x_gui.x_h +
            text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist) - ypos));
    if(x->x_val > (100*x->x_gui.x_h - 100))
        x->x_val = 100*x->x_gui.x_h - 100;
    if(x->x_val < 0)
        x->x_val = 0;
    if (x->x_pos != x->x_val)
    {
        x->x_pos = x->x_val;
        x->x_gui.x_changed = 1;
        x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
    slider_bang(x);
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
        (t_glistmotionfn)vslider_motion, 0, xpos, ypos);
}

static int hslider_newclick(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_slider *x = (t_slider *)z;

    if(doit)
    {
        hslider_click(x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift,
            0, (t_floatarg)alt);
        if(shift)
            x->x_gui.x_finemoved = 1;
        else
            x->x_gui.x_finemoved = 0;
    }
    return (1);
}
static int vslider_newclick(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_slider *x = (t_slider *)z;

    if(doit)
    {
        vslider_click(x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift,
            0, (t_floatarg)alt);
        if(shift)
            x->x_gui.x_finemoved = 1;
        else
            x->x_gui.x_finemoved = 0;
    }
    return (1);
}

static void slider_set(t_slider *x, t_floatarg f)
{
    double g;
    if(x->x_gui.x_reverse)
    {
        if(f > x->x_min) f = x->x_min;
        if(f < x->x_max) f = x->x_max;
    }
    else
    {
        if(f > x->x_max) f = x->x_max;
        if(f < x->x_min) f = x->x_min;
    }
    if(x->x_lin0_log1)
        g = log(f/x->x_min)/x->x_k;
    else
        g = (f - x->x_min) / x->x_k;
    x->x_val = (int)(100.0*g + 0.49999);
    if (x->x_pos != x->x_val)
    {
        x->x_pos = x->x_val;
        x->x_gui.x_changed = 1;
        x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
}

static void slider_float(t_slider *x, t_floatarg f)
{
    x->x_is_last_float = 1;
    x->x_last = f;
    slider_set(x, f);
    if(x->x_gui.x_put_in2out)
        slider_bang(x);
}

static void hslider_size(t_slider *x, t_symbol *s, int ac, t_atom *av)
{
    hslider_check_width(x, (int)atom_getintarg(0, ac, av));
    if(ac > 1)
        x->x_gui.x_h = iemgui_clip_size((int)atom_getintarg(1, ac, av));
    iemgui_size(&x->x_gui);
}
static void vslider_size(t_slider *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    if(ac > 1)
        vslider_check_height(x, (int)atom_getintarg(1, ac, av));
    iemgui_size(&x->x_gui);
}

static void slider_range(t_slider *x, t_symbol *s, int ac, t_atom *av)
{
    slider_check_minmax(x, (double)atom_getfloatarg(0, ac, av),
                         (double)atom_getfloatarg(1, ac, av));
}

static void slider_log(t_slider *x)
{
    x->x_lin0_log1 = 1;
    slider_check_minmax(x, x->x_min, x->x_max);
}

static void hslider_lin(t_slider *x)
{
    x->x_lin0_log1 = 0;
    x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_w - 1);
}
static void vslider_lin(t_slider *x)
{
    x->x_lin0_log1 = 0;
    x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

static void slider_init(t_slider *x, t_floatarg f)
{
    x->x_gui.x_loadinit = (f==0.0)?0:1;
}

static void slider_steady(t_slider *x, t_floatarg f)
{
    x->x_steady = (f==0.0)?0:1;
}

static void slider_loadbang(t_slider *x)
{
    if(!sys_noloadbang && x->x_gui.x_loadinit)
    {
        x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        slider_bang(x);
    }
}

static void *slider_new(t_symbol *s, int argc, t_atom *argv)
{
    int orient = s==gensym("vsl") || s==gensym("vslider");
    t_slider *x = (t_slider *)pd_new(orient ? vslider_class : hslider_class);
    x->x_orient = orient;
    int bflcol[]={-262144, -1, -1};
    int lilo=0;
    int w,h,ldx,ldy,fs=10, v=0, steady=1;
    if (orient) {
        w=IEM_GUI_DEFAULTSIZE; h=IEM_SL_DEFAULTSIZE; ldx=0, ldy=-9;
    } else {
        w=IEM_SL_DEFAULTSIZE; h=IEM_GUI_DEFAULTSIZE; ldx=-2; ldy=-8;
    }
    double min=0.0, max=(double)(IEM_SL_DEFAULTSIZE-1);

    iem_inttosymargs(&x->x_gui, 0);
    iem_inttofstyle(&x->x_gui, 0);

    if(((argc == 17)||(argc == 18))&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
       &&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,3)
       &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&(IS_A_SYMBOL(argv,7)||IS_A_FLOAT(argv,7))
       &&(IS_A_SYMBOL(argv,8)||IS_A_FLOAT(argv,8))
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)
       &&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13)
       &&IS_A_FLOAT(argv,14)&&IS_A_FLOAT(argv,15)&&IS_A_FLOAT(argv,16))
    {
        w = (int)atom_getintarg(0, argc, argv);
        h = (int)atom_getintarg(1, argc, argv);
        min = (double)atom_getfloatarg(2, argc, argv);
        max = (double)atom_getfloatarg(3, argc, argv);
        lilo = (int)atom_getintarg(4, argc, argv);
        iem_inttosymargs(&x->x_gui, atom_getintarg(5, argc, argv));
        iemgui_new_getnames(&x->x_gui, 6, argv);
        ldx = (int)atom_getintarg(9, argc, argv);
        ldy = (int)atom_getintarg(10, argc, argv);
        iem_inttofstyle(&x->x_gui, atom_getintarg(11, argc, argv));
        fs = (int)atom_getintarg(12, argc, argv);
        bflcol[0] = (int)atom_getintarg(13, argc, argv);
        bflcol[1] = (int)atom_getintarg(14, argc, argv);
        bflcol[2] = (int)atom_getintarg(15, argc, argv);
        v = (int)atom_getintarg(16, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 6, 0);
    if((argc == 18)&&IS_A_FLOAT(argv,17))
        steady = (int)atom_getintarg(17, argc, argv);
    x->x_gui.x_draw = orient ? (t_iemfunptr)vslider_draw : (t_iemfunptr)hslider_draw;
    x->x_is_last_float = 0;
    x->x_last = 0.0;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if(x->x_gui.x_loadinit)
        x->x_val = v;
    else
        x->x_val = 0;
    x->x_pos = x->x_val;
    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady != 0) steady = 1;
    x->x_steady = steady;
    if (x->x_gui.x_font_style<0 || x->x_gui.x_font_style>2) x->x_gui.x_font_style=0;
    if(iemgui_has_rcv(&x->x_gui))
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    if (orient) {
        x->x_gui.x_w = iemgui_clip_size(w);
        vslider_check_height(x, h);
    } else {
        x->x_gui.x_h = iemgui_clip_size(h);
        hslider_check_width(x, w);
    }
    slider_check_minmax(x, min, max);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    x->x_thick = 0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    outlet_new(&x->x_gui.x_obj, &s_float);

    t_class *sc = orient ? vscalehandle_class : hscalehandle_class;
    x->x_gui. x_handle = scalehandle_new(sc,(t_iemgui *)x,1);
    x->x_gui.x_lhandle = scalehandle_new(sc,(t_iemgui *)x,0);
    x->x_gui.x_obj.te_iemgui = 1;
    x->x_gui.x_changed = 0;

    return (x);
}

static void slider_free(t_slider *x)
{
    if(iemgui_has_rcv(&x->x_gui))
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);

    if (x->x_gui. x_handle) scalehandle_free(x->x_gui. x_handle);
    if (x->x_gui.x_lhandle) scalehandle_free(x->x_gui.x_lhandle);
}

void slider_addmethods(t_class *c) {
    class_addmethod(c, (t_method)slider_loadbang,
        gensym("loadbang"), 0);
    class_addmethod(c, (t_method)slider_set,
        gensym("set"), A_FLOAT, 0);
    class_addmethod(c, (t_method)slider_log, gensym("log"), 0);
    class_addmethod(c, (t_method)slider_init,
        gensym("init"), A_FLOAT, 0);
    class_addmethod(c, (t_method)slider_steady,
        gensym("steady"), A_FLOAT, 0);
    class_addmethod(c, (t_method)slider_range,
        gensym("range"), A_GIMME, 0);
}

void g_hslider_setup(void)
{
    hslider_class = class_new(gensym("hsl"), (t_newmethod)slider_new,
        (t_method)slider_free, sizeof(t_slider), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)slider_new, gensym("hslider"), A_GIMME, 0);
    class_addbang(hslider_class,slider_bang);
    class_addfloat(hslider_class,slider_float);
    class_addmethod(hslider_class, (t_method)hslider_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(hslider_class, (t_method)hslider_motion, gensym("motion"),
        A_FLOAT, A_FLOAT, 0);
    class_addmethod(hslider_class, (t_method)hslider_dialog, gensym("dialog"),
        A_GIMME, 0);
    slider_addmethods(hslider_class);
    class_addmethod(hslider_class, (t_method)hslider_size,
        gensym("size"), A_GIMME, 0);
    iemgui_class_addmethods(hslider_class);
    class_addmethod(hslider_class, (t_method)hslider_lin, gensym("lin"), 0);
 
    hscalehandle_class = class_new(gensym("_scalehandle"), 0, 0,
        sizeof(t_scalehandle), CLASS_PD, 0);
    class_addmethod(hscalehandle_class, (t_method)slider__clickhook,
        gensym("_click"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(hscalehandle_class, (t_method)hslider__motionhook,
        gensym("_motion"), A_FLOAT, A_FLOAT, 0);

    wb_init(&hslider_widgetbehavior,hslider_getrect,hslider_newclick);
    class_setwidget(hslider_class, &hslider_widgetbehavior);
    class_sethelpsymbol(hslider_class, gensym("hslider"));
    class_setsavefn(hslider_class, hslider_save);
    class_setpropertiesfn(hslider_class, hslider_properties);
}
void g_vslider_setup(void)
{
    vslider_class = class_new(gensym("vsl"), (t_newmethod)slider_new,
        (t_method)slider_free, sizeof(t_slider), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)slider_new, gensym("vslider"), A_GIMME, 0);
    class_addbang(vslider_class,slider_bang);
    class_addfloat(vslider_class,slider_float);
    class_addmethod(vslider_class, (t_method)vslider_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(vslider_class, (t_method)vslider_motion, gensym("motion"),
        A_FLOAT, A_FLOAT, 0);
    class_addmethod(vslider_class, (t_method)vslider_dialog, gensym("dialog"),
        A_GIMME, 0);
    slider_addmethods(hslider_class);
    class_addmethod(vslider_class, (t_method)vslider_size,
        gensym("size"), A_GIMME, 0);
    iemgui_class_addmethods(vslider_class);
    class_addmethod(vslider_class, (t_method)vslider_lin, gensym("lin"), 0);
 
    vscalehandle_class = class_new(gensym("_scalehandle"), 0, 0,
        sizeof(t_scalehandle), CLASS_PD, 0);
    class_addmethod(vscalehandle_class, (t_method)slider__clickhook,
        gensym("_click"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(vscalehandle_class, (t_method)vslider__motionhook,
        gensym("_motion"), A_FLOAT, A_FLOAT, 0);

    wb_init(&vslider_widgetbehavior,vslider_getrect,vslider_newclick);
    class_setwidget(vslider_class, &vslider_widgetbehavior);
    class_sethelpsymbol(vslider_class, gensym("vslider"));
    class_setsavefn(vslider_class, vslider_save);
    class_setpropertiesfn(vslider_class, vslider_properties);
}
