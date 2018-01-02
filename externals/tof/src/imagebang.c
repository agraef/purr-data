#include <m_pd.h>
#include <g_canvas.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#ifdef _MSC_VER
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

/* Append " x " to the following line to show debugging messages */
#define DEBUG(x)



/* ------------------------ imagebang ----------------------------- */

static t_class *imagebang_class;
t_widgetbehavior imagebang_widgetbehavior;

typedef struct _imagebang
{
     t_object x_obj;
     t_glist * glist;
     int width;
     int height;
     t_symbol*  image_a;
     t_symbol*  image_b;
     t_symbol* receive;
     t_symbol* send;
     t_clock* clock_flash;
     t_clock* clock_brk;
     int         flashing;
     t_outlet* outlet;
} t_imagebang;

static void imagebang_bang(t_imagebang *x)
{
    t_glist* glist = glist_getcanvas(x->glist);
    char image_key[MAXPDSTRING];
    if (x->flashing)
    {
        sys_vgui(".x%x.c itemconfigure %ximage -image %x_imagebang \n",
            glist, x,x->image_a);
        clock_delay(x->clock_brk, 50);
        //x->flashed = 1;
        // key_a:
        sprintf(image_key, "%lx_a", (long unsigned int)x);
        gui_vmess("gui_image_configure", "xxss",
            glist_getcanvas(x->glist),
            x,
            image_key,
            "nw");
    }
    else
    {
        sys_vgui(".x%x.c itemconfigure %ximage -image %x_imagebang \n",
            glist, x,x->image_b);
        // key_b:
        sprintf(image_key, "%lx_b", (long unsigned int)x);
        gui_vmess("gui_image_configure", "xxss",
            glist_getcanvas(x->glist),
            x,
            image_key,
            "nw");
        x->flashing = 1;
    }
    clock_delay(x->clock_flash, 250);
       
    outlet_bang(x->outlet);
    
    if (x->send && x->send->s_thing) pd_bang(x->send->s_thing);
}

static void imagebang_flash_timeout(t_imagebang *x)
{
    t_glist* glist = glist_getcanvas(x->glist);
    char key_a[MAXPDSTRING];
    x->flashing = 0;
    sprintf(key_a, "%lx_a", (long unsigned int)x);
    gui_vmess("gui_image_configure", "xxss",
        glist_getcanvas(x->glist),
        x,
        key_a,
        "nw");
    x->flashing = 1;
    //sys_vgui(".x%x.c itemconfigure %ximage -image %x_imagebang \n",
    //    glist, x,x->image_a);
    
}

static void imagebang_brk_timeout(t_imagebang *x)
{
    t_glist* glist = glist_getcanvas(x->glist);
    char key_b[MAXPDSTRING];
    x->flashing = 1;
    //sys_vgui(".x%x.c itemconfigure %ximage -image %x_imagebang \n",
    //    glist, x,x->image_b);
    sprintf(key_b, "%lx_b", (long unsigned int)x);
    gui_vmess("gui_image_configure", "xxss",
        glist_getcanvas(x->glist),
        x,
        key_b,
        "nw");
}

/* widget helper functions */

static const char* imagebang_get_filename(t_imagebang *x,char *file) {
    static char fname[MAXPDSTRING];
    char *bufptr;
    int fd;
    
    fd=open_via_path(canvas_getdir(glist_getcanvas(x->glist))->s_name, 
        file, "",fname, &bufptr, MAXPDSTRING, 1);
    if(fd>0){
          fname[strlen(fname)]='/';
          DEBUG(post("image file: %s",fname);)
          close(fd);
          return fname;
    } else {
        return 0;
    }
}

static int imagebang_click(t_imagebang *x, struct _glist *glist,
    int xpos, int ypos, int shift, int alt, int dbl, int doit)
{
    //DEBUG(post("x:%i y:%i dbl:%i doit:%i",xpos,ypos,dbl,doit);)
    if (doit) imagebang_bang(x);
    return (1);
}

