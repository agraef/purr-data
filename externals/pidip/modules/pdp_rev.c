/*
 *   PiDiP module
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

/*  This object is an adaptation of rev effect from effectv
 *  (c)2002 Ed Tannenbaum
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

static char   *pdp_rev_version = "pdp_rev: version 0.1, port of rev from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_rev_struct
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
    int x_vgrabtime;
    int x_vgrab;
    int x_linespace;
    int x_vscale;
    int x_vcolor;

} t_pdp_rev;

static void pdp_rev_vgrabtime(t_pdp_rev *x, t_floatarg fvgrabtime )
{
   if ( fvgrabtime > 1 )
   {
       x->x_vgrabtime = (int)fvgrabtime;
   }
}

static void pdp_rev_linespace(t_pdp_rev *x, t_floatarg flinespace )
{
   if ( flinespace > 1 )
   {
       x->x_linespace = (int)flinespace;
   }
}

static void pdp_rev_vscale(t_pdp_rev *x, t_floatarg fvscale )
{
   if ( fvscale > 1 )
   {
       x->x_vscale = (int)fvscale;
   }
}

static void pdp_rev_color(t_pdp_rev *x, t_floatarg fvcolor )
{
   if ( ( fvcolor > 0 ) && ( fvcolor < 0xffff ) )
   {
       x->x_vcolor = ((int)fvcolor)<<8;
   }
}

void pdp_rev_vasulka(t_pdp_rev *x, short int *src, short int *dst, int srcx, int srcy, int dstx, int dsty, int w, int h)
{
   short int *cdst=dst+((dsty*x->x_vwidth)+dstx);
   short int *nsrc, *nusrc, *nvsrc;
   int py,px,Y,U,V,yval;
   int offset;

   // draw the offset lines
   for (py=srcy; py<h+srcy; py+=x->x_linespace)
   {
      for(px=srcx; px<=w+srcx; px++) 
      {
         nsrc=src+(py*x->x_vwidth)+px;
         nusrc=src+(((py*x->x_vwidth)+px)>>2)+x->x_vsize;
         nvsrc=src+(((py*x->x_vwidth)+px)>>2)+x->x_vsize+(x->x_vsize>>2);
         // Calc Y Value for curpix
         Y = (*nsrc);
         U = (*nusrc);
         V = (*nvsrc);
         yval = py - ((short)(Y + U + V) / x->x_vscale) ;
         offset = px + yval * x->x_vwidth;
         if(offset >= 0 && offset < x->x_vsize ) 
         {
              cdst[offset]=x->x_vcolor;
              cdst[x->x_vsize+(offset>>2)]=x->x_vcolor;
              cdst[x->x_vsize+(x->x_vsize>>2)+(offset>>2)]=x->x_vcolor;
         }
      }
   }
}

static void pdp_rev_process_yv12(t_pdp_rev *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i;

    unsigned int totalnbpixels;

    totalnbpixels = x->x_vsize;

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, (x->x_vsize + (x->x_vsize>>1))<<1 );
 
    x->x_vgrab++;
    if (x->x_vgrab >= x->x_vgrabtime)
    {
        x->x_vgrab=0;
        pdp_rev_vasulka(x, data, newdata, 0, 0, 0, 0, x->x_vwidth, x->x_vheight);
    }

    return;
}

static void pdp_rev_sendpacket(t_pdp_rev *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_rev_process(t_pdp_rev *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_rev_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_rev_process_yv12, pdp_rev_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_rev_process */
	    break;
	    
	}
    }

}

static void pdp_rev_input_0(t_pdp_rev *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
        x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_rev_process(x);
    }
}

static void pdp_rev_free(t_pdp_rev *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_rev_class;

void *pdp_rev_new(void)
{
    int i;

    t_pdp_rev *x = (t_pdp_rev *)pd_new(pdp_rev_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("vgrabtime"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("linespace"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("vscale"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("color"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_vgrabtime = 1;
    x->x_vgrab = 0;
    x->x_linespace = 6;
    x->x_vscale = 50;
    x->x_vcolor = 0xffff;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_rev_setup(void)
{
//    post( pdp_rev_version );
    pdp_rev_class = class_new(gensym("pdp_rev"), (t_newmethod)pdp_rev_new,
    	(t_method)pdp_rev_free, sizeof(t_pdp_rev), 0, A_NULL);

    class_addmethod(pdp_rev_class, (t_method)pdp_rev_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_rev_class, (t_method)pdp_rev_vgrabtime, gensym("vgrabtime"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_rev_class, (t_method)pdp_rev_linespace, gensym("linespace"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_rev_class, (t_method)pdp_rev_vscale, gensym("vscale"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_rev_class, (t_method)pdp_rev_color, gensym("color"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
