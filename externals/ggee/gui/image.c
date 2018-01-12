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
    int x_click;
    //t_float x_clicked;
    t_symbol* x_fname;
    t_symbol* x_receive;
    //int x_selected;
    //t_symbol* send;
} t_image;

/* widget helper functions */
static void image_select(t_gobj *z, t_glist *glist, int state);

t_symbol *image_trytoopen(t_image* x)
{
    char fname[FILENAME_MAX];
    FILE *file;
    if (x->x_fname == &s_)
    {
        return 0;
    }
    t_glist *glist = glist_getcanvas(x->x_glist);
    canvas_makefilename(glist_getcanvas(x->x_glist), x->x_fname->s_name,
        fname, FILENAME_MAX);
    // try to open the file
    if (file = fopen(fname, "r"))
    {
        fclose(file);
        return gensym(fname);
    }
    else
    {
        return 0;
    }
}

static void image_drawme(t_image *x, t_glist *glist, int firstime)
{
    if (firstime)
    {
        t_symbol *fname = image_trytoopen(x);
        // make a new gobj, border, etc.
        gui_vmess("gui_gobj_new", "xxsiii",
            glist_getcanvas(glist),
            x,
            "obj",
            text_xpix(&x->x_obj, glist),
            text_ypix(&x->x_obj, glist),
            glist_istoplevel(glist));
        if (fname) {
            gui_vmess("gui_load_image", "xxs",
                glist_getcanvas(glist), x, fname->s_name);
        }
        else
        {
            gui_vmess("gui_load_default_image", "xx",
                glist_getcanvas(glist), x);
        }
        // draw the new canvas image
        gui_vmess("gui_gobj_draw_image", "xxxs",
            glist_getcanvas(glist),
            x,
            x,
            "center");
        //sys_vgui("catch {.x%lx.c delete %xS}\n", glist_getcanvas(glist), x);
        //sys_vgui(".x%x.c create image %d %d -tags %xS\n",
        //    glist_getcanvas(glist),text_xpix(&x->x_obj, glist),
        //    text_ypix(&x->x_obj, glist), x);
        gui_vmess("gui_image_size_callback", "xxs",
            glist_getcanvas(glist), x, x->x_receive->s_name);
    }
    else
    {
        // move the gobj
        //sys_vgui(".x%x.c coords %xS %d %d\n",
        //    glist_getcanvas(glist), x,
        //    text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist));
        gui_vmess("gui_image_coords", "xxii",
            glist_getcanvas(glist),
            x,
            text_xpix(&x->x_obj, glist),
            text_ypix(&x->x_obj, glist));
    }
}

static void image_erase(t_image* x,t_glist* glist)
{
    gui_vmess("gui_gobj_erase", "xx", glist_getcanvas(glist), x);
    //sys_vgui("catch {.x%x.c delete %xS}\n",glist_getcanvas(glist), x);
    //sys_vgui("catch {image delete $img%x}\n", x);
    //sys_vgui("catch {.x%x.c delete %xSEL}\n",glist_getcanvas(glist), x);
}

