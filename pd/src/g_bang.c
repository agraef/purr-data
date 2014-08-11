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

static t_class *scalehandle_class;
extern int gfxstub_haveproperties(void *key);
void bng_draw_select(t_bng* x, t_glist* glist);
 
t_widgetbehavior bng_widgetbehavior;
static t_class *bng_class;

void bng_draw_update(t_gobj *xgobj, t_glist *glist)
{
    t_bng *x = (t_bng *)xgobj;
    
    if (x->x_gui.x_changed != x->x_flashed && glist_isvisible(glist))
    {
        sys_vgui(".x%lx.c itemconfigure %lxBUT -fill #%6.6x\n",
            glist_getcanvas(glist), x,
            x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol);
    }
    x->x_gui.x_changed = x->x_flashed;
}
void bng_draw_io(t_bng* x, t_glist* glist, int old_snd_rcv_flags)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_io_draw(&x->x_gui,canvas,old_snd_rcv_flags);
}

void bng_draw_new(t_bng *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

    scalehandle_draw_new(x->x_gui. x_handle,canvas);
    scalehandle_draw_new(x->x_gui.x_lhandle,canvas);

    char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

    iemgui_base_draw_new(&x->x_gui, canvas, nlet_tag);
    t_float cr = (x->x_gui.x_w-2)/2.0;
    t_float cx = xpos+cr+1.5;
    t_float cy = ypos+cr+1.5;
    sys_vgui(".x%lx.c create circle %f %f -r %f "
             "-stroke $pd_colors(iemgui_border) -fill #%6.6x "
             "-tags {%lxBUT %lxOBJ text iemgui border %s}\n",
         canvas, cx, cy, cr, x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol,
         x, x, nlet_tag);
    iemgui_label_draw_new(&x->x_gui,canvas,xpos,ypos,nlet_tag);
    bng_draw_io(x,glist,7);
}

void bng_draw_move(t_bng *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    if (!glist_isvisible(canvas)) return;
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);

    char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);
    iemgui_base_draw_move(&x->x_gui, canvas, nlet_tag);
    t_float cr = (x->x_gui.x_w-2)/2.0;
    t_float cx = xpos+cr+1.5;
    t_float cy = ypos+cr+1.5;
    sys_vgui(".x%lx.c coords %lxBUT %f %f\n", canvas, x, cx, cy);
    sys_vgui(".x%lx.c itemconfigure %lxBUT -fill #%6.6x -r %f\n",
        canvas, x, x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol, cr);
    iemgui_label_draw_move(&x->x_gui,canvas,xpos,ypos);
    iemgui_io_draw_move(&x->x_gui,canvas,nlet_tag);
    if (x->x_gui.x_selected) bng_draw_select(x, x->x_gui.x_glist);
}

void bng_draw_config(t_bng* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_label_draw_config(&x->x_gui,canvas);
    iemgui_base_draw_config(&x->x_gui,canvas);
    sys_vgui(".x%lx.c itemconfigure %lxBUT -fill #%6.6x\n",
        canvas, x, x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol);
}

void bng_draw_select(t_bng* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    iemgui_base_draw_config(&x->x_gui,canvas);
    if(x->x_gui.x_selected)
    {
        /* check if we are drawing inside a gop abstraction visible
           on parent canvas -- if so, disable highlighting */
        if (x->x_gui.x_glist == glist_getcanvas(glist))
        {
            sys_vgui(".x%lx.c itemconfigure %lxBUT "
                     "-stroke $pd_colors(selection)\n", canvas, x);
            scalehandle_draw_select2(&x->x_gui,glist);
        }
    }
    else
    {
        sys_vgui(".x%lx.c itemconfigure %lxBUT -stroke %s\n",
            canvas, x, IEM_GUI_COLOR_NORMAL);
        scalehandle_draw_erase2(&x->x_gui,glist);
    }
    iemgui_label_draw_select(&x->x_gui,canvas);
    iemgui_tag_selected(&x->x_gui,canvas);
}

static void bng__clickhook(t_scalehandle *sh, t_floatarg f,
    t_floatarg xxx, t_floatarg yyy)
{
    t_bng *x = (t_bng *)(sh->h_master);
    int newstate = (int)f;
    if (sh->h_dragon && newstate == 0 && sh->h_scale)
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
            bng_draw_move(x, x->x_gui.x_glist);
            scalehandle_unclick_scale(sh);
        }
    }
    iemgui__clickhook3(sh,newstate);
}

