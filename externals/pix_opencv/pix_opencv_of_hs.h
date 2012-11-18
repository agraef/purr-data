/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Horn and Schunck Optical Flow algorithm

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_OF_HS_H_
#define INCLUDE_PIX_OPENCV_OF_HS_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_of_hs
    
    Horn and Schunck Optical Flow algorithm

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_of_hs : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_of_hs, GemPixObj)

    public:

	//////////
	// Constructor
    	pix_opencv_of_hs();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_of_hs();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 

        void  nightModeMess(float nightmode);
        void  tresholdMess(float threshold);
        void  lambdaMess(float lambda);
        void  usePreviousMess(float previous);
        void  minBlocksMess(float mblocks);

        int comp_xsize;
        int comp_ysize;

        t_outlet *m_meanout;
        t_outlet *m_maxout;

        CvSize x_velsize;
        double x_lambda;

        int x_nightmode;
        int x_threshold;
        int x_useprevious;
        int x_minblocks;

    private:
    
    	//////////
    	// Static member functions
        static void  nightModeMessCallback(void *data, float nightmode);
        static void  tresholdMessCallback(void *data, float threshold);
        static void  lambdaMessCallback(void *data, float lambda);
        static void  usePreviousMessCallback(void *data, float previous);
        static void  minBlocksMessCallback(void *data, float mblocks);

	// Internal Open CV data
        IplImage *rgba, *rgb, *grey, *prev_grey, *swap_temp;
        IplImage *x_velx, *x_vely;
        CvFont font;

        t_atom x_list[3];
};

#endif	// for header file
