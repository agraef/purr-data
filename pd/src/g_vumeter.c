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
static void vu_draw_select(t_vu* x, t_glist* glist);
void vu_check_height(t_vu *x, int h);

/* ----- vu  gui-peak- & rms- vu-meter-display ---------- */

t_widgetbehavior vu_widgetbehavior;
static t_class *vu_class;

/* widget helper functions */

static void vu_update_rms(t_vu *x, t_glist *glist)
{
    if (glist_isvisible(glist))
    {
        int w4 = x->x_gui.x_w / 4, off=text_ypix(&x->x_gui.x_obj, glist) - 1;
        int xpos = text_xpix(&x->x_gui.x_obj, glist),
            quad1 = xpos + w4 + 1, quad3 = xpos + x->x_gui.x_w-w4 - 1;

        sys_vgui(".x%lx.c coords %lxRCOVER %d %d %d %d\n",
            glist_getcanvas(glist), x, quad1 + 1, off + 2, quad3 + 1,
            off + (x->x_led_size + 1) * (IEM_VU_STEPS - x->x_rms) + 2);
    }
}

static void vu_update_peak(t_vu *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    if(glist_isvisible(glist))
    {
        int xpos = text_xpix(&x->x_gui.x_obj, glist);
        int ypos = text_ypix(&x->x_gui.x_obj, glist);

        if(x->x_peak)
        {
            int i = iemgui_vu_col[x->x_peak];
            int j = ypos + (x->x_led_size + 1) * (IEM_VU_STEPS + 1 - x->x_peak)
                - (x->x_led_size + 1) / 2;

            sys_vgui(".x%lx.c coords %lxPLED %d %d %d %d\n",
                canvas, x, xpos + 1, j + 2, xpos + x->x_gui.x_w + 2, j + 2);
            sys_vgui(".x%lx.c itemconfigure %lxPLED -fill #%6.6x\n",
                canvas, x, iemgui_color_hex[i]);
        }
        else
        {
            int mid = xpos + x->x_gui.x_w / 2;

            sys_vgui(".x%lx.c itemconfigure %lxPLED -fill #%6.6x\n",
                     canvas, x, x->x_gui.x_bcol);
            sys_vgui(".x%lx.c coords %lxPLED %d %d %d %d\n",
                     canvas, x, mid+1, ypos+22, mid+1, ypos+22);
        }
    }
}

static void vu_draw_update(t_gobj *client, t_glist *glist)
{
    t_vu *x = (t_vu *)client;
    if (x->x_updaterms)
    {
        vu_update_rms(x, glist);
        x->x_updaterms = 0;
    }
    if (x->x_updatepeak)
    {
        vu_update_peak(x, glist);
        x->x_updatepeak = 0;
    }
}
    
