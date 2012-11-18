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

/*  This object is an adaptation of lens effect from effectv
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include "yuv.h"
#include <math.h>

static char   *pdp_spotlight_version = "pdp_spotlight: version 0.1, specially made for cabaret, written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_spotlight_struct
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
    int x_cx;         // coordinates of lens center
    int x_cy;         // coordinates of lens center
    int x_ssize;      // width of the spotlight
    t_float x_strength; // strength of the light (0<=strength<=1) 
    
    int x_colorR;   // red component of the color
    int x_colorG;   // green component of the color
    int x_colorB;   // blue component of the color

} t_pdp_spotlight;

static void pdp_spotlight_ssize(t_pdp_spotlight *x, t_floatarg fssize )
{
    if ( fssize>=0 )
    {
       x->x_ssize = (int)fssize;
    }
}

static void pdp_spotlight_cy(t_pdp_spotlight *x, t_floatarg fcy )
{
    if ( fcy>0 )
    {
       x->x_cy = (int)fcy;
    }
}

static void pdp_spotlight_cx(t_pdp_spotlight *x, t_floatarg fcx )
{
    if ( fcx>0 )
    {
       x->x_cx = (int)fcx;
    }
}

static void pdp_spotlight_r(t_pdp_spotlight *x, t_floatarg fr )
{
   if ( ( fr >= 0 ) && ( fr <= 255 ) )
   {
      x->x_colorR = (int)fr;
   }
}

static void pdp_spotlight_g(t_pdp_spotlight *x, t_floatarg fg )
{
   if ( ( fg >= 0 ) && ( fg <= 255 ) )
   {
      x->x_colorG = (int)fg;
   }
}

static void pdp_spotlight_b(t_pdp_spotlight *x, t_floatarg fb )
{
   if ( ( fb >= 0 ) && ( fb <= 255 ) )
   {
      x->x_colorB = (int)fb;
   }
}

static void pdp_spotlight_strength(t_pdp_spotlight *x, t_floatarg fstrength )
{
   if ( ( fstrength >= 0.0 ) && ( fstrength <= 1.0 ) )
   {
      x->x_strength = fstrength;
   }
}

static void pdp_spotlight_process_yv12(t_pdp_spotlight *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i;

    short int *poy, *pou, *pov, *pny, *pnu, *pnv;
    short int pmx, pMx, pmy, pMy;
    int px, py, ray2;

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy(newdata, data, (x->x_vsize + (x->x_vsize>>1))<<1);

    poy = data;
    pou = data + x->x_vsize;
    pov = data + x->x_vsize + (x->x_vsize>>2);
    pny = newdata;
    pnu = newdata + x->x_vsize;
    pnv = newdata + x->x_vsize + (x->x_vsize>>2);
    if ( x->x_cy-x->x_ssize < 0 ) 
    {
      pmy=0; 
    }
    else
    {
      pmy=x->x_cy-x->x_ssize; 
    }
    if ( x->x_cy+x->x_ssize > x->x_vheight ) 
    {
      pMy=x->x_vheight-1; 
    }
    else
    {
      pMy=x->x_cy+x->x_ssize; 
    }
    if ( x->x_cx-x->x_ssize < 0 ) 
    {
      pmx=0; 
    }
    else
    {
      pmx=x->x_cx-x->x_ssize; 
    }
    if ( x->x_cx+x->x_ssize > x->x_vwidth ) 
    {
      pMx=x->x_vwidth-1; 
    }
    else
    {
      pMx=x->x_cx+x->x_ssize; 
    }
    ray2 = pow( x->x_ssize, 2 );
    for (py = pmy; py <= pMy ; py++) 
    {
      for (px = pmx; px <= pMx; px++) 
      {
        if ( ( pow( (px-x->x_cx), 2 ) + pow( (py-x->x_cy), 2 ) ) < ray2 )
        {
           *(pny+py*x->x_vwidth+px) = 
              (t_float)(*(pny+py*x->x_vwidth+px))*(1.-x->x_strength) +
                (t_float)(((yuv_RGBtoY( (x->x_colorB << 16) + (x->x_colorG << 8) + x->x_colorR ))<<7))
                                *(x->x_strength);
           *(pnu+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
              (t_float)(*(pnu+(py>>1)*(x->x_vwidth>>1)+(px>>1)))*(1.-x->x_strength) +  
                (t_float)(((yuv_RGBtoU( (x->x_colorB << 16) + (x->x_colorG << 8) 
                               + x->x_colorR ))-128)<<8)*(x->x_strength);
           *(pnv+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 
              (t_float)(*(pnv+(py>>1)*(x->x_vwidth>>1)+(px>>1)))*(1.-x->x_strength) +
                (t_float)(((yuv_RGBtoV( (x->x_colorB << 16) + (x->x_colorG << 8) 
                               + x->x_colorR ))-128)<<8)*(x->x_strength);
        }
      }
    }

    return;
}

static void pdp_spotlight_sendpacket(t_pdp_spotlight *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_spotlight_process(t_pdp_spotlight *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_spotlight_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_spotlight_process_yv12, pdp_spotlight_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_spotlight_process */
	    break;
	    
	}
    }
}

static void pdp_spotlight_input_0(t_pdp_spotlight *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_spotlight_process(x);

    }
}

static void pdp_spotlight_free(t_pdp_spotlight *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_spotlight_class;

void *pdp_spotlight_new(void)
{
    int i;

    t_pdp_spotlight *x = (t_pdp_spotlight *)pd_new(pdp_spotlight_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cx"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cy"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ssize"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("r"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("g"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("b"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("strength"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_cx = 70;
    x->x_cy = 70;
    x->x_ssize = 50;
    x->x_colorR = 255;
    x->x_colorG = 255;
    x->x_colorB = 255;
    x->x_strength = 0.5;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_spotlight_setup(void)
{
//    post( pdp_spotlight_version );
    pdp_spotlight_class = class_new(gensym("pdp_spotlight"), (t_newmethod)pdp_spotlight_new,
    	(t_method)pdp_spotlight_free, sizeof(t_pdp_spotlight), 0, A_NULL);

    class_addmethod(pdp_spotlight_class, (t_method)pdp_spotlight_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_spotlight_class, (t_method)pdp_spotlight_cx, gensym("cx"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spotlight_class, (t_method)pdp_spotlight_cy, gensym("cy"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spotlight_class, (t_method)pdp_spotlight_ssize, gensym("ssize"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spotlight_class, (t_method)pdp_spotlight_r, gensym("r"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spotlight_class, (t_method)pdp_spotlight_g, gensym("g"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spotlight_class, (t_method)pdp_spotlight_b, gensym("b"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_spotlight_class, (t_method)pdp_spotlight_strength, gensym("strength"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
