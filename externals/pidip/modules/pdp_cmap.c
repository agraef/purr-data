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

/*  This object is a color mapper that lets you change the colors within the image
 */

#include "pdp.h"
#include "g_canvas.h"
#include "yuv.h"
#include <math.h>
#include <stdio.h>

struct _rtext
{
    char *x_buf;
    int x_bufsize;
    int x_selstart;
    int x_selend;
    int x_active;
    int x_dragfrom;
    int x_height;
    int x_drawnwidth;
    int x_drawnheight;
    t_text *x_text;
    t_glist *x_glist;
    char x_tag[50];
    struct _rtext *x_next;
};

#define t_rtext struct _rtext

extern int rtext_width(t_rtext *x);
extern int rtext_height(t_rtext *x);
extern t_rtext *glist_findrtext(t_glist *gl, t_text *who);

typedef struct _color
{
    int on;
    int y,u,v;
    int oy,ou,ov;
    int tolerance;
} t_color;

#define COLORHEIGHT 5
#define DEFAULT_CAPACITY 10

static char   *pdp_cmap_version = "pdp_cmap: a color mapper version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_cmap_struct
{
    t_object x_obj;
    t_float x_f;

    int x_packet0;
    int x_dropped;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

    int x_capacity; // number of mapped colors
    int x_current;  // current color
    t_color *x_colors; // color substitution table

    int x_cursor; // show cursor or not
    int x_luminosity; // use luminosity or not

    int x_colorR; // setable r
    int x_colorG; // setable g
    int x_colorB; // setable b

    int x_cursX;  // X coordinate of cursor
    int x_cursY;  // Y coordinate of cursor
    short int *x_frame;  // keep a copy of current frame for picking color

    t_outlet *x_pdp_output; // output packets

    t_canvas *x_canvas;

} t_pdp_cmap;

static void pdp_cmap_draw_color(t_pdp_cmap *x, int r, int g, int b)
{
 int width, height;
 char color[32];

    sprintf( color, "#%.2X%.2X%.2X", r, g, b );
    width = rtext_width( glist_findrtext( (t_glist*)x->x_canvas, (t_text *)x ) );
    height = rtext_height( glist_findrtext( (t_glist*)x->x_canvas, (t_text *)x ) );
    sys_vgui(".x%x.c delete rectangle %xCOLOR\n", x->x_canvas, x );
    sys_vgui(".x%x.c create rectangle %d %d %d %d -fill %s -tags %xCOLOR\n",
             x->x_canvas, x->x_obj.te_xpix+width+5, x->x_obj.te_ypix,
             x->x_obj.te_xpix+width+height+5,
             x->x_obj.te_ypix+height, color, x );
}

static void pdp_cmap_r(t_pdp_cmap *x, t_floatarg fr )
{
   if ( ( fr >= 0 ) && ( fr < 255 ) )
   {
      x->x_colorR = (int)fr;
      x->x_colors[x->x_current].y = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
      x->x_colors[x->x_current].u = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      x->x_colors[x->x_current].v = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      if (glist_isvisible(x->x_canvas))  pdp_cmap_draw_color( x, x->x_colorR, x->x_colorG, x->x_colorB );
   }
}

static void pdp_cmap_g(t_pdp_cmap *x, t_floatarg fg )
{
   if ( ( fg >= 0 ) && ( fg < 255 ) )
   {
      x->x_colorG = (int)fg;
      x->x_colors[x->x_current].y = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
      x->x_colors[x->x_current].u = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      x->x_colors[x->x_current].v = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      if (glist_isvisible(x->x_canvas)) pdp_cmap_draw_color( x, x->x_colorR, x->x_colorG, x->x_colorB );
   }
}

static void pdp_cmap_b(t_pdp_cmap *x, t_floatarg fb )
{
   if ( ( fb >= 0 ) && ( fb < 255 ) )
   {
      x->x_colorB = (int)fb;
      x->x_colors[x->x_current].y = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
      x->x_colors[x->x_current].u = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      x->x_colors[x->x_current].v = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      if (glist_isvisible(x->x_canvas)) pdp_cmap_draw_color( x, x->x_colorR, x->x_colorG, x->x_colorB );
   }
}

static void pdp_cmap_cursx(t_pdp_cmap *x, t_floatarg fx )
{
   if ( ( fx >= 0 ) && ( fx < x->x_vwidth) )
   {
      x->x_cursX = (int)fx;
   }
}

static void pdp_cmap_cursy(t_pdp_cmap *x, t_floatarg fy )
{
   if ( ( fy >= 0 ) && ( fy < x->x_vheight) )
   {
      x->x_cursY = (int)fy;
   }
}

static void pdp_cmap_tolerance(t_pdp_cmap *x, t_floatarg ftolerance )
{
   if ( ftolerance >= 0 ) 
   {
      x->x_colors[x->x_current].tolerance = (int)ftolerance;
   }
}

static void pdp_cmap_luminosity(t_pdp_cmap *x, t_floatarg fluminosity )
{
   if ( ( fluminosity == 0 ) || ( fluminosity == 1 ) )
   {
      x->x_luminosity = (int)fluminosity;
   }
}

