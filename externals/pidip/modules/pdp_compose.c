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

/*  This object is a video compositor mixing two sources
 *  idea expressed by liz
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

#define COLORHEIGHT 5

static char   *pdp_compose_version = "pdp_compose: a video compositor version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_compose_struct
{
    t_object x_obj;
    t_float x_f;

    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_vwidth;
    int x_vheight;
    int x_vsize;
    int x_colorR; // RGB components of mixing color
    int x_colorG;
    int x_colorB;
    int x_colorY; // YUV components of mixing color
    int x_colorU;
    int x_colorV;
    int x_tolerance; // tolerance 
    int x_cursX;  // X coordinate of cursor
    int x_cursY;  // Y coordinate of cursor
    int x_cursor; // cursor drawing flag
    int x_luminosity; // flag to indicate if luminosity is used
    short int *x_frame;  // keep a copy of current frame for picking color
    short int *x_right_frame;  // 2nd video source

    t_outlet *x_pdp_output; // output packets

    t_canvas *x_canvas;

} t_pdp_compose;

static void pdp_compose_draw_color(t_pdp_compose *x)
{
 int width, height;
 char color[32];

    sprintf( color, "#%.2X%.2X%.2X", x->x_colorR, x->x_colorG, x->x_colorB );
    width = rtext_width( glist_findrtext( (t_glist*)x->x_canvas, (t_text *)x ) );
    height = rtext_height( glist_findrtext( (t_glist*)x->x_canvas, (t_text *)x ) );
    sys_vgui(".x%x.c delete rectangle %xCOLOR\n", x->x_canvas, x );
    sys_vgui(".x%x.c create rectangle %d %d %d %d -fill %s -tags %xCOLOR\n",
             x->x_canvas, x->x_obj.te_xpix+width+5, x->x_obj.te_ypix,
             x->x_obj.te_xpix+width+height+5,
             x->x_obj.te_ypix+height, color, x );
}

static void pdp_compose_setcur(t_pdp_compose *x,  t_floatarg fpx, t_floatarg fpy  )
{
   if ( (fpx>=0.0) && (fpx<=1.0) && (fpy>=0.0) && (fpy<=1.0) )
   {
      x->x_cursX = fpx*(t_float)x->x_vwidth;
      x->x_cursY = fpy*(t_float)x->x_vheight;
   }
}

static void pdp_compose_r(t_pdp_compose *x, t_floatarg fr )
{
   if ( ( fr >= 0 ) && ( fr < 255 ) )
   {
      x->x_colorR = (int)fr;
      x->x_colorY = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
      x->x_colorU = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      x->x_colorV = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      if (glist_isvisible(x->x_canvas)) pdp_compose_draw_color( x );
   }
}

static void pdp_compose_g(t_pdp_compose *x, t_floatarg fg )
{
   if ( ( fg >= 0 ) && ( fg < 255 ) )
   {
      x->x_colorG = (int)fg;
      x->x_colorY = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
      x->x_colorU = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      x->x_colorV = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      if (glist_isvisible(x->x_canvas)) pdp_compose_draw_color( x );
   }
}

static void pdp_compose_b(t_pdp_compose *x, t_floatarg fb )
{
   if ( ( fb >= 0 ) && ( fb < 255 ) )
   {
      x->x_colorB = (int)fb;
      x->x_colorY = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
      x->x_colorU = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      x->x_colorV = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      if (glist_isvisible(x->x_canvas)) pdp_compose_draw_color( x );
   }
}

static void pdp_compose_cursor(t_pdp_compose *x, t_floatarg fcursor )
{
   if ( ( (int)fcursor == 0 ) || ( (int)fcursor == 1 ) )
   {
      x->x_cursor = (int)fcursor;
   }
}

static void pdp_compose_tolerance(t_pdp_compose *x, t_floatarg ftolerance )
{
   if ( ftolerance >= 0 ) 
   {
      x->x_tolerance = (int)ftolerance;
   }
}

static void pdp_compose_cursx(t_pdp_compose *x, t_floatarg fx )
{
   if ( ( fx >= 0 ) && ( fx < x->x_vwidth) )
   {
      x->x_cursX = (int)fx;
   }
}

static void pdp_compose_cursy(t_pdp_compose *x, t_floatarg fy )
{
   if ( ( fy >= 0 ) && ( fy < x->x_vheight) )
   {
      x->x_cursY = (int)fy;
   }
}

static void pdp_compose_luminosity(t_pdp_compose *x, t_floatarg fluminosity )
{
   if ( ( fluminosity == 0 ) || ( fluminosity == 1 ) )
   {
      x->x_luminosity = (int)fluminosity;
   }
}

static void pdp_compose_pick(t_pdp_compose *x)
{
 int y,u,v;

   if ( x->x_frame && ( x->x_cursX > 0 ) && ( x->x_cursX < x->x_vwidth ) 
        && ( x->x_cursY > 0 ) && ( x->x_cursY < x->x_vheight ) )
   {
      x->x_colorY = x->x_frame[ x->x_cursY*x->x_vwidth+x->x_cursX ];
      x->x_colorV = x->x_frame[ x->x_vsize + (x->x_cursY>>1)*(x->x_vwidth>>1)+(x->x_cursX>>1) ];
      x->x_colorU = x->x_frame[ x->x_vsize + (x->x_vsize>>2) + (x->x_cursY>>1)*(x->x_vwidth>>1)+(x->x_cursX>>1) ];
      y = (x->x_colorY)>>7;
      u = (x->x_colorU>>8)+128;
      v = (x->x_colorV>>8)+128;
      x->x_colorR = yuv_YUVtoR( y, u, v );
      x->x_colorG = yuv_YUVtoG( y, u, v );
      x->x_colorB = yuv_YUVtoB( y, u, v );
      if (glist_isvisible(x->x_canvas)) pdp_compose_draw_color( x );
   }
}

static void pdp_compose_allocate(t_pdp_compose *x)
{
    x->x_frame = (short int *) getbytes ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );
    x->x_right_frame = (short int *) getbytes ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );

    if ( !x->x_frame || !x->x_right_frame )
    {
       post( "pdp_mgrid : severe error : cannot allocate buffer !!! ");
       return;
    }
}

static void pdp_compose_free_ressources(t_pdp_compose *x)
{
    if ( x->x_frame ) freebytes ( x->x_frame, ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );
    if ( x->x_right_frame ) freebytes ( x->x_right_frame, ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );
}

static void pdp_compose_process_yv12(t_pdp_compose *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata   = (short int *)pdp_packet_data(x->x_packet1);
    int     i, cf;
    int     px=0, py=0, ppx=0, ppy=0, found=0, xcell=0, ycell=0; 
    int     celldiff=0, cellwidth=0, cellheight=0;
    int     y=0, u=0, v=0;
    int     sum;
    short int *pfY, *pfV, *pfU, *prY, *prV, *prU, *pdY, *pdV, *pdU;

    /* allocate all ressources */
    if ( ( (int)header->info.image.width != x->x_vwidth ) ||
         ( (int)header->info.image.height != x->x_vheight ) )
    {
        pdp_compose_free_ressources( x ); 
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_compose_allocate( x ); 
        post( "pdp_compose : reallocated buffers" );
    }

    memcpy(x->x_frame, data, (x->x_vsize + (x->x_vsize>>1))<<1 );

    // draw cursor
    if ( ( x->x_cursor ) && ( x->x_cursX > 0 ) && ( x->x_cursY > 0 ) )
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

    pfY = x->x_frame;
    pfV = x->x_frame+x->x_vsize;
    pfU = x->x_frame+x->x_vsize+(x->x_vsize>>2);
    pdY = newdata;
    pdV = newdata+x->x_vsize;
    pdU = newdata+x->x_vsize+(x->x_vsize>>2);
    prY = x->x_right_frame;
    prV = x->x_right_frame+x->x_vsize;
    prU = x->x_right_frame+x->x_vsize+(x->x_vsize>>2);
    // track color
    if ( x->x_colorR != -1 )
    {
       for ( py=0; py<x->x_vheight; py++ )
       {
         for ( px=0; px<x->x_vwidth; px++ )
         {
            y = *pfY;
            v = *pfV;
            u = *pfU;
            if ( x->x_luminosity )
            {
               sum = (abs(y-x->x_colorY)>>7);
            }
            else
            {
               sum = (abs(y-x->x_colorY)>>7)+(abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
            }
            if ( sum <= x->x_tolerance )
            {
               *pdY = *prY;
               *pdV = *prV;
               *pdU = *prU;
            }
            prY++;pdY++;pfY++;
            if ( (px%2==0) && (py%2==0) )
            {
               prU++; prV++;
               pfU++; pfV++;
               pdU++; pdV++;
            }
         }
       }
    }

    return;
}

