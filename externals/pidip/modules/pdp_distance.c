/*
 *   PiDiP module
 *   Copyright (c) by Yves Degoyon ( ydegoyon@free.fr )
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

/*  This object is a salience distance operator 
 *  ( inspired by Paul Rosin, 91, http://www.cs.cf.ac.uk/User/Paul.Rosin/resources/sdt/ )
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <stdio.h>

static char   *pdp_distance_version = "pdp_distance: morphology : distance operator version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_distance_struct
{
    t_object x_obj;

    int x_packet0;
    int x_packet1;
    int x_queue_id;
    int x_dropped;

    int x_vwidth;
    int x_vheight;
    int x_vsize;
    short int *x_frame;  // keep a copy of current frame for transformations

    int x_coeff1;
    int x_coeff2;
    int x_coeff3;
    int x_coeff4;

    t_outlet *x_pdp_output; // output packets

} t_pdp_distance;

static void pdp_distance_allocate(t_pdp_distance *x)
{
    x->x_frame = (short int *) getbytes ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );

    if ( !x->x_frame )
    {
       post( "pdp_distance : severe error : cannot allocate buffer !!! ");
       return;
    }
}

static void pdp_distance_coeff1(t_pdp_distance *x, t_floatarg fcoeff1 )
{
   x->x_coeff1 = (int) fcoeff1;
}

static void pdp_distance_coeff2(t_pdp_distance *x, t_floatarg fcoeff2 )
{
   x->x_coeff2 = (int) fcoeff2;
}

static void pdp_distance_coeff3(t_pdp_distance *x, t_floatarg fcoeff3 )
{
   x->x_coeff3 = (int) fcoeff3;
}

static void pdp_distance_coeff4(t_pdp_distance *x, t_floatarg fcoeff4 )
{
   x->x_coeff4 = (int) fcoeff4;
}

static void pdp_distance_free_ressources(t_pdp_distance *x)
{
    if ( x->x_frame ) freebytes ( x->x_frame, ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );
}

static void pdp_distance_process_yv12(t_pdp_distance *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i;
    int     px=0, py=0; 
    short int *pfY, *pfU, *pfV;
    int     nvalues, values[5], ival, mval;

    // allocate all ressources
    if ( ( (int)header->info.image.width != x->x_vwidth ) ||
         ( (int)header->info.image.height != x->x_vheight ) )
    {
        pdp_distance_free_ressources( x );
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_distance_allocate( x );
        post( "pdp_distance : reallocated buffers" );
    }

    // post( "pdp_distance : newheader:%x", newheader );

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, x->x_vsize+(x->x_vsize>>1)<<1 );
    memcpy( x->x_frame, data, x->x_vsize+(x->x_vsize>>1)<<1 );

    pfY = x->x_frame;
    pfV = x->x_frame+x->x_vsize;
    pfU = x->x_frame+x->x_vsize+(x->x_vsize>>2);
    
    // thresholding
    for ( py=0; py<x->x_vheight; py++ )
    {
      for ( px=0; px<x->x_vwidth; px++ )
      {
         if ( *(pfY+py*x->x_vwidth+px) > ((128)<<7) )
         {
            *(pfY+py*x->x_vwidth+px) = ((128)<<7);
         }
      }
    }

    // forward pass
    for ( py=0; py<x->x_vheight; py++ )
    {
      for ( px=0; px<x->x_vwidth; px++ )
      {
        nvalues = 0;
        if ( ((px-1)>=0) && ((py-1)>=0) )
        {
           values[nvalues] = *(pfY+(py-1)*x->x_vwidth+(px-1)) + (x->x_coeff1<<7);
           nvalues++;
        }
        if ( (py-1)>=0 )
        {
           values[nvalues] = *(pfY+(py-1)*x->x_vwidth+px) + (x->x_coeff2<<7);
           nvalues++;
        }
        if ( ((px+1)<x->x_vwidth) && ((py-1)>=0) )
        {
           values[nvalues] = *(pfY+(py-1)*x->x_vwidth+(px+1)) + (x->x_coeff3<<7);
           nvalues++;
        }
        if ( (px-1)>=0 )
        {
           values[nvalues] = *(pfY+py*x->x_vwidth+(px-1)) + (x->x_coeff4<<7);
           nvalues++;
        }
        values[nvalues] = *(pfY+py*x->x_vwidth+px);
        nvalues++;

        mval = values[0];
        for (ival=0; ival<nvalues; ival++)
        {
           if (values[ival]<mval) mval=values[ival];
        }
        *(x->x_frame+py*x->x_vwidth+px)=mval;
      }
    }

    // backward pass
    for ( py=x->x_vheight-1; py>=0; py-- )
    {
      for ( px=x->x_vwidth-1; px>=0; px-- )
      {
        nvalues = 0;
        if ( ((px-1)>=0) && ((py+1)<x->x_vheight) )
        {
           values[nvalues] = *(pfY+(py+1)*x->x_vwidth+(px-1)) + (x->x_coeff1<<7);
           nvalues++;
        }
        if ( (py+1)<x->x_vheight )
        {
           values[nvalues] = *(pfY+(py+1)*x->x_vwidth+px) + (x->x_coeff2<<7);
           nvalues++;
        }
        if ( ((px+1)<x->x_vwidth) && ((py+1)<x->x_vheight) )
        {
           values[nvalues] = *(pfY+(py+1)*x->x_vwidth+(px+1)) + (x->x_coeff3<<7);
           nvalues++;
        }
        if ( (px+1)<x->x_vwidth )
        {
           values[nvalues] = *(pfY+py*x->x_vwidth+(px+1)) + (x->x_coeff4<<7);
           nvalues++;
        }
        values[nvalues] = *(pfY+py*x->x_vwidth+px);
        nvalues++;

        mval = values[0];
        for (ival=0; ival<nvalues; ival++)
        {
           if (values[ival]<mval) mval=values[ival];
        }
        *(x->x_frame+py*x->x_vwidth+px)=mval;
      }
    }

    memcpy( newdata, x->x_frame, x->x_vsize+(x->x_vsize>>1)<<1 );

    return;
}

static void pdp_distance_sendpacket(t_pdp_distance *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet1);
}

static void pdp_distance_process(t_pdp_distance *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_distance_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_distance_process_yv12, pdp_distance_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_distance_process */
	    break;
	    
	}
    }

}

