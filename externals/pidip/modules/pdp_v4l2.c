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
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sched.h>
#include <pthread.h>

// dont open any more after a set number 
// of failed attempts
// this is to prevent locks on auto-open
// is reset when manually opened or closed
#define PDP_XV_RETRIES 10


#define DEVICENO 0
#define NBUF 2
#define COMPOSITEIN 1
#define MAX_BUFFERS 8
#define MAX_INPUT   16
#define MAX_NORM    16
#define MAX_FORMAT  32
#define MAX_CTRL    32


#define DO_SANITY_CHECKS 0

static const int UVTranslate[32] = {0, 1, 2, 3, 
			8, 9, 10, 11, 
			16, 17, 18, 19, 
			24, 25, 26, 27, 
			4, 5, 6, 7,
			12, 13, 14, 15,
			20, 21, 22, 23,
			28, 29, 30, 31};

static const int Y_coords_624x[128][2] = {
{ 0,  0}, { 1,  0}, { 2,  0}, { 3,  0}, { 4,  0}, { 5,  0}, { 6,  0}, { 7,  0},
{ 0,  1}, { 1,  1}, { 2,  1}, { 3,  1}, { 4,  1}, { 5,  1}, { 6,  1}, { 7,  1},
{ 0,  2}, { 1,  2}, { 2,  2}, { 3,  2}, { 4,  2}, { 5,  2}, { 6,  2}, { 7,  2},
{ 0,  3}, { 1,  3}, { 2,  3}, { 3,  3}, { 4,  3}, { 5,  3}, { 6,  3}, { 7,  3},

{ 0,  4}, { 1,  4}, { 2,  4}, { 3,  4}, { 4,  4}, { 5,  4}, { 6,  4}, { 7,  4},
{ 0,  5}, { 1,  5}, { 2,  5}, { 3,  5}, { 4,  5}, { 5,  5}, { 6,  5}, { 7,  5},
{ 0,  6}, { 1,  6}, { 2,  6}, { 3,  6}, { 4,  6}, { 5,  6}, { 6,  6}, { 7,  6},
{ 0,  7}, { 1,  7}, { 2,  7}, { 3,  7}, { 4,  7}, { 5,  7}, { 6,  7}, { 7,  7},

{ 8,  0}, { 9,  0}, {10,  0}, {11,  0}, {12,  0}, {13,  0}, {14,  0}, {15,  0},
{ 8,  1}, { 9,  1}, {10,  1}, {11,  1}, {12,  1}, {13,  1}, {14,  1}, {15,  1},
{ 8,  2}, { 9,  2}, {10,  2}, {11,  2}, {12,  2}, {13,  2}, {14,  2}, {15,  2},
{ 8,  3}, { 9,  3}, {10,  3}, {11,  3}, {12,  3}, {13,  3}, {14,  3}, {15,  3},

{ 8,  4}, { 9,  4}, {10,  4}, {11,  4}, {12,  4}, {13,  4}, {14,  4}, {15,  4},
{ 8,  5}, { 9,  5}, {10,  5}, {11,  5}, {12,  5}, {13,  5}, {14,  5}, {15,  5},
{ 8,  6}, { 9,  6}, {10,  6}, {11,  6}, {12,  6}, {13,  6}, {14,  6}, {15,  6},
{ 8,  7}, { 9,  7}, {10,  7}, {11,  7}, {12,  7}, {13,  7}, {14,  7}, {15,  7}
};

static void do_write_u(const unsigned char *buf, unsigned char *ptr,
	int i, int j)
{
	*ptr = buf[i + 128 + j];
}

static void do_write_v(const unsigned char *buf, unsigned char *ptr,
	int i, int j)
{
	*ptr = buf[i + 160 + j];
}


