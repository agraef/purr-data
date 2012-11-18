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

#include "pix_opencv_hist_compare.h"

CPPEXTERN_NEW(pix_opencv_hist_compare)

/////////////////////////////////////////////////////////
//
// pix_opencv_hist_compare
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

pix_opencv_hist_compare :: pix_opencv_hist_compare()
{ 
  int i;

  comp_xsize=320;
  comp_ysize=240;

  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("save"));
  m_dataout = outlet_new(this->x_obj, &s_anything);
  m_measureout = outlet_new(this->x_obj, &s_anything);

  save_now = 0;

  rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
  rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
  grey = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  hsv = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3 );

  h_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  s_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  v_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  planes[0] = h_plane;
  planes[1] = s_plane;
  h_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  s_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  v_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  saved_planes[0] = h_saved_plane;
  saved_planes[1] = s_saved_plane;

  int h_bins = (int)(comp_xsize/10), s_bins = (int)(comp_ysize/10);
  {
     int    hist_size[]  = { h_bins, s_bins };
     float  h_ranges[]   = { 0, 180 };         // hue is [0,180]
     float  s_ranges[]   = { 0, 255 };
     float* ranges[]     = { h_ranges, s_ranges };
     hist = cvCreateHist(
            2,
            hist_size,
            CV_HIST_ARRAY,
            ranges,
            1
      );
      int n;
      for (n=0; n<MAX_HISTOGRAMS_TO_COMPARE; n++) {
          saved_hist[n] = cvCreateHist(
                  2,
                  hist_size,
                  CV_HIST_ARRAY,
                  ranges,
                  1
          );
      }
  }
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_hist_compare :: ~pix_opencv_hist_compare()
{
    //Destroy cv_images to clean memory
    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&grey);
    cvReleaseImage(&h_plane);
    cvReleaseImage(&s_plane);
    cvReleaseImage(&v_plane);
    cvReleaseImage(&h_saved_plane);
    cvReleaseImage(&s_saved_plane);
    cvReleaseImage(&v_saved_plane);

}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_hist_compare :: processRGBAImage(imageStruct &image)
{
  int i;
  int h_bins = (int)(comp_xsize/10), s_bins = (int)(comp_ysize/10);

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    post( "pix_opencv_hist_compare : reallocating buffers" );

    //Destroy cv_images to clean memory
    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&grey);
    cvReleaseImage(&h_plane);
    cvReleaseImage(&s_plane);
    cvReleaseImage(&v_plane);
    cvReleaseImage(&h_saved_plane);
    cvReleaseImage(&s_saved_plane);
    cvReleaseImage(&v_saved_plane);

    //Create cv_images 
    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    grey = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    hsv = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3 );
      
    h_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    s_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    v_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    planes[0] = h_plane;
    planes[1] = s_plane;
    cvCvtPixToPlane( hsv, h_plane, s_plane, v_plane, 0 );
    h_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    s_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    v_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    saved_planes[0] = h_saved_plane;
    saved_planes[1] = s_saved_plane;
      
    h_bins = (int)(comp_xsize/10);
    s_bins = (int)(comp_ysize/10);
    {
      int    hist_size[]  = { h_bins, s_bins };
      float  h_ranges[]   = { 0, 180 };         // hue is [0,180]
      float  s_ranges[]   = { 0, 255 };
      float* ranges[]     = { h_ranges, s_ranges };
      hist = cvCreateHist(
                  2,
                  hist_size,
                  CV_HIST_ARRAY,
                  ranges,
                  1
      );
      int n;
      for (n=0; n<MAX_HISTOGRAMS_TO_COMPARE; n++) {
          saved_hist[n] = cvCreateHist(
                    2,
                    hist_size,
                    CV_HIST_ARRAY,
                    ranges,
                    1
          );
      }
    }
      
  }

  memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );

  // Convert to hsv
  cvCvtColor( rgba, rgb, CV_BGRA2BGR );
  cvCvtColor( rgb, hsv, CV_BGR2HSV );
  cvCvtPixToPlane( hsv, h_plane, s_plane, v_plane, 0 );
  
  // Build the histogram and compute its contents.
  if (save_now>=0) {
       post("saving histogram %d\n",save_now);
       cvCvtPixToPlane( hsv, h_saved_plane, s_saved_plane, v_saved_plane, 0 );
       cvCalcHist( saved_planes, saved_hist[save_now], 0, 0 ); //Compute histogram
       cvNormalizeHist( saved_hist[save_now], 1.0 );  //Normalize it
       save_now=-1;
       nbsaved++;
  }
  cvCalcHist( planes, hist, 0, 0 ); //Compute histogram
  cvNormalizeHist( hist, 1.0 );  //Normalize it

  double tato[nbsaved];
  t_atom datalist[nbsaved];
  int nearest = -1;
  double max  =  0;

  int n;
  if ( nbsaved > 0 )
    for (n=0; n<nbsaved; n++) {
        tato[n] = cvCompareHist(hist, saved_hist[n], CV_COMP_INTERSECT);
        SETFLOAT(&datalist[n], tato[n]);
        if (tato[n]>max) {
                max = tato[n];
                nearest = n;
    }
  }

  if ( nbsaved > 0 ) {
       outlet_float(m_dataout, (float)nearest);
       outlet_list( m_measureout, 0, nbsaved , datalist );
  } else
       outlet_float(m_dataout, -1.0);

  // Create an image to use to visualize our histogram.
  int scale = 10;
  // populate our visualization with little gray squares.
  float max_value = 0;
  cvGetMinMaxHistValue( hist, 0, &max_value, 0, 0 );

  int h = 0;
  int s = 0;

  for( h = 0; h < h_bins; h++ ) {
     for( s = 0; s < s_bins; s++ ) {
        float bin_val = cvQueryHistValue_2D( hist, h, s );
        int intensity = cvRound( bin_val * 255 / max_value );
        cvRectangle(
               rgba,
               cvPoint( h*scale, s*scale ),
               cvPoint( (h+1)*scale - 1, (s+1)*scale - 1),
               CV_RGB(intensity,intensity,intensity), CV_FILLED, 8 , 0 );
        }
  }

  memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );

}

