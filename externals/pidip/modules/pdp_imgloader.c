/*
 *   PiDiP module
 *   Copyright (c) by Yves Degoyon (ydegoyon@free.fr) and Pablo Martin Caedes ( caedes@sindominio.net )
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

/*  This object loads an image from a file and blends it with a video
 *  It uses imlib2 for all graphical operations
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <ctype.h>
#include <Imlib2.h>  // imlib2 is required

#define MAX_ZONES 20

static char   *pdp_imgloader_version = "pdp_imgloader: version 0.2 : image loading object written by ydegoyon@free.fr, improved by caedes@sindominio.net";

typedef struct _triangle
{
  int used;
  int x1, y1;
  int x2, y2;
  int x3, y3;
  t_float a1, b1;
  t_float a2, b2;
  t_float a3, b3;
} t_triangle;

typedef struct pdp_imgloader_struct
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

    int x_xoffset; // x offset of the image
    int x_yoffset; // y offset of the image

        /* imlib data */
    Imlib_Image x_image;
    DATA32     *x_imdata;
    int       x_iwidth;
    int       x_iheight;
    int       x_operation;
    int       b_fit;

    t_float     x_blend;
    int       x_quality;   // quality forces an additional yuv->rgb conversion in yuv mode

    t_triangle  x_hiddenzones[ MAX_ZONES ]; // hide these parts of the image
    unsigned char *x_mask;

} t_pdp_imgloader;

static void draw_rgb_image(t_pdp_imgloader *x);
        /* load an image */
static void pdp_imgloader_load(t_pdp_imgloader *x, t_symbol *filename, t_floatarg fx, t_floatarg fy)
{
  Imlib_Load_Error imliberr;

   // post( "pdp_imgloader : loading : %s", filename->s_name );

   if ( x->x_image != NULL ) 
   {
      imlib_context_set_image(x->x_image);
      imlib_free_image();
   }
   x->x_image = imlib_load_image_with_error_return( filename->s_name, &imliberr );
   if ( imliberr != IMLIB_LOAD_ERROR_NONE )
   {
      post( "pdp_imgloader : severe error : could not load image (err=%d)!!", imliberr );
      x->x_image = NULL;
      return;
   }
   imlib_context_set_image(x->x_image);
   x->x_imdata = imlib_image_get_data();
   x->x_iwidth = imlib_image_get_width();
   x->x_iheight = imlib_image_get_height();
   // post( "pdp_imgloader : loaded : %s (%dx%d)", filename->s_name, x->x_iwidth, x->x_iheight );
   x->x_xoffset = (int) fx;
   x->x_yoffset = (int) fy;

   //if ( fx!= 0.) x->x_xoffset = (int) fx;
   //if ( fy!= 0.) x->x_yoffset = (int) fy;
   DATA8 tablas[768];
   DATA8 *redt,*greent,*bluet;
   redt = &tablas[0];
   greent = &tablas[256];
   bluet = &tablas[512];
}

static void pdp_imgloader_xoffset(t_pdp_imgloader *x, t_floatarg fx )
{
   x->x_xoffset = (int) fx;
}

static void pdp_imgloader_yoffset(t_pdp_imgloader *x, t_floatarg fy )
{
   x->x_yoffset = (int) fy;
}

static void pdp_imgloader_quality(t_pdp_imgloader *x, t_floatarg fqual )
{
   x->x_quality = (int) fqual;
}

