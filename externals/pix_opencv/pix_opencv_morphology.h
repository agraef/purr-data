/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Morphology filter

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_MORPHOLOGY_H_
#define INCLUDE_PIX_OPENCV_MORPHOLOGY_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_morphology
    
    Morphology filter

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_morphology : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_morphology, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_morphology();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_morphology();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
	//////////
    	// Set the new pos
    	void	    	floatPosMess(float pos);
    	// Some varibales to control mophology mode
    	int 		pos;
    	int 		element_shape;
    	int 		mode; //to switch between openclose or dilateerode modes
	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;

    private:
    
    	//////////
    	// Static member functions
    	static void 	floatPosMessCallback(void *data, t_floatarg pos);
    	static void 	modeMessCallback(void *data, t_floatarg mode);
    	static void 	shapeMessCallback(void *data, t_floatarg f);

    	// The output and temporary images
    	IplImage	*rgba, *grey, *rgb, *dst;
	
    	IplConvKernel	*element;
	
};

#endif	// for header file
