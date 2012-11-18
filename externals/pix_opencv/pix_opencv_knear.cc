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

#include "pix_opencv_knear.h"

#include <stdio.h>

CPPEXTERN_NEW_WITH_TWO_ARGS(pix_opencv_knear, t_symbol *, A_DEFSYM, t_floatarg, A_DEFFLOAT )

/////////////////////////////////////////////////////////
//
// pix_opencv_knear
//
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
//
// Find the min box. The min box respect original aspect ratio image
// The image is a binary data and background is white.
//
/////////////////////////////////////////////////////////


void pix_opencv_knear :: findX(IplImage* imgSrc,int* min, int* max)
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

void pix_opencv_knear :: findY(IplImage* imgSrc,int* min, int* max)
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

/////////////////////////////////////////////////////////
//
// Find the bounding box.
//
//
/////////////////////////////////////////////////////////

CvRect pix_opencv_knear :: findBB(IplImage* imgSrc)
{
  CvRect aux;
  int xmin, xmax, ymin, ymax;
  xmin=xmax=ymin=ymax=0;

  this->findX(imgSrc, &xmin, &xmax);
  this->findY(imgSrc, &ymin, &ymax);

  aux=cvRect(xmin, ymin, xmax-xmin, ymax-ymin);

  return aux;
}

