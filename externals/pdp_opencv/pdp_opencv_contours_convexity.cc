/*
 *   Pure Data Packet module.
 *   Copyright (c) by Tom Schouten <pdp@zzz.kotnet.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <dlfcn.h>
#include <math.h>

#include "pdp.h"

#ifndef _EiC
#include "cv.h"
#endif

#define w 500


typedef struct pdp_opencv_contours_convexity_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_nomdef;
    t_outlet *x_dataout;
    int x_packet0;
    int x_packet1;
    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;

    int x_infosok; 

    int levels;
    int area;
    int minarea;
    int maxarea;
    int selected;
    IplImage *image, *gray;

} t_pdp_opencv_contours_convexity;

static void pdp_opencv_contours_convexity_process_rgb(t_pdp_opencv_contours_convexity *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
      

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_opencv_contours_convexity :: resizing plugins");
	
    	//cv_freeplugins(x);

    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->gray);
    
	//create the orig image with new size
        x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);

    	// Create the output images with new sizes
    	x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);
    
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );
    
    // FEM UNA COPIA DEL PACKET A image->imageData ... http://www.cs.iit.edu/~agam/cs512/lect-notes/opencv-intro/opencv-intro.html aqui veiem la estructura de IplImage
    memcpy( x->image->imageData, data, x->x_size*3 );
    
    // Convert to grayscale
    cvCvtColor(x->image, x->gray, CV_BGR2GRAY);

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

    //TODO nous objectes ::: llegeixo el OpenCVRefenceManual i al capitol 11 Structural Analysis Reference
    // m'en adono que
    //de fet aquest objecte no s'ha de dir pdp_opencv_contours_convexity sino pdp_opencv_convexity
    //el pdp_opencv_contours_convexity et donaria una llista de punts en els outles que serien els punts del contorn (poligonal o no)i
    //i seria la base per a una serie de objectes basats en contorns
    //el pdp_opencv_convexHull, el  mateix pdp_opencv_convexity
    //depres nhi ha un altre que surtiria d'aqui :: pdp_opencv_MinAreaRect i el pdp_opencv_MinEnclosingCircle
    //ContourBoundingRect
    //

    
    // TODO afegir parametres
    // Retrieval mode.
    // CV_RETR_TREE || CV_RETR_CCOMP || CV_RETR_LIST || CV_RETR_EXTERNAL
    // Approximation method.
    // CV_CHAIN_APPROX_SIMPLE || CV_CHAIN_CODE || CV_CHAIN_APPROX_NONE || CV_CHAIN_APPROX_TC89_L1 || CV_CHAIN_APPROX_TC89_KCOS || CV_LINK_RUNS
    cvFindContours( x->gray, stor02, &contours, sizeof(CvContour), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cvPoint(0,0) );
    // TODO afegir parametres
    // aqui es fa una aproximacio del contorn per a que sigui mes polinomic i no tingui tants punts
    // els ultims dos parametres han de ser variables
    //           precision , recursive
    if (contours) contours = cvApproxPoly( contours, sizeof(CvContour), stor02, CV_POLY_APPROX_DP, 3, 1 );

    int i = 0; 
    
    //busquem el contorn mes gran
    x->area = 0;
    CvSeq* first_contour;
    first_contour = contours;
    for( ; contours != 0; contours = contours->h_next )
        {
        CvRect rect;
	int count = contours->total; 
	rect = cvContourBoundingRect(contours, 1);
	if  ( (rect.width*rect.height) > x->area ) 
		{
		x->selected = i;
		x->area = rect.width*rect.height;
		}
	i++;
	}

    //cvSeqSort(contours, sort_contour, NULL);
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
	if ( (k==x->selected) ) {
        
        
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
    	    outlet_float( x->x_nomdef, nomdef );
            
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
                cvLine(x->image, *(defectArray[i].start), *(defectArray[i].depth_point),CV_RGB(0,0,255),1, CV_AA, 0 );
                cvCircle( x->image, *(defectArray[i].depth_point), 5, CV_RGB(0,255,0), -1, 8,0);
                cvCircle( x->image, *(defectArray[i].start), 5, CV_RGB(0,255,0), -1, 8,0);
                cvLine(x->image, *(defectArray[i].depth_point), *(defectArray[i].end),CV_RGB(0,0,255),1, CV_AA, 0 );
    	    t_atom rlist[7];
            SETFLOAT(&rlist[0], i);
            SETFLOAT(&rlist[1], defectArray[i].start->x);
            SETFLOAT(&rlist[2], defectArray[i].start->y);
            SETFLOAT(&rlist[3], defectArray[i].depth_point->x);
            SETFLOAT(&rlist[4], defectArray[i].depth_point->y);
            SETFLOAT(&rlist[5], defectArray[i].end->x);
            SETFLOAT(&rlist[6], defectArray[i].end->y);
    	    outlet_list( x->x_dataout, 0, 7, rlist );
            }

	    j++;
             
            // Free memory.       
            free(defectArray);
        }
        
        // Draw current contour.
        //cvDrawContours(x->cnt_img,contours,CV_RGB(255,255,255),CV_RGB(255,255,255),0,1, 8);
        cvDrawContours( x->image, contours, CV_RGB(255,0,0), CV_RGB(0,255,0), x->levels, 2, CV_AA, cvPoint(0,0)  );
        
        // Draw convex hull for current contour.        
        for(i=0; i<hullsize-1; i++)
        {
            cvLine(x->image, PointArray[hull[i]], 
                            PointArray[hull[i+1]],CV_RGB(255,255,255),1, CV_AA, 0 );
        }
        cvLine(x->image, PointArray[hull[hullsize-1]],
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


    memcpy( newdata, x->image->imageData, x->x_size*3 );

 
    return;
}

static void pdp_opencv_contours_convexity_param(t_pdp_opencv_contours_convexity *x, t_floatarg f1, t_floatarg f2)
{

}

static void pdp_opencv_contours_convexity_minarea(t_pdp_opencv_contours_convexity *x, t_floatarg f)
{
	if (f>0) x->minarea = (int)f;
}

static void pdp_opencv_contours_convexity_maxarea(t_pdp_opencv_contours_convexity *x, t_floatarg f)
{
	if (f>0) x->maxarea = (int)f;
}

static void pdp_opencv_contours_convexity_levels(t_pdp_opencv_contours_convexity *x, t_floatarg f)
{
	//x->levels = (int)f;
}

static void pdp_opencv_contours_convexity_sendpacket(t_pdp_opencv_contours_convexity *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_opencv_contours_convexity_process(t_pdp_opencv_contours_convexity *x)
{
   int encoding;
   t_pdp *header = 0;
   char *parname;
   unsigned pi;
   int partype;
   float pardefault;
   t_atom plist[2];
   t_atom tlist[2];
   t_atom vlist[2];

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_opencv_contours_convexity_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

	case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, (void*)pdp_opencv_contours_convexity_process_rgb, (void*)pdp_opencv_contours_convexity_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_opencv_contours_convexity_process */
	    break;
	    
	}
    }

}

