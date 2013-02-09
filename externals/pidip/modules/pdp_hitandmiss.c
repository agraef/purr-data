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

/*  This object is a morphological operator for hit and miss agorithm 
 *  using as a structuring element a WxH square
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <stdio.h>

static char   *pdp_hitandmiss_version = "pdp_hitandmiss: morphology : hit&miss version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_hitandmiss_struct
{
    t_object x_obj;

    int x_packet0;
    int x_packet1;
    int x_queue_id;
    int x_dropped;

    int x_vwidth;
    int x_vheight;
    int x_vsize;
    int x_kernelw; // width of the (square) kernel
    int x_kernelh; // height of the square kernel
    char  *x_kdata;  // kernel data
    int x_nbpasses; // number of passes
    short int *x_frame;  // keep a copy of current frame for transformations

    t_outlet *x_pdp_output; // output packets

} t_pdp_hitandmiss;

static void pdp_hitandmiss_nbpasses(t_pdp_hitandmiss *x,  t_floatarg fpasses )
{
   if ( fpasses>=1.) 
   {
      x->x_nbpasses = (int)fpasses;
   }
}

static void pdp_hitandmiss_kernelw(t_pdp_hitandmiss *x,  t_floatarg fkernelw )
{
 int oldw, minw;
 char *nkdata;

   if ( fkernelw>=0.) 
   {
      oldw = x->x_kernelw;
      x->x_kernelw = (int)fkernelw;

      nkdata= (char*) malloc( x->x_kernelw*x->x_kernelh );

      minw = (x->x_kernelw>oldw?oldw:x->x_kernelw );
      memcpy( nkdata, x->x_kdata, minw*x->x_kernelh );

      if ( x->x_kdata ) 
      {
         free( x->x_kdata );
      }
      x->x_kdata = nkdata;
   }

}

static void pdp_hitandmiss_kernelh(t_pdp_hitandmiss *x,  t_floatarg fkernelh )
{
 int oldh, minh;
 char *nkdata;

   if ( fkernelh>=0.) 
   {
      oldh = x->x_kernelh;
      x->x_kernelh = (int)fkernelh;

      nkdata= (char*) malloc( x->x_kernelw*x->x_kernelh );

      minh = (x->x_kernelh>oldh?oldh:x->x_kernelh );
      memcpy( nkdata, x->x_kdata, x->x_kernelw*minh );

      if ( x->x_kdata ) 
      {
         free( x->x_kdata );
      }
      x->x_kdata = nkdata;
   }
}

static void pdp_hitandmiss_kdata(t_pdp_hitandmiss *x, t_symbol *s, int argc, t_atom *argv)
{
 int nbvalues, ivalue, iv;

   if ( argc > x->x_kernelw*x->x_kernelh )
   {
      nbvalues = x->x_kernelh*x->x_kernelw;
      post( "pdp_hitandmiss : too many data for the kernel, it should be %dx%d", x->x_kernelw, x->x_kernelh ); 
      post( "pdp_hitandmiss : truncated to %d", nbvalues ); 
   }
   else if ( argc < x->x_kernelw*x->x_kernelh )
   {
      nbvalues = argc;
      post( "pdp_hitandmiss : too few data for the kernel, it should be %dx%d", x->x_kernelw, x->x_kernelh ); 
      post( "pdp_hitandmiss : padding with -1 (as if not used)" ); 
      for ( iv=argc; iv<x->x_kernelw*x->x_kernelh; iv++ )
      {
        x->x_kdata[iv] = -1;
      }
   }
   else
   {
      nbvalues = argc;
   }

   for ( iv=0; iv<nbvalues; iv++ )
   {
      if ( argv[iv].a_type != A_FLOAT )
      {
         post( "pdp_hitandmiss : wrong data (%d), unchanged", iv );
      }
      else
      {
         ivalue = (int) argv[iv].a_w.w_float;
         if ( ( ivalue != 0 ) && ( ivalue != 1 ) && ( ivalue != -1 ) )
         {
           post( "pdp_hitandmiss : wrong data (%d), unchanged", iv );
         }
         else
         {
           x->x_kdata[iv] = ivalue;
         }
      }
   }
}

static void pdp_hitandmiss_allocate(t_pdp_hitandmiss *x)
{
    x->x_frame = (short int *) getbytes ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );

    if ( !x->x_frame )
    {
       post( "pdp_hitandmiss : severe error : cannot allocate buffer !!! ");
       return;
    }
}

static void pdp_hitandmiss_free_ressources(t_pdp_hitandmiss *x)
{
    if ( x->x_frame ) freebytes ( x->x_frame, ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );
}

static void pdp_hitandmiss_process_yv12(t_pdp_hitandmiss *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     i;
    int     px=0, py=0; 
    short int *pfY, *pfU, *pfV;
    int     ppx, ppy, ix, iy, pn, kx, ky;
    short int pvalue;

    // allocate all ressources
    if ( ( (int)header->info.image.width != x->x_vwidth ) ||
         ( (int)header->info.image.height != x->x_vheight ) )
    {
        pdp_hitandmiss_free_ressources( x );
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_hitandmiss_allocate( x );
        post( "pdp_hitandmiss : reallocated buffers" );
    }

    // post( "pdp_hitandmiss : newheader:%x", newheader );

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( newdata, data, x->x_vsize+(x->x_vsize>>1)<<1 );
    memcpy( x->x_frame, data, x->x_vsize+(x->x_vsize>>1)<<1 );

    // hit and miss (supposedly) binary image by using a 3x3 square as a structuring element
    pfY = x->x_frame;
    pfV = x->x_frame+x->x_vsize;
    pfU = x->x_frame+x->x_vsize+(x->x_vsize>>2);
    kx = (x->x_kernelw/2);
    ky = (x->x_kernelh/2);
    for ( pn=0; pn<x->x_nbpasses; pn++ )
    {
      memcpy( x->x_frame, newdata, x->x_vsize+(x->x_vsize>>1)<<1 );
      for ( py=0; py<x->x_vheight; py++ )
      {
        for ( px=0; px<x->x_vwidth; px++ )
        {
          *(newdata+py*x->x_vwidth+px) = ((255)<<7);
          for (ix=-kx; ix<=kx; ix++)
          {
            ppx=px+ix;
            if ( (ppx>=0) && (ppx<x->x_vwidth) )
            {
              for (iy=-ky; iy<=ky; iy++)
              {
                ppy=py+iy;
                if ( (ppy>=0) && (ppy<x->x_vheight) )
                {
                   if ( x->x_kdata[(ky+iy)*x->x_kernelw+(kx+ix)] == 1 )
                   {
                      pvalue = ((255)<<7);
                   }
                   else if ( x->x_kdata[(ky+iy)*x->x_kernelw+(kx+ix)] == 0 )
                   {
                      pvalue = 0;
                   }
                   else // unused bit in the kernel, usually -1
                   {
                      continue;
                   }
                   if( *(pfY+ppy*x->x_vwidth+ppx) != pvalue )
                   {
                     *(newdata+py*x->x_vwidth+px) = 0;
                     break;
                   }
                }
              }
              if ( *(newdata+py*x->x_vwidth+px) == 0 ) break;
            }
          }
        }
      }
    }

    return;
}

static void pdp_hitandmiss_sendpacket(t_pdp_hitandmiss *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet1);
}

static void pdp_hitandmiss_process(t_pdp_hitandmiss *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_hitandmiss_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_hitandmiss_process_yv12, pdp_hitandmiss_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_hitandmiss_process */
	    break;
	    
	}
    }

}

