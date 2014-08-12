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
#include "m_imp.h"
#include "g_all_guis.h"
#include <math.h>

t_symbol *s_empty;

int iemgui_color_hex[]=
{
    16579836, 10526880, 4210752, 16572640, 16572608,
    16579784, 14220504, 14220540, 14476540, 16308476,
    14737632, 8158332, 2105376, 16525352, 16559172,
    15263784, 1370132, 2684148, 3952892, 16003312,
    12369084, 6316128, 0, 9177096, 5779456,
    7874580, 2641940, 17488, 5256, 5767248
};

int iemgui_clip_size(int size)
{
    if(size < IEM_GUI_MINSIZE)
        size = IEM_GUI_MINSIZE;
    return(size);
}

int iemgui_clip_font(int size)
{
    if(size < IEM_FONT_MINSIZE)
        size = IEM_FONT_MINSIZE;
    return(size);
}

static int iemgui_modulo_color(int col)
{
    col %= IEM_GUI_MAX_COLOR;
    if (col<0) col += IEM_GUI_MAX_COLOR;
    return col;
}

t_symbol *iemgui_dollar2raute(t_symbol *s)
{
    char buf[MAXPDSTRING+1], *s1, *s2;
    if (strlen(s->s_name) >= MAXPDSTRING)
        return (s);
    for (s1 = s->s_name, s2 = buf; ; s1++, s2++)
    {
        if (*s1 == '$')
            *s2 = '#';
        else if (!(*s2 = *s1))
            break;
    }
    return(gensym(buf));
}

t_symbol *iemgui_raute2dollar(t_symbol *s)
{
    char buf[MAXPDSTRING+1], *s1, *s2;
    if (strlen(s->s_name) >= MAXPDSTRING)
        return (s);
    for (s1 = s->s_name, s2 = buf; ; s1++, s2++)
    {
        if (*s1 == '#')
            *s2 = '$';
        else if (!(*s2 = *s1))
            break;
    }
    return(gensym(buf));
}

void iemgui_verify_snd_ne_rcv(t_iemgui *iemgui)
{
    iemgui->x_put_in2out = 1;
    if(iemgui_has_snd(iemgui) && iemgui_has_rcv(iemgui))
    {
        if(iemgui->x_snd==iemgui->x_rcv)
            iemgui->x_put_in2out = 0;
    }
}

t_symbol *iemgui_new_dogetname(t_iemgui *iemgui, int indx, t_atom *argv)
{
    if (IS_A_SYMBOL(argv, indx))
        return (atom_getsymbolarg(indx, 100000, argv));
    else if (IS_A_FLOAT(argv, indx))
    {
        char str[80];
        sprintf(str, "%d", (int)atom_getintarg(indx, 100000, argv));
        return (gensym(str));
    }
    else return s_empty;
}

void iemgui_new_getnames(t_iemgui *iemgui, int indx, t_atom *argv)
{
    if (argv)
    {
        iemgui->x_snd = iemgui_new_dogetname(iemgui, indx, argv);
        iemgui->x_rcv = iemgui_new_dogetname(iemgui, indx+1, argv);
        iemgui->x_lab = iemgui_new_dogetname(iemgui, indx+2, argv);
    }
    else iemgui->x_snd = iemgui->x_rcv = iemgui->x_lab = s_empty;
    iemgui->x_snd_unexpanded = iemgui->x_rcv_unexpanded =
        iemgui->x_lab_unexpanded = 0;
    iemgui->x_binbufindex = indx;
    iemgui->x_labelbindex = indx + 3;
}

    /* convert symbols in "$" form to the expanded symbols */
void iemgui_all_dollararg2sym(t_iemgui *iemgui, t_symbol **srlsym)
{
        /* save unexpanded ones for later */
    iemgui->x_snd_unexpanded = srlsym[0];
    iemgui->x_rcv_unexpanded = srlsym[1];
    iemgui->x_lab_unexpanded = srlsym[2];
    srlsym[0] = canvas_realizedollar(iemgui->x_glist, srlsym[0]);
    srlsym[1] = canvas_realizedollar(iemgui->x_glist, srlsym[1]);
    srlsym[2] = canvas_realizedollar(iemgui->x_glist, srlsym[2]);
}

    /* initialize a single symbol in unexpanded form.  We reach into the
    binbuf to grab them; if there's nothing there, set it to the
    fallback; if still nothing, set to "empty". */
static void iemgui_init_sym2dollararg(t_iemgui *iemgui, t_symbol **symp,
    int indx, t_symbol *fallback)
{
    if (!*symp)
    {
        t_binbuf *b = iemgui->x_obj.ob_binbuf;
        if (binbuf_getnatom(b) > indx)
        {
            char buf[80];
            atom_string(binbuf_getvec(b) + indx, buf, 80);
            *symp = gensym(buf);
        }
        else if (fallback)
            *symp = fallback;
        else *symp = s_empty;
    }
}

/* get the unexpanded versions of the symbols; initialize them if necessary. */
void iemgui_all_sym2dollararg(t_iemgui *iemgui, t_symbol **srlsym)
{
    iemgui_init_sym2dollararg(iemgui, &iemgui->x_snd_unexpanded,
        iemgui->x_binbufindex+1, iemgui->x_snd);
    iemgui_init_sym2dollararg(iemgui, &iemgui->x_rcv_unexpanded,
        iemgui->x_binbufindex+2, iemgui->x_rcv);
    iemgui_init_sym2dollararg(iemgui, &iemgui->x_lab_unexpanded,
        iemgui->x_labelbindex, iemgui->x_lab);
    srlsym[0] = iemgui->x_snd_unexpanded;
    srlsym[1] = iemgui->x_rcv_unexpanded;
    srlsym[2] = iemgui->x_lab_unexpanded;
}

