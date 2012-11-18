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
 *
 *   pdp_opencv_knear : OCR like pattern recognition
 *   based on basic OCR with Open CV tutorial
 *   by damiles : http://blog.damiles.com/?p=93
 *
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

#include "cv.h"
#include "highgui.h"
#include "ml.h"

typedef struct pdp_opencv_knear_struct
{
  t_object x_obj;
  t_float x_f;

  t_outlet *x_outlet0;
  t_outlet *x_outlet1;

  int x_packet0;
  int x_packet1;
  int x_dropped;
  int x_queue_id;

  int x_width;
  int x_height;
  int x_size;

  int  x_classify;
  int  x_ROIx;
  int  x_ROIy;
  int  x_ROIw;
  int  x_ROIh;

  // The output and temporary images
  IplImage *rgb, *grey;

  // open cv classifier data
  char *x_filepath;
  int  x_nsamples;
  int  x_rsamples;
  CvMat* trainData;
  CvMat* trainClasses;
  CvMat* x_nearest;
  CvMat* x_dist;
  int x_pwidth;
  int x_pheight;
  CvKNearest *knn;

} t_pdp_opencv_knear;

IplImage pdp_opencv_knear_preprocessing(IplImage* imgSrc,int new_width, int new_height);

static void pdp_opencv_knear_process_rgb(t_pdp_opencv_knear *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
    int i,j,k,im;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

      post("pdp_opencv_knear :: resizing");
  
      x->x_width = header->info.image.width;
      x->x_height = header->info.image.height;
      x->x_size = x->x_width*x->x_height;
      x->x_ROIx = 0;
      x->x_ROIy = 0;
      x->x_ROIw = header->info.image.width;
      x->x_ROIh = header->info.image.height;

      cvReleaseImage( &x->rgb );
      cvReleaseImage( &x->grey );

      // create cv_images
      x->rgb = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
      x->grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );
    
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( x->rgb->imageData, data, x->x_size*3 );
        
    cvCvtColor( x->rgb, x->grey, CV_BGR2GRAY );

    if ( x->x_classify )
    {
     IplImage prs_image;
     float result;
     CvMat row_header, *row1, odata;

       // post( "pdp_opencv_knear : size : (%dx%d)", x->x_pwidth, x->x_pheight);

       // process file
       prs_image = pdp_opencv_knear_preprocessing(x->grey, x->x_pwidth, x->x_pheight);

       //Set data
       IplImage* img32 = cvCreateImage( cvSize( x->x_pwidth, x->x_pheight ), IPL_DEPTH_32F, 1 );
       cvConvertScale(&prs_image, img32, 0.0039215, 0);
       cvGetSubRect(img32, &odata, cvRect(0,0, x->x_pwidth, x->x_pheight));
       row1 = cvReshape( &odata, &row_header, 0, 1 );

       result=x->knn->find_nearest(row1,x->x_nsamples,0,0,x->x_nearest,x->x_dist);
       // for ( i=0; i<x->x_nsamples; i++ )
       // {
         // post( "pdp_opencv_knear : distance : %f", x->x_dist->data.fl[i] );
       // }
       outlet_float(x->x_outlet1, x->x_dist->data.fl[0]);

       cvReleaseImage( &img32 );
       x->x_classify = 0;
    }

    memcpy( newdata, x->rgb->imageData, x->x_size*3 );
    return;
}

static void pdp_opencv_knear_sendpacket(t_pdp_opencv_knear *x)
{
  /* release the packet */
  pdp_packet_mark_unused(x->x_packet0);
  x->x_packet0 = -1;

  /* unregister and propagate if valid dest packet */
  pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_knear_process(t_pdp_opencv_knear *x)
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
    
  /* pdp_opencv_knear_process inputs and write into active inlet */
  switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

   case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_knear_process_rgb, (void*)pdp_opencv_knear_sendpacket, &x->x_queue_id);
      break;

   default:
      /* don't know the type, so dont pdp_opencv_knear_process */
      break;
      
   }
  }

}