static void pdp_imgloader_blend(t_pdp_imgloader *x, t_floatarg fblend )
{
   if ( ( fblend >= 0.0 ) && ( fblend <= 1.0 ) )
   {
     x->x_blend = fblend;
   }
}
static void pdp_imgloader_fit(t_pdp_imgloader *x, t_floatarg ffit )
{
   if ( ( ffit >= 0.0 ) )
   {
     x->b_fit = (int)ffit;
   }
}
static void pdp_imgloader_operation(t_pdp_imgloader *x, t_symbol *s)
{
	if      (s == gensym("copy")) x->x_operation = IMLIB_OP_COPY;
	else if      (s == gensym("add")) x->x_operation = IMLIB_OP_ADD;
	else if      (s == gensym("substract")) x->x_operation = IMLIB_OP_SUBTRACT;
	else if      (s == gensym("reshade")) x->x_operation = IMLIB_OP_RESHADE;
}
static int pdp_imgloader_isinzone(t_pdp_imgloader *x, int px, int py, int index)
{
  int c1=0, c2=0, c3=0;

   if ( !x->x_hiddenzones[index].used )
   {
     return 0;
   }
   if ( px < x->x_hiddenzones[index].x1 )
   {
     return 0;
   }
   if ( px > x->x_hiddenzones[index].x3 )
   {
     return 0;
   }
   if ( py < x->x_hiddenzones[index].y1 && 
        py < x->x_hiddenzones[index].y2 &&
        py < x->x_hiddenzones[index].y3 )
   {
     return 0;
   }
   if ( py > x->x_hiddenzones[index].y1 && 
        py > x->x_hiddenzones[index].y2 &&
        py > x->x_hiddenzones[index].y3 )
   {
     return 0;
   }
   if ( ( ( x->x_hiddenzones[index].x2 - x->x_hiddenzones[index].x1 ) == 0 ) && 
	( px >= x->x_hiddenzones[index].x1 ) )
   {
      c1 = 1;
   }
   else
   {
      if ( x->x_hiddenzones[index].a1 == 0 )
      {
	 if ( ( x->x_hiddenzones[index].y3 >= x->x_hiddenzones[index].y1 ) &&
              ( py >= x->x_hiddenzones[index].y1 ))
	 {
            c1 = 1;
	 }
	 if ( ( x->x_hiddenzones[index].y3 <= x->x_hiddenzones[index].y1 ) &&
              ( py <= x->x_hiddenzones[index].y1 ))
	 {
            c1 = 1;
	 }
      }
      else
      {
         if ( ( ( (float) py ) - ( x->x_hiddenzones[index].a1*((float)px)+ x->x_hiddenzones[index].b1 ) )* 
              ( ( (float) x->x_hiddenzones[index].y3 ) - 
		( x->x_hiddenzones[index].a1*((float)x->x_hiddenzones[index].x3)+ 
		  x->x_hiddenzones[index].b1 ) ) >= 0.0 )
         {
            c1 = 1;
         } 
      } 
   }
   if ( ( ( x->x_hiddenzones[index].x3 - x->x_hiddenzones[index].x2 ) == 0 ) && 
	( px <= x->x_hiddenzones[index].x2 ) )
   {
      c2 = 1;
   }
   else
   {
      if ( x->x_hiddenzones[index].a2 == 0 )
      {
	 if ( ( x->x_hiddenzones[index].y1 >= x->x_hiddenzones[index].y2 ) &&
              ( py >= x->x_hiddenzones[index].y2 ))
	 {
            c2 = 1;
	 }
	 if ( ( x->x_hiddenzones[index].y1 <= x->x_hiddenzones[index].y2 ) &&
              ( py <= x->x_hiddenzones[index].y2 ))
	 {
            c2 = 1;
	 }
      }
      else
      {
         if ( ( ( (float) py ) - ( x->x_hiddenzones[index].a2*((float)px)+ x->x_hiddenzones[index].b2 ) )* 
              ( ( (float) x->x_hiddenzones[index].y1 ) - 
		( x->x_hiddenzones[index].a2*((float)x->x_hiddenzones[index].x1)+ 
		  x->x_hiddenzones[index].b2 ) ) >= 0.0 )
         {
            c2 = 1;
         } 
      } 
   }
   if ( ( ( x->x_hiddenzones[index].x3 - x->x_hiddenzones[index].x1 ) == 0 ) && 
	( px >= x->x_hiddenzones[index].x1 ) )
   {
      c3 = 1;
   }
   else
   {
      if ( x->x_hiddenzones[index].a3 == 0 )
      {
	 if ( ( x->x_hiddenzones[index].y2 >= x->x_hiddenzones[index].y1 ) &&
              ( py >= x->x_hiddenzones[index].y1 ))
	 {
            c3 = 1;
	 }
	 if ( ( x->x_hiddenzones[index].y2 <= x->x_hiddenzones[index].y1 ) &&
              ( py <= x->x_hiddenzones[index].y1 ))
	 {
            c3 = 1;
	 }
      }
      else
      {
         if ( ( ( (float) py ) - ( x->x_hiddenzones[index].a3*((float)px)+ x->x_hiddenzones[index].b3 ) )* 
              ( ( (float) x->x_hiddenzones[index].y2 ) - 
		( x->x_hiddenzones[index].a3*((float)x->x_hiddenzones[index].x2)+ 
		  x->x_hiddenzones[index].b3 ) ) >= 0.0 )
         {
            c3 = 1;
         } 
      } 
   }

   return c1 && c2 && c3;
}