static int col2save(int col) {
    return -1-(((0xfc0000 & col) >> 6)|((0xfc00 & col) >> 4)|((0xfc & col) >> 2));
}
void iemgui_all_col2save(t_iemgui *iemgui, int *bflcol)
{
    bflcol[0] = col2save(iemgui->x_bcol);
    bflcol[1] = col2save(iemgui->x_fcol);
    bflcol[2] = col2save(iemgui->x_lcol);
}

static int colfromload(int col) {
    if(col)
    {
        col = -1-col;
        return ((col & 0x3f000) << 6)|((col & 0xfc0) << 4)|((col & 0x3f) << 2);
    }
    else
        return iemgui_color_hex[iemgui_modulo_color(col)];
}
void iemgui_all_colfromload(t_iemgui *iemgui, int *bflcol)
{
    iemgui->x_bcol = colfromload(bflcol[0]);
    iemgui->x_fcol = colfromload(bflcol[1]);
    iemgui->x_lcol = colfromload(bflcol[2]);
}

static int iemgui_compatible_col(int i)
{
    if(i >= 0)
        return(iemgui_color_hex[(iemgui_modulo_color(i))]);
    return((-1-i)&0xffffff);
}

void iemgui_all_dollar2raute(t_symbol **srlsym)
{
    srlsym[0] = iemgui_dollar2raute(srlsym[0]);
    srlsym[1] = iemgui_dollar2raute(srlsym[1]);
    srlsym[2] = iemgui_dollar2raute(srlsym[2]);
}

void iemgui_all_raute2dollar(t_symbol **srlsym)
{
    srlsym[0] = iemgui_raute2dollar(srlsym[0]);
    srlsym[1] = iemgui_raute2dollar(srlsym[1]);
    srlsym[2] = iemgui_raute2dollar(srlsym[2]);
}

void iemgui_send(t_iemgui *x, t_symbol *s)
{
    t_symbol *snd;
    int oldsndrcvable=0;
    if(iemgui_has_rcv(x)) oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui_has_snd(x)) oldsndrcvable += IEM_GUI_OLD_SND_FLAG;

    snd = iemgui_raute2dollar(s);
    x->x_snd_unexpanded = snd;
    x->x_snd = snd = canvas_realizedollar(x->x_glist, snd);
    iemgui_verify_snd_ne_rcv(x);
    x->x_draw(x, x->x_glist, IEM_GUI_DRAW_MODE_IO + oldsndrcvable);
}

void iemgui_receive(t_iemgui *x, t_symbol *s)
{
    t_symbol *rcv;
    int oldsndrcvable=0;
    if(iemgui_has_rcv(x)) oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui_has_snd(x)) oldsndrcvable += IEM_GUI_OLD_SND_FLAG;

    rcv = iemgui_raute2dollar(s);
    x->x_rcv_unexpanded = rcv;
    rcv = canvas_realizedollar(x->x_glist, rcv);
    if(iemgui_has_rcv(x))
    {
        if(rcv!=x->x_rcv)
        {
            if(iemgui_has_rcv(x))
                pd_unbind((t_pd *)x, x->x_rcv);
            x->x_rcv = rcv;
            pd_bind((t_pd *)x, x->x_rcv);
        }
    }
    else if(s!=s_empty && iemgui_has_rcv(x))
    {
        pd_unbind((t_pd *)x, x->x_rcv);
        x->x_rcv = rcv;
    }
    iemgui_verify_snd_ne_rcv(x);
    x->x_draw(x, x->x_glist, IEM_GUI_DRAW_MODE_IO + oldsndrcvable);
}

void iemgui_label(t_iemgui *x, t_symbol *s)
{
    if (s == &s_) s = s_empty; //tb: fix for empty label
    t_symbol *lab = iemgui_raute2dollar(s);
    x->x_lab_unexpanded = lab;
    x->x_lab = lab = canvas_realizedollar(x->x_glist, lab);

    if(glist_isvisible(x->x_glist))
    {
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -text {%s} \n",
            glist_getcanvas(x->x_glist), x,
            s!=s_empty?x->x_lab->s_name:"");
        iemgui_shouldvis(x, IEM_GUI_DRAW_MODE_CONFIG);
    }
}

void iemgui_label_pos(t_iemgui *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_ldx = (int)atom_getintarg(0, ac, av);
    x->x_ldy = (int)atom_getintarg(1, ac, av);
    if(glist_isvisible(x->x_glist))
    {
        sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
            glist_getcanvas(x->x_glist), x,
            text_xpix((t_object *)x,x->x_glist)+x->x_ldx,
            text_ypix((t_object *)x,x->x_glist)+x->x_ldy);
        iemgui_shouldvis(x, IEM_GUI_DRAW_MODE_CONFIG);
    }
}

void iemgui_label_font(t_iemgui *x, t_symbol *s, int ac, t_atom *av)
{
    int f = (int)atom_getintarg(0, ac, av);
    if (f<0 || f>2) f=0;
    x->x_font_style = f;
    f = (int)atom_getintarg(1, ac, av);
    if(f < 4)
        f = 4;
    x->x_fontsize = f;
    if(glist_isvisible(x->x_glist))
    {
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} -%d %s}\n",
            glist_getcanvas(x->x_glist), x, iemgui_font(x), 
            x->x_fontsize, sys_fontweight);
            iemgui_shouldvis(x, IEM_GUI_DRAW_MODE_CONFIG);
    }
}

//Sans: 84 x 10 (14) -> 6 x 10 -> 1.0
//Helvetica: 70 x 10 (14) -> 5 x 10 -> 0.83333
//Times: 61 x 10 (14) -> 4.357 x 10 -> 0.72619; 0.735 appears to work better

// We use this global var to check when getrect should report label:
// It should report it when drawing inside gop to see if we truly fit.
// Otherwise we should not report it while inside gop to avoid label being
// misinterpreted as part of the "hot" area of a widget (e.g. toggle)
extern int gop_redraw;