void v4lconvert_sn9c20x_to_yuv420(const unsigned char *raw, unsigned char *i420,
  int width, int height, int yvu)
{
	int i = 0, x = 0, y = 0, j, relX, relY, x_div2, y_div2;
	const unsigned char *buf = raw;
	unsigned char *ptr;
	int frame_size = width * height;
	int frame_size_div2 = frame_size >> 1;
	int frame_size_div4 = frame_size >> 2;
	int width_div2 = width >> 1;
	int height_div2 = height >> 1;
	void (*do_write_uv1)(const unsigned char *buf, unsigned char *ptr, int i,
	int j) = NULL;
	void (*do_write_uv2)(const unsigned char *buf, unsigned char *ptr, int i,
	int j) = NULL;

	if (yvu) {
		do_write_uv1 = do_write_v;
		do_write_uv2 = do_write_u;
	}
	else {
		do_write_uv1 = do_write_u;
		do_write_uv2 = do_write_v;
	}

	while (i < (frame_size + frame_size_div2)) {
		for (j = 0; j < 128; j++) {
			relX = x + Y_coords_624x[j][0];
			relY = y + Y_coords_624x[j][1];

#if (DO_SANITY_CHECKS==1)
			if ((relX < width) && (relY < height)) {
#endif
				ptr = i420 + relY * width + relX;
				*ptr = buf[i + j];
#if (DO_SANITY_CHECKS==1)
			}
#endif

		}
		x_div2 = x >> 1;
		y_div2 = y >> 1;
		for (j = 0; j < 32; j++) {
			relX = (x_div2) + (j & 0x07);
			relY = (y_div2) + (j >> 3);

#if (DO_SANITY_CHECKS==1)
			if ((relX < width_div2) && (relY < height_div2)) {
#endif
				ptr = i420 + frame_size +
					relY * width_div2 + relX;
				do_write_uv1(buf, ptr, i, j);
				ptr += frame_size_div4;
				do_write_uv2(buf, ptr, i, j);
#if (DO_SANITY_CHECKS==1)
			}
#endif
		}

		i += 192;
		x += 16;
		if (x >= width) {
			x = 0;
			y += 8;
		}
	}
}

typedef struct pdp_v4l2_struct
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
  int x_nbbuffers;

  struct v4l2_capability x_vcap;
  struct v4l2_input x_inputs[MAX_INPUT];
  struct v4l2_standard x_standards[MAX_NORM];
  struct v4l2_fmtdesc x_formats[MAX_FORMAT];
  struct v4l2_streamparm x_streamparam;
  struct v4l2_queryctrl x_controls[MAX_CTRL*2];
  struct v4l2_buffer x_v4l2_buf[MAX_BUFFERS];
  struct v4l2_format x_v4l2_format;
  struct v4l2_requestbuffers x_reqbufs;

  unsigned char *x_pdp_buf[MAX_BUFFERS];
 
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
} t_pdp_v4l2;

static void pdp_v4l2_close(t_pdp_v4l2 *x)
{
  /* close the v4l device and dealloc buffer */

  void *dummy;
  int i;

    /* terminate thread if there is one */
    if(x->x_continue_thread){
	x->x_continue_thread = 0;
	pthread_join (x->x_thread_id, &dummy);
    }

    if (x->x_tvfd >= 0)
    {
        close(x->x_tvfd);
        x->x_tvfd = -1;
    }

    if (x->x_initialized){
        for( i=0; i<x->x_nbbuffers; i++ )
        {
           munmap(x->x_pdp_buf[i], x->x_v4l2_buf[i].length);
        }
	x->x_initialized = false;
    }

}

static void pdp_v4l2_close_manual(t_pdp_v4l2 *x)
{
    x->x_open_retry = PDP_XV_RETRIES;
    pdp_v4l2_close(x);
}

static void pdp_v4l2_close_error(t_pdp_v4l2 *x)
{
    pdp_v4l2_close(x);
    if(x->x_open_retry) x->x_open_retry--;
}

