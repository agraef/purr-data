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
static void vu_draw_select(t_vu* x, t_glist* glist);
void vu_check_height(t_vu *x, int h);

/* ----- vu  gui-peak- & rms- vu-meter-display ---------- */

t_widgetbehavior vu_widgetbehavior;
static t_class *vu_class;

/* widget helper functions */

static void vu_update_rms(t_vu *x, t_glist *glist)
{
    if(glist_isvisible(glist))
    {
        int w4=x->x_gui.x_w/4, off=text_ypix(&x->x_gui.x_obj, glist)-1;
        int xpos=text_xpix(&x->x_gui.x_obj, glist), quad1=xpos+w4+1, quad3=xpos+x->x_gui.x_w-w4-1;

        sys_vgui(".x%lx.c coords %lxRCOVER %d %d %d %d\n",
                 glist_getcanvas(glist), x, quad1+1, off+2, quad3+1,
                 off + (x->x_led_size+1)*(IEM_VU_STEPS-x->x_rms)+2);
    }
}

static void vu_update_peak(t_vu *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    if(glist_isvisible(glist))
    {
        int xpos=text_xpix(&x->x_gui.x_obj, glist);
        int ypos=text_ypix(&x->x_gui.x_obj, glist);

        if(x->x_peak)
        {
            int i=iemgui_vu_col[x->x_peak];
            int j=ypos + (x->x_led_size+1)*(IEM_VU_STEPS+1-x->x_peak)
                - (x->x_led_size+1)/2;

            sys_vgui(".x%lx.c coords %lxPLED %d %d %d %d\n", canvas, x,
                     xpos+1, j+2,
                     xpos+x->x_gui.x_w+2, j+2);
            sys_vgui(".x%lx.c itemconfigure %lxPLED -fill #%6.6x\n", canvas, x,
                     iemgui_color_hex[i]);
        }
        else
        {
            int mid=xpos+x->x_gui.x_w/2;

            sys_vgui(".x%lx.c itemconfigure %lxPLED -fill #%6.6x\n",
                     canvas, x, x->x_gui.x_bcol);
            sys_vgui(".x%lx.c coords %lxPLED %d %d %d %d\n",
                     canvas, x, mid+1, ypos+22,
                     mid+1, ypos+22);
        }
    }
}

static void vu_draw_update(t_gobj *client, t_glist *glist)
{
    t_vu *x = (t_vu *)client;
    if (x->x_updaterms)
    {
        vu_update_rms(x, glist);
        x->x_updaterms = 0;
    }
    if (x->x_updatepeak)
    {
        vu_update_peak(x, glist);
        x->x_updatepeak = 0;
    }
}
    
