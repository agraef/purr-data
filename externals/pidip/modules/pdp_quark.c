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

static int fastrand_val=0;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)


static char   *pdp_quark_version = "pdp_quark: version 0.1, port of quark from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

static char* the_wave_table = NULL;

typedef struct pdp_quark_struct
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

} t_pdp_quark;

static void pdp_quark_free_ressources(t_pdp_quark *x)
{
    if ( x->x_buffer ) free ( x->x_buffer );
    if ( x->x_planetable ) free ( x->x_planetable );
}

static void pdp_quark_allocate(t_pdp_quark *x)
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
       post( "pdp_quark : severe error : cannot allocate buffers !!! ");
       return;
    }
}

static void pdp_quark_planes(t_pdp_quark *x, t_floatarg fplanes)
{
   if ( ( (int)fplanes > 1 ) && ( (int)fplanes < 100 ) )
   {
      pdp_quark_free_ressources(x); 
      x->x_planes = (int) fplanes;
      x->x_plane=x->x_planes-1;
      pdp_quark_allocate(x);
   }
}

static void pdp_quark_tolerance(t_pdp_quark *x, t_floatarg ftolerance)
{
   if ( (int)ftolerance > 1 )
   {
      x->x_tolerance = (int) ftolerance;
   }
}

static void pdp_quark_process_yv12(t_pdp_quark *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i, cf, diff;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize )
    {
        pdp_quark_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        x->x_plane = x->x_planes-1;
        pdp_quark_allocate(x);
        post( "pdp_quark : reallocated buffers" );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy(x->x_planetable[x->x_plane], data, (x->x_vsize + (x->x_vsize>>1))<<1 );
    for(i=0; i<x->x_vsize+(x->x_vsize>>1); i++) 
    {
       cf = (x->x_plane + (inline_fastrand()>>24))&(x->x_planes-1);
       if ( i<x->x_vsize )
       {
          diff = ((x->x_planetable[cf])[i] - data[i])>>7;
       }
       else
       {
          diff = (((x->x_planetable[cf])[i] - data[i] )>>8)+128;
       }
       if ( abs ( diff ) > x->x_tolerance )
       {
         newdata[i] = (x->x_planetable[cf])[i];
       }
       else
       {
         newdata[i] = data[i];
       }
       /* The reason why I use high order 8 bits is written in utils.c
        (or, 'man rand') */
    }
    x->x_plane--;
    if ( x->x_plane < 0 ) x->x_plane = x->x_planes-1;

    return;
}

static void pdp_quark_sendpacket(t_pdp_quark *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_quark_process(t_pdp_quark *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_quark_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_quark_process_yv12, pdp_quark_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_quark_process */
	    break;
	    
	}
    }
}

static void pdp_quark_input_0(t_pdp_quark *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_quark_process(x);
    }
}

static void pdp_quark_free(t_pdp_quark *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_quark_free_ressources(x);
}

t_class *pdp_quark_class;

void *pdp_quark_new(void)
{
    int i;

    t_pdp_quark *x = (t_pdp_quark *)pd_new(pdp_quark_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("planes"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("tolerance"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;
    x->x_planes = DEFAULT_PLANES;

    x->x_buffer = NULL;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_quark_setup(void)
{
//    post( pdp_quark_version );
    pdp_quark_class = class_new(gensym("pdp_quark"), (t_newmethod)pdp_quark_new,
    	(t_method)pdp_quark_free, sizeof(t_pdp_quark), 0, A_NULL);

    class_addmethod(pdp_quark_class, (t_method)pdp_quark_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_quark_class, (t_method)pdp_quark_planes, gensym("planes"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_quark_class, (t_method)pdp_quark_tolerance, gensym("tolerance"), A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
