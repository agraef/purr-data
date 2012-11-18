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

#include "pix_opencv_hu_moments.h"

CPPEXTERN_NEW(pix_opencv_hu_moments)

/////////////////////////////////////////////////////////
//
// pix_opencv_hu_moments
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_hu_moments :: pix_opencv_hu_moments()
{ 
    m_dataout = outlet_new(this->x_obj, &s_anything);

    comp_xsize = 320;
    comp_ysize = 240;

    x_binary = 0;

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_hu_moments :: ~pix_opencv_hu_moments()
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
void pix_opencv_hu_moments :: processRGBAImage(imageStruct &image)
{
  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgba)) 
  {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( rgba )
        {
	  cvReleaseImage(&rgba);
    	  cvReleaseImage(&rgb);
    	  cvReleaseImage(&gray);
        }

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 1);
    }

    memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
    cvCvtColor(rgba, gray, CV_BGRA2GRAY);
  
    cvMoments( gray, &x_moments, x_binary );
    cvGetHuMoments( &x_moments, &x_humoments );

    SETFLOAT(&rlist[0], x_humoments.hu1);
    SETFLOAT(&rlist[1], x_humoments.hu2);
    SETFLOAT(&rlist[2], x_humoments.hu3);
    SETFLOAT(&rlist[3], x_humoments.hu4);
    SETFLOAT(&rlist[4], x_humoments.hu5);
    SETFLOAT(&rlist[5], x_humoments.hu6);
    SETFLOAT(&rlist[6], x_humoments.hu7);

    outlet_list( m_dataout, 0, 7, rlist );
}

void pix_opencv_hu_moments :: processRGBImage(imageStruct &image)
{
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( rgb )
        {
	  cvReleaseImage(&rgba);
    	  cvReleaseImage(&rgb);
    	  cvReleaseImage(&gray);
        }

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    
    }
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    cvCvtColor(rgb, gray, CV_RGB2GRAY);
    
    cvMoments( gray, &x_moments, x_binary );
    cvGetHuMoments( &x_moments, &x_humoments );

    SETFLOAT(&rlist[0], x_humoments.hu1);
    SETFLOAT(&rlist[1], x_humoments.hu2);
    SETFLOAT(&rlist[2], x_humoments.hu3);
    SETFLOAT(&rlist[3], x_humoments.hu4);
    SETFLOAT(&rlist[4], x_humoments.hu5);
    SETFLOAT(&rlist[5], x_humoments.hu6);
    SETFLOAT(&rlist[6], x_humoments.hu7);

    outlet_list( m_dataout, 0, 7, rlist );
}

void pix_opencv_hu_moments :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_hu_moments : yuv format not supported" );
}
    	
void pix_opencv_hu_moments :: processGrayImage(imageStruct &image)
{ 
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( rgb )
        {
	  cvReleaseImage(&rgba);
    	  cvReleaseImage(&rgb);
    	  cvReleaseImage(&gray);
        }

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    
    }
    memcpy( gray->imageData, image.data, image.xsize*image.ysize );

    cvMoments( gray, &x_moments, x_binary );
    cvGetHuMoments( &x_moments, &x_humoments );

    SETFLOAT(&rlist[0], x_humoments.hu1);
    SETFLOAT(&rlist[1], x_humoments.hu2);
    SETFLOAT(&rlist[2], x_humoments.hu3);
    SETFLOAT(&rlist[3], x_humoments.hu4);
    SETFLOAT(&rlist[4], x_humoments.hu5);
    SETFLOAT(&rlist[5], x_humoments.hu6);
    SETFLOAT(&rlist[6], x_humoments.hu7);

    outlet_list( m_dataout, 0, 7, rlist );
}

/////////////////////////////////////////////////////////
// floatThreshMess
//
/////////////////////////////////////////////////////////
void pix_opencv_hu_moments :: floatBinaryMess (float binary)
{
  if ( ((int)binary==1) || ((int)binary==0) ) x_binary = (int)binary;
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_hu_moments :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_hu_moments::floatBinaryMessCallback,
  		  gensym("binary"), A_FLOAT, A_NULL);
}

void pix_opencv_hu_moments :: floatBinaryMessCallback(void *data, t_floatarg binary)
{
  GetMyClass(data)->floatBinaryMess((float)binary);
}

