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

typedef struct pdp_opencv_hough_lines_struct
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

  // The output and temporary images
  IplImage *image, *grey;
  
  int x_mode;
  int x_threshold;
  int x_maxlines;
  double x_minlength;
  double x_gap;
  double x_aresolution;
  double x_dresolution;
  int night_mode;
  CvFont font;
  CvMemStorage* x_storage;
  CvSeq* x_lines;
    
} t_pdp_opencv_hough_lines;

static void pdp_opencv_hough_lines_process_rgb(t_pdp_opencv_hough_lines *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    int i,j,k,ulines;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height) || (!x->image)) 
    {

      post("pdp_opencv_hough_lines :: resizing plugins");
  
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
    // hard-coded cvCanny, it's what works
    cvCanny( x->grey, x->grey, 50, 200, 3 );

    if( x->night_mode )
        cvZero( x->image );
        
    x->x_storage = cvCreateMemStorage(0);

    switch( x->x_mode )
    {

       case CV_HOUGH_STANDARD:

        x->x_lines = cvHoughLines2( x->grey, x->x_storage, x->x_mode, 1, CV_PI/180, x->x_threshold, 0, 0 );

        if ( x->x_lines )
        {
          ulines = ( x->x_lines->total >= x->x_maxlines ) ? x->x_maxlines:x->x_lines->total;
          // post( "pdp_opencv_hough_lines : found %d lines, shown : %d", x->x_lines->total, ulines );
          for( i=0; i<ulines; i++ )
          {
            float* line = (float*)cvGetSeqElem(x->x_lines,i);
            float rho = line[0];
            float theta = line[1];
            CvPoint pt1, pt2;
            char tindex[10];
            double a = cos(theta), b = sin(theta);
            double x0 = a*rho, y0 = b*rho;
            pt1.x = cvRound(x0 + 1000*(-b));
            pt1.y = cvRound(y0 + 1000*(a));
            pt2.x = cvRound(x0 - 1000*(-b));
            pt2.y = cvRound(y0 - 1000*(a));
            cvLine( x->image, pt1, pt2, CV_RGB(255,0,0), 3, 8 );
            SETFLOAT(&x->x_list[0], i);
            SETFLOAT(&x->x_list[1], pt1.x);
            SETFLOAT(&x->x_list[2], pt1.y);
            SETFLOAT(&x->x_list[3], pt2.x);
            SETFLOAT(&x->x_list[4], pt2.y);
            outlet_list( x->x_outlet1, 0, 5, x->x_list );
            pt1.x = (pt1.x+pt2.x)/2;
            pt1.y = (pt1.y+pt2.y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( x->image, tindex, pt1, &x->font, CV_RGB(255,255,255));
          }
        }
        break;

       case CV_HOUGH_PROBABILISTIC:

        x->x_lines = cvHoughLines2( x->grey, x->x_storage, x->x_mode, 1, CV_PI/180, x->x_threshold, x->x_minlength, x->x_gap );

        if ( x->x_lines )
        {
          ulines = ( x->x_lines->total >= x->x_maxlines ) ? x->x_maxlines:x->x_lines->total;
          // post( "pdp_opencv_hough_lines : found %d lines, shown : %d", x->x_lines->total, ulines );
          for( i=0; i<ulines; i++ )
          {
            CvPoint* line = (CvPoint*)cvGetSeqElem(x->x_lines,i);
            char tindex[10];
            cvLine( x->image, line[0], line[1], CV_RGB(255,0,0), 3, 8 );
            SETFLOAT(&x->x_list[0], i);
            SETFLOAT(&x->x_list[1], line[0].x);
            SETFLOAT(&x->x_list[2], line[0].y);
            SETFLOAT(&x->x_list[3], line[1].x);
            SETFLOAT(&x->x_list[4], line[1].y);
            outlet_list( x->x_outlet1, 0, 5, x->x_list );
            line[0].x = (line[0].x+line[1].x)/2;
            line[0].y = (line[0].y+line[1].y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( x->image, tindex, line[0], &x->font, CV_RGB(255,255,255));
          }
        }
        break;

       case CV_HOUGH_MULTI_SCALE:

        x->x_lines = cvHoughLines2( x->grey, x->x_storage, x->x_mode, 1, CV_PI/180, x->x_threshold, x->x_dresolution, x->x_aresolution );

        if ( x->x_lines )
        {
          ulines = ( x->x_lines->total >= x->x_maxlines ) ? x->x_maxlines:x->x_lines->total;
          // post( "pdp_opencv_hough_lines : found %d lines, shown : %d", x->x_lines->total, ulines );
          for( i = 0; i < ulines; i++ )
          {
            float* line = (float*)cvGetSeqElem(x->x_lines,i);
            float rho = line[0];
            float theta = line[1];
            char tindex[10];
            CvPoint pt1, pt2;
            double a = cos(theta), b = sin(theta);
            double x0 = a*rho, y0 = b*rho;
            pt1.x = cvRound(x0 + 1000*(-b));
            pt1.y = cvRound(y0 + 1000*(a));
            pt2.x = cvRound(x0 - 1000*(-b));
            pt2.y = cvRound(y0 - 1000*(a));
            cvLine( x->image, pt1, pt2, CV_RGB(255,0,0), 3, 8 );
            SETFLOAT(&x->x_list[0], i);
            SETFLOAT(&x->x_list[1], pt1.x);
            SETFLOAT(&x->x_list[2], pt1.y);
            SETFLOAT(&x->x_list[3], pt2.x);
            SETFLOAT(&x->x_list[4], pt2.y);
            outlet_list( x->x_outlet1, 0, 5, x->x_list );
            pt1.x = (pt1.x+pt2.x)/2;
            pt1.y = (pt1.y+pt2.y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( x->image, tindex, pt1, &x->font, CV_RGB(255,255,255));
          }
        }
        break;
    }

    cvReleaseMemStorage( &x->x_storage ); 

    memcpy( newdata, x->image->imageData, x->x_size*3 );
    return;
}

static void pdp_opencv_hough_lines_nightmode(t_pdp_opencv_hough_lines *x, t_floatarg fmode)
{
  if ((fmode==0.0)||(fmode==1.0)) x->night_mode = (int)fmode;
}

static void pdp_opencv_hough_lines_threshold(t_pdp_opencv_hough_lines *x, t_floatarg fthresh)
{
  if (fthresh>0.0) x->x_threshold = (int)fthresh;
}

static void pdp_opencv_hough_lines_minlength(t_pdp_opencv_hough_lines *x, t_floatarg flength)
{
  if (flength>0.0) x->x_minlength = (double)flength;
}

static void pdp_opencv_hough_lines_maxlines(t_pdp_opencv_hough_lines *x, t_floatarg fmaxlines)
{
  if (fmaxlines>0.0) x->x_maxlines = (int)fmaxlines;
}

static void pdp_opencv_hough_lines_gap(t_pdp_opencv_hough_lines *x, t_floatarg fgap)
{
  if (fgap>0.0) x->x_gap = (double)fgap;
}

static void pdp_opencv_hough_lines_aresolution(t_pdp_opencv_hough_lines *x, t_floatarg fresol)
{
  if (fresol>0.0) x->x_aresolution = (double)fresol;
}

static void pdp_opencv_hough_lines_dresolution(t_pdp_opencv_hough_lines *x, t_floatarg fresol)
{
  if (fresol>0.0) x->x_dresolution = (double)fresol;
}

static void pdp_opencv_hough_lines_mode(t_pdp_opencv_hough_lines *x, t_floatarg fmode )
{
    if ( fmode == CV_HOUGH_STANDARD )
    {
       x->x_mode = CV_HOUGH_STANDARD;
    }
    if ( fmode == CV_HOUGH_PROBABILISTIC )
    {
       x->x_mode = CV_HOUGH_PROBABILISTIC;
    }
    if ( fmode == CV_HOUGH_MULTI_SCALE )
    {
       x->x_mode = CV_HOUGH_MULTI_SCALE;
    }
}

static void pdp_opencv_hough_lines_sendpacket(t_pdp_opencv_hough_lines *x)
{
  /* release the packet */
  pdp_packet_mark_unused(x->x_packet0);
  x->x_packet0 = -1;

  /* unregister and propagate if valid dest packet */
  pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_hough_lines_process(t_pdp_opencv_hough_lines *x)
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
    
  /* pdp_opencv_hough_lines_process inputs and write into active inlet */
  switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

  case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_hough_lines_process_rgb, (void*)pdp_opencv_hough_lines_sendpacket, &x->x_queue_id);
      break;

  default:
      /* don't know the type, so dont pdp_opencv_hough_lines_process */
      break;
      
  }
    }

}

