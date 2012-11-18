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



typedef struct pdp_opencv_distrans_struct
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
    int build_voronoi;
    int mask_size;

    // The output and temporary images
    IplImage* dist;
    IplImage* dist8u1;
    IplImage* dist8u2;
    IplImage* dist8u;
    IplImage* dist32s;

    IplImage* image;
    IplImage* gray;
    IplImage* edge;
    IplImage* labels;

    
} t_pdp_opencv_distrans;



static void pdp_opencv_distrans_process_rgb(t_pdp_opencv_distrans *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    static const uchar colors[][3] = 
    {
        {0,0,0},
        {255,0,0},
        {255,128,0},
        {255,255,0},
        {0,255,0},
        {0,128,255},
        {0,255,255},
        {0,0,255},
        {255,0,255}
    };
    
    int msize = x->mask_size;
      

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_distrans :: resizing plugins");
	
    	//cv_freeplugins(x);

    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
    	cvReleaseImage( &x->image );
    	cvReleaseImage( &x->gray );
    	cvReleaseImage( &x->edge );
    	cvReleaseImage( &x->dist );
    	cvReleaseImage( &x->dist8u );
    	cvReleaseImage( &x->dist8u1 );
    	cvReleaseImage( &x->dist8u2 );
    	cvReleaseImage( &x->dist32s );
    	cvReleaseImage( &x->labels );
    
    	x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    	x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    	x->dist = cvCreateImage( cvGetSize(x->gray), IPL_DEPTH_32F, 1 );
    	x->dist8u1 = cvCloneImage( x->gray );
    	x->dist8u2 = cvCloneImage( x->gray );
    	x->dist8u = cvCreateImage( cvGetSize(x->gray), IPL_DEPTH_8U, 3 );
    	x->dist32s = cvCreateImage( cvGetSize(x->gray), IPL_DEPTH_32S, 1 );
    	x->edge = cvCloneImage( x->gray );
    	x->labels = cvCreateImage( cvGetSize(x->gray), IPL_DEPTH_32S, 1 );
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    memcpy( x->image->imageData, data, x->x_size*3 );
    cvCvtColor(x->image, x->gray, CV_BGR2GRAY);
    
    cvThreshold( x->gray, x->edge, (float)x->edge_thresh, (float)x->edge_thresh, CV_THRESH_BINARY );

    if( x->build_voronoi )
        msize = CV_DIST_MASK_5;

    cvDistTransform( x->edge, x->dist, CV_DIST_L2, msize, NULL, x->build_voronoi ? x->labels : NULL );

    if( !x->build_voronoi )
    {
        // begin "painting" the distance transform result
        cvConvertScale( x->dist, x->dist, 5000.0, 0 );
        cvPow( x->dist, x->dist, 0.5 );
    
        cvConvertScale( x->dist, x->dist32s, 1.0, 0.5 );
        cvAndS( x->dist32s, cvScalarAll(255), x->dist32s, 0 );
        cvConvertScale( x->dist32s, x->dist8u1, 1, 0 );
        cvConvertScale( x->dist32s, x->dist32s, -1, 0 );
        cvAddS( x->dist32s, cvScalarAll(255), x->dist32s, 0 );
        cvConvertScale( x->dist32s, x->dist8u2, 1, 0 );
        cvMerge( x->dist8u1, x->dist8u2, x->dist8u2, 0, x->dist8u );
        // end "painting" the distance transform result
    }
    else
    {
        int i, j;
        for( i = 0; i < x->labels->height; i++ )
        {
            int* ll = (int*)(x->labels->imageData + i*x->labels->widthStep);
            float* dd = (float*)(x->dist->imageData + i*x->dist->widthStep);
            uchar* d = (uchar*)(x->dist8u->imageData + i*x->dist8u->widthStep);
            for( j = 0; j < x->labels->width; j++ )
            {
                int idx = ll[j] == 0 || dd[j] == 0 ? 0 : (ll[j]-1)%8 + 1;
                int b = cvRound(colors[idx][0]);
                int g = cvRound(colors[idx][1]);
                int r = cvRound(colors[idx][2]);
                d[j*3] = (uchar)b;
                d[j*3+1] = (uchar)g;
                d[j*3+2] = (uchar)r;
            }
        }
    }
    
    memcpy( newdata, x->dist8u->imageData, x->x_size*3 );
 
    return;
}

