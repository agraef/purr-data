/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Adaptive threshold object

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_ATHRESHOLD_H_
#define INCLUDE_PIX_OPENCV_ATHRESHOLD_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_athreshold
    
    Adaptive threshold object

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_athreshold : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_athreshold, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_athreshold();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_athreshold();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
	//////////
    	// Set the new edge threshold
    	void	    	floatMaxValueMess(float maxvalue);
    	void	    	floatModeMess(float mode);
    	void	    	floatMethodMess(float method);
    	void	    	floatBlockSizeMess(float blocksize);
    	void	    	floatDimMess(float dim);

    	// The new edge threshold
        int max_value;
        int x_threshold_mode;
        int x_threshold_method;
        int x_blocksize;
        int x_dim;

	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;

    private:
    
    	//////////
    	// Static member functions
    	static void    	floatMaxValueMessCallback(void *data, float maxvalue);
    	static void    	floatModeMessCallback(void *data, float mode);
    	static void    	floatMethodMessCallback(void *data, float method);
    	static void    	floatBlockSizeMessCallback(void *data, float blocksize);
    	static void   	floatDimMessCallback(void *data, float dim);

	/////////
	// IplImage needed
    	IplImage 	*rgba, *rgb, *gray;
};

#endif	// for header file
