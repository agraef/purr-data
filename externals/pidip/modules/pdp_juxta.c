/*
 *   PiDiP module.
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

/*  This object is an object allowing juxtaposition of frames from two inlets
 *  Written by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

static char   *pdp_juxta_version = "pdp_juxta: version 0.1, frames juxtaposition, written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_juxta_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_packet0;
    int x_packet1;
    int x_packet;
    int x_dropped;
    int x_queue_id;

    int x_vwidth0;
    int x_vheight0;
    int x_vsize0;

    int x_vwidth1;
    int x_vheight1;
    int x_vsize1;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

} t_pdp_juxta;

static void pdp_juxta_process_yv12(t_pdp_juxta *x)
{
    t_pdp     *header0 = pdp_packet_header(x->x_packet0);
    short int *data0   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *header1 = pdp_packet_header(x->x_packet1);
    short int *data1 = (short int *)pdp_packet_data(x->x_packet1);
    t_pdp     *header;
    short int *data;
    int       i;
    short int *poY, *poV, *poU, *p0Y, *p0V, *p0U, *p1Y, *p1V, *p1U;

    int px, py;

    if ( header0 )
    {
      x->x_vwidth0 = header0->info.image.width;
      x->x_vheight0 = header0->info.image.height;
      x->x_vsize0 = x->x_vwidth0*x->x_vheight0;
    }
    else
    {
      x->x_vwidth0 = x->x_vheight0 = x->x_vsize0 = 0;
    }

    if ( header1 )
    {
      x->x_vwidth1 = header1->info.image.width;
      x->x_vheight1 = header1->info.image.height;
      x->x_vsize1 = x->x_vwidth1*x->x_vheight1;
    }
    else
    {
      x->x_vwidth1 = x->x_vheight1 = x->x_vsize1 = 0;
    }

    x->x_vwidth = x->x_vwidth0 + x->x_vwidth1;
    if ( x->x_vheight0 > x->x_vheight1 )
    {
      x->x_vheight = x->x_vheight0;
    }
    else
    {
      x->x_vheight = x->x_vheight1;
    }
    x->x_vsize = x->x_vwidth*x->x_vheight;
    // post( "pdp_juxta : resulting frame : %dx%d", x->x_vwidth, x->x_vheight );

    x->x_packet = pdp_packet_new_image_YCrCb( x->x_vwidth, x->x_vheight );

    header = pdp_packet_header(x->x_packet);
    data = (short int *)pdp_packet_data(x->x_packet);

    header->info.image.encoding = PDP_IMAGE_YV12;
    header->info.image.width = x->x_vwidth;
    header->info.image.height = x->x_vheight;

    poY = data;
    poV = data+x->x_vsize;
    poU = data+x->x_vsize+(x->x_vsize>>2);
    p0Y = data0;
    p0V = data0+x->x_vsize0;
    p0U = data0+x->x_vsize0+(x->x_vsize0>>2);
    p1Y = data1;
    p1V = data1+x->x_vsize1;
    p1U = data1+x->x_vsize1+(x->x_vsize1>>2);
    for ( py=0; py<x->x_vheight; py++ )
    {
      for ( px=0; px<x->x_vwidth; px++ )
      {
         if ( px < x->x_vwidth0 && p0Y )
         {
            *poY = *p0Y;
            *poV = *p0V;
            *poU = *p0U; 
            poY++;p0Y++;
            if ( (px%2==0) && (py%2==0) )
            {
               poU++; poV++;
               p0U++; p0V++;
            }
         }
         if ( px >= x->x_vwidth0 && p1Y )
         {
            *poY = *p1Y;
            *poV = *p1V;
            *poU = *p1U; 
            poY++;p1Y++;
            if ( (px%2==0) && (py%2==0) )
            {
               poU++; poV++;
               p1U++; p1V++;
            }
         }
      }
    }

    return;
}

static void pdp_juxta_sendpacket0(t_pdp_juxta *x)
{
    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet);
}

static void pdp_juxta_sendpacket1(t_pdp_juxta *x)
{
    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet);
}

static void pdp_juxta_process0(t_pdp_juxta *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_juxta_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            pdp_queue_add(x, pdp_juxta_process_yv12, pdp_juxta_sendpacket0, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_juxta_process */
	    break;
	    
	}
    }
}

static void pdp_juxta_process1(t_pdp_juxta *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet1))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_juxta_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet1)->info.image.encoding){

	case PDP_IMAGE_YV12:
            pdp_queue_add(x, pdp_juxta_process_yv12, pdp_juxta_sendpacket1, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_juxta_process */
	    break;
	    
	}
    }
}

static void pdp_juxta_input_0(t_pdp_juxta *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
    {
      /* release the packet */
      if ( x->x_packet0 != -1 )
      {
        pdp_packet_mark_unused(x->x_packet0);
        x->x_packet0 = -1;
      }
      x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_juxta_process0(x);

    }
}

static void pdp_juxta_input_1(t_pdp_juxta *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))  
    {
      /* release the packet */
      if ( x->x_packet1 != -1 )
      {
        pdp_packet_mark_unused(x->x_packet1);
        x->x_packet1 = -1;
      }
      x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet1, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet1) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_juxta_process1(x);

    }
}

static void pdp_juxta_free(t_pdp_juxta *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_packet_mark_unused(x->x_packet1);
}

t_class *pdp_juxta_class;

void *pdp_juxta_new(void)
{
    int i;

    t_pdp_juxta *x = (t_pdp_juxta *)pd_new(pdp_juxta_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("pdp"),  gensym("pdp1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("pdp"),  gensym("pdp2"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_packet = -1;
    x->x_queue_id = -1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_juxta_setup(void)
{
    // post( pdp_juxta_version );
    pdp_juxta_class = class_new(gensym("pdp_juxta"), (t_newmethod)pdp_juxta_new,
    	(t_method)pdp_juxta_free, sizeof(t_pdp_juxta), CLASS_NOINLET, A_NULL);

    class_addmethod(pdp_juxta_class, (t_method)pdp_juxta_input_0, gensym("pdp1"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_juxta_class, (t_method)pdp_juxta_input_1, gensym("pdp2"),  A_SYMBOL, A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
