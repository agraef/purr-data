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
const int MAX_COUNT = 500;

typedef struct pdp_opencv_lk_struct
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

  int win_size;
  double quality;
  int min_distance;
  int x_maxmove;
  int x_ftolerance;
  int x_markall;
  int x_delaunay;
  int x_threshold;
  int x_xmark[MAX_MARKERS];
  int x_ymark[MAX_MARKERS];
  int x_found[MAX_MARKERS];

  // The output and temporary images
  IplImage *image, *oimage, *grey, *prev_grey, *pyramid, *prev_pyramid, *swap_temp;
  
  CvPoint2D32f* points[2], *swap_points;
  char* status;
  int count;
  int need_to_init;
  int night_mode;
  int flags;
  int add_remove_pt;
  CvPoint pt;
  CvFont font;

  // structures needed for the delaunay
  CvRect x_fullrect;
  CvMemStorage* x_storage;
  CvSubdiv2D* x_subdiv;
    
} t_pdp_opencv_lk;

static void pdp_opencv_lk_clear(t_pdp_opencv_lk *x);

static void pdp_opencv_lk_process_rgb(t_pdp_opencv_lk *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    int i,j,k,im,oi;
    int marked;
    float dist, odist;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height) || (!x->image)) 
    {

      post("pdp_opencv_lk :: resizing plugins");
  
      x->x_width = header->info.image.width;
      x->x_height = header->info.image.height;
      x->x_size = x->x_width*x->x_height;
    
      //Destroy cv_images
      cvReleaseImage( &x->image );
      cvReleaseImage( &x->oimage );
      cvReleaseImage( &x->grey );
      cvReleaseImage( &x->prev_grey );
      cvReleaseImage( &x->pyramid );
      cvReleaseImage( &x->prev_pyramid );
   
      //Create cv_images 
      x->image = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
      x->oimage = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
      x->grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
      x->prev_grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
      x->pyramid = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
      x->prev_pyramid = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
      x->points[0] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(x->points[0][0]));
      x->points[1] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(x->points[0][0]));
      x->status = (char*)cvAlloc(MAX_COUNT);
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    memcpy( x->image->imageData, data, x->x_size*3 );
    memcpy( x->oimage->imageData, data, x->x_size*3 );
        
    cvCvtColor( x->image, x->grey, CV_RGB2GRAY );

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

       x->x_storage = cvCreateMemStorage(0);
       x->x_subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*x->x_subdiv),
                               sizeof(CvSubdiv2DPoint),
                               sizeof(CvQuadEdge2D),
                               x->x_storage );
       cvInitSubdivDelaunay2D( x->x_subdiv, x->x_fullrect );
    }

    if( x->need_to_init )
    {
        /* automatic initialization */
        IplImage* eig = cvCreateImage( cvSize(x->grey->width,x->grey->height), 32, 1 );
        IplImage* temp = cvCreateImage( cvSize(x->grey->width,x->grey->height), 32, 1 );

        x->count = MAX_COUNT;
        cvGoodFeaturesToTrack( x->grey, eig, temp, x->points[1], &x->count,
                               x->quality, x->min_distance, 0, 3, 0, 0.04 );
        cvFindCornerSubPix( x->grey, x->points[1], x->count,
             cvSize(x->win_size,x->win_size), cvSize(-1,-1),
             cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
        cvReleaseImage( &eig );
        cvReleaseImage( &temp );

        x->add_remove_pt = 0;
    }
    else if( x->count > 0 )
    {
        cvCalcOpticalFlowPyrLK( x->prev_grey, x->grey, x->prev_pyramid, x->pyramid,
              x->points[0], x->points[1], x->count, cvSize(x->win_size,x->win_size), 3, x->status, 0,
              cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03), x->flags );
        x->flags |= CV_LKFLOW_PYR_A_READY;
        for( i = k = 0; i < x->count; i++ )
        {
           if( x->add_remove_pt )
           {
              double dx = x->pt.x - x->points[1][i].x;
              double dy = x->pt.y - x->points[1][i].y;

              if( dx*dx + dy*dy <= 25 )
              {
                 x->add_remove_pt = 0;
                 continue;
              }
            }
                
            if( !x->status[i] )
                continue;
                
            x->points[1][k++] = x->points[1][i];
            
            if ( x->x_delaunay == 0 ) // add all the points
            {
               cvSubdivDelaunay2DInsert( x->x_subdiv, x->points[1][i] );
               cvCalcSubdivVoronoi2D( x->x_subdiv );
            }
            // only add points included in (color-threshold)<p<(color+treshold)
            if ( ( x->x_delaunay > 0 ) && ( x->x_xmark[x->x_delaunay] != -1 ) ) 
            {
               int px = cvPointFrom32f(x->points[1][i]).x;
               int py = cvPointFrom32f(x->points[1][i]).y;
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

                   // post( "pdp_opencv_lk : point (%d,%d,%d) : diff : %d", blue, green, red, diff );

                   if ( diff < x->x_threshold )
                   {
                      cvSubdivDelaunay2DInsert( x->x_subdiv, x->points[1][i] );
                      cvCalcSubdivVoronoi2D( x->x_subdiv );
                   }
                 }
               }
            }

            cvCircle( x->image, cvPointFrom32f(x->points[1][i]), 3, CV_RGB(0,255,0), -1, 8,0);

            marked=0;
            oi=-1;
            dist=(x->x_width>x->x_height)?x->x_width:x->x_height;

            for ( im=0; im<MAX_MARKERS; im++ )
            {

              if ( x->x_xmark[im] == -1 ) continue; // i don't see the point

              odist=sqrt( pow( x->points[1][i].x - x->x_xmark[im], 2 ) + pow( x->points[1][i].y - x->x_ymark[im], 2 ) );

              // search for this point
              if ( odist <= x->x_maxmove )
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
               cvPutText( x->image, tindex, cvPointFrom32f(x->points[1][i]), &x->font, CV_RGB(255,255,255));
               x->x_xmark[oi]=x->points[1][i].x;
               x->x_ymark[oi]=x->points[1][i].y;
               x->x_found[oi]=x->x_ftolerance;
               SETFLOAT(&x->x_list[0], oi);
               SETFLOAT(&x->x_list[1], x->x_xmark[oi]);
               SETFLOAT(&x->x_list[2], x->x_ymark[oi]);
               outlet_list( x->x_outlet1, 0, 3, x->x_list );
            }

            if ( x->x_markall && !marked )
            {
              for ( im=0; im<MAX_MARKERS; im++)
              {
                if ( x->x_xmark[im] == -1 )
                {
                  x->x_xmark[im]=x->points[1][i].x;
                  x->x_ymark[im]=x->points[1][i].y;
                  x->x_found[im]=x->x_ftolerance;
                  break;
                }
              }
            }
        }
        x->count = k;
    }

    if( x->add_remove_pt && x->count < MAX_COUNT )
    {
        x->points[1][x->count++] = cvPointTo32f(x->pt);
        cvFindCornerSubPix( x->grey, x->points[1] + x->count - 1, 1,
            cvSize(x->win_size,x->win_size), cvSize(-1,-1),
            cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03));
        x->add_remove_pt = 0;
    }

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

    for ( im=0; im<MAX_MARKERS; im++ )
    {
        if ( (x->x_xmark[im] != -1.0 ) && !x->x_found[im] )
        {
           // lost the point
           x->x_xmark[im]=-1.0;
           x->x_ymark[im]=-1.0;
           SETFLOAT(&x->x_list[0], im+1);
           SETFLOAT(&x->x_list[1], x->x_xmark[im]);
           SETFLOAT(&x->x_list[2], x->x_ymark[im]);
           // send a lost point message to the patch
           outlet_list( x->x_outlet1, 0, 3, x->x_list );
           // post( "pdp_opencv_lk : lost point %d", im+1 );
        }
    }

    if ( x->x_delaunay >= 0 )
    {
       cvReleaseMemStorage( &x->x_storage ); 
    }

    CV_SWAP( x->prev_grey, x->grey, x->swap_temp );
    CV_SWAP( x->prev_pyramid, x->pyramid, x->swap_temp );
    CV_SWAP( x->points[0], x->points[1], x->swap_points );
    x->need_to_init = 0;

    memcpy( newdata, x->image->imageData, x->x_size*3 );
    return;
}


