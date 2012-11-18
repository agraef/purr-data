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

#include "pix_opencv_distrans.h"

CPPEXTERN_NEW(pix_opencv_distrans)

/////////////////////////////////////////////////////////
//
// pix_opencv_distrans
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_distrans :: pix_opencv_distrans()
{ 
  int i;

  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("ft1"));

  edge_thresh = 25;
  build_voronoi = 0;
  comp_xsize  = 0;
  comp_ysize  = 0;

  dist = NULL;
  dist8u1 = NULL;
  dist8u2 = NULL;
  dist8u = NULL;
  dist32s = NULL;
  rgb = NULL;
  gray = NULL;
  edge = NULL;
  labels = NULL;
  rgba = NULL;
  alpha = NULL;
  
  mask_size = CV_DIST_MASK_PRECISE;

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_distrans :: ~pix_opencv_distrans()
{
    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &gray );
    	cvReleaseImage( &edge );
    	cvReleaseImage( &dist );
    	cvReleaseImage( &dist8u );
    	cvReleaseImage( &dist8u1 );
    	cvReleaseImage( &dist8u2 );
    	cvReleaseImage( &dist32s );
    	cvReleaseImage( &labels );
    	cvReleaseImage( &rgba );
    	cvReleaseImage( &alpha );
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_distrans :: processRGBAImage(imageStruct &image)
{
  unsigned char *pixels = image.data;
  int i;
    static const uchar colors[][3] = 
    {
        {0,0,0},
        {255,0,0},
        {255,128,0},
        {255,255,0},
        {0,255,0},
        {0,128,255},
        {0,255,255},
        {0,0,255},
        {255,0,255}
    };
  int msize = mask_size;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &gray );
    	cvReleaseImage( &edge );
    	cvReleaseImage( &dist );
    	cvReleaseImage( &dist8u );
    	cvReleaseImage( &dist8u1 );
    	cvReleaseImage( &dist8u2 );
    	cvReleaseImage( &dist32s );
    	cvReleaseImage( &labels );
    	cvReleaseImage( &rgba );
    	cvReleaseImage( &alpha );

	//Create cv_images 
    	rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    	dist = cvCreateImage( cvGetSize(gray), IPL_DEPTH_32F, 1 );
    	dist8u1 = cvCloneImage( gray );
    	dist8u2 = cvCloneImage( gray );
    	dist8u = cvCreateImage( cvGetSize(gray), IPL_DEPTH_8U, 3 );
    	dist32s = cvCreateImage( cvGetSize(gray), IPL_DEPTH_32S, 1 );
    	edge = cvCloneImage( gray );
    	labels = cvCreateImage( cvGetSize(gray), IPL_DEPTH_32S, 1 );
    	rgba = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 4 );
    	alpha = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 1 );
    }
    memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
    
    cvCvtColor(rgba, rgb, CV_RGBA2RGB);
    cvCvtColor(rgb, gray, CV_RGB2GRAY);
    
    cvThreshold( gray, edge, (float)edge_thresh, (float)edge_thresh, CV_THRESH_BINARY );

    if( build_voronoi )
        msize = CV_DIST_MASK_5;

    cvDistTransform( edge, dist, CV_DIST_L2, msize, NULL, build_voronoi ? labels : NULL );

    if( !build_voronoi )
    {
        // begin "painting" the distance transform result
        cvConvertScale( dist, dist, 5000.0, 0 );
        cvPow( dist, dist, 0.5 );
    
        cvConvertScale( dist, dist32s, 1.0, 0.5 );
        cvAndS( dist32s, cvScalarAll(255), dist32s, 0 );
        cvConvertScale( dist32s, dist8u1, 1, 0 );
        cvConvertScale( dist32s, dist32s, -1, 0 );
        cvAddS( dist32s, cvScalarAll(255), dist32s, 0 );
        cvConvertScale( dist32s, dist8u2, 1, 0 );
        cvMerge( dist8u1, dist8u2, dist8u2, 0, dist8u );
        // end "painting" the distance transform result
    }
    else
    {
        int i, j;
        for( i = 0; i < labels->height; i++ )
        {
            int* ll = (int*)(labels->imageData + i*labels->widthStep);
            float* dd = (float*)(dist->imageData + i*dist->widthStep);
            uchar* d = (uchar*)(dist8u->imageData + i*dist8u->widthStep);
            for( j = 0; j < labels->width; j++ )
            {
                int idx = ll[j] == 0 || dd[j] == 0 ? 0 : (ll[j]-1)%8 + 1;
                int b = cvRound(colors[idx][0]);
                int g = cvRound(colors[idx][1]);
                int r = cvRound(colors[idx][2]);
                d[j*3] = (uchar)b;
                d[j*3+1] = (uchar)g;
                d[j*3+2] = (uchar)r;
            }
        }
    }


    cvCvtColor(dist8u, rgba, CV_RGB2RGBA);
    memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_distrans :: processRGBImage(imageStruct &image)
{
  unsigned char *pixels = image.data;
  int i;
    static const uchar colors[][3] = 
    {
        {0,0,0},
        {255,0,0},
        {255,128,0},
        {255,255,0},
        {0,255,0},
        {0,128,255},
        {0,255,255},
        {0,0,255},
        {255,0,255}
    };
  int msize = mask_size;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &gray );
    	cvReleaseImage( &edge );
    	cvReleaseImage( &dist );
    	cvReleaseImage( &dist8u );
    	cvReleaseImage( &dist8u1 );
    	cvReleaseImage( &dist8u2 );
    	cvReleaseImage( &dist32s );
    	cvReleaseImage( &labels );
    	cvReleaseImage( &rgba );
    	cvReleaseImage( &alpha );

	//Create cv_images 
    	rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    	dist = cvCreateImage( cvGetSize(gray), IPL_DEPTH_32F, 1 );
    	dist8u1 = cvCloneImage( gray );
    	dist8u2 = cvCloneImage( gray );
    	dist8u = cvCreateImage( cvGetSize(gray), IPL_DEPTH_8U, 3 );
    	dist32s = cvCreateImage( cvGetSize(gray), IPL_DEPTH_32S, 1 );
    	edge = cvCloneImage( gray );
    	labels = cvCreateImage( cvGetSize(gray), IPL_DEPTH_32S, 1 );
    	rgba = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 4 );
    	alpha = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 1 );
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    
    cvCvtColor(rgb, gray, CV_BGR2GRAY);
    
    cvThreshold( gray, edge, (float)edge_thresh, (float)edge_thresh, CV_THRESH_BINARY );

    if( build_voronoi )
        msize = CV_DIST_MASK_5;

    cvDistTransform( edge, dist, CV_DIST_L2, msize, NULL, build_voronoi ? labels : NULL );

    if( !build_voronoi )
    {
        // begin "painting" the distance transform result
        cvConvertScale( dist, dist, 5000.0, 0 );
        cvPow( dist, dist, 0.5 );
    
        cvConvertScale( dist, dist32s, 1.0, 0.5 );
        cvAndS( dist32s, cvScalarAll(255), dist32s, 0 );
        cvConvertScale( dist32s, dist8u1, 1, 0 );
        cvConvertScale( dist32s, dist32s, -1, 0 );
        cvAddS( dist32s, cvScalarAll(255), dist32s, 0 );
        cvConvertScale( dist32s, dist8u2, 1, 0 );
        cvMerge( dist8u1, dist8u2, dist8u2, 0, dist8u );
        // end "painting" the distance transform result
    }
    else
    {
        int i, j;
        for( i = 0; i < labels->height; i++ )
        {
            int* ll = (int*)(labels->imageData + i*labels->widthStep);
            float* dd = (float*)(dist->imageData + i*dist->widthStep);
            uchar* d = (uchar*)(dist8u->imageData + i*dist8u->widthStep);
            for( j = 0; j < labels->width; j++ )
            {
                int idx = ll[j] == 0 || dd[j] == 0 ? 0 : (ll[j]-1)%8 + 1;
                int b = cvRound(colors[idx][0]);
                int g = cvRound(colors[idx][1]);
                int r = cvRound(colors[idx][2]);
                d[j*3] = (uchar)b;
                d[j*3+1] = (uchar)g;
                d[j*3+2] = (uchar)r;
            }
        }
    }

    //cvShowImage(wndname, cedge);
    memcpy( image.data, dist8u->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_distrans :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_distrans : yuv format not supported" );
}
    	
