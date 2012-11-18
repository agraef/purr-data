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

#define MAX_MARKERS 500
const int MAX_COUNT = 500;

typedef struct pdp_opencv_hough_circles_struct
{
  t_object x_obj;
  t_float x_f;

  t_outlet *x_outlet0;
  t_outlet *x_outlet1;
  t_atom x_list[4];

  int x_packet0;
  int x_packet1;
  int x_dropped;
  int x_queue_id;

  int x_width;
  int x_height;
  int x_size;

  // The output and temporary images
  IplImage *image, *grey;
  
  int x_threshold;
  int x_threshold2;
  int x_maxcircles;
  double x_mindist;
  double x_resolution;
  int night_mode;
  CvFont font;
  CvMemStorage* x_storage;
  CvSeq* x_circles;
    
} t_pdp_opencv_hough_circles;

static void pdp_opencv_hough_circles_process_rgb(t_pdp_opencv_hough_circles *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    int i,j,k,ucircles;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height) || (!x->image)) 
    {

      post("pdp_opencv_hough_circles :: resizing plugins");
  
      x->x_width = header->info.image.width;
      x->x_height = header->info.image.height;
      x->x_size = x->x_width*x->x_height;
    
      //Destroy cv_images
      cvReleaseImage( &x->image );
      cvReleaseImage( &x->grey );
   
      //Create cv_images 
      x->image = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
      x->grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    memcpy( x->image->imageData, data, x->x_size*3 );
        
    cvCvtColor( x->image, x->grey, CV_RGB2GRAY );

    if( x->night_mode )
        cvZero( x->image );
        
    x->x_storage = cvCreateMemStorage(0);

    cvSmooth( x->grey, x->grey, CV_GAUSSIAN, 9, 9 );
    CvSeq* circles = cvHoughCircles( x->grey, x->x_storage, CV_HOUGH_GRADIENT, x->x_resolution, x->x_mindist, x->x_threshold, x->x_threshold2 );
    ucircles = (circles->total>x->x_maxcircles)?x->x_maxcircles:circles->total;
    for( i = 0; i < ucircles; i++ )
    {
       float* p = (float*)cvGetSeqElem( circles, i );
       char tindex[10];
       cvCircle( x->image, cvPoint(cvRound(p[0]),cvRound(p[1])), 3, CV_RGB(0,255,0), -1, 8, 0 );
       cvCircle( x->image, cvPoint(cvRound(p[0]),cvRound(p[1])), cvRound(p[2]), CV_RGB(255,0,0), 3, 8, 0 );
       SETFLOAT(&x->x_list[0], i);
       SETFLOAT(&x->x_list[1], cvRound(p[0]));
       SETFLOAT(&x->x_list[2], cvRound(p[1]));
       SETFLOAT(&x->x_list[3], cvRound(p[2]));
       outlet_list( x->x_outlet1, 0, 4, x->x_list );
       sprintf( tindex, "%d", i );
       cvPutText( x->image, tindex, cvPoint(cvRound(p[0]),cvRound(p[1])), &x->font, CV_RGB(255,255,255));
    }

    cvReleaseMemStorage( &x->x_storage ); 

    memcpy( newdata, x->image->imageData, x->x_size*3 );
    return;
}

static void pdp_opencv_hough_circles_nightmode(t_pdp_opencv_hough_circles *x, t_floatarg fmode)
{
  if ((fmode==0.0)||(fmode==1.0)) x->night_mode = (int)fmode;
}

static void pdp_opencv_hough_circles_threshold(t_pdp_opencv_hough_circles *x, t_floatarg fthresh)
{
  if (fthresh>0.0) x->x_threshold = (int)fthresh;
}

static void pdp_opencv_hough_circles_threshold2(t_pdp_opencv_hough_circles *x, t_floatarg fthresh)
{
  if (fthresh>0.0) x->x_threshold2 = (int)fthresh;
}

