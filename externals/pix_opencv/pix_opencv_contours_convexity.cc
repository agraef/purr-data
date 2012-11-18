////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// zmoelnig@iem.kug.ac.at
//
// Implementation file
//
//    Copyright (c) 1997-2000 Mark Danks.
//    Copyright (c) Günther Geiger.
//    Copyright (c) 2001-2002 IOhannes m zmoelnig. forum::für::umläute. IEM
//    Copyright (c) 2002 James Tittle & Chris Clepper
//    For information on usage and redistribution, and for a DISCLAIMER OF ALL
//    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "pix_opencv_contours_convexity.h"

CPPEXTERN_NEW(pix_opencv_contours_convexity)

/////////////////////////////////////////////////////////
//
// pix_opencv_contours_convexity
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_opencv_contours_convexity :: pix_opencv_contours_convexity()
{ 
  //inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("minarea"));
  //inlet_new(this->x_obj, &this->x_obj->ob_pd, gensym("float"), gensym("maxarea"));
  m_nomdef  = outlet_new(this->x_obj, 0);
  m_dataout = outlet_new(this->x_obj, 0);
  minarea = 1;
  maxarea = 320*240;
  comp_xsize  = 0;
  comp_ysize  = 0;
  orig = NULL;
  gray = NULL;
  rgb = NULL;

}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_opencv_contours_convexity :: ~pix_opencv_contours_convexity()
{ 
    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&rgb);
}

