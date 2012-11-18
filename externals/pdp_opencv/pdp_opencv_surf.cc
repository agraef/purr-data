/*
 *   Pure Data Packet module.
 *   Copyright (c) by Tom Schouten <pdp@zzz.kotnet.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <dlfcn.h>
#include <ctype.h>

#include "pdp.h"

#ifndef _EiC
#include "cv.h"
#endif

#define MAX_MARKERS 500
#define DSCSIZE 128

typedef struct pdp_opencv_surf_struct
{
  t_object x_obj;
  t_float x_f;

  t_outlet *x_outlet0;
  t_outlet *x_outlet1;
  t_atom x_list[3];

  int x_packet0;
  int x_packet1;
  int x_dropped;
  int x_queue_id;

  int x_width;
  int x_height;
  int x_size;

  int x_maxmove;
  int x_delaunay;
  int x_threshold;
  int night_mode;
  int x_markall;
  int x_xmark[MAX_MARKERS];
  int x_ymark[MAX_MARKERS];
  float x_rdesc[MAX_MARKERS][DSCSIZE];
  int x_found[MAX_MARKERS];

  // The output and temporary images
  IplImage *image, *oimage, *grey;
  
  CvSeq *objectKeypoints, *objectDescriptors;
  int x_hessian;
  int x_criteria;
  int x_ftolerance;

  CvFont font;

  // structures needed for the delaunay
  CvRect x_fullrect;
  CvMemStorage* x_storage;
  CvSubdiv2D* x_subdiv;
    
} t_pdp_opencv_surf;

static void pdp_opencv_surf_mark(t_pdp_opencv_surf *x, t_symbol *s, int argc, t_atom *argv );

static void pdp_opencv_surf_clear(t_pdp_opencv_surf *x );

static double pdp_opencv_surf_compare_descriptors( const float* d1, const float* d2, int length )
{
    double total_cost = 0;
    assert( length % 4 == 0 );
    for( int i = 0; i < length; i += 4 )
    {
        double t0 = d1[i] - d2[i];
        double t1 = d1[i+1] - d2[i+1];
        double t2 = d1[i+2] - d2[i+2];
        double t3 = d1[i+3] - d2[i+3];
        total_cost += t0*t0 + t1*t1 + t2*t2 + t3*t3;
    }
    return total_cost;
}

static void pdp_opencv_surf_process_rgb(t_pdp_opencv_surf *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    int i,j,k,im;
    char tindex[4];
    int descsize;
    int oi;
    float dist, odist;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height) || (!x->image)) 
    {

      post("pdp_opencv_surf :: resizing plugins");
  
      x->x_width = header->info.image.width;
      x->x_height = header->info.image.height;
      x->x_size = x->x_width*x->x_height;
    
      //Destroy cv_images
      if ( x->image )
      {
        cvReleaseImage( &x->image );
        cvReleaseImage( &x->oimage );
        cvReleaseImage( &x->grey );
      }
   
      //Create cv_images 
      x->image = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
      x->oimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
      x->grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    memcpy( x->image->imageData, data, x->x_size*3 );
    memcpy( x->oimage->imageData, data, x->x_size*3 );
        
    cvCvtColor( x->image, x->grey, CV_RGB2GRAY );

    x->x_storage = cvCreateMemStorage(0);

    if( x->night_mode )
        cvZero( x->image );
        
    for ( im=0; im<MAX_MARKERS; im++ )
    {
        x->x_found[im]--;
    }

    if ( x->x_delaunay >= 0 )
    {
       // init data structures for the delaunay
       x->x_fullrect.x = -x->x_width/2;
       x->x_fullrect.y = -x->x_height/2;
       x->x_fullrect.width = 2*x->x_width;
       x->x_fullrect.height = 2*x->x_height;

       x->x_subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*x->x_subdiv),
                               sizeof(CvSubdiv2DPoint),
                               sizeof(CvQuadEdge2D),
                               x->x_storage );
       cvInitSubdivDelaunay2D( x->x_subdiv, x->x_fullrect );
    }

    cvExtractSURF( x->grey, 0, &x->objectKeypoints, &x->objectDescriptors, x->x_storage, cvSURFParams(x->x_hessian, 1) );
    descsize = (int)(x->objectDescriptors->elem_size/sizeof(float));

    for( i = 0; i < x->objectKeypoints->total; i++ )
    {
      CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( x->objectKeypoints, i );
      const float* rdesc = (const float*)cvGetSeqElem( x->objectDescriptors, i );

       if ( x->x_delaunay == 0 ) // add all the points
       {
          cvSubdivDelaunay2DInsert( x->x_subdiv, r1->pt );
          cvCalcSubdivVoronoi2D( x->x_subdiv );
       }

       // only add points included in (color-threshold)<p<(color+treshold)
       if ( ( x->x_delaunay > 0 ) && ( x->x_xmark[x->x_delaunay] != -1 ) ) 
       {
         int px = cvPointFrom32f(r1->pt).x;
         int py = cvPointFrom32f(r1->pt).y;
         int ppx, ppy;

         // eight connected pixels 
         for ( ppx=px-1; ppx<=px+1; ppx++ )
         { 
           for ( ppy=py-1; ppy<=py+1; ppy++ )
           { 
             if ( ( ppx < 0 ) || ( ppx >= x->x_width ) ) continue;
             if ( ( ppy < 0 ) || ( ppy >= x->x_height ) ) continue;

             uchar red = ((uchar*)(x->oimage->imageData + x->oimage->widthStep*ppx))[ppy*3];
             uchar green = ((uchar*)(x->oimage->imageData + x->oimage->widthStep*ppx))[ppy*3+1];
             uchar blue = ((uchar*)(x->oimage->imageData + x->oimage->widthStep*ppx))[ppy*3+2];

             uchar pred = ((uchar*)(x->oimage->imageData + x->oimage->widthStep*x->x_xmark[x->x_delaunay]))[x->x_ymark[x->x_delaunay]*3];
             uchar pgreen = ((uchar*)(x->oimage->imageData + x->oimage->widthStep*x->x_xmark[x->x_delaunay]))[x->x_ymark[x->x_delaunay]*3+1];
             uchar pblue = ((uchar*)(x->oimage->imageData + x->oimage->widthStep*x->x_xmark[x->x_delaunay]))[x->x_ymark[x->x_delaunay]*3+2];

             int diff = abs(red-pred) + abs(green-pgreen) + abs(blue-pblue);

             // post( "pdp_opencv_surf : point (%d,%d,%d) : diff : %d", blue, green, red, diff );

             if ( diff < x->x_threshold )
             {
                cvSubdivDelaunay2DInsert( x->x_subdiv, r1->pt );
                cvCalcSubdivVoronoi2D( x->x_subdiv );
             }
           }
         }
       }

       cvCircle( x->image, cvPointFrom32f(r1->pt), 3, CV_RGB(0,255,0), -1, 8,0);

       oi=-1;
       dist=(x->x_width>x->x_height)?x->x_width:x->x_height;

       // mark the point if it is not already
       if ( x->x_markall )
       {
         int marked = 0;

         for ( im=0; im<MAX_MARKERS; im++ )
         {

           if ( x->x_xmark[im] == -1 ) continue; // no points

           odist=sqrt( pow( r1->pt.x-x->x_xmark[im], 2 ) + pow( r1->pt.y-x->x_ymark[im], 2 ) );

           if ( odist <= x->x_maxmove )
           {
             marked = 1;
             break;
           }
         }

         if ( !marked ) 
         {
            for ( i=0; i<MAX_MARKERS; i++)
            {
               if ( x->x_xmark[i] == -1 )
               {
                  x->x_xmark[i] = r1->pt.x;
                  x->x_ymark[i] = r1->pt.y;
                  x->x_found[i] = x->x_ftolerance;
                  memset( (float * )x->x_rdesc[i], 0x0, DSCSIZE*sizeof(float));
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

      oi=-1;
      dist=(x->x_width>x->x_height)?x->x_width:x->x_height;

      if ( x->x_xmark[im] == -1 ) continue; // no points

      for( i = 0; i < x->objectKeypoints->total; i++ )
      {
        CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( x->objectKeypoints, i );
        const float* rdesc = (const float*)cvGetSeqElem( x->objectDescriptors, i );
        int descsize = (int)(x->objectDescriptors->elem_size/sizeof(float));

         // manually marked points
         // recognized on position
         // if ( ( x->x_xmark[im] != -1.0 ) && ( x->x_rdesc[im][0] == 0.0 ) )

         odist=sqrt( pow( r1->pt.x-x->x_xmark[im], 2 ) + pow( r1->pt.y-x->x_ymark[im], 2 ) );
         
         if ( odist <= x->x_maxmove )
         {
            if ( odist < dist )
            {
              oi=im;
              x->x_xmark[oi]=r1->pt.x;
              x->x_ymark[oi]=r1->pt.y;
              memcpy( (float * )x->x_rdesc[oi], rdesc, descsize*sizeof(float));
              dist = odist;
            }
         }
       }

       if ( oi !=-1 )
       {
          sprintf( tindex, "%d", oi );
          cvPutText( x->image, tindex, cvPoint(x->x_xmark[oi],x->x_ymark[oi]), &x->font, CV_RGB(255,255,255));
          x->x_found[oi] = x->x_ftolerance;
          SETFLOAT(&x->x_list[0], oi);
          SETFLOAT(&x->x_list[1], x->x_xmark[oi]);
          SETFLOAT(&x->x_list[2], x->x_ymark[oi]);
          outlet_list( x->x_outlet1, 0, 3, x->x_list );
        }

        // recognize points according to their SURF descriptor ( size = 128 ) 
        // this code is desactivated because it isn't more stable than the positions
        // if ( ( x->x_xmark[im] != -1.0 ) && ( x->x_rdesc[im][0] != 0.0 ) )
        // {
        //   d = pdp_opencv_surf_compare_descriptors( x->x_rdesc[im], rdesc, descsize );
        //   if( d < dist1 )
        //   {
        //     dist1 = d;
        //     neighbour = i;
        //     // post( "pdp_opencv_surf : point %d, min distance : %d ( with %d )", i, (int)d, im );
        //   }
        // }
    }

    // check if we found the point
    // if ( dist1 < x->x_criteria )
    // {
    //   CvSURFPoint* r1 = (CvSURFPoint*)cvGetSeqElem( x->objectKeypoints, neighbour );
    //   const float* rdesc = (const float*)cvGetSeqElem( x->objectDescriptors, neighbour );

    //   // point identified 
    //   sprintf( tindex, "%d", im+1 );
    //   cvPutText( x->image, tindex, cvPointFrom32f(r1->pt), &x->font, CV_RGB(255,255,255));
    //   x->x_xmark[im]=r1->pt.x;
    //   x->x_ymark[im]=r1->pt.y;
    //   memcpy( (float * )x->x_rdesc[im], rdesc, descsize*sizeof(float));
    //   x->x_found[im]=1;
    //   SETFLOAT(&x->x_list[0], im+1);
    //   SETFLOAT(&x->x_list[1], x->x_xmark[im]);
    //   SETFLOAT(&x->x_list[2], x->x_ymark[im]);
    //   outlet_list( x->x_outlet1, 0, 3, x->x_list );
    // }

    // draw the delaunay
    if ( x->x_delaunay >= 0 )
    {
       CvSeqReader  reader;
       int i, total = x->x_subdiv->edges->total;
       int elem_size = x->x_subdiv->edges->elem_size;

       cvStartReadSeq( (CvSeq*)(x->x_subdiv->edges), &reader, 0 );

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

                 if ( ( org.x > 0 ) && ( org.x < x->x_width ) &&
                      ( dst.x > 0 ) && ( dst.x < x->x_width ) &&
                      ( org.y > 0 ) && ( org.y < x->x_height ) &&
                      ( dst.y > 0 ) && ( dst.y < x->x_height ) )
                 cvLine( x->image, iorg, idst, CV_RGB(255,0,0), 1, CV_AA, 0 );
             }
           }

           // draw the voronoi : useless in my opinion as points belongs to contours
           /*
           if( CV_IS_SET_ELEM( edge+1 ))
           {
             org_pt = cvSubdiv2DEdgeOrg((CvSubdiv2DEdge)edge+1);
             dst_pt = cvSubdiv2DEdgeDst((CvSubdiv2DEdge)edge+1);

             if( org_pt && dst_pt )
             {
                 org = org_pt->pt;
                 dst = dst_pt->pt;

                 iorg = cvPoint( cvRound( org.x ), cvRound( org.y ));
                 idst = cvPoint( cvRound( dst.x ), cvRound( dst.y ));

                 cvLine( x->image, iorg, idst, CV_RGB(0,0,255), 1, CV_AA, 0 );
             }
           }
           */

           CV_NEXT_SEQ_ELEM( elem_size, reader );
       }
    }

    // suppress lost points
    for ( im=0; im<MAX_MARKERS; im++ )
    {
        if ( (x->x_xmark[im] != -1.0 ) && (x->x_found[im]<=0) )
        {
           x->x_xmark[im]=-1.0;
           x->x_ymark[im]=-1.0;
           SETFLOAT(&x->x_list[0], im+1);
           SETFLOAT(&x->x_list[1], x->x_xmark[im]);
           SETFLOAT(&x->x_list[2], x->x_ymark[im]);
           // send a lost point message to the patch
           outlet_list( x->x_outlet1, 0, 3, x->x_list );
           // post( "pdp_opencv_surf : lost point %d", im+1 );
        }
    }

    memcpy( newdata, x->image->imageData, x->x_size*3 );

    cvReleaseMemStorage( &x->x_storage ); 

    return;
}