static void vu_draw_new(t_vu *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int w4=x->x_gui.x_w/4, mid=xpos+x->x_gui.x_w/2,
        quad1=xpos+w4+1;
    int quad3=xpos+x->x_gui.x_w-w4,
        end=xpos+x->x_gui.x_w+4;
    int k1=x->x_led_size+1, k2=IEM_VU_STEPS+1, k3=k1/2;
    int led_col, yyy, i, k4=ypos-k3;

    scalehandle_draw_new(x->x_gui. x_handle,canvas);
    scalehandle_draw_new(x->x_gui.x_lhandle,canvas);

        char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

        sys_vgui(".x%lx.c create prect %d %d %d %d "
                 "-stroke $pd_colors(iemgui_border) -fill #%6.6x "
                 "-tags {%lxBASE %lxVU %s text iemgui border}\n",
            canvas, xpos, ypos, xpos+x->x_gui.x_w+2,
            ypos+x->x_gui.x_h+4, x->x_gui.x_bcol, x, x, nlet_tag);
        for(i = 1; i <= IEM_VU_STEPS; i++)
        {
            led_col = iemgui_vu_col[i];
            yyy = k4 + k1 * (k2-i);
            sys_vgui(".x%lx.c create polyline %d %d %d %d "
                     "-strokewidth %d -stroke #%6.6x "
                     "-tags {%lxRLED%d %lxVU %s text iemgui}\n",
                canvas, quad1+1, yyy+2, quad3, yyy+2,
                x->x_led_size, iemgui_color_hex[led_col], x, i, x, nlet_tag);
            if(((i+2) & 3) && (x->x_scale))
                sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w "
                         "-font {{%s} -%d %s} -fill #%6.6x "
                         "-tags {%lxSCALE%d %lxVU %s text iemgui}\n",
                    canvas, end+1, yyy+k3+2, iemgui_vu_scale_str[i], 
                    iemgui_font(&x->x_gui), x->x_gui.x_fontsize,
                    sys_fontweight, x->x_gui.x_lcol, x, i, x, nlet_tag);
        }
        if(x->x_scale)
        {
            i=IEM_VU_STEPS+1;
            yyy = k4 + k1*(k2-i);
            sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w "
                     "-font {{%s} -%d %s} -fill #%6.6x "
                     "-tags {%lxSCALE%d %lxVU %s text iemgui}\n",
                canvas, end+1, yyy+k3+2, iemgui_vu_scale_str[i],
                iemgui_font(&x->x_gui), x->x_gui.x_fontsize, sys_fontweight,
                x->x_gui.x_lcol, x, i, x, nlet_tag);
        }
        sys_vgui(".x%lx.c create prect %d %d %d %d -fill #%6.6x "
                 "-stroke #%6.6x -tags {%lxRCOVER %lxVU %s text iemgui}\n",
            canvas, quad1+1, ypos+1, quad3,
            ypos+1 + k1*IEM_VU_STEPS, x->x_gui.x_bcol, x->x_gui.x_bcol,
            x, x, nlet_tag);
        sys_vgui(".x%lx.c create polyline %d %d %d %d "
                 "-strokewidth %d -fill #%6.6x "
                 "-tags {%lxPLED %lxVU %s text iemgui}\n",
            canvas, mid+1, ypos+12,
            mid+1, ypos+12, x->x_led_size, x->x_gui.x_bcol, x, x, nlet_tag);
        iemgui_label_draw_new(&x->x_gui,canvas,xpos,ypos,nlet_tag,"VU");
        if (!iemgui_has_snd(&x->x_gui) && canvas == x->x_gui.x_glist)
        {
            sys_vgui(".x%lx.c create prect %d %d %d %d "
                     "-stroke $pd_colors(iemgui_nlet) "
                     "-tags {%lxVU%so%d %so%d %lxVU %s outlet iemgui}\n",
                canvas,
                xpos, ypos + x->x_gui.x_h+3,
                xpos + IOWIDTH, ypos + x->x_gui.x_h+4,
                x, nlet_tag, 0, nlet_tag, 0, x, nlet_tag);
            sys_vgui(".x%lx.c create prect %d %d %d %d "
                     "-stroke $pd_colors(iemgui_nlet) "
                     "-tags {%lxVU%so%d %so%d %lxVU %s outlet iemgui}\n",
                canvas,
                xpos+x->x_gui.x_w+2-IOWIDTH, ypos + x->x_gui.x_h+3,
                xpos+x->x_gui.x_w+2, ypos + x->x_gui.x_h+4,
                x, nlet_tag, 1, nlet_tag, 1, x, nlet_tag);
        }
        if (!iemgui_has_rcv(&x->x_gui) && canvas == x->x_gui.x_glist)
        {
            sys_vgui(".x%lx.c create prect %d %d %d %d "
                     "-stroke $pd_colors(iemgui_nlet) "
                     "-tags {%lxVU%si%d %si%d %lxVU %s inlet iemgui}\n",
                canvas,
                xpos, ypos,
                xpos + IOWIDTH, ypos+1,
                x, nlet_tag, 0, nlet_tag, 0, x, nlet_tag);
            sys_vgui(".x%lx.c create prect %d %d %d %d "
                     "-stroke $pd_colors(iemgui_nlet) "
                     "-tags {%lxVU%si%d %si%d %lxVU %s inlet iemgui}\n",
                canvas,
                xpos+x->x_gui.x_w+2-IOWIDTH, ypos,
                xpos+x->x_gui.x_w+2, ypos+1,
                x, nlet_tag, 1, nlet_tag, 1, x, nlet_tag);
        }
        x->x_updaterms = x->x_updatepeak = 1;
        sys_queuegui(x, x->x_gui.x_glist, vu_draw_update);
    //}
}


