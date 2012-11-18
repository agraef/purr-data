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
#include <math.h>

#include "pdp.h"

#ifndef _EiC
#include "cv.h"
#endif

#define MAX_MARKERS 100

typedef struct pdp_opencv_contours_convexhull_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_dataout;
    t_outlet *x_countout;
    t_atom    rlist[5];
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    // contours retrieval mode
    int x_cmode;
    // contours retrieval method
    int x_cmethod;
    int x_accuracy;

    int x_width;
    int x_height;
    int x_size;

    int x_nightmode;  // don't show the original image

    IplImage *image, *gray, *cnt_img;
    
} t_pdp_opencv_contours_convexhull;

static void pdp_opencv_contours_convexhull_process_rgb(t_pdp_opencv_contours_convexhull *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    char tindex[4];
    int i = 0;                   // Indicator of cycles.
    int j=0;
    int k = 0;
    int area = 0;
    int selected = -1;
    CvSeq* first_contour;
    CvSeq* defects;
    CvSeq* contours;
    int* hull;
    int hullsize;
    CvPoint* PointArray;
    CvConvexityDefect* defectArray;
    CvMemStorage* stor02;
    CvMemStorage* stor03;

    stor02 = cvCreateMemStorage(0);
    stor03 = cvCreateMemStorage(0);

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_contours_convexhull :: resizing plugins");
	

    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->gray);
    	cvReleaseImage(&x->cnt_img);
    
	//create the orig image with new size
        x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
    	x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    	x->cnt_img = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 3);
    
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( x->image->imageData, data, x->x_size*3 );
    
    // Convert to grayscale
    cvCvtColor(x->image, x->gray, CV_BGR2GRAY);

    // Retrieval mode.
    // CV_RETR_EXTERNAL || CV_RETR_LIST || CV_RETR_CCOMP || CV_RETR_TREE
    // Approximation method.
    // CV_CHAIN_CODE || CV_CHAIN_APPROX_NONE || CV_CHAIN_APPROX_SIMPLE || CV_CHAIN_APPROX_TC89_L1 || CV_CHAIN_APPROX_TC89_KCOS || CV_LINK_RUNS

    cvFindContours( x->gray, stor02, &contours, sizeof(CvContour), x->x_cmode, x->x_cmethod, cvPoint(0,0) );
    if (contours) contours = cvApproxPoly( contours, sizeof(CvContour), stor02, CV_POLY_APPROX_DP, x->x_accuracy, 1 );

    cvCopy(x->image, x->cnt_img, NULL);
    if ( x->x_nightmode )
    {
       cvZero( x->cnt_img );
    }

    first_contour = contours;

    // searching for biggest contour
    for( ; contours != 0; contours = contours->h_next )
    {
      CvRect rect;
      rect = cvContourBoundingRect(contours, 1);
      if  ( (rect.width*rect.height) > area )
      {
           selected = i;
           area = rect.width*rect.height;
      }
      i++;
    }

    contours = first_contour;
    for( ; contours != 0; contours = contours->h_next )
    {
      int count = contours->total; // This is number point in contour
      CvPoint center;
      CvSize size;
      CvRect rect;

      rect = cvContourBoundingRect( contours, 1);
      if ( (k==selected) ) 
      {
        PointArray = (CvPoint*)malloc( count*sizeof(CvPoint) );
        hull = (int*)malloc(sizeof(int)*count);
        cvCvtSeqToArray(contours, PointArray, CV_WHOLE_SEQ);

        cvConvexHull( PointArray,
                      count,
                      NULL,
                      CV_COUNTER_CLOCKWISE,
                      hull,
                      &hullsize);

        outlet_float( x->x_countout, hullsize );

        t_atom rlist[hullsize*2];

        // Draw convex hull for current contour.
        for(i=0; i<hullsize-1; i++)
        {
            cvLine(x->cnt_img, PointArray[hull[i]], PointArray[hull[i+1]],CV_RGB(0,0,255),3, CV_AA, 0 );
            SETFLOAT(&rlist[j], PointArray[hull[i]].x);
            SETFLOAT(&rlist[j+1], PointArray[hull[i]].y);
            j = j + 2;
        }

        cvLine(x->cnt_img, PointArray[hull[hullsize-1]], PointArray[hull[0]],CV_RGB(0,0,255),3, CV_AA, 0 );
        SETFLOAT(&rlist[j], PointArray[hull[i]].x);
        SETFLOAT(&rlist[j+1], PointArray[hull[i]].y);
        outlet_list( x->x_dataout, 0, hullsize*2, rlist );

        free(PointArray);
        free(hull);
      }
      k++;
    }

    cvReleaseMemStorage( &stor03 );
    cvReleaseMemStorage( &stor02 );
    
    memcpy( newdata, x->cnt_img->imageData, x->x_size*3 );
 
    return;
}

static void pdp_opencv_contours_convexhull_nightmode(t_pdp_opencv_contours_convexhull *x, t_floatarg f)
{
    if  ( ((int)f==1) || ((int)f==0) ) x->x_nightmode = (int)f;
}

static void pdp_opencv_contours_convexhull_accuracy(t_pdp_opencv_contours_convexhull *x, t_floatarg f)
{
    if  ((int)f>=0) x->x_accuracy = (int)f;
}

