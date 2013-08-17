#include <string.h>
#include <m_pd.h>
#include "g_canvas.h"
#include "s_stuff.h"
#include <stdio.h>

#ifdef _MSC_VER
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

/* ------------------------ image ----------------------------- */

static t_class *image_class;

typedef struct _image
{
	t_object x_obj;
	t_glist * x_glist;
	int x_width;
	int x_height;
	int x_img_width;
	int x_img_height;
	int x_gop_spill;
	t_symbol*  x_fname;
	t_symbol* receive;
	//t_symbol* send;
} t_image;

/* widget helper functions */

void image_doopen(t_image* x) {
	t_glist *glist = glist_getcanvas(x->x_glist);
	if (strlen(x->x_fname->s_name) != 0) {
		char fname[FILENAME_MAX];
		canvas_makefilename(glist_getcanvas(x->x_glist), x->x_fname->s_name,
						fname, FILENAME_MAX);
		//fprintf(stderr,"post @ cooked name <%s>\n", fname);
		sys_vgui(".x%x.c create rectangle \
			%d %d %d %d -tags %xMT -outline black -fill gray\n",
			glist,
			text_xpix(&x->x_obj, x->x_glist) - x->x_width/2,
			text_ypix(&x->x_obj, x->x_glist) - x->x_height/2,
			text_xpix(&x->x_obj, x->x_glist) + x->x_width/2,
			text_ypix(&x->x_obj, x->x_glist) + x->x_height/2, x);
		sys_vgui("catch {image delete $img%x}\n", x);
		sys_vgui("set img%x [image create photo -file {%s}]\n", x, fname);
		sys_vgui(".x%x.c itemconfigure %xS -image $img%x\n", 
			   glist, x, x);
		sys_vgui("pd [concat %s _imagesize [image width $img%x] [image height $img%x] \\;]\n",x->receive->s_name, x, x);
	}
	else {
		sys_vgui(".x%x.c create rectangle \
			%d %d %d %d -tags %xMT -outline black -fill gray\n",
			glist,
			text_xpix(&x->x_obj, x->x_glist) - x->x_width/2,
			text_ypix(&x->x_obj, x->x_glist) - x->x_height/2,
			text_xpix(&x->x_obj, x->x_glist) + x->x_width/2,
			text_ypix(&x->x_obj, x->x_glist) + x->x_height/2, x);
	}
}

void image_drawme(t_image *x, t_glist *glist, int redraw)
{
	if (redraw) {
		//first create blank image widget (in case we have no image to begin with)
		//sys_vgui(".x%x.c itemconfigure %xS -image null\n", glist_getcanvas(glist));
		sys_vgui("catch {.x%x.c delete %xMT}\n",glist_getcanvas(glist), x);
		sys_vgui("catch {.x%lx.c delete %xS}\n", glist_getcanvas(glist),x);
		sys_vgui(".x%x.c create image %d %d -tags %xS\n", 
			glist_getcanvas(glist),text_xpix(&x->x_obj, glist), 
			text_ypix(&x->x_obj, glist),x);
		image_doopen(x);
     }     
     else {
		if (x->x_img_width + x->x_img_height == 0) {
			sys_vgui(".x%x.c coords %xMT %d %d\n",
				glist_getcanvas(glist), x,
				text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist));
		}
		sys_vgui(".x%x.c coords %xS %d %d\n",
			glist_getcanvas(glist), x,
			text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist));
     }
	
}


void image_erase(t_image* x,t_glist* glist)
{
	sys_vgui("catch {.x%x.c delete %xMT}\n",glist_getcanvas(glist), x);
	sys_vgui("catch {.x%x.c delete %xS}\n",glist_getcanvas(glist), x);
	sys_vgui("catch {image delete $img%x}\n", x);
	sys_vgui("catch {.x%x.c delete %xSEL}\n",glist_getcanvas(glist), x);
}
	

