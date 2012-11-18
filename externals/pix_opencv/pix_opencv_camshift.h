/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Continously adaptive mean-shift tracker

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_CAMSHIFT_H_
#define INCLUDE_PIX_OPENCV_CAMSHIFT_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

#define MAX_MARKERS 500
const int MAX_COUNT = 500;

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_camshift
    
    Continously adaptive mean-shift tracker

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_camshift : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_camshift, GemPixObj)

    public:

	//////////
	// Constructor
    	pix_opencv_camshift();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_camshift();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 

        void  backProjectMess(float backproject);
        void  vMinMess(float vmin);
        void  vMaxMess(float vmax);
        void  sMinMess(float smin);
        void  trackMess(float px, float py);
        void  rWidthMess(float rwidth);
        void  rHeightMess(float rheight);

        int comp_xsize;
        int comp_ysize;

        t_outlet *m_dataout;
        int     x_track;
        int     x_init;
        int     x_rwidth;
        int     x_rheight;
        int     x_backproject;
        int     x_vmin;
        int     x_vmax;
        int     x_smin;

    private:
    
    	//////////
    	// Static member functions
        static void  backProjectMessCallback(void *data, float backproject);
        static void  vMinMessCallback(void *data, float vmin);
        static void  vMaxMessCallback(void *data, float vmax);
        static void  sMinMessCallback(void *data, float smin);
        static void  trackMessCallback(void *data, float px, float py);
        static void  rWidthMessCallback(void *data, float rwidth);
        static void  rHeightMessCallback(void *data, float rheight);

	// Internal Open CV data
        IplImage *rgba, *rgb, *gray, *hsv, *hue, *mask, *backproject;
        CvHistogram *hist;
        CvPoint origin;
        CvRect selection;
        CvRect trackwindow;
        CvBox2D trackbox;
        CvConnectedComp trackcomp;

        t_atom x_list[5];
};

#endif	// for header file
