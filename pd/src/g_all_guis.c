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

/*  #define GGEE_HSLIDER_COMPATIBLE  */

/*------------------ global varaibles -------------------------*/

int iemgui_color_hex[]=
{
    16579836, 10526880, 4210752, 16572640, 16572608,
    16579784, 14220504, 14220540, 14476540, 16308476,
    14737632, 8158332, 2105376, 16525352, 16559172,
    15263784, 1370132, 2684148, 3952892, 16003312,
    12369084, 6316128, 0, 9177096, 5779456,
    7874580, 2641940, 17488, 5256, 5767248
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

char *iemgui_vu_scale_str[]=
{
    "",
    "<-99",
    "",
    "",
    "",
    "-50",
    "",
    "",
    "",
    "-30",
    "",
    "",
    "",
    "-20",
    "",
    "",
    "",
    "-12",
    "",
    "",
    "",
    "-6",
    "",
    "",
    "",
    "-2",
    "",
    "",
    "",
    "-0dB",
    "",
    "",
    "",
    "+2",
    "",
    "",
    "",
    "+6",
    "",
    "",
    "",
    ">+12",
    "",
    "",
    "",
    "",
    "",
};


/*------------------ global functions -------------------------*/


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

int iemgui_modulo_color(int col)
{
    while(col >= IEM_GUI_MAX_COLOR)
        col -= IEM_GUI_MAX_COLOR;
    while(col < 0)
        col += IEM_GUI_MAX_COLOR;
    return(col);
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
    iemgui->x_fsf.x_put_in2out = 1;
    if(iemgui->x_fsf.x_snd_able && iemgui->x_fsf.x_rcv_able)
    {
        if(!strcmp(iemgui->x_snd->s_name, iemgui->x_rcv->s_name))
            iemgui->x_fsf.x_put_in2out = 0;
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
    else return (gensym("empty"));
}

void iemgui_new_getnames(t_iemgui *iemgui, int indx, t_atom *argv)
{
    if (argv)
    {
        iemgui->x_snd = iemgui_new_dogetname(iemgui, indx, argv);
        iemgui->x_rcv = iemgui_new_dogetname(iemgui, indx+1, argv);
        iemgui->x_lab = iemgui_new_dogetname(iemgui, indx+2, argv);
    }
    else iemgui->x_snd = iemgui->x_rcv = iemgui->x_lab = gensym("empty");
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
        else *symp = gensym("empty");
    }
}

    /* get the unexpanded versions of the symbols; initialize them if
    necessary. */
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

void iemgui_first_dollararg2sym(t_iemgui *iemgui, t_symbol **srlsym)
{
    /* delete this function */
}

void iemgui_all_col2save(t_iemgui *iemgui, int *bflcol)
{
    bflcol[0] = -1 - (((0xfc0000 & iemgui->x_bcol) >> 6)|
                      ((0xfc00 & iemgui->x_bcol) >> 4)|((0xfc & iemgui->x_bcol) >> 2));
    bflcol[1] = -1 - (((0xfc0000 & iemgui->x_fcol) >> 6)|
                      ((0xfc00 & iemgui->x_fcol) >> 4)|((0xfc & iemgui->x_fcol) >> 2));
    bflcol[2] = -1 - (((0xfc0000 & iemgui->x_lcol) >> 6)|
                      ((0xfc00 & iemgui->x_lcol) >> 4)|((0xfc & iemgui->x_lcol) >> 2));
}

void iemgui_all_colfromload(t_iemgui *iemgui, int *bflcol)
{
    if(bflcol[0] < 0)
    {
        bflcol[0] = -1 - bflcol[0];
        iemgui->x_bcol = ((bflcol[0] & 0x3f000) << 6)|((bflcol[0] & 0xfc0) << 4)|
            ((bflcol[0] & 0x3f) << 2);
    }
    else
    {
        bflcol[0] = iemgui_modulo_color(bflcol[0]);
        iemgui->x_bcol = iemgui_color_hex[bflcol[0]];
    }
    if(bflcol[1] < 0)
    {
        bflcol[1] = -1 - bflcol[1];
        iemgui->x_fcol = ((bflcol[1] & 0x3f000) << 6)|((bflcol[1] & 0xfc0) << 4)|
            ((bflcol[1] & 0x3f) << 2);
    }
    else
    {
        bflcol[1] = iemgui_modulo_color(bflcol[1]);
        iemgui->x_fcol = iemgui_color_hex[bflcol[1]];
    }
    if(bflcol[2] < 0)
    {
        bflcol[2] = -1 - bflcol[2];
        iemgui->x_lcol = ((bflcol[2] & 0x3f000) << 6)|((bflcol[2] & 0xfc0) << 4)|
            ((bflcol[2] & 0x3f) << 2);
    }
    else
    {
        bflcol[2] = iemgui_modulo_color(bflcol[2]);
        iemgui->x_lcol = iemgui_color_hex[bflcol[2]];
    }
}

int iemgui_compatible_col(int i)
{
    int j;

    if(i >= 0)
    {
        j = iemgui_modulo_color(i);
        return(iemgui_color_hex[(j)]);
    }
    else
        return((-1 -i)&0xffffff);
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

void iemgui_send(void *x, t_iemgui *iemgui, t_symbol *s)
{
    t_symbol *snd;
    int pargc, tail_len, nth_arg, sndable=1, oldsndrcvable=0;
    t_atom *pargv;

    if(iemgui->x_fsf.x_rcv_able)
        oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui->x_fsf.x_snd_able)
        oldsndrcvable += IEM_GUI_OLD_SND_FLAG;

    if(!strcmp(s->s_name, "empty")) sndable = 0;
    snd = iemgui_raute2dollar(s);
    iemgui->x_snd_unexpanded = snd;
    iemgui->x_snd = snd = canvas_realizedollar(iemgui->x_glist, snd);
    iemgui->x_fsf.x_snd_able = sndable;
    iemgui_verify_snd_ne_rcv(iemgui);
    (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_IO + oldsndrcvable);
}

void iemgui_receive(void *x, t_iemgui *iemgui, t_symbol *s)
{
    t_symbol *rcv;
    int pargc, tail_len, nth_arg, rcvable=1, oldsndrcvable=0;
    t_atom *pargv;

    if(iemgui->x_fsf.x_rcv_able)
        oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui->x_fsf.x_snd_able)
        oldsndrcvable += IEM_GUI_OLD_SND_FLAG;

    if(!strcmp(s->s_name, "empty")) rcvable = 0;
    rcv = iemgui_raute2dollar(s);
    iemgui->x_rcv_unexpanded = rcv;
    rcv = canvas_realizedollar(iemgui->x_glist, rcv);
    if(rcvable)
    {
        if(strcmp(rcv->s_name, iemgui->x_rcv->s_name))
        {
            if(iemgui->x_fsf.x_rcv_able)
                pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
            iemgui->x_rcv = rcv;
            pd_bind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        }
    }
    else if(!rcvable && iemgui->x_fsf.x_rcv_able)
    {
        pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        iemgui->x_rcv = rcv;
    }
    iemgui->x_fsf.x_rcv_able = rcvable;
    iemgui_verify_snd_ne_rcv(iemgui);
    (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_IO + oldsndrcvable);
}

