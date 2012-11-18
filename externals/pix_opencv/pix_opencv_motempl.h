/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Motion detection based on motion history

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_MOTEMPL_H_
#define INCLUDE_PIX_OPENCV_MOTEMPL_H_

#ifndef _EiC
#include "cv.h"
#include <time.h>
#include <math.h>
#include <ctype.h>
#endif
#include <stdio.h>

#include "Base/GemPixObj.h"


/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_motempl
    
    Motion detection based on motion history

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_motempl : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_motempl, GemPixObj)

    public:

    //////////
    // Constructor
    pix_opencv_motempl();
    	
    protected:
    	
    //////////
    // Destructor
    virtual ~pix_opencv_motempl();

    //////////
    // Do the processing
    virtual void 	processRGBAImage(imageStruct &image);
    virtual void 	processRGBImage(imageStruct &image);
    virtual void 	processYUVImage(imageStruct &image);
    virtual void 	processGrayImage(imageStruct &image); 
    	
   //////////
   // Set the new edge threshold
   void	    	floatThreshMess(float thresh);
   void	    	floatMhiDuration(float duration);
   void	    	floatmin_time_delta(float min_time_delta);
   void	    	floatmax_time_delta(float max_time_delta);
   void	    	floatframe_buffer_num(float frame_buffer_num);
   void	    	floatmax_size(float max_size);
   void	    	floatmin_size(float min_size);
   void	    	apertureMess(float aperture);

   // to detect changes in the image size
   int 		comp_xsize;
   int		comp_ysize;
    	
   // Some varibales to control mophology mhi_duration
   double 	mhi_duration;
   int 		diff_threshold;
   int 		mask_size;
   int 		last;
   int          aperture;

   // various tracking parameters (in seconds)
   double max_time_delta;
   double min_time_delta;
   // number of cyclic frame buffer used for motion detection
   // (should, probably, depend on FPS)
   int frame_buffer_num; 

   int max_size;
   int min_size;

   private:

   //////////
   // Static member functions
   static void 	thresholdMessCallback(void *data, t_floatarg pos);
   static void 	mhi_durationMessCallback(void *data, t_floatarg mhi_duration);
   static void 	max_time_deltaMessCallback(void *data, t_floatarg max_time_delta);
   static void 	min_time_deltaMessCallback(void *data, t_floatarg min_time_delta);
   static void 	frame_buffer_numMessCallback(void *data, t_floatarg frame_buffer_num);
   static void 	min_sizeMessCallback(void *data, t_floatarg min_size);
   static void 	max_sizeMessCallback(void *data, t_floatarg max_size);
   static void 	apertureMessCallback(void *data, t_floatarg aperture);

   // The output and temporary images
   IplImage 	*rgb, *motion, *rgba, *grey;
    
   // ring image buffer
   IplImage **buf;

   // temporary images
   IplImage *mhi; // MHI
   IplImage *orient; // orientation
   IplImage *mask; // valid orientation mask
   IplImage *segmask; // motion segmentation map
   CvMemStorage* storage; // temporary storage
   t_outlet 	*m_dataout;
   CvFont font;
   t_atom rlist[6];

};

#endif	// for header file
