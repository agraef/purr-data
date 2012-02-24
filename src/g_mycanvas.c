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
void my_canvas_draw_select(t_my_canvas* x, t_glist* glist);

/* ---------- cnv  my gui-canvas for a window ---------------- */

t_widgetbehavior my_canvas_widgetbehavior;
static t_class *my_canvas_class;

/* widget helper functions */

void my_canvas_draw_new(t_my_canvas *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

	t_scalehandle *sh = (t_scalehandle *)x->x_gui.x_handle;
	sprintf(sh->h_pathname, ".x%lx.h%lx", (t_int)canvas, (t_int)sh);

	t_scalehandle *lh = (t_scalehandle *)x->x_gui.x_lhandle;
	sprintf(lh->h_pathname, ".x%lx.h%lx", (t_int)canvas, (t_int)lh);

	//if (glist_isvisible(glist)) {

		sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%6.6x -outline #%6.6x -tags {%lxRECT %lxMYCNV}\n",
		         canvas, xpos, ypos,
		         xpos + x->x_vis_w, ypos + x->x_vis_h,
		         x->x_gui.x_bcol, x->x_gui.x_bcol, x, x);
		sys_vgui(".x%lx.c create rectangle %d %d %d %d -outline #%6.6x -tags {%lxBASE %lxMYCNV}\n",
		         canvas, xpos, ypos,
		         xpos + x->x_gui.x_w, ypos + x->x_gui.x_h,
		         x->x_gui.x_bcol, x, x);
		sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
		         -font {{%s} %d %s} -fill #%6.6x -tags {%lxLABEL %lxMYCNV}\n",
		         canvas, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy,
		         strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
		         x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
				 x->x_gui.x_lcol, x, x);
	//}
}

void my_canvas_draw_move(t_my_canvas *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

	if (glist_isvisible(canvas)) {

		sys_vgui(".x%lx.c coords %lxRECT %d %d %d %d\n",
		         canvas, x, xpos, ypos, xpos + x->x_vis_w,
		         ypos + x->x_vis_h);
		sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n",
		         canvas, x, xpos, ypos,
		         xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
		sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
		         canvas, x, xpos+x->x_gui.x_ldx,
		         ypos+x->x_gui.x_ldy);
		/* redraw scale handle rectangle if selected */
		if (x->x_gui.x_fsf.x_selected) {
			my_canvas_draw_select(x, x->x_gui.x_glist);
		}
	}
}

void my_canvas_draw_erase(t_my_canvas* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

	sys_vgui(".x%lx.c delete %lxMYCNV\n", canvas, x);
	sys_vgui(".x%lx.c dtag all %lxMYCNV\n", canvas, x);
	if (x->x_gui.x_fsf.x_selected) {
		t_scalehandle *sh = (t_scalehandle *)(x->x_gui.x_handle);
		sys_vgui("destroy %s\n", sh->h_pathname);
		t_scalehandle *lh = (t_scalehandle *)(x->x_gui.x_lhandle);
		sys_vgui("destroy %s\n", lh->h_pathname);
	}
    //sys_vgui(".x%lx.c delete %lxBASE\n", canvas, x);
    //sys_vgui(".x%lx.c delete %lxRECT\n", canvas, x);
    //sys_vgui(".x%lx.c delete %lxLABEL\n", canvas, x);
}

void my_canvas_draw_config(t_my_canvas* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

	/*
	char color[64];
	if (x->x_gui.x_fsf.x_selected)
		sprintf(color, "$select_color");
	else
		sprintf(color, "#%6.6x", x->x_gui.x_bcol);
	*/

    sys_vgui(".x%lx.c itemconfigure %lxRECT -fill #%6.6x -outline #%6.6x\n", canvas, x,
             x->x_gui.x_bcol, x->x_gui.x_bcol);
	if (x->x_gui.x_fsf.x_selected)
    	sys_vgui(".x%lx.c itemconfigure %lxBASE -outline $select_color\n", canvas, x);
	else
    	sys_vgui(".x%lx.c itemconfigure %lxBASE -outline #%6.6x\n", canvas, x,
             x->x_gui.x_bcol);
    sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} %d %s} -fill #%6.6x -text {%s} \n",
             canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
			 x->x_gui.x_lcol,
             strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
}

