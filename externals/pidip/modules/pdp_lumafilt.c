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

/*  This effect filters some levels of luminosity
 *  Useful to isolate some objects
 *  Written by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

#define MAX_LUMA 256

static char   *pdp_lumafilt_version = "pdp_lumafilt: version 0.1, written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_lumafilt_struct
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

    int x_filter[MAX_LUMA]; // transform number

} t_pdp_lumafilt;

static void pdp_lumafilt_free_ressources(t_pdp_lumafilt *x)
{ 
     // nothing
}

static void pdp_lumafilt_allocate(t_pdp_lumafilt *x)
{
     // nothing
}

static void pdp_lumafilt_filter(t_pdp_lumafilt *x, t_floatarg fluma, t_floatarg fonoff )
{
  if ( ( (int)fluma >= 0 ) && ( (int)fluma < MAX_LUMA ) )
  { 
     if ( ((int)fonoff == 0 ) || ((int)fonoff == 1 ) )
     {
        x->x_filter[ (int)fluma ]  = (int)fonoff;
     }	
  }
}

static void pdp_lumafilt_mfilter(t_pdp_lumafilt *x, t_floatarg flumas, t_floatarg flumae, t_floatarg fonoff )
{
  int li;

  if ( ( (int)flumas >= 0 ) && ( (int)flumas < MAX_LUMA ) &&
       ( (int)flumae >= 0 ) && ( (int)flumae < MAX_LUMA ) &&
       ( flumas < flumae ) )
  { 
     if ( ((int)fonoff == 0 ) || ((int)fonoff == 1 ) )
     {
	for ( li=(int)flumas; li<=(int)flumae; li++ )
	{
           x->x_filter[ li ]  = (int)fonoff;
	}
     }	
  }
}

static void pdp_lumafilt_process_yv12(t_pdp_lumafilt *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     px, py, luma;
    short     int *pnY, *pnU, *pnV;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize )
    {
        pdp_lumafilt_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_lumafilt_allocate(x);
        post( "pdp_lumafilt : reallocated buffers" );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    pnY = newdata;
    pnV = newdata+x->x_vsize;
    pnU = newdata+x->x_vsize+(x->x_vsize>>2);

    memcpy( newdata, data, (x->x_vsize + (x->x_vsize>>1))<<1 );

    for (py = 0; py < x->x_vheight; py++) {
      for (px = 0; px < x->x_vwidth; px++) {
         luma = (*(pnY)>>7);
	 if ( ( luma >=0 ) && ( luma < MAX_LUMA ) ) // paranoid
	 {
	   if ( x->x_filter[luma] )
	   {
	     *(pnY) = 0;
	     *(pnU) = 0;
	     *(pnV) = 0;
	   }
	 }
	 else
	 {
           // post( "pdp_lumafilt : luminosity value out of bounds : %d", luma );
	 }
	 pnY++;
	 if ( (px%2==0) && (py%2==0) )
         {
           *pnU++; *pnV++;
         }
      }
    }

    return;
}

static void pdp_lumafilt_sendpacket(t_pdp_lumafilt *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_lumafilt_process(t_pdp_lumafilt *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_lumafilt_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_lumafilt_process_yv12, pdp_lumafilt_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_lumafilt_process */
	    break;
	    
	}
    }
}

static void pdp_lumafilt_input_0(t_pdp_lumafilt *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_lumafilt_process(x);
    }
}

static void pdp_lumafilt_free(t_pdp_lumafilt *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_lumafilt_free_ressources(x);
}

t_class *pdp_lumafilt_class;

void *pdp_lumafilt_new(void)
{
    int fi;

    t_pdp_lumafilt *x = (t_pdp_lumafilt *)pd_new(pdp_lumafilt_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    for ( fi=0; fi<MAX_LUMA; fi++ )
    {
       x->x_filter[fi] = 0;
    }

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_lumafilt_setup(void)
{
//    post( pdp_lumafilt_version );
    pdp_lumafilt_class = class_new(gensym("pdp_lumafilt"), (t_newmethod)pdp_lumafilt_new,
    	(t_method)pdp_lumafilt_free, sizeof(t_pdp_lumafilt), 0, A_NULL);

    class_addmethod(pdp_lumafilt_class, (t_method)pdp_lumafilt_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_lumafilt_class, (t_method)pdp_lumafilt_filter, gensym("filter"),  A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_lumafilt_class, (t_method)pdp_lumafilt_mfilter, gensym("mfilter"),  A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
