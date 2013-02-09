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

/*  This object is an object allowing juxtaposition of frames from two inlets
 *  Written by Yves Degoyon                                 
 */



#include "pdp.h"
#include <math.h>

static char   *pdp_ocanvas_version = "pdp_ocanvas: version 0.1, display for several video sources, written by Yves Degoyon (ydegoyon@free.fr)";

#define MAX_CANVAS_INPUT 10

typedef struct pdp_ocanvas_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_dropped;
    int x_queue_id;

    int x_opacket;

    int x_current;
    t_float x_xmouse;
    t_float x_ymouse;

    int x_average;

    int *x_packets;
    int *x_widths;
    int *x_heights;
    t_float *x_xoffsets;
    t_float *x_yoffsets;
    t_float *x_alphas;
    int *x_sizes;

    int x_owidth;
    int x_oheight;
    int x_osize;
    int x_nbinputs;

} t_pdp_ocanvas;

static void pdp_ocanvas_process_yv12(t_pdp_ocanvas *x)
{
  int     px, py, ppx, ppy, ii, nbs;
  short int *pY, *pU, *pV;
  short int *piY, *piU, *piV;
  t_pdp     *oheader;
  short int *odata;
  t_pdp     *iheader;
  short int *idata;

  x->x_opacket = pdp_packet_new_image_YCrCb( x->x_owidth, x->x_oheight );
  oheader = pdp_packet_header(x->x_opacket);
  odata   = (short int *)pdp_packet_data(x->x_opacket);

  oheader->info.image.encoding = PDP_IMAGE_YV12;
  oheader->info.image.width = x->x_owidth;
  oheader->info.image.height = x->x_oheight;

  memset( odata, 0x00, (x->x_osize + (x->x_osize>>1))<<1 );

  pY = odata;
  pV = odata+x->x_osize;
  pU = odata+x->x_osize+(x->x_osize>>2);
  for ( py=0; py<x->x_oheight; py++ )
  {
    for ( px=0; px<x->x_owidth; px++ )
    {
       nbs=0;
       for ( ii=0; ii<x->x_nbinputs; ii++)
       {
         if ( x->x_packets[ii] != -1 )
         {
            if ( (px >= (int)x->x_xoffsets[ii]) && ( px < (int)x->x_xoffsets[ii] + x->x_widths[ii] )
                 && (py >= (int)x->x_yoffsets[ii]) && ( py < (int)x->x_yoffsets[ii] + x->x_heights[ii] )
            ) 
            {
               nbs++;
               idata = (short int *)pdp_packet_data(x->x_packets[ii]);
               piY = idata;
               piV = idata+x->x_sizes[ii];
               piU = idata+x->x_sizes[ii]+(x->x_sizes[ii]>>2);
               ppx = px-(int)x->x_xoffsets[ii];
               ppy = py-(int)x->x_yoffsets[ii];
               *(pY+py*x->x_owidth+px) += *(piY+ppy*x->x_widths[ii]+ppx)*x->x_alphas[ii];
               if ( (px%2==0) && (py%2==0) )
               {
                 *(pU+(py>>1)*(x->x_owidth>>1)+(px>>1)) +=
                         *(piU+(ppy>>1)*(x->x_widths[ii]>>1)+(ppx>>1));
                 *(pV+(py>>1)*(x->x_owidth>>1)+(px>>1)) +=
                         *(piV+(ppy>>1)*(x->x_widths[ii]>>1)+(ppx>>1));
               }
            }
         }
       }
       if ( ( nbs != 0 ) && x->x_average )
       {
           *(pY+py*x->x_owidth+px) /= nbs;
           if ( (px%2==0) && (py%2==0) )
           {
             *(pU+(py>>1)*(x->x_owidth>>1)+(px>>1)) /= nbs;
             *(pV+(py>>1)*(x->x_owidth>>1)+(px>>1)) /= nbs;
           }
       }
    }
  }

  return;
}

static void pdp_ocanvas_sendpacket(t_pdp_ocanvas *x)
{
  /* unregister and propagate if valid dest packet */
  pdp_packet_pass_if_valid(x->x_outlet0, &x->x_opacket);
}

static void pdp_ocanvas_process(t_pdp_ocanvas *x, int ni)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packets[ni]))
   	&& (PDP_IMAGE == header->type)){
    
	/* pdp_ocanvas_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packets[ni])->info.image.encoding){

	case PDP_IMAGE_YV12:
            pdp_queue_add(x, pdp_ocanvas_process_yv12, pdp_ocanvas_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    break;

	default:
	    /* don't know the type, so dont pdp_ocanvas_process */
	    break;
	    
	}
    }
}

static void pdp_ocanvas_offset(t_pdp_ocanvas *x, t_floatarg ni, t_floatarg xoffset, t_floatarg yoffset)
{
  if ( ( ni < 1 ) || ( ni > x->x_nbinputs ) )
  {
     post( "pdp_ocanvas : offset : wrong source : %d : must be between 1 and %d", ni, x->x_nbinputs );
     return;
  }
  x->x_xoffsets[(int)ni-1] = xoffset;
  x->x_yoffsets[(int)ni-1] = yoffset;
}

