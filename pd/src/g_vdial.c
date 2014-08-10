/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* vdial.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

/* name change to vradio by MSP (it's a radio button really) and changed to
put out a "float" as in sliders, toggles, etc. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "t_tk.h"
#include "g_all_guis.h"
#include <math.h>

extern int gfxstub_haveproperties(void *key);
void vradio_draw_select(t_vradio* x, t_glist* glist);

/* ------------- vdl     gui-vertical radio button ---------------------- */

t_widgetbehavior vradio_widgetbehavior;
static t_class *vradio_class, *vradio_old_class;

/* widget helper functions */

void vradio_draw_update(t_gobj *client, t_glist *glist)
{
    t_hradio *x = (t_hradio *)client;
    if(!glist_isvisible(glist)) return;
    t_canvas *canvas=glist_getcanvas(glist);
    sys_vgui(".x%lx.c itemconfigure %lxBUT%d -fill #%6.6x -stroke #%6.6x\n",
        canvas, x, x->x_drawn, x->x_gui.x_bcol, x->x_gui.x_bcol);
    sys_vgui(".x%lx.c itemconfigure %lxBUT%d -fill #%6.6x -stroke #%6.6x\n",
        canvas, x, x->x_on,    x->x_gui.x_fcol, x->x_gui.x_fcol);
    x->x_drawn = x->x_on;
}

void vradio_draw_io(t_vradio* x, t_glist* glist, int old_snd_rcv_flags)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_io_draw(&x->x_gui,canvas,old_snd_rcv_flags);
}

void vradio_draw_new(t_vradio *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int n=x->x_number, i, d=x->x_gui.x_w, s=d/4;
    int x1=text_xpix(&x->x_gui.x_obj, glist);
    int y1=text_ypix(&x->x_gui.x_obj, glist), yi=y1;
    char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

    iemgui_base_draw_new(&x->x_gui, canvas, nlet_tag);
    scalehandle_draw_new(x->x_gui. x_handle,canvas);
    scalehandle_draw_new(x->x_gui.x_lhandle,canvas);

    for(i=0; i<n; i++)
    {
        if (i) sys_vgui(".x%lx.c create pline %d %d %d %d "
            "-stroke $pd_colors(iemgui_border) "
            "-tags {%lxBASE%d %lxBASEL %lxOBJ %s text iemgui border}\n",
            canvas, x1, yi, x1+d, yi, x, i, x, x, nlet_tag);
        sys_vgui(".x%lx.c create prect %d %d %d %d -fill #%6.6x "
            "-stroke #%6.6x -tags {%lxBUT%d %lxOBJ %s text iemgui}\n",
            canvas, x1+s, yi+s, x1+d-s, yi+d-s,
            (x->x_on==i)?x->x_gui.x_fcol:x->x_gui.x_bcol,
            (x->x_on==i)?x->x_gui.x_fcol:x->x_gui.x_bcol,
            x, i, x, nlet_tag);
        yi += d;
        x->x_drawn = x->x_on;
    }
    iemgui_label_draw_new(&x->x_gui,canvas,x1,y1,nlet_tag);
    vradio_draw_io(x,glist,7);
}

void vradio_draw_move(t_vradio *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    if (!glist_isvisible(canvas)) return;
    int n=x->x_number, i, d=x->x_gui.x_w, s=d/4;
    int x1=text_xpix(&x->x_gui.x_obj, glist);
    int y1=text_ypix(&x->x_gui.x_obj, glist), yi=y1;

    char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);
    for(i=0; i<n; i++)
    {
        sys_vgui(".x%lx.c coords %lxBASE%d %d %d %d %d\n",
            canvas, x, i, x1, yi, x1+d, yi);
        sys_vgui(".x%lx.c coords %lxBUT%d %d %d %d %d\n",
            canvas, x, i, x1+s, yi+s, x1+d-s, yi+d-s);
        yi += d;
    }
    iemgui_label_draw_move(&x->x_gui,canvas,x1,y1);
    iemgui_io_draw_move(&x->x_gui,canvas,nlet_tag);
    if (x->x_gui.x_selected) vradio_draw_select(x, x->x_gui.x_glist);
}