/////////////////////////////////////////////////////////
// processImage
//
/////////////////////////////////////////////////////////
void pix_opencv_contours_convexity :: processRGBAImage(imageStruct &image)
{
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!orig)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&rgb);

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);

    	// Create the output images with new sizes
    	rgb = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 3);

    	gray = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    
    }
    // Here we make a copy of the pixel data from image to orig->imageData
    // orig is a IplImage struct, the default image type in openCV, take a look on the IplImage data structure here
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( orig->imageData, image.data, image.xsize*image.ysize*4 );
    
    // Convert to grayscale
    cvCvtColor(orig, gray, CV_RGBA2GRAY);
    cvCvtColor(orig, rgb, CV_RGBA2RGB);

    CvSeq* seqhull;
    CvSeq* defects;
    CvSeq* contours;
    int* hull;
    int hullsize;
    CvPoint* PointArray;
    CvConvexityDefect* defectArray;
    CvMemStorage* stor02;
    CvMemStorage* stor03;
    stor02 = cvCreateMemStorage(0);
    stor03 = cvCreateMemStorage(0);


    cvFindContours( gray, stor02, &contours, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );
    if (contours) contours = cvApproxPoly( contours, sizeof(CvContour), stor02, CV_POLY_APPROX_DP, 3, 1 );
  
    int i = 0; 
    int area = 0;
    int selected = -1;
    
    //busquem el contorn mes gran
    CvSeq* first_contour;
    first_contour = contours;
    for( ; contours != 0; contours = contours->h_next )
        {
        CvRect rect;
	int count = contours->total; 
	rect = cvContourBoundingRect(contours, 1);
	if  ( (rect.width*rect.height) > area ) 
		{
		selected = i;
		area = rect.width*rect.height;
		}
	i++;
	}

    contours = first_contour;
	
    int k = 0;
    for( ; contours != 0; contours = contours->h_next )
        {
        int i;                   // Indicator of cycles.
        int count = contours->total; // This is number point in contour
        CvPoint center;
        CvSize size;
        CvRect rect;

	rect = cvContourBoundingRect( contours, 1);
	if ( (k==selected) ) {
        
        
    //fprintf(stderr,"malloc\n");
        // Alloc memory for contour point set.    
        PointArray = (CvPoint*)malloc( count*sizeof(CvPoint) );
                
        // Alloc memory for indices of convex hull vertices.
        hull = (int*)malloc(sizeof(int)*count);
        
        // Get contour point set.
    //fprintf(stderr,"cvCvtSeqToArray\n");
        cvCvtSeqToArray(contours, PointArray, CV_WHOLE_SEQ);
        
                
        // Find convex hull for curent contour.
    //fprintf(stderr,"cvConvexHull\n");
        cvConvexHull( PointArray,
                      count,
                      NULL,
                      CV_COUNTER_CLOCKWISE,
                      hull,
                      &hullsize);
        
        // Find convex hull for current contour.
        // This required for cvConvexityDefects().
    //fprintf(stderr,"cvConvexHull2\n");
        seqhull = cvConvexHull2( contours,0,
                             CV_COUNTER_CLOCKWISE,
                             0);
        
        // This required for cvConvexityDefects().
        // Otherwise cvConvexityDefects() falled.
        if( hullsize < 4 )
            continue;
         
        // Find defects of convexity of current contours.                        
    //fprintf(stderr,"cvConvexityDefects\n");
        defects = cvConvexityDefects( contours,
                            seqhull,
                            stor03);
        int j=0;
        // This cycle marks all defects of convexity of current contours.
        for(;defects;defects = defects->h_next)
        {
            int nomdef = defects->total; // defect amount
    	    outlet_float( m_nomdef, nomdef );
            
            if(nomdef == 0)
                continue;
             
            // Alloc memory for defect set.   
    //fprintf(stderr,"malloc\n");
            defectArray = (CvConvexityDefect*)malloc(sizeof(CvConvexityDefect)*nomdef);
            
            // Get defect set.
    //fprintf(stderr,"cvCvtSeqToArray\n");
            cvCvtSeqToArray(defects,defectArray, CV_WHOLE_SEQ);
            

            // Draw marks for all defects.
            for(i=0; i<nomdef; i++)
            {
                cvLine(rgb, *(defectArray[i].start), *(defectArray[i].depth_point),CV_RGB(0,0,255),1, CV_AA, 0 );
                cvCircle( rgb, *(defectArray[i].depth_point), 5, CV_RGB(0,255,0), -1, 8,0);
                cvCircle( rgb, *(defectArray[i].start), 5, CV_RGB(0,255,0), -1, 8,0);
                cvLine(rgb, *(defectArray[i].depth_point), *(defectArray[i].end),CV_RGB(0,0,255),1, CV_AA, 0 );
    	    t_atom rlist[7];
            SETFLOAT(&rlist[0], i);
            SETFLOAT(&rlist[1], defectArray[i].start->x);
            SETFLOAT(&rlist[2], defectArray[i].start->y);
            SETFLOAT(&rlist[3], defectArray[i].depth_point->x);
            SETFLOAT(&rlist[4], defectArray[i].depth_point->y);
            SETFLOAT(&rlist[5], defectArray[i].end->x);
            SETFLOAT(&rlist[6], defectArray[i].end->y);
    	    outlet_list( m_dataout, 0, 7, rlist );
            }

	    j++;
             
            // Free memory.       
            free(defectArray);
        }
        
        // Draw current contour.
        //cvDrawContours(x->cnt_img,contours,CV_RGB(255,255,255),CV_RGB(255,255,255),0,1, 8);
        cvDrawContours( rgb, contours, CV_RGB(255,0,0), CV_RGB(0,255,0), 2, 2, CV_AA, cvPoint(0,0)  );
        
        // Draw convex hull for current contour.        
        for(i=0; i<hullsize-1; i++)
        {
            cvLine(rgb, PointArray[hull[i]], 
                            PointArray[hull[i+1]],CV_RGB(255,255,255),1, CV_AA, 0 );
        }
        cvLine(rgb, PointArray[hull[hullsize-1]],
                             PointArray[hull[0]],CV_RGB(255,255,255),1, CV_AA, 0 );
        
          
        // Free memory.          
        free(PointArray);
        free(hull);
            /* replace CV_FILLED with 1 to see the outlines */
            //cvDrawContours( x->cnt_img, contours, CV_RGB(255,0,0), CV_RGB(0,255,0), x->levels, 3, CV_AA, cvPoint(0,0)  );
	    //cvConvexityDefects( contours, cvConvexHull2( contours, 0, CV_CLOCKWISE, 0 ), stor022 );
        }
	    k++;
        }

    cvReleaseMemStorage( &stor03 );
    cvReleaseMemStorage( &stor02 );
    //if (defects) cvClearSeq(defects);
    //if (seqhull) cvClearSeq(seqhull);

    cvCvtColor(rgb, orig, CV_RGB2RGBA);
    //copy back the processed frame to image
    memcpy( image.data, orig->imageData, image.xsize*image.ysize*4 );
}

