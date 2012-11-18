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

/*  This object is an adaptation of baltan effect from freej
 *  Originally written by Fukuchi Kentarou	
 *  Adapted by Yves Degoyon                                 
 *  Do not expect it to behave like effectv : well, it does things ....
 */



#include "pdp.h"
#include <math.h>

#define PLANES 32

#define STRIDE 8

typedef struct pdp_baltan_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int *x_planebuf;
    int x_plane;
    int x_pixels;
    int x_dfts; /* the factor */

} t_pdp_baltan;

static void pdp_baltan_process_yv12(t_pdp_baltan *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);

    unsigned int w = header->info.image.width;
    unsigned int h = header->info.image.height;

    unsigned int size = w*h;
    unsigned int totalnbpixels = size;
    unsigned int u_offset = size;
    unsigned int v_offset = size + (size>>2);
    unsigned int totnbpixels = size + (size>>1);

    int i, cf;

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = w;
    newheader->info.image.height = h;

    /* allocate buffers if necessary  */
    if ( ( x->x_planebuf == NULL ) || ( (int)size != x->x_pixels ) ) 
    {
       if ( x->x_planebuf ) freebytes( x->x_planebuf, x->x_pixels*PLANES*sizeof(int) );
       
       x->x_pixels = size;
       x->x_planebuf = (int*)getbytes(x->x_pixels*PLANES*sizeof(int));
       post("pdp_baltan : allocated plane buffer (size=%d)", x->x_pixels*PLANES*sizeof(int) );
       bzero(x->x_planebuf, x->x_pixels*PLANES*sizeof(int));
       x->x_plane = 0;
       if ( !x->x_planebuf ) 
       {
          post( "pdp_baltan : serious error : unable to allocate buffers " ) ;
          return;
       }
    }
    
    /* process data packet */
    for(i=0; i<x->x_pixels; i++)
    {
       *(x->x_planebuf+x->x_plane*x->x_pixels+i) = (data[i] & x->x_dfts)>>2;
    }

    cf = x->x_plane & (STRIDE-1);

    for(i=0; i<x->x_pixels; i++) {
      newdata[i] = *(x->x_planebuf+cf*x->x_pixels+i)
        + *(x->x_planebuf+((cf+STRIDE)*x->x_pixels)+i)
        + *(x->x_planebuf+((cf+2*STRIDE)*x->x_pixels)+i)
        + *(x->x_planebuf+((cf+3*STRIDE)*x->x_pixels)+i);
      *(x->x_planebuf+x->x_plane*x->x_pixels+i) = (newdata[i]&x->x_dfts)>>2;
    }

    x->x_plane++;
    x->x_plane = x->x_plane & (PLANES-1);

    /* leave the colors unchanged */
    for( i=size; i<(int)totnbpixels; i++)
    {
       newdata[i] = data[i];
    }

    return;
}

static void pdp_baltan_sendpacket(t_pdp_baltan *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_baltan_process(t_pdp_baltan *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_baltan_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_baltan_process_yv12, pdp_baltan_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_baltan_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_baltan_process */
	    break;
	    
	}
    }
}

static void pdp_baltan_input_0(t_pdp_baltan *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))  
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );


    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_baltan_process(x);

    }
}

static void pdp_baltan_dfts(t_pdp_baltan *x, t_floatarg fdfts )
{
    x->x_dfts = (int)fdfts;
}

static void pdp_baltan_free(t_pdp_baltan *x)
{
    if ( x->x_planebuf ) freebytes( x->x_planebuf, x->x_pixels*PLANES*sizeof(int) );
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_baltan_class;

void *pdp_baltan_new(void)
{
    int i;

    t_pdp_baltan *x = (t_pdp_baltan *)pd_new(pdp_baltan_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("dfts"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_planebuf = NULL; 
    x->x_pixels = 0; 
    x->x_dfts = 0xfcfcfc; 

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_baltan_setup(void)
{


    pdp_baltan_class = class_new(gensym("pdp_baltan"), (t_newmethod)pdp_baltan_new,
    	(t_method)pdp_baltan_free, sizeof(t_pdp_baltan), 0, A_NULL);

    class_addmethod(pdp_baltan_class, (t_method)pdp_baltan_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_baltan_class, (t_method)pdp_baltan_dfts, gensym("dfts"),  A_DEFFLOAT, A_NULL);



}

#ifdef __cplusplus
}
#endif