static void vu_draw_new(t_vu *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int w4=x->x_gui.x_w/4, mid=xpos+x->x_gui.x_w/2,
        quad1=xpos+w4+1;
    int quad3=xpos+x->x_gui.x_w-w4,
        end=xpos+x->x_gui.x_w+4;
    int k1=x->x_led_size+1, k2=IEM_VU_STEPS+1, k3=k1/2;
    int led_col, yyy, i, k4=ypos-k3;

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

		sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%6.6x -tags {%lxBASE %lxVU text}\n",
		         canvas, xpos, ypos,
		         xpos+x->x_gui.x_w+2,
		         ypos+x->x_gui.x_h+4, x->x_gui.x_bcol, x, x);
		for(i=1; i<=IEM_VU_STEPS; i++)
		{
		    led_col = iemgui_vu_col[i];
		    yyy = k4 + k1*(k2-i);
		    sys_vgui(".x%lx.c create line %d %d %d %d -width %d -fill #%6.6x -tags {%lxRLED%d %lxVU text}\n",
		             canvas, quad1+1, yyy+2, quad3+1, yyy+2, x->x_led_size, iemgui_color_hex[led_col], x, i, x);
		    if(((i+2)&3) && (x->x_scale))
		        sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
		                 -font {{%s} -%d %s} -fill #%6.6x -tags {%lxSCALE%d %lxVU text}\n",
		                 canvas, end+1, yyy+k3+2, iemgui_vu_scale_str[i], 
						 x->x_gui.x_font, x->x_gui.x_fontsize,
		                 sys_fontweight, x->x_gui.x_lcol, x, i, x);
		}
		if(x->x_scale)
		{
		    i=IEM_VU_STEPS+1;
		    yyy = k4 + k1*(k2-i);
		    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
		             -font {{%s} -%d %s} -fill #%6.6x -tags {%lxSCALE%d %lxVU text}\n",
		             canvas, end+1, yyy+k3+2, iemgui_vu_scale_str[i], x->x_gui.x_font, 
					 x->x_gui.x_fontsize, sys_fontweight,
		             x->x_gui.x_lcol, x, i, x);
		}
		sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%6.6x -outline #%6.6x -tags {%lxRCOVER %lxVU text}\n",
		         canvas, quad1+1, ypos+1, quad3,
		         ypos+1 + k1*IEM_VU_STEPS, x->x_gui.x_bcol, x->x_gui.x_bcol, x, x);
		sys_vgui(".x%lx.c create line %d %d %d %d -width %d -fill #%6.6x -tags {%lxPLED %lxVU text}\n",
		         canvas, mid+1, ypos+12,
		         mid+1, ypos+12, x->x_led_size, x->x_gui.x_bcol, x, x);
		sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
		         -font {{%s} -%d %s} -fill #%6.6x -tags {%lxLABEL %lxVU text}\n",
		         canvas, xpos+x->x_gui.x_ldx, ypos+x->x_gui.x_ldy,
		         strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"",
		         x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
		         x->x_gui.x_lcol, x, x);
		if(!x->x_gui.x_fsf.x_snd_able)
		{
		    sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags {%so%d %lxVU outlet}\n",
		         canvas,
		         xpos, ypos + x->x_gui.x_h+3,
		         xpos + IOWIDTH, ypos + x->x_gui.x_h+4,
		         nlet_tag, 0, x);
		    sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags {%so%d %lxVU outlet}\n",
		         canvas,
		         xpos+x->x_gui.x_w+2-IOWIDTH, ypos + x->x_gui.x_h+3,
		         xpos+x->x_gui.x_w+2, ypos + x->x_gui.x_h+4,
		         nlet_tag, 1, x);
		}
		if(!x->x_gui.x_fsf.x_rcv_able)
		{
		    sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags {%si%d %lxVU inlet}\n",
		         canvas,
		         xpos, ypos,
		         xpos + IOWIDTH, ypos+1,
		         nlet_tag, 0, x);
		    sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags {%si%d %lxVU inlet}\n",
		         canvas,
		         xpos+x->x_gui.x_w+2-IOWIDTH, ypos,
		         xpos+x->x_gui.x_w+2, ypos+1,
		         nlet_tag, 1, x);
		}
		x->x_updaterms = x->x_updatepeak = 1;
		sys_queuegui(x, x->x_gui.x_glist, vu_draw_update);
	//}
}


static void vu_draw_move(t_vu *x, t_glist *glist)
{
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

		int xpos=text_xpix(&x->x_gui.x_obj, glist);
		int ypos=text_ypix(&x->x_gui.x_obj, glist);
		int w4=x->x_gui.x_w/4, quad1=xpos+w4+1;
		int quad3=xpos+x->x_gui.x_w-w4,
		    end=xpos+x->x_gui.x_w+4;
		int k1=x->x_led_size+1, k2=IEM_VU_STEPS+1, k3=k1/2;
		int yyy, i, k4=ypos-k3;

		sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n",
		         canvas, x, xpos, ypos,
		         xpos+x->x_gui.x_w+2,ypos+x->x_gui.x_h+4);
		for(i=1; i<=IEM_VU_STEPS; i++)
		{
		    yyy = k4 + k1*(k2-i);
		    sys_vgui(".x%lx.c coords %lxRLED%d %d %d %d %d\n",
		             canvas, x, i, quad1+1, yyy+2, quad3+1, yyy+2);
		    if(((i+2)&3) && (x->x_scale))
		        sys_vgui(".x%lx.c coords %lxSCALE%d %d %d\n",
		                 canvas, x, i, end+1, yyy+k3+2);
		}
		if(x->x_scale)
		{
		    i=IEM_VU_STEPS+1;
		    yyy = k4 + k1*(k2-i);
		    sys_vgui(".x%lx.c coords %lxSCALE%d %d %d\n",
		             canvas, x, i, end+1, yyy+k3+2);
		}
		x->x_updaterms = x->x_updatepeak = 1;
		sys_queuegui(x, glist, vu_draw_update);
		sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
		         canvas, x, xpos+x->x_gui.x_ldx,
		         ypos+x->x_gui.x_ldy);
		if(!x->x_gui.x_fsf.x_snd_able)
		{
		    sys_vgui(".x%lx.c coords %so%d %d %d %d %d\n",
		         canvas, nlet_tag, 0,
		         xpos, ypos + x->x_gui.x_h+3,
		         xpos + IOWIDTH, ypos + x->x_gui.x_h+4);
		    sys_vgui(".x%lx.c coords %so%d %d %d %d %d\n",
		         canvas, nlet_tag, 1,
		         xpos+x->x_gui.x_w+2-IOWIDTH, ypos + x->x_gui.x_h+3,
		             xpos+x->x_gui.x_w+2, ypos + x->x_gui.x_h+4);
		}
		if(!x->x_gui.x_fsf.x_rcv_able)
		{
		sys_vgui(".x%lx.c coords %si%d %d %d %d %d\n",
		         canvas, nlet_tag, 0,
		         xpos, ypos,
		         xpos + IOWIDTH, ypos+1);
		sys_vgui(".x%lx.c coords %si%d %d %d %d %d\n",
		         canvas, nlet_tag, 1,
		         xpos+x->x_gui.x_w+2-IOWIDTH, ypos,
		         xpos+x->x_gui.x_w+2, ypos+1);
		}
		/* redraw scale handle rectangle if selected */
		if (x->x_gui.x_fsf.x_selected)
			vu_draw_select(x, x->x_gui.x_glist);
	}
}