static int pdp_v4l2_capture_frame(t_pdp_v4l2* x)
{
    x->x_v4l2_buf[x->x_frame].index  = x->x_frame;
    x->x_v4l2_buf[x->x_frame].type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    x->x_v4l2_buf[x->x_frame].memory = V4L2_MEMORY_MMAP;
 
    if (-1 == ioctl (x->x_tvfd, VIDIOC_DQBUF, &x->x_v4l2_buf[x->x_frame])) 
    {
       switch (errno) 
       {
          case EAGAIN:
            return 0;

          case EIO:
           // could ignore EIO, see spec

          default:
            post( "pdp_v4l2 : error reading buffer : thread exiting");
            exit(-1);
       }
    }

    // reenqueing buffer
    if (-1 == ioctl (x->x_tvfd, VIDIOC_QBUF, &x->x_v4l2_buf[x->x_frame]))
    {
       perror("pdp_v4l2 : error queing buffers : thread exiting");
       exit(-1);
    }

    return 0;
} 

static void pdp_v4l2_wait_frame(t_pdp_v4l2* x)
{
    // wait an event on file descriptor
    fd_set fds;
    struct timeval tv;
    int ret;

    FD_ZERO (&fds);
    FD_SET (x->x_tvfd, &fds);

    // Timeout. 
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    ret = select (x->x_tvfd + 1, &fds, NULL, NULL, &tv);

    if (-1 == ret) {
       if (EINTR == errno) return;
       post ( "pdp_v4l2 : select timeout : closing device");
       pdp_v4l2_close(x);
    }
    if (0 == ret) 
    {
       post ( "pdp_v4l2 : select timeout : closing device");
       pdp_v4l2_close(x);
    }
}

static int pdp_v4l2_start_capturing(t_pdp_v4l2 *x)
{
  enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl (x->x_tvfd, VIDIOC_STREAMON, &type))
    {
       perror("pdp_v4l2 : error starting streaming");
       return -1;
    }
    post("pdp_v4l2 : capture started");
    return 0;
}

static int pdp_v4l2_stop_capturing(t_pdp_v4l2 *x)
{
  enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl (x->x_tvfd, VIDIOC_STREAMOFF, &type))
    {
       perror("pdp_v4l2 : error stopping streaming");
       return -1;
    }
    return 0;
}

static void *pdp_v4l2_thread(void *voidx)
{
    t_pdp_v4l2 *x = ((t_pdp_v4l2 *)voidx);

    x->x_frame = (x->x_frame+1)%x->x_nbbuffers;
    if ( -1 == pdp_v4l2_start_capturing( x ) )
    {
       post( "pdp_v4l2 : problem starting capture.. exiting " );
       exit( -1 );
    }

    /* capture with a double buffering scheme */
    while (x->x_continue_thread)
    {
        /* schedule capture command for next frame */
        pdp_v4l2_wait_frame(x);

        /* wait until previous capture is ready */
        x->x_frame = (x->x_frame+1)%x->x_nbbuffers;
        pdp_v4l2_capture_frame(x);

        /* setup pointers for main thread */
        x->x_frame_ready = 1;
        x->x_last_frame = x->x_frame;
    }

    if ( -1 == pdp_v4l2_stop_capturing( x ) )
    {
       post( "pdp_v4l2 : problem stopping capture.. " );
    }

    post( "pdp_v4l2 : capture thread quitting" );
    return 0;
}

static void pdp_v4l2_setlegaldim(t_pdp_v4l2 *x, int xx, int yy);

static int pdp_v4l2_set_format(t_pdp_v4l2 *x, t_int index)
{
    x->x_v4l2_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    x->x_v4l2_format.fmt.pix.pixelformat = x->x_formats[index].pixelformat;
    x->x_v4l2_format.fmt.pix.field = V4L2_FIELD_ANY;

    post( "pdp_v4l2 : setting format : index : %d : pixel format : %c%c%c%c", 
            index,
            x->x_v4l2_format.fmt.pix.pixelformat & 0xff,
            (x->x_v4l2_format.fmt.pix.pixelformat >>  8) & 0xff,
            (x->x_v4l2_format.fmt.pix.pixelformat >> 16) & 0xff,
            (x->x_v4l2_format.fmt.pix.pixelformat >> 24) & 0xff );

    if (-1 == ioctl(x->x_tvfd, VIDIOC_S_FMT, &x->x_v4l2_format))
    { 
       post( "pdp_v4l2 : defaulting to format : pixel format : %c%c%c%c", 
            x->x_v4l2_format.fmt.pix.pixelformat & 0xff,
            (x->x_v4l2_format.fmt.pix.pixelformat >>  8) & 0xff,
            (x->x_v4l2_format.fmt.pix.pixelformat >> 16) & 0xff,
            (x->x_v4l2_format.fmt.pix.pixelformat >> 24) & 0xff );
    }

    post( "pdp_v4l2 : capture format : width : %d : height :%d : bytesperline : %d : image size : %d",
          x->x_v4l2_format.fmt.pix.width , x->x_v4l2_format.fmt.pix.height,
          x->x_v4l2_format.fmt.pix.bytesperline, x->x_v4l2_format.fmt.pix.sizeimage );
    return 0;
}

