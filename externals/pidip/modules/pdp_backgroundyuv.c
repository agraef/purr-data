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



#include "pdp.h"
#include "yuv.h"
#include "time.h"
#include "sys/time.h"

#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240

typedef struct pdp_backgroundyuv_struct
{
    t_object x_obj;

    t_outlet *x_outlet0;

    int x_packet0;

    int x_colorY;
    int x_colorU;
    int x_colorV;

    int x_width;
    int x_height;

} t_pdp_backgroundyuv;

static void pdp_backgroundyuv_bang(t_pdp_backgroundyuv *x)
{

  t_pdp *header;
  unsigned char *data;
  unsigned char *pY, *pU, *pV;
  int px, py;

    x->x_packet0 = pdp_packet_new_bitmap_yv12( x->x_width, x->x_height );

    data = (unsigned char *)pdp_packet_data(x->x_packet0);
    pY = data;
    pV = data+(x->x_width*x->x_height);
    pU = data+(x->x_width*x->x_height)+((x->x_width*x->x_height)>>2);

    memset( pY, (unsigned char)x->x_colorY, x->x_width*x->x_height );
    memset( pV, (unsigned char)x->x_colorV, (x->x_width*x->x_height>>2) );
    memset( pU, (unsigned char)x->x_colorU, (x->x_width*x->x_height>>2) );

    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet0);

}

static void pdp_backgroundyuv_dim(t_pdp_backgroundyuv *x, t_floatarg fwidth, t_floatarg fheight)
{
   if ( ( (int)fwidth>0 ) && ( (int) fheight>0 ) )
   {
      if ( ((int)fwidth)%8 == 0 )
      {
        x->x_width = (int)fwidth; 
      }
      else
      {
        x->x_width = (int)fwidth + (8-((int)fwidth)%8); // align on 8 
      }
      if ( ((int)fheight)%8 == 0 )
      {
        x->x_height = (int)fheight; 
      }
      else
      {
        x->x_height = (int)fheight + (8-((int)fheight)%8); // align on 8 
      }
   }
}

static void pdp_backgroundyuv_y(t_pdp_backgroundyuv *x, t_floatarg fy)
{
   if ( ( (int)fy>=0 ) && ( (int) fy <= 255 ) )
   {
      x->x_colorY = fy;
   }
}

static void pdp_backgroundyuv_u(t_pdp_backgroundyuv *x, t_floatarg fu)
{
   if ( ( (int)fu>=0 ) && ( (int) fu <= 255 ) )
   {
      x->x_colorU = fu;
   }
}

static void pdp_backgroundyuv_v(t_pdp_backgroundyuv *x, t_floatarg fv)
{
   if ( ( (int)fv>=0 ) && ( (int) fv <= 255 ) )
   {
      x->x_colorV = fv;
   }
}

static void pdp_backgroundyuv_free(t_pdp_backgroundyuv *x)
{
   // nothing to do
}

t_class *pdp_backgroundyuv_class;

void *pdp_backgroundyuv_new(void)
{
    t_pdp_backgroundyuv *x = (t_pdp_backgroundyuv *)pd_new(pdp_backgroundyuv_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("y"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("u"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("v"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);

    x->x_colorY = 255;
    x->x_colorU = 128;
    x->x_colorV = 128;

    x->x_width = DEFAULT_WIDTH;
    x->x_height = DEFAULT_HEIGHT;

    x->x_packet0 = -1;

    return (void *)x;
}

#ifdef __cplusplus
extern "C"
{
#endif


void pdp_backgroundyuv_setup(void)
{
    pdp_backgroundyuv_class = class_new(gensym("pdp_backgroundyuv"), (t_newmethod)pdp_backgroundyuv_new,
    	(t_method)pdp_backgroundyuv_free, sizeof(t_pdp_backgroundyuv), 0, A_NULL);

    class_addmethod(pdp_backgroundyuv_class, (t_method)pdp_backgroundyuv_bang, gensym("bang"), A_NULL);
    class_addmethod(pdp_backgroundyuv_class, (t_method)pdp_backgroundyuv_y, gensym("y"), A_FLOAT, A_NULL);
    class_addmethod(pdp_backgroundyuv_class, (t_method)pdp_backgroundyuv_u, gensym("u"), A_FLOAT, A_NULL);
    class_addmethod(pdp_backgroundyuv_class, (t_method)pdp_backgroundyuv_v, gensym("v"), A_FLOAT, A_NULL);
    class_addmethod(pdp_backgroundyuv_class, (t_method)pdp_backgroundyuv_dim, gensym("dim"), A_FLOAT, A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
