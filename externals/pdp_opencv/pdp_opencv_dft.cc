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

typedef struct pdp_opencv_dft_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_outlet1;
    int x_packet0;
    int x_packet1;
    int x_packet2;
    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;
    int x_calculate;
    int dft_M;
    int dft_N;

    // The output and temporary images
    IplImage  *image;
    IplImage  *gray;
    IplImage  *input_re;
    IplImage  *input_im;
    IplImage  *input_co;
    CvMat     *dft_A;
    IplImage  *image_re;
    IplImage  *image_im;
    IplImage  *image_mout;
    IplImage  *image_pout;

} t_pdp_opencv_dft;

// rearrange the quadrants of Fourier image so that the origin is at
// the image center

void pdp_opencv_dft_shift_dft(CvArr * src_arr, CvArr * dst_arr )
{
    CvMat *tmp=NULL;
    CvMat q1stub, q2stub;
    CvMat q3stub, q4stub;
    CvMat d1stub, d2stub;
    CvMat d3stub, d4stub;
    CvMat * q1, * q2, * q3, * q4;
    CvMat * d1, * d2, * d3, * d4;

    CvSize size = cvGetSize(src_arr);
    CvSize dst_size = cvGetSize(dst_arr);
    int cx, cy;

    if(dst_size.width != size.width ||
       dst_size.height != size.height){
        error( "pdp_opencv_dft : source and destination arrays must have the same size" );
    }

    if(src_arr==dst_arr){
        tmp = cvCreateMat(size.height/2, size.width/2, cvGetElemType(src_arr));
    }
   
    cx = size.width/2;
    cy = size.height/2; // image center

    q1 = cvGetSubRect( src_arr, &q1stub, cvRect(0,0,cx, cy) );
    q2 = cvGetSubRect( src_arr, &q2stub, cvRect(cx,0,cx,cy) );
    q3 = cvGetSubRect( src_arr, &q3stub, cvRect(cx,cy,cx,cy) );
    q4 = cvGetSubRect( src_arr, &q4stub, cvRect(0,cy,cx,cy) );
    d1 = cvGetSubRect( src_arr, &d1stub, cvRect(0,0,cx,cy) );
    d2 = cvGetSubRect( src_arr, &d2stub, cvRect(cx,0,cx,cy) );
    d3 = cvGetSubRect( src_arr, &d3stub, cvRect(cx,cy,cx,cy) );
    d4 = cvGetSubRect( src_arr, &d4stub, cvRect(0,cy,cx,cy) );

    if(src_arr!=dst_arr){
        if( !CV_ARE_TYPES_EQ( q1, d1 )){
            error( "pdp_opencv_dft : source and destination arrays must have the same format" );
        }
        cvCopy(q3, d1, 0);
        cvCopy(q4, d2, 0);
        cvCopy(q1, d3, 0);
        cvCopy(q2, d4, 0);
    }
    else{
        cvCopy(q3, tmp, 0);
        cvCopy(q1, q3, 0);
        cvCopy(tmp, q1, 0);
        cvCopy(q4, tmp, 0);
        cvCopy(q2, q4, 0);
        cvCopy(tmp, q2, 0);
    }
    if(src_arr==dst_arr){
        cvReleaseMat( &tmp );
    }
}

