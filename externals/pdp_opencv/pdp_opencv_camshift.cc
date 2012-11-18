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

typedef struct pdp_opencv_camshift_struct
{
    t_object x_obj;
    t_float x_f;
    t_atom x_list[5];

    t_outlet *x_outlet0;
    t_outlet *x_outlet1;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;

    int x_track;
    int x_init;
    int x_rwidth;
    int x_rheight;
    int x_backproject;
    int x_vmin;
    int x_vmax;
    int x_smin;

    IplImage *image, *hsv, *hue, *mask, *backproject;
    CvHistogram *hist;
    CvPoint origin;
    CvRect selection;
    CvRect trackwindow;
    CvBox2D trackbox;
    CvConnectedComp trackcomp;
    
} t_pdp_opencv_camshift;

static CvScalar pdp_opencv_camshift_hsv2rgb( float hue )
{
    int rgb[3], p, sector;
    static const int sector_data[][3]= {{0,2,1}, {1,2,0}, {1,0,2}, {2,0,1}, {2,1,0}, {0,1,2}};
    hue *= 0.033333333333333333333333333333333f;
    sector = cvFloor(hue);
    p = cvRound(255*(hue - sector));
    p ^= sector & 1 ? 255 : 0;

    rgb[sector_data[sector][0]] = 255;
    rgb[sector_data[sector][1]] = 0;
    rgb[sector_data[sector][2]] = p;

    return cvScalar(rgb[2], rgb[1], rgb[0],0);
}

static void pdp_opencv_camshift_process_rgb(t_pdp_opencv_camshift *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    int hdims = 16;
    float hranges_arr[] = {0,180};
    float* hranges = hranges_arr;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_camshift :: resizing");
	
    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->hsv);
    	cvReleaseImage(&x->hue);
    	cvReleaseImage(&x->mask);
    	cvReleaseImage(&x->backproject);
    	cvReleaseHist(&x->hist);
    
        x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
        x->hsv = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
        x->hue = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
        x->mask = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
        x->backproject = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
        x->hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    memcpy( x->image->imageData, data, x->x_size*3 );
    
    // Convert to hsv
    cvCvtColor(x->image, x->hsv, CV_BGR2HSV);

    if ( x->x_track  )
    {
      cvInRangeS( x->hsv, cvScalar(0,x->x_smin,MIN(x->x_vmin,x->x_vmax),0), cvScalar(180,256,MAX(x->x_vmin,x->x_vmax),0), x->mask );
      cvSplit( x->hsv, x->hue, 0, 0, 0 );
 
      if ( x->x_init )
      {
       float max_val = 0.f;
         x->x_init = 0;
         cvSetImageROI( x->hue, x->selection );
         cvSetImageROI( x->mask, x->selection );
         cvCalcHist( &x->hue, x->hist, 0, x->mask );
         cvGetMinMaxHistValue( x->hist, 0, &max_val, 0, 0 );
         cvConvertScale( x->hist->bins, x->hist->bins, max_val ? 255. / max_val : 0., 0 );
         cvResetImageROI( x->hue );
         cvResetImageROI( x->mask );
         x->trackwindow = x->selection;
      }

      cvCalcBackProject( (IplImage**)&(x->hue), (CvArr*)x->backproject, (const CvHistogram*)x->hist );
      cvAnd( x->backproject, x->mask, x->backproject, 0 );
      cvCamShift( x->backproject, x->trackwindow,
                  cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ),
                  &x->trackcomp, &x->trackbox );
      x->trackwindow = x->trackcomp.rect;

      if( x->x_backproject )
          cvCvtColor( x->backproject, x->image, CV_GRAY2BGR );
      if( !x->image->origin )
           x->trackbox.angle = -x->trackbox.angle;
      cvEllipseBox( x->image, x->trackbox, CV_RGB(255,0,0), 3, CV_AA, 0 );
      SETFLOAT(&x->x_list[0], x->trackbox.center.x);
      SETFLOAT(&x->x_list[1], x->trackbox.center.y);
      SETFLOAT(&x->x_list[2], x->trackbox.size.width);
      SETFLOAT(&x->x_list[3], x->trackbox.size.height);
      SETFLOAT(&x->x_list[4], x->trackbox.angle);
      outlet_list( x->x_outlet1, 0, 5, x->x_list );
    }
  
    memcpy( newdata, x->image->imageData, x->x_size*3 );
 
    return;
}

static void pdp_opencv_camshift_backproject(t_pdp_opencv_camshift *x, t_floatarg f)
{
    if ( ( (int)f==0 ) || ( (int)f==1 ) ) x->x_backproject = (int)f;
}

static void pdp_opencv_camshift_vmin(t_pdp_opencv_camshift *x, t_floatarg f)
{
    if ( ( (int)f>=0 ) || ( (int)f<256 ) ) x->x_vmin = (int)f;
}

