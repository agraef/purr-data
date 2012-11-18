/*
 *   PiDiP module.
 *   Copyright (c) by Yves Degoyon (ydegoyon@free.fr)
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

/*  This object is a ieee1394 video input object for OSX, using QuickTime
 *  Some code is inspired by pix_video from Gem
 *  Written by Yves Degoyon
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
#include <sys/mman.h>
#include <sched.h>
#include <pthread.h>
#include <Carbon/Carbon.h>
#include <Quicktime/QuickTime.h>
#include <Quicktime/QuickTimeComponents.h>

#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240

typedef struct pdp_ieee1394_struct
{
  t_object x_obj;
  t_float x_f;
  
  t_outlet *x_outlet0;

  bool x_initialized;
  bool x_auto_open;

  int        x_packet;
  t_pdp*       x_header;
  short int    *x_data;
  unsigned char *x_sdata; // static data to hold the grabbed images

  unsigned int x_width;
  unsigned int x_height;
  unsigned int x_size;
  int          x_channel;
  pthread_t    x_thread_id;
  int          x_continue_thread;
  unsigned int x_framerate;
  int          x_frame_ready;
  int        x_quality;

  SeqGrabComponent x_sg;   
  SGChannel        x_vc;  
  short            x_pixelDepth;
  Rect             x_srcRect;  
  GWorldPtr        x_srcGWorld;
  PixMapHandle     x_pixMap;  
  Ptr              x_baseAddr;
  long             x_rowBytes;

} t_pdp_ieee1394;

static void pdp_ieee1394_close(t_pdp_ieee1394 *x)
{
  void *dummy;

    /* terminate thread if there is one */
    if(x->x_continue_thread)
    {
	x->x_continue_thread = 0;
	// pthread_join (x->x_thread_id, &dummy);
    }

    // free sequence grabber
    // if (x->x_vc) 
    // {
    //   if (SGDisposeChannel(x->x_sg, x->x_vc)) 
    //   {
    //     post("pdp_ieee1394: unable to dispose video channel");
    //   }
    //   x->x_vc = NULL;
    //   post("pdp_ieee1394: disposed video channel");
    // }
    // if (x->x_sg) 
    // {
    //   if (CloseComponent(x->x_sg)) 
    //   {
    //     post("pdp_ieee1394: unable to free sequence grabber.");
    //   }
    //   x->x_sg = NULL;
    //   post("pdp_ieee1394: freed sequence grabber.");
    // }
    // if (x->x_srcGWorld) 
    // {
    //     DisposeGWorld(x->x_srcGWorld);
    //     post("pdp_ieee1394: disposed world.");
    //     x->x_srcGWorld = NULL;
    // }

}

static void pdp_ieee1394_capture_frame(t_pdp_ieee1394* x)
{
  OSErr       err;

    err = SGIdle(x->x_sg);
    if (err != noErr)
    {
      post("pdp_ieee1394: SGIdle failed.");
      x->x_frame_ready = 0;
    } 
    else 
    {
      x->x_frame_ready = 1;
    }
}


static void *pdp_ieee1394_thread(void *voidx)
{
  t_pdp_ieee1394 *x = ((t_pdp_ieee1394 *)voidx);

    /* capture with a double buffering scheme */
    while (true)
    {
      if (x->x_continue_thread)
      {
	/* schedule capture command for next frame */
	pdp_ieee1394_capture_frame(x);
      }
      else
      {
        sleep(1);
      }
    }

    x->x_thread_id = 0;
    return 0;
}

static void pdp_ieee1394_reset(t_pdp_ieee1394 *x)
{
  OSErr anErr;

  if ( !x->x_initialized )
  {
    post("pdp_ieee1394: trying to reset but the sequence grabber is not initialized");
    return;
  }

  post("pdp_ieee1394: resetting....");

  switch (x->x_quality)
  {
    case 0:
        anErr = SGSetChannelPlayFlags(x->x_vc, channelPlayNormal);
        post("pdp_ieee1394: set sequence grabber to : normal quality");
        break;
    case 1:
        anErr = SGSetChannelPlayFlags(x->x_vc, channelPlayHighQuality);
        post("pdp_ieee1394: set sequence grabber to : high quality");
        break;
    case 2:
        anErr = SGSetChannelPlayFlags(x->x_vc, channelPlayFast);
        post("pdp_ieee1394: set sequence grabber to : fast quality");
        break;
    case 3:
        anErr = SGSetChannelPlayFlags(x->x_vc, channelPlayAllData);
        post("pdp_ieee1394: set sequence grabber to : play all data");
        break;
    }

  post("pdp_ieee1394: done.");
}