static int pdp_imgloader_ishidden(t_pdp_imgloader *x, int px, int py)
{
  int ti;

    for ( ti=0; ti<MAX_ZONES; ti++ )
    {
       if ( x->x_hiddenzones[ti].used )
       {
          if ( pdp_imgloader_isinzone( x, px, py, ti ) )
	  {
             return 1;
	  }
       }
    }
    return 0;
}

static void pdp_imgloader_update_mask(t_pdp_imgloader *x )
{
  int px, py;

  for ( py=0; py<x->x_vheight; py++ )
  {
    for ( px=0; px<x->x_vwidth; px++ )
    {
      *(x->x_mask+py*x->x_vwidth+px) = (unsigned char) pdp_imgloader_ishidden( x, px, py );
    }
  }
}

static void pdp_imgloader_hide(t_pdp_imgloader *x, t_symbol *s, int argc, t_atom *argv)
{
  int ti;
  t_float fx1, fy1, fx2, fy2, fx3, fy3;

   if ( argc != 6 )
   {
     post( "pdp_imgloader : hide : wrong number of arguments : %d", argc );
     return;
   }
   if ( argv[0].a_type != A_FLOAT || argv[1].a_type != A_FLOAT || argv[2].a_type != A_FLOAT ||
        argv[3].a_type != A_FLOAT || argv[4].a_type != A_FLOAT || argv[5].a_type != A_FLOAT ) {
     post( "pdp_imgloader : hide : wrong arguments" );
     return;
   }

   fx1 = argv[0].a_w.w_float;
   fy1 = argv[1].a_w.w_float;
   fx2 = argv[2].a_w.w_float;
   fy2 = argv[3].a_w.w_float;
   fx3 = argv[4].a_w.w_float;
   fy3 = argv[5].a_w.w_float;

   if ( ( (int)fx1 >= 0 ) && ( (int)fx1 < x->x_vwidth ) && 
        ( (int)fx2 >= 0 ) && ( (int)fx2 < x->x_vwidth ) &&
        ( (int)fx3 >= 0 ) && ( (int)fx3 < x->x_vwidth ) &&
        ( (int)fy1 >= 0 ) && ( (int)fy1 < x->x_vheight ) &&
        ( (int)fy2 >= 0 ) && ( (int)fy2 < x->x_vheight ) &&
        ( (int)fy3 >= 0 ) && ( (int)fy3 < x->x_vheight ) )
   {
     // post( "pdp_imgloader : hide : coordinates : %d %d %d %d %d %d",
     //            (int)fx1, (int)fy1, (int)fx2, (int)fy2, (int)fx3, (int)fy3 );
     for ( ti=0; ti<MAX_ZONES; ti++ )
     {
        if ( !x->x_hiddenzones[ti].used )
	{
           x->x_hiddenzones[ti].used = 1;
	   if ( (int) fx1 < (int) fx2 )
	   {
              if ( (int) fx2 < (int) fx3 )
              {
                x->x_hiddenzones[ti].x1 = (int) fx1;
                x->x_hiddenzones[ti].y1 = (int) fy1;
                x->x_hiddenzones[ti].x2 = (int) fx2;
                x->x_hiddenzones[ti].y2 = (int) fy2;
                x->x_hiddenzones[ti].x3 = (int) fx3;
                x->x_hiddenzones[ti].y3 = (int) fy3;
              }
	      else
              {
                if ( (int) fx3 < (int) fx1 )
                {
                  x->x_hiddenzones[ti].x1 = (int) fx3;
                  x->x_hiddenzones[ti].y1 = (int) fy3;
                  x->x_hiddenzones[ti].x2 = (int) fx1;
                  x->x_hiddenzones[ti].y2 = (int) fy1;
                  x->x_hiddenzones[ti].x3 = (int) fx2;
                  x->x_hiddenzones[ti].y3 = (int) fy2;
                }
                else
                {
                  x->x_hiddenzones[ti].x1 = (int) fx1;
                  x->x_hiddenzones[ti].y1 = (int) fy1;
                  x->x_hiddenzones[ti].x2 = (int) fx3;
                  x->x_hiddenzones[ti].y2 = (int) fy3;
                  x->x_hiddenzones[ti].x3 = (int) fx2;
                  x->x_hiddenzones[ti].y3 = (int) fy2;
                }
              }
	   }
	   else
	   {
              if ( (int) fx2 < (int) fx3 )
              {
                if ( (int) fx3 < (int) fx1 )
                {
                  x->x_hiddenzones[ti].x1 = (int) fx2;
                  x->x_hiddenzones[ti].y1 = (int) fy2;
                  x->x_hiddenzones[ti].x2 = (int) fx3;
                  x->x_hiddenzones[ti].y2 = (int) fy3;
                  x->x_hiddenzones[ti].x3 = (int) fx1;
                  x->x_hiddenzones[ti].y3 = (int) fy1;
                }
                else
                {
                  x->x_hiddenzones[ti].x1 = (int) fx2;
                  x->x_hiddenzones[ti].y1 = (int) fy2;
                  x->x_hiddenzones[ti].x2 = (int) fx1;
                  x->x_hiddenzones[ti].y2 = (int) fy1;
                  x->x_hiddenzones[ti].x3 = (int) fx3;
                  x->x_hiddenzones[ti].y3 = (int) fy3;
                }
              }
	      else
              {
                x->x_hiddenzones[ti].x1 = (int) fx3;
                x->x_hiddenzones[ti].y1 = (int) fy3;
                x->x_hiddenzones[ti].x2 = (int) fx2;
                x->x_hiddenzones[ti].y2 = (int) fy2;
                x->x_hiddenzones[ti].x3 = (int) fx1;
                x->x_hiddenzones[ti].y3 = (int) fy1;
              }
	   }
	   // post( "pdp_imgloader : hiding : [%d,%d]/[%d,%d]/[%d,%d], ", 
           //                 x->x_hiddenzones[ti].x1, x->x_hiddenzones[ti].y1,
           //                 x->x_hiddenzones[ti].x2, x->x_hiddenzones[ti].y2,
           //                 x->x_hiddenzones[ti].x3, x->x_hiddenzones[ti].y3 );
           if ( (x->x_hiddenzones[ti].x2-x->x_hiddenzones[ti].x1) != 0 )
	   {
             x->x_hiddenzones[ti].a1 = 
		    ( ( (float) x->x_hiddenzones[ti].y2 ) - ( (float) x->x_hiddenzones[ti].y1 ) ) /
		    ( ( (float) x->x_hiddenzones[ti].x2 ) - ( (float) x->x_hiddenzones[ti].x1 ) );
             x->x_hiddenzones[ti].b1 = 
		    ( ( (float) x->x_hiddenzones[ti].y1 )*( (float) x->x_hiddenzones[ti].x2 ) - 
		      ( (float) x->x_hiddenzones[ti].y2 )*( (float) x->x_hiddenzones[ti].x1 ) ) /
		    ( ( (float) x->x_hiddenzones[ti].x2 ) - ( (float) x->x_hiddenzones[ti].x1 ) );
	   }
           if ( (x->x_hiddenzones[ti].x2-x->x_hiddenzones[ti].x3) != 0 )
	   {
             x->x_hiddenzones[ti].a2 = 
		    ( ( (float) x->x_hiddenzones[ti].y3 ) - ( (float) x->x_hiddenzones[ti].y2 ) ) /
		    ( ( (float) x->x_hiddenzones[ti].x3 ) - ( (float) x->x_hiddenzones[ti].x2 ) );
             x->x_hiddenzones[ti].b2 = 
		    ( ( (float) x->x_hiddenzones[ti].y3 )*( (float) x->x_hiddenzones[ti].x2 ) - 
		      ( (float) x->x_hiddenzones[ti].y2 )*( (float) x->x_hiddenzones[ti].x3 ) ) /
		    ( ( (float) x->x_hiddenzones[ti].x2 ) - ( (float) x->x_hiddenzones[ti].x3 ) );
	   }
           if ( (x->x_hiddenzones[ti].x3-x->x_hiddenzones[ti].x1) != 0 )
	   {
             x->x_hiddenzones[ti].a3 = 
		    ( ( (float) x->x_hiddenzones[ti].y3 ) - ( (float) x->x_hiddenzones[ti].y1 ) ) /
		    ( ( (float) x->x_hiddenzones[ti].x3 ) - ( (float) x->x_hiddenzones[ti].x1 ) );
             x->x_hiddenzones[ti].b3 = 
		    ( ( (float) x->x_hiddenzones[ti].y1 )*( (float) x->x_hiddenzones[ti].x3 ) - 
		      ( (float) x->x_hiddenzones[ti].y3 )*( (float) x->x_hiddenzones[ti].x1 ) ) /
		    ( ( (float) x->x_hiddenzones[ti].x3 ) - ( (float) x->x_hiddenzones[ti].x1 ) );
	   }
	   // post( "pdp_imgloader : hiding : a1=%f b1=%f",
           //        x->x_hiddenzones[ti].a1, x->x_hiddenzones[ti].b1 );
	   // post( "pdp_imgloader : hiding : a2=%f b2=%f",
	   //        x->x_hiddenzones[ti].a2, x->x_hiddenzones[ti].b2 );
	   // post( "pdp_imgloader : hiding : a3=%f b3=%f",
	   //        x->x_hiddenzones[ti].a3, x->x_hiddenzones[ti].b3 );
	   pdp_imgloader_update_mask( x );
	   return;
	}
	if ( x->x_hiddenzones[ti].used && ( ti == MAX_ZONES-1 ) )
        {
           post( "pdp_imgloader : hidden zones table is full" );
           return;
        }
     }
   }
   else
   {
     post( "pdp_imgloader : hide : wrong coordinates : %d %d %d %d %d %d (width=%d) (height=%d)",
                (int)fx1, (int)fy1, (int)fx2, (int)fy2, (int)fx3, (int)fy3, x->x_vwidth, x->x_vheight );
     return;
   }
}

