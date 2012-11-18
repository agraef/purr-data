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

#include "pix_opencv_morphology.h"

CPPEXTERN_NEW(pix_opencv_morphology)

/////////////////////////////////////////////////////////
//
// pix_opencv_morphology
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_morphology :: pix_opencv_morphology()
{ 
  int i;

  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("ft1"));

  pos = 0;
  comp_xsize  = 0;
  comp_ysize  = 0;

  rgba = NULL;
  grey = NULL;
  rgb = NULL;
  dst = NULL;
  
  element_shape = CV_SHAPE_RECT;

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_morphology :: ~pix_opencv_morphology()
{
    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &dst );
    	cvReleaseImage( &rgba );
    	cvReleaseImage( &grey );
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_morphology :: processRGBAImage(imageStruct &image)
{
  unsigned char *pixels = image.data;
  int i;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgba)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &dst );
    	cvReleaseImage( &rgba );
    	cvReleaseImage( &grey );

	//Create cv_images 
    	rgb = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 3 );
	dst = cvCloneImage(rgb);
    	rgba = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 4 );
    	grey = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 1 );
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
    
    cvCvtColor(rgba, rgb, CV_RGBA2RGB);

    if (this->mode == 1) { //open/close
    	int n = pos;
    	int an = n > 0 ? n : -n;
    	element = cvCreateStructuringElementEx( an*2+1, an*2+1, an, an, element_shape, 0 );
    	if( n < 0 )
    	{
        	cvErode(rgb,dst,element,1);
        	cvDilate(dst,dst,element,1);
    	}
    	else
    	{
        	cvDilate(rgb,dst,element,1);
        	cvErode(dst,dst,element,1);
    	}
    	cvReleaseStructuringElement(&element);

    } else {
    	int n = pos;
    	int an = n > 0 ? n : -n;
    	element = cvCreateStructuringElementEx( an*2+1, an*2+1, an, an, element_shape, 0 );
    	if( n < 0 )
    	{
        	cvErode(rgb,dst,element,1);
    	}
    	else
    	{
        	cvDilate(rgb,dst,element,1);
    	}
    	cvReleaseStructuringElement(&element);

    }

    cvCvtColor(dst, rgba, CV_RGB2RGBA);
    memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_morphology :: processRGBImage(imageStruct &image)
{
  unsigned char *pixels = image.data;
  int i;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &dst );

	//Create cv_images 
    	rgb = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 4 );
	dst = cvCloneImage(rgb);
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );

    if (this->mode == 1) { //open/close
    	int n = pos;
    	int an = n > 0 ? n : -n;
    	element = cvCreateStructuringElementEx( an*2+1, an*2+1, an, an, element_shape, 0 );
    	if( n < 0 )
    	{
        	cvErode(rgb,dst,element,1);
        	cvDilate(dst,dst,element,1);
    	}
    	else
    	{
        	cvDilate(rgb,dst,element,1);
        	cvErode(dst,dst,element,1);
    	}
    	cvReleaseStructuringElement(&element);

    } else {
    	int n = pos;
    	int an = n > 0 ? n : -n;
    	element = cvCreateStructuringElementEx( an*2+1, an*2+1, an, an, element_shape, 0 );
    	if( n < 0 )
    	{
        	cvErode(rgb,dst,element,1);
    	}
    	else
    	{
        	cvDilate(rgb,dst,element,1);
    	}
    	cvReleaseStructuringElement(&element);

    }

    //cvShowImage(wndname, cedge);
    memcpy( image.data, dst->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_morphology :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_morphology : yuv format not supported" );
}
    	
void pix_opencv_morphology :: processGrayImage(imageStruct &image)
{ 
  unsigned char *pixels = image.data;
  int i;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!grey)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &dst );
    	cvReleaseImage( &grey );

	//Create cv_images 
    	rgb = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 3 );
	dst = cvCloneImage(rgb);
    	grey = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 1 );
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( grey->imageData, image.data, image.xsize*image.ysize );
    
    cvCvtColor(grey, rgb, CV_GRAY2RGB);
    
    if (this->mode == 1) { //open/close
    	int n = pos;
    	int an = n > 0 ? n : -n;
    	element = cvCreateStructuringElementEx( an*2+1, an*2+1, an, an, element_shape, 0 );
    	if( n < 0 )
    	{
        	cvErode(rgb,dst,element,1);
        	cvDilate(dst,dst,element,1);
    	}
    	else
    	{
        	cvDilate(rgb,dst,element,1);
        	cvErode(dst,dst,element,1);
    	}
    	cvReleaseStructuringElement(&element);

    } else {
    	int n = pos;
    	int an = n > 0 ? n : -n;
    	element = cvCreateStructuringElementEx( an*2+1, an*2+1, an, an, element_shape, 0 );
    	if( n < 0 )
    	{
        	cvErode(rgb,dst,element,1);
    	}
    	else
    	{
        	cvDilate(rgb,dst,element,1);
    	}
    	cvReleaseStructuringElement(&element);

    }

    cvCvtColor(dst, grey, CV_RGB2GRAY);
    //cvShowImage(wndname, cedge);
    memcpy( image.data, grey->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// floatPosMess
//
/////////////////////////////////////////////////////////
void pix_opencv_morphology :: floatPosMess (float pos)
{
  this->pos = (int)pos;
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_morphology :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_morphology::floatPosMessCallback,
  		  gensym("ft1"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_morphology::modeMessCallback,
		  gensym("mode"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_morphology::shapeMessCallback,
		  gensym("shape"), A_DEFFLOAT, A_NULL);
}
void pix_opencv_morphology :: floatPosMessCallback(void *data, t_floatarg pos)
{
  GetMyClass(data)->floatPosMess((float)pos);
}
void pix_opencv_morphology :: modeMessCallback(void *data, t_floatarg mode)
{
  GetMyClass(data)->mode=!(!(int)mode);
}
void pix_opencv_morphology :: shapeMessCallback(void *data, t_floatarg f)
{
        if( (int)f == 1 )
            GetMyClass(data)->element_shape = CV_SHAPE_RECT;
        else if( (int)f == 2 )
            GetMyClass(data)->element_shape = CV_SHAPE_ELLIPSE;
        else if( (int)f == 3 )
            GetMyClass(data)->element_shape = CV_SHAPE_CROSS;
}
