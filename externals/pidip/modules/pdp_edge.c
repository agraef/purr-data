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

/*  This object is an adaptation of edge effect from effectv
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

static char   *pdp_edge_version = "pdp_edge: version 0.1, port of edge from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_edge_struct
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
    int x_mapw;
    int x_maph;
    int x_video_width_margin;
    int *x_map;

} t_pdp_edge;

static void pdp_edge_allocate(t_pdp_edge *x)
{
  x->x_map = (int*) getbytes ( ( x->x_vwidth * x->x_vheight * sizeof (int) ) << 1 );
  bzero(x->x_map, ( x->x_vwidth * x->x_vheight * sizeof (int) ) << 1 );
}

static void pdp_edge_free_ressources(t_pdp_edge *x)
{
  if ( x->x_map ) freebytes ( x->x_map, ( x->x_vwidth * x->x_vheight * sizeof (int) ) << 1 );
}

static void pdp_edge_process_yv12(t_pdp_edge *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i;

    int px, py;
    int y, u, v;
    int y0, u0, v0;
    int y1, u1, v1;
    int y2, u2, v2;
    int y3, u3, v3;
    short int *pdata, *pnewdata;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize )
    {
       pdp_edge_free_ressources(x);
       x->x_vwidth = header->info.image.width;
       x->x_vheight = header->info.image.height;
       x->x_vsize = x->x_vwidth*x->x_vheight;
       x->x_mapw = x->x_vwidth >> 2;
       x->x_maph = x->x_vheight >> 2;
       x->x_video_width_margin = x->x_vwidth - ( x->x_mapw << 2 );
       pdp_edge_allocate(x);
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    pdata = data + x->x_vwidth*4+4;
    pnewdata = newdata + x->x_vwidth*4+4;
    for(py=1; py<x->x_vheight-4; py+=4) 
    {
       for(px=1; px<x->x_vwidth-4; px+=4) 
       {
          /* difference between the current pixel and right neighbor. */
          y2 = (*(pdata+(py*x->x_vwidth+px))>>8) - (*(pdata+(py*x->x_vwidth+px)-4)>>8);
          u2 = (*(pdata+x->x_vsize+(py*x->x_vwidth>>2)+(px>>1))>>8) - 
              (*(pdata+x->x_vsize+(py*x->x_vwidth>>2)+(px>>1)-2)>>8);
          v2 = (*(pdata+x->x_vsize+(x->x_vsize>>2)+(py*x->x_vwidth>>2)+(px>>1))>>8) - 
              (*(pdata+x->x_vsize+(x->x_vsize>>2)+(py*x->x_vwidth>>2)+(px>>1)-2)>>8);
          y2 *= y2;
          u2 *= u2;
          v2 *= v2;
          y2 = y2>>5; /* To lack the lower bit for saturated addition,  */
          u2 = u2>>5; /* devide the value with 32, instead of 16. It is */
          v2 = v2>>4; /* same as `v2 &= 0xfefeff' */
          if(y2>127) y2 = 127;
          if(u2>127) u2 = 127;
          if(v2>255) v2 = 255;

          /* difference between the current pixel and upper neighbor. */
          y3 = (*(pdata+(py*x->x_vwidth+px))>>8) - (*(pdata+(py*x->x_vwidth+px)-4*x->x_vwidth)>>8);
          u3 = (*(pdata+x->x_vsize+(py*x->x_vwidth>>2)+(px>>1))>>8) - 
              (*(pdata+x->x_vsize+(py*x->x_vwidth>>2)+(px>>1)-2*x->x_vwidth)>>8);
          v3 = (*(pdata+x->x_vsize+(x->x_vsize>>2)+(py*x->x_vwidth>>2)+(px>>1))>>8) - 
              (*(pdata+x->x_vsize+(x->x_vsize>>2)+(py*x->x_vwidth>>2)+(px>>1)-2*x->x_vwidth)>>8);
          y3 *= y3;
          u3 *= u3;
          v3 *= v3;
          y3 = y3>>5; /* To lack the lower bit for saturated addition,  */
          u3 = u3>>5; /* devide the value with 32, instead of 16. It is */
          v3 = v3>>4; /* same as `v2 &= 0xfefeff' */
          if(y3>127) y3 = 127;
          if(u3>127) u3 = 127;
          if(v3>255) v3 = 255;

          y0 = (x->x_map[(py-1)*x->x_vwidth*2+px*2]>>17);
          u0 = (x->x_map[(py-1)*x->x_vwidth*2+px*2]>>9)&0xff;
          v0 = x->x_map[(py-1)*x->x_vwidth*2+px*2]&0xff;
          y1 = (x->x_map[py*x->x_vwidth*2+(px-1)*2+1]>>17);
          u1 = (x->x_map[py*x->x_vwidth*2+(px-1)*2+1]>>9)&0xff;
          v1 = x->x_map[py*x->x_vwidth*2+(px-1)*2+1]&0xff;
          x->x_map[py*x->x_vwidth*2+px*2] = (y2<<17)|(u2<<9)|v2;
          x->x_map[py*x->x_vwidth*2+px*2+1] = (y3<<17)|(u3<<9)|v3; 
          y = y0 + y1;
          u = y & 0x01010100;
          *(pnewdata+(py*x->x_vwidth+px)) = (y | (u - (u>>8)))<<8;
          *(pnewdata+x->x_vsize+(py*x->x_vwidth>>2)+(px>>1)) = (y | (u - (u>>8)))<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+(py*x->x_vwidth>>2)+(px>>1)) = (y | (u - (u>>8)))<<8;
          y = y0 + y3;
          u = y & 0x01010100;
          *(pnewdata+(py*x->x_vwidth+px)+1) = (y | (u - (u>>8)))<<8;
          *(pnewdata+x->x_vsize+(py*x->x_vwidth>>2)+(px>>1+1)) = (y | (u - (u>>8)))<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+(py*x->x_vwidth>>2)+(px>>1+1)) = (y | (u - (u>>8)))<<8;
          *(pnewdata+(py*x->x_vwidth+px)+2) = y3<<8;
          *(pnewdata+x->x_vsize+(py*x->x_vwidth>>2)+((px+2)>>1)) = u3<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+(py*x->x_vwidth>>2)+((px+2)>>1)) = v3<<8;
          *(pnewdata+(py*x->x_vwidth+px)+3) = y3<<8;
          *(pnewdata+x->x_vsize+(py*x->x_vwidth>>2)+((px+3)>>1)) = u3<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+(py*x->x_vwidth>>2)+((px+3)>>1)) = v3<<8;
          y = y2 + y1;
          u = y & 0x01010100;
          *(pnewdata+(py*x->x_vwidth+px)+x->x_vwidth) = (y | (u - (u>>8)))<<8;
          *(pnewdata+x->x_vsize+((py+1)*x->x_vwidth>>2)+(px>>1)) = (y | (u - (u>>8)))<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+((py+1)*x->x_vwidth>>2)+(px>>1)) = (y | (u - (u>>8)))<<8;
          y = y2 + y3;
          u = y & 0x01010100;

          *(pnewdata+(py*x->x_vwidth+px)+x->x_vwidth+1) = (y | (u - (u>>8)))<<8;
          *(pnewdata+x->x_vsize+((py+1)*x->x_vwidth>>2)+((px+1)>>1)) = (y | (u - (u>>8)))<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+((py+1)*x->x_vwidth>>2)+((px+1)>>1)) = (y | (u - (u>>8)))<<8;

          *(pnewdata+(py*x->x_vwidth+px)+x->x_vwidth+2) = y3<<8;
          *(pnewdata+x->x_vsize+((py+1)*x->x_vwidth>>2)+((px+2)>>1)) = u3<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+((py+1)*x->x_vwidth>>2)+((px+2)>>1)) = v3<<8;

          *(pnewdata+(py*x->x_vwidth+px)+x->x_vwidth+3) = y3<<8;
          *(pnewdata+x->x_vsize+((py+1)*x->x_vwidth>>2)+((px+3)>>1)) = u3<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+((py+1)*x->x_vwidth>>2)+((px+3)>>1)) = v3<<8;

          *(pnewdata+(py*x->x_vwidth+px)+2*x->x_vwidth) = y2<<8;
          *(pnewdata+x->x_vsize+((py+2)*x->x_vwidth>>2)+((px)>>1)) = u2<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+((py+2)*x->x_vwidth>>2)+((px)>>1)) = v2<<8;

          *(pnewdata+(py*x->x_vwidth+px)+2*x->x_vwidth+1) = y2<<8;
          *(pnewdata+x->x_vsize+((py+2)*x->x_vwidth>>2)+((px+1)>>1)) = u2<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+((py+2)*x->x_vwidth>>2)+((px+1)>>1)) = v2<<8;

          *(pnewdata+(py*x->x_vwidth+px)+3*x->x_vwidth) = y2<<8;
          *(pnewdata+x->x_vsize+((py+3)*x->x_vwidth>>2)+((px)>>1)) = u2<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+((py+3)*x->x_vwidth>>2)+((px)>>1)) = v2<<8;

          *(pnewdata+(py*x->x_vwidth+px)+3*x->x_vwidth+1) = y2<<8;
          *(pnewdata+x->x_vsize+((py+3)*x->x_vwidth>>2)+((px+1)>>1)) = u2<<8;
          *(pnewdata+x->x_vsize+(x->x_vsize>>2)+((py+3)*x->x_vwidth>>2)+((px+1)>>1)) = v2<<8;
       }
    }

    return;
}

static void pdp_edge_sendpacket(t_pdp_edge *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_edge_process(t_pdp_edge *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_edge_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_edge_process_yv12, pdp_edge_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_edge_process */
	    break;
	    
	}
    }

}

static void pdp_edge_input_0(t_pdp_edge *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_edge_process(x);

    }
}

static void pdp_edge_free(t_pdp_edge *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_edge_free_ressources(x);
}

t_class *pdp_edge_class;

void *pdp_edge_new(void)
{
    int i;

    t_pdp_edge *x = (t_pdp_edge *)pd_new(pdp_edge_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_edge_setup(void)
{
//    post( pdp_edge_version );
    pdp_edge_class = class_new(gensym("pdp_edge"), (t_newmethod)pdp_edge_new,
    	(t_method)pdp_edge_free, sizeof(t_pdp_edge), 0, A_NULL);

    class_addmethod(pdp_edge_class, (t_method)pdp_edge_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
