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

/*  This object is a YUV space color filter, based on some (u,v) components
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <stdio.h>
#include "g_canvas.h"

static char   *pdp_ycfilter_version = "pdp_ycfilter: a YUV color filter version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_ycfilter_struct
{
    t_object x_obj;

    int x_packet0;
    int x_packet1;
    int x_queue_id;
    int x_dropped;

    int x_vwidth;
    int x_vheight;
    int x_vsize;
    int x_colorY; // YUV components of binary mask
    int x_colorU;
    int x_colorV;
    int x_colorR; // RGB components of binary mask
    int x_colorG;
    int x_colorB;
    int x_cursX; // X position of the cursor
    int x_cursY; // Y position of the cursor
    int x_tolerance; // tolerance 
    int x_luminosity; // use luminosity component (Y) 
    short int *x_frame;  // keep a copy of current frame for picking color

    t_outlet *x_pdp_output; // output packets
    t_outlet *x_Y;  // output Y component of selected color
    t_outlet *x_U;  // output U component of selected color
    t_outlet *x_V;  // output V component of selected color

    t_canvas *x_canvas;

} t_pdp_ycfilter;

static void pdp_ycfilter_draw_color(t_pdp_ycfilter *x)
{
 int width, height;
 char color[32];

    sprintf( color, "#%.2X%.2X%.2X", x->x_colorR, x->x_colorG, x->x_colorB );
    width = rtext_width( glist_findrtext( (t_glist*)x->x_canvas, (t_text *)x ) );
    height = rtext_height( glist_findrtext( (t_glist*)x->x_canvas, (t_text *)x ) );
    sys_vgui(".x%x.c delete rectangle %xCOLOR\n", x->x_canvas, x );
    sys_vgui(".x%x.c create rectangle %d %d %d %d -fill %s -tags %xCOLOR\n",
             x->x_canvas, x->x_obj.te_xpix+width+5, x->x_obj.te_ypix,
             x->x_obj.te_xpix+width+height+5,
             x->x_obj.te_ypix+height, color, x );
}


static void pdp_ycfilter_y(t_pdp_ycfilter *x, t_floatarg fy )
{
   if ( fy <= 255. && fy >= 0. )
   {
      x->x_colorY = (int)fy;
      outlet_float( x->x_Y, x->x_colorY );
      outlet_float( x->x_U, x->x_colorU );
      outlet_float( x->x_V, x->x_colorV );
      x->x_colorR = yuv_YUVtoR( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorG = yuv_YUVtoG( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorB = yuv_YUVtoB( x->x_colorY, x->x_colorU, x->x_colorV );
      if (glist_isvisible(x->x_canvas)) pdp_ycfilter_draw_color( x );
   }
}

static void pdp_ycfilter_u(t_pdp_ycfilter *x, t_floatarg fu )
{
   if ( fu <= 255. && fu >= 0. )
   {
      x->x_colorU = (int)fu;
      outlet_float( x->x_Y, x->x_colorY );
      outlet_float( x->x_U, x->x_colorU );
      outlet_float( x->x_V, x->x_colorV );
      x->x_colorR = yuv_YUVtoR( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorG = yuv_YUVtoG( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorB = yuv_YUVtoB( x->x_colorY, x->x_colorU, x->x_colorV );
      if (glist_isvisible(x->x_canvas)) pdp_ycfilter_draw_color( x );
   }
}

static void pdp_ycfilter_v(t_pdp_ycfilter *x, t_floatarg fv )
{
   if ( fv < 255 && fv >= 0. )
   {
      x->x_colorV = (int)fv;
      outlet_float( x->x_Y, x->x_colorY );
      outlet_float( x->x_U, x->x_colorU );
      outlet_float( x->x_V, x->x_colorV );
      x->x_colorR = yuv_YUVtoR( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorG = yuv_YUVtoG( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorB = yuv_YUVtoB( x->x_colorY, x->x_colorU, x->x_colorV );
      if (glist_isvisible(x->x_canvas)) pdp_ycfilter_draw_color( x );
   }
}

static void pdp_ycfilter_luminosity(t_pdp_ycfilter *x, t_floatarg f)
{
   if ( (int)f!=0 && (int)f!=1 )
   {
      return;
   }

   x->x_luminosity=(int)f;
}

static void pdp_ycfilter_tolerance(t_pdp_ycfilter *x, t_floatarg ftolerance )
{
   if ( ftolerance >= 0 ) 
   {
      x->x_tolerance = (int)ftolerance;
   }
}

static void pdp_ycfilter_pick(t_pdp_ycfilter *x, t_floatarg X, t_floatarg Y)
{
   x->x_cursX = (int) (X*(t_float)x->x_vwidth);
   x->x_cursY = (int) (Y*(t_float)x->x_vheight);
   // post( "pdp_shape : pick color at : %d,%d", x->x_cursX, x->x_cursY );
   if ( x->x_frame && ( x->x_cursX >= 0 ) && ( x->x_cursX < x->x_vwidth )
        && ( x->x_cursY >= 0 ) && ( x->x_cursY < x->x_vheight ) )
   {
      // post( "pdp_ycfilter : picking up color : x=%d y=%d", x->x_cursX, x->x_cursY );
      x->x_colorY = x->x_frame[ x->x_cursY*x->x_vwidth+x->x_cursX ];
      x->x_colorV = x->x_frame[ x->x_vsize + (x->x_cursY>>1)*(x->x_vwidth>>1)+(x->x_cursX>>1) ];
      x->x_colorU = x->x_frame[ x->x_vsize + (x->x_vsize>>2) + (x->x_cursY>>1)*(x->x_vwidth>>1)+(x->x_cursX>>1) ];
      x->x_colorY = (x->x_colorY>>7);
      x->x_colorV = (x->x_colorV>>8)+128;
      x->x_colorU = (x->x_colorU>>8)+128;
      outlet_float( x->x_Y, x->x_colorY );
      outlet_float( x->x_V, x->x_colorV );
      outlet_float( x->x_U, x->x_colorU );
      x->x_colorR = yuv_YUVtoR( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorG = yuv_YUVtoG( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorB = yuv_YUVtoB( x->x_colorY, x->x_colorU, x->x_colorV );
      if (glist_isvisible(x->x_canvas)) pdp_ycfilter_draw_color( x );
   }
}

static void pdp_ycfilter_allocate(t_pdp_ycfilter *x)
{
    x->x_frame = (short int *) getbytes ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );

    if ( !x->x_frame )
    {
       post( "pdp_ycfilter : severe error : cannot allocate buffer !!! ");
       return;
    }
}

static void pdp_ycfilter_free_ressources(t_pdp_ycfilter *x)
{
    if ( x->x_frame ) freebytes ( x->x_frame, ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );
}

static void pdp_ycfilter_process_yv12(t_pdp_ycfilter *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i;
    int     px=0, py=0; 
    int     y=0, u=0, v=0;
    short int *pfY, *pfU, *pfV;
    short int *pnY, *pnU, *pnV;
    int     diff;

    /* allocate all ressources */
    if ( ( (int)header->info.image.width != x->x_vwidth ) ||
         ( (int)header->info.image.height != x->x_vheight ) )
    {
        pdp_ycfilter_free_ressources( x ); 
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_ycfilter_allocate( x ); 
        post( "pdp_ycfilter : reallocated buffers" );
        outlet_float( x->x_Y, x->x_colorY );
        outlet_float( x->x_U, x->x_colorU );
        outlet_float( x->x_V, x->x_colorV );
    }

    memcpy(x->x_frame, data, (x->x_vsize + (x->x_vsize>>1))<<1 );
    memset(newdata, 0x00, (x->x_vsize + (x->x_vsize>>1))<<1 );

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    // post( "pdp_ycfilter : y=%d, u=%d, v=%d", x->x_colorY, x->x_colorU, x->x_colorV );

    // binarize
    pfY = data;
    pfV = data+x->x_vsize;
    pfU = data+x->x_vsize+(x->x_vsize>>2);
    pnY = newdata;
    pnV = newdata+x->x_vsize;
    pnU = newdata+x->x_vsize+(x->x_vsize>>2);
    for ( py=0; py<x->x_vheight; py++ )
    {
      for ( px=0; px<x->x_vwidth; px++ )
      {
         y = (*(pfY+py*x->x_vwidth+px))>>7;
         v = (*(pfV+(py>>1)*(x->x_vwidth>>1)+(px>>1))>>8)+128;
         u = (*(pfU+(py>>1)*(x->x_vwidth>>1)+(px>>1))>>8)+128;

         diff = 0;
         if ( x->x_luminosity )
         {
            diff += abs(y-x->x_colorY );
         }
         diff += abs(v-x->x_colorV );
         diff += abs(u-x->x_colorU );
       
         if ( diff <= x->x_tolerance )
         {
            *(pnY+py*x->x_vwidth+px) = *(pfY+py*x->x_vwidth+px);
            *(pnU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = *(pfU+(py>>1)*(x->x_vwidth>>1)+(px>>1));
            *(pnV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = *(pfV+(py>>1)*(x->x_vwidth>>1)+(px>>1));
         }
       }
    }

    return;
}

static void pdp_ycfilter_sendpacket(t_pdp_ycfilter *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet1);
}

