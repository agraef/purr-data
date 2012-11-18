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

#include "pix_opencv_threshold.h"

CPPEXTERN_NEW(pix_opencv_threshold)

/////////////////////////////////////////////////////////
//
// pix_opencv_threshold
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_threshold :: pix_opencv_threshold()
{ 
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("max"));
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("threshold"));
  threshold_value = 50;
  max_value = 255;
  threshold_mode  = 0;
  comp_xsize  = 0;
  comp_ysize  = 0;
  orig = NULL;
  rgb = NULL;
  gray = NULL;
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_threshold :: ~pix_opencv_threshold()
{ 
   //Destroy cv_images to clean memory
   cvReleaseImage(&orig);
   cvReleaseImage(&gray);
   cvReleaseImage(&rgb);
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_threshold :: processRGBAImage(imageStruct &image)
{
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!orig)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( orig )
        {
	  cvReleaseImage(&orig);
    	  cvReleaseImage(&gray);
    	  cvReleaseImage(&rgb);
        }

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    }
    // Here we make a copy of the pixel data from image to orig->imageData
    // orig is a IplImage struct, the default image type in openCV, take a look on the IplImage data structure here
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( orig->imageData, image.data, image.xsize*image.ysize*4 );
    
    // Convert to grayscale
    cvCvtColor(orig, gray, CV_BGRA2GRAY);
  
    // Applies fixed-level thresholding to single-channel array.
    switch(this->threshold_mode) {
    	case 0:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_BINARY);
	   break;
    	case 1:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_BINARY_INV);
	   break;
    	case 2:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_TRUNC);
	   break;
    	case 3:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_TOZERO);
	   break;
    	case 4:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_TOZERO_INV);
	   break;
    }
  
    cvCvtColor(gray, orig, CV_GRAY2BGRA);

    //copy back the processed frame to image
    memcpy( image.data, orig->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_threshold :: processRGBImage(imageStruct &image)
{
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( rgb )
        {
	  cvReleaseImage(&orig);
    	  cvReleaseImage(&gray);
    	  cvReleaseImage(&rgb);
        }

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    
    // Convert to grayscale
    cvCvtColor(rgb, gray, CV_RGB2GRAY);
    
    // Applies fixed-level thresholding to single-channel array.
    switch(this->threshold_mode) {
    	case 0:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_BINARY);
	   break;
    	case 1:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_BINARY_INV);
	   break;
    	case 2:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_TRUNC);
	   break;
    	case 3:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_TOZERO);
	   break;
    	case 4:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_TOZERO_INV);
	   break;
    }
  
    cvCvtColor(gray, rgb, CV_GRAY2BGR);

    memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_threshold :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_threshold : yuv format not supported" );
}
    	
void pix_opencv_threshold :: processGrayImage(imageStruct &image)
{ 
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( rgb )
        {
	  cvReleaseImage(&orig);
    	  cvReleaseImage(&gray);
    	  cvReleaseImage(&rgb);
        }

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( gray->imageData, image.data, image.xsize*image.ysize );
    
    // Applies fixed-level thresholding to single-channel array.
    switch(this->threshold_mode) {
    	case 0:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_BINARY);
	   break;
    	case 1:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_BINARY_INV);
	   break;
    	case 2:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_TRUNC);
	   break;
    	case 3:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_TOZERO);
	   break;
    	case 4:
	   cvThreshold(gray, gray, (float)this->threshold_value, (float)this->max_value, CV_THRESH_TOZERO_INV);
	   break;
    }
  
    memcpy( image.data, gray->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// floatThreshMess
//
/////////////////////////////////////////////////////////
void pix_opencv_threshold :: floatMaxMess (float maxvalue)
{
  if ( (int)maxvalue>0 ) this->max_value = maxvalue;
}
void pix_opencv_threshold :: floatThreshMess (float edge_thresh)
{
  this->threshold_value = edge_thresh;
}
void pix_opencv_threshold :: floatModeMess (float mode)
{
  if ((mode>=0)&&(mode<5)) this->threshold_mode = (int)mode;
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_threshold :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_threshold::floatModeMessCallback,
  		  gensym("mode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_threshold::floatMaxMessCallback,
  		  gensym("max"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_threshold::floatThreshMessCallback,
  		  gensym("threshold"), A_FLOAT, A_NULL);
}
void pix_opencv_threshold :: floatMaxMessCallback(void *data, t_floatarg maxvalue)
{
  GetMyClass(data)->floatMaxMess((float)maxvalue);
}
void pix_opencv_threshold :: floatThreshMessCallback(void *data, t_floatarg edge_thresh)
{
  GetMyClass(data)->floatThreshMess((float)edge_thresh);
}
void pix_opencv_threshold :: floatModeMessCallback(void *data, t_floatarg mode)
{
  GetMyClass(data)->floatModeMess((float)mode);
}
