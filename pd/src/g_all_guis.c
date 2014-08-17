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

int iemgui_color_hex[] = {
    0xfcfcfc, 0xa0a0a0, 0x404040, 0xfce0e0, 0xfce0c0, 0xfcfcc8, 0xd8fcd8, 0xd8fcfc, 0xdce4fc, 0xf8d8fc,
    0xe0e0e0, 0x7c7c7c, 0x202020, 0xfc2828, 0xfcac44, 0xe8e828, 0x14e814, 0x28f4f4, 0x3c50fc, 0xf430f0,
    0xbcbcbc, 0x606060, 0x000000, 0x8c0808, 0x583000, 0x782814, 0x285014, 0x004450, 0x001488, 0x580050
};

int iemgui_clip_size(int size) {return maxi(size,IEM_GUI_MINSIZE);}
int iemgui_clip_font(int size) {return maxi(size,IEM_FONT_MINSIZE);}
static void scalehandle_check_and_redraw(t_iemgui *x);

static int iemgui_modulo_color(int col)
{
    const int IEM_GUI_MAX_COLOR = 30;
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

void iemgui_verify_snd_ne_rcv(t_iemgui *x)
{
    x->x_put_in2out = 
        !(iemgui_has_snd(x) && iemgui_has_rcv(x) && x->x_snd==x->x_rcv);
}

t_symbol *iemgui_getfloatsym(t_atom *a)
{
    if (IS_A_SYMBOL(a,0)) return (atom_getsymbol(a));
    if (IS_A_FLOAT(a,0)) {
        char str[40];
        sprintf(str, "%d", (int)atom_getint(a));
        return gensym(str);
    }
    return s_empty;
}
t_symbol *iemgui_getfloatsymarg(int i, int argc, t_atom *argv)
{
    if (i>=0 && i<argc) return iemgui_getfloatsym(argv+i);
    return s_empty;
}

void iemgui_new_getnames(t_iemgui *x, int indx, t_atom *argv)
{
    if (argv)
    {
        x->x_snd = iemgui_getfloatsym(argv+indx+0);
        x->x_rcv = iemgui_getfloatsym(argv+indx+1);
        x->x_lab = iemgui_getfloatsym(argv+indx+2);
    }
    else x->x_snd = x->x_rcv = x->x_lab = s_empty;
    x->x_snd_unexpanded = x->x_rcv_unexpanded = x->x_lab_unexpanded = 0;
    x->x_binbufindex = indx;
    x->x_labelbindex = indx + 3;
}

/* initialize a single symbol in unexpanded form.  We reach into the
   binbuf to grab them; if there's nothing there, set it to the
   fallback; if still nothing, set to "empty". */
static void iemgui_init_sym2dollararg(t_iemgui *x, t_symbol **symp,
    int indx, t_symbol *fallback)
{
    if (!*symp)
    {
        t_binbuf *b = x->x_obj.ob_binbuf;
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
void iemgui_all_sym2dollararg(t_iemgui *x, t_symbol **srlsym)
{
    iemgui_init_sym2dollararg(x, &x->x_snd_unexpanded, x->x_binbufindex+1, x->x_snd);
    iemgui_init_sym2dollararg(x, &x->x_rcv_unexpanded, x->x_binbufindex+2, x->x_rcv);
    iemgui_init_sym2dollararg(x, &x->x_lab_unexpanded, x->x_labelbindex, x->x_lab);
    srlsym[0] = x->x_snd_unexpanded;
    srlsym[1] = x->x_rcv_unexpanded;
    srlsym[2] = x->x_lab_unexpanded;
}

static int col2save(int col) {
    return -1-(((0xfc0000 & col) >> 6)|((0xfc00 & col) >> 4)|((0xfc & col) >> 2));
}
void iemgui_all_col2save(t_iemgui *x, int *bflcol)
{
    bflcol[0] = col2save(x->x_bcol);
    bflcol[1] = col2save(x->x_fcol);
    bflcol[2] = col2save(x->x_lcol);
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
void iemgui_all_colfromload(t_iemgui *x, int *bflcol)
{
    x->x_bcol = colfromload(bflcol[0]);
    x->x_fcol = colfromload(bflcol[1]);
    x->x_lcol = colfromload(bflcol[2]);
}

static int iemgui_compatible_col(int i)
{
    if(i >= 0)
        return(iemgui_color_hex[(iemgui_modulo_color(i))]);
    return((-1-i)&0xffffff);
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
    if (s == &s_) s = s_empty; //tb: fix for empty label
    int oldsndrcvable=0;
    if(iemgui_has_rcv(x)) oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui_has_snd(x)) oldsndrcvable += IEM_GUI_OLD_SND_FLAG;

    snd = iemgui_raute2dollar(s);
    x->x_snd_unexpanded = snd;
    x->x_snd = snd = canvas_realizedollar(x->x_glist, snd);
    iemgui_verify_snd_ne_rcv(x);
    iemgui_draw_io(x, oldsndrcvable);
}

void iemgui_receive(t_iemgui *x, t_symbol *s)
{
    t_symbol *rcv;
    if (s == &s_) s = s_empty; //tb: fix for empty label
    int oldsndrcvable=0;
    if(iemgui_has_rcv(x)) oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui_has_snd(x)) oldsndrcvable += IEM_GUI_OLD_SND_FLAG;

    rcv = iemgui_raute2dollar(s);
    x->x_rcv_unexpanded = rcv;
    rcv = canvas_realizedollar(x->x_glist, rcv);
    if(s!=s_empty)
    {
        if(rcv!=x->x_rcv)
        {
            if(iemgui_has_rcv(x))
                pd_unbind((t_pd *)x, x->x_rcv);
            x->x_rcv = rcv;
            pd_bind((t_pd *)x, x->x_rcv);
        }
    }
    else if(s==s_empty && iemgui_has_rcv(x))
    {
        pd_unbind((t_pd *)x, x->x_rcv);
        x->x_rcv = rcv;
    }
    iemgui_verify_snd_ne_rcv(x);
    iemgui_draw_io(x, oldsndrcvable);
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
    x->x_ldx = atom_getintarg(0, ac, av);
    x->x_ldy = atom_getintarg(1, ac, av);
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
    int f = atom_getintarg(0, ac, av);
    if (f<0 || f>2) f=0;
    x->x_font_style = f;
    x->x_fontsize = maxi(atom_getintarg(1, ac, av),4);
    if(glist_isvisible(x->x_glist))
    {
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -font %s\n",
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

#if 0 // future way of reordering stuff for iemgui_shouldvis
/*
    // some day when the object tagging is
    // properly done for all GUI objects
    glist_noselect(canvas);
    glist_select(canvas, y);
    t_gobj *yy = canvas->gl_list;
    if (yy != y)
    {
        while (yy && yy->g_next != y)
            yy = yy->g_next;
        // now we have yy which is right before our y graph
        t_rtext *yr = NULL;
        if (yy)
        {
            yr = glist_findrtext(canvas, (t_text *)yy);
        }
        if (yr)
        {
            fprintf(stderr,"lower\n");
            sys_vgui(".x%lx.c lower selected %s\n", canvas, rtext_gettag(yr));
            sys_vgui(".x%lx.c raise selected %s\n", canvas, rtext_gettag(yr));
            //canvas_raise_all_cords(canvas);
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
#endif

void iemgui_shouldvis(t_iemgui *x, int mode)
{
    gop_redraw = 1;
    if(gobj_shouldvis((t_gobj *)x, x->x_glist))
    {
        if (!x->x_vis)
        {
            //fprintf(stderr,"draw new %d\n", mode);
            iemgui_draw_new(x);
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

            }
        }
        //fprintf(stderr,"draw move x->x_w=%d\n", x->x_w);
        if      (mode==IEM_GUI_DRAW_MODE_NEW)    iemgui_draw_new(   x);
        else if (mode==IEM_GUI_DRAW_MODE_MOVE)   iemgui_draw_move(  x);
        else if (mode==IEM_GUI_DRAW_MODE_CONFIG) iemgui_draw_config(x);
        else bug("iemgui_shouldvis");
        scalehandle_check_and_redraw(x);
        canvas_fixlinesfor(glist_getcanvas(x->x_glist), (t_text*)x);
    }
    else if (x->x_vis)
    {
        //fprintf(stderr,"draw erase %d\n", mode);
        iemgui_draw_erase(x);
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
    x->x_obj.te_xpix += atom_getintarg(0, ac, av);
    x->x_obj.te_ypix += atom_getintarg(1, ac, av);
    if(glist_isvisible(x->x_glist))
        iemgui_shouldvis(x, IEM_GUI_DRAW_MODE_MOVE);
}

void iemgui_pos(t_iemgui *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_obj.te_xpix = atom_getintarg(0, ac, av);
    x->x_obj.te_ypix = atom_getintarg(1, ac, av);
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
    //iemgui_draw_move(x);
    canvas_fixlinesfor(glist_getcanvas(glist), (t_text *)z);
}


void iemgui_select(t_gobj *z, t_glist *glist, int selected)
{
    t_iemgui *x = (t_iemgui *)z;
    t_canvas *canvas=glist_getcanvas(glist);
    if (selected)
        x->x_selected = canvas;
    else
        x->x_selected = NULL;
    char fcol[8]; sprintf(fcol,"#%6.6x", x->x_fcol);
    sys_vgui(".x%lx.c itemconfigure {x%lx&&border} -stroke %s\n", canvas, x,
        x->x_selected && x->x_glist == canvas ? selection_color : fcol);
    x->x_draw((void *)z, glist, IEM_GUI_DRAW_MODE_SELECT);
    scalehandle_draw(x);
    iemgui_label_draw_select(x);
    iemgui_tag_selected(x);
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
            iemgui_draw_new(x);
        else
        {
            iemgui_draw_erase(x);
            sys_unqueuegui(z);
        }
        x->x_vis = vis;
    }
}

void iemgui_save(t_iemgui *x, t_symbol **srl, int *bflcol)
{
    if (srl) {
       srl[0] = x->x_snd;
       srl[1] = x->x_rcv;
       srl[2] = x->x_lab;
    }
    iemgui_all_sym2dollararg(x, srl);
    iemgui_all_col2save(x, bflcol);
}

void iemgui_properties(t_iemgui *x, t_symbol **srl)
{
    srl[0] = x->x_snd;
    srl[1] = x->x_rcv;
    srl[2] = x->x_lab;
    iemgui_all_sym2dollararg(x, srl);
    srl[0] = iemgui_dollar2raute(srl[0]);
    srl[1] = iemgui_dollar2raute(srl[1]);
    srl[2] = iemgui_dollar2raute(srl[2]);
}

int iemgui_dialog(t_iemgui *x, int argc, t_atom *argv)
{
    t_symbol *srl[3];
    x->x_loadinit = !!atom_getintarg(5, argc, argv);
    srl[0] = iemgui_getfloatsymarg(7,argc,argv);
    srl[1] = iemgui_getfloatsymarg(8,argc,argv);
    srl[2] = iemgui_getfloatsymarg(9,argc,argv);
    x->x_ldx = atom_getintarg(10, argc, argv);
    x->x_ldy = atom_getintarg(11, argc, argv);
    int f = atom_getintarg(12, argc, argv);
    x->x_fontsize = maxi(atom_getintarg(13, argc, argv),4);
    x->x_bcol = atom_getintarg(14, argc, argv) & 0xffffff;
    x->x_fcol = atom_getintarg(15, argc, argv) & 0xffffff;
    x->x_lcol = atom_getintarg(16, argc, argv) & 0xffffff;
    int oldsndrcvable=0;
    if(iemgui_has_rcv(x)) oldsndrcvable |= IEM_GUI_OLD_RCV_FLAG;
    if(iemgui_has_snd(x)) oldsndrcvable |= IEM_GUI_OLD_SND_FLAG;
    iemgui_all_raute2dollar(srl);
    x->x_snd_unexpanded=srl[0]; srl[0]=canvas_realizedollar(x->x_glist, srl[0]);
    x->x_rcv_unexpanded=srl[1]; srl[1]=canvas_realizedollar(x->x_glist, srl[1]);
    x->x_lab_unexpanded=srl[2]; srl[2]=canvas_realizedollar(x->x_glist, srl[2]);
    if(srl[1]!=x->x_rcv)
    {
        if(iemgui_has_rcv(x))
            pd_unbind((t_pd *)x, x->x_rcv);
        x->x_rcv = srl[1];
        pd_bind((t_pd *)x, x->x_rcv);
    }
    x->x_snd = srl[0];
    x->x_lab = srl[2];
    if(f<0 || f>2) f=0;
    x->x_font_style = f;
    iemgui_verify_snd_ne_rcv(x);
    canvas_dirty(x->x_glist, 1);
    return oldsndrcvable;
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
    x->x_selected = NULL;
    x->x_finemoved = 0;
    x->x_put_in2out = 0;
    x->x_change = 0;
}

int iem_fstyletoint(t_iemgui *x)
{
    return ((x->x_font_style << 0) & 63);
}

//----------------------------------------------------------------
// SCALEHANDLE COMMON CODE (by Mathieu, refactored from existing code)

extern int gfxstub_haveproperties(void *key);

int   mini(int   a, int   b) {return a<b?a:b;}
int   maxi(int   a, int   b) {return a>b?a:b;}
float minf(float a, float b) {return a<b?a:b;}
float maxf(float a, float b) {return a>b?a:b;}

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
void scalehandle_draw_select(t_scalehandle *h, int px, int py) {
    char tags[128]; // BNG may need up to 100 chars in 64-bit mode, for example
    t_iemgui *x = (t_iemgui *)h->h_master;
    t_canvas *canvas=glist_getcanvas(x->x_glist);

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

    scalehandle_draw_erase(h);

    if (!h->h_vis) {
        sys_vgui("canvas %s -width %d -height %d -bg $pd_colors(selection) -bd 0 "
            "-cursor %s\n", h->h_pathname, sx, sy, cursor);
        if (h->h_scale) {
            sprintf(tags,"x%lx %lxSCALE iemgui selected", (long)x,(long)x);
        } else {
            sprintf(tags,"x%lx %lx%s iemgui selected", (long)x,
                (long)x,pd_class((t_pd *)x)==canvas_class?"MOVE":"LABELH");
        }
        sys_vgui(".x%x.c create window %d %d -anchor nw -width %d -height %d "
            "-window %s -tags {%s}\n", canvas,
            x->x_obj.te_xpix+px-sx, x->x_obj.te_ypix+py-sy, sx, sy,
            h->h_pathname, tags);
        scalehandle_bind(h);
        h->h_vis = 1;
    /* not yet (this is not supported by current implementation) */
    }/* else {
        sys_vgui(".x%x.c coords %s %d %d\n", canvas, h->h_pathname,
            x->x_obj.te_xpix+px-sx, x->x_obj.te_ypix+py-sy);
        sys_vgui("raise %s\n", h->h_pathname);
    }*/
}

extern t_class *my_canvas_class;

void scalehandle_draw_select2(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    t_class *c = pd_class((t_pd *)x);
    int sx,sy;
    if (c==my_canvas_class) {
        t_my_canvas *y = (t_my_canvas *)x;
        sx=y->x_vis_w; sy=y->x_vis_h;
    } else {
        int x1,y1,x2,y2;
        c->c_wb->w_getrectfn((t_gobj *)x,canvas,&x1,&y1,&x2,&y2);
        sx=x2-x1; sy=y2-y1;
    }
    scalehandle_draw_select(x->x_handle,sx-1,sy-1);
    if (x->x_lab!=s_empty)
        scalehandle_draw_select(x->x_lhandle,x->x_ldx,x->x_ldy);
}

void scalehandle_draw_erase(t_scalehandle *h) {
    t_canvas *canvas=glist_getcanvas(h->h_glist);
    if (!h->h_vis) return;
    sys_vgui("destroy %s\n", h->h_pathname);
    sys_vgui(".x%lx.c delete %lx%s\n", canvas, h->h_master,
        h->h_scale ? "SCALE" : pd_class((t_pd *)h->h_master)==canvas_class?"MOVE":"LABELH");
    h->h_vis = 0;
}

void scalehandle_draw_erase2(t_iemgui *x) {
    t_scalehandle *sh = (t_scalehandle *)(x->x_handle);
    t_scalehandle *lh = (t_scalehandle *)(x->x_lhandle);
    if (sh->h_vis) scalehandle_draw_erase(sh);
    if (lh->h_vis) scalehandle_draw_erase(lh);
}

void scalehandle_draw(t_iemgui *x) {
    if (x->x_glist == glist_getcanvas(x->x_glist)) {
        if(x->x_selected == x->x_glist) scalehandle_draw_select2(x);
        else scalehandle_draw_erase2(x);
    }
}

t_scalehandle *scalehandle_new(t_class *c, t_gobj *x, t_glist *glist, int scale) {
    t_scalehandle *h = (t_scalehandle *)pd_new(c);
    char buf[64];
    h->h_master = (t_gobj*)x;
    h->h_glist = glist;
    sprintf(buf, "_h%lx", (t_int)h);
    pd_bind((t_pd *)h, h->h_bindsym = gensym(buf));
    sprintf(h->h_outlinetag, "h%lx", (t_int)h);
    h->h_dragon = 0;
    h->h_scale = scale;
    //h->h_offset_x = 0; // unused (maybe keep for later)
    //h->h_offset_y = 0; // unused (maybe keep for later)
    h->h_vis = 0;
    sprintf(h->h_pathname, ".x%lx.h%lx", (t_int)h->h_glist, (t_int)h);
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

// here we don't need to use glist_getcanvas(t_canvas *)
// because scalehandle on iemgui objects appears only when
// they are on their own canvas
void scalehandle_unclick_scale(t_scalehandle *h) {
    t_iemgui *x = (t_iemgui *)h->h_master;
    sys_vgui(".x%x.c delete %s\n", x->x_glist, h->h_outlinetag);
    iemgui_io_draw_move(x);
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

// function for updating of handles on iemgui objects
// we don't need glist_getcanvas() because handles are only
// drawn when object is selected on its canvas (instead of GOP)
static void scalehandle_check_and_redraw(t_iemgui *x)
{
    if(x->x_selected == x->x_glist)
        scalehandle_draw_select2(x);
}

//----------------------------------------------------------------
// IEMGUI refactor (by Mathieu)

void iemgui_tag_selected(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    if(x->x_selected)
        sys_vgui(".x%lx.c addtag selected withtag x%lx\n", canvas, x);
    else
        sys_vgui(".x%lx.c dtag x%lx selected\n", canvas, x);
}

void iemgui_label_draw_new(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    int x1=text_xpix(&x->x_obj, x->x_glist);
    int y1=text_ypix(&x->x_obj, x->x_glist);
    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w "
             "-font %s -fill #%6.6x -tags {%lxLABEL x%lx text iemgui}\n",
         canvas, x1+x->x_ldx, y1+x->x_ldy,
         x->x_lab!=s_empty?x->x_lab->s_name:"",
         iemgui_font(x), x->x_lcol, x, x);
}

void iemgui_label_draw_move(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    int x1=text_xpix(&x->x_obj, x->x_glist);
    int y1=text_ypix(&x->x_obj, x->x_glist);
    sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
        canvas, x, x1+x->x_ldx, y1+x->x_ldy);
}

void iemgui_label_draw_config(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    if (x->x_selected == canvas && x->x_glist == canvas)
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -font %s "
                 "-fill $pd_colors(selection) -text {%s} \n",
             canvas, x, iemgui_font(x), 
             x->x_lab!=s_empty?x->x_lab->s_name:"");
    else
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -font %s "
                 "-fill #%6.6x -text {%s} \n",
             canvas, x, iemgui_font(x),
             x->x_lcol, x->x_lab!=s_empty?x->x_lab->s_name:"");
    if (x->x_selected == canvas && x->x_glist == canvas)
    {
        t_scalehandle *lh = (t_scalehandle *)(x->x_lhandle);
        if (x->x_lab==s_empty)    
            scalehandle_draw_erase(x->x_lhandle);
        else if (lh->h_vis == 0)
            scalehandle_draw_select(lh,x->x_ldx,x->x_ldy);
    }
}

