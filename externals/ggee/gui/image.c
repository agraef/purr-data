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
	t_symbol*  x_fname;
} t_image;

/* widget helper functions */


char * image_path_replace(
    char const * const original, 
    char const * const pattern, 
    char const * const replacement
) {
	size_t const replen = strlen(replacement);
	size_t const patlen = strlen(pattern);
	size_t const orilen = strlen(original);

	size_t patcnt = 0;
	const char * oriptr;
	const char * patloc;

	// find how many times the pattern occurs in the original string
	for (oriptr = original; patloc = strstr(oriptr, pattern); oriptr = patloc + patlen)
	{
		patcnt++;
	}

	{
		// allocate memory for the new string
		size_t const retlen = orilen + patcnt * (replen - patlen);
		char * const returned = (char *) malloc( sizeof(char) * (retlen + 1) );

		if (returned != NULL)
		{
			// copy the original string, 
			// replacing all the instances of the pattern
			char * retptr = returned;
			for (oriptr = original; patloc = strstr(oriptr, pattern); oriptr = patloc + patlen)
			{
				size_t const skplen = patloc - oriptr;
				// copy the section until the occurence of the pattern
				strncpy(retptr, oriptr, skplen);
				retptr += skplen;
				// copy the replacement 
				strncpy(retptr, replacement, replen);
				retptr += replen;
			}
			// copy the rest of the string.
			strcpy(retptr, oriptr);
		}
		return returned;
	}
}

void image_doopen(t_image* x) {
	if (strlen(x->x_fname->s_name) != 0) {
		char fname[MAXPDSTRING];
		canvas_makefilename(glist_getcanvas(x->x_glist), x->x_fname->s_name,
						fname, MAXPDSTRING);
		
		//check for @sys_extra path and replace
		if (strstr(fname, "@pd_extra") != NULL) {
			t_namelist *path = pd_extrapath;
			while (path->nl_next)
				path = path->nl_next;
			const char *new_fname = image_path_replace(x->x_fname->s_name, "@pd_extra", path->nl_string);
			strcpy(fname, new_fname);
			freebytes(new_fname, strlen(new_fname));
		}
		sys_vgui("catch {image delete $img%x}\n", x);
		sys_vgui("set img%x [image create photo -file {%s}]\n", x, fname);
		sys_vgui(".x%x.c itemconfigure %xS -image $img%x\n", 
			   glist_getcanvas(x->x_glist),x,x);
	}
}

void image_drawme(t_image *x, t_glist *glist, int redraw)
{
	if (redraw) {
		//first create blank image widget (in case we have no image to begin with)
		//sys_vgui(".x%x.c itemconfigure %xS -image null\n", glist_getcanvas(glist));
		sys_vgui("catch {.x%lx.c delete %xS}\n", glist_getcanvas(glist),x);
		sys_vgui(".x%x.c create image %d %d -tags %xS\n", 
			glist_getcanvas(glist),text_xpix(&x->x_obj, glist), 
			text_ypix(&x->x_obj, glist),x);
		image_doopen(x);
     }     
     else {
		sys_vgui(".x%x.c coords %xS %d %d\n",
			glist_getcanvas(glist), x,
			text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist));
     }
	
}


void image_erase(t_image* x,t_glist* glist)
{
	sys_vgui("catch {.x%x.c delete %xS}\n",glist_getcanvas(glist), x);
	sys_vgui("catch {image delete $img%x}\n", x);
	sys_vgui("catch {.x%x.c delete %xSEL}\n",glist_getcanvas(glist), x);
}
	

static t_symbol *get_filename(t_int argc, t_atom *argv)
{
    t_symbol *fname;
    fname = atom_getsymbolarg(0, argc, argv);
    if(argc > 1)
    {
        int i;
        char buf[MAXPDSTRING];
        strcpy(buf, fname->s_name);
        for(i = 1; i < argc; i++)
        {
            strcat(buf, " ");
            strcat(buf, atom_getsymbolarg(i, argc, argv)->s_name);
        }
        fname = gensym(buf);
    }
    return fname;
}

