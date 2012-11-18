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

/*  This object is a pixels mapper enabling to do some cut and paste
 *  Written by Yves Degoyon                                 
 */

#include "pdp.h"
#include <math.h>

static char   *pdp_mapper_version = "pdp_mapper: version 0.1, a pixels mapper, written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_mapper_struct
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
    unsigned int x_encoding;
    int *x_pixelmap;

} t_pdp_mapper;

static void pdp_mapper_copy(t_pdp_mapper *x, t_floatarg fromX, t_floatarg fromY, t_floatarg toX, t_floatarg toY)
{
   if ( ( fromX >= 0 ) && ( fromX < x->x_vwidth ) &&
        ( toX >= 0 ) && ( toX < x->x_vwidth ) &&
        ( fromY >= 0 ) && ( fromY < x->x_vheight ) &&
        ( toY >= 0 ) && ( toY < x->x_vheight ) )
    {
        x->x_pixelmap[ (int)toY*x->x_vwidth+(int)toX ] = x->x_pixelmap[ (int)fromY*x->x_vwidth+(int)fromX ];
    }
}

static void pdp_mapper_reset(t_pdp_mapper *x)
{
 int px, py;

  if ( x->x_vsize > 0 )
  {
    for ( py=0; py<x->x_vheight; py++ )
    {
      for ( px=0; px<x->x_vwidth; px++ )
      {
         x->x_pixelmap[py*x->x_vwidth+px] = py*x->x_vwidth+px;
      }
    }
  }
}

static void pdp_mapper_swap(t_pdp_mapper *x, t_floatarg fromX, t_floatarg fromY, t_floatarg toX, t_floatarg toY)
{

 int tval;
 
   if ( ( fromX >= 0 ) && ( fromX < x->x_vwidth ) &&
        ( toX >= 0 ) && ( toX < x->x_vwidth ) &&
        ( fromY >= 0 ) && ( fromY < x->x_vheight ) &&
        ( toY >= 0 ) && ( toY < x->x_vheight ) )
    {
        tval = x->x_pixelmap[ (int)toY*x->x_vwidth+(int)toX ];
        x->x_pixelmap[ (int)toY*x->x_vwidth+(int)toX ] = x->x_pixelmap[ (int)fromY*x->x_vwidth+(int)fromX ];
        x->x_pixelmap[ (int)fromY*x->x_vwidth+(int)fromX ] = tval;
    }
}

static void pdp_mapper_allocate(t_pdp_mapper *x, int newsize)
{
 int i, px, py;

  if ( x->x_pixelmap != NULL )
  {
       freebytes( x->x_pixelmap, x->x_vsize*sizeof(int) );
  }

  x->x_vsize = newsize;
  x->x_pixelmap = (int*) getbytes( x->x_vsize*sizeof(int) );

  for ( py=0; py<x->x_vheight; py++ )
  {
    for ( px=0; px<x->x_vwidth; px++ )
    {
       x->x_pixelmap[py*x->x_vwidth+px] = py*x->x_vwidth+px;
    }
  }
}

static void pdp_mapper_process_yv12(t_pdp_mapper *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i;

    int px, py, ppx, ppy, offset;
    short int *sy, *su, *sv, t;
    int *spy;
    short int *sny, *snu, *snv;

    /* allocate all ressources */
    if ( ((int)header->info.image.width != x->x_vwidth ) || 
         ((int)header->info.image.height != x->x_vheight ) )
    {
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        post( "pdp_mapper : reallocating buffers" );
        pdp_mapper_allocate(x, header->info.image.width*header->info.image.height );
    }

    x->x_encoding = header->info.image.encoding;

    newheader->info.image.encoding = x->x_encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    /* copy images if necessary */
    // memcpy( newdata, data, (( x->x_vsize + (x->x_vsize>>1))<<1)); 

    sy = data;
    su = (data+x->x_vsize);
    sv = (data+x->x_vsize+(x->x_vsize>>2));
    spy = x->x_pixelmap;
    sny = newdata;
    snu = (newdata+x->x_vsize);
    snv = (newdata+x->x_vsize+(x->x_vsize>>2));

    for(py=1; py<x->x_vheight; py++) 
    {
       for(px=0; px<x->x_vwidth; px++) 
       {
          ppy = (*(spy)/x->x_vwidth); 
          ppx = (*(spy)%x->x_vwidth); 
          *(sny) = *(sy+ppy*x->x_vwidth+ppx);
          *(snu) = *(su+(ppy>>1)*(x->x_vwidth>>1)+(ppx>>1));
          *(snv) = *(sv+(ppy>>1)*(x->x_vwidth>>1)+(ppx>>1));
          sny++; spy++;
          if ( ( px%2 == 0 ) && ( py%2 == 0 ) )
          {
             snu++; snv++;
          }
       }
    }

    return;
}

static void pdp_mapper_sendpacket(t_pdp_mapper *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_mapper_process(t_pdp_mapper *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_mapper_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_mapper_process_yv12, pdp_mapper_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_mapper_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_mapper_process */
	    break;
	    
	}
    }
}

static void pdp_mapper_input_0(t_pdp_mapper *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_mapper_process(x);
    }
}

static void pdp_mapper_free(t_pdp_mapper *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);

    if ( x->x_pixelmap ) freebytes( x->x_pixelmap, x->x_vsize*sizeof(int) );

}

t_class *pdp_mapper_class;

void *pdp_mapper_new(void)
{
    int i;

    t_pdp_mapper *x = (t_pdp_mapper *)pd_new(pdp_mapper_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;
    x->x_vsize = -1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_mapper_setup(void)
{
//    post( pdp_mapper_version );
    pdp_mapper_class = class_new(gensym("pdp_mapper"), (t_newmethod)pdp_mapper_new,
    	(t_method)pdp_mapper_free, sizeof(t_pdp_mapper), 0, A_NULL);

    class_addmethod(pdp_mapper_class, (t_method)pdp_mapper_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mapper_class, (t_method)pdp_mapper_copy, gensym("copy"),  A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mapper_class, (t_method)pdp_mapper_swap, gensym("swap"),  A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mapper_class, (t_method)pdp_mapper_reset, gensym("reset"), A_NULL);


}

#ifdef __cplusplus
}
#endif
