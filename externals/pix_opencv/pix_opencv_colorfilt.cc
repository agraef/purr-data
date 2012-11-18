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

#include "pix_opencv_colorfilt.h"
#include "g_canvas.h"

CPPEXTERN_NEW(pix_opencv_colorfilt)

/////////////////////////////////////////////////////////
//
// pix_opencv_colorfilt
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_colorfilt :: pix_opencv_colorfilt()
{ 
  inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float, gensym("R"));
  inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float, gensym("G"));
  inlet_new(this->x_obj, &this->x_obj->ob_pd, &s_float, gensym("B"));

  x_R  = outlet_new(this->x_obj, &s_float);
  x_G  = outlet_new(this->x_obj, &s_float);
  x_B  = outlet_new(this->x_obj, &s_float);

  x_colorR = 128;
  x_colorG = 128;
  x_colorB = 128;

  outlet_float( x_R, x_colorR );
  outlet_float( x_G, x_colorG );
  outlet_float( x_B, x_colorB );

  comp_xsize=320;
  comp_ysize=240;

  x_tolerance = 50;

  x_canvas = canvas_getcurrent();

  rgba = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
  rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
  brgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 4);
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_colorfilt :: ~pix_opencv_colorfilt()
{ 
   //Destroy cv_images to clean memory
   cvReleaseImage(&rgba);
   cvReleaseImage(&rgb);
   cvReleaseImage(&brgb);
}

void pix_opencv_colorfilt :: drawColor()
{
 int width, height;
 char color[32];

    sprintf( color, "#%.2X%.2X%.2X", x_colorR, x_colorG, x_colorB );
    width = rtext_width( glist_findrtext( (t_glist*)x_canvas, (t_text *)this->x_obj ) );
    height = rtext_height( glist_findrtext( (t_glist*)x_canvas, (t_text *)this->x_obj ) );
    sys_vgui((char*)".x%x.c delete rectangle %xCOLOR\n", x_canvas, this->x_obj );
    sys_vgui((char*)".x%x.c create rectangle %d %d %d %d -fill %s -tags %xCOLOR\n",
             x_canvas, this->x_obj->te_xpix+width+5, this->x_obj->te_ypix,
             this->x_obj->te_xpix+width+height+5,
             this->x_obj->te_ypix+height, color, this->x_obj );
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_colorfilt :: processRGBAImage(imageStruct &image)
{
  int px,py;
  unsigned char r,g,b;
  int diff; 

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgba)) {

	comp_xsize = image.xsize;
	comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( rgba )
        {
	  cvReleaseImage(&rgba);
    	  cvReleaseImage(&rgb);
    	  cvReleaseImage(&brgb);
        }

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	brgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
    }
    memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
    cvCopy(rgba, brgb);

    for( py=0; py<rgba->height; py++ ) {
      for( px=0; px<rgba->width; px++ ) {
#ifdef __APPLE__
        g = ((uchar*)(rgba->imageData + rgba->widthStep*(int)py))[(int)px*4];
        r = ((uchar*)(rgba->imageData + rgba->widthStep*(int)py))[(int)px*4+1];
        b = ((uchar*)(rgba->imageData + rgba->widthStep*(int)py))[(int)px*4+3];
#else 
        r = ((uchar*)(rgba->imageData + rgba->widthStep*(int)py))[(int)px*4];
        g = ((uchar*)(rgba->imageData + rgba->widthStep*(int)py))[(int)px*4+1];
        b = ((uchar*)(rgba->imageData + rgba->widthStep*(int)py))[(int)px*4+2];
#endif

        diff = 0;
        diff = abs(r-x_colorR );
        diff += abs(g-x_colorG );
        diff += abs(b-x_colorB );
        diff = diff/3;

        if ( diff > x_tolerance )
        {
           (rgba->imageData + rgba->widthStep*(int)py)[(int)px*4] = 0x0;
           (rgba->imageData + rgba->widthStep*(int)py)[(int)px*4+1] = 0x0;
           (rgba->imageData + rgba->widthStep*(int)py)[(int)px*4+2] = 0x0;
           (rgba->imageData + rgba->widthStep*(int)py)[(int)px*4+3] = 0x0;
        }
      }
    }
  
    //copy back the processed frame to image
    memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_colorfilt :: processRGBImage(imageStruct &image)
{
  unsigned char r,g,b;
  int diff; 
  int px, py;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
        if ( rgb )
        {
	  cvReleaseImage(&rgba);
    	  cvReleaseImage(&rgb);
    	  cvReleaseImage(&brgb);
        }

	//create the orig image with new size
        rgba = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);
    	brgb = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 4);
    
    }
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    cvCopy(rgb, brgb);
    
    for( py=0; py<rgb->height; py++ ) {
      for( px=0; px<rgb->width; px++ ) {
        b = ((uchar*)(rgb->imageData + rgb->widthStep*(int)py))[(int)px*3];
        g = ((uchar*)(rgb->imageData + rgb->widthStep*(int)py))[(int)px*3+1];
        r = ((uchar*)(rgb->imageData + rgb->widthStep*(int)py))[(int)px*3+2];

        diff = 0;
        diff = abs(r-x_colorR );
        diff += abs(g-x_colorG );
        diff += abs(b-x_colorB );

        if ( diff > x_tolerance )
        {
           (rgb->imageData + rgb->widthStep*(int)py)[(int)px*3] = 0x0;
           (rgb->imageData + rgb->widthStep*(int)py)[(int)px*3+1] = 0x0;
           (rgb->imageData + rgb->widthStep*(int)py)[(int)px*3+2] = 0x0;
        }
      }
    }
  
    memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_colorfilt :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_colorfilt : yuv format not supported" );
}
    	
