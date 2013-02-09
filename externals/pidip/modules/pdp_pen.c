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

/*  This object is a free hand drawing object 
 *  Written by Yves Degoyon                                  
 */



#include "pdp.h"
#include "yuv.h"
#include <math.h>

static char   *pdp_pen_version = "pdp_pen: version 0.1, free hand drawing object written by Yves Degoyon (ydegoyon@free.fr)";

static int nbits=0; // number of recursive calls

typedef struct pdp_pen_struct
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

    int x_red;
    int x_green;
    int x_blue;
    int x_xoffset;
    int x_yoffset;
    t_float x_alpha;
    
    int x_pwidth;
    int x_mode;  // 0=draw ( default), 1=erase

    short int *x_bdata;

} t_pdp_pen;

static void pdp_pen_allocate(t_pdp_pen *x, int newsize)
{
 int i;

  if ( x->x_bdata ) freebytes( x->x_bdata, (( x->x_vsize + (x->x_vsize>>1))<<1));

  x->x_vsize = newsize;
  x->x_bdata = (short int *)getbytes((( x->x_vsize + (x->x_vsize>>1))<<1));
  memset( x->x_bdata, 0x00, (( x->x_vsize + (x->x_vsize>>1))<<1) );
}

static void pdp_pen_draw(t_pdp_pen *x, t_floatarg X, t_floatarg Y)
{
 short int  *pbY, *pbU, *pbV;
 int mx, Mx, my, My;
 int px, py;

  if ( !x->x_bdata ) return;

  X = X*x->x_vwidth;
  Y = Y*x->x_vheight;
  // post( "pdp_pen : draw %f %f", X, Y );
  if ( (X<0) || (X>x->x_vwidth) )
  {
     // post( "pdp_pen : draw : wrong X position : %f", X );
     return;
  }  
  if ( (Y<0) || (Y>x->x_vheight) )
  {
     // post( "pdp_pen : draw : wrong Y position : %f", Y );
     return;
  }  
  mx = (X-x->x_pwidth<0)?0:X-x->x_pwidth;
  Mx = (X+x->x_pwidth>x->x_vwidth)?x->x_vwidth:X+x->x_pwidth;
  my = (Y-x->x_pwidth<0)?0:Y-x->x_pwidth;
  My = (Y+x->x_pwidth>x->x_vheight)?x->x_vheight:Y+x->x_pwidth;

  pbY = x->x_bdata;
  pbU = (x->x_bdata+x->x_vsize);
  pbV = (x->x_bdata+x->x_vsize+(x->x_vsize>>2));
  for ( px=mx; px<=Mx; px++ )
  {
    for ( py=my; py<=My; py++ )
    {
      if ( sqrt( pow( (px-X), 2 ) + pow( (py-Y), 2 ) ) < x->x_pwidth )
      {
       if ( x->x_mode == 0 )
       {
         *(pbY+py*x->x_vwidth+px) = 
            (yuv_RGBtoY( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))<<7;
         *(pbU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) =
            (yuv_RGBtoU( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
         *(pbV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) =
            (yuv_RGBtoV( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
       }
       if ( x->x_mode == 1 )
       {
         *(pbY+py*x->x_vwidth+px) = 0;
         *(pbU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 0;
         *(pbV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) = 0;
       }
      }
    } 
  }
}

static void pdp_pen_do_fill(t_pdp_pen *x, t_floatarg X, t_floatarg Y)
{
 short int  *pbY, *pbU, *pbV;
 short int  nX, nY;

  if ( !x->x_bdata ) return;

  nbits++;
  // post( "pdp_pen_do_fill : X=%d, Y=%d stack=%d", (int)X, (int)Y, nbits );

  pbY = x->x_bdata;
  pbU = (x->x_bdata+x->x_vsize);
  pbV = (x->x_bdata+x->x_vsize+(x->x_vsize>>2));

  if ( ( (int)X < 0 ) || ( (int)X >= x->x_vwidth ) || 
       ( (int)Y < 0 ) || ( (int)Y >= x->x_vheight ) )
  {
     nbits--;
     return;
  }

  nX = (int) X; 
  nY = (int) Y; 
  if ( *(pbY+(int)Y*x->x_vwidth+(int)X) != 0 )
  {
     nbits--;
     return;
  }
  else
  {
     *(pbY+nY*x->x_vwidth+nX) = 
        (yuv_RGBtoY( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))<<7;
     *(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoU( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
     *(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoV( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
  }

  nX = (int) X+1; 
  nY = (int) Y; 
  if ( (*(pbY+nY*x->x_vwidth+nX)) == 0 )
  {
     pdp_pen_do_fill( x, nX, nY );
     *(pbY+nY*x->x_vwidth+nX) = 
        (yuv_RGBtoY( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))<<7;
     *(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoU( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
     *(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoV( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
  } 

  nX = (int) X-1; 
  nY = (int) Y; 
  if ( *(pbY+nY*x->x_vwidth+nX) == 0 )
  {
     pdp_pen_do_fill( x, nX, nY );
     *(pbY+nY*x->x_vwidth+nX) = 
        (yuv_RGBtoY( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))<<7;
     *(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoU( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
     *(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoV( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
  } 

  nX = (int) X-1; 
  nY = (int) Y-1; 
  if ( *(pbY+nY*x->x_vwidth+nX) == 0 )
  {
     pdp_pen_do_fill( x, nX, nY );
     *(pbY+nY*x->x_vwidth+nX) = 
        (yuv_RGBtoY( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))<<7;
     *(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoU( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
     *(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoV( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
  } 

  nX = (int) X; 
  nY = (int) Y-1; 
  if ( *(pbY+nY*x->x_vwidth+nX) == 0 )
  {
     pdp_pen_do_fill( x, nX, nY );
     *(pbY+nY*x->x_vwidth+nX) = 
        (yuv_RGBtoY( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))<<7;
     *(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoU( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
     *(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoV( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
  } 

  nX = (int) X+1; 
  nY = (int) Y-1; 
  if ( *(pbY+nY*x->x_vwidth+nX) == 0 )
  {
     pdp_pen_do_fill( x, nX, nY );
     *(pbY+nY*x->x_vwidth+nX) = 
        (yuv_RGBtoY( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))<<7;
     *(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoU( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
     *(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoV( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
  } 

  nX = (int) X-1; 
  nY = (int) Y+1; 
  if ( *(pbY+nY*x->x_vwidth+nX) == 0 )
  {
     pdp_pen_do_fill( x, nX, nY );
     *(pbY+nY*x->x_vwidth+nX) = 
        (yuv_RGBtoY( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))<<7;
     *(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoU( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
     *(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoV( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
  } 

  nX = (int) X; 
  nY = (int) Y+1; 
  if ( *(pbY+nY*x->x_vwidth+nX) == 0 )
  {
     pdp_pen_do_fill( x, nX, nY );
     *(pbY+nY*x->x_vwidth+nX) = 
        (yuv_RGBtoY( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))<<7;
     *(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoU( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
     *(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoV( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
  } 

  nX = (int) X+1; 
  nY = (int) Y+1; 
  if ( *(pbY+nY*x->x_vwidth+nX) == 0 )
  {
     pdp_pen_do_fill( x, nX, nY );
     *(pbY+nY*x->x_vwidth+nX) = 
        (yuv_RGBtoY( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))<<7;
     *(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoU( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
     *(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1)) =
        (yuv_RGBtoV( (x->x_blue << 16) + (x->x_green << 8) + x->x_red ))-128<<8;
  } 

  nbits--;
}

static void pdp_pen_fill(t_pdp_pen *x, t_floatarg X, t_floatarg Y)
{
  X = X*x->x_vwidth;
  Y = Y*x->x_vheight;
  // post( "pdp_pen : draw %f %f", X, Y );
  if ( (X<0) || (X>x->x_vwidth) )
  {
     // post( "pdp_pen : fill : wrong X position : %f", X );
     return;
  }  
  if ( (Y<0) || (Y>x->x_vheight) )
  {
     // post( "pdp_pen : fill : wrong Y position : %f", Y );
     return;
  }  

  pdp_pen_do_fill( x, X, Y );
}

static void pdp_pen_clear(t_pdp_pen *x)
{
  if ( !x->x_bdata ) return;
  if ( x->x_vsize > 0 )
  {
    memset( x->x_bdata, 0x00, (( x->x_vsize + (x->x_vsize>>1))<<1) );
  }
}

static void pdp_pen_width(t_pdp_pen *x, t_floatarg width)
{
  if ( width > 0 )
  {
    x->x_pwidth = (int) width;
  }
}

static void pdp_pen_xoffset(t_pdp_pen *x, t_floatarg xoffset)
{
  x->x_xoffset = (int) xoffset;
}

static void pdp_pen_yoffset(t_pdp_pen *x, t_floatarg yoffset)
{
  x->x_yoffset = (int) yoffset;
}

static void pdp_pen_alpha(t_pdp_pen *x, t_floatarg falpha)
{
  if ( falpha >= 0. && falpha <= 1. )
  {
    x->x_alpha = falpha;
  }
}

static void pdp_pen_mode(t_pdp_pen *x, t_floatarg mode)
{
  if ( ( mode == 0. ) || ( mode == 1. ) )
  {
    x->x_mode = (int) mode;
  }
}

static void pdp_pen_rgb(t_pdp_pen *x, t_floatarg r, t_floatarg g, t_floatarg b)
{
  if ( ( r >= 0. ) && ( r <= 255. ) &&
       ( g >= 0. ) && ( g <= 255. ) &&
       ( b >= 0. ) && ( b <= 255. ) ) 
  {
    x->x_red = (int) r;
    x->x_green = (int) g;
    x->x_blue = (int) b;
    if ( ( r == 0. ) && ( g == 0.) && ( b == 0. ) )
    {
       x->x_red=1; 
       x->x_green=1; 
       x->x_blue=1;
    }
  }
}

static void pdp_pen_process_yv12(t_pdp_pen *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int      i;
    int      px, py;
    short int  *pY, *pU, *pV;
    short int  *pbY, *pbU, *pbV;
    short int  *pnY, *pnU, *pnV;

    /* allocate all ressources */
    if ( ((int)header->info.image.width != x->x_vwidth ) ||
         ((int)header->info.image.height != x->x_vheight ) ) 
    {
        pdp_pen_allocate(x, header->info.image.width*header->info.image.height );
        post( "pdp_pen : reallocating buffers" );
    }

    x->x_vwidth = header->info.image.width;
    x->x_vheight = header->info.image.height;
    x->x_vsize = x->x_vwidth*x->x_vheight;

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    if ( !x->x_bdata ) return;

    memcpy( newdata, data, (( x->x_vsize + (x->x_vsize>>1))<<1) );

    pY = data;
    pU = (data+x->x_vsize);
    pV = (data+x->x_vsize+(x->x_vsize>>2));
    pbY = x->x_bdata;
    pbU = (x->x_bdata+x->x_vsize);
    pbV = (x->x_bdata+x->x_vsize+(x->x_vsize>>2));
    pnY = newdata;
    pnU = (newdata+x->x_vsize);
    pnV = (newdata+x->x_vsize+(x->x_vsize>>2));

    for(py=1; py<x->x_vheight; py++) 
    {
      for(px=0; px<x->x_vwidth; px++) 
      {
        if ( ( (px-x->x_xoffset)>=0 ) && ( (px-x->x_xoffset)<x->x_vwidth ) &&
             ( (py-x->x_yoffset)>=0 ) && ( (py-x->x_yoffset)<x->x_vheight ) )
        {
          if ( *(pbY+(py-x->x_yoffset)*x->x_vwidth+(px-x->x_xoffset)) != 0 )
          {
            *(pnY+py*x->x_vwidth+px) = 
                      (*(pbY+(py-x->x_yoffset)*x->x_vwidth+(px-x->x_xoffset))*x->x_alpha) +
                           (*(pnY+py*x->x_vwidth+px)*(1-x->x_alpha));
            *(pnU+(py>>1)*(x->x_vwidth>>1)+(px>>1)) =
                  (*(pbU+((py-x->x_yoffset)>>1)*(x->x_vwidth>>1)+((px-x->x_xoffset)>>1))*x->x_alpha) +
                           (*(pnU+(py>>1)*(x->x_vwidth>>1)+(px>>1))*(1-x->x_alpha));
            *(pnV+(py>>1)*(x->x_vwidth>>1)+(px>>1)) =
                  (*(pbV+((py-x->x_yoffset)>>1)*(x->x_vwidth>>1)+((px-x->x_xoffset)>>1))*x->x_alpha) +
                           (*(pnV+(py>>1)*(x->x_vwidth>>1)+(px>>1))*(1-x->x_alpha));
          }
        }
      } 
    }

    return;
}

static void pdp_pen_sendpacket(t_pdp_pen *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_pen_process(t_pdp_pen *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type))
   {
	/* pdp_pen_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_pen_process_yv12, pdp_pen_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_pen_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_pen_process */
	    break;
	    
	}
    }
}

static void pdp_pen_input_0(t_pdp_pen *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_pen_process(x);
    }
}

static void pdp_pen_free(t_pdp_pen *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);

}

t_class *pdp_pen_class;

void *pdp_pen_new(void)
{
    int i;

    t_pdp_pen *x = (t_pdp_pen *)pd_new(pdp_pen_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("xoffset"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("yoffset"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("alpha"));

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_bdata = NULL;
    x->x_vsize = -1;

    x->x_red = 255;
    x->x_green = 255;
    x->x_blue = 255;
    x->x_xoffset = 0;
    x->x_yoffset = 0;

    x->x_pwidth = 3;
    x->x_mode = 0;
    x->x_alpha = 1.;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_pen_setup(void)
{
//    post( pdp_pen_version );
    pdp_pen_class = class_new(gensym("pdp_pen"), (t_newmethod)pdp_pen_new,
    	(t_method)pdp_pen_free, sizeof(t_pdp_pen), 0, A_NULL);


    class_addmethod(pdp_pen_class, (t_method)pdp_pen_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_pen_class, (t_method)pdp_pen_draw, gensym("draw"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_pen_class, (t_method)pdp_pen_fill, gensym("fill"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_pen_class, (t_method)pdp_pen_clear, gensym("clear"), A_NULL);
    class_addmethod(pdp_pen_class, (t_method)pdp_pen_width, gensym("width"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_pen_class, (t_method)pdp_pen_rgb, gensym("rgb"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_pen_class, (t_method)pdp_pen_mode, gensym("mode"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_pen_class, (t_method)pdp_pen_xoffset, gensym("xoffset"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_pen_class, (t_method)pdp_pen_yoffset, gensym("yoffset"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_pen_class, (t_method)pdp_pen_alpha, gensym("alpha"), A_DEFFLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