void vradio_draw_config(t_vradio* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int n=x->x_number, i;
    iemgui_label_draw_config(&x->x_gui,canvas);
    iemgui_base_draw_config(&x->x_gui,canvas);
    for(i=0; i<n; i++)
    {
        sys_vgui(".x%lx.c itemconfigure %lxBUT%d -fill #%6.6x -stroke #%6.6x\n",
            canvas, x, i,
            (x->x_on==i) ? x->x_gui.x_fcol : x->x_gui.x_bcol,
            (x->x_on==i) ? x->x_gui.x_fcol : x->x_gui.x_bcol);
    }
}

void vradio_draw_select(t_vradio* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_base_draw_config(&x->x_gui,canvas);
    if(x->x_gui.x_selected)
    {
        /* check if we are drawing inside a gop abstraction visible
           on parent canvas. If so, disable highlighting */
        if (x->x_gui.x_glist == glist_getcanvas(glist))
        {
            scalehandle_draw_select2(&x->x_gui,glist,
                x->x_gui.x_w-1,x->x_gui.x_h*x->x_number-1);
        }
    }
    else
    {
        scalehandle_draw_erase2(&x->x_gui,glist);
    }
    iemgui_label_draw_select(&x->x_gui,canvas);
    iemgui_tag_selected(&x->x_gui,canvas);
}

static void vradio__clickhook(t_scalehandle *sh, t_floatarg f, t_floatarg xxx,
    t_floatarg yyy)
{
    t_vradio *x = (t_vradio *)(sh->h_master);
    int newstate = (int)f;
    if (sh->h_dragon && newstate == 0 && sh->h_scale)
    {
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
        if (sh->h_dragx || sh->h_dragy)
        {
            x->x_gui.x_w += sh->h_dragx;
            x->x_gui.x_h += sh->h_dragx;
            canvas_dirty(x->x_gui.x_glist, 1);
        }
        if (glist_isvisible(x->x_gui.x_glist))
        {
            vradio_draw_move(x, x->x_gui.x_glist);
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

static void vradio__motionhook(t_scalehandle *sh, t_floatarg f1, t_floatarg f2)
{
    if (sh->h_dragon && sh->h_scale)
    {
        t_vradio *x = (t_vradio *)(sh->h_master);
        int dx = (int)f1, dy = (int)f2;
        dy = maxi(dy,(IEM_GUI_MINSIZE-x->x_gui.x_h)*x->x_number);
        dx = dy/x->x_number;
        sh->h_dragx = dx;
        sh->h_dragy = dy;
        scalehandle_drag_scale(sh);

        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            int new_w = x->x_gui.x_h + sh->h_dragy;
            properties_set_field_int(properties,"dim.w_ent",new_w);
        }
    }
    scalehandle_dragon_label(sh,f1,f2);
}

void vradio_draw(t_vradio *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui(x, glist, vradio_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        vradio_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
    {
        vradio_draw_new(x, glist);
        sys_vgui(".x%lx.c raise all_cords\n", glist_getcanvas(glist));
    }
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        vradio_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        iemgui_draw_erase(&x->x_gui, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        vradio_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        vradio_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ vdl widgetbehaviour----------------------------- */

static void vradio_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1,
    int *xp2, int *yp2)
{
    t_vradio *x = (t_vradio *)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h*x->x_number;

    iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);
}

static void vradio_save(t_gobj *z, t_binbuf *b)
{
    t_vradio *x = (t_vradio *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiiisssiiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix,
                (int)x->x_gui.x_obj.te_ypix,
                (pd_class(&x->x_gui.x_obj.ob_pd) == vradio_old_class ?
                    gensym("vdl") : gensym("vradio")),
                x->x_gui.x_w,
                x->x_change, iem_symargstoint(&x->x_gui), x->x_number,
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2], x->x_on);
    binbuf_addv(b, ";");
}

static void vradio_properties(t_gobj *z, t_glist *owner)
{
    t_vradio *x = (t_vradio *)z;
    char buf[800];
    t_symbol *srl[3];
    int hchange=-1;

    iemgui_properties(&x->x_gui, srl);
    if(pd_class(&x->x_gui.x_obj.ob_pd) == vradio_old_class)
        hchange = x->x_change;
    sprintf(buf, "pdtk_iemgui_dialog %%s |vradio| \
            ----------dimensions(pix):----------- %d %d size: 0 0 empty \
            empty 0.0 empty 0.0 empty %d \
            %d new-only new&old %d %d number: %d \
            {%s} {%s} \
            {%s} %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, IEM_GUI_MINSIZE,
            0,/*no_schedule*/
            hchange, x->x_gui.x_loadinit, -1, x->x_number,
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol,
            0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void vradio_dialog(t_vradio *x, t_symbol *s, int argc, t_atom *argv)
{
    canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

    t_symbol *srl[3];
    int a = (int)atom_getintarg(0, argc, argv);
    int chg = (int)atom_getintarg(4, argc, argv);
    int num = (int)atom_getintarg(6, argc, argv);
    int sr_flags;

    if(chg != 0) chg = 1;
    x->x_change = chg;
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    x->x_gui.x_w = iemgui_clip_size(a);
    x->x_gui.x_h = x->x_gui.x_w;
    if(x->x_number != num)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_ERASE);
        x->x_number = num;
        if(x->x_on >= x->x_number)
        {
            x->x_on = x->x_number - 1;
            x->x_on_old = x->x_on;
        }
        //(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_NEW);
        iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_NEW);
    }
    else
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
        //(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
        //canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
        iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_MOVE);
    }
    if (x->x_gui.x_selected) vradio_draw_select(x, x->x_gui.x_glist);
    scrollbar_update(x->x_gui.x_glist);
}

