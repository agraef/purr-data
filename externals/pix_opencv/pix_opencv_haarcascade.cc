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

#include "pix_opencv_haarcascade.h"
#include <stdio.h>
#include <stdlib.h>

CPPEXTERN_NEW(pix_opencv_haarcascade)

/////////////////////////////////////////////////////////
//
// pix_opencv_haarcascade
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_haarcascade :: pix_opencv_haarcascade()
{ 
 int i;

  m_numout = outlet_new(this->x_obj, 0);
  m_dataout = outlet_new(this->x_obj, 0);

  scale_factor = 1.1;
  min_neighbors = 2;
  mode = 0;
  min_size = 30;
  
  comp_xsize  = 0;
  comp_ysize  = 0;
  rgba = NULL;
  grey = NULL;
  frame = NULL;
  x_ftolerance = 5;

  for ( i=0; i<MAX_MARKERS; i++ )
  {
    x_xmark[i] = -1;
    x_ymark[i] = -1;
  }

  // initialize font
  cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );

  cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
  if( !cascade )
  {
      post( "ERROR: Could not load classifier cascade\n" );
  }
  else    post( "Loaded classifier cascade from %s", cascade_name );

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_haarcascade :: ~pix_opencv_haarcascade()
{ 
    	//Destroy cv_images to clean memory
	cvReleaseImage(&rgba);
    	cvReleaseImage(&grey);
    	cvReleaseImage(&frame);
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_haarcascade :: processRGBAImage(imageStruct &image)
{
  double scale = 1;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgba)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&rgba);
    	cvReleaseImage(&grey);
    	cvReleaseImage(&frame);

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
    	frame = cvCreateImage(cvSize(rgba->width,rgba->height), IPL_DEPTH_8U, 3);
    	grey = cvCreateImage( cvSize(rgba->width,rgba->height), 8, 1 );
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
    CvMemStorage* storage = cvCreateMemStorage(0);
    
    static CvScalar colors[] = 
    {
        {{0,0,255}},
        {{0,128,255}},
        {{0,255,255}},
        {{0,255,0}},
        {{255,128,0}},
        {{255,255,0}},
        {{255,0,0}},
        {{255,0,255}}
    };

    int i, im;

    if( cascade )
    {
        CvSeq* faces = cvHaarDetectObjects( rgba, cascade, storage,
                                            scale_factor, min_neighbors, mode, cvSize(min_size, min_size) );

        if ( faces && (faces->total > 0 ) )
            outlet_float(this->m_numout, (float)faces->total);
        else
            outlet_float(this->m_numout, 0.0);

        for ( im=0; im<MAX_MARKERS; im++ )
        {
           if ( this->x_xmark[im] != -1.0 )
           {
             this->x_found[im]--;
           }
        }

        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
          int oi, found;
         
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
            CvPoint center;
            int radius;
            center.x = cvRound((r->x + r->width*0.5)*scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);

            found = 0;
            oi = -1;
            for ( im=0; im<MAX_MARKERS; im++ )
            {
              // check if the object is already known
              if ( sqrt( pow(center.x - this->x_xmark[im], 2 ) + pow(center.y - this->x_ymark[im], 2 ) ) <= radius )
              {
                 oi=im;
                 found=1;
                 this->x_found[im] = this->x_ftolerance;
                 this->x_xmark[im] = center.x;
                 this->x_ymark[im] = center.y;
                 break;
              }
            }
            // new object detected
            if ( !found )
            {
               oi = this->mark(center.x, center.y );
            }

            char tindex[4];
            sprintf( tindex, "%d", oi );
            cvCircle( rgba, center, radius, colors[oi%8], 3, 8, 0 );
            cvPutText( rgba, tindex, center, &this->font, CV_RGB(255,255,255));

            SETFLOAT(&this->rlist[0], oi);
            SETFLOAT(&this->rlist[1], center.x);
            SETFLOAT(&this->rlist[2], center.y);
            SETFLOAT(&this->rlist[3], radius);
    	    outlet_list( m_dataout, 0, 4, this->rlist );
        }
        // delete lost objects
        for ( im=0; im<MAX_MARKERS; im++ )
        {
           if ( this->x_found[im] < 0 )
           {
             this->x_xmark[im] = -1.0;
             this->x_ymark[im] = -1,0;
             this->x_found[im] = this->x_ftolerance;
           }
        }

    }

    cvReleaseMemStorage( &storage );
    memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_haarcascade :: processRGBImage(imageStruct &image)
{
  double scale = 1;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!frame)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&rgba);
    	cvReleaseImage(&grey);
    	cvReleaseImage(&frame);

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
    	frame = cvCreateImage(cvSize(rgba->width,rgba->height), IPL_DEPTH_8U, 3);
    	grey = cvCreateImage( cvSize(rgba->width,rgba->height), 8, 1 );
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( frame->imageData, image.data, image.xsize*image.ysize*3 );
    CvMemStorage* storage = cvCreateMemStorage(0);
    
    static CvScalar colors[] = 
    {
        {{0,0,255}},
        {{0,128,255}},
        {{0,255,255}},
        {{0,255,0}},
        {{255,128,0}},
        {{255,255,0}},
        {{255,0,0}},
        {{255,0,255}}
    };

    int i, im;

    if( cascade )
    {
        CvSeq* faces = cvHaarDetectObjects( frame, cascade, storage,
                                            1.1, 2, 0, cvSize(30, 30) );

        if ( faces && (faces->total > 0 ) )
            outlet_float(this->m_numout, (float)faces->total);
        else
            outlet_float(this->m_numout, 0.0);

        for ( im=0; im<MAX_MARKERS; im++ )
        {
           if ( this->x_xmark[im] != -1.0 )
           {
             this->x_found[im]--;
           }
        }

        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
          int oi, found;
         
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
            CvPoint center;
            int radius;
            center.x = cvRound((r->x + r->width*0.5)*scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);

            found = 0;
            oi = -1;
            for ( im=0; im<MAX_MARKERS; im++ )
            {
              // check if the object is already known
              if ( sqrt( pow(center.x - this->x_xmark[im], 2 ) + pow(center.y - this->x_ymark[im], 2 ) ) <= radius )
              {
                 oi=im;
                 found=1;
                 this->x_found[im] = this->x_ftolerance;
                 this->x_xmark[im] = center.x;
                 this->x_ymark[im] = center.y;
                 break;
              }
            }
            // new object detected
            if ( !found )
            {
               oi = this->mark(center.x, center.y );
            }

            char tindex[4];
            sprintf( tindex, "%d", oi );
            cvCircle( frame, center, radius, colors[oi%8], 3, 8, 0 );
            cvPutText( frame, tindex, center, &this->font, CV_RGB(255,255,255));

            SETFLOAT(&this->rlist[0], oi);
            SETFLOAT(&this->rlist[1], center.x);
            SETFLOAT(&this->rlist[2], center.y);
            SETFLOAT(&this->rlist[3], radius);
    	    outlet_list( m_dataout, 0, 4, this->rlist );
        }
        // delete lost objects
        for ( im=0; im<MAX_MARKERS; im++ )
        {
           if ( this->x_found[im] < 0 )
           {
             this->x_xmark[im] = -1.0;
             this->x_ymark[im] = -1,0;
             this->x_found[im] = this->x_ftolerance;
           }
        }
    }
    
    cvReleaseMemStorage( &storage );
    memcpy( image.data, frame->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_haarcascade :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_haarcascade : yuv format not supported" );
}
    	
