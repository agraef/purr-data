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
static void vslider_draw_select(t_vslider* x, t_glist* glist);

/* ------------ vsl gui-vertical  slider ----------------------- */

t_widgetbehavior vslider_widgetbehavior;
static t_class *vslider_class;

/* widget helper functions */

static void vslider_draw_update(t_gobj *client, t_glist *glist)
{
    t_vslider *x = (t_vslider *)client;
    t_canvas *canvas=glist_getcanvas(glist);
    if (glist_isvisible(glist))
    {
        int r = text_ypix(&x->x_gui.x_obj, glist) + 2 + x->x_gui.x_h - (x->x_val + 50)/100;
        int xpos=text_xpix(&x->x_gui.x_obj, glist);

        sys_vgui(".x%lx.c coords %lxKNOB %d %d %d %d\n",
                 canvas, x, xpos+2, r,
                 xpos + x->x_gui.x_w-2, r);
        if(x->x_val == x->x_center)
        {
            if(!x->x_thick)
            {
                sys_vgui(".x%lx.c itemconfigure %lxKNOB -strokewidth 7\n", canvas, x);
                x->x_thick = 1;
            }
        }
        else
        {
            if(x->x_thick)
            {
                sys_vgui(".x%lx.c itemconfigure %lxKNOB -strokewidth 3\n", canvas, x);
                x->x_thick = 0;
            }
        }
    }
}

static void vslider_draw_new(t_vslider *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int r = ypos + 2 + x->x_gui.x_h - (x->x_val + 50)/100;
    t_canvas *canvas=glist_getcanvas(glist);

	t_scalehandle *sh = (t_scalehandle *)x->x_gui.x_handle;
	sprintf(sh->h_pathname, ".x%lx.h%lx", (t_int)canvas, (t_int)sh);
	t_scalehandle *lh = (t_scalehandle *)x->x_gui.x_lhandle;
	sprintf(lh->h_pathname, ".x%lx.h%lx", (t_int)canvas, (t_int)lh);

	//if (glist_isvisible(canvas)) {

		char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

		sys_vgui(".x%lx.c create prect %d %d %d %d -fill #%6.6x -tags {%lxBASE %lxVSLDR %s text}\n",
		         canvas, xpos, ypos,
		         xpos + x->x_gui.x_w, ypos + x->x_gui.x_h+5,
		         x->x_gui.x_bcol, x, x, nlet_tag);
		sys_vgui(".x%lx.c create polyline %d %d %d %d -strokewidth 3 -stroke #%6.6x -tags {%lxKNOB %lxVSLDR %s text}\n",
		         canvas, xpos+2, r,
		         xpos + x->x_gui.x_w-2, r, x->x_gui.x_fcol, x, x, nlet_tag);
		sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
		         -font {{%s} -%d %s} -fill #%6.6x -tags {%lxLABEL %lxVSLDR %s text}\n",
		         canvas, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy,
		         strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
		         x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight, 
		         x->x_gui.x_lcol, x, x, nlet_tag);
		if(!x->x_gui.x_fsf.x_snd_able && canvas == x->x_gui.x_glist)
		    sys_vgui(".x%lx.c create prect %d %d %d %d -tags {%lxVSLDR%so%d %so%d %lxVSLDR %s outlet}\n",
		         canvas,
		         xpos, ypos + x->x_gui.x_h+4,
		         xpos+7, ypos + x->x_gui.x_h+5,
		         x, nlet_tag, 0, nlet_tag, 0, x, nlet_tag);
		if(!x->x_gui.x_fsf.x_rcv_able && canvas == x->x_gui.x_glist)
		    sys_vgui(".x%lx.c create prect %d %d %d %d -tags {%lxVSLDR%si%d %si%d %lxVSLDR %s inlet}\n",
		         canvas,
		         xpos, ypos,
		         xpos+7, ypos+1,
		         x, nlet_tag, 0, nlet_tag, 0, x, nlet_tag);
	//}
}

