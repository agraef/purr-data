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


#define MAX_HISTOGRAMS_TO_COMPARE 80

typedef struct pdp_opencv_hist_compare_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_outlet1;
    t_outlet *x_dataout;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;

    int x_infosok; 

    int save_now;
    int nbsaved;
    
    CvHistogram *hist;
    CvHistogram *saved_hist[MAX_HISTOGRAMS_TO_COMPARE];
    IplImage *src, *hsv, *h_plane, *s_plane, *v_plane, *h_saved_plane, *s_saved_plane, *v_saved_plane, *planes[2],*saved_planes[2];

    
} t_pdp_opencv_hist_compare;



static void pdp_opencv_hist_compare_process_rgb(t_pdp_opencv_hist_compare *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
      
    int h_bins = (int)(x->x_width/10), s_bins = (int)(x->x_height/10);

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	// if image size has changed we must create again our image and histogram structures
    	post("pdp_opencv_hist_compare :: resizing buffers");

    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
	cvReleaseImage(&x->src);
	cvReleaseImage(&x->hsv);
    	cvReleaseImage(&x->h_plane);
    	cvReleaseImage(&x->s_plane);
    	cvReleaseImage(&x->v_plane);
    	cvReleaseImage(&x->h_saved_plane);
    	cvReleaseImage(&x->s_saved_plane);
    	cvReleaseImage(&x->v_saved_plane);
    
	//create the orig image with new size
        x->src = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
	x->hsv = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3 );

    	x->h_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    	x->s_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    	x->v_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
	x->planes[0] = x->h_plane;
	x->planes[1] = x->s_plane;
    	x->h_saved_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    	x->s_saved_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    	x->v_saved_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
	x->saved_planes[0] = x->h_saved_plane;
	x->saved_planes[1] = x->s_saved_plane;
        {
          int    hist_size[]  = { h_bins, s_bins };
          float  h_ranges[]   = { 0, 180 };         // hue is [0,180]
          float  s_ranges[]   = { 0, 255 };
          float* ranges[]     = { h_ranges, s_ranges };
          x->hist = cvCreateHist(
            2,
            hist_size,
            CV_HIST_ARRAY,
            ranges,
            1
          );
	  int n;
	  for (n=0; n<MAX_HISTOGRAMS_TO_COMPARE; n++) {
          	x->saved_hist[n] = cvCreateHist(
          	  2,
          	  hist_size,
          	  CV_HIST_ARRAY,
          	  ranges,
          	  1
          	);
	  }
        }

    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;
    
    memcpy( newdata, data, x->x_size*3 );

    
    // We make here a copy of the PDP packet in image->imageData ... 
    // take a look on the IplImage data structure
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( x->src->imageData, data, x->x_size*3 );
    
    // Convert to grayscale
    cvCvtColor( x->src, x->hsv, CV_BGR2HSV );
    cvCvtPixToPlane( x->hsv, x->h_plane, x->s_plane, x->v_plane, 0 );
	
    // Build the histogram and compute its contents.
    if (x->save_now>=0) {
    		fprintf(stderr,"saving histogram %d\n",x->save_now);
    		cvCvtPixToPlane( x->hsv, x->h_saved_plane, x->s_saved_plane, x->v_saved_plane, 0 );
        	cvCalcHist( x->saved_planes, x->saved_hist[x->save_now], 0, 0 ); //Compute histogram
        	cvNormalizeHist( x->saved_hist[x->save_now], 1.0 );  //Normalize it 
		x->nbsaved++;
		x->save_now=-1;
     } 
     cvCalcHist( x->planes, x->hist, 0, 0 ); //Compute histogram
     cvNormalizeHist( x->hist, 1.0 );  //Normalize it 

     double tato[x->nbsaved];
     t_atom datalist[x->nbsaved];
     int nearest = -1;
     double max  =  0;

     int n;
     if ( x->nbsaved > 0 )
       for (n=0; n<x->nbsaved; n++) {
		tato[n] = cvCompareHist(x->hist, x->saved_hist[n], CV_COMP_INTERSECT);
		SETFLOAT(&datalist[n], tato[n]);
		if (tato[n]>max) {
			max = tato[n];
			nearest = n;
		}
     }
    	
    if ( x->nbsaved > 0 ) {
       outlet_float(x->x_outlet1, (float)nearest);
       outlet_list( x->x_dataout, 0, x->nbsaved, datalist );
    } else
       outlet_float(x->x_outlet1, -1.0);
    	    

    // Create an image to use to visualize our histogram.
    int scale = 10;
    //IplImage* hist_img = cvCreateImage( 
    //  cvSize(x->x_width,x->x_height),
    //  8,
    //  3
    //);
    //cvZero( hist_img );

    // populate our visualization with little gray squares.
	
    float max_value = 0;
    cvGetMinMaxHistValue( x->hist, 0, &max_value, 0, 0 );

    int h = 0;
    int s = 0;

    for( h = 0; h < h_bins; h++ ) {
         for( s = 0; s < s_bins; s++ ) {
                 float bin_val = cvQueryHistValue_2D( x->hist, h, s );
                 int intensity = cvRound( bin_val * 255 / max_value );
                 cvRectangle(
                   x->src,
                   cvPoint( h*scale, s*scale ),
                   cvPoint( (h+1)*scale - 1, (s+1)*scale - 1),
		   CV_RGB(intensity,intensity,intensity), CV_FILLED, 8 , 0 );
               }
    }

    //memory copy again, now from x->cedge->imageData to the new data pdp packet
    //memcpy( newdata, hist_img, x->x_size*3 );
    memcpy( newdata, x->src->imageData, x->x_size*3 );

    return;
}

