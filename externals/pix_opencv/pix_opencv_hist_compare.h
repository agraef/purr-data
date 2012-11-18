/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Histogram reognition object using Open CV

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_HIST_COMPARE_H_
#define INCLUDE_PIX_OPENCV_HIST_COMPARE_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

#define MAX_HISTOGRAMS_TO_COMPARE 80

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_hist_compare
    
    Histogram reognition object using Open CV

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/

class GEM_EXTERN pix_opencv_hist_compare : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_hist_compare, GemPixObj)

    public:

	//////////
	// Constructor
    	pix_opencv_hist_compare();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_hist_compare();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 

        void  saveMess(float index);

        int comp_xsize;
        int comp_ysize;

        t_outlet *m_dataout;
        t_outlet *m_measureout;

    private:
    
    	//////////
    	// Static member functions
        static void     saveMessCallback(void *data, t_floatarg index);

	// The output and temporary images
        int save_now;
        int nbsaved;

        CvHistogram *hist;
        CvHistogram *saved_hist[MAX_HISTOGRAMS_TO_COMPARE];
        IplImage *rgba, *rgb, *grey, *hsv, *h_plane, *s_plane, *v_plane, *h_saved_plane, *s_saved_plane, *v_saved_plane, *planes[2],*saved_planes[2];

	
};

#endif	// for header file
