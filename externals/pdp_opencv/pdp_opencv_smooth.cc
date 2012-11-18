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


typedef struct pdp_opencv_smooth_struct
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

    int x_mode; 
    int x_param1; 
    int x_param2; 

    IplImage *image, *oimage, *grey;

    
} t_pdp_opencv_smooth;


static void pdp_opencv_smooth_process_rgb(t_pdp_opencv_smooth *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
      

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_smooth :: resizing buffers");
	
    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->oimage);
    	cvReleaseImage(&x->grey);
    
	//create the orig image with new size
        x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);

    	//create the output images with new sizes
    	x->oimage = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 3);
    	x->grey = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( x->image->imageData, data, x->x_size*3 );
    
    if ( x->x_mode==CV_BLUR_NO_SCALE ) // operate on the gray scale image
    {
       cvCvtColor(x->image, x->grey, CV_BGR2GRAY);
       cvSmooth( x->grey, x->grey, x->x_mode, x->x_param1, x->x_param2, 0, 0 );
       cvCvtColor(x->grey, x->oimage, CV_GRAY2BGR);
    }
    else
    {
       cvSmooth( x->image, x->oimage, x->x_mode, x->x_param1, x->x_param2, 0, 0 );
    }

    //memory copy again, now from x->oimage->imageData to the new data pdp packet
    memcpy( newdata, x->oimage->imageData, x->x_size*3 );
 
    return;
}

static void pdp_opencv_smooth_mode(t_pdp_opencv_smooth *x, t_floatarg fmode)
{
    if ((int)fmode==CV_BLUR_NO_SCALE )
    {
       x->x_mode=CV_BLUR_NO_SCALE;
    }
    if ((int)fmode==CV_BLUR )
    {
       x->x_mode=CV_BLUR;
    }
    if ((int)fmode==CV_GAUSSIAN )
    {
       x->x_mode=CV_GAUSSIAN;
    }
    if ((int)fmode==CV_MEDIAN )
    {
       x->x_mode=CV_MEDIAN;
    }
    if ((int)fmode==CV_BILATERAL )
    {
       x->x_mode=CV_BILATERAL;
    }
}

static void pdp_opencv_smooth_param1(t_pdp_opencv_smooth *x, t_floatarg fparam1)
{
    if ((int)fparam1>0 && ((int)fparam1%2==1) ) x->x_param1 = (int)fparam1;
}

static void pdp_opencv_smooth_param2(t_pdp_opencv_smooth *x, t_floatarg fparam2)
{
    if ((int)fparam2>0 && ((int)fparam2%2==1) ) x->x_param2 = (int)fparam2;
}

static void pdp_opencv_smooth_sendpacket(t_pdp_opencv_smooth *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_smooth_process(t_pdp_opencv_smooth *x)
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
    
	/* pdp_opencv_smooth_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_smooth_process_rgb, (void*)pdp_opencv_smooth_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_smooth_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_smooth_input_0(t_pdp_opencv_smooth *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_smooth_process(x);
    }
}

static void pdp_opencv_smooth_free(t_pdp_opencv_smooth *x)
{
    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage(&x->image);
    cvReleaseImage(&x->oimage);
    cvReleaseImage(&x->grey);
}

t_class *pdp_opencv_smooth_class;

void *pdp_opencv_smooth_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_smooth *x = (t_pdp_opencv_smooth *)pd_new(pdp_opencv_smooth_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_mode = CV_GAUSSIAN;
    x->x_param1 = 3;
    x->x_param2 = 0;

    // Create the open cv images
    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->oimage = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 3);
    x->grey = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_smooth_setup(void)
{

    post( "		pdp_opencv_smooth");
    pdp_opencv_smooth_class = class_new(gensym("pdp_opencv_smooth"), (t_newmethod)pdp_opencv_smooth_new,
    	(t_method)pdp_opencv_smooth_free, sizeof(t_pdp_opencv_smooth), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_smooth_class, (t_method)pdp_opencv_smooth_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_smooth_class, (t_method)pdp_opencv_smooth_mode, gensym("mode"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_smooth_class, (t_method)pdp_opencv_smooth_param1, gensym("param1"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_smooth_class, (t_method)pdp_opencv_smooth_param2, gensym("param2"),  A_FLOAT, A_NULL );   


}

#ifdef __cplusplus
}
#endif
