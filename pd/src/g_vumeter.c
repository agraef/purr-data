/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "g_all_guis.h"
#include <math.h>

#define IEM_VU_DEFAULTSIZE 3
#define IEM_VU_LARGESMALL  2
#define IEM_VU_MINSIZE     2
#define IEM_VU_MAXSIZE     25
#define IEM_VU_STEPS       40
#define IEM_VU_MINDB    -99.9
#define IEM_VU_MAXDB    12.0
#define IEM_VU_OFFSET   100.0

char *iemgui_vu_scale_str[]={
    "<-99", "-50", "-30", "-20", "-12", "-6",
      "-2", "-0dB",  "+2",  "+6",">+12",  "",
};

int iemgui_vu_db2i[]=
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    9, 9, 9, 9, 9,10,10,10,10,10,
    11,11,11,11,11,12,12,12,12,12,
    13,13,13,13,14,14,14,14,15,15,
    15,15,16,16,16,16,17,17,17,18,
    18,18,19,19,19,20,20,20,21,21,
    22,22,23,23,24,24,25,26,27,28,
    29,30,31,32,33,33,34,34,35,35,
    36,36,37,37,37,38,38,38,39,39,
    39,39,39,39,40,40
};

int iemgui_vu_col[]=
{
    0,17,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
    15,15,15,15,15,15,15,15,15,15,14,14,13,13,13,13,13,13,13,13,13,13,13,19,19,19
};

extern int gfxstub_haveproperties(void *key);
void vu_check_height(t_vu *x, int h);
t_widgetbehavior vu_widgetbehavior;
t_class *vu_class;

static void vu_update_rms(t_vu *x, t_glist *glist)
{
    if (glist_isvisible(glist))
    {
        int w4 = x->x_gui.x_w / 4, off=text_ypix(&x->x_gui.x_obj, glist) - 1;
        int x1 = text_xpix(&x->x_gui.x_obj, glist),
            y1 = text_ypix(&x->x_gui.x_obj, glist),
            quad1 = x1 + w4 + 1, quad3 = x1 + x->x_gui.x_w-w4 - 1;

        gui_vmess("gui_vumeter_update_rms", "xxiiiiii",
            glist_getcanvas(glist), x,
            quad1 + 1, off + 2, quad3 + 1, off + (x->x_led_size + 1) *
            (IEM_VU_STEPS - x->x_rms) + 2, x1, y1);
    }
}