void iemgui_label_getrect(t_iemgui x_gui, t_glist *x,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    //fprintf(stderr,"gop_redraw = %d\n", gop_redraw);
    if (!gop_redraw)
    {
        //fprintf(stderr,"ignoring label\n");
        return;
    }

    t_float width_multiplier;
    int label_length;
    int label_x1;
    int label_y1;
    int label_x2;
    int label_y2;
    int actual_fontsize; //seems tk does its own thing when rendering
    int actual_height;

    if (x->gl_isgraph && !glist_istoplevel(x))
    {
        if (x_gui.x_lab!=s_empty)
        {
            switch(x_gui.x_font_style)
            {
                case 1:  width_multiplier = 0.83333; break;
                case 2:  width_multiplier = 0.735;   break;
                default: width_multiplier = 1.0;     break;
            }
            actual_fontsize = x_gui.x_fontsize;
            actual_height = actual_fontsize;
            //exceptions
            if (x_gui.x_font_style == 0 &&
                (actual_fontsize == 8 || actual_fontsize == 13 ||
                actual_fontsize % 10 == 1 || actual_fontsize % 10 == 6 ||
                    (actual_fontsize > 48 && actual_fontsize < 100 &&
                    (actual_fontsize %10 == 4 || actual_fontsize %10 == 9))))
            {
                actual_fontsize += 1;
            }
            else if (x_gui.x_font_style == 1 && actual_fontsize >= 5 &&
                actual_fontsize < 13 && actual_fontsize % 2 == 1)
                actual_fontsize += 1;
            else if (x_gui.x_font_style == 2 && actual_fontsize >= 5 &&
                actual_fontsize % 2 == 1)
                actual_fontsize += 1;
            if (actual_height == 9)
                actual_height += 1;
            //done with exceptions

            width_multiplier = width_multiplier * (actual_fontsize * 0.6);

            label_length = strlen(x_gui.x_lab->s_name);
            label_x1 = *xp1 + x_gui.x_ldx;
            label_y1 = *yp1 + x_gui.x_ldy - actual_height/2;
            label_x2 = label_x1 + (label_length * width_multiplier);
            label_y2 = label_y1 + actual_height*1.1;

            //DEBUG
            //fprintf(stderr,"%f %d %d\n", width_multiplier,
            //    label_length, x_gui.x_font_style);
            //sys_vgui(".x%lx.c delete iemguiDEBUG\n", x);
            //sys_vgui(".x%lx.c create rectangle %d %d %d %d "
            //    "-tags iemguiDEBUG\n",
            //    x, label_x1, label_y1, label_x2, label_y2);
            if (label_x1 < *xp1) *xp1 = label_x1;
            if (label_x2 > *xp2) *xp2 = label_x2;
            if (label_y1 < *yp1) *yp1 = label_y1;
            if (label_y2 > *yp2) *yp2 = label_y2;
            //DEBUG
            //sys_vgui(".x%lx.c delete iemguiDEBUG\n", x);
            //sys_vgui(".x%lx.c create rectangle %d %d %d %d "
            //    "-tags iemguiDEBUG\n", x, *xp1, *yp1, *xp2, *yp2);
        }
    }
}

void iemgui_shouldvis(t_iemgui *x, int mode)
{
    gop_redraw = 1;
    if(gobj_shouldvis((t_gobj *)x, x->x_glist))
    {
        if (!x->x_vis)
        {
            //fprintf(stderr,"draw new %d\n", mode);
            x->x_draw(x, x->x_glist, IEM_GUI_DRAW_MODE_NEW);
            canvas_fixlinesfor(glist_getcanvas(x->x_glist), (t_text*)x);
            x->x_vis = 1;
            if (x->x_glist != glist_getcanvas(x->x_glist))
            {
                /* if we are inside gop and just have had our object's
                   properties changed we'll adjust our layer position
                   to ensure that ordering is honored */
                t_canvas *canvas = glist_getcanvas(x->x_glist);
                t_gobj *y = (t_gobj *)x->x_glist;
                gobj_vis(y, canvas, 0);
                gobj_vis(y, canvas, 1);
                // reorder it visually
                glist_redraw(canvas);

                /*
                    // some day when the object tagging is
                    // properly done for all GUI objects
                glist_noselect(canvas);
                glist_select(canvas, y);
                t_gobj *yy = canvas->gl_list;
                if (yy != y)
                {
                    fprintf(stderr,"not bottom\n");
                    while (yy && yy->g_next != y)
                    {
                        fprintf(stderr,"+\n");
                        yy = yy->g_next;
                    }
                    // now we have yy which is right before our y graph
                    t_object *ob = NULL;
                    t_rtext *yr = NULL;
                    if (yy)
                    {
                        yr = glist_findrtext(canvas, (t_text *)yy);
                    }
                    if (yr)
                    {
                        fprintf(stderr,"lower\n");
                        sys_vgui(".x%lx.c lower selected %s\n",
                            canvas, rtext_gettag(yr));
                        sys_vgui(".x%lx.c raise selected %s\n",
                            canvas, rtext_gettag(yr));
                        //sys_vgui(".x%lx.c raise all_cords\n", canvas);
                    }
                    else
                    {
                        // fall back to legacy redraw for objects
                        //   that are not patchable
                        fprintf(stderr,"lower fallback redraw\n");
                        canvas_redraw(canvas);
                    }
                }
                else
                {
                    // we get here if we are supposed to go
                    //   all the way to the bottom
                    fprintf(stderr,"lower to the bottom\n");
                    sys_vgui(".x%lx.c lower selected\n", canvas);
                }
                glist_noselect(canvas);
                */
            }
        }
        //fprintf(stderr,"draw move x->x_w=%d\n", x->x_w);
        x->x_draw(x, x->x_glist, mode);
        canvas_fixlinesfor(glist_getcanvas(x->x_glist), (t_text*)x);
    }
    else if (x->x_vis)
    {
        //fprintf(stderr,"draw erase %d\n", mode);
        x->x_draw(x, x->x_glist, IEM_GUI_DRAW_MODE_ERASE);
        x->x_vis = 0;
    }
    gop_redraw = 0;
}

