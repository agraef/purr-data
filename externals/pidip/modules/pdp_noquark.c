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

/*  This object is an adaptation of warp effect from effectv
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

#define DEFAULT_PLANES             16
#define DEFAULT_TOLERANCE          5

static int fastrand_val=0;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)


static char   *pdp_noquark_version = "pdp_noquark: version 0.1, port of quark from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

static char* the_wave_table = NULL;

typedef struct pdp_noquark_struct
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
    short int *x_buffer;
    short int **x_planetable;
    int x_plane;
    int x_planes;
    int x_tolerance;

} t_pdp_noquark;

static void pdp_noquark_free_ressources(t_pdp_noquark *x)
{
    if ( x->x_buffer ) free ( x->x_buffer );
    if ( x->x_planetable ) free ( x->x_planetable );
}

static void pdp_noquark_allocate(t_pdp_noquark *x)
{
  int i;

    // allocate space for the frame buffers.  A lot of memory is required -
    // with the default settings, it totals nearly 5 megs.
    x->x_buffer = (short int *) getbytes ( ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 ) * 2 * x->x_planes);
    x->x_planetable = (short int**) getbytes( x->x_planes*sizeof( short int* ) );

    // set up the array of pointers to the frame buffers
    for(i=0;i<x->x_planes;i++) 
    {
       x->x_planetable[i] = &x->x_buffer[ ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 ) * i];
    }

    if ( !x->x_buffer )
    {
       post( "pdp_noquark : severe error : cannot allocate buffers !!! ");
       return;
    }
}

static void pdp_noquark_planes(t_pdp_noquark *x, t_floatarg fplanes)
{
   if ( ( (int)fplanes > 1 ) && ( (int)fplanes < 100 ) )
   {
      pdp_noquark_free_ressources(x); 
      x->x_planes = (int) fplanes;
      x->x_plane=x->x_planes-1;
      pdp_noquark_allocate(x);
   }
}

static void pdp_noquark_tolerance(t_pdp_noquark *x, t_floatarg ftolerance)
{
   if ( (int)ftolerance > 1 )
   {
      x->x_tolerance = (int) ftolerance;
   }
}

static void pdp_noquark_process_yv12(t_pdp_noquark *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     px, py, cf, diff, rx, ry;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize )
    {
        pdp_noquark_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        x->x_plane = x->x_planes-1;
        pdp_noquark_allocate(x);
        post( "pdp_noquark : reallocated buffers" );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy(x->x_planetable[x->x_plane], data, (x->x_vsize + (x->x_vsize>>1))<<1 );
    for(py=0; py<x->x_vheight; py++)
    {
      for(px=0; px<x->x_vwidth; px++)
      {
        cf = (x->x_plane + (inline_fastrand()>>24))&(x->x_planes-1);
        diff = (((x->x_planetable[cf])[py*x->x_vwidth+px] - data[py*x->x_vwidth+px])>>7) +
               ((((x->x_planetable[cf])[x->x_vsize+((py>>1)*(x->x_vwidth>>1)+(px>>1))] - 
                   data[x->x_vsize+((py>>1)*(x->x_vwidth>>1)+(px>>1))] )>>8)) +
               ((((x->x_planetable[cf])[x->x_vsize+(x->x_vsize>>2)+((py>>1)*(x->x_vwidth>>1)+(px>>1))] - 
                   data[x->x_vsize+(x->x_vsize>>2)+((py>>1)*(x->x_vwidth>>1)+(px>>1))] )>>8));
        if ( abs ( diff ) > x->x_tolerance )
        {
         newdata[py*x->x_vwidth+px] = data[py*x->x_vwidth+px];
         newdata[x->x_vsize+((py>>1)*(x->x_vwidth>>1)+(px>>1))] = data[x->x_vsize+((py>>1)*(x->x_vwidth>>1)+(px>>1))];
         newdata[x->x_vsize+(x->x_vsize>>2)+((py>>1)*(x->x_vwidth>>1)+(px>>1))] = 
             data[x->x_vsize+(x->x_vsize>>2)+((py>>1)*(x->x_vwidth>>1)+(px>>1))];
        }
        else
        {
         rx = inline_fastrand()&x->x_vwidth;
         ry = inline_fastrand()&x->x_vheight;
         newdata[py*x->x_vwidth+px] = data[ry*x->x_vwidth+rx];
         newdata[x->x_vsize+((py>>1)*(x->x_vwidth>>1)+(px>>1))] = 
                 data[x->x_vsize+((ry>>1)*(x->x_vwidth>>1)+(rx>>1))];
         newdata[x->x_vsize+(x->x_vsize>>2)+((py>>1)*(x->x_vwidth>>1)+(px>>1))] = 
                 data[x->x_vsize+(x->x_vsize>>2)+((ry>>1)*(x->x_vwidth>>1)+(rx>>1))];
        }
        /* The reason why I use high order 8 bits is written in utils.c
        (or, 'man rand') */
      }
    }
    x->x_plane--;
    if ( x->x_plane < 0 ) x->x_plane = x->x_planes-1;

    return;
}

static void pdp_noquark_sendpacket(t_pdp_noquark *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_noquark_process(t_pdp_noquark *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_noquark_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_noquark_process_yv12, pdp_noquark_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_noquark_process */
	    break;
	    
	}
    }
}

static void pdp_noquark_input_0(t_pdp_noquark *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_noquark_process(x);
    }
}

static void pdp_noquark_free(t_pdp_noquark *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_noquark_free_ressources(x);
}

t_class *pdp_noquark_class;

void *pdp_noquark_new(void)
{
    int i;

    t_pdp_noquark *x = (t_pdp_noquark *)pd_new(pdp_noquark_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("planes"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("tolerance"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;
    x->x_planes = DEFAULT_PLANES;
    x->x_tolerance = DEFAULT_TOLERANCE;

    x->x_buffer = NULL;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_noquark_setup(void)
{
//    post( pdp_noquark_version );
    pdp_noquark_class = class_new(gensym("pdp_noquark"), (t_newmethod)pdp_noquark_new,
    	(t_method)pdp_noquark_free, sizeof(t_pdp_noquark), 0, A_NULL);

    class_addmethod(pdp_noquark_class, (t_method)pdp_noquark_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_noquark_class, (t_method)pdp_noquark_planes, gensym("planes"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_noquark_class, (t_method)pdp_noquark_tolerance, gensym("tolerance"), A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
