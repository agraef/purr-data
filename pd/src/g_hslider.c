/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "t_tk.h"
#include "g_all_guis.h"
#include <math.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

extern int gfxstub_haveproperties(void *key);
static void hslider_draw_select(t_hslider* x,t_glist* glist);

/* ------------ hsl    gui-horicontal  slider ----------------------- */

t_widgetbehavior hslider_widgetbehavior;
static t_class *hslider_class;

/* widget helper functions */

static void hslider_draw_update(t_gobj *client, t_glist *glist)
{
    t_hslider *x = (t_hslider *)client;
    if (x->x_gui.x_changed == 0) return;
    t_canvas *canvas=glist_getcanvas(glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);

    if (glist_isvisible(glist))
    {
        int r = text_xpix(&x->x_gui.x_obj, glist) + 3 + (x->x_val + 50)/100;
        sys_vgui(".x%lx.c coords %lxKNOB %d %d %d %d\n",
                 canvas, x, r, ypos+2,
                 r, ypos + x->x_gui.x_h-2);
        if(x->x_val == x->x_center)
        {
            if(!x->x_thick)
            {
                sys_vgui(".x%lx.c itemconfigure %lxKNOB -strokewidth 7\n",
                    canvas, x);
                x->x_thick = 1;
            }
        }
        else
        {
            if(x->x_thick)
            {
                sys_vgui(".x%lx.c itemconfigure %lxKNOB -strokewidth 3\n",
                    canvas, x);
                x->x_thick = 0;
            }
        }
    }
    x->x_gui.x_changed = 0;
}

static void hslider_draw_io(t_hslider* x,t_glist* glist, int old_snd_rcv_flags)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_io_draw(&x->x_gui,canvas,old_snd_rcv_flags,"HSLDR");
}
static void hslider_draw_new(t_hslider *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int r = xpos + 3 + (x->x_val + 50)/100;
    t_canvas *canvas=glist_getcanvas(glist);

    scalehandle_draw_new(x->x_gui. x_handle,canvas);
    scalehandle_draw_new(x->x_gui.x_lhandle,canvas);

        char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

        sys_vgui(".x%lx.c create prect %d %d %d %d "
                 "-stroke $pd_colors(iemgui_border) -fill #%6.6x "
                 "-tags {%lxBASE %lxHSLDR %s text iemgui border}\n",
             canvas, xpos, ypos,
             xpos + x->x_gui.x_w+5, ypos + x->x_gui.x_h,
             x->x_gui.x_bcol, x, x, nlet_tag);
        sys_vgui(".x%lx.c create polyline %d %d %d %d -strokewidth 3 "
                 "-stroke #%6.6x -tags {%lxKNOB %lxHSLDR %s text iemgui}\n",
             canvas, r, ypos+2, r,
             ypos + x->x_gui.x_h-2, x->x_gui.x_fcol, x, x, nlet_tag);
        iemgui_label_draw_new(&x->x_gui,canvas,xpos,ypos,nlet_tag,"HSLDR");
    hslider_draw_io(x,glist,7);
}

static void hslider_draw_move(t_hslider *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int r = xpos + 3 + (x->x_val + 50)/100;
    t_canvas *canvas=glist_getcanvas(glist);

    if (glist_isvisible(canvas))
    {

        char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

        sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n",
                 canvas, x,
                 xpos, ypos,
                 xpos + x->x_gui.x_w+5, ypos + x->x_gui.x_h);
        sys_vgui(".x%lx.c coords %lxKNOB %d %d %d %d\n",
                 canvas, x, r, ypos+2,
                 r, ypos + x->x_gui.x_h-2);
        iemgui_label_draw_move(&x->x_gui,canvas,xpos,ypos);
        if(!iemgui_has_snd(&x->x_gui) && canvas == x->x_gui.x_glist)
            sys_vgui(".x%lx.c coords %lxHSLDR%so%d %d %d %d %d\n",
                 canvas, x, nlet_tag, 0,
                 xpos, ypos + x->x_gui.x_h-1,
                 xpos+7, ypos + x->x_gui.x_h);
        if(!iemgui_has_rcv(&x->x_gui) && canvas == x->x_gui.x_glist)
            sys_vgui(".x%lx.c coords %lxHSLDR%si%d %d %d %d %d\n",
                 canvas, x, nlet_tag, 0,
                 xpos, ypos,
                 xpos+7, ypos+1);
        /* redraw scale handle rectangle if selected */
        if (x->x_gui.x_selected)
        {
            hslider_draw_select(x, x->x_gui.x_glist);
        }
    }
}