static void pdp_opencv_camshift_vmax(t_pdp_opencv_camshift *x, t_floatarg f)
{
    if ( ( (int)f>=0 ) || ( (int)f<256 ) ) x->x_vmax = (int)f;
}

static void pdp_opencv_camshift_smin(t_pdp_opencv_camshift *x, t_floatarg f)
{
    if ( ( (int)f>=0 ) || ( (int)f<256 ) ) x->x_smin = (int)f;
}

static void pdp_opencv_camshift_track(t_pdp_opencv_camshift *x, t_floatarg perx, t_floatarg pery)
{
  int ox,oy;
  int rx,ry;
  int w,h;

    if ( ( perx<0.0 ) || ( perx>1.0 ) || ( pery<0.0 ) || ( pery>1.0 ) ) return;
    
    ox = (int)(perx*x->x_width);
    oy = (int)(pery*x->x_height);
    x->origin = cvPoint(ox,oy);
    rx = ( ox-(x->x_rwidth/2) < 0 )? 0:ox-(x->x_rwidth/2);
    ry = ( oy-(x->x_rheight/2) < 0 )? 0:oy-(x->x_rheight/2);
    w = (rx+x->x_rwidth>x->x_width ) ? ( x->x_width - rx ):x->x_rwidth;
    h = (ry+x->x_rheight>x->x_height ) ? ( x->x_height - ry ):x->x_rheight;
    x->selection = cvRect(rx,ry,w,h);
    x->x_track = 1;
    x->x_init = 1;
}

static void pdp_opencv_camshift_rwidth(t_pdp_opencv_camshift *x, t_floatarg f)
{
    if ( (int)f>=0 ) x->x_rwidth = (int)f;
    // refresh selection zone
    pdp_opencv_camshift_track( x, ((float)x->origin.x)/((float)x->x_width), ((float)x->origin.y)/((float)x->x_height) );
}

static void pdp_opencv_camshift_rheight(t_pdp_opencv_camshift *x, t_floatarg f)
{
    if ( (int)f>=0 ) x->x_rheight = (int)f;
    // refresh selection zone
    pdp_opencv_camshift_track( x, ((float)x->origin.x)/((float)x->x_width), ((float)x->origin.y)/((float)x->x_height) );
}

static void pdp_opencv_camshift_sendpacket(t_pdp_opencv_camshift *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_camshift_process(t_pdp_opencv_camshift *x)
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
    
	/* pdp_opencv_camshift_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_camshift_process_rgb, (void*)pdp_opencv_camshift_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_camshift_process */
	    break;
	    
	}
    }
}

static void pdp_opencv_camshift_input_0(t_pdp_opencv_camshift *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_camshift_process(x);
    }
}

static void pdp_opencv_camshift_free(t_pdp_opencv_camshift *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage(&x->image);
    cvReleaseImage(&x->hsv);
    cvReleaseImage(&x->hue);
    cvReleaseImage(&x->mask);
    cvReleaseImage(&x->backproject);
    cvReleaseHist(&x->hist);
}

t_class *pdp_opencv_camshift_class;

void *pdp_opencv_camshift_new(t_floatarg f)
{
  int hdims = 16;
  float hranges_arr[] = {0,180};
  float* hranges = hranges_arr;

    t_pdp_opencv_camshift *x = (t_pdp_opencv_camshift *)pd_new(pdp_opencv_camshift_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    x->x_track = 0;
    x->x_init = 0;
    x->x_rwidth = 20;
    x->x_rheight = 20;
    x->x_backproject = 0;
    x->x_vmin = 50;
    x->x_vmax = 256;
    x->x_smin = 30;

    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->hsv = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->hue = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    x->mask = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    x->backproject = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 1);
    x->hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_camshift_setup(void)
{

    post( "		pdp_opencv_camshift");
    pdp_opencv_camshift_class = class_new(gensym("pdp_opencv_camshift"), (t_newmethod)pdp_opencv_camshift_new,
    	(t_method)pdp_opencv_camshift_free, sizeof(t_pdp_opencv_camshift), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_camshift_class, (t_method)pdp_opencv_camshift_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_camshift_class, (t_method)pdp_opencv_camshift_backproject, gensym("backproject"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_camshift_class, (t_method)pdp_opencv_camshift_vmin, gensym("vmin"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_camshift_class, (t_method)pdp_opencv_camshift_vmax, gensym("vmax"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_camshift_class, (t_method)pdp_opencv_camshift_smin, gensym("smin"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_camshift_class, (t_method)pdp_opencv_camshift_track, gensym("track"),  A_FLOAT, A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_camshift_class, (t_method)pdp_opencv_camshift_rwidth, gensym("rwidth"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_camshift_class, (t_method)pdp_opencv_camshift_rheight, gensym("rheight"),  A_FLOAT, A_NULL );   

}

#ifdef __cplusplus
}
#endif
