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

/*  This object is a port of cycle effect from EffecTV
 *  Originally written by clifford smith <nullset@dookie.net>
 *  Pd-fication by Yves Degoyon ( ydegoyon@free.fr )                             
 */


#include "pdp.h"
#include <math.h>

#define NEWCOLOR(c,o) ((c+o)%230)

static char   *pdp_cycle_version = "pdp_cycle: version 0.1, port of cycle from EffecTV by clifford smith, adapted by ydegoyon@free.fr ";

typedef struct pdp_cycle_struct
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

    int x_cycley; // flag to activate y cycling
    int x_cycleu; // flag to activate u cycling
    int x_cyclev; // flag to activate v cycling

    int x_yoffset;
    int x_uoffset;
    int x_voffset;

} t_pdp_cycle;

static void pdp_cycle_cycley(t_pdp_cycle *x, t_floatarg fcycley )
{
    if ( ( fcycley == 0 ) || ( fcycley == 1 ) )
    {
       x->x_cycley = fcycley;
    }
}

static void pdp_cycle_cycleu(t_pdp_cycle *x, t_floatarg fcycleu )
{
    if ( ( fcycleu == 0 ) || ( fcycleu == 1 ) )
    {
       x->x_cycleu = fcycleu;
    }
}

static void pdp_cycle_cyclev(t_pdp_cycle *x, t_floatarg fcyclev )
{
    if ( ( fcyclev == 0 ) || ( fcyclev == 1 ) )
    {
       x->x_cyclev = fcyclev;
    }
}

static void pdp_cycle_process_yv12(t_pdp_cycle *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i;
    int     px, py, y, u, v;

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    x->x_yoffset += 1;
    x->x_uoffset += 3; 
    x->x_voffset += 7;

    for(py=1; py<x->x_vheight; py++)
    {
      for(px=0; px<x->x_vwidth; px++)
      {
         if ( x->x_cycley )
         {
           y = *(data+py*x->x_vwidth+px)>>7;
           *(newdata+py*x->x_vwidth+px) = NEWCOLOR(y,x->x_yoffset)<<7;
         }
         if ( x->x_cycleu )
         {
           u = ( *(data+x->x_vsize+(x->x_vsize>>2)+((py*x->x_vwidth>>2)+(px>>1))) >>8 ) + 128;
           *(newdata+x->x_vsize+(x->x_vsize>>2)+((py*x->x_vwidth>>2)+(px>>1))) = (NEWCOLOR(u,x->x_uoffset)-128)<<8;
         }
         if ( x->x_cyclev )
         {
           v = ( *(data+x->x_vsize+((py*x->x_vwidth>>2)+(px>>1))) >>8 ) + 128;
           *(newdata+x->x_vsize+((py*x->x_vwidth>>2)+(px>>1))) = (NEWCOLOR(v,x->x_voffset)-128)<<8;
         }
      }
    }

    return;
}

static void pdp_cycle_sendpacket(t_pdp_cycle *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_cycle_process(t_pdp_cycle *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_cycle_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_cycle_process_yv12, pdp_cycle_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_cycle_process */
	    break;
	    
	}
    }

}

static void pdp_cycle_input_0(t_pdp_cycle *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );


    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_cycle_process(x);

    }

}

static void pdp_cycle_free(t_pdp_cycle *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_cycle_class;

void *pdp_cycle_new(void)
{
    int i;

    t_pdp_cycle *x = (t_pdp_cycle *)pd_new(pdp_cycle_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cycley"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cycleu"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cyclev"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_cycley = 1;
    x->x_cycleu = 0; 
    x->x_cyclev = 0;

    x->x_yoffset = 0;
    x->x_uoffset = 0; 
    x->x_voffset = 0;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_cycle_setup(void)
{
//    post( pdp_cycle_version );
    pdp_cycle_class = class_new(gensym("pdp_cycle"), (t_newmethod)pdp_cycle_new,
    	(t_method)pdp_cycle_free, sizeof(t_pdp_cycle), 0, A_NULL);

    class_addmethod(pdp_cycle_class, (t_method)pdp_cycle_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cycle_class, (t_method)pdp_cycle_cycley, gensym("cycley"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cycle_class, (t_method)pdp_cycle_cycleu, gensym("cycleu"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cycle_class, (t_method)pdp_cycle_cyclev, gensym("cyclev"),  A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