static void pdp_imgloader_rawhide(t_pdp_imgloader *x, t_symbol *s, int argc, t_atom *argv)
{
   if ( (x->x_vwidth == 0) || ( x->x_vheight == 0 ) )
   {
     post( "pdp_imgloader : rawhide : no video loaded" );
     return;
   }

   argv[0].a_w.w_float *= x->x_vwidth;
   argv[1].a_w.w_float *= x->x_vheight;
   argv[2].a_w.w_float *= x->x_vwidth;
   argv[3].a_w.w_float *= x->x_vheight;
   argv[4].a_w.w_float *= x->x_vwidth;
   argv[5].a_w.w_float *= x->x_vheight;

   // post( "pdp_imgloader : rawhide : coordinates : %f %f %f %f %f %f (width=%d) (height=%d)",
   //       argv[0].a_w.w_float, argv[1].a_w.w_float, argv[2].a_w.w_float,
   //       argv[3].a_w.w_float, argv[4].a_w.w_float, argv[5].a_w.w_float,
   //       x->x_vwidth, x->x_vheight );

   pdp_imgloader_hide( x, s, argc, argv );
}

static void pdp_imgloader_unhide(t_pdp_imgloader *x, t_floatarg findex )
{
  if ( ( (int) findex < 0 ) || ( (int) findex >= MAX_ZONES ) )
  {
     post( "pdp_imgloader : unhide : wrong index" );
     return;
  }
  x->x_hiddenzones[(int)findex].used = 0;
  pdp_imgloader_update_mask( x );
}

