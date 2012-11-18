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
#include "cvaux.h"
#endif


typedef struct pdp_opencv_bgsubstract_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;

    int x_infosok; 
    int x_set; 
    int x_threshold; 

    IplImage *image, *prev_image, *gray, *grayLow, *grayUp, *diff_8U;
    
} t_pdp_opencv_bgsubstract;

static void pdp_opencv_bgsubstract_process_rgb(t_pdp_opencv_bgsubstract *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
      

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_bgsubstract :: resizing plugins");
	
    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->gray);
    	cvReleaseImage(&x->grayLow);
    	cvReleaseImage(&x->grayUp);
    	cvReleaseImage(&x->prev_image);
    	cvReleaseImage(&x->diff_8U);
    
	//create the orig image with new size
        x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    	// Create the output and temp images with new sizes
    	x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    	x->grayLow = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    	x->grayUp = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
        x->prev_image = cvCreateImage( cvSize(x->image->width,x->image->height), 8, 3 );
        x->diff_8U = cvCreateImage( cvSize(x->image->width,x->image->height), 8, 1 );
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( x->image->imageData, data, x->x_size*3 );

    //cvCvtColor( x->image, x->gray, CV_BGR2GRAY );

    if (x->x_set) {
    	memcpy( x->prev_image->imageData, data, x->x_size*3 );
	x->x_set=0;
    } 

    //cvSubS (x->prev_image,cvScalar(x->x_threshold,x->x_threshold,x->x_threshold,x->x_threshold),x->grayLow,NULL);
    //cvAddS (x->prev_image,cvScalar(x->x_threshold,x->x_threshold,x->x_threshold,x->x_threshold),x->grayUp,NULL);
    //cvInRange (x->gray, x->grayLow, x->grayUp, x->diff_8U);

    //cvNot (x->diff_8U,x->diff_8U);

    //cvCvtColor(x->diff_8U,x->image,CV_GRAY2BGR);

  int h,w,hlength, chRed, chGreen, chBlue;
  long src,pixsize;

  chRed   =0;
  chGreen =1;
  chBlue  =2;
  
  src = 0;


  for (h=0; h<x->image->height; h++){
    for(w=0; w<x->image->width; w++){
      if (((x->image->imageData[src+chRed  ] > x->prev_image->imageData[src+chRed  ] - x->x_threshold)&&
	   (x->image->imageData[src+chRed  ] < x->prev_image->imageData[src+chRed  ] + x->x_threshold))&&
	  ((x->image->imageData[src+chGreen] > x->prev_image->imageData[src+chGreen] - x->x_threshold)&&
	   (x->image->imageData[src+chGreen] < x->prev_image->imageData[src+chGreen] + x->x_threshold))&&
	  ((x->image->imageData[src+chBlue ] > x->prev_image->imageData[src+chBlue ] - x->x_threshold)&&
	   (x->image->imageData[src+chBlue ] < x->prev_image->imageData[src+chBlue ] + x->x_threshold)))
	{
	  x->image->imageData[src+chRed] = 0;
	  x->image->imageData[src+chGreen] = 0;
	  x->image->imageData[src+chBlue] = 0;
	}
      src+=3;
    }
  }

  
    memcpy( newdata, x->image->imageData, x->x_size*3 );
 
    return;
}

static void pdp_opencv_bgsubstract_threshold(t_pdp_opencv_bgsubstract *x, t_floatarg f)
{
	if (f>=1) x->x_threshold=(int)f;
}

static void pdp_opencv_bgsubstract_set(t_pdp_opencv_bgsubstract *x)
{
	x->x_set=1;
}

static void pdp_opencv_bgsubstract_sendpacket(t_pdp_opencv_bgsubstract *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_bgsubstract_process(t_pdp_opencv_bgsubstract *x)
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
    
	/* pdp_opencv_bgsubstract_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_bgsubstract_process_rgb, (void*)pdp_opencv_bgsubstract_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_bgsubstract_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_bgsubstract_input_0(t_pdp_opencv_bgsubstract *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_bgsubstract_process(x);
    }
}

static void pdp_opencv_bgsubstract_free(t_pdp_opencv_bgsubstract *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    //cv_freeplugins(x);
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->gray);
    	cvReleaseImage(&x->grayLow);
    	cvReleaseImage(&x->grayUp);
    	cvReleaseImage(&x->prev_image);
    	cvReleaseImage(&x->diff_8U);
}

t_class *pdp_opencv_bgsubstract_class;


void *pdp_opencv_bgsubstract_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_bgsubstract *x = (t_pdp_opencv_bgsubstract *)pd_new(pdp_opencv_bgsubstract_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("threshold"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_infosok = 0;
    x->x_set = 1;
    x->x_threshold = 13;

    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    x->grayLow = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    x->grayUp = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    x->prev_image = cvCreateImage( cvSize(x->image->width,x->image->height), 8, 3 );
    x->diff_8U = cvCreateImage( cvSize(x->image->width,x->image->height), 8, 1 );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_bgsubstract_setup(void)
{

    post( "		pdp_opencv_bgsubstract");
    pdp_opencv_bgsubstract_class = class_new(gensym("pdp_opencv_bgsubstract"), (t_newmethod)pdp_opencv_bgsubstract_new,
    	(t_method)pdp_opencv_bgsubstract_free, sizeof(t_pdp_opencv_bgsubstract), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_bgsubstract_class, (t_method)pdp_opencv_bgsubstract_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_bgsubstract_class, (t_method)pdp_opencv_bgsubstract_set, gensym("set"),  A_NULL );   
    class_addmethod(pdp_opencv_bgsubstract_class, (t_method)pdp_opencv_bgsubstract_threshold, gensym("threshold"),  A_FLOAT, A_NULL );   

}

#ifdef __cplusplus
}
#endif