void pix_opencv_hist_compare :: processRGBImage(imageStruct &image)
{
  int i;
  int h_bins = (int)(comp_xsize/10), s_bins = (int)(comp_ysize/10);

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    //Destroy cv_images to clean memory
    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&grey);
    cvReleaseImage(&h_plane);
    cvReleaseImage(&s_plane);
    cvReleaseImage(&v_plane);
    cvReleaseImage(&h_saved_plane);
    cvReleaseImage(&s_saved_plane);
    cvReleaseImage(&v_saved_plane);

    //Create cv_images 
    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    grey = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    hsv = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3 );
      
    h_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    s_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    v_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    planes[0] = h_plane;
    planes[1] = s_plane;
    cvCvtPixToPlane( hsv, h_plane, s_plane, v_plane, 0 );
    h_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    s_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    v_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    saved_planes[0] = h_saved_plane;
    saved_planes[1] = s_saved_plane;
      
    h_bins = (int)(comp_xsize/10);
    s_bins = (int)(comp_ysize/10);
    {
      int    hist_size[]  = { h_bins, s_bins };
      float  h_ranges[]   = { 0, 180 };         // hue is [0,180]
      float  s_ranges[]   = { 0, 255 };
      float* ranges[]     = { h_ranges, s_ranges };
      hist = cvCreateHist(
                  2,
                  hist_size,
                  CV_HIST_ARRAY,
                  ranges,
                  1
      );
      int n;
      for (n=0; n<MAX_HISTOGRAMS_TO_COMPARE; n++) {
          saved_hist[n] = cvCreateHist(
                    2,
                    hist_size,
                    CV_HIST_ARRAY,
                    ranges,
                    1
          );
      }
    }
      
  }

  memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );

  // Convert to hsv
  cvCvtColor( rgb, hsv, CV_BGR2HSV );
  cvCvtPixToPlane( hsv, h_plane, s_plane, v_plane, 0 );
  
  // Build the histogram and compute its contents.
  if (save_now>=0) {
       post("saving histogram %d\n",save_now);
       cvCvtPixToPlane( hsv, h_saved_plane, s_saved_plane, v_saved_plane, 0 );
       cvCalcHist( saved_planes, saved_hist[save_now], 0, 0 ); //Compute histogram
       cvNormalizeHist( saved_hist[save_now], 1.0 );  //Normalize it
       save_now=-1;
       nbsaved++;
   }
   cvCalcHist( planes, hist, 0, 0 ); //Compute histogram
   cvNormalizeHist( hist, 1.0 );  //Normalize it

   double tato[nbsaved];
   t_atom datalist[nbsaved];
   int nearest = -1;
   double max  =  0;

   int n;
   if ( nbsaved > 0 )
      for (n=0; n<nbsaved; n++) {
        tato[n] = cvCompareHist(hist, saved_hist[n], CV_COMP_INTERSECT);
        SETFLOAT(&datalist[n], tato[n]);
        if (tato[n]>max) {
                max = tato[n];
                nearest = n;
      }
   }

  if ( nbsaved > 0 ) {
       outlet_float(m_dataout, (float)nearest);
       outlet_list( m_measureout, 0, nbsaved , datalist );
  } else
       outlet_float(m_dataout, -1.0);

   // Create an image to use to visualize our histogram.
   int scale = 10;
   // populate our visualization with little gray squares.
   float max_value = 0;
   cvGetMinMaxHistValue( hist, 0, &max_value, 0, 0 );

   int h = 0;
   int s = 0;

   for( h = 0; h < h_bins; h++ ) {
       for( s = 0; s < s_bins; s++ ) {
          float bin_val = cvQueryHistValue_2D( hist, h, s );
          int intensity = cvRound( bin_val * 255 / max_value );
          cvRectangle(
                rgb,
                cvPoint( h*scale, s*scale ),
                cvPoint( (h+1)*scale - 1, (s+1)*scale - 1),
                CV_RGB(intensity,intensity,intensity), CV_FILLED, 8 , 0 );
        }
   }

   memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );

}

