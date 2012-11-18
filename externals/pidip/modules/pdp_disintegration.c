/*
 *   PiDiP module
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

/*  This object is a piksels sum-up operator
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <stdio.h>

static char   *pdp_disintegration_version = "pdp_disintegration: piksels sum-up version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_disintegration_struct
{
    t_object x_obj;

    int x_packet0;
    int x_packet1;
    int x_queue_id;
    int x_dropped;

    int x_vwidth;
    int x_vheight;
    int x_vsize;
    int x_nbpasses;  // number of passes
    int x_reductor; // fraction reductor
    short int *x_frame;  // keep a copy of current frame for transformations

    t_outlet *x_pdp_output; // output packets

} t_pdp_disintegration;

static void pdp_disintegration_nbpasses(t_pdp_disintegration *x,  t_floatarg fpasses )
{
   if ( fpasses>=1.) 
   {
      x->x_nbpasses = (int)fpasses;
   }
}

static void pdp_disintegration_reductor(t_pdp_disintegration *x,  t_floatarg freductor )
{
   if ( freductor>=1.) 
   {
      x->x_reductor = (int)freductor;
   }
}

static void pdp_disintegration_allocate(t_pdp_disintegration *x)
{
    x->x_frame = (short int *) getbytes ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );

    if ( !x->x_frame )
    {
       post( "pdp_disintegration : severe error : cannot allocate buffer !!! ");
       return;
    }
}

static void pdp_disintegration_free_ressources(t_pdp_disintegration *x)
{
    if ( x->x_frame ) freebytes ( x->x_frame, ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );
}

static void pdp_disintegration_process_yv12(t_pdp_disintegration *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i;
    int     px=0, py=0; 
    short int *pfY, *pfU, *pfV;
    int     ppx, ppy, ix, iy, pn;
    int     nvalue;

    // allocate all ressources
    if ( ( (int)header->info.image.width != x->x_vwidth ) ||
         ( (int)header->info.image.height != x->x_vheight ) )
    {
        pdp_disintegration_free_ressources( x );
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_disintegration_allocate( x );
        post( "pdp_disintegration : reallocated buffers" );
    }

    // post( "pdp_disintegration : newheader:%x", newheader );

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, x->x_vsize+(x->x_vsize>>1)<<1 );
    memcpy( x->x_frame, data, x->x_vsize+(x->x_vsize>>1)<<1 );

    pfY = x->x_frame;
    pfV = x->x_frame+x->x_vsize;
    pfU = x->x_frame+x->x_vsize+(x->x_vsize>>2);
    
    // foreground piksels are now 1
    for ( py=0; py<x->x_vheight; py++ )
    {
      for ( px=0; px<x->x_vwidth; px++ )
      {
         if ( *(pfY+py*x->x_vwidth+px) == ((255)<<7) )
         {
           *(pfY+py*x->x_vwidth+px) = ((1)<<7); 
         }
      }
    }

    for ( pn=0; pn<x->x_nbpasses; pn++ )
    {
      memcpy( x->x_frame, newdata, x->x_vsize+(x->x_vsize>>1)<<1 );
      for ( py=0; py<x->x_vheight; py++ )
      {
        for ( px=0; px<x->x_vwidth; px++ )
        {
          nvalue = 0;
          for (ix=-1; ix<=1; ix++)
          {
            ppx=px+ix;
            if ( (ppx>=0) && (ppx<x->x_vwidth) )
            {
              for (iy=-1; iy<=1; iy++)
              {
                ppy=py+iy;
                if ( (ppy>=0) && (ppy<x->x_vheight) )
                {
                   nvalue += *(pfY+ppy*x->x_vwidth+ppx);
                }
              }
            }
          }
          if ( nvalue > ((255)<<7)*9 )
          {
            *(newdata+py*x->x_vwidth+px) = ((255)<<7);
          }
          else
          {
            *(newdata+py*x->x_vwidth+px) = nvalue/x->x_reductor;
          }
        }
      }
    }

    return;
}

static void pdp_disintegration_sendpacket(t_pdp_disintegration *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet1);
}

static void pdp_disintegration_process(t_pdp_disintegration *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_disintegration_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_disintegration_process_yv12, pdp_disintegration_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_disintegration_process */
	    break;
	    
	}
    }

}

static void pdp_disintegration_input_0(t_pdp_disintegration *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        pdp_disintegration_process(x);
    }
}

static void pdp_disintegration_free(t_pdp_disintegration *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
    pdp_disintegration_free_ressources( x );
}

t_class *pdp_disintegration_class;

void *pdp_disintegration_new(void)
{
    int i;

    t_pdp_disintegration *x = (t_pdp_disintegration *)pd_new(pdp_disintegration_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("nbpasses"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("reductor"));

    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_vwidth = -1;
    x->x_vheight = -1;
    x->x_vsize = -1;
    x->x_frame = NULL;
    x->x_nbpasses = 3;
    x->x_reductor = 5;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_disintegration_setup(void)
{
    // post( pdp_disintegration_version );
    pdp_disintegration_class = class_new(gensym("pdp_disintegration"), (t_newmethod)pdp_disintegration_new,
    	(t_method)pdp_disintegration_free, sizeof(t_pdp_disintegration), 0, A_NULL);

    class_addmethod(pdp_disintegration_class, (t_method)pdp_disintegration_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_disintegration_class, (t_method)pdp_disintegration_nbpasses, gensym("nbpasses"), A_FLOAT, A_NULL);
    class_addmethod(pdp_disintegration_class, (t_method)pdp_disintegration_reductor, gensym("reductor"), A_FLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