void iemgui_size(t_iemgui *x)
{
    if(glist_isvisible(x->x_glist))
        iemgui_shouldvis(x, IEM_GUI_DRAW_MODE_MOVE);
}

void iemgui_delta(t_iemgui *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_obj.te_xpix += (int)atom_getintarg(0, ac, av);
    x->x_obj.te_ypix += (int)atom_getintarg(1, ac, av);
    if(glist_isvisible(x->x_glist))
    {
        iemgui_shouldvis(x, IEM_GUI_DRAW_MODE_MOVE);
    }
}

void iemgui_pos(t_iemgui *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_obj.te_xpix = (int)atom_getintarg(0, ac, av);
    x->x_obj.te_ypix = (int)atom_getintarg(1, ac, av);
    if(glist_isvisible(x->x_glist))
        iemgui_shouldvis(x, IEM_GUI_DRAW_MODE_MOVE);
}

void iemgui_color(t_iemgui *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_bcol = iemgui_compatible_col(atom_getintarg(0, ac, av));
    if(ac > 2)
    {
        x->x_fcol = iemgui_compatible_col(atom_getintarg(1, ac, av));
        x->x_lcol = iemgui_compatible_col(atom_getintarg(2, ac, av));
    }
    else
        x->x_lcol = iemgui_compatible_col(atom_getintarg(1, ac, av));
    if(glist_isvisible(x->x_glist))
        x->x_draw(x, x->x_glist, IEM_GUI_DRAW_MODE_CONFIG);
}

void iemgui_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_iemgui *x = (t_iemgui *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    iemgui_shouldvis(x, IEM_GUI_DRAW_MODE_MOVE);
}

void iemgui_displace_withtag(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_iemgui *x = (t_iemgui *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    //x->x_gui.x_draw((void *)z, glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(glist), (t_text *)z);
}


void iemgui_select(t_gobj *z, t_glist *glist, int selected)
{
    t_iemgui *x = (t_iemgui *)z;

    x->x_selected = selected;
    x->x_draw((void *)z, glist, IEM_GUI_DRAW_MODE_SELECT);
}

void iemgui_delete(t_gobj *z, t_glist *glist)
{
    canvas_deletelinesfor(glist, (t_text *)z);
}

void iemgui_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_iemgui *x = (t_iemgui *)z;
    if (gobj_shouldvis(z, glist))
    {
        if (vis)
        {
            x->x_draw((void *)z, glist, IEM_GUI_DRAW_MODE_NEW);
        }
        else
        {
            x->x_draw((void *)z, glist, IEM_GUI_DRAW_MODE_ERASE);
            sys_unqueuegui(z);
        }
        x->x_vis = vis;
    }
}

void iemgui_save(t_iemgui *iemgui, t_symbol **srl, int *bflcol)
{
    srl[0] = iemgui->x_snd;
    srl[1] = iemgui->x_rcv;
    srl[2] = iemgui->x_lab;
    iemgui_all_sym2dollararg(iemgui, srl);
    iemgui_all_col2save(iemgui, bflcol);
}

void iemgui_properties(t_iemgui *iemgui, t_symbol **srl)
{
    srl[0] = iemgui->x_snd;
    srl[1] = iemgui->x_rcv;
    srl[2] = iemgui->x_lab;
    iemgui_all_sym2dollararg(iemgui, srl);
    iemgui_all_dollar2raute(srl);
}

int iemgui_dialog(t_iemgui *iemgui, t_symbol **srl, int argc, t_atom *argv)
{
    char str[144];
    int init = (int)atom_getintarg(5, argc, argv);
    int ldx = (int)atom_getintarg(10, argc, argv);
    int ldy = (int)atom_getintarg(11, argc, argv);
    int f = (int)atom_getintarg(12, argc, argv);
    int fs = (int)atom_getintarg(13, argc, argv);
    int bcol = (int)atom_getintarg(14, argc, argv);
    int fcol = (int)atom_getintarg(15, argc, argv);
    int lcol = (int)atom_getintarg(16, argc, argv);
    int rcvable=1, oldsndrcvable=0;

    if(iemgui_has_rcv(iemgui))
        oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui_has_snd(iemgui))
        oldsndrcvable += IEM_GUI_OLD_SND_FLAG;
    if(IS_A_SYMBOL(argv,7))
        srl[0] = atom_getsymbolarg(7, argc, argv);
    else if(IS_A_FLOAT(argv,7))
    {
        sprintf(str, "%d", (int)atom_getintarg(7, argc, argv));
        srl[0] = gensym(str);
    }
    if(IS_A_SYMBOL(argv,8))
        srl[1] = atom_getsymbolarg(8, argc, argv);
    else if(IS_A_FLOAT(argv,8))
    {
        sprintf(str, "%d", (int)atom_getintarg(8, argc, argv));
        srl[1] = gensym(str);
    }
    if(IS_A_SYMBOL(argv,9))
        srl[2] = atom_getsymbolarg(9, argc, argv);
    else if(IS_A_FLOAT(argv,9))
    {
        sprintf(str, "%d", (int)atom_getintarg(9, argc, argv));
        srl[2] = gensym(str);
    }
    if(init != 0) init = 1;
    iemgui->x_loadinit = init;
    iemgui_all_raute2dollar(srl);
    iemgui_all_dollararg2sym(iemgui, srl);
    if(rcvable)
    {
        if(srl[1]!=iemgui->x_rcv)
        {
            if(iemgui_has_rcv(iemgui))
                pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
            iemgui->x_rcv = srl[1];
            pd_bind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        }
    }
    else if(!rcvable && iemgui_has_rcv(iemgui))
    {
        pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        iemgui->x_rcv = srl[1];
    }
    iemgui->x_snd = srl[0];
    iemgui->x_lcol = lcol & 0xffffff;
    iemgui->x_fcol = fcol & 0xffffff;
    iemgui->x_bcol = bcol & 0xffffff;
    iemgui->x_lab = srl[2];
    iemgui->x_ldx = ldx;
    iemgui->x_ldy = ldy;
    if(f<0 || f>2) f=0;
    iemgui->x_font_style = f;
    if(fs < 4)
        fs = 4;
    iemgui->x_fontsize = fs;
    iemgui_verify_snd_ne_rcv(iemgui);
    canvas_dirty(iemgui->x_glist, 1);
    return(oldsndrcvable);
}

