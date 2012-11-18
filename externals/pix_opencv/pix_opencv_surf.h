/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    SURF point detection and tracking

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_SURF_H_
#define INCLUDE_PIX_OPENCV_SURF_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

#define MAX_MARKERS 500
#define DSCSIZE 128

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_surf
    
    SURF point detection and tracking

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_surf : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_surf, GemPixObj)

    public:

	//////////
	// Constructor
    	pix_opencv_surf();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_surf();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 

        void  nightModeMess(float nightmode);
        void  hessianMess(float hessian);
        void  markMess(int, t_atom*);
        void  deleteMess(float index);
        void  clearMess(void);
        void  maxMoveMess(float maxmove);
        void  ftoleranceMess(float ftolerance);
        void  delaunayMess(t_symbol *s);
        void  pdelaunayMess(float fpoint, float fthreshold);

        int comp_xsize;
        int comp_ysize;

        t_outlet *m_dataout;
        int x_hessian;
        int x_criteria;
        int night_mode;
        int x_maxmove;
        int x_markall;
        int x_ftolerance;
        int x_delaunay;
        int x_threshold;

    private:
    
    	//////////
    	// Static member functions
        static void  nightModeMessCallback(void *data, t_floatarg nightmode);
        static void  hessianMessCallback(void *data, t_floatarg hessian);
        static void  markMessCallback(void *data, t_symbol* name, int argc, t_atom* argv);
        static void  deleteMessCallback(void *data, t_floatarg index);
        static void  clearMessCallback(void *data);
        static void  maxMoveMessCallback(void *data, t_floatarg maxmove);
        static void  ftoleranceMessCallback(void *data, t_floatarg ftolerance);
        static void  delaunayMessCallback(void *data, t_symbol *s);
        static void  pdelaunayMessCallback(void *data, t_floatarg fpoint, t_floatarg fthreshold);

	// Internal Open CV data
        IplImage *orgb, *rgba, *rgb, *gray, *ogray;
        t_atom x_list[3];

        int x_xmark[MAX_MARKERS];
        int x_ymark[MAX_MARKERS];
        float x_rdesc[MAX_MARKERS][DSCSIZE];
        int x_found[MAX_MARKERS];

        // internal OpenCV structures
        CvSeq *objectKeypoints, *objectDescriptors;
        CvFont font;

        // structures needed for the delaunay
        CvRect x_fullrect;
        CvMemStorage* x_storage;
        CvSubdiv2D* x_subdiv;
};

#endif	// for header file
