/*
 *   Pure Data Packet module.
 *   Copyright (c) by Lluis Gomez i Bigorda <lluisgomez@hangar.org>
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
#include <getopt.h>             /* getopt_long() */
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <sched.h>
#include <pthread.h>

#include "dv1394.h"
#include <libdv/dv.h>

#define N_BUF 2 /*DV1394_MAX_FRAMES/4*/
#define PAL 0
#define NTSC 1


typedef struct pdp_ieee1394_struct
{
  t_object x_obj;
  
  t_outlet *x_outlet0;

  int x_width;
  int x_height;

  int dvfd;
  unsigned char *videobuf;
  unsigned char *decodedbuf;
  bool x_frame_ready;
  int  x_frame, x_lastframe;
  bool x_continue_thread;
  pthread_t x_thread_id;

  int x_framesize;
  unsigned char *x_mmapbuf;

  dv_decoder_t *x_decoder;

  bool x_haveVideo;
  bool x_capturing;
  bool x_norm;
  char* x_devicename;
  int x_devicenum;

} t_pdp_ieee1394;



static void
process_image                   (t_pdp_ieee1394 *x)
{
	
    unsigned int w,h;
    int object,length,pos,i,encoding;
    t_pdp* header;
    t_image* image;
    short int * data;
	
    //fputc ('.', stdout);
    //fflush (stdout);
	
	
	
    /* create new packet */
    w = x->x_width;
    h = x->x_height;


    object = pdp_packet_new_image(PDP_IMAGE_YV12, w, h);
    header = pdp_packet_header(object);
    image = pdp_packet_image_info(object);

    if (!header){
	post("pdp_v4l: ERROR: can't allocate packet");
	return;
    }

    data = (short int *) pdp_packet_data(object);
	
    /* convert data to pdp packet */
    //pdp_llconv(x->decodedbuf, RIF_YVYU_P____U8, data, RIF_YVU__P411_S16, w, h); 
    pdp_llconv(x->decodedbuf, RIF_RGB__P____U8, data, RIF_YVU__P411_S16, w, h); //


    pdp_packet_pass_if_valid(x->x_outlet0, &object);
}

static int pdp_ieee1394_read_frame(t_pdp_ieee1394 *x)
{

  if (!x->x_decoder)return 0;
  if (!x->x_frame_ready) {
	//x->x_image.newimage = 0;
  }
  else {
    dv_parse_header(x->x_decoder, x->videobuf);
    dv_parse_packs (x->x_decoder, x->videobuf);
    if(dv_frame_changed(x->x_decoder)) {
      int pitches[3] = {0,0,0};
      //      pitches[0]=x_decoder->width*3; // rgb
      //      pitches[0]=x_decoder->width*((x_reqFormat==GL_RGBA)?3:2);
      pitches[0]=x->x_decoder->width*3;
      x->x_height=x->x_decoder->height;
      x->x_width=x->x_decoder->width;
      
      /* decode the DV-data to something we can handle and that is similar to the wanted format */
      //      dv_report_video_error(x_decoder, videobuf);  // do we need this ?
      // gosh, this(e_dv_color_rgb) is expansive:: the decoding is done in software only...
      //      dv_decode_full_frame(x_decoder, videobuf, ((x_reqFormat==GL_RGBA)?e_dv_color_rgb:e_dv_color_yuv), &decodedbuf, pitches);
      dv_decode_full_frame(x->x_decoder, x->videobuf, e_dv_color_rgb, &x->decodedbuf, pitches);

      //     post("sampling %d", x_decoder->sampling);

      /* convert the colour-space to the one we want */
      /*
       * btw. shouldn't this be done in [pix_video] rather than here ?
       * no because [pix_video] knows nothing about the possible colourspaces in here
       */

      // letting the library do the conversion to RGB and then doing the conversion to RGBA
      // is really stupid.
      // let's do it all ourselfes:
      //      if (x_reqFormat==GL_RGBA)x_image.image.fromRGB(decodedbuf); else
      //x_image.image.fromYVYU(decodedbuf);
    	process_image (x);
	
    }

    x->x_frame_ready = false;
  }
	
  return 1;
}


