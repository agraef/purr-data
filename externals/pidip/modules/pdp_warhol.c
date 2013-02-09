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

/*  This object is an adaptation of warhol effect from effectv
 *  Copyright (C) 2002 Jun IIO
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

#define NBCOLORS 9
static int colortable[NBCOLORS] = {
        0x000080, 0x008045, 0x07f0e7,
        0x0000f0, 0x00f07f, 0x037a10,
        0x0023d9, 0x0080f0, 0x083df0
};

static char   *pdp_warhol_version = "pdp_warhol: version 0.1, port of warhol from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_warhol_struct
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
    int x_dividerx;
    int x_dividery;
    int x_colorindex;

} t_pdp_warhol;

static void pdp_warhol_dividerx(t_pdp_warhol *x, t_floatarg fdivider )
{
   if ( ( fdivider > 1 ) && ( fdivider < x->x_vwidth ) )
   {
      x->x_dividerx = (int) fdivider;
   }
}

static void pdp_warhol_dividery(t_pdp_warhol *x, t_floatarg fdivider )
{
   if ( ( fdivider > 1 ) && ( fdivider < x->x_vwidth ) )
   {
      x->x_dividery = (int) fdivider;
   }
}

static void pdp_warhol_colorindex(t_pdp_warhol *x, t_floatarg findex )
{
   if ( ( findex >= 0 ) && ( findex < NBCOLORS ) )
   {
      x->x_colorindex = (int) findex;
   }
}

static void pdp_warhol_v(t_pdp_warhol *x, t_floatarg fv )
{
 int tc;

   if ( ( fv >= 0 ) && ( fv < 255 ) )
   {
      tc = colortable[x->x_colorindex] & 0xffff00;
      tc = tc | (int) fv;
      colortable[x->x_colorindex] = tc;
   }
}

static void pdp_warhol_u(t_pdp_warhol *x, t_floatarg fu )
{
 int tc;

   if ( ( fu >= 0 ) && ( fu < 255 ) )
   {
      tc = colortable[x->x_colorindex] & 0xff00ff;
      tc = tc | (((int)fu)<<8);
      colortable[x->x_colorindex] = tc;
   }
}

static void pdp_warhol_y(t_pdp_warhol *x, t_floatarg fy )
{
 int tc;

   if ( ( fy >= 0 ) && ( fy < 255 ) )
   {
      tc = colortable[x->x_colorindex] & 0x00ffff;
      tc = tc | (((int)fy)<<16);
      colortable[x->x_colorindex] = tc;
   }
}

static void pdp_warhol_process_yv12(t_pdp_warhol *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    short int *pny, *pnu, *pnv;
    short int *poy, *pou, *pov;

    short int Y, U, V;
    int p, q, px, py, i;

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    poy = data;
    pou = data + x->x_vsize;
    pov = data + x->x_vsize + (x->x_vsize>>2);
    pny = newdata;
    pnu = newdata + x->x_vsize;
    pnv = newdata + x->x_vsize + (x->x_vsize>>2);
    for (py = 0; py < x->x_vheight; py++)
    {
      for (px = 0; px < x->x_vwidth; px++)
      {
        p = (px * x->x_dividerx) % x->x_vwidth;
        q = (py * x->x_dividery) % x->x_vheight;
        i = ( ((py * x->x_dividery) / x->x_vheight) * x->x_dividery
            + ((px * x->x_dividerx) / x->x_vwidth) ) % NBCOLORS;
        Y = (colortable[i] >> 16);
        U = ( (colortable[i] >> 8) & 0xff );
        V = ( colortable[i] & 0xff);
        *pny = ( *(poy + (q*x->x_vwidth+p) ) ) ^ ( Y<<8 );
        pny++;
        if ( ( px%2==0 ) && ( py%2==0 ) )
        {
          *pnu = ( ( U - 128 << 7 ) );
          *pnv = ( ( V - 128 << 7 ) );
          // *pnu = ( *(pou + ((q>>1)*(x->x_vwidth>>1)+(p>>1)) ) ) ^ ( ( U - 128 << 7 ) );
          // *pnv = ( *(pov + ((q>>1)*(x->x_vwidth>>1)+(p>>1)) ) ) ^ ( ( V - 128 << 7 ) );
          pnu++; pnv++;
        }
      }
    }
    
    return;
}

static void pdp_warhol_sendpacket(t_pdp_warhol *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_warhol_process(t_pdp_warhol *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_warhol_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_warhol_process_yv12, pdp_warhol_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_warhol_process */
	    break;
	    
	}
    }
}

static void pdp_warhol_input_0(t_pdp_warhol *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_warhol_process(x);
    }
}

static void pdp_warhol_free(t_pdp_warhol *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_warhol_class;

void *pdp_warhol_new(void)
{
    int i;

    t_pdp_warhol *x = (t_pdp_warhol *)pd_new(pdp_warhol_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("dividerx"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("dividery"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("colorindex"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("Y"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("U"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("V"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_dividerx = 3;
    x->x_dividery = 3;
    x->x_colorindex = 0;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_warhol_setup(void)
{
//    post( pdp_warhol_version );
    pdp_warhol_class = class_new(gensym("pdp_warhol"), (t_newmethod)pdp_warhol_new,
    	(t_method)pdp_warhol_free, sizeof(t_pdp_warhol), 0, A_NULL);

    class_addmethod(pdp_warhol_class, (t_method)pdp_warhol_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_warhol_class, (t_method)pdp_warhol_dividerx, gensym("dividerx"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_warhol_class, (t_method)pdp_warhol_dividery, gensym("dividery"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_warhol_class, (t_method)pdp_warhol_colorindex, gensym("colorindex"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_warhol_class, (t_method)pdp_warhol_y, gensym("Y"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_warhol_class, (t_method)pdp_warhol_u, gensym("U"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_warhol_class, (t_method)pdp_warhol_v, gensym("V"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