static t_symbol *get_filename(t_int argc, t_atom *argv)
{
    t_symbol *fname;
    fname = atom_getsymbolarg(0, argc, argv);
	//fprintf(stderr,"fname=<%s>\n", fname->s_name);
    if(argc > 1)
    {
        int i;
        char buf[MAXPDSTRING];
        strcpy(buf, fname->s_name);
        for(i = 1; i < argc; i++)
        {
			if (argv[i].a_type == A_SYMBOL) {
		        strcat(buf, " ");
		        strcat(buf, atom_getsymbolarg(i, argc, argv)->s_name);
			} else {
				break;
			}
        }
        fname = gensym(buf);
		//fprintf(stderr,"argc>1 fname=<%s>\n", fname->s_name);
    }
    return fname;
}

/* ------------------------ image widgetbehaviour----------------------------- */

static void image_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
	int width, height;
	t_image* x = (t_image*)z;

	if (!x->x_gop_spill && (x->x_img_width + x->x_img_height) >= 2) {
		width = x->x_img_width;
		height = x->x_img_height;	
	} else {
		width = x->x_width;
		height = x->x_height;
	}
	*xp1 = text_xpix(&x->x_obj, glist) - width/2;
	*yp1 = text_ypix(&x->x_obj, glist) - height/2;
	*xp2 = text_xpix(&x->x_obj, glist) + width/2;
	*yp2 = text_ypix(&x->x_obj, glist) + height/2;
	//fprintf(stderr,"image_getrect %d %d %d %d\n", *xp1, *yp1, *xp2, *yp2);
}

static void image_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
	//fprintf(stderr,"image displace\n");
    t_image *x = (t_image *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
	if (!x->x_gop_spill && (x->x_img_width + x->x_img_height) >= 2){
		sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
			glist_getcanvas(glist), x,
			text_xpix(&x->x_obj, glist) - x->x_img_width/2,
			text_ypix(&x->x_obj, glist) - x->x_img_height/2,
			text_xpix(&x->x_obj, glist) + x->x_img_width/2,
			text_ypix(&x->x_obj, glist) + x->x_img_height/2);
	} else {
		sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
			glist_getcanvas(glist), x,
			text_xpix(&x->x_obj, glist) - x->x_width/2,
			text_ypix(&x->x_obj, glist) - x->x_height/2,
			text_xpix(&x->x_obj, glist) + x->x_width/2,
			text_ypix(&x->x_obj, glist) + x->x_height/2);
		if (x->x_img_width + x->x_img_height == 0)
			sys_vgui(".x%x.c coords %xMT %d %d %d %d\n",
				glist_getcanvas(glist), x,
				text_xpix(&x->x_obj, glist) - x->x_width/2,
				text_ypix(&x->x_obj, glist) - x->x_height/2,
				text_xpix(&x->x_obj, glist) + x->x_width/2,
				text_ypix(&x->x_obj, glist) + x->x_height/2);
	}

    image_drawme(x, glist, 0);
    canvas_fixlinesfor(glist,(t_text*) x);
}

