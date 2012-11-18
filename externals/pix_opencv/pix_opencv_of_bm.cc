
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

#include "pix_opencv_of_bm.h"
#include <stdio.h>

CPPEXTERN_NEW(pix_opencv_of_bm)

/////////////////////////////////////////////////////////
//
// pix_opencv_of_bm
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////

pix_opencv_of_bm :: pix_opencv_of_bm()
{ 
  comp_xsize=320;
  comp_ysize=240;

  m_meanout = outlet_new(this->x_obj, &s_anything);
  m_maxout = outlet_new(this->x_obj, &s_anything);

  x_nightmode=0;
  x_threshold=10;
  x_blocksize.width = 20;
  x_blocksize.height = 20;
  x_shiftsize.width = 20;
  x_shiftsize.height = 20;
  x_maxrange.width = 10;
  x_maxrange.height = 10;
  x_useprevious = 0;
  x_minblocks = 10;
  x_velsize.width = (comp_xsize-x_blocksize.width)/x_shiftsize.width;
  x_velsize.height = (comp_ysize-x_blocksize.height)/x_shiftsize.height;

  // initialize font
  cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );

  rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
  rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
  grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
  prev_grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

  x_velx = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );
  x_vely = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_of_bm :: ~pix_opencv_of_bm()
{
  // Destroy cv_images
  cvReleaseImage( &rgba );
  cvReleaseImage( &rgb );
  cvReleaseImage( &grey );
  cvReleaseImage( &prev_grey );
  cvReleaseImage( &x_velx );
  cvReleaseImage( &x_vely );

}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_of_bm :: processRGBAImage(imageStruct &image)
{
  int px,py;
  double globangle=0.0, globx=0.0, globy=0.0, maxamp=0.0, maxangle=0.0;
  int nbblocks=0;
  CvPoint orig, dest;
  double angle=0.0;
  double hypotenuse=0.0;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    x_velsize.width = (comp_xsize-x_blocksize.width)/x_shiftsize.width;
    x_velsize.height = (comp_ysize-x_blocksize.height)/x_shiftsize.height;

    cvReleaseImage( &rgba );
    cvReleaseImage( &rgb );
    cvReleaseImage( &grey );
    cvReleaseImage( &prev_grey );
    cvReleaseImage( &x_velx );
    cvReleaseImage( &x_vely );

    rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    prev_grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

    x_velx = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );
    x_vely = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );
  }

  memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );

  // Convert to hsv
  cvCvtColor(rgba, rgb, CV_BGRA2BGR);
  cvCvtColor(rgb, grey, CV_BGR2GRAY);

  if( x_nightmode )
      cvZero( rgb );

  cvCalcOpticalFlowBM( prev_grey, grey,
                       x_blocksize, x_shiftsize,
                       x_maxrange, x_useprevious,
                       x_velx, x_vely  );

  nbblocks = 0;
  globangle = 0;
  globx = 0;
  globy = 0;
  for( py=0; py<x_velsize.height; py++ )
  {
    for( px=0; px<x_velsize.width; px++ )
    {
        orig.x = (px*comp_xsize)/x_velsize.width;
        orig.y = (py*comp_ysize)/x_velsize.height;
        dest.x = (int)(orig.x + cvGet2D(x_velx, py, px).val[0]);
        dest.y = (int)(orig.y + cvGet2D(x_vely, py, px).val[0]);
        angle = -atan2( (double) (dest.y-orig.y), (double) (dest.x-orig.x) );
        hypotenuse = sqrt( pow(dest.y-orig.y, 2) + pow(dest.x-orig.x, 2) );

        /* Now draw the tips of the arrow. I do some scaling so that the
        * tips look proportional to the main line of the arrow.
        */
        if (hypotenuse >= x_threshold)
        {
          cvLine( rgb, orig, dest, CV_RGB(0,255,0), (int)hypotenuse/10, CV_AA, 0 );

          orig.x = (int) (dest.x - (x_shiftsize.width/4) * cos(angle + M_PI / 4));
          orig.y = (int) (dest.y + (x_shiftsize.height/4) * sin(angle + M_PI / 4));
          cvLine( rgb, orig, dest, CV_RGB(0,0,255), (int)hypotenuse/10, CV_AA, 0 );
          orig.x = (int) (dest.x - (x_shiftsize.width/4) * cos(angle - M_PI / 4));
          orig.y = (int) (dest.y + (x_shiftsize.height/4) * sin(angle - M_PI / 4));
          cvLine( rgb, orig, dest, CV_RGB(0,0,255), (int)hypotenuse/10, CV_AA, 0 );

          globx = globx+cvGet2D(x_velx, py, px).val[0];
          globy = globy+cvGet2D(x_vely, py, px).val[0];
          if ( hypotenuse > maxamp )
          {
             maxamp = hypotenuse;
             maxangle = angle;
          }
          // post( "pdp_opencv_of_bm : block %d : amp : %f : angle : %f", nbblocks, hypotenuse, (angle*180)/M_PI );
          nbblocks++;
        }

      }
  }

  if ( nbblocks >= x_minblocks )
  {
      globangle=-atan2( globy, globx );
      // post( "pdp_opencv_of_bm : globangle : %f", (globangle*180)/M_PI );

      orig.x = (int) (comp_xsize/2);
      orig.y = (int) (comp_ysize/2);
      dest.x = (int) (orig.x+((comp_xsize>comp_ysize)?comp_ysize/2:comp_xsize/2)*cos(globangle));
      dest.y = (int) (orig.y-((comp_xsize>comp_ysize)?comp_ysize/2:comp_xsize/2)*sin(globangle));
      cvLine( rgb, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );
      orig.x = (int) (dest.x - (x_shiftsize.width/2) * cos(globangle + M_PI / 4));
      orig.y = (int) (dest.y + (x_shiftsize.height/2) * sin(globangle + M_PI / 4));
      cvLine( rgb, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );
      orig.x = (int) (dest.x - (x_shiftsize.width/2) * cos(globangle - M_PI / 4));
      orig.y = (int) (dest.y + (x_shiftsize.height/2) * sin(globangle - M_PI / 4));
      cvLine( rgb, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );

      // outputs the average angle of movement
      globangle = (globangle*180)/M_PI;
      SETFLOAT(&x_list[0], globangle);
      outlet_list( m_meanout, 0, 1, x_list );

      // outputs the amplitude and angle of the maximum movement
      maxangle = (maxangle*180)/M_PI;
      SETFLOAT(&x_list[0], maxamp);
      SETFLOAT(&x_list[1], maxangle);
      outlet_list( m_maxout, 0, 2, x_list );
  }

  memcpy( prev_grey->imageData, grey->imageData, image.xsize*image.ysize );

  cvCvtColor(rgb, rgba, CV_BGR2BGRA);
  memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_of_bm :: processRGBImage(imageStruct &image)
{ 
  int px,py;
  double globangle=0.0, globx=0.0, globy=0.0, maxamp=0.0, maxangle=0.0;
  int nbblocks=0;
  CvPoint orig, dest;
  double angle=0.0;
  double hypotenuse=0.0;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    x_velsize.width = (comp_xsize-x_blocksize.width)/x_shiftsize.width;
    x_velsize.height = (comp_ysize-x_blocksize.height)/x_shiftsize.height;

    cvReleaseImage( &rgba );
    cvReleaseImage( &rgb );
    cvReleaseImage( &grey );
    cvReleaseImage( &prev_grey );
    cvReleaseImage( &x_velx );
    cvReleaseImage( &x_vely );

    rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    prev_grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

    x_velx = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );
    x_vely = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );
  }

  memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );

  // Convert to hsv
  cvCvtColor(rgba, rgb, CV_BGRA2BGR);
  cvCvtColor(rgb, grey, CV_BGR2GRAY);

  if( x_nightmode )
      cvZero( rgb );

  cvCalcOpticalFlowBM( prev_grey, grey,
                       x_blocksize, x_shiftsize,
                       x_maxrange, x_useprevious,
                       x_velx, x_vely  );

  nbblocks = 0;
  globangle = 0;
  globx = 0;
  globy = 0;
  for( py=0; py<x_velsize.height; py++ )
  {
    for( px=0; px<x_velsize.width; px++ )
    {
        // post( "pdp_opencv_of_bm : (%d,%d) values (%f,%f)", px, py, velxf, velyf );
        orig.x = (px*comp_xsize)/x_velsize.width;
        orig.y = (py*comp_ysize)/x_velsize.height;
        dest.x = (int)(orig.x + cvGet2D(x_velx, py, px).val[0]);
        dest.y = (int)(orig.y + cvGet2D(x_vely, py, px).val[0]);
        angle = -atan2( (double) (dest.y-orig.y), (double) (dest.x-orig.x) );
        hypotenuse = sqrt( pow(dest.y-orig.y, 2) + pow(dest.x-orig.x, 2) );

        /* Now draw the tips of the arrow. I do some scaling so that the
        * tips look proportional to the main line of the arrow.
        */
        if (hypotenuse >= x_threshold)
        {
          cvLine( rgb, orig, dest, CV_RGB(0,255,0), (int)hypotenuse/10, CV_AA, 0 );

          orig.x = (int) (dest.x - (x_shiftsize.width/4) * cos(angle + M_PI / 4));
          orig.y = (int) (dest.y + (x_shiftsize.height/4) * sin(angle + M_PI / 4));
          cvLine( rgb, orig, dest, CV_RGB(0,0,255), (int)hypotenuse/10, CV_AA, 0 );
          orig.x = (int) (dest.x - (x_shiftsize.width/4) * cos(angle - M_PI / 4));
          orig.y = (int) (dest.y + (x_shiftsize.height/4) * sin(angle - M_PI / 4));
          cvLine( rgb, orig, dest, CV_RGB(0,0,255), (int)hypotenuse/10, CV_AA, 0 );

          globx = globx+cvGet2D(x_velx, py, px).val[0];
          globy = globy+cvGet2D(x_vely, py, px).val[0];
          if ( hypotenuse > maxamp )
          {
             maxamp = hypotenuse;
             maxangle = angle;
          }
          // post( "pdp_opencv_of_bm : block %d : amp : %f : angle : %f", nbblocks, hypotenuse, (angle*180)/M_PI );
          nbblocks++;
        }

      }
  }

  if ( nbblocks >= x_minblocks )
  {
      globangle=-atan2( globy, globx );
      // post( "pdp_opencv_of_bm : globangle : %f", (globangle*180)/M_PI );

      orig.x = (int) (comp_xsize/2);
      orig.y = (int) (comp_ysize/2);
      dest.x = (int) (orig.x+((comp_xsize>comp_ysize)?comp_ysize/2:comp_xsize/2)*cos(globangle));
      dest.y = (int) (orig.y-((comp_xsize>comp_ysize)?comp_ysize/2:comp_xsize/2)*sin(globangle));
      cvLine( rgb, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );
      orig.x = (int) (dest.x - (x_shiftsize.width/2) * cos(globangle + M_PI / 4));
      orig.y = (int) (dest.y + (x_shiftsize.height/2) * sin(globangle + M_PI / 4));
      cvLine( rgb, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );
      orig.x = (int) (dest.x - (x_shiftsize.width/2) * cos(globangle - M_PI / 4));
      orig.y = (int) (dest.y + (x_shiftsize.height/2) * sin(globangle - M_PI / 4));
      cvLine( rgb, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );

      // outputs the average angle of movement
      globangle = (globangle*180)/M_PI;
      SETFLOAT(&x_list[0], globangle);
      outlet_list( m_meanout, 0, 1, x_list );

      // outputs the amplitude and angle of the maximum movement
      maxangle = (maxangle*180)/M_PI;
      SETFLOAT(&x_list[0], maxamp);
      SETFLOAT(&x_list[1], maxangle);
      outlet_list( m_maxout, 0, 2, x_list );
  }

  memcpy( prev_grey->imageData, grey->imageData, image.xsize*image.ysize );

  memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_of_bm :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_of_bm : yuv format not supported" );
}
    	