static void imagebang_drawme(t_imagebang *x, t_glist *glist, int firsttime)
{
    if (firsttime)
    {
        char key_a[MAXPDSTRING];
        char key_b[MAXPDSTRING];
        //DEBUG(post("Rendering: \n   %x_imagebang:%s \n   %x_imagebang:%s",x->image_a,x->image_a->s_name,x->image_b,x->image_b->s_name);)

        //sys_vgui(".x%x.c create image %d %d -anchor nw -image %x_imagebang -disabledimage %x_imagebang -tags %ximage\n", 
        //    glist_getcanvas(glist),
        //    text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),x->image_a,x->image_b,x);

        //sys_vgui("pd [concat %s _imagesize [image width %x_imagebang] [image height %x_imagebang] \\;]\n",x->receive->s_name,x->image_a,x->image_a);

        gui_vmess("gui_gobj_new", "xxsiii",
            glist_getcanvas(glist),
            x,
            "obj",
            text_xpix(&x->x_obj, glist),
            text_ypix(&x->x_obj, glist),
            glist_istoplevel(glist));
        sprintf(key_a, "%lx_a", (long unsigned int)x);
        gui_vmess("gui_gobj_draw_image", "xxss",
            glist_getcanvas(glist),
            x,
            key_a,
            "nw");
        gui_vmess("gui_image_size_callback", "xss",
            glist_getcanvas(glist), key_a, x->receive->s_name);
    }
    else
    {
        //sys_vgui(".x%x.c coords %ximage %d %d\n",
        //    glist_getcanvas(glist), x,
        //    text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist));
        gui_vmess("gui_image_coords", "xxii",
            glist_getcanvas(glist),
            x,
            text_xpix(&x->x_obj, glist),
            text_ypix(&x->x_obj, glist));
    }
}

void imagebang_erase(t_imagebang* x,t_glist* glist)
{
    int n;
    sys_vgui(".x%x.c delete %ximage\n",
        glist_getcanvas(glist), x);
    gui_vmess("gui_gobj_erase", "xx", glist_getcanvas(glist), x);
}


/* ------------------------ image widgetbehaviour----------------------------- */


static void imagebang_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    int width, height;
    t_imagebang* x = (t_imagebang*)z;

    width = x->width;
    height = x->height;
    *xp1 = text_xpix(&x->x_obj, glist);
    *yp1 = text_ypix(&x->x_obj, glist);
    *xp2 = text_xpix(&x->x_obj, glist) + width;
    *yp2 = text_ypix(&x->x_obj, glist) + height;
    //fprintf(stderr,"%d %d %d %d\n", *xp1, *yp1, *xp2, *yp2);
}

static void imagebang_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    t_imagebang *x = (t_imagebang *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    //sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
    //       glist_getcanvas(glist), x,
    //       text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
    //       text_xpix(&x->x_obj, glist) + x->width, text_ypix(&x->x_obj, glist) + x->height);

    imagebang_drawme(x, glist, 0);
    canvas_fixlinesfor(glist,(t_text*) x);
}

static void imagebang_displace_wtag(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    //fprintf(stderr,"image displace_wtag\n");
    t_imagebang *x = (t_imagebang *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    /*sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
           glist_getcanvas(glist), x,
           text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
           text_xpix(&x->x_obj, glist) + x->x_width, text_ypix(&x->x_obj, glist) + x->x_height);

    imagebang_drawme(x, glist, 0);*/
    canvas_fixlinesfor(glist,(t_text*) x);
}

static void imagebang_select(t_gobj *z, t_glist *glist, int state)
{
    t_imagebang *x = (t_imagebang *)z;
    if (state) {
     sys_vgui(".x%x.c create rectangle \
           %d %d %d %d -tags %xSEL -outline $select_color\n",
           glist_getcanvas(glist),
           text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),
           text_xpix(&x->x_obj, glist) + x->width,
           text_ypix(&x->x_obj, glist) + x->height,
           x);
        gui_vmess("gui_gobj_select", "xx", glist_getcanvas(glist), x);
    }
    else {
        sys_vgui(".x%x.c delete %xSEL\n",
            glist_getcanvas(glist), x);
        gui_vmess("gui_gobj_deselect", "xx", glist_getcanvas(glist), x);
    }
}

static void imagebang_activate(t_gobj *z, t_glist *glist, int state)
{
/*    t_text *x = (t_text *)z;
    t_rtext *y = glist_findrtext(glist, x);
    if (z->g_pd != gatom_class) rtext_activate(y, state);*/
}