/* ------------------------ image widgetbehaviour----------------------------- */


static void image_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
	int width, height;
	t_image* x = (t_image*)z;

	width = x->x_width;
	height = x->x_height;
	*xp1 = text_xpix(&x->x_obj, glist);
	*yp1 = text_ypix(&x->x_obj, glist);
	*xp2 = text_xpix(&x->x_obj, glist) + width;
	*yp2 = text_ypix(&x->x_obj, glist) + height;
	//fprintf(stderr,"image_getrect %d %d %d %d\n", *xp1, *yp1, *xp2, *yp2);
}

static void image_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
	//fprintf(stderr,"image displace\n");
    t_image *x = (t_image *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
		   glist_getcanvas(glist), x,
		   text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
		   text_xpix(&x->x_obj, glist) + x->x_width, text_ypix(&x->x_obj, glist) + x->x_height);

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
		if (glist_istoplevel(glist))
			sys_vgui(".x%x.c create rectangle \
				%d %d %d %d -tags %xSEL -outline $select_color\n",
		glist_getcanvas(glist),
		text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
		text_xpix(&x->x_obj, glist) + x->x_width, text_ypix(&x->x_obj, glist) + x->x_height, x);
		//if (glist->gl_owner && !glist_istoplevel(glist))
		sys_vgui(".x%x.c addtag selected withtag %xS\n", glist_getcanvas(glist), x);
		sys_vgui(".x%x.c addtag selected withtag %xSEL\n", glist_getcanvas(glist), x);
	}
	else {
		sys_vgui("catch {.x%x.c delete %xSEL}\n",
		glist_getcanvas(glist), x);
		//if (glist->gl_owner && !glist_istoplevel(glist))
		sys_vgui(".x%lx.c dtag %xS selected\n", glist_getcanvas(glist), x);
	}
}


static void image_activate(t_gobj *z, t_glist *glist, int state)
{
	/*t_text *x = (t_text *)z;
	t_rtext *y = glist_findrtext(glist, x);
	if (z->g_pd != gatom_class) rtext_activate(y, state);*/
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

void image_size(t_image* x,t_floatarg w,t_floatarg h) {
     x->x_width = w;
     x->x_height = h;
	 image_displace((t_gobj*)x, x->x_glist, 0.0, 0.0);
}

void image_color(t_image* x,t_symbol* col)
{
/*     outlet_bang(x->x_obj.ob_outlet); only bang if there was a bang .. 
       so color black does the same as bang, but doesn't forward the bang 
*/
}

/*static int image_newclick(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
	t_image *x = (t_image *)z;
	if (doit)
		outlet_bang(x->x_obj.ob_outlet);
	return(1);
}*/

void image_open(t_image* x, t_symbol *s, t_int argc, t_atom *argv)
{
    x->x_fname = get_filename(argc, argv);
	image_doopen(x);
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
	//sys_vgui(".x%x.c delete %xSEL\n", x);
	//sys_vgui(".x%x.c delete %xS\n", x);
}

static void *image_new(t_symbol *s, t_int argc, t_atom *argv)
{
    t_image *x = (t_image *)pd_new(image_class);

    x->x_glist = (t_glist*) canvas_getcurrent();

    x->x_width = 15;
    x->x_height = 15;

    x->x_fname = get_filename(argc, argv);
    //outlet_new(&x->x_obj, &s_bang);
    return (x);
}

void image_setup(void)
{
    image_class = class_new(gensym("image"), (t_newmethod)image_new, (t_method)image_free,
				sizeof(t_image),0, A_GIMME,0);

    class_addmethod(image_class, (t_method)image_size, gensym("size"),
    	A_FLOAT, A_FLOAT, 0);

/*
    class_addmethod(image_class, (t_method)image_color, gensym("color"),
    	A_SYMBOL, 0);
*/

    class_addmethod(image_class, (t_method)image_open, gensym("open"),
    	A_GIMME, 0);
	
    image_setwidget();
    class_setwidget(image_class,&image_widgetbehavior);
#if PD_MINOR_VERSION >= 37
    class_setsavefn(image_class,&image_save);
#endif
}


