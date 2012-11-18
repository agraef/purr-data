////////////////////////////////////////////////////////
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

#include "pix_opencv_hough_circles.h"
#include <stdio.h>

CPPEXTERN_NEW(pix_opencv_hough_circles)

/////////////////////////////////////////////////////////
//
// pix_opencv_hough_circles
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

pix_opencv_hough_circles :: pix_opencv_hough_circles()
{ 
  int i;

  comp_xsize=320;
  comp_ysize=240;

  m_dataout = outlet_new(this->x_obj, &s_anything);

  x_threshold = 100;
  x_threshold2 = 10;
  x_maxcircles = 10;
  x_mindist = 30.0;
  x_resolution = 1.0;
  night_mode = 0;

  // initialize font
  cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );

  rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
  rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
  gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_hough_circles :: ~pix_opencv_hough_circles()
{
    //Destroy cv_images to clean memory
    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_hough_circles :: processRGBAImage(imageStruct &image)
{
  int i, ucircles;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);

  }

  memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
  cvCvtColor( rgba, gray, CV_BGRA2GRAY );

  if( night_mode )
      cvZero( rgba );

  x_storage = cvCreateMemStorage(0);

  cvSmooth( gray, gray, CV_GAUSSIAN, 9, 9 );
  CvSeq* circles = cvHoughCircles( gray, x_storage, CV_HOUGH_GRADIENT, x_resolution, x_mindist, x_threshold, x_threshold2 );
  ucircles = (circles->total>x_maxcircles)?x_maxcircles:circles->total;
  for( i = 0; i < ucircles; i++ )
  {
    float* p = (float*)cvGetSeqElem( circles, i );
    char tindex[10];

       cvCircle( rgba, cvPoint(cvRound(p[0]),cvRound(p[1])), 3, CV_RGB(0,255,0), -1, 8, 0 );
       cvCircle( rgba, cvPoint(cvRound(p[0]),cvRound(p[1])), cvRound(p[2]), CV_RGB(255,0,0), 3, 8, 0 );
       SETFLOAT(&x_list[0], i);
       SETFLOAT(&x_list[1], cvRound(p[0]));
       SETFLOAT(&x_list[2], cvRound(p[1]));
       SETFLOAT(&x_list[3], cvRound(p[2]));
       outlet_list( m_dataout, 0, 4, x_list );
       sprintf( tindex, "%d", i );
       cvPutText( rgba, tindex, cvPoint(cvRound(p[0]),cvRound(p[1])), &font, CV_RGB(255,255,255));
  }

  cvReleaseMemStorage( &x_storage );

  memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );

}

void pix_opencv_hough_circles :: processRGBImage(imageStruct &image)
{
  int i, ucircles;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);

  }

  memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
  cvCvtColor( rgb, gray, CV_BGR2GRAY );

  if( night_mode )
      cvZero( rgb );

  x_storage = cvCreateMemStorage(0);

  cvSmooth( gray, gray, CV_GAUSSIAN, 9, 9 );
  CvSeq* circles = cvHoughCircles( gray, x_storage, CV_HOUGH_GRADIENT, x_resolution, x_mindist, x_threshold, x_threshold2 );
  ucircles = (circles->total>x_maxcircles)?x_maxcircles:circles->total;
  for( i = 0; i < ucircles; i++ )
  {
    float* p = (float*)cvGetSeqElem( circles, i );
    char tindex[10];

       cvCircle( rgb, cvPoint(cvRound(p[0]),cvRound(p[1])), 3, CV_RGB(0,255,0), -1, 8, 0 );
       cvCircle( rgb, cvPoint(cvRound(p[0]),cvRound(p[1])), cvRound(p[2]), CV_RGB(255,0,0), 3, 8, 0 );
       SETFLOAT(&x_list[0], i);
       SETFLOAT(&x_list[1], cvRound(p[0]));
       SETFLOAT(&x_list[2], cvRound(p[1]));
       SETFLOAT(&x_list[3], cvRound(p[2]));
       outlet_list( m_dataout, 0, 4, x_list );
       sprintf( tindex, "%d", i );
       cvPutText( rgb, tindex, cvPoint(cvRound(p[0]),cvRound(p[1])), &font, CV_RGB(255,255,255));
  }

  cvReleaseMemStorage( &x_storage );
  memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );

}

void pix_opencv_hough_circles :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_hough_circles : yuv format not supported" );
}
    	