static void pdp_opencv_surf_nightmode(t_pdp_opencv_surf *x, t_floatarg f)
{
  if ((f==0.0)||(f==1.0)) x->night_mode = (int)f;
}

static void pdp_opencv_surf_ftolerance(t_pdp_opencv_surf *x, t_floatarg f)
{
  if (f>0.0) x->x_ftolerance = (int)f;
}

static void pdp_opencv_surf_maxmove(t_pdp_opencv_surf *x, t_floatarg f)
{
  // has to be more than the size of a point
  if (f>=3.0) x->x_maxmove = (int)f;
}

static void pdp_opencv_surf_hessian(t_pdp_opencv_surf *x, t_floatarg f)
{
  if ( (int)f>0 ) x->x_hessian = (int)f;
}

static void pdp_opencv_surf_criteria(t_pdp_opencv_surf *x, t_floatarg f)
{
  if ( (int)f>0 ) x->x_criteria = (int)f;
}

static void pdp_opencv_surf_delaunay(t_pdp_opencv_surf *x, t_symbol *s)
{
  if (s == gensym("on")) 
     x->x_delaunay = 0;
  if (s == gensym("off")) 
     x->x_delaunay = -1;
}

static void pdp_opencv_surf_pdelaunay(t_pdp_opencv_surf *x, t_floatarg point, t_floatarg threshold)
{
  if (((int)point>0) && ((int)point<MAX_MARKERS))
  {
     x->x_delaunay = (int)point;
     x->x_threshold = (int)threshold;
  }
}

