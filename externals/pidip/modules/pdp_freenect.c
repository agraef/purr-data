/*
 *   PiDiP module
 *   Authors : tffobject@gmail.com
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

/*  This object is a M$ kinect interface object
 *  inspired by libfreenect example by Hector Martin ( marcan ) & more 
 */


#include "pdp_config.h"
#include "pdp.h"
#include "pdp_llconv.h"
#include "pdp_imageproc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <linux/types.h>
#include <sched.h>
#include <pthread.h>

#include <libfreenect.h>

#define FREENECT_FRAME_W 640
#define FREENECT_FRAME_H 480
#define FREENECT_FRAME_PIX FREENECT_FRAME_W*FREENECT_FRAME_H

// colors are fixed ( map 2048 levels of grey to colors )
static uint16_t t_gamma[2048];

typedef struct pdp_freenect_struct
{
  t_object x_obj;
  t_float x_f;
  
  t_outlet *x_outlet0;
  t_outlet *x_outlet1;
  t_outlet *x_outlet2;
  t_outlet *x_outlet3;
  t_outlet *x_outlet4;

  bool x_initialized;
  bool x_stop;
  pthread_t x_thread_id;        // kinect thread id

  int x_ndevices;               // number of kinect devices found
  int x_angle;                  // tilt angle

  uint8_t *x_depth_mid;
  uint16_t *x_depth_grey;
  uint8_t *x_rgb_back;
  uint8_t *x_rgb_mid;
  uint8_t *x_rgb_front;

  int x_gotdepth; // flag of a new depth frame
  int x_gotrgb;   // flag of a new video frame
  int x_minlevel; // minimum level of grey to output
  int x_maxlevel; // maximum level of grey to output

  freenect_context *x_fctx;     // freenect context
  freenect_device *x_fdev;      // freenect device

  t_atom  x_slist[2];
  t_atom  x_alist[3];

} t_pdp_freenect;

static void pdp_freenect_close(t_pdp_freenect *x)
{
  if ( !x->x_initialized )
  {
     post("pdp_freenect : no device open, nothing to do");
     return;
  }

  // close the kinect device and dealloc buffers

  x->x_initialized = false;
  x->x_stop = true; // set the flag to stop the thread
}

void pdp_freenect_got_depth(freenect_device *dev, void *v_depth, uint32_t timestamp)
{
  int i;
  uint16_t *depth = (uint16_t*)v_depth;
  t_pdp_freenect *x = (t_pdp_freenect*)freenect_get_user(dev);

  if ( x->x_gotdepth ) return;
  for (i=0; i<FREENECT_FRAME_PIX; i++) {

        // simplified grey level image
        x->x_depth_grey[i] = 255-((float)depth[i]/2047)*255;

        // all black objects are just too close
        if ( x->x_depth_grey[i] == 0 )  x->x_depth_grey[i] = 255;

        // filter levels 
        if ( ( x->x_depth_grey[i] < x->x_minlevel ) ||
            ( x->x_depth_grey[i] > x->x_maxlevel ) )
        {
            x->x_depth_grey[i]=0;
        }
        x->x_depth_grey[i] = x->x_depth_grey[i]<<7;

        int pval = t_gamma[depth[i]];
        int lb = pval & 0xff;
        switch (pval>>8) {
              case 0:
                    x->x_depth_mid[3*i+0] = 255;
                    x->x_depth_mid[3*i+1] = 255-lb;
                    x->x_depth_mid[3*i+2] = 255-lb;
                    break;
              case 1:
                    x->x_depth_mid[3*i+0] = 255;
                    x->x_depth_mid[3*i+1] = lb;
                    x->x_depth_mid[3*i+2] = 0;
                    break;
              case 2:
                    x->x_depth_mid[3*i+0] = 255-lb;
                    x->x_depth_mid[3*i+1] = 255;
                    x->x_depth_mid[3*i+2] = 0;
                    break;
              case 3:
                    x->x_depth_mid[3*i+0] = 0;
                    x->x_depth_mid[3*i+1] = 255;
                    x->x_depth_mid[3*i+2] = lb;
                    break;
              case 4:
                    x->x_depth_mid[3*i+0] = 0;
                    x->x_depth_mid[3*i+1] = 255-lb;
                    x->x_depth_mid[3*i+2] = 255;
                    break;
              case 5:
                    x->x_depth_mid[3*i+0] = 0;
                    x->x_depth_mid[3*i+1] = 0;
                    x->x_depth_mid[3*i+2] = 255-lb;
                    break;
              default:
                    x->x_depth_mid[3*i+0] = 0;
                    x->x_depth_mid[3*i+1] = 0;
                    x->x_depth_mid[3*i+2] = 0;
              break;
    }
  }
  x->x_gotdepth++;
}

void pdp_freenect_got_rgb(freenect_device *dev, void *rgb, uint32_t timestamp)
{
  t_pdp_freenect *x = (t_pdp_freenect*)freenect_get_user(dev);

  if ( x->x_gotrgb ) return;

  // swap buffers
  assert (x->x_rgb_back == rgb);
  x->x_rgb_back = x->x_rgb_mid;
  freenect_set_video_buffer(dev, x->x_rgb_back);
  x->x_rgb_mid = (uint8_t*)rgb;

  x->x_gotrgb++;
}