static void vu_draw_move(t_vu *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    if (glist_isvisible(canvas))
    {

        char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

        int xpos=text_xpix(&x->x_gui.x_obj, glist);
        int ypos=text_ypix(&x->x_gui.x_obj, glist);
        int w4=x->x_gui.x_w/4, quad1=xpos+w4+1;
        int quad3=xpos+x->x_gui.x_w-w4,
            end=xpos+x->x_gui.x_w+4;
        int k1=x->x_led_size+1, k2=IEM_VU_STEPS+1, k3=k1/2;
        int yyy, i, k4=ypos-k3;

        sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n",
                 canvas, x, xpos, ypos,
                 xpos+x->x_gui.x_w+2,ypos+x->x_gui.x_h+4);
        for(i=1; i<=IEM_VU_STEPS; i++)
        {
            yyy = k4 + k1*(k2-i);
            sys_vgui(".x%lx.c coords %lxRLED%d %d %d %d %d\n",
                     canvas, x, i, quad1+1, yyy+2, quad3, yyy+2);
            if(((i+2)&3) && (x->x_scale))
                sys_vgui(".x%lx.c coords %lxSCALE%d %d %d\n",
                         canvas, x, i, end+1, yyy+k3+2);
        }
        if(x->x_scale)
        {
            i=IEM_VU_STEPS+1;
            yyy = k4 + k1*(k2-i);
            sys_vgui(".x%lx.c coords %lxSCALE%d %d %d\n",
                     canvas, x, i, end+1, yyy+k3+2);
        }
        x->x_updaterms = x->x_updatepeak = 1;
        sys_queuegui(x, glist, vu_draw_update);
        iemgui_label_draw_move(&x->x_gui,canvas,xpos,ypos);
        if(!iemgui_has_snd(&x->x_gui) && canvas == x->x_gui.x_glist)
        {
            sys_vgui(".x%lx.c coords %lxVU%so%d %d %d %d %d\n",
                 canvas, x, nlet_tag, 0,
                 xpos, ypos + x->x_gui.x_h+3,
                 xpos + IOWIDTH, ypos + x->x_gui.x_h+4);
            sys_vgui(".x%lx.c coords %lxVU%so%d %d %d %d %d\n",
                 canvas, x, nlet_tag, 1,
                 xpos+x->x_gui.x_w+2-IOWIDTH, ypos + x->x_gui.x_h+3,
                     xpos+x->x_gui.x_w+2, ypos + x->x_gui.x_h+4);
        }
        if(!iemgui_has_rcv(&x->x_gui) && canvas == x->x_gui.x_glist)
        {
        sys_vgui(".x%lx.c coords %lxVU%si%d %d %d %d %d\n",
                 canvas, x, nlet_tag, 0,
                 xpos, ypos,
                 xpos + IOWIDTH, ypos+1);
        sys_vgui(".x%lx.c coords %lxVU%si%d %d %d %d %d\n",
                 canvas, x, nlet_tag, 1,
                 xpos+x->x_gui.x_w+2-IOWIDTH, ypos,
                 xpos+x->x_gui.x_w+2, ypos+1);
        }
        /* redraw scale handle rectangle if selected */
        if (x->x_gui.x_selected)
            vu_draw_select(x, x->x_gui.x_glist);
    }
}

static void vu_draw_config(t_vu* x, t_glist* glist)
{
    int i;
    t_canvas *canvas=glist_getcanvas(glist);
    for(i = 1; i <= IEM_VU_STEPS; i++)
    {
        sys_vgui(".x%lx.c itemconfigure %lxRLED%d -strokewidth %d\n",
            canvas, x, i, x->x_led_size);
        if(((i + 2) & 3) && (x->x_scale))
            if (x->x_gui.x_selected)
                sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -text {%s} "
                         "-font {{%s} -%d %s} -fill $pd_colors(selection)\n",
                     canvas, x, i, iemgui_vu_scale_str[i], iemgui_font(&x->x_gui), 
                     x->x_gui.x_fontsize, sys_fontweight);
            else
                sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -text {%s} "
                         "-font {{%s} -%d %s} -fill #%6.6x\n",
                     canvas, x, i, iemgui_vu_scale_str[i], iemgui_font(&x->x_gui), 
                     x->x_gui.x_fontsize, sys_fontweight, 
                     x->x_gui.x_lcol);
    }
    if(x->x_scale)
    {
        i = IEM_VU_STEPS + 1;
        if (x->x_gui.x_selected)
            sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -text {%s} "
                     "-font {{%s} -%d %s} -fill $pd_colors(selection)\n",
                canvas, x, i, iemgui_vu_scale_str[i], iemgui_font(&x->x_gui), 
                x->x_gui.x_fontsize, sys_fontweight);
        else
            sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -text {%s} "
                     "-font {{%s} -%d %s} -fill #%6.6x\n",
                canvas, x, i, iemgui_vu_scale_str[i], iemgui_font(&x->x_gui), 
                x->x_gui.x_fontsize, sys_fontweight,
                x->x_gui.x_lcol);
    }
    iemgui_label_draw_config(&x->x_gui,canvas);
    sys_vgui(".x%lx.c itemconfigure %lxRCOVER -fill #%6.6x -stroke #%6.6x\n"
             ".x%lx.c itemconfigure %lxPLED -strokewidth %d\n"
             ".x%lx.c itemconfigure %lxBASE -fill #%6.6x\n",
             canvas, x, x->x_gui.x_bcol, x->x_gui.x_bcol,
             canvas, x, x->x_led_size,
             canvas, x, x->x_gui.x_bcol);
}