static int pdp_v4l2_init_mmap(t_pdp_v4l2 *x)
{
  int i;

    // get mmap numbers 
    x->x_reqbufs.count  = MAX_BUFFERS;
    x->x_reqbufs.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    x->x_reqbufs.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(x->x_tvfd, VIDIOC_REQBUFS, &x->x_reqbufs, 0))
    {
        post( "pdp_v4l2 : error : couldn't init driver buffers" ); 
        return -1;
    }
    post("pdp_v4l2: got %d buffers type %d memory %d", 
        x->x_reqbufs.count, x->x_reqbufs.type, x->x_reqbufs.memory );

    if ( x->x_reqbufs.count > MAX_BUFFERS )
    {
        post( "pdp_v4l2 : this device requires more buffer space" ); 
        return -1;
    }

    x->x_nbbuffers = x->x_reqbufs.count;

    for (i = 0; i < x->x_nbbuffers; i++) 
    {
        x->x_v4l2_buf[i].index  = i;
        x->x_v4l2_buf[i].type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        x->x_v4l2_buf[i].memory = V4L2_MEMORY_MMAP;
        if (-1 == ioctl(x->x_tvfd, VIDIOC_QUERYBUF, &x->x_v4l2_buf[i], 0))
        {
            post( "pdp_v4l2 : error : couldn't query buffer %d", i ); 
            return -1;
        }
        x->x_pdp_buf[i] = (unsigned char *) mmap(NULL, x->x_v4l2_buf[i].length,
                          PROT_READ | PROT_WRITE, MAP_SHARED,
                          x->x_tvfd, x->x_v4l2_buf[i].m.offset);
        if (MAP_FAILED == x->x_pdp_buf[i]) 
        {
            perror("pdp_v4l2 : mmap");
            return -1;
        }
    }
    post( "pdp_v4l2 : mapped %d buffers", x->x_reqbufs.count ); 

    for (i = 0; i < x->x_nbbuffers; i++) 
    {
        x->x_v4l2_buf[i].type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        x->x_v4l2_buf[i].memory      = V4L2_MEMORY_MMAP;
        x->x_v4l2_buf[i].index       = i;

        if (-1 == ioctl (x->x_tvfd, VIDIOC_QBUF, &x->x_v4l2_buf[i]))
        {
            perror("pdp_v4l2 : error queing buffers");
            return -1;
        }
    }
    post( "pdp_v4l2 : queued %d buffers", x->x_reqbufs.count ); 

    return 0;
}

