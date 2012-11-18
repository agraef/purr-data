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

/*  This object is an adaptation of 1d effect from effectv
 *  but i found it funnier to rename it as underwatch
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

static char   *pdp_underwatch_version = "pdp_underwatch: version 0.1, inspired by 1d from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_underwatch_struct
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
    int x_line;
    int x_sline;
    int x_sheight;
    int x_prevline;
    int x_prevsline;
    int x_prevsheight;
    int x_stripsize;

} t_pdp_underwatch;

static void pdp_underwatch_setparams(t_pdp_underwatch *x)
{
   int snext;

     x->x_sline = x->x_line;
     snext = (x->x_line + 1);
     x->x_sheight = snext - x->x_sline;
}

static void pdp_underwatch_process_yv12(t_pdp_underwatch *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    short int *p=0, *po=0, *pu=0, *pv=0, *pou=0, *pov=0;
    int       i;

    int px, py, pd, t;

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    /* copy region */
    for (pd=0; pd<x->x_stripsize; pd++ )
    {
      p = newdata+x->x_vwidth*x->x_sline;
      pu = newdata+((x->x_vwidth*x->x_sline)>>2)+x->x_vsize;
      pv = newdata+((x->x_vwidth*x->x_sline)>>2)+x->x_vsize+(x->x_vsize>>2);
      po = data+x->x_vwidth*x->x_line;
      pou = data+((x->x_vwidth*x->x_line)>>2)+x->x_vsize;
      pov = data+((x->x_vwidth*x->x_line)>>2)+x->x_vsize+(x->x_vsize>>2);
      // post("INIT : pov=%x limit=%x", pov, data+x->x_vsize+(x->x_vsize>>1) );
      for(py=0; py<=x->x_sheight; py++) 
      {
        for(px=0; px<x->x_vwidth; px++) 
        {
           if( po < data+x->x_vsize+(x->x_vsize>>1) ) *p = *po;
           if( pou < data+x->x_vsize+(x->x_vsize>>1) ) *(pu) = *(pou);
           if( pov < data+x->x_vsize+(x->x_vsize>>1) ) *(pv) = *(pov);
           p++;
           po++;
           if ( ((px+1)%2==0) && ((py+1)%2==0) ) { pu++; pv++; pou++; pov++; };
         }
      }
      x->x_prevline = x->x_line;
      x->x_prevsline = x->x_sline;
      x->x_prevsheight = x->x_sheight;
      x->x_line=(x->x_line+1)%(x->x_vheight);
      pdp_underwatch_setparams(x);
      //p = newdata + x->x_vwidth * x->x_sline+1;
      //for(px=0; px<x->x_vwidth; px++) 
      //{
      //    p[px] = 0xff00;
      //}
    }
    
    return;
}

static void pdp_underwatch_sendpacket(t_pdp_underwatch *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_underwatch_process(t_pdp_underwatch *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_underwatch_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_underwatch_process_yv12, pdp_underwatch_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_underwatch_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_underwatch_process */
	    break;
	    
	}
    }
}

static void pdp_underwatch_stripsize(t_pdp_underwatch *x, t_floatarg fstripsize )
{
    if ( fstripsize>0 && fstripsize<x->x_vheight )
    {
       x->x_stripsize = (int)fstripsize;
    }
}

static void pdp_underwatch_input_0(t_pdp_underwatch *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_underwatch_process(x);
    }
}

static void pdp_underwatch_free(t_pdp_underwatch *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_underwatch_class;

void *pdp_underwatch_new(void)
{
    int i;

    t_pdp_underwatch *x = (t_pdp_underwatch *)pd_new(pdp_underwatch_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("stripsize"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_line = 0;
    pdp_underwatch_setparams(x);
    x->x_prevline = 0;
    x->x_prevsline = 0;
    x->x_prevsheight = 0;
    x->x_stripsize = 10;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_underwatch_setup(void)
{
//    post( pdp_underwatch_version );
    pdp_underwatch_class = class_new(gensym("pdp_underwatch"), (t_newmethod)pdp_underwatch_new,
    	(t_method)pdp_underwatch_free, sizeof(t_pdp_underwatch), 0, A_NULL);

    class_addmethod(pdp_underwatch_class, (t_method)pdp_underwatch_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_underwatch_class, (t_method)pdp_underwatch_stripsize, gensym("stripsize"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