void pix_opencv_distrans :: processGrayImage(imageStruct &image)
{ 
  unsigned char *pixels = image.data;
  int i;
    static const uchar colors[][3] = 
    {
        {0,0,0},
        {255,0,0},
        {255,128,0},
        {255,255,0},
        {0,255,0},
        {0,128,255},
        {0,255,255},
        {0,0,255},
        {255,0,255}
    };
  int msize = mask_size;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &gray );
    	cvReleaseImage( &edge );
    	cvReleaseImage( &dist );
    	cvReleaseImage( &dist8u );
    	cvReleaseImage( &dist8u1 );
    	cvReleaseImage( &dist8u2 );
    	cvReleaseImage( &dist32s );
    	cvReleaseImage( &labels );
    	cvReleaseImage( &rgba );
    	cvReleaseImage( &alpha );

	//Create cv_images 
    	rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    	dist = cvCreateImage( cvGetSize(gray), IPL_DEPTH_32F, 1 );
    	dist8u1 = cvCloneImage( gray );
    	dist8u2 = cvCloneImage( gray );
    	dist8u = cvCreateImage( cvGetSize(gray), IPL_DEPTH_8U, 3 );
    	dist32s = cvCreateImage( cvGetSize(gray), IPL_DEPTH_32S, 1 );
    	edge = cvCloneImage( gray );
    	labels = cvCreateImage( cvGetSize(gray), IPL_DEPTH_32S, 1 );
    	rgba = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 4 );
    	alpha = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 1 );
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( gray->imageData, image.data, image.xsize*image.ysize );
    
    
    cvThreshold( gray, edge, (float)edge_thresh, (float)edge_thresh, CV_THRESH_BINARY );

    if( build_voronoi )
        msize = CV_DIST_MASK_5;

    cvDistTransform( edge, dist, CV_DIST_L2, msize, NULL, build_voronoi ? labels : NULL );

    if( !build_voronoi )
    {
        // begin "painting" the distance transform result
        cvConvertScale( dist, dist, 5000.0, 0 );
        cvPow( dist, dist, 0.5 );
    
        cvConvertScale( dist, dist32s, 1.0, 0.5 );
        cvAndS( dist32s, cvScalarAll(255), dist32s, 0 );
        cvConvertScale( dist32s, dist8u1, 1, 0 );
        cvConvertScale( dist32s, dist32s, -1, 0 );
        cvAddS( dist32s, cvScalarAll(255), dist32s, 0 );
        cvConvertScale( dist32s, dist8u2, 1, 0 );
        cvMerge( dist8u1, dist8u2, dist8u2, 0, dist8u );
        // end "painting" the distance transform result
    }
    else
    {
        int i, j;
        for( i = 0; i < labels->height; i++ )
        {
            int* ll = (int*)(labels->imageData + i*labels->widthStep);
            float* dd = (float*)(dist->imageData + i*dist->widthStep);
            uchar* d = (uchar*)(dist8u->imageData + i*dist8u->widthStep);
            for( j = 0; j < labels->width; j++ )
            {
                int idx = ll[j] == 0 || dd[j] == 0 ? 0 : (ll[j]-1)%8 + 1;
                int b = cvRound(colors[idx][0]);
                int g = cvRound(colors[idx][1]);
                int r = cvRound(colors[idx][2]);
                d[j*3] = (uchar)b;
                d[j*3+1] = (uchar)g;
                d[j*3+2] = (uchar)r;
            }
        }
    }


    cvCvtColor(dist8u, gray, CV_RGB2GRAY);
    //cvShowImage(wndname, cedge);
    memcpy( image.data, gray->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_distrans :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_distrans::thresholdMessCallback,
  		  gensym("ft1"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_distrans::voronoiMessCallback,
		  gensym("voronoi"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_distrans::maskMessCallback,
		  gensym("mask"), A_DEFFLOAT, A_NULL);
}
void pix_opencv_distrans :: thresholdMessCallback(void *data, t_floatarg pos)
{
  if (pos>=0) GetMyClass(data)->edge_thresh = (int)pos;
}
void pix_opencv_distrans :: voronoiMessCallback(void *data, t_floatarg voronoi)
{
  GetMyClass(data)->build_voronoi=!(!(int)voronoi);
}
void pix_opencv_distrans :: maskMessCallback(void *data, t_floatarg f)
{
        if( (int)f == 3 )
            GetMyClass(data)->mask_size = CV_DIST_MASK_3;
        else if( (int)f == 5 )
            GetMyClass(data)->mask_size = CV_DIST_MASK_5;
        else if( (int)f == 0 )
            GetMyClass(data)->mask_size = CV_DIST_MASK_PRECISE;
}
