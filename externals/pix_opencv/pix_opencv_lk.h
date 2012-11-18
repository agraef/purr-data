/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    LK point detection and tracking

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_LK_H_
#define INCLUDE_PIX_OPENCV_LK_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

#define MAX_MARKERS 500
const int MAX_COUNT = 500;

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_lk
    
    LK point detection and tracking

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_lk : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_lk, GemPixObj)

    public:

	//////////
	// Constructor
    	pix_opencv_lk();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_lk();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 

        void  winSizeMess(float winsize);
        void  nightModeMess(float nightmode);
        void  qualityMess(float quality);
        void  initMess(void);
        void  markMess(int, t_atom*);
        void  deleteMess(float index);
        void  clearMess(void);
        void  minDistanceMess(float mindistance);
        void  maxMoveMess(float maxmove);
        void  ftoleranceMess(float ftolerance);
        void  delaunayMess(t_symbol *s);
        void  pdelaunayMess(float fpoint, float fthreshold);

        int comp_xsize;
        int comp_ysize;

        t_outlet *m_dataout;
        t_atom x_list[3];
        int win_size;
        double quality;
        int min_distance;
        int night_mode;
        int maxmove;
        int markall;
        int ftolerance;
        int delaunay;
        int threshold;

    private:
    
    	//////////
    	// Static member functions
        static void  winSizeMessCallback(void *data, t_floatarg winsize);
        static void  nightModeMessCallback(void *data, t_floatarg nightmode);
        static void  qualityMessCallback(void *data, t_floatarg quality);
        static void  initMessCallback(void *data);
        static void  markMessCallback(void *data, t_symbol* name, int argc, t_atom* argv);
        static void  deleteMessCallback(void *data, t_floatarg index);
        static void  clearMessCallback(void *data);
        static void  minDistanceMessCallback(void *data, t_floatarg mindistance);
        static void  maxMoveMessCallback(void *data, t_floatarg maxmove);
        static void  ftoleranceMessCallback(void *data, t_floatarg ftolerance);
        static void  delaunayMessCallback(void *data, t_symbol *s);
        static void  pdelaunayMessCallback(void *data, t_floatarg fpoint, t_floatarg fthreshold);

	// Internal Open CV data
        IplImage *rgba, *rgb, *orgb, *gray, *ogray, *prev_gray, *pyramid, *prev_pyramid, *swap_temp;
        int x_xmark[MAX_MARKERS];
        int x_ymark[MAX_MARKERS];
        int x_found[MAX_MARKERS];
        CvPoint2D32f* points[2], *swap_points;
        char* status;
        int count;
        int need_to_init;
        int flags;
        int add_remove_pt;
        CvPoint pt;
        CvFont font;

        // structures needed for the delaunay
        CvRect x_fullrect;
        CvMemStorage* x_storage;
        CvSubdiv2D* x_subdiv;
	
};

#endif	// for header file