static void pdp_ocanvas_alpha(t_pdp_ocanvas *x, t_floatarg ni, t_floatarg xalpha)
{
  if ( ( ni < 1 ) || ( ni > x->x_nbinputs ) )
  {
     post( "pdp_ocanvas : alpha : wrong source : %d : must be between 1 and %d", ni, x->x_nbinputs );
     return;
  }
  if ( ( xalpha < .0 ) || ( xalpha > 1.0 ) )
  {
     post( "pdp_ocanvas : alpha : wrong alpha value : %f : must be between 0 and 1", xalpha );
     return;
  }
  x->x_alphas[(int)ni-1] = xalpha;
}

static void pdp_ocanvas_select(t_pdp_ocanvas *x, t_floatarg X, t_floatarg Y)
{
 int ii;

  x->x_current = -1;
  X = X*x->x_owidth;
  Y = Y*x->x_oheight;
  // post( "pdp_ocanvas : select %f %f", X, Y );
  for ( ii=0; ii<x->x_nbinputs; ii++)
  {
    if ( x->x_packets[ii] != -1 )
    {
      if ( (X >= x->x_xoffsets[ii]) && ( X < x->x_xoffsets[ii] + x->x_widths[ii] )
           && (Y >= x->x_yoffsets[ii]) && ( Y < x->x_yoffsets[ii] + x->x_heights[ii] )
         ) 
      {
         x->x_current = ii;
         x->x_xmouse = X;
         x->x_ymouse = Y;
      }
    }
  }
}

static void pdp_ocanvas_drag(t_pdp_ocanvas *x, t_floatarg X, t_floatarg Y)
{
  X = X*x->x_owidth;
  Y = Y*x->x_oheight;
  // post( "pdp_ocanvas : drag %f %f", X, Y );
  if ( x->x_current != -1 )
  {
     x->x_xoffsets[ x->x_current ] += (X-x->x_xmouse);
     x->x_yoffsets[ x->x_current ] += (Y-x->x_ymouse);
     x->x_xmouse = X;
     x->x_ymouse = Y;
  }
}

static void pdp_ocanvas_unselect(t_pdp_ocanvas *x)
{
  x->x_current = -1;
}

static void pdp_ocanvas_average(t_pdp_ocanvas *x, t_floatarg bvalue)
{
  if ( ( bvalue == 0.0 ) || ( bvalue == 1.0 ) )
  {
     x->x_average = (int) bvalue;
  }
}

static void pdp_ocanvas_input(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f, int ni)
{
  t_pdp     *header;
  short int *data;

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
    {
      /* release the packet */
      if ( x->x_packets[ni] != -1 )
      {
        pdp_packet_delete(x->x_packets[ni]);
        x->x_packets[ni] = -1;
      }
      x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packets[ni], (int)f, pdp_gensym("image/YCrCb/*") );
      if ( x->x_packets[ni] != -1 )
      {
        header = pdp_packet_header(x->x_packets[ni]);
        x->x_widths[ni] = header->info.image.width;
        x->x_heights[ni] = header->info.image.height;
        x->x_sizes[ni] = x->x_widths[ni]*x->x_heights[ni];
      }
    }

    if ((s == gensym("process")) && (-1 != x->x_packets[ni]) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_ocanvas_process(x, ni);
    }
}

static void pdp_ocanvas_input0(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_ocanvas_input(x, s, f, 0);
}

static void pdp_ocanvas_input1(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_ocanvas_input(x, s, f, 1);
}

static void pdp_ocanvas_input2(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_ocanvas_input(x, s, f, 2);
}

static void pdp_ocanvas_input3(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_ocanvas_input(x, s, f, 3);
}

static void pdp_ocanvas_input4(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_ocanvas_input(x, s, f, 4);
}

static void pdp_ocanvas_input5(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_ocanvas_input(x, s, f, 5);
}

static void pdp_ocanvas_input6(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_ocanvas_input(x, s, f, 6);
}

static void pdp_ocanvas_input7(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_ocanvas_input(x, s, f, 7);
}

static void pdp_ocanvas_input8(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_ocanvas_input(x, s, f, 8);
}

static void pdp_ocanvas_input9(t_pdp_ocanvas *x, t_symbol *s, t_floatarg f)
{
  pdp_ocanvas_input(x, s, f, 9);
}