static void vslider_draw_move(t_vslider *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int r = ypos + 2 + x->x_gui.x_h - (x->x_val + 50)/100;
    t_canvas *canvas=glist_getcanvas(glist);

	if (glist_isvisible(canvas)) {

		char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

		sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n",
		         canvas, x,
		         xpos, ypos,
		         xpos + x->x_gui.x_w, ypos + x->x_gui.x_h+5);
		sys_vgui(".x%lx.c coords %lxKNOB %d %d %d %d\n",
		         canvas, x, xpos+2, r,
		         xpos + x->x_gui.x_w-2, r);
		sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
		         canvas, x, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy);
		if(!x->x_gui.x_fsf.x_snd_able && canvas == x->x_gui.x_glist)
		    sys_vgui(".x%lx.c coords %lxVSLDR%so%d %d %d %d %d\n",
		         canvas, x, nlet_tag, 0,
		         xpos, ypos + x->x_gui.x_h+4,
		         xpos+7, ypos + x->x_gui.x_h+5);
		if(!x->x_gui.x_fsf.x_rcv_able && canvas == x->x_gui.x_glist)
		    sys_vgui(".x%lx.c coords %lxVSLDR%si%d %d %d %d %d\n",
		         canvas, x, nlet_tag, 0,
		         xpos, ypos,
		         xpos+7, ypos+1);
		/* redraw scale handle rectangle if selected */
		if (x->x_gui.x_fsf.x_selected) {
			vslider_draw_select(x, x->x_gui.x_glist);
		}
	}
}

static void vslider_draw_erase(t_vslider* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

	sys_vgui(".x%lx.c delete %lxVSLDR\n", canvas, x);
	sys_vgui(".x%lx.c dtag all %lxVSLDR\n", canvas, x);
	if (x->x_gui.x_fsf.x_selected) {
		t_scalehandle *sh = (t_scalehandle *)(x->x_gui.x_handle);
		sys_vgui("destroy %s\n", sh->h_pathname);
		sys_vgui(".x%lx.c delete %lxSCALE\n", canvas, x);
		t_scalehandle *lh = (t_scalehandle *)(x->x_gui.x_lhandle);
		sys_vgui("destroy %s\n", lh->h_pathname);
		sys_vgui(".x%lx.c delete %lxLABELH\n", canvas, x);
	}
}

static void vslider_draw_config(t_vslider* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

	/*
	char color[64];
	if (x->x_gui.x_fsf.x_selected)
		sprintf(color, "$select_color");
	else
		sprintf(color, "#%6.6x", x->x_gui.x_lcol);
	*/

	if (x->x_gui.x_fsf.x_selected && x->x_gui.x_glist == canvas)
		sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} -%d %s} -fill $select_color -text {%s} \n",
		         canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight, 
		         strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
	else
		sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} -%d %s} -fill #%6.6x -text {%s} \n",
		         canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight, 
		         x->x_gui.x_lcol,
		         strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
    sys_vgui(".x%lx.c itemconfigure %lxKNOB -stroke #%6.6x\n .x%lx.c itemconfigure %lxBASE -fill #%6.6x\n",
			 canvas, x, x->x_gui.x_fcol, canvas, x, x->x_gui.x_bcol);
    /*sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%6.6x\n", canvas,
             x, x->x_gui.x_bcol);*/
}

static void vslider_draw_io(t_vslider* x,t_glist* glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

	if (glist_isvisible(canvas) && canvas == x->x_gui.x_glist) {

		char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

		if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
		    sys_vgui(".x%lx.c create prect %d %d %d %d -tags {%lxVSLDR%so%d %so%d %lxVSLDR %s outlet}\n",
		         canvas,
		         xpos, ypos + x->x_gui.x_h+4,
		         xpos+7, ypos + x->x_gui.x_h+5,
		         x, nlet_tag, 0, nlet_tag, 0, x, nlet_tag);
		if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
		    sys_vgui(".x%lx.c delete %lxVSLDR%so%d\n", canvas, x, nlet_tag, 0);
		if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
		    sys_vgui(".x%lx.c create prect %d %d %d %d -tags {%lxVSLDR%si%d %si%d %lxVSLDR %s inlet}\n",
		         canvas,
		         xpos, ypos,
		         xpos+7, ypos+1,
		         x, nlet_tag, 0, nlet_tag, 0, x, nlet_tag);
		if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
		    sys_vgui(".x%lx.c delete %lxVSLDR%si%d\n", canvas, x, nlet_tag, 0);
	}
}

