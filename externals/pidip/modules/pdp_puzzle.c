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

/*  This object is a port of puzzle effect from EffecTV
 *  Originally written by  Fukuchi Kentaro <nullset@dookie.net>
 *  Pd-fication by Yves Degoyon ( ydegoyon@free.fr )                             
 *  The origin of PuzzleTV is ``Video Puzzle'' by Suutarou in 1993.
 *  It runs on Fujitsu FM-TOWNS.
 */


#include "pdp.h"
#include <math.h>

#define DEFAULT_BLOCK_NUMBER  5

static unsigned int fastrand_val;
#define inline_fastrand() (fastrand_val=fastrand_val*1103515245+12345)

static char   *pdp_puzzle_version = "pdp_puzzle: version 0.1, port of puzzle from EffecTV by  Fukuchi Kentaro, adapted by ydegoyon@free.fr ";

typedef struct pdp_puzzle_struct
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

     /* puzzle parameters */
    int *x_blockpos;
    int *x_blockoffset;
    int *x_ublockoffset;
    int *x_vblockoffset;
    int x_nbblocks;
    int x_blockwidth;
    int x_blockheight;
    int x_blockw;
    int x_blockh;
    int x_blocknum;
    int x_spacepos;
    int x_spacex;
    int x_spacey;

} t_pdp_puzzle;

static void pdp_puzzle_init_tables(t_pdp_puzzle *x)
{
  int i, a, b, c;

    for(i=0; i<x->x_blocknum; i++)
    {
      x->x_blockpos[i] = i;
    }
    for(i=0; i<20*x->x_blockw; i++) 
    {
      /* the number of shuffling times is a rule of thumb. */
      a = inline_fastrand()%(x->x_blocknum-1);
      b = inline_fastrand()%(x->x_blocknum-1);
      if(a == b) b = (b+1)%(x->x_blocknum-1);
      c = x->x_blockpos[a];
      x->x_blockpos[a] = x->x_blockpos[b];
      x->x_blockpos[b] = c;
    }
    x->x_spacepos = x->x_blocknum-1;
    x->x_spacex = x->x_blockw-1;
    x->x_spacey = x->x_blockh-1;

    return;
}

static void pdp_puzzle_up(t_pdp_puzzle *x )
{
  int tmp, nextpos=-1;

    if(x->x_spacey>0) 
    {
      nextpos = x->x_spacepos - x->x_blockw;
      x->x_spacey--;
    }
    if(nextpos>=0) 
    {
      tmp = x->x_blockpos[x->x_spacepos];
      x->x_blockpos[x->x_spacepos] = x->x_blockpos[nextpos];
      x->x_blockpos[nextpos] = tmp;
      x->x_spacepos = nextpos;
    }
}

static void pdp_puzzle_down(t_pdp_puzzle *x )
{
  int tmp, nextpos=-1;

    if(x->x_spacey<x->x_blockh-1) 
    {
      nextpos = x->x_spacepos + x->x_blockw;
      x->x_spacey++;
    }
    if(nextpos>=0) 
    {
      tmp = x->x_blockpos[x->x_spacepos];
      x->x_blockpos[x->x_spacepos] = x->x_blockpos[nextpos];
      x->x_blockpos[nextpos] = tmp;
      x->x_spacepos = nextpos;
    }
}

static void pdp_puzzle_left(t_pdp_puzzle *x )
{
  int tmp, nextpos=-1;

    if(x->x_spacex>0) 
    {
      nextpos = x->x_spacepos - 1;
      x->x_spacex--;
    }
    if(nextpos>=0) 
    {
      tmp = x->x_blockpos[x->x_spacepos];
      x->x_blockpos[x->x_spacepos] = x->x_blockpos[nextpos];
      x->x_blockpos[nextpos] = tmp;
      x->x_spacepos = nextpos;
    }
}