static void vu_draw_erase(t_vu* x,t_glist* glist)
{
    int i;
    t_canvas *canvas=glist_getcanvas(glist);

	sys_vgui(".x%lx.c delete %lxVU\n", canvas, x);
	sys_vgui(".x%lx.c dtag all %lxVU\n", canvas, x);
	if (x->x_gui.x_fsf.x_selected) {
		t_scalehandle *sh = (t_scalehandle *)(x->x_gui.x_handle);
		sys_vgui("destroy %s\n", sh->h_pathname);
		t_scalehandle *lh = (t_scalehandle *)(x->x_gui.x_lhandle);
		sys_vgui("destroy %s\n", lh->h_pathname);
	}

/*
    sys_vgui(".x%lx.c delete %lxBASE\n", canvas, x);
    for(i=1; i<=IEM_VU_STEPS; i++)
    {
        sys_vgui(".x%lx.c delete %lxRLED%d\n", canvas, x, i);
        if(((i+2)&3) && (x->x_scale))
            sys_vgui(".x%lx.c delete %lxSCALE%d\n", canvas, x, i);
    }
    if(x->x_scale)
    {
        i=IEM_VU_STEPS+1;
        sys_vgui(".x%lx.c delete %lxSCALE%d\n", canvas, x, i);
    }
    sys_vgui(".x%lx.c delete %lxPLED\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxRCOVER\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxLABEL\n", canvas, x);
    if(!x->x_gui.x_fsf.x_snd_able)
    {
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
        sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 1);
    }
    if(!x->x_gui.x_fsf.x_rcv_able)
    {
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
        sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 1);
    }
*/
}

static void vu_draw_config(t_vu* x, t_glist* glist)
{
    int i;
    t_canvas *canvas=glist_getcanvas(glist);

	/*
	char color[64];
	if (x->x_gui.x_fsf.x_selected)
		sprintf(color, "$select_color");
	else
		sprintf(color, "#%6.6x", x->x_gui.x_lcol);
	*/

    for(i=1; i<=IEM_VU_STEPS; i++)
    {
        sys_vgui(".x%lx.c itemconfigure %lxRLED%d -width %d\n", canvas, x, i,
                 x->x_led_size);
        if(((i+2)&3) && (x->x_scale))
			if (x->x_gui.x_fsf.x_selected)
            	sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -text {%s} -font {{%s} -%d %s} -fill $select_color\n",
                     canvas, x, i, iemgui_vu_scale_str[i], x->x_gui.x_font, 
					 x->x_gui.x_fontsize, sys_fontweight);
			else
            	sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -text {%s} -font {{%s} -%d %s} -fill #%6.6x\n",
                     canvas, x, i, iemgui_vu_scale_str[i], x->x_gui.x_font, 
					 x->x_gui.x_fontsize, sys_fontweight, 
                     x->x_gui.x_lcol);
    }
    if(x->x_scale)
    {
        i=IEM_VU_STEPS+1;
		if (x->x_gui.x_fsf.x_selected)
		    sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -text {%s} -font {{%s} -%d %s} -fill $select_color\n",
		             canvas, x, i, iemgui_vu_scale_str[i], x->x_gui.x_font, 
					 x->x_gui.x_fontsize, sys_fontweight);
		else
		    sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -text {%s} -font {{%s} -%d %s} -fill #%6.6x\n",
		             canvas, x, i, iemgui_vu_scale_str[i], x->x_gui.x_font, 
					 x->x_gui.x_fontsize, sys_fontweight,
		             x->x_gui.x_lcol);
    }
	if (x->x_gui.x_fsf.x_selected)
		sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} -%d %s} -fill $select_color -text {%s} \n",
		         canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
		         strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");
	else
		sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} -%d %s} -fill #%6.6x -text {%s} \n",
		         canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize, sys_fontweight,
		         x->x_gui.x_lcol,
		         strcmp(x->x_gui.x_lab->s_name, "empty")?x->x_gui.x_lab->s_name:"");

    sys_vgui(".x%lx.c itemconfigure %lxRCOVER -fill #%6.6x -outline #%6.6x\n .x%lx.c itemconfigure %lxPLED -width %d\n .x%lx.c itemconfigure %lxBASE -fill #%6.6x\n",
			 canvas, x, x->x_gui.x_bcol, x->x_gui.x_bcol, canvas, x, x->x_led_size,
			 canvas, x, x->x_gui.x_bcol);
    /*
	sys_vgui(".x%lx.c itemconfigure %lxPLED -width %d\n", canvas, x,
             x->x_led_size);
    sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%6.6x\n", canvas, x, x->x_gui.x_bcol);
    */
}