static void pdp_opencv_knear_input_0(t_pdp_opencv_knear *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_knear_process(x);
    }
}

static void pdp_opencv_knear_free(t_pdp_opencv_knear *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);

    // destroy cv_images
    cvReleaseImage( &x->rgb );
    cvReleaseImage( &x->grey );
    cvReleaseMat( &x->trainData );
    cvReleaseMat( &x->trainClasses );

}

t_class *pdp_opencv_knear_class;

//
//
// Find the min box. The min box respect original aspect ratio image
// The image is a binary data and background is white.
//
//


void pdp_opencv_knear_findX(IplImage* imgSrc,int* min, int* max)
{
  int i;
  int minFound=0;
  CvMat data;
  CvScalar maxVal=cvRealScalar(imgSrc->width * 255);
  CvScalar val=cvRealScalar(0);

  // for each col sum, if sum < width*255 then we find the min
  // then continue to end to search the max, if sum< width*255 then is new max
  for (i=0; i< imgSrc->width; i++)
  {
    cvGetCol(imgSrc, &data, i);
    val= cvSum(&data);
    if(val.val[0] < maxVal.val[0])
    {
      *max= i;
      if(!minFound)
      {
        *min= i;
         minFound= 1;
      }
    }
  }
}

void pdp_opencv_knear_findY(IplImage* imgSrc,int* min, int* max)
{
  int i;
  int minFound=0;
  CvMat data;
  CvScalar maxVal=cvRealScalar(imgSrc->width * 255);
  CvScalar val=cvRealScalar(0);

  // for each col sum, if sum < width*255 then we find the min
  // then continue to end to search the max, if sum< width*255 then is new max
  for (i=0; i< imgSrc->height; i++)
  {
    cvGetRow(imgSrc, &data, i);
    val= cvSum(&data);
    if(val.val[0] < maxVal.val[0])
    {
      *max=i;
      if(!minFound)
      {
        *min= i;
         minFound= 1;
      }
    }
  }
}

//
//
// Find the bounding box. 
//
//

CvRect pdp_opencv_knear_findBB(IplImage* imgSrc)
{
  CvRect aux;
  int xmin, xmax, ymin, ymax;
  xmin=xmax=ymin=ymax=0;

  pdp_opencv_knear_findX(imgSrc, &xmin, &xmax);
  pdp_opencv_knear_findY(imgSrc, &ymin, &ymax);

  aux=cvRect(xmin, ymin, xmax-xmin, ymax-ymin);

  return aux;
}

IplImage pdp_opencv_knear_preprocessing(IplImage* imgSrc,int new_width, int new_height)
{
  IplImage* result;
  IplImage* scaledResult;

  CvMat data;
  CvMat dataA;
  CvRect bb;//bounding box
  CvRect bba;//boundinb box maintain aspect ratio

  // find bounding box
  bb=pdp_opencv_knear_findBB(imgSrc);

  if ( ( bb.width == 0 ) || ( bb.height == 0 ) )
  {
     bb.x = 0;
     bb.y = 0;
     bb.width = imgSrc->width;
     bb.height = imgSrc->height;
  }

  // get bounding box data and no with aspect ratio, the x and y can be corrupted
  cvGetSubRect(imgSrc, &data, cvRect(bb.x, bb.y, bb.width, bb.height));
  // create image with this data with width and height with aspect ratio 1
  // then we get highest size betwen width and height of our bounding box
  int size=(bb.width>bb.height)?bb.width:bb.height;
  result=cvCreateImage( cvSize( size, size ), 8, 1 );
  cvSet(result,CV_RGB(255,255,255),NULL);
  // copy de data in center of image
  int x=(int)floor((float)(size-bb.width)/2.0f);
  int y=(int)floor((float)(size-bb.height)/2.0f);
  cvGetSubRect(result, &dataA, cvRect(x,y,bb.width, bb.height));
  cvCopy(&data, &dataA, NULL);
  // scale result
  scaledResult=cvCreateImage( cvSize( new_width, new_height ), 8, 1 );
  cvResize(result, scaledResult, CV_INTER_NN);

  // return processed data
  return *scaledResult;

}

