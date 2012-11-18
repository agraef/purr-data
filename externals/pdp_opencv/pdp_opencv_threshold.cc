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



typedef struct pdp_opencv_threshold_struct
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

    int threshold_value;
    int max_value;
    int threshold_mode;
    IplImage *image, *gray;

    
} t_pdp_opencv_threshold;

static void pdp_opencv_threshold_process_rgb(t_pdp_opencv_threshold *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_threshold :: resizing plugins");
	
    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->gray);
    
	//create the orig image with new size
        x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
    	x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    memcpy( x->image->imageData, data, x->x_size*3 );
    
    // Convert to grayscale
    cvCvtColor(x->image, x->gray, CV_BGR2GRAY);
  
    // Applies fixed-level thresholding to single-channel array.
    switch(x->threshold_mode) {
    	case 0:
	   cvThreshold(x->gray, x->gray, (float)x->threshold_value, (float)x->max_value, CV_THRESH_BINARY);
	   break;
    	case 1:
	   cvThreshold(x->gray, x->gray, (float)x->threshold_value, (float)x->max_value, CV_THRESH_BINARY_INV);
	   break;
    	case 2:
	   cvThreshold(x->gray, x->gray, (float)x->threshold_value, (float)x->max_value, CV_THRESH_TRUNC);
	   break;
    	case 3:
	   cvThreshold(x->gray, x->gray, (float)x->threshold_value, (float)x->max_value, CV_THRESH_TOZERO);
	   break;
    	case 4:
	   cvThreshold(x->gray, x->gray, (float)x->threshold_value, (float)x->max_value, CV_THRESH_TOZERO_INV);
	   break;
    }
  
    cvCvtColor(x->gray, x->image, CV_GRAY2BGR);
    memcpy( newdata, x->image->imageData, x->x_size*3 );
 
    return;
}

static void pdp_opencv_threshold_thresh(t_pdp_opencv_threshold *x, t_floatarg f)
{
    x->threshold_value = (int)f;
}

static void pdp_opencv_threshold_max(t_pdp_opencv_threshold *x, t_floatarg f)
{
    x->max_value = (int)f;
}

static void pdp_opencv_threshold_mode(t_pdp_opencv_threshold *x, t_floatarg f)
{
    x->threshold_mode = (int)f;
}

static void pdp_opencv_threshold_sendpacket(t_pdp_opencv_threshold *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_threshold_process(t_pdp_opencv_threshold *x)
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
    
	/* pdp_opencv_threshold_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_threshold_process_rgb, (void*)pdp_opencv_threshold_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_threshold_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_threshold_input_0(t_pdp_opencv_threshold *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_threshold_process(x);
    }
}

static void pdp_opencv_threshold_free(t_pdp_opencv_threshold *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    //cv_freeplugins(x);
    
    //Destroy cv_images
    cvReleaseImage(&x->image);
    cvReleaseImage(&x->gray);
}

t_class *pdp_opencv_threshold_class;

void *pdp_opencv_threshold_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_threshold *x = (t_pdp_opencv_threshold *)pd_new(pdp_opencv_threshold_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("max_value"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("threshold_value"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_infosok = 0;

    x->threshold_value = 50;
    x->max_value = 255;
    x->threshold_mode  = 0;

    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_threshold_setup(void)
{

    post( "		pdp_opencv_threshold");
    pdp_opencv_threshold_class = class_new(gensym("pdp_opencv_threshold"), (t_newmethod)pdp_opencv_threshold_new,
    	(t_method)pdp_opencv_threshold_free, sizeof(t_pdp_opencv_threshold), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_threshold_class, (t_method)pdp_opencv_threshold_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_threshold_class, (t_method)pdp_opencv_threshold_max, gensym("max_value"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_threshold_class, (t_method)pdp_opencv_threshold_thresh, gensym("threshold_value"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_threshold_class, (t_method)pdp_opencv_threshold_mode, gensym("mode"),  A_FLOAT, A_NULL );   


}

#ifdef __cplusplus
}
#endif
