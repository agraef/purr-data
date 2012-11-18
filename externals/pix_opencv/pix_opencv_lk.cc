
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

#include "pix_opencv_lk.h"
#include <stdio.h>

CPPEXTERN_NEW(pix_opencv_lk)

/////////////////////////////////////////////////////////
//
// pix_opencv_lk
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

pix_opencv_lk :: pix_opencv_lk()
{ 
  int i;

  comp_xsize=320;
  comp_ysize=240;

  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("winsize"));

  m_dataout = outlet_new(this->x_obj, &s_anything);
  win_size = 10;

  points[0] = 0;
  points[1] = 0;
  status = 0;
  count = 0;
  need_to_init = 1;
  night_mode = 0;
  flags = 0;
  add_remove_pt = 0;
  quality = 0.1;
  min_distance = 10;
  maxmove = 20;
  markall = 0;
  ftolerance = 5;
  delaunay = -1;
  threshold = -1;

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
  prev_gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
  pyramid = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
  prev_pyramid = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
  points[0] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
  points[1] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
  status = (char*)cvAlloc(MAX_COUNT);

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_lk :: ~pix_opencv_lk()
{
  // Destroy cv_images
  cvReleaseImage( &rgba );
  cvReleaseImage( &orgb );
  cvReleaseImage( &rgb );
  cvReleaseImage( &gray );
  cvReleaseImage( &ogray );
  cvReleaseImage( &prev_gray );
  cvReleaseImage( &pyramid );
  cvReleaseImage( &prev_pyramid );
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_lk :: processRGBAImage(imageStruct &image)
{
  int i, k;
  int im, oi;
  int marked;
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
    cvReleaseImage( &prev_gray );
    cvReleaseImage( &pyramid );
    cvReleaseImage( &prev_pyramid );

    rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    orgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    ogray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    prev_gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    pyramid = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    prev_pyramid = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    points[0] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
    points[1] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
    status = (char*)cvAlloc(MAX_COUNT);

  }

  memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
  cvCvtColor(rgba, rgb, CV_BGRA2RGB);
  cvCvtColor(rgba, orgb, CV_BGRA2RGB);
  cvCvtColor(rgba, gray, CV_BGRA2GRAY);

  if( night_mode )
      cvZero( rgb );

  for ( im=0; im<MAX_MARKERS; im++ )
  {
       x_found[im]--;
  }

  if ( delaunay >= 0 )
  {
    // init data structures for the delaunay
    x_fullrect.x = -comp_xsize/2;
    x_fullrect.y = -comp_ysize/2;
    x_fullrect.width = 2*comp_xsize;
    x_fullrect.height = 2*comp_ysize;

    x_storage = cvCreateMemStorage(0);
    x_subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*x_subdiv),
                               sizeof(CvSubdiv2DPoint),
                               sizeof(CvQuadEdge2D),
                               x_storage );
     cvInitSubdivDelaunay2D( x_subdiv, x_fullrect );
  }

  if( need_to_init )
  {
     /* automatic initialization */
     IplImage* eig = cvCreateImage( cvSize(gray->width,gray->height), 32, 1 );
     IplImage* temp = cvCreateImage( cvSize(gray->width,gray->height), 32, 1 );

     count = MAX_COUNT;
     cvGoodFeaturesToTrack( gray, eig, temp, points[1], &count,
                               quality, min_distance, 0, 3, 0, 0.04 );
     cvFindCornerSubPix( gray, points[1], count,
          cvSize(win_size,win_size), cvSize(-1,-1),
          cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
     cvReleaseImage( &eig );
     cvReleaseImage( &temp );

     add_remove_pt = 0;
  }
  else if( count > 0 )
  {
     cvCalcOpticalFlowPyrLK( prev_gray, gray, prev_pyramid, pyramid,
              points[0], points[1], count, cvSize(win_size,win_size), 3, status, 0,
              cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03), flags );
     flags |= CV_LKFLOW_PYR_A_READY;
     for( i = k = 0; i < count; i++ )
     {
        if( add_remove_pt )
        {
           double dx = pt.x - points[1][i].x;
           double dy = pt.y - points[1][i].y;

           if( dx*dx + dy*dy <= 25 )
           {
              add_remove_pt = 0;
              continue;
           }
         }

         if( !status[i] )
          continue;

         points[1][k++] = points[1][i];
         if ( delaunay == 0 ) // add all the points
         {
            cvSubdivDelaunay2DInsert( x_subdiv, points[1][i] );
            cvCalcSubdivVoronoi2D( x_subdiv );
         }
         // only add points included in (color-threshold)<p<(color+treshold)
         if ( ( delaunay > 0 ) && ( x_xmark[delaunay-1] != -1 ) )
         {
            int px = cvPointFrom32f(points[1][i]).x;
            int py = cvPointFrom32f(points[1][i]).y;
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

                uchar pred = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[delaunay-1]))[x_ymark[delaunay-1]*3];
                uchar pgreen = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[delaunay-1]))[x_ymark[delaunay-1]*3+1];
                uchar pblue = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[delaunay-1]))[x_ymark[delaunay-1]*3+2];

                int diff = abs(red-pred) + abs(green-pgreen) + abs(blue-pblue);

                // post( "pix_opencv_lk : point (%d,%d,%d) : diff : %d threshold : %d", blue, green, red, diff, threshold );

                if ( diff < threshold )
                {
                   cvSubdivDelaunay2DInsert( x_subdiv, points[1][i] );
                   cvCalcSubdivVoronoi2D( x_subdiv );
                }
              }
            }
         }

         cvCircle( rgb, cvPointFrom32f(points[1][i]), 3, CV_RGB(0,255,0), -1, 8,0);

         marked=0;
         oi=-1;
         dist=(comp_xsize>comp_ysize)?comp_xsize:comp_ysize;

         for ( im=0; im<MAX_MARKERS; im++ )
         {

           if ( x_xmark[im] == -1 ) continue; // i don't see the point

           odist=sqrt( pow( points[1][i].x - x_xmark[im], 2 ) + pow( points[1][i].y - x_ymark[im], 2 ) );

           // search for the closest point
           if ( odist <= maxmove )
           {
               if ( odist < dist )
               {
                 dist = odist;
                 marked=1;
                 oi=im;
               }
           }
         }

         if ( oi != -1 )
         {
           char tindex[4];
             sprintf( tindex, "%d", oi );
             cvPutText( rgb, tindex, cvPointFrom32f(points[1][i]), &font, CV_RGB(255,255,255));
             x_xmark[oi]=(int)points[1][i].x;
             x_ymark[oi]=(int)points[1][i].y;
             x_found[oi]=ftolerance;
             SETFLOAT(&x_list[0], oi);
             SETFLOAT(&x_list[1], x_xmark[oi]);
             SETFLOAT(&x_list[2], x_ymark[oi]);
             outlet_list( m_dataout, 0, 3, x_list );
         }

         if ( markall && !marked )
         {
           for ( im=0; im<MAX_MARKERS; im++)
           {
             if ( x_xmark[im] == -1 )
             {
               x_xmark[im]=points[1][i].x;
               x_ymark[im]=points[1][i].y;
               x_found[im]=ftolerance;
               break;
             }
           }
         }
      }
      count = k;
  }

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
           post( "pix_opencv_lk : lost point %d", im+1 );
        }
  }

  if( add_remove_pt && count < MAX_COUNT )
  {
        points[1][count++] = cvPointTo32f(pt);
        cvFindCornerSubPix( gray, points[1] + count - 1, 1,
           cvSize(win_size,win_size), cvSize(-1,-1),
           cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
        add_remove_pt = 0;
  }

  // draw the delaunay
  if ( delaunay >= 0 )
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

  CV_SWAP( prev_gray, gray, swap_temp );
  CV_SWAP( prev_pyramid, pyramid, swap_temp );
  CV_SWAP( points[0], points[1], swap_points );
  need_to_init = 0;

  cvCvtColor(rgb, rgba, CV_BGR2BGRA);
  memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_lk :: processRGBImage(imageStruct &image)
{ 
  int i, k;
  int im, oi;
  int marked;
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
    cvReleaseImage( &prev_gray );
    cvReleaseImage( &pyramid );
    cvReleaseImage( &prev_pyramid );

    rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    orgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    ogray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    prev_gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    pyramid = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    prev_pyramid = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    points[0] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
    points[1] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
    status = (char*)cvAlloc(MAX_COUNT);

  }

  memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
  memcpy( orgb->imageData, image.data, image.xsize*image.ysize*3 );
  cvCvtColor(rgb, gray, CV_BGRA2GRAY);

  if( night_mode )
      cvZero( rgb );

  for ( im=0; im<MAX_MARKERS; im++ )
  {
       x_found[im]--;
  }

  if ( delaunay >= 0 )
  {
    // init data structures for the delaunay
    x_fullrect.x = -comp_xsize/2;
    x_fullrect.y = -comp_ysize/2;
    x_fullrect.width = 2*comp_xsize;
    x_fullrect.height = 2*comp_ysize;

    x_storage = cvCreateMemStorage(0);
    x_subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*x_subdiv),
                               sizeof(CvSubdiv2DPoint),
                               sizeof(CvQuadEdge2D),
                               x_storage );
     cvInitSubdivDelaunay2D( x_subdiv, x_fullrect );
  }

  if( need_to_init )
  {
     /* automatic initialization */
     IplImage* eig = cvCreateImage( cvSize(gray->width,gray->height), 32, 1 );
     IplImage* temp = cvCreateImage( cvSize(gray->width,gray->height), 32, 1 );

     count = MAX_COUNT;
     cvGoodFeaturesToTrack( gray, eig, temp, points[1], &count,
                               quality, min_distance, 0, 3, 0, 0.04 );
     cvFindCornerSubPix( gray, points[1], count,
          cvSize(win_size,win_size), cvSize(-1,-1),
          cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
     cvReleaseImage( &eig );
     cvReleaseImage( &temp );

     add_remove_pt = 0;
  }
  else if( count > 0 )
  {
     cvCalcOpticalFlowPyrLK( prev_gray, gray, prev_pyramid, pyramid,
              points[0], points[1], count, cvSize(win_size,win_size), 3, status, 0,
              cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03), flags );
     flags |= CV_LKFLOW_PYR_A_READY;
     for( i = k = 0; i < count; i++ )
     {
        if( add_remove_pt )
        {
           double dx = pt.x - points[1][i].x;
           double dy = pt.y - points[1][i].y;

           if( dx*dx + dy*dy <= 25 )
           {
              add_remove_pt = 0;
              continue;
           }
         }

         if( !status[i] )
          continue;

         points[1][k++] = points[1][i];
         if ( delaunay == 0 ) // add all the points
         {
            cvSubdivDelaunay2DInsert( x_subdiv, points[1][i] );
            cvCalcSubdivVoronoi2D( x_subdiv );
         }
         // only add points included in (color-threshold)<p<(color+treshold)
         if ( ( delaunay > 0 ) && ( x_xmark[delaunay-1] != -1 ) )
         {
            int px = cvPointFrom32f(points[1][i]).x;
            int py = cvPointFrom32f(points[1][i]).y;
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

                uchar pred = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[delaunay-1]))[x_ymark[delaunay-1]*3];
                uchar pgreen = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[delaunay-1]))[x_ymark[delaunay-1]*3+1];
                uchar pblue = ((uchar*)(orgb->imageData + orgb->widthStep*x_xmark[delaunay-1]))[x_ymark[delaunay-1]*3+2];

                int diff = abs(red-pred) + abs(green-pgreen) + abs(blue-pblue);

                // post( "pix_opencv_lk : point (%d,%d,%d) : diff : %d", blue, green, red, diff );

                if ( diff < threshold )
                {
                   cvSubdivDelaunay2DInsert( x_subdiv, points[1][i] );
                   cvCalcSubdivVoronoi2D( x_subdiv );
                }
              }
            }
         }

         cvCircle( rgb, cvPointFrom32f(points[1][i]), 3, CV_RGB(0,255,0), -1, 8,0);

         marked=0;
         oi=-1;
         dist=(comp_xsize>comp_ysize)?comp_xsize:comp_ysize;

         for ( im=0; im<MAX_MARKERS; im++ )
         {
           if ( x_xmark[im] == -1 ) continue; // i don't see the point

           odist=sqrt( pow( points[1][i].x - x_xmark[im], 2 ) + pow( points[1][i].y - x_ymark[im], 2 ) );

           // search for this point
           if ( odist <= maxmove )
           {
               if ( odist < dist )
               {
                 dist = odist;
                 marked=1;
                 oi=im;
               }
           }
         }

         if ( oi !=-1 )
         { 
           char tindex[4];
              sprintf( tindex, "%d", oi );
              cvPutText( rgb, tindex, cvPointFrom32f(points[1][i]), &font, CV_RGB(255,255,255));
              x_xmark[oi]=points[1][i].x;
              x_ymark[oi]=points[1][i].y;
              x_found[oi]=ftolerance;
              SETFLOAT(&x_list[0], oi);
              SETFLOAT(&x_list[1], x_xmark[oi]);
              SETFLOAT(&x_list[2], x_ymark[oi]);
              outlet_list( m_dataout, 0, 3, x_list );
         }

         if ( markall && !marked )
         {
           for ( im=0; im<MAX_MARKERS; im++)
           {
             if ( x_xmark[im] == -1 )
             {
               x_xmark[im]=points[1][i].x;
               x_ymark[im]=points[1][i].y;
               x_found[im]=ftolerance;
               break;
             }
           }
         }
      }
      count = k;
  }

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
           post( "pix_opencv_lk : lost point %d", im+1 );
        }
  }

  if( add_remove_pt && count < MAX_COUNT )
  {
        points[1][count++] = cvPointTo32f(pt);
        cvFindCornerSubPix( gray, points[1] + count - 1, 1,
           cvSize(win_size,win_size), cvSize(-1,-1),
           cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
        add_remove_pt = 0;
  }

  // draw the delaunay
  if ( delaunay >= 0 )
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

  CV_SWAP( prev_gray, gray, swap_temp );
  CV_SWAP( prev_pyramid, pyramid, swap_temp );
  CV_SWAP( points[0], points[1], swap_points );
  need_to_init = 0;

  memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_lk :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_lk : yuv format not supported" );
}
    	