static t_symbol *get_filename(t_int argc, t_atom *argv)
{
    t_symbol *fname;
    fname = atom_getsymbolarg(0, argc, argv);
    if (argc > 1)
    {
        int i;
        char buf[MAXPDSTRING];
        strcpy(buf, fname->s_name);
        for (i = 1; i < argc; i++)
        {
            if (argv[i].a_type == A_SYMBOL)
            {
                strcat(buf, " ");
                strcat(buf, atom_getsymbolarg(i, argc, argv)->s_name);
            }
            else
            {
                break;
            }
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
    //printf("image_getrect %d %d\n", *xp1, *yp1);

    if (!x->x_gop_spill && (x->x_img_width + x->x_img_height) >= 2) {
        width = x->x_img_width;
        height = x->x_img_height;
    }
    else
    {
        width = x->x_width;
        height = x->x_height;
    }
    *xp1 = text_xpix(&x->x_obj, glist) - width/2;
    *yp1 = text_ypix(&x->x_obj, glist) - height/2;
    *xp2 = text_xpix(&x->x_obj, glist) + width/2;
    *yp2 = text_ypix(&x->x_obj, glist) + height/2;

    // if we have click detection disabled and we are in runmode,
    // return 0 size to allow for click relegation to hidden objects below
    // CAREFUL: this code is not reusable for objects that have more than
    // one inlet or outlet because it will cram them together
    if ((glist_getcanvas(glist) != glist && !x->x_click) || (!glist->gl_edit && !x->x_click))
    {
        *xp2 = *xp1;
        // only if we have an image loaded and we are placed within a GOP obliterate the height
        //if (glist_getcanvas(glist) != glist && (x->x_img_width + x->x_img_height) >= 2)
        //{
            //printf("blah\n");
            //*yp2 = *yp1;
        //}
    }
    //fprintf(stderr,"image_getrect %d %d %d %d\n", *xp1, *yp1, *xp2, *yp2);
}

static void image_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    //fprintf(stderr,"image displace\n");
    t_image *x = (t_image *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    if (!x->x_gop_spill && (x->x_img_width + x->x_img_height) >= 2)
    {
        sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
            glist_getcanvas(glist), x,
            text_xpix(&x->x_obj, glist) - x->x_img_width/2,
            text_ypix(&x->x_obj, glist) - x->x_img_height/2,
            text_xpix(&x->x_obj, glist) + x->x_img_width/2,
            text_ypix(&x->x_obj, glist) + x->x_img_height/2);
    }
    else
    {
        sys_vgui(".x%x.c coords %xSEL %d %d %d %d\n",
            glist_getcanvas(glist), x,
            text_xpix(&x->x_obj, glist) - x->x_width/2,
            text_ypix(&x->x_obj, glist) - x->x_height/2,
            text_xpix(&x->x_obj, glist) + x->x_width/2,
            text_ypix(&x->x_obj, glist) + x->x_height/2);
        /*if (x->x_img_width + x->x_img_height == 0)
            sys_vgui(".x%x.c coords %xMT %d %d %d %d\n",
                glist_getcanvas(glist), x,
                text_xpix(&x->x_obj, glist) - x->x_width/2,
                text_ypix(&x->x_obj, glist) - x->x_height/2,
                text_xpix(&x->x_obj, glist) + x->x_width/2,
                text_ypix(&x->x_obj, glist) + x->x_height/2);*/
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
    gui_vmess("gui_image_toggle_border", "xxi",
        glist_getcanvas(glist), x, state);
    if (state)
    {
        if (x->x_glist == glist_getcanvas(glist))
        {
            //x->x_selected = state;
            if (!x->x_gop_spill && (x->x_img_width + x->x_img_height) >= 2)
            {
                sys_vgui(".x%x.c create prect %d %d %d %d \
                        -tags %xSEL -strokewidth 1 -stroke $pd_colors(selection)\n",
                    glist_getcanvas(glist),
                    text_xpix(&x->x_obj, glist) - x->x_img_width/2,
                    text_ypix(&x->x_obj, glist) - x->x_img_height/2,
                    text_xpix(&x->x_obj, glist) + x->x_img_width/2,
                    text_ypix(&x->x_obj, glist) + x->x_img_height/2, x);
            }
            else
            {
                sys_vgui(".x%x.c create prect %d %d %d %d \
                        -tags %xSEL -strokewidth 1 -stroke $pd_colors(selection)\n",
                    glist_getcanvas(glist),
                    text_xpix(&x->x_obj, glist) - x->x_width/2,
                    text_ypix(&x->x_obj, glist) - x->x_height/2,
                    text_xpix(&x->x_obj, glist) + x->x_width/2,
                    text_ypix(&x->x_obj, glist) + x->x_height/2, x);
            }
        }
        //if (glist->gl_owner && !glist_istoplevel(glist))
        //sys_vgui(".x%x.c addtag selected withtag %xS\n", glist_getcanvas(glist), x);
        //sys_vgui(".x%x.c addtag selected withtag %xMT\n", glist_getcanvas(glist), x);
        //sys_vgui(".x%x.c addtag selected withtag %xSEL\n", glist_getcanvas(glist), x);
        gui_vmess("gui_gobj_select", "xx", glist_getcanvas(glist), x);
    }
    else
    {
        //sys_vgui("catch {.x%x.c delete %xSEL}\n",
        //glist_getcanvas(glist), x);
        //if (glist->gl_owner && !glist_istoplevel(glist))
        //sys_vgui(".x%lx.c dtag %xS selected\n", glist_getcanvas(glist), x);
        //sys_vgui(".x%lx.c dtag %xMT selected\n", glist_getcanvas(glist), x);
        gui_vmess("gui_gobj_deselect", "xx", glist_getcanvas(glist), x);
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
    binbuf_addv(b, "ssiissi", gensym("#X"), gensym("obj"),
                x->x_obj.te_xpix, x->x_obj.te_ypix,   
                atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)),
                x->x_fname, x->x_gop_spill);
    binbuf_addv(b, ";");
}

static t_widgetbehavior image_widgetbehavior;

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

static int image_newclick(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    //printf("doit=%d\n", doit);
    t_image *x = (t_image *)z;
    if (doit && x->x_click)
        outlet_bang(x->x_obj.ob_outlet);
    // LATER: figure out how to do click on and click off
    // and provide a toggle button behavior insteadS
    /*{
        x->x_clicked = 1;
        outlet_float(x->x_obj.ob_outlet, x->x_clicked);
    }
    else if (x->x_clicked)
    {
        x->x_clicked = 0;
        outlet_float(x->x_obj.ob_outlet, x->x_clicked);
    }*/
    return(1);
}

static void image_click(t_image *x, t_float f)
{
    if (f == 0)
        x->x_click = 0;
    else if (f == 1)
        x->x_click = 1;
}

static void image_gop_spill(t_image* x, t_floatarg f)
{
    x->x_gop_spill = (f >= 0 ? f : 0);
    image_displace((t_gobj*)x, x->x_glist, 0.0, 0.0);
}

static void image_gop_spill_size(t_image* x, t_floatarg f)
{
    //printf("image_gop_spill_size=%d\n", (int)f);
    // we need a size of at least 3 to have a meaningful
    // selection frame around the selection box
    if ((int)f >= 3)
    {
        x->x_width = (int)f;
        x->x_height = x->x_width;
        image_displace((t_gobj*)x, x->x_glist, 0.0, 0.0);
    }
}

static void image_open(t_image* x, t_symbol *s, t_int argc, t_atom *argv)
{
    x->x_fname = get_filename(argc, argv);
    x->x_img_width = 0;
    x->x_img_height = 0;
    t_symbol *fname = image_trytoopen(x);
    if (fname) {
        gui_vmess("gui_load_image", "xxs",
            glist_getcanvas(x->x_glist), x, fname->s_name);
    }
    else
    {
        gui_vmess("gui_load_default_image", "xx",
            glist_getcanvas(x->x_glist), x);
    }
    if (glist_isvisible(glist_getcanvas(x->x_glist)))
    {
        gui_vmess("gui_image_configure", "xxxs",
            glist_getcanvas(x->x_glist),
            x,
            x,
            "center");
        gui_vmess("gui_image_size_callback", "xxs",
            glist_getcanvas(x->x_glist), x, x->x_receive->s_name);
    }
    //image_vis((t_gobj *)x, x->x_glist, 0);
    //image_vis((t_gobj *)x, x->x_glist, 1);
}

static void image_imagesize_callback(t_image *x, t_float w, t_float h) {
    //fprintf(stderr,"received w %f h %f should %d spill %d\n", w, h, gobj_shouldvis((t_gobj *)x, glist_getcanvas(x->x_glist)), x->x_gop_spill);
    x->x_img_width = w;
    x->x_img_height = h;
    if (x->x_img_width + x->x_img_height == 0)
    {
        //invalid image
        if (strcmp("@pd_extra/ggee/empty_image.png", x->x_fname->s_name) != 0)
        {
            x->x_fname = gensym("@pd_extra/ggee/empty_image.png");
            image_trytoopen(x);
            return;
        }
    }
    if (!gobj_shouldvis((t_gobj *)x, x->x_glist) && !x->x_gop_spill)
    {
            //fprintf(stderr,"erasing\n");
            image_erase(x, glist_getcanvas(x->x_glist));
    }
    else
    {
        //sys_vgui("catch {.x%x.c delete %xMT}\n", glist_getcanvas(x->x_glist), x);
        // reselect if we are on a toplevel canvas to adjust the selection rectangle, if necessary

        gui_vmess("gui_image_draw_border", "xxiiii",
            glist_getcanvas(x->x_glist),
            x,
            0 - x->x_img_width/2,
            0 - x->x_img_height/2,
            x->x_img_width,
            x->x_img_height);

        if (glist_isselected(x->x_glist, (t_gobj *)x) && glist_getcanvas(x->x_glist) == x->x_glist)
        {
            image_select((t_gobj *)x, glist_getcanvas(x->x_glist), 0);
            image_select((t_gobj *)x, glist_getcanvas(x->x_glist), 1);
        }
        canvas_fixlinesfor(x->x_glist,(t_text*) x);
    }
}

static void image_setwidget(void)
{
    image_widgetbehavior.w_getrectfn = image_getrect;
    image_widgetbehavior.w_displacefn = image_displace;
    image_widgetbehavior.w_selectfn = image_select;
    image_widgetbehavior.w_activatefn = image_activate;
    image_widgetbehavior.w_deletefn = image_delete;
    image_widgetbehavior.w_visfn = image_vis;
    image_widgetbehavior.w_clickfn = image_newclick;
    image_widgetbehavior.w_displacefnwtag = image_displace_wtag;
}

static void image_free(t_image *x)
{
    //sys_vgui("image delete img%x\n", x);
    gui_vmess("gui_image_free", "x", x);
    if (x->x_receive)
    {
        pd_unbind(&x->x_obj.ob_pd,x->x_receive);
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
    x->x_click = 0;
    //x->x_clicked = 0;
    //x->x_selected = 0;
    x->x_fname = get_filename(argc, argv);
    if (strlen(x->x_fname->s_name) > 0)
    {
        //fprintf(stderr,"get_filename succeeded <%s> <%s>\n", x->x_fname->s_name, atom_getsymbol(argv)->s_name);
        argc--;
        argv++;
    }
    if (argc && argv[0].a_type == A_FLOAT)
    {
        //we have optional gop_spill flag first
        //fprintf(stderr,"gop_spill succeeded\n");
        x->x_gop_spill = (int)atom_getfloat(&argv[0]);
        argc--;
        argv++;
    }
    // Create default receiver
    char buf[MAXPDSTRING];
    sprintf(buf, "#%lx", (long)x);
    x->x_receive = gensym(buf);
    pd_bind(&x->x_obj.ob_pd, x->x_receive);
    outlet_new(&x->x_obj, &s_bang);
    //outlet_new(&x->x_obj, &s_float);
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
    class_addmethod(image_class, (t_method)image_click, gensym("click"),
        A_DEFFLOAT, 0);
    class_addmethod(image_class, (t_method)image_open, gensym("open"),
        A_GIMME, 0);
    class_addmethod(image_class, (t_method)image_gop_spill, gensym("gopspill"),
        A_DEFFLOAT, 0);
    class_addmethod(image_class, (t_method)image_gop_spill_size, gensym("gopspill_size"),
        A_DEFFLOAT, 0);
    class_addmethod(image_class, (t_method)image_imagesize_callback,\
                     gensym("_imagesize"), A_DEFFLOAT, A_DEFFLOAT, 0);

    image_setwidget();
    class_setwidget(image_class,&image_widgetbehavior);
    class_setsavefn(image_class,&image_save);
}
