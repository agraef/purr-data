/*
 *   PiDiP module.
 *   Copyright (c) by Yves Degoyon (ydegoyon@free.fr)
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

/*  This object recognize all blobs of a given (y,u,v) color
 *  Written by Yves Degoyon                                  
 */

#include "pdp.h"
#include "yuv.h"
#include <math.h>
#include "g_canvas.h"
#ifndef _EiC
#include "opencv/cv.h"
#endif

static char   *pdp_shapes_version = "pdp_shapes: version 0.1, color blobs detection (yuv colorspace) object written by Yves Degoyon (ydegoyon@free.fr)";

typedef struct pdp_shapes_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_outlet1; // output color blob data
    t_outlet *x_outletY; // output Y component
    t_outlet *x_outletU; // output U component
    t_outlet *x_outletV; // output V component

    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_vwidth;
    int x_vheight;
    int x_vsize;

    int x_cursX;
    int x_cursY;
    
    float x_meanX;
    float x_meanY;
    int x_nbpts;
    
    int x_colorY;
    int x_colorU;
    int x_colorV;
    int x_colorR;
    int x_colorG;
    int x_colorB;

    t_canvas *x_canvas;

    int x_minsize; // minimum size of a blob ( default 10x10 )
    int x_maxsize; // maximum size of a blob ( default 320x240 )

    int x_bcounter; // blobs counter
    t_atom  plist[7]; // list of blobs data

    int x_tolerance;  // tolerance
    int x_isolate;    // isolate option
    int x_shape;      // drawing shape option
    int x_luminosity; // use luminosity compoenent or not (Y)

    short int *x_bdata;
    char *x_bbdata;
    char *x_checked;

    int    x_vx1; // x1 coordinate of blob
    int    x_vx2; // x1 coordinate of blob
    int    x_vy1; // x1 coordinate of blob
    int    x_vy2; // x1 coordinate of blob

    IplImage *image;
    CvFont font;

} t_pdp_shapes;

static void pdp_shapes_draw_color(t_pdp_shapes *x)
{
 int width, height;
 char color[32];

    sprintf( color, "#%.2X%.2X%.2X", x->x_colorR, x->x_colorG, x->x_colorB );
    width = rtext_width( glist_findrtext( (t_glist*)x->x_canvas, (t_text *)x ) );
    height = rtext_height( glist_findrtext( (t_glist*)x->x_canvas, (t_text *)x ) );
    sys_vgui(".x%x.c delete rectangle %xCOLOR\n", x->x_canvas, x );
    sys_vgui(".x%x.c create rectangle %d %d %d %d -fill %s -tags %xCOLOR\n",
             x->x_canvas, x->x_obj.te_xpix+width+5, x->x_obj.te_ypix,
             x->x_obj.te_xpix+width+height+5,
             x->x_obj.te_ypix+height, color, x );
}

static void pdp_shapes_allocate(t_pdp_shapes *x)
{
 int i;

  if ( x->x_bdata ) free( x->x_bdata );
  if ( x->x_bbdata ) free( x->x_bbdata );
  if ( x->x_checked ) free( x->x_checked );
  if ( x->image ) cvReleaseImage(&x->image);

  x->x_bdata = (short int *)malloc((( x->x_vsize + (x->x_vsize>>1))<<1));
  x->x_bbdata = (char *)malloc( x->x_vsize * 3 * sizeof(char) );
  x->x_checked = (char *)malloc( x->x_vsize );
  x->image = cvCreateImage(cvSize(x->x_vwidth,x->x_vheight), IPL_DEPTH_8U, 3);
}

static void pdp_shapes_tolerance(t_pdp_shapes *x, t_floatarg ftolerance )
{
   if ( ftolerance >= 0. )
   {
      x->x_tolerance = (int)ftolerance;
   }
}

static void pdp_shapes_isolate(t_pdp_shapes *x, t_floatarg fisolate )
{
   if ( ( fisolate == 0. ) || ( fisolate == 1. ) )
   {
      x->x_isolate = (int)fisolate;
   }
}