static void bng__motionhook(t_scalehandle *sh,
                    t_floatarg f1, t_floatarg f2)
{
    if (sh->h_dragon && sh->h_scale)
    {
        t_bng *x = (t_bng *)(sh->h_master);
        int d = maxi((int)f1,(int)f2);
        d = maxi(d,IEM_GUI_MINSIZE-x->x_gui.x_w);
        sh->h_dragx = d;
        sh->h_dragy = d;
        scalehandle_drag_scale(sh);

        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            int new_w = x->x_gui.x_w + sh->h_dragx;
            properties_set_field_int(properties,"dim.w_ent",new_w);
        }
    }
    scalehandle_dragon_label(sh,f1,f2);
}

void bng_draw(t_bng *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui((t_gobj*)x, x->x_gui.x_glist, bng_draw_update);
        //bng_draw_update(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        bng_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
    {
        bng_draw_new(x, glist);
        sys_vgui(".x%lx.c raise all_cords\n", glist_getcanvas(glist));
    }
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        bng_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        iemgui_draw_erase(&x->x_gui, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        bng_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        bng_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
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
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiiisssiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("bng"), x->x_gui.x_w,
                x->x_flashtime_hold, x->x_flashtime_break,
                iem_symargstoint(&x->x_gui),
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2]);
    binbuf_addv(b, ";");
}

void bng_check_minmax(t_bng *x, int ftbreak, int fthold)
{
    if(ftbreak > fthold)
    {
        int h = ftbreak;
        ftbreak = fthold;
        fthold = h;
    }
    if(ftbreak < IEM_BNG_MINBREAKFLASHTIME)
        ftbreak = IEM_BNG_MINBREAKFLASHTIME;
    if(fthold < IEM_BNG_MINHOLDFLASHTIME)
        fthold = IEM_BNG_MINHOLDFLASHTIME;
    x->x_flashtime_break = ftbreak;
    x->x_flashtime_hold = fthold;
}

static void bng_properties(t_gobj *z, t_glist *owner)
{
    t_bng *x = (t_bng *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |bang| \
            ----------dimensions(pix):----------- %d %d size: 0 0 empty \
            --------flash-time(ms)(ms):--------- %d intrrpt: %d hold: %d \
            %d empty empty %d %d empty %d \
            {%s} {%s} \
            {%s} %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, IEM_GUI_MINSIZE,
            x->x_flashtime_break, x->x_flashtime_hold, 2,/*min_max_schedule+clip*/
            -1, x->x_gui.x_loadinit, -1, -1,/*no linlog, no multi*/
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void bng_set(t_bng *x)
{
    if(x->x_flashed)
    {
        x->x_flashed = 0;
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        clock_delay(x->x_clock_brk, x->x_flashtime_break);
        x->x_flashed = 1;
    }
    else
    {
        x->x_flashed = 1;
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    }
    clock_delay(x->x_clock_hld, x->x_flashtime_hold);
}

static void bng_bout1(t_bng *x)/*wird nur mehr gesendet, wenn snd != rcv*/
{
    if(!x->x_gui.x_put_in2out)
    {
        x->x_gui.x_locked = 1;
        clock_delay(x->x_clock_lck, 2);
    }
    outlet_bang(x->x_gui.x_obj.ob_outlet);
    if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing &&
        x->x_gui.x_put_in2out)
        pd_bang(x->x_gui.x_snd->s_thing);
}

static void bng_bout2(t_bng *x)/*wird immer gesendet, wenn moeglich*/
{
    if(!x->x_gui.x_put_in2out)
    {
        x->x_gui.x_locked = 1;
        clock_delay(x->x_clock_lck, 2);
    }
    outlet_bang(x->x_gui.x_obj.ob_outlet);
    if(iemgui_has_snd(&x->x_gui) && x->x_gui.x_snd->s_thing)
        pd_bang(x->x_gui.x_snd->s_thing);
}

static void bng_bang(t_bng *x)/*wird nur mehr gesendet, wenn snd != rcv*/
{
    if(!x->x_gui.x_locked)
    {
        bng_set(x);
        bng_bout1(x);
    }
}

static void bng_bang2(t_bng *x)/*wird immer gesendet, wenn moeglich*/
{
    if(!x->x_gui.x_locked)
    {
        bng_set(x);
        bng_bout2(x);
    }
}

static void bng_dialog(t_bng *x, t_symbol *s, int argc, t_atom *argv)
{
    canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

    t_symbol *srl[3];
    int a = (int)atom_getintarg(0, argc, argv);
    int fthold = (int)atom_getintarg(2, argc, argv);
    int ftbreak = (int)atom_getintarg(3, argc, argv);
    int sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);

    x->x_gui.x_w = iemgui_clip_size(a);
    x->x_gui.x_h = x->x_gui.x_w;
    bng_check_minmax(x, ftbreak, fthold);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    //(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    //canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
    iemgui_shouldvis(&x->x_gui, IEM_GUI_DRAW_MODE_MOVE);

    /* forcing redraw of the scale handle */
    if (x->x_gui.x_selected)
    {
        bng_draw_select(x, x->x_gui.x_glist);
    }
    scrollbar_update(x->x_gui.x_glist);
}