static void vslider_draw_select(t_vslider *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
	t_scalehandle *sh = (t_scalehandle *)(x->x_gui.x_handle);
	t_scalehandle *lh = (t_scalehandle *)(x->x_gui.x_lhandle);

	//if (glist_isvisible(canvas)) {

		if(x->x_gui.x_fsf.x_selected)
		{
			// check if we are drawing inside a gop abstraction visible on parent canvas
			// if so, disable highlighting
			if (x->x_gui.x_glist == glist_getcanvas(glist)) {

				char *nlet_tag = iem_get_tag(glist, (t_iemgui *)x);

				sys_vgui(".x%lx.c itemconfigure %lxBASE -stroke $select_color\n", canvas, x);
				sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill $select_color\n", canvas, x);

				if (x->x_gui.scale_vis) {
					sys_vgui("destroy %s\n", sh->h_pathname);
					sys_vgui(".x%lx.c delete %lxSCALE\n", canvas, x);
				}

				sys_vgui("canvas %s -width %d -height %d -bg $select_color -bd 0 -cursor bottom_right_corner\n",
					 sh->h_pathname, SCALEHANDLE_WIDTH, SCALEHANDLE_HEIGHT);
				sys_vgui(".x%x.c create window %d %d -anchor nw -width %d -height %d -window %s -tags {%lxSCALE %lxVSLDR %s}\n",
					 canvas, x->x_gui.x_obj.te_xpix + x->x_gui.x_w - SCALEHANDLE_WIDTH - 1,
					 x->x_gui.x_obj.te_ypix + 5 + x->x_gui.x_h - SCALEHANDLE_HEIGHT - 1,
					 SCALEHANDLE_WIDTH, SCALEHANDLE_HEIGHT,
					 sh->h_pathname, x, x, nlet_tag);
				sys_vgui("bind %s <Button> {pd [concat %s _click 1 %%x %%y \\;]}\n",
					 sh->h_pathname, sh->h_bindsym->s_name);
				sys_vgui("bind %s <ButtonRelease> {pd [concat %s _click 0 0 0 \\;]}\n",
					 sh->h_pathname, sh->h_bindsym->s_name);
				sys_vgui("bind %s <Motion> {pd [concat %s _motion %%x %%y \\;]}\n",
					 sh->h_pathname, sh->h_bindsym->s_name);
				x->x_gui.scale_vis = 1;

				if (strcmp(x->x_gui.x_lab->s_name, "empty") != 0)
				{
					if (x->x_gui.label_vis) {
						sys_vgui("destroy %s\n", lh->h_pathname);
						sys_vgui(".x%lx.c delete %lxLABELH\n", canvas, x);
					}

					sys_vgui("canvas %s -width %d -height %d -bg $select_color -bd 0 -cursor crosshair\n",
						lh->h_pathname, LABELHANDLE_WIDTH, LABELHANDLE_HEIGHT);
					sys_vgui(".x%x.c create window %d %d -anchor nw -width %d -height %d -window %s -tags {%lxLABEL %lxLABELH %lxVSLDR %s}\n",
						canvas, x->x_gui.x_obj.te_xpix+ x->x_gui.x_ldx - LABELHANDLE_WIDTH,
						x->x_gui.x_obj.te_ypix + x->x_gui.x_ldy - LABELHANDLE_HEIGHT,
						LABELHANDLE_WIDTH, LABELHANDLE_HEIGHT,
						lh->h_pathname, x, x, x, nlet_tag);
					sys_vgui("bind %s <Button> {pd [concat %s _click 1 %%x %%y \\;]}\n",
						lh->h_pathname, lh->h_bindsym->s_name);
					sys_vgui("bind %s <ButtonRelease> {pd [concat %s _click 0 0 0 \\;]}\n",
						lh->h_pathname, lh->h_bindsym->s_name);
					sys_vgui("bind %s <Motion> {pd [concat %s _motion %%x %%y \\;]}\n",
						lh->h_pathname, lh->h_bindsym->s_name); 
					x->x_gui.label_vis = 1;
				}
			}

			sys_vgui(".x%lx.c addtag selected withtag %lxVSLDR\n", canvas, x);
		}
		else
		{
		    sys_vgui(".x%lx.c itemconfigure %lxBASE -stroke #%6.6x\n", canvas, x, IEM_GUI_COLOR_NORMAL);
		    sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n", canvas, x, x->x_gui.x_lcol);
			sys_vgui(".x%lx.c dtag %lxVSLDR selected\n", canvas, x);
			sys_vgui("destroy %s\n", sh->h_pathname);
			sys_vgui(".x%lx.c delete %lxSCALE\n", canvas, x);
			x->x_gui.scale_vis = 0;
			sys_vgui("destroy %s\n", lh->h_pathname);
			sys_vgui(".x%lx.c delete %lxLABELH\n", canvas, x);
			x->x_gui.label_vis = 0;
		}
	//}
}

