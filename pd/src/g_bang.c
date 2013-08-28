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
void bng_draw_select(t_bng* x, t_glist* glist);
 
/* --------------- bng     gui-bang ------------------------- */

t_widgetbehavior bng_widgetbehavior;
static t_class *bng_class;

/*  widget helper functions  */

void bng_draw_update(t_bng *x, t_glist *glist)
{
    if(glist_isvisible(glist))
    {
        sys_vgui(".x%lx.c itemconfigure %lxBUT -fill #%6.6x\n", glist_getcanvas(glist), x,
                 x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol);
    }
}

void bng_draw_new(t_bng *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

	t_scalehandle *sh = (t_scalehandle *)x->x_gui.x_handle;
	sprintf(sh->h_pathname, ".x%lx.h%lx", (t_int)canvas, (t_int)sh);

	t_scalehandle *lh = (t_scalehandle *)x->x_gui.x_lhandle;
	sprintf(lh->h_pathname, ".x%lx.h%lx", (t_int)canvas, (t_int)lh);

	//if (glist_isvisible(canvas)) {

		t_gobj *y = (t_gobj *)x;
		t_object *ob = pd_checkobject(&y->g_pd);

		/* GOP objects are unable to call findrtext triggering consistency check error */
		t_rtext *yyyy = NULL;
		if (!glist->gl_isgraph || glist_istoplevel(glist))
			yyyy = glist_findrtext(canvas, (t_text *)&ob->ob_g);

		/* on GOP we cause segfault as apparently text_gettag() returns bogus data */
		char *nlet_tag;
		if (yyyy) nlet_tag = rtext_gettag(yyyy);
		else nlet_tag = "bogus";


		sys_vgui(".x%lx.c create prect %d %d %d %d -fill #%6.6x -tags {%lxBASE %lxBNG %lx text}\n",
		         canvas, xpos, ypos,
		         xpos + x->x_gui.x_w, ypos + x->x_gui.x_h,
		         x->x_gui.x_bcol, x, x, x);
		int cr = (x->x_gui.x_w-(x->x_gui.x_w % 2 ? 0 : 1))/2;
		int cx = xpos+1+cr;
		int cy = ypos+1+cr;
		sys_vgui(".x%lx.c create circle %d %d -r %d -fill #%6.6x -tags {%lxBUT %lxBNG %lx text}\n",
		         canvas, cx, cy, cr,
		         x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol, x, x, x);
		sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
		         -font {{%s} -%d %s} -fill #%6.6x -tags {%lxLABEL %lxBNG %lx text}\n",
		         canvas, xpos+x->x_gui.x_ldx,
		         ypos+x->x_gui.x_ldy,
		         strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
		         x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
				 x->x_gui.x_lcol, x, x, x);
		if(!x->x_gui.x_fsf.x_snd_able && canvas == x->x_gui.x_glist) {
		    sys_vgui(".x%lx.c create prect %d %d %d %d -tags {%lxBNG%so%d %so%d %lxBNG %lx outlet}\n",
		         canvas, xpos,
		         ypos + x->x_gui.x_h-1, xpos + IOWIDTH,
		         ypos + x->x_gui.x_h, x, nlet_tag, 0, nlet_tag, 0, x, x);
		}
		if(!x->x_gui.x_fsf.x_rcv_able && canvas == x->x_gui.x_glist) {
		    sys_vgui(".x%lx.c create prect %d %d %d %d -tags {%lxBNG%si%d %si%d %lxBNG %lx inlet}\n",
		         canvas, xpos, ypos,
		         xpos + IOWIDTH, ypos+1, x, nlet_tag, 0, nlet_tag, 0, x, x);
		}
	//}
}

