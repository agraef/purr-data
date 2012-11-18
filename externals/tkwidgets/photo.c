/* [photo] object for dislaying images in a patch

   Copyright (C) 2002-2004 Guenter Geiger
   Copyright (C) 2007 Hans-Christoph Steiner <hans@at.or.at>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   This is part of the tkwidgets library for Pd.

*/

#include "tkwidgets.h"

/* ------------------------ photo ----------------------------- */

static t_class *photo_class;

typedef struct _photo
{
    t_object     x_obj;
    t_glist*     x_glist;
    t_symbol*    receive_name;
    int          x_width;
    int          x_height;
    t_symbol*    filename;
} t_photo;

static char *photo_tk_options[] = {
    "activeimage",
    "disabledimage",
    "gamma",
    "image",
    "state"
};

/* widget helper functions */

    void photo_drawme(t_photo *x, t_glist *glist, int firsttime)
{
	if (firsttime) 
    {
		char fname[MAXPDSTRING];
		canvas_makefilename(glist_getcanvas(x->x_glist), x->filename->s_name,
							fname, MAXPDSTRING);

        sys_vgui("image create photo img%x -file {%s}\n", x, fname);
        sys_vgui(".x%x.c create image %d %d -image img%x -anchor nw -tags %xS\n", 
                 glist_getcanvas(glist),text_xpix(&x->x_obj, glist), 
                 text_ypix(&x->x_obj, glist),x,x);

        /* TODO callback from gui
           sys_vgui("photo_size logo");
        */
    }     
    else 
    {
        sys_vgui(".x%x.c coords %xS %d %d\n",
                 glist_getcanvas(glist), x,
                 text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist));
    }
	
}


void photo_erase(t_photo* x,t_glist* glist)
{
    int n;
    sys_vgui(".x%x.c delete %xS\n",
             glist_getcanvas(glist), x);

}
	


/* ------------------------ photo widgetbehaviour----------------------------- */


static void photo_getrect(t_gobj *z, t_glist *glist,
                          int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_photo* x = (t_photo*)z;

    *xp1 = text_xpix(&x->x_obj, glist);
    *yp1 = text_ypix(&x->x_obj, glist);
    *xp2 = text_xpix(&x->x_obj, glist) + x->x_width;
    *yp2 = text_ypix(&x->x_obj, glist) + x->x_height;
}

static void photo_displace(t_gobj *z, t_glist *glist,
                           int dx, int dy)
{
    t_photo *x = (t_photo *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
             glist_getcanvas(glist), x,
             text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
             text_xpix(&x->x_obj, glist) + x->x_width, text_ypix(&x->x_obj, glist) + x->x_height);

    photo_drawme(x, glist, 0);
    canvas_fixlinesfor(glist_getcanvas(glist),(t_text*) x);
}

static void photo_select(t_gobj *z, t_glist *glist, int state)
{
    t_photo *x = (t_photo *)z;
    if (state) {
        sys_vgui(".x%x.c create rectangle \
%d %d %d %d -tags %xSEL -outline blue\n",
                 glist_getcanvas(glist),
                 text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
                 text_xpix(&x->x_obj, glist) + x->x_width, text_ypix(&x->x_obj, glist) + x->x_height,
                 x);
    }
    else 
    {
        sys_vgui(".x%x.c delete %xSEL\n",
                 glist_getcanvas(glist), x);
    }
}


static void photo_activate(t_gobj *z, t_glist *glist, int state)
{
/*    t_text *x = (t_text *)z;
      t_rtext *y = glist_findrtext(glist, x);
      if (z->g_pd != gatom_class) rtext_activate(y, state);*/
}

static void photo_delete(t_gobj *z, t_glist *glist)
{
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist_getcanvas(glist), x);
}

       
static void photo_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_photo* s = (t_photo*)z;
    if (vis)
        photo_drawme(s, glist, 1);
    else
        photo_erase(s,glist);
}

/* can we use the normal text save function ?? */

static void photo_save(t_gobj *z, t_binbuf *b)
{
    t_photo *x = (t_photo *)z;
    binbuf_addv(b, "ssiiss;", gensym("#X"), gensym("obj"),
                x->x_obj.te_xpix, x->x_obj.te_ypix,   
                atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)),
                x->filename);
}


void photo_size(t_photo* x,t_floatarg w,t_floatarg h) {
    x->x_width = w;
    x->x_height = h;
}

void photo_open(t_photo* x, t_symbol* fname)
{
    x->filename = fname;
    photo_erase(x, x->x_glist);
    photo_drawme(x, x->x_glist, 1);
}


static void photo_free(t_photo *x)
{
    pd_unbind(&x->x_obj.ob_pd, x->receive_name);
}

static void *photo_new(t_symbol* s)
{
    t_photo *x = (t_photo *)pd_new(photo_class);

    x->x_glist = (t_glist*) canvas_getcurrent();

    x->x_width = 15;
    x->x_height = 15;

    x->x_glist = canvas_getcurrent();

	x->filename = s;
    outlet_new(&x->x_obj, &s_float);
    return (x);
}


static t_widgetbehavior photo_widgetbehavior = {
w_getrectfn:	photo_getrect,
w_displacefn:	photo_displace,
w_selectfn:     photo_select,
w_activatefn:	photo_activate,
w_deletefn:	    photo_delete,
w_visfn:        photo_vis,
w_clickfn:      NULL,
};

void photo_setup(void)
{
    photo_class = class_new(gensym("photo"), (t_newmethod)photo_new, 
                            (t_method)photo_free, 
                            sizeof(t_photo), 0, A_DEFSYM,0);
    
    class_addmethod(photo_class, (t_method)photo_size, gensym("size"),
                    A_FLOAT, A_FLOAT, 0);

/*
  class_addmethod(photo_class, (t_method)photo_color, gensym("color"),
  A_SYMBOL, 0);
*/

    class_addmethod(photo_class, (t_method)photo_open, gensym("open"),
                    A_SYMBOL, 0);
	
    class_setwidget(photo_class, &photo_widgetbehavior);
    class_setsavefn(photo_class, &photo_save);
}


