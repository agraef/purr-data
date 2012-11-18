/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2000, 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *              Bill May        wmay@cisco.com
 *              video aspect ratio by:
 *              Peter Maersk-Moller peter@maersk-moller.net
 *
 * Adapted to PD/PDP by Yves Degoyon (ydegoyon@free.fr)
 */

/*
 * video.cpp - provides codec to video hardware class
 */

#include <string.h>
#include "pdp_mp4videosync.h"
#include "pdp_mp4playersession.h"
#include "player_util.h"
#include "m_pd.h"

#define video_message(loglevel, fmt...) message(loglevel, "videosync", fmt)

CPDPVideoSync::CPDPVideoSync (CPlayerSession *psptr, t_pdp_mp4player *pdp_father) : CVideoSync(psptr)
{
  char buf[32];

  m_screen = NULL;
  m_image = NULL;
  m_video_initialized = 0;
  m_config_set = 0;
  m_have_data = 0;
  m_y_buffer[0] = NULL;
  m_u_buffer[0] = NULL;
  m_v_buffer[0] = NULL;
  m_buffer_filled[0] = 0;
  m_play_index = m_fill_index = 0;
  m_decode_waiting = 0;
  m_dont_fill = 0;
  m_paused = 1;
  m_behind_frames = 0;
  m_total_frames = 0;
  m_behind_time = 0;
  m_behind_time_max = 0;
  m_skipped_render = 0;
  m_video_scale = 2;
  m_msec_per_frame = 0;
  m_consec_skipped = 0;
  m_fullscreen = 0;
  m_filled_frames = 0;
  m_double_width = 0;
  m_pixel_width = 0;
  m_pixel_height = 0;
  m_max_width = 0;
  m_max_height = 0;
  m_father = pdp_father;
}

CPDPVideoSync::~CPDPVideoSync (void)
{
  if (m_fullscreen != 0) {
    m_fullscreen = 0;
    do_video_resize();
  }
  if (m_image) {
    m_image = NULL;
  }
  if (m_screen) {
    m_screen = NULL;
  }
  if (m_y_buffer[0] != NULL) {
    free(m_y_buffer[0]);
    m_y_buffer[0] = NULL;
  }
  if (m_u_buffer[0] != NULL) {
    free(m_u_buffer[0]);
    m_u_buffer[0] = NULL;
  }
  if (m_v_buffer[0] != NULL) {
    free(m_v_buffer[0]);
    m_v_buffer[0] = NULL;
  }
}

void CPDPVideoSync::config (int w, int h)
{
  m_width = w;
  m_height = h;
  m_y_buffer[0] = (uint8_t *)malloc(w * h * sizeof(uint8_t));
  m_u_buffer[0] = (uint8_t *)malloc(w/2 * h/2 * sizeof(uint8_t));
  m_v_buffer[0] = (uint8_t *)malloc(w/2 * h/2 * sizeof(uint8_t));
  m_buffer_filled[0] = 0;
  m_config_set = 1;
  post( "pdp_mp4videosync : configuration done : %dx%d", m_width, m_height );
}

int CPDPVideoSync::initialize_video (const char *name)
{
  if (m_video_initialized == 0) {
    if (m_config_set) {
      int ret;
      int video_scale = m_video_scale;

      int w = m_width * video_scale / 2;
      if (m_double_width) w *= 2;
      int h = m_height * video_scale / 2;
      m_video_initialized = 1;
      post( "pdp_mp4videosync : video initialized : %dx%d", m_width, m_height );
      return (1);
    } else {
     return (0);
    }
  }
  return (1);
}

int CPDPVideoSync::is_video_ready (uint64_t &disptime)
{
  return 1;
}

void CPDPVideoSync::play_video (void) 
{

}

int64_t CPDPVideoSync::play_video_at (uint64_t current_time, int &have_eof)
{
  uint64_t play_this_at;
  unsigned int ix;
  uint8_t *to, *from;

  post( "pdp_mp4videosync : play video at  : %ld", current_time );
  
  return (10);
}

int CPDPVideoSync::get_video_buffer(uint8_t **y,
				 uint8_t **u,
				 uint8_t **v)
{
  
  post( "pdp_mp4videosync : get video buffer" );
  *y = m_y_buffer[m_fill_index];
  *u = m_u_buffer[m_fill_index];
  *v = m_v_buffer[m_fill_index];
  return (1);
}