static void pdp_opencv_distrans_type(t_pdp_opencv_distrans *x, t_floatarg f)
{
        if( (int)f == 3 )
            x->mask_size = CV_DIST_MASK_3;
        else if( (int)f == 5 )
            x->mask_size = CV_DIST_MASK_5;
        else if( (int)f == 0 )
            x->mask_size = CV_DIST_MASK_PRECISE;

}

static void pdp_opencv_distrans_voronoi(t_pdp_opencv_distrans *x, t_floatarg f)
{
    int v = (int)f;
    if (v<0) v=0;
    else if (v>1) v=1;

    x->build_voronoi=(int)v;
}

static void pdp_opencv_distrans_thresh(t_pdp_opencv_distrans *x, t_floatarg f)
{
	x->edge_thresh = (int)f;
}

static void pdp_opencv_distrans_sendpacket(t_pdp_opencv_distrans *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_distrans_process(t_pdp_opencv_distrans *x)
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
    
	/* pdp_opencv_distrans_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_distrans_process_rgb, (void*)pdp_opencv_distrans_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_distrans_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_distrans_input_0(t_pdp_opencv_distrans *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_distrans_process(x);
    }
}

static void pdp_opencv_distrans_free(t_pdp_opencv_distrans *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    //cv_freeplugins(x);
    
    	//Destroy cv_images
    cvReleaseImage( &x->image );
    cvReleaseImage( &x->gray );
    cvReleaseImage( &x->edge );
    cvReleaseImage( &x->dist );
    cvReleaseImage( &x->dist8u );
    cvReleaseImage( &x->dist8u1 );
    cvReleaseImage( &x->dist8u2 );
    cvReleaseImage( &x->dist32s );
    cvReleaseImage( &x->labels );
}

t_class *pdp_opencv_distrans_class;


void *pdp_opencv_distrans_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_distrans *x = (t_pdp_opencv_distrans *)pd_new(pdp_opencv_distrans_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("edge_thresh"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    //load the plugins
    x->x_infosok = 0;
    //cv_loadplugins(x, FF_PLUGIN_DIR);

    //pdp_opencv_distrans_plugin(x, f);
    x->edge_thresh = 100;
    x->build_voronoi = 0;
    x->mask_size = CV_DIST_MASK_5;
    


    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    cvCvtColor(x->image, x->gray, CV_BGR2GRAY);
    x->dist = cvCreateImage( cvSize(x->x_width,x->x_height), IPL_DEPTH_32F, 1 );
    x->dist8u1 = cvCloneImage( x->gray );
    x->dist8u2 = cvCloneImage( x->gray );
    x->dist8u = cvCreateImage( cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3 );
    x->dist32s = cvCreateImage( cvSize(x->x_width,x->x_height), IPL_DEPTH_32S, 1 );
    x->edge = cvCloneImage( x->gray );
    x->labels = cvCreateImage( cvSize(x->x_width,x->x_height), IPL_DEPTH_32S, 1 );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_distrans_setup(void)
{

    post( "		pdp_opencv_distrans");
    pdp_opencv_distrans_class = class_new(gensym("pdp_opencv_distrans"), (t_newmethod)pdp_opencv_distrans_new,
    	(t_method)pdp_opencv_distrans_free, sizeof(t_pdp_opencv_distrans), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_distrans_class, (t_method)pdp_opencv_distrans_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_distrans_class, (t_method)pdp_opencv_distrans_thresh, gensym("edge_thresh"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_distrans_class, (t_method)pdp_opencv_distrans_type, gensym("type"),  A_FLOAT, A_NULL);   
    class_addmethod(pdp_opencv_distrans_class, (t_method)pdp_opencv_distrans_voronoi, gensym("voronoi"),  A_FLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
