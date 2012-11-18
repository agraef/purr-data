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

#include "pix_opencv_contours_boundingrect.h"
#include <stdio.h>

CPPEXTERN_NEW(pix_opencv_contours_boundingrect)

/////////////////////////////////////////////////////////
//
// pix_opencv_contours_boundingrect
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_contours_boundingrect :: pix_opencv_contours_boundingrect()
{ 
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("minarea"));
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("maxarea"));
  m_dataout = outlet_new(this->x_obj, 0);
  m_countout = outlet_new(this->x_obj, 0);
  minarea = 10*10;
  maxarea = 320*240;
  comp_xsize  = 320;
  comp_ysize  = 240;
  orig = NULL;
  gray = NULL;
  cnt_img = NULL;
  rgb = NULL;
  x_ftolerance  = 5;
  x_mmove   = 20;
  x_nightmode   = 0;
  x_show   = 0;
  x_draw   = 1;
  x_cmode   = CV_RETR_LIST;
  x_cmethod = CV_CHAIN_APPROX_SIMPLE;

  // initialize font
  cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 1.0, 1, 8 );

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_contours_boundingrect :: ~pix_opencv_contours_boundingrect()
{ 
    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&cnt_img);
    	cvReleaseImage(&rgb);
}

/////////////////////////////////////////////////////////
// Mark a contour
//
/////////////////////////////////////////////////////////
int pix_opencv_contours_boundingrect :: mark(float fx, float fy, float fw, float fh )
{
  int i;

    if ( ( fx < 0.0 ) || ( fx > this->comp_xsize ) || ( fy < 0 ) || ( fy > this->comp_ysize ) )
    {
       return -1;
    }

    for ( i=0; i<MAX_MARKERS; i++)
    {
       if ( x_xmark[i] == -1 )
       {
          x_xmark[i] = (float)(fx+(fw/2));
          x_ymark[i] = (float)(fy+(fh/2));
          x_wmark[i] = (int)fw;
          x_hmark[i] = (int)fh;
          x_found[i] = x_ftolerance;
          return i;
       }
    }

    // post( "pix_opencv_contours_boundingrect : max markers reached" );
    return -1;
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_contours_boundingrect :: processRGBAImage(imageStruct &image)
{
  unsigned char *pixels = image.data;
  char tindex[4];
  int im = 0, i, ic;           // Indicator of markers.
  int oi, found;               
  float dist, odist;           // Distances
  t_atom rlist[5];

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!orig)) 
  {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&cnt_img);
    	cvReleaseImage(&rgb);

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);

    	// Create the output images with new sizes
    	rgb = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 3);

    	gray = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	cnt_img = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    
    }
    // Here we make a copy of the pixel data from image to orig->imageData
    // orig is a IplImage struct, the default image type in openCV, take a look on the IplImage data structure here
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( orig->imageData, image.data, image.xsize*image.ysize*4 );
    
    // Convert to grayscale
    cvCvtColor(orig, gray, CV_RGBA2GRAY);
    if ( x_nightmode )
    {
       cvZero( orig );
    }

    CvSeq* contours;
    CvMemStorage* stor02;
    stor02 = cvCreateMemStorage(0);

    cvFindContours( gray, stor02, &contours, sizeof(CvContour), x_cmode, x_cmethod, cvPoint(0,0) );
    if (contours) contours = cvApproxPoly( contours, sizeof(CvContour), stor02, CV_POLY_APPROX_DP, 3, 1 );
  
    for ( im=0; im<MAX_MARKERS; im++ )
    {
      if ( x_xmark[im] != -1.0 )
      {
        x_found[im]--;
      }
    }

    i = 0;                   // Indicator of cycles.
    ic = 0;                  // Indicator of contours.
    for( ; contours != 0; contours = contours->h_next )
    {
        int count = contours->total; // This is number point in contour
        CvRect rect;

	rect = cvContourBoundingRect( contours, 1);

	if ( ( (rect.width*rect.height) > minarea ) && ( (rect.width*rect.height) < maxarea ) ) 
        {

            found = 0;
            oi = -1;
            dist=(comp_xsize>comp_ysize)?comp_xsize:comp_ysize;

            for ( im=0; im<MAX_MARKERS; im++ )
            {

              if ( x_xmark[im]==-1 ) continue;  // no contours

              odist=sqrt( pow( ((float)rect.x+rect.width/2)-x_xmark[im], 2 ) + pow( ((float)rect.y+rect.height/2)-x_ymark[im], 2 ) );

              // search for the closest known contour
              // that is likely to be this one
              if ( odist < x_mmove )
              {
                if ( odist < dist )
                {
                  found=1;
                  oi=im;
                  x_xmark[oi] = (float)(rect.x+rect.width/2);
                  x_ymark[oi] = (float)(rect.y+rect.height/2);
                  x_wmark[oi] = (int)rect.width;
                  x_hmark[oi] = (int)rect.height;
                  x_found[oi] = x_ftolerance;
                  dist=odist;
                }
              }
            }

            // new object detected
            if ( !found )
            {
               oi = this->mark(rect.x, rect.y, rect.width, rect.height );
            }

            if ( x_draw )
            {
	      cvRectangle( orig, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(255,0,0), 2, 8 , 0 );
              sprintf( tindex, "%d", oi );
              cvPutText( orig, tindex, cvPoint(x_xmark[oi],x_ymark[oi]), &font, CV_RGB(255,0,0));
            }

            if ( x_show )
            {
              cvDrawContours( orig, contours, CV_RGB(255,255,255), CV_RGB(255,255,255), 0, 1, 8, cvPoint(0,0) );
            }

            SETFLOAT(&rlist[0], oi);
            SETFLOAT(&rlist[1], rect.x);
            SETFLOAT(&rlist[2], rect.y);
            SETFLOAT(&rlist[3], rect.width);
            SETFLOAT(&rlist[4], rect.height);

    	    outlet_list( m_dataout, 0, 5, rlist );
	    i++;
	    ic++;
	}
    }

    outlet_float( m_countout, ic );

    // delete lost objects
    for ( im=0; im<MAX_MARKERS; im++ )
    {
       if ( x_found[im] < 0 )
       {
         x_xmark[im] = -1.0;
         x_ymark[im] = -1,0;
         x_found[im] = x_ftolerance;
         SETFLOAT(&rlist[0], im);
         SETFLOAT(&rlist[1], -1.0);
         SETFLOAT(&rlist[2], -1.0);
         SETFLOAT(&rlist[3], 0.0);
         SETFLOAT(&rlist[4], 0.0);
    	 outlet_list( m_dataout, 0, 5, rlist );
       }
    }

    cvReleaseMemStorage( &stor02 );

    //copy back the processed frame to image
    memcpy( image.data, orig->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_contours_boundingrect :: processRGBImage(imageStruct &image)
{
  unsigned char *pixels = image.data;
  char tindex[4];
  t_atom rlist[5];
  int im = 0;                  // Indicator of markers.
  int oi, found;  
  float dist, odist;           // Distances

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) 
  {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&cnt_img);
    	cvReleaseImage(&rgb);

	//create the orig image with new size
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);

    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    	cnt_img = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    
    // Convert to grayscale
    cvCvtColor(rgb, gray, CV_RGB2GRAY);
    if ( x_nightmode )
    {
       cvZero( rgb );
    }
  
    CvSeq* contours;
    CvMemStorage* stor02;
    stor02 = cvCreateMemStorage(0);

    cvFindContours( gray, stor02, &contours, sizeof(CvContour), x_cmode, x_cmethod, cvPoint(0,0) );
    if (contours) contours = cvApproxPoly( contours, sizeof(CvContour), stor02, CV_POLY_APPROX_DP, 3, 1 );
  
    for ( im=0; im<MAX_MARKERS; im++ )
    {
      if ( x_xmark[im] != -1.0 )
      {
        x_found[im]--;
      }
    }

    int i = 0;                   // Indicator of cycles.
    int ic = 0;                  // Indicator of contours.
    for( ; contours != 0; contours = contours->h_next )
    {
        int count = contours->total; // This is number point in contour
        CvRect rect;

	rect = cvContourBoundingRect( contours, 1);
	if ( ( (rect.width*rect.height) > minarea ) && ( (rect.width*rect.height) < maxarea ) ) 
        {

            found = 0;
            oi = -1;
            dist=(comp_xsize>comp_ysize)?comp_xsize:comp_ysize;

            for ( im=0; im<MAX_MARKERS; im++ )
            {

              if ( x_xmark[im]==-1 ) continue;

              odist=sqrt( pow( ((float)rect.x+rect.width/2)-x_xmark[im], 2 ) + pow( ((float)rect.y+rect.height/2)-x_ymark[im], 2 ) );

              // search for the closest known contour
              // that is likely to be this one
              if ( odist < x_mmove )
              {
                if ( odist < dist )
                {
                  found=1;
                  oi=im;
                  x_xmark[oi] = (float)(rect.x+rect.width/2);
                  x_ymark[oi] = (float)(rect.y+rect.height/2);
                  x_wmark[oi] = (int)rect.width;
                  x_hmark[oi] = (int)rect.height;
                  x_found[oi] = x_ftolerance;
                  dist=odist;
                }
              }
            }

            // new object detected
            if ( !found )
            {
               oi = this->mark(rect.x, rect.y, rect.width, rect.height );
            }

            if ( x_draw )
            {
	      cvRectangle( rgb, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(255,0,0), 2, 8 , 0 );
              sprintf( tindex, "%d", oi );
              cvPutText( rgb, tindex, cvPoint(x_xmark[oi],x_ymark[oi]), &font, CV_RGB(255,0,0));
            }

            if ( x_show )
            {
              cvDrawContours( rgb, contours, CV_RGB(255,255,255), CV_RGB(255,255,255), 0, 1, 8, cvPoint(0,0) );
            }

            SETFLOAT(&rlist[0], oi);
            SETFLOAT(&rlist[1], rect.x);
            SETFLOAT(&rlist[2], rect.y);
            SETFLOAT(&rlist[3], rect.width);
            SETFLOAT(&rlist[4], rect.height);

    	    outlet_list( m_dataout, 0, 5, rlist );
	    i++;
	    ic++;
	}
    }
    outlet_float( m_countout, ic );

    // delete lost objects
    for ( im=0; im<MAX_MARKERS; im++ )
    {
       if ( x_found[im] < 0 )
       {
         x_xmark[im] = -1.0;
         x_ymark[im] = -1,0;
         x_found[im] = x_ftolerance;
         SETFLOAT(&rlist[0], im);
         SETFLOAT(&rlist[1], -1.0);
         SETFLOAT(&rlist[2], -1.0);
         SETFLOAT(&rlist[3], 0.0);
         SETFLOAT(&rlist[4], 0.0);
    	 outlet_list( m_dataout, 0, 5, rlist );
       }
    }

    cvReleaseMemStorage( &stor02 );

    memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_contours_boundingrect :: processYUVImage(imageStruct &image)
{
   post( "pix_opencv_contours_boundingrect : yuv format not supported" );
}
    	
