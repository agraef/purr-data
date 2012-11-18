/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Distrans algorithm

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_DISTRANS_H_
#define INCLUDE_PIX_OPENCV_DISTRANS_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_distrans
    
    Distrans algorithm

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_distrans : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_distrans, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_distrans();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_distrans();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
    	// Some varibales to control mophology voronoi
    	int 		edge_thresh;
    	int 		build_voronoi;
    	int 		mask_size;

	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;
    

    private:
    
    	//////////
    	// Static member functions
    	static void 	thresholdMessCallback(void *data, t_floatarg pos);
    	static void 	voronoiMessCallback(void *data, t_floatarg voronoi);
    	static void 	maskMessCallback(void *data, t_floatarg f);

	// The output and temporary images
    	IplImage 	*dist, *dist8u1, *dist8u2, *dist8u, *dist32s;
    	IplImage 	*rgb, *gray, *edge, *labels;
    	IplImage 	*rgba, *alpha;
	
};

#endif	// for header file
