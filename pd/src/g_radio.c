// g_radio.c
// written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 as hdial/vdial.
// thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja.
// Copyright (c) 2014 by Mathieu Bouchard. (rewrite).
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt", in this distribution.

// name change to hradio/vradio by MSP (it's a radio button really) and
// changed to put out a "float" as in sliders, toggles, etc.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "g_all_guis.h"
#include <math.h>

#define IEM_RADIO_MAX   128

extern int gfxstub_haveproperties(void *key);
t_widgetbehavior radio_widgetbehavior;
t_class *hradio_class, *hradio_old_class;
t_class *vradio_class, *vradio_old_class;

void radio_draw_update(t_gobj *client, t_glist *glist)
{
    t_radio *x = (t_radio *)client;
    if (!glist_isvisible(glist)) return;
    t_canvas *canvas=glist_getcanvas(glist);
    gui_vmess("gui_radio_update", "xxxii",
        canvas,
        x,
        x->x_gui.x_fcol,
        x->x_drawn,
        x->x_on);
    x->x_drawn = x->x_on;
}

void radio_draw_new(t_radio *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int n=x->x_number, i, d=x->x_gui.x_w, s=d/4;
    int x1=text_xpix(&x->x_gui.x_obj, glist), xi=x1;
    int y1=text_ypix(&x->x_gui.x_obj, glist), yi=y1; 
    iemgui_base_draw_new(&x->x_gui);

    for (i=0; i<n; i++)
    {
        if (x->x_orient)
        {
            if (i)
            {
                gui_vmess("gui_create_radio", "xxiiiiiii",
                    canvas, x, x1, yi, x1+d, yi, i, x1, y1);
            }
            gui_vmess("gui_create_radio_buttons", "xxxiiiiiiii",
                canvas, x,
                x->x_gui.x_fcol,
                x1+s, yi+s, x1+d-s, yi+d-s, x1, y1, i, x->x_on==i);
            yi += d;
            x->x_drawn = x->x_on;
        }
        else
        {
            if (i)
            {
                gui_vmess("gui_create_radio", "xxiiiiiii", canvas,
                    x, xi, y1, xi, y1+d, i, x1, y1);
            }
            gui_vmess("gui_create_radio_buttons", "xxxiiiiiiii",
                canvas, x,
                x->x_gui.x_fcol,
                xi+s, y1+s, xi+d-s, yi+d-s, x1, y1, i, x->x_on==i);
            xi += d;
            x->x_drawn = x->x_on;
        }
    }
}

void radio_draw_move(t_radio *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    if (!glist_isvisible(canvas)) return;
    int n=x->x_number, i, d=x->x_gui.x_w, s=d/4;
    int x1=text_xpix(&x->x_gui.x_obj, glist), xi=0;
    int y1=text_ypix(&x->x_gui.x_obj, glist), yi=0;
    iemgui_base_draw_move(&x->x_gui);
    for (i=0; i<n; i++) 
    {
        if (x->x_orient) {
            gui_vmess("gui_radio_button_coords", "xxiiiiiiii",
                canvas, x,
                x1, y1, xi, yi, i, s, d, x->x_orient);
            yi += d;
        } else {
            gui_vmess("gui_radio_button_coords", "xxiiiiiiii",
                canvas, x,
                x1, y1, xi, yi, i, s, d, x->x_orient);
            xi += d;
        }
    }
}

void radio_draw_config(t_radio *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int n=x->x_number, i;
    iemgui_base_draw_config(&x->x_gui);
    for (i=0; i<n; i++)
    {
        //sys_vgui(".x%lx.c itemconfigure %lxBUT%d -fill #%6.6x -stroke #%6.6x\n",
        //    canvas, x, i,
        //    (x->x_on==i) ? x->x_gui.x_fcol : x->x_gui.x_bcol,
        //    (x->x_on==i) ? x->x_gui.x_fcol : x->x_gui.x_bcol);
    }
    gui_vmess("gui_radio_update", "xxxii",
        canvas, x, x->x_gui.x_fcol, 0, x->x_on);

}

static void radio__clickhook(t_scalehandle *sh, int newstate)
{
    t_radio *x = (t_radio *)(sh->h_master);
    if (newstate)
    {
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
        if (!sh->h_scale)
            scalehandle_click_label(sh);
    }
    sh->h_dragon = newstate;
}