void pix_opencv_colorfilt :: processGrayImage(imageStruct &image)
{ 
  post( "pix_opencv_colorfilt : gray format not supported" );
}

void pix_opencv_colorfilt :: floatToleranceMess (float tolerance)
{
  if ( (int)tolerance>0 ) x_tolerance = (int)tolerance;
}

void pix_opencv_colorfilt :: floatRMess (float r)
{
  if ( ( (int)r>=0 ) && ( (int)r<=255 ) ) x_colorR = (int)r;
  if (glist_isvisible(x_canvas)) drawColor();
}

void pix_opencv_colorfilt :: floatGMess (float g)
{
  if ( ( (int)g>=0 ) && ( (int)g<=255 ) ) x_colorG = (int)g;
  if (glist_isvisible(x_canvas)) drawColor();
}

void pix_opencv_colorfilt :: floatBMess (float b)
{
  if ( ( (int)b>=0 ) && ( (int)b<=255 ) ) x_colorB = (int)b;
  if (glist_isvisible(x_canvas)) drawColor();
}

void pix_opencv_colorfilt :: pickMess (float xcur, float ycur)
{
   if ( ( xcur >= 0. ) && ( xcur <= comp_xsize )
        && ( ycur > 0. ) && ( ycur < comp_ysize ) )
   {
#ifdef __APPLE__
      x_colorR = ((uchar*)(brgb->imageData + brgb->widthStep*(int)ycur))[(int)xcur*4+1];
      x_colorG = ((uchar*)(brgb->imageData + brgb->widthStep*(int)ycur))[(int)xcur*4+2];
      x_colorB = ((uchar*)(brgb->imageData + brgb->widthStep*(int)ycur))[(int)xcur*4+3];
#else
      //ycur = brgb->height - ycur;
      x_colorR = ((uchar*)(brgb->imageData + brgb->widthStep*(int)ycur))[(int)xcur*4];
      x_colorG = ((uchar*)(brgb->imageData + brgb->widthStep*(int)ycur))[(int)xcur*4+1];
      x_colorB = ((uchar*)(brgb->imageData + brgb->widthStep*(int)ycur))[(int)xcur*4+2];
#endif
      outlet_float( x_R, x_colorR );
      outlet_float( x_G, x_colorG );
      outlet_float( x_B, x_colorB );

      if (glist_isvisible(x_canvas)) drawColor();
   }
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_colorfilt :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_colorfilt::floatToleranceMessCallback,
  		  gensym("tolerance"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_colorfilt::floatRMessCallback,
  		  gensym("R"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_colorfilt::floatGMessCallback,
  		  gensym("G"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_colorfilt::floatBMessCallback,
  		  gensym("B"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_colorfilt::pickMessCallback,
  		  gensym("pick"), A_FLOAT, A_FLOAT, A_NULL);
}

void pix_opencv_colorfilt :: floatToleranceMessCallback(void *data, t_floatarg tolerance)
{
  GetMyClass(data)->floatToleranceMess((float)tolerance);
}

void pix_opencv_colorfilt :: floatRMessCallback(void *data, t_floatarg r)
{
  GetMyClass(data)->floatRMess((float)r);
}

void pix_opencv_colorfilt :: floatGMessCallback(void *data, t_floatarg g)
{
  GetMyClass(data)->floatGMess((float)g);
}

void pix_opencv_colorfilt :: floatBMessCallback(void *data, t_floatarg b)
{
  GetMyClass(data)->floatBMess((float)b);
}

void pix_opencv_colorfilt :: pickMessCallback(void *data, t_floatarg xcur, t_floatarg ycur)
{
  GetMyClass(data)->pickMess((float)xcur,(float)ycur);
}