static void pdp_opencv_hough_circles_mindist(t_pdp_opencv_hough_circles *x, t_floatarg fdist)
{
  if (fdist>0.0) x->x_mindist = (double)fdist;
}

static void pdp_opencv_hough_circles_maxcircles(t_pdp_opencv_hough_circles *x, t_floatarg fmaxcircles)
{
  if (fmaxcircles>0.0) x->x_maxcircles = (int)fmaxcircles;
}

static void pdp_opencv_hough_circles_resolution(t_pdp_opencv_hough_circles *x, t_floatarg fresol)
{
  if (fresol>0.0) x->x_resolution = (double)fresol;
}

static void pdp_opencv_hough_circles_sendpacket(t_pdp_opencv_hough_circles *x)
{
  /* release the packet */
  pdp_packet_mark_unused(x->x_packet0);
  x->x_packet0 = -1;

  /* unregister and propagate if valid dest packet */
  pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_hough_circles_process(t_pdp_opencv_hough_circles *x)
{
   int encoding;
   t_pdp *header = 0;
   char *parname;
   unsigned pi;
   int partype;
   float pardefault;
   t_atom plist[2];
   t_atom tlist[2];
   t_atom vlist[2];

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
  && (PDP_BITMAP == header->type)){
    
  /* pdp_opencv_hough_circles_process inputs and write into active inlet */
  switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

  case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_hough_circles_process_rgb, (void*)pdp_opencv_hough_circles_sendpacket, &x->x_queue_id);
      break;

  default:
      /* don't know the type, so dont pdp_opencv_hough_circles_process */
      break;
      
  }
 }

}

static void pdp_opencv_hough_circles_input_0(t_pdp_opencv_hough_circles *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_hough_circles_process(x);
    }
}

static void pdp_opencv_hough_circles_free(t_pdp_opencv_hough_circles *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage( &x->image );
    cvReleaseImage( &x->grey );
}

t_class *pdp_opencv_hough_circles_class;

void *pdp_opencv_hough_circles_new(t_floatarg f)
{
  int i;

  t_pdp_opencv_hough_circles *x = (t_pdp_opencv_hough_circles *)pd_new(pdp_opencv_hough_circles_class);

  x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
  x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);

  x->x_packet0 = -1;
  x->x_packet1 = -1;
  x->x_queue_id = -1;

  x->x_width  = 320;
  x->x_height = 240;
  x->x_size   = x->x_width * x->x_height;

  x->x_threshold = 100;
  x->x_threshold2 = 10;
  x->x_maxcircles = 10;
  x->x_mindist = 30.0;
  x->x_resolution = 1.0;
  x->night_mode = 0;

  // initialize font
  cvInitFont( &x->font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );
    
  x->image = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
  x->grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );

  return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_hough_circles_setup(void)
{

    post( "    pdp_opencv_hough_circles");
    pdp_opencv_hough_circles_class = class_new(gensym("pdp_opencv_hough_circles"), (t_newmethod)pdp_opencv_hough_circles_new,
      (t_method)pdp_opencv_hough_circles_free, sizeof(t_pdp_opencv_hough_circles), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_hough_circles_class, (t_method)pdp_opencv_hough_circles_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_hough_circles_class, (t_method)pdp_opencv_hough_circles_nightmode, gensym("nightmode"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_circles_class, (t_method)pdp_opencv_hough_circles_threshold, gensym("threshold"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_circles_class, (t_method)pdp_opencv_hough_circles_threshold2, gensym("threshold2"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_circles_class, (t_method)pdp_opencv_hough_circles_mindist, gensym("mindist"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_circles_class, (t_method)pdp_opencv_hough_circles_resolution, gensym("resolution"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_circles_class, (t_method)pdp_opencv_hough_circles_maxcircles, gensym("maxcircles"), A_FLOAT, A_NULL );   
}

#ifdef __cplusplus
}
#endif