static void image_displace_wtag(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
	//fprintf(stderr,"image displace_wtag\n");
    t_image *x = (t_image *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    /*sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
		   glist_getcanvas(glist), x,
		   text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
		   text_xpix(&x->x_obj, glist) + x->x_width, text_ypix(&x->x_obj, glist) + x->x_height);

    image_drawme(x, glist, 0);*/
    canvas_fixlinesfor(glist,(t_text*) x);
}

static void image_select(t_gobj *z, t_glist *glist, int state)
{
	//fprintf(stderr,"image_select %d\n", state);
	t_image *x = (t_image *)z;
	if (state) {
		if (glist_istoplevel(glist)) {
			if (!x->x_gop_spill && (x->x_img_width + x->x_img_height) >= 2)
				sys_vgui(".x%x.c create rectangle \
					%d %d %d %d -tags %xSEL -outline $select_color\n",
					glist_getcanvas(glist),
					text_xpix(&x->x_obj, glist) - x->x_img_width/2,
					text_ypix(&x->x_obj, glist) - x->x_img_height/2,
					text_xpix(&x->x_obj, glist) + x->x_img_width/2,
					text_ypix(&x->x_obj, glist) + x->x_img_height/2, x);
			else
				sys_vgui(".x%x.c create rectangle \
					%d %d %d %d -tags %xSEL -outline $select_color\n",
					glist_getcanvas(glist),
					text_xpix(&x->x_obj, glist) - x->x_width/2,
					text_ypix(&x->x_obj, glist) - x->x_height/2,
					text_xpix(&x->x_obj, glist) + x->x_width/2,
					text_ypix(&x->x_obj, glist) + x->x_height/2, x);
		}
		//if (glist->gl_owner && !glist_istoplevel(glist))
		sys_vgui(".x%x.c addtag selected withtag %xS\n", glist_getcanvas(glist), x);
		sys_vgui(".x%x.c addtag selected withtag %xMT\n", glist_getcanvas(glist), x);
		sys_vgui(".x%x.c addtag selected withtag %xSEL\n", glist_getcanvas(glist), x);
	}
	else {
		sys_vgui("catch {.x%x.c delete %xSEL}\n",
		glist_getcanvas(glist), x);
		//if (glist->gl_owner && !glist_istoplevel(glist))
		sys_vgui(".x%lx.c dtag %xS selected\n", glist_getcanvas(glist), x);
		sys_vgui(".x%lx.c dtag %xMT selected\n", glist_getcanvas(glist), x);
	}
}


static void image_activate(t_gobj *z, t_glist *glist, int state)
{
	/*fprintf(stderr,"activate...\n");
	t_text *x = (t_text *)z;
	t_rtext *y = glist_findrtext(glist, x);
	rtext_activate(y, state);
	t_image *i = (t_image *)z;
	canvas_redraw(i->x_glist);*/
}

static void image_delete(t_gobj *z, t_glist *glist)
{
	t_text *x = (t_text *)z;
	canvas_deletelinesfor(glist, x);
}

       
static void image_vis(t_gobj *z, t_glist *glist, int vis)
{
	//fprintf(stderr,"image_vis %d\n", vis);
	t_image* x = (t_image*)z;
	if (vis)
		image_drawme(x, glist, 1);
	else
		image_erase(x,glist);
}

/* can we use the normal text save function ?? */

static void image_save(t_gobj *z, t_binbuf *b)
{
    t_image *x = (t_image *)z;
    binbuf_addv(b, "ssiiss", gensym("#X"), gensym("obj"),
                x->x_obj.te_xpix, x->x_obj.te_ypix,   
                atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)),
                x->x_fname);
    binbuf_addv(b, ";");
}


t_widgetbehavior   image_widgetbehavior;

/*void image_size(t_image* x,t_floatarg w,t_floatarg h) {
     x->x_width = w;
     x->x_height = h;
	 image_displace((t_gobj*)x, x->x_glist, 0.0, 0.0);
}*/

/*void image_color(t_image* x,t_symbol* col)
{
     //outlet_bang(x->x_obj.ob_outlet); only bang if there was a bang .. 
     //so color black does the same as bang, but doesn't forward the bang 

}*/

/*static int image_newclick(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
	t_image *x = (t_image *)z;
	if (doit)
		outlet_bang(x->x_obj.ob_outlet);
	return(1);
}*/

void image_gop_spill(t_image* x, t_floatarg f)
{
     x->x_gop_spill = (f >= 0 ? f : 0);
	 image_displace((t_gobj*)x, x->x_glist, 0.0, 0.0);
}

void image_open(t_image* x, t_symbol *s, t_int argc, t_atom *argv)
{
    x->x_fname = get_filename(argc, argv);
	x->x_img_width = 0;
	x->x_img_height = 0;
	image_doopen(x);
}