void bng_draw_move(t_bng *x, t_glist *glist)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

	if (glist_isvisible(canvas)) {

		t_gobj *y = (t_gobj *)x;
		t_object *ob = pd_checkobject(&y->g_pd);

		/* GOP objects are unable to call findrtext triggering consistency check error */
		t_rtext *yyyy = NULL;
		if (!glist->gl_isgraph || glist_istoplevel(glist))
			yyyy = glist_findrtext(canvas, (t_text *)&ob->ob_g);

		/* on GOP we cause segfault as apparently text_gettag() returns bogus data */
		char *nlet_tag;
		if (yyyy) nlet_tag = rtext_gettag(yyyy);
		else nlet_tag = "bogus";

		sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n",
		         canvas, x, xpos, ypos,
		         xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);
		int cr = (x->x_gui.x_w-(x->x_gui.x_w % 2 ? 0 : 1))/2;
		int cx = xpos+1+cr;
		int cy = ypos+1+cr;
		/*sys_vgui(".x%lx.c create circle %d %d -r %d -stroke #%6.6x -tags {%lxBUT %lxBNG %lx text}\n",
		         canvas, cx, cy, cr,*/
		sys_vgui(".x%lx.c coords %lxBUT %d %d\n",
		         canvas, x, cx, cy);
		sys_vgui(".x%lx.c itemconfigure %lxBUT -fill #%6.6x -r %d\n", canvas, x,
		         x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol, cr);
		sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
		         canvas, x, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy);
		if(!x->x_gui.x_fsf.x_snd_able && canvas == x->x_gui.x_glist)
		    sys_vgui(".x%lx.c coords %lxBNG%so%d %d %d %d %d\n",
		         canvas, x, nlet_tag, 0, xpos,
		         ypos + x->x_gui.x_h-1, xpos + IOWIDTH,
		         ypos + x->x_gui.x_h);
		if(!x->x_gui.x_fsf.x_rcv_able && canvas == x->x_gui.x_glist)
		    sys_vgui(".x%lx.c coords %lxBNG%si%d %d %d %d %d\n",
		         canvas, x, nlet_tag, 0, xpos, ypos,
		         xpos + IOWIDTH, ypos+1);
		/* redraw scale handle rectangle if selected */
		if (x->x_gui.x_fsf.x_selected)
			bng_draw_select(x, x->x_gui.x_glist);
	}
}

void bng_draw_erase(t_bng* x, t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
	sys_vgui(".x%lx.c delete %lxBNG\n", canvas, x);
	sys_vgui(".x%lx.c dtag all %lxBNG\n", canvas, x);
	if (x->x_gui.x_fsf.x_selected) {
		t_scalehandle *sh = (t_scalehandle *)(x->x_gui.x_handle);
		sys_vgui("destroy %s\n", sh->h_pathname);
		sys_vgui(".x%lx.c delete %lxSCALE\n", canvas, x);
		t_scalehandle *lh = (t_scalehandle *)(x->x_gui.x_lhandle);
		sys_vgui("destroy %s\n", lh->h_pathname);
		sys_vgui(".x%lx.c delete %lxLABELH\n", canvas, x);
	}
/*
    sys_vgui(".x%lx.c delete %lxBASE\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxBUT\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxLABEL\n", canvas, x);
    if(!x->x_gui.x_fsf.x_snd_able)
        sys_vgui(".x%lx.c delete %so%d\n", canvas, rtext_gettag(yy), 0);
    if(!x->x_gui.x_fsf.x_rcv_able)
        sys_vgui(".x%lx.c delete %si%d\n", canvas, rtext_gettag(yy), 0);
*/
}

void bng_draw_config(t_bng* x, t_glist* glist)
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
    sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%6.6x\n .x%lx.c itemconfigure %lxBUT -fill #%6.6x\n",
			 canvas, x, x->x_gui.x_bcol, canvas, x,
             x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol);
    /*sys_vgui(".x%lx.c itemconfigure %lxBUT -fill #%6.6x\n", canvas, x,
             x->x_flashed?x->x_gui.x_fcol:x->x_gui.x_bcol);*/
}