static void pdp_v4l2_open(t_pdp_v4l2 *x, t_symbol *name)
{
    // open a v4l device and allocate a buffer

    unsigned int size;
    int i;

    /* if already opened -> close */
    if (x->x_initialized) pdp_v4l2_close(x);

    /* exit if retried too much */
    if (!x->x_open_retry){
	post("pdp_v4l2: retry count reached zero for %s", name->s_name);
	post("pdp_v4l2: try to open manually");
	return;
    }

    post("pdp_v4l2: opening %s", name->s_name);

    x->x_device = name;

    if ((x->x_tvfd = open(name->s_name, O_RDWR)) < 0)
    {
        post("pdp_v4l2: error: open %s: %s",name->s_name,strerror(errno));
        perror(name->s_name);
        pdp_v4l2_close_error(x);
        x->x_initialized = false;
        return;
    }

    if (ioctl(x->x_tvfd, VIDIOC_QUERYCAP, &x->x_vcap) < 0)
    {
        perror("get capabilities");
        return;
    }

    post("pdp_v4l2: driver info: %s %d.%d.%d / %s @ %s",
          x->x_vcap.driver, (x->x_vcap.version >> 16) & 0xff, (x->x_vcap.version >>  8) & 0xff, x->x_vcap.version & 0xff, x->x_vcap.card, x->x_vcap.bus_info);

    for (x->x_ninputs = 0; x->x_ninputs < MAX_INPUT; x->x_ninputs++) {
        x->x_inputs[x->x_ninputs].index = x->x_ninputs;
        if (-1 == ioctl(x->x_tvfd, VIDIOC_ENUMINPUT, &x->x_inputs[x->x_ninputs]))
        {
            // perror("get inputs");
            break;
        }
        else
        {
            post ("pdp_v4l2 : input %d : %s",  x->x_ninputs, x->x_inputs[x->x_ninputs].name );
        }
    }
    if (x->x_debug) post("pdp_v4l2: device has %d inputs", x->x_ninputs );

    if ( x->x_ninputs > 0 )
    {
      if (ioctl(x->x_tvfd, VIDIOC_S_INPUT, &x->x_curinput) < 0)
      {
          perror("pdp_v4l2: error: VIDIOC_S_INPUT");
          post("pdp_v4l2: cant switch to input %d",x->x_curinput);
      }
      else
      {
         post("pdp_v4l2: switched to input %d", x->x_curinput);
      }

    }

    for (x->x_nstandards = 0; x->x_nstandards < MAX_NORM; x->x_nstandards++) {
        x->x_standards[x->x_nstandards].index = x->x_nstandards;
        if (-1 == ioctl(x->x_tvfd, VIDIOC_ENUMSTD, &x->x_standards[x->x_nstandards]))
        {
            // perror("get standards");
            break;
        }
        else
        {
            post ("pdp_v4l2 : standard %d : %s",  x->x_nstandards, x->x_standards[x->x_nstandards].name );
        }
    }
    if (x->x_debug) post("pdp_v4l2: device supports %d standards", x->x_nstandards );

    // switch to desired norm ( if available )
    if ( x->x_nstandards > 0 )
    {
      if (ioctl(x->x_tvfd, VIDIOC_S_STD, &x->x_curstandard) < 0)
      {
          perror("pdp_v4l2: error: VIDIOC_S_STD");
          post("pdp_v4l2: cant switch to standard %d",x->x_curstandard);
      }
      else
      {
         post("pdp_v4l2: switched to standard %d", x->x_curstandard);
      }
    }

    if (x->x_freq > 0){
        if (ioctl(x->x_tvfd, VIDIOC_S_FREQUENCY, &x->x_freq) < 0)
            perror ("couldn't set frequency :");
    }

    for (x->x_nformats = 0; x->x_nformats < MAX_FORMAT; x->x_nformats++) {
        x->x_formats[x->x_nformats].index = x->x_nformats;
        x->x_formats[x->x_nformats].type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(x->x_tvfd, VIDIOC_ENUM_FMT, &x->x_formats[x->x_nformats]))
        {
            break;
        }
        else
        {
            post ("pdp_v4l2 : format %d : %s",  x->x_nformats, x->x_formats[x->x_nformats].description );
        }
    }
    if (x->x_debug) post("pdp_v4l2: device supports %d formats", x->x_nformats );

    if ( x->x_nformats > 0 )
    {
       x->x_v4l2_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
       if ((i = ioctl(x->x_tvfd, VIDIOC_G_FMT, &x->x_v4l2_format)) < 0) {
          perror("pdp_v4l2: error: VIDIOC_G_FMT");
          pdp_v4l2_close_error(x);
          x->x_initialized = false;
       }
       x->x_width = x->x_v4l2_format.fmt.pix.width;
       x->x_height = x->x_v4l2_format.fmt.pix.height;

       post( "pdp_v4l2 : current format is : %c%c%c%c", 
            x->x_v4l2_format.fmt.pix.pixelformat & 0xff,
            (x->x_v4l2_format.fmt.pix.pixelformat >>  8) & 0xff,
            (x->x_v4l2_format.fmt.pix.pixelformat >> 16) & 0xff,
            (x->x_v4l2_format.fmt.pix.pixelformat >> 24) & 0xff );

       pdp_v4l2_set_format(x, x->x_curformat);
    }
    else
    {
      post( "pdp_v4l2 : error : no available formats : closing..." );
      pdp_v4l2_close_error(x);
      x->x_initialized = false;
      return;
    }

        /* controls */
    for (i = 0; i < MAX_CTRL; i++) {
        x->x_controls[i].id = V4L2_CID_BASE+i;
        if (-1 == ioctl(x->x_tvfd, VIDIOC_QUERYCTRL, &x->x_controls[i]) ||
            (x->x_controls[i].flags & V4L2_CTRL_FLAG_DISABLED))
            x->x_controls[i].id = -1;
        else if (x->x_debug) post( "control %d active (i:%d)", x->x_controls[i].id, i );
    }
    for (i = 0; i < MAX_CTRL; i++) {
        x->x_controls[i+MAX_CTRL].id = V4L2_CID_PRIVATE_BASE+i;
        if (-1 == ioctl(x->x_tvfd, VIDIOC_QUERYCTRL, &x->x_controls[i+MAX_CTRL]) ||
            (x->x_controls[i+MAX_CTRL].flags & V4L2_CTRL_FLAG_DISABLED))
            x->x_controls[i+MAX_CTRL].id = -1;
        else if (x->x_debug) post( "control %d active (i:%d)", x->x_controls[i+MAX_CTRL].id, i );
    }

    if ( pdp_v4l2_init_mmap(x) < 0 )
    {
      post( "pdp_v4l2 : error : couldn't initialize memory mapping : closing..." );
      pdp_v4l2_close_error(x);
      x->x_initialized = false;
      return;
    }

    x->x_initialized=true;
    post( "pdp_v4l2 : device initialized" );

    // create thread 
    x->x_continue_thread = 1;
    x->x_frame_ready = 0;
    pthread_create(&x->x_thread_id, 0, pdp_v4l2_thread, x);
    post( "pdp_v4l2 : created thread : %u", x->x_thread_id );

}