static void pdp_opencv_contours_convexity_input_0(t_pdp_opencv_contours_convexity *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_opencv_contours_convexity_process(x);
    }
}

static void pdp_opencv_contours_convexity_free(t_pdp_opencv_contours_convexity *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    //cv_freeplugins(x);
    
    	//Destroy cv_images
	cvReleaseImage(&x->image);
    	cvReleaseImage(&x->gray);
}

t_class *pdp_opencv_contours_convexity_class;


void *pdp_opencv_contours_convexity_new(t_floatarg f)
{
    int i;

    t_pdp_opencv_contours_convexity *x = (t_pdp_opencv_contours_convexity *)pd_new(pdp_opencv_contours_convexity_class);
    //inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("minarea"));
    //inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("maxarea"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_nomdef  = outlet_new(&x->x_obj, &s_float); 
    x->x_dataout = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    //load the plugins
    x->x_infosok = 0;
    //cv_loadplugins(x, FF_PLUGIN_DIR);

    //pdp_opencv_contours_convexity_plugin(x, f);
    x->levels      = 1;
    x->area   = 1;
    x->minarea   = 1;
    x->maxarea   = 76800;
    x->selected  = 0;
    



    x->image = cvCreateImage(cvSize(x->x_width,x->x_height), IPL_DEPTH_8U, 3);
    x->gray = cvCreateImage(cvSize(x->image->width,x->image->height), IPL_DEPTH_8U, 1);

    //contours = 0;
    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_opencv_contours_convexity_setup(void)
{

    post( "		pdp_opencv_contours_convexity");
    pdp_opencv_contours_convexity_class = class_new(gensym("pdp_opencv_contours_convexity"), (t_newmethod)pdp_opencv_contours_convexity_new,
    	(t_method)pdp_opencv_contours_convexity_free, sizeof(t_pdp_opencv_contours_convexity), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_opencv_contours_convexity_class, (t_method)pdp_opencv_contours_convexity_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    //class_addmethod(pdp_opencv_contours_convexity_class, (t_method)pdp_opencv_contours_convexity_levels, gensym("levels"),  A_FLOAT, A_NULL );   
    //class_addmethod(pdp_opencv_contours_convexity_class, (t_method)pdp_opencv_contours_convexity_minarea, gensym("minarea"),  A_FLOAT, A_NULL );   
    //class_addmethod(pdp_opencv_contours_convexity_class, (t_method)pdp_opencv_contours_convexity_maxarea, gensym("maxarea"),  A_FLOAT, A_NULL );   


}

#ifdef __cplusplus
}
#endif