void iem_inttosymargs(t_iemgui *x, int n)
{
    x->x_loadinit = (n >>  0);
    x->x_locked = 0;
    x->x_reverse = 0;
}

int iem_symargstoint(t_iemgui *x)
{
    return ((x->x_loadinit & 1) <<  0);
}

void iem_inttofstyle(t_iemgui *x, int n)
{
    x->x_font_style = (n >> 0);
    x->x_selected = 0;
    x->x_finemoved = 0;
    x->x_put_in2out = 0;
    x->x_change = 0;
}

int iem_fstyletoint(t_iemgui *x)
{
    return ((x->x_font_style << 0) & 63);
}

char *iem_get_tag(t_canvas *glist, t_iemgui *iem_obj)
{
    t_gobj *y = (t_gobj *)iem_obj;
    t_object *ob = pd_checkobject(&y->g_pd);

    /* GOP objects are unable to call findrtext
       triggering consistency check error */
    t_rtext *yyyy = NULL;
    if (!glist->gl_isgraph || glist_istoplevel(glist))
        yyyy = glist_findrtext(glist_getcanvas(glist), (t_text *)&ob->ob_g);

    /* on GOP we cause segfault as text_gettag() returns bogus data */
    if (yyyy) return(rtext_gettag(yyyy));
    else return("bogus");
}

//----------------------------------------------------------------
// SCALEHANDLE COMMON CODE (by Mathieu, refactored from existing code)

extern int gfxstub_haveproperties(void *key);

int mini(int a, int b) {return a<b?a:b;}
int maxi(int a, int b) {return a>b?a:b;}

// in all 20 cases :
// [bng], [tgl], [hradio], [vradio], [hsl], [vsl], [cnv], [nbx], [vu]
// for both scale & label, plus canvas' scale & move.
void scalehandle_bind(t_scalehandle *h) {
    sys_vgui("bind %s <Button> {pd [concat %s _click 1 %%x %%y \\;]}\n",
        h->h_pathname, h->h_bindsym->s_name);
    sys_vgui("bind %s <ButtonRelease> {pd [concat %s _click 0 0 0 \\;]}\n",
        h->h_pathname, h->h_bindsym->s_name);
    sys_vgui("bind %s <Motion> {pd [concat %s _motion %%x %%y \\;]}\n",
        h->h_pathname, h->h_bindsym->s_name);
}

// in 18 cases only, because canvas does not fit the pattern below.
// canvas has no label handle and has a motion handle
// but in the case of canvas, the "iemgui" tag is added (it wasn't the case originally)
void scalehandle_draw_select(t_scalehandle *h, t_glist *canvas, int px, int py, const char *nlet_tag) {
    char tags[128]; // BNG may need up to 100 chars in 64-bit mode, for example
    t_iemgui *x = (t_iemgui *)h->h_master;
    //if (!nlet_tag) nlet_tag = iem_get_tag(canvas, (t_iemgui *)x);

    //int px,py;
    //t_class *c = pd_class((t_pd *)x);
    //if (h->h_scale) {
    //    int x1,y1,x2,y2;
    //    c->c_wb->w_getrectfn((t_gobj *)x,canvas,&x1,&y1,&x2,&y2);
    //    px=x2-x1; py=y2-y1;
    //} else if (c==canvas_class) {  
    //} else {
    //    px=x->x_ldx; py=x->x_ldy;
    //}

    const char *cursor = h->h_scale ? "bottom_right_corner" : "crosshair";
    int sx = h->h_scale ? SCALEHANDLE_WIDTH  : LABELHANDLE_WIDTH;
    int sy = h->h_scale ? SCALEHANDLE_HEIGHT : LABELHANDLE_HEIGHT;

    //printf("scalehandle_draw_select(x%lx,x%lx,%d,%d,\"%s\",\"%s\")\n",h,canvas,px,py,nlet_tag);

    if (h->h_vis) scalehandle_draw_erase(h,canvas);

    sys_vgui("canvas %s -width %d -height %d -bg $pd_colors(selection) -bd 0 "
//    sys_vgui("canvas %s -width %d -height %d -bg #0080ff -bd 0 "
        "-cursor %s\n", h->h_pathname, sx, sy, cursor);
    // there was a %lxBNG tag (or similar) in every scalehandle,
    // but it didn't seem to be used —mathieu
    if (h->h_scale) {
        sprintf(tags,"%lxOBJ %lxSCALE iemgui %s",
            (long)x,(long)x,nlet_tag);
    } else {
        //sprintf(tags,"%lx%s %lxLABEL %lxLABELH iemgui %s", // causes unknown option "-fill"
        sprintf(tags,"%lxOBJ %lx%s iemgui %s", (long)x,
            (long)x,pd_class((t_pd *)x)==canvas_class?"MOVE":"LABELH",nlet_tag);
    }
    sys_vgui(".x%x.c create window %d %d -anchor nw -width %d -height %d "
        "-window %s -tags {%s}\n", canvas, x->x_obj.te_xpix+px-sx, x->x_obj.te_ypix+py-sy,
        sx, sy, h->h_pathname, tags);
    scalehandle_bind(h);
    h->h_vis = 1;
}