static void pdp_v4l2_open_manual(t_pdp_v4l2 *x, t_symbol *name)
{
    x->x_open_retry = PDP_XV_RETRIES;
    pdp_v4l2_open(x, name);
}


static void pdp_v4l2_input(t_pdp_v4l2 *x, t_float f)
{
    if (!x->x_initialized){
       post( "pdp_v4l2 : cannot set input : no device opened ");
       return;
    }
    if ( ( (int)f < 0 ) || ( (int)f >= x->x_ninputs ) )
    {
       post( "pdp_v4l2 : input number %d out of range", (int)f );
       return;
    }
    if (x->x_initialized){
        pdp_v4l2_close(x);
        x->x_curinput = (int)f;
        pdp_v4l2_open(x, x->x_device);
    }
}

static void pdp_v4l2_standard(t_pdp_v4l2 *x, t_float f)
{
    if (!x->x_initialized){
       post( "pdp_v4l2 : cannot set standard : no device opened ");
       return;
    }
    if ( ( (int)f < 0 ) || ( (int)f >= x->x_nstandards ) )
    {
       post( "pdp_v4l2 : standard number %d out of range", (int)f );
       return;
    }
    if (x->x_initialized){
        pdp_v4l2_close(x);
        x->x_curstandard = (int)f;
        pdp_v4l2_open(x, x->x_device);
    }
}

