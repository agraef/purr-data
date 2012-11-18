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

/*  This object is a motion detection grid
 */

#include "pdp.h"
#include <math.h>

#define DEFAULT_X_DIM 10
#define DEFAULT_Y_DIM 10
#define DEFAULT_THRESHOLD 20
#define DEFAULT_COLOR 128

static char   *pdp_mgrid_version = "pdp_mgrid: a motion detection grid version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_mgrid_struct
{
    t_object x_obj;
    t_float x_f;

    int x_packet0;
    int x_dropped;

    int x_vwidth;
    int x_vheight;
    int x_vsize;
    short int *x_previous_frame;
    int x_xdim;
    int x_ydim;
    int x_threshold;
    short int x_color;
    int x_firstimage;

    t_outlet *x_pdp_output; // output packets
    t_outlet *x_xmotion; // output x coordinate of block which has been detected
    t_outlet *x_ymotion; // output y coordinate of block which has been detected


} t_pdp_mgrid;

static void pdp_mgrid_free_ressources(t_pdp_mgrid *x)
{
    if ( x->x_previous_frame ) freebytes ( x->x_previous_frame, ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );
}

static void pdp_mgrid_allocate(t_pdp_mgrid *x)
{
    x->x_previous_frame = (short int *) getbytes ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );

    if ( !x->x_previous_frame )
    {
       post( "pdp_mgrid : severe error : cannot allocate buffer !!! ");
       return;
    }
}

static void pdp_mgrid_x_dim(t_pdp_mgrid *x, t_floatarg fxdim )
{
   if ( x->x_previous_frame == NULL )
   {
      post( "pdp_mgrid : tried to set grid dimension but image dimensions are unknown" );
      return;
   }
   if ( ( fxdim >= 0 ) && ( fxdim < (x->x_vwidth/3) ) )
   {
      x->x_xdim = (int)fxdim;
   }
}

static void pdp_mgrid_y_dim(t_pdp_mgrid *x, t_floatarg fydim )
{
   if ( x->x_previous_frame == NULL )
   {
      post( "pdp_mgrid : tried to set grid dimension but image dimensions are unknown" );
      return;
   }

   if ( ( fydim >= 0 ) && ( fydim < (x->x_vheight/3) ) )
   {
      x->x_ydim = (int)fydim;
   }
}

static void pdp_mgrid_threshold(t_pdp_mgrid *x, t_floatarg fthreshold )
{
   if ( fthreshold > 0 )
   {
      x->x_threshold = (int)fthreshold;
   }
}

static void pdp_mgrid_color(t_pdp_mgrid *x, t_floatarg fcolor )
{
   if ( ( fcolor >= 0 ) && ( fcolor < 255 ) )
   {
      x->x_color = (int)fcolor;
   }
}

