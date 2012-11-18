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

/*  This object is a dot matrix filter
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <stdio.h>

static char   *pdp_dot_version = "pdp_dot: dot matrix filter version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_dot_struct
{
    t_object x_obj;

    int x_packet0;
    int x_packet1;
    int x_queue_id;
    int x_dropped;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

    int x_nbx;
    int x_nby;
    char *x_mask;  // pre-calculated mask

    t_outlet *x_pdp_output; // output packets

} t_pdp_dot;

static void pdp_dot_update_mask(t_pdp_dot *x)
{
  int   dotsizeX, dotsizeY;
  int   ray;
  int   curX=0, curY=0;
  char  *pfY;
  int   px, py;

    dotsizeX = (int) ( x->x_vwidth / x->x_nbx );
    dotsizeY = (int) ( x->x_vheight / x->x_nby );
    ray = (dotsizeX>dotsizeY) ? (dotsizeY/2):(dotsizeX/2); 

    // post( "dotsizeX=%d dotsizeY=%d", dotsizeX, dotsizeY );

    pfY = x->x_mask;
    curY=0;

    while ( curY <= ( x->x_vheight - dotsizeY ) )
    {
      curX=0;
      while ( curX <= ( x->x_vwidth - dotsizeX ) )
      {
         for(py=0; py<dotsizeY; py++)
         {
           for(px=0; px<dotsizeX; px++)
           {
              if ( sqrt( pow( (px-(dotsizeX/2)), 2 ) + pow( (py-(dotsizeY/2)), 2 ) ) < ray )
              {
                 *(pfY+(curY+py)*x->x_vwidth+(curX+px))=1;
              }
              else
              {
                 *(pfY+(curY+py)*x->x_vwidth+(curX+px))=0;
              }
           }
         }
         curX += dotsizeX;
         // post ( "curX=%d", curX );
      }
      curY += dotsizeY;
      // post ( "curY=%d", curY );
    }
}

static void pdp_dot_allocate(t_pdp_dot *x)
{
    x->x_mask = (char *) getbytes ( x->x_vsize*sizeof(char*) );

    if ( !x->x_mask )
    {
       post( "pdp_dot : severe error : cannot allocate buffer !!! ");
       return;
    }

}

static void pdp_dot_nbx(t_pdp_dot *x, t_floatarg fnbx )
{
   if ( ( (int)fnbx > 5 ) && ( (int)fnbx <= x->x_vwidth ) )
   {
      x->x_nbx = (int) fnbx;
      pdp_dot_update_mask( x );
   }
}

static void pdp_dot_nby(t_pdp_dot *x, t_floatarg fnby )
{
   if ( ( (int)fnby > 5 ) && ( (int)fnby <= x->x_vheight ) )
   {
      x->x_nby = (int) fnby;
      pdp_dot_update_mask( x );
   }
}

static void pdp_dot_free_ressources(t_pdp_dot *x)
{
   if ( x->x_mask ) freebytes ( x->x_mask, x->x_vsize*sizeof(char*) );
}

static void pdp_dot_process_yv12(t_pdp_dot *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i;
    int     px=0, py=0; 
    short int *pfY, *pfU, *pfV;
    char    *pmY;
    double   meanY, meanU, meanV;
    int   dotsizeX, dotsizeY;
    int   nbpixs, curX=0, curY=0;

    // allocate all ressources
    if ( ( (int)header->info.image.width != x->x_vwidth ) ||
         ( (int)header->info.image.height != x->x_vheight ) )
    {
        pdp_dot_free_ressources( x );
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_dot_allocate( x );
        pdp_dot_update_mask( x );
        post( "pdp_dot : reallocated buffers" );
    }

    memcpy( newdata, data, x->x_vsize+(x->x_vsize>>1)<<1 );

    dotsizeX = (int) ( x->x_vwidth / x->x_nbx );
    dotsizeY = (int) ( x->x_vheight / x->x_nby );

    // post( "dotsizeX=%d dotsizeY=%d", dotsizeX, dotsizeY );

    pmY = x->x_mask;
    pfY = newdata;
    pfV = newdata+x->x_vsize;
    pfU = newdata+x->x_vsize+(x->x_vsize>>2);
    nbpixs=0;
    curY=0;

    while ( curY <= ( x->x_vheight - dotsizeY ) )
    {
      curX=0;
      while ( curX <= ( x->x_vwidth - dotsizeX ) )
      {
         meanY=0;
         meanU=0;
         meanV=0;
         nbpixs=0;
         for(py=0; py<dotsizeY; py++)
         {
           for(px=0; px<dotsizeX; px++)
           {
              meanY += (double)*(pfY+(curY+py)*x->x_vwidth+(curX+px));
              meanU += (double)*(pfU+((curY+py)>>1)*(x->x_vwidth>>1)+((curX+px)>>1));
              meanV += (double)*(pfV+((curY+py)>>1)*(x->x_vwidth>>1)+((curX+px)>>1));
              nbpixs++;
           }
         }
         for(py=0; py<dotsizeY; py++)
         {
           for(px=0; px<dotsizeX; px++)
           {
              *(pfY+(curY+py)*x->x_vwidth+(curX+px))=(meanY/nbpixs)*(*(pmY+(curY+py)*x->x_vwidth+(curX+px)));
              *(pfU+((curY+py)>>1)*(x->x_vwidth>>1)+((curX+px)>>1))=(meanU/nbpixs)*(*(pmY+(curY+py)*x->x_vwidth+(curX+px)));
              *(pfV+((curY+py)>>1)*(x->x_vwidth>>1)+((curX+px)>>1))=(meanV/nbpixs)*(*(pmY+(curY+py)*x->x_vwidth+(curX+px)));
           }
         }
         curX += dotsizeX;
         // post ( "curX=%d", curX );
      }
      if ( curX < x->x_vwidth )
      {
        curX -= dotsizeX;
        for(py=0; py<dotsizeY; py++)
        {
           for(px=curX; px<x->x_vwidth; px++)
           {
              if ( (curY+py) < x->x_vheight )
              {
                *(pfY+(curY+py)*x->x_vwidth+(px))=0;
              }
           }
        }
      }
      curY += dotsizeY;
      // post ( "curY=%d", curY );
    }
    if ( curY < x->x_vheight )
    {
      curY -= dotsizeY;
      for(py=curY; py<x->x_vheight; py++)
      {
         for(px=0; px<x->x_vwidth; px++)
         {
            *(pfY+(py)*x->x_vwidth+(px))=0;
         }
      }
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    return;
}

static void pdp_dot_sendpacket(t_pdp_dot *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet1);
}

static void pdp_dot_process(t_pdp_dot *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_dot_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_dot_process_yv12, pdp_dot_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_dot_process */
	    break;
	    
	}
    }

}

static void pdp_dot_input_0(t_pdp_dot *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        pdp_dot_process(x);
    }
}

static void pdp_dot_free(t_pdp_dot *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
    pdp_dot_free_ressources( x );
}

t_class *pdp_dot_class;

void *pdp_dot_new(void)
{
    int i;

    t_pdp_dot *x = (t_pdp_dot *)pd_new(pdp_dot_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("nbx"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("nby"));

    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_vwidth = -1;
    x->x_vheight = -1;
    x->x_vsize = -1;

    x->x_nbx = 100;
    x->x_nby = 100;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_dot_setup(void)
{
    // post( pdp_dot_version );
    pdp_dot_class = class_new(gensym("pdp_dot"), (t_newmethod)pdp_dot_new,
    	(t_method)pdp_dot_free, sizeof(t_pdp_dot), 0, A_NULL);

    class_addmethod(pdp_dot_class, (t_method)pdp_dot_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_dot_class, (t_method)pdp_dot_nbx, gensym("nbx"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_dot_class, (t_method)pdp_dot_nby, gensym("nby"),  A_DEFFLOAT, A_NULL);
}

#ifdef __cplusplus
}
#endif