static void pdp_opencv_lk_winsize(t_pdp_opencv_lk *x, t_floatarg f)
{
  if (f>1.0) x->win_size = (int)f;
}

static void pdp_opencv_lk_nightmode(t_pdp_opencv_lk *x, t_floatarg f)
{
  if ((f==0.0)||(f==1.0)) x->night_mode = (int)f;
}

static void pdp_opencv_lk_quality(t_pdp_opencv_lk *x, t_floatarg f)
{
  if (f>0.0) x->quality = f;
}

static void pdp_opencv_lk_mindistance(t_pdp_opencv_lk *x, t_floatarg f)
{
  if (f>1.0) x->min_distance = (int)f;
}

static void pdp_opencv_lk_maxmove(t_pdp_opencv_lk *x, t_floatarg f)
{
  // has to be more than the size of a point
  if (f>=3.0) x->x_maxmove = (int)f;
}

static void pdp_opencv_lk_ftolerance(t_pdp_opencv_lk *x, t_floatarg f)
{
  if (f>0.0) x->x_ftolerance = (int)f;
}

static void pdp_opencv_lk_delaunay(t_pdp_opencv_lk *x, t_symbol *s)
{
  if (s == gensym("on")) 
     x->x_delaunay = 0;
  if (s == gensym("off")) 
     x->x_delaunay = -1;
}

static void pdp_opencv_lk_pdelaunay(t_pdp_opencv_lk *x, t_floatarg point, t_floatarg threshold)
{
  if (((int)point>0) && ((int)point<MAX_MARKERS))
  {
     x->x_delaunay = (int)point;
     x->x_threshold = (int)threshold;
  }
}

