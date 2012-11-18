/*
 *   Pd OpenCV module
 *   Copyright (c) by Yves Degoyon ( ydegoyon@free.fr )
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

/*  This object is a color filter 
 */

#include "pdp.h"
#include <math.h>
#include <stdio.h>
#include "g_canvas.h"

typedef struct pdp_opencv_colorfilt_struct
{
    t_object x_obj;

    int x_packet0;
    int x_packet1;
    int x_queue_id;
    int x_dropped;

    int x_width;
    int x_height;
    int x_size;
    unsigned char x_colorR; // RGB components of binary mask
    unsigned char x_colorG;
    unsigned char x_colorB;
    int x_tolerance; // tolerance 
    unsigned char *x_frame;  // keep a copy of current frame for picking color

    t_outlet *x_pdp_output; // output packets
    t_outlet *x_R;  // output R component of selected color
    t_outlet *x_G;  // output G component of selected color
    t_outlet *x_B;  // output B component of selected color

    t_canvas *x_canvas;

} t_pdp_opencv_colorfilt;

static void pdp_opencv_colorfilt_draw_color(t_pdp_opencv_colorfilt *x)
{
 int width, height;
 char color[32];

    sprintf( color, "#%.2X%.2X%.2X", x->x_colorR, x->x_colorG, x->x_colorB );
    width = rtext_width( glist_findrtext( (t_glist*)x->x_canvas, (t_text *)x ) );
    height = rtext_height( glist_findrtext( (t_glist*)x->x_canvas, (t_text *)x ) );
    sys_vgui((char*)".x%x.c delete rectangle %xCOLOR\n", x->x_canvas, x );
    sys_vgui((char*)".x%x.c create rectangle %d %d %d %d -fill %s -tags %xCOLOR\n",
             x->x_canvas, x->x_obj.te_xpix+width+5, x->x_obj.te_ypix,
             x->x_obj.te_xpix+width+height+5,
             x->x_obj.te_ypix+height, color, x );
}


static void pdp_opencv_colorfilt_r(t_pdp_opencv_colorfilt *x, t_floatarg fr )
{
   if ( fr <= 255. && fr >= 0. )
   {
      x->x_colorR = (int)fr;
      outlet_float( x->x_R, x->x_colorR );
      if (glist_isvisible(x->x_canvas)) pdp_opencv_colorfilt_draw_color( x );
   }
}

static void pdp_opencv_colorfilt_g(t_pdp_opencv_colorfilt *x, t_floatarg fg )
{
   if ( fg <= 255. && fg >= 0. )
   {
      x->x_colorG = (int)fg;
      outlet_float( x->x_G, x->x_colorG );
      if (glist_isvisible(x->x_canvas)) pdp_opencv_colorfilt_draw_color( x );
   }
}

static void pdp_opencv_colorfilt_b(t_pdp_opencv_colorfilt *x, t_floatarg fb )
{
   if ( fb < 255 && fb >= 0. )
   {
      x->x_colorB = (int)fb;
      outlet_float( x->x_B, x->x_colorB );
      if (glist_isvisible(x->x_canvas)) pdp_opencv_colorfilt_draw_color( x );
   }
}

static void pdp_opencv_colorfilt_tolerance(t_pdp_opencv_colorfilt *x, t_floatarg ftolerance )
{
   if ( ftolerance >= 0 ) 
   {
      x->x_tolerance = (int)ftolerance;
   }
}

static void pdp_opencv_colorfilt_pick(t_pdp_opencv_colorfilt *x, t_floatarg px, t_floatarg py )
{
   if ( x->x_frame && ( px >= 0. ) && ( px <= 1.0 ) 
        && ( py > 0. ) && ( py < 1.0 ) )
   {
      int xcur = px*x->x_width;
      int ycur = py*x->x_height;

      x->x_colorR = x->x_frame[(ycur*x->x_width*3)+xcur*3];
      x->x_colorG = x->x_frame[(ycur*x->x_width*3)+xcur*3+1];
      x->x_colorB = x->x_frame[(ycur*x->x_width*3)+xcur*3+2];
      outlet_float( x->x_R, x->x_colorR );
      outlet_float( x->x_G, x->x_colorG );
      outlet_float( x->x_B, x->x_colorB );
      post( "pdp_opencv_colorfilt : picked up color : x=%d y=%d r=%d g=%d b=%d", xcur, ycur, x->x_colorR, x->x_colorG, x->x_colorB );

      if (glist_isvisible(x->x_canvas)) pdp_opencv_colorfilt_draw_color( x );
   }
}

static void pdp_opencv_colorfilt_allocate(t_pdp_opencv_colorfilt *x)
{
    x->x_frame = (unsigned char *) getbytes ( x->x_size*3*sizeof(unsigned char) );

    if ( !x->x_frame )
    {
       post( "pdp_binary : severe error : cannot allocate buffer !!! ");
       return;
    }
}

