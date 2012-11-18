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
 *              Peter Maersk-Moller peter @maersk-moller.net
 *              
 * Adapted for PD/PDP by Yves Degoyon (ydegoyon@free.fr)
 *
 */

/*
 * pdp_mp4playersession.h - provides definitions for a CPlayerSession.
 * CPlayerSession is the base class that provides a combination audio/video
 * stream/file playback.
 * This class should be the main interface between any outside functionality
 * and the player window.
 */


#ifndef __PDP_MP4PLAYERSESSION_H__
#define __PDP_MP4PLAYERSESSION_H__

struct rtcp_sync_t;
typedef struct rtcp_sync_t rtcp_sync_t;

#include <rtsp/rtsp_client.h>
#include <sdp/sdp.h>
#include "our_msg_queue.h"
#include "ip_port.h"
#include "pdp_mp4player~.h"

class CPlayerMedia;
class CPDPAudioSync;
class CPDPVideoSync;

typedef enum {
  SESSION_PAUSED,
  SESSION_BUFFERING,
  SESSION_PLAYING,
  SESSION_DONE
} session_state_t;

typedef struct rtcp_sync_t {
  uint64_t first_pak_ts;
  uint64_t rtcp_ts;
  uint32_t first_pak_rtp_ts;
  uint32_t rtcp_rtp_ts;
  uint64_t timescale;
} rtcp_sync_t;

typedef void (*media_close_callback_f)(void *);

class CPlayerSession {
 public:
  /*
   * API routine - create player session.
   */
  CPlayerSession(CMsgQueue *master_queue,
		 SDL_sem *master_sem,
		 const char *name,
                 t_pdp_mp4player *pdp_father);
  /*
   * API routine - destroy session - free all sub-structures, cleans
   * up rtsp, etc
   */
  ~CPlayerSession();
  /*
   * API routine - create a rtsp session with the url.  After that, you
   * need to associate media
   */
  int create_streaming_broadcast(session_desc_t *sdp,
				 char *ermsg,
				 uint32_t errlen);
  int create_streaming_ondemand(const char *url,
				char *errmsg,
				uint32_t errlen,
				int use_rtp_tcp);
  /*
   * API routine - play at time.  If start_from_begin is FALSE, start_time
   * and we're paused, it will continue from where it left off.
   */
  int play_all_media(int start_from_begin = FALSE, double start_time = 0.0,
		     char *errmsg = NULL, uint32_t errlen = 0);
  /*
   * API routine - pause
   */
  int pause_all_media(void);
  /*
   * API routine for media set up - associate a created
   * media with the session.
   */
  void add_media(CPlayerMedia *m);
  /*
   * API routine - returns sdp info for streamed session
   */
  session_desc_t *get_sdp_info (void) { return m_sdp_info;} ;
  rtsp_client_t *get_rtsp_client (void) { return m_rtsp_client; };
  /*
   * API routine - after setting up media, need to set up sync thread
   */
  void set_up_sync_thread(void);
  CPDPVideoSync *set_up_video_sync(void);
  CPDPAudioSync *set_up_audio_sync(void);
  /*
   * API routine - get the current time
   */
  uint64_t get_playing_time (void) {
    if (m_streaming && session_is_seekable() == 0) {
      return (m_current_time - m_first_time_played);
    }
    return (m_current_time);
  };
  /*
   * API routine - get max play time
   */
  double get_max_time (void);
  /*
   * Other API routines
   */
  int session_has_audio(void);
  int session_has_video(void);
  void set_audio_volume(int volume);
  int get_audio_volume(void) { return m_audio_volume; };
  void session_set_seekable (int seekable) {
    m_seekable = seekable;
  };
  int session_is_seekable (void) {
    return (m_seekable);
  };
  session_state_t get_session_state(void) {
    return (m_session_state);
  }
  void set_media_close_callback (media_close_callback_f mccf,
				 void *mccd) {
    m_media_close_callback = mccf;
    m_media_close_callback_data = mccd;
  }
  int session_is_network (int &on_demand, int &rtp_over_rtsp) {
    if (m_streaming == 0) {
      return 0;
    }
    if (m_seekable) { 
      on_demand = 1;
      rtp_over_rtsp = m_rtp_over_rtsp;
    } else {
      on_demand = 0;
      rtp_over_rtsp = 0;
    }
	return 1;
  }
  /*
   * Non-API routines - used for c interfaces, for sync task APIs.
   */
  void wake_sync_thread (void) {
    SDL_SemPost(m_sync_sem);
  }
  int send_sync_thread_a_message(uint32_t msgval,
				 unsigned char *msg = NULL,
				 uint32_t msg_len = 0)
    {
      return (m_sync_thread_msg_queue.send_message(msgval, msg, msg_len, m_sync_sem));
    };
  int sync_thread(int state);
  uint64_t get_current_time(void);
  void audio_is_ready (uint64_t latency, uint64_t time);
  void adjust_start_time(int64_t delta);
  int session_control_is_aggregate (void) {
    return m_session_control_is_aggregate;
  };
  void set_session_control (int is_aggregate) {
    m_session_control_is_aggregate = is_aggregate;
  }
  CPlayerMedia *rtsp_url_to_media (const char *url);
  int set_session_desc(int line, const char *desc);
  const char *get_session_desc(int line);
  void streaming_media_set_up(void) { m_streaming_media_set_up = 1; };
  CIpPort **get_unused_ip_port_ptr(void) { return &m_unused_ports; };
  void syncronize_rtp_bytestreams(rtcp_sync_t *sync);
 private:
  int process_msg_queue(int state);
  int sync_thread_init(void);
  int sync_thread_wait_sync(void);
  int sync_thread_wait_audio(void);
  int sync_thread_playing(void);
  int sync_thread_paused(void);
  int sync_thread_done(void);
  const char *m_session_name;
  const char *m_content_base;
  int m_paused;
  int m_streaming;
  uint64_t m_current_time; // current time playing
  uint64_t m_start;
  uint64_t m_latency;
  int m_clock_wrapped;
  uint64_t m_play_start_time;
  session_desc_t *m_sdp_info;
  rtsp_client_t *m_rtsp_client;
  CPlayerMedia *m_my_media;
  CPDPAudioSync *m_audio_sync;
  CPDPVideoSync *m_video_sync;
  SDL_Thread *m_sync_thread;
  SDL_sem *m_sync_sem;
  CMsgQueue *m_master_msg_queue;
  SDL_sem *m_master_msg_queue_sem;
  CMsgQueue m_sync_thread_msg_queue;
  range_desc_t *m_range;
  int m_session_control_is_aggregate;
  int m_waiting_for_audio;
  int m_audio_volume;
  int m_screen_scale;
  int m_fullscreen;
  int m_pixel_height;
  int m_pixel_width;
  int m_seekable;
  volatile int m_sync_pause_done;
  session_state_t m_session_state;
  int m_hardware_error;
  #define SESSION_DESC_COUNT 4
  const char *m_session_desc[SESSION_DESC_COUNT];
  media_close_callback_f m_media_close_callback;
  void *m_media_close_callback_data;
  int m_streaming_media_set_up;
  CIpPort *m_unused_ports;
  int m_rtp_over_rtsp;
  uint64_t m_first_time_played;
  bool m_have_audio_rtcp_sync;
  rtcp_sync_t m_audio_rtcp_sync;
  t_pdp_mp4player *m_father;
};

int pdp_sync_thread(void *data);

#endif
