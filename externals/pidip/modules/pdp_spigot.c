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

/*  This object is video packet routing utility
 *  Written by Yves Degoyon ( ydegoyon@free.fr )                             
 */


#include "pdp.h"
#include <math.h>

static char   *pdp_spigot_version = "pdp_spigot: version 0.1, a video packets routing utility";

typedef struct pdp_spigot_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_outlet1;
    int x_vwidth;
    int x_vheight;
    int x_vsize;

    int x_packet0;
    int x_toggle;

} t_pdp_spigot;

static void pdp_spigot_toggle(t_pdp_spigot *x, t_floatarg ftoggle )
{
    if ( ( ftoggle == 0 ) || ( ftoggle == 1 ) )
    {
       x->x_toggle = ftoggle;
    }
}

static void pdp_spigot_process_packet(t_pdp_spigot *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = 0;
    short int *newdata = 0;
    int     newpacket = -1, i;

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    return;
}

static void pdp_spigot_process(t_pdp_spigot *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_spigot_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
	    pdp_spigot_process_packet(x);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_spigot_process */
	    break;
	    
	}
    }

    /* propagate if valid */
    if(x->x_packet0 != -1){
        if ( x->x_toggle )
        {
	   pdp_packet_pass_if_valid(x->x_outlet1, &x->x_packet0);
        }
        else
        {
	   pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet0);
        }
	pdp_packet_mark_unused(x->x_packet0);
	x->x_packet0 = -1;
    }

}

static void pdp_spigot_input_0(t_pdp_spigot *x, t_symbol *s, t_floatarg f)
{
    if (s == gensym("register_rw")){
	pdp_packet_mark_unused(x->x_packet0);
	x->x_packet0 = pdp_packet_convert_rw((int)f, pdp_gensym("image/YCrCb/*") );
    }
    else if (s == gensym("process")){
	pdp_spigot_process(x);
    }
}

static void pdp_spigot_free(t_pdp_spigot *x)
{
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_spigot_class;

void *pdp_spigot_new(void)
{
    int i;

    t_pdp_spigot *x = (t_pdp_spigot *)pd_new(pdp_spigot_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("toggle"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet1 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_spigot_setup(void)
{
//    post( pdp_spigot_version );
    pdp_spigot_class = class_new(gensym("pdp_spigot"), (t_newmethod)pdp_spigot_new,
    	(t_method)pdp_spigot_free, sizeof(t_pdp_spigot), 0, A_NULL);

    class_addmethod(pdp_spigot_class, (t_method)pdp_spigot_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_spigot_class, (t_method)pdp_spigot_toggle, gensym("toggle"),  A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