static void pdp_cmap_delete(t_pdp_cmap *x, t_floatarg fcolor )
{
   if ( ( fcolor >= 0 ) && ( fcolor < x->x_capacity ) )
   {
      x->x_colors[(int)fcolor].on = 0;
   }
}

static void pdp_cmap_clear(t_pdp_cmap *x)
{
  int ci;

   for ( ci=0; ci<x->x_capacity; ci++)
   {
      x->x_colors[ci].on = 0;
   }
   x->x_current = 0;
}

static void pdp_cmap_resize(t_pdp_cmap *x,  t_floatarg fnewsize  )
{
  t_color *colors;
  int ci, csize;

    if ( (int) fnewsize<=0 ) return;

    // allocate new structures
    colors = (t_color*) getbytes( fnewsize*sizeof(t_color) );

    for ( ci=0; ci<fnewsize; ci++ )
    {
       colors[ci].on = 0;
       colors[ci].tolerance = 10;
    }

    if ( fnewsize < x->x_capacity )
    {
       post( "pdp_form : new size is too small : texts lost !!" );
       csize = fnewsize;
    }
    else
    {
       csize = x->x_capacity;
    }

    // copy all values
    for ( ci=0; ci<csize; ci++ )
    {
        memcpy( &colors[ci], &x->x_colors[ci], sizeof( t_color ) );
    }

    // free old structures
    if ( x->x_colors ) freebytes( x->x_colors, x->x_capacity*sizeof(t_color) );

    // set new structures
    x->x_colors = colors;
    x->x_capacity = fnewsize;
    x->x_current = 0;
}

static void pdp_cmap_setcur(t_pdp_cmap *x,  t_floatarg fpx, t_floatarg fpy  )
{
   if ( (fpx>=0.0) && (fpx<=1.0) && (fpy>=0.0) && (fpy<=1.0) )
   {
      x->x_cursX = fpx*(t_float)x->x_vwidth;
      x->x_cursY = fpy*(t_float)x->x_vheight;
   }
}

static void pdp_cmap_current(t_pdp_cmap *x, t_floatarg fcurrent )
{
   if ( ( fcurrent >= 0 ) && ( fcurrent < x->x_capacity ) )
   {
      x->x_current = (int)fcurrent;
      post( "pdp_cmap : color index set to : %d", x->x_current );
   }
}

static void pdp_cmap_cursor(t_pdp_cmap *x, t_floatarg fcursor )
{
   if ( ( fcursor == 0 ) || ( fcursor == 1 ) )
   {
      x->x_cursor = (int)fcursor;
   }
}

static void pdp_cmap_pick(t_pdp_cmap *x)
{
 int y,u,v;

   if ( x->x_frame && ( x->x_cursX > 0 ) && ( x->x_cursX < x->x_vwidth ) 
        && ( x->x_cursY > 0 ) && ( x->x_cursY < x->x_vheight ) )
   {
      x->x_colors[x->x_current].oy = x->x_frame[ x->x_cursY*x->x_vwidth+x->x_cursX ];
      x->x_colors[x->x_current].ov = x->x_frame[ x->x_vsize+((x->x_cursY>>1)*(x->x_vwidth>>1)+(x->x_cursX>>1)) ];
      x->x_colors[x->x_current].ou = x->x_frame[ x->x_vsize+(x->x_vsize>>2)+((x->x_cursY>>1)*(x->x_vwidth>>1)+(x->x_cursX>>1)) ];
      y = (x->x_colors[x->x_current].oy)>>7;
      v = (x->x_colors[x->x_current].ov>>8)+128;
      u = (x->x_colors[x->x_current].ou>>8)+128;
      x->x_colorR = yuv_YUVtoR( y, u, v );
      x->x_colorG = yuv_YUVtoG( y, u, v );
      x->x_colorB = yuv_YUVtoB( y, u, v );
      if (glist_isvisible(x->x_canvas)) pdp_cmap_draw_color( x, x->x_colorR, x->x_colorG, x->x_colorB );
      x->x_colors[x->x_current].y = rand() & 255;
      x->x_colors[x->x_current].u = rand() & 255;
      x->x_colors[x->x_current].v = rand() & 255;
      x->x_colors[x->x_current].on = 1;
   }
}

static void pdp_cmap_allocate(t_pdp_cmap *x)
{
    x->x_frame = (short int *) getbytes ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );

    if ( !x->x_frame )
    {
       post( "pdp_mgrid : severe error : cannot allocate buffer !!! ");
       return;
    }
}

static void pdp_cmap_free_ressources(t_pdp_cmap *x)
{
    if ( x->x_frame ) freebytes ( x->x_frame, ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );
}