IplImage pix_opencv_knear :: preprocessing(IplImage* imgSrc,int new_width, int new_height)
{
  IplImage* result;
  IplImage* scaledResult;

  CvMat data;
  CvMat dataA;
  CvRect bb;//bounding box
  CvRect bba;//boundinb box maintain aspect ratio

  // find bounding box
  bb=this->findBB(imgSrc);

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

void pix_opencv_knear :: load_patterns(void)
{
  IplImage* src_image;
  IplImage prs_image;
  CvMat row,data;
  char file[255];
  int i=0,j;
  CvMat row_header, *row1;

  this->x_rsamples = 0;

  for( j = 0; j< this->x_nsamples; j++)
  {

     // load file
     sprintf(file,"%s/%03d.png",this->x_filepath, j);
     src_image = cvLoadImage(file,0);
     if(!src_image)
     {
       post("pix_opencv_knear : error: couldn't load image %s\n", file);
       continue;
     }
     if ( ( this->x_pwidth == -1 ) || ( this->x_pheight == -1 ) )
     {
        this->x_pwidth = src_image->width;
        this->x_pheight = src_image->height;
        // post( "pix_opencv_knear : loaded : %s (%dx%d)", file, src_image->width, src_image->height);
        this->x_rsamples++;
     }
     else if ( ( src_image->width != this->x_pwidth ) || ( src_image->height != this->x_pheight ) )
     {
        post( "pix_opencv_knear : error : %s (%dx%d) : wrong size ( should be %dx%d )", file, src_image->width, src_image->height, this->x_pwidth, this->x_pheight);
        continue;
     }
     else
     {
        // post( "pix_opencv_knear : loaded : %s (%dx%d)", file, src_image->width, src_image->height);
        this->x_rsamples++;
     }

     // process file
     prs_image = this->preprocessing(src_image, this->x_pwidth, this->x_pheight);
     // post( "pix_opencv_knear : preprocessed : %s (%dx%d)", file, this->x_pwidth, this->x_pheight);

     if ( ( this->trainData == NULL ) || ( this->trainClasses == NULL ))
     {
       this->trainData = cvCreateMat(this->x_nsamples, this->x_pwidth*this->x_pheight, CV_32FC1);
       this->trainClasses = cvCreateMat(this->x_nsamples, 1, CV_32FC1);
     }

     // set class label
     cvGetRow(this->trainClasses, &row, j);
     cvSet(&row, cvRealScalar(i), NULL);
     // set data
     cvGetRow(this->trainData, &row, j);

     IplImage* img = cvCreateImage( cvSize( this->x_pwidth, this->x_pheight ), IPL_DEPTH_32F, 1 );
     // convert 8 bits image to 32 float image
     cvConvertScale(&prs_image, img, 0.0039215, 0);

     cvGetSubRect(img, &data, cvRect( 0, 0, this->x_pwidth, this->x_pheight) );

     // convert data matrix sizexsize to vecor
     row1 = cvReshape( &data, &row_header, 0, 1 );
     cvCopy(row1, &row, NULL);
     cvReleaseImage( &img );
  }

  // create the classifier
  post( "pix_opencv_knear : loaded : %d samples from %s", this->x_rsamples, this->x_filepath);
  if ( this->x_rsamples == this->x_nsamples )
  {
    this->knn=new CvKNearest( this->trainData, this->trainClasses, 0, false, this->x_nsamples );
    this->x_nearest=cvCreateMat(1,this->x_nsamples,CV_32FC1);
    this->x_dist=cvCreateMat(1,this->x_nsamples,CV_32FC1);
  }
}


/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_knear :: pix_opencv_knear(t_symbol *path, t_floatarg nsamples)
{ 
  m_dataout = outlet_new(this->x_obj, &s_anything);

  comp_xsize  = 320;
  comp_ysize = 240;

  rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
  rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
  grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

  x_filepath = ( char * ) getbytes( 1024 );
  sprintf( x_filepath, path->s_name );
  x_nsamples = (int)nsamples;

  x_classify = 0;
  x_pwidth = -1;
  x_pheight = -1;

  trainData = NULL;
  trainClasses = NULL;

  this->load_patterns();

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_knear :: ~pix_opencv_knear()
{
    // destroy cv structures
    cvReleaseImage( &rgba );
    cvReleaseImage( &rgb );
    cvReleaseImage( &grey );
    cvReleaseMat( &trainData );
    cvReleaseMat( &trainClasses );

}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_knear :: processRGBAImage(imageStruct &image)
{
 int i;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize = image.xsize;
    this->comp_ysize = image.ysize;

    // destroy cv_images to clean memory
    cvReleaseImage( &rgba );
    cvReleaseImage( &rgb );
    cvReleaseImage( &grey );

    // create cv_images 
    this->rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    this->rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    this->grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

  }

  memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
  cvCvtColor(rgba, grey, CV_BGRA2GRAY);

  if ( this->x_classify )
  {
     IplImage prs_image;
     float result;
     CvMat row_header, *row1, odata;

       // post( "pix_opencv_knear : size : (%dx%d)", this->x_pwidth, this->x_pheight);

       // process file
       prs_image = this->preprocessing(this->grey, this->x_pwidth, this->x_pheight);

       //Set data
       IplImage* img32 = cvCreateImage( cvSize( this->x_pwidth, this->x_pheight ), IPL_DEPTH_32F, 1 );
       cvConvertScale(&prs_image, img32, 0.0039215, 0);
       cvGetSubRect(img32, &odata, cvRect(0,0, this->x_pwidth, this->x_pheight));
       row1 = cvReshape( &odata, &row_header, 0, 1 );

       result=this->knn->find_nearest(row1,this->x_nsamples,0,0,this->x_nearest,this->x_dist);
       for ( i=0; i<this->x_nsamples; i++ )
       {
         // post( "pix_opencv_knear : distance : %f", this->x_dist->data.fl[i] );
       }
       outlet_float(this->m_dataout, this->x_dist->data.fl[0]);

       cvReleaseImage( &img32 );
       this->x_classify = 0;
  }
    
  memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_knear :: processRGBImage(imageStruct &image)
{
 int i;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize = image.xsize;
    this->comp_ysize = image.ysize;

    // destroy cv_images to clean memory
    cvReleaseImage( &rgba );
    cvReleaseImage( &rgb );
    cvReleaseImage( &grey );

    // create cv_images 
    this->rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    this->rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    this->grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

  }

  memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
  cvCvtColor(rgb, grey, CV_BGR2GRAY);

  if ( this->x_classify )
  {
     IplImage prs_image;
     float result;
     CvMat row_header, *row1, odata;

       // post( "pix_opencv_knear : size : (%dx%d)", this->x_pwidth, this->x_pheight);

       // process file
       prs_image = this->preprocessing(this->grey, this->x_pwidth, this->x_pheight);

       //Set data
       IplImage* img32 = cvCreateImage( cvSize( this->x_pwidth, this->x_pheight ), IPL_DEPTH_32F, 1 );
       cvConvertScale(&prs_image, img32, 0.0039215, 0);
       cvGetSubRect(img32, &odata, cvRect(0,0, this->x_pwidth, this->x_pheight));
       row1 = cvReshape( &odata, &row_header, 0, 1 );

       result=this->knn->find_nearest(row1,this->x_nsamples,0,0,this->x_nearest,this->x_dist);
       for ( i=0; i<this->x_nsamples; i++ )
       {
         // post( "pix_opencv_knear : distance : %f", this->x_dist->data.fl[i] );
       }
       outlet_float(this->m_dataout, this->x_dist->data.fl[0]);

       cvReleaseImage( &img32 );
       this->x_classify = 0;
  }
    
  memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_knear :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_knear : yuv format not supported" );
}
    	
void pix_opencv_knear :: processGrayImage(imageStruct &image)
{ 
 int i;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize = image.xsize;
    this->comp_ysize = image.ysize;

    // destroy cv_images to clean memory
    cvReleaseImage( &rgba );
    cvReleaseImage( &rgb );
    cvReleaseImage( &grey );

    // create cv_images 
    this->rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    this->rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    this->grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

  }

  memcpy( grey->imageData, image.data, image.xsize*image.ysize*1 );

  if ( this->x_classify )
  {
     IplImage prs_image;
     float result;
     CvMat row_header, *row1, odata;

       // post( "pix_opencv_knear : size : (%dx%d)", this->x_pwidth, this->x_pheight);

       // process file
       prs_image = this->preprocessing(this->grey, this->x_pwidth, this->x_pheight);

       //Set data
       IplImage* img32 = cvCreateImage( cvSize( this->x_pwidth, this->x_pheight ), IPL_DEPTH_32F, 1 );
       cvConvertScale(&prs_image, img32, 0.0039215, 0);
       cvGetSubRect(img32, &odata, cvRect(0,0, this->x_pwidth, this->x_pheight));
       row1 = cvReshape( &odata, &row_header, 0, 1 );

       result=this->knn->find_nearest(row1,this->x_nsamples,0,0,this->x_nearest,this->x_dist);
       for ( i=0; i<this->x_nsamples; i++ )
       {
         // post( "pix_opencv_knear : distance : %f", this->x_dist->data.fl[i] );
       }
       outlet_float(this->m_dataout, this->x_dist->data.fl[0]);

       cvReleaseImage( &img32 );
       this->x_classify = 0;
  }
    
  memcpy( image.data, grey->imageData, image.xsize*image.ysize*1 );
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_knear :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_knear::bangMessCallback,
  		  gensym("bang"), A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_knear::loadMessCallback,
		  gensym("load"), A_SYMBOL, A_DEFFLOAT, A_NULL);
}

void pix_opencv_knear :: bangMessCallback(void *data)
{
   if ( GetMyClass(data)->trainData == NULL )
  {
     ::post( "pix_opencv_knear : no patterns loaded : cannot process" );
     return;
  }
  GetMyClass(data)->x_classify=1;
}

void pix_opencv_knear :: loadMessCallback(void *data, t_symbol *path, t_floatarg nsamples)
{
   if ( (int) nsamples <= 0 )
   {
      ::post( "pix_opencv_knear : wrong number of samples : %d", nsamples );
      return;
   }
   else
   {
      GetMyClass(data)->x_nsamples = (int)nsamples;
      GetMyClass(data)->x_rsamples = 0;
      cvReleaseMat( &GetMyClass(data)->trainData );
      cvReleaseMat( &GetMyClass(data)->trainClasses );
      GetMyClass(data)->trainData = NULL;
      GetMyClass(data)->trainClasses = NULL;
   }
   strcpy( GetMyClass(data)->x_filepath, path->s_name );
   GetMyClass(data)->load_patterns();
}
