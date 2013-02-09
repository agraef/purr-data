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

/*  This object is a vloopback output module,
 *  inspired by the code of mjpegtools_yuv_to_v4l by Jan Panteltje
 *  it enables you to send pd output to another application
 *  that supports v4l input
 */


#include "pdp_config.h"
#include "pdp.h"
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
#include <libv4l1-videodev.h>

typedef struct pdp_vloopback_struct
{
  t_object x_obj;
  t_float x_f;
  
  t_symbol *x_device;
  int x_vlfd;

  int x_packet0;
  int x_packet1;
  int x_dropped;
  int x_queue_id;
  
  t_outlet *x_outlet0;
  t_outlet *x_outlet1;

  int x_nbframes;
  int x_invertrb;

  struct video_window x_vidwin;
  struct video_picture x_vidpic;

  bool x_initialized;

  unsigned int x_width;
  unsigned int x_height;

} t_pdp_vloopback;

static void pdp_vloopback_close(t_pdp_vloopback *x)
{
  // close the vloopback device
  if ( close( x->x_vlfd ) == -1 )
  {
     post( "could not close device %s", x->x_device->s_name );
     perror( "closing device" );
  }
  else
  {
     post( "closed device %s", x->x_device->s_name );
  }
  
  x->x_initialized=0;
  x->x_nbframes=0;

}

static void pdp_vloopback_open(t_pdp_vloopback *x, t_symbol *name)
{
  // open a vloopback device and allocate buffers

  // check if the vloopback module is loaded
  // it might be the one of webcamstudio called webcamstudio
  int module_loaded=0;
  int ret;

  x->x_device=name;

#ifdef __LINUX__

  if ( ( ret = system( "/sbin/lsmod | grep vloopback" ) ) == 256 )
  {
     post( "pdp_vloopback : vloopback module is not loaded (ret=%d)", ret );
  } 
  else
  {
     post( "pdp_vloopback : vloopback module is loaded" );
     module_loaded=1;
  }

  if ( ( ret = system( "/sbin/lsmod | grep webcamstudio" ) ) == 256 )
  {
     post( "pdp_vloopback : webcamstudio module is not loaded (ret=%d)", ret );
  } 
  else
  {
     post( "pdp_vloopback : alternate module webcamstudio is loaded" );
     module_loaded=1;
  }

  if ( module_loaded==0 )
  {
     post( "pdp_vloopback : refusing to proceed" );
     return;
  }

#endif

  if ( x->x_initialized )
  {
     pdp_vloopback_close(x);
  }

  // open the device
  if ( ( x->x_vlfd = open (name->s_name, O_RDWR) ) < 0 )
  {
     post( "pdp_vloopback : unable to open : %s", name->s_name );
     perror( "open device" );
     return;
  }

  // setting palette to RGB24
  if( ioctl(x->x_vlfd, VIDIOCGPICT, &x->x_vidpic) == -1)
  {
     post("pdp_vloopback : could get palette ( VIDIOCGPICT )");
     perror( "ioctl VIDIOCGPICT" );
     return;
  }

  x->x_vidpic.palette = VIDEO_PALETTE_RGB24;
  if( ioctl(x->x_vlfd, VIDIOCSPICT, &x->x_vidpic) == -1)
  {
     post("pdp_vloopback : could set palette ( VIDIOCSPICT )");
     perror( "ioctl VIDIOCSPICT" );
     return;
  }

  // setting window size
  if(ioctl (x->x_vlfd, VIDIOCGWIN, &x->x_vidwin) == -1)
  {
     post("pdp_vloopback : could get dimensions ( VIDIOCGWIN )");
     perror( "ioctl VIDIOCGWIN" );
     return;
  }

  x->x_vidwin.width = x->x_width;
  x->x_vidwin.height = x->x_height;

  if(ioctl (x->x_vlfd, VIDIOCSWIN, &x->x_vidwin) == -1)
  {
     post("pdp_vloopback : could set dimensions ( VIDIOCSWIN )");
     perror( "ioctl VIDIOCSWIN" );
     return;
  }

  // all went well we are now initialized 
  x->x_initialized=1;
  post( "pdp_vloopback : opened device : %s", x->x_device->s_name );

}

static void pdp_vloopback_setlegaldim(t_pdp_vloopback *x, int xx, int yy)
{
    x->x_width = pdp_imageproc_legalwidth((int)xx);
    x->x_height = pdp_imageproc_legalheight((int)yy);
}