void scalehandle_draw_select2(t_iemgui *x, t_glist *canvas) {
    char *nlet_tag = iem_get_tag(canvas, (t_iemgui *)x);
    t_class *c = pd_class((t_pd *)x);
    int x1,y1,x2,y2;
   c->c_wb->w_getrectfn((t_gobj *)x,canvas,&x1,&y1,&x2,&y2);
    scalehandle_draw_select(x->x_handle,canvas,x2-x1-1,y2-y1-1,nlet_tag);
    if (x->x_lab!=s_empty)
        scalehandle_draw_select(x->x_lhandle,canvas,x->x_ldx,x->x_ldy,nlet_tag);
}

void scalehandle_draw_erase(t_scalehandle *h, t_glist *canvas) {
        sys_vgui("destroy %s\n", h->h_pathname);
        sys_vgui(".x%lx.c delete %lx%s\n", canvas, h->h_master, h->h_scale ? "SCALE" : "LABELH");
        h->h_vis = 0;
}

void scalehandle_draw_erase2(t_iemgui *x, t_glist *canvas) {
	t_scalehandle *sh = (t_scalehandle *)(x->x_handle);
	t_scalehandle *lh = (t_scalehandle *)(x->x_lhandle);
    if (sh->h_vis) scalehandle_draw_erase(sh,canvas);
    if (lh->h_vis) scalehandle_draw_erase(lh,canvas);
}

void scalehandle_draw_new(t_scalehandle *h, t_glist *canvas) {
    sprintf(h->h_pathname, ".x%lx.h%lx", (t_int)canvas, (t_int)h);
}

t_scalehandle *scalehandle_new(t_class *c, t_iemgui *x, int scale) {
    t_scalehandle *h = (t_scalehandle *)pd_new(c);
    char buf[64];
    h->h_master = (t_gobj*)x;
    sprintf(buf, "_h%lx", (t_int)h);
    pd_bind((t_pd *)h, h->h_bindsym = gensym(buf));
    sprintf(h->h_outlinetag, "h%lx", (t_int)h);
    h->h_dragon = 0;
    h->h_scale = scale;
    //h->h_offset_x = 0; // unused (maybe keep for later)
    //h->h_offset_y = 0; // unused (maybe keep for later)
    h->h_vis = 0;
    return h;
}

void scalehandle_free(t_scalehandle *h) {
    pd_unbind((t_pd *)h, h->h_bindsym);
    pd_free((t_pd *)h);
}

void properties_set_field_int(long props, const char *gui_field, int value) {
    sys_vgui(".gfxstub%lx.%s delete 0 end\n", props, gui_field);
    sys_vgui(".gfxstub%lx.%s insert 0 %d\n", props, gui_field, value);
};

void scalehandle_dragon_label(t_scalehandle *h, float f1, float f2) {
    if (h->h_dragon && !h->h_scale)
    {
        t_iemgui *x = (t_iemgui *)(h->h_master);
        int dx = (int)f1, dy = (int)f2;
        h->h_dragx = dx;
        h->h_dragy = dy;
        int properties = gfxstub_haveproperties((void *)x);
        if (properties)
        {
            int new_x = x->x_ldx + h->h_dragx;
            int new_y = x->x_ldy + h->h_dragy;
            properties_set_field_int(properties,"label.xy.x_entry",new_x);
            properties_set_field_int(properties,"label.xy.y_entry",new_y);
        }
        if (glist_isvisible(x->x_glist))
        {
            int xpos=text_xpix(&x->x_obj, x->x_glist);
            int ypos=text_ypix(&x->x_obj, x->x_glist);
            t_canvas *canvas=glist_getcanvas(x->x_glist);
            sys_vgui(".x%lx.c coords %lxLABEL %d %d\n", canvas, x,
                xpos+x->x_ldx + h->h_dragx,
                ypos+x->x_ldy + h->h_dragy);
        }
    }
}

void scalehandle_unclick_label(t_scalehandle *h) {
    t_iemgui *x = (t_iemgui *)h->h_master;
    canvas_apply_setundo(x->x_glist, (t_gobj *)x);
    if (h->h_dragx || h->h_dragy)
    {
        x->x_ldx += h->h_dragx;
        x->x_ldy += h->h_dragy;
        canvas_dirty(x->x_glist, 1);
    }
    if (glist_isvisible(x->x_glist))
    {
        iemgui_select((t_gobj *)x, x->x_glist, 1);
        canvas_fixlinesfor(x->x_glist, (t_text *)x);
        sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x->x_glist);
    }
}

void scalehandle_click_label(t_scalehandle *h) {
    t_iemgui *x = (t_iemgui *)h->h_master;
    if (glist_isvisible(x->x_glist))
    {
        sys_vgui("lower %s\n", h->h_pathname);
        t_scalehandle *othersh = x->x_handle;
        sys_vgui("lower .x%lx.h%lx\n",
            (t_int)glist_getcanvas(x->x_glist), (t_int)othersh);
    }
    h->h_dragx = 0;
    h->h_dragy = 0;
}

extern t_class *my_canvas_class;
void scalehandle_getrect_master(t_scalehandle *h, int *x1, int *y1, int *x2, int *y2) {
    t_iemgui *x = (t_iemgui *)h->h_master;
    t_class *c = pd_class((t_pd *)x);
    c->c_wb->w_getrectfn((t_gobj *)x,x->x_glist,x1,y1,x2,y2);
    //printf("%s\n",c->c_name->s_name);
    if (c==my_canvas_class) {
        t_my_canvas *xx = (t_my_canvas *)x;
        *x2=*x1+xx->x_vis_w;
        *y2=*y1+xx->x_vis_h;
    }
}

