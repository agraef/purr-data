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

/*  This object turns an image into sound
 */

/*  Listening to :
 *  The Deviants - Nothing Man
 *  90 day men - My Trip To Venus
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <ctype.h>
#include <Imlib2.h>  // imlib2 is required

static char   *pdp_segsnd_version = "pdp_segsnd~: version 0.1 : turns an image into sound written by ydegoyon@free.fr ";

typedef struct pdp_segsnd_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

    int x_x1;  // coordinates of fixed segment
    int x_y1;
    int x_x2;
    int x_y2;
    int x_random;

    short int *x_data;

    /* imlib data */
    Imlib_Image x_image;

} t_pdp_segsnd;

static void pdp_segsnd_x1(t_pdp_segsnd *x, t_floatarg fx )
{
    if ( ( fx >= 0 ) && ( fx < x->x_x2 ) )
    {
       x->x_x1 = fx;
    }
}

static void pdp_segsnd_y1(t_pdp_segsnd *x, t_floatarg fy )
{
    if ( ( fy >= 0 ) && ( fy < x->x_y2 ) )
    {
       x->x_y1 = fy;
    }
}

static void pdp_segsnd_x2(t_pdp_segsnd *x, t_floatarg fx )
{
    if ( ( fx >= x->x_x1 ) && ( fx < x->x_vwidth ) )
    {
       x->x_x2 = fx;
    }
}

static void pdp_segsnd_y2(t_pdp_segsnd *x, t_floatarg fy )
{
    if ( ( fy >= x->x_y1 ) && ( fy < x->x_vheight ) )
    {
       x->x_y2 = fy;
    }
}

static void pdp_segsnd_random(t_pdp_segsnd *x, t_floatarg frand )
{
    if ( ( frand == 0 ) || ( frand == 1 ) )
    {
       x->x_random = frand;
    }
}

static void pdp_segsnd_allocate(t_pdp_segsnd *x)
{
   x->x_image = imlib_create_image( x->x_vwidth, x->x_vheight );
   if ( x->x_image == NULL )
   {
      post( "pdp_form : severe error : could not allocate image !!" );
   }
   imlib_context_set_image(x->x_image);
   x->x_data = (short int *)getbytes((( x->x_vsize + (x->x_vsize>>1))<<1));
}

static void pdp_segsnd_free_ressources(t_pdp_segsnd *x)
{
   // if ( x->x_image != NULL ) imlib_free_image();
   x->x_image = NULL;
   if ( x->x_data ) freebytes( x->x_data, (( x->x_vsize + (x->x_vsize>>1))<<1));
   x->x_data = NULL;
}

static void pdp_segsnd_process_yv12(t_pdp_segsnd *x)
{
  t_pdp     *header = pdp_packet_header(x->x_packet0);
  short int *data   = (short int *)pdp_packet_data(x->x_packet0);
  t_pdp     *newheader = pdp_packet_header(x->x_packet1);
  short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
  int     ti;
  int     px, py;
  unsigned char y, u, v;
  short int *pY, *pU, *pV;
  DATA32    *imdata;
  DATA32    bgcolor;

    if ( ( (int)(header->info.image.width) != x->x_vwidth ) ||
         ( (int)(header->info.image.height) != x->x_vheight ) )
    {
         pdp_segsnd_free_ressources(x);
         x->x_vwidth = header->info.image.width;
         x->x_vheight = header->info.image.height;
         x->x_vsize = x->x_vwidth*x->x_vheight;
         pdp_segsnd_allocate(x);
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, (x->x_vsize+(x->x_vsize>>1))<<1 );
    memcpy( x->x_data, data, ((x->x_vsize+(x->x_vsize>>1))<<1));

    if ( x->x_image != NULL ) imlib_context_set_image(x->x_image);
    imlib_image_clear();
    imlib_context_set_direction(IMLIB_TEXT_TO_ANGLE);
    imdata = imlib_image_get_data();
    bgcolor = imdata[0];

    // post( "pdp_segsnd : x1=%d y1=%d x2=%d y2=%d", x->x_x1, x->x_y1, x->x_x2, x->x_y2 );
    if ( x->x_x1 != -1 )
    {
      imlib_context_set_color( 255, 255, 255, 255 );
      imlib_image_draw_line( x->x_x1, x->x_y1, x->x_x2, x->x_y2, 1);

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

            *(pY) = y<<7;
            if ( (px%2==0) && (py%2==0) )
            {
              *(pV) = (v-128)<<8;
              *(pU) = (u-128)<<8;
            }
          }
          pY++;
          if ( (px%2==0) && (py%2==0) )
          {
            pV++;pU++;
          }
       }
      }
      if ( x->x_random )
      {
         x->x_x2 = ((t_float)rand()/RAND_MAX)*x->x_vwidth;
         x->x_x1 = ((t_float)rand()/RAND_MAX)*x->x_x2;
         x->x_y2 = ((t_float)rand()/RAND_MAX)*x->x_vheight;
         x->x_y1 = ((t_float)rand()/RAND_MAX)*x->x_y2;
      }
    }

    return;
}