void my_canvas_draw_select(t_my_canvas* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
	t_scalehandle *sh = (t_scalehandle *)(x->x_gui.x_handle);
	t_scalehandle *lh = (t_scalehandle *)(x->x_gui.x_lhandle);

	if (glist_isvisible(canvas)) {

		if(x->x_gui.x_fsf.x_selected)
		{
		    sys_vgui(".x%lx.c itemconfigure %lxBASE -outline $select_color\n", canvas, x);

			// check if we are drawing inside a gop abstraction visible on parent canvas
			// if so, disable drawing of the handles
			if (x->x_gui.x_glist == glist_getcanvas(glist)) {

				if (x->x_gui.scale_vis)
					sys_vgui("destroy %s\n", sh->h_pathname);

				sys_vgui("canvas %s -width %d -height %d -bg $select_color -bd 0 -cursor bottom_right_corner\n",
					 sh->h_pathname, SCALEHANDLE_WIDTH, SCALEHANDLE_HEIGHT);
				sys_vgui(".x%x.c create window %d %d -anchor nw -width %d -height %d -window %s -tags {%lxSCALE %lxMYCNV}\n",
					 canvas, x->x_gui.x_obj.te_xpix + x->x_vis_w - SCALEHANDLE_WIDTH - 1,
					 x->x_gui.x_obj.te_ypix + x->x_vis_h - SCALEHANDLE_HEIGHT - 1,
					 SCALEHANDLE_WIDTH, SCALEHANDLE_HEIGHT,
					 sh->h_pathname, x, x);
				sys_vgui("bind %s <Button> {pd [concat %s _click 1 %%x %%y \\;]}\n",
					 sh->h_pathname, sh->h_bindsym->s_name);
				sys_vgui("bind %s <ButtonRelease> {pd [concat %s _click 0 0 0 \\;]}\n",
					 sh->h_pathname, sh->h_bindsym->s_name);
				sys_vgui("bind %s <Motion> {pd [concat %s _motion %%x %%y \\;]}\n",
					 sh->h_pathname, sh->h_bindsym->s_name);
				x->x_gui.scale_vis = 1;

				if (strcmp(x->x_gui.x_lab->s_name, "empty") != 0)
				{
					if (x->x_gui.label_vis)
						sys_vgui("destroy %s\n", lh->h_pathname);

					sys_vgui("canvas %s -width %d -height %d -bg $select_color -bd 0 -cursor crosshair\n",
						lh->h_pathname, LABELHANDLE_WIDTH, LABELHANDLE_HEIGHT);
					sys_vgui(".x%x.c create window %d %d -anchor nw -width %d -height %d -window %s -tags {%lxLABEL %lxMYCNV}\n",
						canvas, x->x_gui.x_obj.te_xpix+ x->x_gui.x_ldx - LABELHANDLE_WIDTH,
						x->x_gui.x_obj.te_ypix + x->x_gui.x_ldy - LABELHANDLE_HEIGHT,
						LABELHANDLE_WIDTH, LABELHANDLE_HEIGHT,
						lh->h_pathname, x, x);
					sys_vgui("bind %s <Button> {pd [concat %s _click 1 %%x %%y \\;]}\n",
						lh->h_pathname, lh->h_bindsym->s_name);
					sys_vgui("bind %s <ButtonRelease> {pd [concat %s _click 0 0 0 \\;]}\n",
						lh->h_pathname, lh->h_bindsym->s_name);
					sys_vgui("bind %s <Motion> {pd [concat %s _motion %%x %%y \\;]}\n",
						lh->h_pathname, lh->h_bindsym->s_name); 
					x->x_gui.label_vis = 1;
				}
			}

			sys_vgui(".x%lx.c addtag selected withtag %lxMYCNV\n", canvas, x);
		}
		else
		{
		    sys_vgui(".x%lx.c itemconfigure %lxBASE -outline #%6.6x\n", canvas, x, x->x_gui.x_bcol);
			sys_vgui(".x%lx.c dtag %lxMYCNV selected\n", canvas, x);
			sys_vgui("destroy %s\n", sh->h_pathname);
			x->x_gui.scale_vis = 0;
			sys_vgui("destroy %s\n", lh->h_pathname);
			x->x_gui.label_vis = 0;
		}
	}
}

