/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Laplace transform / edge detection

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_LAPLACE_H_
#define INCLUDE_PIX_OPENCV_LAPLACE_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_laplace
    
    Laplace transform / edge detection

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_laplace : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_laplace, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_laplace();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_laplace();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
	//////////
    	// Set the new aperture_size
    	void	    	floatApertureMess(float aperture_size);
    	// The new aperture size
	int 		aperture_size;
	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;

    private:
    
    	//////////
    	// Static member functions
    	static void 	floatApertureMessCallback(void *data, t_floatarg aperture_size);

	/////////
	// IplImage needed
    	IplImage 	*rgb, *rgba, *grey, *laplace, *colorlaplace, *planes[3];
	
};

#endif	// for header file
