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

typedef struct pdp_opencv_morphology_struct
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

    int pos;
    int element_shape;
    int mode; //to switch between openclose or dilateerode modes

    // The output and temporary images
    IplImage* src;
    IplImage* dst;

    IplConvKernel* element;
    
} t_pdp_opencv_morphology;


// callback function for open/close trackbar
void pdp_opencv_morphology_OpenClose(t_pdp_opencv_morphology *x, int pos)   
{
    int n = x->pos;
    int an = n > 0 ? n : -n;
    x->element = cvCreateStructuringElementEx( an*2+1, an*2+1, an, an, x->element_shape, 0 );
    if( n < 0 )
    {
        cvErode(x->src,x->dst,x->element,1);
        cvDilate(x->dst,x->dst,x->element,1);
    }
    else
    {
        cvDilate(x->src,x->dst,x->element,1);
        cvErode(x->dst,x->dst,x->element,1);
    }
    cvReleaseStructuringElement(&x->element);
}   

// callback function for erode/dilate trackbar
void pdp_opencv_morphology_ErodeDilate(t_pdp_opencv_morphology *x, int pos)   
{
    int n = x->pos;
    int an = n > 0 ? n : -n;
    x->element = cvCreateStructuringElementEx( an*2+1, an*2+1, an, an, x->element_shape, 0 );
    if( n < 0 )
    {
        cvErode(x->src,x->dst,x->element,1);
    }
    else
    {
        cvDilate(x->src,x->dst,x->element,1);
    }
    cvReleaseStructuringElement(&x->element);
}   


static void pdp_opencv_morphology_process_rgb(t_pdp_opencv_morphology *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    int i;
      

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_morphology :: resizing plugins");
	
    	//cv_freeplugins(x);

    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
    	cvReleaseImage( &x->src );
    	cvReleaseImage( &x->dst );
   
	//Create cv_images 
    	x->src = cvCreateImage( cvSize(x->x_width,x->x_height), 8, 3 );
    	x->dst = cvCloneImage(x->src);
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    
    // FEM UNA COPIA DEL PACKET A x->grey->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( x->src->imageData, data, x->x_size*3 );
       
    if (x->mode==1) pdp_opencv_morphology_OpenClose(x,x->pos);
    else pdp_opencv_morphology_ErodeDilate(x,x->pos);

    memcpy( newdata, x->dst->imageData, x->x_size*3 );
 
    return;
}

static void pdp_opencv_morphology_shape(t_pdp_opencv_morphology *x, t_floatarg f)
{
        if( (int)f == 1 )
            x->element_shape = CV_SHAPE_RECT;
        else if( (int)f == 2 )
            x->element_shape = CV_SHAPE_ELLIPSE;
        else if( (int)f == 3 )
            x->element_shape = CV_SHAPE_CROSS;

}

static void pdp_opencv_morphology_mode(t_pdp_opencv_morphology *x, t_floatarg f)
{
    int v = (int)f;
    if (v<0) v=0;
    else if (v>1) v=1;

    x->mode=(int)v;
}

static void pdp_opencv_morphology_pos(t_pdp_opencv_morphology *x, t_floatarg f)
{
    x->pos = (int)f;
}

static void pdp_opencv_morphology_sendpacket(t_pdp_opencv_morphology *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_morphology_process(t_pdp_opencv_morphology *x)
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
    
	/* pdp_opencv_morphology_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_morphology_process_rgb, (void*)pdp_opencv_morphology_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_morphology_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_morphology_input_0(t_pdp_opencv_morphology *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_morphology_process(x);
    }
}

static void pdp_opencv_morphology_free(t_pdp_opencv_morphology *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage( &x->src );
    cvReleaseImage( &x->dst );
}

t_class *pdp_opencv_morphology_class;

void *pdp_opencv_morphology_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_morphology *x = (t_pdp_opencv_morphology *)pd_new(pdp_opencv_morphology_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("pos"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_infosok = 0;

    x->element_shape = CV_SHAPE_RECT;
    x->pos = 0;
    x->mode = 0;
    
    x->element = 0;
    
    x->src = cvCreateImage( cvSize(x->x_width,x->x_height), 8, 3 );
    x->dst = cvCloneImage(x->src);

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_morphology_setup(void)
{

    post( "		pdp_opencv_morphology");
    pdp_opencv_morphology_class = class_new(gensym("pdp_opencv_morphology"), (t_newmethod)pdp_opencv_morphology_new,
    	(t_method)pdp_opencv_morphology_free, sizeof(t_pdp_opencv_morphology), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_morphology_class, (t_method)pdp_opencv_morphology_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_morphology_class, (t_method)pdp_opencv_morphology_pos, gensym("pos"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_morphology_class, (t_method)pdp_opencv_morphology_mode, gensym("mode"),  A_FLOAT, A_NULL);   
    class_addmethod(pdp_opencv_morphology_class, (t_method)pdp_opencv_morphology_shape, gensym("shape"),  A_FLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