void pix_opencv_contours_convexity :: processRGBImage(imageStruct &image)
{
  unsigned char *pixels = image.data;

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!rgb)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&rgb);

	//create the orig image with new size
        rgb = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes

    	gray = cvCreateImage(cvSize(rgb->width,rgb->height), IPL_DEPTH_8U, 1);
    
    }
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( rgb->imageData, image.data, image.xsize*image.ysize*3 );
    
    // Convert to grayscale
    cvCvtColor(rgb, gray, CV_RGB2GRAY);
  

    CvSeq* seqhull;
    CvSeq* defects;
    CvSeq* contours;
    int* hull;
    int hullsize;
    CvPoint* PointArray;
    CvConvexityDefect* defectArray;
    CvMemStorage* stor02;
    CvMemStorage* stor03;
    stor02 = cvCreateMemStorage(0);
    stor03 = cvCreateMemStorage(0);


    cvFindContours( gray, stor02, &contours, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );
    if (contours) contours = cvApproxPoly( contours, sizeof(CvContour), stor02, CV_POLY_APPROX_DP, 3, 1 );
  
    int i = 0; 
    int area = 0;
    int selected = -1;
    
    //busquem el contorn mes gran
    CvSeq* first_contour;
    first_contour = contours;
    for( ; contours != 0; contours = contours->h_next )
        {
        CvRect rect;
	int count = contours->total; 
	rect = cvContourBoundingRect(contours, 1);
	if  ( (rect.width*rect.height) > area ) 
		{
		selected = i;
		area = rect.width*rect.height;
		}
	i++;
	}

    contours = first_contour;
	
    int k = 0;
    for( ; contours != 0; contours = contours->h_next )
        {
        int i;                   // Indicator of cycles.
        int count = contours->total; // This is number point in contour
        CvPoint center;
        CvSize size;
        CvRect rect;

	rect = cvContourBoundingRect( contours, 1);
	if ( (k==selected) ) {
        
        
    //fprintf(stderr,"malloc\n");
        // Alloc memory for contour point set.    
        PointArray = (CvPoint*)malloc( count*sizeof(CvPoint) );
                
        // Alloc memory for indices of convex hull vertices.
        hull = (int*)malloc(sizeof(int)*count);
        
        // Get contour point set.
    //fprintf(stderr,"cvCvtSeqToArray\n");
        cvCvtSeqToArray(contours, PointArray, CV_WHOLE_SEQ);
        
                
        // Find convex hull for curent contour.
    //fprintf(stderr,"cvConvexHull\n");
        cvConvexHull( PointArray,
                      count,
                      NULL,
                      CV_COUNTER_CLOCKWISE,
                      hull,
                      &hullsize);
        
        // Find convex hull for current contour.
        // This required for cvConvexityDefects().
    //fprintf(stderr,"cvConvexHull2\n");
        seqhull = cvConvexHull2( contours,0,
                             CV_COUNTER_CLOCKWISE,
                             0);
        
        // This required for cvConvexityDefects().
        // Otherwise cvConvexityDefects() falled.
        if( hullsize < 4 )
            continue;
         
        // Find defects of convexity of current contours.                        
    //fprintf(stderr,"cvConvexityDefects\n");
        defects = cvConvexityDefects( contours,
                            seqhull,
                            stor03);
        int j=0;
        // This cycle marks all defects of convexity of current contours.
        for(;defects;defects = defects->h_next)
        {
            int nomdef = defects->total; // defect amount
    	    outlet_float( m_nomdef, nomdef );
            
            if(nomdef == 0)
                continue;
             
            // Alloc memory for defect set.   
    //fprintf(stderr,"malloc\n");
            defectArray = (CvConvexityDefect*)malloc(sizeof(CvConvexityDefect)*nomdef);
            
            // Get defect set.
    //fprintf(stderr,"cvCvtSeqToArray\n");
            cvCvtSeqToArray(defects,defectArray, CV_WHOLE_SEQ);
            

            // Draw marks for all defects.
            for(i=0; i<nomdef; i++)
            {
                cvLine(rgb, *(defectArray[i].start), *(defectArray[i].depth_point),CV_RGB(0,0,255),1, CV_AA, 0 );
                cvCircle( rgb, *(defectArray[i].depth_point), 5, CV_RGB(0,255,0), -1, 8,0);
                cvCircle( rgb, *(defectArray[i].start), 5, CV_RGB(0,255,0), -1, 8,0);
                cvLine(rgb, *(defectArray[i].depth_point), *(defectArray[i].end),CV_RGB(0,0,255),1, CV_AA, 0 );
    	    t_atom rlist[7];
            SETFLOAT(&rlist[0], i);
            SETFLOAT(&rlist[1], defectArray[i].start->x);
            SETFLOAT(&rlist[2], defectArray[i].start->y);
            SETFLOAT(&rlist[3], defectArray[i].depth_point->x);
            SETFLOAT(&rlist[4], defectArray[i].depth_point->y);
            SETFLOAT(&rlist[5], defectArray[i].end->x);
            SETFLOAT(&rlist[6], defectArray[i].end->y);
    	    outlet_list( m_dataout, 0, 7, rlist );
            }

	    j++;
             
            // Free memory.       
            free(defectArray);
        }
        
        // Draw current contour.
        //cvDrawContours(x->cnt_img,contours,CV_RGB(255,255,255),CV_RGB(255,255,255),0,1, 8);
        cvDrawContours( rgb, contours, CV_RGB(255,0,0), CV_RGB(0,255,0), 2, 2, CV_AA, cvPoint(0,0)  );
        
        // Draw convex hull for current contour.        
        for(i=0; i<hullsize-1; i++)
        {
            cvLine(rgb, PointArray[hull[i]], 
                            PointArray[hull[i+1]],CV_RGB(255,255,255),1, CV_AA, 0 );
        }
        cvLine(rgb, PointArray[hull[hullsize-1]],
                             PointArray[hull[0]],CV_RGB(255,255,255),1, CV_AA, 0 );
        
          
        // Free memory.          
        free(PointArray);
        free(hull);
            /* replace CV_FILLED with 1 to see the outlines */
            //cvDrawContours( x->cnt_img, contours, CV_RGB(255,0,0), CV_RGB(0,255,0), x->levels, 3, CV_AA, cvPoint(0,0)  );
	    //cvConvexityDefects( contours, cvConvexHull2( contours, 0, CV_CLOCKWISE, 0 ), stor022 );
        }
	    k++;
        }

    cvReleaseMemStorage( &stor03 );
    cvReleaseMemStorage( &stor02 );
    //if (defects) cvClearSeq(defects);
    //if (seqhull) cvClearSeq(seqhull);

    //cvShowImage(wndname, cedge);
    memcpy( image.data, rgb->imageData, image.xsize*image.ysize*3 );
}