static void pdp_v4l2_format(t_pdp_v4l2 *x, t_float f)
{
    if ( ( (int)f < 0 ) || ( (int)f >= x->x_nformats ) )
    {
       post( "pdp_v4l2 : format number %d out of range", (int)f );
       return;
    }
    x->x_curformat = (int)f;
    if (x->x_initialized){
        pdp_v4l2_close(x);
        pdp_v4l2_set_format(x, x->x_curformat);
        pdp_v4l2_open(x, x->x_device);
    }
}

static void pdp_v4l2_freq(t_pdp_v4l2 *x, t_float f)
{
    if (!x->x_initialized){
       post( "pdp_v4l2 : cannot set format : no device opened ");
       return;
    }
    x->x_freq = (int)f;
    if (x->x_freq > 0)
    {
       if (ioctl(x->x_tvfd, VIDIOC_S_FREQUENCY, &x->x_freq) < 0)
       {
         perror ("couldn't set frequency :");
       }
       else 
       {
         post("pdp_v4l2: tuner frequency set to : %f MHz", f / 16.0f);
       }
    }
}

static void pdp_v4l2_freqMHz(t_pdp_v4l2 *x, t_float f)
{
   pdp_v4l2_freq(x, f*16.0f); 
}


static void pdp_v4l2_bang(t_pdp_v4l2 *x)
{
   
  /* if initialized, grab a frame and output it */

  unsigned int w,h,nbpixels,packet_size,plane1,plane2;
  unsigned char *newimage=NULL;
  int pdp_packt,length,pos,i,encoding;
  t_pdp* header;
  t_image* image;
  short int * data;

  static short int gain[4] = {0x7fff, 0x7fff, 0x7fff, 0x7fff};

  if (!(x->x_initialized)){
	post("pdp_v4l2: no device opened");

	if (x->x_auto_open){
	  post("pdp_v4l2: attempting auto open");
	  pdp_v4l2_open(x, x->x_device);
	  if (!(x->x_initialized)){
	    post("pdp_v4l2: auto open failed");
	    return;
	  }
	}
	else return;
    }

    /* do nothing if there is no frame ready */
    if((!x->x_frame_ready) && (x->x_only_new_frames)) return;
    x->x_frame_ready = 0;

    newimage = x->x_pdp_buf[x->x_last_frame];

    /* create new packet */

    pdp_packt = pdp_packet_new_image(PDP_IMAGE_YV12, x->x_width, x->x_height);
    header = pdp_packet_header(pdp_packt);
    image = pdp_packet_image_info(pdp_packt);

    if (!header){
	post("pdp_v4l2: ERROR: can't allocate packet");
	return;
    }

    data = (short int *) pdp_packet_data(pdp_packt);

    /* convert data to pdp packet */

    switch(x->x_v4l2_format.fmt.pix.pixelformat){
    case  V4L2_PIX_FMT_YUV420:
	pdp_llconv(newimage, RIF_YUV__P411_U8, data, RIF_YVU__P411_S16, x->x_width, x->x_height); 
	break;
	
	/* long live standards. v4l's rgb is in fact ogl's bgr */
    case  V4L2_PIX_FMT_RGB24:
	pdp_llconv(newimage, RIF_BGR__P____U8, data, RIF_YVU__P411_S16, x->x_width, x->x_height); 
	break;

    case  V4L2_PIX_FMT_RGB32:
	pdp_llconv(newimage, RIF_BGRA_P____U8, data, RIF_YVU__P411_S16, x->x_width, x->x_height); 
	break;

    case  V4L2_PIX_FMT_YUYV:
	pdp_llconv(newimage, RIF_YUYV_P____U8, data, RIF_YVU__P411_S16, x->x_width, x->x_height); 
	break;

    case  V4L2_PIX_FMT_UYVY: 
        pdp_llconv(newimage, RIF_UYVY_P____U8, data, RIF_YVU__P411_S16, x->x_width, x->x_height);
        break;

    case  v4l2_fourcc('S', '9', '2', '0'):
	v4lconvert_sn9c20x_to_yuv420( newimage, (unsigned char*)data, x->x_width, x->x_height,1);
	pdp_llconv(data, RIF_YUV__P411_U8, newimage, RIF_YVU__P411_S16, x->x_width, x->x_height);
	memcpy(data, newimage, x->x_width * x->x_height *2); 
        break;

    default:
	post("pdp_v4l2: unsupported color model: %d", x->x_v4l2_format.fmt.pix.pixelformat);
	break;
    }

    pdp_packet_pass_if_valid(x->x_outlet0, &pdp_packt);

}