void pix_opencv_hough_circles :: processGrayImage(imageStruct &image)
{ 
  int i, ucircles;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);

  }

  memcpy( gray->imageData, image.data, image.xsize*image.ysize );

  x_storage = cvCreateMemStorage(0);

  cvSmooth( gray, gray, CV_GAUSSIAN, 9, 9 );
  CvSeq* circles = cvHoughCircles( gray, x_storage, CV_HOUGH_GRADIENT, x_resolution, x_mindist, x_threshold, x_threshold2 );

  if( night_mode )
      cvZero( gray );

  ucircles = (circles->total>x_maxcircles)?x_maxcircles:circles->total;
  for( i = 0; i < ucircles; i++ )
  {
    float* p = (float*)cvGetSeqElem( circles, i );
    char tindex[10];

       cvCircle( gray, cvPoint(cvRound(p[0]),cvRound(p[1])), 3, CV_RGB(255,255,255), -1, 8, 0 );
       cvCircle( gray, cvPoint(cvRound(p[0]),cvRound(p[1])), cvRound(p[2]), CV_RGB(255,255,255), 3, 8, 0 );
       SETFLOAT(&x_list[0], i);
       SETFLOAT(&x_list[1], cvRound(p[0]));
       SETFLOAT(&x_list[2], cvRound(p[1]));
       SETFLOAT(&x_list[3], cvRound(p[2]));
       outlet_list( m_dataout, 0, 4, x_list );
       sprintf( tindex, "%d", i );
       cvPutText( gray, tindex, cvPoint(cvRound(p[0]),cvRound(p[1])), &font, CV_RGB(255,255,255));
  }

  cvReleaseMemStorage( &x_storage );
  memcpy( image.data, gray->imageData, image.xsize*image.ysize );

}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////

void pix_opencv_hough_circles :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_circles::floatNightModeMessCallback,
                  gensym("nightmode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_circles::floatThresholdMessCallback,
                  gensym("threshold"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_circles::floatThreshold2MessCallback,
                  gensym("threshold2"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_circles::floatMinDistMessCallback,
                  gensym("mindist"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_circles::floatResolutionMessCallback,
                  gensym("resolution"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_circles::floatMaxCirclesMessCallback,
                  gensym("maxcircles"), A_FLOAT, A_NULL);
}

void pix_opencv_hough_circles :: floatNightModeMessCallback(void *data, t_floatarg nightmode)
{
  GetMyClass(data)->floatNightModeMess((float)nightmode);
}

void pix_opencv_hough_circles :: floatThresholdMessCallback(void *data, t_floatarg threshold)
{
  GetMyClass(data)->floatThresholdMess((float)threshold);
}

void pix_opencv_hough_circles :: floatThreshold2MessCallback(void *data, t_floatarg threshold)
{
  GetMyClass(data)->floatThreshold2Mess((float)threshold);
}

void pix_opencv_hough_circles :: floatMinDistMessCallback(void *data, t_floatarg mindist)
{
  GetMyClass(data)->floatMinDistMess((float)mindist);
}

void pix_opencv_hough_circles :: floatResolutionMessCallback(void *data, t_floatarg resolution)
{
  GetMyClass(data)->floatResolutionMess((float)resolution);
}

void pix_opencv_hough_circles :: floatMaxCirclesMessCallback(void *data, t_floatarg maxcircles)
{
  GetMyClass(data)->floatMaxCirclesMess((float)maxcircles);
}

void pix_opencv_hough_circles :: floatNightModeMess(float nightmode)
{
  if ((nightmode==0.0)||(nightmode==1.0)) night_mode = (int)nightmode;
}

void pix_opencv_hough_circles :: floatThresholdMess(float threshold)
{
  if (threshold>0.0) x_threshold = (int)threshold;
}

void pix_opencv_hough_circles :: floatThreshold2Mess(float threshold)
{
  if (threshold>0.0) x_threshold2 = (int)threshold;
}

void pix_opencv_hough_circles :: floatMinDistMess(float mindist)
{
  if (mindist>0.0) x_mindist = (int)mindist;
}

void pix_opencv_hough_circles :: floatResolutionMess(float resolution)
{
  if (resolution>0.0) x_resolution = (int)resolution;
}

void pix_opencv_hough_circles :: floatMaxCirclesMess(float maxcircles)
{
  if (maxcircles>0.0) x_maxcircles = (int)maxcircles;
}