static void image_imagesize_callback(t_image *x, t_float w, t_float h) {
	//fprintf(stderr,"received w %f h %f should %d spill %d\n", w, h, gobj_shouldvis((t_gobj *)x, glist_getcanvas(x->x_glist)), x->x_gop_spill);
	x->x_img_width = w;
	x->x_img_height = h;
	if (!gobj_shouldvis((t_gobj *)x, x->x_glist) && !x->x_gop_spill) {
			//fprintf(stderr,"erasing\n");
			image_erase(x, glist_getcanvas(x->x_glist));
	} else {
		sys_vgui("catch {.x%x.c delete %xMT}\n", glist_getcanvas(x->x_glist), x);
		canvas_fixlinesfor(x->x_glist,(t_text*) x);
	}
}

static void image_setwidget(void)
{
    image_widgetbehavior.w_getrectfn =     	image_getrect;
    image_widgetbehavior.w_displacefn =    	image_displace;
    image_widgetbehavior.w_selectfn =   	image_select;
    image_widgetbehavior.w_activatefn =   	image_activate;
    image_widgetbehavior.w_deletefn =   	image_delete;
    image_widgetbehavior.w_visfn =   		image_vis;
    //image_widgetbehavior.w_clickfn = 		image_newclick;
    image_widgetbehavior.w_displacefnwtag =	image_displace_wtag;
}

static void image_free(t_image *x)
{
	sys_vgui("image delete img%x\n", x);
    if (x->receive) {
		pd_unbind(&x->x_obj.ob_pd,x->receive);
	}
	//sys_vgui(".x%x.c delete %xSEL\n", x);
	//sys_vgui(".x%x.c delete %xS\n", x);
}

static void *image_new(t_symbol *s, t_int argc, t_atom *argv)
{
    t_image *x = (t_image *)pd_new(image_class);

    x->x_glist = (t_glist*) canvas_getcurrent();

    x->x_width = 15;
    x->x_height = 15;
	x->x_img_width = 0;
	x->x_img_height = 0;
	x->x_gop_spill = 0;

	x->x_fname = get_filename(argc, argv);
	if (strlen(x->x_fname->s_name) > 0) {
		//fprintf(stderr,"get_filename succeeded <%s> <%s>\n", x->x_fname->s_name, atom_getsymbol(argv)->s_name);
		argc--;
		argv++;
	}

	if (argc && argv[0].a_type == A_FLOAT) {
		//we have optional gop_spill flag first
		//fprintf(stderr,"gop_spill succeeded\n");
		x->x_gop_spill = (int)atom_getfloat(&argv[0]);
		argc--;
		argv++;
	}

	// Create default receiver
	char buf[MAXPDSTRING];
	sprintf(buf, "#%lx", (long)x);
	x->receive = gensym(buf);
    pd_bind(&x->x_obj.ob_pd, x->receive);

    //outlet_new(&x->x_obj, &s_bang);
    return (x);
}

void image_setup(void)
{
    image_class = class_new(gensym("image"),
				(t_newmethod)image_new, (t_method)image_free,
				sizeof(t_image),0, A_GIMME,0);
/*
	class_addmethod(image_class, (t_method)image_size, gensym("size"),
    	A_FLOAT, A_FLOAT, 0);
	class_addmethod(image_class, (t_method)image_color, gensym("color"),
    	A_SYMBOL, 0);
*/
    class_addmethod(image_class, (t_method)image_open, gensym("open"),
    	A_GIMME, 0);
    class_addmethod(image_class, (t_method)image_gop_spill, gensym("gopspill"),
    	A_DEFFLOAT, 0);
    class_addmethod(image_class, (t_method)image_imagesize_callback,\
                     gensym("_imagesize"), A_DEFFLOAT, A_DEFFLOAT, 0);
	
    image_setwidget();
    class_setwidget(image_class,&image_widgetbehavior);
    class_setsavefn(image_class,&image_save);
}


