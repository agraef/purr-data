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

#define IEM_BNG_DEFAULTHOLDFLASHTIME 250
#define IEM_BNG_DEFAULTBREAKFLASHTIME 50
#define IEM_BNG_MINHOLDFLASHTIME 50
#define IEM_BNG_MINBREAKFLASHTIME 10

extern int gfxstub_haveproperties(void *key);
t_widgetbehavior bng_widgetbehavior;
static t_class *bng_class;

void bng_draw_update(t_gobj *xgobj, t_glist *glist)
{
    t_bng *x = (t_bng *)xgobj;
    char cbuf[8];
    if (x->x_flashed)
        sprintf(cbuf, "#%6.6x", x->x_gui.x_fcol);
    else
        sprintf(cbuf, "none");
    if (x->x_gui.x_changed != x->x_flashed && glist_isvisible(glist))
    {
        gui_vmess("gui_bng_button_color", "xxs",
            glist_getcanvas(glist),
            &x->x_gui,
            cbuf);
    }
    x->x_gui.x_changed = x->x_flashed;
}

void bng_draw_new(t_bng *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int x1=text_xpix(&x->x_gui.x_obj, glist);
    int y1=text_ypix(&x->x_gui.x_obj, glist);
    iemgui_base_draw_new(&x->x_gui);
    t_float cr = (x->x_gui.x_w-2)/2.0;
    t_float cx = x1+cr+1;
    t_float cy = y1+cr+1;
    /* The circle isn't quite centered in the nw.js port. Let's use the
       old interface to see if there's anything we're doing wrong. Then once
       we get the circle placement right we can remove the old code here... */
    //sys_vgui(".x%zx.c create circle %f %f -r %f "
    //         "-stroke $pd_colors(iemgui_border) -fill #%6.6x "
    //         "-tags {%zxBUT x%zx text iemgui border}\n",
    //     canvas, cx, cy, cr, x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol,
    //     x, x);
    gui_vmess("gui_bng_new", "xxfff",
        canvas,
        &x->x_gui,
        cx - x1,
        cy - y1,
        cr);
}

void bng_draw_move(t_bng *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    char cbuf[8];
    sprintf(cbuf, "#%6.6x", x->x_flashed ? x->x_gui.x_fcol : x->x_gui.x_bcol);
    if (!glist_isvisible(canvas)) return;
    int x1=text_xpix(&x->x_gui.x_obj, glist);
    int y1=text_ypix(&x->x_gui.x_obj, glist);

    iemgui_base_draw_move(&x->x_gui);
    t_float cr = (x->x_gui.x_w-2)/2.0;
    t_float cx = x1+cr+1;
    t_float cy = y1+cr+1;
    gui_vmess("gui_bng_configure", "xxsfff",
        canvas,
        x,
        cbuf,
        cx - x1,
        cy - y1,
        cr);
}

void bng_draw_config(t_bng* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    char cbuf[8];
    if (x->x_flashed)
        sprintf(cbuf, "#%6.6x", x->x_gui.x_fcol);
    else
        sprintf(cbuf, "none");
    iemgui_base_draw_config(&x->x_gui);
    gui_vmess("gui_bng_button_color", "xxs",
        canvas,
        x,
        cbuf);
}

/* we may no longer need h_dragon... */
static void bng__clickhook(t_scalehandle *sh, int newstate)
{
    t_bng *x = (t_bng *)(sh->h_master);
    if (newstate)
    {
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
        if (!sh->h_scale) /* click on a label handle */
            scalehandle_click_label(sh);
    }
    /* We no longer need this "clickhook", as we can handle the dragging
       either in the GUI (for the label handle) or or in canvas_doclick */
    //iemgui__clickhook3(sh,newstate);
    sh->h_dragon = newstate;
}

static void bng__motionhook(t_scalehandle *sh,
                    t_floatarg mouse_x, t_floatarg mouse_y)
{
    if (sh->h_scale)
    {
        t_bng *x = (t_bng *)(sh->h_master);
        int width = x->x_gui.x_w,
            height = x->x_gui.x_h;
        int x1, y1, x2, y2, d;
        x1 = text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist);
        y1 = text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist);
        x2 = x1 + width;
        y2 = y1 + height;

        /* This is convoluted, but I can't think of another
           way to get this behavior... */
        if (mouse_x <= x2)
        {
            if (mouse_y > y2)
                d = mouse_y - y2;
            else if (abs((int)(mouse_y - y2)) < abs((int)(mouse_x - x2)))
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
            bng_draw_move(x, x->x_gui.x_glist);
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

void bng_draw(t_bng *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)      sys_queuegui(x, x->x_gui.x_glist, bng_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)   bng_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)    bng_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG) bng_draw_config(x, glist);
}