static void pdp_ycfilter_process(t_pdp_ycfilter *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_ycfilter_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_ycfilter_process_yv12, pdp_ycfilter_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_ycfilter_process */
	    break;
	    
	}
    }

}

static void pdp_ycfilter_input_0(t_pdp_ycfilter *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        pdp_ycfilter_process(x);
    }
}

static void pdp_ycfilter_free(t_pdp_ycfilter *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
    pdp_ycfilter_free_ressources( x );
}

t_class *pdp_ycfilter_class;

void *pdp_ycfilter_new(void)
{
    int i;

    t_pdp_ycfilter *x = (t_pdp_ycfilter *)pd_new(pdp_ycfilter_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("Y"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("U"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("V"));

    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything); 
    x->x_Y  = outlet_new(&x->x_obj, &s_float); 
    x->x_U  = outlet_new(&x->x_obj, &s_float); 
    x->x_V  = outlet_new(&x->x_obj, &s_float); 

    x->x_colorY = 255;
    x->x_colorU = -1;
    x->x_colorV = -1;

    x->x_colorR = yuv_YUVtoR( x->x_colorY, x->x_colorU, x->x_colorV );
    x->x_colorG = yuv_YUVtoG( x->x_colorY, x->x_colorU, x->x_colorV );
    x->x_colorB = yuv_YUVtoB( x->x_colorY, x->x_colorU, x->x_colorV );

    x->x_packet0 = -1;
    x->x_packet1 = -1;

    x->x_cursX = -1;
    x->x_cursY = -1;
    x->x_tolerance = 10;
    x->x_luminosity = 0;

    x->x_vwidth = -1;
    x->x_vheight = -1;
    x->x_vsize = -1;
    x->x_frame = NULL;

    x->x_canvas = canvas_getcurrent();

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_ycfilter_setup(void)
{
    // post( pdp_ycfilter_version );
    pdp_ycfilter_class = class_new(gensym("pdp_ycfilter"), (t_newmethod)pdp_ycfilter_new,
    	(t_method)pdp_ycfilter_free, sizeof(t_pdp_ycfilter), 0, A_NULL);

    class_addmethod(pdp_ycfilter_class, (t_method)pdp_ycfilter_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_ycfilter_class, (t_method)pdp_ycfilter_y, gensym("Y"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_ycfilter_class, (t_method)pdp_ycfilter_u, gensym("U"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_ycfilter_class, (t_method)pdp_ycfilter_v, gensym("V"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_ycfilter_class, (t_method)pdp_ycfilter_pick, gensym("pick"),  A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_ycfilter_class, (t_method)pdp_ycfilter_tolerance, gensym("tolerance"), A_FLOAT, A_NULL);
    class_addmethod(pdp_ycfilter_class, (t_method)pdp_ycfilter_luminosity, gensym("luminosity"), A_FLOAT, A_NULL);
}

#ifdef __cplusplus
}
#endif