void pix_opencv_lk :: processGrayImage(imageStruct &image)
{ 
  int i, k;
  int im, oi;
  int marked;
  float dist, odist;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    cvReleaseImage( &rgba );
    cvReleaseImage( &orgb );
    cvReleaseImage( &rgb );
    cvReleaseImage( &ogray );
    cvReleaseImage( &gray );
    cvReleaseImage( &prev_gray );
    cvReleaseImage( &pyramid );
    cvReleaseImage( &prev_pyramid );

    rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    orgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    prev_gray = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    pyramid = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    prev_pyramid = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    points[0] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
    points[1] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(points[0][0]));
    status = (char*)cvAlloc(MAX_COUNT);

  }

  memcpy( gray->imageData, image.data, image.xsize*image.ysize );
  memcpy( ogray->imageData, image.data, image.xsize*image.ysize );

  if( night_mode )
      cvZero( gray );

  for ( im=0; im<MAX_MARKERS; im++ )
  {
       x_found[im]--;
  }

  if ( delaunay >= 0 )
  {
    // init data structures for the delaunay
    x_fullrect.x = -comp_xsize/2;
    x_fullrect.y = -comp_ysize/2;
    x_fullrect.width = 2*comp_xsize;
    x_fullrect.height = 2*comp_ysize;

    x_storage = cvCreateMemStorage(0);
    x_subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*x_subdiv),
                               sizeof(CvSubdiv2DPoint),
                               sizeof(CvQuadEdge2D),
                               x_storage );
     cvInitSubdivDelaunay2D( x_subdiv, x_fullrect );
  }

  if( need_to_init )
  {
     /* automatic initialization */
     IplImage* eig = cvCreateImage( cvSize(gray->width,gray->height), 32, 1 );
     IplImage* temp = cvCreateImage( cvSize(gray->width,gray->height), 32, 1 );

     count = MAX_COUNT;
     cvGoodFeaturesToTrack( gray, eig, temp, points[1], &count,
                               quality, min_distance, 0, 3, 0, 0.04 );
     cvFindCornerSubPix( gray, points[1], count,
          cvSize(win_size,win_size), cvSize(-1,-1),
          cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
     cvReleaseImage( &eig );
     cvReleaseImage( &temp );

     add_remove_pt = 0;
  }
  else if( count > 0 )
  {
     cvCalcOpticalFlowPyrLK( prev_gray, gray, prev_pyramid, pyramid,
              points[0], points[1], count, cvSize(win_size,win_size), 3, status, 0,
              cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03), flags );
     flags |= CV_LKFLOW_PYR_A_READY;
     for( i = k = 0; i < count; i++ )
     {
        if( add_remove_pt )
        {
           double dx = pt.x - points[1][i].x;
           double dy = pt.y - points[1][i].y;

           if( dx*dx + dy*dy <= 25 )
           {
              add_remove_pt = 0;
              continue;
           }
         }

         if( !status[i] )
          continue;

         points[1][k++] = points[1][i];
         if ( delaunay == 0 ) // add all the points
         {
            cvSubdivDelaunay2DInsert( x_subdiv, points[1][i] );
            cvCalcSubdivVoronoi2D( x_subdiv );
         }
         // only add points included in (color-threshold)<p<(color+treshold)
         if ( ( delaunay > 0 ) && ( x_xmark[delaunay-1] != -1 ) )
         {
            int px = cvPointFrom32f(points[1][i]).x;
            int py = cvPointFrom32f(points[1][i]).y;
            int ppx, ppy;

            // eight connected pixels
            for ( ppx=px-1; ppx<=px+1; ppx++ )
            {
              for ( ppy=py-1; ppy<=py+1; ppy++ )
              {
                if ( ( ppx < 0 ) || ( ppx >= comp_xsize ) ) continue;
                if ( ( ppy < 0 ) || ( ppy >= comp_ysize ) ) continue;

                uchar lum = ((uchar*)(ogray->imageData + ogray->widthStep*ppx))[ppy];
                uchar plum = ((uchar*)(ogray->imageData + ogray->widthStep*x_xmark[delaunay-1]))[x_ymark[delaunay-1]];

                int diff = abs(plum-lum);

                // post( "pix_opencv_lk : point (%d,%d,%d) : diff : %d", blue, green, red, diff );

                if ( diff < threshold )
                {
                   cvSubdivDelaunay2DInsert( x_subdiv, points[1][i] );
                   cvCalcSubdivVoronoi2D( x_subdiv );
                }
              }
            }
         }

         cvCircle( gray, cvPointFrom32f(points[1][i]), 3, CV_RGB(0,255,0), -1, 8,0);

         marked=0;
         oi=-1;
         dist=(comp_xsize>comp_ysize)?comp_xsize:comp_ysize;

         for ( im=0; im<MAX_MARKERS; im++ )
         {
           if ( x_xmark[im] == -1 ) continue; // i don't see the point

            odist=sqrt( pow( points[1][i].x - x_xmark[im], 2 ) + pow( points[1][i].y - x_ymark[im], 2 ) );

            // search for the closest point
            if ( odist <= maxmove )
            {
               if ( odist < dist )
               {
                 dist = odist;
                 marked=1;
                 oi=im;
               }
            }
          }

          if ( oi !=-1 )
          { 
             char tindex[4];
               sprintf( tindex, "%d", oi );
               cvPutText( gray, tindex, cvPointFrom32f(points[1][i]), &font, CV_RGB(255,255,255));
               x_xmark[oi]=points[1][i].x;
               x_ymark[oi]=points[1][i].y;
               x_found[oi]=ftolerance;
               SETFLOAT(&x_list[0], oi);
               SETFLOAT(&x_list[1], x_xmark[oi]);
               SETFLOAT(&x_list[2], x_ymark[oi]);
               outlet_list( m_dataout, 0, 3, x_list );
         }

         if ( markall && !marked )
         {
           for ( im=0; im<MAX_MARKERS; im++)
           {
             if ( x_xmark[im] == -1 )
             {
               x_xmark[im]=points[1][i].x;
               x_ymark[im]=points[1][i].y;
               x_found[im]=ftolerance;
               break;
             }
           }
         }
      }
      count = k;
  }

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
           post( "pix_opencv_lk : lost point %d", im+1 );
        }
  }

  if( add_remove_pt && count < MAX_COUNT )
  {
        points[1][count++] = cvPointTo32f(pt);
        cvFindCornerSubPix( gray, points[1] + count - 1, 1,
           cvSize(win_size,win_size), cvSize(-1,-1),
           cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
        add_remove_pt = 0;
  }

  // draw the delaunay
  if ( delaunay >= 0 )
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
               cvLine( gray, iorg, idst, CV_RGB(255,0,0), 1, CV_AA, 0 );
           }
         }

         CV_NEXT_SEQ_ELEM( elem_size, reader );
     }
  }

  CV_SWAP( prev_gray, gray, swap_temp );
  CV_SWAP( prev_pyramid, pyramid, swap_temp );
  CV_SWAP( points[0], points[1], swap_points );
  need_to_init = 0;

  memcpy( image.data, gray->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////

void pix_opencv_lk :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::winSizeMessCallback,
		  gensym("winsize"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::nightModeMessCallback,
		  gensym("nightmode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::qualityMessCallback,
		  gensym("quality"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::initMessCallback,
		  gensym("init"), A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::markMessCallback,
		  gensym("mark"), A_GIMME, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::deleteMessCallback,
		  gensym("delete"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::clearMessCallback,
		  gensym("clear"), A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::minDistanceMessCallback,
		  gensym("mindistance"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::maxMoveMessCallback,
		  gensym("maxmove"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::ftoleranceMessCallback,
		  gensym("ftolerance"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::delaunayMessCallback,
		  gensym("delaunay"), A_SYMBOL, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_lk::pdelaunayMessCallback,
		  gensym("pdelaunay"), A_FLOAT, A_FLOAT, A_NULL);
}

void  pix_opencv_lk :: winSizeMessCallback(void *data, t_floatarg winsize)
{
    GetMyClass(data)->winSizeMess((float)winsize);
}

void  pix_opencv_lk :: nightModeMessCallback(void *data, t_floatarg nightmode)
{
    GetMyClass(data)->nightModeMess((float)nightmode);
}

void  pix_opencv_lk :: qualityMessCallback(void *data, t_floatarg quality)
{
    GetMyClass(data)->qualityMess((float)quality);
}

void  pix_opencv_lk :: initMessCallback(void *data)
{
    GetMyClass(data)->initMess();
}

void  pix_opencv_lk :: markMessCallback(void *data, t_symbol *s, int argc, t_atom *argv)
{
    GetMyClass(data)->markMess(argc, argv);
}

void  pix_opencv_lk :: deleteMessCallback(void *data, t_floatarg index)
{
    GetMyClass(data)->deleteMess((float)index);
}

void  pix_opencv_lk :: clearMessCallback(void *data)
{
    GetMyClass(data)->clearMess();
}

void  pix_opencv_lk :: minDistanceMessCallback(void *data, t_floatarg mindistance)
{
    GetMyClass(data)->minDistanceMess((float)mindistance);
}

void  pix_opencv_lk :: maxMoveMessCallback(void *data, t_floatarg maxmove)
{
    GetMyClass(data)->maxMoveMess((float)maxmove);
}

void  pix_opencv_lk :: ftoleranceMessCallback(void *data, t_floatarg ftolerance)
{
    GetMyClass(data)->ftoleranceMess((float)ftolerance);
}

void  pix_opencv_lk :: delaunayMessCallback(void *data, t_symbol *s)
{
    GetMyClass(data)->delaunayMess(s);
}

void  pix_opencv_lk :: pdelaunayMessCallback(void *data, t_floatarg fpoint, t_floatarg fthreshold)
{
    GetMyClass(data)->pdelaunayMess((float)fpoint, (float)fthreshold);
}

void  pix_opencv_lk :: winSizeMess(float fwinsize)
{
    if (fwinsize>1.0) win_size = (int)fwinsize;
}

void  pix_opencv_lk :: nightModeMess(float fnightmode)
{
    if ((fnightmode==0.0)||(fnightmode==1.0)) night_mode = (int)fnightmode;
}

void  pix_opencv_lk :: qualityMess(float fquality)
{
    if (fquality>0.0) quality = fquality;
}

void  pix_opencv_lk :: initMess(void)
{
    need_to_init = 1;
}

void  pix_opencv_lk :: markMess(int argc, t_atom *argv)
{
  int i;
  int inserted;

    if ( argc == 1 ) // mark all or none
    {
      if ( argv[0].a_type != A_SYMBOL )
      {
        error( "pix_opencv_lk : wrong argument (should be 'all')" );
        return;
      }
      if ( !strcmp( argv[0].a_w.w_symbol->s_name, "all" ) )
      {
        markall = 1;
        return;
      }
      if ( !strcmp( argv[0].a_w.w_symbol->s_name, "none" ) )
      {
        markall = 0;
        clearMess();
        return;
      }
    }
    else
    {
      if ( ( argv[0].a_type != A_FLOAT ) || ( argv[1].a_type != A_FLOAT ) )
      {
        error( "pix_opencv_lk : wrong argument (should be mark px py)" );
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
              x_found[i] = ftolerance;
              inserted = 1;
              // post( "pix_opencv_lk : inserted point (%d,%d)", px, py );
              break;
           }
        }
        if ( !inserted )
        {
           post( "pix_opencv_lk : max markers reached" );
        }
      }
   }
}

void  pix_opencv_lk :: deleteMess(float index)
{
  int i;

    if ( ( index < 0.0 ) || ( index >= MAX_MARKERS ) )
    {
       return;
    }

    x_xmark[(int)index] = -1;
    x_ymark[(int)index] = -1;

}

void  pix_opencv_lk :: clearMess(void)
{
  int i;

    for ( i=0; i<MAX_MARKERS; i++)
    {
      x_xmark[i] = -1;
      x_ymark[i] = -1;
    }

}

void  pix_opencv_lk :: minDistanceMess(float fmindistance)
{
    if (fmindistance>1.0) min_distance = (int)fmindistance;
}

void  pix_opencv_lk :: maxMoveMess(float fmaxmove)
{
  // has to be more than the size of a point
  if (fmaxmove>=3.0) maxmove = (int)fmaxmove;
}

void  pix_opencv_lk :: ftoleranceMess(float ftolerance)
{
  if (ftolerance>=0.0) ftolerance = (int)ftolerance;
}

void  pix_opencv_lk :: delaunayMess(t_symbol *s)
{
  if (s == gensym("on"))
     delaunay = 0;
  if (s == gensym("off"))
     delaunay = -1;
}

void  pix_opencv_lk :: pdelaunayMess(float fpoint, float fthreshold)
{
  if (((int)fpoint>0) && ((int)fpoint<MAX_MARKERS))
  {
     delaunay = (int)fpoint;
     threshold = (int)fthreshold;
     // post( "pix_opencv_lk : setting threshold to : %d", threshold );
  }
}

