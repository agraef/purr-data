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

/* This object is an skeletonization object that should be fed 
 * with a binary blobs image.
 *
 * the algorithm comes from :
 * 
 * "Efficient Binary Image Thinning using Neighborhood Maps"
 * by Joseph M. Cychosz, 3ksnn64@ecn.purdue.edu
 * in "Graphics Gems IV", Academic Press, 1994
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <stdio.h>

                   // direction masks
                   //   N     S     W     E
static int masks[] = { 0200, 0002, 0040, 0010 };

//      True if pixel neighbor map indicates the pixel is 8-simple and 
//      not an end point and thus can be deleted.  The neighborhood    
//      map is defined as an integer of bits abcdefghi with a non-zero 
//      bit representing a non-zero pixel.  The bit assignment for the 
//      neighborhood is:                                               
//                                                                    
//                              a b c                                  
//                              d e f                                  
//                              g h i                                 

static  unsigned char   delete[512] = {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};


static char   *pdp_skeleton_version = "pdp_skeleton: a skeletonization object version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_skeleton_struct
{
    t_object x_obj;

    int x_packet0;
    int x_packet1;
    int x_queue_id;
    int x_dropped;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

    int x_nbpasses;

    t_outlet *x_outlet0; // output skeleton gradient

} t_pdp_skeleton;

static void pdp_skeleton_passes(t_pdp_skeleton *x, t_floatarg f)
{
  if ( (int)f <= 0 )
  {
     post( "pdp_skeleton : wrong pass number : %d", (int)f );
     return;
  }
  x->x_nbpasses = (int)f;
}

static void pdp_skeleton_process_grey(t_pdp_skeleton *x)
{
  t_pdp     *header = pdp_packet_header(x->x_packet1);
  unsigned char* data = (unsigned char*)pdp_packet_data(x->x_packet1);
  short int* newdata = (short int*)pdp_packet_data(x->x_packet0);
  int ix, iy;
  int ip;
  int pc=0;
  int dcount = 1; // deleted pixel count
  int p, q;       // neighborhood maps of adjacent cells
  unsigned char  *qb; // neighborhood maps of previous scanline
  int m;              // deletion direction mask

  /* allocate all ressources */
  if ( ( (int)header->info.image.width != x->x_vwidth ) ||
       ( (int)header->info.image.height != x->x_vheight ) )
  {
      x->x_vwidth = header->info.image.width;
      x->x_vheight = header->info.image.height;
      x->x_vsize = x->x_vwidth*x->x_vheight;
  }
 
  qb = (unsigned char*) malloc(x->x_vwidth*sizeof(unsigned char));
  qb[x->x_vwidth-1] = 0; // used for lower-right pixel

   while ( (dcount>0) && (pc < x->x_nbpasses) ) {   // scan image
     pc++;
     dcount = 0;

     for ( ip = 0 ; ip < 4 ; ip++ ) {

       m = masks[ip];

       // build initial previous scan buffer.
       p = data[0] != 0;
       for ( ix = 0 ; ix < x->x_vwidth-1 ; ix++ )
         qb[ix] = p = ((p<<1)&0006) | (data[ix+1] != 0);

       // scan image for pixel deletion candidates.
       for ( iy = 0 ; iy < x->x_vheight-1 ; iy++ ) {

         q = qb[0];
         p = ((q<<3)&0110) | (data[(iy+1)*x->x_vwidth] != 0);

         for ( ix = 0 ; ix < x->x_vwidth-1 ; ix++ ) {
           q = qb[ix];
           p = ((p<<1)&0666) | ((q<<3)&0110) | (data[(iy+1)*x->x_vwidth+(ix+1)] != 0);
           qb[ix] = p;
           if  ( ((p&m) == 0) && delete[p] ) {
                 dcount++;
                 data[iy*x->x_vwidth+ix] = 0;
           }
         }

         // process right edge pixel.
         p = (p<<1)&0666;
         if  ( (p&m) == 0 && delete[p] ) {
              dcount++;
              data[iy*x->x_vwidth+x->x_vwidth-1] = 0;
         }
      }

      // process bottom scan line
      for ( ix = 0 ; ix < x->x_vwidth ; ix++ ) {
         q = qb[ix];
         p = ((p<<1)&0666) | ((q<<3)&0110);
         if  ( (p&m) == 0 && delete[p] ) {
            dcount++;
            data[(x->x_vheight-1)*x->x_vwidth+ix] = 0;
        }
      }
    }

    // post( "pdp_skeleton : deleted %d pixels", dcount );
  }

  for ( iy = 0 ; iy < x->x_vheight ; iy++ ) {
    for ( ix = 0 ; ix < x->x_vwidth ; ix++ ) {
       newdata[iy*x->x_vwidth+ix]=(short int)data[iy*x->x_vwidth+ix]<<7;
    }
  }

  return;
}

static void pdp_skeleton_sendpacket(t_pdp_skeleton *x)
{
    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet0);

    pdp_packet_mark_unused(x->x_packet1);
    x->x_packet1 = -1;
}

static void pdp_skeleton_process(t_pdp_skeleton *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet1))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_skeleton_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet1)->info.image.encoding)
        {

	case PDP_BITMAP_GREY:
            pdp_queue_add(x, pdp_skeleton_process_grey, pdp_skeleton_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_skeleton_process */
	    break;
	    
	}
    }

}

static void pdp_skeleton_input_0(t_pdp_skeleton *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/grey/*") );
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet1, (int)x->x_packet0, pdp_gensym("bitmap/grey/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        pdp_skeleton_process(x);
    }
}

static void pdp_skeleton_free(t_pdp_skeleton *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_skeleton_class;

void *pdp_skeleton_new(void)
{
    int i;

    t_pdp_skeleton *x = (t_pdp_skeleton *)pd_new(pdp_skeleton_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("passes"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;

    x->x_vwidth = 320;
    x->x_vheight = 240;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    x->x_nbpasses = 20;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_skeleton_setup(void)
{
    // post( pdp_skeleton_version );
    pdp_skeleton_class = class_new(gensym("pdp_skeleton"), (t_newmethod)pdp_skeleton_new,
    	(t_method)pdp_skeleton_free, sizeof(t_pdp_skeleton), 0, A_NULL);

    class_addmethod(pdp_skeleton_class, (t_method)pdp_skeleton_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_skeleton_class, (t_method)pdp_skeleton_passes, gensym("passes"),  A_FLOAT, A_NULL);
}

#ifdef __cplusplus
}
#endif
