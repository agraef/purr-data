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

#include "pix_opencv_bgsubstract.h"

CPPEXTERN_NEW(pix_opencv_bgsubstract)

/////////////////////////////////////////////////////////
//
// pix_opencv_bgsubstract
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_bgsubstract :: pix_opencv_bgsubstract()
{ 
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("ft1"));
  x_threshold = 13;
  x_set       = 1;
  comp_xsize  = 0;
  comp_ysize  = 0;
  orig      = NULL;
  gray      = NULL;
  rgb  	    = NULL;
  grayLow   = NULL;
  grayUp    = NULL;
  prev_gray = NULL;
  diff_8U   = NULL;
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_bgsubstract :: ~pix_opencv_bgsubstract()
{ 
    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&rgb);
    	cvReleaseImage(&prev_gray);
    	cvReleaseImage(&grayLow);
    	cvReleaseImage(&grayUp);
    	cvReleaseImage(&diff_8U);
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_bgsubstract :: processRGBAImage(imageStruct &image)
{

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!orig)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&rgb);
    	cvReleaseImage(&prev_gray);
    	cvReleaseImage(&grayLow);
    	cvReleaseImage(&grayUp);
    	cvReleaseImage(&diff_8U);

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb  = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
    	gray = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	grayLow = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	grayUp = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	prev_gray = cvCreateImage(cvSize(orig->width,orig->height), 8, 1);
    	diff_8U = cvCreateImage(cvSize(orig->width,orig->height), 8, 1);
    }
    // Here we make a copy of the pixel data from image to orig->imageData
    // orig is a IplImage struct, the default image type in openCV, take a look on the IplImage data structure here
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( orig->imageData, image.data, image.xsize*image.ysize*4 );
    
    // Convert to grayscale
    cvCvtColor(orig, gray, CV_BGRA2GRAY);

    if (x_set) {
    	memcpy( prev_gray->imageData, gray->imageData, image.xsize*image.ysize );
	x_set=0;
    } 

    cvSubS (prev_gray,cvScalar(x_threshold,x_threshold,x_threshold,x_threshold),grayLow,NULL);
    cvAddS (prev_gray,cvScalar(x_threshold,x_threshold,x_threshold,x_threshold),grayUp,NULL);
    cvInRange (gray, grayLow, grayUp, diff_8U);

    cvNot (diff_8U,diff_8U);

    cvCvtColor(diff_8U, orig, CV_GRAY2BGRA);
  
    //copy back the processed frame to image
    memcpy( image.data, orig->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_bgsubstract :: processRGBImage(imageStruct &image)
{

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!orig)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&rgb);
    	cvReleaseImage(&prev_gray);
    	cvReleaseImage(&grayLow);
    	cvReleaseImage(&grayUp);
    	cvReleaseImage(&diff_8U);

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb  = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
    	gray = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	grayLow = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	grayUp = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	prev_gray = cvCreateImage(cvSize(orig->width,orig->height), 8, 1);
    	diff_8U = cvCreateImage(cvSize(orig->width,orig->height), 8, 1);
    }
    // Here we make a copy of the pixel data from image to orig->imageData
    // orig is a IplImage struct, the default image type in openCV, take a look on the IplImage data structure here
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    
    // Convert to grayscale
    cvCvtColor(rgb, gray, CV_BGRA2GRAY);

    if (x_set) {
    	memcpy( prev_gray->imageData, gray->imageData, image.xsize*image.ysize );
	x_set=0;
    } 

    cvSubS (prev_gray,cvScalar(x_threshold,x_threshold,x_threshold,x_threshold),grayLow,NULL);
    cvAddS (prev_gray,cvScalar(x_threshold,x_threshold,x_threshold,x_threshold),grayUp,NULL);
    cvInRange (gray, grayLow, grayUp, diff_8U);

    cvNot (diff_8U,diff_8U);

    cvCvtColor(diff_8U, rgb, CV_GRAY2BGR);
  
    //copy back the processed frame to image
    memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );

}

void pix_opencv_bgsubstract :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_bgsubstract : yuv format not supported" );
}
    	
void pix_opencv_bgsubstract :: processGrayImage(imageStruct &image)
{ 

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!orig)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&rgb);
    	cvReleaseImage(&prev_gray);
    	cvReleaseImage(&grayLow);
    	cvReleaseImage(&grayUp);
    	cvReleaseImage(&diff_8U);

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb  = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
    	gray = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	grayLow = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	grayUp = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	prev_gray = cvCreateImage(cvSize(orig->width,orig->height), 8, 1);
    	diff_8U = cvCreateImage(cvSize(orig->width,orig->height), 8, 1);
    }
    // Here we make a copy of the pixel data from image to orig->imageData
    // orig is a IplImage struct, the default image type in openCV, take a look on the IplImage data structure here
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( gray->imageData, image.data, image.xsize*image.ysize );
    

    if (x_set) {
    	memcpy( prev_gray->imageData, gray->imageData, image.xsize*image.ysize );
	x_set=0;
    } 

    cvSubS (prev_gray,cvScalar(x_threshold,x_threshold,x_threshold,x_threshold),grayLow,NULL);
    cvAddS (prev_gray,cvScalar(x_threshold,x_threshold,x_threshold,x_threshold),grayUp,NULL);
    cvInRange (gray, grayLow, grayUp, diff_8U);

    cvNot (diff_8U,diff_8U);

    //copy back the processed frame to image
    memcpy( image.data, diff_8U->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// floatThreshMess
//
/////////////////////////////////////////////////////////
void pix_opencv_bgsubstract :: floatThreshMess (float x_threshold)
{
  this->x_threshold = (int)x_threshold;
}
void pix_opencv_bgsubstract :: SetMess ()
{
  this->x_set = 1;
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_bgsubstract :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_bgsubstract::floatTreshMessCallback,
  		  gensym("ft1"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_bgsubstract::SetMessCallback,
  		  gensym("set"), A_NULL);
}
void pix_opencv_bgsubstract :: floatTreshMessCallback(void *data, t_floatarg x_threshold)
{
  GetMyClass(data)->floatThreshMess((float)x_threshold);
}
void pix_opencv_bgsubstract :: SetMessCallback(void *data)
{
  GetMyClass(data)->SetMess();
}
