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
#include <time.h>
#include <math.h>
#include <ctype.h>
#endif

typedef struct pdp_opencv_motempl_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_dataout;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;

    int x_thresh;
    double x_mhi_duration;
    int x_aperture;

    int x_infosok; 

    // ring image buffer
    IplImage **buf;
    int last;

    // temporary images
    IplImage *mhi; // MHI
    IplImage *orient; // orientation
    IplImage *mask; // valid orientation mask
    IplImage *segmask; // motion segmentation map
    CvMemStorage* storage; // temporary storage
    CvFont font;
    
    IplImage* image;
    IplImage* motion;

    // various tracking parameters (in seconds)
    double max_time_delta;
    double min_time_delta;

    // number of cyclic frame buffer used for motion detection
    // (should, probably, depend on FPS)
    int frame_buffer_num; 

    int max_size;
    int min_size;
    
    t_atom rlist[6];

} t_pdp_opencv_motempl;

void  pdp_opencv_motempl_update_mhi( t_pdp_opencv_motempl *x, IplImage* img, IplImage* dst, int diff_threshold )
{
  double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
  CvSize size = cvSize(img->width,img->height); // get current frame size
  int i, j, idx1 = x->last, idx2;
  IplImage* silh;
  CvSeq* seq;
  CvRect comp_rect;
  double count;
  double angle;
  CvPoint center;
  double magnitude;          
  CvScalar color;
  char tindex[10];

    // allocate images at the beginning or
    // reallocate them if the frame size is changed
    if( !x->mhi || x->mhi->width != size.width || x->mhi->height != size.height || !x->buf ) {
        if( x->buf == 0 ) {
            x->buf = (IplImage**)malloc(x->frame_buffer_num*sizeof(x->buf[0]));
            memset( x->buf, 0, x->frame_buffer_num*sizeof(x->buf[0]));
        }
        
        for( i = 0; i < x->frame_buffer_num; i++ ) {
            cvReleaseImage( &x->buf[i] );
            x->buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
            cvZero( x->buf[i] );
        }
        cvReleaseImage( &x->mhi );
        cvReleaseImage( &x->orient );
        cvReleaseImage( &x->segmask );
        cvReleaseImage( &x->mask );
        
        x->mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        cvZero( x->mhi ); // clear MHI at the beginning
        x->orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        x->segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        x->mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
    }

    cvCvtColor( img, x->buf[x->last], CV_BGR2GRAY ); // convert frame to grayscale

    idx2 = (x->last + 1) % x->frame_buffer_num; // index of (last - (N-1))th frame
    x->last = idx2;

    silh = x->buf[idx2];
    cvAbsDiff( x->buf[idx1], x->buf[idx2], silh ); // get difference between frames
    
    cvThreshold( silh, silh, diff_threshold, 1, CV_THRESH_BINARY ); // and threshold it
    cvUpdateMotionHistory( silh, x->mhi, timestamp, x->x_mhi_duration ); // update MHI

    // convert MHI to red 8u image
    cvCvtScale( x->mhi, x->mask, 255./x->x_mhi_duration,
                (x->x_mhi_duration - timestamp)*255./x->x_mhi_duration );
    cvZero( dst );
    cvCvtPlaneToPix( x->mask, 0, 0, 0, dst );

    // calculate motion gradient orientation and valid orientation mask
    cvCalcMotionGradient( x->mhi, x->mask, x->orient, x->max_time_delta, x->min_time_delta, x->x_aperture ); 
    
    if( !x->storage )
        x->storage = cvCreateMemStorage(0);
    else
        cvClearMemStorage(x->storage);
    
    // segment motion: get sequence of motion components
    // segmask is marked motion components map. It is not used further
    seq = cvSegmentMotion( x->mhi, x->segmask, x->storage, timestamp, x->max_time_delta ); 

    // iterate through the motion components,
    // One more iteration (i == -1) corresponds to the whole image (global motion)
    j=0;
    for( i = -1; i < seq->total; i++ ) {

        if( i < 0 ) { // case of the whole image
            comp_rect = cvRect( 0, 0, size.width, size.height );
            color = CV_RGB(255,255,255);
            magnitude = 100;
        }
        else { // i-th motion component
            comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
            if(( comp_rect.width + comp_rect.height < x->min_size )||( comp_rect.width + comp_rect.height > x->max_size )) // reject very small/big components 
                continue;
            color = CV_RGB(255,0,0);
            magnitude = (comp_rect.width + comp_rect.height) / 4;
        }

        // select component ROI
        cvSetImageROI( silh, comp_rect );
        cvSetImageROI( x->mhi, comp_rect );
        cvSetImageROI( x->orient, comp_rect );
        cvSetImageROI( x->mask, comp_rect );

        // calculate orientation
        angle = cvCalcGlobalOrientation( x->orient, x->mask, x->mhi, timestamp, x->x_mhi_duration);
        angle = 360.0 - angle;  // adjust for images with top-left origin

        count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI

        cvResetImageROI( x->mhi );
        cvResetImageROI( x->orient );
        cvResetImageROI( x->mask );
        cvResetImageROI( silh );

        // check for the case of little motion
        if( count < comp_rect.width*comp_rect.height * 0.05 )
            continue;

        // draw a clock with arrow indicating the direction
        center = cvPoint( (comp_rect.x + comp_rect.width/2),
                          (comp_rect.y + comp_rect.height/2) );

        cvCircle( dst, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
        cvLine( dst, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
                cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );

        if ( i<0 )
        {
          sprintf( tindex, "%d", i );
          cvPutText( dst, tindex, center, &x->font, CV_RGB(255,255,255));
          SETFLOAT(&x->rlist[0], i);
          SETFLOAT(&x->rlist[1], center.x);
          SETFLOAT(&x->rlist[2], center.y);
          SETFLOAT(&x->rlist[3], comp_rect.width);
          SETFLOAT(&x->rlist[4], comp_rect.height);
          SETFLOAT(&x->rlist[5], angle);
          outlet_list( x->x_dataout, 0, 6, x->rlist );
        }
        else
        {
          sprintf( tindex, "%d", ++j );
          cvPutText( dst, tindex, center, &x->font, CV_RGB(255,255,255));
          SETFLOAT(&x->rlist[0], j);
          SETFLOAT(&x->rlist[1], center.x);
          SETFLOAT(&x->rlist[2], center.y);
          SETFLOAT(&x->rlist[3], comp_rect.width);
          SETFLOAT(&x->rlist[4], comp_rect.height);
          SETFLOAT(&x->rlist[5], angle);
          outlet_list( x->x_dataout, 0, 6, x->rlist );
        }
    }
}

static void pdp_opencv_motempl_process_rgb(t_pdp_opencv_motempl *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    int i;
      

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

      post("pdp_opencv_motempl :: resizing plugins");
  
      //cv_freeplugins(x);

      x->x_width = header->info.image.width;
      x->x_height = header->info.image.height;
      x->x_size = x->x_width*x->x_height;
    
      //Destroy cv_images
      cvReleaseImage( &x->image );
      cvReleaseImage( &x->motion );
   
      //create cv_images 
      x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
      x->motion = cvCreateImage( cvSize(x->image->width,x->image->height), 8, 3 );
      cvZero( x->motion );
      x->motion->origin = x->image->origin;
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    memcpy( x->image->imageData, data, x->x_size*3 );
        
    pdp_opencv_motempl_update_mhi( x, x->image, x->motion, x->x_thresh );

    memcpy( newdata, x->motion->imageData, x->x_size*3 );

    return;
}

static void pdp_opencv_motempl_thresh(t_pdp_opencv_motempl *x, t_floatarg f)
{
  x->x_thresh = (int)f;
}

static void pdp_opencv_motempl_min_size(t_pdp_opencv_motempl *x, t_floatarg f)
{
  if (f>=0) x->min_size = (int)f;
}

static void pdp_opencv_motempl_max_size(t_pdp_opencv_motempl *x, t_floatarg f)
{
  if (f>=0) x->max_size = (int)f;
}

static void pdp_opencv_motempl_mhi_duration(t_pdp_opencv_motempl *x, t_floatarg f)
{
  if (f>0) x->x_mhi_duration = f;
}

static void pdp_opencv_motempl_aperture(t_pdp_opencv_motempl *x, t_floatarg f)
{
  if ( ( (int)f == 1.0 ) || ( (int)f == 3.0 ) || ( (int)f == 5.0 ) || (  (int)f == 7.0 ) )
  {
    x->x_aperture = (int)f;
  }
}

static void pdp_opencv_motempl_max_time_delta(t_pdp_opencv_motempl *x, t_floatarg f)
{
  if (f>0) x->max_time_delta = f;
}

static void pdp_opencv_motempl_min_time_delta(t_pdp_opencv_motempl *x, t_floatarg f)
{
  if (f>0) x->min_time_delta = f;
}

static void pdp_opencv_motempl_frame_buffer_num(t_pdp_opencv_motempl *x, t_floatarg f)
{
  if (f>=3) 
  {
    x->frame_buffer_num = (int)f;
    x->buf = NULL;
  }
}

static void pdp_opencv_motempl_sendpacket(t_pdp_opencv_motempl *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_motempl_process(t_pdp_opencv_motempl *x)
{
 int encoding;
 t_pdp *header = 0;

 /* check if image data packets are compatible */
 if ( (header = pdp_packet_header(x->x_packet0))
  && (PDP_BITMAP == header->type)){
    
  /* pdp_opencv_motempl_process inputs and write into active inlet */
  switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

  case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_motempl_process_rgb, (void*)pdp_opencv_motempl_sendpacket, &x->x_queue_id);
      break;

  default:
      /* don't know the type, so dont pdp_opencv_motempl_process */
      break;
      
  }
 }

}