static void pdp_puzzle_right(t_pdp_puzzle *x )
{
  int tmp, nextpos=-1;

    if(x->x_spacex<x->x_blockw-1) 
    {
      nextpos = x->x_spacepos + 1;
      x->x_spacex++;
    }
    if(nextpos>=0) 
    {
      tmp = x->x_blockpos[x->x_spacepos];
      x->x_blockpos[x->x_spacepos] = x->x_blockpos[nextpos];
      x->x_blockpos[nextpos] = tmp;
      x->x_spacepos = nextpos;
    }
}

static void pdp_puzzle_free_ressources(t_pdp_puzzle *x)
{
    if ( x->x_blockpos ) freebytes( x->x_blockpos, x->x_blocknum*sizeof(int) );
    if ( x->x_blockoffset ) freebytes( x->x_blockoffset, x->x_blocknum*sizeof(int) );
    if ( x->x_ublockoffset ) freebytes( x->x_ublockoffset, x->x_blocknum*sizeof(int) );
    if ( x->x_vblockoffset ) freebytes( x->x_vblockoffset, x->x_blocknum*sizeof(int) );
}

static void pdp_puzzle_allocate(t_pdp_puzzle *x)
{
 int px, py;

   x->x_blockwidth = x->x_vwidth / x->x_nbblocks;
   x->x_blockheight = x->x_vheight / x->x_nbblocks;
   x->x_blockw = x->x_nbblocks;
   x->x_blockh = x->x_nbblocks;
   x->x_blocknum = x->x_blockw * x->x_blockh;

   x->x_blockpos = (int *) getbytes( x->x_blocknum*sizeof(int) );
   x->x_blockoffset = (int *) getbytes( x->x_blocknum*sizeof(int) );
   x->x_ublockoffset = (int *) getbytes( x->x_blocknum*sizeof(int) );
   x->x_vblockoffset = (int *) getbytes( x->x_blocknum*sizeof(int) );
   if( x->x_blockpos == NULL ||  x->x_blockoffset == NULL ||
       x->x_ublockoffset == NULL || x->x_vblockoffset == NULL ) 
   {
     post( "pdp_puzzle : severe error : cannot allocate buffers !!! ");
     return;
   }

   for(py=0; py<x->x_blockh; py++) 
   {
     for(px=0; px<x->x_blockw; px++) 
     {
       x->x_blockoffset[py*x->x_blockw+px] = py*x->x_blockheight*x->x_vwidth + px*x->x_blockwidth;
       x->x_vblockoffset[py*x->x_blockw+px] = x->x_vsize + (py*x->x_blockheight>>1)*(x->x_vwidth>>1) + (px*x->x_blockwidth>>1);
       x->x_ublockoffset[py*x->x_blockw+px] = x->x_vsize + (x->x_vsize>>2) + (py*x->x_blockheight>>1)*(x->x_vwidth>>1) + (px*x->x_blockwidth>>1);
     }
   }
}

static void pdp_puzzle_nbblocks(t_pdp_puzzle *x, t_floatarg fnbblocks )
{
   if ( ( fnbblocks > 1 ) && ( fnbblocks < x->x_vwidth/10 ) )
   {
      x->x_nbblocks = fnbblocks;
      post( "pdp_puzzle : number of blocks set to : %d", x->x_nbblocks );
      pdp_puzzle_free_ressources(x);
      pdp_puzzle_allocate(x);
      pdp_puzzle_init_tables(x);
   }
}

