/*
 *   Pure Data Packet module.
 *   Copyright (c) by Tom Schouten <pdp@zzz.kotnet.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <dlfcn.h>

#include "pdp.h"

#ifndef _EiC
#include "cv.h"
#endif

#define MAX_COMPONENTS 10

typedef struct pdp_opencv_floodfill_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_outlet1;
    t_atom x_list[5];
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;

    int x_infosok; 

    int x_up;
    int x_lo;
    int x_connectivity;
    int x_color;

    // tracked components
    int x_xcomp[MAX_COMPONENTS];
    int x_ycomp[MAX_COMPONENTS];

    // fill color
    int x_r[MAX_COMPONENTS];
    int x_g[MAX_COMPONENTS];
    int x_b[MAX_COMPONENTS];

    // opencv data
    IplImage* color_img;
    IplImage* gray_img;
    
} t_pdp_opencv_floodfill;



static void pdp_opencv_floodfill_process_rgb(t_pdp_opencv_floodfill *x)
{
  t_pdp     *header = pdp_packet_header(x->x_packet0);
  short int *data   = (short int *)pdp_packet_data(x->x_packet0);
  t_pdp     *newheader = pdp_packet_header(x->x_packet1);
  short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
  int i;
  CvConnectedComp comp;
  int flags = x->x_connectivity + ( 255 << 8 ) + CV_FLOODFILL_FIXED_RANGE;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

      post("pdp_opencv_floodfill :: resizing plugins");
  
      x->x_width = header->info.image.width;
      x->x_height = header->info.image.height;
      x->x_size = x->x_width*x->x_height;
    
      //Destroy cv_images
      cvReleaseImage( &x->color_img );
      cvReleaseImage( &x->gray_img );
   
      //Create cv_images 
      x->color_img = cvCreateImage( cvSize(x->x_width,x->x_height), 8, 3 );
      x->gray_img = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    memcpy( x->color_img->imageData, data, x->x_size*3 );
    
    if ( !x->x_color )
    {
      cvCvtColor(x->color_img, x->gray_img, CV_BGR2GRAY);
    }

    // mark recognized components
    for ( i=0; i<MAX_COMPONENTS; i++ )
    {
       if ( x->x_xcomp[i] != -1 )
       {
         if ( x->x_color )
         {
            CvPoint seed = cvPoint(x->x_xcomp[i],x->x_ycomp[i]);
            CvScalar color = CV_RGB( x->x_r[i], x->x_g[i], x->x_b[i] );
            cvFloodFill( x->color_img, seed, color, CV_RGB( x->x_lo, x->x_lo, x->x_lo ),
                         CV_RGB( x->x_up, x->x_up, x->x_up ), &comp, flags, NULL );
         }
         else
         {
            CvPoint seed = cvPoint(x->x_xcomp[i],x->x_ycomp[i]);
            CvScalar brightness = cvRealScalar((x->x_r[i]*2 + x->x_g[i]*7 + x->x_b[i] + 5)/10);
            cvFloodFill( x->gray_img, seed, brightness, cvRealScalar(x->x_lo),
                         cvRealScalar(x->x_up), &comp, flags, NULL );
         }
         SETFLOAT(&x->x_list[0], i);
         SETFLOAT(&x->x_list[1], comp.rect.x);
         SETFLOAT(&x->x_list[2], comp.rect.y);
         SETFLOAT(&x->x_list[3], comp.rect.width);
         SETFLOAT(&x->x_list[4], comp.rect.height);
         outlet_list( x->x_outlet1, 0, 5, x->x_list );
       }
    }

    if ( !x->x_color )
    {
      cvCvtColor(x->gray_img, x->color_img, CV_GRAY2RGB);
    }

    memcpy( newdata, x->color_img->imageData, x->x_size*3 );
    return;
}


static void pdp_opencv_floodfill_sendpacket(t_pdp_opencv_floodfill *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_floodfill_process(t_pdp_opencv_floodfill *x)
{
  int encoding;
  t_pdp *header = 0;

  /* check if image data packets are compatible */
  if ( (header = pdp_packet_header(x->x_packet0))
  && (PDP_BITMAP == header->type)){
    
  /* pdp_opencv_floodfill_process inputs and write into active inlet */
  switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

   case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_floodfill_process_rgb, (void*)pdp_opencv_floodfill_sendpacket, &x->x_queue_id);
      break;

   default:
      /* don't know the type, so dont pdp_opencv_floodfill_process */
      break;
      
   }
  }

}

static void pdp_opencv_floodfill_input_0(t_pdp_opencv_floodfill *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_floodfill_process(x);
    }
}

static void pdp_opencv_floodfill_free(t_pdp_opencv_floodfill *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage( &x->color_img );
    cvReleaseImage( &x->gray_img );
}

static void pdp_opencv_floodfill_up_diff(t_pdp_opencv_floodfill *x, t_floatarg fupdiff )
{
    if ( ( (int)fupdiff >= 0 ) && ( (int)fupdiff <= 255 ) )
    {
       x->x_up = (int)fupdiff;
    }
}

static void pdp_opencv_floodfill_lo_diff(t_pdp_opencv_floodfill *x, t_floatarg flodiff )
{
    if ( ( (int)flodiff >= 0 ) && ( (int)flodiff <= 255 ) )
    {
       x->x_lo = (int)flodiff;
    }
}

