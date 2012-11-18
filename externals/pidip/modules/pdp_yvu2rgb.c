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
#include "yuv.h"
#include <math.h>


typedef struct pdp_yvu2rgb_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    unsigned int *x_RGBFrame;
    int        x_RGBFrameSize;


} t_pdp_yvu2rgb;

static void pdp_yvu2rgb_process_yv12(t_pdp_yvu2rgb *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i;

    unsigned int w = header->info.image.width;
    unsigned int h = header->info.image.height;

    unsigned int size = w*h;
    unsigned int totalnbpixels = size;
    unsigned int u_offset = size;
    unsigned int v_offset = size + (size>>2);
    unsigned int totnbpixels = size + (size>>1);

    unsigned int row, col;

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = w;
    newheader->info.image.height = h;

    if ( !x->x_RGBFrame )
    {
      x->x_RGBFrame = ( unsigned int* ) getbytes( size*sizeof( unsigned int ) );
      x->x_RGBFrameSize = size*sizeof( unsigned int );
      post( "pdp_yvu2rgb : allocated frame size=%d", x->x_RGBFrameSize );
    }
    if ( !x->x_RGBFrame ) 
    {
       post( "pdp_yvu2rgb : cannot allocate frame" );
       return;
    }

    yuv_Y122RGB( data, x->x_RGBFrame, w, h );
    // post( "pdp_yvu2rgb : converted to RGB" );
    yuv_RGB2Y12( x->x_RGBFrame, newdata, w, h );
    // post( "pdp_yvu2rgb : converted to Y12" );

    return;
}

static void pdp_yvu2rgb_sendpacket(t_pdp_yvu2rgb *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_yvu2rgb_process(t_pdp_yvu2rgb *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_yvu2rgb_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_yvu2rgb_process_yv12, pdp_yvu2rgb_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_yvu2rgb_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_yvu2rgb_process */
	    break;
	    
	}
    }
}

static void pdp_yvu2rgb_input_0(t_pdp_yvu2rgb *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_yvu2rgb_process(x);
    }
}

static void pdp_yvu2rgb_free(t_pdp_yvu2rgb *x)
{
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    if (x->x_RGBFrame ) freebytes( x->x_RGBFrame, x->x_RGBFrameSize );
}

t_class *pdp_yvu2rgb_class;

void *pdp_yvu2rgb_new(void)
{
    int i;

    t_pdp_yvu2rgb *x = (t_pdp_yvu2rgb *)pd_new(pdp_yvu2rgb_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_yvu2rgb_setup(void)
{


    pdp_yvu2rgb_class = class_new(gensym("pdp_yvu2rgb"), (t_newmethod)pdp_yvu2rgb_new,
    	(t_method)pdp_yvu2rgb_free, sizeof(t_pdp_yvu2rgb), 0, A_NULL);

    class_addmethod(pdp_yvu2rgb_class, (t_method)pdp_yvu2rgb_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
