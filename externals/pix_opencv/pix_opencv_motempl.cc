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

#include "pix_opencv_motempl.h"

CPPEXTERN_NEW(pix_opencv_motempl)

/////////////////////////////////////////////////////////
//
// pix_opencv_motempl
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_motempl :: pix_opencv_motempl()
{ 
  int i;

  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("ft1"));
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("min_size"));
  inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("max_size"));
  m_dataout = outlet_new(this->x_obj, 0);

  mhi_duration = 1.0;
  aperture = 3;
  diff_threshold = 30;
  last = 0;
  comp_xsize  = 320;
  comp_ysize  = 240;

  // various tracking parameters (in seconds)
  max_time_delta = 0.5;
  min_time_delta = 0.05;
  // number of cyclic frame buffer used for motion detection
  // (should, probably, depend on FPS)
  frame_buffer_num = 4;

  min_size=50;
  max_size=500; 

  rgb = NULL;
  motion = NULL;
  rgba = NULL;
  grey = NULL;
  mhi = NULL;
  orient = NULL; 
  mask = NULL; 
  segmask = NULL;
  storage = NULL; 
  
  mask_size = CV_DIST_MASK_PRECISE;

  cvInitFont( &font, CV_FONT_HERSHEY_PLAIN, 1.0, 1.0, 0, 1, 8 );

  rgb = cvCreateImage(cvSize(comp_xsize,comp_ysize), IPL_DEPTH_8U, 3);
  motion = cvCreateImage( cvSize(comp_xsize,comp_ysize), 8, 3 );
  cvZero( motion );
  motion->origin = rgb->origin;
  rgba = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 4 );
  grey = cvCreateImage( cvSize(comp_xsize, comp_ysize), 8, 1 );

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_motempl :: ~pix_opencv_motempl()
{
    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &motion );
    	cvReleaseImage( &rgba );
    	cvReleaseImage( &grey );
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_motempl :: processRGBAImage(imageStruct &image)
{
 double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
 CvSize size = cvSize(image.xsize,image.ysize); // get current frame size
 int i, j, idx1 = last, idx2;
 IplImage* silh;
 CvSeq* seq;
 CvRect comp_rect;
 double count;
 double angle;
 CvPoint center;
 double magnitude;          
 CvScalar color;
 char tindex[10];

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &motion );
    	cvReleaseImage( &rgba );
    	cvReleaseImage( &grey );

	//Create cv_images 
    	rgb = cvCreateImage(cvSize(image.xsize, image.ysize), IPL_DEPTH_8U, 3);
    	motion = cvCreateImage( cvSize(rgb->width,rgb->height), 8, 3 );
    	cvZero( motion );
    	motion->origin = rgb->origin;
    	rgba = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 4 );
    	grey = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 1 );
    }
    memcpy( rgba->imageData, image.data, image.xsize*image.ysize*4 );
    
    cvCvtColor( rgba, rgb, CV_RGBA2RGB);

    // allocate images at the beginning or
    // reallocate them if the frame size is changed
    if( (!mhi) || (mhi->width != size.width) || (mhi->height != size.height) || (!buf)) {
        if( buf == 0 ) {
            buf = (IplImage**)malloc(frame_buffer_num*sizeof(buf[0]));
            //memset( buf, 0, N*sizeof(buf[0]));
        }
        
        for( i = 0; i < frame_buffer_num; i++ ) {
            // TODO if ( buf[i] != NULL ) cvReleaseImage( &buf[i] );
            buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
            cvZero( buf[i] );
        }
        if ( mhi != NULL ) cvReleaseImage( &mhi );
        if ( orient != NULL ) cvReleaseImage( &orient );
        if ( segmask != NULL ) cvReleaseImage( &segmask );
        if ( mask != NULL ) cvReleaseImage( &mask );
        
        mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        cvZero( mhi ); // clear MHI at the beginning
        orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
    }

    cvCvtColor( rgb, buf[last], CV_BGR2GRAY ); // convert frame to grayscale

    idx2 = (last + 1) % frame_buffer_num; // index of (last - (N-1))th frame
    last = idx2;

    silh = buf[idx2];
    cvAbsDiff( buf[idx1], buf[idx2], silh ); // get difference between frames
    
    cvThreshold( silh, silh, diff_threshold, 1, CV_THRESH_BINARY ); // and threshold it
    cvUpdateMotionHistory( silh, mhi, timestamp, mhi_duration ); // update MHI

    // convert MHI to red 8u image
    cvCvtScale( mhi, mask, 255./mhi_duration,
                (mhi_duration - timestamp)*255./mhi_duration );
    cvZero( motion );
    cvCvtPlaneToPix( mask, 0, 0, 0, motion );

    // calculate motion gradient orientation and valid orientation mask
    cvCalcMotionGradient( mhi, mask, orient, max_time_delta, min_time_delta, aperture );
    
    if( !storage )
        storage = cvCreateMemStorage(0);
    else
        cvClearMemStorage(storage);
    
    // segment motion: get sequence of motion components
    // segmask is marked motion components map. It is not used further
    seq = cvSegmentMotion( mhi, segmask, storage, timestamp, max_time_delta );

    // iterate through the motion components,
    // One more iteration (i == -1) corresponds to the whole image (global motion)
    j=0;
    for( i = -1; i < seq->total; i++ ) {

        if( i < 0 ) { // case of the whole image
            comp_rect = cvRect( 0, 0, size.width, size.height );
            color = CV_RGB(255,255,255);
            magnitude = 100;
        }
        else { // i-th motion component
            comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
            if(( comp_rect.width + comp_rect.height < min_size )||( comp_rect.width + comp_rect.height > max_size )) // reject very small/big components 
                continue;
            color = CV_RGB(255,0,0);
            magnitude = (comp_rect.width + comp_rect.height) /4;
        }

        // select component ROI
        cvSetImageROI( silh, comp_rect );
        cvSetImageROI( mhi, comp_rect );
        cvSetImageROI( orient, comp_rect );
        cvSetImageROI( mask, comp_rect );

        // calculate orientation
        angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, mhi_duration);
        angle = 360.0 - angle;  // adjust for images with top-left origin

        count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI

        cvResetImageROI( mhi );
        cvResetImageROI( orient );
        cvResetImageROI( mask );
        cvResetImageROI( silh );

        // check for the case of little motion
        if( count < comp_rect.width*comp_rect.height * 0.05 )
            continue;

        // draw a clock with arrow indicating the direction
        center = cvPoint( (comp_rect.x + comp_rect.width/2),
                          (comp_rect.y + comp_rect.height/2) );

        cvCircle( motion, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
        cvLine( motion, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
                cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );

        if( i < 0 )  // case of the whole image
        {
          sprintf( tindex, "%d", i );
          cvPutText( motion, tindex, center, &font, CV_RGB(255,255,255));
          SETFLOAT(&rlist[0], i);
          SETFLOAT(&rlist[1], center.x);
          SETFLOAT(&rlist[2], center.y);
          SETFLOAT(&rlist[3], comp_rect.width);
          SETFLOAT(&rlist[4], comp_rect.height);
          SETFLOAT(&rlist[5], angle);
          outlet_list( m_dataout, 0, 6, rlist );
        }
        else
        {
          sprintf( tindex, "%d", ++j );
          cvPutText( motion, tindex, center, &font, CV_RGB(255,255,255));
          SETFLOAT(&rlist[0], j);
          SETFLOAT(&rlist[1], center.x);
          SETFLOAT(&rlist[2], center.y);
          SETFLOAT(&rlist[3], comp_rect.width);
          SETFLOAT(&rlist[4], comp_rect.height);
          SETFLOAT(&rlist[5], angle);
          outlet_list( m_dataout, 0, 6, rlist );
        }
    }

    
    cvCvtColor( motion, rgba, CV_RGB2RGBA);
    memcpy( image.data, rgba->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_motempl :: processRGBImage(imageStruct &image)
{
 double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
 CvSize size = cvSize(image.xsize,image.ysize); // get current frame size
 int i, j, idx1 = last, idx2;
 IplImage* silh;
 CvSeq* seq;
 CvRect comp_rect;
 double count;
 double angle;
 CvPoint center;
 double magnitude;          
 CvScalar color;
 char tindex[10];

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &motion );
    	cvReleaseImage( &rgba );
    	cvReleaseImage( &grey );

	//Create cv_images 
    	rgb = cvCreateImage(cvSize(image.xsize, image.ysize), IPL_DEPTH_8U, 3);
    	motion = cvCreateImage( cvSize(rgb->width,rgb->height), 8, 3 );
    	cvZero( motion );
    	motion->origin = rgb->origin;
    	rgba = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 4 );
    	grey = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 1 );
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    
    // allocate images at the beginning or
    // reallocate them if the frame size is changed
    if( (!mhi) || (mhi->width != size.width) || (mhi->height != size.height)  || (!buf)) {
        if( buf == 0 ) {
            buf = (IplImage**)malloc(frame_buffer_num*sizeof(buf[0]));
            //memset( buf, 0, N*sizeof(buf[0]));
        }
        
        for( i = 0; i < frame_buffer_num; i++ ) {
            // TODO if ( buf[i] != NULL ) cvReleaseImage( &(buf[i]) );
            buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
            cvZero( buf[i] );
        }
        if ( mhi != NULL ) cvReleaseImage( &mhi );
        if ( orient != NULL ) cvReleaseImage( &orient );
        if ( segmask != NULL ) cvReleaseImage( &segmask );
        if ( mask != NULL ) cvReleaseImage( &mask );
        
        mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        cvZero( mhi ); // clear MHI at the beginning
        orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
    }

    cvCvtColor( rgb, buf[last], CV_BGR2GRAY ); // convert frame to grayscale

    idx2 = (last + 1) % frame_buffer_num; // index of (last - (N-1))th frame
    last = idx2;

    silh = buf[idx2];
    cvAbsDiff( buf[idx1], buf[idx2], silh ); // get difference between frames
    
    cvThreshold( silh, silh, diff_threshold, 1, CV_THRESH_BINARY ); // and threshold it
    cvUpdateMotionHistory( silh, mhi, timestamp, mhi_duration ); // update MHI

    // convert MHI to blue 8u image
    cvCvtScale( mhi, mask, 255./mhi_duration,
                (mhi_duration - timestamp)*255./mhi_duration );
    cvZero( motion );
    cvCvtPlaneToPix( mask, 0, 0, 0, motion );

    // calculate motion gradient orientation and valid orientation mask
    cvCalcMotionGradient( mhi, mask, orient, max_time_delta, min_time_delta, aperture );
    
    if( !storage )
        storage = cvCreateMemStorage(0);
    else
        cvClearMemStorage(storage);
    
    // segment motion: get sequence of motion components
    // segmask is marked motion components map. It is not used further
    seq = cvSegmentMotion( mhi, segmask, storage, timestamp, max_time_delta );

    // iterate through the motion components,
    // One more iteration (i == -1) corresponds to the whole image (global motion)
    j=0;
    for( i = -1; i < seq->total; i++ ) {

        if( i < 0 ) { // case of the whole image
            comp_rect = cvRect( 0, 0, size.width, size.height );
            color = CV_RGB(255,255,255);
            magnitude = 100;
        }
        else { // i-th motion component
            comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
            if(( comp_rect.width + comp_rect.height < min_size )||( comp_rect.width + comp_rect.height > max_size )) // reject very small/big components 
                continue;
            color = CV_RGB(255,0,0);
            magnitude = (comp_rect.width + comp_rect.height) / 4;
        }

        // select component ROI
        cvSetImageROI( silh, comp_rect );
        cvSetImageROI( mhi, comp_rect );
        cvSetImageROI( orient, comp_rect );
        cvSetImageROI( mask, comp_rect );

        // calculate orientation
        angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, mhi_duration);
        angle = 360.0 - angle;  // adjust for images with top-left origin

        count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI

        cvResetImageROI( mhi );
        cvResetImageROI( orient );
        cvResetImageROI( mask );
        cvResetImageROI( silh );

        // check for the case of little motion
        if( count < comp_rect.width*comp_rect.height * 0.05 )
            continue;

        // draw a clock with arrow indicating the direction
        center = cvPoint( (comp_rect.x + comp_rect.width/2),
                          (comp_rect.y + comp_rect.height/2) );

        cvCircle( motion, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
        cvLine( motion, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
                cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );

        if( i < 0 )  // case of the whole image
        {
          sprintf( tindex, "%d", i );
          cvPutText( motion, tindex, center, &font, CV_RGB(255,255,255));
          SETFLOAT(&rlist[0], i);
          SETFLOAT(&rlist[1], center.x);
          SETFLOAT(&rlist[2], center.y);
          SETFLOAT(&rlist[3], comp_rect.width);
          SETFLOAT(&rlist[4], comp_rect.height);
          SETFLOAT(&rlist[5], angle);
          outlet_list( m_dataout, 0, 6, rlist );
        }
        else
        {
          sprintf( tindex, "%d", ++j );
          cvPutText( motion, tindex, center, &font, CV_RGB(255,255,255));
          SETFLOAT(&rlist[0], j);
          SETFLOAT(&rlist[1], center.x);
          SETFLOAT(&rlist[2], center.y);
          SETFLOAT(&rlist[3], comp_rect.width);
          SETFLOAT(&rlist[4], comp_rect.height);
          SETFLOAT(&rlist[5], angle);
          outlet_list( m_dataout, 0, 6, rlist );
        }
    }


    //cvShowImage(wndname, cedge);
    memcpy( image.data, motion->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_motempl :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_motempl : yuv format not supported" );
}
    	
