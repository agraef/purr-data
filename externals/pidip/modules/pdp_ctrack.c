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

/*  This object is a color tracker that lets you follow the movement of objects
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

static char   *pdp_ctrack_version = "pdp_ctrack: a color tracker version 0.1 written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_ctrack_struct
{
    t_object x_obj;
    t_float x_f;

    int x_packet0;
    int x_dropped;

    int x_vwidth;
    int x_vheight;
    int x_vsize;
    int x_colorR; // RGB components of tracked color
    int x_colorG;
    int x_colorB;
    int x_colorY; // YUV components of tracked color
    int x_colorU;
    int x_colorV;
    int x_tolerance; // tolerance 
    int x_luminosity; // use luminosity or not
    int x_steady; // steady mode : the zone is searched around the cursor
    int x_cursor; // show cursor or not
    int x_showframe; // show frame or not
    int x_cursX;  // X coordinate of cursor
    int x_cursY;  // Y coordinate of cursor
    short int *x_frame;  // keep a copy of current frame for picking color

    t_outlet *x_pdp_output; // output packets
    t_outlet *x_x1; // output x1 coordinate of block which has been detected
    t_outlet *x_y1; // output y1 coordinate of block which has been detected
    t_outlet *x_x2; // output x2 coordinate of block which has been detected
    t_outlet *x_y2; // output y2 coordinate of block which has been detected
    t_outlet *x_R;  // output R component of selected color
    t_outlet *x_G;  // output G component of selected color
    t_outlet *x_B;  // output B component of selected color

    t_canvas *x_canvas;

} t_pdp_ctrack;

static void pdp_ctrack_draw_color(t_pdp_ctrack *x)
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

static void pdp_ctrack_setcur(t_pdp_ctrack *x,  t_floatarg fpx, t_floatarg fpy  )
{
   if ( (fpx>=0.0) && (fpx<=1.0) && (fpy>=0.0) && (fpy<=1.0) )
   {
      x->x_cursX = fpx*(t_float)x->x_vwidth;
      x->x_cursY = fpy*(t_float)x->x_vheight;
   }
}

static void pdp_ctrack_r(t_pdp_ctrack *x, t_floatarg fr )
{
   if ( ( fr >= 0 ) && ( fr < 255 ) )
   {
      x->x_colorR = (int)fr;
      x->x_colorY = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
      x->x_colorU = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      x->x_colorV = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      if (glist_isvisible(x->x_canvas)) pdp_ctrack_draw_color( x );
      outlet_float( x->x_R, x->x_colorR );
   }
}

static void pdp_ctrack_g(t_pdp_ctrack *x, t_floatarg fg )
{
   if ( ( fg >= 0 ) && ( fg < 255 ) )
   {
      x->x_colorG = (int)fg;
      x->x_colorY = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
      x->x_colorU = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      x->x_colorV = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      if (glist_isvisible(x->x_canvas)) pdp_ctrack_draw_color( x );
      outlet_float( x->x_G, x->x_colorG );
   }
}

static void pdp_ctrack_b(t_pdp_ctrack *x, t_floatarg fb )
{
   if ( ( fb >= 0 ) && ( fb < 255 ) )
   {
      x->x_colorB = (int)fb;
      x->x_colorY = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
      x->x_colorU = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      x->x_colorV = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
      if (glist_isvisible(x->x_canvas)) pdp_ctrack_draw_color( x );
      outlet_float( x->x_B, x->x_colorB );
   }
}

static void pdp_ctrack_cursx(t_pdp_ctrack *x, t_floatarg fx )
{
   if ( ( fx >= 0 ) && ( fx < x->x_vwidth) )
   {
      x->x_cursX = (int)fx;
   }
}

static void pdp_ctrack_tolerance(t_pdp_ctrack *x, t_floatarg ftolerance )
{
   if ( ftolerance >= 0 ) 
   {
      x->x_tolerance = (int)ftolerance;
   }
}

static void pdp_ctrack_cursy(t_pdp_ctrack *x, t_floatarg fy )
{
   if ( ( fy >= 0 ) && ( fy < x->x_vheight) )
   {
      x->x_cursY = (int)fy;
   }
}

static void pdp_ctrack_luminosity(t_pdp_ctrack *x, t_floatarg fluminosity )
{
   if ( ( fluminosity == 0 ) || ( fluminosity == 1 ) )
   {
      x->x_luminosity = (int)fluminosity;
   }
}

static void pdp_ctrack_cursor(t_pdp_ctrack *x, t_floatarg fcursor )
{
   if ( ( fcursor == 0 ) || ( fcursor == 1 ) )
   {
      x->x_cursor = (int)fcursor;
   }
}

static void pdp_ctrack_frame(t_pdp_ctrack *x, t_floatarg fframe )
{
   if ( ( fframe == 0 ) || ( fframe == 1 ) )
   {
      x->x_showframe = (int)fframe;
   }
}

static void pdp_ctrack_steady(t_pdp_ctrack *x, t_floatarg fsteady )
{
   if ( ( fsteady == 0 ) || ( fsteady == 1 ) )
   {
      x->x_steady = (int)fsteady;
   }
}

static void pdp_ctrack_pick(t_pdp_ctrack *x)
{
 int y,u,v;

   if ( x->x_frame && ( x->x_cursX > 0 ) && ( x->x_cursX < x->x_vwidth ) 
        && ( x->x_cursY > 0 ) && ( x->x_cursY < x->x_vheight ) )
   {
      // post( "pdp_ctrack : picking up color : x=%d y=%d", x->x_cursX, x->x_cursY );
      x->x_colorY = x->x_frame[ x->x_cursY*x->x_vwidth+x->x_cursX ];
      x->x_colorV = x->x_frame[ x->x_vsize + (x->x_cursY>>1)*(x->x_vwidth>>1)+(x->x_cursX>>1) ];
      x->x_colorU = x->x_frame[ x->x_vsize + (x->x_vsize>>2) + (x->x_cursY>>1)*(x->x_vwidth>>1)+(x->x_cursX>>1) ];
      y = (x->x_colorY)>>7;
      u = (x->x_colorU>>8)+128;
      v = (x->x_colorV>>8)+128;
      x->x_colorR = yuv_YUVtoR( y, u, v );
      outlet_float( x->x_R, x->x_colorR );
      x->x_colorG = yuv_YUVtoG( y, u, v );
      outlet_float( x->x_G, x->x_colorG );
      x->x_colorB = yuv_YUVtoB( y, u, v );
      outlet_float( x->x_B, x->x_colorB );
      if (glist_isvisible(x->x_canvas)) pdp_ctrack_draw_color( x );
   }
}

static void pdp_ctrack_allocate(t_pdp_ctrack *x)
{
    x->x_frame = (short int *) getbytes ( ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );

    if ( !x->x_frame )
    {
       post( "pdp_mgrid : severe error : cannot allocate buffer !!! ");
       return;
    }
}

static void pdp_ctrack_free_ressources(t_pdp_ctrack *x)
{
    if ( x->x_frame ) freebytes ( x->x_frame, ( x->x_vsize + ( x->x_vsize>>1 ) ) << 1 );
}

static void pdp_ctrack_process_yv12(t_pdp_ctrack *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    int     i, cf;
    int     px=0, py=0, ppx=0, ppy=0, found=0, xcell=0, ycell=0; 
    int     y=0, u=0, v=0;
    int     x1=0, y1=0, x2=0, y2=0;
    int     X1=0, Y1=0, X2=0, Y2=0;
    short int *pfY, *pfU, *pfV;
    int     diff;

    /* allocate all ressources */
    if ( ( (int)header->info.image.width != x->x_vwidth ) ||
         ( (int)header->info.image.height != x->x_vheight ) )
    {
        pdp_ctrack_free_ressources( x ); 
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_ctrack_allocate( x ); 
        post( "pdp_ctrack : reallocated buffers" );
    }

    memcpy(x->x_frame, data, (x->x_vsize + (x->x_vsize>>1))<<1 );

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

    // track color
    pfY = x->x_frame;
    pfV = x->x_frame+x->x_vsize;
    pfU = x->x_frame+x->x_vsize+(x->x_vsize>>2);
    if ( x->x_colorR != -1 )
    {
       for ( py=0; py<x->x_vheight; py++ )
       {
         for ( px=0; px<x->x_vwidth; px++ )
         {
            y = *pfY++;
            v = *pfV;
            u = *pfU;
            
            if ( x->x_luminosity )
            {
               diff = (abs(y-x->x_colorY)>>7)+(abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
            }
            else
            {
               diff = (abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
            }

            if ( diff <= x->x_tolerance )
            {
               x1=x2=px;
               y1=y2=py;
               found=0;

               for (ppy=y1; ppy<x->x_vheight; ppy++)
               {
                 found = 0;
                 for (ppx=x1; ppx<x->x_vwidth; ppx++)
                 {
                   y = x->x_frame[ ppy*x->x_vwidth+ppx ];
                   v = x->x_frame[ x->x_vsize + (((ppy>>1)*(x->x_vwidth>>1))+(ppx>>1)) ];
                   u = x->x_frame[ x->x_vsize + (x->x_vsize>>2) + (((ppy>>1)*(x->x_vwidth>>1))+(ppx>>1)) ];

                   if ( x->x_luminosity )
                   {
                      diff = (abs(y-x->x_colorY)>>7)+(abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
                   }
                   else
                   {
                      diff = (abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
                   }

                   if ( diff <= x->x_tolerance )
                   {
                      if ( ppx > x2 ) x2 = ppx; 
                      y2 = ppy; 
                      found=1;
                   }
                   else 
                   {
                      break;
                   }
                 }
                 if (!found) break; // a whole line without the looked-up color
               }
               // check further extensions
               // to the bottom
               for (ppx=x1; ppx<x2; ppx++)
               {
                 for (ppy=y2; ppy<x->x_vheight; ppy++)
                 {
                   y = x->x_frame[ ppy*x->x_vwidth+ppx ];
                   v = x->x_frame[ x->x_vsize + (((ppy>>1)*(x->x_vwidth>>1))+(ppx>>1)) ];
                   u = x->x_frame[ x->x_vsize + (x->x_vsize>>2) + (((ppy>>1)*(x->x_vwidth>>1))+(ppx>>1)) ];

                   if ( x->x_luminosity )
                   {
                      diff = (abs(y-x->x_colorY)>>7)+(abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
                   }
                   else
                   {
                      diff = (abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
                   }

                   if ( diff <= x->x_tolerance )
                   {
                      y2 = ppy; 
                   }
                   else 
                   {
                      break;
                   }
                 }
               }
               // to the left
               for (ppy=y1; ppy<=y2; ppy++)
               {
                 for (ppx=x1; ppx>0; ppx--)
                 {
                   y = x->x_frame[ ppy*x->x_vwidth+ppx ];
                   v = x->x_frame[ x->x_vsize + (((ppy>>1)*(x->x_vwidth>>1))+(ppx>>1)) ];
                   u = x->x_frame[ x->x_vsize + (x->x_vsize>>2) + (((ppy>>1)*(x->x_vwidth>>1))+(ppx>>1)) ];

                   if ( x->x_luminosity )
                   {
                      diff = (abs(y-x->x_colorY)>>7)+(abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
                   }
                   else
                   {
                      diff = (abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
                   }

                   if ( diff <= x->x_tolerance )
                   {
                      x1 = ppx; 
                   }
                   else 
                   {
                      break;
                   }
                 }
               }
               // to the right
               for (ppy=y1; ppy<=y2; ppy++)
               {
                 for (ppx=x2; ppx<x->x_vwidth; ppx++)
                 {
                   y = x->x_frame[ ppy*x->x_vwidth+ppx ];
                   v = x->x_frame[ x->x_vsize + (((ppy>>1)*(x->x_vwidth>>1))+(ppx>>1)) ];
                   u = x->x_frame[ x->x_vsize + (x->x_vsize>>2) + (((ppy>>1)*(x->x_vwidth>>1))+(ppx>>1)) ];

                   if ( x->x_luminosity )
                   {
                      diff = (abs(y-x->x_colorY)>>7)+(abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
                   }
                   else
                   {
                      diff = (abs(u-x->x_colorU)>>8)+(abs(v-x->x_colorV)>>8);
                   }

                   if ( diff <= x->x_tolerance )
                   {
                      x2 = ppx; 
                   }
                   else 
                   {
                      break;
                   }
                 }
               }

               // select zone
               if ( !x->x_steady )
               {
                  // track the biggest object
                  if ( abs(x2-x1)*abs(y2-y1) > abs(X2-X1)*abs(Y2-Y1) )
                  { 
                     X1=x1; X2=x2; Y1=y1; Y2=y2;
                  }
               }
               else
               {
                  // check if the cursor is in selected zone
                  if ( (x->x_cursX>=x1) && (x->x_cursX<=x2)
                       && (x->x_cursY>=y1) && (x->x_cursY<=y2 ) )
                  {
                     X1=x1; X2=x2; Y1=y1; Y2=y2;
                     // set new cursor position
                     x->x_cursX = ( X1+X2 )/2;
                     x->x_cursY = ( Y1+Y2 )/2;
                  }
               }

               px=x2+1; py=y2;
               // post( "pdp_ctrack : px=%d py=%d", px, py );
            } 

            if ( (px%2==0) && (py%2==0) )
            {
               pfU++;pfV++;
            }
         }
       }
       if ( X1!=0 || X2!=0 || Y1!=0 || Y2!=0 )
       {
         outlet_float( x->x_x1, X1 );
         outlet_float( x->x_y1, Y1 );
         outlet_float( x->x_x2, X2 );
         outlet_float( x->x_y2, Y2 );

         // draw the frame
         if ( x->x_showframe )
         { 
           for (ppy=Y1; ppy<=Y2; ppy++)
           {
             if ( ((*(data+ppy*x->x_vwidth+X1))>>7) < 128 )
             {
                *(data+ppy*x->x_vwidth+X1) = 0xff<<7;
             }
             else
             {
                *(data+ppy*x->x_vwidth+X1) = 0x00<<7;
             }
             if ( ((*(data+ppy*x->x_vwidth+X2))>>7) < 128 )
             {
                *(data+ppy*x->x_vwidth+X2) = 0xff<<7;
             }
             else
             {
                *(data+ppy*x->x_vwidth+X2) = 0x00<<7;
             }
           }
           for (ppx=X1; ppx<=X2; ppx++)
           {
             if ( ((*(data+Y1*x->x_vwidth+ppx))>>7) < 128 )
             {
                *(data+Y1*x->x_vwidth+ppx) = 0xff<<7;
             }
             else
             {
                *(data+Y1*x->x_vwidth+ppx) = 0x00<<7;
             }
             if ( ((*(data+Y2*x->x_vwidth+ppx))>>7) < 128 )
             {
                *(data+Y2*x->x_vwidth+ppx) = 0xff<<7;
             }
             else
             {
                *(data+Y2*x->x_vwidth+ppx) = 0x00<<7;
             }
           }
         }
       }
    }

    pdp_packet_pass_if_valid(x->x_pdp_output, &x->x_packet0);

    return;
}

static void pdp_ctrack_process(t_pdp_ctrack *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_ctrack_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            pdp_ctrack_process_yv12(x);
	    break;

	case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	default:
	    /* don't know the type, so dont pdp_ctrack_process */
	    break;
	    
	}
    }

}

static void pdp_ctrack_input_0(t_pdp_ctrack *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );


    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        pdp_ctrack_process(x);

    }
}

static void pdp_ctrack_free(t_pdp_ctrack *x)
{
  int i;

    pdp_packet_mark_unused(x->x_packet0);
    pdp_ctrack_free_ressources( x );
}

t_class *pdp_ctrack_class;

void *pdp_ctrack_new(void)
{
    int i;

    t_pdp_ctrack *x = (t_pdp_ctrack *)pd_new(pdp_ctrack_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("R"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("G"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("B"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cursx"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("cursy"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("tolerance"));

    x->x_pdp_output = outlet_new(&x->x_obj, &s_anything); 
    x->x_x1 = outlet_new(&x->x_obj, &s_float); 
    x->x_y1 = outlet_new(&x->x_obj, &s_float); 
    x->x_x2 = outlet_new(&x->x_obj, &s_float); 
    x->x_y2 = outlet_new(&x->x_obj, &s_float); 
    x->x_R  = outlet_new(&x->x_obj, &s_float); 
    x->x_G  = outlet_new(&x->x_obj, &s_float); 
    x->x_B  = outlet_new(&x->x_obj, &s_float); 

    x->x_colorR = -1;
    x->x_colorG = -1;
    x->x_colorB = -1;

    x->x_colorY = (yuv_RGBtoY( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB ))<<7;
    x->x_colorU = (yuv_RGBtoU( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;
    x->x_colorV = (yuv_RGBtoV( (x->x_colorR << 16) + (x->x_colorG << 8) + x->x_colorB )-128)<<8;

    x->x_packet0 = -1;

    x->x_cursX = -1;
    x->x_cursY = -1;
    x->x_tolerance = 50;
    x->x_luminosity = 1;
    x->x_steady = 0;
    x->x_cursor = 1;
    x->x_showframe = 1;

    x->x_canvas = canvas_getcurrent();

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_ctrack_setup(void)
{
    // post( pdp_ctrack_version );
    pdp_ctrack_class = class_new(gensym("pdp_ctrack"), (t_newmethod)pdp_ctrack_new,
    	(t_method)pdp_ctrack_free, sizeof(t_pdp_ctrack), 0, A_NULL);

    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_r, gensym("R"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_g, gensym("G"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_b, gensym("B"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_cursx, gensym("cursx"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_cursy, gensym("cursy"),  A_FLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_pick, gensym("pick"),  A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_tolerance, gensym("tolerance"), A_FLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_luminosity, gensym("luminosity"), A_FLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_steady, gensym("steady"), A_FLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_cursor, gensym("cursor"), A_FLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_frame, gensym("frame"), A_FLOAT, A_NULL);
    class_addmethod(pdp_ctrack_class, (t_method)pdp_ctrack_setcur, gensym("setcur"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