static void my_canvas__clickhook(t_scalehandle *sh, t_floatarg f, t_floatarg xxx, t_floatarg yyy)
{

	t_my_canvas *x = (t_my_canvas *)(sh->h_master);

	if (xxx) {
		x->x_gui.scale_offset_x = xxx;
		x->x_gui.label_offset_x = xxx;
	}
	if (yyy) {
		x->x_gui.scale_offset_y = yyy;
		x->x_gui.label_offset_y = yyy;
	}

    int newstate = (int)f;
    if (sh->h_dragon && newstate == 0 && sh->h_scale)
    {
		/* done dragging */

		/* first set up the undo apply */
		canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

		if (sh->h_dragx || sh->h_dragy) {

			x->x_vis_w = x->x_vis_w + sh->h_dragx - x->x_gui.scale_offset_x;
			if (x->x_vis_w < SCALE_CNV_MINWIDTH)
				x->x_vis_w = SCALE_CNV_MINWIDTH;
			x->x_vis_h = x->x_vis_h + sh->h_dragy - x->x_gui.scale_offset_y;
			if (x->x_vis_h < SCALE_CNV_MINHEIGHT)
				x->x_vis_h = SCALE_CNV_MINHEIGHT;

			if (x->x_gui.x_w > x->x_vis_w || x->x_gui.x_h > x->x_vis_h) {
				x->x_gui.x_w = (x->x_vis_w > x->x_vis_h ? x->x_vis_h : x->x_vis_w);
				x->x_gui.x_h = x->x_gui.x_w;
			}
			canvas_dirty(x->x_gui.x_glist, 1);
		}

		int properties = gfxstub_haveproperties((void *)x);

		if (properties) {
			sys_vgui(".gfxstub%lx.rng.min_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.rng.min_ent insert 0 %d\n", properties, x->x_vis_w);
			sys_vgui(".gfxstub%lx.rng.max_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.rng.max_ent insert 0 %d\n", properties, x->x_vis_h);
			sys_vgui(".gfxstub%lx.dim.w_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.dim.w_ent insert 0 %d\n", properties, x->x_gui.x_w);
		}

		if (glist_isvisible(x->x_gui.x_glist))
		{
			sys_vgui(".x%x.c delete %s\n", x->x_gui.x_glist, sh->h_outlinetag);
			my_canvas_draw_move(x, x->x_gui.x_glist);
			sys_vgui("destroy %s\n", sh->h_pathname);
			iemgui_select((t_gobj *)x, x->x_gui.x_glist, 1);
			canvas_fixlinesfor(x->x_gui.x_glist, (t_text *)x);
			sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x->x_gui.x_glist);
		}
    }
    else if (!sh->h_dragon && newstate && sh->h_scale)
    {
		/* dragging */
		if (glist_isvisible(x->x_gui.x_glist))
		{
			sys_vgui("lower %s\n", sh->h_pathname);
			sys_vgui(".x%x.c create rectangle %d %d %d %d\
	 -outline $select_color -width 1 -tags %s\n",
				 x->x_gui.x_glist, x->x_gui.x_obj.te_xpix, x->x_gui.x_obj.te_ypix,
					x->x_gui.x_obj.te_xpix + x->x_vis_w,
					x->x_gui.x_obj.te_ypix + x->x_vis_h, sh->h_outlinetag);
		}

		sh->h_dragx = 0;
		sh->h_dragy = 0;
    }
    else if (sh->h_dragon && newstate == 0 && !sh->h_scale)
    {
		/* done dragging */

		/* first set up the undo apply */
		canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

		if (sh->h_dragx || sh->h_dragy) {

			x->x_gui.x_ldx = x->x_gui.x_ldx + sh->h_dragx - x->x_gui.label_offset_x;
			x->x_gui.x_ldy = x->x_gui.x_ldy + sh->h_dragy - x->x_gui.label_offset_y;

			canvas_dirty(x->x_gui.x_glist, 1);
		}

		int properties = gfxstub_haveproperties((void *)x);

		if (properties) {
			sys_vgui(".gfxstub%lx.label.xy.x_entry delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.label.xy.x_entry insert 0 %d\n", properties, x->x_gui.x_ldx);
			sys_vgui(".gfxstub%lx.label.xy.y_entry delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.label.xy.y_entry insert 0 %d\n", properties, x->x_gui.x_ldy);
		}

		if (glist_isvisible(x->x_gui.x_glist))
		{
			my_canvas_draw_move(x, x->x_gui.x_glist);
			sys_vgui("destroy %s\n", sh->h_pathname);
			iemgui_select((t_gobj *)x, x->x_gui.x_glist, 1);
			canvas_fixlinesfor(x->x_gui.x_glist, (t_text *)x);
			sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x->x_gui.x_glist);
		}
    }
    else if (!sh->h_dragon && newstate && !sh->h_scale)
    {
		/* dragging */
		if (glist_isvisible(x->x_gui.x_glist)) {
			sys_vgui("lower %s\n", sh->h_pathname);
			t_scalehandle *othersh = (t_scalehandle *)x->x_gui.x_handle;
			sys_vgui("lower .x%lx.h%lx\n", (t_int)glist_getcanvas(x->x_gui.x_glist), (t_int)othersh);
		}

		sh->h_dragx = 0;
		sh->h_dragy = 0;
    }
    sh->h_dragon = newstate;
}