static void pdp_ieee1394_quality(t_pdp_ieee1394 *x, t_floatarg fquality)
{
  if ( ( (int)fquality < 0 ) || ( (int)fquality > 3 ) )
  {
     post("pdp_ieee1394: wrong quality %d", (int)fquality );
     return;
  }
  else
  {
     x->x_quality = (int)fquality;
  }
}

static void pdp_ieee1394_free(t_pdp_ieee1394 *x)
{
    pdp_ieee1394_close(x);
}

static int pdp_ieee1394_init_grabber(t_pdp_ieee1394 *x)
{
  OSErr anErr;
  x->x_srcRect.top = 0;
  x->x_srcRect.left = 0;
  x->x_srcRect.bottom = x->x_height;
  x->x_srcRect.right = x->x_width;

  x->x_sg = OpenDefaultComponent(SeqGrabComponentType, 0);
  if(x->x_sg==NULL)
  {
    post("pdp_ieee1394: could not open default component");
    return -1;
  }    
  else
  {
    post("pdp_ieee1394: opened default component");
  }

  anErr = SGInitialize(x->x_sg);
  if(anErr!=noErr)
  {
    post("pdp_ieee1394: could not initialize sequence grabber");
    return -1;
  }
  else
  {
    post("pdp_ieee1394: initialized sequence grabber");
  }

  anErr = SGSetDataRef(x->x_sg, 0, 0, seqGrabDontMakeMovie);
  if (anErr != noErr)
  {
    post("pdp_ieee1394: couldn't set data ref");
    return -1;
  }
  else
  {
    post("pdp_ieee1394: set data ref ok.");
  }

  anErr = SGNewChannel(x->x_sg, VideoMediaType, &x->x_vc);
  if(anErr!=noErr)
  {
    post("pdp_ieee1394: could not create new sequence grabber channnel");
    return -1;
  }
  else
  {
    post("pdp_ieee1394: created new sequence grabber channnel.");
  }

  anErr = SGSetChannelBounds(x->x_vc, &x->x_srcRect);
  if(anErr!=noErr)
  {
    post("pdp_ieee1394: could not set sequence grabber ChannelBounds ");
    return -1;
  }
  else
  {
    post("pdp_ieee1394: set sequence grabber ChannelBounds");
  }

  anErr = SGSetChannelUsage(x->x_vc, seqGrabPreview);
  if(anErr!=noErr)
  {
    post("pdp_ieee1394: could not set sequence grabber ChannelUsage ");
    return -1;
  }
  else
  {
    post("pdp_ieee1394: set sequence grabber ChannelUsage");
  }

  switch (x->x_quality)
  {
    case 0:
        anErr = SGSetChannelPlayFlags(x->x_vc, channelPlayNormal);
        post("pdp_ieee1394: set sequence grabber to : normal quality");
        break;
    case 1:
        anErr = SGSetChannelPlayFlags(x->x_vc, channelPlayHighQuality);
        post("pdp_ieee1394: set sequence grabber to : high quality");
        break;
    case 2:
        anErr = SGSetChannelPlayFlags(x->x_vc, channelPlayFast);
        post("pdp_ieee1394: set sequence grabber to : fast quality");
        break;
    case 3:
        anErr = SGSetChannelPlayFlags(x->x_vc, channelPlayAllData);
        post("pdp_ieee1394: set sequence grabber to : play all data");
        break;
  }

  anErr = QTNewGWorldFromPtr (&x->x_srcGWorld,
                              k422YpCbCr8CodecType,
                              &x->x_srcRect,
                              NULL,
                              NULL,
                              0,
                              x->x_sdata,
                              x->x_width*2);
  if (anErr!= noErr)
  {
    post ("pdp_ieee1394: QTNewGWorldFromPtr returned %d", anErr);
    return -1;
  }

  if (NULL == x->x_srcGWorld)
  {
    post ("pdp_ieee1394: could not allocate off screen");
    return -1;
  }
  SGSetGWorld(x->x_sg,(CGrafPtr)x->x_srcGWorld, NULL);
  SGStartPreview(x->x_sg);

  return 0;
}