static void pdp_imgloader_clear(t_pdp_imgloader *x )
{
   if ( x->x_image != NULL ) 
   {
      imlib_context_set_image(x->x_image);
      imlib_image_put_back_data(x->x_imdata);
      imlib_free_image();
   }
   x->x_image = NULL;
}

static void pdp_imgloader_free_ressources(t_pdp_imgloader *x )
{
    if ( x->x_mask != NULL )
    {
       freebytes( x->x_mask, x->x_vsize );
    }
}

static void pdp_imgloader_allocate(t_pdp_imgloader *x )
{
    x->x_mask = (unsigned char*)getbytes( x->x_vsize );
}

static void draw_rgb_image(t_pdp_imgloader *x)
{
   Imlib_Image im_target = imlib_context_get_image();
   imlib_context_set_operation(x->x_operation);
   
   int i;
   DATA8 redt[256], greent[256], bluet[256], alphat[256];
   Imlib_Color_Modifier colormod = imlib_create_color_modifier();
   imlib_context_set_color_modifier(colormod);
   imlib_context_set_image(x->x_image);
   Imlib_Image image_buf= imlib_clone_image();   //clones for color mod
   imlib_context_set_image(image_buf);
   if ( x->x_blend < 1.0 ) 
   {
     imlib_get_color_modifier_tables( redt, greent, bluet, alphat );
     for ( i=0; i<=255; i++ )
     {
        alphat[i]=alphat[i]*x->x_blend;
     } 
     imlib_set_color_modifier_tables( redt, greent, bluet, alphat );
     imlib_apply_color_modifier();
   }
   imlib_context_set_image(im_target);
   if (x->b_fit)
   {
       imlib_blend_image_onto_image(image_buf,0,0,0,x->x_iwidth,x->x_iheight,x->x_xoffset,x->x_yoffset,
                                    x->x_xoffset+x->x_vwidth,x->x_yoffset+x->x_vheight);
   }
   else
   {
       imlib_blend_image_onto_image(image_buf,0,0,0,x->x_iwidth,x->x_iheight,x->x_xoffset,x->x_yoffset,x->x_iwidth,x->x_iheight);
   }
   imlib_context_set_operation(IMLIB_OP_COPY);
   
   imlib_context_set_image(image_buf);
   imlib_free_image();
   imlib_free_color_modifier();
   imlib_context_set_image(im_target);
}