static void pdp_shapes_luminosity(t_pdp_shapes *x, t_floatarg fluminosity )
{
   if ( ( fluminosity == 0. ) || ( fluminosity == 1. ) )
   {
      x->x_luminosity = (int)fluminosity;
   }
}

static void pdp_shapes_shape(t_pdp_shapes *x, t_floatarg fshape )
{
   if ( ( (int)fshape == 0 ) || ( (int)fshape == 1 ) )
   {
      x->x_shape = (int)fshape;
   }
}

static void pdp_shapes_pick(t_pdp_shapes *x, t_floatarg X, t_floatarg Y)
{
 int u,v;

   x->x_cursX = (int) (X*(t_float)x->x_vwidth);
   x->x_cursY = (int) (Y*(t_float)x->x_vheight);
   // post( "pdp_shape : pick color at : %d,%d", x->x_cursX, x->x_cursY );
   if ( ( x->x_cursX >= 0 ) && ( x->x_cursX < x->x_vwidth )
        && ( x->x_cursY >= 0 ) && ( x->x_cursY < x->x_vheight ) )
   {
      x->x_colorY = (*(x->x_bdata + x->x_cursY*x->x_vwidth + x->x_cursX));
      x->x_colorV = (*(x->x_bdata + x->x_vsize + (x->x_cursY>>1)*(x->x_vwidth>>1) + (x->x_cursX>>1)));
      x->x_colorU = (*(x->x_bdata + x->x_vsize + (x->x_vsize>>2) + (x->x_cursY>>1)*(x->x_vwidth>>1) + (x->x_cursX>>1)));
      x->x_colorY = (x->x_colorY>>7);
      x->x_colorV = (x->x_colorV>>8)+128;
      x->x_colorU = (x->x_colorU>>8)+128;
      outlet_float( x->x_outletY, x->x_colorY );
      outlet_float( x->x_outletU, x->x_colorU );
      outlet_float( x->x_outletV, x->x_colorV );
      x->x_colorR = yuv_YUVtoR( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorG = yuv_YUVtoG( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorB = yuv_YUVtoB( x->x_colorY, x->x_colorU, x->x_colorV );
      if (glist_isvisible(x->x_canvas)) pdp_shapes_draw_color( x );
      post( "pdp_shape : picked color : %d,%d", x->x_colorU, x->x_colorV );
   }
}

static void pdp_shapes_do_detect(t_pdp_shapes *x, t_floatarg X, t_floatarg Y);
static void pdp_shapes_frame_detect(t_pdp_shapes *x);

static void pdp_shapes_y(t_pdp_shapes *x, t_floatarg fy )
{
   if ( fy <= 255. && fy >= 0. )
   {
      x->x_colorY = (int)fy;
      outlet_float( x->x_outletY, x->x_colorY );
      outlet_float( x->x_outletU, x->x_colorU );
      outlet_float( x->x_outletV, x->x_colorV );
      x->x_colorR = yuv_YUVtoR( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorG = yuv_YUVtoG( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorB = yuv_YUVtoB( x->x_colorY, x->x_colorU, x->x_colorV );
      if (glist_isvisible(x->x_canvas)) pdp_shapes_draw_color( x );
   }
}

static void pdp_shapes_u(t_pdp_shapes *x, t_floatarg fu )
{
   if ( fu <= 255. && fu >= 0. )
   {
      x->x_colorU = (int)fu;
      outlet_float( x->x_outletY, x->x_colorY );
      outlet_float( x->x_outletU, x->x_colorU );
      outlet_float( x->x_outletV, x->x_colorV );
      x->x_colorR = yuv_YUVtoR( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorG = yuv_YUVtoG( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorB = yuv_YUVtoB( x->x_colorY, x->x_colorU, x->x_colorV );
      if (glist_isvisible(x->x_canvas)) pdp_shapes_draw_color( x );
   }
}

static void pdp_shapes_v(t_pdp_shapes *x, t_floatarg fv )
{
   if ( fv <= 255. && fv >= 0. )
   {
      x->x_colorV = (int)fv;
      outlet_float( x->x_outletY, x->x_colorY );
      outlet_float( x->x_outletU, x->x_colorU );
      outlet_float( x->x_outletV, x->x_colorV );
      x->x_colorR = yuv_YUVtoR( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorG = yuv_YUVtoG( x->x_colorY, x->x_colorU, x->x_colorV );
      x->x_colorB = yuv_YUVtoB( x->x_colorY, x->x_colorU, x->x_colorV );
      if (glist_isvisible(x->x_canvas)) pdp_shapes_draw_color( x );
   }
}

static void pdp_shapes_minsize(t_pdp_shapes *x, t_floatarg fs )
{
   if ( fs <= 0. || fs >= x->x_vsize || fs > x->x_maxsize )
   {
      post( "pdp_shapes : wrong minimal size: %d", (int)fs );
   }
   x->x_minsize=(int)fs;
}

static void pdp_shapes_maxsize(t_pdp_shapes *x, t_floatarg fs )
{
   if ( fs <= 0. || fs >= x->x_vsize || fs < x->x_minsize )
   {
      post( "pdp_shapes : wrong maximal size: %d", (int)fs );
   }
   x->x_maxsize=(int)fs;
}

static int pdp_shapes_check_point(t_pdp_shapes *x, int nX, int nY)
{
 short int  *pbY, *pbU, *pbV;
 short int  y, v, u;
 int      diff;

  if ( ( nX < 0 ) || ( nX >= x->x_vwidth ) || 
       ( nY < 0 ) || ( nY >= x->x_vheight ) )
  {
    return 0;
  }

  pbY = x->x_bdata;
  pbV = (x->x_bdata+x->x_vsize);
  pbU = (x->x_bdata+x->x_vsize+(x->x_vsize>>2));

  y = (*(pbY+nY*x->x_vwidth+nX))>>7;
  v = (*(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1))>>8)+128;
  u = (*(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1))>>8)+128;
  diff = abs(u-x->x_colorU)+abs(v-x->x_colorV);
  if ( x->x_luminosity )
  {
    diff += abs(y-x->x_colorY);
  }

  if ( diff <= x->x_tolerance )
  {
    return 1;
  }
  return 0;
}

static void pdp_shapes_propagate(t_pdp_shapes *x, int nX, int nY)
{

  if ( ( nX >= 0 ) && ( nX < x->x_vwidth ) && 
       ( nY >= 0 ) && ( nY < x->x_vheight ) &&
       ( !*(x->x_checked + nY*x->x_vwidth + nX) )  
     )
  {
    pdp_shapes_do_detect( x, nX, nY );
  }
}

static void pdp_shapes_do_detect(t_pdp_shapes *x, t_floatarg X, t_floatarg Y)
{
 short int  *pbY, *pbU, *pbV;
 char  *pbbRGB;
 short int  nX, nY, y, v, u;
 short int *data;
 int      diff, px, py, inc, maxXY;

  pbY = x->x_bdata;
  pbU = (x->x_bdata+x->x_vsize);
  pbV = (x->x_bdata+x->x_vsize+(x->x_vsize>>2));
  pbbRGB = x->x_bbdata;

  if ( ( (int)X < 0 ) || ( (int)X >= x->x_vwidth ) || 
       ( (int)Y < 0 ) || ( (int)Y >= x->x_vheight ) )
  {
     return;
  }

  nX = (int) X; 
  nY = (int) Y; 
  *(x->x_checked + nY*x->x_vwidth + nX) = 1;

  y = (*(pbY+nY*x->x_vwidth+nX))>>7;
  u = (*(pbV+(nY>>1)*(x->x_vwidth>>1)+(nX>>1))>>8)+128;
  v = (*(pbU+(nY>>1)*(x->x_vwidth>>1)+(nX>>1))>>8)+128;
  diff = abs(u-x->x_colorU)+abs(v-x->x_colorV);
  if ( x->x_luminosity )
  {
    diff += abs(y-x->x_colorY);
  }

  if ( diff > x->x_tolerance )
  {
     // paint an ellipse and the center with opencv
     // post( "pdp_shapes_do_detect : paint : %d %d", nX, nY );
     if ( x->x_shape )
     {
       *(pbbRGB+3*(nY*x->x_vwidth+nX)) = 255;
       *(pbbRGB+3*(nY*x->x_vwidth+nX)+1) = 255;
       *(pbbRGB+3*(nY*x->x_vwidth+nX)+2) = 255;
     }

     if ( ( nX < x->x_vx1 ) || ( x->x_vx1 == -1 ) )
     {
        x->x_vx1 = nX;
     }
     if ( ( nX > x->x_vx2 ) || ( x->x_vx2 == -1 ) )
     {
        x->x_vx2 = nX;
     }
     if ( ( nY < x->x_vy1 ) || ( x->x_vy1 == -1 ) )
     {
        x->x_vy1 = nY;
     }
     if ( ( nY > x->x_vy2 ) || ( x->x_vy2 == -1 ) )
     {
        x->x_vy2 = nY;
     }

     return;
  }
  else
  {
     if ( x->x_isolate )
     {
       *(pbbRGB+3*(nY*x->x_vwidth+nX)) = yuv_YUVtoR(y,u,v);
       *(pbbRGB+3*(nY*x->x_vwidth+nX)+1) = yuv_YUVtoG(y,u,v);
       *(pbbRGB+3*(nY*x->x_vwidth+nX)+2) = yuv_YUVtoB(y,u,v);
     }
     x->x_meanX = ( x->x_nbpts*x->x_meanX + nX )/(x->x_nbpts+1);
     x->x_meanY = ( x->x_nbpts*x->x_meanY + nY )/(x->x_nbpts+1);
     x->x_nbpts++;
  }

  nX = (int) X+1; 
  nY = (int) Y; 
  pdp_shapes_propagate(x, nX, nY);
    
  nX = (int) X-1; 
  nY = (int) Y; 
  pdp_shapes_propagate(x, nX, nY);

  nX = (int) X-1; 
  nY = (int) Y-1; 
  pdp_shapes_propagate(x, nX, nY);

  nX = (int) X; 
  nY = (int) Y-1; 
  pdp_shapes_propagate(x, nX, nY);

  nX = (int) X+1; 
  nY = (int) Y-1; 
  pdp_shapes_propagate(x, nX, nY);

  nX = (int) X-1; 
  nY = (int) Y+1; 
  pdp_shapes_propagate(x, nX, nY);

  nX = (int) X; 
  nY = (int) Y+1; 
  pdp_shapes_propagate(x, nX, nY);

  nX = (int) X+1; 
  nY = (int) Y+1; 
  pdp_shapes_propagate(x, nX, nY);

}

static void pdp_shapes_frame_detect(t_pdp_shapes *x)
{
  char tindex[4];

   if ( x->x_bdata == NULL ) return;

   // post( "pdp_shapes : detect %d %d", (int)x->x_cursX, (int)x->x_cursY );
   x->x_vx1 = -1; 
   x->x_vx2 = -1; 
   x->x_vy1 = -1; 
   x->x_vy2 = -1; 
   x->x_nbpts = 0;
   x->x_meanX = 0;
   x->x_meanY = 0;
   pdp_shapes_do_detect( x, x->x_cursX, x->x_cursY );

   if ( ( x->x_nbpts > x->x_minsize ) && ( x->x_nbpts <= x->x_maxsize ) )
   {
     CvBox2D trackbox;
     CvPoint pt1, pt2;
     memcpy( x->image->imageData, x->x_bbdata, x->x_vsize*3 );
     trackbox.center.x = (int)x->x_meanX;
     trackbox.center.y = (int)x->x_meanY;
     trackbox.size.width = abs(x->x_vx2-x->x_vx1);
     trackbox.size.height = abs(x->x_vy2-x->x_vy1);
     if ( (x->x_meanX-(x->x_vx2+x->x_vx1/2)) != 0 )
     {
       trackbox.angle = atan( (x->x_meanY-(x->x_vy2+x->x_vy1/2))/(x->x_meanX-(x->x_vx2+x->x_vx1/2)) );
     }
     else
     { 
       trackbox.angle = 0;
     }
     cvEllipseBox( x->image, trackbox, CV_RGB(255,0,0), 3, CV_AA, 0 );

     pt1.x = (int)x->x_meanX-5;
     pt1.y = (int)x->x_meanY-5;
     pt2.x = (int)x->x_meanX+5;
     pt2.y = (int)x->x_meanY+5;
     cvLine( x->image, pt1, pt2, CV_RGB(0,255,0), 3, 8, 0 );

     pt1.x = (int)x->x_meanX-5;
     pt1.y = (int)x->x_meanY+5;
     pt2.x = (int)x->x_meanX+5;
     pt2.y = (int)x->x_meanY-5;
     cvLine( x->image, pt1, pt2, CV_RGB(0,255,0), 3, 8, 0 );

     sprintf( tindex, "%d", x->x_bcounter );
     cvPutText( x->image, tindex, cvPoint(x->x_vx2,x->x_vy2), &x->font, CV_RGB(0,255,0));

     memcpy( x->x_bbdata, x->image->imageData, x->x_vsize*3 );

     SETFLOAT(&x->plist[0], x->x_bcounter++);
     SETFLOAT(&x->plist[1], x->x_nbpts ); // area
     SETFLOAT(&x->plist[2], (int)x->x_meanX ); // center X
     SETFLOAT(&x->plist[3], (int)x->x_meanY ); // center Y
     SETFLOAT(&x->plist[4], abs(x->x_vx2-x->x_vx1) ); // width
     SETFLOAT(&x->plist[5], abs(x->x_vy2-x->x_vy1) ); // height
     SETFLOAT(&x->plist[6], trackbox.angle*180/CV_PI ); // angle in degrees
     outlet_list( x->x_outlet1, 0, 7, x->plist );
   }
}

static void pdp_shapes_process_yv12(t_pdp_shapes *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    char      *newdata = (char *)pdp_packet_data(x->x_packet1);
    short int  *pfY, *pfU, *pfV;
    int  px, py, y, u, v, diff;

    /* allocate all ressources */
    if ( ((int)header->info.image.width != x->x_vwidth ) ||
         ((int)header->info.image.height != x->x_vheight ) ) 
    {
        x->x_vwidth = header->info.image.width;
        x->x_vheight = header->info.image.height;
        x->x_vsize = x->x_vwidth*x->x_vheight;
        pdp_shapes_allocate(x);
        post( "pdp_shapes : reallocating buffers" );
    }

    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_vwidth;
    newheader->info.image.height = x->x_vheight;

    memcpy( x->x_bdata, data, (x->x_vsize+(x->x_vsize>>1))<<1 );
    if ( !x->x_isolate )
    {
      memcpy( x->x_bbdata, newdata, x->x_vsize*3 );
    }
    else
    {
      memset( x->x_bbdata, 0x0, x->x_vsize*3 );
    }

    pfY = data;
    pfV = data+x->x_vsize;
    pfU = data+x->x_vsize+(x->x_vsize>>2);

    memset( x->x_checked, 0x0, x->x_vsize );
    x->x_bcounter = 0;

    for ( py=0; py<x->x_vheight; py++ )
    {
      for ( px=0; px<x->x_vwidth; px++ )
      {
         if ( *(x->x_checked + py*x->x_vwidth + px) ) continue;
  
         y = (*(pfY+py*x->x_vwidth+px))>>7;
         v = (*(pfV+(py>>1)*(x->x_vwidth>>1)+(px>>1))>>8)+128;
         u = (*(pfU+(py>>1)*(x->x_vwidth>>1)+(px>>1))>>8)+128;

         diff = abs(v-x->x_colorV );
         diff += abs(u-x->x_colorU );

         if ( x->x_luminosity )
         {
           diff += abs(y-x->x_colorY );
         }

         if ( diff <= x->x_tolerance )
         {
            x->x_cursX = px;
            x->x_cursY = py;
            pdp_shapes_frame_detect(x);
         }
       }
    }
  
    memcpy( newdata, x->x_bbdata, x->x_vsize*3 );

    return;
}

static void pdp_shapes_sendpacket(t_pdp_shapes *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_shapes_process(t_pdp_shapes *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_IMAGE == header->type))
   {
	/* pdp_shapes_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_IMAGE_YV12:
            x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet1, (int)x->x_packet0, pdp_gensym("bitmap/rgb/*") );
            pdp_queue_add(x, pdp_shapes_process_yv12, pdp_shapes_sendpacket, &x->x_queue_id);
	    break;

	case PDP_IMAGE_GREY:
	    // pdp_shapes_process_packet(x);
	    break;

	default:
	    /* don't know the type, so dont pdp_shapes_process */
	    break;
	    
	}
    }
}

static void pdp_shapes_input_0(t_pdp_shapes *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("image/YCrCb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_shapes_process(x);
    }
}

static void pdp_shapes_free(t_pdp_shapes *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);

}

t_class *pdp_shapes_class;

void *pdp_shapes_new(void)
{
    int i;

    t_pdp_shapes *x = (t_pdp_shapes *)pd_new(pdp_shapes_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("Y"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("U"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("V"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("minsize"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("maxsize"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);
    x->x_outletY = outlet_new(&x->x_obj, &s_float);
    x->x_outletU = outlet_new(&x->x_obj, &s_float);
    x->x_outletV = outlet_new(&x->x_obj, &s_float);

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_vwidth = -1;
    x->x_vheight = -1;
    x->x_vsize = -1;

    x->x_tolerance = 40;
    x->x_isolate = 1;
    x->x_shape = 1;
    x->x_luminosity = 0;

    x->x_cursX = -1;
    x->x_cursY = -1;

    x->x_colorY = 255;
    x->x_colorU = 0;
    x->x_colorV = 0;

    x->x_minsize=10*10;
    x->x_maxsize=320*240;

    x->x_bcounter = 0;

    x->x_bdata = NULL;
    x->image = NULL;
    x->x_bbdata = NULL;
    x->x_checked = NULL;

    x->x_canvas = canvas_getcurrent();

    // initialize font
    cvInitFont( &x->font, CV_FONT_HERSHEY_PLAIN, 0.5, 0.5, 0, 1, 8 );

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_shapes_setup(void)
{
//    post( pdp_shapes_version );
    pdp_shapes_class = class_new(gensym("pdp_shapes"), (t_newmethod)pdp_shapes_new,
    	(t_method)pdp_shapes_free, sizeof(t_pdp_shapes), 0, A_NULL);

    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_tolerance, gensym("tolerance"), A_FLOAT, A_NULL);
    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_isolate, gensym("isolate"), A_FLOAT, A_NULL);
    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_luminosity, gensym("luminosity"), A_FLOAT, A_NULL);
    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_shape, gensym("shape"), A_FLOAT, A_NULL);
    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_y, gensym("Y"), A_FLOAT, A_NULL);
    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_u, gensym("U"), A_FLOAT, A_NULL);
    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_v, gensym("V"), A_FLOAT, A_NULL);
    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_minsize, gensym("minsize"), A_FLOAT, A_NULL);
    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_maxsize, gensym("maxsize"), A_FLOAT, A_NULL);
    class_addmethod(pdp_shapes_class, (t_method)pdp_shapes_pick, gensym("pick"), A_DEFFLOAT, A_DEFFLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