static void *pdp_freenect_thread(void *voidx)
{
   t_pdp_freenect *x = ((t_pdp_freenect *)voidx);
   double dx,dy,dz;
   freenect_raw_tilt_state* state;
   struct timespec stime;

   freenect_set_led(x->x_fdev,LED_RED);
   freenect_set_tilt_degs(x->x_fdev,x->x_angle);
   freenect_set_depth_callback(x->x_fdev, pdp_freenect_got_depth);
   freenect_set_video_callback(x->x_fdev, pdp_freenect_got_rgb);
   freenect_set_video_mode(x->x_fdev, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB));
   freenect_set_depth_mode(x->x_fdev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));
   freenect_set_video_buffer(x->x_fdev, x->x_rgb_back);

   freenect_start_depth(x->x_fdev);
   freenect_start_video(x->x_fdev);

   freenect_set_led(x->x_fdev,LED_GREEN);
   x->x_initialized = true;
   x->x_stop = false;

   while (!x->x_stop && freenect_process_events(x->x_fctx) >= 0) 
   {
     freenect_update_tilt_state(x->x_fdev);
     state = freenect_get_tilt_state(x->x_fdev);
     freenect_get_mks_accel(state, &dx, &dy, &dz);

     SETFLOAT(&x->x_slist[0], (float)state->tilt_status);
     SETFLOAT(&x->x_slist[1], (float)state->tilt_angle);

     SETFLOAT(&x->x_alist[0], (float)dx);
     SETFLOAT(&x->x_alist[1], (float)dy);
     SETFLOAT(&x->x_alist[2], (float)dz);

     stime.tv_sec = 0;
     stime.tv_nsec = 10000000; // 10 ms
     // nanosleep(&stime, NULL);
   }

   post("pdp_freenet : capture exiting ... closing device ...");

   freenect_stop_depth(x->x_fdev);
   freenect_stop_video(x->x_fdev);
                                                   
   freenect_close_device(x->x_fdev);

   return NULL;
}

static void pdp_freenect_open(t_pdp_freenect *x, t_float devnum)
{
  int pres;

    if ( (int)devnum <= 0 || ( (int)devnum > x->x_ndevices) )
    {
         post( "pdp_freenect : wrong device number : %d", (int)devnum );
         return;
    }

    if (freenect_open_device(x->x_fctx, &x->x_fdev, (int)devnum-1) < 0) 
    {
         post("pdp_freenect : could not open device : %d", (int)devnum);
         return;
    }
    freenect_set_user(x->x_fdev, (void*)x);

    pres = pthread_create(&x->x_thread_id, NULL, pdp_freenect_thread, (void *)x);
    if (pres) 
    {
        post("pdp_freenect : could not create thread : ret : %d", pres);
        return;
    }
    else
    {
        post("pdp_freenect : capture thread created : %ul", x->x_thread_id);
    }

}

static void pdp_freenect_tilt(t_pdp_freenect *x, t_float tilt)
{
  // if initialized, grab a frame and output it 
  if ( !x->x_initialized )
  {
     post("pdp_freenect : no device open, cannot tilt");
     return;
  }

  x->x_angle = (int)tilt;
  if( x->x_angle>30 ) x->x_angle=30;
  if( x->x_angle<-30 ) x->x_angle=-30;

  freenect_set_tilt_degs(x->x_fdev,x->x_angle);
}

static void pdp_freenect_minlevel(t_pdp_freenect *x, t_float flevel)
{
  if ( ( (int)flevel < 0 ) || ( (int)flevel > 255 ) || ( (int)flevel > x->x_maxlevel ) )
  {
     post("pdp_freenect : wrong min level : %d", (int)flevel );
     return;
  }

  x->x_minlevel = (int)flevel;
}

static void pdp_freenect_maxlevel(t_pdp_freenect *x, t_float flevel)
{
  if ( ( (int)flevel < 0 ) || ( (int)flevel > 255 ) || ( (int)flevel < x->x_minlevel ) )
  {
     post("pdp_freenect : wrong max level : %d", (int)flevel );
     return;
  }

  x->x_maxlevel = (int)flevel;
}

