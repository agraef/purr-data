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

/*  This object is an adaptation of aging effect from effectv
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

static char   *pdp_aging_version = "pdp_aging: version 0.1, port of aging from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

#define PDP_AGING_MAX_SCRATCHES 100
static unsigned int fastrand_val;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)

typedef struct _scratch
{
    int life;
    int x;
    int dx;
    int init;
} scratch;

typedef struct pdp_aging_struct
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
    int x_area_scale;
    int x_nb_scratches;
    int x_dusinterval;
    int x_pits_interval;
    scratch x_scratches[PDP_AGING_MAX_SCRATCHES];

} t_pdp_aging;

static void pdp_aging_area_scale(t_pdp_aging *x, t_floatarg fscale )
{
    if ( (int) fscale > 1 )
    {
       x->x_area_scale = (int)fscale;
    }
}

static void pdp_aging_scratches(t_pdp_aging *x, t_floatarg fscratches )
{
   if ( ( (int)fscratches < PDP_AGING_MAX_SCRATCHES ) && ( (int)fscratches>=0) )
   {
      x->x_nb_scratches = (int)fscratches;
   }
}

static void pdp_aging_coloraging(t_pdp_aging *x, short int *src, short int *dest)
{
   short int a, b;
   int i;

     for(i=0; i<x->x_vsize; i++) 
     {
         a = *src++;
         b = (a & 0xfcfc)>>2;
         *dest++ = a - b + 0x1818 + ((inline_fastrand()>>8)&0x1010);
     }
     // set all to b&w
     for(i=x->x_vsize; i<( x->x_vsize + (x->x_vsize>>1) ); i++) 
     {
         *dest++ = 0;
     }
}

static void pdp_aging_scratching(t_pdp_aging *x, short int *dest)
{
  int i, y, y1, y2;
  short int *p, a, b;
  const int width = x->x_vwidth;
  const int height = x->x_vheight;

  for(i=0; i<x->x_nb_scratches; i++) 
  {
     if(x->x_scratches[i].life) 
     {
        x->x_scratches[i].x = x->x_scratches[i].x + x->x_scratches[i].dx;
        if(x->x_scratches[i].x < 0 || x->x_scratches[i].x > width*256) 
        {
          x->x_scratches[i].life = 0;
          break;
        }
        p = dest + (x->x_scratches[i].x>>8);
        if(x->x_scratches[i].init) 
        {
          y1 = x->x_scratches[i].init;
          x->x_scratches[i].init = 0;
        } 
        else 
        {
           y1 = 0;
        }
        x->x_scratches[i].life--;
        if(x->x_scratches[i].life) 
        {
           y2 = height;
        } 
        else 
        {
           y2 = inline_fastrand() % height;
        }
        for(y=y1; y<y2; y++) 
        {
            a = *p & 0xfeff;
            a += 0x2020;
            b = a & 0x10100;
            *p = a | (b - (b>>8));
            p += width;
         }
      } 
      else 
      {
         if((inline_fastrand()&0xf0000000) == 0) 
         {
             x->x_scratches[i].life = 2 + (inline_fastrand()>>27);
             x->x_scratches[i].x = inline_fastrand() % (width * 256);
             x->x_scratches[i].dx = ((int)inline_fastrand())>>23;
             x->x_scratches[i].init = (inline_fastrand() % (height-1))+1;
          }
       }
    }
}

static void pdp_aging_dusts(t_pdp_aging *x, short int *dest)
{
   int dx[8] = { 1, 1, 0, -1, -1, -1,  0, 1};
   int dy[8] = { 0, -1, -1, -1, 0, 1, 1, 1};
   int i, j;
   int dnum;
   int d, len;
   int px, py;
   const int width = x->x_vwidth;
   const int height = x->x_vheight;

   if(x->x_dusinterval == 0) 
   {
     if((inline_fastrand()&0xf0000000) == 0) {
       x->x_dusinterval = inline_fastrand()>>29;
     }
     return;
   }

   dnum = x->x_area_scale*4 + (inline_fastrand()>>27);
   for(i=0; i<dnum; i++) 
   {
      px = inline_fastrand()%width;
      py = inline_fastrand()%height;
      d = inline_fastrand()>>29;
      len = inline_fastrand()%x->x_area_scale + 5;
      for(j=0; j<len; j++) {
         dest[py*width + px] = 0x1010;
         py += dy[d];
         px += dx[d];
         if(px<0 || px>=width) break;
         if(py<0 || py>=height) break;
         d = (d + inline_fastrand()%3 - 1) & 7;
      }
    }
    x->x_dusinterval--;
}

static void pdp_aging_pits(t_pdp_aging *x, short int *dest)
{
  int i, j;
  int pnum, size, pnumscale;
  int px, py;
  const int width = x->x_vwidth;
  const int height = x->x_vheight;

   pnumscale = x->x_area_scale * 2;
   if(x->x_pits_interval) 
   {
      pnum = pnumscale + (inline_fastrand()%pnumscale);
      x->x_pits_interval--;
   } 
   else 
   {
      pnum = inline_fastrand()%pnumscale;
      if((inline_fastrand()&0xf8000000) == 0) 
      {
         x->x_pits_interval = (inline_fastrand()>>28) + 20;
      }
    }
    for(i=0; i<pnum; i++) 
    {
       px = inline_fastrand()%(width-1);
       py = inline_fastrand()%(height-1);
       size = inline_fastrand()>>28;
       for(j=0; j<size; j++) 
       {
          px = px + inline_fastrand()%3-1;
          py = py + inline_fastrand()%3-1;
          if(px<0 || px>=width) break;
          if(py<0 || py>=height) break;
          dest[py*width + px] = 0xc0c0;
       }
    }
}

static void pdp_aging_process_yv12(t_pdp_aging *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int       i;

    unsigned int totalnbpixels;
    unsigned int u_offset;
    unsigned int v_offset;
    unsigned int totnbpixels;
    short int *poy, *pou, *pov, *pny, *pnu, *pnv;
    int px, py;
    int noy, pos, nox;
    int *p;

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    totalnbpixels = x->x_vsize;
    u_offset = x->x_vsize;
    v_offset = x->x_vsize + (x->x_vsize>>2);
    totnbpixels = x->x_vsize + (x->x_vsize>>1);

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    pdp_aging_coloraging( x, data, newdata );
    pdp_aging_scratching( x, newdata );
    pdp_aging_pits( x, newdata );
    if ( x->x_area_scale > 1 ) pdp_aging_dusts( x, newdata );

    return;
}

static void pdp_aging_sendpacket(t_pdp_aging *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_aging_process(t_pdp_aging *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_aging_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_aging_process_yv12, pdp_aging_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_aging_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_aging_process */
	    break;
	    
	}
    }

}

static void pdp_aging_input_0(t_pdp_aging *x, t_symbol *s, t_floatarg f)
{
    if (s== gensym("register_rw"))  
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );


    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_aging_process(x);

    }

}

static void pdp_aging_free(t_pdp_aging *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);

}

t_class *pdp_aging_class;

void *pdp_aging_new(void)
{
    int i;

    t_pdp_aging *x = (t_pdp_aging *)pd_new(pdp_aging_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("area_scale"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("scratches"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_nb_scratches = 7;
    x->x_area_scale=5;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_aging_setup(void)
{
//    post( pdp_aging_version );
    pdp_aging_class = class_new(gensym("pdp_aging"), (t_newmethod)pdp_aging_new,
    	(t_method)pdp_aging_free, sizeof(t_pdp_aging), 0, A_NULL);

    class_addmethod(pdp_aging_class, (t_method)pdp_aging_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_aging_class, (t_method)pdp_aging_area_scale, gensym("area_scale"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_aging_class, (t_method)pdp_aging_scratches, gensym("scratches"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