static void vu_draw_io(t_vu* x, t_glist* glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    if (glist_isvisible(canvas) && canvas == x->x_gui.x_glist)
    {

        char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

        if ((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) &&
            !iemgui_has_snd(&x->x_gui))
        {
            sys_vgui(".x%lx.c create prect %d %d %d %d "
                     "-stroke $pd_colors(iemgui_nlet) "
                     "-tags {%lxVU%so%d %so%d %lxVU %s outlet}\n",
                 canvas, xpos, ypos + x->x_gui.x_h+3, xpos + IOWIDTH,
                 ypos + x->x_gui.x_h+4,
                 x, nlet_tag, 0, nlet_tag, 0, x, nlet_tag);
            sys_vgui(".x%lx.c create prect %d %d %d %d "
                     "-stroke $pd_colors(iemgui_nlet) "
                     "-tags {%lxVU%so%d %so%d %lxVU %s outlet}\n",
                 canvas,
                 xpos+x->x_gui.x_w+2-IOWIDTH, ypos + x->x_gui.x_h+3,
                 xpos+x->x_gui.x_w+2, ypos + x->x_gui.x_h+4,
                 x, nlet_tag, 1, nlet_tag, 1, x, nlet_tag);
        }
        if (!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) &&
            iemgui_has_snd(&x->x_gui))
        {
            sys_vgui(".x%lx.c delete %lxVU%so%d\n", canvas, x, nlet_tag, 0);
            sys_vgui(".x%lx.c delete %lxVU%so%d\n", canvas, x, nlet_tag, 1);
        }
        if ((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) &&
            !iemgui_has_rcv(&x->x_gui))
        {
            sys_vgui(".x%lx.c create prect %d %d %d %d "
                     "-tags {%lxVU%si%d %si%d %lxVU %s outlet}\n",
                 canvas,
                 xpos, ypos,
                 xpos + IOWIDTH, ypos+1,
                 x, nlet_tag, 0, nlet_tag, 0, x, nlet_tag);
            sys_vgui(".x%lx.c create prect %d %d %d %d "
                     "-tags {%lxVU%si%d %si%d %lxVU %s outlet}\n",
                 canvas,
                 xpos+x->x_gui.x_w+2-IOWIDTH, ypos,
                 xpos+x->x_gui.x_w+2, ypos+1,
                 x, nlet_tag, 1, nlet_tag, 1, x, nlet_tag);
        }
        if (!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) &&
            iemgui_has_rcv(&x->x_gui))
        {
            sys_vgui(".x%lx.c delete %lxVU%si%d\n", canvas, x, nlet_tag, 0);
            sys_vgui(".x%lx.c delete %lxVU%si%d\n", canvas, x, nlet_tag, 1);
        }
    }
}

