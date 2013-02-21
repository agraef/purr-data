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
 *   Horn and Schunck optical flow algorithm
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <dlfcn.h>
#include <ctype.h>
#include <math.h>
#include <cvaux.h>

#include "pdp.h"

#ifndef _EiC
#include "cv.h"
#endif

typedef struct pdp_opencv_of_hs_struct
{
  t_object x_obj;
  t_float x_f;

  t_outlet *x_outlet0;
  t_outlet *x_outlet1;
  t_outlet *x_outlet2;
  t_atom x_list[3];

  int x_packet0;
  int x_packet1;
  int x_dropped;
  int x_queue_id;

  int x_width;
  int x_height;
  int x_size;

  // OpenCv structures
  IplImage *image, *grey, *prev_grey, *swap_temp;
  IplImage *x_velx, *x_vely;
  CvSize   x_velsize;
  double x_lambda;

  int x_nightmode;
  int x_threshold;
  int x_useprevious;
  int x_minblocks;
  CvFont font;

} t_pdp_opencv_of_hs;

static void pdp_opencv_of_hs_process_rgb(t_pdp_opencv_of_hs *x)
{
  t_pdp     *header = pdp_packet_header(x->x_packet0);
  short int *data   = (short int *)pdp_packet_data(x->x_packet0);
  t_pdp     *newheader = pdp_packet_header(x->x_packet1);
  short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
  int i,j,k,im;
  int marked;
  int px,py;
  double globangle=0.0, globx=0.0, globy=0.0, maxamp=0.0, maxangle=0.0;
  int nbblocks=0;
  CvPoint orig, dest;
  double angle=0.0;
  double hypotenuse=0.0;
  char tindex[4];

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height) || (!x->image)) 
    {

      post("pdp_opencv_of_hs :: resizing plugins");
  
      x->x_width = header->info.image.width;
      x->x_height = header->info.image.height;
      x->x_size = x->x_width*x->x_height;

      x->x_velsize.width = x->x_width;
      x->x_velsize.height = x->x_height;
    
      //Destroy cv_images
      cvReleaseImage( &x->image );
      cvReleaseImage( &x->grey );
      cvReleaseImage( &x->prev_grey );
      cvReleaseImage( &x->x_velx );
      cvReleaseImage( &x->x_vely );
   
      //Create cv_images 
      x->image = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
      x->grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
      x->prev_grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );

      x->x_velx = cvCreateImage( x->x_velsize, IPL_DEPTH_32F, 1 );
      x->x_vely = cvCreateImage( x->x_velsize, IPL_DEPTH_32F, 1 );
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    memcpy( x->image->imageData, data, x->x_size*3 );
        
    cvCvtColor( x->image, x->grey, CV_RGB2GRAY );

    if( x->x_nightmode )
        cvZero( x->image );
        
    cvCalcOpticalFlowHS( x->prev_grey, x->grey, 
                         x->x_useprevious, x->x_velx, x->x_vely,
                         x->x_lambda, 
                         cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03) );

    nbblocks = 0;
    globangle = 0;
    globx = 0;
    globy = 0;
    for( py=0; py<x->x_velsize.height; py++ ) 
    {
      for( px=0; px<x->x_velsize.width; px++ )
      {
        // post( "pdp_opencv_of_hs : (%d,%d) values (%f,%f)", px, py, velxf, velyf );
        orig.x = px;
        orig.y = py;
        dest.x = (int)(orig.x + cvGet2D(x->x_velx, py, px).val[0]);
        dest.y = (int)(orig.y + cvGet2D(x->x_vely, py, px).val[0]);
        angle = -atan2( (double) (dest.y-orig.y), (double) (dest.x-orig.x) );
        hypotenuse = sqrt( pow(dest.y-orig.y, 2) + pow(dest.x-orig.x, 2) );

        /* Now draw the tips of the arrow. I do some scaling so that the
        * tips look proportional to the main line of the arrow.
        */
        if (hypotenuse >= x->x_threshold)
        {
          cvLine( x->image, orig, dest, CV_RGB(0,255,0), 1, CV_AA, 0 );

          orig.x = (int) (dest.x - (6) * cos(angle + M_PI / 4));
          orig.y = (int) (dest.y + (6) * sin(angle + M_PI / 4));
          cvLine( x->image, orig, dest, CV_RGB(0,0,255), 1, CV_AA, 0 );
          orig.x = (int) (dest.x - (6) * cos(angle - M_PI / 4));
          orig.y = (int) (dest.y + (6) * sin(angle - M_PI / 4));
          cvLine( x->image, orig, dest, CV_RGB(0,0,255), 1, CV_AA, 0 );

          globx = globx+cvGet2D(x->x_velx, py, px).val[0];
          globy = globy+cvGet2D(x->x_vely, py, px).val[0];
          if ( hypotenuse > maxamp )
          {
             maxamp = hypotenuse;
             maxangle = angle;
          } 
          // post( "pdp_opencv_of_hs : block %d : amp : %f : angle : %f", nbblocks, hypotenuse, (angle*180)/M_PI );
          nbblocks++;
        } 

      }
    }

    if ( nbblocks >= x->x_minblocks )
    {
      globangle=-atan2( globy, globx ); 
      // post( "pdp_opencv_of_hs : globangle : %f", (globangle*180)/M_PI );

      orig.x = (int) (x->x_width/2);
      orig.y = (int) (x->x_height/2);
      dest.x = (int) (orig.x+((x->x_width>x->x_height)?x->x_height/2:x->x_width/2)*cos(globangle));
      dest.y = (int) (orig.y-((x->x_width>x->x_height)?x->x_height/2:x->x_width/2)*sin(globangle));

      cvLine( x->image, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );
      orig.x = (int) (dest.x - (6) * cos(globangle + M_PI / 4));
      orig.y = (int) (dest.y + (6) * sin(globangle + M_PI / 4));
      cvLine( x->image, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );
      orig.x = (int) (dest.x - (6) * cos(globangle - M_PI / 4));
      orig.y = (int) (dest.y + (6) * sin(globangle - M_PI / 4));
      cvLine( x->image, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );

      // outputs the average angle of movement
      globangle = (globangle*180)/M_PI;
      SETFLOAT(&x->x_list[0], globangle);
      outlet_list( x->x_outlet1, 0, 1, x->x_list );

      // outputs the amplitude and angle of the maximum movement
      maxangle = (maxangle*180)/M_PI;
      SETFLOAT(&x->x_list[0], maxamp);
      SETFLOAT(&x->x_list[1], maxangle);
      outlet_list( x->x_outlet2, 0, 2, x->x_list );
    }

    memcpy( x->prev_grey->imageData, x->grey->imageData, x->x_size );

    memcpy( newdata, x->image->imageData, x->x_size*3 );
    return;
}