static void imagebang_delete(t_gobj *z, t_glist *glist)
{
    t_text *x = (t_text *)z;
    //canvas_deletelinesfor(glist_getcanvas(glist), x);
    canvas_deletelinesfor(glist, x);
}
      
static void imagebang_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_imagebang* s = (t_imagebang*)z;
    if (vis)
        imagebang_drawme(s, glist, 1);
    else
        imagebang_erase(s,glist);
}

static void imagebang_size(t_imagebang* x,t_floatarg w,t_floatarg h) {
     x->width = w;
     x->height = h;
}

static void imagebang_imagesize_callback(t_imagebang *x, t_float w, t_float h) {
    DEBUG(post("received w %f h %f",w,h);)
    post("received w %f h %f",w,h);
    x->width = w;
    x->height = h;
    canvas_fixlinesfor(x->glist,(t_text*) x);
}
    
static void imagebang_free(t_imagebang *x) {
    char key_a[MAXPDSTRING];
    char key_b[MAXPDSTRING]; 
    // check first if variable has been unset and image is unused
    // then delete image and unset variable
     DEBUG(sys_vgui("pd [concat DEBUG b in use [image inuse %x_imagebang] \\;]\n",x->image_b);)
     DEBUG(sys_vgui("pd [concat DEBUG a in use [image inuse %x_imagebang] \\;]\n",x->image_a);)
    
    sys_vgui("if { [info exists %x_imagebang] == 1 && [image inuse %x_imagebang] == 0} { image delete %x_imagebang \n unset %x_imagebang\n} \n",x->image_b,x->image_b,x->image_b,x->image_b);
    sys_vgui("if { [info exists %x_imagebang] == 1 && [image inuse %x_imagebang] == 0} { image delete %x_imagebang \n unset %x_imagebang\n} \n",x->image_a,x->image_a,x->image_a,x->image_a);
    
    DEBUG(sys_vgui("pd [concat DEBUG b exists [info exists %x_imagebang] \\;]\n",x->image_b);)
     DEBUG(sys_vgui("pd [concat DEBUG a exists [info exists %x_imagebang] \\;]\n",x->image_a);)
    
    sprintf(key_a, "%lx_a", (long unsigned int)x);
    sprintf(key_b, "%lx_b", (long unsigned int)x);
    gui_vmess("gui_image_free", "s", key_a);
    gui_vmess("gui_image_free", "s", key_b);
    if (x->receive) {
        pd_unbind(&x->x_obj.ob_pd,x->receive);
    }
    clock_free(x->clock_flash);
    clock_free(x->clock_brk);
    
}
    