static void radio__motionhook(t_scalehandle *sh, t_floatarg mouse_x, t_floatarg mouse_y)
{
    if (sh->h_scale)
    {
        t_radio *x = (t_radio *)(sh->h_master);
        int width = x->x_gui.x_w,
            height = x->x_gui.x_h;
        int x1, y1, x2, y2, d;
        x1 = text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist);
        y1 = text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist);
        x2 = x->x_orient ? x1 + width : x1 + width * x->x_number;
        y2 = x->x_orient ? y1 + height * x->x_number : y1 + height;

        d = x->x_orient ? (int)mouse_y - y2: (int)mouse_x - x2;

        width = width + d / x->x_number;
        width = maxi(width, IEM_GUI_MINSIZE);
        height = width;

        x->x_gui.x_w = width;
        x->x_gui.x_h = height;

        if (glist_isvisible(x->x_gui.x_glist))
        {
            radio_draw_move(x, x->x_gui.x_glist);
            scalehandle_unclick_scale(sh);
        }

        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            properties_set_field_int(properties,"size", x->x_orient ?
                x->x_gui.x_h + sh->h_dragy :
                x->x_gui.x_w + sh->h_dragx);
        }
    }
    scalehandle_dragon_label(sh,mouse_x, mouse_y);
}

void radio_draw(t_radio *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)        sys_queuegui(x, glist, radio_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)     radio_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)      radio_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)   radio_draw_config(x, glist);
}

static void radio_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1,
    int *xp2, int *yp2)
{
    t_radio *x = (t_radio *)z;
    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    if (x->x_orient) {
        *xp2 = *xp1 + x->x_gui.x_w;
        *yp2 = *yp1 + x->x_gui.x_h*x->x_number;
    } else {
        *xp2 = *xp1 + x->x_gui.x_w*x->x_number;
        *yp2 = *yp1 + x->x_gui.x_h;
    }
    iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);
}

static void radio_save(t_gobj *z, t_binbuf *b)
{
    t_radio *x = (t_radio *)z;
    int bflcol[3];
    t_symbol *srl[3];
    t_class *c = pd_class((t_pd *)x);
    t_symbol *cname =
        c == vradio_old_class ? gensym("vdl") :
        c == hradio_old_class ? gensym("hdl") :
        x->x_orient ? gensym("vradio") : gensym("hradio");
    
    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiiisssiiiiiiii", gensym("#X"),gensym("obj"),
        (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
        cname, x->x_gui.x_w,
        x->x_change, iem_symargstoint(&x->x_gui), x->x_number,
        srl[0], srl[1], srl[2], x->x_gui.x_ldx, x->x_gui.x_ldy,
        iem_fstyletoint(&x->x_gui), x->x_gui.x_fontsize,
        bflcol[0], bflcol[1], bflcol[2], x->x_on);
    binbuf_addv(b, ";");
}

static void radio_properties(t_gobj *z, t_glist *owner)
{
    t_radio *x = (t_radio *)z;
    char buf[800], *gfx_tag;
    t_symbol *srl[3];
    int hchange=-1;

    iemgui_properties(&x->x_gui, srl);
    if (pd_class((t_pd *)x) == hradio_old_class || pd_class((t_pd *)x) == vradio_old_class)
        hchange = x->x_change;
    sprintf(buf, "pdtk_iemgui_dialog %%s |%cradio| \
        ----------dimensions(pix):----------- %d %d size: 0 0 empty \
        empty 0.0 empty 0.0 empty %d %d new-only new&old %d %d number: %d \
        {%s} {%s} {%s} %d %d %d %d %d %d %d\n",
        x->x_orient ? 'v' : 'h',
        x->x_gui.x_w, IEM_GUI_MINSIZE, 0,/*no_schedule*/
        hchange, x->x_gui.x_loadinit, -1, x->x_number,
        srl[0]->s_name, srl[1]->s_name,
        srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
        x->x_gui.x_font_style, x->x_gui.x_fontsize,
        0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol,
        0xffffff & x->x_gui.x_lcol);
    //gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
    gfx_tag = gfxstub_new2(&x->x_gui.x_obj.ob_pd, x);

    gui_start_vmess("gui_iemgui_dialog", "s", gfx_tag);
    gui_start_array();

    gui_s("type");         gui_s(x->x_orient ? "vradio" : "hradio");
    gui_s("size");         gui_i(x->x_gui.x_w);
    gui_s("minimum-size"); gui_i(IEM_GUI_MINSIZE);

    gui_s("range-schedule"); // no idea what this is...
    gui_i(0);

    gui_s("hchange"); // no idea...
    gui_i(hchange);

    gui_s("init");             gui_i(x->x_gui.x_loadinit);
    gui_s("number");           gui_i(x->x_number);
    gui_s("send-symbol");      gui_s(srl[0]->s_name);
    gui_s("receive-symbol");   gui_s(srl[1]->s_name);
    gui_s("label");            gui_s(srl[2]->s_name);
    gui_s("x-offset");         gui_i(x->x_gui.x_ldx);
    gui_s("y-offset");         gui_i(x->x_gui.x_ldy);
    gui_s("font-style");       gui_i(x->x_gui.x_font_style);
    gui_s("font-size");        gui_i(x->x_gui.x_fontsize);
    gui_s("background-color"); gui_i(0xffffff & x->x_gui.x_bcol);
    gui_s("foreground-color"); gui_i(0xffffff & x->x_gui.x_fcol);
    gui_s("label-color");      gui_i(0xffffff & x->x_gui.x_lcol);
    
    gui_end_array();
    gui_end_vmess();
}

