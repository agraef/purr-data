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

static char   *pdp_fdiff_version = "pdp_fdiff: version 0.1, frame difference estimator, written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_fdiff_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_diffy;
    t_outlet *x_diffu;
    t_outlet *x_diffv;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_vwidth;
    int x_vheight;
    int x_vsize;
    unsigned int x_encoding;
    short int *x_pframe;

} t_pdp_fdiff;

static void pdp_fdiff_allocate(t_pdp_fdiff *x, int newsize)
{
 int i;

  if ( x->x_pframe != NULL ) freebytes( x->x_pframe, (x->x_vsize + (x->x_vsize>>1))<<1 ); 

  x->x_vsize = newsize;
  x->x_pframe = (short int*) getbytes((x->x_vsize + (x->x_vsize>>1))<<1);
}

static void pdp_fdiff_process_yv12(t_pdp_fdiff *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i;

    int px, py;
    short int *diff;
    short int *sy, *su, *sv, t;
    short int *sby, *sbu, *sbv;
    int Y=0, U=0, V=0;
    int maxdy=0, maxdu=0, maxdv=0, pdiff;

    /* allocate all ressources */
    if ( ((int)header->info.image.width != x->x_vwidth ) ||
         ((int)header->info.image.height != x->x_vheight ) ) 
    {
        pdp_fdiff_allocate(x, header->info.image.width*header->info.image.height );
        post( "pdp_fdiff : reallocating buffers" );
    }

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;
    x->x_encoding = header->info.image.encoding;

    newheader->info.image.encoding = x->x_encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    sy = data;
    sv = (data+x->x_vsize);
    su = (data+x->x_vsize+(x->x_vsize>>2));
    sby = x->x_pframe;
    sbv = (x->x_pframe+x->x_vsize);
    sbu = (x->x_pframe+x->x_vsize+(x->x_vsize>>2));
    maxdy = 0; maxdv = 0; maxdu =0;

    for(py=0; py<x->x_vheight; py++) 
    {
      for(px=0; px<x->x_vwidth; px++) 
      {
         pdiff = abs( *(sy+py*x->x_vwidth+px) - *(sby+py*x->x_vwidth+px) );
         if ( pdiff > maxdy ) maxdy = pdiff;
         if ( px%2==0 && py%2==0 )
         {
           pdiff = abs( *(su+(py>>1)*(x->x_vwidth>>1)+(px>>1)) - *(sbu+(py>>1)*(x->x_vwidth>>1)+(px>>1)) );
           if ( pdiff > maxdu ) maxdu = pdiff;
           pdiff = abs( *(sv+(py>>1)*(x->x_vwidth>>1)+(px>>1)) - *(sbv+(py>>1)*(x->x_vwidth>>1)+(px>>1)) );
           if ( pdiff > maxdv ) maxdv = pdiff;
         }
      }
    }

    outlet_float( x->x_diffy, (maxdy>>7) );
    outlet_float( x->x_diffu, (maxdu>>8) );
    outlet_float( x->x_diffv, (maxdv>>8) );

    /* save previous frame */
    memcpy( x->x_pframe, data, (( x->x_vsize + (x->x_vsize>>1))<<1)); 
    memcpy( newdata, data, (( x->x_vsize + (x->x_vsize>>1))<<1)); 
    
    return;
}

static void pdp_fdiff_sendpacket(t_pdp_fdiff *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_fdiff_process(t_pdp_fdiff *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_fdiff_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_fdiff_process_yv12, pdp_fdiff_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_fdiff_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_fdiff_process */
	    break;
	    
	}
    }
}

static void pdp_fdiff_input_0(t_pdp_fdiff *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_fdiff_process(x);

    }
}

static void pdp_fdiff_free(t_pdp_fdiff *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);

    if ( x->x_pframe ) freebytes( x->x_pframe, (x->x_vsize + (x->x_vsize>>1))<<1 );

}

t_class *pdp_fdiff_class;

void *pdp_fdiff_new(void)
{
    int i;

    t_pdp_fdiff *x = (t_pdp_fdiff *)pd_new(pdp_fdiff_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_diffy = outlet_new(&x->x_obj, &s_float); 
    x->x_diffu = outlet_new(&x->x_obj, &s_float); 
    x->x_diffv = outlet_new(&x->x_obj, &s_float); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_pframe = NULL;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_fdiff_setup(void)
{
//    post( pdp_fdiff_version );
    pdp_fdiff_class = class_new(gensym("pdp_fdiff"), (t_newmethod)pdp_fdiff_new,
    	(t_method)pdp_fdiff_free, sizeof(t_pdp_fdiff), 0, A_NULL);

    class_addmethod(pdp_fdiff_class, (t_method)pdp_fdiff_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
