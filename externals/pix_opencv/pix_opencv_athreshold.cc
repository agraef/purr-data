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

#include "pix_opencv_athreshold.h"

CPPEXTERN_NEW(pix_opencv_athreshold)

/////////////////////////////////////////////////////////
//
// pix_opencv_athreshold
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_athreshold :: pix_opencv_athreshold()
{ 
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("max_value"));
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("blocksize"));

  max_value = 255;
  x_threshold_mode  = 0;
  x_threshold_method  = CV_ADAPTIVE_THRESH_MEAN_C;
  x_blocksize  = 3;
  x_dim = 0;

  comp_xsize=320;
  comp_ysize=240;

  rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
  rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
  gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_athreshold :: ~pix_opencv_athreshold()
{ 
   //Destroy cv_images to clean memory
   cvReleaseImage(&rgba);
   cvReleaseImage(&gray);
   cvReleaseImage(&rgb);
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_athreshold :: processRGBAImage(imageStruct &image)
{
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgba)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( rgba )
        {
	  cvReleaseImage(&rgba);
    	  cvReleaseImage(&gray);
    	  cvReleaseImage(&rgb);
        }

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 1);
    }
    memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
    cvCvtColor(rgba, gray, CV_BGRA2GRAY);
  
    // Applies fixed-level thresholding to single-channel array.
    switch(x_threshold_mode) {
        case 0:
           cvAdaptiveThreshold(gray, gray, (float)max_value, x_threshold_method, CV_THRESH_BINARY, x_blocksize, x_dim);
           break;
        case 1:
           cvAdaptiveThreshold(gray, gray, (float)max_value, x_threshold_method, CV_THRESH_BINARY_INV, x_blocksize, x_dim);
           break;
    }

    cvCvtColor(gray, rgba, CV_GRAY2BGRA);
    //copy back the processed frame to image
    memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_athreshold :: processRGBImage(imageStruct &image)
{
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( rgb )
        {
	  cvReleaseImage(&rgba);
    	  cvReleaseImage(&gray);
    	  cvReleaseImage(&rgb);
        }

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    
    }
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    cvCvtColor(rgb, gray, CV_RGB2GRAY);
    
    // Applies fixed-level thresholding to single-channel array.
    switch(x_threshold_mode) {
        case 0:
           cvAdaptiveThreshold(gray, gray, (float)max_value, x_threshold_method, CV_THRESH_BINARY, x_blocksize, x_dim);
           break;
        case 1:
           cvAdaptiveThreshold(gray, gray, (float)max_value, x_threshold_method, CV_THRESH_BINARY_INV, x_blocksize, x_dim);
           break;
    }

    cvCvtColor(gray, rgb, CV_GRAY2BGR);
    memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_athreshold :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_athreshold : yuv format not supported" );
}
    	
void pix_opencv_athreshold :: processGrayImage(imageStruct &image)
{ 
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( rgb )
        {
	  cvReleaseImage(&rgba);
    	  cvReleaseImage(&gray);
    	  cvReleaseImage(&rgb);
        }

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    
    }
    memcpy( gray->imageData, image.data, image.xsize*image.ysize );
    
    // Applies fixed-level thresholding to single-channel array.
    switch(x_threshold_mode) {
        case 0:
           cvAdaptiveThreshold(gray, gray, (float)max_value, x_threshold_method, CV_THRESH_BINARY, x_blocksize, x_dim);
           break;
        case 1:
           cvAdaptiveThreshold(gray, gray, (float)max_value, x_threshold_method, CV_THRESH_BINARY_INV, x_blocksize, x_dim);
           break;
    }

    memcpy( image.data, gray->imageData, image.xsize*image.ysize );
}

void pix_opencv_athreshold :: floatMaxValueMess (float maxvalue)
{
  if ( (int)maxvalue>0 ) max_value = (int)maxvalue;
}

void pix_opencv_athreshold :: floatModeMess (float mode)
{
  if ( ( (int)mode==0 ) || ( (int)mode==1 ) ) x_threshold_mode = (int)mode;
}

void pix_opencv_athreshold :: floatMethodMess (float method)
{
  if ( (int)method==CV_ADAPTIVE_THRESH_MEAN_C ) x_threshold_method = CV_ADAPTIVE_THRESH_MEAN_C;
  if ( (int)method==CV_ADAPTIVE_THRESH_GAUSSIAN_C ) x_threshold_method = CV_ADAPTIVE_THRESH_GAUSSIAN_C;
}

void pix_opencv_athreshold :: floatBlockSizeMess (float blocksize)
{
  if ( ( (int)blocksize>=3 ) && ( (int)(blocksize+1)%2 == 0 ) )
  {
     x_blocksize = (int)blocksize;
  }
}

void pix_opencv_athreshold :: floatDimMess (float dim)
{
  if ( (int)dim>0 ) x_dim = (int)dim;
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_athreshold :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_athreshold::floatModeMessCallback,
  		  gensym("mode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_athreshold::floatMethodMessCallback,
  		  gensym("method"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_athreshold::floatMaxValueMessCallback,
  		  gensym("max_value"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_athreshold::floatBlockSizeMessCallback,
  		  gensym("blocksize"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_athreshold::floatDimMessCallback,
  		  gensym("dim"), A_FLOAT, A_NULL);
}

void pix_opencv_athreshold :: floatModeMessCallback(void *data, t_floatarg mode)
{
  GetMyClass(data)->floatModeMess((float)mode);
}

void pix_opencv_athreshold :: floatMethodMessCallback(void *data, t_floatarg method)
{
  GetMyClass(data)->floatMethodMess((float)method);
}

void pix_opencv_athreshold :: floatMaxValueMessCallback(void *data, t_floatarg maxvalue)
{
  GetMyClass(data)->floatMaxValueMess((float)maxvalue);
}

void pix_opencv_athreshold :: floatBlockSizeMessCallback(void *data, t_floatarg blocksize)
{
  GetMyClass(data)->floatBlockSizeMess((float)blocksize);
}

void pix_opencv_athreshold :: floatDimMessCallback(void *data, t_floatarg dim)
{
  GetMyClass(data)->floatDimMess((float)dim);
}