static void vu_draw_io(t_vu* x, t_glist* glist, int old_snd_rcv_flags)
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

		if((old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && !x->x_gui.x_fsf.x_snd_able)
		{
		    sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %so%d\n",
		         canvas,
		         xpos, ypos + x->x_gui.x_h+3,
		         xpos + IOWIDTH, ypos + x->x_gui.x_h+4,
		         nlet_tag, 0);
		    sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %so%d\n",
		         canvas,
		         xpos+x->x_gui.x_w+2-IOWIDTH, ypos + x->x_gui.x_h+3,
		         xpos+x->x_gui.x_w+2, ypos + x->x_gui.x_h+4,
		         nlet_tag, 1);
		}
		if(!(old_snd_rcv_flags & IEM_GUI_OLD_SND_FLAG) && x->x_gui.x_fsf.x_snd_able)
		{
		    sys_vgui(".x%lx.c delete %so%d\n", canvas, nlet_tag, 0);
		    sys_vgui(".x%lx.c delete %so%d\n", canvas, nlet_tag, 1);
		}
		if((old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && !x->x_gui.x_fsf.x_rcv_able)
		{
		    sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %si%d\n",
		         canvas,
		         xpos, ypos,
		         xpos + IOWIDTH, ypos+1,
		         nlet_tag, 0);
		    sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %si%d\n",
		         canvas,
		         xpos+x->x_gui.x_w+2-IOWIDTH, ypos,
		         xpos+x->x_gui.x_w+2, ypos+1,
		         nlet_tag, 1);
		}
		if(!(old_snd_rcv_flags & IEM_GUI_OLD_RCV_FLAG) && x->x_gui.x_fsf.x_rcv_able)
		{
		    sys_vgui(".x%lx.c delete %si%d\n", canvas, nlet_tag, 0);
		    sys_vgui(".x%lx.c delete %si%d\n", canvas, nlet_tag, 1);
		}
	}
}

static void vu_draw_select(t_vu* x,t_glist* glist)
{
    int i;
    t_canvas *canvas=glist_getcanvas(glist);
	t_scalehandle *sh = (t_scalehandle *)(x->x_gui.x_handle);
	t_scalehandle *lh = (t_scalehandle *)(x->x_gui.x_lhandle);

	if (glist_isvisible(canvas)) {

		if(x->x_gui.x_fsf.x_selected)
		{
			// check if we are drawing inside a gop abstraction visible on parent canvas
			// if so, disable highlighting
			if (x->x_gui.x_glist == glist_getcanvas(glist)) {

				sys_vgui(".x%lx.c itemconfigure %lxBASE -outline $select_color\n", canvas, x);
				for(i=1; i<=IEM_VU_STEPS; i++)
				{
				    if(((i+2)&3) && (x->x_scale))
				        sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -fill $select_color\n",
				                 canvas, x, i);
				}
				if(x->x_scale)
				{
				    i=IEM_VU_STEPS+1;
				    sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -fill $select_color\n",
				             canvas, x, i);
				}
				sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill $select_color\n", canvas, x);

				if (x->x_gui.scale_vis)
					sys_vgui("destroy %s\n", sh->h_pathname);

				sys_vgui("canvas %s -width %d -height %d -bg $select_color -bd 0 -cursor bottom_right_corner\n",
					 sh->h_pathname, SCALEHANDLE_WIDTH, SCALEHANDLE_HEIGHT);
				sys_vgui(".x%x.c create window %d %d -anchor nw -width %d -height %d -window %s -tags {%lxSCALE %lxVU}\n",
					 canvas, x->x_gui.x_obj.te_xpix + x->x_gui.x_w + 2 - SCALEHANDLE_WIDTH - 1,
					 x->x_gui.x_obj.te_ypix + x->x_gui.x_h + 4 - SCALEHANDLE_HEIGHT - 1,
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
					sys_vgui(".x%x.c create window %d %d -anchor nw -width %d -height %d -window %s -tags {%lxLABEL %lxVU}\n",
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

			sys_vgui(".x%lx.c addtag selected withtag %lxVU\n", canvas, x);
		}
		else
		{
			sys_vgui(".x%lx.c dtag %lxVU selected\n", canvas, x);
		    sys_vgui(".x%lx.c itemconfigure %lxBASE -outline #%6.6x\n", canvas, x, IEM_GUI_COLOR_NORMAL);
		    for(i=1; i<=IEM_VU_STEPS; i++)
		    {
		        if(((i+2)&3) && (x->x_scale))
		            sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -fill #%6.6x\n",
		                     canvas, x, i, x->x_gui.x_lcol);
		    }
		    if(x->x_scale)
		    {
		        i=IEM_VU_STEPS+1;
		        sys_vgui(".x%lx.c itemconfigure %lxSCALE%d -fill #%6.6x\n",
		                 canvas, x, i, x->x_gui.x_lcol);
		    }
		    sys_vgui(".x%lx.c itemconfigure %lxLABEL -fill #%6.6x\n", canvas, x, x->x_gui.x_lcol);
			sys_vgui("destroy %s\n", sh->h_pathname);
			x->x_gui.scale_vis = 0;
			sys_vgui("destroy %s\n", lh->h_pathname);
			x->x_gui.label_vis = 0;
		}
	}
}