static void pdp_hitandmiss_input_0(t_pdp_hitandmiss *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        pdp_hitandmiss_process(x);
    }
}

static void pdp_hitandmiss_free(t_pdp_hitandmiss *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
    pdp_hitandmiss_free_ressources( x );
}

t_class *pdp_hitandmiss_class;

void *pdp_hitandmiss_new(void)
{
    int i;

    t_pdp_hitandmiss *x = (t_pdp_hitandmiss *)pd_new(pdp_hitandmiss_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("nbpasses"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("kernelw"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("kernelh"));

    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_vwidth = -1;
    x->x_vheight = -1;
    x->x_vsize = -1;
    x->x_frame = NULL;
    x->x_kernelw = 3;
    x->x_kernelh = 3;
    x->x_kdata = (char *)malloc( x->x_kernelw*x->x_kernelh );
    x->x_kdata[0] = -1;
    x->x_kdata[1] = 1;
    x->x_kdata[2] = -1;
    x->x_kdata[3] = 0;
    x->x_kdata[4] = 1;
    x->x_kdata[5] = 1;
    x->x_kdata[6] = 0;
    x->x_kdata[7] = 0;
    x->x_kdata[8] = -1;
    // that is :
    // | -1 1 -1 |
    // | 0  1  1 |
    // | 0  0 -1 |

    x->x_nbpasses = 1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_hitandmiss_setup(void)
{
    // post( pdp_hitandmiss_version );
    pdp_hitandmiss_class = class_new(gensym("pdp_hitandmiss"), (t_newmethod)pdp_hitandmiss_new,
    	(t_method)pdp_hitandmiss_free, sizeof(t_pdp_hitandmiss), 0, A_NULL);

    class_addmethod(pdp_hitandmiss_class, (t_method)pdp_hitandmiss_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_hitandmiss_class, (t_method)pdp_hitandmiss_nbpasses, gensym("nbpasses"), A_FLOAT, A_NULL);
    class_addmethod(pdp_hitandmiss_class, (t_method)pdp_hitandmiss_kernelw, gensym("kernelw"), A_FLOAT, A_NULL);
    class_addmethod(pdp_hitandmiss_class, (t_method)pdp_hitandmiss_kernelh, gensym("kernelh"), A_FLOAT, A_NULL);
    class_addmethod(pdp_hitandmiss_class, (t_method)pdp_hitandmiss_kdata, gensym("kernel"), A_GIMME, A_NULL);

}

#ifdef __cplusplus
}
#endif