static void radio_dialog(t_radio *x, t_symbol *s, int argc, t_atom *argv)
{
    canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
    x->x_gui.x_h =
    x->x_gui.x_w = iemgui_clip_size(atom_getintarg(0, argc, argv));
    x->x_change = !!atom_getintarg(4, argc, argv);
    int num = atom_getintarg(6, argc, argv);
    int sr_flags = iemgui_dialog(&x->x_gui, argc, argv);
    if(x->x_number != num)
    {
        iemgui_draw_erase(&x->x_gui);
        x->x_number = num;
        if(x->x_on >= x->x_number)
        {
            x->x_on = x->x_number - 1;
            x->x_on_old = x->x_on;
        }
        iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_NEW);
        if (glist_isselected(x->x_gui.x_glist, (t_gobj *)x))
            iemgui_select((t_gobj *)x, x->x_gui.x_glist, 1);
    }
    else
    {
        iemgui_draw_config(&x->x_gui);
        iemgui_draw_io(&x->x_gui, sr_flags);
        iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_MOVE);
    }
    scalehandle_draw(&x->x_gui);
    scrollbar_update(x->x_gui.x_glist);
}

static void radio_set(t_radio *x, t_floatarg f)
{
    int i=mini(maxi((int)f,0),x->x_number-1);
    if(x->x_on != x->x_on_old)
    {
        int old = x->x_on_old;
        x->x_on_old = x->x_on;
        x->x_on = i;
        if (x->x_on != x->x_on_old)
            x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        x->x_on_old = old;
    }
    else
    {
        x->x_on = i;
        if (x->x_on != x->x_on_old)
            x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
}

static void radio_bang(t_radio *x)
{
    if (pd_class(&x->x_gui.x_obj.ob_pd) == hradio_old_class ||
        pd_class(&x->x_gui.x_obj.ob_pd) == vradio_old_class)
    {
        if((x->x_change)&&(x->x_on != x->x_on_old))
        {
            SETFLOAT(x->x_at, (t_float)x->x_on_old);
            SETFLOAT(x->x_at+1, 0.0);
            iemgui_out_list(&x->x_gui, 0, 0, &s_list, 2, x->x_at);
        }
        x->x_on_old = x->x_on;
        SETFLOAT(x->x_at, (t_float)x->x_on);
        SETFLOAT(x->x_at+1, 1.0);
        iemgui_out_list(&x->x_gui, 0, 0, &s_list, 2, x->x_at);
    }
    else iemgui_out_float(&x->x_gui, 0, 0, x->x_on);
}

static void radio_float2(t_radio *x, t_floatarg f, int doout)
{
    int i=mini(maxi((int)f,0),x->x_number-1);

    if (pd_class(&x->x_gui.x_obj.ob_pd) == hradio_old_class ||
        pd_class(&x->x_gui.x_obj.ob_pd) == vradio_old_class)
    {
        if((x->x_change)&&(i != x->x_on_old))
        {
            if(doout)
            {
                SETFLOAT(x->x_at, (t_float)x->x_on_old);
                SETFLOAT(x->x_at+1, 0.0);
                iemgui_out_list(&x->x_gui, 0, 0, &s_list, 2, x->x_at);
            }
        }
        x->x_on_old = x->x_on;
        x->x_on = i;
        x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        x->x_on_old = x->x_on;
        if(doout)
        {
            SETFLOAT(x->x_at, (t_float)x->x_on);
            SETFLOAT(x->x_at+1, 1.0);
            iemgui_out_list(&x->x_gui, 0, 0, &s_list, 2, x->x_at);
        }
    }
    else
    {
        x->x_on_old = x->x_on;
        x->x_on = i;
        if (i != x->x_on_old)
            x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        if (doout)
        {
            iemgui_out_float(&x->x_gui, 0, 0, x->x_on);
        }
    }
}

static void radio_fout(t_radio *x, t_floatarg f)
{
    radio_float2(x,f,1);
}

static void radio_float(t_radio *x, t_floatarg f)
{
    radio_float2(x,f,x->x_gui.x_put_in2out);
}

static void radio_click(t_radio *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    if (x->x_orient) {
        int yy = (int)ypos - text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist);
        radio_fout(x, (t_float)(yy / x->x_gui.x_h));
    } else {
        int xx = (int)xpos - text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist)-1; 
        radio_fout(x, (t_float)(xx / x->x_gui.x_w));
    }
}