static void pdp_cmap_process_yv12(t_pdp_cmap *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    int     i, ci;
    int     px=0, py=0, ppx=0, ppy=0; 
    int     y=0, u=0, v=0;
    short int *pfY, *pfU, *pfV;
    short int *poY, *poU, *poV;
    int     diff;

    /* allocate all ressources */
    if ( ( (int)header->info.image.width != x->x_vwidth ) ||
         ( (int)header->info.image.height != x->x_vheight ) )
    {
        pdp_cmap_free_ressources( x ); 
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_cmap_allocate( x ); 
        post( "pdp_cmap : reallocated buffers" );
    }

    memcpy(x->x_frame, data, (x->x_vsize + (x->x_vsize>>1))<<1 );

    // map colors
    for ( ci=0; ci<x->x_capacity; ci++ )
    {
       if ( x->x_colors[ci].on )
       {
         pfY = data;
         pfV = data+x->x_vsize;
         pfU = data+x->x_vsize+(x->x_vsize>>2);
         poY = x->x_frame;
         poV = x->x_frame+x->x_vsize;
         poU = x->x_frame+x->x_vsize+(x->x_vsize>>2);
         for ( py=0; py<x->x_vheight; py++ )
         {
           for ( px=0; px<x->x_vwidth; px++ )
           {
              y = *poY;
              v = *poV;
              u = *poU;
              
              if ( x->x_luminosity )
              {
                 diff = (abs(y-x->x_colors[ci].oy)>>7)+(abs(u-x->x_colors[ci].ou)>>8)+(abs(v-x->x_colors[ci].ov)>>8);
              }
              else
              {
                 diff = (abs(u-x->x_colors[ci].ou)>>8)+(abs(v-x->x_colors[ci].ov)>>8);
              }
  
              if ( diff <= x->x_colors[ci].tolerance )
              {
                 // change color not luminosity
                 // *pfY = x->x_colors[ci].y;
                 *pfV = ( x->x_colors[ci].v - 128 )<<8;
                 *pfU = ( x->x_colors[ci].u - 128 )<<8;
              } 
  
              pfY++;poY++;
              if ( (px%2==0) && (py%2==0) )
              {
                 pfU++;pfV++;
                 poU++;poV++;
              }
           }
         }
       }
    }

    // draw cursor
    if ( ( x->x_cursX > 0 ) && ( x->x_cursY > 0 ) && ( x->x_cursor ) )
    {
       for ( px=(x->x_cursX-5); px<=(x->x_cursX+5); px++ )
       {
         if ( ( px > 0 ) && ( px < x->x_vwidth ) )
         {
           if ( ((*(data+x->x_cursY*x->x_vwidth+px))>>7) < 128 )   
           {
              *(data+x->x_cursY*x->x_vwidth+px) = 0xff<<7;  
           }
           else
           {
              *(data+x->x_cursY*x->x_vwidth+px) = 0x00<<7;  
           }
         }
       }
       for ( py=(x->x_cursY-5); py<=(x->x_cursY+5); py++ )
       {
         if ( ( py > 0 ) && ( py < x->x_vheight ) )
         {
           if ( ((*(data+py*x->x_vwidth+x->x_cursX))>>7) < 128 )
           {
              *(data+py*x->x_vwidth+x->x_cursX) = 0xff<<7;
           }
           else
           {
              *(data+py*x->x_vwidth+x->x_cursX) = 0x00<<7;
           }
         }
       }
    }

    pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet0);

    return;
}

static void pdp_cmap_process(t_pdp_cmap *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_cmap_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            pdp_cmap_process_yv12(x);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_cmap_process */
	    break;
	    
	}
    }

}

static void pdp_cmap_input_0(t_pdp_cmap *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))  
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        pdp_cmap_process(x);

    }
}

static void pdp_cmap_free(t_pdp_cmap *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
    pdp_cmap_free_ressources( x );
}

t_class *pdp_cmap_class;

void *pdp_cmap_new(void)
{
  int ci;

    t_pdp_cmap *x = (t_pdp_cmap *)pd_new(pdp_cmap_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("current"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cursx"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cursy"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("R"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("G"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("B"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("tolerance"));

    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;

    x->x_cursX = -1;
    x->x_cursY = -1;
    x->x_cursor = 1;
    x->x_luminosity = 1;

    x->x_capacity = DEFAULT_CAPACITY;
    x->x_current = 0;
    x->x_colors = (t_color *) getbytes( x->x_capacity*sizeof(t_color) );
    for ( ci=0; ci<x->x_capacity; ci++)
    {
       x->x_colors[ci].on = 0;
       x->x_colors[ci].tolerance = 10;
    }

    x->x_canvas = canvas_getcurrent();

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_cmap_setup(void)
{
    // post( pdp_cmap_version );
    pdp_cmap_class = class_new(gensym("pdp_cmap"), (t_newmethod)pdp_cmap_new,
    	(t_method)pdp_cmap_free, sizeof(t_pdp_cmap), 0, A_NULL);

    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_r, gensym("R"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_g, gensym("G"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_b, gensym("B"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_cursx, gensym("cursx"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_cursy, gensym("cursy"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_pick, gensym("pick"),  A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_tolerance, gensym("tolerance"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_cursor, gensym("cursor"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_luminosity, gensym("luminosity"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_current, gensym("current"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_clear, gensym("clear"), A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_delete, gensym("delete"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_resize, gensym("resize"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cmap_class, (t_method)pdp_cmap_setcur, gensym("setcur"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);
}

#ifdef __cplusplus
}
#endif
