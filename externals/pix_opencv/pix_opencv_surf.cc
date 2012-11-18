
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

#include "pix_opencv_surf.h"
#include <stdio.h>


CPPEXTERN_NEW(pix_opencv_surf)

/////////////////////////////////////////////////////////
//
// pix_opencv_surf
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

pix_opencv_surf :: pix_opencv_surf()
{ 
  int i;

  comp_xsize=320;
  comp_ysize=240;

  m_dataout = outlet_new(this->x_obj, &s_anything);

  night_mode = 0;
  x_maxmove = 20;
  x_delaunay = -1;
  x_threshold = -1;

  objectKeypoints = NULL;
  objectDescriptors = NULL;
  x_hessian = 1000;
  x_ftolerance = 5;

  x_markall = 0;
  for ( i=0; i<MAX_MARKERS; i++ )
  {
     x_xmark[i] = -1;
     x_ymark[i] = -1;
  }

  // initialize font
  cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );

  rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
  orgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
  rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
  gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
  ogray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_surf :: ~pix_opencv_surf()
{
  // Destroy cv_images
  cvReleaseImage( &rgba );
  cvReleaseImage( &orgb );
  cvReleaseImage( &rgb );
  cvReleaseImage( &gray );
  cvReleaseImage( &ogray );
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_surf :: processRGBAImage(imageStruct &image)
{
  int i, k;
  int im, oi;
  int marked;
  int descsize;
  char tindex[4];
  float dist, odist;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    cvReleaseImage( &rgba );
    cvReleaseImage( &orgb );
    cvReleaseImage( &rgb );
    cvReleaseImage( &gray );
    cvReleaseImage( &ogray );

    rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    orgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    ogray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

  }

  memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
  cvCvtColor(rgba, orgb, CV_BGRA2BGR);
  cvCvtColor(rgba, rgb, CV_BGRA2BGR);
  cvCvtColor(rgba, gray, CV_BGRA2GRAY);

  x_storage = cvCreateMemStorage(0);

  if( night_mode )
      cvZero( rgb );

  for ( im=0; im<MAX_MARKERS; im++ )
  {
     x_found[im]--;
  }

  if ( x_delaunay >= 0 )
  {
     // init data structures for the delaunay
     x_fullrect.x = -comp_xsize/2;
     x_fullrect.y = -comp_ysize/2;
     x_fullrect.width = 2*comp_xsize;
     x_fullrect.height = 2*comp_ysize;

     x_subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*x_subdiv),
                             sizeof(CvSubdiv2DPoint),
                             sizeof(CvQuadEdge2D),
                             x_storage );
     cvInitSubdivDelaunay2D( x_subdiv, x_fullrect );
  }

  cvExtractSURF( gray, 0, &objectKeypoints, &objectDescriptors, x_storage, cvSURFParams(x_hessian, 1) );
  descsize = (int)(objectDescriptors->elem_size/sizeof(float));

  for( i = 0; i < objectKeypoints->total; i++ )
  {
    CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( objectKeypoints, i );
    const float* rdesc = (const float*)cvGetSeqElem( objectDescriptors, i );

     if ( x_delaunay == 0 ) // add all the points
     {
        cvSubdivDelaunay2DInsert( x_subdiv, r1->pt );
        cvCalcSubdivVoronoi2D( x_subdiv );
     }

     // only add points included in (color-threshold)<p<(color+treshold)
     if ( ( x_delaunay > 0 ) && ( x_xmark[x_delaunay-1] != -1 ) )
     {
       int px = cvPointFrom32f(r1->pt).x;
       int py = cvPointFrom32f(r1->pt).y;
       int ppx, ppy;

       // eight connected pixels
       for ( ppx=px-1; ppx<=px+1; ppx++ )
       {
         for ( ppy=py-1; ppy<=py+1; ppy++ )
         {
           if ( ( ppx < 0 ) || ( ppx >= comp_xsize ) ) continue;
           if ( ( ppy < 0 ) || ( ppy >= comp_ysize ) ) continue;

           uchar red = ((uchar*)(orgb->imageData + orgb->widthStep*ppx))[ppy*3];
           uchar green = ((uchar*)(orgb->imageData + orgb->widthStep*ppx))[ppy*3+1];
           uchar blue = ((uchar*)(orgb->imageData + orgb->widthStep*ppx))[ppy*3+2];

           uchar pred = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[x_delaunay-1]))[x_ymark[x_delaunay-1]*3];
           uchar pgreen = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[x_delaunay-1]))[x_ymark[x_delaunay-1]*3+1];
           uchar pblue = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[x_delaunay-1]))[x_ymark[x_delaunay-1]*3+2];

           int diff = abs(red-pred) + abs(green-pgreen) + abs(blue-pblue);

           // post( "pdp_opencv_surf : point (%d,%d,%d) : diff : %d", blue, green, red, diff );

           if ( diff < x_threshold )
           {
              cvSubdivDelaunay2DInsert( x_subdiv, r1->pt );
              cvCalcSubdivVoronoi2D( x_subdiv );
           }
         }
       }
     }

     cvCircle( rgb, cvPointFrom32f(r1->pt), 3, CV_RGB(0,255,0), -1, 8,0);

     // mark the point if it is not already
     if ( x_markall )
     {
       int marked = 0;

       for ( im=0; im<MAX_MARKERS; im++ )
       {
           if ( x_xmark[im] == -1 ) continue; // no points

           odist=sqrt( pow( r1->pt.x-x_xmark[im], 2 ) + pow( r1->pt.y-x_ymark[im], 2 ) );

           if ( odist <= x_maxmove )
           {
             marked = 1;
             // post( "pdp_opencv_surf : point already marked" );
             break;
           }
       }
       if ( !marked )
       {
          for ( i=0; i<MAX_MARKERS; i++)
          {
             if ( x_xmark[i] == -1 )
             {
                x_xmark[i] = r1->pt.x;
                x_ymark[i] = r1->pt.y;
                x_found[i] = x_ftolerance;
                memset( (float * )x_rdesc[i], 0x0, DSCSIZE*sizeof(float));
                break;
             }
          }
       }
     }
  }

  for ( im=0; im<MAX_MARKERS; im++ )
  {
    int neighbour = -1;
    double d, dist1 = 1000000, dist2 = 1000000;

    if ( x_xmark[im] == -1 ) continue; // no points

    oi=-1;
    dist=(comp_xsize>comp_ysize)?comp_xsize:comp_ysize;

    for( i = 0; i < objectKeypoints->total; i++ )
    {
      CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( objectKeypoints, i );
      const float* rdesc = (const float*)cvGetSeqElem( objectDescriptors, i );
      int descsize = (int)(objectDescriptors->elem_size/sizeof(float));

       // manually marked points
       // recognized on position
       odist=sqrt( pow( r1->pt.x-x_xmark[im], 2 ) + pow( r1->pt.y-x_ymark[im], 2 ) );

       if ( odist <= x_maxmove )
       {
           if ( odist < dist )
           {
             oi=im;
             x_xmark[oi]=r1->pt.x;
             x_ymark[oi]=r1->pt.y;
             memcpy( (float * )x_rdesc[oi], rdesc, descsize*sizeof(float));
             dist = odist;
           }
       }
     }

     if ( oi !=-1 )
     {
        sprintf( tindex, "%d", oi );
        cvPutText( rgb, tindex, cvPoint(x_xmark[oi],x_ymark[oi]), &font, CV_RGB(255,255,255));
        x_found[oi] = x_ftolerance;
        SETFLOAT(&x_list[0], oi);
        SETFLOAT(&x_list[1], x_xmark[oi]);
        SETFLOAT(&x_list[2], x_ymark[oi]);
        outlet_list( m_dataout, 0, 3, x_list );
     }
  }

  // draw the delaunay
  if ( x_delaunay >= 0 )
  {
     CvSeqReader  reader;
     int i, total = x_subdiv->edges->total;
     int elem_size = x_subdiv->edges->elem_size;

     cvStartReadSeq( (CvSeq*)(x_subdiv->edges), &reader, 0 );

     for( i = 0; i < total; i++ )
     {
       CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);
       CvSubdiv2DPoint* org_pt;
       CvSubdiv2DPoint* dst_pt;
       CvPoint2D32f org;
       CvPoint2D32f dst;
       CvPoint iorg, idst;

         if( CV_IS_SET_ELEM( edge ))
         {
           org_pt = cvSubdiv2DEdgeOrg((CvSubdiv2DEdge)edge);
           dst_pt = cvSubdiv2DEdgeDst((CvSubdiv2DEdge)edge);

           if( org_pt && dst_pt )
           {
               org = org_pt->pt;
               dst = dst_pt->pt;

               iorg = cvPoint( cvRound( org.x ), cvRound( org.y ));
               idst = cvPoint( cvRound( dst.x ), cvRound( dst.y ));

               if ( ( org.x > 0 ) && ( org.x < comp_xsize ) &&
                    ( dst.x > 0 ) && ( dst.x < comp_xsize ) &&
                    ( org.y > 0 ) && ( org.y < comp_ysize ) &&
                    ( dst.y > 0 ) && ( dst.y < comp_ysize ) )
               cvLine( rgb, iorg, idst, CV_RGB(255,0,0), 1, CV_AA, 0 );
           }
         }

         CV_NEXT_SEQ_ELEM( elem_size, reader );
     }
  }

  // suppress lost points
  for ( im=0; im<MAX_MARKERS; im++ )
  {
      if ( (x_xmark[im] != -1.0 ) && !x_found[im] )
      {
         x_xmark[im]=-1.0;
         x_ymark[im]=-1.0;
         SETFLOAT(&x_list[0], im+1);
         SETFLOAT(&x_list[1], x_xmark[im]);
         SETFLOAT(&x_list[2], x_ymark[im]);
         // send a lost point message to the patch
         outlet_list( m_dataout, 0, 3, x_list );
         // post( "pdp_opencv_surf : lost point %d", im+1 );
      }
  }

  cvReleaseMemStorage( &x_storage );

  cvCvtColor(rgb, rgba, CV_BGR2BGRA);
  memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_surf :: processRGBImage(imageStruct &image)
{ 
  int i, k;
  int im, oi;
  int marked;
  int descsize;
  char tindex[4];
  float dist, odist;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    cvReleaseImage( &rgba );
    cvReleaseImage( &orgb );
    cvReleaseImage( &rgb );
    cvReleaseImage( &gray );
    cvReleaseImage( &ogray );

    rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    orgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    ogray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

  }

  memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
  memcpy( orgb->imageData, image.data, image.xsize*image.ysize*3 );
  cvCvtColor(rgb, gray, CV_BGRA2GRAY);

  x_storage = cvCreateMemStorage(0);

  if( night_mode )
      cvZero( rgb );

  for ( im=0; im<MAX_MARKERS; im++ )
  {
     x_found[im]--;
  }

  if ( x_delaunay >= 0 )
  {
     // init data structures for the delaunay
     x_fullrect.x = -comp_xsize/2;
     x_fullrect.y = -comp_ysize/2;
     x_fullrect.width = 2*comp_xsize;
     x_fullrect.height = 2*comp_ysize;

     x_subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*x_subdiv),
                             sizeof(CvSubdiv2DPoint),
                             sizeof(CvQuadEdge2D),
                             x_storage );
     cvInitSubdivDelaunay2D( x_subdiv, x_fullrect );
  }

  cvExtractSURF( gray, 0, &objectKeypoints, &objectDescriptors, x_storage, cvSURFParams(x_hessian, 1) );
  descsize = (int)(objectDescriptors->elem_size/sizeof(float));

  for( i = 0; i < objectKeypoints->total; i++ )
  {
    CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( objectKeypoints, i );
    const float* rdesc = (const float*)cvGetSeqElem( objectDescriptors, i );

     if ( x_delaunay == 0 ) // add all the points
     {
        cvSubdivDelaunay2DInsert( x_subdiv, r1->pt );
        cvCalcSubdivVoronoi2D( x_subdiv );
     }

     // only add points included in (color-threshold)<p<(color+treshold)
     if ( ( x_delaunay > 0 ) && ( x_xmark[x_delaunay-1] != -1 ) )
     {
       int px = cvPointFrom32f(r1->pt).x;
       int py = cvPointFrom32f(r1->pt).y;
       int ppx, ppy;

       // eight connected pixels
       for ( ppx=px-1; ppx<=px+1; ppx++ )
       {
         for ( ppy=py-1; ppy<=py+1; ppy++ )
         {
           if ( ( ppx < 0 ) || ( ppx >= comp_xsize ) ) continue;
           if ( ( ppy < 0 ) || ( ppy >= comp_ysize ) ) continue;

           uchar red = ((uchar*)(orgb->imageData + orgb->widthStep*ppx))[ppy*3];
           uchar green = ((uchar*)(orgb->imageData + orgb->widthStep*ppx))[ppy*3+1];
           uchar blue = ((uchar*)(orgb->imageData + orgb->widthStep*ppx))[ppy*3+2];

           uchar pred = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[x_delaunay-1]))[x_ymark[x_delaunay-1]*3];
           uchar pgreen = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[x_delaunay-1]))[x_ymark[x_delaunay-1]*3+1];
           uchar pblue = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[x_delaunay-1]))[x_ymark[x_delaunay-1]*3+2];

           int diff = abs(red-pred) + abs(green-pgreen) + abs(blue-pblue);

           // post( "pdp_opencv_surf : point (%d,%d,%d) : diff : %d", blue, green, red, diff );

           if ( diff < x_threshold )
           {
              cvSubdivDelaunay2DInsert( x_subdiv, r1->pt );
              cvCalcSubdivVoronoi2D( x_subdiv );
           }
         }
       }
     }

     cvCircle( rgb, cvPointFrom32f(r1->pt), 3, CV_RGB(0,255,0), -1, 8,0);

     // mark the point if it is not already
     if ( x_markall )
     {
       int marked = 0;

       for ( im=0; im<MAX_MARKERS; im++ )
       {
         if ( x_xmark[im] == -1 ) continue; // no points

         odist=sqrt( pow( r1->pt.x-x_xmark[im], 2 ) + pow( r1->pt.y-x_ymark[im], 2 ) );

         if ( odist <= x_maxmove )
         {
             marked = 1;
             // post( "pdp_opencv_surf : point already marked" );
             break;
         }
       }
       if ( !marked )
       {
          for ( i=0; i<MAX_MARKERS; i++)
          {
             if ( x_xmark[i] == -1 )
             {
                x_xmark[i] = r1->pt.x;
                x_ymark[i] = r1->pt.y;
                x_found[i] = x_ftolerance;
                memset( (float * )x_rdesc[i], 0x0, DSCSIZE*sizeof(float));
                break;
             }
          }
       }
     }
  }

  for ( im=0; im<MAX_MARKERS; im++ )
  {
    int neighbour = -1;
    double d, dist1 = 1000000, dist2 = 1000000;

    if ( x_xmark[im] == -1 ) continue; // no points

    oi=-1;
    dist=(comp_xsize>comp_ysize)?comp_xsize:comp_ysize;

    for( i = 0; i < objectKeypoints->total; i++ )
    {
      CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( objectKeypoints, i );
      const float* rdesc = (const float*)cvGetSeqElem( objectDescriptors, i );
      int descsize = (int)(objectDescriptors->elem_size/sizeof(float));

       // manually marked points
       // recognized on position
       odist=sqrt( pow( r1->pt.x-x_xmark[im], 2 ) + pow( r1->pt.y-x_ymark[im], 2 ) );

       if ( odist <= x_maxmove )
       {
           if ( odist < dist )
           {
             oi=im;
             x_xmark[oi]=r1->pt.x;
             x_ymark[oi]=r1->pt.y;
             memcpy( (float * )x_rdesc[oi], rdesc, descsize*sizeof(float));
             dist = odist;
           }
       }
     }

     if ( oi !=-1 )
     {
        sprintf( tindex, "%d", oi );
        cvPutText( rgb, tindex, cvPoint(x_xmark[oi],x_ymark[oi]), &font, CV_RGB(255,255,255));
        x_found[oi] = x_ftolerance;
        SETFLOAT(&x_list[0], oi);
        SETFLOAT(&x_list[1], x_xmark[oi]);
        SETFLOAT(&x_list[2], x_ymark[oi]);
        outlet_list( m_dataout, 0, 3, x_list );
     }
  }

  // draw the delaunay
  if ( x_delaunay >= 0 )
  {
     CvSeqReader  reader;
     int i, total = x_subdiv->edges->total;
     int elem_size = x_subdiv->edges->elem_size;

     cvStartReadSeq( (CvSeq*)(x_subdiv->edges), &reader, 0 );

     for( i = 0; i < total; i++ )
     {
       CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);
       CvSubdiv2DPoint* org_pt;
       CvSubdiv2DPoint* dst_pt;
       CvPoint2D32f org;
       CvPoint2D32f dst;
       CvPoint iorg, idst;

         if( CV_IS_SET_ELEM( edge ))
         {
           org_pt = cvSubdiv2DEdgeOrg((CvSubdiv2DEdge)edge);
           dst_pt = cvSubdiv2DEdgeDst((CvSubdiv2DEdge)edge);

           if( org_pt && dst_pt )
           {
               org = org_pt->pt;
               dst = dst_pt->pt;

               iorg = cvPoint( cvRound( org.x ), cvRound( org.y ));
               idst = cvPoint( cvRound( dst.x ), cvRound( dst.y ));

               if ( ( org.x > 0 ) && ( org.x < comp_xsize ) &&
                    ( dst.x > 0 ) && ( dst.x < comp_xsize ) &&
                    ( org.y > 0 ) && ( org.y < comp_ysize ) &&
                    ( dst.y > 0 ) && ( dst.y < comp_ysize ) )
               cvLine( rgb, iorg, idst, CV_RGB(255,0,0), 1, CV_AA, 0 );
           }
         }

         CV_NEXT_SEQ_ELEM( elem_size, reader );
     }
  }

  // suppress lost points
  for ( im=0; im<MAX_MARKERS; im++ )
  {
      if ( (x_xmark[im] != -1.0 ) && !x_found[im] )
      {
         x_xmark[im]=-1.0;
         x_ymark[im]=-1.0;
         SETFLOAT(&x_list[0], im+1);
         SETFLOAT(&x_list[1], x_xmark[im]);
         SETFLOAT(&x_list[2], x_ymark[im]);
         // send a lost point message to the patch
         outlet_list( m_dataout, 0, 3, x_list );
         // post( "pdp_opencv_surf : lost point %d", im+1 );
      }
  }

  cvReleaseMemStorage( &x_storage );
  memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_surf :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_surf : yuv format not supported" );
}
    	