static void pdp_imgloader_process_yv12(t_pdp_imgloader *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     px, py;
    t_float   alpha, factor;
    unsigned  char y, u, v;
    short int *pY, *pU, *pV;

    if ( ( (int)(header->info.image.width) != x->x_vwidth ) ||
         ( (int)(header->info.image.height) != x->x_vheight ) )
    {
         pdp_imgloader_free_ressources( x );
         x->x_vwidth = header->info.image.width;
         x->x_vheight = header->info.image.height;
         x->x_vsize = x->x_vwidth*x->x_vheight;
         pdp_imgloader_allocate( x );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;
    if (x->x_quality && x->x_image != NULL)
    {
        Imlib_Image newframe;	//only for quality mode
	newframe = imlib_create_image(x->x_vwidth, x->x_vheight);
	imlib_context_set_image(newframe);
	DATA32 *imdata = imlib_image_get_data();
        yuv_Y122RGB( data, imdata, x->x_vwidth, x->x_vheight );
	draw_rgb_image(x);
        yuv_RGB2Y12( imdata, newdata, x->x_vwidth, x->x_vheight );
	imlib_image_put_back_data(imdata);
	imlib_free_image();
    }
    else
    {
        if ( x->x_image != NULL ) imlib_context_set_image(x->x_image);
        memcpy( newdata, data, (x->x_vsize+(x->x_vsize>>1))<<1 );

        pY = newdata;
        pV = newdata+x->x_vsize;
        pU = newdata+x->x_vsize+(x->x_vsize>>2);
        for ( py=0; py<x->x_vheight; py++ )
        {
          for ( px=0; px<x->x_vwidth; px++ )
          {
            if ( ( x->x_image != NULL ) 
                 && (px >= x->x_xoffset) && ( px < x->x_xoffset + x->x_iwidth )
    	     && (py >= x->x_yoffset) && ( py < x->x_yoffset + x->x_iheight ) 
    	     && ( !(*(x->x_mask+py*x->x_vwidth+px)) )
               )
            {
                y = yuv_RGBtoY(x->x_imdata[(py-x->x_yoffset)*x->x_iwidth+(px-x->x_xoffset)]);
                u = yuv_RGBtoU(x->x_imdata[(py-x->x_yoffset)*x->x_iwidth+(px-x->x_xoffset)]);
                v = yuv_RGBtoV(x->x_imdata[(py-x->x_yoffset)*x->x_iwidth+(px-x->x_xoffset)]);
    
    
    	        if ( imlib_image_has_alpha() )
    	        {
    	          alpha = (x->x_imdata[(py-x->x_yoffset)*x->x_iwidth+(px-x->x_xoffset)] >> 24)/255; 
    	        }
    	        else
    	        {
                  alpha = 1.0;
    	        }
                factor = x->x_blend*alpha;
              
                *(pY) = (int)((1-factor)*(*(pY)) + factor*(y<<7));
                if ( (px%2==0) && (py%2==0) )
                {
                  *(pV) = (int)((1-factor)*(*(pV)) + factor*((v-128)<<8));
                  *(pU) = (int)((1-factor)*(*(pU)) + factor*((u-128)<<8));
                }
            }
            pY++;
            if ( (px%2==0) && (py%2==0) )
            {
              pV++;pU++;
            }
          }
        }
    }

    return;
}

static void pdp_imgloader_sendpacket(t_pdp_imgloader *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_imgloader_process(t_pdp_imgloader *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_imgloader_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_imgloader_process_yv12, pdp_imgloader_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;
	  default:
	    /* don't know the type, so dont pdp_imgloader_process */
	    break;
	    
	}
    }

}