static void vu_update_peak(t_vu *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    if(glist_isvisible(glist))
    {
        int x1 = text_xpix(&x->x_gui.x_obj, glist);
        int y1 = text_ypix(&x->x_gui.x_obj, glist);

        if(x->x_peak)
        {
            int i = iemgui_vu_col[x->x_peak];
            int j = y1 + (x->x_led_size + 1) * (IEM_VU_STEPS + 1 - x->x_peak)
                - (x->x_led_size + 1) / 2;

            gui_vmess("gui_vumeter_update_peak", "xxxiiiiii",
                canvas, x, iemgui_color_hex[i],
                x1 + 1, j + 2, x1 + x->x_gui.x_w + 2, j + 2, x1, y1);
        }
        else
        {
            int mid = x1 + x->x_gui.x_w / 2;
            gui_vmess("gui_vumeter_update_peak", "xxxiiiiii",
                canvas, x, x->x_gui.x_bcol,
                mid+1, y1+22, mid+1, y1+22, x1, y1);
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
    int x1=text_xpix(&x->x_gui.x_obj, glist), x2=x1+x->x_gui.x_w;
    int y1=text_ypix(&x->x_gui.x_obj, glist);
    int w4=x->x_gui.x_w/4, mid=x1+x->x_gui.x_w/2, quad1=x1+w4+1;
    int quad3=x2-w4, end=x2+4;
    int k1=x->x_led_size+1, k2=IEM_VU_STEPS+1, k3=k1/2;
    int led_col, yyy, i, k4=y1-k3;
    iemgui_base_draw_new(&x->x_gui);

    for(i = 1; i <= IEM_VU_STEPS+1; i++)
    {
        yyy = k4 + k1 * (k2-i);
        if((i&3)==1 && (x->x_scale))
        {
            // not handling font size yet
            gui_vmess("gui_create_vumeter_text", "xxxiisiii",
                canvas, x,
                x->x_gui.x_lcol, end+1, yyy+k3+2, iemgui_vu_scale_str[i/4],
                i, x1, y1);
        }
        led_col = iemgui_vu_col[i];
        if (i<=IEM_VU_STEPS)
        {
            gui_vmess("gui_create_vumeter_steps", "xxxiiiiiiiii",
                canvas, x, iemgui_color_hex[led_col], quad1+1,
                yyy+2, quad3, yyy+2, x->x_led_size, index, x1, y1, i);
        }
    }
    gui_vmess("gui_create_vumeter_rect", "xxxiiiiii",
        canvas, x,
        x->x_gui.x_bcol, quad1+1, y1+1, quad3, y1+1 + k1*IEM_VU_STEPS, x1, y1);
    gui_vmess("gui_create_vumeter_peak", "xxxiiiiiii",
        canvas, x,
        x->x_gui.x_bcol, mid+1, y1+12, mid+1, y1+12, x->x_led_size, x1, y1);
    x->x_updaterms = x->x_updatepeak = 1;
    sys_queuegui(x, x->x_gui.x_glist, vu_draw_update);
}

static void vu_draw_move(t_vu *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    if (!glist_isvisible(canvas)) return;

    int x1=text_xpix(&x->x_gui.x_obj, glist);
    int y1=text_ypix(&x->x_gui.x_obj, glist);
    int w4=x->x_gui.x_w/4, quad1=x1+w4+1;
    int quad3=x1+x->x_gui.x_w-w4,
          end=x1+x->x_gui.x_w+4;
    int k1=x->x_led_size+1, k2=IEM_VU_STEPS+1, k3=k1/2;
    int yyy, i, k4=y1-k3;

    gui_vmess("gui_vumeter_border_size", "xxii",
        canvas, x, x->x_gui.x_w+2, x->x_gui.x_h+4);
    for(i=1; i<=IEM_VU_STEPS; i++)
    {
        yyy = k4 + k1*(k2-i);
        gui_vmess("gui_update_vumeter_step_coords", "xxiiiiiii",
            canvas, x, i, quad1+1, yyy+2, quad3, yyy+2,
            x1, y1);
        //if(((i+2)&3) && (x->x_scale))
        if((i&3)==1 && (x->x_scale))
        {
            gui_vmess("gui_vumeter_text_coords", "xxiiiii",
                canvas, x, i,
                end+1, yyy+k3+2, x1, y1);
        }
    }
    if(x->x_scale)
    {
        i=IEM_VU_STEPS+1;
        yyy = k4 + k1*(k2-i);
        gui_vmess("gui_vumeter_text_coords", "xxiiiii",
            canvas, x, i,
            end+1, yyy+k3+2, x1, y1);
    }
    x->x_updaterms = x->x_updatepeak = 1;
    sys_queuegui(x, glist, vu_draw_update);
}

static void vu_draw_config(t_vu* x, t_glist* glist)
{
    int i;
    t_canvas *canvas=glist_getcanvas(glist);
    for(i = 1; i <= IEM_VU_STEPS+1; i++)
    {
        if (i <= IEM_VU_STEPS)
        {
            gui_vmess("gui_update_vumeter_steps", "xxii",
                canvas, x, i, x->x_led_size);
        }
        //if((i&3)==1)
        if((i&3)==1)
        {
            int isselected = x->x_gui.x_selected == canvas &&
                x->x_gui.x_glist == canvas && x->x_scale;
            gui_vmess("gui_update_vumeter_text", "xxssixi",
                canvas, x, iemgui_vu_scale_str[i/4],
                iemgui_font(&x->x_gui), isselected, x->x_gui.x_lcol, i);
        }
    }
    gui_vmess("gui_update_vumeter_rect", "xxx",
        canvas, x, x->x_gui.x_bcol);
    gui_vmess("gui_update_vumeter_peak", "xxi",
        canvas, x, x->x_led_size);
    iemgui_base_draw_config(&x->x_gui);
}

static void vu_draw_select(t_vu* x,t_glist* glist)
{
    /* Not needed anymore */
}

static void vu__clickhook(t_scalehandle *sh, int newstate)
{
    t_vu *x = (t_vu *)(sh->h_master);
    if (newstate)
    {
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
        if (!sh->h_scale)
            scalehandle_click_label(sh);
    }
    sh->h_dragon = newstate;
}

static void vu__motionhook(t_scalehandle *sh, t_floatarg mouse_x, t_floatarg mouse_y)
{
    if (sh->h_scale)
    {
        t_vu *x = (t_vu *)(sh->h_master);
        int width = ((int)mouse_x) -
                text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist),
            height = ((int)mouse_y) -
                text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist);

        width = maxi(width, 8);

        sh->h_dragx = width - x->x_gui.x_w;

        scalehandle_drag_scale(sh);

        x->x_gui.x_w = width;
        vu_check_height(x, height);
        if (glist_isvisible(x->x_gui.x_glist))
        {
            /* draw_move doesn't seem to cut it for vu, so we
               just toggle visibility */
            //vu_draw_move(x, x->x_gui.x_glist);
            gobj_vis((t_gobj *)x, x->x_gui.x_glist, 0);
            gobj_vis((t_gobj *)x, x->x_gui.x_glist, 1);
            scalehandle_unclick_scale(sh);
        }

        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            int new_w = width;
            int new_h = x->x_gui.x_h + sh->h_dragy;
            properties_set_field_int(properties,"width",new_w);
            properties_set_field_int(properties,"height",new_h);
        }
    }
    scalehandle_dragon_label(sh,mouse_x, mouse_y);
}

