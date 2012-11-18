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
#include <ctype.h>

#include "pdp.h"

#ifndef _EiC
#include "cv.h"
#endif

typedef struct pdp_opencv_channels_struct
{
  t_object x_obj;
  t_float x_f;

  t_outlet *x_outlet0;
  t_outlet *x_outlet1;
  t_outlet *x_outlet2;

  int x_packet0;
  int x_packet1;
  int x_packet2;
  int x_packet3;
  int x_dropped;
  int x_queue_id;

  int x_width;
  int x_height;
  int x_size;

  // The output and temporary images
  IplImage *image, *rimage, *gimage, *bimage, *frimage, *fgimage, *fbimage, *zimage;
  
} t_pdp_opencv_channels;

static void pdp_opencv_channels_process_rgb(t_pdp_opencv_channels *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *rheader = pdp_packet_header(x->x_packet1);
    short int *rdata = (short int *)pdp_packet_data(x->x_packet1); 
    t_pdp     *gheader = pdp_packet_header(x->x_packet2);
    short int *gdata = (short int *)pdp_packet_data(x->x_packet2); 
    t_pdp     *bheader = pdp_packet_header(x->x_packet3);
    short int *bdata = (short int *)pdp_packet_data(x->x_packet3); 
    int i,j,k,im;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height) || (!x->image)) 
    {

      post("pdp_opencv_channels :: resizing plugins");
  
      x->x_width = header->info.image.width;
      x->x_height = header->info.image.height;
      x->x_size = x->x_width*x->x_height;
    
      //Destroy cv_images
      cvReleaseImage( &x->image );
      cvReleaseImage( &x->rimage );
      cvReleaseImage( &x->gimage );
      cvReleaseImage( &x->bimage );
      cvReleaseImage( &x->zimage );
      cvReleaseImage( &x->frimage );
      cvReleaseImage( &x->fgimage );
      cvReleaseImage( &x->fbimage );
   
      //Create cv_images 
      x->image = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
      x->rimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
      x->gimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
      x->bimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
      x->zimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
      x->frimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
      x->fgimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
      x->fbimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
    }
    
    rheader->info.image.encoding = header->info.image.encoding;
    rheader->info.image.width = x->x_width;
    rheader->info.image.height = x->x_height;

    gheader->info.image.encoding = header->info.image.encoding;
    gheader->info.image.width = x->x_width;
    gheader->info.image.height = x->x_height;

    bheader->info.image.encoding = header->info.image.encoding;
    bheader->info.image.width = x->x_width;
    bheader->info.image.height = x->x_height;

    memcpy( x->image->imageData, data, x->x_size*3 );
        
    cvSplit( x->image, x->bimage, x->gimage, x->rimage, NULL );
    cvZero( x->zimage );
    cvMerge( x->zimage, x->zimage, x->rimage, NULL, x->frimage );
    cvMerge( x->zimage, x->gimage, x->zimage, NULL, x->fgimage);
    cvMerge( x->bimage, x->zimage, x->zimage, NULL, x->fbimage );

    memcpy( rdata, x->frimage->imageData, x->x_size*3 );
    memcpy( gdata, x->fgimage->imageData, x->x_size*3 );
    memcpy( bdata, x->fbimage->imageData, x->x_size*3 );

    return;
}

static void pdp_opencv_channels_sendpacket(t_pdp_opencv_channels *x)
{
  /* release the packet */
  pdp_packet_mark_unused(x->x_packet0);
  x->x_packet0 = -1;

  /* unregister and propagate if valid dest packet */
  pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
  pdp_packet_pass_if_valid(x->x_outlet1, &x->x_packet2);
  pdp_packet_pass_if_valid(x->x_outlet2, &x->x_packet3);
}

static void pdp_opencv_channels_process(t_pdp_opencv_channels *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
  && (PDP_BITMAP == header->type)){
    
  /* pdp_opencv_channels_process inputs and write into active inlet */
  switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

  case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            x->x_packet2 = pdp_packet_clone_rw(x->x_packet0);
            x->x_packet3 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_channels_process_rgb, (void*)pdp_opencv_channels_sendpacket, &x->x_queue_id);
      break;

  default:
      /* don't know the type, so dont pdp_opencv_channels_process */
      break;
      
  }
    }

}

static void pdp_opencv_channels_input_0(t_pdp_opencv_channels *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_channels_process(x);
    }
}

static void pdp_opencv_channels_free(t_pdp_opencv_channels *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage( &x->image );
    cvReleaseImage( &x->rimage );
    cvReleaseImage( &x->gimage );
    cvReleaseImage( &x->bimage );
    cvReleaseImage( &x->frimage );
    cvReleaseImage( &x->fgimage );
    cvReleaseImage( &x->fbimage );
    cvReleaseImage( &x->zimage );
}

t_class *pdp_opencv_channels_class;

void *pdp_opencv_channels_new(t_floatarg f)
{
  int i;

  t_pdp_opencv_channels *x = (t_pdp_opencv_channels *)pd_new(pdp_opencv_channels_class);

  x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
  x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);
  x->x_outlet2 = outlet_new(&x->x_obj, &s_anything);

  x->x_packet0 = -1;
  x->x_packet1 = -1;
  x->x_packet2 = -1;
  x->x_packet3 = -1;
  x->x_queue_id = -1;

  x->x_width  = 320;
  x->x_height = 240;
  x->x_size   = x->x_width * x->x_height;

  x->image = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
  x->rimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
  x->gimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
  x->bimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
  x->zimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
  x->frimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
  x->fgimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
  x->fbimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );

  return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_channels_setup(void)
{

    post( "    pdp_opencv_channels");
    pdp_opencv_channels_class = class_new(gensym("pdp_opencv_channels"), (t_newmethod)pdp_opencv_channels_new,
      (t_method)pdp_opencv_channels_free, sizeof(t_pdp_opencv_channels), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_channels_class, (t_method)pdp_opencv_channels_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
}

#ifdef __cplusplus
}
#endif