static void pdp_opencv_hough_lines_input_0(t_pdp_opencv_hough_lines *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_hough_lines_process(x);
    }
}

static void pdp_opencv_hough_lines_free(t_pdp_opencv_hough_lines *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage( &x->image );
    cvReleaseImage( &x->grey );
}

t_class *pdp_opencv_hough_lines_class;

void *pdp_opencv_hough_lines_new(t_floatarg f)
{
  int i;

  t_pdp_opencv_hough_lines *x = (t_pdp_opencv_hough_lines *)pd_new(pdp_opencv_hough_lines_class);

  x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
  x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);

  x->x_packet0 = -1;
  x->x_packet1 = -1;
  x->x_queue_id = -1;

  x->x_width  = 320;
  x->x_height = 240;
  x->x_size   = x->x_width * x->x_height;

  x->x_mode = CV_HOUGH_PROBABILISTIC;
  x->x_threshold = 50;
  x->x_maxlines = 10;
  x->x_minlength = 30.0;
  x->x_gap = 10.0;
  x->x_aresolution = 10.0;
  x->x_dresolution = 30.0;
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


void pdp_opencv_hough_lines_setup(void)
{

    post( "    pdp_opencv_hough_lines");
    pdp_opencv_hough_lines_class = class_new(gensym("pdp_opencv_hough_lines"), (t_newmethod)pdp_opencv_hough_lines_new,
      (t_method)pdp_opencv_hough_lines_free, sizeof(t_pdp_opencv_hough_lines), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_hough_lines_class, (t_method)pdp_opencv_hough_lines_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_hough_lines_class, (t_method)pdp_opencv_hough_lines_nightmode, gensym("nightmode"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_lines_class, (t_method)pdp_opencv_hough_lines_mode, gensym("mode"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_lines_class, (t_method)pdp_opencv_hough_lines_threshold, gensym("threshold"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_lines_class, (t_method)pdp_opencv_hough_lines_minlength, gensym("minlength"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_lines_class, (t_method)pdp_opencv_hough_lines_gap, gensym("gap"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_lines_class, (t_method)pdp_opencv_hough_lines_aresolution, gensym("aresolution"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_lines_class, (t_method)pdp_opencv_hough_lines_dresolution, gensym("dresolution"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_hough_lines_class, (t_method)pdp_opencv_hough_lines_maxlines, gensym("maxlines"), A_FLOAT, A_NULL );   
}

#ifdef __cplusplus
}
#endif
