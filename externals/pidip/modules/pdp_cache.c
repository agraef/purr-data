/*
 *   PiDiP module
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

/*  This object hides tringular zones from a video stream
 *  It is useful to make some collages ( see collage.pd patch )
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include <ctype.h>

#define MAX_ZONES 20

static char   *pdp_cache_version = "pdp_cache: version 0.1 : video cache object written by ydegoyon@free.fr ";

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

typedef struct pdp_cache_struct
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

    t_triangle  x_hiddenzones[ MAX_ZONES ]; // hide these parts of the image
    unsigned char *x_mask;

} t_pdp_cache;

static int pdp_cache_isinzone(t_pdp_cache *x, int px, int py, int index)
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

static int pdp_cache_ishidden(t_pdp_cache *x, int px, int py)
{
  int ti;

    for ( ti=0; ti<MAX_ZONES; ti++ )
    {
       if ( x->x_hiddenzones[ti].used )
       {
          if ( pdp_cache_isinzone( x, px, py, ti ) )
	  {
             return 1;
	  }
       }
    }
    return 0;
}

static void pdp_cache_free_ressources(t_pdp_cache *x )
{
   if ( x->x_mask != NULL )
   {
     freebytes( x->x_mask, x->x_vsize );
   }
}

static void pdp_cache_allocate(t_pdp_cache *x )
{
   x->x_mask = (unsigned char*)getbytes( x->x_vsize );
}

static void pdp_cache_update_mask(t_pdp_cache *x )
{
  int px, py;

   for ( py=0; py<x->x_vheight; py++ )
   {
     for ( px=0; px<x->x_vwidth; px++ )
     {
       *(x->x_mask+py*x->x_vwidth+px) = (unsigned char) pdp_cache_ishidden( x, px, py );
     }
   }
}

static void pdp_cache_hide(t_pdp_cache *x, t_symbol *s, int argc, t_atom *argv)
{
  int ti;
  t_float fx1, fy1, fx2, fy2, fx3, fy3;

   if ( argc != 6 )
   {
     post( "pdp_cache : hide : wrong number of arguments : %d", argc );
     return;
   }
   if ( argv[0].a_type != A_FLOAT || argv[1].a_type != A_FLOAT || argv[2].a_type != A_FLOAT ||
        argv[3].a_type != A_FLOAT || argv[4].a_type != A_FLOAT || argv[5].a_type != A_FLOAT ) {
     post( "pdp_cache : hide : wrong arguments" );
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
     post( "pdp_cache : hide : coordinates : %d %d %d %d %d %d",
                (int)fx1, (int)fy1, (int)fx2, (int)fy2, (int)fx3, (int)fy3 );
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
	   post( "pdp_cache : hiding : [%d,%d]/[%d,%d]/[%d,%d], ", 
                           x->x_hiddenzones[ti].x1, x->x_hiddenzones[ti].y1,
                           x->x_hiddenzones[ti].x2, x->x_hiddenzones[ti].y2,
                           x->x_hiddenzones[ti].x3, x->x_hiddenzones[ti].y3 );
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
	   post( "pdp_cache : hiding : a1=%f b1=%f",
			   x->x_hiddenzones[ti].a1, x->x_hiddenzones[ti].b1 );
	   post( "pdp_cache : hiding : a2=%f b2=%f",
			   x->x_hiddenzones[ti].a2, x->x_hiddenzones[ti].b2 );
	   post( "pdp_cache : hiding : a3=%f b3=%f",
			   x->x_hiddenzones[ti].a3, x->x_hiddenzones[ti].b3 );
	   pdp_cache_update_mask(x);
	   return;
	}
	if ( x->x_hiddenzones[ti].used && ( ti == MAX_ZONES-1 ) )
        {
           post( "pdp_cache : hidden zones table is full" );
           return;
        }
     }
   }
   else
   {
     post( "pdp_cache : hide : wrong coordinates : %d %d %d %d %d %d (width=%d) (height=%d)",
                (int)fx1, (int)fy1, (int)fx2, (int)fy2, (int)fx3, (int)fy3, x->x_vwidth, x->x_vheight );
     return;
   }
}


static void pdp_cache_rawhide(t_pdp_cache *x, t_symbol *s, int argc, t_atom *argv)
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

   pdp_cache_hide( x, s, argc, argv );
}

static void pdp_cache_unhide(t_pdp_cache *x, t_floatarg findex )
{
  if ( ( (int) findex < 0 ) || ( (int) findex >= MAX_ZONES ) )
  {
     post( "pdp_cache : unhide : wrong index" );
     return;
  }
  x->x_hiddenzones[(int)findex].used = 0;
  pdp_cache_update_mask(x);
}

static void pdp_cache_process_yv12(t_pdp_cache *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
    int     px, py;
    t_float   alpha, factor;
    unsigned  char y, u, v;
    short int *pY, *pU, *pV;
    short int *poY, *poU, *poV;

    if ( ( (int)(header->info.image.width) != x->x_vwidth ) ||
         ( (int)(header->info.image.height) != x->x_vheight ) )
    {
         pdp_cache_free_ressources(x);
         x->x_vwidth = header->info.image.width;
         x->x_vheight = header->info.image.height;
         x->x_vsize = x->x_vwidth*x->x_vheight;
         pdp_cache_allocate(x);
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    poY = data;
    poV = data+x->x_vsize;
    poU = data+x->x_vsize+(x->x_vsize>>2);
    pY = newdata;
    pV = newdata+x->x_vsize;
    pU = newdata+x->x_vsize+(x->x_vsize>>2);
    for ( py=0; py<x->x_vheight; py++ )
    {
      for ( px=0; px<x->x_vwidth; px++ )
      {
        if ( !(*(x->x_mask+py*x->x_vwidth+px)) )
        {
           *(pY) = *(poY);
           if ( (px%2==0) && (py%2==0) )
           {
             *(pV) = *(poV);
             *(pU) = *(poU);
           }
        }
	else
        {
           *(pY) = 0x00;
           if ( (px%2==0) && (py%2==0) )
           {
             *(pV) = 0x00;
             *(pU) = 0x00;
           }
        }
        pY++; poY++;
        if ( (px%2==0) && (py%2==0) )
        {
          pV++;pU++;
          poV++;poU++;
        }
      }
    }

    return;
}

static void pdp_cache_sendpacket(t_pdp_cache *x)
{
    /* delete source packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_cache_process(t_pdp_cache *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type)){
    
	/* pdp_cache_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding)
        {

	  case PDP_IMAGE_YV12:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_cache_process_yv12, pdp_cache_sendpacket, &x->x_queue_id);
	    break;

	  case PDP_IMAGE_GREY:
            // should write something to handle these one day
            // but i don't use this mode                      
	    break;

	  default:
	    /* don't know the type, so dont pdp_cache_process */
	    break;
	    
	}
    }

}

