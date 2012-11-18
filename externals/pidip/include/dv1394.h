/*
 * dv1394.h - DV input/output over IEEE 1394 on OHCI chips
 *   Copyright (C)2001 Daniel Maas <dmaas@dcine.com>
 *     receive, proc_fs by Dan Dennedy <dan@dennedy.org>
 *
 * based on:
 *   video1394.h - driver for OHCI 1394 boards
 *   Copyright (C)1999,2000 Sebastien Rougeaux <sebastien.rougeaux@anu.edu.au>
 *                          Peter Schlaile <udbz@rz.uni-karlsruhe.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _DV_1394_H
#define _DV_1394_H

#include <sys/types.h>
#include <sys/ioctl.h>

/* This is the public user-space interface. Try not to break it. */

#define DV1394_API_VERSION 0x20011127

/* ********************
   **                **
   **   DV1394 API   **
   **                **
   ********************

   There are two methods of operating the DV1394 DV output device.

   1)

   The simplest is an interface based on write(): simply write
   full DV frames of data to the device, and they will be transmitted
   as quickly as possible. The FD may be set for non-blocking I/O,
   in which case you can use select() or poll() to wait for output
   buffer space.

   To set the DV output parameters (e.g. whether you want NTSC or PAL
   video), use the DV1394_INIT ioctl, passing in the parameters you
   want in a struct dv1394_init.
 
   Example 1:
         To play a raw .DV file:   cat foo.DV > /dev/dv1394
	 (cat will use write() internally)

   Example 2:
           static struct dv1394_init init = {
	      0x63,        (broadcast channel)
              4,           (four-frame ringbuffer)
	      DV1394_NTSC, (send NTSC video)
	      0, 0         (default empty packet rate)
           }

	   ioctl(fd, DV1394_INIT, &init);

	   while(1) {
	          read( <a raw DV file>, buf, DV1394_NTSC_FRAME_SIZE );
		  write( <the dv1394 FD>, buf, DV1394_NTSC_FRAME_SIZE );
           }

   2)

   For more control over buffering, and to avoid unnecessary copies
   of the DV data, you can use the more sophisticated the mmap() interface. 
   First, call the DV1394_INIT ioctl to specify your parameters, 
   including the number of frames in the ringbuffer. Then, calling mmap() 
   on the dv1394 device will give you direct access to the ringbuffer
   from which the DV card reads your frame data.

   The ringbuffer is simply one large, contiguous region of memory
   containing two or more frames of packed DV data. Each frame of DV data
   is 120000 bytes (NTSC) or 144000 bytes (PAL).

   Fill one or more frames in the ringbuffer, then use the DV1394_SUBMIT_FRAMES
   ioctl to begin I/O. You can use either the DV1394_WAIT_FRAMES ioctl
   or select()/poll() to wait until the frames are transmitted. Next, you'll
   need to call the DV1394_GET_STATUS ioctl to determine which ringbuffer
   frames are clear (ready to be filled with new DV data). Finally, use
   DV1394_SUBMIT_FRAMES again to send the new data to the DV output.


   Example: here is what a four-frame ringbuffer might look like
            during DV transmission:


         frame 0   frame 1   frame 2   frame 3

        *--------------------------------------*
        | CLEAR   | DV data | DV data | CLEAR  |
        *--------------------------------------*
                   <ACTIVE> 

	transmission goes in this direction --->>>


   The DV hardware is currently transmitting the data in frame 1.
   Once frame 1 is finished, it will automatically transmit frame 2.
   (if frame 2 finishes before frame 3 is submitted, the device
   will continue to transmit frame 2, and will increase the dropped_frames
   counter each time it repeats the transmission).

 
   If you called DV1394_GET_STATUS at this instant, you would
   receive the following values:
   
          n_frames          = 4
          active_frame      = 1
          first_clear_frame = 3
          n_clear_frames    = 2

   At this point, you should write new DV data into frame 3 and optionally
   frame 0. Then call DV1394_SUBMIT_FRAMES to inform the device that
   it may transmit the new frames.

*/


/* maximum number of frames in the ringbuffer */
#define DV1394_MAX_FRAMES 32

/* number of *full* isochronous packets per DV frame */
#define DV1394_NTSC_PACKETS_PER_FRAME 250
#define DV1394_PAL_PACKETS_PER_FRAME  300

