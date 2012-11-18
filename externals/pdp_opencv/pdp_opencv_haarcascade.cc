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
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

#include "pdp.h"

#ifndef _EiC
#include "cv.h"
#endif

#define MAX_MARKERS 50

const char* default_cascade ="./haarcascade_frontalface_alt.xml";

typedef struct pdp_opencv_haarcascade_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_outlet1;
    t_outlet *x_dataout;
    t_atom rlist[4];
    int x_xmark[MAX_MARKERS];
    int x_ymark[MAX_MARKERS];
    int x_found[MAX_MARKERS];
    int x_ftolerance;

    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;

    int x_infosok; 

    float scale_factor;
    int min_neighbors;
    int mode;
    int min_size;

    IplImage *frame, *img;
    CvMemStorage* storage;
    CvHaarClassifierCascade* cascade;
    CvFont font;
    
} t_pdp_opencv_haarcascade;

static int pdp_opencv_haarcascade_mark(t_pdp_opencv_haarcascade *x, t_floatarg fx, t_floatarg fy )
{
  int i;
  
    if ( ( fx < 0.0 ) || ( fx > x->x_width ) || ( fy < 0 ) || ( fy > x->x_height ) )
    {
       return -1;
    }

    for ( i=0; i<MAX_MARKERS; i++)
    {
       if ( x->x_xmark[i] == -1 )
       {
          x->x_xmark[i] = (int)fx;
          x->x_ymark[i] = (int)fy;
          x->x_found[i] = x->x_ftolerance;
          // post( "pdp_opencv_haarcascade : inserted point %d (%d,%d)", i, x->x_xmark[i], x->x_ymark[i] );
          return i;
       }
    }

    post( "pdp_opencv_haarcascade : max markers reached" );
    return -1;
}

static void pdp_opencv_haarcascade_process_rgb(t_pdp_opencv_haarcascade *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    int i, im;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_haarcascade :: resizing plugins");
	
    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
	cvReleaseImage(&x->frame);
    	cvReleaseImage(&x->img);
    
	//create the orig image with new size
        x->frame = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
    	x->img = cvCreateImage(cvSize(x->frame->width,x->frame->height), IPL_DEPTH_8U, 3);

    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    memcpy( x->frame->imageData, data, x->x_size*3 );
    
    if( x->frame->origin == IPL_ORIGIN_TL )
        cvCopy( x->frame, x->img, 0 );
    else
        cvFlip( x->frame, x->img, 0 );
            
    static CvScalar colors[] = 
    {
        {{0,0,255}},
        {{0,128,255}},
        {{0,255,255}},
        {{0,255,0}},
        {{255,128,0}},
        {{255,255,0}},
        {{255,0,0}},
        {{255,0,255}}
    };

    double scale = 1.3;
    IplImage* gray = cvCreateImage( cvSize(x->img->width,x->img->height), 8, 1 );
    IplImage* small_img = cvCreateImage( cvSize( cvRound (x->img->width/scale),
                         cvRound (x->img->height/scale)), 8, 1 );

    cvCvtColor( x->img, gray, CV_BGR2GRAY );
    cvResize( gray, small_img, CV_INTER_LINEAR );
    cvEqualizeHist( small_img, small_img );
    cvClearMemStorage( x->storage );

    if( x->cascade )
    {
        double t = (double)cvGetTickCount();
        CvSeq* faces = cvHaarDetectObjects( small_img, x->cascade, x->storage,
				x->scale_factor, x->min_neighbors, x->mode, cvSize(x->min_size, x->min_size) );
        //t = (double)cvGetTickCount() - t;
        //printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );

        if ( faces && (faces->total > 0 ) )
            outlet_float(x->x_outlet1, (float)faces->total);
        else
            outlet_float(x->x_outlet1, 0.0);

        for ( im=0; im<MAX_MARKERS; im++ )
        {
           if ( x->x_xmark[im] != -1.0 )
           {
             x->x_found[im]--;
           }
        }

        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
          int oi, found;

            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
            CvPoint center;
            int radius;
            center.x = cvRound((r->x + r->width*0.5)*scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);

            found = 0;
            oi = -1;
            for ( im=0; im<MAX_MARKERS; im++ )
            {
              // check if the object is already known
              if ( sqrt( pow(center.x - x->x_xmark[im], 2 ) + pow(center.y - x->x_ymark[im], 2 ) ) <= radius )
              {
                 oi=im;
                 found=1;
                 x->x_found[im] = x->x_ftolerance;
                 x->x_xmark[im] = center.x;
                 x->x_ymark[im] = center.y;
                 break;
              }
            }
            // new object detected
            if ( !found )
            {
               oi = pdp_opencv_haarcascade_mark(x, center.x, center.y );
            }
            char tindex[4];
            sprintf( tindex, "%d", oi );
            cvCircle( x->img, center, radius, colors[oi%8], 3, 8, 0 );
            cvPutText( x->img, tindex, center, &x->font, CV_RGB(255,255,255));        

            SETFLOAT(&x->rlist[0], oi);
            SETFLOAT(&x->rlist[1], center.x);
            SETFLOAT(&x->rlist[2], center.y);
            SETFLOAT(&x->rlist[3], radius);
    	    outlet_list( x->x_dataout, 0, 4, x->rlist );
        }
        // delete lost objects
        for ( im=0; im<MAX_MARKERS; im++ )
        {
           if ( x->x_found[im] < 0 )
           {
             x->x_xmark[im] = -1.0;
             x->x_ymark[im] = -1,0;
             x->x_found[im] = x->x_ftolerance;
             // post( "deleted point %d", im );
           }
        }
    }

    cvReleaseImage( &gray );
    cvReleaseImage( &small_img );
    cvClearMemStorage( x->storage );

    memcpy( newdata, x->img->imageData, x->x_size*3 );
 
    return;
}