static void pdp_cache_input_0(t_pdp_cache *x, t_symbol *s, t_floatarg f)
{

    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_cache_process(x);

    }

}

static void pdp_cache_free(t_pdp_cache *x)
{
  int i;

    pdp_cache_free_ressources(x);
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
}

t_class *pdp_cache_class;

void *pdp_cache_new(void)
{
  int ti;

    t_pdp_cache *x = (t_pdp_cache *)pd_new(pdp_cache_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    for ( ti=0; ti<MAX_ZONES; ti++ )
    {
       x->x_hiddenzones[ti].used = 0;
    }

    x->x_mask = NULL;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_cache_setup(void)
{

    // post( pdp_cache_version );
    pdp_cache_class = class_new(gensym("pdp_cache"), (t_newmethod)pdp_cache_new,
    	(t_method)pdp_cache_free, sizeof(t_pdp_cache), 0, A_NULL);

    class_addmethod(pdp_cache_class, (t_method)pdp_cache_input_0, gensym("pdp"),  
                             A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_cache_class, (t_method)pdp_cache_hide, gensym("hide"),  A_GIMME, A_NULL);
    class_addmethod(pdp_cache_class, (t_method)pdp_cache_rawhide, gensym("rawhide"),  A_GIMME, A_NULL);
    class_addmethod(pdp_cache_class, (t_method)pdp_cache_unhide, gensym("unhide"),  A_DEFFLOAT, A_NULL);


}

#ifdef __cplusplus
}
#endif