static void pdp_v4l2_setlegaldim(t_pdp_v4l2 *x, int xx, int yy)
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

static void pdp_v4l2_dim(t_pdp_v4l2 *x, t_floatarg xx, t_floatarg yy)
{
    if (!x->x_initialized){
       post( "pdp_v4l2 : cannot set dim : no device opened ");
       return;
    }
    if (x->x_initialized){
        pdp_v4l2_close(x);
        pdp_v4l2_setlegaldim(x, (int)xx, (int)yy);
        pdp_v4l2_open(x, x->x_device);
    }
}

static void pdp_v4l2_free(t_pdp_v4l2 *x)
{
    pdp_v4l2_close(x);
}

t_class *pdp_v4l2_class;

void *pdp_v4l2_new(t_symbol *vdef)
{
    t_pdp_v4l2 *x = (t_pdp_v4l2 *)pd_new(pdp_v4l2_class);

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything);

    x->x_initialized = false;

    x->x_tvfd = -1;
    x->x_ninputs = 0;
    x->x_curinput = 0;
    x->x_curstandard = 0;
    x->x_curformat = 0;
    x->x_freq = -1;
    x->x_nstandards = 0;
    x->x_nformats = 0;
    x->x_frame = 0;
    x->x_last_frame = 0;

    x->x_auto_open = true;
    if (vdef != gensym("")){
	x->x_device = vdef;
    }
    else{
	x->x_device = gensym("/dev/video0");
    }

    x->x_continue_thread = 0;
    x->x_only_new_frames = 1;

    x->x_width = 320;
    x->x_height = 240;

    x->x_open_retry = PDP_XV_RETRIES;

    x->x_minwidth = pdp_imageproc_legalwidth(0);
    x->x_maxwidth = pdp_imageproc_legalwidth_round_down(0x7fffffff);
    x->x_minheight = pdp_imageproc_legalheight(0);
    x->x_maxheight = pdp_imageproc_legalheight_round_down(0x7fffffff);

    x->x_debug = 1;

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_v4l2_setup(void)
{
    pdp_v4l2_class = class_new(gensym("pdp_v4l2"), (t_newmethod)pdp_v4l2_new,
    	(t_method)pdp_v4l2_free, sizeof(t_pdp_v4l2), 0, A_DEFSYMBOL, A_DEFSYMBOL, A_NULL);

    class_addmethod(pdp_v4l2_class, (t_method)pdp_v4l2_close_manual, gensym("close"), A_NULL);
    class_addmethod(pdp_v4l2_class, (t_method)pdp_v4l2_open_manual, gensym("open"), A_SYMBOL, A_NULL);
    class_addmethod(pdp_v4l2_class, (t_method)pdp_v4l2_input, gensym("input"), A_FLOAT, A_NULL);
    class_addmethod(pdp_v4l2_class, (t_method)pdp_v4l2_format, gensym("format"), A_FLOAT, A_NULL);
    class_addmethod(pdp_v4l2_class, (t_method)pdp_v4l2_standard, gensym("standard"), A_FLOAT, A_NULL);
    class_addmethod(pdp_v4l2_class, (t_method)pdp_v4l2_dim, gensym("dim"), A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(pdp_v4l2_class, (t_method)pdp_v4l2_freq, gensym("freq"), A_FLOAT, A_NULL);
    class_addmethod(pdp_v4l2_class, (t_method)pdp_v4l2_freqMHz, gensym("freqMHz"), A_FLOAT, A_NULL);
    class_addmethod(pdp_v4l2_class, (t_method)pdp_v4l2_bang, gensym("bang"), A_NULL);

}

#ifdef __cplusplus
}
#endif