static void pdp_puzzle_process_yv12(t_pdp_puzzle *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     px, py, xx, yy, i;
    short int *pY, *qY, *pU, *pV, *qU, *qV;

    /* allocate all ressources */
    if ( ((int)header->info.image.width != x->x_vwidth) ||
         ((int)header->info.image.height != x->x_vheight) )
    {
        pdp_puzzle_free_ressources(x);
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_puzzle_allocate(x);
        post( "pdp_puzzle : reallocated buffers" );
        pdp_puzzle_init_tables(x);
        post( "pdp_puzzle : initialized tables" );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    i = 0;
    for(py=0; py<x->x_blockh; py++) 
    {
      for(px=0; px<x->x_blockw; px++) 
      {
        pY = &data[x->x_blockoffset[x->x_blockpos[i]]];
        pU = &data[x->x_ublockoffset[x->x_blockpos[i]]];
        pV = &data[x->x_vblockoffset[x->x_blockpos[i]]];
        qY = &newdata[x->x_blockoffset[i]];
        qU = &newdata[x->x_ublockoffset[i]];
        qV = &newdata[x->x_vblockoffset[i]];
        if(x->x_spacepos == i) 
        {
          for(yy=0; yy<x->x_blockheight; yy++) 
          {
             for(xx=0; xx<x->x_blockwidth; xx++) 
             {
               *(qY++) = 0;
               if ( (xx%2==0) && (yy%2==0) )
               {
                  *(qU++) = 0;
                  *(qV++) = 0;
               }
             }
             qY+=x->x_vwidth-x->x_blockwidth;
             if ( yy%2==0 )
             {
                qU+=(x->x_vwidth-x->x_blockwidth)>>1;
                qV+=(x->x_vwidth-x->x_blockwidth)>>1;
             }
          }
        } 
        else 
        {
          for(yy=0; yy<x->x_blockheight; yy++) 
          {
             for(xx=0; xx<x->x_blockwidth; xx++) 
             {
               *(qY++) = *(pY++);
               if ( (xx%2==0) && (yy%2==0) )
               {
                  *(qU++) = *(pU++);
                  *(qV++) = *(pV++);
               }
             }
             qY+=x->x_vwidth-x->x_blockwidth;
             pY+=x->x_vwidth-x->x_blockwidth;
             if ( yy%2==0 )
             {
                qU+=(x->x_vwidth-x->x_blockwidth)>>1;
                pU+=(x->x_vwidth-x->x_blockwidth)>>1;
                qV+=(x->x_vwidth-x->x_blockwidth)>>1;
                pV+=(x->x_vwidth-x->x_blockwidth)>>1;
             }
          }
        }
        i++;
      }
    }

    return;
}

static void pdp_puzzle_sendpacket(t_pdp_puzzle *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_puzzle_process(t_pdp_puzzle *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_puzzle_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_puzzle_process_yv12, pdp_puzzle_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_puzzle_process */
	    break;
	    
	}
    }

}

static void pdp_puzzle_input_0(t_pdp_puzzle *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_puzzle_process(x);

    }

}

static void pdp_puzzle_free(t_pdp_puzzle *x)
{
  int i;

    pdp_puzzle_free_ressources(x);
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_puzzle_class;

void *pdp_puzzle_new(void)
{
    int i;

    t_pdp_puzzle *x = (t_pdp_puzzle *)pd_new(pdp_puzzle_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("nbblocks"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_nbblocks = DEFAULT_BLOCK_NUMBER;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_puzzle_setup(void)
{
//    post( pdp_puzzle_version );
    pdp_puzzle_class = class_new(gensym("pdp_puzzle"), (t_newmethod)pdp_puzzle_new,
    	(t_method)pdp_puzzle_free, sizeof(t_pdp_puzzle), 0, A_NULL);

    class_addmethod(pdp_puzzle_class, (t_method)pdp_puzzle_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_puzzle_class, (t_method)pdp_puzzle_up, gensym("up"),  A_NULL);
    class_addmethod(pdp_puzzle_class, (t_method)pdp_puzzle_down, gensym("down"), A_NULL);
    class_addmethod(pdp_puzzle_class, (t_method)pdp_puzzle_left, gensym("left"), A_NULL);
    class_addmethod(pdp_puzzle_class, (t_method)pdp_puzzle_right, gensym("right"), A_NULL);
    class_addmethod(pdp_puzzle_class, (t_method)pdp_puzzle_nbblocks, gensym("nbblocks"), A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