static void pdp_opencv_surf_mark(t_pdp_opencv_surf *x, t_symbol *s, int argc, t_atom *argv )
{
  int i;
  int inserted;
  int px,py;

    if ( argc == 1 ) // mark all
    {
      if ( argv[0].a_type != A_SYMBOL )
      {
        error( "pdp_opencv_surf : wrong argument (should be 'all')" );
        return;
      }
      if ( !strcmp( argv[0].a_w.w_symbol->s_name, "all" ) )
      {
        x->x_markall = 1;
        return;
      }
      if ( !strcmp( argv[0].a_w.w_symbol->s_name, "none" ) )
      {
        x->x_markall = 0;
        pdp_opencv_surf_clear(x);
        return;
      }
    }
    else
    {
      if ( ( argv[0].a_type != A_FLOAT ) || ( argv[1].a_type != A_FLOAT ) )
      {
        error( "pdp_opencv_surf : wrong argument (should be mark px py)" );
        return;
      }
      else 
      {
        float fperx = argv[0].a_w.w_float;
        float fpery = argv[1].a_w.w_float;
   
        if ( ( fperx < 0.0 ) || ( fperx > 1.0 ) || ( fpery < 0.0 ) || ( fpery > 1.0 ) )
        {
           return;
        }

        px = (int)(fperx*x->x_width);
        py = (int)(fpery*x->x_height);
        inserted = 0;
        for ( i=0; i<MAX_MARKERS; i++)
        {
           if ( x->x_xmark[i] == -1 )
           {
              x->x_xmark[i] = px;
              x->x_ymark[i] = py;
              x->x_found[i] = x->x_ftolerance;
              memset( (float * )x->x_rdesc[i], 0x0, DSCSIZE*sizeof(float));
              inserted = 1;
              break;
           }
        }
        if ( !inserted )
        {
           post( "pdp_opencv_surf : max markers reached" );
        }
      }
    }
}

