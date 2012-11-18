/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Discrete Fourier Transform

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_DFT_H_
#define INCLUDE_PIX_OPENCV_DFT_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_dft
    
    Discrete Fourier Transform

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_dft : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_dft, GemPixObj)

    public:

	//////////
	// Constructor
    	pix_opencv_dft();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_dft();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 

        int comp_xsize;
        int comp_ysize;
        int x_calculate;
        int dft_M;
        int dft_N;

    private:
    
    	//////////
    	// Static member functions
    	static void 	calculateCallback(void *data);
        static void     shiftDFT(CvArr*, CvArr*);

	// The output and temporary images
        IplImage  *rgb;
        IplImage  *rgba;
        IplImage  *gray;
        IplImage  *input_re;
        IplImage  *input_im;
        IplImage  *input_co;
        CvMat     *dft_A;
        IplImage  *image_re;
        IplImage  *image_im;
        IplImage  *image_mout;
	
};

#endif	// for header file