void pix_opencv_motempl :: processGrayImage(imageStruct &image)
{ 
 double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
 CvSize size = cvSize(image.xsize,image.ysize); // get current frame size
 int i, j, idx1 = last, idx2;
 IplImage* silh;
 CvSeq* seq;
 CvRect comp_rect;
 double count;
 double angle;
 CvPoint center;
 double magnitude;          
 CvScalar color;
 char tindex[10];

  if ((this->comp_xsize!=image.xsize)&&(this->comp_ysize!=image.ysize)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
    	cvReleaseImage( &rgb );
    	cvReleaseImage( &motion );
    	cvReleaseImage( &rgba );
    	cvReleaseImage( &grey );

	//Create cv_images 
    	rgb = cvCreateImage(cvSize(image.xsize, image.ysize), IPL_DEPTH_8U, 3);
    	motion = cvCreateImage( cvSize(rgb->width,rgb->height), 8, 3 );
    	cvZero( motion );
    	motion->origin = rgb->origin;
    	rgba = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 4 );
    	grey = cvCreateImage( cvSize(image.xsize, image.ysize), 8, 1 );
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( grey->imageData, image.data, image.xsize*image.ysize );
    
    // Convert to RGB
    cvCvtColor( grey, rgb, CV_GRAY2RGB);
    
    // allocate images at the beginning or
    // reallocate them if the frame size is changed
    if( (!mhi) || (mhi->width != size.width) || (mhi->height != size.height) || (!buf)) {
        if( buf == 0 ) {
            buf = (IplImage**)malloc(frame_buffer_num*sizeof(buf[0]));
            //memset( buf, 0, N*sizeof(buf[0]));
        }
        
        for( i = 0; i < frame_buffer_num; i++ ) {
            // TODO cvReleaseImage( &(buf[i]) );
            buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
            cvZero( buf[i] );
        }
        if ( mhi != NULL ) cvReleaseImage( &mhi );
        if ( orient != NULL ) cvReleaseImage( &orient );
        if ( segmask != NULL ) cvReleaseImage( &segmask );
        if ( mask != NULL ) cvReleaseImage( &mask );
        
        mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        cvZero( mhi ); // clear MHI at the beginning
        orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
    }

    cvCvtColor( rgb, buf[last], CV_BGR2GRAY ); // convert frame to grayscale

    idx2 = (last + 1) % frame_buffer_num; // index of (last - (N-1))th frame
    last = idx2;

    silh = buf[idx2];
    cvAbsDiff( buf[idx1], buf[idx2], silh ); // get difference between frames
    
    cvThreshold( silh, silh, diff_threshold, 1, CV_THRESH_BINARY ); // and threshold it
    cvUpdateMotionHistory( silh, mhi, timestamp, mhi_duration ); // update MHI

    // convert MHI to blue 8u image
    cvCvtScale( mhi, mask, 255./mhi_duration,
                (mhi_duration - timestamp)*255./mhi_duration );
    cvZero( motion );
    cvCvtPlaneToPix( mask, 0, 0, 0, motion );

    // calculate motion gradient orientation and valid orientation mask
    cvCalcMotionGradient( mhi, mask, orient, max_time_delta, min_time_delta, aperture );
    
    if( !storage )
        storage = cvCreateMemStorage(0);
    else
        cvClearMemStorage(storage);
    
    // segment motion: get sequence of motion components
    // segmask is marked motion components map. It is not used further
    seq = cvSegmentMotion( mhi, segmask, storage, timestamp, max_time_delta );

    // iterate through the motion components,
    // One more iteration (i == -1) corresponds to the whole image (global motion)
    j=0;
    for( i = -1; i < seq->total; i++ ) {

        if( i < 0 ) { // case of the whole image
            comp_rect = cvRect( 0, 0, size.width, size.height );
            color = CV_RGB(255,255,255);
            magnitude = 100;
        }
        else { // i-th motion component
            comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
            if(( comp_rect.width + comp_rect.height < min_size )||( comp_rect.width + comp_rect.height > max_size )) // reject very small components
                continue;
            color = CV_RGB(255,0,0);
            magnitude = (comp_rect.width + comp_rect.height) / 4;
        }

        // select component ROI
        cvSetImageROI( silh, comp_rect );
        cvSetImageROI( mhi, comp_rect );
        cvSetImageROI( orient, comp_rect );
        cvSetImageROI( mask, comp_rect );

        // calculate orientation
        angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, mhi_duration);
        angle = 360.0 - angle;  // adjust for images with top-left origin

        count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI

        cvResetImageROI( mhi );
        cvResetImageROI( orient );
        cvResetImageROI( mask );
        cvResetImageROI( silh );

        // check for the case of little motion
        if( count < comp_rect.width*comp_rect.height * 0.05 )
            continue;

        // draw a clock with arrow indicating the direction
        center = cvPoint( (comp_rect.x + comp_rect.width/2),
                          (comp_rect.y + comp_rect.height/2) );

        cvCircle( motion, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
        cvLine( motion, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
                cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );

        if( i < 0 )  // case of the whole image
        {
          sprintf( tindex, "%d", i );
          cvPutText( motion, tindex, center, &font, CV_RGB(255,255,255));
          SETFLOAT(&rlist[0], i);
          SETFLOAT(&rlist[1], center.x);
          SETFLOAT(&rlist[2], center.y);
          SETFLOAT(&rlist[3], comp_rect.width);
          SETFLOAT(&rlist[4], comp_rect.height);
          SETFLOAT(&rlist[5], angle);
          outlet_list( m_dataout, 0, 6, rlist );
        }
        else
        {
          sprintf( tindex, "%d", ++j );
          cvPutText( motion, tindex, center, &font, CV_RGB(255,255,255));
          SETFLOAT(&rlist[0], j);
          SETFLOAT(&rlist[1], center.x);
          SETFLOAT(&rlist[2], center.y);
          SETFLOAT(&rlist[3], comp_rect.width);
          SETFLOAT(&rlist[4], comp_rect.height);
          SETFLOAT(&rlist[5], angle);
    	  outlet_list( m_dataout, 0, 6, rlist );
        }
    }


    // Convert to grayscale
    cvCvtColor( motion, grey, CV_RGB2GRAY);
    //cvShowImage(wndname, cedge);
    memcpy( image.data, grey->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_motempl :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_motempl::thresholdMessCallback,
  		  gensym("ft1"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_motempl::mhi_durationMessCallback,
		  gensym("mhi_duration"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_motempl::max_time_deltaMessCallback, gensym("max_time_delta"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_motempl::min_time_deltaMessCallback, gensym("min_time_delta"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_motempl::frame_buffer_numMessCallback, gensym("frame_buffer_num"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_motempl::min_sizeMessCallback, gensym("min_size"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_motempl::max_sizeMessCallback, gensym("max_size"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_motempl::apertureMessCallback, gensym("aperture"), A_FLOAT, A_NULL);
}
void pix_opencv_motempl :: thresholdMessCallback(void *data, t_floatarg pos)
{
  GetMyClass(data)->floatThreshMess((float)pos);
}
void pix_opencv_motempl :: mhi_durationMessCallback(void *data, t_floatarg mhi_duration)
{
  GetMyClass(data)->floatMhiDuration((float)mhi_duration);
}
void pix_opencv_motempl :: min_sizeMessCallback(void *data, t_floatarg min_size)
{
  GetMyClass(data)->floatmin_size((float)min_size);
}
void pix_opencv_motempl :: max_sizeMessCallback(void *data, t_floatarg max_size)
{
  GetMyClass(data)->floatmax_size((float)max_size);
}
void pix_opencv_motempl :: max_time_deltaMessCallback(void *data, t_floatarg max_time_delta)
{
  GetMyClass(data)->floatmax_time_delta((float)max_time_delta);
}
void pix_opencv_motempl :: min_time_deltaMessCallback(void *data, t_floatarg min_time_delta)
{
  GetMyClass(data)->floatmin_time_delta((float)min_time_delta);
}
void pix_opencv_motempl :: frame_buffer_numMessCallback(void *data, t_floatarg frame_buffer_num)
{
  GetMyClass(data)->floatframe_buffer_num((float)frame_buffer_num);
}
void pix_opencv_motempl :: apertureMessCallback(void *data, t_floatarg aperture)
{
  GetMyClass(data)->apertureMess((float)aperture);
}
void pix_opencv_motempl :: floatThreshMess(float thresh)
{
  if (thresh>=0) diff_threshold = (int)thresh;
}
void pix_opencv_motempl :: floatMhiDuration(float duration)
{
  if ( duration < 1.0 ) mhi_duration = duration;
}
void pix_opencv_motempl :: apertureMess(float aperture)
{
  if ( ( aperture == 3.0 ) || ( aperture == 5.0 ) || ( aperture == 7.0 ) )
  {
    aperture = (int)aperture;
  }
}
void pix_opencv_motempl :: floatmax_size(float max_size)
{
  if (max_size>=0) this->max_size = (int)max_size;
}
void pix_opencv_motempl :: floatmin_size(float min_size)
{
  if (min_size>=0) this->min_size = (int)min_size;
}
void pix_opencv_motempl :: floatframe_buffer_num(float frame_buffer_num)
{
  if (frame_buffer_num>=3) 
  {
    this->frame_buffer_num = (int)frame_buffer_num;
    this->buf = NULL;
  }
}
void pix_opencv_motempl :: floatmax_time_delta(float max_time_delta)
{
  if (max_time_delta>=0) this->max_time_delta = max_time_delta;
}
void pix_opencv_motempl :: floatmin_time_delta(float min_time_delta)
{
  if (min_time_delta>=0) this->min_time_delta = min_time_delta;
}