static void hslider_draw_config(t_hslider* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_label_draw_config(&x->x_gui,canvas);
    sys_vgui(".x%lx.c itemconfigure %lxKNOB "
             "-stroke #%6.6x\n .x%lx.c itemconfigure %lxBASE -fill #%6.6x\n",
        canvas, x, x->x_gui.x_fcol, canvas, x, x->x_gui.x_bcol);
}

static void hslider_draw_select(t_hslider* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    if(x->x_gui.x_selected)
    {
        // check if we are drawing inside a gop abstraction
        // visible on parent canvas. If so, disable highlighting
        if (x->x_gui.x_glist == glist_getcanvas(glist))
        {
            sys_vgui(".x%lx.c itemconfigure %lxBASE "
                     "-stroke $pd_colors(selection)\n", canvas, x);
            scalehandle_draw_select2(&x->x_gui,glist,"HSLDR",
                x->x_gui.x_w+5-1,x->x_gui.x_h-1);
        }
    }
    else
    {
        sys_vgui(".x%lx.c itemconfigure %lxBASE -stroke %s\n",
            canvas, x, IEM_GUI_COLOR_NORMAL);
        scalehandle_draw_erase2(&x->x_gui,glist);
    }
    iemgui_label_draw_select(&x->x_gui,canvas);
    iemgui_tag_selected(&x->x_gui,canvas,"HSLDR");
}

void hslider_check_minmax(t_hslider *x, double min, double max);
void hslider_check_width(t_hslider *x, int w);

static void hslider__clickhook(t_scalehandle *sh, t_floatarg f,
    t_floatarg xxx, t_floatarg yyy)
{
    t_hslider *x = (t_hslider *)(sh->h_master);
    int newstate = (int)f;
    if (sh->h_dragon && newstate == 0 && sh->h_scale)
    {
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
        if (sh->h_dragx || sh->h_dragy)
        {
            double width_change_ratio = (double)(x->x_gui.x_w +
                sh->h_dragx)/(double)x->x_gui.x_w;
            x->x_val = x->x_val * width_change_ratio;
            hslider_check_width(x, x->x_gui.x_w + sh->h_dragx);
            x->x_gui.x_h += sh->h_dragy;
            hslider_check_minmax(x, x->x_min, x->x_max);
            canvas_dirty(x->x_gui.x_glist, 1);
        }
        if (glist_isvisible(x->x_gui.x_glist))
        {
            hslider_draw_move(x, x->x_gui.x_glist);
            scalehandle_unclick_scale(sh);
        }
    }
    else if (!sh->h_dragon && newstate && sh->h_scale)
    {
        scalehandle_click_scale(sh);
    }
    else if (sh->h_dragon && newstate == 0 && !sh->h_scale)
    {
        scalehandle_unclick_label(sh);
    }
    else if (!sh->h_dragon && newstate && !sh->h_scale)
    {
        scalehandle_click_label(sh);
    }
    sh->h_dragon = newstate;
}