void pix_opencv_of_bm :: processGrayImage(imageStruct &image)
{ 
  int px,py;
  double globangle=0.0, globx=0.0, globy=0.0, maxamp=0.0, maxangle=0.0;
  int nbblocks=0;
  CvPoint orig, dest;
  double angle=0.0;
  double hypotenuse=0.0;

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) 
  {

    this->comp_xsize=image.xsize;
    this->comp_ysize=image.ysize;

    x_velsize.width = (comp_xsize-x_blocksize.width)/x_shiftsize.width;
    x_velsize.height = (comp_ysize-x_blocksize.height)/x_shiftsize.height;

    cvReleaseImage( &rgba );
    cvReleaseImage( &rgb );
    cvReleaseImage( &grey );
    cvReleaseImage( &prev_grey );
    cvReleaseImage( &x_velx );
    cvReleaseImage( &x_vely );

    rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
    rgb = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 3 );
    grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );
    prev_grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

    x_velx = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );
    x_vely = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );
  }

  memcpy( grey->imageData, image.data, image.xsize*image.ysize );

  if( x_nightmode )
      cvZero( grey );

  cvCalcOpticalFlowBM( prev_grey, grey,
                       x_blocksize, x_shiftsize,
                       x_maxrange, x_useprevious,
                       x_velx, x_vely  );

  nbblocks = 0;
  globangle = 0;
  globx = 0;
  globy = 0;
  for( py=0; py<x_velsize.height; py++ )
  {
    for( px=0; px<x_velsize.width; px++ )
    {
        // post( "pdp_opencv_of_bm : (%d,%d) values (%f,%f)", px, py, velxf, velyf );
        orig.x = (px*comp_xsize)/x_velsize.width;
        orig.y = (py*comp_ysize)/x_velsize.height;
        dest.x = (int)(orig.x + cvGet2D(x_velx, py, px).val[0]);
        dest.y = (int)(orig.y + cvGet2D(x_vely, py, px).val[0]);
        angle = -atan2( (double) (dest.y-orig.y), (double) (dest.x-orig.x) );
        hypotenuse = sqrt( pow(dest.y-orig.y, 2) + pow(dest.x-orig.x, 2) );

        /* Now draw the tips of the arrow. I do some scaling so that the
        * tips look proportional to the main line of the arrow.
        */
        if (hypotenuse >= x_threshold)
        {
          cvLine( grey, orig, dest, CV_RGB(0,255,0), (int)hypotenuse/10, CV_AA, 0 );

          orig.x = (int) (dest.x - (x_shiftsize.width/4) * cos(angle + M_PI / 4));
          orig.y = (int) (dest.y + (x_shiftsize.height/4) * sin(angle + M_PI / 4));
          cvLine( grey, orig, dest, CV_RGB(0,0,255), (int)hypotenuse/10, CV_AA, 0 );
          orig.x = (int) (dest.x - (x_shiftsize.width/4) * cos(angle - M_PI / 4));
          orig.y = (int) (dest.y + (x_shiftsize.height/4) * sin(angle - M_PI / 4));
          cvLine( grey, orig, dest, CV_RGB(0,0,255), (int)hypotenuse/10, CV_AA, 0 );

          globx = globx+cvGet2D(x_velx, py, px).val[0];
          globy = globy+cvGet2D(x_vely, py, px).val[0];
          if ( hypotenuse > maxamp )
          {
             maxamp = hypotenuse;
             maxangle = angle;
          }
          // post( "pdp_opencv_of_bm : block %d : amp : %f : angle : %f", nbblocks, hypotenuse, (angle*180)/M_PI );
          nbblocks++;
        }

      }
  }

  if ( nbblocks >= x_minblocks )
  {
      globangle=-atan2( globy, globx );
      // post( "pdp_opencv_of_bm : globangle : %f", (globangle*180)/M_PI );

      orig.x = (int) (comp_xsize/2);
      orig.y = (int) (comp_ysize/2);
      dest.x = (int) (orig.x+((comp_xsize>comp_ysize)?comp_ysize/2:comp_xsize/2)*cos(globangle));
      dest.y = (int) (orig.y-((comp_xsize>comp_ysize)?comp_ysize/2:comp_xsize/2)*sin(globangle));
      cvLine( grey, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );
      orig.x = (int) (dest.x - (x_shiftsize.width/2) * cos(globangle + M_PI / 4));
      orig.y = (int) (dest.y + (x_shiftsize.height/2) * sin(globangle + M_PI / 4));
      cvLine( grey, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );
      orig.x = (int) (dest.x - (x_shiftsize.width/2) * cos(globangle - M_PI / 4));
      orig.y = (int) (dest.y + (x_shiftsize.height/2) * sin(globangle - M_PI / 4));
      cvLine( grey, orig, dest, CV_RGB(255,255,255), 3, CV_AA, 0 );

      // outputs the average angle of movement
      globangle = (globangle*180)/M_PI;
      SETFLOAT(&x_list[0], globangle);
      outlet_list( m_meanout, 0, 1, x_list );

      // outputs the amplitude and angle of the maximum movement
      maxangle = (maxangle*180)/M_PI;
      SETFLOAT(&x_list[0], maxamp);
      SETFLOAT(&x_list[1], maxangle);
      outlet_list( m_maxout, 0, 2, x_list );
  }

  memcpy( prev_grey->imageData, grey->imageData, image.xsize*image.ysize );

  memcpy( image.data, grey->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////

void pix_opencv_of_bm :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_of_bm::nightModeMessCallback,
		  gensym("nightmode"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_of_bm::tresholdMessCallback,
		  gensym("threshold"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_of_bm::blocksizeMessCallback,
		  gensym("blocksize"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_of_bm::shiftsizeMessCallback,
		  gensym("shiftsize"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_of_bm::maxrangeMessCallback,
		  gensym("maxrange"), A_FLOAT, A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_of_bm::usePreviousMessCallback,
		  gensym("useprevious"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_of_bm::minBlocksMessCallback,
		  gensym("minblocks"), A_FLOAT, A_NULL);
}

void  pix_opencv_of_bm :: nightModeMessCallback(void *data, t_floatarg nightmode)
{
    GetMyClass(data)->nightModeMess((float)nightmode);
}

void  pix_opencv_of_bm :: tresholdMessCallback(void *data, t_floatarg threshold)
{
    GetMyClass(data)->tresholdMess((float)threshold);
}

void  pix_opencv_of_bm :: blocksizeMessCallback(void *data, t_floatarg fwidth, t_floatarg fheight)
{
    GetMyClass(data)->blocksizeMess((float)fwidth, (float)fheight);
}

void  pix_opencv_of_bm :: shiftsizeMessCallback(void *data, t_floatarg fwidth, t_floatarg fheight)
{
    GetMyClass(data)->shiftsizeMess((float)fwidth, (float)fheight);
}

void  pix_opencv_of_bm :: maxrangeMessCallback(void *data, t_floatarg fwidth, t_floatarg fheight)
{
    GetMyClass(data)->maxrangeMess((float)fwidth, (float)fheight);
}

void  pix_opencv_of_bm :: usePreviousMessCallback(void *data, t_floatarg previous)
{
    GetMyClass(data)->usePreviousMess((float)previous);
}

void  pix_opencv_of_bm :: minBlocksMessCallback(void *data, t_floatarg minblocks)
{
    GetMyClass(data)->minBlocksMess((float)minblocks);
}

void  pix_opencv_of_bm :: nightModeMess(float nightmode)
{
   if ( ( (int)nightmode==0 ) || ( (int)nightmode==1 ) ) x_nightmode = (int)nightmode;
}

void  pix_opencv_of_bm :: tresholdMess(float threshold)
{
   if ( (int)threshold>0 ) x_threshold = (int)threshold;
}

void  pix_opencv_of_bm :: blocksizeMess(float fwidth, float fheight)
{
  if (fwidth>=5.0) x_blocksize.width = (int)fwidth;
  if (fheight>=5.0) x_blocksize.height = (int)fheight;

  x_velsize.width = (comp_xsize-x_blocksize.width)/x_shiftsize.width;
  x_velsize.height = (comp_ysize-x_blocksize.height)/x_shiftsize.height;
  cvReleaseImage( &x_velx );
  cvReleaseImage( &x_vely );
  x_velx = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );
  x_vely = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );

}

void  pix_opencv_of_bm :: shiftsizeMess(float fwidth, float fheight)
{
  if (fwidth>=5.0) x_shiftsize.width = (int)fwidth;
  if (fheight>=5.0) x_shiftsize.height = (int)fheight;

  x_velsize.width = (comp_xsize-x_blocksize.width)/x_shiftsize.width;
  x_velsize.height = (comp_ysize-x_blocksize.height)/x_shiftsize.height;
  cvReleaseImage( &x_velx );
  cvReleaseImage( &x_vely );
  x_velx = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );
  x_vely = cvCreateImage( x_velsize, IPL_DEPTH_32F, 1 );

}

void  pix_opencv_of_bm :: maxrangeMess(float fwidth, float fheight)
{
  if (fwidth>=5.0) x_maxrange.width = (int)fwidth;
  if (fheight>=5.0) x_maxrange.height = (int)fheight;
}

void  pix_opencv_of_bm :: usePreviousMess(float previous)
{
  if ((previous==0.0)||(previous==1.0)) x_useprevious = (int)previous;
}

void  pix_opencv_of_bm :: minBlocksMess(float minblocks)
{
  if (minblocks>=1.0) x_minblocks = (int)minblocks;
}