static void vu_draw_select(t_vu* x,t_glist* glist)
{
    int i;
    t_canvas *canvas=glist_getcanvas(glist);
    if(x->x_gui.x_selected)
    {
        // check if we are drawing inside a gop abstraction
        // visible on parent canvas. If so, disable highlighting
        if (x->x_gui.x_glist == glist_getcanvas(glist))
        {
            sys_vgui(".x%lx.c itemconfigure %lxBASE "
                     "-stroke $pd_colors(selection)\n", canvas, x);
            for(i = 1; i <= IEM_VU_STEPS; i++)
            {
                if(((i + 2) & 3) && (x->x_scale))
                    sys_vgui(".x%lx.c itemconfigure %lxSCALE%d "
                        "-fill $pd_colors(selection)\n", canvas, x, i);
            }
            if(x->x_scale)
            {
                i=IEM_VU_STEPS+1;
                sys_vgui(".x%lx.c itemconfigure %lxSCALE%d "
                         "-fill $pd_colors(selection)\n", canvas, x, i);
            }
            scalehandle_draw_select2(&x->x_gui,glist,"VU",
                x->x_gui.x_w+2-1,x->x_gui.x_h+4-1);
        }
    }
    else
    {
        sys_vgui(".x%lx.c itemconfigure %lxBASE -stroke %s\n",
            canvas, x, IEM_GUI_COLOR_NORMAL);
        for(i = 1; i <= IEM_VU_STEPS; i++)
        {
            if(((i + 2) & 3) && (x->x_scale))
                sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -fill #%6.6x\n",
                    canvas, x, i, x->x_gui.x_lcol);
        }
        if(x->x_scale)
        {
            i = IEM_VU_STEPS + 1;
            sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -fill #%6.6x\n",
                canvas, x, i, x->x_gui.x_lcol);
        }
        scalehandle_draw_erase2(&x->x_gui,glist);
    }
    iemgui_label_draw_select(&x->x_gui,canvas);
    iemgui_tag_selected(&x->x_gui,canvas,"VU");
}

static void vu__clickhook(t_scalehandle *sh, t_floatarg f, t_floatarg xxx,
    t_floatarg yyy)
{
    t_vu *x = (t_vu *)(sh->h_master);
    int newstate = (int)f;
    if (sh->h_dragon && newstate == 0  && sh->h_scale)
    {
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
        if (sh->h_dragx || sh->h_dragy)
        {
            x->x_gui.x_w += sh->h_dragx;
            x->x_gui.x_h += sh->h_dragy;
            canvas_dirty(x->x_gui.x_glist, 1);
        }
        if (glist_isvisible(x->x_gui.x_glist))
        {
            vu_check_height(x, x->x_gui.x_h);
            vu_draw_move(x, x->x_gui.x_glist);
            vu_draw_config(x, x->x_gui.x_glist);
            vu_draw_update((t_gobj *)x, x->x_gui.x_glist);
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

static void vu__motionhook(t_scalehandle *sh, t_floatarg f1, t_floatarg f2)
{
    if (sh->h_dragon && sh->h_scale)
    {
        t_vu *x = (t_vu *)(sh->h_master);
        int dx = (int)f1, dy = (int)f2;
        dx = maxi(dx, 8-x->x_gui.x_w);
        dy = maxi(dy,80-x->x_gui.x_h);        
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

void vu_draw(t_vu *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui((t_gobj*)x, x->x_gui.x_glist, vu_draw_update);
    if(mode == IEM_GUI_DRAW_MODE_MOVE)
        vu_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
    {
        vu_draw_new(x, glist);
        sys_vgui(".x%lx.c raise all_cords\n", glist_getcanvas(glist));
    }
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        vu_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        iemgui_draw_erase(&x->x_gui, glist, "VU");
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        vu_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        vu_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ vu widgetbehaviour----------------------------- */

static void vu_scale_getrect(t_iemgui x_gui, t_glist *x, int *xp1, int *yp1,
    int *xp2, int *yp2, int scale_x, int scale_y)
{
    t_float width_multiplier;
    int scale_length;    
    int scale_x1;
    int scale_y1;
    int scale_x2;
    int scale_y2;
    int actual_fontsize; //seems tk does its own thing when it comes to rendering
    int actual_height;

    if (x->gl_isgraph && !glist_istoplevel(x))
    {
        //fprintf(stderr,"vu_scale_getrect\n");

        switch(x_gui.x_font_style)
        {
            case 1:
                width_multiplier = 0.83333;
                break;
            case 2:
                width_multiplier = 0.735;
                break;
            default:
                width_multiplier = 1.0;
                break;
        }
        if (x_gui.x_fontsize % 2 == 0)
        {
            actual_fontsize = x_gui.x_fontsize;
        }
        else
        {
            actual_fontsize = x_gui.x_fontsize;
        }
        actual_height = actual_fontsize;
        //exceptions
        if (x_gui.x_font_style == 0 &&
            (actual_fontsize == 8 || actual_fontsize == 13 ||
             actual_fontsize % 10 == 1 || actual_fontsize % 10 == 6 ||
             (actual_fontsize > 48 && actual_fontsize < 100 &&
              (actual_fontsize %10 == 4 || actual_fontsize %10 == 9))) )
        {
            actual_fontsize += 1;
        }
        else if (x_gui.x_font_style == 1 &&
            actual_fontsize >= 5 &&
            actual_fontsize < 13 &&
            actual_fontsize % 2 == 1)
        {
            actual_fontsize += 1;
        }
        else if (x_gui.x_font_style == 2 &&
            actual_fontsize >= 5 &&
            actual_fontsize % 2 == 1)
        {
            actual_fontsize += 1;
        }
        if (actual_height == 9)
            actual_height += 1;
        //done with exceptions

        width_multiplier = width_multiplier * (actual_fontsize * 0.6);

        scale_length = 4;
        scale_x1 = scale_x;
        scale_y1 = scale_y - actual_height/2;
        scale_x2 = scale_x1 + (scale_length * width_multiplier);
        scale_y2 = scale_y1 + actual_height*1.1;

        //DEBUG
        //fprintf(stderr,"%f %d %d\n",
        //    width_multiplier, scale_length, x_gui.x_font_style);
        //sys_vgui(".x%lx.c delete iemguiDEBUG\n", x);
        //sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags iemguiDEBUG\n",
        //    x, scale_x1, scale_y1, scale_x2, scale_y2);
        if (scale_x1 < *xp1) *xp1 = scale_x1;
        if (scale_x2 > *xp2) *xp2 = scale_x2;
        if (scale_y1 < *yp1) *yp1 = scale_y1;
        if (scale_y2 > *yp2) *yp2 = scale_y2;
        //DEBUG
        //sys_vgui(".x%lx.c delete iemguiDEBUG\n", glist_getcanvas(x));
        //sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags iemguiDEBUG\n",
        //    glist_getcanvas(x), *xp1, *yp1, *xp2, *yp2);
    }
}

static void vu_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_vu* x = (t_vu*)z;

    int yyy, end;
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int k1=x->x_led_size+1, k2=IEM_VU_STEPS+1, k3=k1/2, k4=ypos-k3;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist) + 2;
    *xp2 = *xp1 + x->x_gui.x_w + 2;
    *yp2 = *yp1 + x->x_gui.x_h + 2;

    iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);

    if (x->x_scale)
    {
        //vu has custom scale all labels unlike other iemgui object
        end=xpos+x->x_gui.x_w+4;
        yyy = k4 + k1*(k2-1);
        vu_scale_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2, end+1, yyy+k3+2);
        yyy = k4;
        vu_scale_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2, end+1, yyy+k3+2);
    }
}

static void vu_save(t_gobj *z, t_binbuf *b)
{
    t_vu *x = (t_vu *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiissiiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("vu"), x->x_gui.x_w, x->x_gui.x_h,
                srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui), x->x_gui.x_fontsize,
                bflcol[0], bflcol[2], x->x_scale,
                iem_symargstoint(&x->x_gui));
    binbuf_addv(b, ";");
}