void scalehandle_click_scale(t_scalehandle *h) {
    int x1,y1,x2,y2;
    t_iemgui *x = (t_iemgui *)h->h_master;
    scalehandle_getrect_master(h,&x1,&y1,&x2,&y2);
    if (glist_isvisible(x->x_glist)) {
        sys_vgui("lower %s\n", h->h_pathname);
        sys_vgui(".x%x.c create prect %d %d %d %d -stroke $pd_colors(selection) -strokewidth 1 -tags %s\n",
            x->x_glist, x1, y1, x2, y2, h->h_outlinetag);
    }
    h->h_dragx = 0;
    h->h_dragy = 0;
}

void scalehandle_unclick_scale(t_scalehandle *h) {
    t_iemgui *x = (t_iemgui *)h->h_master;
    sys_vgui(".x%x.c delete %s\n", x->x_glist, h->h_outlinetag);
    iemgui_select((t_gobj *)x, x->x_glist, 1);
    canvas_fixlinesfor(x->x_glist, (t_text *)x);
    sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x->x_glist);
}

void scalehandle_drag_scale(t_scalehandle *h) {
    int x1,y1,x2,y2;
    t_iemgui *x = (t_iemgui *)h->h_master;
    scalehandle_getrect_master(h,&x1,&y1,&x2,&y2);
    if (glist_isvisible(x->x_glist)) {
        sys_vgui(".x%x.c coords %s %d %d %d %d\n", x->x_glist, h->h_outlinetag,
            x1, y1, x2+h->h_dragx, y2+h->h_dragy);
    }
}

void iemgui__clickhook3(t_scalehandle *sh, int newstate) {
    if (!sh->h_dragon && newstate && sh->h_scale)
        scalehandle_click_scale(sh);
    else if (sh->h_dragon && newstate == 0 && !sh->h_scale)
        scalehandle_unclick_label(sh);
    else if (!sh->h_dragon && newstate && !sh->h_scale)
        scalehandle_click_label(sh);
    sh->h_dragon = newstate;
}

//----------------------------------------------------------------
// IEMGUI refactor (by Mathieu)

void iemgui_tag_selected(t_iemgui *x, t_glist *canvas) {
    if(x->x_selected)
        sys_vgui(".x%lx.c addtag selected withtag %lxOBJ\n", canvas, x);
    else
        sys_vgui(".x%lx.c dtag %lxOBJ selected\n", canvas, x);
}

void iemgui_label_draw_new(t_iemgui *x, t_glist *canvas, int xpos, int ypos, const char *nlet_tag) {
    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w "
             "-font {{%s} -%d %s} -fill #%6.6x "
             "-tags {%lxLABEL %lxOBJ text iemgui %s}\n",
         canvas, xpos+x->x_ldx, ypos+x->x_ldy,
         x->x_lab!=s_empty?x->x_lab->s_name:"",
         iemgui_font(x), x->x_fontsize, sys_fontweight,
         x->x_lcol, x, x, nlet_tag);
}

void iemgui_label_draw_move(t_iemgui *x, t_glist *canvas, int xpos, int ypos) {
    sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
        canvas, x, xpos+x->x_ldx, ypos+x->x_ldy);
}

void iemgui_label_draw_config(t_iemgui *x, t_glist *canvas) {
    if (x->x_selected && x->x_glist == canvas)
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} -%d %s} "
                 "-fill $pd_colors(selection) -text {%s} \n",
             canvas, x, iemgui_font(x), x->x_fontsize, sys_fontweight,
             x->x_lab!=s_empty?x->x_lab->s_name:"");
    else
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} -%d %s} "
                 "-fill #%6.6x -text {%s} \n",
             canvas, x, iemgui_font(x), x->x_fontsize, sys_fontweight,
             x->x_lcol, x->x_lab!=s_empty?x->x_lab->s_name:"");
}

void iemgui_label_draw_select(t_iemgui *x, t_glist *canvas) {
    if (x->x_selected && x->x_glist == canvas)
        sys_vgui(".x%lx.c itemconfigure %lxLABEL "
            "-fill $pd_colors(selection)\n", canvas, x);
    else
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n",
            canvas, x, x->x_lcol);
}

extern t_class *vu_class;
void iemgui_io_draw(t_iemgui *x, t_glist *canvas, int old_sr_flags) {
	if (x->x_glist != canvas) return; // is gop
    int a,b;
    t_class *c = pd_class((t_pd *)x);
    //printf("--- iemgui_io_draw %s flags=%d\n",c->c_name->s_name,old_sr_flags);
    char *nlet_tag = iem_get_tag(/*glist*/ x->x_glist, (t_iemgui *)x);

    if (!(old_sr_flags&4) && (!glist_isvisible(canvas) || !(canvas == x->x_glist))) {
        //printf("---/iemgui_io_draw not visible\n");
        return;
    }

    int x1,y1,x2,y2;
    c->c_wb->w_getrectfn((t_gobj *)x,canvas,&x1,&y1,&x2,&y2);

    int i, n = c==vu_class ? 2 : 1, k=(x2-x1)-IOWIDTH;

    a=old_sr_flags&IEM_GUI_OLD_SND_FLAG;
    b=x->x_snd!=s_empty;
    //printf("a=%d b=%d snd=%s\n",a,b,x->x_snd->s_name);
    if(a && !b) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c create prect %d %d %d %d "
                 "-stroke $pd_colors(iemgui_nlet) "
                 "-tags {%lxOBJ%so%d %so%d %lxOBJ outlet iemgui %s}\n",
             canvas, x1+i*k, y2-1, x1+i*k + IOWIDTH, y2,
             x, nlet_tag, i, nlet_tag, i, x, nlet_tag);
    if(!a && b) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c delete %lxOBJ%so%d\n", canvas, x, nlet_tag, 0);

    a=old_sr_flags&IEM_GUI_OLD_RCV_FLAG;
    b=x->x_rcv!=s_empty;
    //printf("a=%d b=%d rcv=%s\n",a,b,x->x_rcv->s_name);
    if(a && !b) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c create prect %d %d %d %d "
                 "-stroke $pd_colors(iemgui_nlet) "
                 "-tags {%lxOBJ%si%d %si%d %lxOBJ inlet iemgui %s}\n",
             canvas, x1+i*k, y1, x1+i*k + IOWIDTH, y1+1,
             x, nlet_tag, i, nlet_tag, i, x, nlet_tag);
    if(!a && b) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c delete %lxOBJ%si%d\n", canvas, x, nlet_tag, 0);
    //printf("---/iemgui_io_draw\n");
}

