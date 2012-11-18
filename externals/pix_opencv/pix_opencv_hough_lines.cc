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

#include "pix_opencv_hough_lines.h"
#include <stdio.h>

CPPEXTERN_NEW(pix_opencv_hough_lines)

/////////////////////////////////////////////////////////
//
// pix_opencv_hough_lines
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

pix_opencv_hough_lines :: pix_opencv_hough_lines()
{ 
  int i;

  comp_xsize=320;
  comp_ysize=240;

  m_dataout = outlet_new(this->x_obj, &s_anything);

  x_mode = CV_HOUGH_PROBABILISTIC;
  x_threshold = 50;
  x_maxlines = 10;
  x_minlength = 30.0;
  x_gap = 10.0;
  x_aresolution = 10.0;
  x_dresolution = 30.0;
  night_mode = 0;

  cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );

  rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
  rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
  gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_hough_lines :: ~pix_opencv_hough_lines()
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
void pix_opencv_hough_lines :: processRGBAImage(imageStruct &image)
{
  int i, ulines;

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

  cvCanny( gray, gray, 50, 200, 3 );

  if( night_mode )
       cvZero( rgba );

  x_storage = cvCreateMemStorage(0);

  switch( x_mode )
  {
     case CV_HOUGH_STANDARD:

        x_lines = cvHoughLines2( gray, x_storage, x_mode, 1, CV_PI/180, x_threshold, 0, 0 );

        if ( x_lines )
        {
          ulines = ( x_lines->total >= x_maxlines ) ? x_maxlines:x_lines->total;
          for( i=0; i<ulines; i++ )
          {
            float* line = (float*)cvGetSeqElem(x_lines,i);
            float rho = line[0];
            float theta = line[1];
            CvPoint pt1, pt2;
            char tindex[10];
            double a = cos(theta), b = sin(theta);
            double x0 = a*rho, y0 = b*rho;
            pt1.x = cvRound(x0 + 1000*(-b));
            pt1.y = cvRound(y0 + 1000*(a));
            pt2.x = cvRound(x0 - 1000*(-b));
            pt2.y = cvRound(y0 - 1000*(a));
            cvLine( rgba, pt1, pt2, CV_RGB(255,0,0), 3, 8 );
            SETFLOAT(&x_list[0], i);
            SETFLOAT(&x_list[1], pt1.x);
            SETFLOAT(&x_list[2], pt1.y);
            SETFLOAT(&x_list[3], pt2.x);
            SETFLOAT(&x_list[4], pt2.y);
            outlet_list( m_dataout, 0, 5, x_list );
            pt1.x = (pt1.x+pt2.x)/2;
            pt1.y = (pt1.y+pt2.y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( rgba, tindex, pt1, &font, CV_RGB(255,255,255));
          }
        }
        break;

     case CV_HOUGH_PROBABILISTIC:

        x_lines = cvHoughLines2( gray, x_storage, x_mode, 1, CV_PI/180, x_threshold, x_minlength, x_gap );

        if ( x_lines )
        {
          ulines = ( x_lines->total >= x_maxlines ) ? x_maxlines:x_lines->total;
          for( i=0; i<ulines; i++ )
          {
            CvPoint* line = (CvPoint*)cvGetSeqElem(x_lines,i);
            char tindex[10];
            cvLine( rgba, line[0], line[1], CV_RGB(255,0,0), 3, 8 );
            SETFLOAT(&x_list[0], i);
            SETFLOAT(&x_list[1], line[0].x);
            SETFLOAT(&x_list[2], line[0].y);
            SETFLOAT(&x_list[3], line[1].x);
            SETFLOAT(&x_list[4], line[1].y);
            outlet_list( m_dataout, 0, 5, x_list );
            line[0].x = (line[0].x+line[1].x)/2;
            line[0].y = (line[0].y+line[1].y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( rgba, tindex, line[0], &font, CV_RGB(255,255,255));
          }
        }
      break;

     case CV_HOUGH_MULTI_SCALE:

        x_lines = cvHoughLines2( gray, x_storage, x_mode, 1, CV_PI/180, x_threshold, x_dresolution, x_aresolution );

        if ( x_lines )
        {
          ulines = ( x_lines->total >= x_maxlines ) ? x_maxlines:x_lines->total;
          for( i = 0; i < ulines; i++ )
          {
            float* line = (float*)cvGetSeqElem(x_lines,i);
            float rho = line[0];
            float theta = line[1];
            char tindex[10];
            CvPoint pt1, pt2;
            double a = cos(theta), b = sin(theta);
            double x0 = a*rho, y0 = b*rho;
            pt1.x = cvRound(x0 + 1000*(-b));
            pt1.y = cvRound(y0 + 1000*(a));
            pt2.x = cvRound(x0 - 1000*(-b));
            pt2.y = cvRound(y0 - 1000*(a));
            cvLine( rgba, pt1, pt2, CV_RGB(255,0,0), 3, 8 );
            SETFLOAT(&x_list[0], i);
            SETFLOAT(&x_list[1], pt1.x);
            SETFLOAT(&x_list[2], pt1.y);
            SETFLOAT(&x_list[3], pt2.x);
            SETFLOAT(&x_list[4], pt2.y);
            outlet_list( m_dataout, 0, 5, x_list );
            pt1.x = (pt1.x+pt2.x)/2;
            pt1.y = (pt1.y+pt2.y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( rgba, tindex, pt1, &font, CV_RGB(255,255,255));
          }
        }
      break;
  }

  cvReleaseMemStorage( &x_storage );
  memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );

}

void pix_opencv_hough_lines :: processRGBImage(imageStruct &image)
{
  int i, ulines;

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

  cvCanny( gray, gray, 50, 200, 3 );

  if( night_mode )
       cvZero( rgb );

  x_storage = cvCreateMemStorage(0);

  switch( x_mode )
  {
     case CV_HOUGH_STANDARD:

        x_lines = cvHoughLines2( gray, x_storage, x_mode, 1, CV_PI/180, x_threshold, 0, 0 );

        if ( x_lines )
        {
          ulines = ( x_lines->total >= x_maxlines ) ? x_maxlines:x_lines->total;
          for( i=0; i<ulines; i++ )
          {
            float* line = (float*)cvGetSeqElem(x_lines,i);
            float rho = line[0];
            float theta = line[1];
            CvPoint pt1, pt2;
            char tindex[10];
            double a = cos(theta), b = sin(theta);
            double x0 = a*rho, y0 = b*rho;
            pt1.x = cvRound(x0 + 1000*(-b));
            pt1.y = cvRound(y0 + 1000*(a));
            pt2.x = cvRound(x0 - 1000*(-b));
            pt2.y = cvRound(y0 - 1000*(a));
            cvLine( rgb, pt1, pt2, CV_RGB(255,0,0), 3, 8 );
            SETFLOAT(&x_list[0], i);
            SETFLOAT(&x_list[1], pt1.x);
            SETFLOAT(&x_list[2], pt1.y);
            SETFLOAT(&x_list[3], pt2.x);
            SETFLOAT(&x_list[4], pt2.y);
            outlet_list( m_dataout, 0, 5, x_list );
            pt1.x = (pt1.x+pt2.x)/2;
            pt1.y = (pt1.y+pt2.y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( rgb, tindex, pt1, &font, CV_RGB(255,255,255));
          }
        }
        break;

     case CV_HOUGH_PROBABILISTIC:

        x_lines = cvHoughLines2( gray, x_storage, x_mode, 1, CV_PI/180, x_threshold, x_minlength, x_gap );

        if ( x_lines )
        {
          ulines = ( x_lines->total >= x_maxlines ) ? x_maxlines:x_lines->total;
          for( i=0; i<ulines; i++ )
          {
            CvPoint* line = (CvPoint*)cvGetSeqElem(x_lines,i);
            char tindex[10];
            cvLine( rgb, line[0], line[1], CV_RGB(255,0,0), 3, 8 );
            SETFLOAT(&x_list[0], i);
            SETFLOAT(&x_list[1], line[0].x);
            SETFLOAT(&x_list[2], line[0].y);
            SETFLOAT(&x_list[3], line[1].x);
            SETFLOAT(&x_list[4], line[1].y);
            outlet_list( m_dataout, 0, 5, x_list );
            line[0].x = (line[0].x+line[1].x)/2;
            line[0].y = (line[0].y+line[1].y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( rgb, tindex, line[0], &font, CV_RGB(255,255,255));
          }
        }
      break;

     case CV_HOUGH_MULTI_SCALE:

        x_lines = cvHoughLines2( gray, x_storage, x_mode, 1, CV_PI/180, x_threshold, x_dresolution, x_aresolution );

        if ( x_lines )
        {
          ulines = ( x_lines->total >= x_maxlines ) ? x_maxlines:x_lines->total;
          for( i = 0; i < ulines; i++ )
          {
            float* line = (float*)cvGetSeqElem(x_lines,i);
            float rho = line[0];
            float theta = line[1];
            char tindex[10];
            CvPoint pt1, pt2;
            double a = cos(theta), b = sin(theta);
            double x0 = a*rho, y0 = b*rho;
            pt1.x = cvRound(x0 + 1000*(-b));
            pt1.y = cvRound(y0 + 1000*(a));
            pt2.x = cvRound(x0 - 1000*(-b));
            pt2.y = cvRound(y0 - 1000*(a));
            cvLine( rgb, pt1, pt2, CV_RGB(255,0,0), 3, 8 );
            SETFLOAT(&x_list[0], i);
            SETFLOAT(&x_list[1], pt1.x);
            SETFLOAT(&x_list[2], pt1.y);
            SETFLOAT(&x_list[3], pt2.x);
            SETFLOAT(&x_list[4], pt2.y);
            outlet_list( m_dataout, 0, 5, x_list );
            pt1.x = (pt1.x+pt2.x)/2;
            pt1.y = (pt1.y+pt2.y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( rgb, tindex, pt1, &font, CV_RGB(255,255,255));
          }
        }
      break;
  }

  cvReleaseMemStorage( &x_storage );
  memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );

}

void pix_opencv_hough_lines :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_hough_lines : yuv format not supported" );
}
    	
void pix_opencv_hough_lines :: processGrayImage(imageStruct &image)
{ 
  int i, ulines;

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
  cvCanny( gray, gray, 50, 200, 3 );

  x_storage = cvCreateMemStorage(0);

  switch( x_mode )
  {
     case CV_HOUGH_STANDARD:

        x_lines = cvHoughLines2( gray, x_storage, x_mode, 1, CV_PI/180, x_threshold, 0, 0 );

        if( night_mode )
             cvZero( gray );

        if ( x_lines )
        {
          ulines = ( x_lines->total >= x_maxlines ) ? x_maxlines:x_lines->total;
          for( i=0; i<ulines; i++ )
          {
            float* line = (float*)cvGetSeqElem(x_lines,i);
            float rho = line[0];
            float theta = line[1];
            CvPoint pt1, pt2;
            char tindex[10];
            double a = cos(theta), b = sin(theta);
            double x0 = a*rho, y0 = b*rho;
            pt1.x = cvRound(x0 + 1000*(-b));
            pt1.y = cvRound(y0 + 1000*(a));
            pt2.x = cvRound(x0 - 1000*(-b));
            pt2.y = cvRound(y0 - 1000*(a));
            cvLine( gray, pt1, pt2, CV_RGB(255,255,255), 3, 8 );
            SETFLOAT(&x_list[0], i);
            SETFLOAT(&x_list[1], pt1.x);
            SETFLOAT(&x_list[2], pt1.y);
            SETFLOAT(&x_list[3], pt2.x);
            SETFLOAT(&x_list[4], pt2.y);
            outlet_list( m_dataout, 0, 5, x_list );
            pt1.x = (pt1.x+pt2.x)/2;
            pt1.y = (pt1.y+pt2.y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( gray, tindex, pt1, &font, CV_RGB(255,255,255));
          }
        }
        break;

     case CV_HOUGH_PROBABILISTIC:

        x_lines = cvHoughLines2( gray, x_storage, x_mode, 1, CV_PI/180, x_threshold, x_minlength, x_gap );

        if( night_mode )
             cvZero( gray );

        if ( x_lines )
        {
          ulines = ( x_lines->total >= x_maxlines ) ? x_maxlines:x_lines->total;
          for( i=0; i<ulines; i++ )
          {
            CvPoint* line = (CvPoint*)cvGetSeqElem(x_lines,i);
            char tindex[10];
            cvLine( gray, line[0], line[1], CV_RGB(255,255,255), 3, 8 );
            SETFLOAT(&x_list[0], i);
            SETFLOAT(&x_list[1], line[0].x);
            SETFLOAT(&x_list[2], line[0].y);
            SETFLOAT(&x_list[3], line[1].x);
            SETFLOAT(&x_list[4], line[1].y);
            outlet_list( m_dataout, 0, 5, x_list );
            line[0].x = (line[0].x+line[1].x)/2;
            line[0].y = (line[0].y+line[1].y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( gray, tindex, line[0], &font, CV_RGB(255,255,255));
          }
        }
      break;

     case CV_HOUGH_MULTI_SCALE:

        x_lines = cvHoughLines2( gray, x_storage, x_mode, 1, CV_PI/180, x_threshold, x_dresolution, x_aresolution );

        if( night_mode )
             cvZero( gray );

        if ( x_lines )
        {
          ulines = ( x_lines->total >= x_maxlines ) ? x_maxlines:x_lines->total;
          for( i = 0; i < ulines; i++ )
          {
            float* line = (float*)cvGetSeqElem(x_lines,i);
            float rho = line[0];
            float theta = line[1];
            char tindex[10];
            CvPoint pt1, pt2;
            double a = cos(theta), b = sin(theta);
            double x0 = a*rho, y0 = b*rho;
            pt1.x = cvRound(x0 + 1000*(-b));
            pt1.y = cvRound(y0 + 1000*(a));
            pt2.x = cvRound(x0 - 1000*(-b));
            pt2.y = cvRound(y0 - 1000*(a));
            cvLine( gray, pt1, pt2, CV_RGB(255,255,255), 3, 8 );
            SETFLOAT(&x_list[0], i);
            SETFLOAT(&x_list[1], pt1.x);
            SETFLOAT(&x_list[2], pt1.y);
            SETFLOAT(&x_list[3], pt2.x);
            SETFLOAT(&x_list[4], pt2.y);
            outlet_list( m_dataout, 0, 5, x_list );
            pt1.x = (pt1.x+pt2.x)/2;
            pt1.y = (pt1.y+pt2.y)/2;
            sprintf( tindex, "%d", i );
            cvPutText( gray, tindex, pt1, &font, CV_RGB(255,255,255));
          }
        }
      break;
  }

  cvReleaseMemStorage( &x_storage );
  memcpy( image.data, gray->imageData, image.xsize*image.ysize );

}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////

void pix_opencv_hough_lines :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_lines::floatNightModeMessCallback,
                  gensym("nightmode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_lines::floatModeMessCallback,
                  gensym("mode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_lines::floatThresholdMessCallback,
                  gensym("threshold"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_lines::floatMinLengthMessCallback,
                  gensym("minlength"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_lines::floatGapMessCallback,
                  gensym("gap"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_lines::floatAResolutionMessCallback,
                  gensym("aresolution"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_lines::floatDResolutionMessCallback,
                  gensym("dresolution"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_hough_lines::floatMaxLinesMessCallback,
                  gensym("maxlines"), A_FLOAT, A_NULL);
}

void pix_opencv_hough_lines :: floatNightModeMessCallback(void *data, t_floatarg nightmode)
{
  GetMyClass(data)->floatNightModeMess((float)nightmode);
}

void pix_opencv_hough_lines :: floatModeMessCallback(void *data, t_floatarg mode)
{
  GetMyClass(data)->floatModeMess((float)mode);
}

void pix_opencv_hough_lines :: floatThresholdMessCallback(void *data, t_floatarg threshold)
{
  GetMyClass(data)->floatThresholdMess((float)threshold);
}

void pix_opencv_hough_lines :: floatMinLengthMessCallback(void *data, t_floatarg minlength)
{
  GetMyClass(data)->floatMinLengthMess((float)minlength);
}

void pix_opencv_hough_lines :: floatGapMessCallback(void *data, t_floatarg gap)
{
  GetMyClass(data)->floatGapMess((float)gap);
}

void pix_opencv_hough_lines :: floatAResolutionMessCallback(void *data, t_floatarg aresolution)
{
  GetMyClass(data)->floatAResolutionMess((float)aresolution);
}

void pix_opencv_hough_lines :: floatDResolutionMessCallback(void *data, t_floatarg dresolution)
{
  GetMyClass(data)->floatDResolutionMess((float)dresolution);
}

void pix_opencv_hough_lines :: floatMaxLinesMessCallback(void *data, t_floatarg maxlines)
{
  GetMyClass(data)->floatMaxLinesMess((float)maxlines);
}

void pix_opencv_hough_lines :: floatNightModeMess(float nightmode)
{
  if ((nightmode==0.0)||(nightmode==1.0)) night_mode = (int)nightmode;
}

void pix_opencv_hough_lines :: floatModeMess(float mode)
{
   if ( mode == CV_HOUGH_STANDARD )
   {
      x_mode = CV_HOUGH_STANDARD;
   }
   if ( mode == CV_HOUGH_PROBABILISTIC )
   {
      x_mode = CV_HOUGH_PROBABILISTIC;
   }
   if ( mode == CV_HOUGH_MULTI_SCALE )
   {
      x_mode = CV_HOUGH_MULTI_SCALE;
   }
}

void pix_opencv_hough_lines :: floatThresholdMess(float threshold)
{
  if (threshold>0.0) x_threshold = (int)threshold;
}

void pix_opencv_hough_lines :: floatMinLengthMess(float minlength)
{
  if (minlength>0.0) x_minlength = (double)minlength;
}

void pix_opencv_hough_lines :: floatGapMess(float gap)
{
  if (gap>0.0) x_gap = (double)gap;
}

void pix_opencv_hough_lines :: floatAResolutionMess(float aresolution)
{
  if (aresolution>0.0) x_aresolution = (double)aresolution;
}

void pix_opencv_hough_lines :: floatDResolutionMess(float dresolution)
{
  if (dresolution>0.0) x_dresolution = (double)dresolution;
}

void pix_opencv_hough_lines :: floatMaxLinesMess(float maxlines)
{
  if (maxlines>0.0) x_maxlines = (int)maxlines;
}

