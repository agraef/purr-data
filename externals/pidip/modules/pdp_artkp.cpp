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

#include "ARToolKitPlus/TrackerMultiMarkerImpl.h"
#include "ARToolKitPlus/matrix.h"

#include "pdp.h"

#define PDP_ARTKP_CAMERA_DAT "/usr/share/ARToolKitPlus/camera.dat"
#define PDP_ARTKP_MARKER_CFG "/usr/share/ARToolKitPlus/markerboard_480-499.cfg"

#define MAX_MARKERS 60

//
// strangely in ARToolKitPlus, these functions are not public 
// thus importing the math utilities here 
//

void arUtilMatInv(ARFloat s[4][4], ARFloat d[4][4])
{
     ARToolKitPlus::ARMat       *mat;
     int         i, j;

     mat = ARToolKitPlus::Matrix::alloc( 4, 4 );
     for( j = 0; j < 4; j++ ) {
         for( i = 0; i < 4; i++ ) {
             mat->m[j*4+i] = s[j][i];
         }
     }
     ARToolKitPlus::Matrix::selfInv( mat );
     for( j = 0; j < 4; j++ ) {
         for( i = 0; i < 4; i++ ) {
             d[j][i] = mat->m[j*4+i];
         }
     }
     ARToolKitPlus::Matrix::free( mat );

     return;
}

int arGetAngle( ARFloat rot[3][3], ARFloat *wa, ARFloat *wb, ARFloat *wc )
{
    ARFloat      a, b, c;
    ARFloat      sina, cosa, sinb, cosb, sinc, cosc;

    if( rot[2][2] > 1.0 ) {
        rot[2][2] = 1.0;
    }
    else if( rot[2][2] < -1.0 ) {
        rot[2][2] = -1.0;
    }
    cosb = rot[2][2];
    b = (ARFloat)acos( cosb );
    sinb = (ARFloat)sin( b );
    if( b >= 0.000001 || b <= -0.000001) {
        cosa = rot[0][2] / sinb;
        sina = rot[1][2] / sinb;
        if( cosa > 1.0 ) {
            /* printf("cos(alph) = %f\n", cosa); */
            cosa = 1.0;
            sina = 0.0;
        }
        if( cosa < -1.0 ) {
            cosa = -1.0;
            sina =  0.0;
        }
       if( sina > 1.0 ) {
            sina = 1.0;
            cosa = 0.0;
        }
        if( sina < -1.0 ) {
            sina = -1.0;
            cosa =  0.0;
        }
        a = (ARFloat)acos( cosa );
        if( sina < 0 ) a = -a;

        sinc =  (rot[2][1]*rot[0][2]-rot[2][0]*rot[1][2])
              / (rot[0][2]*rot[0][2]+rot[1][2]*rot[1][2]);
        cosc =  -(rot[0][2]*rot[2][0]+rot[1][2]*rot[2][1])
               / (rot[0][2]*rot[0][2]+rot[1][2]*rot[1][2]);
        if( cosc > 1.0 ) {
            cosc = 1.0;
            sinc = 0.0;
        }
        if( cosc < -1.0 ) {
            cosc = -1.0;
            sinc =  0.0;
        }
        if( sinc > 1.0 ) {
            sinc = 1.0;
            cosc = 0.0;
        }
        if( sinc < -1.0 ) {
            sinc = -1.0;
            cosc =  0.0;
        }
        c = (ARFloat)acos( cosc );
        if( sinc < 0 ) c = -c;
    }
    else {
        a = b = 0.0;
        cosa = cosb = 1.0;
        sina = sinb = 0.0;
        cosc = rot[0][0];
        sinc = rot[1][0];
        if( cosc > 1.0 ) {
            cosc = 1.0;
            sinc = 0.0;
        }
        if( cosc < -1.0 ) {
            cosc = -1.0;
            sinc =  0.0;
        }
        if( sinc > 1.0 ) {
            sinc = 1.0;
            cosc = 0.0;
        }
        if( sinc < -1.0 ) {
            sinc = -1.0;
            cosc =  0.0;
        }
        c = (ARFloat)acos( cosc );
        if( sinc < 0 ) c = -c;
    }

    *wa = a;
    *wb = b;
    *wc = c;

    return 0;
}