void CPDPVideoSync::filled_video_buffers (uint64_t time)
{
  int ix;
  // post( "pdp_mp4videosync : filled video buffer : %ld", time );
  m_psptr->wake_sync_thread();
}

void CPDPVideoSync::set_video_frame(const uint8_t *y, 
				    const uint8_t *u, 
				    const uint8_t *v,
				    int pixelw_y, 
				    int pixelw_uv, 
				    uint64_t time)
{
 short int *pY, *pU, *pV;
 t_int px, py;

  m_psptr->wake_sync_thread();

  if ( !m_father->x_streaming )
  {
    return;
  }

  // transforming into a image for pdp 
  if ( ( (t_int)m_father->x_vheight != (t_int)m_height ) ||
       ( (t_int)m_father->x_vwidth != (t_int)m_width ) )
  {
    m_father->x_vheight = m_height;
    m_father->x_vwidth = m_width;
    m_father->x_vsize = m_father->x_vheight*m_father->x_vwidth;
    post( "pdp_mp4videosync : allocating video data : %dx%d", m_width, m_height ); 

    // allocate video data
    m_father->x_datav = ( short int* ) malloc( (m_father->x_vsize+(m_father->x_vsize>>1))<<1 );
  }

  // post( "pdp_mp4videosync : set video frame : width : y:%d, uv:%d", pixelw_y, pixelw_uv );

  pY = m_father->x_datav;
  pV = m_father->x_datav+m_father->x_vsize;
  pU = m_father->x_datav+m_father->x_vsize+(m_father->x_vsize>>2);
  for(py=0; py<m_father->x_vheight; py++) 
  {
    for(px=0; px<m_father->x_vwidth; px++)
    {
       *(pY+py*m_father->x_vwidth+px) = *(y+py*pixelw_y+px)<<7;
       if ( ( px%2 == 0 ) && ( py%2 == 0 ) )
       {
         *(pU+(py>>1)*(m_father->x_vwidth>>1)+(px>>1)) = ((*(u+(py>>1)*pixelw_uv+(px>>1)))-128)<<8;
         *(pV+(py>>1)*(m_father->x_vwidth>>1)+(px>>1)) = ((*(v+(py>>1)*pixelw_uv+(px>>1)))-128)<<8;
       }
    }
  }

  // pass the data to the pdp object
  m_father->x_newpicture = 1;
  return;
}

void CPDPVideoSync::flush_sync_buffers (void)
{
  post( "pdp_mp4videosync : flush sync buffer" );
}

void CPDPVideoSync::flush_decode_buffers (void)
{
  post( "pdp_mp4videosync : flush decode buffer" );
}

static void pdp_video_configure (void *ifptr,
			      int w,
			      int h,
			      int format,
                              double aspect_ratio )
{
  ((CPDPVideoSync *)ifptr)->config(w, h);
}

static int pdp_video_get_buffer (void *ifptr, 
			       uint8_t **y,
			       uint8_t **u,
			       uint8_t **v)
{
  return (((CPDPVideoSync *)ifptr)->get_video_buffer(y, u, v));
}

static void pdp_video_filled_buffer(void *ifptr, uint64_t time)
{
  ((CPDPVideoSync *)ifptr)->filled_video_buffers(time);
}

static void pdp_video_have_frame (void *ifptr,
			       const uint8_t *y,
			       const uint8_t *u,
			       const uint8_t *v,
			       int m_pixelw_y,
			       int m_pixelw_uv,
			       uint64_t time)
{
  CPDPVideoSync *foo = (CPDPVideoSync *)ifptr;

  foo->set_video_frame(y, u, v, m_pixelw_y, m_pixelw_uv, time);
}

video_vft_t video_vft = 
{
  message,
  pdp_video_configure,
  pdp_video_get_buffer,
  pdp_video_filled_buffer,
  pdp_video_have_frame,
};

video_vft_t *get_video_vft (void)
{
  return (&video_vft);
}

CPDPVideoSync *pdp_create_video_sync (CPlayerSession *psptr, t_pdp_mp4player *pdp_father) 
{
  return new CPDPVideoSync(psptr, pdp_father);
}
