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

#include "pix_opencv_edge.h"

CPPEXTERN_NEW(pix_opencv_edge)

/////////////////////////////////////////////////////////
//
// pix_opencv_edge
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_edge :: pix_opencv_edge()
{ 
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("ft1"));
  edge_thresh = 50;
  comp_xsize  = 0;
  comp_ysize  = 0;
  orig = NULL;
  gray = NULL;
  edge = NULL;
  cedge = NULL;
  cedgergb = NULL;
  rgb = NULL;

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_edge :: ~pix_opencv_edge()
{ 
    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&edge);
    	cvReleaseImage(&cedge);
    	cvReleaseImage(&cedgergb);
    	cvReleaseImage(&rgb);
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_edge :: processRGBAImage(imageStruct &image)
{
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!orig)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&edge);
    	cvReleaseImage(&cedge);
    	cvReleaseImage(&cedgergb);
    	cvReleaseImage(&rgb);

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);

    	// Create the output images with new sizes
    	cedge = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 4);

    	gray = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	edge = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    
    }
    // Here we make a copy of the pixel data from image to orig->imageData
    // orig is a IplImage struct, the default image type in openCV, take a look on the IplImage data structure here
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( orig->imageData, image.data, image.xsize*image.ysize*4 );
    
    // Convert to grayscale
    cvCvtColor(orig, gray, CV_BGRA2GRAY);
  
    cvSmooth( gray, edge, CV_BLUR, 3, 3, 0, 0 );
    cvNot( gray, edge );

    // Run the edge detector on grayscale
    cvCanny(gray, edge, (float)this->edge_thresh, (float)this->edge_thresh*3, 3);
  
    cvZero( cedge );
    // copy edge points
    cvCopy( orig, cedge, edge );

    //copy back the processed frame to image
    memcpy( image.data, cedge->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_edge :: processRGBImage(imageStruct &image)
{
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&edge);
    	cvReleaseImage(&cedge);
    	cvReleaseImage(&cedgergb);
    	cvReleaseImage(&rgb);

	//create the orig image with new size
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
    	cedgergb = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 3);

    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    	edge = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    
    // Convert to grayscale
    cvCvtColor(rgb, gray, CV_RGB2GRAY);
  
    cvSmooth( gray, edge, CV_BLUR, 3, 3, 0, 0 );
    cvNot( gray, edge );

    // Run the edge detector on grayscale
    cvCanny(gray, edge, (float)this->edge_thresh, (float)this->edge_thresh*3, 3);
  
    cvZero( cedgergb );
    // copy edge points
    cvCopy( rgb, cedgergb, edge );

    //cvShowImage(wndname, cedge);
    memcpy( image.data, cedgergb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_edge :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_edge : yuv format not supported" );
}
    	
void pix_opencv_edge :: processGrayImage(imageStruct &image)
{ 
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&edge);
    	cvReleaseImage(&cedge);
    	cvReleaseImage(&cedgergb);
    	cvReleaseImage(&rgb);

	//create the orig image with new size
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
    	cedgergb = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 3);

    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    	edge = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( gray->imageData, image.data, image.xsize*image.ysize );
    
    // Convert to RGB
    cvCvtColor( gray, rgb, CV_GRAY2RGB);
  
    cvSmooth( gray, edge, CV_BLUR, 3, 3, 0, 0 );
    cvNot( gray, edge );

    // Run the edge detector on grayscale
    cvCanny(gray, edge, (float)this->edge_thresh, (float)this->edge_thresh*3, 3);
  
    cvZero( cedgergb );
    // copy edge points
    cvCopy( rgb, cedgergb, edge );

    cvCvtColor( cedgergb, gray, CV_RGB2GRAY);
    //cvShowImage(wndname, cedge);
    memcpy( image.data, gray->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// floatThreshMess
//
/////////////////////////////////////////////////////////
void pix_opencv_edge :: floatThreshMess (float edge_thresh)
{
  this->edge_thresh = (int)edge_thresh;
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_edge :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_edge::floatTreshMessCallback,
  		  gensym("ft1"), A_FLOAT, A_NULL);
}
void pix_opencv_edge :: floatTreshMessCallback(void *data, t_floatarg edge_thresh)
{
  GetMyClass(data)->floatThreshMess((float)edge_thresh);
}