static void bng_click(t_bng *x, t_floatarg xpos, t_floatarg ypos,
    t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    bng_set(x);
    bng_bout2(x);
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

static void bng_loadbang(t_bng *x)
{
    if(!sys_noloadbang && x->x_gui.x_loadinit)
    {
        bng_set(x);
        bng_bout2(x);
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
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void bng_tick_brk(t_bng *x)
{
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void bng_tick_lck(t_bng *x)
{
    x->x_gui.x_locked = 0;
}

static void *bng_new(t_symbol *s, int argc, t_atom *argv)
{
    t_bng *x = (t_bng *)pd_new(bng_class);
    int bflcol[]={-262144, -1, -1};
    int a=IEM_GUI_DEFAULTSIZE;
    int ldx=17, ldy=7;
    int fs=10;
    int ftbreak=IEM_BNG_DEFAULTBREAKFLASHTIME,
        fthold=IEM_BNG_DEFAULTHOLDFLASHTIME;

    iem_inttosymargs(&x->x_gui, 0);
    iem_inttofstyle(&x->x_gui, 0);

    if((argc == 14)&&IS_A_FLOAT(argv,0)
       &&IS_A_FLOAT(argv,1)&&IS_A_FLOAT(argv,2)
       &&IS_A_FLOAT(argv,3)
       &&(IS_A_SYMBOL(argv,4)||IS_A_FLOAT(argv,4))
       &&(IS_A_SYMBOL(argv,5)||IS_A_FLOAT(argv,5))
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&IS_A_FLOAT(argv,7)&&IS_A_FLOAT(argv,8)
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)&&IS_A_FLOAT(argv,11)
       &&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13))
    {

        a = (int)atom_getintarg(0, argc, argv);
        fthold = (int)atom_getintarg(1, argc, argv);
        ftbreak = (int)atom_getintarg(2, argc, argv);
        iem_inttosymargs(&x->x_gui, atom_getintarg(3, argc, argv));
        iemgui_new_getnames(&x->x_gui, 4, argv);
        ldx = (int)atom_getintarg(7, argc, argv);
        ldy = (int)atom_getintarg(8, argc, argv);
        iem_inttofstyle(&x->x_gui, atom_getintarg(9, argc, argv));
        fs = (int)atom_getintarg(10, argc, argv);
        bflcol[0] = (int)atom_getintarg(11, argc, argv);
        bflcol[1] = (int)atom_getintarg(12, argc, argv);
        bflcol[2] = (int)atom_getintarg(13, argc, argv);
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

    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = iemgui_clip_size(a);
    x->x_gui.x_h = x->x_gui.x_w;
    bng_check_minmax(x, ftbreak, fthold);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    x->x_gui.x_locked = 0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    x->x_clock_hld = clock_new(x, (t_method)bng_tick_hld);
    x->x_clock_brk = clock_new(x, (t_method)bng_tick_brk);
    x->x_clock_lck = clock_new(x, (t_method)bng_tick_lck);
    outlet_new(&x->x_gui.x_obj, &s_bang);

    x->x_gui. x_handle = scalehandle_new(scalehandle_class,(t_iemgui *)x,1);
    x->x_gui.x_lhandle = scalehandle_new(scalehandle_class,(t_iemgui *)x,0);
    x->x_gui.x_obj.te_iemgui = 1;
    x->x_gui.x_changed = 0;

    return (x);
}

static void bng_ff(t_bng *x)
{
    if(iemgui_has_rcv(&x->x_gui))
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
    class_addmethod(bng_class, (t_method)bng_loadbang, gensym("loadbang"), 0);
    class_addmethod(bng_class, (t_method)bng_size, gensym("size"), A_GIMME, 0);
    iemgui_class_addmethods(bng_class);
    class_addmethod(bng_class, (t_method)bng_flashtime, gensym("flashtime"),
        A_GIMME, 0);
    class_addmethod(bng_class, (t_method)iemgui_init, gensym("init"), A_FLOAT, 0);
 
    scalehandle_class = class_new(gensym("_scalehandle"), 0, 0,
                  sizeof(t_scalehandle), CLASS_PD, 0);
    class_addmethod(scalehandle_class, (t_method)bng__clickhook,
            gensym("_click"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scalehandle_class, (t_method)bng__motionhook,
            gensym("_motion"), A_FLOAT, A_FLOAT, 0);

    wb_init(&bng_widgetbehavior,bng_getrect,bng_newclick);
    class_setwidget(bng_class, &bng_widgetbehavior);
    class_sethelpsymbol(bng_class, gensym("bng"));
    class_setsavefn(bng_class, bng_save);
    class_setpropertiesfn(bng_class, bng_properties);
}