static void *pdp_ieee1394_thread(void *voidx)
{
    t_pdp_ieee1394 *x = ((t_pdp_ieee1394 *)voidx);

  int fd=x->dvfd;
  int framesize = x->x_framesize;
  struct dv1394_status dvst;
  int n_frames = N_BUF;
  unsigned char* mmapbuf = x->x_mmapbuf;

  /* this will hang if no ieee1394-device is present, what to do about it ??? */
  x->x_haveVideo=false;
  if(ioctl(fd, DV1394_WAIT_FRAMES, 1)) {
    perror("error: ioctl WAIT_FRAMES");
    x->x_capturing=false; return NULL;
  }
  if (ioctl(fd, DV1394_GET_STATUS, &dvst))   {
    perror("ioctl GET_STATUS");
    x->x_capturing=false; return NULL;
  }
  x->x_haveVideo=true;
  x->x_capturing=true;

  //fprintf(stderr,"aqui1");
  while(x->x_continue_thread){
  //fprintf(stderr,"aqui2");
    if(ioctl(fd, DV1394_WAIT_FRAMES, n_frames - 1)) {
      perror("error: ioctl WAIT_FRAMES");
      x->x_capturing=false; return NULL;
    }
    if (ioctl(fd, DV1394_GET_STATUS, &dvst))   {
      perror("ioctl GET_STATUS");
      x->x_capturing=false; return NULL;
    }
  //fprintf(stderr,"aqui3");
    /*
      dvst.init
      dvst.active_frame
      dvst.first_clear_frame
      dvst.n_clear_frames
      dvst.dropped_frames
    */	
    if (dvst.dropped_frames > 0) {
      verbose(1,"dv1394: dropped at least %d frames", dvst.dropped_frames);
    }
    /*
      memcpy( g_current_frame->data, 
      (g_dv1394_map + (dvst.first_clear_frame * DV1394_PAL_FRAME_SIZE)),
      DV1394_PAL_FRAME_SIZE );
    */
    x->videobuf = mmapbuf + (dvst.first_clear_frame * framesize);

    //post("thread %d\t%x %x", me->frame, me->tvfd, me->vmmap);
    if (ioctl(fd, DV1394_RECEIVE_FRAMES, 1) < 0)    {
      perror("receiving...");
    }
    x->x_lastframe=x->x_frame;
    x->x_frame++;
    x->x_frame%=N_BUF;
    x->x_frame_ready = true;
  }
  x->x_capturing=false;

  //process_image  (x, x->videobuf);

    return 0;
}

static void
close_device                    (t_pdp_ieee1394 *x)
{
  if(x->x_mmapbuf!=NULL)munmap(x->x_mmapbuf, N_BUF*x->x_framesize);
  if(x->dvfd>=0)close(x->dvfd);
  x->x_haveVideo=false;

}


static int
startTransfer (t_pdp_ieee1394 *x)
{
  //if ((x->dvfd=openDevice(format))<0){
  //  verbose(1, "DV4L: closed");
  //  return(0);
  //}
  //x->x_image.newimage=0;
  //x->x_image.image.data=0;
  //x->x_image.image.xsize=720;
  //x->x_image.image.ysize=576;
  //x->x_image.image.setCsizeByFormat(x->x_reqFormat);
  //x->x_image.image.reallocate();
  x->videobuf=NULL;

  x->x_frame_ready = false; 

  if(x->x_decoder!=NULL)dv_decoder_free(x->x_decoder);
  if (!(x->x_decoder=dv_decoder_new(1, 1, 1))){
    //error("DV4L: unable to create DV-decoder...closing");
    close_device(x);
    return(0);
  }
  //x->x_decoder->quality=x->x_quality;
  x->x_decoder->quality = DV_QUALITY_BEST;
  verbose(1, "DV4L: DV decoding quality %d ", x->x_decoder->quality);
  //fprintf(stderr,"before");
  x->x_continue_thread = true;
  pthread_create(&x->x_thread_id, 0, pdp_ieee1394_thread, x);
  return 1;
}

static int
stopTransfer                   (t_pdp_ieee1394 *x)
{
  /* close the dv4l device and dealloc buffer */
  /* terminate thread if there is one */
  x->x_continue_thread=false;
  int i=0;
  if(x->x_haveVideo){
    while(x->x_capturing){
      struct timeval sleep;
      sleep.tv_sec=0;  sleep.tv_usec=10; /* 10us */
      select(0,0,0,0,&sleep);
      i++;
    }
    verbose(1, "DV4L: shutting down dv1394 after %d usec", i*10);
    ioctl(x->dvfd, DV1394_SHUTDOWN);
  }
  close_device(x);
  return(1);
}