static int radio_newclick(t_gobj *z, struct _glist *glist, int xpix, int ypix,
    int shift, int alt, int dbl, int doit)
{
    if(doit)
        radio_click((t_radio *)z, (t_floatarg)xpix, (t_floatarg)ypix,
            (t_floatarg)shift, 0, (t_floatarg)alt);
    return (1);
}

static void radio_loadbang(t_radio *x)
{
    if(!sys_noloadbang && x->x_gui.x_loadinit)
    {
        radio_bang(x);
    }
}

static void radio_number(t_radio *x, t_floatarg num)
{
    int n=mini(maxi((int)num,1),IEM_RADIO_MAX);
    if(n != x->x_number)
    {
        iemgui_draw_erase(&x->x_gui);
        x->x_number = n;
        if(x->x_on >= x->x_number)
            x->x_on = x->x_number - 1;
        x->x_on_old = x->x_on;
        iemgui_draw_new(&x->x_gui);
    }
}

static void radio_size(t_radio *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    x->x_gui.x_h = x->x_gui.x_w;
    iemgui_size(&x->x_gui);
}

static void radio_double_change(t_radio *x) {x->x_change = 1;}
static void radio_single_change(t_radio *x) {x->x_change = 0;}

static void *radio_new(t_symbol *s, int argc, t_atom *argv)
{
    t_radio *x = (t_radio *)pd_new(
        s==gensym("hdl") ? hradio_old_class :
        s==gensym("vdl") ? vradio_old_class :
        s==gensym("vradio") ? vradio_class : hradio_class);
    x->x_orient = s==gensym("vdl") || s==gensym("vradio");
    int bflcol[]={-262144, -1, -1};
    int a=IEM_GUI_DEFAULTSIZE, on=0, ldx=0, ldy=-8, chg=1, num=8, fs=10;
    iem_inttosymargs(&x->x_gui, 0);
    iem_inttofstyle(&x->x_gui, 0);

    if((argc == 15)&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)&&IS_A_FLOAT(argv,2)
       &&IS_A_FLOAT(argv,3)
       &&(IS_A_SYMBOL(argv,4)||IS_A_FLOAT(argv,4))
       &&(IS_A_SYMBOL(argv,5)||IS_A_FLOAT(argv,5))
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&IS_A_FLOAT(argv,7)&&IS_A_FLOAT(argv,8)
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)&&IS_A_FLOAT(argv,11)
       &&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13)&&IS_A_FLOAT(argv,14))
    {
        a = atom_getintarg(0, argc, argv);
        chg = atom_getintarg(1, argc, argv);
        iem_inttosymargs(&x->x_gui, atom_getintarg(2, argc, argv));
        num = mini(maxi(atom_getintarg(3, argc, argv),1),IEM_RADIO_MAX);
        iemgui_new_getnames(&x->x_gui, 4, argv);
        ldx = atom_getintarg(7, argc, argv);
        ldy = atom_getintarg(8, argc, argv);
        iem_inttofstyle(&x->x_gui, atom_getintarg(9, argc, argv));
        fs = maxi(atom_getintarg(10, argc, argv),4);
        bflcol[0] = atom_getintarg(11, argc, argv);
        bflcol[1] = atom_getintarg(12, argc, argv);
        bflcol[2] = atom_getintarg(13, argc, argv);
        on = mini(maxi(atom_getintarg(14, argc, argv),0),num-1);
    }
    else iemgui_new_getnames(&x->x_gui, 4, 0);
    x->x_gui.x_draw = (t_iemfunptr)radio_draw;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if (x->x_gui.x_font_style<0 || x->x_gui.x_font_style>2) x->x_gui.x_font_style=0;
    x->x_number = num;
    x->x_on = x->x_gui.x_loadinit ? on : 0;
    x->x_on_old = -1;
    x->x_change = (chg==0)?0:1;
    if (iemgui_has_rcv(&x->x_gui))
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = iemgui_clip_size(a);
    x->x_gui.x_h = x->x_gui.x_w;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    outlet_new(&x->x_gui.x_obj, &s_list);

    x->x_gui.x_handle = scalehandle_new((t_object *)x,x->x_gui.x_glist,1,radio__clickhook,radio__motionhook);
    x->x_gui.x_lhandle = scalehandle_new((t_object *)x,x->x_gui.x_glist,0,radio__clickhook,radio__motionhook);
    x->x_gui.x_obj.te_iemgui = 1;

    x->x_gui.legacy_x = 0;
    x->x_gui.legacy_y = 1;

    return (x);
}