/* size of one frame's worth of DV data, in bytes */
#define DV1394_NTSC_FRAME_SIZE (480 * DV1394_NTSC_PACKETS_PER_FRAME)
#define DV1394_PAL_FRAME_SIZE  (480 * DV1394_PAL_PACKETS_PER_FRAME)


enum pal_or_ntsc {
	DV1394_NTSC = 0,
	DV1394_PAL
};


/* this is the argument to DV1394_INIT */
struct dv1394_init {
	/* DV1394_API_VERSION */
	unsigned int api_version;
	
	/* isochronous transmission channel to use */
	unsigned int channel;

	/* number of frames in the ringbuffer. Must be at least 2
	   and at most DV1394_MAX_FRAMES. */
	unsigned int n_frames;

	/* send/receive PAL or NTSC video format */
	enum pal_or_ntsc format;

	/* the following are used only for transmission */
 
	/* set these to zero unless you want a
	   non-default empty packet rate (see below) */
	unsigned long cip_n;
	unsigned long cip_d;

	/* set this to zero unless you want a
	   non-default SYT cycle offset (default = 3 cycles) */
	unsigned int syt_offset;
};

/* Q: What are cip_n and cip_d? */

/*
  A: DV video streams do not utilize 100% of the potential bandwidth offered
  by IEEE 1394 (FireWire). To achieve the correct rate of data transmission,
  DV devices must periodically insert empty packets into the 1394 data stream.
  Typically there is one empty packet per 14-16 data-carrying packets.

  Some DV devices will accept a wide range of empty packet rates, while others
  require a precise rate. If the dv1394 driver produces empty packets at
  a rate that your device does not accept, you may see ugly patterns on the
  DV output, or even no output at all.

  The default empty packet insertion rate seems to work for many people; if
  your DV output is stable, you can simply ignore this discussion. However,
  we have exposed the empty packet rate as a parameter to support devices that
  do not work with the default rate. 

  The decision to insert an empty packet is made with a numerator/denominator
  algorithm. Empty packets are produced at an average rate of CIP_N / CIP_D.
  You can alter the empty packet rate by passing non-zero values for cip_n
  and cip_d to the INIT ioctl.
  
 */

struct dv1394_status {
	/* this embedded init struct returns the current dv1394
	   parameters in use */
	struct dv1394_init init;

	/* the ringbuffer frame that is currently being
	   displayed. (-1 if the device is not transmitting anything) */
	int active_frame;

	/* index of the first buffer (ahead of active_frame) that
	   is ready to be filled with data */
	unsigned int first_clear_frame;

	/* how many buffers, including first_clear_buffer, are
	   ready to be filled with data */
	unsigned int n_clear_frames;

	/* how many times the DV output has underflowed
	   since the last call to DV1394_GET_STATUS */
	unsigned int dropped_frames;

	/* N.B. The dropped_frames counter is only a lower bound on the actual
	   number of dropped frames, with the special case that if dropped_frames
	   is zero, then it is guaranteed that NO frames have been dropped
	   since the last call to DV1394_GET_STATUS.
	*/
};

/* Get the driver ready to transmit video.  pass a struct dv1394_init* as
 * the parameter (see below), or NULL to get default parameters */
#define DV1394_INIT			_IOW('#', 0x06, struct dv1394_init)

/* Stop transmitting video and free the ringbuffer */
#define DV1394_SHUTDOWN		_IO ('#', 0x07)

/* Submit N new frames to be transmitted, where the index of the first new
 * frame is first_clear_buffer, and the index of the last new frame is
 * (first_clear_buffer + N) % n_frames */
#define DV1394_SUBMIT_FRAMES	_IO ('#', 0x08)

/* Block until N buffers are clear (pass N as the parameter) Because we
 * re-transmit the last frame on underrun, there will at most be n_frames
 * - 1 clear frames at any time */
#define DV1394_WAIT_FRAMES		_IO ('#', 0x09)

/* Capture new frames that have been received, where the index of the
 * first new frame is first_clear_buffer, and the index of the last new
 * frame is (first_clear_buffer + N) % n_frames */
#define DV1394_RECEIVE_FRAMES	_IO ('#', 0x0a)

/* Tell card to start receiving DMA */
#define DV1394_START_RECEIVE	_IO ('#', 0x0b)

/* Pass a struct dv1394_status* as the parameter */
#define DV1394_GET_STATUS		_IOR('#', 0x0c, struct dv1394_status)

#endif /* _DV_1394_H */