static void pdp_ieee1394_close(t_pdp_ieee1394 *x)
{
  /* close the v4l device and dealloc buffer */

    void *dummy;
    //x->x_initialized = false;
    /* terminate thread if there is one */
    if(x->x_continue_thread){
	x->x_continue_thread = 0;
	pthread_join (x->x_thread_id, &dummy);
    }


        //stop_capturing (x);

        //uninit_device (x);

        close_device (x);
	
	if (-1 == close (x->dvfd))
	        post ("close");

        x->dvfd = -1;
}


static int pdp_ieee1394_open(t_pdp_ieee1394 *x, t_symbol *name)
{
  x->x_devicename = name->s_name;

  if(x->x_haveVideo){
    verbose(1, "Stream already going on. Doing some clean-up...");
    stopTransfer(x);
  }

  /*
  All of the errors in this method return -1 anyhow, so fd should be 0 to allow
  successful open if everything goes ok.

  Ico Bukvic ico@vt.edu 2-18-07
  */
  int fd = 0; 
  struct dv1394_init init = {
    DV1394_API_VERSION, // api version 
    0x63,              // isochronous transmission channel
    N_BUF,             // number of frames in ringbuffer
    (x->x_norm==NTSC)?DV1394_NTSC:DV1394_PAL,         // PAL or NTSC
    //DV1394_PAL,         // PAL or NTSC
    0, 0 , 0                // default packet rate
  };

  x->x_framesize=(x->x_norm==NTSC)?DV1394_NTSC_FRAME_SIZE:DV1394_PAL_FRAME_SIZE;
  //x->x_framesize=DV1394_PAL_FRAME_SIZE;

  if(x->x_devicename){
    if ((fd = open(x->x_devicename, O_RDWR)) < 0) {
        perror(x->x_devicename);
        return -1;
    }
  } else {
    signed char devnum=(x->x_devicenum<0)?0:(signed char)x->x_devicenum;
    char buf[256];
    buf[255]=0;buf[32]=0;buf[33]=0;
    if (devnum<0)devnum=0;
    snprintf(buf, 32, "/dev/ieee1394/dv/host%d/%s/in", devnum, (x->x_norm==NTSC)?"NTSC":"PAL");
    //snprintf(buf, 32, "/dev/ieee1394/dv/host%d/%s/in", devnum, "PAL");
    if ((fd = open(buf, O_RDWR)) < 0)    {
      snprintf(buf, 32, "/dev/dv1394/%d", devnum);
      if ((fd = open(buf, O_RDWR)) < 0) {
	if ((fd=open("/dev/dv1394", O_RDWR)) < 0)    {
	  perror(buf);
	  return -1;
	}
      }
    }
  }
  if (ioctl(fd, DV1394_INIT, &init) < 0)    {
    perror("initializing");
    close(fd);
    return -1;
  }
  
  x->x_mmapbuf = (unsigned char *) mmap( NULL, N_BUF*x->x_framesize,
				       PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if(x->x_mmapbuf == MAP_FAILED) {
    perror("mmap frame buffers");
    close(fd);
    return -1;
  }
  
  if(ioctl(fd, DV1394_START_RECEIVE, NULL)) {
    perror("dv1394 START_RECEIVE ioctl");
    close(fd);
    return -1;
  }
  /*Extra verbosity never hurt anyone...

  Ico Bukvic ico@vt.edu 2-18-07
  */
  post("DV4L: Successfully opened...");
  startTransfer(x);
  x->dvfd=fd;

  return 1;
	
}


static int pdp_ieee1394_norm(t_pdp_ieee1394 *x, t_symbol *s)
{
  int inorm = x->x_norm;
  char* norm=s->s_name;
  switch(norm[0]){
  case 'N': case 'n':
    inorm=NTSC;
    break;
  case 'P': case 'p':
    inorm=PAL;
    break;
  }
  if (inorm==x->x_norm)return 0;
  x->x_norm=inorm;
  return 0;
}


static void pdp_ieee1394_bang(t_pdp_ieee1394 *x)
{
   
  /* if initialized, grab a frame and output it */



    /* convert data to pdp packet */
/*
    switch(x->x_v4l_palette){
    case  VIDEO_PALETTE_YUV420P:
	pdp_llconv(newimage, RIF_YUV__P411_U8, data, RIF_YVU__P411_S16, w, h); 
	break;*/
	
	/* long live standards. v4l's rgb is in fact ogl's bgr */
/*    case  VIDEO_PALETTE_RGB24:
	pdp_llconv(newimage, RIF_BGR__P____U8, data, RIF_YVU__P411_S16, w, h); 
	break;

    case  VIDEO_PALETTE_RGB32:
	pdp_llconv(newimage, RIF_BGRA_P____U8, data, RIF_YVU__P411_S16, w, h); 
	break;

    case  VIDEO_PALETTE_YUV422:
	pdp_llconv(newimage, RIF_YUYV_P____U8, data, RIF_YVU__P411_S16, w, h); 
	break;*/


    /*default:
	post("pdp_ieee1394: unsupported palette");
	break;
    }*/

/*
    if (PDP_IMAGE_YV12 == x->x_pdp_image_type){
	pixel_unpack_u8s16_y(&newimage[0], data, nbpixels>>3, x->x_state_data->gain);
	pixel_unpack_u8s16_uv(&newimage[plane1], &data[plane2], nbpixels>>5, x->x_state_data->gain);
	pixel_unpack_u8s16_uv(&newimage[plane2], &data[plane1], nbpixels>>5, x->x_state_data->gain);
    }
*/
    //x->x_v4l_palette = VIDEO_PALETTE_YUV420P;
    //x->x_v4l_palette = VIDEO_PALETTE_RGB24;

/*

    else if(PDP_IMAGE_GREY == x->x_pdp_image_type){
	pixel_unpack_u8s16_y(&newimage[0], data, nbpixels>>3, x->x_state_data->gain);
    }
*/
    //post("pdp_ieee1394: mark unused %d", object);

    /*pdp_packet_pass_if_valid(x->x_outlet0, &object);*/

}



static void pdp_ieee1394_free(t_pdp_ieee1394 *x)
{
  //pdp_ieee1394_close(x);
  if(x->x_haveVideo)stopTransfer(x);
  //if(x->decodedbuf)delete[]decodedbuf;
  if(x->x_decoder!=NULL)dv_decoder_free(x->x_decoder);
}

t_class *pdp_ieee1394_class;



void *pdp_ieee1394_new(t_symbol *vdef, t_symbol *format)
{
    t_pdp_ieee1394 *x = (t_pdp_ieee1394 *)pd_new(pdp_ieee1394_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);

    //x->x_channel = 0;//0x63;
    x->x_devicenum  = 0;
    x->x_norm = PAL;
    x->x_decoder=NULL;
    x->x_frame_ready=false;
    x->x_width=720;
    x->x_height=576;
    x->x_framesize=DV1394_PAL_FRAME_SIZE;
    //x->x_quality = DV_QUALITY_BEST;
    //x->decodedbuf = new unsigned char[720*576*3];
    x->decodedbuf = malloc (720*576*3*sizeof(unsigned char));
    x->x_haveVideo=false;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_ieee1394_setup(void)
{
    // post( " pdp_ieee1394 : linux dv interface by Lluis Gomez i Bigorda (lluisgomez@hangar.org)" );

    pdp_ieee1394_class = class_new(gensym("pdp_ieee1394"), (t_newmethod)pdp_ieee1394_new,
    	(t_method)pdp_ieee1394_free, sizeof(t_pdp_ieee1394), 0, A_DEFSYMBOL, A_DEFSYMBOL, A_NULL);


    class_addmethod(pdp_ieee1394_class, (t_method)pdp_ieee1394_read_frame, gensym("bang"), A_NULL);
    class_addmethod(pdp_ieee1394_class, (t_method)pdp_ieee1394_close, gensym("close"), A_NULL);
    class_addmethod(pdp_ieee1394_class, (t_method)pdp_ieee1394_open, gensym("open"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_ieee1394_class, (t_method)pdp_ieee1394_norm, gensym("norm"), A_SYMBOL, A_NULL);

}

#ifdef __cplusplus
}
#endif
