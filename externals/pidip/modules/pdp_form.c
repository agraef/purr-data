/*
 *   PiDiP module
 *   Copyright (c) by Yves Degoyon (ydegoyon@free.fr)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*  This object is a geometric forms generator object for PDP
 *  It uses imlib2 for all graphical operations
 */

/*  Listening to :
 *  Culturcide - Bruce
 *  This Heat - Independence
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <ctype.h>
#include <Imlib2.h>  // imlib2 is required


/* forms type */
enum _form_type
{
   IMLIB_LINE,
   IMLIB_RECTANGLE,
   IMLIB_ELLIPSE
};
typedef enum _form_type t_form_type;

typedef struct _form
{
    t_form_type type;
    int n1,n2,n3,n4; // numerical coordinates or rays
    int r,g,b;
} t_form;


#define DEFAULT_CAPACITY 10

static char   *pdp_form_version = "pdp_form: version 0.1 : forms rendering object written by ydegoyon@free.fr ";

typedef struct pdp_form_struct
{
    t_object x_obj;
    t_float x_f;

    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    t_outlet *x_outlet0;
    int x_vwidth;
    int x_vheight;
    int x_vsize;

    t_form *x_forms;
    int x_nbforms;
    int x_current;
    int x_capacity;
    t_float x_alpha;

        /* imlib data */
    Imlib_Image x_image;

} t_pdp_form;

        /* add a line */
static void pdp_form_line(t_pdp_form *x, t_symbol *s, int argc, t_atom *argv)
{
   if ( x->x_nbforms >= x->x_capacity )
   {
      post( "pdp_form : sorry, maximum capacity has been reached... try resize" );
      return;
   } 
   
   if ( argc < 4 )
   {
      post( "pdp_form : error in the number of arguments ( minimum is 4 )", argc );
      return;
   }
   if ( argv[0].a_type != A_FLOAT || argv[1].a_type != A_FLOAT || 
        argv[2].a_type != A_FLOAT || argv[3].a_type != A_FLOAT ) {
      post( "pdp_form : add : wrong arguments" );
      return;
   }

   x->x_forms[x->x_nbforms].type = IMLIB_LINE;
   x->x_forms[x->x_nbforms].n1 = (int)argv[0].a_w.w_float;
   x->x_forms[x->x_nbforms].n2 = (int)argv[1].a_w.w_float;
   x->x_forms[x->x_nbforms].n3 = (int)argv[2].a_w.w_float;
   x->x_forms[x->x_nbforms].n4 = (int)argv[3].a_w.w_float;

   if ( (argc>=5) && (argv[4].a_type == A_FLOAT) )
   {
      x->x_forms[x->x_nbforms].r = (int)argv[4].a_w.w_float;
   }
   if ( (argc>=6) && (argv[5].a_type == A_FLOAT) )
   {
      x->x_forms[x->x_nbforms].g = (int)argv[5].a_w.w_float;
   }
   if ( (argc>=7) && (argv[6].a_type == A_FLOAT) )
   {
      x->x_forms[x->x_nbforms].b = (int)argv[6].a_w.w_float;
   }
   
   post( "pdp_form : added line [%d,%d,%d,%d] @ %d (r=%d g=%d b=%d)", 
                x->x_forms[x->x_nbforms].n1, x->x_forms[x->x_nbforms].n2,
                x->x_forms[x->x_nbforms].n3, x->x_forms[x->x_nbforms].n4, x->x_nbforms,
                x->x_forms[x->x_nbforms].r, x->x_forms[x->x_nbforms].g, x->x_forms[x->x_nbforms].b );

   if ( x->x_current == -1 ) x->x_current = x->x_nbforms;
   x->x_nbforms++;
   
}

        /* add a rectangle */