static void pdp_opencv_lk_init(t_pdp_opencv_lk *x)
{
  x->need_to_init = 1;
}

static void pdp_opencv_lk_mark(t_pdp_opencv_lk *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  int inserted;
  int px,py;

    if ( argc == 1 ) // mark all
    {
      if ( argv[0].a_type != A_SYMBOL )
      {
        error( "pdp_opencv_lk : wrong argument (should be 'all')" );
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
        pdp_opencv_lk_clear(x);
        return;
      }
    }
    else
    {
      if ( ( argv[0].a_type != A_FLOAT ) || ( argv[1].a_type != A_FLOAT ) )
      {
        error( "pdp_opencv_lk : wrong argument (should be mark px py)" );
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
              inserted = 1;
              break;
           }
        }
        if ( !inserted )
        {
           post( "pdp_opencv_lk : max markers reached" );
        }
      }
   }
}

static void pdp_opencv_lk_delete(t_pdp_opencv_lk *x, t_floatarg findex )
{
  int i;

    if ( ( findex < 1.0 ) || ( findex > MAX_MARKERS ) )
    {
       return;
    }

    x->x_xmark[(int)findex] = -1;
    x->x_ymark[(int)findex] = -1;
}

static void pdp_opencv_lk_clear(t_pdp_opencv_lk *x )
{
  int i;

    for ( i=0; i<MAX_MARKERS; i++)
    {
      x->x_xmark[i] = -1;
      x->x_ymark[i] = -1;
    }
}

static void pdp_opencv_lk_sendpacket(t_pdp_opencv_lk *x)
{
  /* release the packet */
  pdp_packet_mark_unused(x->x_packet0);
  x->x_packet0 = -1;

  /* unregister and propagate if valid dest packet */
  pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_lk_process(t_pdp_opencv_lk *x)
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
    
  /* pdp_opencv_lk_process inputs and write into active inlet */
  switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

  case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_lk_process_rgb, (void*)pdp_opencv_lk_sendpacket, &x->x_queue_id);
      break;

  default:
      /* don't know the type, so dont pdp_opencv_lk_process */
      break;
      
  }
    }

}

static void pdp_opencv_lk_input_0(t_pdp_opencv_lk *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_lk_process(x);
    }
}

static void pdp_opencv_lk_free(t_pdp_opencv_lk *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    //cv_freeplugins(x);
    
    //Destroy cv_images
    cvReleaseImage( &x->image );
    cvReleaseImage( &x->oimage );
    cvReleaseImage( &x->grey );
    cvReleaseImage( &x->prev_grey );
    cvReleaseImage( &x->pyramid );
    cvReleaseImage( &x->prev_pyramid );
}

t_class *pdp_opencv_lk_class;


void *pdp_opencv_lk_new(t_floatarg f)
{
  int i;

  t_pdp_opencv_lk *x = (t_pdp_opencv_lk *)pd_new(pdp_opencv_lk_class);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("win_size"));

  x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
  x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);

  x->x_packet0 = -1;
  x->x_packet1 = -1;
  x->x_queue_id = -1;

  x->x_width  = 320;
  x->x_height = 240;
  x->x_size   = x->x_width * x->x_height;

  x->win_size = 10;
 
  x->points [0] = 0;
  x->points [1] = 0;
  x->status = 0;
  x->count = 0;
  x->need_to_init = 1;
  x->night_mode = 0;
  x->flags = 0;
  x->add_remove_pt = 0;
  x->quality = 0.1;
  x->min_distance = 10;
  x->x_maxmove = 20;
  x->x_ftolerance = 5;
  x->x_delaunay = -1;
  x->x_threshold = -1;

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
  x->prev_grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
  x->pyramid = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
  x->prev_pyramid = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
  x->points[0] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(x->points[0][0]));
  x->points[1] = (CvPoint2D32f*)cvAlloc(MAX_COUNT*sizeof(x->points[0][0]));
  x->status = (char*)cvAlloc(MAX_COUNT);

  return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_lk_setup(void)
{

    post( "    pdp_opencv_lk");
    pdp_opencv_lk_class = class_new(gensym("pdp_opencv_lk"), (t_newmethod)pdp_opencv_lk_new,
      (t_method)pdp_opencv_lk_free, sizeof(t_pdp_opencv_lk), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_winsize, gensym("win_size"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_nightmode, gensym("nightmode"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_quality, gensym("quality"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_init, gensym("init"), A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_mark, gensym("mark"), A_GIMME, A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_delete, gensym("delete"), A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_clear, gensym("clear"), A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_mindistance, gensym("mindistance"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_maxmove, gensym("maxmove"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_ftolerance, gensym("ftolerance"),  A_FLOAT, A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_delaunay, gensym("delaunay"),  A_SYMBOL, A_NULL );   
    class_addmethod(pdp_opencv_lk_class, (t_method)pdp_opencv_lk_pdelaunay, gensym("pdelaunay"),  A_FLOAT, A_FLOAT, A_NULL );   

}

#ifdef __cplusplus
}
#endif
