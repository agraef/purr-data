/*-----------------------------------------------------------------
LOG
    GEM - Graphics Environment for Multimedia

    Contours Bounding Rectangle detection

    Copyright (c) 1997-1999 Mark Danks. mark@danks.org
    Copyright (c) Günther Geiger. geiger@epy.co.at
    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM. zmoelnig@iem.kug.ac.at
    Copyright (c) 2002 James Tittle & Chris Clepper
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

-----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_OPENCV_CONTOURS_BOUNDINGRECT_H_
#define INCLUDE_PIX_OPENCV_CONTOURS_BOUNDINGRECT_H_

#ifndef _EiC
#include "cv.h"
#endif

#include "Base/GemPixObj.h"

#define MAX_MARKERS 500

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_opencv_contours_boundingrect
    
    Contours Bounding Rectangle detection

KEYWORDS
    pix
    
DESCRIPTION
   
-----------------------------------------------------------------*/
class GEM_EXTERN pix_opencv_contours_boundingrect : public GemPixObj
{
    CPPEXTERN_HEADER(pix_opencv_contours_boundingrect, GemPixObj)

    public:

	    //////////
	    // Constructor
    	pix_opencv_contours_boundingrect();
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_opencv_contours_boundingrect();

    	//////////
    	// Do the processing
    	virtual void 	processRGBAImage(imageStruct &image);
    	virtual void 	processRGBImage(imageStruct &image);
	virtual void 	processYUVImage(imageStruct &image);
    	virtual void 	processGrayImage(imageStruct &image); 
    	
	//////////
    	// Set the new edge threshold
    	void	    	floatMinAreaMess(float minarea);
    	void	    	floatMaxAreaMess(float maxarea);
    	void	    	floatFToleranceMess(float maxarea);
    	void	    	floatMMoveMess(float maxarea);
    	void	    	floatCModeMess(float maxarea);
    	void	    	floatCMethodMess(float maxarea);
    	void	    	floatClearMess(void);
    	void	    	floatNightmodeMess(float nightmode);
    	void	    	floatShowMess(float show);
    	void	    	floatDrawMess(float draw);
        int             mark(float fx, float fy, float fw, float fh );
    	void	    	deleteMark(float findex);
    	// The new minimal/maximal area 
	int 		minarea;
	int 		maxarea;
	// to detect changes in the image size
	int 		comp_xsize;
	int		comp_ysize;
        float           x_xmark[MAX_MARKERS];
        float           x_ymark[MAX_MARKERS];
        int             x_wmark[MAX_MARKERS];
        int             x_hmark[MAX_MARKERS];
        int             x_found[MAX_MARKERS];
        int             x_ftolerance;
        int             x_mmove;
        int             x_nightmode;
        int             x_draw;
        int             x_show;

        // contours retrieval mode
        int             x_cmode;
        // contours retrieval method
        int             x_cmethod;

    private:
    
	t_outlet 	*m_dataout;
	t_outlet 	*m_countout;
    	//////////
    	// Static member functions
    	static void 	floatMinAreaMessCallback(void *data, t_floatarg minarea);
    	static void 	floatMaxAreaMessCallback(void *data, t_floatarg maxarea);
    	static void 	floatFToleranceMessCallback(void *data, t_floatarg ftolerance);
    	static void 	floatMMoveMessCallback(void *data, t_floatarg mmove);
    	static void 	floatCModeMessCallback(void *data, t_floatarg cmode);
    	static void 	floatCMethodMessCallback(void *data, t_floatarg cmethod);
    	static void 	floatClearMessCallback(void *data);
    	static void 	floatNightmodeMessCallback(void *data, t_floatarg nightmode);
    	static void 	floatShowMessCallback(void *data, t_floatarg show);
    	static void 	floatDrawMessCallback(void *data, t_floatarg draw);

	/////////
	// IplImage needed
    	IplImage 	*rgb, *orig, *cnt_img, *gray;
        CvFont font;
	
};

#endif	// for header file