void pix_opencv_haarcascade :: processGrayImage(imageStruct &image)
{ 
    double scale = 1;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!grey)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&rgba);
    	cvReleaseImage(&grey);
    	cvReleaseImage(&frame);

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
    	frame = cvCreateImage(cvSize(rgba->width,rgba->height), IPL_DEPTH_8U, 3);
    	grey = cvCreateImage( cvSize(rgba->width,rgba->height), 8, 1 );
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( grey->imageData, image.data, image.xsize*image.ysize );
    CvMemStorage* storage = cvCreateMemStorage(0);
    
    static CvScalar colors[] = 
    {
        {{0,0,255}},
        {{0,128,255}},
        {{0,255,255}},
        {{0,255,0}},
        {{255,128,0}},
        {{255,255,0}},
        {{255,0,0}},
        {{255,0,255}}
    };

    int i;

    if( cascade )
    {
        CvSeq* faces = cvHaarDetectObjects( grey, cascade, storage,
                                            1.1, 2, 0, cvSize(30, 30) );
        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
            CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
            CvPoint center;
            int radius;
            center.x = cvRound((r->x + r->width*0.5)*scale);
            center.y = cvRound((r->y + r->height*0.5)*scale);
            radius = cvRound((r->width + r->height)*0.25*scale);
            cvCircle( grey, center, radius, colors[i%8], 3, 8, 0 );

    	    t_atom rlist[4];
            SETFLOAT(&rlist[0], i);
            SETFLOAT(&rlist[1], center.x);
            SETFLOAT(&rlist[2], center.y);
            SETFLOAT(&rlist[3], radius);
    	    outlet_list( m_dataout, 0, 4, rlist );
        }
    }

    
    cvReleaseMemStorage( &storage );
    //cvShowImage(wndname, cedge);
    memcpy( image.data, grey->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// scaleFactorMess
//
/////////////////////////////////////////////////////////
void pix_opencv_haarcascade :: scaleFactorMess (float scale_factor)
{
  this->scale_factor = scale_factor;
}

/////////////////////////////////////////////////////////
// minNeighborsMess
//
/////////////////////////////////////////////////////////
void pix_opencv_haarcascade :: minNeighborsMess (float min_neighbors)
{
  this->min_neighbors = (int)min_neighbors;
}

/////////////////////////////////////////////////////////
// modeMess
//
/////////////////////////////////////////////////////////
void pix_opencv_haarcascade :: modeMess (float mode)
{
  this->mode = !(!(int)mode);
}

/////////////////////////////////////////////////////////
// minSizeMess
//
/////////////////////////////////////////////////////////
void pix_opencv_haarcascade :: minSizeMess (float min_size)
{
  this->min_size = (int)min_size;
}

/////////////////////////////////////////////////////////
// fToleranceMess
//
/////////////////////////////////////////////////////////
void pix_opencv_haarcascade :: fToleranceMess (float ftolerance)
{
  this->x_ftolerance = (int)ftolerance;
}

/////////////////////////////////////////////////////////
// clearMess
//
/////////////////////////////////////////////////////////
void pix_opencv_haarcascade :: clearMess (void)
{
  int i;

    for ( i=0; i<MAX_MARKERS; i++)
    {
      this->x_xmark[i] = -1;
      this->x_ymark[i] = -1;
      this->x_found[i] = this->x_ftolerance;
    }
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_haarcascade :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_haarcascade::scaleFactorMessCallback,
  		  gensym("scale_factor"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_haarcascade::minNeighborsMessCallback,
  		  gensym("min_neighbors"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_haarcascade::modeMessCallback,
  		  gensym("mode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_haarcascade::minSizeMessCallback,
  		  gensym("min_size"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_haarcascade::loadCascadeMessCallback,
  		  gensym("load"), A_SYMBOL, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_haarcascade::fToleranceMessCallback,
  		  gensym("ftolerance"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_haarcascade::clearMessCallback,
  		  gensym("clear"), A_NULL);
}
void pix_opencv_haarcascade :: scaleFactorMessCallback(void *data, t_floatarg scale_factor)
{
  if (scale_factor>1) GetMyClass(data)->scaleFactorMess((float)scale_factor);
}
void pix_opencv_haarcascade :: minNeighborsMessCallback(void *data, t_floatarg min_neighbors)
{
  if (min_neighbors>=1) GetMyClass(data)->minNeighborsMess((float)min_neighbors);
}
void pix_opencv_haarcascade :: modeMessCallback(void *data, t_floatarg mode)
{
  if ((mode==0)||(mode==1)) GetMyClass(data)->modeMess((float)mode);
}
void pix_opencv_haarcascade :: minSizeMessCallback(void *data, t_floatarg min_size)
{
  if (min_size>1) GetMyClass(data)->minSizeMess((float)min_size);
}
void pix_opencv_haarcascade :: fToleranceMessCallback(void *data, t_floatarg ftolerance)
{
  if (ftolerance>1) GetMyClass(data)->fToleranceMess((float)ftolerance);
}
void pix_opencv_haarcascade :: clearMessCallback(void *data)
{
  GetMyClass(data)->clearMess();
}
void pix_opencv_haarcascade :: loadCascadeMessCallback(void *data, t_symbol* filename)
{
	    GetMyClass(data)->loadCascadeMess(filename);
}
void pix_opencv_haarcascade :: loadCascadeMess(t_symbol *filename)
{
    cascade = (CvHaarClassifierCascade*)cvLoad( filename->s_name, 0, 0, 0 );
    if( !cascade )
    {
        post( "ERROR: Could not load classifier cascade" );
    }
    else    post( "Loaded classifier cascade from %s", filename->s_name );
}
int pix_opencv_haarcascade :: mark(float fx, float fy )
{
  int i;

    if ( ( fx < 0.0 ) || ( fx > this->rgba->width ) || ( fy < 0 ) || ( fy > this->rgba->height ) )
    {
       return -1;
    }

    for ( i=0; i<MAX_MARKERS; i++)
    {
       if ( this->x_xmark[i] == -1 )
       {
          this->x_xmark[i] = (int)fx;
          this->x_ymark[i] = (int)fy;
          this->x_found[i] = this->x_ftolerance;
          return i;
       }
    }

    post( "pix_opencv_haarcascade : max markers reached" );
    return -1;
}
