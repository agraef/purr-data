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

/*  This object is an adaptation of warp effect from effectv
 *  Originally written by Fukuchi Kentaro & others
 *  Pd-fication by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

#define CTABLE_SIZE 1024

static int sintable[CTABLE_SIZE+256];

static char   *pdp_warp_version = "pdp_warp: version 0.1, port of warp from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_warp_struct
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
    int x_tval;
    int x_mode;
    int x_ctable[CTABLE_SIZE];
    int *x_disttable;
    int *x_offstable;

} t_pdp_warp;

static void pdp_warp_mode(t_pdp_warp *x, t_floatarg fmode )
{
   if ( ( fmode == 0 ) || ( fmode == 1 ) )
   {
       x->x_mode = (int)fmode;
   }
}

static void pdp_warp_tval(t_pdp_warp *x, t_floatarg ftval )
{
   x->x_tval = (int)ftval;
}

static void pdp_warp_init_sin_table(void) 
{
  int  *tptr, *tsinptr;
  double  i;

   tsinptr = tptr = sintable;

   for (i = 0; i < 1024; i++)
   {
      *tptr++ = (int) (sin (i*M_PI/512) * 32767);
   }

   for (i = 0; i < 256; i++)
   {
      *tptr++ = *tsinptr++;
   }
}

static void pdp_warp_init_offs_table(t_pdp_warp* x) 
{
  int y;

  for (y = 0; y < x->x_vheight; y++) {
      x->x_offstable[y] = y * x->x_vwidth;
  }
}

static void pdp_warp_init_dist_table(t_pdp_warp *x) 
{
  int  halfw, halfh, *distptr;
  double  px,py,m;

    halfw = x->x_vwidth>> 1;
    halfh = x->x_vheight >> 1;

    distptr = x->x_disttable;

    m = sqrt ((double)(halfw*halfw + halfh*halfh));

    for (py = -halfh; py < halfh; py++)
    {
      for (px= -halfw; px < halfw; px++)
      {
        *distptr++ = ((int) ( (sqrt (px*px+py*py) * 511.9999) / m)) << 1;
      }
    }
}


static void pdp_warp_free_ressources(t_pdp_warp *x)
{
  if ( x->x_offstable ) freebytes( x->x_offstable, x->x_vheight * sizeof (int) );
  if ( x->x_disttable ) freebytes( x->x_disttable, x->x_vwidth * x->x_vheight * sizeof (int) );
}

static void pdp_warp_allocate(t_pdp_warp *x)
{
 int i;

  x->x_offstable = (int*) getbytes ( x->x_vheight * sizeof (int) );
  x->x_disttable = (int*) getbytes ( x->x_vwidth * x->x_vheight * sizeof (int) );
  pdp_warp_init_offs_table(x); 
  pdp_warp_init_dist_table(x);
  
}

void pdp_warp_do_warp(t_pdp_warp *x, short int* src, short int *dest, int xw, int yw, int cw) 
{
  int c, i, px, py, dx, dy, dxu, dyu, maxx, maxy;
  int width, height, skip, *ctptr, *distptr;
  short int *destptr, *destptru, *destptrv;

    ctptr = x->x_ctable;
    distptr = x->x_disttable;
    width = x->x_vwidth;
    height = x->x_vheight;
    destptr = dest;
    destptrv = dest+x->x_vsize;
    destptru = dest+x->x_vsize+(x->x_vsize>>2);
    skip = 0 ; /* x->x_vwidth*sizeof(short int)/4 - x->x_vwidth; */
    c = 0;
    for (px = 0; px < 512; px++) 
    {
       i = (c >> 3) & 0x3FE;
       *ctptr++ = ((sintable[i] * yw) >> 15);
       *ctptr++ = ((sintable[i+256] * xw) >> 15);
       c += cw;
    }
    maxx = width - 2; maxy = height - 2;
    for (py = 0; py < height-1; py++) 
    {
      for (px = 0; px < width; px++) 
      {
        i = *distptr++;
        dx = x->x_ctable [i+1] + px;
        dxu = x->x_ctable [i+1] + (px>>1);
        dy = x->x_ctable [i] + py;
        dyu = x->x_ctable [i] + (py>>1);

        if (dx < 0) dx = 0;
        else if (dx > maxx) dx = maxx;
        if (dy < 0) dy = 0;
        else if (dy > maxy) dy = maxy;
        if (dxu < 0) dxu = 0;
        else if (dxu > (maxx>>1)) dxu = (maxx>>1);
        if (dyu < 0) dyu = 0;
        else if (dyu > (maxy>>1)) dyu = (maxy>>1);

        *destptr++ = src[dy*x->x_vwidth+dx];
        if ( (py%2==0) && (px%2==0) )
        {
           *destptrv++ = src[x->x_vsize+((dyu*x->x_vwidth)>>1)+dxu];
           *destptru++ = src[x->x_vsize+(x->x_vsize>>2)+((dyu*x->x_vwidth)>>1)+dxu];
        }
      }
   }

}

static void pdp_warp_process_yv12(t_pdp_warp *x)
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

    int px, py;
    int dx, dy;
    int h, v;
    int width, height;
    int *p, *q, *r;
    signed char *vp;
    int xw, yw, cw;

    /* allocate all ressources */
    if ( (int)(header->info.image.width*header->info.image.height) != x->x_vsize )
    {
        pdp_warp_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_warp_allocate(x);
        post( "pdp_warp : reallocated buffers" );
    }

    totalnbpixels = x->x_vsize;
    u_offset = x->x_vsize;
    v_offset = x->x_vsize + (x->x_vsize>>2);
    totnbpixels = x->x_vsize + (x->x_vsize>>1);

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, (x->x_vsize + (x->x_vsize>>1))<<1 );

    xw  = (int) (sin((x->x_tval+100)*M_PI/128) * 30);
    yw  = (int) (sin((x->x_tval)*M_PI/256) * -35);
    cw  = (int) (sin((x->x_tval-70)*M_PI/64) * 50);
    xw += (int) (sin((x->x_tval-10)*M_PI/512) * 40);
    yw += (int) (sin((x->x_tval+30)*M_PI/512) * 40);

    pdp_warp_do_warp( x, data, newdata, xw, yw, cw);
    if ( x->x_mode )  x->x_tval = (x->x_tval+1) &511;

    return;
}

static void pdp_warp_sendpacket(t_pdp_warp *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_warp_process(t_pdp_warp *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_warp_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_warp_process_yv12, pdp_warp_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_warp_process */
	    break;
	    
	}
    }
}

static void pdp_warp_input_0(t_pdp_warp *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_warp_process(x);
    }
}

static void pdp_warp_free(t_pdp_warp *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    pdp_warp_free_ressources(x);
}

t_class *pdp_warp_class;

void *pdp_warp_new(void)
{
    int i;

    t_pdp_warp *x = (t_pdp_warp *)pd_new(pdp_warp_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("mode"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("tval"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_mode = 0;
    x->x_tval = 0;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_warp_setup(void)
{
//    post( pdp_warp_version );
    pdp_warp_class = class_new(gensym("pdp_warp"), (t_newmethod)pdp_warp_new,
    	(t_method)pdp_warp_free, sizeof(t_pdp_warp), 0, A_NULL);

    pdp_warp_init_sin_table(); 

    class_addmethod(pdp_warp_class, (t_method)pdp_warp_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_warp_class, (t_method)pdp_warp_mode, gensym("mode"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_warp_class, (t_method)pdp_warp_tval, gensym("tval"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