static void pdp_vloopback_dim(t_pdp_vloopback *x, t_floatarg xx, t_floatarg yy)
{
    pdp_vloopback_setlegaldim(x, (int)xx, (int)yy);
    if (x->x_initialized){
        pdp_vloopback_close(x);
    }
    pdp_vloopback_open(x, x->x_device);
}

static void pdp_vloopback_process_rgb(t_pdp_vloopback *x)
{
  t_pdp *header = pdp_packet_header(x->x_packet0);
  char *data = (char *)pdp_packet_data(x->x_packet0);
  char cvalue;
  unsigned int px,py;
  int written, flength;

  if ( !x->x_initialized )
  {
     post( "pdp_vloopback : no vloopback device opened" );
     return;
  }

  if ( (header->info.image.width != x->x_width ) ||
       (header->info.image.height != x->x_height ) )
  {
     pdp_vloopback_dim(x, header->info.image.width, header->info.image.height);
  }

  // output the image
  flength = x->x_width*x->x_height*3;

  // invert red and blue channels ( with webcamstudio module )
  if ( x->x_invertrb )
  {
     for ( py=0; py<x->x_height; py++ )
     {
       for ( px=0; px<x->x_width; px++ )
       {
           cvalue=*(data+3*py*x->x_width+3*px);
           *(data+3*py*x->x_width+3*px)=*(data+3*py*x->x_width+3*px+2);
           *(data+3*py*x->x_width+3*px+2)=cvalue;
       }
     }
  }

  if ( ( written = write(x->x_vlfd, data, flength) ) != flength )
  {
     post( "pdp_vloopback : problem writing frame...closing" );
     post( "pdp_vloopback : wrote %d bytes out of %d", written, flength );
  }
  else
  {
     outlet_float( x->x_outlet1, x->x_nbframes++);
  }

}

static void pdp_vloopback_sendpacket(t_pdp_vloopback *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet0);
}

static void pdp_vloopback_process(t_pdp_vloopback *x)
{
   int encoding;
   t_pdp *header = 0;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet0))
        && (PDP_BITMAP == header->type)){

        /* pdp_vloopback_process inputs and write into active inlet */
        switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

        case PDP_BITMAP_RGB:
            pdp_queue_add(x, pdp_vloopback_process_rgb, pdp_vloopback_sendpacket, &x->x_queue_id);
            break;

        default:
            /* don't know the type, so dont pdp_vloopback_process */
            break;

        }
    }
}

static void pdp_vloopback_input_0(t_pdp_vloopback *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s== gensym("register_rw"))
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped)){

        /* add the process method and callback to the process queue */
        pdp_vloopback_process(x);

    }
}

static void pdp_vloopback_invertrb(t_pdp_vloopback *x, t_floatarg finvert)
{
    if ( ((int)finvert == 0) || ((int)finvert == 1) )
    {
       x->x_invertrb = (int)finvert;
    }
}

static void pdp_vloopback_free(t_pdp_vloopback *x)
{
    pdp_vloopback_close(x);
}

t_class *pdp_vloopback_class;

void *pdp_vloopback_new(t_symbol *vdev)
{
    t_pdp_vloopback *x = (t_pdp_vloopback *)pd_new(pdp_vloopback_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);
    x->x_outlet1 = outlet_new(&x->x_obj, &s_float);

    x->x_initialized = false;
    x->x_nbframes = 0;
    x->x_invertrb = 1;

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width = 320;
    x->x_height = 240;

    x->x_vlfd = -1;
    if (vdev != gensym("")){
        x->x_device = vdev;
        pdp_vloopback_open(x, x->x_device);
    }
    else
    {
        x->x_device = gensym("/dev/video1");
    }

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_vloopback_setup(void)
{
    pdp_vloopback_class = class_new(gensym("pdp_vloopback"), (t_newmethod)pdp_vloopback_new,
    	(t_method)pdp_vloopback_free, sizeof(t_pdp_vloopback), CLASS_DEFAULT, A_DEFSYMBOL, A_DEFSYMBOL, A_NULL);

    class_addmethod(pdp_vloopback_class, (t_method)pdp_vloopback_close, gensym("close"), A_NULL);
    class_addmethod(pdp_vloopback_class, (t_method)pdp_vloopback_open, gensym("open"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_vloopback_class, (t_method)pdp_vloopback_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_vloopback_class, (t_method)pdp_vloopback_invertrb, gensym("invert"), A_DEFFLOAT, A_NULL);

}

#ifdef __cplusplus
}
#endif
