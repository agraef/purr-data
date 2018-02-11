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

extern int gfxstub_haveproperties(void *key);
t_widgetbehavior toggle_widgetbehavior;
static t_class *toggle_class;

void toggle_draw_update(t_gobj *xgobj, t_glist *glist)
{
    t_toggle *x = (t_toggle *)xgobj;
    char cbuf[8];
    sprintf(cbuf, "#%6.6x", x->x_gui.x_fcol);
    if (x->x_gui.x_changed)
    {
        if(glist_isvisible(glist_getcanvas(glist)))
        {
            t_canvas *canvas=glist_getcanvas(glist);
            gui_vmess("gui_toggle_update", "xxis", canvas,
                x, x->x_on != 0.0, cbuf);
        }
        x->x_gui.x_changed = 0;
    }
}

void toggle_draw_new(t_toggle *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    char cbuf[8];
    sprintf(cbuf, "#%6.6x", x->x_gui.x_fcol);
    int w=(x->x_gui.x_w+29)/30;
    int x1=text_xpix(&x->x_gui.x_obj, glist);
    int y1=text_ypix(&x->x_gui.x_obj, glist);
    int x2=x1+x->x_gui.x_w, y2=y1+x->x_gui.x_h;

    iemgui_base_draw_new(&x->x_gui);
    gui_vmess("gui_toggle_new", "xxsiiiiiiiiiiii", canvas,
        x, cbuf, w,
        (x->x_on != 0.0),
        x1+w+1, y1+w+1, x2-w-1, y2-w-1,
        x1+w+1, y2-w-1, x2-w-1, y1+w+1, x1, y1);
}

void toggle_draw_move(t_toggle *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    if (!glist_isvisible(canvas)) return;
    int w=(x->x_gui.x_w+29)/30, s=w+1;
    int x1=text_xpix(&x->x_gui.x_obj, glist), x2=x1+x->x_gui.x_w;
    int y1=text_ypix(&x->x_gui.x_obj, glist), y2=y1+x->x_gui.x_h;

    iemgui_base_draw_move(&x->x_gui);
    gui_vmess("gui_toggle_resize_cross", "xxiiiiiiiiiii",
        canvas, x,
        w,
        x1+s, y1+s, x2-s, y2-s,
        x1+s, y2-s, x2-s, y1+s,
        x1, y1);
}

void toggle_draw_config(t_toggle* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    char cbuf[8];
    sprintf(cbuf, "#%6.6x", x->x_gui.x_fcol);
    iemgui_base_draw_config(&x->x_gui);
    if (glist_isvisible(glist_getcanvas(glist)))
    {
        gui_vmess("gui_toggle_update", "xxis",
            canvas,
            x,
            x->x_on != 0.0,
            cbuf);
    }
}

static void toggle__clickhook(t_scalehandle *sh, int newstate)
{
    t_toggle *x = (t_toggle *)(sh->h_master);
    if (newstate)
    {
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
        if (!sh->h_scale)
            scalehandle_click_label(sh);
    }
    sh->h_dragon = newstate;
}

static void toggle__motionhook(t_scalehandle *sh, t_floatarg mouse_x, t_floatarg mouse_y)
{
    if (sh->h_scale)
    {
        t_toggle *x = (t_toggle *)(sh->h_master);
        int width = x->x_gui.x_w,
            height = x->x_gui.x_h;
        int x1, y1, x2, y2, d;
        x1 = text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist);
        y1 = text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist);
        x2 = x1 + width;
        y2 = y1 + height;

        if (mouse_x <= x2)
        {
            if (mouse_y > y2)
                d = mouse_y - y2;
            else if (abs(mouse_y - y2) < abs(mouse_x - x2))
                d = mouse_y - y2;
            else
                d = mouse_x - x2;
        }
        else
        {
            if (mouse_y <= y2)
                d = mouse_x - x2;
            else
                d = maxi(mouse_y - y2, mouse_x - x2);
        }
        sh->h_dragx = d;
        sh->h_dragy = d;
        scalehandle_drag_scale(sh);

        width = maxi(width + d, IEM_GUI_MINSIZE);
        height = width;

        x->x_gui.x_w = width;
        x->x_gui.x_h = height;

        if (glist_isvisible(x->x_gui.x_glist))
        {
            toggle_draw_move(x, x->x_gui.x_glist);
            scalehandle_unclick_scale(sh);
        }

        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            int new_w = x->x_gui.x_w + sh->h_dragx;
            properties_set_field_int(properties,"size",new_w);
        }
    }
    scalehandle_dragon_label(sh,mouse_x, mouse_y);
}