void iemgui_label(void *x, t_iemgui *iemgui, t_symbol *s)
{
    t_symbol *lab;
    int pargc, tail_len, nth_arg;
    t_atom *pargv;

        /* tb: fix for empty label { */
        if (s == gensym(""))
                s = gensym("empty");
        /* tb } */

    lab = iemgui_raute2dollar(s);
    iemgui->x_lab_unexpanded = lab;
    iemgui->x_lab = lab = canvas_realizedollar(iemgui->x_glist, lab);

    if(glist_isvisible(iemgui->x_glist)) {
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -text {%s} \n",
                 glist_getcanvas(iemgui->x_glist), x,
                 strcmp(s->s_name, "empty")?iemgui->x_lab->s_name:"");
		iemgui_shouldvis(x, iemgui, IEM_GUI_DRAW_MODE_CONFIG);
	}
}

void iemgui_label_pos(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    iemgui->x_ldx = (int)atom_getintarg(0, ac, av);
    iemgui->x_ldy = (int)atom_getintarg(1, ac, av);
    if(glist_isvisible(iemgui->x_glist)) {
	    sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
	             glist_getcanvas(iemgui->x_glist), x,
                 text_xpix((t_object *)x,iemgui->x_glist)+iemgui->x_ldx,
                 text_ypix((t_object *)x,iemgui->x_glist)+iemgui->x_ldy);
		iemgui_shouldvis(x, iemgui, IEM_GUI_DRAW_MODE_CONFIG);
	}
}