static void radio_free(t_radio *x)
{
    if(iemgui_has_rcv(&x->x_gui))
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);

    if (x->x_gui. x_handle) scalehandle_free(x->x_gui. x_handle);
    if (x->x_gui.x_lhandle) scalehandle_free(x->x_gui.x_lhandle);
}

void radio_addmethods(t_class *c)
{
    class_addbang(c, radio_bang);
    class_addfloat(c, radio_float);
    class_addmethod(c, (t_method)radio_loadbang,  gensym("loadbang"), 0);
    class_addmethod(c, (t_method)radio_set,       gensym("set"), A_FLOAT, 0);
    class_addmethod(c, (t_method)radio_size,      gensym("size"), A_GIMME, 0);
    class_addmethod(c, (t_method)iemgui_init,      gensym("init"), A_FLOAT, 0);
    class_addmethod(c, (t_method)radio_number,  gensym("number"), A_FLOAT, 0);
    class_addmethod(c, (t_method)radio_single_change,
        gensym("single_change"), 0);
    class_addmethod(c, (t_method)radio_double_change,
        gensym("double_change"), 0);
    class_addmethod(c, (t_method)radio_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_setpropertiesfn(c, radio_properties);
    class_addmethod(c, (t_method)radio_dialog, gensym("dialog"), A_GIMME, 0);
    class_setsavefn(c, radio_save);
    class_setwidget(c, &radio_widgetbehavior);
}

void g_radio_setup(void)
{
    wb_init(&radio_widgetbehavior,radio_getrect,radio_newclick);
    hradio_class = class_new(gensym("hradio"), (t_newmethod)radio_new,
        (t_method)radio_free, sizeof(t_radio), 0, A_GIMME, 0);
    hradio_old_class = class_new(gensym("hdl"), (t_newmethod)radio_new,
        (t_method)radio_free, sizeof(t_radio), 0, A_GIMME, 0);
    vradio_class = class_new(gensym("vradio"), (t_newmethod)radio_new,
        (t_method)radio_free, sizeof(t_radio), 0, A_GIMME, 0);
    vradio_old_class = class_new(gensym("vdl"), (t_newmethod)radio_new,
        (t_method)radio_free, sizeof(t_radio), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)radio_new, gensym("rdb"), A_GIMME, 0);
    class_addcreator((t_newmethod)radio_new, gensym("radiobut"), A_GIMME, 0);
    class_addcreator((t_newmethod)radio_new, gensym("radiobutton"),
        A_GIMME, 0);

    iemgui_class_addmethods(hradio_class);
    iemgui_class_addmethods(hradio_old_class);
    iemgui_class_addmethods(vradio_class);
    iemgui_class_addmethods(vradio_old_class);

    radio_addmethods(hradio_class);
    radio_addmethods(hradio_old_class);
    radio_addmethods(vradio_class);
    radio_addmethods(vradio_old_class);

    class_sethelpsymbol(hradio_class, gensym("hradio"));
    class_sethelpsymbol(hradio_old_class, gensym("hradio"));
    class_sethelpsymbol(vradio_class, gensym("vradio"));
    class_sethelpsymbol(vradio_old_class, gensym("vradio"));
}
