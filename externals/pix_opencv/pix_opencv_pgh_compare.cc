////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.kug.ac.at
//
// Implementation file
//
//    Copyright (c) 1997-1998 Mark Danks.
//    Copyright (c) Günther Geiger.
//    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM
//    Copyright (c) 2002 James Tittle & Chris Clepper
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "pix_opencv_pgh_compare.h"

CPPEXTERN_NEW_WITH_GIMME(pix_opencv_pgh_compare)

/////////////////////////////////////////////////////////
//
// pix_opencv_pgh_compare
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_pgh_compare :: pix_opencv_pgh_compare(int argc, t_atom*argv)
{ 
    m_dataout = outlet_new(this->x_obj, &s_anything);
    m_posout = outlet_new(this->x_obj, &s_anything);

    comp_xsize=320;
    comp_ysize=240;

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    rgbar = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgbr = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    grayr = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);

    x_storage = cvCreateMemStorage(0);

    x_bcontourr = NULL;
    x_minsize = 10*10;
    x_cdistance = 0.05;
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_pgh_compare :: ~pix_opencv_pgh_compare()
{ 
    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);
    cvReleaseImage(&rgbar);
    cvReleaseImage(&rgbr);
    cvReleaseImage(&grayr);
}

/////////////////////////////////////////////////////////
// processDualImage
//
/////////////////////////////////////////////////////////
void pix_opencv_pgh_compare :: processRGBA_RGBA(imageStruct &left, imageStruct &right)
{
 double dist = 100.0, ndist;
 int i = 0;                   // Indicator of cycles.
 CvSeq *contourl=NULL, *contourlp;
 CvRect rect;
 CvMemStorage *mstorage;
 CvSeq  *contourr = NULL;
 int    size;
 int dims[] = {8, 8};
 float range[] = {-180, 180, -100, 100};
 float *ranges[] = {&range[0], &range[2]};
 CvHistogram *histl, *histr ;

  if ((left.xsize!=right.xsize) || (left.ysize!=right.ysize) )
  {
    post( "pix_opencv_pgh_compare : left and right image are not of the same size" );
    return;
  }

  if ((this->comp_xsize!=left.xsize)&&(this->comp_ysize!=left.ysize))
  {
    this->comp_xsize=left.xsize;
    this->comp_ysize=left.ysize;

    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);
    cvReleaseImage(&rgbar);
    cvReleaseImage(&rgbr);
    cvReleaseImage(&grayr);

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    rgbar = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgbr = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    grayr = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  }

  memcpy( rgbar->imageData, right.data, right.xsize*right.ysize*4 );
  cvCvtColor(rgbar, grayr, CV_BGRA2GRAY);

  // calculate the biggest contour
  try {
      cvFindContours( grayr, x_storage, &contourr, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );
  }
  catch(...) {
      post( "pix_opencv_pgh_compare : error calculating contours" );
      return;
  }

  if ( contourr )
  {
    size=0;
    for( ; contourr != 0; contourr = contourr->h_next )
    {
       rect = cvContourBoundingRect( contourr, 1);
       if ( rect.width*rect.height > size && rect.width*rect.height < (comp_xsize-2)*(comp_ysize-2))
       {
          x_bcontourr = contourr;
          size = rect.width*rect.height;
       }
    }
  }

  memcpy( rgba->imageData, left.data, left.xsize*left.ysize*4 );
  cvCvtColor(rgba, gray, CV_BGRA2GRAY);

  mstorage = cvCreateMemStorage(0);

  cvFindContours( gray, mstorage, &contourl, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );

  i=0;
  if ( contourl && x_bcontourr )
  {
    contourlp=contourl;
    for( ; contourlp != 0; contourlp = contourlp->h_next )
    {
      rect = cvContourBoundingRect( contourlp, 1);
      if ( rect.width*rect.height > x_minsize && rect.width*rect.height < (comp_xsize-2)*(comp_ysize-2))
      {
         histr = cvCreateHist(2, dims, CV_HIST_ARRAY, ranges, 1);
         histl = cvCreateHist(2, dims, CV_HIST_ARRAY, ranges, 1);
         cvCalcPGH(x_bcontourr, histr);
         cvCalcPGH(contourlp, histl);
         cvNormalizeHist(histr, 100.0f);
         cvNormalizeHist(histl, 100.0f);
         ndist = cvCompareHist(histr, histl, CV_COMP_BHATTACHARYYA);
         cvReleaseHist(&histr);
         cvReleaseHist(&histl);

         if ( ndist < dist ) dist = ndist;
         if ( ndist < x_cdistance )
         {
           cvRectangle( gray, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(255,255,255), 2, 8 , 0 );
           cvDrawContours( gray, contourlp, CV_RGB(255,255,255), CV_RGB(255,255,255), 0, 1, 8, cvPoint(0,0) );
           SETFLOAT(&rlist[0], i++);
           SETFLOAT(&rlist[1], rect.x);
           SETFLOAT(&rlist[2], rect.y);
           SETFLOAT(&rlist[3], rect.width);
           SETFLOAT(&rlist[4], rect.height);
           outlet_list( m_posout, 0, 5, rlist );
         }
         else
         {
           cvRectangle( gray, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(128,128,128), 2, 8 , 0 );
           cvDrawContours( gray, contourlp, CV_RGB(128,128,128), CV_RGB(128,128,128), 0, 1, 8, cvPoint(0,0) );
         }
      }
    }
  }

  if ( dist < 100.00 ) outlet_float( m_dataout, dist );

  cvReleaseMemStorage(&mstorage);

  cvCvtColor(gray, rgba, CV_GRAY2BGR);
  memcpy( left.data, rgba->imageData, left.xsize*left.ysize*4 );

}