void toggle_draw(t_toggle *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)      sys_queuegui(x, x->x_gui.x_glist, toggle_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)   toggle_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)    toggle_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG) toggle_draw_config(x, glist);
}

/* ------------------------ tgl widgetbehaviour----------------------------- */

static void toggle_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_toggle *x = (t_toggle *)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h;

    iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);
}

static void toggle_save(t_gobj *z, t_binbuf *b)
{
    t_toggle *x = (t_toggle *)z;
    int bflcol[3];
    t_symbol *srl[3];
    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiisssiiiiiiiff;", gensym("#X"),gensym("obj"),
        (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
        gensym("tgl"), x->x_gui.x_w, iem_symargstoint(&x->x_gui),
        srl[0], srl[1], srl[2], x->x_gui.x_ldx, x->x_gui.x_ldy,
        iem_fstyletoint(&x->x_gui), x->x_gui.x_fontsize,
        bflcol[0], bflcol[1], bflcol[2], x->x_on, x->x_nonzero);
}

static void toggle_properties(t_gobj *z, t_glist *owner)
{
    t_toggle *x = (t_toggle *)z;
    char buf[800], *gfx_tag;
    t_symbol *srl[3];
    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |tgl| \
        ----------dimensions(pix):----------- %d %d size: 0 0 empty \
        -----------non-zero-value:----------- %g value: 0.0 empty %g \
        -1 lin log %d %d empty %d {%s} {%s} {%s} %d %d %d %d %d %d %d\n",
        x->x_gui.x_w, IEM_GUI_MINSIZE,
        x->x_nonzero, 1.0,/*non_zero-schedule*/
        x->x_gui.x_loadinit, -1, -1,/*no multi*/
        srl[0]->s_name, srl[1]->s_name, srl[2]->s_name,
        x->x_gui.x_ldx, x->x_gui.x_ldy,
        x->x_gui.x_font_style, x->x_gui.x_fontsize,
        0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
    //gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
    gfx_tag = gfxstub_new2(&x->x_gui.x_obj.ob_pd, x);

    gui_start_vmess("gui_iemgui_dialog", "s", gfx_tag);
    gui_start_array();

    gui_s("type");
    gui_s("tgl");

    gui_s("size");
    gui_i(x->x_gui.x_w);

    gui_s("minimum_size");
    gui_i(IEM_GUI_MINSIZE);

    gui_s("nonzero_value");
    gui_f(x->x_nonzero);

    gui_s("nonzero_schedule");  // no idea what this is...
    gui_f(1.0);

    gui_s("init");
    gui_i(x->x_gui.x_loadinit); 

    gui_s("send_symbol");
    gui_s(srl[0]->s_name);

    gui_s("receive_symbol");
    gui_s(srl[1]->s_name);

    gui_s("label");
    gui_s(srl[2]->s_name);

    gui_s("x_offset");
    gui_i(x->x_gui.x_ldx);

    gui_s("y_offset");
    gui_i(x->x_gui.x_ldy);

    gui_s("font_style");
    gui_i(x->x_gui.x_font_style);

    gui_s("font_size");
    gui_i(x->x_gui.x_fontsize);

    gui_s("background_color");
    gui_i(0xffffff & x->x_gui.x_bcol);

    gui_s("foreground_color");
    gui_i(0xffffff & x->x_gui.x_fcol);

    gui_s("label_color");
    gui_i(0xffffff & x->x_gui.x_lcol);

    gui_end_array();
    gui_end_vmess();
}