void vu_check_height(t_vu *x, int h)
{
    int n = maxi(h / IEM_VU_STEPS, IEM_VU_MINSIZE);
    x->x_led_size = n-1;
    x->x_gui.x_h = IEM_VU_STEPS * n;
}

static void vu_scale(t_vu *x, t_floatarg fscale)
{
    int i, scale = (int)fscale;

    if(scale != 0) scale = 1;
    if(x->x_scale && !scale)
    {
        t_canvas *canvas=glist_getcanvas(x->x_gui.x_glist);

        x->x_scale = (int)scale;
        if(glist_isvisible(x->x_gui.x_glist))
        {
            for(i=1; i<=IEM_VU_STEPS; i++)
            {
                if((i+2)&3)
                    sys_vgui(".x%lx.c delete %lxSCALE%d\n", canvas, x, i);
            }
            i=IEM_VU_STEPS+1;
            sys_vgui(".x%lx.c delete %lxSCALE%d\n", canvas, x, i);
        }
    }
    if(!x->x_scale && scale)
    {
        int end=text_xpix(&x->x_gui.x_obj,
            x->x_gui.x_glist)+x->x_gui.x_w+4;
        int k1=x->x_led_size+1, k2=IEM_VU_STEPS+1, k3=k1/2;
        int yyy, k4=text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist)-k3;
        t_canvas *canvas=glist_getcanvas(x->x_gui.x_glist);

        x->x_scale = (int)scale;
        if(glist_isvisible(x->x_gui.x_glist))
        {
            for(i=1; i<=IEM_VU_STEPS; i++)
            {
                yyy = k4 + k1*(k2-i);
                if((i + 2) & 3)
                    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w "
                             "-font {{%s} -%d %s} -fill #%6.6x "
                             "-tags {%lxSCALE%d %lxVU %lx}\n",
                        canvas, end+1, yyy+k3+2, iemgui_vu_scale_str[i], 
                        iemgui_font(&x->x_gui), x->x_gui.x_fontsize,
                        sys_fontweight, x->x_gui.x_lcol, x, i, x, x);
            }
            i = IEM_VU_STEPS + 1;
            yyy = k4 + k1*(k2-i);
            sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w "
                     "-font {{%s} -%d %s} -fill #%6.6x "
                     "-tags {%lxSCALE%d %lxVU %lx}\n",
                canvas, end+1, yyy+k3+2, iemgui_vu_scale_str[i], 
                iemgui_font(&x->x_gui), x->x_gui.x_fontsize,
                sys_fontweight, x->x_gui.x_lcol, x, i, x, x);
        }
    }
}