void iemgui_io_draw_move(t_iemgui *x, t_glist *canvas, const char *nlet_tag) {
    t_class *c = pd_class((t_pd *)x);
    int x1,y1,x2,y2;
    c->c_wb->w_getrectfn((t_gobj *)x,canvas,&x1,&y1,&x2,&y2);
    int i, n = c==vu_class ? 2 : 1, k=(x2-x1)-IOWIDTH;
    if(!iemgui_has_snd(x) && canvas == x->x_glist) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c coords %lxOBJ%so%d %d %d %d %d\n",
            canvas, x, nlet_tag, i, x1+i*k, y2-1, x1+i*k+IOWIDTH, y2);
    if(!iemgui_has_rcv(x) && canvas == x->x_glist) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c coords %lxOBJ%si%d %d %d %d %d\n",
            canvas, x, nlet_tag, i, x1+i*k, y1, x1+i*k+IOWIDTH, y1+1);
}

void iemgui_base_draw_new(t_iemgui *x, t_glist *canvas, const char *nlet_tag) {
    t_class *c = pd_class((t_pd *)x);
    int x1,y1,x2,y2,gr=gop_redraw; gop_redraw=0;
    c->c_wb->w_getrectfn((t_gobj *)x,x->x_glist,&x1,&y1,&x2,&y2);
    gop_redraw=gr;
    sys_vgui(".x%lx.c create prect %d %d %d %d "
             "-stroke $pd_colors(iemgui_border) -fill #%6.6x "
             "-tags {%lxBASE %lxOBJ text iemgui border %s}\n",
         canvas, x1,y1,x2,y2, x->x_bcol, x, x, nlet_tag);
}

void iemgui_base_draw_move(t_iemgui *x, t_glist *canvas, const char *nlet_tag) {
    t_class *c = pd_class((t_pd *)x);
    int x1,y1,x2,y2,gr=gop_redraw; gop_redraw=0;
    c->c_wb->w_getrectfn((t_gobj *)x,x->x_glist,&x1,&y1,&x2,&y2);
    gop_redraw=gr;
    sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n", canvas, x, x1, y1, x2, y2);
}

void iemgui_base_draw_config(t_iemgui *x, t_glist *canvas) {
    char fcol[8]; sprintf(fcol,"#%6.6x", x->x_fcol);
    sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%6.6x\n", canvas, x, x->x_bcol);
    sys_vgui(".x%lx.c itemconfigure {%lxBASE||%lxBASEL} -stroke %s\n", canvas, x, x,
        x->x_selected && x->x_glist == canvas ? selection_color : fcol);
}

void iemgui_draw_erase(t_iemgui *x, t_glist* glist) {
    t_canvas *canvas=glist_getcanvas(glist);
    sys_vgui(".x%lx.c delete %lxOBJ\n", canvas, x);
    sys_vgui(".x%lx.c dtag all %lxOBJ\n", canvas, x);
    scalehandle_draw_erase2(x,glist);
}

void scrollbar_update(t_glist *glist) {
    //ico@bukvic.net 100518 update scrollbars when object potentially
    //exceeds window size
    t_canvas *canvas=(t_canvas *)glist_getcanvas(glist);
    sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", (long unsigned int)canvas);
}

void wb_init(t_widgetbehavior *wb, t_getrectfn gr, t_clickfn cl) {
    wb->w_getrectfn = gr;
    wb->w_displacefn = iemgui_displace;
    wb->w_selectfn = iemgui_select;
    wb->w_activatefn = NULL;
    wb->w_deletefn = iemgui_delete;
    wb->w_visfn = iemgui_vis;
    wb->w_clickfn = cl;
    wb->w_displacefnwtag = iemgui_displace_withtag;
}

const char *iemgui_font(t_iemgui *x) {
	int f = x->x_font_style;
	if(f == 0) return sys_font;
    if(f == 1) return "helvetica";
    if(f == 2) return "times";
    return "invalid-font";
}

void iemgui_init(t_iemgui *x, t_floatarg f) {x->x_loadinit = f!=0.0;}

void iemgui_class_addmethods(t_class *c) {
    class_addmethod(c, (t_method)iemgui_delta,
        gensym("delta"), A_GIMME, 0);
    class_addmethod(c, (t_method)iemgui_pos,
        gensym("pos"), A_GIMME, 0);
    class_addmethod(c, (t_method)iemgui_color,
        gensym("color"), A_GIMME, 0);
    class_addmethod(c, (t_method)iemgui_send,
        gensym("send"), A_DEFSYM, 0);
    class_addmethod(c, (t_method)iemgui_receive,
        gensym("receive"), A_DEFSYM, 0);
    class_addmethod(c, (t_method)iemgui_label,
        gensym("label"), A_DEFSYM, 0);
    class_addmethod(c, (t_method)iemgui_label_pos,
        gensym("label_pos"), A_GIMME, 0);
    class_addmethod(c, (t_method)iemgui_label_font,
        gensym("label_font"), A_GIMME, 0);
}

void g_iemgui_setup (void) {
    s_empty = gensym("empty");
}

const char *selection_color = "$pd_colors(selection)";