void iemgui_label_font(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    int f = (int)atom_getintarg(0, ac, av);

    if(f == 1) strcpy(iemgui->x_font, "helvetica");
    else if(f == 2) strcpy(iemgui->x_font, "times");
    else
    {
        f = 0;
        strcpy(iemgui->x_font, sys_font);
    }
    iemgui->x_fsf.x_font_style = f;
    f = (int)atom_getintarg(1, ac, av);
    if(f < 4)
        f = 4;
    iemgui->x_fontsize = f;
    if(glist_isvisible(iemgui->x_glist)) {
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} -%d %s}\n",
                 glist_getcanvas(iemgui->x_glist), x, iemgui->x_font, 
				 iemgui->x_fontsize, sys_fontweight);
		iemgui_shouldvis(x, iemgui, IEM_GUI_DRAW_MODE_CONFIG);
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

void iemgui_label_getrect(t_iemgui x_gui, t_glist *x, int *xp1, int *yp1, int *xp2, int *yp2)
{
	//fprintf(stderr,"gop_redraw = %d\n", gop_redraw);
	if (!gop_redraw) {
		//fprintf(stderr,"ignoring label\n");
		return;
	}

	t_float width_multiplier;
	int label_length;	
	int label_x1;
	int label_y1;
	int label_x2;
	int label_y2;
	int actual_fontsize; //seems tk does its own thing when it comes to rendering
	int actual_height;

	if (x->gl_isgraph && !glist_istoplevel(x)) {
		//fprintf(stderr,"iemgui_label_getrect\n");

		if (strcmp(x_gui.x_lab->s_name, "empty")) {
			switch(x_gui.x_fsf.x_font_style) {
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
			if (x_gui.x_fontsize % 2 == 0) {
				actual_fontsize = x_gui.x_fontsize;
			} else {
				actual_fontsize = x_gui.x_fontsize;
			}
			actual_height = actual_fontsize;
			//exceptions
			if (x_gui.x_fsf.x_font_style == 0 && (actual_fontsize == 8 || actual_fontsize == 13 || actual_fontsize % 10 == 1 || actual_fontsize % 10 == 6 || (actual_fontsize > 48 && actual_fontsize < 100 && (actual_fontsize %10 == 4 || actual_fontsize %10 == 9))) ) {
				actual_fontsize += 1;
			}
			else if (x_gui.x_fsf.x_font_style == 1 && actual_fontsize >= 5 && actual_fontsize < 13 && actual_fontsize % 2 == 1)
				actual_fontsize += 1;
			else if (x_gui.x_fsf.x_font_style == 2 && actual_fontsize >= 5 && actual_fontsize % 2 == 1)
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
			//fprintf(stderr,"%f %d %d\n", width_multiplier, label_length, x_gui.x_fsf.x_font_style);
			//sys_vgui(".x%lx.c delete iemguiDEBUG\n", x);
			//sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags iemguiDEBUG\n", x, label_x1, label_y1, label_x2, label_y2);
			if (label_x1 < *xp1) *xp1 = label_x1;
			if (label_x2 > *xp2) *xp2 = label_x2;
			if (label_y1 < *yp1) *yp1 = label_y1;
			if (label_y2 > *yp2) *yp2 = label_y2;
			//DEBUG
			//sys_vgui(".x%lx.c delete iemguiDEBUG\n", x);
			//sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags iemguiDEBUG\n", x, *xp1, *yp1, *xp2, *yp2);
		}
	}
}

void iemgui_shouldvis(void *x, t_iemgui *iemgui, int mode)
{
	gop_redraw = 1;
	if(gobj_shouldvis(x, iemgui->x_glist)) {
		if (!iemgui->x_vis) {
			//fprintf(stderr,"draw new %d\n", mode);
    		(*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_NEW);
    		canvas_fixlinesfor(glist_getcanvas(iemgui->x_glist), (t_text*)x);
			iemgui->x_vis = 1;
			if (iemgui->x_glist != glist_getcanvas(iemgui->x_glist)) {
				// if we are inside gop and just have had our object's properties changed
				// we'll adjust our layer position to ensure that ordering is honored
				t_canvas *canvas = glist_getcanvas(iemgui->x_glist);
				t_gobj *y = (t_gobj *)iemgui->x_glist;
				gobj_vis(y, canvas, 0);
				gobj_vis(y, canvas, 1);
				// reorder it visually
				glist_redraw(canvas);
				/* some day when the object tagging is properly done for all GUI objects
				glist_noselect(canvas);
				glist_select(canvas, y);
				t_gobj *yy = canvas->gl_list;
				if (yy != y) {
					fprintf(stderr,"not bottom\n");
					while (yy && yy->g_next != y) {
						fprintf(stderr,"+\n");
						yy = yy->g_next;
					}
					// now we have yy which is right before our y graph
					t_object *ob = NULL;
					t_rtext *yr = NULL;
					if (yy) {
						yr = glist_findrtext(canvas, (t_text *)yy);
					}
					if (yr) {
						fprintf(stderr,"lower\n");
						sys_vgui(".x%lx.c lower selected %s\n", canvas, rtext_gettag(yr));
						sys_vgui(".x%lx.c raise selected %s\n", canvas, rtext_gettag(yr));
						//sys_vgui(".x%lx.c raise all_cords\n", canvas);
					} else {
						// fall back to legacy redraw for objects that are not patchable
						fprintf(stderr,"lower fallback redraw\n");
						canvas_redraw(canvas);
					}
				} else {
					// we get here if we are supposed to go all the way to the bottom
					fprintf(stderr,"lower to the bottom\n");
					sys_vgui(".x%lx.c lower selected\n", canvas);
				}
				glist_noselect(canvas);*/
			}
		}
		//fprintf(stderr,"draw move iemgui->x_w=%d\n", iemgui->x_w);
	    (*iemgui->x_draw)(x, iemgui->x_glist, mode);
	    canvas_fixlinesfor(glist_getcanvas(iemgui->x_glist), (t_text*)x);
	} else if (iemgui->x_vis) {
		//fprintf(stderr,"draw erase %d\n", mode);
		(*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_ERASE);
		iemgui->x_vis = 0;
	}
	gop_redraw = 0;
}

void iemgui_size(void *x, t_iemgui *iemgui)
{
    if(glist_isvisible(iemgui->x_glist))
    {
		iemgui_shouldvis(x, iemgui, IEM_GUI_DRAW_MODE_MOVE);		
    }
}

void iemgui_delta(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    iemgui->x_obj.te_xpix += (int)atom_getintarg(0, ac, av);
    iemgui->x_obj.te_ypix += (int)atom_getintarg(1, ac, av);
    if(glist_isvisible(iemgui->x_glist))
    {
		//fprintf(stderr,"iemgui_delta->shouldvis\n");
        iemgui_shouldvis(x, iemgui, IEM_GUI_DRAW_MODE_MOVE);
    }
}

void iemgui_pos(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    iemgui->x_obj.te_xpix = (int)atom_getintarg(0, ac, av);
    iemgui->x_obj.te_ypix = (int)atom_getintarg(1, ac, av);
    if(glist_isvisible(iemgui->x_glist))
		iemgui_shouldvis(x, iemgui, IEM_GUI_DRAW_MODE_MOVE);
}

void iemgui_color(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    iemgui->x_bcol = iemgui_compatible_col(atom_getintarg(0, ac, av));
    if(ac > 2)
    {
        iemgui->x_fcol = iemgui_compatible_col(atom_getintarg(1, ac, av));
        iemgui->x_lcol = iemgui_compatible_col(atom_getintarg(2, ac, av));
    }
    else
        iemgui->x_lcol = iemgui_compatible_col(atom_getintarg(1, ac, av));
    if(glist_isvisible(iemgui->x_glist))
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_CONFIG);
}

