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

/*  This object is a port of dice effect from EffecTV
 *  Originally written by clifford smith <nullset@dookie.net>
 *  Pd-fication by Yves Degoyon ( ydegoyon@free.fr )                             
 */


#include "pdp.h"
#include <math.h>

#define DEFAULT_CUBE_BITS   4
#define MAX_CUBE_BITS       5
#define MIN_CUBE_BITS       0

typedef enum _pdp_dice_dir {
    up = 0,
    right = 1,
    down = 2,
    left = 3
} t_pdp_dice_dir;

static unsigned int fastrand_val;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)

static char   *pdp_dice_version = "pdp_dice: version 0.1, port of dice from EffecTV by clifford smith, adapted by ydegoyon@free.fr ";

typedef struct pdp_dice_struct
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

    char  *x_dicemap;

    int x_cube_bits;
    int x_cube_size;
    int x_map_height;
    int x_map_width;

} t_pdp_dice;

static void pdp_dice_create_map(t_pdp_dice *x)
{
  int px, py, i;

    if ( x->x_dicemap == NULL )
    {
       post( "pdp_dice : tried to create map but it's not allocated" );
       return;
    }

    x->x_map_height = x->x_vheight >> x->x_cube_bits;
    x->x_map_width = x->x_vwidth >> x->x_cube_bits;
    x->x_cube_size = 1 << x->x_cube_bits;

    i = 0;
    for (py=0; py<x->x_map_height; py++) 
    {
       for(px=0; px<x->x_map_width; px++) 
       {
         x->x_dicemap[i] = (inline_fastrand() >> 24) & 0x03;
         i++;
       }
    }

    return;
}

static void pdp_dice_cubebits(t_pdp_dice *x, t_floatarg fcubebits )
{
    if ( ( fcubebits >= MIN_CUBE_BITS ) || ( fcubebits <= MAX_CUBE_BITS ) )
    {
       x->x_cube_bits = fcubebits;
       pdp_dice_create_map(x);
    }
}

static void pdp_dice_free_ressources(t_pdp_dice *x)
{
    if ( x->x_dicemap ) freebytes( x->x_dicemap, x->x_vsize );
}

static void pdp_dice_allocate(t_pdp_dice *x)
{
    x->x_dicemap = (char *) getbytes( x->x_vsize );
}