static void pdp_opencv_hist_compare_save(t_pdp_opencv_hist_compare *x, t_floatarg f)
{
	if (((int)f>=0)&&((int)f<MAX_HISTOGRAMS_TO_COMPARE)) x->save_now = (int)f;
}

static void pdp_opencv_hist_compare_sendpacket(t_pdp_opencv_hist_compare *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_hist_compare_process(t_pdp_opencv_hist_compare *x)
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
    
	/* pdp_opencv_hist_compare_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_hist_compare_process_rgb, (void*)pdp_opencv_hist_compare_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_hist_compare_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_hist_compare_input_0(t_pdp_opencv_hist_compare *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_hist_compare_process(x);
    }
}

static void pdp_opencv_hist_compare_free(t_pdp_opencv_hist_compare *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    //cv_freeplugins(x);
    
    //Destroy cv_images
    cvReleaseImage(&x->src);
    cvReleaseImage(&x->hsv);
    cvReleaseImage(&x->h_plane);
    cvReleaseImage(&x->s_plane);
    cvReleaseImage(&x->v_plane);
    cvReleaseImage(&x->h_saved_plane);
    cvReleaseImage(&x->s_saved_plane);
    cvReleaseImage(&x->v_saved_plane);
}

t_class *pdp_opencv_hist_compare_class;


void *pdp_opencv_hist_compare_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_hist_compare *x = (t_pdp_opencv_hist_compare *)pd_new(pdp_opencv_hist_compare_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("save"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet1 = outlet_new(&x->x_obj, &s_float);
    x->x_dataout = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_infosok = 0;
    x->save_now = 0;
    x->nbsaved = 0;
    
    x->src = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->hsv = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3 );

    x->h_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    x->s_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    x->v_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    x->planes[0] = x->h_plane;
    x->planes[1] = x->s_plane;
    cvCvtPixToPlane( x->hsv, x->h_plane, x->s_plane, x->v_plane, 0 );
    x->h_saved_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    x->s_saved_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    x->v_saved_plane = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    x->saved_planes[0] = x->h_saved_plane;
    x->saved_planes[1] = x->s_saved_plane;
    cvCvtPixToPlane( x->hsv, x->h_saved_plane, x->s_saved_plane, x->v_saved_plane, 0 );

    cvCvtColor( x->src, x->hsv, CV_BGR2HSV );

    int h_bins = (int)(x->x_width/10), s_bins = (int)(x->x_height/10);
    {
          int    hist_size[]  = { h_bins, s_bins };
          float  h_ranges[]   = { 0, 180 };         // hue is [0,180]
          float  s_ranges[]   = { 0, 255 };
          float* ranges[]     = { h_ranges, s_ranges };
          x->hist = cvCreateHist(
            2,
            hist_size,
            CV_HIST_ARRAY,
            ranges,
            1
          );
	  int n;
	  for (n=0; n<MAX_HISTOGRAMS_TO_COMPARE; n++) {
          	x->saved_hist[n] = cvCreateHist(
          	  2,
          	  hist_size,
          	  CV_HIST_ARRAY,
          	  ranges,
          	  1
          	);
	  }
    }

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_hist_compare_setup(void)
{

    post( "		pdp_opencv_hist_compare");
    pdp_opencv_hist_compare_class = class_new(gensym("pdp_opencv_hist_compare"), (t_newmethod)pdp_opencv_hist_compare_new,
    	(t_method)pdp_opencv_hist_compare_free, sizeof(t_pdp_opencv_hist_compare), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_hist_compare_class, (t_method)pdp_opencv_hist_compare_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_hist_compare_class, (t_method)pdp_opencv_hist_compare_save, gensym("save"),  A_FLOAT, A_NULL );   

}

#ifdef __cplusplus
}
#endif