void pix_opencv_contours_boundingrect :: processGrayImage(imageStruct &image)
{ 
  char tindex[4];
  t_atom rlist[5];
  int im = 0;                  // Indicator of markers.
  int oi, found;
  float dist, odist;           // Distances

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!orig)) 
  {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&cnt_img);
    	cvReleaseImage(&rgb);

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);

    	// Create the output images with new sizes
    	rgb = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 3);

    	gray = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    	cnt_img = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    
    }
    // Here we make a copy of the pixel data from image to orig->imageData
    // orig is a IplImage struct, the default image type in openCV, take a look on the IplImage data structure here
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( gray->imageData, image.data, image.xsize*image.ysize );
    memcpy( cnt_img->imageData, image.data, image.xsize*image.ysize );
    if ( x_nightmode )
    {
       cvZero( cnt_img );
    }

    CvSeq* contours;
    CvMemStorage* stor02;
    stor02 = cvCreateMemStorage(0);

    cvFindContours( gray, stor02, &contours, sizeof(CvContour), x_cmode, x_cmethod, cvPoint(0,0) );
    if (contours) contours = cvApproxPoly( contours, sizeof(CvContour), stor02, CV_POLY_APPROX_DP, 3, 1 );
  
    for ( im=0; im<MAX_MARKERS; im++ )
    {
      if ( x_xmark[im] != -1.0 )
      {
        x_found[im]--;
      }
    }

    int i = 0;                   // Indicator of cycles.
    int ic = 0;                  // Indicator of contours.
    for( ; contours != 0; contours = contours->h_next )
    {
        int count = contours->total; // This is number point in contour
        CvRect rect;

	rect = cvContourBoundingRect( contours, 1);
	if ( ( (rect.width*rect.height) > minarea ) && ( (rect.width*rect.height) < maxarea ) ) 
        {

            found = 0;
            oi = -1;
            dist=(comp_xsize>comp_ysize)?comp_xsize:comp_ysize;

            for ( im=0; im<MAX_MARKERS; im++ )
            {

              if ( x_xmark[im]==-1 ) continue;

              odist=sqrt( pow( ((float)rect.x+rect.width/2)-x_xmark[im], 2 ) + pow( ((float)rect.y+rect.height/2)-x_ymark[im], 2 ) );

              // search for the closest known contour
              // that is likely to be this one
              if ( odist < x_mmove )
              {
                if ( odist < dist )
                {
                  found=1;
                  oi=im;
                  x_xmark[oi] = (float)(rect.x+rect.width/2);
                  x_ymark[oi] = (float)(rect.y+rect.height/2);
                  x_wmark[oi] = (int)rect.width;
                  x_hmark[oi] = (int)rect.height;
                  x_found[oi] = x_ftolerance;
                  dist=odist;
                }
              }
            }

            // new object detected
            if ( !found )
            {
               oi = this->mark(rect.x, rect.y, rect.width, rect.height );
            }

            if ( x_draw )
            {
	      cvRectangle( cnt_img, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), cvScalarAll(255), 2, 8 , 0 );
              sprintf( tindex, "%d", oi );
              cvPutText( cnt_img, tindex, cvPoint(x_xmark[oi],x_ymark[oi]), &font, cvScalarAll(255));
            }

            if ( x_show )
            {
              cvDrawContours( cnt_img, contours, CV_RGB(255,255,255), CV_RGB(255,255,255), 0, 1, 8, cvPoint(0,0) );
            }

            SETFLOAT(&rlist[0], oi);
            SETFLOAT(&rlist[1], rect.x);
            SETFLOAT(&rlist[2], rect.y);
            SETFLOAT(&rlist[3], rect.width);
            SETFLOAT(&rlist[4], rect.height);

    	    outlet_list( m_dataout, 0, 5, rlist );
	    i++;
	    ic++;
	}
    }

    outlet_float( m_countout, ic );

    // delete lost objects
    for ( im=0; im<MAX_MARKERS; im++ )
    {
       if ( x_found[im] < 0 )
       {
         x_xmark[im] = -1.0;
         x_ymark[im] = -1,0;
         x_found[im] = x_ftolerance;
         SETFLOAT(&rlist[0], im);
         SETFLOAT(&rlist[1], -1.0);
         SETFLOAT(&rlist[2], -1.0);
         SETFLOAT(&rlist[3], 0.0);
         SETFLOAT(&rlist[4], 0.0);
    	 outlet_list( m_dataout, 0, 5, rlist );
       }
    }

    cvReleaseMemStorage( &stor02 );

    //copy back the processed frame to image
    memcpy( image.data, cnt_img->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// floatThreshMess
//
/////////////////////////////////////////////////////////
void pix_opencv_contours_boundingrect :: floatMinAreaMess (float minarea)
{
  if (minarea>0) this->minarea = (int)minarea;
}

void pix_opencv_contours_boundingrect :: floatMaxAreaMess (float maxarea)
{
  if (maxarea>0) this->maxarea = (int)maxarea;
}

void pix_opencv_contours_boundingrect :: floatFToleranceMess (float ftolerance)
{
  if ((int)ftolerance>=1) x_ftolerance = (int)ftolerance;
}

void pix_opencv_contours_boundingrect :: floatMMoveMess (float mmove)
{
  if ((int)mmove>=1) x_mmove = (int)mmove;
}

void pix_opencv_contours_boundingrect :: floatCModeMess (float cmode)
{
    // CV_RETR_EXTERNAL || CV_RETR_LIST || CV_RETR_CCOMP || CV_RETR_TREE
    int mode = (int)cmode;

    if ( mode == CV_RETR_EXTERNAL )
    {
       x_cmode = CV_RETR_EXTERNAL;
       post( "pix_opencv_contours_boundingrect : mode set to CV_RETR_EXTERNAL" );
    }
    if ( mode == CV_RETR_LIST )
    {
       x_cmode = CV_RETR_LIST;
       post( "pix_opencv_contours_boundingrect : mode set to CV_RETR_LIST" );
    }
    if ( mode == CV_RETR_CCOMP )
    {
       x_cmode = CV_RETR_CCOMP;
       post( "pix_opencv_contours_boundingrect : mode set to CV_RETR_CCOMP" );
    }
    if ( mode == CV_RETR_TREE )
    {
       x_cmode = CV_RETR_TREE;
       post( "pix_opencv_contours_boundingrect : mode set to CV_RETR_TREE" );
    }
}

void pix_opencv_contours_boundingrect :: floatCMethodMess (float cmethod)
{
  int method = (int)cmethod;

    // CV_CHAIN_CODE || CV_CHAIN_APPROX_NONE || CV_CHAIN_APPROX_SIMPLE || CV_CHAIN_APPROX_TC89_L1 || CV_CHAIN_APPROX_TC89_KCOS || CV_LINK_RUNS
    if ( method == CV_CHAIN_CODE )
    {
       post( "pix_opencv_contours_boundingrect : not supported method : CV_CHAIN_CODE" );
    }
    if ( method == CV_CHAIN_APPROX_NONE )
    {
       x_cmethod = CV_CHAIN_APPROX_NONE;
       post( "pix_opencv_contours_boundingrect : method set to CV_CHAIN_APPROX_NONE" );
    }
    if ( method == CV_CHAIN_APPROX_SIMPLE )
    {
       x_cmethod = CV_CHAIN_APPROX_SIMPLE;
       post( "pix_opencv_contours_boundingrect : method set to CV_CHAIN_APPROX_SIMPLE" );
    }
    if ( method == CV_CHAIN_APPROX_TC89_L1 )
    {
       x_cmethod = CV_CHAIN_APPROX_TC89_L1;
       post( "pix_opencv_contours_boundingrect : method set to CV_CHAIN_APPROX_TC89_L1" );
    }
    if ( method == CV_CHAIN_APPROX_TC89_KCOS )
    {
       x_cmethod = CV_CHAIN_APPROX_TC89_KCOS;
       post( "pix_opencv_contours_boundingrect : method set to CV_CHAIN_APPROX_TC89_KCOS" );
    }
    if ( ( method == CV_LINK_RUNS ) && ( x_cmode == CV_RETR_LIST ) )
    {
       x_cmethod = CV_LINK_RUNS;
       post( "pix_opencv_contours_boundingrect : method set to CV_LINK_RUNS" );
    }

}

void pix_opencv_contours_boundingrect :: deleteMark(t_floatarg findex )
{
  int i;

    if ( ( findex < 0.0 ) || ( findex >= MAX_MARKERS ) )
    {
       return;
    }

    x_xmark[(int)findex] = -1;
    x_ymark[(int)findex] = -1;
    x_wmark[(int)findex] = -1;
    x_hmark[(int)findex] = -1;
}


void pix_opencv_contours_boundingrect :: floatClearMess (void)
{
  int i;

    for ( i=0; i<MAX_MARKERS; i++)
    {
      x_xmark[i] = -1;
      x_ymark[i] = -1;
      x_wmark[i] = -1;
      x_hmark[i] = -1;
      x_found[i] = x_ftolerance;
    }
}

void pix_opencv_contours_boundingrect :: floatNightmodeMess (float nightmode)
{
  if ( ((int)nightmode==1) || ((int)nightmode==0) ) x_nightmode = (int)nightmode;
}

void pix_opencv_contours_boundingrect :: floatShowMess (float show)
{
  if ( ((int)show==1) || ((int)show==0) ) x_show = (int)show;
}

void pix_opencv_contours_boundingrect :: floatDrawMess (float draw)
{
  if ( ((int)draw==1) || ((int)draw==0) ) x_draw = (int)draw;
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_contours_boundingrect :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_boundingrect::floatMinAreaMessCallback,
  		  gensym("minarea"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_boundingrect::floatMaxAreaMessCallback,
  		  gensym("maxarea"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_boundingrect::floatFToleranceMessCallback,
  		  gensym("ftolerance"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_boundingrect::floatMMoveMessCallback,
  		  gensym("maxmove"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_boundingrect::floatCModeMessCallback,
  		  gensym("mode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_boundingrect::floatCMethodMessCallback,
  		  gensym("method"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_boundingrect::floatClearMessCallback,
  		  gensym("clear"), A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_boundingrect::floatNightmodeMessCallback,
  		  gensym("nightmode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_boundingrect::floatShowMessCallback,
  		  gensym("show"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_boundingrect::floatDrawMessCallback,
  		  gensym("draw"), A_FLOAT, A_NULL);
}

void pix_opencv_contours_boundingrect :: floatMaxAreaMessCallback(void *data, t_floatarg maxarea)
{
  GetMyClass(data)->floatMaxAreaMess((float)maxarea);
}

void pix_opencv_contours_boundingrect :: floatMinAreaMessCallback(void *data, t_floatarg minarea)
{
  GetMyClass(data)->floatMinAreaMess((float)minarea);
}

void pix_opencv_contours_boundingrect :: floatFToleranceMessCallback(void *data, t_floatarg ftolerance)
{
  GetMyClass(data)->floatFToleranceMess((float)ftolerance);
}

void pix_opencv_contours_boundingrect :: floatMMoveMessCallback(void *data, t_floatarg mmove)
{
  GetMyClass(data)->floatMMoveMess((float)mmove);
}

void pix_opencv_contours_boundingrect :: floatCModeMessCallback(void *data, t_floatarg cmode)
{
  GetMyClass(data)->floatCModeMess((float)cmode);
}

void pix_opencv_contours_boundingrect :: floatCMethodMessCallback(void *data, t_floatarg cmethod)
{
  GetMyClass(data)->floatCMethodMess((float)cmethod);
}

void pix_opencv_contours_boundingrect :: floatClearMessCallback(void *data)
{
  GetMyClass(data)->floatClearMess();
}

void pix_opencv_contours_boundingrect :: floatNightmodeMessCallback(void *data, t_floatarg nightmode)
{
  GetMyClass(data)->floatNightmodeMess(nightmode);
}

void pix_opencv_contours_boundingrect :: floatShowMessCallback(void *data, t_floatarg show)
{
  GetMyClass(data)->floatShowMess(show);
}

void pix_opencv_contours_boundingrect :: floatDrawMessCallback(void *data, t_floatarg draw)
{
  GetMyClass(data)->floatDrawMess(draw);
}
