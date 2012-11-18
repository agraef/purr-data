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

#include "pix_opencv_dft.h"

CPPEXTERN_NEW(pix_opencv_dft)

/////////////////////////////////////////////////////////
//
// pix_opencv_dft
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

pix_opencv_dft :: pix_opencv_dft()
{ 
  int i;

  x_calculate = 1;
  comp_xsize=0;
  comp_ysize=0;

  rgb = NULL;
  rgba = NULL;
  gray = NULL;
  input_re = NULL;
  input_im = NULL;
  input_co = NULL;
  dft_A = NULL;
  image_re = NULL;
  image_im = NULL;
  image_mout = NULL;
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_dft :: ~pix_opencv_dft()
{
    //Destroy cv_images to clean memory
    cvReleaseImage( &rgb );
    cvReleaseImage( &rgba );
    //cvReleaseImage( &gray );
    cvReleaseImage( &input_re );
    cvReleaseImage( &input_im );
    cvReleaseImage( &input_co );
    cvReleaseMat( &dft_A );
    //cvReleaseImage( &mage_re );
    //cvReleaseImage( &image_im );
    //cvReleaseImage( &image_mout );

}

/////////////////////////////////////////////////////////
// shiftDFT
//
/////////////////////////////////////////////////////////
void pix_opencv_dft :: shiftDFT(CvArr * src_arr, CvArr * dst_arr )
{
    CvMat *tmp=NULL;
    CvMat q1stub, q2stub;
    CvMat q3stub, q4stub;
    CvMat d1stub, d2stub;
    CvMat d3stub, d4stub;
    CvMat * q1, * q2, * q3, * q4;
    CvMat * d1, * d2, * d3, * d4;

    CvSize size = cvGetSize(src_arr);
    CvSize dst_size = cvGetSize(dst_arr);
    int cx, cy;

    if(dst_size.width != size.width ||
       dst_size.height != size.height){
            return;
    }

    if(src_arr==dst_arr){
        tmp = cvCreateMat(size.height/2, size.width/2, cvGetElemType(src_arr));
    }

    cx = size.width/2;
    cy = size.height/2; // image center

    q1 = cvGetSubRect( src_arr, &q1stub, cvRect(0,0,cx, cy) );
    q2 = cvGetSubRect( src_arr, &q2stub, cvRect(cx,0,cx,cy) );
    q3 = cvGetSubRect( src_arr, &q3stub, cvRect(cx,cy,cx,cy) );
    q4 = cvGetSubRect( src_arr, &q4stub, cvRect(0,cy,cx,cy) );
    d1 = cvGetSubRect( src_arr, &d1stub, cvRect(0,0,cx,cy) );
    d2 = cvGetSubRect( src_arr, &d2stub, cvRect(cx,0,cx,cy) );
    d3 = cvGetSubRect( src_arr, &d3stub, cvRect(cx,cy,cx,cy) );
    d4 = cvGetSubRect( src_arr, &d4stub, cvRect(0,cy,cx,cy) );

    if(src_arr!=dst_arr)
    {
        if( !CV_ARE_TYPES_EQ( q1, d1 )){
            return;
        }
        cvCopy(q3, d1, 0);
        cvCopy(q4, d2, 0);
        cvCopy(q1, d3, 0);
        cvCopy(q2, d4, 0);
    }
    else
    {
        cvCopy(q3, tmp, 0);
        cvCopy(q1, q3, 0);
        cvCopy(tmp, q1, 0);
        cvCopy(q4, tmp, 0);
        cvCopy(q2, q4, 0);
        cvCopy(tmp, q2, 0);
    }
    if(src_arr==dst_arr){
        cvReleaseMat( &tmp );
    }

}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_dft :: processRGBAImage(imageStruct &image)
{
  unsigned char *pixels = image.data;
  int i;
  CvMat tmp;
  double m,M;
  int px,py;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

        this->comp_xsize=image.xsize;
        this->comp_ysize=image.ysize;

    	//Destroy cv_images to clean memory
        cvReleaseImage( &rgb );
        cvReleaseImage( &rgba );
        //cvReleaseImage( &gray );
        cvReleaseImage( &input_re );
        cvReleaseImage( &input_im );
        cvReleaseImage( &input_co );
        cvReleaseMat( &dft_A );
        //cvReleaseImage( &image_re );
        //cvReleaseImage( &image_im );
        //cvReleaseImage( &image_mout );
        //cvReleaseImage( &image_pout );

	//Create cv_images 
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        gray = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 1);
        input_re = cvCreateImage( cvGetSize(rgb), IPL_DEPTH_64F, 1);
        input_im = cvCreateImage( cvGetSize(rgb), IPL_DEPTH_64F, 1);
        input_co = cvCreateImage( cvGetSize(rgb), IPL_DEPTH_64F, 2);
        dft_M = cvGetOptimalDFTSize( image.ysize - 1 );
        dft_N = cvGetOptimalDFTSize( image.xsize - 1 );
        dft_A = cvCreateMat( dft_M, dft_N, CV_64FC2 );
        image_re = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_64F, 1);
        image_im = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_64F, 1);
        image_mout = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_8U, 1);

    }

    memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
    cvCvtColor(rgba, gray, CV_BGRA2GRAY);

    if ( x_calculate )
    {
      // discrete fourier transform
      cvScale(gray, input_re, 1.0, 0.0);
      cvZero(input_im);
      cvMerge(input_re, input_im, NULL, NULL, input_co);
 
      // copy A to dft_A and pad dft_A with zeros
      cvGetSubRect( dft_A, &tmp, cvRect(0,0, gray->width, gray->height));
      cvCopy( input_co, &tmp, NULL );
      if( dft_A->cols > gray->width )
      {
          cvGetSubRect( dft_A, &tmp, cvRect(gray->width,0, dft_A->cols - gray->width, gray->height));
          cvZero( &tmp );
      }
 
      // no need to pad bottom part of dft_A with zeros because of
      // use nonzero_rows parameter in cvDFT() call below
      cvDFT( dft_A, dft_A, CV_DXT_FORWARD, input_co->height );
 
      // Split Fourier in real and imaginary parts
      cvSplit( dft_A, image_re, image_im, 0, 0 );
 
      // Compute the magnitude of the spectrum Mag = sqrt(Re^2 + Im^2)
      cvPow( image_re, image_re, 2.0);
      cvPow( image_im, image_im, 2.0);
      cvAdd( image_re, image_im, image_re, NULL);
      cvPow( image_re, image_re, 0.5 );

      // Compute log(1 + Mag)
      cvAddS( image_re, cvScalarAll(1.0), image_re, NULL ); // 1 + Mag
      cvLog( image_re, image_re ); // log(1 + Mag)

      // Rearrange the quadrants of Fourier image so that the origin is at
      // the image center
      this->shiftDFT( image_re, image_re );

      // normalize image
      cvMinMaxLoc(image_re, &m, &M, NULL, NULL, NULL);
      cvScale(image_re, image_re, 255.0/(M-m), 255.0*(-m)/(M-m));

      for( py=0; py<image_re->height; py++ ) {
         double* ptri = (double*) ( image_re->imageData + py * image_re->widthStep);
         unsigned char* ptrp = (unsigned char*) ( image_mout->imageData + py * image_mout->widthStep);
         for( px=0; px<image_re->width; px++ ) {
           if ( *(ptrp+px) > 255.0 ) post( "pix_opencv_dft : error value over 255" );
           (*(ptrp+px)) = (unsigned char)( (*(ptri+px)) );
         }
      }

      x_calculate=0;
    }

    cvCvtColor(image_mout, rgba, CV_GRAY2RGBA);
    memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_dft :: processRGBImage(imageStruct &image)
{
  unsigned char *pixels = image.data;
  int i;
  CvMat tmp;
  double m,M;
  int px,py;

    if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

        this->comp_xsize=image.xsize;
        this->comp_ysize=image.ysize;

    	//Destroy cv_images to clean memory
        cvReleaseImage( &rgb );
        cvReleaseImage( &rgba );
        //cvReleaseImage( &gray );
        cvReleaseImage( &input_re );
        cvReleaseImage( &input_im );
        cvReleaseImage( &input_co );
        cvReleaseMat( &dft_A );
        //cvReleaseImage( &image_re );
        //cvReleaseImage( &image_im );
        //cvReleaseImage( &image_mout );
        //cvReleaseImage( &image_pout );

	//Create cv_images 
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        gray = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 1);
        input_re = cvCreateImage( cvGetSize(rgb), IPL_DEPTH_64F, 1);
        input_im = cvCreateImage( cvGetSize(rgb), IPL_DEPTH_64F, 1);
        input_co = cvCreateImage( cvGetSize(rgb), IPL_DEPTH_64F, 2);
        dft_M = cvGetOptimalDFTSize( image.ysize - 1 );
        dft_N = cvGetOptimalDFTSize( image.xsize - 1 );
        dft_A = cvCreateMat( dft_M, dft_N, CV_64FC2 );
        image_re = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_64F, 1);
        image_im = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_64F, 1);
        image_mout = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_8U, 1);

    }

    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    cvCvtColor(rgb, gray, CV_BGR2GRAY);

    if ( x_calculate )
    {
      // discrete fourier transform
      cvScale(gray, input_re, 1.0, 0.0);
      cvZero(input_im);
      cvMerge(input_re, input_im, NULL, NULL, input_co);
 
      // copy A to dft_A and pad dft_A with zeros
      cvGetSubRect( dft_A, &tmp, cvRect(0,0, gray->width, gray->height));
      cvCopy( input_co, &tmp, NULL );
      if( dft_A->cols > gray->width )
      {
          cvGetSubRect( dft_A, &tmp, cvRect(gray->width,0, dft_A->cols - gray->width, gray->height));
          cvZero( &tmp );
      }
 
      // no need to pad bottom part of dft_A with zeros because of
      // use nonzero_rows parameter in cvDFT() call below
      cvDFT( dft_A, dft_A, CV_DXT_FORWARD, input_co->height );
 
      // Split Fourier in real and imaginary parts
      cvSplit( dft_A, image_re, image_im, 0, 0 );
 
      // Compute the magnitude of the spectrum Mag = sqrt(Re^2 + Im^2)
      cvPow( image_re, image_re, 2.0);
      cvPow( image_im, image_im, 2.0);
      cvAdd( image_re, image_im, image_re, NULL);
      cvPow( image_re, image_re, 0.5 );

      // Compute log(1 + Mag)
      cvAddS( image_re, cvScalarAll(1.0), image_re, NULL ); // 1 + Mag
      cvLog( image_re, image_re ); // log(1 + Mag)

      // Rearrange the quadrants of Fourier image so that the origin is at
      // the image center
      this->shiftDFT( image_re, image_re );

      // normalize image
      cvMinMaxLoc(image_re, &m, &M, NULL, NULL, NULL);
      cvScale(image_re, image_re, 255.0/(M-m), 255.0*(-m)/(M-m));

      for( py=0; py<image_re->height; py++ ) {
         double* ptri = (double*) ( image_re->imageData + py * image_re->widthStep);
         unsigned char* ptrp = (unsigned char*) ( image_mout->imageData + py * image_mout->widthStep);
         for( px=0; px<image_re->width; px++ ) {
           if ( *(ptrp+px) > 255.0 ) post( "pix_opencv_dft : error value over 255" );
           (*(ptrp+px)) = (unsigned char)( (*(ptri+px)) );
         }
      }

      x_calculate=0;
    }

    cvCvtColor(image_mout, rgb, CV_GRAY2RGB);
    memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_dft :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_contours_convexity : yuv format not supported" );
}
    	