static void pdp_form_rectangle(t_pdp_form *x, t_symbol *s, int argc, t_atom *argv)
{
   if ( x->x_nbforms >= x->x_capacity )
   {
      post( "pdp_form : sorry, maximum capacity has been reached... try resize" );
      return;
   } 
   
   if ( argc < 4 )
   {
      post( "pdp_form : error in the number of arguments ( minimum is 4 )", argc );
      return;
   }
   if ( argv[0].a_type != A_FLOAT || argv[1].a_type != A_FLOAT || 
        argv[2].a_type != A_FLOAT || argv[3].a_type != A_FLOAT ) {
      post( "pdp_form : add : wrong arguments" );
      return;
   }

   x->x_forms[x->x_nbforms].type = IMLIB_RECTANGLE;
   x->x_forms[x->x_nbforms].n1 = (int)argv[0].a_w.w_float;
   x->x_forms[x->x_nbforms].n2 = (int)argv[1].a_w.w_float;
   x->x_forms[x->x_nbforms].n3 = (int)argv[2].a_w.w_float;
   x->x_forms[x->x_nbforms].n4 = (int)argv[3].a_w.w_float;

   if ( (argc>=5) && (argv[4].a_type == A_FLOAT) )
   {
      x->x_forms[x->x_nbforms].r = (int)argv[4].a_w.w_float;
   }
   if ( (argc>=6) && (argv[5].a_type == A_FLOAT) )
   {
      x->x_forms[x->x_nbforms].g = (int)argv[5].a_w.w_float;
   }
   if ( (argc>=7) && (argv[6].a_type == A_FLOAT) )
   {
      x->x_forms[x->x_nbforms].b = (int)argv[6].a_w.w_float;
   }
   
   post( "pdp_form : added rectangle [%d,%d,%d,%d] @ %d (r=%d g=%d b=%d)", 
                x->x_forms[x->x_nbforms].n1, x->x_forms[x->x_nbforms].n2,
                x->x_forms[x->x_nbforms].n3, x->x_forms[x->x_nbforms].n4, x->x_nbforms,
                x->x_forms[x->x_nbforms].r, x->x_forms[x->x_nbforms].g, x->x_forms[x->x_nbforms].b );

   if ( x->x_current == -1 ) x->x_current = x->x_nbforms;
   x->x_nbforms++;
   
}

        /* add an ellipse */
static void pdp_form_ellipse(t_pdp_form *x, t_symbol *s, int argc, t_atom *argv)
{
   if ( x->x_nbforms >= x->x_capacity )
   {
      post( "pdp_form : sorry, maximum capacity has been reached... try resize" );
      return;
   } 
   
   if ( argc < 4 )
   {
      post( "pdp_form : error in the number of arguments ( minimum is 4 )", argc );
      return;
   }
   if ( argv[0].a_type != A_FLOAT || argv[1].a_type != A_FLOAT ||
        argv[2].a_type != A_FLOAT || argv[3].a_type != A_FLOAT ) {
      post( "pdp_form : add : wrong arguments" );
      return;
   }

   x->x_forms[x->x_nbforms].type = IMLIB_ELLIPSE;
   x->x_forms[x->x_nbforms].n1 = (int)argv[0].a_w.w_float;
   x->x_forms[x->x_nbforms].n2 = (int)argv[1].a_w.w_float;
   x->x_forms[x->x_nbforms].n3 = (int)argv[2].a_w.w_float;
   x->x_forms[x->x_nbforms].n4 = (int)argv[3].a_w.w_float;

   if ( (argc>=5) && (argv[4].a_type == A_FLOAT) )
   {
      x->x_forms[x->x_nbforms].r = (int)argv[4].a_w.w_float;
   }
   if ( (argc>=6) && (argv[5].a_type == A_FLOAT) )
   {
      x->x_forms[x->x_nbforms].g = (int)argv[5].a_w.w_float;
   }
   if ( (argc>=7) && (argv[6].a_type == A_FLOAT) )
   {
      x->x_forms[x->x_nbforms].b = (int)argv[6].a_w.w_float;
   }
   
   post( "pdp_form : added ellipse [%d,%d,%d,%d] @ %d (r=%d g=%d b=%d)", 
                x->x_forms[x->x_nbforms].n1, x->x_forms[x->x_nbforms].n2,
                x->x_forms[x->x_nbforms].n3, x->x_forms[x->x_nbforms].n4, x->x_nbforms,
                x->x_forms[x->x_nbforms].r, x->x_forms[x->x_nbforms].g, x->x_forms[x->x_nbforms].b );

   if ( x->x_current == -1 ) x->x_current = x->x_nbforms;
   x->x_nbforms++;
   
}

static void pdp_form_current(t_pdp_form *x, t_floatarg fcurrent )
{
    if ( ( fcurrent >= 0 ) && ( fcurrent < x->x_nbforms ) )
    {
       // post( "pdp_form : current item set to %d", x->x_current );
       x->x_current = fcurrent;
    }
}

static void pdp_form_x1(t_pdp_form *x, t_floatarg fx )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbforms ) )
    {
       x->x_forms[x->x_current].n1 = fx;
    }
}

static void pdp_form_y1(t_pdp_form *x, t_floatarg fy )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbforms ) )
    {
       x->x_forms[x->x_current].n2 = fy;
    }
}