static void vradio_set(t_vradio *x, t_floatarg f)
{
    int i=(int)f;
    int old;

    if(i < 0)
        i = 0;
    if(i >= x->x_number)
        i = x->x_number-1;
    if(x->x_on != x->x_on_old)
    {
        old = x->x_on_old;
        x->x_on_old = x->x_on;
        x->x_on = i;
        if (x->x_on != x->x_on_old)
            (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        x->x_on_old = old;
    }
    else
    {
        x->x_on = i;
        if (x->x_on != x->x_on_old)
            (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
}

static void vradio_bang(t_vradio *x)
{
        /* compatibility with earlier  "vdial" behavior */
    if (pd_class(&x->x_gui.x_obj.ob_pd) == vradio_old_class)
    {
        if((x->x_change)&&(x->x_on != x->x_on_old))
        {
            SETFLOAT(x->x_at, (t_float)x->x_on_old);
            SETFLOAT(x->x_at+1, 0.0);
            outlet_list(x->x_gui.x_obj.ob_outlet, &s_list, 2, x->x_at);
            if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
                pd_list(x->x_gui.x_snd->s_thing, &s_list, 2, x->x_at);
        }
        x->x_on_old = x->x_on;
        SETFLOAT(x->x_at, (t_float)x->x_on);
        SETFLOAT(x->x_at+1, 1.0);
        outlet_list(x->x_gui.x_obj.ob_outlet, &s_list, 2, x->x_at);
        if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
            pd_list(x->x_gui.x_snd->s_thing, &s_list, 2, x->x_at);
    }
    else
    {
        outlet_float(x->x_gui.x_obj.ob_outlet, x->x_on);
        if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
            pd_float(x->x_gui.x_snd->s_thing, x->x_on);
    }
}

static void vradio_fout(t_vradio *x, t_floatarg f)
{
    int i=(int)f;

    if(i < 0)
        i = 0;
    if(i >= x->x_number)
        i = x->x_number-1;

    if (pd_class(&x->x_gui.x_obj.ob_pd) == vradio_old_class)
    {
            /* compatibility with earlier  "vdial" behavior */
        if((x->x_change)&&(i != x->x_on_old))
        {
            SETFLOAT(x->x_at, (t_float)x->x_on_old);
            SETFLOAT(x->x_at+1, 0.0);
            outlet_list(x->x_gui.x_obj.ob_outlet, &s_list, 2, x->x_at);
            if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
                pd_list(x->x_gui.x_snd->s_thing, &s_list, 2, x->x_at);
        }
        if(x->x_on != x->x_on_old)
            x->x_on_old = x->x_on;
        x->x_on = i;
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        x->x_on_old = x->x_on;
        SETFLOAT(x->x_at, (t_float)x->x_on);
        SETFLOAT(x->x_at+1, 1.0);
        outlet_list(x->x_gui.x_obj.ob_outlet, &s_list, 2, x->x_at);
        if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
            pd_list(x->x_gui.x_snd->s_thing, &s_list, 2, x->x_at);
    }
    else
    {
        x->x_on_old = x->x_on;
        x->x_on = i;
        if (x->x_on != x->x_on_old)
            (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        outlet_float(x->x_gui.x_obj.ob_outlet, x->x_on);
        if (iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
            pd_float(x->x_gui.x_snd->s_thing, x->x_on);
    }
}

static void vradio_float(t_vradio *x, t_floatarg f)
{
    int i=(int)f;

    if(i < 0)
        i = 0;
    if(i >= x->x_number)
        i = x->x_number-1;

    if (pd_class(&x->x_gui.x_obj.ob_pd) == vradio_old_class)
    {
            /* compatibility with earlier  "vdial" behavior */
        if((x->x_change)&&(i != x->x_on_old))
        {
            if(x->x_gui.x_put_in2out)
            {
                SETFLOAT(x->x_at, (t_float)x->x_on_old);
                SETFLOAT(x->x_at+1, 0.0);
                outlet_list(x->x_gui.x_obj.ob_outlet, &s_list, 2, x->x_at);
                if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
                    pd_list(x->x_gui.x_snd->s_thing, &s_list, 2, x->x_at);
            }
        }
        if(x->x_on != x->x_on_old)
            x->x_on_old = x->x_on;
        x->x_on = i;
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        x->x_on_old = x->x_on;
        if(x->x_gui.x_put_in2out)
        {
            SETFLOAT(x->x_at, (t_float)x->x_on);
            SETFLOAT(x->x_at+1, 1.0);
            outlet_list(x->x_gui.x_obj.ob_outlet, &s_list, 2, x->x_at);
            if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
                pd_list(x->x_gui.x_snd->s_thing, &s_list, 2, x->x_at);
        }
    }
    else
    {
        x->x_on_old = x->x_on;
        x->x_on = i;
        if (x->x_on != x->x_on_old)
            (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        if (x->x_gui.x_put_in2out)
        {
            outlet_float(x->x_gui.x_obj.ob_outlet, x->x_on);
            if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
                pd_float(x->x_gui.x_snd->s_thing, x->x_on);
        }
    }
}

static void vradio_click(t_vradio *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    int yy =  (int)ypos - text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist);

    vradio_fout(x, (t_float)(yy / x->x_gui.x_h));
}

static int vradio_newclick(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    if(doit)
        vradio_click((t_vradio *)z, (t_floatarg)xpix, (t_floatarg)ypix,
            (t_floatarg)shift, 0, (t_floatarg)alt);
    return (1);
}

static void vradio_loadbang(t_vradio *x)
{
    if(!sys_noloadbang && x->x_gui.x_loadinit)
        vradio_bang(x);
}

static void vradio_number(t_vradio *x, t_floatarg num)
{
    int n=(int)num;

    if(n < 1)
        n = 1;
    if(n > IEM_RADIO_MAX)
        n = IEM_RADIO_MAX;
    if(n != x->x_number)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_ERASE);
        x->x_number = n;
        if(x->x_on >= x->x_number)
            x->x_on = x->x_number - 1;
        x->x_on_old = x->x_on;
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_NEW);
    }
}

static void vradio_size(t_vradio *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    x->x_gui.x_h = x->x_gui.x_w;
    iemgui_size(&x->x_gui);
}

static void vradio_init(t_vradio *x, t_floatarg f)
{
    x->x_gui.x_loadinit = (f==0.0)?0:1;
}

static void vradio_double_change(t_vradio *x)
{x->x_change = 1;}

static void vradio_single_change(t_vradio *x)
{x->x_change = 0;}

static void *vradio_donew(t_symbol *s, int argc, t_atom *argv, int old)
{
    t_vradio *x = (t_vradio *)pd_new(old? vradio_old_class : vradio_class);
    int bflcol[]={-262144, -1, -1};
    int a=IEM_GUI_DEFAULTSIZE, on=0;
    int ldx=0, ldy=-8, chg=1, num=8;
    int fs=10;

    if((argc == 15)&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)&&IS_A_FLOAT(argv,2)
       &&IS_A_FLOAT(argv,3)
       &&(IS_A_SYMBOL(argv,4)||IS_A_FLOAT(argv,4))
       &&(IS_A_SYMBOL(argv,5)||IS_A_FLOAT(argv,5))
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&IS_A_FLOAT(argv,7)&&IS_A_FLOAT(argv,8)
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)&&IS_A_FLOAT(argv,11)
       &&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13)&&IS_A_FLOAT(argv,14))
    {
        a = (int)atom_getintarg(0, argc, argv);
        chg = (int)atom_getintarg(1, argc, argv);
        iem_inttosymargs(&x->x_gui, atom_getintarg(2, argc, argv));
        num = (int)atom_getintarg(3, argc, argv);
        iemgui_new_getnames(&x->x_gui, 4, argv);
        ldx = (int)atom_getintarg(7, argc, argv);
        ldy = (int)atom_getintarg(8, argc, argv);
        iem_inttofstyle(&x->x_gui, atom_getintarg(9, argc, argv));
        fs = (int)atom_getintarg(10, argc, argv);
        bflcol[0] = (int)atom_getintarg(11, argc, argv);
        bflcol[1] = (int)atom_getintarg(12, argc, argv);
        bflcol[2] = (int)atom_getintarg(13, argc, argv);
        on = (int)atom_getintarg(14, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 4, 0);
    x->x_gui.x_draw = (t_iemfunptr)vradio_draw;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if (x->x_gui.x_font_style<0 || x->x_gui.x_font_style>2) x->x_gui.x_font_style=0;
    if(num < 1)
        num = 1;
    if(num > IEM_RADIO_MAX)
        num = IEM_RADIO_MAX;
    x->x_number = num;
    if(on < 0)
        on = 0;
    if(on >= x->x_number)
        on = x->x_number - 1;
    if(x->x_gui.x_loadinit)
        x->x_on = on;
    else
        x->x_on = 0;
    x->x_on_old = x->x_on;
    x->x_change = (chg==0)?0:1;
    if (iemgui_has_rcv(&x->x_gui))
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = iemgui_clip_size(a);
    x->x_gui.x_h = x->x_gui.x_w;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    outlet_new(&x->x_gui.x_obj, &s_list);

    x->x_gui. x_handle = scalehandle_new(scalehandle_class,(t_iemgui *)x,1);
    x->x_gui.x_lhandle = scalehandle_new(scalehandle_class,(t_iemgui *)x,0);
    x->x_gui.x_obj.te_iemgui = 1;

    return (x);
}