static void my_canvas__motionhook(t_scalehandle *sh,
				    t_floatarg f1, t_floatarg f2)
{
    if (sh->h_dragon && sh->h_scale)
    {
		t_my_canvas *x = (t_my_canvas *)(sh->h_master);
		int dx = (int)f1, dy = (int)f2;
		int newx, newy;
		newx = x->x_gui.x_obj.te_xpix + x->x_vis_w - x->x_gui.scale_offset_x + dx;
		newy = x->x_gui.x_obj.te_ypix + x->x_vis_h - x->x_gui.scale_offset_y + dy;

		if (newx < x->x_gui.x_obj.te_xpix + SCALE_CNV_MINWIDTH)
			newx = x->x_gui.x_obj.te_xpix + SCALE_CNV_MINWIDTH;
		if (newy < x->x_gui.x_obj.te_ypix + SCALE_CNV_MINHEIGHT)
			newy = x->x_gui.x_obj.te_ypix + SCALE_CNV_MINHEIGHT;

		if (glist_isvisible(x->x_gui.x_glist)) {
			sys_vgui(".x%x.c coords %s %d %d %d %d\n",
				 x->x_gui.x_glist, sh->h_outlinetag, x->x_gui.x_obj.te_xpix,
				 x->x_gui.x_obj.te_ypix, newx, newy);
		}
		sh->h_dragx = dx;
		sh->h_dragy = dy;

		int properties = gfxstub_haveproperties((void *)x);

		if (properties) {
			int new_w = x->x_vis_w - x->x_gui.scale_offset_x + sh->h_dragx;
			int new_h = x->x_vis_h - x->x_gui.scale_offset_y + sh->h_dragy;
			sys_vgui(".gfxstub%lx.rng.min_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.rng.min_ent insert 0 %d\n", properties, new_w);
			sys_vgui(".gfxstub%lx.rng.max_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.rng.max_ent insert 0 %d\n", properties, new_h);
			int min = (new_w < new_h ? new_w : new_h);
			if (min <= x->x_gui.x_w) {
				sys_vgui(".gfxstub%lx.dim.w_ent delete 0 end\n", properties);
				sys_vgui(".gfxstub%lx.dim.w_ent insert 0 %d\n", properties, min);
			}
		}
    }
	else if (sh->h_dragon && !sh->h_scale)
	{
		t_my_canvas *x = (t_my_canvas *)(sh->h_master);
		int dx = (int)f1, dy = (int)f2;
		int newx, newy;
		newx = x->x_gui.x_obj.te_xpix + x->x_gui.x_ldx - x->x_gui.label_offset_x + dx;
		newy = x->x_gui.x_obj.te_ypix + x->x_gui.x_ldy - x->x_gui.label_offset_y + dy;

		sh->h_dragx = dx;
		sh->h_dragy = dy;

		int properties = gfxstub_haveproperties((void *)x);

		if (properties) {
			int new_x = x->x_gui.x_ldx - x->x_gui.label_offset_x + sh->h_dragx;
			int new_y = x->x_gui.x_ldy - x->x_gui.label_offset_y + sh->h_dragy;
			sys_vgui(".gfxstub%lx.label.xy.x_entry delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.label.xy.x_entry insert 0 %d\n", properties, new_x);
			sys_vgui(".gfxstub%lx.label.xy.y_entry delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.label.xy.y_entry insert 0 %d\n", properties, new_y);
		}

		if (glist_isvisible(x->x_gui.x_glist)) {
			int xpos=text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist);
    		int ypos=text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist);
    		t_canvas *canvas=glist_getcanvas(x->x_gui.x_glist);
			sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
		    	canvas, x, xpos+x->x_gui.x_ldx + sh->h_dragx - x->x_gui.label_offset_x,
		    	ypos+x->x_gui.x_ldy + sh->h_dragy - x->x_gui.label_offset_y);
		}
	}
}

