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

typedef struct pdp_opencv_hu_compare_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_patternout;
    t_outlet *x_dataout;
    t_outlet *x_posout;

    int x_packet0;
    int x_packet1;
    int x_packet2;
    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;

    int x_method;
    int x_minsize;
    float x_cdistance;
    t_atom    rlist[5];

    IplImage *image, *gray;
    IplImage *imager, *grayr;

    CvMemStorage *x_storage;
    CvSeq        *x_bcontourr;

} t_pdp_opencv_hu_compare;

static void pdp_opencv_hu_compare_process_rgb(t_pdp_opencv_hu_compare *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    double dist = 100.0, ndist;
    int i = 0;                   // Indicator of cycles.
    CvSeq *contourl=NULL, *contourlp;
    CvRect rect;
    CvMemStorage *mstorage;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_hu_compare :: resizing");

    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->gray);
	cvReleaseImage(&x->imager);
    	cvReleaseImage(&x->grayr);
    
	//create the orig image with new size
        x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    	x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
        x->imager = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    	x->grayr = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( x->image->imageData, data, x->x_size*3 );

    mstorage = cvCreateMemStorage(0);

    // Convert to grayscale
    cvCvtColor(x->image, x->gray, CV_BGR2GRAY);

    cvFindContours( x->gray, mstorage, &contourl, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );

    i=0;
    if ( contourl && x->x_bcontourr )
    {
      contourlp=contourl;
      for( ; contourlp != 0; contourlp = contourlp->h_next )
      {
        rect = cvContourBoundingRect( contourlp, 1);
        if ( rect.width*rect.height > x->x_minsize && rect.width*rect.height < (x->x_width-2)*(x->x_height-2))
        {
           ndist = cvMatchShapes( x->x_bcontourr, contourlp, x->x_method, 0 );
           if ( ndist < dist ) dist = ndist;
           if ( ndist < x->x_cdistance )
           {
             cvRectangle( x->gray, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(255,255,255), 2, 8 , 0 );
             cvDrawContours( x->gray, contourlp, CV_RGB(255,255,255), CV_RGB(255,255,255), 0, 1, 8, cvPoint(0,0) );
             SETFLOAT(&x->rlist[0], i++);
             SETFLOAT(&x->rlist[1], rect.x);
             SETFLOAT(&x->rlist[2], rect.y);
             SETFLOAT(&x->rlist[3], rect.width);
             SETFLOAT(&x->rlist[4], rect.height);
             outlet_list( x->x_posout, 0, 5, x->rlist );
           }
           else
           {
             cvRectangle( x->gray, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(128,128,128), 2, 8 , 0 );
             cvDrawContours( x->gray, contourlp, CV_RGB(128,128,128), CV_RGB(128,128,128), 0, 1, 8, cvPoint(0,0) );

           }
        }
      }
    } 

    if ( dist < 100.00 ) outlet_float( x->x_dataout, dist );

    cvCvtColor(x->gray, x->image, CV_GRAY2BGR);
    memcpy( newdata, x->image->imageData, x->x_size*3 );
 
    cvReleaseMemStorage(&mstorage);

    return;
}

static void pdp_opencv_hu_compare_sendpacket(t_pdp_opencv_hu_compare *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_hu_compare_process(t_pdp_opencv_hu_compare *x)
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
    
	/* pdp_opencv_hu_compare_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_hu_compare_process_rgb, (void*)pdp_opencv_hu_compare_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_hu_compare_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_hu_compare_input_0(t_pdp_opencv_hu_compare *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_hu_compare_process(x);
    }
}

static void pdp_opencv_hu_compare_input_1(t_pdp_opencv_hu_compare *x, t_symbol *s, t_floatarg fpacket)
{
  t_pdp     *rightheader;
  short int *rightdata; 
  CvSeq     *contourr = NULL;
  int       i, size;
  CvRect    rect;

    if (s == gensym("register_rw") && ( x->x_bcontourr == NULL ) ) 
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet2, (int)fpacket, pdp_gensym((char*)"bitmap/rgb/*") );

       rightheader = pdp_packet_header(x->x_packet2);
       rightdata = (short int *)pdp_packet_data(x->x_packet2);
       if ( !rightheader || !rightdata )
       {
          post( "pdp_opencv_hu_compare : error : null packet" );
          return;
       }

       if ((x->x_width != (t_int)rightheader->info.image.width) || 
           (x->x_height != (t_int)rightheader->info.image.height)) 
       {
          post( "pdp_opencv_hu_compare : error : wrong right image size" );
          return;
       }

       memcpy( x->imager->imageData, rightdata, x->x_size*3 );
       // Convert to grayscale
       cvCvtColor(x->imager, x->grayr, CV_BGR2GRAY);

       // calculate the biggest contour and output packet
       try {
          cvFindContours( x->grayr, x->x_storage, &contourr, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );
       }
       catch(...) {
          post( "pdp_opencv_hu_compare : error calculating contours" );
          return;
       }

       if ( contourr )
       {
         size=0;
         for( ; contourr != 0; contourr = contourr->h_next )
         {
            rect = cvContourBoundingRect( contourr, 1);
            if ( rect.width*rect.height > size && rect.width*rect.height < (x->x_width-2)*(x->x_height-2))
            {
               x->x_bcontourr = contourr;
               size = rect.width*rect.height;
            }
         }
         rect = cvContourBoundingRect( x->x_bcontourr, 1);
         cvRectangle( x->grayr, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(255,255,255), 2, 8 , 0 );
         cvDrawContours( x->grayr, x->x_bcontourr, CV_RGB(255,255,255), CV_RGB(128,128,128), 0, 1, 8, cvPoint(0,0) );
       }

       cvCvtColor(x->grayr, x->imager, CV_GRAY2BGR);
       memcpy( rightdata, x->imager->imageData, x->x_size*3 );

       /* unregister and propagate if valid dest packet */
       pdp_packet_pass_if_valid(x->x_patternout, &x->x_packet2);
    }
}

    /* if this is a register_ro message or register_rw message, register with packet factory */