static void toggle_bang(t_toggle *x)
{
    x->x_gui.x_changed = 1;
    x->x_on = (x->x_on==0.0)?x->x_nonzero:0.0;
    x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    iemgui_out_float(&x->x_gui, 0, 0, x->x_on);
}

static void toggle_dialog(t_toggle *x, t_symbol *s, int argc, t_atom *argv)
{
    if (atom_getintarg(19, argc, argv))
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
    x->x_gui.x_h =
    x->x_gui.x_w = iemgui_clip_size(atom_getintarg(0, argc, argv));
    t_float nonzero = atom_getfloatarg(2, argc, argv);
    if(nonzero == 0.0)
        nonzero = 1.0;
    x->x_nonzero = nonzero;
    if(x->x_on != 0.0)
        x->x_on = x->x_nonzero;
    int sr_flags = iemgui_dialog(&x->x_gui, argc, argv);
    iemgui_draw_config(&x->x_gui);
    iemgui_draw_io(&x->x_gui, sr_flags);
    iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_MOVE);
    scalehandle_draw(&x->x_gui);
    scrollbar_update(x->x_gui.x_glist);
}

static void toggle_click(t_toggle *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{toggle_bang(x);}

static int toggle_newclick(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    if(doit)
        toggle_click((t_toggle *)z, (t_floatarg)xpix, (t_floatarg)ypix,
            (t_floatarg)shift, 0, (t_floatarg)alt);
    return (1);
}

static void toggle_set(t_toggle *x, t_floatarg f)
{
    if (x->x_on != f) x->x_gui.x_changed = 1;
    x->x_on = f;
    if(f != 0.0)
        x->x_nonzero = f;
    x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void toggle_float(t_toggle *x, t_floatarg f)
{
    toggle_set(x, f);
    if(x->x_gui.x_put_in2out)
        iemgui_out_float(&x->x_gui, 0, 0, x->x_on);
}

static void toggle_fout(t_toggle *x, t_floatarg f)
{
    toggle_set(x, f);
    iemgui_out_float(&x->x_gui, 0, 0, x->x_on);
}

static void toggle_loadbang(t_toggle *x, t_floatarg action)
{
    if (action == LB_LOAD && x->x_gui.x_loadinit)
        toggle_fout(x, (t_float)x->x_on);
}

static void toggle_size(t_toggle *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    x->x_gui.x_h = x->x_gui.x_w;
    iemgui_size(&x->x_gui);
}

static void toggle_nonzero(t_toggle *x, t_floatarg f)
{
    if(f != 0.0)
        x->x_nonzero = f;
}

static void *toggle_new(t_symbol *s, int argc, t_atom *argv)
{
    t_toggle *x = (t_toggle *)pd_new(toggle_class);
    int bflcol[]={-262144, -1, -1};
    int a=IEM_GUI_DEFAULTSIZE;
    int ldx=17, ldy=7;
    int fs=10;
    t_float on=0.0, nonzero=1.0;

    iem_inttosymargs(&x->x_gui, 0);
    iem_inttofstyle(&x->x_gui, 0);

    if(((argc == 13)||(argc == 14))&&IS_A_FLOAT(argv,0)
       &&IS_A_FLOAT(argv,1)
       &&(IS_A_SYMBOL(argv,2)||IS_A_FLOAT(argv,2))
       &&(IS_A_SYMBOL(argv,3)||IS_A_FLOAT(argv,3))
       &&(IS_A_SYMBOL(argv,4)||IS_A_FLOAT(argv,4))
       &&IS_A_FLOAT(argv,5)&&IS_A_FLOAT(argv,6)
       &&IS_A_FLOAT(argv,7)&&IS_A_FLOAT(argv,8)&&IS_A_FLOAT(argv,9)
       &&IS_A_FLOAT(argv,10)&&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12))
    {
        a = atom_getintarg(0, argc, argv);
        iem_inttosymargs(&x->x_gui, atom_getintarg(1, argc, argv));
        iemgui_new_getnames(&x->x_gui, 2, argv);
        ldx = atom_getintarg(5, argc, argv);
        ldy = atom_getintarg(6, argc, argv);
        iem_inttofstyle(&x->x_gui, atom_getintarg(7, argc, argv));
        fs = maxi(atom_getintarg(8, argc, argv),4);
        bflcol[0] = atom_getintarg(9, argc, argv);
        bflcol[1] = atom_getintarg(10, argc, argv);
        bflcol[2] = atom_getintarg(11, argc, argv);
        on = atom_getfloatarg(12, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 2, 0);
    if((argc == 14)&&IS_A_FLOAT(argv,13))
        nonzero = atom_getfloatarg(13, argc, argv);
    x->x_gui.x_draw = (t_iemfunptr)toggle_draw;

    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if (x->x_gui.x_font_style<0 || x->x_gui.x_font_style>2) x->x_gui.x_font_style=0;
    x->x_nonzero = (nonzero!=0.0)?nonzero:1.0;
    if(x->x_gui.x_loadinit)
        x->x_on = (on!=0.0)?nonzero:0.0;
    else
        x->x_on = 0.0;
    if (iemgui_has_rcv(&x->x_gui))
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;

    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = iemgui_clip_size(a);
    x->x_gui.x_h = x->x_gui.x_w;
    iemgui_all_colfromload(&x->x_gui, bflcol);
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    outlet_new(&x->x_gui.x_obj, &s_float);

    x->x_gui.x_handle = scalehandle_new((t_object *)x,x->x_gui.x_glist,1,toggle__clickhook,toggle__motionhook);
    x->x_gui.x_lhandle = scalehandle_new((t_object *)x,x->x_gui.x_glist,0,toggle__clickhook,toggle__motionhook);
    x->x_gui.x_obj.te_iemgui = 1;
    x->x_gui.x_changed = 1;

    x->x_gui.legacy_x = 0;
    x->x_gui.legacy_y = 1;

    //x->x_gui.click_x = 1;
    //x->x_gui.click_y = 2;

    return (x);
}

static void toggle_ff(t_toggle *x)
{
    if(iemgui_has_rcv(&x->x_gui))
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);

    if (x->x_gui. x_handle) scalehandle_free(x->x_gui. x_handle);
    if (x->x_gui.x_lhandle) scalehandle_free(x->x_gui.x_lhandle);
}

void g_toggle_setup(void)
{
    toggle_class = class_new(gensym("tgl"), (t_newmethod)toggle_new,
                             (t_method)toggle_ff, sizeof(t_toggle), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)toggle_new, gensym("toggle"), A_GIMME, 0);
    class_addbang(toggle_class, toggle_bang);
    class_addfloat(toggle_class, toggle_float);
    class_addmethod(toggle_class, (t_method)toggle_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(toggle_class, (t_method)toggle_dialog, gensym("dialog"),
                    A_GIMME, 0);
    class_addmethod(toggle_class, (t_method)toggle_loadbang, gensym("loadbang"),
        A_DEFFLOAT, 0);
    class_addmethod(toggle_class, (t_method)toggle_set, gensym("set"),
        A_FLOAT, 0);
    class_addmethod(toggle_class, (t_method)toggle_size, gensym("size"),
        A_GIMME, 0);
    iemgui_class_addmethods(toggle_class);
    class_addmethod(toggle_class, (t_method)iemgui_init, gensym("init"),
        A_FLOAT, 0);
    class_addmethod(toggle_class, (t_method)toggle_nonzero, gensym("nonzero"),
        A_FLOAT, 0);
 
    wb_init(&toggle_widgetbehavior,toggle_getrect,toggle_newclick);
    class_setwidget(toggle_class, &toggle_widgetbehavior);
    class_sethelpsymbol(toggle_class, gensym("toggle"));
    class_setsavefn(toggle_class, toggle_save);
    class_setpropertiesfn(toggle_class, toggle_properties);
}