static void pdp_distance_input_0(t_pdp_distance *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        pdp_distance_process(x);
    }
}

static void pdp_distance_free(t_pdp_distance *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
    pdp_distance_free_ressources( x );
}

t_class *pdp_distance_class;

void *pdp_distance_new(void)
{
    int i;

    t_pdp_distance *x = (t_pdp_distance *)pd_new(pdp_distance_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("coeff1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("coeff2"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("coeff3"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("coeff4"));

    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_vwidth = -1;
    x->x_vheight = -1;
    x->x_vsize = -1;
    x->x_frame = NULL;

    x->x_coeff1 = 4;
    x->x_coeff2 = 3;
    x->x_coeff3 = 4;
    x->x_coeff4 = 3;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_distance_setup(void)
{
    // post( pdp_distance_version );
    pdp_distance_class = class_new(gensym("pdp_distance"), (t_newmethod)pdp_distance_new,
    	(t_method)pdp_distance_free, sizeof(t_pdp_distance), 0, A_NULL);

    class_addmethod(pdp_distance_class, (t_method)pdp_distance_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_distance_class, (t_method)pdp_distance_coeff1, gensym("coeff1"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_distance_class, (t_method)pdp_distance_coeff2, gensym("coeff2"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_distance_class, (t_method)pdp_distance_coeff3, gensym("coeff3"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_distance_class, (t_method)pdp_distance_coeff4, gensym("coeff4"),  A_DEFFLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