static void pdp_segsnd_sendpacket(t_pdp_segsnd *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_segsnd_process(t_pdp_segsnd *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type))
   {
    
	/* pdp_segsnd_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_segsnd_process_yv12, pdp_segsnd_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_segsnd_process */
	    break;
	    
	}
    }

}

static void pdp_segsnd_input_0(t_pdp_segsnd *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))  
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }
    // post( "pdp_segsnd : action=%s dropped=%d", s->s_name, x->x_dropped );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {

        /* add the process method and callback to the process queue */
        pdp_segsnd_process(x);

    }

}

static void pdp_segsnd_free(t_pdp_segsnd *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

static t_int *pdp_segsnd_perform(t_int *w)
{
  t_float *out   = (t_float *)(w[1]);       // audio generated sound
  t_pdp_segsnd *x = (t_pdp_segsnd *)(w[2]);
  int n = (int)(w[3]); 
  int npoints, xi, px, py;
  t_float a=0;

   // set initial coordinates
   if ( ( x->x_x1 == -1 ) && (x->x_vwidth != -1 ))
   {
     x->x_x1 = 10; 
     x->x_y1 = 10; 
     if ( 10+n > (x->x_vwidth-1) )
     {
        x->x_x2 = x->x_vwidth-1;
     }
     else
     {
        x->x_x2 = 10+n;
     }
     if ( 10+n > (x->x_vheight-1) )
     {
        x->x_y2 = x->x_vheight-1;
     }
     else
     {
        x->x_y2 = 10+n;
     }
   }
   // post( "pdp_segsnd : x1=%d y1=%d x2=%d y2=%d", x->x_x1, x->x_y1, x->x_x2, x->x_y2 );

   // output image data
   if ( x->x_x1 == -1 ) 
   {
      npoints = 0;
   }
   else if ( x->x_x2-x->x_x1 > n )
   {
      npoints = n;
   }
   else
   {
      npoints = x->x_x2-x->x_x1;
   }
   if ( (x->x_x2-x->x_x1) > 0 )
   {
      a = (x->x_y2-x->x_y1)/(x->x_x2-x->x_x1);
   } 
   // post( "pdp_segsnd : npoints=%d a=%f", npoints, a );
   // read pixels
   for (xi=0; xi<npoints; xi++)
   {
      px = x->x_x1 + xi; 
      py = x->x_y1 + (int)(a*xi);
      *out = (((t_float)(x->x_data[py*x->x_vwidth+px]>>7))-127)/128.0; // scaled to -1 ... 1
      out++;
   }
   // fill up with zeros
   for (xi=npoints; xi<n; xi++)
   {
       *out++ = 0.0;
   }

   return (w+4);
}

static void pdp_segsnd_dsp(t_pdp_segsnd *x, t_signal **sp)
{
    dsp_add(pdp_segsnd_perform, 3, sp[0]->s_vec, x, sp[0]->s_n);
}

t_class *pdp_segsnd_class;

void *pdp_segsnd_new(void)
{
    int i;

    t_pdp_segsnd *x = (t_pdp_segsnd *)pd_new(pdp_segsnd_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("x1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("y1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("x2"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("y2"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("random"));

    // pdp output
    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);

    // sound output
    outlet_new (&x->x_obj, &s_signal);

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_vwidth = -1;
    x->x_vheight = -1;

    x->x_image = NULL;
    x->x_data = NULL;
    x->x_x1 = -1;
    x->x_y1 = -1;
    x->x_x2 = -1;
    x->x_y2 = -1;
    x->x_random = 0;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_segsnd_tilde_setup(void)
{

    // post( pdp_segsnd_version );
    pdp_segsnd_class = class_new(gensym("pdp_segsnd~"), (t_newmethod)pdp_segsnd_new,
    	(t_method)pdp_segsnd_free, sizeof(t_pdp_segsnd), 0, A_NULL);

    class_addmethod(pdp_segsnd_class, (t_method)pdp_segsnd_dsp, gensym("dsp"), 0);
    class_addmethod(pdp_segsnd_class, (t_method)pdp_segsnd_input_0, gensym("pdp"),  
                             A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_segsnd_class, (t_method)pdp_segsnd_x1, gensym("x1"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_segsnd_class, (t_method)pdp_segsnd_y1, gensym("y1"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_segsnd_class, (t_method)pdp_segsnd_x2, gensym("x2"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_segsnd_class, (t_method)pdp_segsnd_y2, gensym("y2"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_segsnd_class, (t_method)pdp_segsnd_random, gensym("random"),  A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
