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



typedef struct pdp_opencv_edge_struct
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

    int edge_thresh;
    IplImage *image, *cedge, *gray, *edge;

    
} t_pdp_opencv_edge;


void panic(const char *panicstr, ...)
{
    post("pdp_opencv_edge :: PANIC!! %s\n", panicstr);
    exit(1);
}

static void pdp_opencv_edge_process_rgb(t_pdp_opencv_edge *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
      

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_edge :: resizing buffers");
	
    	//cv_freeplugins(x);

    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->gray);
    	cvReleaseImage(&x->edge);
    	cvReleaseImage(&x->cedge);
    
	//create the orig image with new size
        x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
    	x->cedge = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 3);

    	x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    	x->edge = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    
    // We make here a copy of the PDP packet in image->imageData ... 
    // take a look on the IplImage data structure
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( x->image->imageData, data, x->x_size*3 );
    
    // Convert to grayscale
    cvCvtColor(x->image, x->gray, CV_BGR2GRAY);
  
    cvSmooth( x->gray, x->edge, CV_BLUR, 3, 3, 0, 0 );
    cvNot( x->gray, x->edge );

    // Run the edge detector on grayscale
    cvCanny(x->gray, x->edge, (float)x->edge_thresh, (float)x->edge_thresh*3, 3);
  
    cvZero( x->cedge );
    
    // copy edge points
    cvCopy( x->image, x->cedge, x->edge );

    //memory copy again, now from x->cedge->imageData to the new data pdp packet
    memcpy( newdata, x->cedge->imageData, x->x_size*3 );

 
    return;
}

static void pdp_opencv_edge_thresh(t_pdp_opencv_edge *x, t_floatarg f)
{
	x->edge_thresh = (int)f;
}

static void pdp_opencv_edge_sendpacket(t_pdp_opencv_edge *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_edge_process(t_pdp_opencv_edge *x)
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
    
	/* pdp_opencv_edge_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_edge_process_rgb, (void*)pdp_opencv_edge_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_edge_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_edge_input_0(t_pdp_opencv_edge *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_edge_process(x);
    }
}

static void pdp_opencv_edge_free(t_pdp_opencv_edge *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    //cv_freeplugins(x);
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->gray);
    	cvReleaseImage(&x->edge);
    	cvReleaseImage(&x->cedge);
}

t_class *pdp_opencv_edge_class;


void *pdp_opencv_edge_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_edge *x = (t_pdp_opencv_edge *)pd_new(pdp_opencv_edge_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("edge_thresh"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_infosok = 0;
    x->edge_thresh = 1;
    



    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);


// Create the output image
    x->cedge = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 3);

    // Convert to grayscale
    x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    x->edge = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    cvCvtColor(x->image, x->gray, CV_BGR2GRAY);

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_edge_setup(void)
{

    post( "		pdp_opencv_edge");
    pdp_opencv_edge_class = class_new(gensym("pdp_opencv_edge"), (t_newmethod)pdp_opencv_edge_new,
    	(t_method)pdp_opencv_edge_free, sizeof(t_pdp_opencv_edge), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_edge_class, (t_method)pdp_opencv_edge_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_edge_class, (t_method)pdp_opencv_edge_thresh, gensym("edge_thresh"),  A_FLOAT, A_NULL );   


}

#ifdef __cplusplus
}
#endif