static void pdp_ocanvas_free(t_pdp_ocanvas *x)
{
 int ii;

  pdp_queue_finish(x->x_queue_id);
  for ( ii=0; ii<x->x_nbinputs; ii++)
  {
    pdp_packet_mark_unused(x->x_packets[ii]);
  }
  pdp_packet_mark_unused(x->x_opacket);
  if ( x->x_packets ) freebytes( x->x_packets, x->x_nbinputs*sizeof(int) );
  if ( x->x_widths ) freebytes( x->x_widths, x->x_nbinputs*sizeof(int) );
  if ( x->x_heights ) freebytes( x->x_heights, x->x_nbinputs*sizeof(int) );
  if ( x->x_sizes ) freebytes( x->x_sizes, x->x_nbinputs*sizeof(int) );
  if ( x->x_xoffsets ) freebytes( x->x_xoffsets, x->x_nbinputs*sizeof(t_float) );
  if ( x->x_yoffsets ) freebytes( x->x_yoffsets, x->x_nbinputs*sizeof(t_float) );
  if ( x->x_alphas ) freebytes( x->x_alphas, x->x_nbinputs*sizeof(t_float) );
}

t_class *pdp_ocanvas_class;

void *pdp_ocanvas_new(t_symbol *s, int argc, t_atom *argv)
{
  t_pdp_ocanvas *x = (t_pdp_ocanvas *)pd_new(pdp_ocanvas_class);
  int ii;
  char *imes[32];

  if ( argc != 3 )
  {
    post( "pdp_ocanvas : wrong constructor : pdp_ocanvas <width> <height> <nb inputs> (argc=%d)", argc);
    return NULL;
  }
  if ( argv[0].a_type != A_FLOAT || argv[1].a_type != A_FLOAT ||
       argv[2].a_type != A_FLOAT )
  {
    post( "pdp_ocanvas : wrong constructor : pdp_ocanvas <width> <height> <nb inputs>");
    return NULL;
  }

  x->x_owidth = ( (int) argv[0].a_w.w_float / 8 ) * 8;  // round to a multiple of 8
  x->x_oheight = ( (int) argv[1].a_w.w_float / 8 ) * 8; // round to a multiple of 8
  x->x_osize = x->x_owidth*x->x_oheight;
  x->x_nbinputs = (int) argv[2].a_w.w_float;

  if ( x->x_owidth < 0 )
  {
    post( "pdp_ocanvas : wrong width : %d", x->x_owidth);
    return NULL;
  }
  if ( x->x_oheight < 0 )
  {
    post( "pdp_ocanvas : wrong height : %d", x->x_oheight);
    return NULL;
  }
  if ( x->x_nbinputs < 0 )
  {
    post( "pdp_ocanvas : wrong number of inputs : %d", x->x_nbinputs);
    return NULL;
  }
  if ( x->x_nbinputs > MAX_CANVAS_INPUT )
  {
    post( "pdp_ocanvas : number of inputs is too high : %d : only %d supported", 
          x->x_nbinputs, MAX_CANVAS_INPUT);
    return NULL;
  }

  post ( "pdp_ocanvas : new %dx%d canvas with %d inputs", x->x_owidth, x->x_oheight, x->x_nbinputs );

  x->x_packets = ( int* ) getbytes( x->x_nbinputs*sizeof(int) );
  x->x_widths = ( int* ) getbytes( x->x_nbinputs*sizeof(int) );
  x->x_heights = ( int* ) getbytes( x->x_nbinputs*sizeof(int) );
  x->x_sizes = ( int* ) getbytes( x->x_nbinputs*sizeof(int) );
  x->x_xoffsets = ( t_float* ) getbytes( x->x_nbinputs*sizeof(t_float) );
  x->x_yoffsets = ( t_float* ) getbytes( x->x_nbinputs*sizeof(t_float) );
  x->x_alphas = ( t_float* ) getbytes( x->x_nbinputs*sizeof(t_float) );

  x->x_opacket = pdp_packet_new_image_YCrCb( x->x_owidth, x->x_oheight );

  for ( ii=0; ii<x->x_nbinputs; ii++)
  {
    sprintf( (char*)imes, "pdp%d", ii );
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("pdp"), gensym((char*)imes) );
    x->x_packets[ii] = -1;
    x->x_xoffsets[ii] = 0.;
    x->x_yoffsets[ii] = 0.;
    x->x_alphas[ii] = 1.;
  }
  x->x_current = -1;
  x->x_average = 0;
  x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

  return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_ocanvas_setup(void)
{
 char *imes[32];

  // post( pdp_ocanvas_version );
  pdp_ocanvas_class = class_new(gensym("pdp_ocanvas"), (t_newmethod)pdp_ocanvas_new,
    	(t_method)pdp_ocanvas_free, sizeof(t_pdp_ocanvas), 0, A_GIMME, A_NULL);


  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_input0, gensym("pdp0"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_input1, gensym("pdp1"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_input2, gensym("pdp2"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_input3, gensym("pdp3"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_input4, gensym("pdp4"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_input5, gensym("pdp5"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_input6, gensym("pdp6"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_input7, gensym("pdp7"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_input8, gensym("pdp8"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_input9, gensym("pdp9"), A_SYMBOL, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_offset, gensym("offset"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_alpha, gensym("alpha"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_select, gensym("select"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_drag, gensym("drag"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_unselect, gensym("unselect"), A_NULL);
  class_addmethod(pdp_ocanvas_class, (t_method)pdp_ocanvas_average, gensym("average"), A_DEFFLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
