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

#include "pdp.h"
#include <math.h>

static char   *pdp_hue_version = "pdp_hue: version 0.1, frame hue estimator, written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_hue_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_meanr;
    t_outlet *x_meang;
    t_outlet *x_meanb;

    int x_packet0;
    int x_dropped;
    int x_queue_id;

    int x_vwidth;
    int x_vheight;
    int x_vsize;
    unsigned int x_encoding;

} t_pdp_hue;

static void pdp_hue_process_rgb(t_pdp_hue *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    unsigned char *data = (unsigned char*)pdp_packet_data(x->x_packet0);
    int       i;

    int px, py;
    unsigned char *sr, *sg, *sb;
    double meanr=0, meang=0, meanb=0;
    int nbpixs=0;

    /* allocate all ressources */
    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;
    x->x_encoding = header->info.image.encoding;

    sr = data;
    meanr = 0.; meang = 0.; meanb =0.;

    for(py=0; py<x->x_vheight; py++) 
    {
      for(px=0; px<x->x_vwidth; px++) 
      {
         meanr += (double)*(sr);
         meang += (double)*(sr+1);
         meanb += (double)*(sr+2);
         sr+=3;
         nbpixs++;
      }
    }
    if ( nbpixs > 0 )
    {
      meanr = meanr / nbpixs;  
      meang = meang / nbpixs;  
      meanb = meanb / nbpixs;  
    }

    outlet_float( x->x_meanr, (t_float)meanr );
    outlet_float( x->x_meang, (t_float)meang );
    outlet_float( x->x_meanb, (t_float)meanb );

    return;
}

static void pdp_hue_killpacket(t_pdp_hue *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;
}

static void pdp_hue_process(t_pdp_hue *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_BITMAP == header->type))
   {
    
	/* pdp_hue_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            pdp_queue_add(x, pdp_hue_process_rgb, pdp_hue_killpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_hue_process */
	    break;
	    
	}
    }
}

static void pdp_hue_input_0(t_pdp_hue *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_hue_process(x);

    }
}

static void pdp_hue_free(t_pdp_hue *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_hue_class;

void *pdp_hue_new(void)
{
    int i;

    t_pdp_hue *x = (t_pdp_hue *)pd_new(pdp_hue_class);

    x->x_meanr = outlet_new(&x->x_obj, &s_float); 
    x->x_meang = outlet_new(&x->x_obj, &s_float); 
    x->x_meanb = outlet_new(&x->x_obj, &s_float); 

    x->x_packet0 = -1;
    x->x_queue_id = -1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_hue_setup(void)
{
//    post( pdp_hue_version );
    pdp_hue_class = class_new(gensym("pdp_hue"), (t_newmethod)pdp_hue_new,
    	(t_method)pdp_hue_free, sizeof(t_pdp_hue), 0, A_NULL);

    class_addmethod(pdp_hue_class, (t_method)pdp_hue_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