void pix_opencv_dft :: processGrayImage(imageStruct &image)
{ 
  unsigned char *pixels = image.data;
  int i;
  CvMat tmp;
  double m,M;
  int px,py;

    if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

        this->comp_xsize=image.xsize;
        this->comp_ysize=image.ysize;

    	//Destroy cv_images to clean memory
        cvReleaseImage( &rgb );
        cvReleaseImage( &rgba );
        //cvReleaseImage( &gray );
        cvReleaseImage( &input_re );
        cvReleaseImage( &input_im );
        cvReleaseImage( &input_co );
        cvReleaseMat( &dft_A );
        //cvReleaseImage( &image_re );
        //cvReleaseImage( &image_im );
        //cvReleaseImage( &image_mout );
        //cvReleaseImage( &image_pout );

	//Create cv_images 
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        gray = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 1);
        input_re = cvCreateImage( cvGetSize(rgb), IPL_DEPTH_64F, 1);
        input_im = cvCreateImage( cvGetSize(rgb), IPL_DEPTH_64F, 1);
        input_co = cvCreateImage( cvGetSize(rgb), IPL_DEPTH_64F, 2);
        dft_M = cvGetOptimalDFTSize( image.ysize - 1 );
        dft_N = cvGetOptimalDFTSize( image.xsize - 1 );
        dft_A = cvCreateMat( dft_M, dft_N, CV_64FC2 );
        image_re = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_64F, 1);
        image_im = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_64F, 1);
        image_mout = cvCreateImage( cvSize(dft_N, dft_M), IPL_DEPTH_8U, 1);

    }

    memcpy( gray->imageData, image.data, image.xsize*image.ysize );

    if ( x_calculate )
    {
      // discrete fourier transform
      cvScale(gray, input_re, 1.0, 0.0);
      cvZero(input_im);
      cvMerge(input_re, input_im, NULL, NULL, input_co);
 
      // copy A to dft_A and pad dft_A with zeros
      cvGetSubRect( dft_A, &tmp, cvRect(0,0, gray->width, gray->height));
      cvCopy( input_co, &tmp, NULL );
      if( dft_A->cols > gray->width )
      {
          cvGetSubRect( dft_A, &tmp, cvRect(gray->width,0, dft_A->cols - gray->width, gray->height));
          cvZero( &tmp );
      }
 
      // no need to pad bottom part of dft_A with zeros because of
      // use nonzero_rows parameter in cvDFT() call below
      cvDFT( dft_A, dft_A, CV_DXT_FORWARD, input_co->height );
 
      // Split Fourier in real and imaginary parts
      cvSplit( dft_A, image_re, image_im, 0, 0 );
 
      // Compute the magnitude of the spectrum Mag = sqrt(Re^2 + Im^2)
      cvPow( image_re, image_re, 2.0);
      cvPow( image_im, image_im, 2.0);
      cvAdd( image_re, image_im, image_re, NULL);
      cvPow( image_re, image_re, 0.5 );

      // Compute log(1 + Mag)
      cvAddS( image_re, cvScalarAll(1.0), image_re, NULL ); // 1 + Mag
      cvLog( image_re, image_re ); // log(1 + Mag)

      // Rearrange the quadrants of Fourier image so that the origin is at
      // the image center
      this->shiftDFT( image_re, image_re );

      // normalize image
      cvMinMaxLoc(image_re, &m, &M, NULL, NULL, NULL);
      cvScale(image_re, image_re, 255.0/(M-m), 255.0*(-m)/(M-m));

      for( py=0; py<image_re->height; py++ ) {
         double* ptri = (double*) ( image_re->imageData + py * image_re->widthStep);
         unsigned char* ptrp = (unsigned char*) ( image_mout->imageData + py * image_mout->widthStep);
         for( px=0; px<image_re->width; px++ ) {
           if ( *(ptrp+px) > 255.0 ) post( "pix_opencv_dft : error value over 255" );
           (*(ptrp+px)) = (unsigned char)( (*(ptri+px)) );
         }
      }

      x_calculate=0;
    }

    memcpy( image.data, image_mout->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////

void pix_opencv_dft :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_dft::calculateCallback,
		  gensym("bang"), A_NULL);
}

void pix_opencv_dft :: calculateCallback(void *data)
{
  GetMyClass(data)->x_calculate = 1.0;
}
