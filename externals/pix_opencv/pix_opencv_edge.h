/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Edge detection

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_EDGE_H_
#define INCLUDE_PIX_OPENCV_EDGE_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_edge
    
    Edge detection

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_edge : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_edge, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_edge();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_edge();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
	//////////
    	// Set the new edge threshold
    	void	    	floatThreshMess(float edge_thresh);
    	// The new edge threshold
	int 		edge_thresh;
	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;

    private:
    
    	//////////
    	// Static member functions
    	static void 	floatTreshMessCallback(void *data, t_floatarg edge_thresh);

	/////////
	// IplImage needed
    	IplImage 	*rgb, *orig, *cedge, *cedgergb, *gray, *edge;
	
};

#endif	// for header file