static void pdp_opencv_hu_compare_free(t_pdp_opencv_hu_compare *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage(&x->image);
    cvReleaseImage(&x->gray);
    cvReleaseImage(&x->imager);
    cvReleaseImage(&x->grayr);

    cvReleaseMemStorage(&x->x_storage);
}

t_class *pdp_opencv_hu_compare_class;


void *pdp_opencv_hu_compare_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_hu_compare *x = (t_pdp_opencv_hu_compare *)pd_new(pdp_opencv_hu_compare_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("pdp"), gensym("pdp1"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_patternout = outlet_new(&x->x_obj, &s_anything); 
    x->x_dataout = outlet_new(&x->x_obj, &s_anything); 
    x->x_posout = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_packet2 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_method = CV_CONTOURS_MATCH_I1;

    x->x_storage = cvCreateMemStorage(0);

    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    x->imager = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->grayr = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);

    x->x_bcontourr = NULL;
    x->x_minsize = 10*10;
    x->x_cdistance = 0.1;

    return (void *)x;
}

static void pdp_opencv_hu_compare_method(t_pdp_opencv_hu_compare *x, t_floatarg f)
{
    if ((int)f==CV_CONTOURS_MATCH_I1) 
    {
       post( "pdp_opencv_hu_compare : method set to CV_CONTOURS_MATCH_I1" );
       x->x_method = (int)f;
    }
    if ((int)f==CV_CONTOURS_MATCH_I2) 
    {
       post( "pdp_opencv_hu_compare : method set to CV_CONTOURS_MATCH_I2" );
       x->x_method = (int)f;
    }
    if ((int)f==CV_CONTOURS_MATCH_I3) 
    {
       post( "pdp_opencv_hu_compare : method set to CV_CONTOURS_MATCH_I3" );
       x->x_method = (int)f;
    }
}

static void pdp_opencv_hu_compare_minsize(t_pdp_opencv_hu_compare *x, t_floatarg f)
{
    if ( (int)f > 0 )
    {
       x->x_minsize = (int)f;
    }
}

static void pdp_opencv_hu_compare_criteria(t_pdp_opencv_hu_compare *x, t_floatarg f)
{
    if ( f > 0.0 )
    {
       x->x_cdistance = f;
    }
}


static void pdp_opencv_hu_compare_clear(t_pdp_opencv_hu_compare *x)
{
    x->x_bcontourr = NULL;
}

#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_hu_compare_setup(void)
{

    post( "		pdp_opencv_hu_compare");
    pdp_opencv_hu_compare_class = class_new(gensym("pdp_opencv_hu_compare"), (t_newmethod)pdp_opencv_hu_compare_new,
    	(t_method)pdp_opencv_hu_compare_free, sizeof(t_pdp_opencv_hu_compare), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_hu_compare_class, (t_method)pdp_opencv_hu_compare_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_hu_compare_class, (t_method)pdp_opencv_hu_compare_input_1, gensym("pdp1"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_hu_compare_class, (t_method)pdp_opencv_hu_compare_method, gensym("method"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_hu_compare_class, (t_method)pdp_opencv_hu_compare_minsize, gensym("minsize"), A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_hu_compare_class, (t_method)pdp_opencv_hu_compare_clear, gensym("clear"), A_NULL);
    class_addmethod(pdp_opencv_hu_compare_class, (t_method)pdp_opencv_hu_compare_criteria, gensym("criteria"), A_DEFFLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
