/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Threshold filter

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_THRESHOLD_H_
#define INCLUDE_PIX_OPENCV_THRESHOLD_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_threshold
    
    Threshold filter

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_threshold : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_threshold, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_threshold();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_threshold();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
	//////////
    	// Set the new edge threshold
    	void	    	floatMaxMess(float maxvalue);
    	void	    	floatThreshMess(float edge_thresh);
    	void	    	floatModeMess(float mode);

    	// The new edge threshold
	float 		threshold_value;
    	float       	max_value;
    	int 		threshold_mode;

	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;

    private:
    
    	//////////
    	// Static member functions
    	static void 	floatMaxMessCallback(void *data, t_floatarg maxvalue);
    	static void 	floatThreshMessCallback(void *data, t_floatarg thresh_value);
    	static void 	floatModeMessCallback(void *data, t_floatarg thresh_mode_value);

	/////////
	// IplImage needed
    	IplImage 	*orig, *rgb, *gray;
};

#endif	// for header file
