
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.kug.ac.at
//
// Implementation file
//
//    Copyright (c) 1997-2000 Mark Danks.
//    Copyright (c) Günther Geiger.
//    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM
//    Copyright (c) 2002 James Tittle & Chris Clepper
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "pix_opencv_camshift.h"
#include <stdio.h>

CPPEXTERN_NEW(pix_opencv_camshift)

/////////////////////////////////////////////////////////
//
// pix_opencv_camshift
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

pix_opencv_camshift :: pix_opencv_camshift()
{ 
  int i;
  int hdims = 16;
  float hranges_arr[] = {0,180};
  float* hranges = hranges_arr;

  comp_xsize=320;
  comp_ysize=240;

  m_dataout = outlet_new(this->x_obj, &s_anything);

  x_track = 0;
  x_init = 0;
  x_rwidth = 20;
  x_rheight = 20;
  x_backproject = 0;
  x_vmin = 50;
  x_vmax = 256;
  x_smin = 30;

  rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
  rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
  gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  hsv = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
  hue = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  mask = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  backproject = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_camshift :: ~pix_opencv_camshift()
{
  // Destroy cv_images
  cvReleaseImage(&rgba);
  cvReleaseImage(&rgb);
  cvReleaseImage(&gray);
  cvReleaseImage(&hsv);
  cvReleaseImage(&hue);
  cvReleaseImage(&mask);
  cvReleaseImage(&backproject);
  cvReleaseHist(&hist);
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_camshift :: processRGBAImage(imageStruct &image)
{
  int i, k;
  int im;
  int marked;
  int hdims = 16;
  float hranges_arr[] = {0,180};
  float* hranges = hranges_arr;


  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);
    cvReleaseImage(&hsv);
    cvReleaseImage(&hue);
    cvReleaseImage(&mask);
    cvReleaseImage(&backproject);
    cvReleaseHist(&hist);

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    hsv = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    hue = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    mask = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    backproject = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );

  }

  memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );

  // Convert to hsv
  cvCvtColor(rgba, rgb, CV_BGRA2BGR);
  cvCvtColor(rgb, hsv, CV_BGR2HSV);

  if ( x_track  )
  {
      cvInRangeS( hsv, cvScalar(0,x_smin,MIN(x_vmin,x_vmax),0), cvScalar(180,256,MAX(x_vmin,x_vmax),0), mask );
      cvSplit( hsv, hue, 0, 0, 0 );

      if ( x_init )
      {
       float max_val = 0.f;
         x_init = 0;
         cvSetImageROI( hue, selection );
         cvSetImageROI( mask, selection );
         cvCalcHist( &hue, hist, 0, mask );
         cvGetMinMaxHistValue( hist, 0, &max_val, 0, 0 );
         cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
         cvResetImageROI( hue );
         cvResetImageROI( mask );
         trackwindow = selection;
      }

      cvCalcBackProject( (IplImage**)&(hue), (CvArr*)backproject, (const CvHistogram*)hist );
      cvAnd( backproject, mask, backproject, 0 );
      cvCamShift( backproject, trackwindow,
                  cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ),
                  &trackcomp, &trackbox );
      trackwindow = trackcomp.rect;

      if( x_backproject )
          cvCvtColor( backproject, rgb, CV_GRAY2BGR );
      if( !rgb->origin )
           trackbox.angle = -trackbox.angle;
      cvEllipseBox( rgb, trackbox, CV_RGB(255,0,0), 3, CV_AA, 0 );
      SETFLOAT(&x_list[0], trackbox.center.x);
      SETFLOAT(&x_list[1], trackbox.center.y);
      SETFLOAT(&x_list[2], trackbox.size.width);
      SETFLOAT(&x_list[3], trackbox.size.height);
      SETFLOAT(&x_list[4], trackbox.angle);
      outlet_list( m_dataout, 0, 5, x_list );
  }

  cvCvtColor(rgb, rgba, CV_BGR2BGRA);
  memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_camshift :: processRGBImage(imageStruct &image)
{ 
  int i, k;
  int im;
  int marked;
  int hdims = 16;
  float hranges_arr[] = {0,180};
  float* hranges = hranges_arr;


  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);
    cvReleaseImage(&hsv);
    cvReleaseImage(&hue);
    cvReleaseImage(&mask);
    cvReleaseImage(&backproject);
    cvReleaseHist(&hist);

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    hsv = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    hue = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    mask = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    backproject = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );

  }

  memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );

  // Convert to hsv
  cvCvtColor(rgb, hsv, CV_BGR2HSV);

  if ( x_track  )
  {
      cvInRangeS( hsv, cvScalar(0,x_smin,MIN(x_vmin,x_vmax),0), cvScalar(180,256,MAX(x_vmin,x_vmax),0), mask );
      cvSplit( hsv, hue, 0, 0, 0 );

      if ( x_init )
      {
       float max_val = 0.f;
         x_init = 0;
         cvSetImageROI( hue, selection );
         cvSetImageROI( mask, selection );
         cvCalcHist( &hue, hist, 0, mask );
         cvGetMinMaxHistValue( hist, 0, &max_val, 0, 0 );
         cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
         cvResetImageROI( hue );
         cvResetImageROI( mask );
         trackwindow = selection;
      }

      cvCalcBackProject( (IplImage**)&(hue), (CvArr*)backproject, (const CvHistogram*)hist );
      cvAnd( backproject, mask, backproject, 0 );
      cvCamShift( backproject, trackwindow,
                  cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ),
                  &trackcomp, &trackbox );
      trackwindow = trackcomp.rect;

      if( x_backproject )
          cvCvtColor( backproject, rgb, CV_GRAY2BGR );
      if( !rgb->origin )
           trackbox.angle = -trackbox.angle;
      cvEllipseBox( rgb, trackbox, CV_RGB(255,0,0), 3, CV_AA, 0 );
      SETFLOAT(&x_list[0], trackbox.center.x);
      SETFLOAT(&x_list[1], trackbox.center.y);
      SETFLOAT(&x_list[2], trackbox.size.width);
      SETFLOAT(&x_list[3], trackbox.size.height);
      SETFLOAT(&x_list[4], trackbox.angle);
      outlet_list( m_dataout, 0, 5, x_list );
  }

  memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_camshift :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_camshift : yuv format not supported" );
}
    	
