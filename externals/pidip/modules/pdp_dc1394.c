/*
 *   PiDiP module
 *   Authors : Yves Degoyon ( ydegoyon@free.fr ) and Lluis Gomez i Bigorda
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

/*  This object is a video 4 linux 2 driver wrapper,
 *  inspired by pdp_v4l by Tom Schouten
 *  and some driver code from xawtv ( thanks to Gerd Knorr <kraxel@bytesex.org> )
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
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <linux/types.h>
#include <sys/mman.h>
#include <sched.h>
#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>
#include <dc1394/dc1394.h>

// dont open any more after a set number 
// of failed attempts
// this is to prevent locks on auto-open
// is reset when manually opened or closed
#define PDP_XV_RETRIES 10


#define DEVICENO 0
#define NBUF 2
#define COMPOSITEIN 1
#define WANTED_BUFFERS 2
#define MAX_INPUT   16
#define MAX_NORM    16
#define MAX_FORMAT  32
#define MAX_CTRL    32

#define IMAGE_FILE_NAME "image.pgm"
/*-----------------------------------------------------------------------
 *  Releases the cameras and exits
 *-----------------------------------------------------------------------*/
void cleanup_and_exit(dc1394camera_t *camera)
{
    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_capture_stop(camera);
    dc1394_camera_free(camera);
    exit(1);
}



typedef struct pdp_dc1394_struct
{
  t_object x_obj;
  t_float x_f;
  
  t_outlet *x_outlet0;

  bool x_initialized;
  bool x_auto_open;

  unsigned int x_width;
  unsigned int x_height;

  int x_curinput;
  int x_curstandard;
  int x_curformat;
  int x_freq;

  // video 4 linux 2 structures
  int x_ninputs;
  int x_nstandards;
  int x_nformats;

  unsigned char *x_pdp_buf[WANTED_BUFFERS];
 
  int x_tvfd;
  int x_frame;
  int x_skipnext;
  int x_mytopmargin, x_mybottommargin;
  int x_myleftmargin, x_myrightmargin;

  t_symbol *x_device;

  pthread_t x_thread_id;
  int x_continue_thread;
  int x_frame_ready;
  int x_only_new_frames;
  int x_last_frame;

  int x_open_retry;

  u32 x_minwidth;
  u32 x_maxwidth;
  u32 x_minheight;
  u32 x_maxheight;

  int x_debug;


    dc1394camera_t *camera;
    int i;
    dc1394featureset_t features;
    dc1394framerates_t framerates;
    dc1394video_modes_t video_modes;
    dc1394framerate_t framerate;
    dc1394video_mode_t video_mode;
    dc1394color_coding_t coding;
    unsigned int width, height;
    dc1394video_frame_t *frame;
    dc1394_t * d;
    dc1394camera_list_t * list;

    dc1394error_t err;

} t_pdp_dc1394;

static void pdp_dc1394_close(t_pdp_dc1394 *x)
{

}

static void pdp_dc1394_close_manual(t_pdp_dc1394 *x)
{
}

static void pdp_dc1394_close_error(t_pdp_dc1394 *x)
{
}

static int pdp_dc1394_capture_frame(t_pdp_dc1394* x)
{
    return 0;
} 

static void pdp_dc1394_wait_frame(t_pdp_dc1394* x)
{
}

static int pdp_dc1394_start_capturing(t_pdp_dc1394 *x)
{
    return 0;
}

static int pdp_dc1394_stop_capturing(t_pdp_dc1394 *x)
{
    return 0;
}

static void *pdp_dc1394_thread(void *voidx)
{
    return 0;
}

static void pdp_dc1394_setlegaldim(t_pdp_dc1394 *x, int xx, int yy);

static int pdp_dc1394_set_format(t_pdp_dc1394 *x, t_int index)
{
    return 0;
}

static int pdp_dc1394_init_mmap(t_pdp_dc1394 *x)
{
    return 0;
}

static void pdp_dc1394_open(t_pdp_dc1394 *x, t_symbol *name)
{

}

static void pdp_dc1394_open_manual(t_pdp_dc1394 *x, t_symbol *name)
{
}



static void pdp_dc1394_standard(t_pdp_dc1394 *x, t_float f)
{
}

static void pdp_dc1394_format(t_pdp_dc1394 *x, t_float f)
{
}

static void pdp_dc1394_freq(t_pdp_dc1394 *x, t_float f)
{
}

static void pdp_dc1394_freqMHz(t_pdp_dc1394 *x, t_float f)
{
}