void iemgui_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_iemguidummy *x = (t_iemguidummy *)z;
    x->x_gui.x_obj.te_xpix += dx;
    x->x_gui.x_obj.te_ypix += dy;
    iemgui_shouldvis(x, &x->x_gui, IEM_GUI_DRAW_MODE_MOVE);
}

void iemgui_displace_withtag(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_iemguidummy *x = (t_iemguidummy *)z;
    x->x_gui.x_obj.te_xpix += dx;
    x->x_gui.x_obj.te_ypix += dy;
    //(*x->x_gui.x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(glist), (t_text *)z);
}


void iemgui_select(t_gobj *z, t_glist *glist, int selected)
{
    t_iemguidummy *x = (t_iemguidummy *)z;

    x->x_gui.x_fsf.x_selected = selected;
    (*x->x_gui.x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_SELECT);
}

void iemgui_delete(t_gobj *z, t_glist *glist)
{
    canvas_deletelinesfor(glist, (t_text *)z);
}

void iemgui_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_iemguidummy *x = (t_iemguidummy *)z;
	if (gobj_shouldvis(z, glist)) {
		if (vis) {
		    (*x->x_gui.x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_NEW);
			x->x_gui.x_vis = 1;
		}
		else
		{
		    (*x->x_gui.x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_ERASE);
		    sys_unqueuegui(z);
			x->x_gui.x_vis = 0;
		}
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
    int sndable=1, rcvable=1, oldsndrcvable=0;

    if(iemgui->x_fsf.x_rcv_able)
        oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui->x_fsf.x_snd_able)
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
    iemgui->x_isa.x_loadinit = init;
    if(!strcmp(srl[0]->s_name, "empty")) sndable = 0;
    if(!strcmp(srl[1]->s_name, "empty")) rcvable = 0;
    iemgui_all_raute2dollar(srl);
    iemgui_all_dollararg2sym(iemgui, srl);
    if(rcvable)
    {
        if(strcmp(srl[1]->s_name, iemgui->x_rcv->s_name))
        {
            if(iemgui->x_fsf.x_rcv_able)
                pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
            iemgui->x_rcv = srl[1];
            pd_bind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        }
    }
    else if(!rcvable && iemgui->x_fsf.x_rcv_able)
    {
        pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        iemgui->x_rcv = srl[1];
    }
    iemgui->x_snd = srl[0];
    iemgui->x_fsf.x_snd_able = sndable;
    iemgui->x_fsf.x_rcv_able = rcvable;
    iemgui->x_lcol = lcol & 0xffffff;
    iemgui->x_fcol = fcol & 0xffffff;
    iemgui->x_bcol = bcol & 0xffffff;
    iemgui->x_lab = srl[2];
    iemgui->x_ldx = ldx;
    iemgui->x_ldy = ldy;
    if(f == 1) strcpy(iemgui->x_font, "helvetica");
    else if(f == 2) strcpy(iemgui->x_font, "times");
    else
    {
        f = 0;
        strcpy(iemgui->x_font, sys_font);
    }
    iemgui->x_fsf.x_font_style = f;
    if(fs < 4)
        fs = 4;
    iemgui->x_fontsize = fs;
    iemgui_verify_snd_ne_rcv(iemgui);
    canvas_dirty(iemgui->x_glist, 1);
    return(oldsndrcvable);
}