void bng_draw_io(t_bng* x, t_glist* glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    t_canvas *canvas=glist_getcanvas(glist);

	if (glist_isvisible(canvas) && canvas == x->x_gui.x_glist) {

		t_gobj *y = (t_gobj *)x;
		t_object *ob = pd_checkobject(&y->g_pd);

		/* GOP objects are unable to call findrtext triggering consistency check error */
		t_rtext *yyyy = NULL;
		if (!glist->gl_isgraph || glist_istoplevel(glist))
			yyyy = glist_findrtext(canvas, (t_text *)&ob->ob_g);

		/* on GOP we cause segfault as apparently text_gettag() returns bogus data */
		char *nlet_tag;
		if (yyyy) nlet_tag = rtext_gettag(yyyy);
		else nlet_tag = "bogus";

		if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
		    sys_vgui(".x%lx.c create prect %d %d %d %d -tags {%lxBNG%so%d %so%d %lxBNG %lx outlet}\n",
		         canvas, xpos,
		         ypos + x->x_gui.x_h-1, xpos + IOWIDTH,
		         ypos + x->x_gui.x_h, x, nlet_tag, 0, nlet_tag, 0, x, x);
		if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
		    sys_vgui(".x%lx.c delete %lxBNG%so%d\n", canvas, x, nlet_tag, 0);
		if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
		    sys_vgui(".x%lx.c create prect %d %d %d %d -tags {%lxBNG%si%d %si%d %lxBNG %lx inlet}\n",
		         canvas, xpos, ypos,
		         xpos + IOWIDTH, ypos+1, x, nlet_tag, 0, nlet_tag, 0, x, x);
		if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
		    sys_vgui(".x%lx.c delete %lxBNG%si%d\n", canvas, x, nlet_tag, 0);
	}
}

void bng_draw_select(t_bng* x, t_glist* glist)
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

				sys_vgui(".x%lx.c itemconfigure %lxBASE -stroke $select_color\n", canvas, x);
				sys_vgui(".x%lx.c itemconfigure %lxBUT -stroke $select_color\n", canvas, x);
				sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill $select_color\n", canvas, x);

				if (x->x_gui.scale_vis) {
					sys_vgui("destroy %s\n", sh->h_pathname);
					sys_vgui(".x%lx.c delete %lxSCALE\n", canvas, x);
				}

				sys_vgui("canvas %s -width %d -height %d -bg $select_color -bd 0 -cursor bottom_right_corner\n",
					 sh->h_pathname, SCALEHANDLE_WIDTH, SCALEHANDLE_HEIGHT);
				sys_vgui(".x%x.c create window %d %d -anchor nw -width %d -height %d -window %s -tags {%lxSCALE %lxBNG %lx}\n",
					 canvas, x->x_gui.x_obj.te_xpix + x->x_gui.x_w - SCALEHANDLE_WIDTH - 1,
					 x->x_gui.x_obj.te_ypix + x->x_gui.x_h - SCALEHANDLE_HEIGHT - 1,
					 SCALEHANDLE_WIDTH, SCALEHANDLE_HEIGHT,
					 sh->h_pathname, x, x, x);
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
					sys_vgui(".x%x.c create window %d %d -anchor nw -width %d -height %d -window %s -tags {%lxLABEL %lxLABELH %lxBNG %lx}\n",
						canvas, x->x_gui.x_obj.te_xpix+ x->x_gui.x_ldx - LABELHANDLE_WIDTH,
						x->x_gui.x_obj.te_ypix + x->x_gui.x_ldy - LABELHANDLE_HEIGHT,
						LABELHANDLE_WIDTH, LABELHANDLE_HEIGHT,
						lh->h_pathname, x, x, x, x);
					sys_vgui("bind %s <Button> {pd [concat %s _click 1 %%x %%y \\;]}\n",
						lh->h_pathname, lh->h_bindsym->s_name);
					sys_vgui("bind %s <ButtonRelease> {pd [concat %s _click 0 0 0 \\;]}\n",
						lh->h_pathname, lh->h_bindsym->s_name);
					sys_vgui("bind %s <Motion> {pd [concat %s _motion %%x %%y \\;]}\n",
						lh->h_pathname, lh->h_bindsym->s_name); 
					x->x_gui.label_vis = 1;
				}
			}

			sys_vgui(".x%lx.c addtag selected withtag %lxBNG\n", canvas, x);
		}
		else
		{
		    sys_vgui(".x%lx.c itemconfigure %lxBASE -stroke #%6.6x\n", canvas, x, IEM_GUI_COLOR_NORMAL);
		    sys_vgui(".x%lx.c itemconfigure %lxBUT -stroke #%6.6x\n", canvas, x, IEM_GUI_COLOR_NORMAL);
		    sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n", canvas, x, x->x_gui.x_lcol);
			sys_vgui(".x%lx.c dtag %lxBNG selected\n", canvas, x);
			sys_vgui("destroy %s\n", sh->h_pathname);
			sys_vgui(".x%lx.c delete %lxSCALE\n", canvas, x);
			x->x_gui.scale_vis = 0;
			sys_vgui("destroy %s\n", lh->h_pathname);
			sys_vgui(".x%lx.c delete %lxLABELH\n", canvas, x);
			x->x_gui.label_vis = 0;
		}
	//}
}