static void pdp_form_x2(t_pdp_form *x, t_floatarg fx )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbforms ) )
    {
       x->x_forms[x->x_current].n3 = fx;
    }
}

static void pdp_form_y2(t_pdp_form *x, t_floatarg fy )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbforms ) )
    {
       x->x_forms[x->x_current].n4 = fy;
    }
}

static void pdp_form_r(t_pdp_form *x, t_floatarg fr )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbforms ) )
    {
       x->x_forms[x->x_current].r = fr;
    }
}

static void pdp_form_g(t_pdp_form *x, t_floatarg fg )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbforms ) )
    {
       x->x_forms[x->x_current].g = fg;
    }
}

static void pdp_form_b(t_pdp_form *x, t_floatarg fb )
{
    if ( ( x->x_current  >= 0 ) && ( x->x_current  < x->x_nbforms ) )
    {
       x->x_forms[x->x_current].b = fb;
    }
}

static void pdp_form_clear(t_pdp_form *x )
{
    x->x_nbforms = 0;
}

static void pdp_form_delete(t_pdp_form *x,  t_floatarg fnum  )
{
  int i;
  char *lostword;

    if ( ( fnum>0 ) && ( fnum<=x->x_nbforms ) )
    {
       for ( i=(int)fnum; i<x->x_nbforms; i++ )
       {
          memcpy( &x->x_forms[ i-1 ], &x->x_forms[ i ], sizeof( t_form ) );
       }
       x->x_nbforms--;
    }
}

static void pdp_form_alpha(t_pdp_form *x, t_floatarg falpha )
{
    if ( ( falpha  >= 0. ) && ( falpha  <= 1. ) )
    {
       x->x_alpha = falpha;
    }
}

static void pdp_form_resize(t_pdp_form *x,  t_floatarg fnewsize  )
{
  t_form *forms;
  int i, csize;

    if ( (int) fnewsize<=0 ) return;

    // allocate new structures
    forms = (t_form*) getbytes( fnewsize*sizeof(t_form) );

    for ( i=0; i<fnewsize; i++ )
    {
       forms[i].r = forms[i].g = forms[i].b = 255;
    }

    if ( fnewsize < x->x_nbforms )
    {
       post( "pdp_form : new size is too small : texts lost !!" );
       csize = fnewsize;
    }
    else
    {
       csize = x->x_nbforms;
    }

    // copy all values
    for ( i=0; i<csize; i++ )
    {
        memcpy( &forms[i], &x->x_forms[i], sizeof( t_form ) );
    }
  
    // free old structures
    if ( x->x_forms ) freebytes( x->x_forms, x->x_capacity*sizeof(t_form) );

    // set new structures
    x->x_forms = forms;
    x->x_nbforms = csize;
    x->x_capacity = fnewsize;
    if ( x->x_nbforms > 0 )
    {
       x->x_current = 0;
    }
    else
    {
       x->x_current = -1;
    }
}

static void pdp_form_allocate(t_pdp_form *x)
{
   x->x_image = imlib_create_image( x->x_vwidth, x->x_vheight );
   if ( x->x_image == NULL )
   {
      post( "pdp_form : severe error : could not allocate image !!" );
   }
   imlib_context_set_image(x->x_image);
}

static void pdp_form_free_ressources(t_pdp_form *x)
{
   // if ( x->x_image != NULL ) imlib_free_image();
}