static void pdp_freenect_bang(t_pdp_freenect *x)
{

 uint8_t *tmp;
 int new_depth;
 int new_depth_grey;
 int new_rgb;
 uint8_t *depth_data = NULL;
 uint16_t *grey_depth_data = NULL;
 uint8_t *rgb_data = NULL;

  // if initialized, grab a frame and output it 
  if ( !x->x_initialized )
  {
     post("pdp_freenect : no device open, no image returned");
     return;
  }

  if ( x->x_gotdepth )
  {
    new_depth = pdp_packet_new_bitmap_rgb(FREENECT_FRAME_W, FREENECT_FRAME_H);
    depth_data = (uint8_t*)pdp_packet_data( new_depth );
    memcpy( (void*)depth_data, (void*)x->x_depth_mid, FREENECT_FRAME_PIX*3 );
    pdp_packet_pass_if_valid(x->x_outlet0, &new_depth);

    new_depth_grey = pdp_packet_new_image_grey(FREENECT_FRAME_W, FREENECT_FRAME_H);
    grey_depth_data = (uint16_t*)pdp_packet_data( new_depth_grey );
    memcpy( (void*)grey_depth_data, (void*)x->x_depth_grey, FREENECT_FRAME_PIX*sizeof(uint16_t) );
    pdp_packet_pass_if_valid(x->x_outlet1, &new_depth_grey);
    x->x_gotdepth--;
  }
  else
  {
    // post("pdp_freenect : no depth frame");
  } 

  if ( x->x_gotrgb )
  {
    tmp = x->x_rgb_front;
    x->x_rgb_front = x->x_rgb_mid;
    x->x_rgb_mid = tmp;
    new_rgb = pdp_packet_new_bitmap_rgb(FREENECT_FRAME_W, FREENECT_FRAME_H);
    rgb_data = (uint8_t*)pdp_packet_data( new_rgb );
    memcpy( (void*)rgb_data, (void*)x->x_rgb_mid, FREENECT_FRAME_PIX*3 );
    pdp_packet_pass_if_valid(x->x_outlet2, &new_rgb);
    x->x_gotrgb--;
  }
  else
  {
    // post("pdp_freenect : no video frame");
  } 

  // output tilt state
  outlet_list( x->x_outlet3, 0, 2, x->x_slist );

  // output accelerometer position ( angles )
  outlet_list( x->x_outlet4, 0, 3, x->x_alist );
}

static void pdp_freenect_free(t_pdp_freenect *x)
{
   freenect_shutdown(x->x_fctx);
}

t_class *pdp_freenect_class;

void *pdp_freenect_new(t_symbol *vdef)
{
  t_pdp_freenect *x = (t_pdp_freenect *)pd_new(pdp_freenect_class);
  t_pdp *header;

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);
    x->x_outlet1 = outlet_new(&x->x_obj, &s_anything);
    x->x_outlet2 = outlet_new(&x->x_obj, &s_anything);
    x->x_outlet3 = outlet_new(&x->x_obj, &s_anything);
    x->x_outlet4 = outlet_new(&x->x_obj, &s_anything);

    x->x_initialized = false;
    x->x_stop = false;

    x->x_angle = 0;
    x->x_minlevel = 0;
    x->x_maxlevel = 255;

    // allocate static images
    x->x_depth_mid = (uint8_t*)malloc(FREENECT_FRAME_PIX*3);
    x->x_depth_grey = (uint16_t*)malloc(FREENECT_FRAME_PIX*sizeof(uint16_t));
    x->x_rgb_back = (uint8_t*)malloc(FREENECT_FRAME_PIX*3);
    x->x_rgb_mid = (uint8_t*)malloc(FREENECT_FRAME_PIX*3);
    x->x_rgb_front = (uint8_t*)malloc(FREENECT_FRAME_PIX*3);

    x->x_gotdepth = 0;
    x->x_gotrgb = 0;

    post("pdp_freenect : initializing freenet lib");
    if (freenect_init(&x->x_fctx, NULL) < 0) 
    {
       post("pdp_freenect : freenect_init() failed\n");
    }

    freenect_set_log_level(x->x_fctx, FREENECT_LOG_ERROR);

    x->x_ndevices = freenect_num_devices (x->x_fctx);
    post("pdp_freenect : found %d devices", x->x_ndevices);

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_freenect_setup(void)
{
  int i;
  float v;

    pdp_freenect_class = class_new(gensym("pdp_freenect"), (t_newmethod)pdp_freenect_new,
    	(t_method)pdp_freenect_free, sizeof(t_pdp_freenect), 0, A_DEFSYMBOL, A_DEFSYMBOL, A_NULL);

    class_addmethod(pdp_freenect_class, (t_method)pdp_freenect_close, gensym("close"), A_NULL);
    class_addmethod(pdp_freenect_class, (t_method)pdp_freenect_open, gensym("open"), A_FLOAT, A_NULL);
    class_addmethod(pdp_freenect_class, (t_method)pdp_freenect_tilt, gensym("tilt"), A_FLOAT, A_NULL);
    class_addmethod(pdp_freenect_class, (t_method)pdp_freenect_bang, gensym("bang"), A_NULL);
    class_addmethod(pdp_freenect_class, (t_method)pdp_freenect_minlevel, gensym("minlevel"), A_FLOAT, A_NULL);
    class_addmethod(pdp_freenect_class, (t_method)pdp_freenect_maxlevel, gensym("maxlevel"), A_FLOAT, A_NULL);

    for (i=0; i<2048; i++) {
        v = i/2048.0;
        v = powf(v, 3)* 6;
        t_gamma[i] = v*6*256;
    }
}

#ifdef __cplusplus
}
#endif
