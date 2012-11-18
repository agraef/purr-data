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
#include <math.h>

static char   *pdp_lens_version = "pdp_lens: version 0.1, port of lens from effectv( Fukuchi Kentaro ) adapted by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_lens_struct
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
    t_float   x_zoom;     // zoom factor
    int     x_cx;       // coordinates of lens center
    int     x_cy;       // coordinates of lens center
    int     x_csize;    // width of the lens
    int     x_xd;
    int     x_yd;
    int     x_mode;
    int     *x_lens;
    int     x_init;

} t_pdp_lens;

static void pdp_lens_preset(t_pdp_lens *x, int oldsize, int newsize)
{
 int px, py, r;

  if ( x->x_lens ) freebytes(x->x_lens, oldsize * oldsize * sizeof( int) ); 
  x->x_lens = (int *) getbytes( newsize * newsize * sizeof( int ) );
  r = x->x_csize / 2;

  /* it is sufficient to generate 1/4 of the lens and reflect this
   * around; a sphere is mirrored on both the x and y axes */
  for (py = 0; py < r; py++) {
        for (px = 0; px < r; px++) {
            int ix, iy, offset, dist;
               dist = px*px + py*py - r*r;
               if(dist < 0) {
                 double shift = x->x_zoom/sqrt(x->x_zoom*x->x_zoom - dist);
                 ix = px * shift - px;
                 iy = py * shift - py;
               } else {
                ix = 0;
                iy = 0;
               }
               offset = (iy * x->x_vwidth + ix);
                        x->x_lens[(r - py)*x->x_csize + r - px] = -offset;
                        x->x_lens[(r + py)*x->x_csize + r + px] = offset;
               offset = (-iy * x->x_vwidth + ix);
                        x->x_lens[(r + py)*x->x_csize + r - px] = -offset;
                        x->x_lens[(r - py)*x->x_csize + r + px] = offset;
        }
  }
}

static void pdp_lens_cliplens(t_pdp_lens *x)
{
    if (x->x_cy<0-(x->x_csize/2)+1)x->x_cy=0-(x->x_csize/2)+1;
    if (x->x_cy>=x->x_vheight-x->x_csize/2-1)x->x_cy=x->x_vheight-x->x_csize/2-1;

    if (x->x_cx<0-(x->x_csize/2)+1) x->x_cx=0-x->x_csize/2+1;
    if(x->x_cx>=x->x_vwidth-x->x_csize/2-1) x->x_cx=x->x_vwidth-x->x_csize/2-1;
}

static void pdp_lens_zoom(t_pdp_lens *x, t_floatarg fzoom )
{
    if ( fzoom>0 )
    {
       x->x_zoom = fzoom;
       if (x->x_zoom<5) x->x_zoom=5;
       if (x->x_zoom>200) x->x_zoom=200;
       pdp_lens_preset(x, x->x_csize, x->x_csize);
    }
}

static void pdp_lens_csize(t_pdp_lens *x, t_floatarg fcsize )
{
    if ( fcsize>0 )
    {
       x->x_csize = (int)fcsize;
       if (x->x_csize>x->x_vheight) x->x_csize = x->x_vheight;
       if (x->x_csize<3) x->x_csize = 3;
       pdp_lens_preset(x, x->x_csize, x->x_csize);
       pdp_lens_cliplens(x);
    }
}

static void pdp_lens_cy(t_pdp_lens *x, t_floatarg fcy )
{
    if ( fcy>0 )
    {
       x->x_cy = (int)fcy;
       pdp_lens_cliplens(x);
    }
}

static void pdp_lens_cx(t_pdp_lens *x, t_floatarg fcx )
{
    if ( fcx>0 )
    {
       x->x_cx = (int)fcx;
       pdp_lens_cliplens(x);
    }
}

static void pdp_lens_mode(t_pdp_lens *x, t_floatarg fmode )
{
    if ( ( fmode == 0 ) || ( fmode == 1 ) )
    {
       x->x_mode = (int)fmode;
    }
}

static void pdp_lens_process_yv12(t_pdp_lens *x)
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
    int noy, pos, posu, nox;
    int *p;

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    if ( x->x_init == -1 )
    {
         pdp_lens_preset( x, x->x_csize, x->x_csize );
         x->x_init = 1;
    }

    totalnbpixels = x->x_vsize;
    u_offset = x->x_vsize;
    v_offset = x->x_vsize + (x->x_vsize>>2);
    totnbpixels = x->x_vsize + (x->x_vsize>>1);

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy(newdata, data, (x->x_vsize + (x->x_vsize>>1))<<1);

    p = x->x_lens;
    poy = data;
    pou = data + x->x_vsize;
    pov = data + x->x_vsize + (x->x_vsize>>2);
    pny = newdata;
    pnu = newdata + x->x_vsize;
    pnv = newdata + x->x_vsize + (x->x_vsize>>2);
    for (py = 0; py < x->x_csize; py++) 
    {
      for (px = 0; px < x->x_csize; px++) 
      {
        noy=(py+x->x_cy);  
        nox=(px+x->x_cx);
        if ((nox>=0)&&(noy>=0)&&(nox<x->x_vwidth)&&(noy<x->x_vheight)){
            pos = (noy * x->x_vwidth) + nox;
            posu = ((noy>>1) * (x->x_vwidth>>1)) + (nox>>1);
            if ( ( ( pos + *p )< x->x_vsize ) && ( pos < x->x_vsize ) ) 
            {
               *(pny+pos) = *(poy + pos + *p);
               *(pnu+posu) = *(pou + posu + *p );
               *(pnv+posu) = *(pov + posu + *p );
            }
        }
        p++;
      }
    }

    if (x->x_mode==1)
    {
       x->x_cx+= x->x_xd; x->x_cy+=x->x_yd;
       if (x->x_cx > (x->x_vwidth - x->x_csize - 5) || x->x_cx < 5) x->x_xd = -x->x_xd;
       if (x->x_cy > (x->x_vwidth - x->x_csize - 5) || x->x_cy < 5) x->x_yd = -x->x_yd;
    }

    return;
}

static void pdp_lens_sendpacket(t_pdp_lens *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_lens_process(t_pdp_lens *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_lens_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_lens_process_yv12, pdp_lens_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_lens_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_lens_process */
	    break;
	    
	}
    }
}

static void pdp_lens_input_0(t_pdp_lens *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_lens_process(x);

    }
}

static void pdp_lens_free(t_pdp_lens *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_lens_class;

void *pdp_lens_new(void)
{
    int i;

    t_pdp_lens *x = (t_pdp_lens *)pd_new(pdp_lens_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cx"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cy"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("csize"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("zoom"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("mode"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_cx = x->x_cy = 16;
    x->x_xd = x->x_yd = 5;
    x->x_csize = 150;
    x->x_zoom = 30;
    x->x_init = -1;
    x->x_mode = 0;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_lens_setup(void)
{
//    post( pdp_lens_version );
    pdp_lens_class = class_new(gensym("pdp_lens"), (t_newmethod)pdp_lens_new,
    	(t_method)pdp_lens_free, sizeof(t_pdp_lens), 0, A_NULL);

    class_addmethod(pdp_lens_class, (t_method)pdp_lens_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_lens_class, (t_method)pdp_lens_cx, gensym("cx"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_lens_class, (t_method)pdp_lens_cy, gensym("cy"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_lens_class, (t_method)pdp_lens_csize, gensym("csize"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_lens_class, (t_method)pdp_lens_zoom, gensym("zoom"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_lens_class, (t_method)pdp_lens_mode, gensym("mode"),  A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