void pix_opencv_hist_compare :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_hist_compare : yuv format not supported" );
}
    	
void pix_opencv_hist_compare :: processGrayImage(imageStruct &image)
{ 
  int i;
  int h_bins = (int)(comp_xsize/10), s_bins = (int)(comp_ysize/10);

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    //Destroy cv_images to clean memory
    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&grey);
    cvReleaseImage(&h_plane);
    cvReleaseImage(&s_plane);
    cvReleaseImage(&v_plane);
    cvReleaseImage(&h_saved_plane);
    cvReleaseImage(&s_saved_plane);
    cvReleaseImage(&v_saved_plane);

    //Create cv_images 
    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    grey = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    hsv = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3 );
      
    h_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    s_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    v_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    planes[0] = h_plane;
    planes[1] = s_plane;
    cvCvtPixToPlane( hsv, h_plane, s_plane, v_plane, 0 );
    h_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    s_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    v_saved_plane = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    saved_planes[0] = h_saved_plane;
    saved_planes[1] = s_saved_plane;
      
    h_bins = (int)(comp_xsize/10);
    s_bins = (int)(comp_ysize/10);
    {
      int    hist_size[]  = { h_bins, s_bins };
      float  h_ranges[]   = { 0, 180 };         // hue is [0,180]
      float  s_ranges[]   = { 0, 255 };
      float* ranges[]     = { h_ranges, s_ranges };
      hist = cvCreateHist(
                  2,
                  hist_size,
                  CV_HIST_ARRAY,
                  ranges,
                  1
      );
      int n;
      for (n=0; n<MAX_HISTOGRAMS_TO_COMPARE; n++) {
          saved_hist[n] = cvCreateHist(
                    2,
                    hist_size,
                    CV_HIST_ARRAY,
                    ranges,
                    1
          );
      }
    }
      
  }

  memcpy( grey->imageData, image.data, image.xsize*image.ysize );

  // Convert to hsv
  cvCvtColor( grey, rgb, CV_GRAY2BGR );
  cvCvtColor( rgb, hsv, CV_BGR2HSV );
  cvCvtPixToPlane( hsv, h_plane, s_plane, v_plane, 0 );
  
  // Build the histogram and compute its contents.
  if (save_now>=0) {
       post("saving histogram %d\n",save_now);
       cvCvtPixToPlane( hsv, h_saved_plane, s_saved_plane, v_saved_plane, 0 );
       cvCalcHist( saved_planes, saved_hist[save_now], 0, 0 ); //Compute histogram
       cvNormalizeHist( saved_hist[save_now], 1.0 );  //Normalize it
       save_now=-1;
       nbsaved++;
   }
   cvCalcHist( planes, hist, 0, 0 ); //Compute histogram
   cvNormalizeHist( hist, 1.0 );  //Normalize it

   double tato[nbsaved];
   t_atom datalist[nbsaved];
   int nearest = -1;
   double max  =  0;

   int n;
   if ( nbsaved > 0 )
      for (n=0; n<nbsaved; n++) {
        tato[n] = cvCompareHist(hist, saved_hist[n], CV_COMP_INTERSECT);
        SETFLOAT(&datalist[n], tato[n]);
        if (tato[n]>max) {
                max = tato[n];
                nearest = n;
      }
   }

   if ( nbsaved > 0 ) {
      outlet_float(m_dataout, (float)nearest);
      outlet_list( m_measureout, 0, nbsaved , datalist );
   } else
      outlet_float(m_dataout, -1.0);

   // Create an image to use to visualize our histogram.
   int scale = 10;
   // populate our visualization with little gray squares.
   float max_value = 0;
   cvGetMinMaxHistValue( hist, 0, &max_value, 0, 0 );

   int h = 0;
   int s = 0;

   for( h = 0; h < h_bins; h++ ) {
      for( s = 0; s < s_bins; s++ ) {
          float bin_val = cvQueryHistValue_2D( hist, h, s );
          int intensity = cvRound( bin_val * 255 / max_value );
          cvRectangle(
                grey,
                cvPoint( h*scale, s*scale ),
                cvPoint( (h+1)*scale - 1, (s+1)*scale - 1),
                CV_RGB(intensity,intensity,intensity), CV_FILLED, 8 , 0 );
        }
   }

   memcpy( image.data, grey->imageData, image.xsize*image.ysize );

}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////

void pix_opencv_hist_compare :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_hist_compare::saveMessCallback,
                  gensym("save"), A_FLOAT, A_NULL);
}

void pix_opencv_hist_compare :: saveMess(float index)
{
    if (((int)index>=0)&&((int)index<MAX_HISTOGRAMS_TO_COMPARE)) save_now = (int)index;
}

void pix_opencv_hist_compare :: saveMessCallback(void *data, t_floatarg index)
{
  GetMyClass(data)->saveMess(index);
}