static void pdp_dc1394_bang(t_pdp_dc1394 *x)
{
   
  /* if initialized, grab a frame and output it */

  unsigned int w,h,nbpixels,packet_size,plane1,plane2;
  unsigned char *newimage=NULL;
  int pdp_packt,length,pos,i,encoding;
  t_pdp* header;
  t_image* image;
  unsigned char * data;

  static short int gain[4] = {0x7fff, 0x7fff, 0x7fff, 0x7fff};

    /*-----------------------------------------------------------------------
     *  capture one frame
     *-----------------------------------------------------------------------*/
    //x->err=dc1394_capture_dequeue(x->camera, DC1394_CAPTURE_POLICY_WAIT, &x->frame);
    //DC1394_ERR_CLN_RTN(x->err,cleanup_and_exit(x->camera),"Could not capture a frame");
    //post("Could not capture a frame");
    if (x->err=dc1394_capture_dequeue(x->camera, DC1394_CAPTURE_POLICY_WAIT, &x->frame)!=DC1394_SUCCESS) {
        post("Failed to capture from camera %d", x->err);
	return;
    }

    /* create new packet */
    dc1394_get_image_size_from_video_mode(x->camera, x->video_mode, &x->width, &x->height);

    pdp_packt = pdp_packet_new_bitmap_rgb(x->width, x->height);
    header = pdp_packet_header(pdp_packt);
    //image = pdp_packet_image_info(pdp_packt);

    if (!header){
	post("pdp_dc1394: ERROR: can't allocate packet");
	return;
    }

    data = ( unsigned char *) pdp_packet_data(pdp_packt);
    int j;

    for (i=0; i<x->width; i++)	
      for (j=0; j<x->height; j++) {
	data[(j*x->width+i)*3] = x->frame->image[j*x->width+i];
	data[(j*x->width+i)*3+1] = x->frame->image[j*x->width+i];
	data[(j*x->width+i)*3+2] = x->frame->image[j*x->width+i];
    }

    //memcpy( data, x->frame->image, x->width*x->height*sizeof( unsigned char) );

    /*FILE* imagefile=fopen(IMAGE_FILE_NAME, "wb");

    if( imagefile == NULL) {
        perror( "Can't create '" IMAGE_FILE_NAME "'");
        cleanup_and_exit(x->camera);
    }

    fprintf(imagefile,"P5\n%u %u 255\n", x->width, x->height);
    fwrite(x->frame->image, 1, x->height*x->width, imagefile);
    fclose(imagefile);
    fprintf(stderr,"wrote: " IMAGE_FILE_NAME "\n");*/

    if (x->frame)
                dc1394_capture_enqueue (x->camera, x->frame);

    pdp_packet_pass_if_valid(x->x_outlet0, &pdp_packt);

}


static void pdp_dc1394_setlegaldim(t_pdp_dc1394 *x, int xx, int yy)
{

    unsigned int w,h;

    w  = pdp_imageproc_legalwidth((int)xx);
    h  = pdp_imageproc_legalheight((int)yy);
    
    w = (w < x->x_maxwidth) ? w : x->x_maxwidth;
    w = (w > x->x_minwidth) ? w : x->x_minwidth;

    h = (h < x->x_maxheight) ? h : x->x_maxheight;
    h = (h > x->x_minheight) ? h : x->x_minheight;

    x->x_width = w;
    x->x_height = h;
}

static void pdp_dc1394_dim(t_pdp_dc1394 *x, t_floatarg xx, t_floatarg yy)
{
    if (!x->x_initialized){
       post( "pdp_dc1394 : cannot set dim : no device opened ");
       return;
    }
    if (x->x_initialized){
        pdp_dc1394_close(x);
        pdp_dc1394_setlegaldim(x, (int)xx, (int)yy);
        pdp_dc1394_open(x, x->x_device);
    }
}

static void pdp_dc1394_free(t_pdp_dc1394 *x)
{
    dc1394_video_set_transmission(x->camera, DC1394_OFF);
    dc1394_capture_stop(x->camera);
    dc1394_camera_free(x->camera);
    dc1394_free (x->d);
}

t_class *pdp_dc1394_class;