static void pdp_form_process_yv12(t_pdp_form *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     ti;
    int     px, py;
    unsigned char y, u, v;
    DATA32    *imdata;
    DATA32    bgcolor;
    short int *pY, *pU, *pV;

    if ( ( (int)(header->info.image.width) != x->x_vwidth ) ||
         ( (int)(header->info.image.height) != x->x_vheight ) )
    {
         pdp_form_free_ressources(x);
         x->x_vwidth = header->info.image.width;
         x->x_vheight = header->info.image.height;
         x->x_vsize = x->x_vwidth*x->x_vheight;
         pdp_form_allocate(x);
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, (x->x_vsize+(x->x_vsize>>1))<<1 );

    // draw all texts
    if ( x->x_image != NULL ) imlib_context_set_image(x->x_image); 
    imlib_image_clear();
    imlib_context_set_direction(IMLIB_TEXT_TO_ANGLE);
    imdata = imlib_image_get_data();
    bgcolor = imdata[0];

    for (ti=0; ti<x->x_nbforms; ti++)
    {
       imlib_context_set_color( x->x_forms[ti].r, x->x_forms[ti].g, x->x_forms[ti].b, 255 );

       switch ( x->x_forms[ti].type )
       {
          case IMLIB_LINE :
            imlib_image_draw_line( x->x_forms[ti].n1, x->x_forms[ti].n2, x->x_forms[ti].n3, x->x_forms[ti].n4, 1);
            break;

          case IMLIB_RECTANGLE :
            imlib_image_draw_rectangle( x->x_forms[ti].n1, x->x_forms[ti].n2, 
                                        x->x_forms[ti].n3-x->x_forms[ti].n1, x->x_forms[ti].n4-x->x_forms[ti].n2 );
            imlib_image_fill_rectangle( x->x_forms[ti].n1, x->x_forms[ti].n2, 
                                        x->x_forms[ti].n3-x->x_forms[ti].n1, x->x_forms[ti].n4-x->x_forms[ti].n2 );
            break;

          case IMLIB_ELLIPSE :
            imlib_image_draw_ellipse( x->x_forms[ti].n1, x->x_forms[ti].n2, x->x_forms[ti].n3, x->x_forms[ti].n4 );
            imlib_image_fill_ellipse( x->x_forms[ti].n1, x->x_forms[ti].n2, x->x_forms[ti].n3, x->x_forms[ti].n4 );
            break;
       }
    }

    pY = newdata;
    pV = newdata+x->x_vsize;
    pU = newdata+x->x_vsize+(x->x_vsize>>2);
    for ( py=0; py<x->x_vheight; py++ )
    {
       for ( px=0; px<x->x_vwidth; px++ )
       {
          if ( imdata[py*x->x_vwidth+px] != bgcolor )
          {
            y = yuv_RGBtoY(imdata[py*x->x_vwidth+px]);
            u = yuv_RGBtoU(imdata[py*x->x_vwidth+px]);
            v = yuv_RGBtoV(imdata[py*x->x_vwidth+px]);
          
            *(pY) = (y<<7)*x->x_alpha + (*pY)*(1-x->x_alpha);
            if ( (px%2==0) && (py%2==0) )
            {
              *(pV) = ((v-128)<<8)*x->x_alpha + (*pV)*(1-x->x_alpha);
              *(pU) = ((u-128)<<8)*x->x_alpha + (*pU)*(1-x->x_alpha);
            }
          }
          pY++;
          if ( (px%2==0) && (py%2==0) )
          {
            pV++;pU++;
          }
       }
    }

    return;
}

static void pdp_form_sendpacket(t_pdp_form *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_form_process(t_pdp_form *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_form_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_form_process_yv12, pdp_form_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_form_process */
	    break;
	    
	}
    }

}

static void pdp_form_input_0(t_pdp_form *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_form_process(x);

    }

}

static void pdp_form_free(t_pdp_form *x)
{
  int i;

    pdp_form_free_ressources(x);
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_form_class;

void *pdp_form_new(void)
{
    int i;

    t_pdp_form *x = (t_pdp_form *)pd_new(pdp_form_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("current"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("x1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("y1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("x2"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("y2"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("r"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("g"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("b"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("alpha"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;
    x->x_image = NULL;

    x->x_capacity = DEFAULT_CAPACITY;

    x->x_forms = (t_form *) getbytes( x->x_capacity*sizeof(t_form) );

    for ( i=0; i<x->x_capacity; i++ )
    {
       x->x_forms[i].r = x->x_forms[i].g = x->x_forms[i].b = 255;
    }

    x->x_nbforms = 0;
    x->x_current = -1;
    x->x_alpha = 1.;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_form_setup(void)
{

    // post( pdp_form_version );
    pdp_form_class = class_new(gensym("pdp_form"), (t_newmethod)pdp_form_new,
    	(t_method)pdp_form_free, sizeof(t_pdp_form), 0, A_NULL);

    class_addmethod(pdp_form_class, (t_method)pdp_form_input_0, gensym("pdp"),  
                             A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_line, gensym("line"),  A_GIMME, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_rectangle, gensym("rectangle"),  A_GIMME, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_ellipse, gensym("ellipse"),  A_GIMME, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_current, gensym("current"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_x1, gensym("x1"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_y1, gensym("y1"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_x2, gensym("x2"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_y2, gensym("y2"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_r, gensym("r"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_g, gensym("g"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_b, gensym("b"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_clear, gensym("clear"),  A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_delete, gensym("delete"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_resize, gensym("resize"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_form_class, (t_method)pdp_form_alpha, gensym("alpha"),  A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