static void vu__clickhook(t_scalehandle *sh, t_floatarg f, t_floatarg xxx, t_floatarg yyy)
{

	t_vu *x = (t_vu *)(sh->h_master);

 	if (xxx) {
 		x->x_gui.scale_offset_x = xxx;
 		x->x_gui.label_offset_x = xxx;
 	}
 	if (yyy) {
 		x->x_gui.scale_offset_y = yyy;
 		x->x_gui.label_offset_y = yyy;
 	}

    int newstate = (int)f;
    if (sh->h_dragon && newstate == 0  && sh->h_scale)
    {
		/* done dragging */

		/* first set up the undo apply */
		canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

		if (sh->h_dragx || sh->h_dragy) {

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
			sys_vgui(".gfxstub%lx.dim.h_ent delete 0 end\n", properties);
			sys_vgui(".gfxstub%lx.dim.h_ent insert 0 %d\n", properties, x->x_gui.x_h);
		}

		if (glist_isvisible(x->x_gui.x_glist))
		{
			sys_vgui(".x%x.c delete %s\n", x->x_gui.x_glist, sh->h_outlinetag);
			vu_check_height(x, x->x_gui.x_h);
			vu_draw_move(x, x->x_gui.x_glist);
			vu_draw_config(x, x->x_gui.x_glist);
			vu_draw_update((t_gobj *)x, x->x_gui.x_glist);
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
					x->x_gui.x_obj.te_xpix + x->x_gui.x_w + 2,
					x->x_gui.x_obj.te_ypix + x->x_gui.x_h + 4, sh->h_outlinetag);
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
			vu_draw_move(x, x->x_gui.x_glist);
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

static void vu__motionhook(t_scalehandle *sh,
				    t_floatarg f1, t_floatarg f2)
{
    if (sh->h_dragon && sh->h_scale)
    {
		t_vu *x = (t_vu *)(sh->h_master);
		int dx = (int)f1, dy = (int)f2;
		int newx, newy;

		int y_incr = (int)((dy - x->x_gui.scale_offset_y) / IEM_VU_STEPS);
		if (dy - x->x_gui.scale_offset_y < 0)
			y_incr -= 1;

		newx = x->x_gui.x_obj.te_xpix + x->x_gui.x_w - x->x_gui.scale_offset_x + dx;
		newy = x->x_gui.x_obj.te_ypix + x->x_gui.x_h + y_incr * IEM_VU_STEPS;

		if (newx < x->x_gui.x_obj.te_xpix + SCALE_BNG_MINWIDTH)
			newx = x->x_gui.x_obj.te_xpix + SCALE_BNG_MINWIDTH;
		if (newy < x->x_gui.x_obj.te_ypix + SCALE_BNG_MINHEIGHT)
			newy = x->x_gui.x_obj.te_ypix + SCALE_BNG_MINHEIGHT;

		if (glist_isvisible(x->x_gui.x_glist)) {
			sys_vgui(".x%x.c coords %s %d %d %d %d\n",
				 x->x_gui.x_glist, sh->h_outlinetag, x->x_gui.x_obj.te_xpix,
				 x->x_gui.x_obj.te_ypix, newx + 2, newy + 4);
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

void vu_draw(t_vu *x, t_glist *glist, int mode)
{
	//fprintf(stderr,"vu_draw %d\n", mode);
    if(mode == IEM_GUI_DRAW_MODE_MOVE)
        vu_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
        vu_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        vu_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        vu_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        vu_draw_config(x, glist);
    else if(mode >= IEM_GUI_DRAW_MODE_IO)
        vu_draw_io(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
}

/* ------------------------ vu widgetbehaviour----------------------------- */


static void vu_getrect(t_gobj *z, t_glist *glist,
                       int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_vu* x = (t_vu*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w + 3;
    *yp2 = *yp1 + x->x_gui.x_h + 6;
}

static void vu_save(t_gobj *z, t_binbuf *b)
{
    t_vu *x = (t_vu *)z;
    int bflcol[3];
    t_symbol *srl[3];

    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiissiiiiiiii", gensym("#X"),gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("vu"), x->x_gui.x_w, x->x_gui.x_h,
                srl[1], srl[2],
                x->x_gui.x_ldx, x->x_gui.x_ldy,
                iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
                bflcol[0], bflcol[2], x->x_scale,
                iem_symargstoint(&x->x_gui.x_isa));
    binbuf_addv(b, ";");
}

void vu_check_height(t_vu *x, int h)
{
    int n;

    n = h / IEM_VU_STEPS;
    if(n < IEM_VU_MINSIZE)
        n = IEM_VU_MINSIZE;
    x->x_led_size = n-1;
    x->x_gui.x_h = IEM_VU_STEPS * n;
}

static void vu_scale(t_vu *x, t_floatarg fscale)
{
	//fprintf(stderr,"vu_scale\n");

    int i, scale = (int)fscale;

    if(scale != 0) scale = 1;
    if(x->x_scale && !scale)
    {
        t_canvas *canvas=glist_getcanvas(x->x_gui.x_glist);

        x->x_scale = (int)scale;
        if(glist_isvisible(x->x_gui.x_glist))
        {
            for(i=1; i<=IEM_VU_STEPS; i++)
            {
                if((i+2)&3)
                    sys_vgui(".x%lx.c delete %lxSCALE%d\n", canvas, x, i);
            }
            i=IEM_VU_STEPS+1;
            sys_vgui(".x%lx.c delete %lxSCALE%d\n", canvas, x, i);
        }
    }
    if(!x->x_scale && scale)
    {
        int w4=x->x_gui.x_w/4, end=text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist)+x->x_gui.x_w+4;
        int k1=x->x_led_size+1, k2=IEM_VU_STEPS+1, k3=k1/2;
        int yyy, k4=text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist)-k3;
        t_canvas *canvas=glist_getcanvas(x->x_gui.x_glist);

        x->x_scale = (int)scale;
        if(glist_isvisible(x->x_gui.x_glist))
        {
            for(i=1; i<=IEM_VU_STEPS; i++)
            {
                yyy = k4 + k1*(k2-i);
                if((i+2)&3)
                    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
                             -font {{%s} -%d %s} -fill #%6.6x -tags {%lxSCALE%d %lxVU}\n",
                             canvas, end+1, yyy+k3+2, iemgui_vu_scale_str[i], 
							 x->x_gui.x_font, x->x_gui.x_fontsize,
                             sys_fontweight, x->x_gui.x_lcol, x, i, x);
            }
            i=IEM_VU_STEPS+1;
            yyy = k4 + k1*(k2-i);
            sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w \
                     -font {{%s} -%d %s} -fill #%6.6x -tags {%lxSCALE%d %lxVU}\n",
                     canvas, end+1, yyy+k3+2, iemgui_vu_scale_str[i], 
					 x->x_gui.x_font, x->x_gui.x_fontsize,
                     sys_fontweight, x->x_gui.x_lcol, x, i, x);
        }
    }
}

static void vu_properties(t_gobj *z, t_glist *owner)
{
    t_vu *x = (t_vu *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s |vu| \
            --------dimensions(pix)(pix):-------- %d %d width: %d %d height: \
            empty 0.0 empty 0.0 empty %d \
            %d no_scale scale %d %d empty %d \
            {%s} {%s} \
            {%s} %d %d \
            %d %d \
            %d %d %d\n",
            x->x_gui.x_w, IEM_GUI_MINSIZE, x->x_gui.x_h, IEM_VU_STEPS*IEM_VU_MINSIZE,
            0,/*no_schedule*/
            x->x_scale, -1, -1, -1,/*no linlog, no init, no multi*/
            "nosndno", srl[1]->s_name,/*no send*/
            srl[2]->s_name, x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, -1/*no front-color*/, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void vu_dialog(t_vu *x, t_symbol *s, int argc, t_atom *argv)
{
	canvas_apply_setundo(x->x_gui.x_glist, (t_gobj *)x);

    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    int scale = (int)atom_getintarg(4, argc, argv);
    int sr_flags;

    srl[0] = gensym("empty");
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    x->x_gui.x_fsf.x_snd_able = 0;
    x->x_gui.x_isa.x_loadinit = 0;
    x->x_gui.x_w = iemgui_clip_size(w);
    vu_check_height(x, h);
    if(scale != 0)
        scale = 1;
    vu_scale(x, (t_float)scale);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);

	/* forcing redraw of the scale handle */
	if (x->x_gui.x_fsf.x_selected) {
		vu_draw_select(x, x->x_gui.x_glist);
	}

	//ico@bukvic.net 100518 update scrollbars when object potentially exceeds window size
    t_canvas *canvas=(t_canvas *)glist_getcanvas(x->x_gui.x_glist);
	sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", (long unsigned int)canvas);
}

static void vu_size(t_vu *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_gui.x_w = iemgui_clip_size((int)atom_getintarg(0, ac, av));
    if(ac > 1)
        vu_check_height(x, (int)atom_getintarg(1, ac, av));
    if(glist_isvisible(x->x_gui.x_glist))
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
        canvas_fixlinesfor(glist_getcanvas(x->x_gui.x_glist), (t_text*)x);
    }
}

static void vu_delta(t_vu *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void vu_pos(t_vu *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}

static void vu_color(t_vu *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}

static void vu_receive(t_vu *x, t_symbol *s)
{iemgui_receive(x, &x->x_gui, s);}

static void vu_label(t_vu *x, t_symbol *s)
{iemgui_label((void *)x, &x->x_gui, s);}

static void vu_label_pos(t_vu *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);}

static void vu_label_font(t_vu *x, t_symbol *s, int ac, t_atom *av)
{iemgui_label_font((void *)x, &x->x_gui, s, ac, av);}

static void vu_float(t_vu *x, t_floatarg rms)
{
    int i;

    if(rms <= IEM_VU_MINDB)
        x->x_rms = 0;
    else if(rms >= IEM_VU_MAXDB)
        x->x_rms = IEM_VU_STEPS;
    else
    {
        int i = (int)(2.0*(rms + IEM_VU_OFFSET));
        x->x_rms = iemgui_vu_db2i[i];
    }
    i = (int)(100.0*rms + 10000.5);
    rms = 0.01*(t_float)(i - 10000);
    x->x_fr = rms;
    outlet_float(x->x_out_rms, rms);
    x->x_updaterms = 1;
    sys_queuegui(x, x->x_gui.x_glist, vu_draw_update);
}

static void vu_ft1(t_vu *x, t_floatarg peak)
{
    int i;

    if(peak <= IEM_VU_MINDB)
        x->x_peak = 0;
    else if(peak >= IEM_VU_MAXDB)
        x->x_peak = IEM_VU_STEPS;
    else
    {
        int i = (int)(2.0*(peak + IEM_VU_OFFSET));
        x->x_peak = iemgui_vu_db2i[i];
    }
    i = (int)(100.0*peak + 10000.5);
    peak = 0.01*(t_float)(i - 10000);
    x->x_fp = peak;
    x->x_updatepeak = 1;
    sys_queuegui(x, x->x_gui.x_glist, vu_draw_update);
    outlet_float(x->x_out_peak, peak);
}

static void vu_bang(t_vu *x)
{
    outlet_float(x->x_out_peak, x->x_fp);
    outlet_float(x->x_out_rms, x->x_fr);
    x->x_updaterms = x->x_updatepeak = 1;
    sys_queuegui(x, x->x_gui.x_glist, vu_draw_update);
}

static void *vu_new(t_symbol *s, int argc, t_atom *argv)
{
    t_vu *x = (t_vu *)pd_new(vu_class);
    int bflcol[]={-66577, -1, -1};
    int w=IEM_GUI_DEFAULTSIZE, h=IEM_VU_STEPS*IEM_VU_DEFAULTSIZE;
    int ldx=-1, ldy=-8, f=0, fs=10, scale=1;
    int ftbreak=IEM_BNG_DEFAULTBREAKFLASHTIME, fthold=IEM_BNG_DEFAULTHOLDFLASHTIME;
    char str[144];

    iem_inttosymargs(&x->x_gui.x_isa, 0);
    iem_inttofstyle(&x->x_gui.x_fsf, 0);

    if((argc >= 11)&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
       &&(IS_A_SYMBOL(argv,2)||IS_A_FLOAT(argv,2))
       &&(IS_A_SYMBOL(argv,3)||IS_A_FLOAT(argv,3))
       &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
       &&IS_A_FLOAT(argv,6)&&IS_A_FLOAT(argv,7)
       &&IS_A_FLOAT(argv,8)&&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10))
    {
        w = (int)atom_getintarg(0, argc, argv);
        h = (int)atom_getintarg(1, argc, argv);
        iemgui_new_getnames(&x->x_gui, 1, argv);
        ldx = (int)atom_getintarg(4, argc, argv);
        ldy = (int)atom_getintarg(5, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(6, argc, argv));
        fs = (int)atom_getintarg(7, argc, argv);
        bflcol[0] = (int)atom_getintarg(8, argc, argv);
        bflcol[2] = (int)atom_getintarg(9, argc, argv);
        scale = (int)atom_getintarg(10, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 1, 0);
    if((argc == 12)&&IS_A_FLOAT(argv,11))
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(11, argc, argv));
    x->x_gui.x_draw = (t_iemfunptr)vu_draw;

    x->x_gui.x_fsf.x_snd_able = 0;
    x->x_gui.x_fsf.x_rcv_able = 1;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    if (!strcmp(x->x_gui.x_rcv->s_name, "empty"))
        x->x_gui.x_fsf.x_rcv_able = 0;
    if (x->x_gui.x_fsf.x_font_style == 1)
        strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2)
        strcpy(x->x_gui.x_font, "times");
    else { x->x_gui.x_fsf.x_font_style = 0;
        strcpy(x->x_gui.x_font, sys_font); }
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;

    if(fs < 4)
        fs = 4;
    x->x_gui.x_fontsize = fs;
    x->x_gui.x_w = iemgui_clip_size(w);
    vu_check_height(x, h);
    iemgui_all_colfromload(&x->x_gui, bflcol);
    if(scale != 0)
        scale = 1;
    x->x_scale = scale;
    x->x_peak = 0;
    x->x_rms = 0;
    x->x_fp = -101.0;
    x->x_fr = -101.0;
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    inlet_new(&x->x_gui.x_obj, &x->x_gui.x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_out_rms = outlet_new(&x->x_gui.x_obj, &s_float);
    x->x_out_peak = outlet_new(&x->x_gui.x_obj, &s_float);

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

static void vu_free(t_vu *x)
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

void g_vumeter_setup(void)
{
    vu_class = class_new(gensym("vu"), (t_newmethod)vu_new, (t_method)vu_free,
                         sizeof(t_vu), 0, A_GIMME, 0);
    class_addbang(vu_class,vu_bang);
    class_addfloat(vu_class,vu_float);
    class_addmethod(vu_class, (t_method)vu_ft1, gensym("ft1"), A_FLOAT, 0);
    class_addmethod(vu_class, (t_method)vu_dialog, gensym("dialog"),
                    A_GIMME, 0);
    class_addmethod(vu_class, (t_method)vu_size, gensym("size"), A_GIMME, 0);
    class_addmethod(vu_class, (t_method)vu_scale, gensym("scale"), A_DEFFLOAT, 0);
    class_addmethod(vu_class, (t_method)vu_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(vu_class, (t_method)vu_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(vu_class, (t_method)vu_color, gensym("color"), A_GIMME, 0);
    class_addmethod(vu_class, (t_method)vu_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(vu_class, (t_method)vu_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(vu_class, (t_method)vu_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(vu_class, (t_method)vu_label_font, gensym("label_font"), A_GIMME, 0);
 
    scalehandle_class = class_new(gensym("_scalehandle"), 0, 0,
				  sizeof(t_scalehandle), CLASS_PD, 0);
    class_addmethod(scalehandle_class, (t_method)vu__clickhook,
		    gensym("_click"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scalehandle_class, (t_method)vu__motionhook,
		    gensym("_motion"), A_FLOAT, A_FLOAT, 0);

    vu_widgetbehavior.w_getrectfn =    vu_getrect;
    vu_widgetbehavior.w_displacefn =   iemgui_displace;
    vu_widgetbehavior.w_selectfn =     iemgui_select;
    vu_widgetbehavior.w_activatefn =   NULL;
    vu_widgetbehavior.w_deletefn =     iemgui_delete;
    vu_widgetbehavior.w_visfn =        iemgui_vis;
    vu_widgetbehavior.w_clickfn =      NULL;
	vu_widgetbehavior.w_displacefnwtag = iemgui_displace_withtag;
    class_setwidget(vu_class,&vu_widgetbehavior);
    class_sethelpsymbol(vu_class, gensym("vu"));
    class_setsavefn(vu_class, vu_save);
    class_setpropertiesfn(vu_class, vu_properties);
}