static void pdp_opencv_dft_process_rgb(t_pdp_opencv_dft *x)
{
  t_pdp     *header = pdp_packet_header(x->x_packet0);
  short int *data   = (short int *)pdp_packet_data(x->x_packet0);
  t_pdp     *newheader = pdp_packet_header(x->x_packet1);
  short int *newdata = (short int *)pdp_packet_data(x->x_packet1);
  t_pdp     *phaseheader = pdp_packet_header(x->x_packet2);
  short int *phasedata = (short int *)pdp_packet_data(x->x_packet2);
  CvMat tmp;
  double m,M;
  int px,py;
    
    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {
    	post("pdp_opencv_dft :: resizing");
	
    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
    	cvReleaseImage( &x->image );
    	//cvReleaseImage( &x->gray );
    	cvReleaseImage( &x->input_re );
    	cvReleaseImage( &x->input_im );
    	cvReleaseImage( &x->input_co );
    	cvReleaseMat( &x->dft_A );
    	//cvReleaseImage( &x->image_re );
    	//cvReleaseImage( &x->image_im );
    	//cvReleaseImage( &x->image_mout );
    	//cvReleaseImage( &x->image_pout );
    
    	x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    	x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
        x->input_re = cvCreateImage( cvGetSize(x->image), IPL_DEPTH_64F, 1);
        x->input_im = cvCreateImage( cvGetSize(x->image), IPL_DEPTH_64F, 1);
        x->input_co = cvCreateImage( cvGetSize(x->image), IPL_DEPTH_64F, 2);
        x->dft_M = cvGetOptimalDFTSize( x->gray->height - 1 );
        x->dft_N = cvGetOptimalDFTSize( x->gray->width - 1 );
        x->dft_A = cvCreateMat( x->dft_M, x->dft_N, CV_64FC2 );
        x->image_re = cvCreateImage( cvSize(x->dft_N, x->dft_M), IPL_DEPTH_64F, 1);
        x->image_im = cvCreateImage( cvSize(x->dft_N, x->dft_M), IPL_DEPTH_64F, 1);
        x->image_mout = cvCreateImage( cvSize(x->image->width, x->image->height), IPL_DEPTH_8U, 1);
        x->image_pout = cvCreateImage( cvSize(x->image->width, x->image->height), IPL_DEPTH_8U, 1);
    }
    
    newheader->info.image.encoding = PDP_BITMAP_GREY;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( x->image->imageData, data, x->x_size*3 );
    cvCvtColor(x->image, x->gray, CV_BGR2GRAY);

    if ( x->x_calculate )
    {
      // discrete fourier transform
      cvScale(x->gray, x->input_re, 1.0, 0.0);
      cvZero(x->input_im);
      cvMerge(x->input_re, x->input_im, NULL, NULL, x->input_co);
  
      // copy A to dft_A and pad dft_A with zeros
      cvGetSubRect( x->dft_A, &tmp, cvRect(0,0, x->gray->width, x->gray->height));
      cvCopy( x->input_co, &tmp, NULL );
      if( x->dft_A->cols > x->gray->width )
      {
          cvGetSubRect( x->dft_A, &tmp, cvRect(x->gray->width,0, x->dft_A->cols - x->gray->width, x->gray->height));
          cvZero( &tmp );
      }
  
      // no need to pad bottom part of dft_A with zeros because of
      // use nonzero_rows parameter in cvDFT() call below
      cvDFT( x->dft_A, x->dft_A, CV_DXT_FORWARD, x->input_co->height );
  
      // Split Fourier in real and imaginary parts
      cvSplit( x->dft_A, x->image_re, x->image_im, 0, 0 );
  
      // calculate phase
      for( py=0; py<x->image_re->height; py++ ) {
         double* ptrr = (double*) ( x->image_re->imageData + py * x->image_re->widthStep);
         double* ptri = (double*) ( x->image_im->imageData + py * x->image_im->widthStep);
         float* ptrp = (float*) ( x->image_pout->imageData + py * x->image_pout->widthStep);
         for( px=0; px<x->image_re->width; px++ ) {
           (*(ptrp+px)) = cvFastArctan( (float)*(ptri+px), (float)*(ptrr+px) );
         }
      }

      // Compute the magnitude of the spectrum Mag = sqrt(Re^2 + Im^2)
      cvPow( x->image_re, x->image_re, 2.0);
      cvPow( x->image_im, x->image_im, 2.0);
      cvAdd( x->image_re, x->image_im, x->image_re, NULL);
      cvPow( x->image_re, x->image_re, 0.5 );
  
      // Compute log(1 + Mag)
      cvAddS( x->image_re, cvScalarAll(1.0), x->image_re, NULL ); // 1 + Mag
      cvLog( x->image_re, x->image_re ); // log(1 + Mag)
  
      // Rearrange the quadrants of Fourier image so that the origin is at
      // the image center
      pdp_opencv_dft_shift_dft( x->image_re, x->image_re );
  
      // normalize image
      cvMinMaxLoc(x->image_re, &m, &M, NULL, NULL, NULL);
      cvScale(x->image_re, x->image_re, 255.0/(M-m), 255.0*(-m)/(M-m));
  
      for( py=0; py<x->image_re->height; py++ ) {
         double* ptri = (double*) ( x->image_re->imageData + py * x->image_re->widthStep);
         unsigned char* ptrp = (unsigned char*) ( x->image_mout->imageData + py * x->image_mout->widthStep);
         for( px=0; px<x->image_re->width; px++ ) {
           if ( *(ptrp+px) > 255.0 ) post( "pdp_opencv_dft : error value over 255" );
           (*(ptrp+px)) = (unsigned char)( (*(ptri+px)) );
         }
      }
 
      x->x_calculate=0;
    }

    cvCvtColor(x->image_mout, x->image, CV_GRAY2RGB);
    memcpy( newdata, x->image->imageData, x->x_size*3 );
    cvCvtColor(x->image_pout, x->image, CV_GRAY2RGB);
    memcpy( phasedata, x->image->imageData, x->x_size*3 );

    return;
}