/////////////////////////////////////////////////////////
// processDualImage
//
/////////////////////////////////////////////////////////
void pix_opencv_pgh_compare :: processRGB_RGB(imageStruct &left, imageStruct &right)
{
 double dist = 100.0, ndist;
 int i = 0;                   // Indicator of cycles.
 CvSeq *contourl=NULL, *contourlp;
 CvRect rect;
 CvMemStorage *mstorage;
 CvSeq  *contourr = NULL;
 int    size;
 int dims[] = {8, 8};
 float range[] = {-180, 180, -100, 100};
 float *ranges[] = {&range[0], &range[2]};
 CvHistogram *histl, *histr ;

  if ((left.xsize!=right.xsize) || (left.ysize!=right.ysize) )
  {
    post( "pix_opencv_pgh_compare : left and right image are not of the same size" );
    return;
  }

  if ((this->comp_xsize!=left.xsize)&&(this->comp_ysize!=left.ysize))
  {
    this->comp_xsize=left.xsize;
    this->comp_ysize=left.ysize;

    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);
    cvReleaseImage(&rgbar);
    cvReleaseImage(&rgbr);
    cvReleaseImage(&grayr);

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    rgbar = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgbr = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    grayr = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  }

  memcpy( rgbr->imageData, right.data, right.xsize*right.ysize*3 );
  cvCvtColor(rgbr, grayr, CV_BGRA2GRAY);

  // calculate the biggest contour
  try {
      cvFindContours( grayr, x_storage, &contourr, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );
  }
  catch(...) {
      post( "pix_opencv_pgh_compare : error calculating contours" );
      return;
  }

  if ( contourr )
  {
    size=0;
    for( ; contourr != 0; contourr = contourr->h_next )
    {
       rect = cvContourBoundingRect( contourr, 1);
       if ( rect.width*rect.height > size && rect.width*rect.height < (comp_xsize-2)*(comp_ysize-2))
       {
          x_bcontourr = contourr;
          size = rect.width*rect.height;
       }
    }
  }

  memcpy( rgb->imageData, left.data, left.xsize*left.ysize*3 );
  cvCvtColor(rgb, gray, CV_BGRA2GRAY);

  mstorage = cvCreateMemStorage(0);

  cvFindContours( gray, mstorage, &contourl, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );

  if ( contourl && x_bcontourr )
  {
    contourlp=contourl;
    for( ; contourlp != 0; contourlp = contourlp->h_next )
    {
      rect = cvContourBoundingRect( contourlp, 1);
      if ( rect.width*rect.height > x_minsize && rect.width*rect.height < (comp_xsize-2)*(comp_ysize-2))
      {
         histr = cvCreateHist(2, dims, CV_HIST_ARRAY, ranges, 1);
         histl = cvCreateHist(2, dims, CV_HIST_ARRAY, ranges, 1);
         cvCalcPGH(x_bcontourr, histr);
         cvCalcPGH(contourlp, histl);
         cvNormalizeHist(histr, 100.0f);
         cvNormalizeHist(histl, 100.0f);
         ndist = cvCompareHist(histr, histl, CV_COMP_BHATTACHARYYA);
         cvReleaseHist(&histr);
         cvReleaseHist(&histl);

         if ( ndist < dist ) dist = ndist;
         if ( ndist < x_cdistance )
         {
           cvRectangle( gray, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(255,255,255), 2, 8 , 0 );
           cvDrawContours( gray, contourlp, CV_RGB(255,255,255), CV_RGB(255,255,255), 0, 1, 8, cvPoint(0,0) );
         }
         else
         {
           cvRectangle( gray, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(128,128,128), 2, 8 , 0 );
           cvDrawContours( gray, contourlp, CV_RGB(128,128,128), CV_RGB(128,128,128), 0, 1, 8, cvPoint(0,0) );
         }
      }
    }
  }

  if ( dist < 100.00 ) outlet_float( m_dataout, dist );

  cvReleaseMemStorage(&mstorage);

  cvCvtColor(gray, rgb, CV_GRAY2BGR);
  memcpy( left.data, rgb->imageData, left.xsize*left.ysize*3 );

}