class PdLogger : public ARToolKitPlus::Logger
{
    void artLog(const char* msg)
    {
        post("pdp_artkp : %s", msg);
    }
};

typedef struct pdp_artkp
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_outlet1;
    t_outlet *x_outlet2;
    int x_packet0;
    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;

    int x_init;

    // ARToolKitPlus data
    ARToolKitPlus::TrackerMultiMarker *x_tracker; /// the ARToolKitPlus marker tracker
    PdLogger *x_logger; // tracker logger
    int x_nummarkers;   // number of detected markers
    int x_threshold;    // detection threshold
    float x_nearclip;   // near clip matrix coefficient
    float x_farclip;    // far clip matrix coefficient
    float x_border;     // marker border
    float x_mwidth;     // width of a marker
    t_atom x_mdata[16]; // basic marker data ( list )
                        // <index> <id> <size> <x> <y> <x1> <y1> <x2> <y2> <x3> <y3> <x4> <y4> <alpha> <beta> <gamma>
    ARFloat x_alpha[MAX_MARKERS];    // euler angles 
    ARFloat x_beta[MAX_MARKERS];
    ARFloat x_gamma[MAX_MARKERS];

} t_pdp_artkp;

static void pdp_artkp_init(t_pdp_artkp *x)
{

  int im,row,column;

   if( (x->x_width>1024) || (x->x_height>1024) )
   {
     post( "pdp_artkp : warning : detection might not work well with resolution > 1024");
   } 

   x->x_tracker = new ARToolKitPlus::TrackerMultiMarkerImpl<6,6,6,1>(x->x_width, x->x_height); // 666 : size of the markers
   post( "pdp_artkp : %s", x->x_tracker->getDescription() );

   if( !x->x_tracker )
   {
     post( "pdp_artkp : FATAL... could not allocate tracker");
     exit(-1);
   }

   // setting pixel format
   x->x_tracker->setPixelFormat(ARToolKitPlus::PIXEL_FORMAT_RGB);

   if ( (fopen(PDP_ARTKP_CAMERA_DAT, "r") == NULL ) || (fopen(PDP_ARTKP_MARKER_CFG, "r") == NULL ) )
   {
        post( "pdp_artkp : FATAL... ARToolKitPlus init() failed.");
        post( "pdp_artkp : did you install artkp data files in /usr/share/ARToolKitPlus ?");
        x->x_init=0;
        return;
   }

   if(!x->x_tracker->init(PDP_ARTKP_CAMERA_DAT, PDP_ARTKP_MARKER_CFG, x->x_nearclip, x->x_farclip))
   {
        post( "pdp_artkp : FATAL... ARToolKitPlus init() failed.");
        post( "pdp_artkp : did you install artkp data files in /usr/share/ARToolKitPlus ?");
        exit(-1);
   } 

   // the marker in the BCH test image has a thiner border...
   x->x_tracker->setBorderWidth(x->x_border);

   // activate automatic thresholding
   if ( x->x_threshold < 0)
   {
      x->x_tracker->activateAutoThreshold(true);
   }
   else
   {
      x->x_tracker->setThreshold(x->x_threshold);
   }

   // let's use lookup-table undistortion for high-speed
   // note: LUT only works with images up to 1024x1024
   x->x_tracker->setUndistortionMode(ARToolKitPlus::UNDIST_LUT);

   x->x_tracker->setPoseEstimator(ARToolKitPlus::POSE_ESTIMATOR_RPP);

   // switch to simple ID based markers
   // use the tool in tools/IdPatGen to generate markers
   x->x_tracker->setMarkerMode(ARToolKitPlus::MARKER_ID_SIMPLE);

   x->x_logger = new PdLogger;
   if( !x->x_logger )
   {
     post( "pdp_artkp : FATAL... could not allocate logger");
     exit(-1);
   }
   else
   {
     x->x_tracker->setLogger(x->x_logger);
   }

   // show configuration
   // const ARToolKitPlus::ARMultiMarkerInfoT *artkpConfig = x->x_tracker->getMultiMarkerConfig();
   // post("pdp_artkp : %d markers defined in multi marker cfg\n", artkpConfig->marker_num);

   // post("pdp_artkp : marker matrices:\n");
   // for(im = 0; im < artkpConfig->marker_num; im++)
   // {
   //     post("pdp_artkp : marker %d, id %d", im, artkpConfig->marker[im].patt_id);
   // }

   x->x_init=1;
}