static void pdp_mgrid_process_yv12(t_pdp_mgrid *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    int     i, cf;
    int     px=0, py=0, xcell=0, ycell=0; 
    int     celldiff=0, cellwidth=0, cellheight=0;
    int     yindex=0, uindex=0, vindex=0;

    /* allocate all ressources */
    if ( ( (int)header->info.image.width != x->x_vwidth ) ||
         ( (int)header->info.image.height != x->x_vheight ) )
    {
        pdp_mgrid_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_mgrid_allocate(x);
        x->x_firstimage = 1;
        post( "pdp_mgrid : reallocated buffers" );
    }

    // draw horizontal lines
    if ( x->x_ydim > 0 )
    {
     for(py=0; py<x->x_vheight; py+=(x->x_vheight/x->x_ydim))
     {
      if ( py >= x->x_vheight ) break;
      for(px=0; px<x->x_vwidth; px++)
      {
        data[py*x->x_vwidth+px] = 
          ( ( data[py*x->x_vwidth+px] >> 7 ) ^ x->x_color) << 7;
      }
     }
    }

    // draw vertical lines
    if ( x->x_xdim > 0 )
    {
     for(px=0; px<x->x_vwidth; px+=(x->x_vwidth/x->x_xdim))
     {
      if ( px >= x->x_vwidth ) break;
      for(py=0; py<x->x_vheight; py++)
      {
        data[py*x->x_vwidth+px] = 
          ( ( data[py*x->x_vwidth+px] >> 7 ) ^ x->x_color) << 7;
      }
     }
    }

    if ( !x->x_firstimage )
    {
      // detect cell where a movement occurred
      ycell=0;
      celldiff=0;
      if ( x->x_xdim > 0 )
      {
         cellwidth=(x->x_vwidth/x->x_xdim);
      }
      else
      {
         cellwidth=x->x_vwidth;
      }
      if ( x->x_ydim > 0 )
      {
         cellheight=(x->x_vheight/x->x_ydim);
      }
      else
      {
         cellheight=x->x_vheight;
      }
      for(xcell=0; xcell<x->x_xdim; xcell++)
      {
        for(ycell=0; ycell<x->x_ydim; ycell++)
        {
           celldiff=0;
           for ( px=xcell*cellwidth; px<(xcell+1)*cellwidth; px++ )
           {
             if ( px >= x->x_vwidth ) break;
             for ( py=ycell*cellheight; py<(ycell+1)*cellheight; py++ )
             {
               if ( py >= x->x_vheight ) break;
               yindex = py*x->x_vwidth+px;
               uindex = x->x_vsize + ((py*x->x_vwidth>>2)+(px>>1));
               vindex = x->x_vsize + (x->x_vsize>>2) + ((py*x->x_vwidth>>2)+(px>>1));

               // this calculation although more accurate is heavy
               // celldiff += sqrt( pow(((data[ yindex ]>>7) - (x->x_previous_frame[ yindex ]>>7)), 2)
               //             + pow(((data[ uindex ]>>8) - (x->x_previous_frame[ uindex ]>>8)), 2)
               //             + pow(((data[ vindex ]>>8) - (x->x_previous_frame[ vindex ]>>8)), 2));
               celldiff += abs(((data[ yindex ]>>7) - (x->x_previous_frame[ yindex ]>>7)))
                           + abs(((data[ uindex ]>>8) - (x->x_previous_frame[ uindex ]>>8)))
                           + abs(((data[ vindex ]>>8) - (x->x_previous_frame[ vindex ]>>8)));
             }
           }
           if ( celldiff > x->x_threshold*cellwidth*cellheight )
           {
  	    outlet_float(x->x_xmotion, xcell+1);
  	    outlet_float(x->x_ymotion, ycell+1);
           }
           // post( "pdp_mgrid : cell [%d,%d] diff=%d", xcell, ycell, celldiff );
        }
      }
    }
    else
    {
      x->x_firstimage = 0;
    }

    memcpy(x->x_previous_frame, data, (x->x_vsize + (x->x_vsize>>1))<<1 );

    pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet0);
    
    return;
}

static void pdp_mgrid_process(t_pdp_mgrid *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_mgrid_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            pdp_mgrid_process_yv12(x);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_mgrid_process */
	    break;
	    
	}
    }
}

static void pdp_mgrid_input_0(t_pdp_mgrid *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        pdp_mgrid_process(x);

    }
}

static void pdp_mgrid_free(t_pdp_mgrid *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
    pdp_mgrid_free_ressources(x);
}

t_class *pdp_mgrid_class;

void *pdp_mgrid_new(void)
{
    int i;

    t_pdp_mgrid *x = (t_pdp_mgrid *)pd_new(pdp_mgrid_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("threshold"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("dimx"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("dimy"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("color"));

    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything); 
    x->x_xmotion = outlet_new(&x->x_obj, &s_float); 
    x->x_ymotion = outlet_new(&x->x_obj, &s_float); 

    x->x_packet0 = -1;

    x->x_previous_frame = NULL;
    x->x_xdim = DEFAULT_X_DIM;
    x->x_ydim = DEFAULT_Y_DIM;
    x->x_threshold = DEFAULT_THRESHOLD;
    x->x_color = DEFAULT_COLOR;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_mgrid_setup(void)
{
    // post( pdp_mgrid_version );
    pdp_mgrid_class = class_new(gensym("pdp_mgrid"), (t_newmethod)pdp_mgrid_new,
    	(t_method)pdp_mgrid_free, sizeof(t_pdp_mgrid), 0, A_NULL);

    class_addmethod(pdp_mgrid_class, (t_method)pdp_mgrid_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_mgrid_class, (t_method)pdp_mgrid_threshold, gensym("threshold"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_mgrid_class, (t_method)pdp_mgrid_x_dim, gensym("dimx"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_mgrid_class, (t_method)pdp_mgrid_y_dim, gensym("dimy"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_mgrid_class, (t_method)pdp_mgrid_color, gensym("color"),  A_FLOAT, A_NULL);



}

#ifdef __cplusplus
}
#endif