/////////////////////////////////////////////////////////
// processDualImage
//
/////////////////////////////////////////////////////////
void pix_opencv_pgh_compare :: processGray_Gray(imageStruct &left, imageStruct &right)
{
 double dist = 100.0, ndist;
 int i = 0;                   // Indicator of cycles.
 CvSeq *contourl=NULL, *contourlp;
 CvRect rect;
 CvMemStorage *mstorage;
 CvSeq  *contourr = NULL;
 int    size;
 int dims[] = {8, 8};
 float range[] = {-180, 180, -100, 100};
 float *ranges[] = {&range[0], &range[2]};
 CvHistogram *histl, *histr ;


  if ((left.xsize!=right.xsize) || (left.ysize!=right.ysize) )
  {
    post( "pix_opencv_pgh_compare : left and right image are not of the same size" );
    return;
  }

  if ((this->comp_xsize!=left.xsize)&&(this->comp_ysize!=left.ysize))
  {
    this->comp_xsize=left.xsize;
    this->comp_ysize=left.ysize;

    cvReleaseImage(&rgba);
    cvReleaseImage(&rgb);
    cvReleaseImage(&gray);
    cvReleaseImage(&rgbar);
    cvReleaseImage(&rgbr);
    cvReleaseImage(&grayr);

    rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    gray = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
    rgbar = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
    rgbr = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
    grayr = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 1);
  }

  memcpy( grayr->imageData, right.data, right.xsize*right.ysize );

  // calculate the biggest contour
  try {
      cvFindContours( grayr, x_storage, &contourr, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );
  }
  catch(...) {
      post( "pix_opencv_pgh_compare : error calculating contours" );
      return;
  }

  if ( contourr )
  {
    size=0;
    for( ; contourr != 0; contourr = contourr->h_next )
    {
       rect = cvContourBoundingRect( contourr, 1);
       if ( rect.width*rect.height > size && rect.width*rect.height < (comp_xsize-2)*(comp_ysize-2))
       {
          x_bcontourr = contourr;
          size = rect.width*rect.height;
       }
    }
  }

  memcpy( gray->imageData, left.data, left.xsize*left.ysize );

  mstorage = cvCreateMemStorage(0);

  cvFindContours( gray, mstorage, &contourl, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );

  if ( contourl && x_bcontourr )
  {
    contourlp=contourl;
    for( ; contourlp != 0; contourlp = contourlp->h_next )
    {
      rect = cvContourBoundingRect( contourlp, 1);
      if ( rect.width*rect.height > x_minsize && rect.width*rect.height < (comp_xsize-2)*(comp_ysize-2))
      {
         histr = cvCreateHist(2, dims, CV_HIST_ARRAY, ranges, 1);
         histl = cvCreateHist(2, dims, CV_HIST_ARRAY, ranges, 1);
         cvCalcPGH(x_bcontourr, histr);
         cvCalcPGH(contourlp, histl);
         cvNormalizeHist(histr, 100.0f);
         cvNormalizeHist(histl, 100.0f);
         ndist = cvCompareHist(histr, histl, CV_COMP_BHATTACHARYYA);
         cvReleaseHist(&histr);
         cvReleaseHist(&histl);

         if ( ndist < dist ) dist = ndist;
         if ( ndist < x_cdistance )
         {
           cvRectangle( gray, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(255,255,255), 2, 8 , 0 );
           cvDrawContours( gray, contourlp, CV_RGB(255,255,255), CV_RGB(255,255,255), 0, 1, 8, cvPoint(0,0) );
         }
         else
         {
           cvRectangle( gray, cvPoint(rect.x,rect.y), cvPoint(rect.x+rect.width,rect.y+rect.height), CV_RGB(128,128,128), 2, 8 , 0 );
           cvDrawContours( gray, contourlp, CV_RGB(128,128,128), CV_RGB(128,128,128), 0, 1, 8, cvPoint(0,0) );
         }
      }
    }
  }

  if ( dist < 100.00 ) outlet_float( m_dataout, dist );

  cvReleaseMemStorage(&mstorage);

  memcpy( left.data, gray->imageData, left.xsize*left.ysize );

}