static void *vradio_new(t_symbol *s, int argc, t_atom *argv)
{
    return (vradio_donew(s, argc, argv, 0));
}

static void *vdial_new(t_symbol *s, int argc, t_atom *argv)
{
    return (vradio_donew(s, argc, argv, 1));
}

static void vradio_ff(t_vradio *x)
{
    if(iemgui_has_rcv(&x->x_gui))
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);

    if (x->x_gui. x_handle) scalehandle_free(x->x_gui. x_handle);
    if (x->x_gui.x_lhandle) scalehandle_free(x->x_gui.x_lhandle);
}

void g_vradio_setup(void)
{
    vradio_class = class_new(gensym("vradio"), (t_newmethod)vradio_new,
        (t_method)vradio_ff, sizeof(t_vradio), 0, A_GIMME, 0);
    class_addbang(vradio_class, vradio_bang);
    class_addfloat(vradio_class, vradio_float);
    class_addmethod(vradio_class, (t_method)vradio_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(vradio_class, (t_method)vradio_dialog, gensym("dialog"),
                    A_GIMME, 0);
    class_addmethod(vradio_class, (t_method)vradio_loadbang,
        gensym("loadbang"), 0);
    class_addmethod(vradio_class, (t_method)vradio_set,
        gensym("set"), A_FLOAT, 0);
    class_addmethod(vradio_class, (t_method)vradio_size,
        gensym("size"), A_GIMME, 0);
    iemgui_class_addmethods(vradio_class);
    class_addmethod(vradio_class, (t_method)vradio_init,
        gensym("init"), A_FLOAT, 0);
    class_addmethod(vradio_class, (t_method)vradio_number,
        gensym("number"), A_FLOAT, 0);
    class_addmethod(vradio_class, (t_method)vradio_single_change,
        gensym("single_change"), 0);
    class_addmethod(vradio_class, (t_method)vradio_double_change,
        gensym("double_change"), 0);
 
    scalehandle_class = class_new(gensym("_scalehandle"), 0, 0,
                  sizeof(t_scalehandle), CLASS_PD, 0);
    class_addmethod(scalehandle_class, (t_method)vradio__clickhook,
            gensym("_click"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scalehandle_class, (t_method)vradio__motionhook,
            gensym("_motion"), A_FLOAT, A_FLOAT, 0);

    wb_init(&vradio_widgetbehavior,vradio_getrect,vradio_newclick);
    class_setwidget(vradio_class, &vradio_widgetbehavior);
    class_sethelpsymbol(vradio_class, gensym("vradio"));
    class_setsavefn(vradio_class, vradio_save);
    class_setpropertiesfn(vradio_class, vradio_properties);

        /* obsolete version (0.34-0.35) */
    vradio_old_class = class_new(gensym("vdl"), (t_newmethod)vdial_new,
        (t_method)vradio_ff, sizeof(t_vradio), 0, A_GIMME, 0);
    class_addbang(vradio_old_class, vradio_bang);
    class_addfloat(vradio_old_class, vradio_float);
    class_addmethod(vradio_old_class, (t_method)vradio_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(vradio_old_class, (t_method)vradio_dialog,
        gensym("dialog"), A_GIMME, 0);
    class_addmethod(vradio_old_class, (t_method)vradio_loadbang,
        gensym("loadbang"), 0);
    class_addmethod(vradio_old_class, (t_method)vradio_set,
        gensym("set"), A_FLOAT, 0);
    class_addmethod(vradio_old_class, (t_method)vradio_size,
        gensym("size"), A_GIMME, 0);
    iemgui_class_addmethods(vradio_old_class);
    class_addmethod(vradio_old_class, (t_method)vradio_init,
        gensym("init"), A_FLOAT, 0);
    class_addmethod(vradio_old_class, (t_method)vradio_number,
        gensym("number"), A_FLOAT, 0);
    class_addmethod(vradio_old_class, (t_method)vradio_single_change,
        gensym("single_change"), 0);
    class_addmethod(vradio_old_class, (t_method)vradio_double_change,
        gensym("double_change"), 0);
    class_setwidget(vradio_old_class, &vradio_widgetbehavior);
    class_sethelpsymbol(vradio_old_class, gensym("vradio"));
}