static void vu_properties(t_gobj *z, t_glist *owner)
{
    t_vu *x = (t_vu *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |vu| \
            --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
            empty 0.0 empty 0.0 empty %d \
            %d no_scale scale %d %d empty %d \
            {%s} {%s} \
            {%s} %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, IEM_GUI_MINSIZE, x->x_gui.x_h,
            IEM_VU_STEPS*IEM_VU_MINSIZE,
            0,/*no_schedule*/
            x->x_scale, -1, -1, -1,/*no linlog, no init, no multi*/
            "nosndno", srl[1]->s_name,/*no send*/
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, -1/*no front-color*/,
            0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void vu_dialog(t_vu *x, t_symbol *s, int argc, t_atom *argv)
{
    canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    int scale = (int)atom_getintarg(4, argc, argv);
    int sr_flags;

    srl[0] = s_empty;
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    x->x_gui.x_loadinit = 0;
    x->x_gui.x_w = iemgui_clip_size(w);
    vu_check_height(x, h);
    if(scale != 0)
        scale = 1;
    vu_scale(x, (t_float)scale);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    //(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    //canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
    iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_MOVE);

    /* forcing redraw of the scale handle */
    if (x->x_gui.x_selected)
    {
        vu_draw_select(x, x->x_gui.x_glist);
    }

    // ico@bukvic.net 100518
    // update scrollbars when object potentially exceeds window size
    t_canvas *canvas=(t_canvas *)glist_getcanvas(x->x_gui.x_glist);
    sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", (long unsigned int)canvas);
}

static void vu_size(t_vu *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    if(ac > 1)
        vu_check_height(x, (int)atom_getintarg(1, ac, av));
    if(glist_isvisible(x->x_gui.x_glist))
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
        canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
    }
}

static void vu_float(t_vu *x, t_floatarg rms)
{
    int i;

    if(rms <= IEM_VU_MINDB)
        x->x_rms = 0;
    else if(rms >= IEM_VU_MAXDB)
        x->x_rms = IEM_VU_STEPS;
    else
    {
        int i = (int)(2.0*(rms + IEM_VU_OFFSET));
        x->x_rms = iemgui_vu_db2i[i];
    }
    i = (int)(100.0*rms + 10000.5);
    rms = 0.01*(t_float)(i - 10000);
    x->x_fr = rms;
    outlet_float(x->x_out_rms, rms);
    x->x_updaterms = 1;
    sys_queuegui(x, x->x_gui.x_glist, vu_draw_update);
}

static void vu_ft1(t_vu *x, t_floatarg peak)
{
    int i;

    if(peak <= IEM_VU_MINDB)
        x->x_peak = 0;
    else if(peak >= IEM_VU_MAXDB)
        x->x_peak = IEM_VU_STEPS;
    else
    {
        int i = (int)(2.0*(peak + IEM_VU_OFFSET));
        x->x_peak = iemgui_vu_db2i[i];
    }
    i = (int)(100.0*peak + 10000.5);
    peak = 0.01*(t_float)(i - 10000);
    x->x_fp = peak;
    x->x_updatepeak = 1;
    sys_queuegui(x, x->x_gui.x_glist, vu_draw_update);
    outlet_float(x->x_out_peak, peak);
}

static void vu_bang(t_vu *x)
{
    outlet_float(x->x_out_peak, x->x_fp);
    outlet_float(x->x_out_rms, x->x_fr);
    x->x_updaterms = x->x_updatepeak = 1;
    sys_queuegui(x, x->x_gui.x_glist, vu_draw_update);
}