/* ------------------------ bng widgetbehaviour----------------------------- */

static void bng_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1,
    int *xp2, int *yp2)
{
    t_bng *x = (t_bng *)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h;

    iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);
}

static void bng_save(t_gobj *z, t_binbuf *b)
{
    t_bng *x = (t_bng *)z;
    t_symbol *bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiiisssiiiisss;", gensym("#X"),gensym("obj"),
        (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
        gensym("bng"), x->x_gui.x_w,
        x->x_flashtime_hold, x->x_flashtime_break,
        iem_symargstoint(&x->x_gui),
        srl[0], srl[1], srl[2], x->x_gui.x_ldx, x->x_gui.x_ldy,
        iem_fstyletoint(&x->x_gui), x->x_gui.x_fontsize,
        bflcol[0], bflcol[1], bflcol[2]);
}

void bng_check_minmax(t_bng *x, int ftbreak, int fthold)
{
    if(ftbreak > fthold)
    {
        int h = ftbreak;
        ftbreak = fthold;
        fthold = h;
    }
    x->x_flashtime_break = maxi(ftbreak,IEM_BNG_MINBREAKFLASHTIME);
    x->x_flashtime_hold  = maxi(fthold,  IEM_BNG_MINHOLDFLASHTIME);
}

static void bng_properties(t_gobj *z, t_glist *owner)
{
    t_bng *x = (t_bng *)z;
    char buf[800], *gfx_tag;
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |bang| \
        ----------dimensions(pix):----------- %d %d size: 0 0 empty \
        --------flash-time(ms)(ms):--------- %d intrrpt: %d hold: %d \
        %d empty empty %d %d empty %d {%s} {%s} {%s} %d %d %d %d %d %d %d\n",

        x->x_gui.x_w,
        IEM_GUI_MINSIZE,
        x->x_flashtime_break,
        x->x_flashtime_hold,
        2, /*min_max_schedule+clip*/
        -1,
        x->x_gui.x_loadinit,
        -1,
        -1, /*no linlog, no multi*/
        srl[0]->s_name, srl[1]->s_name, srl[2]->s_name,
        x->x_gui.x_ldx, x->x_gui.x_ldy,
        x->x_gui.x_font_style, x->x_gui.x_fontsize,
        0xffffff & x->x_gui.x_bcol,
        0xffffff & x->x_gui.x_fcol,
        0xffffff & x->x_gui.x_lcol);

    gfx_tag = gfxstub_new2(&x->x_gui.x_obj.ob_pd, x);
    /* todo: send along the x/y of the object here so we can
       create the window in the right place */

    gui_start_vmess("gui_iemgui_dialog", "s", gfx_tag);
    gui_start_array();

    gui_s("type");
    gui_s("bng");

    gui_s("size"); 
    gui_i(x->x_gui.x_w);

    gui_s("minimum_size");
    gui_i(IEM_GUI_MINSIZE);

    gui_s("range_schedule"); // no idea what this is...
    gui_i(2);

    gui_s("flash_interrupt");
    gui_i(x->x_flashtime_break);

    gui_s("flash_hold");
    gui_i(x->x_flashtime_hold);

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