static void hslider__motionhook(t_scalehandle *sh, t_floatarg f1, t_floatarg f2)
{
    if (sh->h_dragon && sh->h_scale)
    {
        t_hslider *x = (t_hslider *)(sh->h_master);
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


void hslider_draw(t_hslider *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui(x, glist, hslider_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        hslider_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
    {
        hslider_draw_new(x, glist);
        sys_vgui(".x%lx.c raise all_cords\n", glist_getcanvas(glist));
    }
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        hslider_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        iemgui_draw_erase(&x->x_gui, glist, "HSLDR");
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        hslider_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        hslider_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ hsl widgetbehaviour----------------------------- */

static void hslider_getrect(t_gobj *z, t_glist *glist,
                            int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_hslider* x = (t_hslider*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w + 5;
    *yp2 = *yp1 + x->x_gui.x_h;

    iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);
}

static void hslider_save(t_gobj *z, t_binbuf *b)
{
    t_hslider *x = (t_hslider *)z;
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

void hslider_check_width(t_hslider *x, int w)
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

void hslider_check_minmax(t_hslider *x, double min, double max)
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
    if(x->x_min > x->x_max)                /* bugfix */
        x->x_gui.x_reverse = 1;
    else
        x->x_gui.x_reverse = 0;
    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_w - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_w - 1);
}

static void hslider_properties(t_gobj *z, t_glist *owner)
{
    t_hslider *x = (t_hslider *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |hsl| \
            --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
            -----------output-range:----------- %g left: %g right: %g \
            %d lin log %d %d empty %d \
            {%s} {%s} \
            {%s} %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, IEM_SL_MINSIZE, x->x_gui.x_h, IEM_GUI_MINSIZE,
            x->x_min, x->x_max, 0.0,/*no_schedule*/
            x->x_lin0_log1, x->x_gui.x_loadinit, x->x_steady, -1,/*no multi, but iem-characteristic*/
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void hslider_set(t_hslider *x, t_floatarg f)    /* bugfix */
{
    double g;

    if(x->x_gui.x_reverse)    /* bugfix */
    {
        if(f > x->x_min)
            f = x->x_min;
        if(f < x->x_max)
            f = x->x_max;
    }
    else
    {
        if(f > x->x_max)
            f = x->x_max;
        if(f < x->x_min)
            f = x->x_min;
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
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
}

static void hslider_bang(t_hslider *x)
{
    double out;

    if(x->x_lin0_log1)
        out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else
    {
        if (x->x_is_last_float &&
            x->x_last <= x->x_max && x->x_last >= x->x_min)
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

static void hslider_dialog(t_hslider *x, t_symbol *s, int argc, t_atom *argv)
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
    x->x_gui.x_h = iemgui_clip_size(h);
    int old_width = x->x_gui.x_w;
    hslider_check_width(x, w);
    if (x->x_gui.x_w != old_width)
    {
        x->x_val = x->x_val * ((double)x->x_gui.x_w/(double)old_width);
    }
    hslider_check_minmax(x, min, max);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    //(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    //canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
    iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_MOVE);

    /* forcing redraw of the scale handle */
    if (x->x_gui.x_selected)
    {
        hslider_draw_select(x, x->x_gui.x_glist);
    }

    // ico@bukvic.net 100518
    // update scrollbars when object potentially exceeds window size
    t_canvas *canvas=(t_canvas *)glist_getcanvas(x->x_gui.x_glist);
    sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", (long unsigned int)canvas);
}

static void hslider_motion(t_hslider *x, t_floatarg dx, t_floatarg dy)
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
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        hslider_bang(x);
    }
}

static void hslider_click(t_hslider *x, t_floatarg xpos, t_floatarg ypos,
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
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
    hslider_bang(x);
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
        (t_glistmotionfn)hslider_motion,
        0, xpos, ypos);
}

static int hslider_newclick(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_hslider* x = (t_hslider *)z;

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

static void hslider_size(t_hslider *x, t_symbol *s, int ac, t_atom *av)
{
    hslider_check_width(x, (int)atom_getintarg(0, ac, av));
    if(ac > 1)
        x->x_gui.x_h = iemgui_clip_size((int)atom_getintarg(1, ac, av));
    iemgui_size(&x->x_gui);
}

static void hslider_range(t_hslider *x, t_symbol *s, int ac, t_atom *av)
{
    hslider_check_minmax(x, (double)atom_getfloatarg(0, ac, av),
                         (double)atom_getfloatarg(1, ac, av));
}

static void hslider_log(t_hslider *x)
{
    x->x_lin0_log1 = 1;
    hslider_check_minmax(x, x->x_min, x->x_max);
}

static void hslider_lin(t_hslider *x)
{
    x->x_lin0_log1 = 0;
    x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_w - 1);
}

static void hslider_init(t_hslider *x, t_floatarg f)
{
    x->x_gui.x_loadinit = (f==0.0)?0:1;
}

static void hslider_steady(t_hslider *x, t_floatarg f)
{
    x->x_steady = (f==0.0)?0:1;
}

static void hslider_float(t_hslider *x, t_floatarg f)
{
    double out;
    x->x_is_last_float = 1;
    x->x_last = f;

    hslider_set(x, f);
    if(x->x_lin0_log1)
        out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else
        if (f <= x->x_max && f >= x->x_min)
        {
            out = f;
            //x->x_val = f;
        } else
            out = (double)(x->x_val)*0.01*x->x_k + x->x_min;
    if((out < 1.0e-10)&&(out > -1.0e-10))
        out = 0.0;
    if(x->x_gui.x_put_in2out)
    {
        outlet_float(x->x_gui.x_obj.ob_outlet, out);
        if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
            pd_float(x->x_gui.x_snd->s_thing, out);
    }
}

static void hslider_loadbang(t_hslider *x)
{
    if(!sys_noloadbang && x->x_gui.x_loadinit)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        hslider_bang(x);
    }
}

static void *hslider_new(t_symbol *s, int argc, t_atom *argv)
{
    t_hslider *x = (t_hslider *)pd_new(hslider_class);
    int bflcol[]={-262144, -1, -1};
    int w=IEM_SL_DEFAULTSIZE, h=IEM_GUI_DEFAULTSIZE;
    int lilo=0, ldx=-2, ldy=-8, v=0, steady=1;
    int fs=10;
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

    x->x_gui.x_draw = (t_iemfunptr)hslider_draw;

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
    x->x_gui.x_h = iemgui_clip_size(h);
    hslider_check_width(x, w);
    hslider_check_minmax(x, min, max);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    x->x_thick = 0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    outlet_new(&x->x_gui.x_obj, &s_float);

    x->x_gui. x_handle = scalehandle_new(scalehandle_class,(t_iemgui *)x,1);
    x->x_gui.x_lhandle = scalehandle_new(scalehandle_class,(t_iemgui *)x,0);
    x->x_gui.x_obj.te_iemgui = 1;
    x->x_gui.x_changed = 0;

    return (x);
}

static void hslider_free(t_hslider *x)
{
    if(iemgui_has_rcv(&x->x_gui))
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);

    if (x->x_gui. x_handle) scalehandle_free(x->x_gui. x_handle);
    if (x->x_gui.x_lhandle) scalehandle_free(x->x_gui.x_lhandle);
}

void g_hslider_setup(void)
{
    hslider_class = class_new(gensym("hsl"), (t_newmethod)hslider_new,
        (t_method)hslider_free, sizeof(t_hslider), 0, A_GIMME, 0);
#ifndef GGEE_HSLIDER_COMPATIBLE
    class_addcreator((t_newmethod)hslider_new, gensym("hslider"), A_GIMME, 0);
#endif
    class_addbang(hslider_class,hslider_bang);
    class_addfloat(hslider_class,hslider_float);
    class_addmethod(hslider_class, (t_method)hslider_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(hslider_class, (t_method)hslider_motion, gensym("motion"),
                    A_FLOAT, A_FLOAT, 0);
    class_addmethod(hslider_class, (t_method)hslider_dialog,
        gensym("dialog"), A_GIMME, 0);
    class_addmethod(hslider_class, (t_method)hslider_loadbang,
        gensym("loadbang"), 0);
    class_addmethod(hslider_class, (t_method)hslider_set,
        gensym("set"), A_FLOAT, 0);
    class_addmethod(hslider_class, (t_method)hslider_size,
        gensym("size"), A_GIMME, 0);
    iemgui_class_addmethods(hslider_class);
    class_addmethod(hslider_class, (t_method)hslider_range,
        gensym("range"), A_GIMME, 0);
    class_addmethod(hslider_class, (t_method)hslider_log, gensym("log"), 0);
    class_addmethod(hslider_class, (t_method)hslider_lin, gensym("lin"), 0);
    class_addmethod(hslider_class, (t_method)hslider_init,
        gensym("init"), A_FLOAT, 0);
    class_addmethod(hslider_class, (t_method)hslider_steady,
        gensym("steady"), A_FLOAT, 0);
 
    scalehandle_class = class_new(gensym("_scalehandle"), 0, 0,
                  sizeof(t_scalehandle), CLASS_PD, 0);
    class_addmethod(scalehandle_class, (t_method)hslider__clickhook,
            gensym("_click"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scalehandle_class, (t_method)hslider__motionhook,
            gensym("_motion"), A_FLOAT, A_FLOAT, 0);

    wb_init(&hslider_widgetbehavior,hslider_getrect,hslider_newclick);
    class_setwidget(hslider_class, &hslider_widgetbehavior);
    class_sethelpsymbol(hslider_class, gensym("hslider"));
    class_setsavefn(hslider_class, hslider_save);
    class_setpropertiesfn(hslider_class, hslider_properties);
}