static void bng__clickhook(t_scalehandle *sh, t_floatarg f, t_floatarg xxx, t_floatarg yyy)
{

	t_bng *x = (t_bng *)(sh->h_master);

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

			if (sh->h_dragx > sh->h_dragy)
				sh->h_dragx = sh->h_dragy;
			else sh->h_dragy = sh->h_dragx;

			x->x_gui.x_w = x->x_gui.x_w + sh->h_dragx - x->x_gui.scale_offset_x;
			if (x->x_gui.x_w < SCALE_BNG_MINWIDTH)
				x->x_gui.x_w = SCALE_BNG_MINWIDTH;
			x->x_gui.x_h = x->x_gui.x_h + sh->h_dragy - x->x_gui.scale_offset_y;
			if (x->x_gui.x_h < SCALE_BNG_MINHEIGHT)
				x->x_gui.x_h = SCALE_BNG_MINHEIGHT;

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
			bng_draw_move(x, x->x_gui.x_glist);
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
					x->x_gui.x_obj.te_ypix + x->x_gui.x_h, sh->h_outlinetag);
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
			bng_draw_move(x, x->x_gui.x_glist);
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

static void bng__motionhook(t_scalehandle *sh,
				    t_floatarg f1, t_floatarg f2)
{
    if (sh->h_dragon && sh->h_scale)
    {
		t_bng *x = (t_bng *)(sh->h_master);
		int dx = (int)f1, dy = (int)f2;
		int newx, newy;

		if (dx > dy) {
			dx = dy;
			x->x_gui.scale_offset_x = x->x_gui.scale_offset_y;
		}
		else {
			dy = dx;
			x->x_gui.scale_offset_y = x->x_gui.scale_offset_x;
		}

		newx = x->x_gui.x_obj.te_xpix + x->x_gui.x_w - x->x_gui.scale_offset_x + dx;
		newy = x->x_gui.x_obj.te_ypix + x->x_gui.x_h - x->x_gui.scale_offset_y + dy;

		if (newx < x->x_gui.x_obj.te_xpix + SCALE_BNG_MINWIDTH)
			newx = x->x_gui.x_obj.te_xpix + SCALE_BNG_MINWIDTH;
		if (newy < x->x_gui.x_obj.te_ypix + SCALE_BNG_MINHEIGHT)
			newy = x->x_gui.x_obj.te_ypix + SCALE_BNG_MINHEIGHT;

		if (glist_isvisible(x->x_gui.x_glist)) {
			sys_vgui(".x%x.c coords %s %d %d %d %d\n",
				 x->x_gui.x_glist, sh->h_outlinetag, x->x_gui.x_obj.te_xpix,
				 x->x_gui.x_obj.te_ypix, newx, newy);
		}
		sh->h_dragx = dx;
		sh->h_dragy = dy;

		int properties = gfxstub_haveproperties((void *)x);

		if (properties) {
			int new_w = x->x_gui.x_w - x->x_gui.scale_offset_x + sh->h_dragx;
			int new_h = x->x_gui.x_h - x->x_gui.scale_offset_y + sh->h_dragy;
			sys_vgui(".gfxstub%lx.dim.w_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.dim.w_ent insert 0 %d\n", properties, new_w);
			//sys_vgui(".gfxstub%lx.dim.h_ent delete 0 end\n", properties);
			//sys_vgui(".gfxstub%lx.dim.h_ent insert 0 %d\n", properties, new_h);
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

void bng_draw(t_bng *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        bng_draw_update(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        bng_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW) {
        bng_draw_new(x, glist);
		sys_vgui(".x%lx.c raise all_cords\n", glist_getcanvas(glist));
	}
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        bng_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        bng_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        bng_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        bng_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ bng widgetbehaviour----------------------------- */

static void bng_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2)
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
                iem_symargstoint(&x->x_gui.x_isa),
                srl[0], srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
                bflcol[0], bflcol[1], bflcol[2]);
    binbuf_addv(b, ";");
}

void bng_check_minmax(t_bng *x, int ftbreak, int fthold)
{
    if(ftbreak > fthold)
    {
        int h;

        h = ftbreak;
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
            -1, x->x_gui.x_isa.x_loadinit, -1, -1,/*no linlog, no multi*/
            srl[0]->s_name, srl[1]->s_name,
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
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
    if(!x->x_gui.x_fsf.x_put_in2out)
    {
        x->x_gui.x_isa.x_locked = 1;
        clock_delay(x->x_clock_lck, 2);
    }
    outlet_bang(x->x_gui.x_obj.ob_outlet);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing && x->x_gui.x_fsf.x_put_in2out)
        pd_bang(x->x_gui.x_snd->s_thing);
}

static void bng_bout2(t_bng *x)/*wird immer gesendet, wenn moeglich*/
{
    if(!x->x_gui.x_fsf.x_put_in2out)
    {
        x->x_gui.x_isa.x_locked = 1;
        clock_delay(x->x_clock_lck, 2);
    }
    outlet_bang(x->x_gui.x_obj.ob_outlet);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
        pd_bang(x->x_gui.x_snd->s_thing);
}

static void bng_bang(t_bng *x)/*wird nur mehr gesendet, wenn snd != rcv*/
{
    if(!x->x_gui.x_isa.x_locked)
    {
        bng_set(x);
        bng_bout1(x);
    }
}

static void bng_bang2(t_bng *x)/*wird immer gesendet, wenn moeglich*/
{
    if(!x->x_gui.x_isa.x_locked)
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
	iemgui_shouldvis((void *)x, &x->x_gui, IEM_GUI_DRAW_MODE_MOVE);

	/* forcing redraw of the scale handle */
	if (x->x_gui.x_fsf.x_selected) {
		bng_draw_select(x, x->x_gui.x_glist);
	}

	//ico@bukvic.net 100518 update scrollbars when object potentially exceeds window size
    t_canvas *canvas=(t_canvas *)glist_getcanvas(x->x_gui.x_glist);
	sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", (long unsigned int)canvas);
}

static void bng_click(t_bng *x, t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    bng_set(x);
    bng_bout2(x);
}

static int bng_newclick(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    if(doit)
        bng_click((t_bng *)z, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift, 0, (t_floatarg)alt);
    return (1);
}

static void bng_float(t_bng *x, t_floatarg f)
{bng_bang2(x);}

static void bng_symbol(t_bng *x, t_symbol *s)
{bng_bang2(x);}

static void bng_pointer(t_bng *x, t_gpointer *gp)
{bng_bang2(x);}

static void bng_list(t_bng *x, t_symbol *s, int ac, t_atom *av)
{
    bng_bang2(x);
}

static void bng_anything(t_bng *x, t_symbol *s, int argc, t_atom *argv)
{bng_bang2(x);}

static void bng_loadbang(t_bng *x)
{
    if(!sys_noloadbang && x->x_gui.x_isa.x_loadinit)
    {
        bng_set(x);
        bng_bout2(x);
    }
}

static void bng_size(t_bng *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    x->x_gui.x_h = x->x_gui.x_w;
    iemgui_size((void *)x, &x->x_gui);
}

static void bng_delta(t_bng *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void bng_pos(t_bng *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void bng_flashtime(t_bng *x, t_symbol *s, int ac, t_atom *av)
{
    bng_check_minmax(x, (int)atom_getintarg(0, ac, av),
                     (int)atom_getintarg(1, ac, av));
}

static void bng_color(t_bng *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void bng_send(t_bng *x, t_symbol *s)
{iemgui_send(x, &x->x_gui, s);}

static void bng_receive(t_bng *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void bng_label(t_bng *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void bng_label_pos(t_bng *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void bng_label_font(t_bng *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void bng_init(t_bng *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
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
    x->x_gui.x_isa.x_locked = 0;
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
    char str[144];

    iem_inttosymargs(&x->x_gui.x_isa, 0);
    iem_inttofstyle(&x->x_gui.x_fsf, 0);

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
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(3, argc, argv));
        iemgui_new_getnames(&x->x_gui, 4, argv);
        ldx = (int)atom_getintarg(7, argc, argv);
        ldy = (int)atom_getintarg(8, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(9, argc, argv));
        fs = (int)atom_getintarg(10, argc, argv);
        bflcol[0] = (int)atom_getintarg(11, argc, argv);
        bflcol[1] = (int)atom_getintarg(12, argc, argv);
        bflcol[2] = (int)atom_getintarg(13, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 4, 0);

    x->x_gui.x_draw = (t_iemfunptr)bng_draw;

    x->x_gui.x_fsf.x_snd_able = 1;
    x->x_gui.x_fsf.x_rcv_able = 1;
    x->x_flashed = 0;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if (!strcmp(x->x_gui.x_snd->s_name, "empty"))
        x->x_gui.x_fsf.x_snd_able = 0;
    if (!strcmp(x->x_gui.x_rcv->s_name, "empty"))
        x->x_gui.x_fsf.x_rcv_able = 0;
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
    x->x_gui.x_w = iemgui_clip_size(a);
    x->x_gui.x_h = x->x_gui.x_w;
    bng_check_minmax(x, ftbreak, fthold);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    x->x_gui.x_isa.x_locked = 0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    x->x_clock_hld = clock_new(x, (t_method)bng_tick_hld);
    x->x_clock_brk = clock_new(x, (t_method)bng_tick_brk);
    x->x_clock_lck = clock_new(x, (t_method)bng_tick_lck);
    outlet_new(&x->x_gui.x_obj, &s_bang);

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

static void bng_ff(t_bng *x)
{
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    clock_free(x->x_clock_lck);
    clock_free(x->x_clock_brk);
    clock_free(x->x_clock_hld);
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
    class_addmethod(bng_class, (t_method)bng_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(bng_class, (t_method)bng_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(bng_class, (t_method)bng_flashtime, gensym("flashtime"), A_GIMME, 0);
    class_addmethod(bng_class, (t_method)bng_color, gensym("color"), A_GIMME, 0);
    class_addmethod(bng_class, (t_method)bng_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(bng_class, (t_method)bng_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(bng_class, (t_method)bng_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(bng_class, (t_method)bng_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(bng_class, (t_method)bng_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(bng_class, (t_method)bng_init, gensym("init"), A_FLOAT, 0);
 
    scalehandle_class = class_new(gensym("_scalehandle"), 0, 0,
				  sizeof(t_scalehandle), CLASS_PD, 0);
    class_addmethod(scalehandle_class, (t_method)bng__clickhook,
		    gensym("_click"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scalehandle_class, (t_method)bng__motionhook,
		    gensym("_motion"), A_FLOAT, A_FLOAT, 0);

    bng_widgetbehavior.w_getrectfn = bng_getrect;
    bng_widgetbehavior.w_displacefn = iemgui_displace;
    bng_widgetbehavior.w_selectfn = iemgui_select;
    bng_widgetbehavior.w_activatefn = NULL;
    bng_widgetbehavior.w_deletefn = iemgui_delete;
    bng_widgetbehavior.w_visfn = iemgui_vis;
    bng_widgetbehavior.w_clickfn = bng_newclick;
    bng_widgetbehavior.w_displacefnwtag = iemgui_displace_withtag;
    class_setwidget(bng_class, &bng_widgetbehavior);
    class_sethelpsymbol(bng_class, gensym("bng"));
    class_setsavefn(bng_class, bng_save);
    class_setpropertiesfn(bng_class, bng_properties);
}
