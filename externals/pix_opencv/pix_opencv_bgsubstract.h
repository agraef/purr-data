/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Change pix to greyscale

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_BGSUBSTRACT_H_
#define INCLUDE_PIX_OPENCV_BGSUBSTRACT_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_bgsubstract
    
    Change pix to greyscale

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_bgsubstract : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_bgsubstract, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_bgsubstract();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_bgsubstract();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
	//////////
    	// Set the new edge threshold
    	void	    	floatThreshMess(float x_threshold);
    	void	    	SetMess();
    	// The new threshold
	int 		x_threshold;
	int 		x_set;

	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;

    private:
    
    	//////////
    	// Static member functions
    	static void 	floatTreshMessCallback(void *data, t_floatarg thresh_value);
    	static void 	SetMessCallback(void *data);

	/////////
	// IplImage needed
    	IplImage 	*orig, *rgb, *gray, *prev_gray, *grayLow, *grayUp, *diff_8U;
	
};

#endif	// for header file