void vu_draw(t_vu *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)      sys_queuegui((t_gobj*)x, x->x_gui.x_glist, vu_draw_update);
    if(mode == IEM_GUI_DRAW_MODE_MOVE)        vu_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)    vu_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT) vu_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG) vu_draw_config(x, glist);
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
        switch(x_gui.x_font_style)
        {
            case 1:   width_multiplier = 0.83333; break;
            case 2:   width_multiplier = 0.735;   break;
            default:  width_multiplier = 1.0;     break;
        }
        actual_fontsize = x_gui.x_fontsize;
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

        if (scale_x1 < *xp1) *xp1 = scale_x1;
        if (scale_x2 > *xp2) *xp2 = scale_x2;
        if (scale_y1 < *yp1) *yp1 = scale_y1;
        if (scale_y2 > *yp2) *yp2 = scale_y2;
    }
}

static void vu_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_vu* x = (t_vu*)z;

    int yyy, end;
    int x1=text_xpix(&x->x_gui.x_obj, glist);
    int y1=text_ypix(&x->x_gui.x_obj, glist);
    int k1=x->x_led_size+1, k2=IEM_VU_STEPS+1, k3=k1/2, k4=y1-k3;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w + 2;
    *yp2 = *yp1 + x->x_gui.x_h + 4;

    iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);

    /* In legacy mode we don't include the scale in the rect */
    if (x->x_scale && !sys_legacy)
    {
        //vu has custom scale all labels unlike other iemgui object
        end=x1+x->x_gui.x_w+4;
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
    binbuf_addv(b, "ssiisiissiiiiiiii;", gensym("#X"),gensym("obj"),
        (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
        gensym("vu"), x->x_gui.x_w, x->x_gui.x_h,
        srl[1], srl[2], x->x_gui.x_ldx, x->x_gui.x_ldy,
        iem_fstyletoint(&x->x_gui), x->x_gui.x_fontsize,
        bflcol[0], bflcol[2], x->x_scale,
        iem_symargstoint(&x->x_gui));
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
                /* if((i+2)&3) */
                if((i&3)==1)
                {
                    gui_vmess("gui_erase_vumeter_text", "xxi",
                        canvas, x, i);
                }
            }
            i=IEM_VU_STEPS+1;
            gui_vmess("gui_erase_vumeter_text", "xxi",
                canvas, x, i);
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
                if((i&3)==1)
                {
                    gui_vmess("gui_create_vumeter_text", "xxxiisiii",
                        canvas, x, x->x_gui.x_lcol,
                        end+1, yyy+k3+2, iemgui_vu_scale_str[i/4],
                        i, end, yyy);
                }
            }
            i = IEM_VU_STEPS + 1;
            yyy = k4 + k1*(k2-i);
            gui_vmess("gui_create_vumeter_text", "xxxiisiii",
                canvas, x, x->x_gui.x_lcol,
                end+1, yyy+k3+2, iemgui_vu_scale_str[i/4],
                i, end, yyy);
        }
    }
}