static void pdp_ieee1394_open(t_pdp_ieee1394 *x)
{

    x->x_initialized = true;
    x->x_continue_thread = 1;

    /* create thread */
    if ( x->x_thread_id == 0 )
    {
      if ( pdp_ieee1394_init_grabber( x ) != 0 )
      {
        post("pdp_ieee1394: grabber initialization failed");
        return;
      }
      x->x_frame_ready = 0;
      pthread_create(&x->x_thread_id, 0, pdp_ieee1394_thread, x);
    }
}

static void pdp_ieee1394_bang(t_pdp_ieee1394 *x)
{
  unsigned char *pQ;
  short int *pY, *pU, *pV;
  int px, py;

    if (!(x->x_continue_thread))
    {
      post("pdp_ieee1394: not initialized.");

      if (x->x_auto_open)
      {
        post("pdp_ieee1394: attempting auto open");
        pdp_ieee1394_open(x);
        if (!(x->x_initialized))
        {
          post("pdp_ieee1394: auto open failed");
          return;
        }
      }
      else return;
    }

    /* do nothing if there is no frame ready */
    if (!x->x_frame_ready) return;

    x->x_packet = pdp_packet_new_image_YCrCb(x->x_width, x->x_height);
    x->x_header = pdp_packet_header(x->x_packet);

    if (!x->x_header) 
    {
      post("pdp_ieee1394: FATAL: can't allocate packet");
      return;
    }

    x->x_data = (short int *) pdp_packet_data(x->x_packet);
    memset( x->x_data, 0x0, (x->x_size+(x->x_size>>1))<<1 );
    pQ = x->x_sdata;
    pY = x->x_data;
    pV = x->x_data+x->x_size;
    pU = x->x_data+x->x_size+(x->x_size>>2);
    for ( py=0; py<(int)x->x_height; py++ )
    {
      for ( px=0; px<(int)x->x_width; px++ )
      {
        *(pY+py*x->x_width+px) = (*(pQ+1+2*(py*x->x_width+px)))<<7;
        if ( px%2 == 0 )
        {
          *(pU+((py>>1)*(x->x_width>>1)+(px>>1))) = (*(pQ+2*(py*x->x_width+px))-128)<<8;
        }
        if ( px%2 == 1 )
        {
          *(pV+((py>>1)*(x->x_width>>1)+(px>>1))) = (*(pQ+2*(py*x->x_width+px))-128)<<8;
        }
      }
    }

    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet);

    x->x_frame_ready = 0;
}

t_class *pdp_ieee1394_class;

void *pdp_ieee1394_new(t_floatarg fwidth, t_floatarg fheight)
{
  t_pdp_ieee1394 *x = (t_pdp_ieee1394 *)pd_new(pdp_ieee1394_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);
    x->x_initialized = false;

    x->x_auto_open = true;

    x->x_continue_thread = 0;

    if (fwidth > 0.)
    {
      x->x_width = (int)fwidth;
    }
    else
    {
      x->x_width = DEFAULT_WIDTH;
    }

    if (fheight > 0.)
    {
      x->x_height = (int)fheight;
    }
    else
    {
      x->x_height = DEFAULT_WIDTH;
    }
    x->x_size = x->x_width*x->x_height;
    x->x_sdata = (unsigned char*) getbytes( (x->x_size+(x->x_size>>1))<<1 );
    if ( !x->x_sdata )
    {
      post ("pdp_ieee1394: FATAL : couldn't allocate static data.");
      return NULL;
    }

    x->x_quality = 1;
    x->x_thread_id = 0;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_ieee1394_setup(void)
{
    pdp_ieee1394_class = class_new(gensym("pdp_ieee1394"), (t_newmethod)pdp_ieee1394_new,
    	(t_method)pdp_ieee1394_free, sizeof(t_pdp_ieee1394), 0, A_DEFFLOAT, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_ieee1394_class, (t_method)pdp_ieee1394_bang, gensym("bang"), A_NULL);
    class_addmethod(pdp_ieee1394_class, (t_method)pdp_ieee1394_close, gensym("close"), A_NULL);
    class_addmethod(pdp_ieee1394_class, (t_method)pdp_ieee1394_open, gensym("open"), A_NULL);
    class_addmethod(pdp_ieee1394_class, (t_method)pdp_ieee1394_reset, gensym("reset"), A_NULL);
    class_addmethod(pdp_ieee1394_class, (t_method)pdp_ieee1394_quality, gensym("quality"), A_DEFFLOAT, A_NULL);
}

#ifdef __cplusplus
}
#endif