void *pdp_dc1394_new(t_symbol *vdef)
{
    t_pdp_dc1394 *x = (t_pdp_dc1394 *)pd_new(pdp_dc1394_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);

    x->d = dc1394_new ();
    x->err=dc1394_camera_enumerate (x->d, &x->list);
    //DC1394_ERR_RTN(x->err,"Failed to enumerate cameras");
    //post("Failed to enumerate cameras");

    if (x->list->num == 0) {
        dc1394_log_error("No cameras found");
        return 1;
    }

    x->camera = dc1394_camera_new (x->d, x->list->ids[0].guid);
    if (!x->camera) {
        //dc1394_log_error("Failed to initialize camera with guid %"PRIx64, list->ids[0].guid);
        return 1;
    }
    dc1394_camera_free_list (x->list);

    //printf("Using camera with GUID %"PRIx64"\n", camera->guid);

    /*-----------------------------------------------------------------------
     *  get the best video mode and highest framerate. This can be skipped
     *  if you already know which mode/framerate you want...
     *-----------------------------------------------------------------------*/
    // get video modes:
    x->err=dc1394_video_get_supported_modes(x->camera,&x->video_modes);
    //DC1394_ERR_CLN_RTN(x->err,cleanup_and_exit(x->camera),"Can't get video modes");

    // select highest res mode:
    for (x->i=x->video_modes.num-1;x->i>=0;x->i--) {
        if (!dc1394_is_video_mode_scalable(x->video_modes.modes[x->i])) {
            dc1394_get_color_coding_from_video_mode(x->camera,x->video_modes.modes[x->i], &x->coding);
            if (x->coding==DC1394_COLOR_CODING_MONO8) {
                x->video_mode=x->video_modes.modes[x->i];
		fprintf(stderr,"video_mode %d: %d\n",x->i,x->video_modes.modes[x->i]);
                break;
            }
        }
    }
    if (x->i < 0) {
        dc1394_log_error("Could not get a valid MONO8 mode");
        cleanup_and_exit(x->camera);
    }

    x->err=dc1394_get_color_coding_from_video_mode(x->camera, x->video_mode,&x->coding);
    //DC1394_ERR_CLN_RTN(x->err,cleanup_and_exit(x->camera),"Could not get color coding");
    fprintf(stderr,"color_coding : %d\n",x->coding);

    // get highest framerate
    x->err=dc1394_video_get_supported_framerates(x->camera,x->video_mode,&x->framerates);
    //DC1394_ERR_CLN_RTN(x->err,cleanup_and_exit(x->camera),"Could not get framrates");
    x->framerate=x->framerates.framerates[x->framerates.num-1];
    fprintf(stderr,"framerate : %d\n",x->framerate);

    /*-----------------------------------------------------------------------
     *  setup capture
     *-----------------------------------------------------------------------*/

    x->err=dc1394_video_set_iso_speed(x->camera, DC1394_ISO_SPEED_400);
    //DC1394_ERR_CLN_RTN(x->err,cleanup_and_exit(x->camera),"Could not set iso speed");

    x->err=dc1394_video_set_mode(x->camera, x->video_mode);
    //DC1394_ERR_CLN_RTN(x->err,cleanup_and_exit(x->camera),"Could not set video mode");

    x->err=dc1394_video_set_framerate(x->camera, x->framerate);
    //DC1394_ERR_CLN_RTN(x->err,cleanup_and_exit(x->camera),"Could not set framerate");

    x->err=dc1394_capture_setup(x->camera,4, DC1394_CAPTURE_FLAGS_DEFAULT);
    //DC1394_ERR_CLN_RTN(x->err,cleanup_and_exit(x->camera),"Could not setup camera-\nmake sure that the video mode and framerate are\nsupported by your camera");

    /*-----------------------------------------------------------------------
     *  report camera's features
     *-----------------------------------------------------------------------*/
    x->err=dc1394_feature_get_all(x->camera,&x->features);
    if (x->err!=DC1394_SUCCESS) {
        dc1394_log_warning("Could not get feature set");
    }
    else {
        dc1394_feature_print_all(&x->features, stdout);
    }

    /*-----------------------------------------------------------------------
     *  have the camera start sending us data
     *-----------------------------------------------------------------------*/
    x->err=dc1394_video_set_transmission(x->camera, DC1394_ON);
    //DC1394_ERR_CLN_RTN(x->err,cleanup_and_exit(x->camera),"Could not start camera iso transmission");



    x->x_initialized = true;


    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_dc1394_setup(void)
{
    pdp_dc1394_class = class_new(gensym("pdp_dc1394"), (t_newmethod)pdp_dc1394_new,
    	(t_method)pdp_dc1394_free, sizeof(t_pdp_dc1394), 0, A_DEFSYMBOL, A_DEFSYMBOL, A_NULL);

    class_addmethod(pdp_dc1394_class, (t_method)pdp_dc1394_close_manual, gensym("close"), A_NULL);
    class_addmethod(pdp_dc1394_class, (t_method)pdp_dc1394_open_manual, gensym("open"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_dc1394_class, (t_method)pdp_dc1394_format, gensym("format"), A_FLOAT, A_NULL);
    class_addmethod(pdp_dc1394_class, (t_method)pdp_dc1394_standard, gensym("standard"), A_FLOAT, A_NULL);
    class_addmethod(pdp_dc1394_class, (t_method)pdp_dc1394_dim, gensym("dim"), A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(pdp_dc1394_class, (t_method)pdp_dc1394_bang, gensym("bang"), A_NULL);

}

#ifdef __cplusplus
}
#endif
