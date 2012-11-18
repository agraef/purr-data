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

/*  This object is an adaptation of mosaic effect from effectv
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

#define MAGIC_THRESHOLD 30
#define CENSOR_LEVEL 20

static char   *pdp_mosaic_version = "pdp_mosaic: version 0.1, port of mosaic from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_mosaic_struct
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
    int x_censor_level;
    int x_ssize;
    short int *x_diff;
    short int *x_bdata;
    int x_snapshot;

} t_pdp_mosaic;


static void pdp_mosaic_ssize(t_pdp_mosaic *x, t_floatarg fssize )
{
   if ( ( fssize > 1 ) && ( fssize < x->x_vwidth ) )
   {
      x->x_ssize = (int)fssize;
   }
}

static void pdp_mosaic_level(t_pdp_mosaic *x, t_floatarg flevel )
{
   if ( flevel > 0 )
   {
      x->x_censor_level = (int)flevel;
   }
}

static void pdp_mosaic_background(t_pdp_mosaic *x )
{
   x->x_snapshot = 1;
}

static void pdp_mosaic_free_ressources(t_pdp_mosaic *x)
{
  if ( x->x_diff != NULL ) freebytes( x->x_diff, (x->x_vsize + (x->x_vsize>>1))<<1 );
  if ( x->x_bdata != NULL ) freebytes( x->x_bdata, (( x->x_vsize + (x->x_vsize>>1))<<1));
}

static void pdp_mosaic_allocate(t_pdp_mosaic *x)
{
 int i;

  x->x_diff = (short int*) getbytes((x->x_vsize + (x->x_vsize>>1))<<1);
  x->x_bdata = (short int *) getbytes((( x->x_vsize + (x->x_vsize>>1))<<1));
  if( !x->x_bdata || ! x->x_diff ) {
      post( "pdp_mosaic : severe error : cannot allocate buffers" );
  }

}

/* check if there is a real difference with background image */
static void pdp_mosaic_diff(t_pdp_mosaic *x, short int *src)
{
   int i;
   int Yy=0, Yu=0, Yv=0;
   int Yby=0, Ybu=0, Ybv=0;
   short int *p=NULL;
   short int *pb=NULL;
   short int *r=NULL;
   int v;

   p = src;
   pb = x->x_bdata;
   r = x->x_diff;
   for(i=0; i<(x->x_vsize); i++) 
   {
         Yy = (*p);
         Yu = (*(p+x->x_vsize+(i>>2)));
         if ( x->x_vsize+(x->x_vsize>>2)+(i>>2) > x->x_vsize+(x->x_vsize>>1) )
         {
            post ("pdp_mosaic : overflow : offset=%d limit=%d", x->x_vsize+(x->x_vsize>>2)+(i>>2),
                   x->x_vsize+(x->x_vsize>>1) );
            return;
         }
         Yv = (*(p+x->x_vsize+(x->x_vsize>>2)+(i>>2)));
         Yby = (*pb);
         Ybu = (*(pb+x->x_vsize+(i>>2)));
         Ybv = (*(pb+x->x_vsize+(x->x_vsize>>2)+(i>>2)));
         if ( !r ) { post( "pdp_mosaic : hey, buffers are not allocated !!" ); return; };
         *r = ( (Yy - Yby) + (Yu - Ybu) + (Yv - Ybv) );
         r++;
   }

}

static void pdp_mosaic_process_yv12(t_pdp_mosaic *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i;

    int px=0, py=0, xx, yy, y, u, v;
    int count;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize )
    {
        pdp_mosaic_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_mosaic_allocate(x);
        post( "pdp_mosaic : reallocated buffers" );
    }

    if ( x->x_bdata && x->x_snapshot )
    {
       x->x_snapshot = 0;
       memcpy( x->x_bdata, data, (x->x_vsize + (x->x_vsize>>1))<<1 );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    pdp_mosaic_diff(x, data);

    memcpy( newdata, data, (x->x_vsize + (x->x_vsize>>1))<<1 );

    for(py=0; py<(x->x_vheight-x->x_ssize-1); py+=x->x_ssize) 
    {
       for(px=0; px<(x->x_vwidth-x->x_ssize-1); px+=x->x_ssize) 
       {
          count = 0;
          for(yy=0; yy<x->x_ssize; yy++) 
          {
            for(xx=0; xx<x->x_ssize; xx++) 
            {
               count += *(x->x_diff + (py+yy)*x->x_vwidth + (px+xx) );
               count += *(x->x_diff + x->x_vsize + (((py+yy)*x->x_vwidth)>>2) + ((px+xx)>>1) );
               count += *(x->x_diff + x->x_vsize + (x->x_vsize>>2) + (((py+yy)*x->x_vwidth)>>2) + ((px+xx)>>1) );
            }
          }
          if(count > x->x_censor_level) 
          {
             // post( "pdp_mosaic : censored" );
             y = *(data + (py+3)*x->x_vwidth + (px+3));
             u = *(data + x->x_vsize + (((py+3)*x->x_vwidth)>>2) + ((px+3)>>1) );
             v = *(data + x->x_vsize + (((py+3)*x->x_vwidth)>>2) + ((px+3)>>1) );
             for(yy=0; yy<x->x_ssize; yy++) 
             {
               for(xx=0; xx<x->x_ssize; xx++)
               {
                 *(newdata + (py+yy)*x->x_vwidth + (px+xx)) = y;
                 *(newdata + x->x_vsize + (((py+yy)*x->x_vwidth)>>2) + ((px+xx)>>1) ) = u;
                 *(newdata + x->x_vsize + (x->x_vsize>>2) + (((py+yy)*x->x_vwidth)>>2) + ((px+xx)>>1) ) = v;
               }
             }
          } 
       }
    }

    return;
}

static void pdp_mosaic_sendpacket(t_pdp_mosaic *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}


static void pdp_mosaic_process(t_pdp_mosaic *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_mosaic_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_mosaic_process_yv12, pdp_mosaic_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_mosaic_process */
	    break;
	    
	}
    }
}

static void pdp_mosaic_input_0(t_pdp_mosaic *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_mosaic_process(x);
    }
}

static void pdp_mosaic_free(t_pdp_mosaic *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_mosaic_free_ressources(x);
}

t_class *pdp_mosaic_class;

void *pdp_mosaic_new(void)
{
    int i;

    t_pdp_mosaic *x = (t_pdp_mosaic *)pd_new(pdp_mosaic_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_bang, gensym("background"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("level"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_vsize = -1;
    x->x_snapshot = 1;
    x->x_ssize = 8;       // square size
    x->x_censor_level = CENSOR_LEVEL;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_mosaic_setup(void)
{
//    post( pdp_mosaic_version );
    pdp_mosaic_class = class_new(gensym("pdp_mosaic"), (t_newmethod)pdp_mosaic_new,
    	(t_method)pdp_mosaic_free, sizeof(t_pdp_mosaic), 0, A_NULL);

    class_addmethod(pdp_mosaic_class, (t_method)pdp_mosaic_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mosaic_class, (t_method)pdp_mosaic_background, gensym("background"), A_NULL);
    class_addmethod(pdp_mosaic_class, (t_method)pdp_mosaic_level, gensym("level"), A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