static void bng_set(t_bng *x)
{
    if(x->x_flashed)
    {
        x->x_flashed = 0;
        x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        clock_delay(x->x_clock_brk, x->x_flashtime_break);
        x->x_flashed = 1;
    }
    else
    {
        x->x_flashed = 1;
        x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
    clock_delay(x->x_clock_hld, x->x_flashtime_hold);
}

static void bng_bout(t_bng *x, int chk_putin)
/* chk_putin=1: wird nur mehr gesendet, wenn snd != rcv*/
/* chk_putin=0: wird immer gesendet, wenn moeglich */
{
    if(!x->x_gui.x_put_in2out)
    {
        x->x_gui.x_locked = 1;
        clock_delay(x->x_clock_lck, 2);
    }
    iemgui_out_bang(&x->x_gui,0,chk_putin);
}
static void bng_bang(t_bng *x)/*wird nur mehr gesendet, wenn snd != rcv*/
{
    if(!x->x_gui.x_locked) {bng_set(x); bng_bout(x,1);}
}

static void bng_bang2(t_bng *x)/*wird immer gesendet, wenn moeglich*/
{
    if(!x->x_gui.x_locked) {bng_set(x); bng_bout(x,0);}
}

static void bng_dialog(t_bng *x, t_symbol *s, int argc, t_atom *argv)
{
    if (atom_getintarg(19, argc, argv))
        canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);
    x->x_gui.x_h = x->x_gui.x_w = atom_getintarg(0, argc, argv);
    int fthold = atom_getintarg(2, argc, argv);
    int ftbreak = atom_getintarg(3, argc, argv);
    int sr_flags = iemgui_dialog(&x->x_gui, argc, argv);
    bng_check_minmax(x, ftbreak, fthold);
    iemgui_draw_config(&x->x_gui);
    iemgui_draw_io(&x->x_gui, sr_flags);
    iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_MOVE);
    scalehandle_draw(&x->x_gui);
    scrollbar_update(x->x_gui.x_glist);
}

static void bng_click(t_bng *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    bng_set(x);
    bng_bout(x,0);
}

static int bng_newclick(t_gobj *z, struct _glist *glist, int xpix, int ypix,
    int shift, int alt, int dbl, int doit)
{
    if(doit)
        bng_click((t_bng *)z, (t_floatarg)xpix, (t_floatarg)ypix,
            (t_floatarg)shift, 0, (t_floatarg)alt);
    return (1);
}

static void bng_float(t_bng *x, t_floatarg f)                  {bng_bang2(x);}
static void bng_symbol(t_bng *x, t_symbol *s)                  {bng_bang2(x);}
static void bng_pointer(t_bng *x, t_gpointer *gp)              {bng_bang2(x);}
static void bng_list(t_bng *x, t_symbol *s, int ac, t_atom *av){bng_bang2(x);}

static void bng_anything(t_bng *x, t_symbol *s, int argc, t_atom *argv)
{bng_bang2(x);}

static void bng_loadbang(t_bng *x, t_floatarg action)
{
    if (action == LB_LOAD && x->x_gui.x_loadinit)
    {
        bng_set(x);
        bng_bout(x,0);
    }
}

static void bng_size(t_bng *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_h = x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    iemgui_size(&x->x_gui);
}

static void bng_flashtime(t_bng *x, t_symbol *s, int ac, t_atom *av)
{
    bng_check_minmax(x, (int)atom_getintarg(0, ac, av),
                     (int)atom_getintarg(1, ac, av));
}