static void pdp_imgloader_input_0(t_pdp_imgloader *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
	switch(pdp_packet_header((int)f)->info.image.encoding)
        {
	  case PDP_IMAGE_YV12:
          x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
	  break;
	}
    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_imgloader_process(x);

    }

}

static void pdp_imgloader_free(t_pdp_imgloader *x)
{
  int i;

    pdp_imgloader_free_ressources(x);
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_imgloader_class;

void *pdp_imgloader_new(void)
{
  int ti;

    t_pdp_imgloader *x = (t_pdp_imgloader *)pd_new(pdp_imgloader_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("xoffset"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("yoffset"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("blend"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;
    x->x_image = NULL;

    x->x_blend = 1.0;
    x->x_mask = NULL;
    x->x_quality = 0;
    x->b_fit = 0;

    for ( ti=0; ti<MAX_ZONES; ti++ )
    {
       x->x_hiddenzones[ti].used = 0;
    }

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_imgloader_setup(void)
{

    // post( pdp_imgloader_version );
    pdp_imgloader_class = class_new(gensym("pdp_imgloader"), (t_newmethod)pdp_imgloader_new,
    	(t_method)pdp_imgloader_free, sizeof(t_pdp_imgloader), 0, A_NULL);

    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_input_0, gensym("pdp"),  
                             A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_load, gensym("load"),  A_SYMBOL, A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_clear, gensym("clear"),  A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_xoffset, gensym("xoffset"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_yoffset, gensym("yoffset"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_blend, gensym("blend"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_quality, gensym("quality"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_hide, gensym("hide"),  A_GIMME, A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_rawhide, gensym("rawhide"),  A_GIMME, A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_unhide, gensym("unhide"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_operation, gensym("operation"),  A_SYMBOL, A_NULL);
    class_addmethod(pdp_imgloader_class, (t_method)pdp_imgloader_fit, gensym("fit"),  A_DEFFLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