static void pdp_opencv_haarcascade_scale_factor(t_pdp_opencv_haarcascade *x, t_floatarg f)
{
	if (f>1) x->scale_factor = f;
}

static void pdp_opencv_haarcascade_min_size(t_pdp_opencv_haarcascade *x, t_floatarg f)
{
	if (f>1) x->min_size = (int)f;
}

static void pdp_opencv_haarcascade_mode(t_pdp_opencv_haarcascade *x, t_floatarg f)
{
	if ((f==0)||(f==1)) x->mode = (int)f;
}

static void pdp_opencv_haarcascade_min_neighbors(t_pdp_opencv_haarcascade *x, t_floatarg f)
{
	if (f>=1) x->min_neighbors = (int)f;
}

static void pdp_opencv_haarcascade_ftolerance(t_pdp_opencv_haarcascade *x, t_floatarg f)
{
	if (f>=1) x->x_ftolerance = (int)f;
}

static void pdp_opencv_haarcascade_delete(t_pdp_opencv_haarcascade *x, t_floatarg findex )
{
  int i;

    if ( ( findex < 1.0 ) || ( findex > 10 ) )
    {
       return;
    }

    x->x_xmark[(int)findex-1] = -1;
    x->x_ymark[(int)findex-1] = -1;
}

static void pdp_opencv_haarcascade_clear(t_pdp_opencv_haarcascade *x )
{
  int i;

    for ( i=0; i<MAX_MARKERS; i++)
    {
      x->x_xmark[i] = -1;
      x->x_ymark[i] = -1;
      x->x_found[i] = x->x_ftolerance;
    }
}

static void pdp_opencv_haarcascade_load(t_pdp_opencv_haarcascade *x, t_symbol *filename)
{
    x->cascade = (CvHaarClassifierCascade*)cvLoad( filename->s_name, 0, 0, 0 );
    if( !x->cascade )
    {
        post( "pdp_opencv_haarcascade: ERROR: Could not load classifier cascade from %s\n", filename->s_name );
    } else  post( "pdp_opencv_haarcascade: Loaded classifier cascade from %s\n", filename->s_name );
}

static void pdp_opencv_haarcascade_sendpacket(t_pdp_opencv_haarcascade *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_haarcascade_process(t_pdp_opencv_haarcascade *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_opencv_haarcascade_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_haarcascade_process_rgb, (void*)pdp_opencv_haarcascade_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_haarcascade_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_haarcascade_input_0(t_pdp_opencv_haarcascade *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_haarcascade_process(x);
    }
}

static void pdp_opencv_haarcascade_free(t_pdp_opencv_haarcascade *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
   
    cvReleaseMemStorage( &x->storage );
    //cvReleaseHaarClassifierCascade(&x->cascade);
    cvReleaseImage(&x->frame);
    cvReleaseImage(&x->img);
}

t_class *pdp_opencv_haarcascade_class;


void *pdp_opencv_haarcascade_new(t_floatarg f)
{
  int i;

    t_pdp_opencv_haarcascade *x = (t_pdp_opencv_haarcascade *)pd_new(pdp_opencv_haarcascade_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet1 = outlet_new(&x->x_obj, &s_anything); 
    x->x_dataout = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_infosok = 0;

    x->scale_factor = 1.1;
    x->min_neighbors = 2;
    x->mode = 0;
    x->min_size = 30;
    x->x_ftolerance = 5;

    for ( i=0; i<MAX_MARKERS; i++ )
    {
      x->x_xmark[i] = -1;
      x->x_ymark[i] = -1;
    }

    x->cascade = (CvHaarClassifierCascade*)cvLoad( default_cascade, 0, 0, 0 );
    if( !x->cascade )
    {
        post( "pdp_opencv_haarcascade : ERROR : Could not load default classifier cascade\n" );
    } else post( "pdp_opencv_haarcascade: Loaded default classifier cascade\n" );
    x->storage = cvCreateMemStorage(0);

    x->frame = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->img = cvCreateImage(cvSize(x->frame->width,x->frame->height), IPL_DEPTH_8U, 3);

    // initialize font
    cvInitFont( &x->font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_haarcascade_setup(void)
{

    post( "		pdp_opencv_haarcascade");
    pdp_opencv_haarcascade_class = class_new(gensym("pdp_opencv_haarcascade"), (t_newmethod)pdp_opencv_haarcascade_new,
    	(t_method)pdp_opencv_haarcascade_free, sizeof(t_pdp_opencv_haarcascade), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_haarcascade_class, (t_method)pdp_opencv_haarcascade_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_haarcascade_class, (t_method)pdp_opencv_haarcascade_load, gensym("load"),  A_SYMBOL, A_NULL );   
    class_addmethod(pdp_opencv_haarcascade_class, (t_method)pdp_opencv_haarcascade_scale_factor, gensym("scale_factor"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_haarcascade_class, (t_method)pdp_opencv_haarcascade_min_neighbors, gensym("min_neighbors"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_haarcascade_class, (t_method)pdp_opencv_haarcascade_mode, gensym("mode"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_haarcascade_class, (t_method)pdp_opencv_haarcascade_min_size, gensym("min_size"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_haarcascade_class, (t_method)pdp_opencv_haarcascade_ftolerance, gensym("ftolerance"), A_FLOAT, A_NULL );
    class_addmethod(pdp_opencv_haarcascade_class, (t_method)pdp_opencv_haarcascade_clear, gensym("clear"), A_NULL );
}

#ifdef __cplusplus
}
#endif