static void pdp_artkp_process_rgb(t_pdp_artkp *x)
{
  t_pdp         *header = pdp_packet_header(x->x_packet0);
  unsigned char *data   = (unsigned char *)pdp_packet_data(x->x_packet0);
  int im, i, j, ia;
  int numDetected;
  int poserr;

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

      post("pdp_artkp :: resizing plugins");
  
      x->x_width = header->info.image.width;
      x->x_height = header->info.image.height;
      x->x_size = x->x_width*x->x_height;

      pdp_artkp_init(x);
    }

    if ( x->x_init == 0 )
    {
        post( "pdp_artkp : not processing : did you install artkp data files in /usr/share/ARToolKitPlus ?");
        return;
    }
    
    numDetected = x->x_tracker->calc((unsigned char*)data);

    if ( numDetected != x->x_nummarkers )
    {
       x->x_nummarkers = numDetected;
       outlet_float( x->x_outlet2, x->x_nummarkers );
    }

    for (im=0; im<numDetected; im++) 
    {
       ARToolKitPlus::ARMarkerInfo marker = x->x_tracker->getDetectedMarker(im);
       ARToolKitPlus::ARMarkerInfo markerb = x->x_tracker->getDetectedMarker(im);
       ARFloat center[2]; 
       ARFloat tmark[2]; 
       ARFloat width; 
       ARFloat oGLMat[4][4], rotMat[3][3];

       // reorder vertexes
       markerb.dir = 1;
       markerb.vertex[0][0] = marker.vertex[(5-marker.dir)%4][0];
       markerb.vertex[0][1] = marker.vertex[(5-marker.dir)%4][1];
       markerb.vertex[1][0] = marker.vertex[(6-marker.dir)%4][0];
       markerb.vertex[1][1] = marker.vertex[(6-marker.dir)%4][1];
       markerb.vertex[2][0] = marker.vertex[(7-marker.dir)%4][0];
       markerb.vertex[2][1] = marker.vertex[(7-marker.dir)%4][1];
       markerb.vertex[3][0] = marker.vertex[(8-marker.dir)%4][0];
       markerb.vertex[3][1] = marker.vertex[(8-marker.dir)%4][1];

       width = x->x_mwidth; // width of the marker
       center[0] = markerb.pos[0];
       center[1] = markerb.pos[1];

       if ( ( poserr = x->x_tracker->calcOpenGLMatrixFromMarker( &markerb, center, width, (ARFloat*)&oGLMat ) ) < 0 )
       {
          // post( "pdp_artkp : error in pose estimator");
       }
       else
       {
          // post( "pdp_artkp : pose estimator error : %d", poserr );

          // rotation matrix is included in the new matrix
          for ( j=0; j<3; j++ )
          {
             for ( i=0; i<3; i++ )
             {
                 rotMat[i][j] = oGLMat[i][j];
                 // printf( "%f ", oGLMat[i][j] );
             }
             // printf( "\n" );
          }
          // printf( "----------------------------------------------------------------\n" );

          // calculate angles from the rotation matrix
          arGetAngle( rotMat, &x->x_alpha[im], &x->x_beta[im], &x->x_gamma[im] );
       }

       SETFLOAT(&x->x_mdata[0], im);
       SETFLOAT(&x->x_mdata[1], markerb.id);
       SETFLOAT(&x->x_mdata[2], markerb.area);
       SETFLOAT(&x->x_mdata[3], markerb.pos[0]);
       SETFLOAT(&x->x_mdata[4], markerb.pos[1]);

       for ( i=0; i<4; i++ )
       {
          for ( j=0; j<2; j++ )
          {
              SETFLOAT(&x->x_mdata[5+i*2+j], markerb.vertex[i][j]);
          }
       }

       // crazy patch here for a sign issue
       if ( pow( markerb.vertex[0][0] - markerb.vertex[1][0], 2) + pow( markerb.vertex[0][1] - markerb.vertex[1][1], 2) <  
            pow( markerb.vertex[2][0] - markerb.vertex[3][0], 2) + pow( markerb.vertex[2][1] - markerb.vertex[3][1], 2) )   
       {
          // post( "pdp_artkp : correct angle" );
          x->x_beta[im] = -x->x_beta[im];
       }
 
 
       SETFLOAT(&x->x_mdata[13], x->x_alpha[im]);
       SETFLOAT(&x->x_mdata[14], x->x_beta[im]);
       SETFLOAT(&x->x_mdata[15], x->x_gamma[im]);

       outlet_list( x->x_outlet1, 0, 16, x->x_mdata );
    }

    return;
}


