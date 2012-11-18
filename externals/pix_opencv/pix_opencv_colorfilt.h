/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Color filter

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_COLORFILT_H_
#define INCLUDE_PIX_OPENCV_COLORFILT_H_

#include <stdio.h>

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_colorfilt
    
    Color filter object

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_colorfilt : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_colorfilt, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_colorfilt();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_colorfilt();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
	//////////
    	// Set the new edge threshold
    	void	    	floatToleranceMess(float tolerance);
    	void	    	floatRMess(float r);
    	void	    	floatGMess(float g);
    	void	    	floatBMess(float b);
    	void	    	pickMess(float xcur, float ycur);
    	void	    	drawColor(void);

    	// The color tolerance
        int x_tolerance;
        unsigned char x_colorR; // RGB components of binary mask
        unsigned char x_colorG;
        unsigned char x_colorB;

        t_outlet *x_R;  // output R component of selected color
        t_outlet *x_G;  // output G component of selected color
        t_outlet *x_B;  // output B component of selected color

        t_canvas *x_canvas;

	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;

    private:
    
    	//////////
    	// Static member functions
    	static void    	floatToleranceMessCallback(void *data, float tolerance);
    	static void    	floatRMessCallback(void *data, float r);
    	static void    	floatGMessCallback(void *data, float g);
    	static void    	floatBMessCallback(void *data, float b);
    	static void   	pickMessCallback(void *data, float xcur, float ycur);

	/////////
	// IplImage needed
    	IplImage 	*rgba, *rgb, *brgb;
};

#endif	// for header file