void pix_opencv_pgh_compare :: processYUV_YUV(imageStruct &left, imageStruct &right)
{
   post( "pix_opencv_pgh_compare : YUV colorspace not supported" );
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_pgh_compare :: obj_setupCallback(t_class *classPtr)
{ 
  class_addmethod(classPtr, (t_method)&pix_opencv_pgh_compare::floatMinSizeMessCallback,
		  gensym("minsize"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_pgh_compare::clearMessCallback,
		  gensym("clear"), A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_pgh_compare::floatCriteriaMessCallback,
		  gensym("criteria"), A_FLOAT, A_NULL);
}

void pix_opencv_pgh_compare :: floatMinSizeMessCallback(void *data, t_floatarg minsize)
{
  GetMyClass(data)->floatMinSizeMess((float)minsize);
}

void pix_opencv_pgh_compare :: clearMessCallback(void *data)
{
  GetMyClass(data)->clearMess();
}

void pix_opencv_pgh_compare :: floatCriteriaMessCallback(void *data, t_floatarg criteria)
{
  GetMyClass(data)->floatCriteriaMess((float)criteria);
}

void  pix_opencv_pgh_compare :: floatMinSizeMess(float minsize)
{
   if ( (int)minsize > 0 )
   {
     x_minsize = (int)minsize;
   }
}

void  pix_opencv_pgh_compare :: clearMess(void)
{
   x_bcontourr = NULL;
}

void  pix_opencv_pgh_compare :: floatCriteriaMess(float criteria)
{
   if ( criteria > 0.0 )
   {
      x_cdistance = criteria;
   }
}