static void pdp_artkp_sendpacket(t_pdp_artkp *x)
{
    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet0);
}

static void pdp_artkp_process(t_pdp_artkp *x)
{
  int encoding;
  t_pdp *header = 0;

  /* check if image data packets are compatible */
  if ( (header = pdp_packet_header(x->x_packet0))
  && (PDP_BITMAP == header->type)){
    
  /* pdp_artkp_process inputs and write into active inlet */
  switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

   case PDP_BITMAP_RGB:
            pdp_queue_add(x, (void*)pdp_artkp_process_rgb, (void*)pdp_artkp_sendpacket, &x->x_queue_id);
      break;

   default:
      /* don't know the type, so dont pdp_artkp_process */
      break;
      
   }
  }

}

static void pdp_artkp_input_0(t_pdp_artkp *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
    {
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym((char*)"bitmap/rgb/*") );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_artkp_process(x);
    }
}

static void pdp_artkp_threshold(t_pdp_artkp *x, t_floatarg f)
{
   x->x_threshold = (int)f;
   x->x_tracker->setThreshold(x->x_threshold);
}

static void pdp_artkp_nearclip(t_pdp_artkp *x, t_floatarg f)
{
   x->x_nearclip = (int)f;
   pdp_artkp_init(x);
}

static void pdp_artkp_farclip(t_pdp_artkp *x, t_floatarg f)
{
   x->x_farclip = (int)f;
   pdp_artkp_init(x);
}

static void pdp_artkp_border(t_pdp_artkp *x, t_floatarg f)
{
   x->x_border = (int)f;
   pdp_artkp_init(x);
}

static void pdp_artkp_mwidth(t_pdp_artkp *x, t_floatarg f)
{
   x->x_mwidth = (int)f;
}

static void pdp_artkp_free(t_pdp_artkp *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    
}

t_class *pdp_artkp_class;

void *pdp_artkp_new(t_floatarg f)
{
  int i;

    t_pdp_artkp *x = (t_pdp_artkp *)pd_new(pdp_artkp_class);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("threshold"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("nearclip"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("farclip"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("border"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("width"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet1 = outlet_new(&x->x_obj, &s_anything); 
    x->x_outlet2 = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_queue_id = -1;

    x->x_width  = 640;
    x->x_height = 480;
    x->x_size   = x->x_width * x->x_height;

    // init the ARToolKitPlus tracker
    x->x_nummarkers = 0;
    x->x_threshold = -1;
    x->x_nearclip = 0.0f;
    x->x_farclip = 1000.0f;
    x->x_border = 0.1f;
    x->x_mwidth = 80.0f;
    for ( i=0; i<MAX_MARKERS;i++ )
    {
       x->x_alpha[i] = 0;
       x->x_beta[i] = 0;
       x->x_gamma[i] = 0;
    }
    x->x_init = 0;
    pdp_artkp_init(x);
    
    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_artkp_setup(void)
{

    pdp_artkp_class = class_new(gensym("pdp_artkp"), (t_newmethod)pdp_artkp_new,
      (t_method)pdp_artkp_free, sizeof(t_pdp_artkp), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_artkp_class, (t_method)pdp_artkp_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_artkp_class, (t_method)pdp_artkp_threshold, gensym("threshold"),  A_FLOAT, A_NULL );
    class_addmethod(pdp_artkp_class, (t_method)pdp_artkp_nearclip, gensym("nearclip"),  A_FLOAT, A_NULL );
    class_addmethod(pdp_artkp_class, (t_method)pdp_artkp_farclip, gensym("farclip"),  A_FLOAT, A_NULL );
    class_addmethod(pdp_artkp_class, (t_method)pdp_artkp_border, gensym("border"),  A_FLOAT, A_NULL );
    class_addmethod(pdp_artkp_class, (t_method)pdp_artkp_mwidth, gensym("width"),  A_FLOAT, A_NULL );

}

#ifdef __cplusplus
}
#endif