void my_canvas_draw(t_my_canvas *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_MOVE)
        my_canvas_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
        my_canvas_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        my_canvas_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        my_canvas_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        my_canvas_draw_config(x, glist);
}

/* ------------------------ cnv widgetbehaviour----------------------------- */

static void my_canvas_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_my_canvas *x = (t_my_canvas *)z;
    
    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_vis_w;
    *yp2 = *yp1 + x->x_vis_h;
}

static void my_canvas_save(t_gobj *z, t_binbuf *b)
{
    t_my_canvas *x = (t_my_canvas *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiisssiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("cnv"), x->x_gui.x_w, x->x_vis_w, x->x_vis_h,
                srl[0], srl[1], srl[2], x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
                bflcol[0], bflcol[2], iem_symargstoint(&x->x_gui.x_isa));
    binbuf_addv(b, ";");
}

static void my_canvas_properties(t_gobj *z, t_glist *owner)
{
    t_my_canvas *x = (t_my_canvas *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |cnv| \
            ------selectable_dimensions(pix):------ %d %d size: 0.0 0.0 empty \
            ------visible_rectangle(pix)(pix):------ %d width: %d height: %d \
            %d empty empty %d %d empty %d \
            %s %s \
            %s %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, 1,
            x->x_vis_w, x->x_vis_h, 0,/*no_schedule*/
            -1, -1, -1, -1,/*no linlog, no init, no multi*/
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, -1/*no frontcolor*/, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void my_canvas_get_pos(t_my_canvas *x)
{
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
    {
        x->x_at[0].a_w.w_float = text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist);
        x->x_at[1].a_w.w_float = text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist);
        pd_list(x->x_gui.x_snd->s_thing, &s_list, 2, x->x_at);
    }
}

static void my_canvas_dialog(t_my_canvas *x, t_symbol *s, int argc, t_atom *argv)
{
	canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

    t_symbol *srl[3];
    int a = (int)atom_getintarg(0, argc, argv);
    int w = (int)atom_getintarg(2, argc, argv);
    int h = (int)atom_getintarg(3, argc, argv);
    int sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);

    x->x_gui.x_isa.x_loadinit = 0;
    if(a < 1)
        a = 1;
    x->x_gui.x_w = a;
    x->x_gui.x_h = x->x_gui.x_w;
    if(w < 1)
        w = 1;
    x->x_vis_w = w;
    if(h < 1)
        h = 1;
    x->x_vis_h = h;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);

	/* forcing redraw of the scale handle */
	if (x->x_gui.x_fsf.x_selected) {
		my_canvas_draw_select(x, x->x_gui.x_glist);
	}

	//ico@bukvic.net 100518 update scrollbars when object potentially exceeds window size
    t_canvas *canvas=(t_canvas *)glist_getcanvas(x->x_gui.x_glist);
	sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", (long unsigned int)canvas);
}

static void my_canvas_size(t_my_canvas *x, t_symbol *s, int ac, t_atom *av)
{
    int i = (int)atom_getintarg(0, ac, av);

    if(i < 1)
        i = 1;
    x->x_gui.x_w = i;
    x->x_gui.x_h = i;
    iemgui_size((void *)x, &x->x_gui);
}

