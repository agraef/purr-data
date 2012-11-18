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

#ifndef INCLUDE_PIX_OPENCV_CONTOURS_CONVEXITY_H_
#define INCLUDE_PIX_OPENCV_CONTOURS_CONVEXITY_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_contours_convexhull
    
    Change pix to greyscale

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_contours_convexhull : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_contours_convexhull, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_contours_convexhull();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_contours_convexhull();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
	//////////
    	// Set the new edge threshold
    	void	    	floatMinAreaMess(float minarea);
    	void	    	floatMaxAreaMess(float maxarea);
    	// The new minimal/maximal area 
	int 		minarea;
	int 		maxarea;
	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;

    private:
    
	t_outlet 	*m_nomdef;
	t_outlet 	*m_dataout;
    	//////////
    	// Static member functions
    	static void 	floatMinAreaMessCallback(void *data, t_floatarg minarea);
    	static void 	floatMaxAreaMessCallback(void *data, t_floatarg maxarea);

	/////////
	// IplImage needed
    	IplImage 	*rgb, *orig, *gray;
	
};

#endif	// for header file