void vslider_check_minmax(t_vslider *x, double min, double max);
void vslider_check_height(t_vslider *x, int w);

static void vslider__clickhook(t_scalehandle *sh, t_floatarg f, t_floatarg xxx, t_floatarg yyy)
{

	t_vslider *x = (t_vslider *)(sh->h_master);

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

			double height_change_ratio = (double)(x->x_gui.x_h + sh->h_dragy - x->x_gui.scale_offset_y)/(double)x->x_gui.x_h;
			x->x_val = x->x_val * height_change_ratio;

			x->x_gui.x_w = x->x_gui.x_w + sh->h_dragx - x->x_gui.scale_offset_x;
			if (x->x_gui.x_w < SCALE_VSLD_MINWIDTH)
				x->x_gui.x_w = SCALE_VSLD_MINWIDTH;
			x->x_gui.x_h = x->x_gui.x_h + sh->h_dragy - x->x_gui.scale_offset_y;
			if (x->x_gui.x_h < SCALE_VSLD_MINHEIGHT)
				x->x_gui.x_h = SCALE_VSLD_MINHEIGHT;

			canvas_dirty(x->x_gui.x_glist, 1);
		}

	    vslider_check_height(x, x->x_gui.x_h);
	    vslider_check_minmax(x, x->x_min, x->x_max);

		int properties = gfxstub_haveproperties((void *)x);

		if (properties) {
			sys_vgui(".gfxstub%lx.dim.w_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.dim.w_ent insert 0 %d\n", properties, x->x_gui.x_w);
			sys_vgui(".gfxstub%lx.dim.h_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.dim.h_ent insert 0 %d\n", properties, x->x_gui.x_h);
		}

		if (glist_isvisible(x->x_gui.x_glist))
		{
			sys_vgui(".x%x.c delete %s\n", x->x_gui.x_glist, sh->h_outlinetag);
			vslider_draw_move(x, x->x_gui.x_glist);
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
			sys_vgui(".x%x.c create prect %d %d %d %d\
	 -stroke $select_color -strokewidth 1 -tags %s\n",
				 x->x_gui.x_glist, x->x_gui.x_obj.te_xpix, x->x_gui.x_obj.te_ypix,
					x->x_gui.x_obj.te_xpix + x->x_gui.x_w,
					x->x_gui.x_obj.te_ypix + 5 + x->x_gui.x_h, sh->h_outlinetag);
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
			sys_vgui(".gfxstub%lx.dim.w_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.dim.w_ent insert 0 %d\n", properties, x->x_gui.x_w);
			//sys_vgui(".gfxstub%lx.dim.h_ent delete 0 end\n", properties);
			//sys_vgui(".gfxstub%lx.dim.h_ent insert 0 %d\n", properties, x->x_gui.x_h);
		}

		if (glist_isvisible(x->x_gui.x_glist))
		{
			sys_vgui(".x%x.c delete %s\n", x->x_gui.x_glist, sh->h_outlinetag);
			vslider_draw_move(x, x->x_gui.x_glist);
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

static void vslider__motionhook(t_scalehandle *sh,
				    t_floatarg f1, t_floatarg f2)
{
    if (sh->h_dragon && sh->h_scale)
    {
		t_vslider *x = (t_vslider *)(sh->h_master);
		int dx = (int)f1, dy = (int)f2;
		int newx, newy;
		newx = x->x_gui.x_obj.te_xpix + x->x_gui.x_w - x->x_gui.scale_offset_x + dx;
		newy = x->x_gui.x_obj.te_ypix + x->x_gui.x_h - x->x_gui.scale_offset_y + dy;

		if (newx < x->x_gui.x_obj.te_xpix + SCALE_VSLD_MINWIDTH)
			newx = x->x_gui.x_obj.te_xpix + SCALE_VSLD_MINWIDTH;
		if (newy < x->x_gui.x_obj.te_ypix + SCALE_VSLD_MINHEIGHT)
			newy = x->x_gui.x_obj.te_ypix + SCALE_VSLD_MINHEIGHT;

		if (glist_isvisible(x->x_gui.x_glist)) {
			sys_vgui(".x%x.c coords %s %d %d %d %d\n",
				 x->x_gui.x_glist, sh->h_outlinetag, x->x_gui.x_obj.te_xpix,
				 x->x_gui.x_obj.te_ypix, newx, newy + 5);
		}
		sh->h_dragx = dx;
		sh->h_dragy = dy;

		int properties = gfxstub_haveproperties((void *)x);

		if (properties) {
			int new_w = x->x_gui.x_w - x->x_gui.scale_offset_x + sh->h_dragx;
			int new_h = x->x_gui.x_h - x->x_gui.scale_offset_y + sh->h_dragy;
			sys_vgui(".gfxstub%lx.dim.w_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.dim.w_ent insert 0 %d\n", properties, new_w);
			sys_vgui(".gfxstub%lx.dim.h_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.dim.h_ent insert 0 %d\n", properties, new_h);
		}
    }
	if (sh->h_dragon && !sh->h_scale)
    {
		t_bng *x = (t_bng *)(sh->h_master);
		int dx = (int)f1, dy = (int)f2;
		int newx, newy;
		newx = x->x_gui.x_obj.te_xpix + x->x_gui.x_w - x->x_gui.scale_offset_x + dx;
		newy = x->x_gui.x_obj.te_ypix + x->x_gui.x_h - x->x_gui.scale_offset_y + dy;

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



void vslider_draw(t_vslider *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui(x, glist, vslider_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        vslider_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW) {
        vslider_draw_new(x, glist);
		sys_vgui(".x%lx.c raise all_cords\n", glist_getcanvas(glist));
	}
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        vslider_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        vslider_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        vslider_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        vslider_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ vsl widgetbehaviour----------------------------- */

static void vslider_getrect(t_gobj *z, t_glist *glist,
                            int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_vslider* x = (t_vslider*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h + 5;

	iemgui_label_getrect(x->x_gui, glist, xp1, yp1, xp2, yp2);
}

static void vslider_save(t_gobj *z, t_binbuf *b)
{
    t_vslider *x = (t_vslider *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiffiisssiiiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("vsl"), x->x_gui.x_w, x->x_gui.x_h,
                (t_float)x->x_min, (t_float)x->x_max,
                x->x_lin0_log1, iem_symargstoint(&x->x_gui.x_isa),
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2],
                x->x_val, x->x_steady);
    binbuf_addv(b, ";");
}

void vslider_check_height(t_vslider *x, int h)
{
    if(h < IEM_SL_MINSIZE)
        h = IEM_SL_MINSIZE;
    x->x_gui.x_h = h;
	x->x_center = (x->x_gui.x_h-1)*50;
    if(x->x_val > (x->x_gui.x_h*100 - 100))
    {
        x->x_pos = x->x_gui.x_h*100 - 100;
        x->x_val = x->x_pos;
    }
    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_h - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

void vslider_check_minmax(t_vslider *x, double min, double max)
{
    if(x->x_lin0_log1)
    {
        if((min == 0.0)&&(max == 0.0))
            max = 1.0;
        if(max > 0.0)
        {
            if(min <= 0.0)
                min = 0.01*max;
        }
        else
        {
            if(min > 0.0)
                max = 0.01*min;
        }
    }
    x->x_min = min;
    x->x_max = max;
    if(x->x_min > x->x_max)                /* bugfix */
        x->x_gui.x_isa.x_reverse = 1;
    else
        x->x_gui.x_isa.x_reverse = 0;
    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_gui.x_h - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

static void vslider_properties(t_gobj *z, t_glist *owner)
{
    t_vslider *x = (t_vslider *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);

    sprintf(buf, "pdtk_iemgui_dialog %%s |vsl| \
            --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
            -----------output-range:----------- %g bottom: %g top: %d \
            %d lin log %d %d empty %d \
            {%s} {%s} \
            {%s} %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, IEM_GUI_MINSIZE, x->x_gui.x_h, IEM_SL_MINSIZE,
            x->x_min, x->x_max, 0,/*no_schedule*/
            x->x_lin0_log1, x->x_gui.x_isa.x_loadinit, x->x_steady, -1,/*no multi, but iem-characteristic*/
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void vslider_bang(t_vslider *x)
{
    double out;

    if(x->x_lin0_log1)
        out = x->x_min*exp(x->x_k*(double)(x->x_val)*0.01);
    else
		if (x->x_is_last_float && x->x_last <= x->x_max && x->x_last >= x->x_min)
			out = x->x_last;
		else
        	out = (double)(x->x_val)*0.01*x->x_k + x->x_min;
    if((out < 1.0e-10)&&(out > -1.0e-10))
        out = 0.0;

    outlet_float(x->x_gui.x_obj.ob_outlet, out);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
        pd_float(x->x_gui.x_snd->s_thing, out);
}

static void vslider_dialog(t_vslider *x, t_symbol *s, int argc, t_atom *argv)
{
	canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    double min = (double)atom_getfloatarg(2, argc, argv);
    double max = (double)atom_getfloatarg(3, argc, argv);
    int lilo = (int)atom_getintarg(4, argc, argv);
    int steady = (int)atom_getintarg(17, argc, argv);
    int sr_flags;

    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady)
        x->x_steady = 1;
    else
        x->x_steady = 0;
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    x->x_gui.x_w = iemgui_clip_size(w);
	int old_height = x->x_gui.x_h;
    vslider_check_height(x, h);
	if (x->x_gui.x_h != old_height) {
		x->x_val = x->x_val * ((double)x->x_gui.x_h/(double)old_height);
	}
    vslider_check_minmax(x, min, max);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    //(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    //canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
	iemgui_shouldvis((void *)x, &x->x_gui, IEM_GUI_DRAW_MODE_MOVE);

	/* forcing redraw of the scale handle */
	if (x->x_gui.x_fsf.x_selected) {
		vslider_draw_select(x, x->x_gui.x_glist);
	}

	//ico@bukvic.net 100518 update scrollbars when object potentially exceeds window size
    t_canvas *canvas=(t_canvas *)glist_getcanvas(x->x_gui.x_glist);
	sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", (long unsigned int)canvas);
}

static void vslider_motion(t_vslider *x, t_floatarg dx, t_floatarg dy)
{
	x->x_is_last_float = 0;
    int old = x->x_val;

    if(x->x_gui.x_fsf.x_finemoved)
        x->x_pos -= (int)dy;
    else
        x->x_pos -= 100*(int)dy;
    x->x_val = x->x_pos;
    if(x->x_val > (100*x->x_gui.x_h - 100))
    {
        x->x_val = 100*x->x_gui.x_h - 100;
        x->x_pos += 50;
        x->x_pos -= x->x_pos%100;
    }
    if(x->x_val < 0)
    {
        x->x_val = 0;
        x->x_pos -= 50;
        x->x_pos -= x->x_pos%100;
    }
    if(old != x->x_val)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        vslider_bang(x);
    }
}

static void vslider_click(t_vslider *x, t_floatarg xpos, t_floatarg ypos,
                          t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    if(!x->x_steady)
        x->x_val = (int)(100.0 * (x->x_gui.x_h + text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist) - ypos));
    if(x->x_val > (100*x->x_gui.x_h - 100))
        x->x_val = 100*x->x_gui.x_h - 100;
    if(x->x_val < 0)
        x->x_val = 0;
    x->x_pos = x->x_val;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    vslider_bang(x);
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
        (t_glistmotionfn)vslider_motion, 0, xpos, ypos);
}

static int vslider_newclick(t_gobj *z, struct _glist *glist,
                            int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_vslider* x = (t_vslider *)z;

    if(doit)
    {
        vslider_click( x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift,
                       0, (t_floatarg)alt);
        if(shift)
            x->x_gui.x_fsf.x_finemoved = 1;
        else
            x->x_gui.x_fsf.x_finemoved = 0;
    }
    return (1);
}

static void vslider_set(t_vslider *x, t_floatarg f)
{
    double g;

    if(x->x_gui.x_isa.x_reverse)    /* bugfix */
    {
        if(f > x->x_min)
            f = x->x_min;
        if(f < x->x_max)
            f = x->x_max;
    }
    else
    {
        if(f > x->x_max)
            f = x->x_max;
        if(f < x->x_min)
            f = x->x_min;
    }
    if(x->x_lin0_log1)
        g = log(f/x->x_min)/x->x_k;
    else
        g = (f - x->x_min) / x->x_k;
    x->x_val = (int)(100.0*g + 0.49999);
    x->x_pos = x->x_val;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void vslider_float(t_vslider *x, t_floatarg f)
{
	x->x_is_last_float = 1;
	x->x_last = f;
    vslider_set(x, f);
    if(x->x_gui.x_fsf.x_put_in2out)
        vslider_bang(x);
}

static void vslider_size(t_vslider *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    if(ac > 1)
        vslider_check_height(x, (int)atom_getintarg(1, ac, av));
    iemgui_size((void *)x, &x->x_gui);
}

static void vslider_delta(t_vslider *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void vslider_pos(t_vslider *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void vslider_range(t_vslider *x, t_symbol *s, int ac, t_atom *av)
{
    vslider_check_minmax(x, (double)atom_getfloatarg(0, ac, av),
                         (double)atom_getfloatarg(1, ac, av));
}

static void vslider_color(t_vslider *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void vslider_send(t_vslider *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void vslider_receive(t_vslider *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void vslider_label(t_vslider *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void vslider_label_pos(t_vslider *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void vslider_label_font(t_vslider *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void vslider_log(t_vslider *x)
{
    x->x_lin0_log1 = 1;
    vslider_check_minmax(x, x->x_min, x->x_max);
}

static void vslider_lin(t_vslider *x)
{
    x->x_lin0_log1 = 0;
    x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_h - 1);
}

static void vslider_init(t_vslider *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
}

static void vslider_steady(t_vslider *x, t_floatarg f)
{
    x->x_steady = (f==0.0)?0:1;
}

static void vslider_loadbang(t_vslider *x)
{
    if(!sys_noloadbang && x->x_gui.x_isa.x_loadinit)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        vslider_bang(x);
    }
}

static void *vslider_new(t_symbol *s, int argc, t_atom *argv)
{
    t_vslider *x = (t_vslider *)pd_new(vslider_class);
    int bflcol[]={-262144, -1, -1};
    int w=IEM_GUI_DEFAULTSIZE, h=IEM_SL_DEFAULTSIZE;
    int lilo=0, f=0, ldx=0, ldy=-9;
    int fs=10, v=0, steady=1;
    double min=0.0, max=(double)(IEM_SL_DEFAULTSIZE-1);
    char str[144];

    iem_inttosymargs(&x->x_gui.x_isa, 0);
    iem_inttofstyle(&x->x_gui.x_fsf, 0);

    if(((argc == 17)||(argc == 18))&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
       &&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,3)
       &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
       &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
       &&(IS_A_SYMBOL(argv,7)||IS_A_FLOAT(argv,7))
       &&(IS_A_SYMBOL(argv,8)||IS_A_FLOAT(argv,8))
       &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)
       &&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,13)
       &&IS_A_FLOAT(argv,14)&&IS_A_FLOAT(argv,15)&&IS_A_FLOAT(argv,16))
    {
        w = (int)atom_getintarg(0, argc, argv);
        h = (int)atom_getintarg(1, argc, argv);
        min = (double)atom_getfloatarg(2, argc, argv);
        max = (double)atom_getfloatarg(3, argc, argv);
        lilo = (int)atom_getintarg(4, argc, argv);
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(5, argc, argv));
        iemgui_new_getnames(&x->x_gui, 6, argv);
        ldx = (int)atom_getintarg(9, argc, argv);
        ldy = (int)atom_getintarg(10, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(11, argc, argv));
        fs = (int)atom_getintarg(12, argc, argv);
        bflcol[0] = (int)atom_getintarg(13, argc, argv);
        bflcol[1] = (int)atom_getintarg(14, argc, argv);
        bflcol[2] = (int)atom_getintarg(15, argc, argv);
        v = (int)atom_getintarg(16, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 6, 0);
    if((argc == 18)&&IS_A_FLOAT(argv,17))
        steady = (int)atom_getintarg(17, argc, argv);
    x->x_gui.x_draw = (t_iemfunptr)vslider_draw;
    x->x_gui.x_fsf.x_snd_able = 1;
    x->x_gui.x_fsf.x_rcv_able = 1;
	x->x_is_last_float = 0;
	x->x_last = 0.0;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if(x->x_gui.x_isa.x_loadinit)
        x->x_val = v;
    else
        x->x_val = 0;
    x->x_pos = x->x_val;
    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady != 0) steady = 1;
    x->x_steady = steady;
    if(!strcmp(x->x_gui.x_snd->s_name, "empty")) x->x_gui.x_fsf.x_snd_able = 0;
    if(!strcmp(x->x_gui.x_rcv->s_name, "empty")) x->x_gui.x_fsf.x_rcv_able = 0;
    if(x->x_gui.x_fsf.x_font_style == 1) strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2) strcpy(x->x_gui.x_font, "times");
    else { x->x_gui.x_fsf.x_font_style = 0;
        strcpy(x->x_gui.x_font, sys_font); }
    if(x->x_gui.x_fsf.x_rcv_able) pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = iemgui_clip_size(w);
    vslider_check_height(x, h);
    vslider_check_minmax(x, min, max);
    iemgui_all_colfromload(&x->x_gui, bflcol);
	x->x_thick = 0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    outlet_new(&x->x_gui.x_obj, &s_float);

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

	x->x_gui.x_obj.te_iemgui = 1;

    return (x);
}

static void vslider_free(t_vslider *x)
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

void g_vslider_setup(void)
{
    vslider_class = class_new(gensym("vsl"), (t_newmethod)vslider_new,
                              (t_method)vslider_free, sizeof(t_vslider), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)vslider_new, gensym("vslider"), A_GIMME, 0);
    class_addbang(vslider_class,vslider_bang);
    class_addfloat(vslider_class,vslider_float);
    class_addmethod(vslider_class, (t_method)vslider_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(vslider_class, (t_method)vslider_motion, gensym("motion"),
                    A_FLOAT, A_FLOAT, 0);
    class_addmethod(vslider_class, (t_method)vslider_dialog, gensym("dialog"),
                    A_GIMME, 0);
    class_addmethod(vslider_class, (t_method)vslider_loadbang, gensym("loadbang"), 0);
    class_addmethod(vslider_class, (t_method)vslider_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(vslider_class, (t_method)vslider_size, gensym("size"), A_GIMME, 0);
    class_addmethod(vslider_class, (t_method)vslider_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(vslider_class, (t_method)vslider_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(vslider_class, (t_method)vslider_range, gensym("range"), A_GIMME, 0);
    class_addmethod(vslider_class, (t_method)vslider_color, gensym("color"), A_GIMME, 0);
    class_addmethod(vslider_class, (t_method)vslider_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(vslider_class, (t_method)vslider_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(vslider_class, (t_method)vslider_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(vslider_class, (t_method)vslider_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(vslider_class, (t_method)vslider_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(vslider_class, (t_method)vslider_log, gensym("log"), 0);
    class_addmethod(vslider_class, (t_method)vslider_lin, gensym("lin"), 0);
    class_addmethod(vslider_class, (t_method)vslider_init, gensym("init"), A_FLOAT, 0);
    class_addmethod(vslider_class, (t_method)vslider_steady, gensym("steady"), A_FLOAT, 0);
 
    scalehandle_class = class_new(gensym("_scalehandle"), 0, 0,
				  sizeof(t_scalehandle), CLASS_PD, 0);
    class_addmethod(scalehandle_class, (t_method)vslider__clickhook,
		    gensym("_click"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scalehandle_class, (t_method)vslider__motionhook,
		    gensym("_motion"), A_FLOAT, A_FLOAT, 0);

    vslider_widgetbehavior.w_getrectfn =    vslider_getrect;
    vslider_widgetbehavior.w_displacefn =   iemgui_displace;
    vslider_widgetbehavior.w_selectfn =     iemgui_select;
    vslider_widgetbehavior.w_activatefn =   NULL;
    vslider_widgetbehavior.w_deletefn =     iemgui_delete;
    vslider_widgetbehavior.w_visfn =        iemgui_vis;
    vslider_widgetbehavior.w_clickfn =      vslider_newclick;
	vslider_widgetbehavior.w_displacefnwtag = iemgui_displace_withtag;
    class_setwidget(vslider_class, &vslider_widgetbehavior);
    class_sethelpsymbol(vslider_class, gensym("vslider"));
    class_setsavefn(vslider_class, vslider_save);
    class_setpropertiesfn(vslider_class, vslider_properties);
}