static void vu_properties(t_gobj *z, t_glist *owner)
{
    t_vu *x = (t_vu *)z;
    char buf[800], *gfx_tag;
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |vu| \
        --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
        empty 0.0 empty 0.0 empty %d %d no_scale scale %d %d empty %d \
        {%s} {%s} {%s} %d %d %d %d %d %d %d\n",
        x->x_gui.x_w, IEM_GUI_MINSIZE, x->x_gui.x_h,
        IEM_VU_STEPS*IEM_VU_MINSIZE,
        0,/*no_schedule*/
        x->x_scale, -1, -1, -1,/*no linlog, no init, no multi*/
        "nosndno", srl[1]->s_name,/*no send*/
        srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
        x->x_gui.x_font_style, x->x_gui.x_fontsize,
        0xffffff & x->x_gui.x_bcol, -1/*no front-color*/,
        0xffffff & x->x_gui.x_lcol);
    //gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
    gfx_tag = gfxstub_new2(&x->x_gui.x_obj.ob_pd, x);

    gui_start_vmess("gui_iemgui_dialog", "s", gfx_tag);

    gui_start_array();

    gui_s("type");
    gui_s("vu");

    gui_s("width"); gui_i(x->x_gui.x_w);
    gui_s("height"); gui_i(x->x_gui.x_h);
    
    gui_s("vu-scale"); gui_i(x->x_scale);

    gui_s("minimum-size"); gui_i(IEM_GUI_MINSIZE);
    
    gui_s("range-schedule"); // no idea what this is...
    gui_i(2);

    gui_s("receive-symbol");   gui_s(srl[1]->s_name);
    gui_s("label");            gui_s(srl[2]->s_name);
    gui_s("x-offset");         gui_i(x->x_gui.x_ldx);
    gui_s("y-offset");         gui_i(x->x_gui.x_ldy);
    gui_s("font-style");       gui_i(x->x_gui.x_font_style);
    gui_s("font-size");        gui_i(x->x_gui.x_fontsize);
    gui_s("background-color"); gui_i(0xffffff & x->x_gui.x_bcol);
    gui_s("label-color");      gui_i(0xffffff & x->x_gui.x_lcol);
    
    gui_end_array();
    gui_end_vmess();
}

static void vu_dialog(t_vu *x, t_symbol *s, int argc, t_atom *argv)
{
    canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
    int w = atom_getintarg(0, argc, argv);
    int h = atom_getintarg(1, argc, argv);

    int scale = !!atom_getintarg(4, argc, argv);
    int sr_flags = iemgui_dialog(&x->x_gui, argc, argv);
    x->x_gui.x_loadinit = 0;
    x->x_gui.x_w = iemgui_clip_size(w);
    vu_check_height(x, h);
    vu_scale(x, (t_float)scale);
    iemgui_draw_config(&x->x_gui);
    iemgui_draw_io(&x->x_gui, sr_flags);
    iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_MOVE);
    scalehandle_draw(&x->x_gui);
    scrollbar_update(x->x_gui.x_glist);
}

static void vu_size(t_vu *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    if(ac > 1)
        vu_check_height(x, (int)atom_getintarg(1, ac, av));
    if(glist_isvisible(x->x_gui.x_glist))
    {
        iemgui_draw_move(&x->x_gui);
        iemgui_draw_config(&x->x_gui);
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
        w = atom_getintarg(0, argc, argv);
        h = atom_getintarg(1, argc, argv);
        iemgui_new_getnames(&x->x_gui, 1, argv);
        ldx = atom_getintarg(4, argc, argv);
        ldy = atom_getintarg(5, argc, argv);
        iem_inttofstyle(&x->x_gui, atom_getintarg(6, argc, argv));
        fs = maxi(atom_getintarg(7, argc, argv),4);
        bflcol[0] = atom_getintarg(8, argc, argv);
        bflcol[2] = atom_getintarg(9, argc, argv);
        scale = !!atom_getintarg(10, argc, argv);
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
    x->x_gui.x_snd = s_empty;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = iemgui_clip_size(w);
    vu_check_height(x, h);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    x->x_scale = scale;
    x->x_peak = 0;
    x->x_rms = 0;
    x->x_fp = -101.0;
    x->x_fr = -101.0;
    iemgui_verify_snd_ne_rcv(&x->x_gui); // makes no sense, because snd is unused
    inlet_new(&x->x_gui.x_obj, &x->x_gui.x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_out_rms = outlet_new(&x->x_gui.x_obj, &s_float);
    x->x_out_peak = outlet_new(&x->x_gui.x_obj, &s_float);

    x->x_gui.x_handle = scalehandle_new((t_object *)x,x->x_gui.x_glist,1,vu__clickhook,vu__motionhook);
    x->x_gui.x_lhandle = scalehandle_new((t_object *)x,x->x_gui.x_glist,0,vu__clickhook,vu__motionhook);
    x->x_gui.x_obj.te_iemgui = 1;

    x->x_gui.legacy_x = -1;
    x->x_gui.legacy_y = -1;

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

    wb_init(&vu_widgetbehavior,vu_getrect,0);
    class_setwidget(vu_class,&vu_widgetbehavior);
    class_sethelpsymbol(vu_class, gensym("vu"));
    class_setsavefn(vu_class, vu_save);
    class_setpropertiesfn(vu_class, vu_properties);
}
