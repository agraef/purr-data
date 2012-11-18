/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Simple distance classifier

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_KNEAR_H_
#define INCLUDE_PIX_OPENCV_KNEAR_H_

#ifndef _EiC
#include "cv.h"
#include "highgui.h"
#include "ml.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    
   pix_opencv_knear : OCR like pattern recognition
   based on basic OCR with Open CV tutorial
   by damiles : http://blog.damiles.com/?p=93    

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_knear : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_knear, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_knear(t_symbol *path, t_floatarg nsamples);
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_knear();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
        void findX(IplImage* imgSrc,int* min, int* max);
        void findY(IplImage* imgSrc,int* min, int* max);
        CvRect findBB(IplImage* imgSrc);
        IplImage preprocessing(IplImage* imgSrc,int new_width, int new_height);
        void load_patterns(void);

	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;
    

    private:
    
    	//////////
    	// Static member functions
    	static void 	bangMessCallback(void *data);
    	static void 	loadMessCallback(void *data, t_symbol *path, t_floatarg nsamples);

        // internal data
        t_outlet        *m_dataout;
        int             x_classify;

         // open cv classifier data
        char            *x_filepath;
        int             x_nsamples;
        int             x_rsamples;
        CvMat           *trainData;
        CvMat           *trainClasses;
        CvMat           *x_nearest;
        CvMat           *x_dist;
        int             x_pwidth;
        int             x_pheight;
        CvKNearest      *knn;

	// The output and temporary images
    	IplImage 	*rgba, *rgb, *grey;
	
};

#endif	// for header file