static void my_canvas_delta(t_my_canvas *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void my_canvas_pos(t_my_canvas *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void my_canvas_vis_size(t_my_canvas *x, t_symbol *s, int ac, t_atom *av)
{
    int i;

    i = (int)atom_getintarg(0, ac, av);
    if(i < 1)
        i = 1;
    x->x_vis_w = i;
    if(ac > 1)
    {
        i = (int)atom_getintarg(1, ac, av);
        if(i < 1)
            i = 1;
    }
    x->x_vis_h = i;
    if(glist_isvisible(x->x_gui.x_glist))
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
}

static void my_canvas_color(t_my_canvas *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void my_canvas_send(t_my_canvas *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void my_canvas_receive(t_my_canvas *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void my_canvas_label(t_my_canvas *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void my_canvas_label_pos(t_my_canvas *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void my_canvas_label_font(t_my_canvas *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void *my_canvas_new(t_symbol *s, int argc, t_atom *argv)
{
    t_my_canvas *x = (t_my_canvas *)pd_new(my_canvas_class);
    int bflcol[]={-233017, -1, -66577};
    int a=IEM_GUI_DEFAULTSIZE, w=100, h=60;
    int ldx=20, ldy=12, f=2, i=0;
    int fs=14;
    char str[144];

    iem_inttosymargs(&x->x_gui.x_isa, 0);
    iem_inttofstyle(&x->x_gui.x_fsf, 0);

    if(((argc >= 10)&&(argc <= 13))
       &&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)&&IS_A_FLOAT(argv,2))
    {
        a = (int)atom_getintarg(0, argc, argv);
        w = (int)atom_getintarg(1, argc, argv);
        h = (int)atom_getintarg(2, argc, argv);
    }
    if((argc >= 12)&&(IS_A_SYMBOL(argv,3)||IS_A_FLOAT(argv,3))&&(IS_A_SYMBOL(argv,4)||IS_A_FLOAT(argv,4)))
    {
        i = 2;
        iemgui_new_getnames(&x->x_gui, 3, argv);
    }
    else if((argc == 11)&&(IS_A_SYMBOL(argv,3)||IS_A_FLOAT(argv,3)))
    {
        i = 1;
        iemgui_new_getnames(&x->x_gui, 3, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 3, 0);

    if(((argc >= 10)&&(argc <= 13))
       &&(IS_A_SYMBOL(argv,i+3)||IS_A_FLOAT(argv,i+3))&&IS_A_FLOAT(argv,i+4)
       &&IS_A_FLOAT(argv,i+5)&&IS_A_FLOAT(argv,i+6)
       &&IS_A_FLOAT(argv,i+7)&&IS_A_FLOAT(argv,i+8)
       &&IS_A_FLOAT(argv,i+9))
    {
            /* disastrously, the "label" sits in a different part of the
            message.  So we have to track its location separately (in
            the slot x_labelbindex) and initialize it specially here. */
        iemgui_new_dogetname(&x->x_gui, i+3, argv);
        x->x_gui.x_labelbindex = i+4;
        ldx = (int)atom_getintarg(i+4, argc, argv);
        ldy = (int)atom_getintarg(i+5, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(i+6, argc, argv));
        fs = (int)atom_getintarg(i+7, argc, argv);
        bflcol[0] = (int)atom_getintarg(i+8, argc, argv);
        bflcol[2] = (int)atom_getintarg(i+9, argc, argv);
    }
    if((argc == 13)&&IS_A_FLOAT(argv,i+10))
    {
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(i+10, argc, argv));
    }
    x->x_gui.x_draw = (t_iemfunptr)my_canvas_draw;
    x->x_gui.x_fsf.x_snd_able = 1;
    x->x_gui.x_fsf.x_rcv_able = 1;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if (!strcmp(x->x_gui.x_snd->s_name, "empty"))
        x->x_gui.x_fsf.x_snd_able = 0;
    if (!strcmp(x->x_gui.x_rcv->s_name, "empty"))
        x->x_gui.x_fsf.x_rcv_able = 0;
    if(a < 1)
        a = 1;
    x->x_gui.x_w = a;
    x->x_gui.x_h = x->x_gui.x_w;
    if(w < 1)
        w = 1;
    x->x_vis_w = w;
    if(h < 1)
        h = 1;
    x->x_vis_h = h;
    if(x->x_gui.x_fsf.x_font_style == 1) strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2) strcpy(x->x_gui.x_font, "times");
    else { x->x_gui.x_fsf.x_font_style = 0;
        strcpy(x->x_gui.x_font, sys_font); }
    if (x->x_gui.x_fsf.x_rcv_able)
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    iemgui_all_colfromload(&x->x_gui, bflcol);
    x->x_at[0].a_type = A_FLOAT;
    x->x_at[1].a_type = A_FLOAT;
    iemgui_verify_snd_ne_rcv(&x->x_gui);

	/* scale handle init */
    t_scalehandle *sh;
    char buf[64];
    x->x_gui.x_handle = pd_new(scalehandle_class);
    sh = (t_scalehandle *)x->x_gui.x_handle;
    sh->h_master = (t_gobj*)x;
    sprintf(buf, "_h%lx", (t_int)sh);
    pd_bind(x->x_gui.x_handle, sh->h_bindsym = gensym(buf));
    sprintf(sh->h_outlinetag, "h%lx", (t_int)sh);
    sh->h_dragon = 0;
	sh->h_scale = 1;
	x->x_gui.scale_offset_x = 0;
	x->x_gui.scale_offset_y = 0;
	x->x_gui.scale_vis = 0;

	/* label handle init */
	t_scalehandle *lh;
	char lhbuf[64];
	x->x_gui.x_lhandle = pd_new(scalehandle_class);
	lh = (t_scalehandle *)x->x_gui.x_lhandle;
	lh->h_master = (t_gobj*)x;
	sprintf(lhbuf, "_h%lx", (t_int)lh);
	pd_bind(x->x_gui.x_lhandle, lh->h_bindsym = gensym(lhbuf));
	sprintf(lh->h_outlinetag, "h%lx", (t_int)lh);
	lh->h_dragon = 0;
	lh->h_scale = 0;
	x->x_gui.label_offset_x = 0;
	x->x_gui.label_offset_y = 0;
	x->x_gui.label_vis = 0;

    return (x);
}

static void my_canvas_ff(t_my_canvas *x)
{
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);

	/* scale handle deconstructor */
    if (x->x_gui.x_handle)
    {
		pd_unbind(x->x_gui.x_handle, ((t_scalehandle *)x->x_gui.x_handle)->h_bindsym);
		pd_free(x->x_gui.x_handle);
    }

	/* label handle deconstructor */
	if (x->x_gui.x_lhandle)
	{
		pd_unbind(x->x_gui.x_lhandle, ((t_scalehandle *)x->x_gui.x_lhandle)->h_bindsym);
		pd_free(x->x_gui.x_lhandle);
	}
}

void g_mycanvas_setup(void)
{
    my_canvas_class = class_new(gensym("cnv"), (t_newmethod)my_canvas_new,
                                (t_method)my_canvas_ff, sizeof(t_my_canvas), CLASS_NOINLET, A_GIMME, 0);
    class_addcreator((t_newmethod)my_canvas_new, gensym("my_canvas"), A_GIMME, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_dialog, gensym("dialog"), A_GIMME, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_size, gensym("size"), A_GIMME, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_vis_size, gensym("vis_size"), A_GIMME, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_color, gensym("color"), A_GIMME, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(my_canvas_class, (t_method)my_canvas_get_pos, gensym("get_pos"), 0);

    scalehandle_class = class_new(gensym("_scalehandle"), 0, 0,
				  sizeof(t_scalehandle), CLASS_PD, 0);
    class_addmethod(scalehandle_class, (t_method)my_canvas__clickhook,
		    gensym("_click"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scalehandle_class, (t_method)my_canvas__motionhook,
		    gensym("_motion"), A_FLOAT, A_FLOAT, 0);

    my_canvas_widgetbehavior.w_getrectfn = my_canvas_getrect;
    my_canvas_widgetbehavior.w_displacefn = iemgui_displace;
    my_canvas_widgetbehavior.w_selectfn = iemgui_select;
    my_canvas_widgetbehavior.w_activatefn = NULL;
    my_canvas_widgetbehavior.w_deletefn = iemgui_delete;
    my_canvas_widgetbehavior.w_visfn = iemgui_vis;
    my_canvas_widgetbehavior.w_clickfn = NULL;
	my_canvas_widgetbehavior.w_displacefnwtag = iemgui_displace_withtag;
    class_setwidget(my_canvas_class, &my_canvas_widgetbehavior);
    class_sethelpsymbol(my_canvas_class, gensym("my_canvas"));
    class_setsavefn(my_canvas_class, my_canvas_save);
    class_setpropertiesfn(my_canvas_class, my_canvas_properties);
}
