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

/*  This object is a port of shagadelic effect from EffecTV
 *  Originally written by Fukuchi Kentaro
 *  Pd-fication by Yves Degoyon ( ydegoyon@free.fr )                             
 */


#include "pdp.h"
#include <math.h>

#define MAX_TABLES 6
static unsigned int fastrand_val;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)

static char   *pdp_shagadelic_version = "pdp_shagadelic: version 0.1, port of cycle from EffecTV by clifford smith, adapted by ydegoyon@free.fr ";

typedef struct pdp_shagadelic_struct
{
    t_object x_obj;
    t_float x_f;

    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    t_outlet *x_outlet0;
    int x_vwidth;
    int x_vheight;
    int x_vsize;

    /* shagadelic parameters */
    char *x_ripple;
    char *x_spiral;
    unsigned char x_phase;
    int x_rx, x_ry;
    int x_bx, x_by;
    int x_rvx, x_rvy;
    int x_bvx, x_bvy;
    short int x_mask;

} t_pdp_shagadelic;

static void pdp_shagadelic_mask(t_pdp_shagadelic *x, t_floatarg fmask )
{
    if ( ( fmask >= 0 ) || ( fmask < 65536 ) )
    {
       x->x_mask = fmask;
    }
}

static int pdp_shagadelic_map_from_table(t_pdp_shagadelic *x, int px, int py, int t) 
{
  int xd,yd;

    yd = py + (inline_fastrand() >> 30)-2;
    xd = px + (inline_fastrand() >> 30)-2;
    if (xd > x->x_vwidth) {
      xd-=1;
    }
    return (xd+yd*x->x_vwidth);
}

static void pdp_shagadelic_init_tables(t_pdp_shagadelic *x)
{
  int px, py, i;
  double xx, yy;

   i = 0;
   for(py=0; py<x->x_vheight*2; py++) 
   {
     yy = py - x->x_vheight;
     yy *= yy;
     for(px=0; px<x->x_vwidth*2; px++) 
     {
       xx = px - x->x_vwidth;
       x->x_ripple[i++] = ((unsigned int)(sqrt(xx*xx+yy)*8))&255;
     }
   }
   i = 0;
   for(py=0; py<x->x_vheight; py++) 
   {
     yy = py - x->x_vheight/2;
     for(px=0; px<x->x_vwidth; px++) 
     {
        xx = px - x->x_vwidth/2;
        x->x_spiral[i++] = ((unsigned int) ((atan2(xx, yy)/M_PI*256*9) + (sqrt(xx*xx+yy*yy)*5)))&255;
     }
   }

   x->x_rx = inline_fastrand()%x->x_vwidth;
   x->x_ry = inline_fastrand()%x->x_vheight;
   x->x_bx = inline_fastrand()%x->x_vwidth;
   x->x_by = inline_fastrand()%x->x_vheight;
}

static void pdp_shagadelic_free_ressources(t_pdp_shagadelic *x)
{
    if (x->x_ripple) freebytes( x->x_ripple, x->x_vsize*4 );
    if (x->x_spiral) freebytes( x->x_spiral, x->x_vsize );
}

static void pdp_shagadelic_allocate(t_pdp_shagadelic *x)
{
    x->x_ripple = (char *) getbytes( x->x_vsize*4 );
    x->x_spiral = (char *) getbytes( x->x_vsize );
}

static void pdp_shagadelic_process_yv12(t_pdp_shagadelic *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i;
    int     px, py;
    unsigned  char y, u, v;
    char      *p_y, *p_u, *p_v;


    /* allocate all ressources */
    if ( ((int)header->info.image.width != x->x_vwidth) ||
         ((int)header->info.image.height != x->x_vheight) )
    {
        pdp_shagadelic_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_shagadelic_allocate(x);
        post( "pdp_shagadelic : reallocated buffers" );
        pdp_shagadelic_init_tables(x);
        post( "pdp_shagadelic : initialized tables" );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    p_y = &x->x_ripple[x->x_ry*x->x_vwidth*2 + x->x_rx];
    p_u = x->x_spiral;
    p_v = &x->x_ripple[x->x_by*x->x_vwidth*2 + x->x_bx];

    for(py=0; py<x->x_vheight; py++) 
    {
      for(px=0; px<x->x_vwidth; px++) 
      {
         y = (char)(*p_y+x->x_phase*2)>>7;
         u = (char)(*p_u+x->x_phase*3)>>7;
         v = (char)(*p_v-x->x_phase)>>7;
         *(newdata+py*x->x_vwidth+px) = *(data) & (y<<7) & x->x_mask;
         *(newdata+x->x_vsize+((py*x->x_vwidth+px)>>2)) = *(data) & ((u-128)<<8) & x->x_mask;
         *(newdata+x->x_vsize+(x->x_vsize>>2)+((py*x->x_vwidth+px)>>2)) = *(data) & ((v-128)<<8) & x->x_mask;
         p_y++;
         p_u++;
         p_v++;
         data++;
      }
      p_y += x->x_vwidth;
      p_v += x->x_vwidth;
    }

    x->x_phase -= 8;
    if((x->x_rx+x->x_rvx)<0 || (x->x_rx+x->x_rvx)>=x->x_vwidth) x->x_rvx =-x->x_rvx;
    if((x->x_ry+x->x_rvy)<0 || (x->x_ry+x->x_rvy)>=x->x_vheight) x->x_rvy =-x->x_rvy;
    if((x->x_bx+x->x_bvx)<0 || (x->x_bx+x->x_bvx)>=x->x_vwidth) x->x_bvx =-x->x_bvx;
    if((x->x_by+x->x_bvy)<0 || (x->x_by+x->x_bvy)>=x->x_vheight) x->x_bvy =-x->x_bvy;

    x->x_rx += x->x_rvx;
    x->x_ry += x->x_rvy;
    x->x_bx += x->x_bvx;
    x->x_by += x->x_bvy;

    return;
}

static void pdp_shagadelic_sendpacket(t_pdp_shagadelic *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_shagadelic_process(t_pdp_shagadelic *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_shagadelic_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_shagadelic_process_yv12, pdp_shagadelic_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_shagadelic_process */
	    break;
	    
	}
    }

}

static void pdp_shagadelic_input_0(t_pdp_shagadelic *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_shagadelic_process(x);

    }

}

static void pdp_shagadelic_free(t_pdp_shagadelic *x)
{
  int i;

    pdp_shagadelic_free_ressources(x);
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_shagadelic_class;

void *pdp_shagadelic_new(void)
{
    int i;

    t_pdp_shagadelic *x = (t_pdp_shagadelic *)pd_new(pdp_shagadelic_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("mask"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_rvx = -2;
    x->x_rvy = -2;
    x->x_bvx = 2;
    x->x_bvy = 2;
    x->x_phase = 0;
    x->x_mask = 0xffff;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_shagadelic_setup(void)
{
//    post( pdp_shagadelic_version );
    pdp_shagadelic_class = class_new(gensym("pdp_shagadelic"), (t_newmethod)pdp_shagadelic_new,
    	(t_method)pdp_shagadelic_free, sizeof(t_pdp_shagadelic), 0, A_NULL);

    class_addmethod(pdp_shagadelic_class, (t_method)pdp_shagadelic_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_shagadelic_class, (t_method)pdp_shagadelic_mask, gensym("mask"),  A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