static void pdp_opencv_floodfill_color(t_pdp_opencv_floodfill *x, t_floatarg fcolor )
{
    if ( ( (int)fcolor == 0 ) || ( (int)fcolor == 1 ) )
    {
       x->x_color = (int)fcolor;
    }
}

static void pdp_opencv_floodfill_fillcolor(t_pdp_opencv_floodfill *x, t_floatarg findex, t_floatarg fr, t_floatarg fg, t_floatarg fb )
{
    if ( ( (int)findex <= 0 ) || ( (int)findex > MAX_COMPONENTS ) )
    {
       post( "pdp_opencv_floodfill : wrong color index : %d", (int)findex );
       return;
    }

    if ( ( (int)fr >= 0 ) || ( (int)fr <= 255 ) )
    {
       x->x_r[(int)findex-1] = (int)fr;
    }

    if ( ( (int)fg >= 0 ) || ( (int)fg <= 255 ) )
    {
       x->x_g[(int)findex-1] = (int)fg;
    }

    if ( ( (int)fb >= 0 ) || ( (int)fb <= 255 ) )
    {
       x->x_b[(int)findex-1] = (int)fb;
    }

}

static void pdp_opencv_floodfill_mark(t_pdp_opencv_floodfill *x, t_floatarg fperx, t_floatarg fpery )
{
  int i;
  int inserted;
    
    if ( ( fperx < 0.0 ) || ( fperx > 1.0 ) || ( fpery < 0.0 ) || ( fpery > 1.0 ) )
    {
       return;
    }

    inserted = 0;
    for ( i=0; i<MAX_COMPONENTS; i++)
    {
       if ( x->x_xcomp[i] == -1 )
       {
          x->x_xcomp[i] = (int)(fperx*x->x_width);
          x->x_ycomp[i] = (int)(fpery*x->x_height);
          // post( "pdp_opencv_floodfill : inserted point (%d,%d)", x->x_xcomp[i], x->x_ycomp[i] );
          inserted = 1;
          break;
       }
    }
    if ( !inserted )
    {
       post( "pdp_opencv_floodfill : max components reached" );
    }
}

static void pdp_opencv_floodfill_delete(t_pdp_opencv_floodfill *x, t_floatarg findex )
{
  int i;
    
    if ( ( findex < 1.0 ) || ( findex > 10 ) )
    {
       return;
    }

    x->x_xcomp[(int)findex-1] = -1;
    x->x_ycomp[(int)findex-1] = -1;
}

static void pdp_opencv_floodfill_connectivity(t_pdp_opencv_floodfill *x, t_floatarg fconnectivity )
{
  int i;
    
    if ( ( fconnectivity != 4.0 ) && ( fconnectivity != 8.0 ) )
    {
       return;
    }

    x->x_connectivity = (int)fconnectivity;
}

static void pdp_opencv_floodfill_clear(t_pdp_opencv_floodfill *x )
{
  int i;
    
    for ( i=0; i<MAX_COMPONENTS; i++)
    {
      x->x_xcomp[i] = -1;
      x->x_ycomp[i] = -1;
    }
}

t_class *pdp_opencv_floodfill_class;

void *pdp_opencv_floodfill_new(t_floatarg f)
{
  int i;

    t_pdp_opencv_floodfill *x = (t_pdp_opencv_floodfill *)pd_new(pdp_opencv_floodfill_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("lo_diff"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("up_diff"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet1 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_infosok = 0;

    x->x_lo = 20; 
    x->x_up = 20;
    x->x_connectivity = 4;
    x->x_color = 1;

    for ( i=0; i<MAX_COMPONENTS; i++)
    {
       x->x_xcomp[i] = -1;
       x->x_ycomp[i] = -1;
       x->x_r[i] = rand() & 255;
       x->x_g[i] = rand() & 255;
       x->x_b[i] = rand() & 255;
    }

    x->color_img = cvCreateImage( cvSize(x->x_width,x->x_height), 8, 3 );
    x->gray_img = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_floodfill_setup(void)
{

    post( "    pdp_opencv_floodfill");
    pdp_opencv_floodfill_class = class_new(gensym("pdp_opencv_floodfill"), (t_newmethod)pdp_opencv_floodfill_new,
      (t_method)pdp_opencv_floodfill_free, sizeof(t_pdp_opencv_floodfill), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_floodfill_class, (t_method)pdp_opencv_floodfill_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_floodfill_class, (t_method)pdp_opencv_floodfill_color, gensym("color"),  A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_floodfill_class, (t_method)pdp_opencv_floodfill_fillcolor, gensym("fillcolor"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_floodfill_class, (t_method)pdp_opencv_floodfill_mark, gensym("mark"),  A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_floodfill_class, (t_method)pdp_opencv_floodfill_delete, gensym("delete"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_floodfill_class, (t_method)pdp_opencv_floodfill_connectivity, gensym("connectivity"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_floodfill_class, (t_method)pdp_opencv_floodfill_clear, gensym("clear"), A_NULL);
    class_addmethod(pdp_opencv_floodfill_class, (t_method)pdp_opencv_floodfill_up_diff, gensym("up_diff"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_floodfill_class, (t_method)pdp_opencv_floodfill_lo_diff, gensym("lo_diff"),  A_FLOAT, A_NULL );   

}

#ifdef __cplusplus
}
#endif