void iemgui_label_draw_select(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    if (x->x_selected == canvas && x->x_glist == canvas)
        sys_vgui(".x%lx.c itemconfigure %lxLABEL "
            "-fill $pd_colors(selection)\n", canvas, x);
    else
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n",
            canvas, x, x->x_lcol);
}

extern t_class *my_numbox_class;
extern t_class *vu_class;
void iemgui_draw_io(t_iemgui *x, int old_sr_flags)
{
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    if (x->x_glist != canvas) return; // is gop
    t_class *c = pd_class((t_pd *)x);
    if (c==my_numbox_class && ((t_my_numbox *)x)->x_hide_frame > 1) return; //sigh

    if (!(old_sr_flags&4) && (!glist_isvisible(canvas) || !(canvas == x->x_glist))) {
        return;
    }
    if (c==my_canvas_class) return;

    int x1,y1,x2,y2;
    c->c_wb->w_getrectfn((t_gobj *)x,canvas,&x1,&y1,&x2,&y2);

    int i, n = c==vu_class ? 2 : 1, k=(x2-x1)-IOWIDTH;

    int a=old_sr_flags&IEM_GUI_OLD_SND_FLAG;
    int b=x->x_snd!=s_empty;
    //fprintf(stderr,"%lx SND: old_sr_flags=%d SND_FLAG=%d || OUTCOME: OLD_SND_FLAG=%d not_empty=%d\n", (t_int)x, old_sr_flags, IEM_GUI_OLD_SND_FLAG, a, b);
    
    if(a && !b) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c create prect %d %d %d %d "
                 "-stroke $pd_colors(iemgui_nlet) "
                 "-tags {x%lxo%d x%lx outlet iemgui}\n",
             canvas, x1+i*k, y2-1, x1+i*k + IOWIDTH, y2,
             x, i, x);
    if(!a && b) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c delete x%lxo%d\n", canvas, x, i);

    a=old_sr_flags&IEM_GUI_OLD_RCV_FLAG;
    b=x->x_rcv!=s_empty;
    //fprintf(stderr,"%lx RCV: old_sr_flags=%d RCV_FLAG=%d || OUTCOME: OLD_RCV_FLAG=%d not_empty=%d\n", (t_int)x, old_sr_flags, IEM_GUI_OLD_RCV_FLAG, a, b);
    if(a && !b) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c create prect %d %d %d %d "
                 "-stroke $pd_colors(iemgui_nlet) "
                 "-tags {x%lxi%d x%lx inlet iemgui}\n",
             canvas, x1+i*k, y1, x1+i*k + IOWIDTH, y1+1,
             x, i, x);
    if(!a && b) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c delete x%lxi%d\n", canvas, x, i);
}