static void pdp_opencv_of_hs_nightmode(t_pdp_opencv_of_hs *x, t_floatarg f)
{
  if ((f==0.0)||(f==1.0)) x->x_nightmode = (int)f;
}

static void pdp_opencv_of_hs_useprevious(t_pdp_opencv_of_hs *x, t_floatarg f)
{
  if ((f==0.0)||(f==1.0)) x->x_useprevious = (int)f;
}

static void pdp_opencv_of_hs_minblocks(t_pdp_opencv_of_hs *x, t_floatarg f)
{
  if (f>=1.0) x->x_minblocks = (int)f;
}

static void pdp_opencv_of_hs_threshold(t_pdp_opencv_of_hs *x, t_floatarg f)
{
  if (f>=0.0) x->x_threshold = (int)f;
}

static void pdp_opencv_of_hs_lambda(t_pdp_opencv_of_hs *x, t_floatarg flambda )
{
  if (flambda>0.0) x->x_lambda = (double)flambda;
}

static void pdp_opencv_of_hs_sendpacket(t_pdp_opencv_of_hs *x)
{
  /* release the packet */
  pdp_packet_mark_unused(x->x_packet0);
  x->x_packet0 = -1;

  /* unregister and propagate if valid dest packet */
  pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_of_hs_process(t_pdp_opencv_of_hs *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
     && (PDP_BITMAP == header->type)){
    
     /* pdp_opencv_of_hs_process inputs and write into active inlet */
     switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

     case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_of_hs_process_rgb, (void*)pdp_opencv_of_hs_sendpacket, &x->x_queue_id);
      break;

     default:
      /* don't know the type, so dont pdp_opencv_of_hs_process */
      break;
      
     }
   }

}

static void pdp_opencv_of_hs_input_0(t_pdp_opencv_of_hs *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_of_hs_process(x);
    }
}

static void pdp_opencv_of_hs_free(t_pdp_opencv_of_hs *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    //cv_freeplugins(x);
    
    //Destroy cv_images
    cvReleaseImage( &x->image );
    cvReleaseImage( &x->grey );
    cvReleaseImage( &x->prev_grey );
    cvReleaseImage( &x->x_velx );
    cvReleaseImage( &x->x_vely );
}

t_class *pdp_opencv_of_hs_class;

void *pdp_opencv_of_hs_new(t_floatarg f)
{
  int i;

  t_pdp_opencv_of_hs *x = (t_pdp_opencv_of_hs *)pd_new(pdp_opencv_of_hs_class);

  x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
  x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);
  x->x_outlet2 = outlet_new(&x->x_obj, &s_anything);

  x->x_packet0 = -1;
  x->x_packet1 = -1;
  x->x_queue_id = -1;

  x->x_width  = 320;
  x->x_height = 240;
  x->x_size   = x->x_width * x->x_height;

  x->x_nightmode=0;
  x->x_threshold=100;
  x->x_lambda = 1.0;
  x->x_useprevious = 0;
  x->x_minblocks = 10;
  x->x_velsize.width = x->x_width; 
  x->x_velsize.height = x->x_height; 

  // initialize font
  cvInitFont( &x->font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );
    
  x->image = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
  x->grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
  x->prev_grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );

  x->x_velx = cvCreateImage( x->x_velsize, IPL_DEPTH_32F, 1 );
  x->x_vely = cvCreateImage( x->x_velsize, IPL_DEPTH_32F, 1 );

  return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_of_hs_setup(void)
{

    post( "    pdp_opencv_of_hs");
    pdp_opencv_of_hs_class = class_new(gensym("pdp_opencv_of_hs"), (t_newmethod)pdp_opencv_of_hs_new,
      (t_method)pdp_opencv_of_hs_free, sizeof(t_pdp_opencv_of_hs), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_of_hs_class, (t_method)pdp_opencv_of_hs_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_of_hs_class, (t_method)pdp_opencv_of_hs_nightmode, gensym("nightmode"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_of_hs_class, (t_method)pdp_opencv_of_hs_threshold, gensym("threshold"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_of_hs_class, (t_method)pdp_opencv_of_hs_useprevious, gensym("useprevious"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_of_hs_class, (t_method)pdp_opencv_of_hs_lambda, gensym("lambda"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_of_hs_class, (t_method)pdp_opencv_of_hs_minblocks, gensym("minblocks"), A_FLOAT, A_NULL );   

}

#ifdef __cplusplus
}
#endif
