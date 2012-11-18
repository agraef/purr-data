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

typedef struct pdp_opencv_laplace_struct
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

    int aperture_size;
    int build_voronoi;
    int mask_size;

    // The output and temporary images
    IplImage* frame;
    IplImage* laplace;
    IplImage* colorlaplace;
    IplImage* planes[3];

    
} t_pdp_opencv_laplace;

static void pdp_opencv_laplace_process_rgb(t_pdp_opencv_laplace *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    int i;
      
    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_laplace :: resizing plugins");
	
    	//cv_freeplugins(x);

    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
    	for( i = 0; i < 3; i++ )
    		cvReleaseImage( &x->planes[i] );
    	cvReleaseImage( &x->frame );
    	cvReleaseImage( &x->laplace );
    	cvReleaseImage( &x->colorlaplace );
   
	//Create cv_images 
    	for( i = 0; i < 3; i++ )
     	 	x->planes[i] = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
    	x->laplace = cvCreateImage( cvSize(x->x_width, x->x_height), IPL_DEPTH_16S, 1 );
    	x->colorlaplace = cvCreateImage( cvSize(x->x_width,x->x_height), 8, 3 );
    	x->frame = cvCreateImage( cvSize(x->x_width,x->x_height), 8, 3 );
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    // FEM UNA COPIA DEL PACKET A x->grey->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( x->frame->imageData, data, x->x_size*3 );
        
    cvCvtPixToPlane( x->frame, x->planes[0], x->planes[1], x->planes[2], 0 );
    for( i = 0; i < 3; i++ )
    {
      cvLaplace( x->planes[i], x->laplace, x->aperture_size );
      cvConvertScaleAbs( x->laplace, x->planes[i], 1, 0 );
    }
    cvCvtPlaneToPix( x->planes[0], x->planes[1], x->planes[2], 0, x->colorlaplace );
    x->colorlaplace->origin = x->frame->origin;

    memcpy( newdata, x->colorlaplace->imageData, x->x_size*3 );
 
    return;
}


static void pdp_opencv_laplace_thresh(t_pdp_opencv_laplace *x, t_floatarg f)
{
    if ((f==1)||(f==3)||(f==5)||(f==7)) x->aperture_size = (int)f;
}

static void pdp_opencv_laplace_sendpacket(t_pdp_opencv_laplace *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_laplace_process(t_pdp_opencv_laplace *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_opencv_laplace_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_laplace_process_rgb, (void*)pdp_opencv_laplace_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_laplace_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_laplace_input_0(t_pdp_opencv_laplace *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_laplace_process(x);
    }
}

static void pdp_opencv_laplace_free(t_pdp_opencv_laplace *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    for( i = 0; i < 3; i++ )
    	cvReleaseImage( &x->planes[i] );
    cvReleaseImage( &x->frame );
    cvReleaseImage( &x->laplace );
    cvReleaseImage( &x->colorlaplace );
}

t_class *pdp_opencv_laplace_class;


void *pdp_opencv_laplace_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_laplace *x = (t_pdp_opencv_laplace *)pd_new(pdp_opencv_laplace_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("aperture_size"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_infosok = 0;

    x->aperture_size = 3;
    
    for( i = 0; i < 3; i++ )
     x->planes[i] = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
    x->laplace = cvCreateImage( cvSize(x->x_width, x->x_height), IPL_DEPTH_16S, 1 );
    x->colorlaplace = cvCreateImage( cvSize(x->x_width,x->x_height), 8, 3 );
    x->frame = cvCreateImage( cvSize(x->x_width,x->x_height), 8, 3 );


    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_laplace_setup(void)
{

    post( "		pdp_opencv_laplace");
    pdp_opencv_laplace_class = class_new(gensym("pdp_opencv_laplace"), (t_newmethod)pdp_opencv_laplace_new,
    	(t_method)pdp_opencv_laplace_free, sizeof(t_pdp_opencv_laplace), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_laplace_class, (t_method)pdp_opencv_laplace_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_laplace_class, (t_method)pdp_opencv_laplace_thresh, gensym("aperture_size"),  A_FLOAT, A_NULL );   

}

#ifdef __cplusplus
}
#endif