static void pdp_opencv_contours_convexhull_cmode(t_pdp_opencv_contours_convexhull *x, t_floatarg f)
{
    // CV_RETR_EXTERNAL || CV_RETR_LIST || CV_RETR_CCOMP || CV_RETR_TREE
    int mode = (int)f;

    if ( mode == CV_RETR_EXTERNAL )
    {
       x->x_cmode = CV_RETR_EXTERNAL;
       post( "pdp_opencv_contours_convexhull : mode set to CV_RETR_EXTERNAL" );
    }
    if ( mode == CV_RETR_LIST )
    {
       x->x_cmode = CV_RETR_LIST;
       post( "pdp_opencv_contours_convexhull : mode set to CV_RETR_LIST" );
    }
    if ( mode == CV_RETR_CCOMP )
    {
       x->x_cmode = CV_RETR_CCOMP;
       post( "pdp_opencv_contours_convexhull : mode set to CV_RETR_CCOMP" );
    }
    if ( mode == CV_RETR_TREE )
    {
       x->x_cmode = CV_RETR_TREE;
       post( "pdp_opencv_contours_convexhull : mode set to CV_RETR_TREE" );
    }
}

static void pdp_opencv_contours_convexhull_cmethod(t_pdp_opencv_contours_convexhull *x, t_floatarg f)
{
  int method = (int)f;

    // CV_CHAIN_CODE || CV_CHAIN_APPROX_NONE || CV_CHAIN_APPROX_SIMPLE || CV_CHAIN_APPROX_TC89_L1 || CV_CHAIN_APPROX_TC89_KCOS || CV_LINK_RUNS
    if ( method == CV_CHAIN_CODE )
    {
       post( "pdp_opencv_contours_convexhull : not supported method : CV_CHAIN_CODE" );
    }
    if ( method == CV_CHAIN_APPROX_NONE )
    {
       x->x_cmethod = CV_CHAIN_APPROX_NONE;
       post( "pdp_opencv_contours_convexhull : method set to CV_CHAIN_APPROX_NONE" );
    }
    if ( method == CV_CHAIN_APPROX_SIMPLE )
    {
       x->x_cmethod = CV_CHAIN_APPROX_SIMPLE;
       post( "pdp_opencv_contours_convexhull : method set to CV_CHAIN_APPROX_SIMPLE" );
    }
    if ( method == CV_CHAIN_APPROX_TC89_L1 )
    {
       x->x_cmethod = CV_CHAIN_APPROX_TC89_L1;
       post( "pdp_opencv_contours_convexhull : method set to CV_CHAIN_APPROX_TC89_L1" );
    }
    if ( method == CV_CHAIN_APPROX_TC89_KCOS )
    {
       x->x_cmethod = CV_CHAIN_APPROX_TC89_KCOS;
       post( "pdp_opencv_contours_convexhull : method set to CV_CHAIN_APPROX_TC89_KCOS" );
    }
    if ( ( method == CV_LINK_RUNS ) && ( x->x_cmode == CV_RETR_LIST ) )
    {
       x->x_cmethod = CV_LINK_RUNS;
       post( "pdp_opencv_contours_convexhull : method set to CV_LINK_RUNS" );
    }
}

static void pdp_opencv_contours_convexhull_sendpacket(t_pdp_opencv_contours_convexhull *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_contours_convexhull_process(t_pdp_opencv_contours_convexhull *x)
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
    
	/* pdp_opencv_contours_convexhull_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_contours_convexhull_process_rgb, (void*)pdp_opencv_contours_convexhull_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_contours_convexhull_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_contours_convexhull_input_0(t_pdp_opencv_contours_convexhull *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_contours_convexhull_process(x);
    }
}

static void pdp_opencv_contours_convexhull_free(t_pdp_opencv_contours_convexhull *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage(&x->image);
    cvReleaseImage(&x->gray);
    cvReleaseImage(&x->cnt_img);
}

t_class *pdp_opencv_contours_convexhull_class;


void *pdp_opencv_contours_convexhull_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_contours_convexhull *x = (t_pdp_opencv_contours_convexhull *)pd_new(pdp_opencv_contours_convexhull_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_countout = outlet_new(&x->x_obj, &s_float); 
    x->x_dataout = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_cmode   = CV_RETR_LIST;
    x->x_cmethod = CV_CHAIN_APPROX_SIMPLE;

    x->x_nightmode = 0;
    x->x_accuracy = 3;

    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    x->cnt_img = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 3);

    //contours = 0;
    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_contours_convexhull_setup(void)
{

    post( "		pdp_opencv_contours_convexhull");
    pdp_opencv_contours_convexhull_class = class_new(gensym("pdp_opencv_contours_convexhull"), (t_newmethod)pdp_opencv_contours_convexhull_new,
    	(t_method)pdp_opencv_contours_convexhull_free, sizeof(t_pdp_opencv_contours_convexhull), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_contours_convexhull_class, (t_method)pdp_opencv_contours_convexhull_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_contours_convexhull_class, (t_method)pdp_opencv_contours_convexhull_cmode, gensym("mode"), A_FLOAT, A_NULL );
    class_addmethod(pdp_opencv_contours_convexhull_class, (t_method)pdp_opencv_contours_convexhull_cmethod, gensym("method"), A_FLOAT, A_NULL );
    class_addmethod(pdp_opencv_contours_convexhull_class, (t_method)pdp_opencv_contours_convexhull_nightmode, gensym("nightmode"), A_FLOAT, A_NULL );
    class_addmethod(pdp_opencv_contours_convexhull_class, (t_method)pdp_opencv_contours_convexhull_accuracy, gensym("accuracy"), A_FLOAT, A_NULL );

}

#ifdef __cplusplus
}
#endif