static void pdp_opencv_colorfilt_free_ressources(t_pdp_opencv_colorfilt *x)
{
    if ( x->x_frame ) freebytes ( x->x_frame, x->x_size*3*sizeof(unsigned char) );
}

static void pdp_opencv_colorfilt_process_rgb(t_pdp_opencv_colorfilt *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    unsigned char *data   = (unsigned char *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    unsigned char *newdata = (unsigned char *)pdp_packet_data(x->x_packet1);
    int     i;
    int     px=0, py=0; 
    unsigned char r=0, g=0, b=0;
    unsigned char *pf;
    int     diff;

    /* allocate all ressources */
    if ( ( (int)header->info.image.width != x->x_width ) ||
         ( (int)header->info.image.height != x->x_height ) )
    {
        pdp_opencv_colorfilt_free_ressources( x ); 
        x->x_width = header->info.image.width;
        x->x_height = header->info.image.height;
        x->x_size = x->x_width*x->x_height;
        pdp_opencv_colorfilt_allocate( x ); 
        post( "pdp_opencv_colorfilt : reallocated buffers" );
    }

    memcpy(x->x_frame, data, x->x_size*3*sizeof(unsigned char));
    memcpy(newdata, data, x->x_size*3*sizeof(unsigned char));

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    // filter
    pf = newdata;
    for ( py=0; py<x->x_height; py++ )
    {
      for ( px=0; px<x->x_width; px++ )
      {
         r = *(pf);
         g = *(pf+1);
         b = *(pf+2);

         diff = 0;
         diff = abs(r-x->x_colorR );
         diff += abs(g-x->x_colorG );
         diff += abs(b-x->x_colorB );
       
         if ( diff > x->x_tolerance )
         {
            *(pf) = 0x0;
            *(pf+1) = 0x0;
            *(pf+2) = 0x0;
         }
            
         pf+=3;
       }
    }

    return;
}

static void pdp_opencv_colorfilt_sendpacket(t_pdp_opencv_colorfilt *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet1);
}

static void pdp_opencv_colorfilt_process(t_pdp_opencv_colorfilt *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_opencv_colorfilt_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_colorfilt_process_rgb, (void*)pdp_opencv_colorfilt_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_colorfilt_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_colorfilt_input_0(t_pdp_opencv_colorfilt *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        pdp_opencv_colorfilt_process(x);
    }
}

static void pdp_opencv_colorfilt_free(t_pdp_opencv_colorfilt *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
    pdp_opencv_colorfilt_free_ressources( x );
}

t_class *pdp_opencv_colorfilt_class;

void *pdp_opencv_colorfilt_new(void)
{
    int i;

    t_pdp_opencv_colorfilt *x = (t_pdp_opencv_colorfilt *)pd_new(pdp_opencv_colorfilt_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("R"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("G"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("B"));

    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything); 
    x->x_R  = outlet_new(&x->x_obj, &s_float); 
    x->x_G  = outlet_new(&x->x_obj, &s_float); 
    x->x_B  = outlet_new(&x->x_obj, &s_float); 

    x->x_colorR = 128;
    x->x_colorG = 128;
    x->x_colorB = 128;

    outlet_float( x->x_R, x->x_colorR );
    outlet_float( x->x_G, x->x_colorG );
    outlet_float( x->x_B, x->x_colorB );

    x->x_packet0 = -1;
    x->x_packet1 = -1;

    x->x_tolerance = 50;

    x->x_width = -1;
    x->x_height = -1;
    x->x_size = -1;
    x->x_frame = NULL;

    x->x_canvas = canvas_getcurrent();

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_colorfilt_setup(void)
{
    pdp_opencv_colorfilt_class = class_new(gensym("pdp_opencv_colorfilt"), (t_newmethod)pdp_opencv_colorfilt_new,
    	(t_method)pdp_opencv_colorfilt_free, sizeof(t_pdp_opencv_colorfilt), 0, A_NULL);

    class_addmethod(pdp_opencv_colorfilt_class, (t_method)pdp_opencv_colorfilt_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_colorfilt_class, (t_method)pdp_opencv_colorfilt_r, gensym("R"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_opencv_colorfilt_class, (t_method)pdp_opencv_colorfilt_g, gensym("G"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_opencv_colorfilt_class, (t_method)pdp_opencv_colorfilt_b, gensym("B"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_opencv_colorfilt_class, (t_method)pdp_opencv_colorfilt_pick, gensym("pick"), A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(pdp_opencv_colorfilt_class, (t_method)pdp_opencv_colorfilt_tolerance, gensym("tolerance"), A_FLOAT, A_NULL);

    class_sethelpsymbol( pdp_opencv_colorfilt_class, gensym("pdp_opencv_colorfilt.pd") );
}

#ifdef __cplusplus
}
#endif
