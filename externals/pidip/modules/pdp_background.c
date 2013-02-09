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

#define DEFAULT_RED_VALUE  255
#define DEFAULT_GREEN_VALUE 255
#define DEFAULT_BLUE_VALUE 255
#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240

typedef struct pdp_background_struct
{
    t_object x_obj;

    t_outlet *x_outlet0;

    int x_packet0;

    int x_colorR;
    int x_colorG;
    int x_colorB;
    int x_colorY;
    int x_colorU;
    int x_colorV;

    int x_width;
    int x_height;

} t_pdp_background;

static void pdp_background_bang(t_pdp_background *x)
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

static void pdp_background_dim(t_pdp_background *x, t_floatarg fwidth, t_floatarg fheight)
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

static void pdp_background_red(t_pdp_background *x, t_floatarg fred)
{
   if ( ( (int)fred>=0 ) && ( (int) fred <= 255 ) )
   {
      x->x_colorR = (int) fred;
      x->x_colorY = yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
      x->x_colorU = yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
      x->x_colorV = yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
   }
}

static void pdp_background_green(t_pdp_background *x, t_floatarg fgreen)
{
   if ( ( (int)fgreen>=0 ) && ( (int) fgreen <= 255 ) )
   {
      x->x_colorG = (int) fgreen;
      x->x_colorY = yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
      x->x_colorU = yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
      x->x_colorV = yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
   }
}

static void pdp_background_blue(t_pdp_background *x, t_floatarg fblue)
{
   if ( ( (int)fblue>=0 ) && ( (int) fblue <= 255 ) )
   {
      x->x_colorB = (int) fblue;
      x->x_colorY = yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
      x->x_colorU = yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
      x->x_colorV = yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
   }
}

static void pdp_background_free(t_pdp_background *x)
{
   // nothing to do
}

t_class *pdp_background_class;

void *pdp_background_new(void)
{
    t_pdp_background *x = (t_pdp_background *)pd_new(pdp_background_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("red"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("green"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("blue"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);

    x->x_colorR = DEFAULT_RED_VALUE;
    x->x_colorG = DEFAULT_GREEN_VALUE;
    x->x_colorB = DEFAULT_BLUE_VALUE;

    x->x_colorY = yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
    x->x_colorU = yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );
    x->x_colorV = yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB );

    x->x_width = DEFAULT_WIDTH;
    x->x_height = DEFAULT_HEIGHT;

    x->x_packet0 = -1;

    return (void *)x;
}

#ifdef __cplusplus
extern "C"
{
#endif


void pdp_background_setup(void)
{
    pdp_background_class = class_new(gensym("pdp_background"), (t_newmethod)pdp_background_new,
    	(t_method)pdp_background_free, sizeof(t_pdp_background), 0, A_NULL);

    class_addmethod(pdp_background_class, (t_method)pdp_background_bang, gensym("bang"), A_NULL);
    class_addmethod(pdp_background_class, (t_method)pdp_background_red, gensym("red"), A_FLOAT, A_NULL);
    class_addmethod(pdp_background_class, (t_method)pdp_background_green, gensym("green"), A_FLOAT, A_NULL);
    class_addmethod(pdp_background_class, (t_method)pdp_background_blue, gensym("blue"), A_FLOAT, A_NULL);
    class_addmethod(pdp_background_class, (t_method)pdp_background_dim, gensym("dim"), A_FLOAT, A_FLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