void iemgui_io_draw_move(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    t_class *c = pd_class((t_pd *)x);
    int x1,y1,x2,y2;
    c->c_wb->w_getrectfn((t_gobj *)x,canvas,&x1,&y1,&x2,&y2);
    int i, n = c==vu_class ? 2 : 1, k=(x2-x1)-IOWIDTH;
    if(!iemgui_has_snd(x) && canvas == x->x_glist) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c coords x%lxo%d %d %d %d %d\n",
            canvas, x, i, x1+i*k, y2-1, x1+i*k+IOWIDTH, y2);
    if(!iemgui_has_rcv(x) && canvas == x->x_glist) for (i=0; i<n; i++)
        sys_vgui(".x%lx.c coords x%lxi%d %d %d %d %d\n",
            canvas, x, i, x1+i*k, y1, x1+i*k+IOWIDTH, y1+1);
}

void iemgui_base_draw_new(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    t_class *c = pd_class((t_pd *)x);
    int x1,y1,x2,y2,gr=gop_redraw; gop_redraw=0;
    c->c_wb->w_getrectfn((t_gobj *)x,x->x_glist,&x1,&y1,&x2,&y2);
    gop_redraw=gr;
    sys_vgui(".x%lx.c create prect %d %d %d %d "
             "-stroke $pd_colors(iemgui_border) -fill #%6.6x "
             "-tags {%lxBASE x%lx text iemgui border}\n",
         canvas, x1,y1,x2,y2, x->x_bcol, x, x);
}