void pix_opencv_camshift :: processGrayImage(imageStruct &image)
{ 
  int i, k;
  int im;
  int marked;
  int hdims = 16;
  float hranges_arr[] = {0,180};
  float* hranges = hranges_arr;


  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);
    cvReleaseImage(&hsv);
    cvReleaseImage(&hue);
    cvReleaseImage(&mask);
    cvReleaseImage(&backproject);
    cvReleaseHist(&hist);

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    hsv = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    hue = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    mask = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    backproject = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    hist = cvCreateHist( 1, &hdims, CV_HIST_ARRAY, &hranges, 1 );

  }

  memcpy( gray->imageData, image.data, image.xsize*image.ysize );

  // Convert to hsv
  cvCvtColor(gray, rgb, CV_GRAY2BGR);
  cvCvtColor(rgb, hsv, CV_BGR2HSV);

  if ( x_track  )
  {
      cvInRangeS( hsv, cvScalar(0,x_smin,MIN(x_vmin,x_vmax),0), cvScalar(180,256,MAX(x_vmin,x_vmax),0), mask );
      cvSplit( hsv, hue, 0, 0, 0 );

      if ( x_init )
      {
       float max_val = 0.f;
         x_init = 0;
         cvSetImageROI( hue, selection );
         cvSetImageROI( mask, selection );
         cvCalcHist( &hue, hist, 0, mask );
         cvGetMinMaxHistValue( hist, 0, &max_val, 0, 0 );
         cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
         cvResetImageROI( hue );
         cvResetImageROI( mask );
         trackwindow = selection;
      }

      cvCalcBackProject( (IplImage**)&(hue), (CvArr*)backproject, (const CvHistogram*)hist );
      cvAnd( backproject, mask, backproject, 0 );
      cvCamShift( backproject, trackwindow,
                  cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ),
                  &trackcomp, &trackbox );
      trackwindow = trackcomp.rect;

      if( x_backproject )
          memcpy( gray->imageData, backproject->imageData, image.xsize*image.ysize );
      if( !rgb->origin )
           trackbox.angle = -trackbox.angle;
      cvEllipseBox( gray, trackbox, CV_RGB(255,0,0), 3, CV_AA, 0 );
      SETFLOAT(&x_list[0], trackbox.center.x);
      SETFLOAT(&x_list[1], trackbox.center.y);
      SETFLOAT(&x_list[2], trackbox.size.width);
      SETFLOAT(&x_list[3], trackbox.size.height);
      SETFLOAT(&x_list[4], trackbox.angle);
      outlet_list( m_dataout, 0, 5, x_list );
  }

  memcpy( image.data, gray->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////

void pix_opencv_camshift :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_camshift::backProjectMessCallback,
		  gensym("backproject"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_camshift::vMinMessCallback,
		  gensym("vmin"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_camshift::vMaxMessCallback,
		  gensym("vmax"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_camshift::sMinMessCallback,
		  gensym("smin"), A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_camshift::trackMessCallback,
		  gensym("track"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_camshift::rWidthMessCallback,
		  gensym("rwidth"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_camshift::rHeightMessCallback,
		  gensym("rheight"), A_NULL);
}

void  pix_opencv_camshift :: backProjectMessCallback(void *data, t_floatarg backproject)
{
    GetMyClass(data)->backProjectMess((float)backproject);
}

void  pix_opencv_camshift :: vMinMessCallback(void *data, t_floatarg vmin)
{
    GetMyClass(data)->vMinMess((float)vmin);
}

void  pix_opencv_camshift :: vMaxMessCallback(void *data, t_floatarg vmax)
{
    GetMyClass(data)->vMaxMess((float)vmax);
}

void  pix_opencv_camshift :: sMinMessCallback(void *data, t_floatarg smin)
{
    GetMyClass(data)->sMinMess((float)smin);
}

void  pix_opencv_camshift :: trackMessCallback(void *data, t_floatarg px, t_floatarg py)
{
    GetMyClass(data)->trackMess((float)px, (float)py);
}

void  pix_opencv_camshift :: rWidthMessCallback(void *data, t_floatarg rwidth)
{
    GetMyClass(data)->rWidthMess((float)rwidth);
}

void  pix_opencv_camshift :: rHeightMessCallback(void *data, t_floatarg rheight)
{
    GetMyClass(data)->rHeightMess((float)rheight);
}

void  pix_opencv_camshift :: backProjectMess(float backproject)
{
   if ( ( (int)backproject==0 ) || ( (int)backproject==1 ) ) x_backproject = (int)backproject;
}

void  pix_opencv_camshift :: vMinMess(float vmin)
{
   if ( ( (int)vmin>=0 ) || ( (int)vmin<256 ) ) x_vmin = (int)vmin;
}

void  pix_opencv_camshift :: vMaxMess(float vmax)
{
   if ( ( (int)vmax>=0 ) || ( (int)vmax<256 ) ) x_vmax = (int)vmax;
}

void  pix_opencv_camshift :: sMinMess(float smin)
{
   if ( ( (int)smin>=0 ) || ( (int)smin<256 ) ) x_smin = (int)smin;
}

void  pix_opencv_camshift :: trackMess(float px, float py)
{
  int rx, ry, w, h;

    if ( ( px<0.0 ) || ( px>comp_xsize ) || ( py<0.0 ) || ( py>comp_ysize ) ) return;

    //py = comp_ysize - py;
    origin = cvPoint((int)px,(int)py);
    rx = ( (int)px-(x_rwidth/2) < 0 )? 0:(int)px-(x_rwidth/2);
    ry = ( (int)py-(x_rheight/2) < 0 )? 0:(int)py-(x_rheight/2);
    w = (rx+x_rwidth>comp_xsize ) ? ( comp_xsize - rx ):x_rwidth;
    h = (ry+x_rheight>comp_ysize ) ? ( comp_ysize - ry ):x_rheight;
    selection = cvRect(rx,ry,w,h);
    post( "pix_opencv_camshift : track point (%f,%f) region (%d %d %d %d)", px, py, rx, ry, w, h );
    x_track = 1;
    x_init = 1;
}

void  pix_opencv_camshift :: rWidthMess(float rwidth)
{
    if ( (int)rwidth>=0 ) x_rwidth = (int)rwidth;
    // refresh selection zone
    trackMess( (float)origin.x, (float)origin.y );
}

void  pix_opencv_camshift :: rHeightMess(float rheight)
{
    if ( (int)rheight>=0 ) x_rheight = (int)rheight;
    // refresh selection zone
    trackMess( (float)origin.x, (float)origin.y );
}