static void pdp_opencv_dft_sendpacket(t_pdp_opencv_dft *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet1, &x->x_packet2);

    pdp_packet_mark_unused(x->x_packet1);
    pdp_packet_mark_unused(x->x_packet2);
}

static void pdp_opencv_dft_process(t_pdp_opencv_dft *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_opencv_dft_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            x->x_packet2 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_dft_process_rgb, (void*)pdp_opencv_dft_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_dft_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_dft_calculate(t_pdp_opencv_dft *x)
{
    x->x_calculate=1;
}

static void pdp_opencv_dft_input_0(t_pdp_opencv_dft *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_dft_process(x);
    }
}

static void pdp_opencv_dft_free(t_pdp_opencv_dft *x)
{
  int i;

    //destroy cv structures
    cvReleaseImage( &x->image );
    //cvReleaseImage( &x->gray );
    cvReleaseImage( &x->input_re );
    cvReleaseImage( &x->input_im );
    cvReleaseImage( &x->input_co );
    cvReleaseMat( &x->dft_A );
    //cvReleaseImage( &x->image_re );
    //cvReleaseImage( &x->image_im );
    //cvReleaseImage( &x->image_mout );
    //cvReleaseImage( &x->image_pout );

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
}

t_class *pdp_opencv_dft_class;


void *pdp_opencv_dft_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_dft *x = (t_pdp_opencv_dft *)pd_new(pdp_opencv_dft_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet1 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_packet2 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->gray = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    x->input_re = cvCreateImage( cvGetSize(x->image), IPL_DEPTH_64F, 1);
    x->input_im = cvCreateImage( cvGetSize(x->image), IPL_DEPTH_64F, 1);
    x->input_co = cvCreateImage( cvGetSize(x->image), IPL_DEPTH_64F, 2);
    x->dft_M = cvGetOptimalDFTSize( x->gray->height - 1 );
    x->dft_N = cvGetOptimalDFTSize( x->gray->width - 1 );
    x->dft_A = cvCreateMat( x->dft_M, x->dft_N, CV_64FC2 );
    x->image_re = cvCreateImage( cvSize(x->dft_N, x->dft_M), IPL_DEPTH_64F, 1);
    x->image_im = cvCreateImage( cvSize(x->dft_N, x->dft_M), IPL_DEPTH_64F, 1);
    x->image_mout = cvCreateImage( cvSize(x->dft_N, x->dft_M), IPL_DEPTH_8U, 1);
    x->image_pout = cvCreateImage( cvSize(x->dft_N, x->dft_M), IPL_DEPTH_8U, 1);

    // calculate first dft
    x->x_calculate=1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_dft_setup(void)
{

    post( "		pdp_opencv_dft");
    pdp_opencv_dft_class = class_new(gensym("pdp_opencv_dft"), (t_newmethod)pdp_opencv_dft_new,
    	(t_method)pdp_opencv_dft_free, sizeof(t_pdp_opencv_dft), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_dft_class, (t_method)pdp_opencv_dft_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_dft_class, (t_method)pdp_opencv_dft_calculate, gensym("bang"), A_NULL);

}

#ifdef __cplusplus
}
#endif