void pix_opencv_contours_convexity :: processYUVImage(imageStruct &image)
{
  post( "pix_opencv_contours_convexity : yuv format not supported" );
}
    	
void pix_opencv_contours_convexity :: processGrayImage(imageStruct &image)
{ 

  if ((this->comp_xsize!=image.xsize)||(this->comp_ysize!=image.ysize)||(!orig)) {

	this->comp_xsize = image.xsize;
	this->comp_ysize = image.ysize;

    	//Destroy cv_images to clean memory
	cvReleaseImage(&orig);
    	cvReleaseImage(&gray);
    	cvReleaseImage(&rgb);

	//create the orig image with new size
        orig = cvCreateImage(cvSize(image.xsize,image.ysize), IPL_DEPTH_8U, 4);

    	// Create the output images with new sizes
    	rgb = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 3);

    	gray = cvCreateImage(cvSize(orig->width,orig->height), IPL_DEPTH_8U, 1);
    
    }
    // Here we make a copy of the pixel data from image to orig->imageData
    // orig is a IplImage struct, the default image type in openCV, take a look on the IplImage data structure here
    // http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html 
    memcpy( gray->imageData, image.data, image.xsize*image.ysize );
    cvCvtColor(gray, rgb, CV_GRAY2RGB);
    
    CvSeq* seqhull;
    CvSeq* defects;
    CvSeq* contours;
    int* hull;
    int hullsize;
    CvPoint* PointArray;
    CvConvexityDefect* defectArray;
    CvMemStorage* stor02;
    CvMemStorage* stor03;
    stor02 = cvCreateMemStorage(0);
    stor03 = cvCreateMemStorage(0);


    cvFindContours( gray, stor02, &contours, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );
    if (contours) contours = cvApproxPoly( contours, sizeof(CvContour), stor02, CV_POLY_APPROX_DP, 3, 1 );
  
    int i = 0; 
    int area = 0;
    int selected = -1;
    
    //busquem el contorn mes gran
    CvSeq* first_contour;
    first_contour = contours;
    for( ; contours != 0; contours = contours->h_next )
        {
        CvRect rect;
	int count = contours->total; 
	rect = cvContourBoundingRect(contours, 1);
	if  ( (rect.width*rect.height) > area ) 
		{
		selected = i;
		area = rect.width*rect.height;
		}
	i++;
	}

    contours = first_contour;
	
    int k = 0;
    for( ; contours != 0; contours = contours->h_next )
        {
        int i;                   // Indicator of cycles.
        int count = contours->total; // This is number point in contour
        CvPoint center;
        CvSize size;
        CvRect rect;

	rect = cvContourBoundingRect( contours, 1);
	if ( (k==selected) ) {
        
        
    //fprintf(stderr,"malloc\n");
        // Alloc memory for contour point set.    
        PointArray = (CvPoint*)malloc( count*sizeof(CvPoint) );
                
        // Alloc memory for indices of convex hull vertices.
        hull = (int*)malloc(sizeof(int)*count);
        
        // Get contour point set.
    //fprintf(stderr,"cvCvtSeqToArray\n");
        cvCvtSeqToArray(contours, PointArray, CV_WHOLE_SEQ);
        
                
        // Find convex hull for curent contour.
    //fprintf(stderr,"cvConvexHull\n");
        cvConvexHull( PointArray,
                      count,
                      NULL,
                      CV_COUNTER_CLOCKWISE,
                      hull,
                      &hullsize);
        
        // Find convex hull for current contour.
        // This required for cvConvexityDefects().
    //fprintf(stderr,"cvConvexHull2\n");
        seqhull = cvConvexHull2( contours,0,
                             CV_COUNTER_CLOCKWISE,
                             0);
        
        // This required for cvConvexityDefects().
        // Otherwise cvConvexityDefects() falled.
        if( hullsize < 4 )
            continue;
         
        // Find defects of convexity of current contours.                        
    //fprintf(stderr,"cvConvexityDefects\n");
        defects = cvConvexityDefects( contours,
                            seqhull,
                            stor03);
        int j=0;
        // This cycle marks all defects of convexity of current contours.
        for(;defects;defects = defects->h_next)
        {
            int nomdef = defects->total; // defect amount
    	    outlet_float( m_nomdef, nomdef );
            
            if(nomdef == 0)
                continue;
             
            // Alloc memory for defect set.   
    //fprintf(stderr,"malloc\n");
            defectArray = (CvConvexityDefect*)malloc(sizeof(CvConvexityDefect)*nomdef);
            
            // Get defect set.
    //fprintf(stderr,"cvCvtSeqToArray\n");
            cvCvtSeqToArray(defects,defectArray, CV_WHOLE_SEQ);
            

            // Draw marks for all defects.
            for(i=0; i<nomdef; i++)
            {
                cvLine(rgb, *(defectArray[i].start), *(defectArray[i].depth_point),CV_RGB(0,0,255),1, CV_AA, 0 );
                cvCircle( rgb, *(defectArray[i].depth_point), 5, CV_RGB(0,255,0), -1, 8,0);
                cvCircle( rgb, *(defectArray[i].start), 5, CV_RGB(0,255,0), -1, 8,0);
                cvLine(rgb, *(defectArray[i].depth_point), *(defectArray[i].end),CV_RGB(0,0,255),1, CV_AA, 0 );
    	    t_atom rlist[7];
            SETFLOAT(&rlist[0], i);
            SETFLOAT(&rlist[1], defectArray[i].start->x);
            SETFLOAT(&rlist[2], defectArray[i].start->y);
            SETFLOAT(&rlist[3], defectArray[i].depth_point->x);
            SETFLOAT(&rlist[4], defectArray[i].depth_point->y);
            SETFLOAT(&rlist[5], defectArray[i].end->x);
            SETFLOAT(&rlist[6], defectArray[i].end->y);
    	    outlet_list( m_dataout, 0, 7, rlist );
            }

	    j++;
             
            // Free memory.       
            free(defectArray);
        }
        
        // Draw current contour.
        //cvDrawContours(x->cnt_img,contours,CV_RGB(255,255,255),CV_RGB(255,255,255),0,1, 8);
        cvDrawContours( rgb, contours, CV_RGB(255,0,0), CV_RGB(0,255,0), 2, 2, CV_AA, cvPoint(0,0)  );
        
        // Draw convex hull for current contour.        
        for(i=0; i<hullsize-1; i++)
        {
            cvLine(rgb, PointArray[hull[i]], 
                            PointArray[hull[i+1]],CV_RGB(255,255,255),1, CV_AA, 0 );
        }
        cvLine(rgb, PointArray[hull[hullsize-1]],
                             PointArray[hull[0]],CV_RGB(255,255,255),1, CV_AA, 0 );
        
          
        // Free memory.          
        free(PointArray);
        free(hull);
            /* replace CV_FILLED with 1 to see the outlines */
            //cvDrawContours( x->cnt_img, contours, CV_RGB(255,0,0), CV_RGB(0,255,0), x->levels, 3, CV_AA, cvPoint(0,0)  );
	    //cvConvexityDefects( contours, cvConvexHull2( contours, 0, CV_CLOCKWISE, 0 ), stor022 );
        }
	    k++;
        }

    cvReleaseMemStorage( &stor03 );
    cvReleaseMemStorage( &stor02 );
    //if (defects) cvClearSeq(defects);
    //if (seqhull) cvClearSeq(seqhull);

    cvCvtColor(rgb, gray, CV_RGB2GRAY);

    //copy back the processed frame to image
    memcpy( image.data, gray->imageData, image.xsize*image.ysize );
}

/////////////////////////////////////////////////////////
// floatThreshMess
//
/////////////////////////////////////////////////////////
void pix_opencv_contours_convexity :: floatMinAreaMess (float minarea)
{
  if (minarea>0) this->minarea = (int)minarea;
}
void pix_opencv_contours_convexity :: floatMaxAreaMess (float maxarea)
{
  if (maxarea>0) this->maxarea = (int)maxarea;
}

/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_opencv_contours_convexity :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_convexity::floatMinAreaMessCallback,
  		  gensym("minarea"), A_FLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_opencv_contours_convexity::floatMaxAreaMessCallback,
  		  gensym("maxarea"), A_FLOAT, A_NULL);
}
void pix_opencv_contours_convexity :: floatMaxAreaMessCallback(void *data, t_floatarg maxarea)
{
  GetMyClass(data)->floatMaxAreaMess((float)maxarea);
}
void pix_opencv_contours_convexity :: floatMinAreaMessCallback(void *data, t_floatarg minarea)
{
  GetMyClass(data)->floatMinAreaMess((float)minarea);
}