void iemgui_base_draw_move(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    t_class *c = pd_class((t_pd *)x);
    int x1,y1,x2,y2,gr=gop_redraw; gop_redraw=0;
    c->c_wb->w_getrectfn((t_gobj *)x,x->x_glist,&x1,&y1,&x2,&y2);
    gop_redraw=gr;
    sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n", canvas, x, x1, y1, x2, y2);
}

void iemgui_base_draw_config(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    char fcol[8]; sprintf(fcol,"#%6.6x", x->x_fcol);
    sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%6.6x\n", canvas, x, x->x_bcol);
}

void iemgui_draw_update(t_iemgui *x, t_glist *glist) {
    x->x_draw(x, x->x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

void iemgui_draw_new(t_iemgui *x) {
    x->x_draw(x, x->x_glist, IEM_GUI_DRAW_MODE_NEW);
    iemgui_label_draw_new(x);
    iemgui_draw_io(x,7);
    canvas_raise_all_cords(glist_getcanvas(x->x_glist)); // used to be inside x_draw
}

void iemgui_draw_config(t_iemgui *x) {
    x->x_draw(x, x->x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    iemgui_label_draw_config(x);
    //iemgui_base_draw_config(x); // can't
}

void iemgui_draw_move(t_iemgui *x) {
    x->x_draw(x, x->x_glist, IEM_GUI_DRAW_MODE_MOVE);
    iemgui_label_draw_move(x);
    //iemgui_base_draw_move(x); // can't
    iemgui_io_draw_move(x);
}

void iemgui_draw_erase(t_iemgui *x) {
    t_canvas *canvas=glist_getcanvas(x->x_glist);
    sys_vgui(".x%lx.c delete x%lx\n", canvas, x);
    scalehandle_draw_erase2(x);
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

const char *iemgui_typeface(t_iemgui *x) {
    int f = x->x_font_style;
    if(f == 0) return sys_font;
    if(f == 1) return "helvetica";
    if(f == 2) return "times";
    return "invalid-font";
}
// this uses a static buffer, so don't use it twice in the same sys_vgui.
// the static buffer could be replaced by a malloc when sys_vgui is replaced
// by something that frees that memory.
const char *iemgui_font(t_iemgui *x) {
    static char buf[64];
    sprintf(buf, "{{%s} -%d %s}", iemgui_typeface(x), x->x_fontsize, sys_fontweight);
    return buf;
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

#define GET_OUTLET t_outlet *out = x->x_obj.ob_outlet; /* can't use int o because there's not obj_nth_outlet function */
#define SEND_BY_SYMBOL (iemgui_has_snd(x) && x->x_snd->s_thing && (!chk_putin || x->x_put_in2out))

void iemgui_out_bang(t_iemgui *x, int o, int chk_putin) {
    GET_OUTLET outlet_bang(out);
    if(SEND_BY_SYMBOL) pd_bang(x->x_snd->s_thing);
}
void iemgui_out_float(t_iemgui *x, int o, int chk_putin, t_float f) {
    GET_OUTLET outlet_float(out,f);
    if(SEND_BY_SYMBOL) pd_float(x->x_snd->s_thing,f);
}
void iemgui_out_list(t_iemgui *x, int o, int chk_putin, t_symbol *s, int argc, t_atom *argv) {
    GET_OUTLET outlet_list(out,s,argc,argv);
    if(SEND_BY_SYMBOL) pd_list(x->x_snd->s_thing,s,argc,argv);
}