static void *vu_new(t_symbol *s, int argc, t_atom *argv)
{
    t_vu *x = (t_vu *)pd_new(vu_class);
    int bflcol[] = {-66577, -1, -1};
    int w = IEM_GUI_DEFAULTSIZE, h = IEM_VU_STEPS*IEM_VU_DEFAULTSIZE;
    int ldx = -1, ldy = -8, fs = 10, scale = 1;

    iem_inttosymargs(&x->x_gui, 0);
    iem_inttofstyle(&x->x_gui, 0);

    if((argc >= 11)&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
       &&(IS_A_SYMBOL(argv,2)||IS_A_FLOAT(argv,2))
       &&(IS_A_SYMBOL(argv,3)||IS_A_FLOAT(argv,3))
       &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
       &&IS_A_FLOAT(argv,6)&&IS_A_FLOAT(argv,7)
       &&IS_A_FLOAT(argv,8)&&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10))
    {
        w = (int)atom_getintarg(0, argc, argv);
        h = (int)atom_getintarg(1, argc, argv);
        iemgui_new_getnames(&x->x_gui, 1, argv);
        ldx = (int)atom_getintarg(4, argc, argv);
        ldy = (int)atom_getintarg(5, argc, argv);
        iem_inttofstyle(&x->x_gui, atom_getintarg(6, argc, argv));
        fs = (int)atom_getintarg(7, argc, argv);
        bflcol[0] = (int)atom_getintarg(8, argc, argv);
        bflcol[2] = (int)atom_getintarg(9, argc, argv);
        scale = (int)atom_getintarg(10, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 1, 0);
    if((argc == 12)&&IS_A_FLOAT(argv,11))
        iem_inttosymargs(&x->x_gui, atom_getintarg(11, argc, argv));
    x->x_gui.x_draw = (t_iemfunptr)vu_draw;

    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if (x->x_gui.x_font_style<0 || x->x_gui.x_font_style>2) x->x_gui.x_font_style=0;
    if(iemgui_has_rcv(&x->x_gui))
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;

    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = iemgui_clip_size(w);
    vu_check_height(x, h);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    if(scale != 0)
        scale = 1;
    x->x_scale = scale;
    x->x_peak = 0;
    x->x_rms = 0;
    x->x_fp = -101.0;
    x->x_fr = -101.0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    inlet_new(&x->x_gui.x_obj, &x->x_gui.x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_out_rms = outlet_new(&x->x_gui.x_obj, &s_float);
    x->x_out_peak = outlet_new(&x->x_gui.x_obj, &s_float);

    x->x_gui. x_handle = scalehandle_new(scalehandle_class,(t_iemgui *)x,1);
    x->x_gui.x_lhandle = scalehandle_new(scalehandle_class,(t_iemgui *)x,0);
    x->x_gui.x_obj.te_iemgui = 1;

    return (x);
}

static void vu_free(t_vu *x)
{
    if(iemgui_has_rcv(&x->x_gui))
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);

    if (x->x_gui. x_handle) scalehandle_free(x->x_gui. x_handle);
    if (x->x_gui.x_lhandle) scalehandle_free(x->x_gui.x_lhandle);
}

void g_vumeter_setup(void)
{
    vu_class = class_new(gensym("vu"), (t_newmethod)vu_new, (t_method)vu_free,
        sizeof(t_vu), 0, A_GIMME, 0);
    class_addbang(vu_class,vu_bang);
    class_addfloat(vu_class,vu_float);
    class_addmethod(vu_class, (t_method)vu_ft1, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(vu_class, (t_method)vu_dialog, gensym("dialog"),
        A_GIMME, 0);
    class_addmethod(vu_class, (t_method)vu_size, gensym("size"), A_GIMME, 0);
    class_addmethod(vu_class, (t_method)vu_scale,
        gensym("scale"), A_DEFFLOAT, 0);
    iemgui_class_addmethods(vu_class);
    scalehandle_class = class_new(gensym("_scalehandle"), 0, 0,
                  sizeof(t_scalehandle), CLASS_PD, 0);
    class_addmethod(scalehandle_class, (t_method)vu__clickhook,
            gensym("_click"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scalehandle_class, (t_method)vu__motionhook,
            gensym("_motion"), A_FLOAT, A_FLOAT, 0);

    wb_init(&vu_widgetbehavior,vu_getrect,0);
    class_setwidget(vu_class,&vu_widgetbehavior);
    class_sethelpsymbol(vu_class, gensym("vu"));
    class_setsavefn(vu_class, vu_save);
    class_setpropertiesfn(vu_class, vu_properties);
}