void pdp_opencv_knear_load(t_pdp_opencv_knear *x)
{
  IplImage* src_image;
  IplImage prs_image;
  CvMat row,data;
  char file[255];
  int i=0,j;
  CvMat row_header, *row1;

  x->x_rsamples = 0;
  
  for( j = 0; j< x->x_nsamples; j++)
  {

     // load file
     sprintf(file,"%s/%03d.png",x->x_filepath, j);
     src_image = cvLoadImage(file,0);
     if(!src_image)
     {
       post("pdp_opencv_knear : error: couldn't load image %s\n", file);
       continue;
     }
     if ( ( x->x_pwidth == -1 ) || ( x->x_pheight == -1 ) )
     {
        x->x_pwidth = src_image->width;
        x->x_pheight = src_image->height;
        // post( "pdp_opencv_knear : loaded : %s (%dx%d)", file, src_image->width, src_image->height);
        x->x_rsamples++;
     }
     else if ( ( src_image->width != x->x_pwidth ) || ( src_image->height != x->x_pheight ) )
     {
        post( "pdp_opencv_knear : error : %s (%dx%d) : wrong size ( should be %dx%d )", file, src_image->width, src_image->height, x->x_pwidth, x->x_pheight);
        continue;
     }
     else
     {
        // post( "pdp_opencv_knear : loaded : %s (%dx%d)", file, src_image->width, src_image->height);
        x->x_rsamples++;
     }

     // process file
     prs_image = pdp_opencv_knear_preprocessing(src_image, x->x_pwidth, x->x_pheight);
     // post( "pdp_opencv_knear : preprocessed : %s (%dx%d)", file, x->x_pwidth, x->x_pheight);

     if ( ( x->trainData == NULL ) || ( x->trainClasses == NULL ))
     {
       x->trainData = cvCreateMat(x->x_nsamples, x->x_pwidth*x->x_pheight, CV_32FC1);
       x->trainClasses = cvCreateMat(x->x_nsamples, 1, CV_32FC1);
     }

     // set class label
     cvGetRow(x->trainClasses, &row, j);
     cvSet(&row, cvRealScalar(i), NULL);
     // set data
     cvGetRow(x->trainData, &row, j);

     IplImage* img = cvCreateImage( cvSize( x->x_pwidth, x->x_pheight ), IPL_DEPTH_32F, 1 );
     // convert 8 bits image to 32 float image
     cvConvertScale(&prs_image, img, 0.0039215, 0);

     cvGetSubRect(img, &data, cvRect( 0, 0, x->x_pwidth, x->x_pheight) );

     // convert data matrix sizexsize to vecor
     row1 = cvReshape( &data, &row_header, 0, 1 );
     cvCopy(row1, &row, NULL);
     cvReleaseImage( &img );
  }

  // create the classifier
  post( "pdp_opencv_knear : loaded : %d samples", x->x_rsamples);
  if ( x->x_rsamples == x->x_nsamples )
  {
    x->knn=new CvKNearest( x->trainData, x->trainClasses, 0, false, x->x_nsamples );
    x->x_nearest=cvCreateMat(1,x->x_nsamples,CV_32FC1);
    x->x_dist=cvCreateMat(1,x->x_nsamples,CV_32FC1);
  }
}

static void pdp_opencv_knear_classify(t_pdp_opencv_knear *x)
{
    if ( x->trainData == NULL )
    {
       post( "pdp_opencv_knear : no patterns loaded : cannot process" );
       return;
    }
    x->x_classify=1;
}