static void *imagebang_new(t_symbol *s, int argc, t_atom *argv)
{
    t_imagebang *x = (t_imagebang *)pd_new(imagebang_class);
    char key_a[MAXPDSTRING];
    char key_b[MAXPDSTRING];
    t_atom sane_defaults[2];
    if (!argc)
    {
        post("imagebang: warning: no arguments provided: setting defaults to "
             "[imagebang dummy dummy]");
        SETSYMBOL(sane_defaults, gensym("dummy"));
        SETSYMBOL(sane_defaults+1, gensym("dummy"));
        argc = 2;
        argv = sane_defaults;
    }
    x->glist = (t_glist*) canvas_getcurrent();
    // Set up a callback to get the size
    x->width = 10;
    x->height = 10;

    x->flashing = 0;

    x->image_a = NULL;
    x->image_b = NULL;

    t_symbol* image_a = NULL;
    t_symbol* image_b = NULL;

    const char *fname;

    // CREATE IMAGES
    // images are only created if they have not been created yet
    // we use the symbol pointer to distinguish between image files

    if ( argc && (argv)->a_type == A_SYMBOL )
    {
        image_a= atom_getsymbol(argv);
        sprintf(key_a, "%lx_a", (long unsigned int)x);

        // Get image file path
        fname = imagebang_get_filename(x,image_a->s_name);
        if (fname)
        {
            x->image_a = gensym(fname);
            //sys_vgui("set %x_a \"%s\" \n",x,fname);
            // Create the image only if the class has not already loaded the
            // same image (with the same symbolic path name)
            //sys_vgui("if { [info exists %x_imagebang] == 0 } "
            //         "{ image create photo %x_imagebang "
            //         "-file \"%s\"\n set %x_imagebang 1\n} \n",
            //             x->image_a,x->image_a,fname,x->image_a); 
            //sys_vgui("pd [concat test %x_imagebang \\;]\n",x->image_a);
            gui_vmess("gui_load_image", "xss",
                canvas_getcurrent(),
                key_a,
                fname);
        }
        else
        {
            x->image_a = &s_;
            pd_error(x, "could not find image \"%s\": using default image",
                image_a->s_name);
            gui_vmess("gui_load_default_image", "xs",
                canvas_getcurrent(),
                key_a);
        }
    }
    if ( argc > 1 && (argv+1)->a_type == A_SYMBOL )
    {
        image_b= atom_getsymbol(argv+1);
        sprintf(key_b, "%lx_b", (long unsigned int)x);

        // Get image file path
        fname = imagebang_get_filename(x,image_b->s_name);
        if (fname)
        {
            x->image_b = gensym(fname);
            //sys_vgui("set %x_b \"%s\" \n",x,fname);
            //sys_vgui("if { [info exists %x_imagebang] == 0} "
            //         "{ image create photo %x_imagebang "
            //         "-file \"%s\"\n set %x_imagebang 1\n} \n",
            //             x->image_b,x->image_b,fname,x->image_b);
            //sys_vgui("pd [concat test %x_imagebang \\;]\n",x->image_b);
            gui_vmess("gui_load_image", "xss",
                canvas_getcurrent(),
                key_b,
                fname);
        }
        else
        {
            x->image_b = &s_;
            pd_error(x, "could not find image \"%s\": using default image",
                image_b->s_name);
            gui_vmess("gui_load_default_image", "xs",
                canvas_getcurrent(),
                key_b);
        }
    }

    // Stop if no images    
    //if ( x->image_a == NULL || x->image_b == NULL)
    //{
    //    post("Could not create [imagebang]... "
    //         "either no gif images defined or found!");
    //    return NULL;
    //}

    x->send = NULL;
    if ( argc > 2 && (argv+2)->a_type == A_SYMBOL )
    {
        x->send = atom_getsymbol(argv+2);
    }

    if ( argc > 3 && (argv+3)->a_type == A_SYMBOL )
    {
        x->receive = atom_getsymbol(argv+3);
    }
    else
    {
       // Create default receiver if none set
        char buf[MAXPDSTRING];
        sprintf(buf, "#%lx", (long)x);
        x->receive = gensym(buf);
    }

    pd_bind(&x->x_obj.ob_pd, x->receive );

    x->clock_flash = clock_new(x, (t_method)imagebang_flash_timeout);
    x->clock_brk = clock_new(x, (t_method)imagebang_brk_timeout);
    x->outlet = outlet_new(&x->x_obj, &s_float);
    return (x);
}

void imagebang_setup(void)
{
    imagebang_class = class_new(gensym("imagebang"),
        (t_newmethod)imagebang_new, (t_method)imagebang_free,
        sizeof(t_imagebang),0, A_GIMME,0);

    class_addmethod(imagebang_class, (t_method)imagebang_imagesize_callback,
        gensym("_imagesize"), A_DEFFLOAT, A_DEFFLOAT, 0);

    class_addbang(imagebang_class,(t_method)imagebang_bang);
    
    imagebang_widgetbehavior.w_getrectfn =    imagebang_getrect;
    imagebang_widgetbehavior.w_displacefn =    imagebang_displace;
    imagebang_widgetbehavior.w_selectfn =    imagebang_select;
    imagebang_widgetbehavior.w_activatefn =    imagebang_activate;
    imagebang_widgetbehavior.w_deletefn =    imagebang_delete;
    imagebang_widgetbehavior.w_visfn =        imagebang_vis;
    imagebang_widgetbehavior.w_displacefnwtag =    imagebang_displace_wtag;

    imagebang_widgetbehavior.w_clickfn = (t_clickfn)imagebang_click;
    

#if PD_MINOR_VERSION < 37
    imagebang_widgetbehavior.w_propertiesfn = NULL; 
    //imagebang_widgetbehavior.w_savefn = imagebang_save;
#endif
    
    class_setwidget(imagebang_class,&imagebang_widgetbehavior);
#if PD_MINOR_VERSION >= 37
   // class_setsavefn(imagebang_class,&imagebang_save);
#endif
}