static void pdp_compose_sendpacket(t_pdp_compose *x)
{
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0=-1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet1);
}

static void pdp_compose_process(t_pdp_compose *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_compose_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_compose_process_yv12, pdp_compose_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_compose_process */
	    break;
	    
	}
    }

}

static void pdp_compose_input_1(t_pdp_compose *x, t_symbol *s, t_floatarg f)
{
  short int *rightdata   = (short int *)pdp_packet_data((int)f);

    if ( s== gensym("register_rw") )  
    {
      memcpy(x->x_right_frame, rightdata, (x->x_vsize + (x->x_vsize>>1))<<1 );
    }

    pdp_packet_mark_unused( (int)f );
}

static void pdp_compose_input_0(t_pdp_compose *x, t_symbol *s, t_floatarg f)
{
    if ( s== gensym("register_rw") )
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        pdp_compose_process(x);
    }
}

static void pdp_compose_free(t_pdp_compose *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet1);
    pdp_compose_free_ressources( x );
}

t_class *pdp_compose_class;

void *pdp_compose_new(void)
{
    int i;

    t_pdp_compose *x = (t_pdp_compose *)pd_new(pdp_compose_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("pdp"), gensym("pdp1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("R"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("G"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("B"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cursx"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cursy"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("tolerance"));

    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything); 

    x->x_colorR = -1;
    x->x_colorG = -1;
    x->x_colorB = -1;

    x->x_colorY = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
    x->x_colorU = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
    x->x_colorV = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_cursX = -1;
    x->x_cursY = -1;
    x->x_tolerance = 20;
    x->x_luminosity = 1;
    x->x_cursor = 1;

    x->x_canvas = canvas_getcurrent();

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_compose_setup(void)
{
    // post( pdp_compose_version );
    pdp_compose_class = class_new(gensym("pdp_compose"), (t_newmethod)pdp_compose_new,
    	(t_method)pdp_compose_free, sizeof(t_pdp_compose), 0, A_NULL);

    class_addmethod(pdp_compose_class, (t_method)pdp_compose_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_input_1, gensym("pdp1"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_r, gensym("R"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_g, gensym("G"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_b, gensym("B"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_cursx, gensym("cursx"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_cursy, gensym("cursy"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_pick, gensym("pick"), A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_cursor, gensym("cursor"), A_FLOAT, A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_tolerance, gensym("tolerance"), A_FLOAT, A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_luminosity, gensym("luminosity"), A_FLOAT, A_NULL);
    class_addmethod(pdp_compose_class, (t_method)pdp_compose_setcur, gensym("setcur"), A_FLOAT, A_FLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