static void pdp_dice_process_yv12(t_pdp_dice *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i, iuv;
    int     mapx, mapy, mapi;
    int     base, baseuv, dx, dy, di, diuv;

    /* allocate all ressources */
    if ( ((int)header->info.image.width != x->x_vwidth) ||
         ((int)header->info.image.height != x->x_vheight) )
    {
        pdp_dice_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_dice_allocate(x);
        post( "pdp_dice : reallocated buffers" );
        pdp_dice_create_map(x);
        post( "pdp_dice : initialized map" );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    mapi = 0;
    for(mapy = 0; mapy < x->x_map_height; mapy++)
    {
      for(mapx = 0; mapx < x->x_map_width; mapx++)
      {
        base = (mapy << x->x_cube_bits) * x->x_vwidth + (mapx << x->x_cube_bits);
        baseuv = (((mapy << x->x_cube_bits)>>1) * (x->x_vwidth>>1)) + ((mapx << x->x_cube_bits)>>1);
        switch (x->x_dicemap[mapi])
        {
          case up:
            for (dy = 0; dy < x->x_cube_size; dy++)
            {
                i = base + dy * x->x_vwidth;
                iuv = baseuv + ((dy>>1) * (x->x_vwidth>>1));
                for (dx = 0; dx < x->x_cube_size; dx++)
                {
                  newdata[i] = data[i];
                  newdata[x->x_vsize+iuv] = data[x->x_vsize+iuv];
                  newdata[x->x_vsize+(x->x_vsize>>2)+iuv] = data[x->x_vsize+(x->x_vsize>>2)+iuv];
                  i++;
                  if ( (dx%2==0) && (dy%2==0) ) iuv++;
                }
            }
            break;

          case left:
            for (dy = 0; dy < x->x_cube_size; dy++)
            {
                i = base + dy * x->x_vwidth;
                iuv = baseuv + ((dy>>1) * (x->x_vwidth>>1));
                for (dx = 0; dx < x->x_cube_size; dx++)
                {
                  di = base + (dx * x->x_vwidth) + (x->x_cube_size - dy - 1);
                  diuv = baseuv + ((dx>>1) * (x->x_vwidth>>1)) + ((x->x_cube_size - dy - 1)>>1);
                  newdata[di] = data[i];
                  newdata[x->x_vsize+diuv] = data[x->x_vsize+iuv];
                  newdata[x->x_vsize+(x->x_vsize>>2)+diuv] = data[x->x_vsize+(x->x_vsize>>2)+iuv];
                  i++;
                  if ( (dx%2==0) && (dy%2==0) ) iuv++;
                }
            }
            break;

          case down:
            for (dy = 0; dy < x->x_cube_size; dy++)
            {
              di = base + dy * x->x_vwidth;
              diuv = baseuv + ((dy>>1) * (x->x_vwidth>>1));
              i = base + (x->x_cube_size - dy - 1) * x->x_vwidth + x->x_cube_size;
              iuv = baseuv + (((x->x_cube_size - dy - 1)>>1) * (x->x_vwidth>>1)) + (x->x_cube_size>>1);
              for (dx = 0; dx < x->x_cube_size; dx++)
              {
                i--;
                if ( dx%2==0) iuv--;
                newdata[di] = data[i];
                newdata[x->x_vsize+diuv] = data[x->x_vsize+iuv];
                newdata[x->x_vsize+(x->x_vsize>>2)+diuv] = data[x->x_vsize+(x->x_vsize>>2)+iuv];
                di++;
                if ( (dx%2==0) && (dy%2==0) ) iuv++;
              }
            }
            break;

          case right:
            for (dy = 0; dy < x->x_cube_size; dy++)
            {
              i = base + (dy * x->x_vwidth);
              iuv = baseuv + ((dy>>1) * (x->x_vwidth>>1));
              for (dx = 0; dx < x->x_cube_size; dx++)
              {
                di = base + dy + (x->x_cube_size - dx - 1) * x->x_vwidth;
                diuv = baseuv + (dy>>1) + (((x->x_cube_size - dx - 1)>>1) * (x->x_vwidth>>1));
                newdata[di] = data[i];
                newdata[x->x_vsize+diuv] = data[x->x_vsize+iuv];
                newdata[x->x_vsize+(x->x_vsize>>2)+diuv] = data[x->x_vsize+(x->x_vsize>>2)+iuv];
                i++;
                if ( (dx%2==0) && (dy%2==0) ) iuv++;
              }
            }
            break;

          default:
            break;
        }
        mapi++;
      }
    }

    return;
}

static void pdp_dice_sendpacket(t_pdp_dice *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_dice_process(t_pdp_dice *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_dice_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_dice_process_yv12, pdp_dice_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_dice_process */
	    break;
	    
	}
    }

}

static void pdp_dice_input_0(t_pdp_dice *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_dice_process(x);

    }

}

static void pdp_dice_free(t_pdp_dice *x)
{
  int i;

    pdp_dice_free_ressources(x);
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_dice_class;

void *pdp_dice_new(void)
{
    int i;

    t_pdp_dice *x = (t_pdp_dice *)pd_new(pdp_dice_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cubebits"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_cube_bits = DEFAULT_CUBE_BITS;
    x->x_cube_size = 0;
    x->x_map_height = 0;
    x->x_map_width = 0;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_dice_setup(void)
{
//    post( pdp_dice_version );
    pdp_dice_class = class_new(gensym("pdp_dice"), (t_newmethod)pdp_dice_new,
    	(t_method)pdp_dice_free, sizeof(t_pdp_dice), 0, A_NULL);

    class_addmethod(pdp_dice_class, (t_method)pdp_dice_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_dice_class, (t_method)pdp_dice_cubebits, gensym("cubebits"),  A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