static void pdp_opencv_surf_delete(t_pdp_opencv_surf *x, t_floatarg findex )
{
  int i;

    if ( ( findex < 1.0 ) || ( findex > MAX_MARKERS ) )
    {
       return;
    }

    x->x_xmark[(int)findex] = -1;
    x->x_ymark[(int)findex] = -1;
}

static void pdp_opencv_surf_clear(t_pdp_opencv_surf *x )
{
  int i;

    for ( i=0; i<MAX_MARKERS; i++)
    {
      x->x_xmark[i] = -1;
      x->x_ymark[i] = -1;
    }
}

static void pdp_opencv_surf_sendpacket(t_pdp_opencv_surf *x)
{
  /* release the packet */
  pdp_packet_mark_unused(x->x_packet0);
  x->x_packet0 = -1;

  /* unregister and propagate if valid dest packet */
  pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_surf_process(t_pdp_opencv_surf *x)
{
   int encoding;
   t_pdp *header = 0;
   char *parname;
   unsigned pi;
   int partype;
   float pardefault;
   t_atom plist[2];
   t_atom tlist[2];
   t_atom vlist[2];

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
  && (PDP_BITMAP == header->type)){
    
  /* pdp_opencv_surf_process inputs and write into active inlet */
  switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

  case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_surf_process_rgb, (void*)pdp_opencv_surf_sendpacket, &x->x_queue_id);
      break;

  default:
      /* don't know the type, so dont pdp_opencv_surf_process */
      break;
      
  }
    }

}