static void pdp_opencv_knear_pload(t_pdp_opencv_knear *x, t_symbol *path, t_floatarg nsamples )
{
   if ( (int) nsamples <= 0 )
   {
      post( "pdp_opencv_knear : wrong number of samples : %d", nsamples );
      return;
   }
   else
   {
      x->x_nsamples = (int)nsamples;
      x->x_rsamples = 0;
      cvReleaseMat( &x->trainData );
      cvReleaseMat( &x->trainClasses );
      x->trainData = NULL;
      x->trainClasses = NULL;
   }
   strcpy( x->x_filepath, path->s_name );
   pdp_opencv_knear_load(x);
}

void *pdp_opencv_knear_new(t_symbol *s, int argc, t_atom *argv )
{
  int i;

  t_pdp_opencv_knear *x = (t_pdp_opencv_knear *)pd_new(pdp_opencv_knear_class);

  x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
  x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);

  x->x_packet0 = -1;
  x->x_packet1 = -1;
  x->x_queue_id = -1;

  x->x_width  = 320;
  x->x_height = 240;
  x->x_size   = x->x_width * x->x_height;
  x->x_ROIx = 0;
  x->x_ROIy = 0;
  x->x_ROIw = x->x_width;
  x->x_ROIh = x->x_height;

  x->rgb = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 3 );
  x->grey = cvCreateImage( cvSize(x->x_width, x->x_height), 8, 1 );

  x->x_filepath = ( char * ) getbytes( 1024 );
  sprintf( x->x_filepath, "./data" );
  x->x_nsamples = 10;

  if ( argc >= 1 )
  {
      if ( argv[0].a_type != A_SYMBOL )
      {
        error( "pdp_opencv_knear : wrong argument (file path : 1)" );
        return NULL;
      }
      if ( !strcmp( argv[0].a_w.w_symbol->s_name, "" ) )
      {
        error( "pdp_opencv_knear : no extension specified" );
        error( "pdp_opencv_knear : usage : pdp_opencv_knear_new <file path> <nsamples>" );
        return NULL;
      }
      strcpy( x->x_filepath, argv[0].a_w.w_symbol->s_name );
  }
  if ( argc >= 2 )
  {
      if ( argv[1].a_type != A_FLOAT )
      {
        error( "pdp_opencv_knear : wrong argument (nsamples : 2)" );
        return NULL;
      }
      if ( (int)argv[1].a_w.w_float <= 0 )
      {
        error( "pdp_opencv_knear : wrong number of samples (%d)", (t_int)(int)argv[1].a_w.w_float );
        error( "pdp_opencv_knear : usage : pdp_opencv_knear_new <file path> <nsamples>" );
        return NULL;
      }
      x->x_nsamples = (int)argv[1].a_w.w_float;
  }

  x->x_classify = 0;
  x->x_pwidth = -1;
  x->x_pheight = -1;

  x->trainData = NULL;
  x->trainClasses = NULL;

  post( "pdp_opencv_knear : loading %d samples from : %s", x->x_nsamples, x->x_filepath );
  pdp_opencv_knear_load( x );

  return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_knear_setup(void)
{

    post( "    pdp_opencv_knear");
    pdp_opencv_knear_class = class_new(gensym("pdp_opencv_knear"), (t_newmethod)pdp_opencv_knear_new,
      (t_method)pdp_opencv_knear_free, sizeof(t_pdp_opencv_knear), CLASS_DEFAULT, A_GIMME, A_NULL);

    class_addmethod(pdp_opencv_knear_class, (t_method)pdp_opencv_knear_input_0, gensym("pdp"), A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_opencv_knear_class, (t_method)pdp_opencv_knear_classify, gensym("bang"), A_NULL);
    class_addmethod(pdp_opencv_knear_class, (t_method)pdp_opencv_knear_pload, gensym("load"), A_SYMBOL, A_DEFFLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