static void pdp_opencv_motempl_input_0(t_pdp_opencv_motempl *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_motempl_process(x);
    }
}

static void pdp_opencv_motempl_free(t_pdp_opencv_motempl *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage( &x->image );
    cvReleaseImage( &x->motion );
}

t_class *pdp_opencv_motempl_class;

void *pdp_opencv_motempl_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_motempl *x = (t_pdp_opencv_motempl *)pd_new(pdp_opencv_motempl_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("threshold"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("min_size"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("max_size"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_dataout = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_infosok = 0;

    x->x_thresh = 30;
    x->x_mhi_duration = 1;
    x->x_aperture = 3;
  
    x->last = 0;
    // various tracking parameters (in seconds)
    x->max_time_delta = 0.5;
    x->min_time_delta = 0.05;
    // number of cyclic frame buffer used for motion detection
    // (should, probably, depend on FPS)
    x->frame_buffer_num = 4;

    x->min_size=50;
    x->max_size=500; 
    
    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->motion = cvCreateImage( cvSize(x->image->width,x->image->height), 8, 3 );
    cvZero( x->motion );
    x->motion->origin = x->image->origin;

    x->storage = NULL; 

    // initialize font
    cvInitFont( &x->font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_motempl_setup(void)
{

    post( "    pdp_opencv_motempl");
    pdp_opencv_motempl_class = class_new(gensym("pdp_opencv_motempl"), (t_newmethod)pdp_opencv_motempl_new,
      (t_method)pdp_opencv_motempl_free, sizeof(t_pdp_opencv_motempl), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_motempl_class, (t_method)pdp_opencv_motempl_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_motempl_class, (t_method)pdp_opencv_motempl_thresh, gensym("threshold"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_motempl_class, (t_method)pdp_opencv_motempl_mhi_duration, gensym("mhi_duration"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_motempl_class, (t_method)pdp_opencv_motempl_aperture, gensym("aperture"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_motempl_class, (t_method)pdp_opencv_motempl_max_time_delta, gensym("max_time_delta"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_motempl_class, (t_method)pdp_opencv_motempl_min_time_delta, gensym("min_time_delta"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_motempl_class, (t_method)pdp_opencv_motempl_frame_buffer_num, gensym("frame_buffer_num"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_motempl_class, (t_method)pdp_opencv_motempl_min_size, gensym("min_size"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_motempl_class, (t_method)pdp_opencv_motempl_max_size, gensym("max_size"),  A_FLOAT, A_NULL );   

}

#ifdef __cplusplus
}
#endif