static void bng_tick_hld(t_bng *x)
{
    x->x_flashed = 0;
    x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void bng_tick_brk(t_bng *x)
{
    x->x_gui.x_draw(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void bng_tick_lck(t_bng *x)
{
    x->x_gui.x_locked = 0;
}

static void *bng_new(t_symbol *s, int argc, t_atom *argv)
{
    t_bng *x = (t_bng *)pd_new(bng_class);
    int a=IEM_GUI_DEFAULTSIZE;
    int ldx=17, ldy=7;
    int fs=10;
    int ftbreak=IEM_BNG_DEFAULTBREAKFLASHTIME,
        fthold=IEM_BNG_DEFAULTHOLDFLASHTIME;

    iem_inttosymargs(&x->x_gui, 0);
    iem_inttofstyle(&x->x_gui, 0);

    x->x_gui.x_bcol = 0xFCFCFC;
    x->x_gui.x_fcol = 0x00;
    x->x_gui.x_lcol = 0x00;

    if((argc == 14)&&IS_A_FLOAT(argv,0)
       &&IS_A_FLOAT(argv,1)&&IS_A_FLOAT(argv,2)
       &&IS_A_FLOAT(argv,3)
       &&(IS_A_SYMBOL(argv,4)||IS_A_FLOAT(argv,4))
       &&(IS_A_SYMBOL(argv,5)||IS_A_FLOAT(argv,5))
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&IS_A_FLOAT(argv,7)&&IS_A_FLOAT(argv,8)
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10))
    {
        a = atom_getintarg(0, argc, argv);
        fthold = atom_getintarg(1, argc, argv);
        ftbreak = atom_getintarg(2, argc, argv);
        iem_inttosymargs(&x->x_gui, atom_getintarg(3, argc, argv));
        iemgui_new_getnames(&x->x_gui, 4, argv);
        ldx = atom_getintarg(7, argc, argv);
        ldy = atom_getintarg(8, argc, argv);
        iem_inttofstyle(&x->x_gui, atom_getintarg(9, argc, argv));
        fs = maxi(atom_getintarg(10, argc, argv),4);
        iemgui_all_loadcolors(&x->x_gui, argv+11, argv+12, argv+13);
    }
    else iemgui_new_getnames(&x->x_gui, 4, 0);

    x->x_gui.x_draw = (t_iemfunptr)bng_draw;

    x->x_flashed = 0;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if (x->x_gui.x_font_style<0 || x->x_gui.x_font_style>2) x->x_gui.x_font_style=0;
    if (iemgui_has_rcv(&x->x_gui))
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = iemgui_clip_size(a);
    x->x_gui.x_h = x->x_gui.x_w;
    bng_check_minmax(x, ftbreak, fthold);
    x->x_gui.x_locked = 0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    x->x_clock_hld = clock_new(x, (t_method)bng_tick_hld);
    x->x_clock_brk = clock_new(x, (t_method)bng_tick_brk);
    x->x_clock_lck = clock_new(x, (t_method)bng_tick_lck);
    outlet_new(&x->x_gui.x_obj, &s_bang);

    x->x_gui.x_handle = scalehandle_new((t_object *)x,x->x_gui.x_glist,1,bng__clickhook,bng__motionhook);
    x->x_gui.x_lhandle = scalehandle_new((t_object *)x,x->x_gui.x_glist,0,bng__clickhook,bng__motionhook);
    x->x_gui.x_obj.te_iemgui = 1;
    x->x_gui.x_changed = -1;

    x->x_gui.legacy_x = 0;
    x->x_gui.legacy_y = 1;

    //x->x_gui.click_x = 1;
    //x->x_gui.click_y = 2;

    return (x);
}

static void bng_ff(t_bng *x)
{
    if (iemgui_has_rcv(&x->x_gui))
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    clock_free(x->x_clock_lck);
    clock_free(x->x_clock_brk);
    clock_free(x->x_clock_hld);
    gfxstub_deleteforkey(x);

    if (x->x_gui. x_handle) scalehandle_free(x->x_gui. x_handle);
    if (x->x_gui.x_lhandle) scalehandle_free(x->x_gui.x_lhandle);
}

void g_bang_setup(void)
{
    bng_class = class_new(gensym("bng"), (t_newmethod)bng_new,
                          (t_method)bng_ff, sizeof(t_bng), 0, A_GIMME, 0);
    class_addbang(bng_class, bng_bang);
    class_addfloat(bng_class, bng_float);
    class_addsymbol(bng_class, bng_symbol);
    class_addpointer(bng_class, bng_pointer);
    class_addlist(bng_class, bng_list);
    class_addanything(bng_class, bng_anything);
    class_addmethod(bng_class, (t_method)bng_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(bng_class, (t_method)bng_dialog, gensym("dialog"),
                    A_GIMME, 0);
    class_addmethod(bng_class, (t_method)bng_loadbang, gensym("loadbang"),
        A_DEFFLOAT, 0);
    class_addmethod(bng_class, (t_method)bng_size, gensym("size"), A_GIMME, 0);
    iemgui_class_addmethods(bng_class);
    class_addmethod(bng_class, (t_method)bng_flashtime, gensym("flashtime"),
        A_GIMME, 0);
    class_addmethod(bng_class, (t_method)iemgui_init, gensym("init"), A_FLOAT, 0);
 
    wb_init(&bng_widgetbehavior,bng_getrect,bng_newclick);
    class_setwidget(bng_class, &bng_widgetbehavior);
    class_sethelpsymbol(bng_class, gensym("bng"));
    class_setsavefn(bng_class, bng_save);
    class_setpropertiesfn(bng_class, bng_properties);
}