void pix_opencv_surf :: processGrayImage(imageStruct &image)
{ 
  int i, k;
  int im, oi;
  int marked;
  int descsize;
  char tindex[4];
  float dist, odist;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    cvReleaseImage( &rgba );
    cvReleaseImage( &orgb );
    cvReleaseImage( &rgb );
    cvReleaseImage( &gray );
    cvReleaseImage( &ogray );

    rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    orgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    ogray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

  }

  memcpy( gray->imageData, image.data, image.xsize*image.ysize );
  memcpy( ogray->imageData, image.data, image.xsize*image.ysize );
  x_storage = cvCreateMemStorage(0);

  if( night_mode )
      cvZero( gray );

  for ( im=0; im<MAX_MARKERS; im++ )
  {
     x_found[im]--;
  }

  if ( x_delaunay >= 0 )
  {
     // init data structures for the delaunay
     x_fullrect.x = -comp_xsize/2;
     x_fullrect.y = -comp_ysize/2;
     x_fullrect.width = 2*comp_xsize;
     x_fullrect.height = 2*comp_ysize;

     x_subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*x_subdiv),
                             sizeof(CvSubdiv2DPoint),
                             sizeof(CvQuadEdge2D),
                             x_storage );
     cvInitSubdivDelaunay2D( x_subdiv, x_fullrect );
  }

  cvExtractSURF( ogray, 0, &objectKeypoints, &objectDescriptors, x_storage, cvSURFParams(x_hessian, 1) );
  descsize = (int)(objectDescriptors->elem_size/sizeof(float));

  for( i = 0; i < objectKeypoints->total; i++ )
  {
    CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( objectKeypoints, i );
    const float* rdesc = (const float*)cvGetSeqElem( objectDescriptors, i );

     if ( x_delaunay == 0 ) // add all the points
     {
        cvSubdivDelaunay2DInsert( x_subdiv, r1->pt );
        cvCalcSubdivVoronoi2D( x_subdiv );
     }

     // only add points included in (color-threshold)<p<(color+treshold)
     if ( ( x_delaunay > 0 ) && ( x_xmark[x_delaunay-1] != -1 ) )
     {
       int px = cvPointFrom32f(r1->pt).x;
       int py = cvPointFrom32f(r1->pt).y;
       int ppx, ppy;

       // eight connected pixels
       for ( ppx=px-1; ppx<=px+1; ppx++ )
       {
         for ( ppy=py-1; ppy<=py+1; ppy++ )
         {
           if ( ( ppx < 0 ) || ( ppx >= comp_xsize ) ) continue;
           if ( ( ppy < 0 ) || ( ppy >= comp_ysize ) ) continue;

           uchar lum = ((uchar*)(ogray->imageData + ogray->widthStep*ppx))[ppy];

           uchar plum = ((uchar*)(ogray->imageData + ogray->widthStep*x_xmark[x_delaunay-1]))[x_ymark[x_delaunay-1]];

           int diff = abs(lum-plum);

           if ( diff < x_threshold )
           {
              cvSubdivDelaunay2DInsert( x_subdiv, r1->pt );
              cvCalcSubdivVoronoi2D( x_subdiv );
           }
         }
       }
     }

     cvCircle( gray, cvPointFrom32f(r1->pt), 3, CV_RGB(255,255,255), -1, 8,0);

     // mark the point if it is not already
     if ( x_markall )
     {
       int marked = 0;

       for ( im=0; im<MAX_MARKERS; im++ )
       {
         if ( x_xmark[im] == -1 ) continue; // no points

         odist=sqrt( pow( r1->pt.x-x_xmark[im], 2 ) + pow( r1->pt.y-x_ymark[im], 2 ) );

         if ( odist <= x_maxmove )
         {
           marked = 1;
           // post( "pdp_opencv_surf : point already marked" );
           break;
         }
       }

       if ( !marked )
       {
          for ( i=0; i<MAX_MARKERS; i++)
          {
             if ( x_xmark[i] == -1 )
             {
                x_xmark[i] = r1->pt.x;
                x_ymark[i] = r1->pt.y;
                x_found[i] = x_ftolerance;
                memset( (float * )x_rdesc[i], 0x0, DSCSIZE*sizeof(float));
                break;
             }
          }
       }
     }
  }

  for ( im=0; im<MAX_MARKERS; im++ )
  {
    int neighbour = -1;
    double d, dist1 = 1000000, dist2 = 1000000;

    if ( x_xmark[im] == -1 ) continue; // no points

    oi=-1;
    dist=(comp_xsize>comp_ysize)?comp_xsize:comp_ysize;

    for( i = 0; i < objectKeypoints->total; i++ )
    {
      CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( objectKeypoints, i );
      const float* rdesc = (const float*)cvGetSeqElem( objectDescriptors, i );
      int descsize = (int)(objectDescriptors->elem_size/sizeof(float));

       // manually marked points
       // recognized on position
       odist=sqrt( pow( r1->pt.x-x_xmark[im], 2 ) + pow( r1->pt.y-x_ymark[im], 2 ) );

       if ( odist <= x_maxmove )
       {
           if ( odist < dist )
           {
             oi=im;
             x_xmark[oi]=r1->pt.x;
             x_ymark[oi]=r1->pt.y;
             memcpy( (float * )x_rdesc[oi], rdesc, descsize*sizeof(float));
             dist = odist;
           }
       }
     }

     if ( oi !=-1 )
     {
        sprintf( tindex, "%d", oi );
        cvPutText( gray, tindex, cvPoint(x_xmark[oi],x_ymark[oi]), &font, CV_RGB(255,255,255));
        x_found[oi] = x_ftolerance;
        SETFLOAT(&x_list[0], oi);
        SETFLOAT(&x_list[1], x_xmark[oi]);
        SETFLOAT(&x_list[2], x_ymark[oi]);
        outlet_list( m_dataout, 0, 3, x_list );
     }
  }

  // draw the delaunay
  if ( x_delaunay >= 0 )
  {
     CvSeqReader  reader;
     int i, total = x_subdiv->edges->total;
     int elem_size = x_subdiv->edges->elem_size;

     cvStartReadSeq( (CvSeq*)(x_subdiv->edges), &reader, 0 );

     for( i = 0; i < total; i++ )
     {
       CvQuadEdge2D* edge = (CvQuadEdge2D*)(reader.ptr);
       CvSubdiv2DPoint* org_pt;
       CvSubdiv2DPoint* dst_pt;
       CvPoint2D32f org;
       CvPoint2D32f dst;
       CvPoint iorg, idst;

         if( CV_IS_SET_ELEM( edge ))
         {
           org_pt = cvSubdiv2DEdgeOrg((CvSubdiv2DEdge)edge);
           dst_pt = cvSubdiv2DEdgeDst((CvSubdiv2DEdge)edge);

           if( org_pt && dst_pt )
           {
               org = org_pt->pt;
               dst = dst_pt->pt;

               iorg = cvPoint( cvRound( org.x ), cvRound( org.y ));
               idst = cvPoint( cvRound( dst.x ), cvRound( dst.y ));

               if ( ( org.x > 0 ) && ( org.x < comp_xsize ) &&
                    ( dst.x > 0 ) && ( dst.x < comp_xsize ) &&
                    ( org.y > 0 ) && ( org.y < comp_ysize ) &&
                    ( dst.y > 0 ) && ( dst.y < comp_ysize ) )
               cvLine( gray, iorg, idst, CV_RGB(255,255,255), 1, CV_AA, 0 );
           }
         }

         CV_NEXT_SEQ_ELEM( elem_size, reader );
     }
  }

  // suppress lost points
  for ( im=0; im<MAX_MARKERS; im++ )
  {
      if ( (x_xmark[im] != -1.0 ) && !x_found[im] )
      {
         x_xmark[im]=-1.0;
         x_ymark[im]=-1.0;
         SETFLOAT(&x_list[0], im+1);
         SETFLOAT(&x_list[1], x_xmark[im]);
         SETFLOAT(&x_list[2], x_ymark[im]);
         // send a lost point message to the patch
         outlet_list( m_dataout, 0, 3, x_list );
         // post( "pdp_opencv_surf : lost point %d", im+1 );
      }
  }

  cvReleaseMemStorage( &x_storage );
  memcpy( image.data, gray->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////

void pix_opencv_surf :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_surf::nightModeMessCallback,
		  gensym("nightmode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_surf::hessianMessCallback,
		  gensym("hessian"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_surf::markMessCallback,
		  gensym("mark"), A_GIMME, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_surf::deleteMessCallback,
		  gensym("delete"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_surf::clearMessCallback,
		  gensym("clear"), A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_surf::maxMoveMessCallback,
		  gensym("maxmove"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_surf::ftoleranceMessCallback,
		  gensym("ftolerance"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_surf::delaunayMessCallback,
		  gensym("delaunay"), A_SYMBOL, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_surf::pdelaunayMessCallback,
		  gensym("pdelaunay"), A_FLOAT, A_FLOAT, A_NULL);
}

void  pix_opencv_surf :: nightModeMessCallback(void *data, t_floatarg nightmode)
{
    GetMyClass(data)->nightModeMess((float)nightmode);
}

void  pix_opencv_surf :: hessianMessCallback(void *data, t_floatarg hessian)
{
    GetMyClass(data)->hessianMess((float)hessian);
}

void  pix_opencv_surf :: markMessCallback(void *data, t_symbol *s, int argc, t_atom *argv)
{
    GetMyClass(data)->markMess(argc, argv);
}

void  pix_opencv_surf :: deleteMessCallback(void *data, t_floatarg index)
{
    GetMyClass(data)->deleteMess((float)index);
}

void  pix_opencv_surf :: clearMessCallback(void *data)
{
    GetMyClass(data)->clearMess();
}

void  pix_opencv_surf :: maxMoveMessCallback(void *data, t_floatarg maxmove)
{
    GetMyClass(data)->maxMoveMess((float)maxmove);
}

void  pix_opencv_surf :: ftoleranceMessCallback(void *data, t_floatarg ftolerance)
{
    GetMyClass(data)->ftoleranceMess((float)ftolerance);
}

void  pix_opencv_surf :: delaunayMessCallback(void *data, t_symbol *s)
{
    GetMyClass(data)->delaunayMess(s);
}

void  pix_opencv_surf :: pdelaunayMessCallback(void *data, t_floatarg fpoint, t_floatarg fthreshold)
{
    GetMyClass(data)->pdelaunayMess((float)fpoint, (float)fthreshold);
}

void  pix_opencv_surf :: nightModeMess(float nightmode)
{
    if ((nightmode==0.0)||(nightmode==1.0)) night_mode = (int)nightmode;
}

void  pix_opencv_surf :: hessianMess(float hessian)
{
    if (hessian>0.0) x_hessian = (int)hessian;
}

void  pix_opencv_surf :: markMess(int argc, t_atom *argv)
{
  int i;
  int inserted;

    if ( argc == 1 ) // mark all or none
    {
      if ( argv[0].a_type != A_SYMBOL )
      {
        error( "pix_opencv_surf : wrong argument (should be 'all')" );
        return;
      }
      if ( !strcmp( argv[0].a_w.w_symbol->s_name, "all" ) )
      {
        x_markall = 1;
        return;
      }
      if ( !strcmp( argv[0].a_w.w_symbol->s_name, "none" ) )
      {
        x_markall = 0;
        clearMess();
        return;
      }
    }
    else
    {
      if ( ( argv[0].a_type != A_FLOAT ) || ( argv[1].a_type != A_FLOAT ) )
      {
        error( "pix_opencv_surf : wrong argument (should be mark px py)" );
        return;
      }
      else
      {
        float fpx = argv[0].a_w.w_float;
        float fpy = argv[1].a_w.w_float;
        int px, py;

        if ( ( fpx < 0.0 ) || ( fpx > comp_xsize ) || ( fpy < 0.0 ) || ( fpy > comp_ysize ) )
        {
           return;
        }

        px = (int)fpx;
        py = (int)fpy;
        inserted = 0;
        for ( i=0; i<MAX_MARKERS; i++)
        {
           if ( x_xmark[i] == -1 )
           {
              x_xmark[i] = px;
              x_ymark[i] = py;
              x_found[i] = x_ftolerance;
              inserted = 1;
              // post( "pix_opencv_surf : inserted point (%d,%d)", px, py );
              break;
           }
        }
        if ( !inserted )
        {
           post( "pix_opencv_surf : max markers reached" );
        }
      }
   }
}

void  pix_opencv_surf :: deleteMess(float index)
{
  int i;

    if ( ( index < 1.0 ) || ( index > MAX_MARKERS ) )
    {
       return;
    }

    x_xmark[(int)index-1] = -1;
    x_ymark[(int)index-1] = -1;

}

void  pix_opencv_surf :: clearMess(void)
{
  int i;

    for ( i=0; i<MAX_MARKERS; i++)
    {
      x_xmark[i] = -1;
      x_ymark[i] = -1;
    }

}

void  pix_opencv_surf :: maxMoveMess(float maxmove)
{
  // has to be more than the size of a point
  if (maxmove>=3.0) maxmove = (int)maxmove;
}

void  pix_opencv_surf :: ftoleranceMess(float ftolerance)
{
  if (ftolerance>=0.0) ftolerance = (int)ftolerance;
}

void  pix_opencv_surf :: delaunayMess(t_symbol *s)
{
  if (s == gensym("on"))
     x_delaunay = 0;
  if (s == gensym("off"))
     x_delaunay = -1;
}

void  pix_opencv_surf :: pdelaunayMess(float point, float threshold)
{
  if (((int)point>0) && ((int)point<MAX_MARKERS))
  {
     x_delaunay = (int)point;
     x_threshold = (int)threshold;
     // post( "pix_opencv_surf : setting threshold to : %d", x_threshold );
  }
}