void iem_inttosymargs(t_iem_init_symargs *symargp, int n)
{
    memset(symargp, 0, sizeof(*symargp));
    symargp->x_loadinit = (n >>  0);
    symargp->x_scale = (n >>  20);
    symargp->x_flashed = 0;
    symargp->x_locked = 0;
    symargp->x_reverse = 0;
    symargp->dummy = 0;
}

int iem_symargstoint(t_iem_init_symargs *symargp)
{
    return (
        (((symargp->x_loadinit & 1) <<  0) |
        ((symargp->x_scale & 1) <<  20)));
}

void iem_inttofstyle(t_iem_fstyle_flags *fstylep, int n)
{
    memset(fstylep, 0, sizeof(*fstylep));
    fstylep->x_font_style = (n >> 0);
    fstylep->x_shiftdown = 0;
    fstylep->x_selected = 0;
    fstylep->x_finemoved = 0;
    fstylep->x_put_in2out = 0;
    fstylep->x_change = 0;
    fstylep->x_thick = 0;
    fstylep->x_lin0_log1 = 0;
    fstylep->x_steady = 0;
    fstylep->dummy = 0;
}

int iem_fstyletoint(t_iem_fstyle_flags *fstylep)
{
    return ((fstylep->x_font_style << 0) & 63);
}