static void pdp_opencv_surf_input_0(t_pdp_opencv_surf *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char *)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_surf_process(x);
    }
}

static void pdp_opencv_surf_free(t_pdp_opencv_surf *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
    //Destroy cv_images
    cvReleaseImage( &x->image );
    cvReleaseImage( &x->grey );
}

t_class *pdp_opencv_surf_class;


void *pdp_opencv_surf_new(t_floatarg f)
{
  int i;

  t_pdp_opencv_surf *x = (t_pdp_opencv_surf *)pd_new(pdp_opencv_surf_class);

  x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
  x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);

  x->x_packet0 = -1;
  x->x_packet1 = -1;
  x->x_queue_id = -1;

  x->x_width  = 320;
  x->x_height = 240;
  x->x_size   = x->x_width * x->x_height;

  x->night_mode = 0;
  x->x_maxmove = 20;
  x->x_delaunay = -1;
  x->x_threshold = -1;

  x->objectKeypoints = NULL;
  x->objectDescriptors = NULL;
  x->x_hessian = 500;
  x->x_criteria = 20;
  x->x_ftolerance = 5;

  x->x_markall = 0;
  for ( i=0; i<MAX_MARKERS; i++ )
  {
     x->x_xmark[i] = -1;
     x->x_ymark[i] = -1;
  }

  // initialize font
  cvInitFont( &x->font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );
    
  x->image = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
  x->oimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
  x->grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );

  return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_surf_setup(void)
{

    post( "    pdp_opencv_surf");
    pdp_opencv_surf_class = class_new(gensym("pdp_opencv_surf"), (t_newmethod)pdp_opencv_surf_new,
      (t_method)pdp_opencv_surf_free, sizeof(t_pdp_opencv_surf), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_surf_class, (t_method)pdp_opencv_surf_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_surf_class, (t_method)pdp_opencv_surf_nightmode, gensym("nightmode"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_surf_class, (t_method)pdp_opencv_surf_hessian, gensym("hessian"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_surf_class, (t_method)pdp_opencv_surf_ftolerance, gensym("ftolerance"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_surf_class, (t_method)pdp_opencv_surf_mark, gensym("mark"), A_GIMME, A_NULL );   
    class_addmethod(pdp_opencv_surf_class, (t_method)pdp_opencv_surf_delete, gensym("delete"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_surf_class, (t_method)pdp_opencv_surf_clear, gensym("clear"), A_NULL );   
    class_addmethod(pdp_opencv_surf_class, (t_method)pdp_opencv_surf_maxmove, gensym("maxmove"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_surf_class, (t_method)pdp_opencv_surf_delaunay, gensym("delaunay"),  A_SYMBOL, A_NULL );   
    class_addmethod(pdp_opencv_surf_class, (t_method)pdp_opencv_surf_pdelaunay, gensym("pdelaunay"),  A_FLOAT, A_FLOAT, A_NULL );   

}

#ifdef __cplusplus
}
#endif
