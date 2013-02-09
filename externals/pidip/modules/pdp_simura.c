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

/*  This object is an adaptation of simura effect from freej
 *  Originally written by Fukuchi Kentarou                  
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

static char   *pdp_simura_version = "pdp_simura: version 0.1, port of simura from freej ( Fukuchi Kentarou ), adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_simura_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    unsigned short int x_color; /* color for the mask */
    int x_mode;      /* mirror mode        */

} t_pdp_simura;

static void pdp_simura_process_yv12(t_pdp_simura *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       newpacket = -1, i;

    unsigned int w = header->info.image.width;
    unsigned int hw = w/2;
    unsigned int hhw = w/4;
    unsigned int h = header->info.image.height;
    unsigned int hh = h/2;
    unsigned int hhh = h/4;

    unsigned int size = w*h;
    unsigned int totalnbpixels = size;
    unsigned int u_offset = size;
    unsigned int v_offset = size + (size>>2);
    unsigned int totnbpixels = size + (size>>1);

    unsigned int px, py;

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = w;
    newheader->info.image.height = h;

    switch ( x->x_mode )
    {
      case 0 :
         // y component
         for(py=0; py<h; py++){
           for(px=0; px<w; px++){
             newdata[py*w+px] = data[py*w+px];
           }
         }
         // u component
         for(py=0; py<hh; py++){
           for(px=0; px<hw; px++){
             newdata[u_offset+py*hw+px] = data[u_offset+py*hw+px] ^ x->x_color;
           }
         }
         // v component
         for(py=0; py<hh; py++){
           for(px=0; px<hw; px++){
             newdata[v_offset+py*hw+px] = data[v_offset+py*hw+px] ^ x->x_color;
           }
         }
         break;
      case 1 :
         // y component
         for(py=0; py<(hh); py++){
           for(px=0; px<w; px++){
             newdata[py*(w)+px] = data[py*(w)+px];
             newdata[((h)-py-1)*(w)+px] = data[py*(w)+px];
           }
         }
         // u component
         for(py=0; py<(hhh); py++){
           for(px=0; px<hw; px++){
             newdata[u_offset+py*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
             newdata[u_offset+((hh)-py-1)*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         // v component
         for(py=0; py<(hhh); py++){
           for(px=0; px<hw; px++){
             newdata[v_offset+py*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
             newdata[v_offset+((hh)-py-1)*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         break;
      case 2 :
         // y component
         for(py=(hh); py<h; py++){
           for(px=0; px<w; px++){
              newdata[py*(w)+px] = data[py*(w)+px];
              newdata[((h)-py-1)*(w)+px] = data[py*(w)+px];
           }
         }
         // u component
         for(py=(hhh); py<hh; py++){
           for(px=0; px<hw; px++){
              newdata[u_offset+py*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
              newdata[u_offset+((hh)-py-1)*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         // v component
         for(py=(hhh); py<hh; py++){
           for(px=0; px<hw; px++){
              newdata[v_offset+py*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
              newdata[v_offset+((hh)-py-1)*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         break;
      case 3 :
         // y component
         for(py=0; py<h; py++){
           for(px=0; px<(hw); px++){
              newdata[py*(w)+px] = data[py*(w)+px];
              newdata[py*(w)+((w)-px-1)] = data[py*(w)+px];
           }
         }
         // u component
         for(py=0; py<hh; py++){
           for(px=0; px<(hhw); px++){
              newdata[u_offset+py*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
              newdata[u_offset+py*(hw)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         // v component
         for(py=0; py<hh; py++){
           for(px=0; px<(hhw); px++){
              newdata[v_offset+py*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
              newdata[v_offset+py*(hw)+((hw)-px-1)] = data[v_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         break;
      case 4 :
         // y component
         for(py=0; py<h; py++){
           for(px=(hw); px<w; px++){
               newdata[py*(w)+px] = data[py*(w)+px];
               newdata[py*(w)+((w)-px-1)] = data[py*(w)+px];
           }
         }
         // u component
         for(py=0; py<hh; py++){
           for(px=(hhw); px<hw; px++){
               newdata[u_offset+py*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+py*(hw)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         // u component
         for(py=0; py<hh; py++){
           for(px=(hhw); px<hw; px++){
               newdata[u_offset+py*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+py*(hw)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         break;
      case 5 :
         // y component
         for(py=0; py<(hh); py++){
           for(px=0; px<(hw); px++){
               newdata[py*(w)+px] = data[py*(w)+px];
               newdata[py*(w)+((w)-px-1)] = data[py*(w)+px];
               newdata[((h)-py-1)*(w)+px] = data[py*(w)+px];
               newdata[((h)-py-1)*(w)+((w)-px-1)] = data[py*(w)+px];
           }
         }
         // u component
         for(py=0; py<(hhh); py++){
           for(px=0; px<(hhw); px++){
               newdata[u_offset+py*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+py*(hw)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+((hh)-py-1)*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+((hh)-py-1)*(w)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         // v component
         for(py=0; py<(hhh); py++){
           for(px=0; px<(hhw); px++){
               newdata[v_offset+py*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+py*(hw)+((hw)-px-1)] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+((hh)-py-1)*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+((hh)-py-1)*(hw)+((hw)-px-1)] = data[v_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         break;
      case 6 :
         // y component
         for(py=0; py<(hh); py++){
           for(px=(hw); px<w; px++){
               newdata[py*(w)+px] = data[py*(w)+px];
               newdata[py*(w)+((w)-px-1)] = data[py*(w)+px];
               newdata[((h)-py-1)*(w)+px] = data[py*(w)+px];
               newdata[((h)-py-1)*(w)+((w)-px-1)] = data[py*(w)+px];
           }
         }
         // u component
         for(py=0; py<(hhh); py++){
           for(px=(hhw); px<(hw); px++){
               newdata[u_offset+py*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+py*(hw)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+((hh)-py-1)*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+((hh)-py-1)*(hw)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         // v component
         for(py=0; py<(hhh); py++){
           for(px=(hhw); px<(hw); px++){
               newdata[v_offset+py*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+py*(hw)+((hw)-px-1)] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+((hh)-py-1)*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+((hh)-py-1)*(hw)+((hw)-px-1)] = data[v_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         break;
      case 7 :
         // y component
         for(py=(hh); py<(h); py++){
           for(px=0; px<(hw); px++){
               newdata[py*(w)+px] = data[py*(w)+px];
               newdata[py*(w)+((w)-px-1)] = data[py*(w)+px];
               newdata[((h)-py-1)*(w)+px] = data[py*(w)+px];
               newdata[((h)-py-1)*(w)+((w)-px-1)] = data[py*(w)+px];
           }
         }
         // u component
         for(py=(hhh); py<(hh); py++){
           for(px=0; px<(hhw); px++){
               newdata[u_offset+py*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+py*(hw)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+((hh)-py-1)*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+((hh)-py-1)*(hw)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         // v component
         for(py=(hhh); py<(hh); py++){
           for(px=0; px<(hhw); px++){
               newdata[v_offset+py*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+py*(hw)+((hw)-px-1)] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+((hh)-py-1)*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+((hh)-py-1)*(hw)+((hw)-px-1)] = data[v_offset+py*(hw)+px]^ x->x_color;
           }
         }
         break;
      case 8 :
         // y component
         for(py=(hh); py<h; py++){
           for(px=(hw); px<w; px++){
               newdata[py*(w)+px] = data[py*(w)+px];
               newdata[py*(w)+((w)-px-1)] = data[py*(w)+px];
               newdata[((h)-py-1)*(w)+px] = data[py*(w)+px];
               newdata[((h)-py-1)*(w)+((w)-px-1)] = data[py*(w)+px];
           }
         }
         // u component
         for(py=(hhh); py<(hh); py++){
           for(px=(hhw); px<(hw); px++){
               newdata[u_offset+py*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+py*(hw)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+((hh)-py-1)*(hw)+px] = data[u_offset+py*(hw)+px] ^ x->x_color;
               newdata[u_offset+((hh)-py-1)*(hw)+((hw)-px-1)] = data[u_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         // v component
         for(py=(hhh); py<(hh); py++){
           for(px=(hhw); px<(hw); px++){
               newdata[v_offset+py*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+py*(hw)+((hw)-px-1)] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+((hh)-py-1)*(hw)+px] = data[v_offset+py*(hw)+px] ^ x->x_color;
               newdata[v_offset+((hh)-py-1)*(hw)+((hw)-px-1)] = data[v_offset+py*(hw)+px] ^ x->x_color;
           }
         }
         break;
    }
    // post( "pdp_simura : size=%d tsize=%d", size, (int)(size + (size>>1))<<1 );

    /* delete source packet and replace with new packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = newpacket;
    return;
}

static void pdp_simura_sendpacket(t_pdp_simura *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_simura_process(t_pdp_simura *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_simura_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_simura_process_yv12, pdp_simura_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_simura_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_simura_process */
	    break;
	    
	}
    }
}

static void pdp_simura_input_0(t_pdp_simura *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_simura_process(x);
    }
}

static void pdp_simura_color(t_pdp_simura *x, t_floatarg fcolor )
{
   if ( (int)fcolor >0 && (int)fcolor < 0xFFFF )
   {
     x->x_color = (unsigned short int)fcolor;
   }
   else
   {
     post( "pdp_simura : wrong color %d", (int) fcolor );
   }
}

static void pdp_simura_mode(t_pdp_simura *x, t_floatarg fmode )
{
   if ( (int)fmode >=0 && (int)fmode <= 8 )
   {
     x->x_mode = (int)fmode;
   }
   else
   {
      post( "pdp_simura : wrong mode : %d : must be 0<=mode<=8", (int)fmode );
   }
}


static void pdp_simura_free(t_pdp_simura *x)
{
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_simura_class;

void *pdp_simura_new(void)
{
    int i;

    t_pdp_simura *x = (t_pdp_simura *)pd_new(pdp_simura_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("color"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("mode"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_color = 0;
    x->x_mode = 0; // no mirror

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_simura_setup(void)
{
//    post( pdp_simura_version );

    pdp_simura_class = class_new(gensym("pdp_simura"), (t_newmethod)pdp_simura_new,
    	(t_method)pdp_simura_free, sizeof(t_pdp_simura), 0, A_NULL);

    class_addmethod(pdp_simura_class, (t_method)pdp_simura_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_simura_class, (t_method)pdp_simura_color, gensym("color"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_simura_class, (t_method)pdp_simura_mode, gensym("mode"),  A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
