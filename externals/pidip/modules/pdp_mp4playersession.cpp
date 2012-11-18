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
 */

/* 
 * pdp_mp4playersession.cpp - describes player session class, which is the
 * main access point for the player
 */

#include "mpeg4ip.h"
#include "pdp_mp4playersession.h"
#include "pdp_mp4playermedia.h"
#include "pdp_mp4audiosync.h"
#include "pdp_mp4videosync.h"
#include "pdp_mp4player~.h"
#include "player_sdp.h"
#include "player_util.h"
#include <stdlib.h>
#include <SDL.h>
#include <SDL_thread.h>
#include "player_util.h"
#include "m_pd.h"

#define sync_message(loglevel, fmt...) message(loglevel, "avsync", fmt)

enum {
  SYNC_STATE_INIT = 0,
  SYNC_STATE_WAIT_SYNC = 1,
  SYNC_STATE_WAIT_AUDIO = 2,
  SYNC_STATE_PLAYING = 3,
  SYNC_STATE_PAUSED = 4,
  SYNC_STATE_DONE = 5,
  SYNC_STATE_EXIT = 6,
};

#ifdef DEBUG_SYNC_STATE
const char *sync_state[] = {
  "Init",
  "Wait Sync",
  "Wait Audio",
  "Playing",
  "Paused",
  "Done",
  "Exit"
};
#endif

CPlayerSession::CPlayerSession (CMsgQueue *master_mq, 
				SDL_sem *master_sem,
				const char *name,
                                t_pdp_mp4player *pdp_father)
{
  m_sdp_info = NULL;
  m_my_media = NULL;
  m_rtsp_client = NULL;
  m_video_sync = NULL;
  m_audio_sync = NULL;
  m_sync_thread = NULL;
  m_sync_sem = NULL;
  m_content_base = NULL;
  m_master_msg_queue = master_mq;
  m_master_msg_queue_sem = master_sem;
  m_paused = 0;
  m_streaming = 0;
  m_session_name = strdup(name);
  m_audio_volume = 75;
  m_current_time = 0;
  m_seekable = 0;
  m_session_state = SESSION_PAUSED;
  m_clock_wrapped = -1;
  m_hardware_error = 0;
  m_pixel_height = -1;
  m_pixel_width = -1;
  m_session_control_is_aggregate = 0;
  for (int ix = 0; ix < SESSION_DESC_COUNT; ix++) {
    m_session_desc[ix] = NULL;
  }
  m_media_close_callback = NULL;
  m_media_close_callback_data = NULL;
  m_streaming_media_set_up = 0;
  m_unused_ports = NULL;
  m_first_time_played = 0;
  m_latency = 0;
  m_have_audio_rtcp_sync = false;
  m_father = pdp_father;
}

CPlayerSession::~CPlayerSession ()
{
  int hadthread = 0;
#ifndef NEED_SDL_VIDEO_IN_MAIN_THREAD
  if (m_sync_thread) {
    send_sync_thread_a_message(MSG_STOP_THREAD);
    SDL_WaitThread(m_sync_thread, NULL);
    m_sync_thread = NULL;
    hadthread = 1;
  }
#else
  send_sync_thread_a_message(MSG_STOP_THREAD);
  hadthread = 1;
#endif



  if (m_streaming_media_set_up != 0 &&
      session_control_is_aggregate()) {
    rtsp_command_t cmd;
    rtsp_decode_t *decode;
    memset(&cmd, 0, sizeof(rtsp_command_t));
    rtsp_send_aggregate_teardown(m_rtsp_client,
				 m_sdp_info->control_string,
				 &cmd,
				 &decode);
    free_decode_response(decode);
  }


  if (m_rtsp_client) {
    free_rtsp_client(m_rtsp_client);
    m_rtsp_client = NULL;
  }

  while (m_my_media != NULL) {
    CPlayerMedia *p;
    p = m_my_media;
    m_my_media = p->get_next();
    delete p;
  }  

  if (m_sdp_info) {
    sdp_free_session_desc(m_sdp_info);
    m_sdp_info = NULL;
  }

  if (m_sync_sem) {
    SDL_DestroySemaphore(m_sync_sem);
    m_sync_sem = NULL;
  }
  
  if (m_video_sync != NULL) {
    delete m_video_sync;
    m_video_sync = NULL;
  }

  if (m_audio_sync != NULL) {
    delete m_audio_sync;
    m_audio_sync = NULL;
  }
  if (m_session_name) {
    free((void *)m_session_name);
    m_session_name = NULL;
  }
  if (m_content_base) {
    free((void *) m_content_base);
    m_content_base = NULL;
  }
  for (int ix = 0; ix < SESSION_DESC_COUNT; ix++) {
    if (m_session_desc[ix] != NULL) 
      free((void *)m_session_desc[ix]);
    m_session_desc[ix] = NULL;
  }
  
  if (m_media_close_callback != NULL) {
    m_media_close_callback(m_media_close_callback_data);
  }

  while (m_unused_ports != NULL) {
    CIpPort *first;
    first = m_unused_ports;
    m_unused_ports = first->get_next();
    delete first;
  }
  if (hadthread != 0) {
    SDL_Quit();
  }
}

int CPlayerSession::create_streaming_broadcast (session_desc_t *sdp,
						char *ermsg,
						uint32_t errlen)
{
  session_set_seekable(0);
  m_sdp_info = sdp;
  m_streaming = 1;
  m_rtp_over_rtsp = 0;
  return (0);
}

/*
 * create_streaming - create a session for streaming.  Create an
 * RTSP session with the server, get the SDP information from it.
 */
int CPlayerSession::create_streaming_ondemand (const char *url, 
					       char *errmsg,
					       uint32_t errlen, 
					       int use_tcp)
{
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  sdp_decode_info_t *sdpdecode;
  int dummy;
  int err;

  // streaming has seek capability (at least on demand)
  session_set_seekable(1);
  post("pdp_mp4playersession : creating streaming %s (use_tcp=%d)", url, use_tcp);
  memset(&cmd, 0, sizeof(rtsp_command_t));

  /*
   * create RTSP session
   */
  if (use_tcp != 0) {
    m_rtsp_client = rtsp_create_client_for_rtp_tcp(url, &err);
  } else {
    m_rtsp_client = rtsp_create_client(url, &err);
  }
  if (m_rtsp_client == NULL) {
    snprintf(errmsg, errlen, "Failed to create RTSP client");
    player_error_message("Failed to create rtsp client - error %d", err);
    return (err);
  }
  m_rtp_over_rtsp = use_tcp;

  cmd.accept = "application/sdp";

  /*
   * Send the RTSP describe.  This should return SDP information about
   * the session.
   */
  int rtsp_resp;

  rtsp_resp = rtsp_send_describe(m_rtsp_client, &cmd, &decode);
  if (rtsp_resp != RTSP_RESPONSE_GOOD) {
    int retval;
    if (decode != NULL) {
      retval = (((decode->retcode[0] - '0') * 100) +
		((decode->retcode[1] - '0') * 10) +
		(decode->retcode[2] - '0'));
      snprintf(errmsg, errlen, "RTSP describe error %d %s", retval,
	       decode->retresp != NULL ? decode->retresp : "");
      free_decode_response(decode);
    } else {
      retval = -1;
      snprintf(errmsg, errlen, "RTSP return invalid %d", rtsp_resp);
    }
    player_error_message("Describe response not good (error=%s)\n", errmsg);
    return (retval);
  }

  sdpdecode = set_sdp_decode_from_memory(decode->body);
  if (sdpdecode == NULL) {
    snprintf(errmsg, errlen, "Memory failure");
    player_error_message("Couldn't get sdp decode\n");
    free_decode_response(decode);
    return (-1);
  }

  /*
   * Decode the SDP information into structures we can use.
   */
  err = sdp_decode(sdpdecode, &m_sdp_info, &dummy);
  free(sdpdecode);
  if (err != 0) {
    snprintf(errmsg, errlen, "Couldn't decode session description %s",
	     decode->body);
    player_error_message("Couldn't decode sdp %s", decode->body);
    free_decode_response(decode);
    return (-1);
  }
  if (dummy != 1) {
    snprintf(errmsg, errlen, "Incorrect number of sessions in sdp decode %d",
	     dummy);
    player_error_message(errmsg);
    free_decode_response(decode);
    return (-1);
  }

  if (m_sdp_info->control_string != NULL) {
    set_session_control(1);
  }
  /*
   * Make sure we can use the urls in the sdp info
   */
  if (decode->content_location != NULL) {
    // Note - we may have problems if the content location is not absolute.
    m_content_base = strdup(decode->content_location);
  } else if (decode->content_base != NULL) {
    m_content_base = strdup(decode->content_base);
  } else {
    int urllen = strlen(url);
    if (url[urllen] != '/') {
      char *temp;
      temp = (char *)malloc(urllen + 2);
      strcpy(temp, url);
      strcat(temp, "/");
      m_content_base = temp;
    } else {
      m_content_base = strdup(url);
    }
  }

  convert_relative_urls_to_absolute(m_sdp_info,
				    m_content_base);

  free_decode_response(decode);
  m_streaming = 1;
  return (0);
}

CPDPVideoSync * CPlayerSession::set_up_video_sync (void)
{
  if (m_video_sync == NULL) {
    m_video_sync = pdp_create_video_sync(this, m_father);
  }
  return m_video_sync;
}

CPDPAudioSync *CPlayerSession::set_up_audio_sync (void)
{
  if (m_audio_sync == NULL) {
    m_audio_sync = pdp_create_audio_sync(this, m_father);
  }
  return m_audio_sync;
}
/*
 * set_up_sync_thread.  Creates the sync thread, and a sync class
 * for each media
 */
void CPlayerSession::set_up_sync_thread(void) 
{
  CPlayerMedia *media;

  media = m_my_media;
  while (media != NULL) {
    if (media->is_video()) {
      media->set_video_sync(set_up_video_sync());
    } else {
      media->set_audio_sync(set_up_audio_sync());
    }
    media= media->get_next();
  }
  m_sync_sem = SDL_CreateSemaphore(0);
}

/*
 * play_all_media - get all media to play
 */
int CPlayerSession::play_all_media (int start_from_begin, 
				    double start_time,
				    char *errmsg, 
				    uint32_t errlen)
{
  int ret;
  CPlayerMedia *p;
  range_desc_t *range;

  if (m_sdp_info && m_sdp_info->session_range.have_range != FALSE) {
    range = &m_sdp_info->session_range;
  } else {
    range = NULL;
    p = m_my_media;
    while (range == NULL && p != NULL) {
      media_desc_t *media;
      media = p->get_sdp_media_desc();
      if (media && media->media_range.have_range) {
	range = &media->media_range;
      }
      p = p->get_next();
    }
  }
  p = m_my_media;
  m_session_state = SESSION_BUFFERING;
  if (m_paused == 1 && start_time == 0.0 && start_from_begin == FALSE) {
    /*
     * we were paused.  Continue.
     */
    m_play_start_time = m_current_time;
    start_time = UINT64_TO_DOUBLE(m_current_time);
    start_time /= 1000.0;
    player_debug_message("Restarting at " U64 ", %g", m_current_time, start_time);
  } else {
    /*
     * We might have been paused, but we're told to seek
     */
    // Indicate what time we're starting at for sync task.
    m_play_start_time = (uint64_t)(start_time * 1000.0);
  }
  m_paused = 0;

  send_sync_thread_a_message(MSG_START_SESSION);
  // If we're doing aggregate rtsp, send the play command...

  if (session_control_is_aggregate()) {
    char buffer[80];
    rtsp_command_t cmd;
    rtsp_decode_t *decode;

    memset(&cmd, 0, sizeof(rtsp_command_t));
    if (range != NULL) {
      uint64_t stime = (uint64_t)(start_time * 1000.0);
      uint64_t etime = (uint64_t)(range->range_end * 1000.0);
      sprintf(buffer, "npt="U64"."U64"-"U64"."U64, 
	      stime / 1000, stime % 1000, etime / 1000, etime % 1000);
      cmd.range = buffer;
    }
    if (rtsp_send_aggregate_play(m_rtsp_client,
				 m_sdp_info->control_string,
				 &cmd,
				 &decode) != 0) {
      if (errmsg != NULL) {
	snprintf(errmsg, errlen, "RTSP Aggregate Play Error %s-%s", 
		 decode->retcode,
		 decode->retresp != NULL ? decode->retresp : "");
      }
      player_debug_message("RTSP aggregate play command failed");
      free_decode_response(decode);
      return (-1);
    }
    if (decode->rtp_info == NULL) {
      player_error_message("No rtp info field");
    } else {
      player_debug_message("rtp info is \'%s\'", decode->rtp_info);
    }
    int ret = pdp_process_rtsp_rtpinfo(decode->rtp_info, this, NULL);
    free_decode_response(decode);
    if (ret < 0) {
      if (errmsg != NULL) {
	snprintf(errmsg, errlen, "RTSP aggregate RtpInfo response failure");
      }
      player_debug_message("rtsp aggregate rtpinfo failed");
      return (-1);
    }
  }

  while (p != NULL) {
    ret = p->do_play(start_time, errmsg, errlen);
    if (ret != 0) return (ret);
    p = p->get_next();
  }
  return (0);
}

/*
 * pause_all_media - do a spin loop until the sync thread indicates it's
 * paused.
 */
int CPlayerSession::pause_all_media (void) 
{
  int ret;
  CPlayerMedia *p;
  m_session_state = SESSION_PAUSED;
  if (session_control_is_aggregate()) {
    rtsp_command_t cmd;
    rtsp_decode_t *decode;

    memset(&cmd, 0, sizeof(rtsp_command_t));
    if (rtsp_send_aggregate_pause(m_rtsp_client,
				  m_sdp_info->control_string,
				  &cmd,
				  &decode) != 0) {
      player_debug_message("RTSP aggregate pause command failed");
      free_decode_response(decode);
      return (-1);
    }
    free_decode_response(decode);
  }
  p = m_my_media;
  while (p != NULL) {
    ret = p->do_pause();
    if (ret != 0) return (ret);
    p = p->get_next();
  }
  m_sync_pause_done = 0;
  send_sync_thread_a_message(MSG_PAUSE_SESSION);
#ifndef NEED_SDL_VIDEO_IN_MAIN_THREAD
  do {
#endif
    SDL_Delay(100);
#ifndef NEED_SDL_VIDEO_IN_MAIN_THREAD
  } while (m_sync_pause_done == 0);
#endif
  m_paused = 1;
  return (0);
}
void CPlayerSession::add_media (CPlayerMedia *m) 
{
  CPlayerMedia *p;
  if (m_my_media == NULL) {
    m_my_media = m;
  } else {
    p = m_my_media;
    while (p->get_next() != NULL) {
      if (p == m) return;
      p = p->get_next();
    }
    p->set_next(m);
  }
}

int CPlayerSession::session_has_audio (void)
{
  CPlayerMedia *p;
  p = m_my_media;
  while (p != NULL) {
    if (p->is_video() == FALSE) {
      return (1);
    }
    p = p->get_next();
  }
  return (0);
}

int CPlayerSession::session_has_video (void)
{
  CPlayerMedia *p;
  p = m_my_media;
  while (p != NULL) {
    if (p->is_video() != FALSE) {
      return (1);
    }
    p = p->get_next();
  }
  return (0);
}

void CPlayerSession::set_audio_volume (int volume)
{
  m_audio_volume = volume;
  if (m_audio_sync) {
    m_audio_sync->set_volume(m_audio_volume);
  }
}

double CPlayerSession::get_max_time (void)
{
  CPlayerMedia *p;
  double max = 0.0;
  p = m_my_media;
  while (p != NULL) {
    double temp = p->get_max_playtime();
    if (temp > max) max = temp;
    p = p->get_next();
  }
  return (max);
}

/*
 * Matches a url with the corresponding media. 
 * Return the media, or NULL if no match. 
 */
CPlayerMedia *CPlayerSession::rtsp_url_to_media (const char *url)
{
  CPlayerMedia *p = m_my_media;
  while (p != NULL) {
	rtsp_session_t *session = p->get_rtsp_session();
	if (rtsp_is_url_my_stream(session, url, m_content_base, 
							  m_session_name) == 1) 
	  return p;
    p = p->get_next();
  }
  return (NULL);
}

int CPlayerSession::set_session_desc (int line, const char *desc)
{
  if (line >= SESSION_DESC_COUNT) {
    return -1;
  }
  if (m_session_desc[line] != NULL) free((void *)m_session_desc[line]);
  m_session_desc[line] = strdup(desc);
  if (m_session_desc[line] == NULL) 
    return -1;
  return (0);
}

const char *CPlayerSession::get_session_desc (int line)
{
  return m_session_desc[line];
}
/*
 * audio_is_ready - when the audio indicates that it's ready, it will
 * send a latency number, and a play time
 */
void CPlayerSession::audio_is_ready (uint64_t latency, uint64_t time)
{
  m_start = get_time_of_day();
  sync_message(LOG_DEBUG, "Aisready "U64, m_start);
  m_start -= time;
  m_latency = latency;
  if (latency != 0) {
    m_clock_wrapped = -1;
  }
  sync_message(LOG_DEBUG, "Audio is ready "U64" - latency "U64, time, latency);
  sync_message(LOG_DEBUG, "m_start is "X64, m_start);
  m_waiting_for_audio = 0;
  SDL_SemPost(m_sync_sem);
}

void CPlayerSession::adjust_start_time (int64_t time)
{
  m_start -= time;
  m_clock_wrapped = -1;
#if 0
  sync_message(LOG_INFO, "Adjusting start time "LLD " to " LLU, time,
	       get_current_time());
#endif
  SDL_SemPost(m_sync_sem);
}

/*
 * get_current_time.  Gets the time of day, subtracts off the start time
 * to get the current play time.
 */
uint64_t CPlayerSession::get_current_time (void)
{
  uint64_t current_time;

  if (m_waiting_for_audio != 0) {
    return 0;
  }
  current_time = get_time_of_day();
#if 0
  sync_message(LOG_DEBUG, "current time %llx m_start %llx", 
	       current_time, m_start);
  if (current_time < m_start) {
    if (m_clock_wrapped == -1) {
      return (0);
    } else {
      m_clock_wrapped = 1;
    }
  } else{
    if (m_clock_wrapped > 0) {
      uint64_t temp;
      temp = 1;
      temp <<= 32;
      temp /= 1000;
      current_time += temp;
    } else {
      m_clock_wrapped = 0;
    }
  }
#endif
  //if (current_time < m_start) return 0;
  m_current_time = current_time - m_start;
  if (m_current_time >= m_latency) m_current_time -= m_latency;
  return(m_current_time);
}

void CPlayerSession::syncronize_rtp_bytestreams (rtcp_sync_t *sync)
{
  if (sync != NULL) {
    m_audio_rtcp_sync = *sync;
    m_have_audio_rtcp_sync = true;
  } else {
    if (!m_have_audio_rtcp_sync) 
      return;
  }
  CPlayerMedia *mptr = m_my_media;
  while (mptr != NULL) {
    if (mptr->is_video()) {
      mptr->syncronize_rtp_bytestreams(&m_audio_rtcp_sync);
    }
    mptr = mptr->get_next();
  }
}

int CPlayerSession::process_msg_queue (int state) 
{
  CMsg *newmsg;
  while ((newmsg = m_sync_thread_msg_queue.get_message()) != NULL) {
#ifdef DEBUG_SYNC_MSGS
    sync_message(LOG_DEBUG, "Sync thread msg %d", newmsg->get_value());
#endif
    switch (newmsg->get_value()) {
    case MSG_PAUSE_SESSION:
      state = SYNC_STATE_PAUSED;
      break;
    case MSG_START_SESSION:
      state = SYNC_STATE_WAIT_SYNC;
      break;
    case MSG_STOP_THREAD:
      state = SYNC_STATE_EXIT;
      break;
    default:
      sync_message(LOG_ERR, "Sync thread received message %d", 
		   newmsg->get_value());
      break;
    }
    delete newmsg;
    newmsg = NULL;
  }
  return (state);
}

/***************************************************************************
 * Sync thread state handlers
 ***************************************************************************/

/*
 * sync_thread_init - wait until all the sync routines are initialized.
 */
int CPlayerSession::sync_thread_init (void)
{
  int ret = 1;
  for (CPlayerMedia *mptr = m_my_media;
       mptr != NULL && ret == 1;
       mptr = mptr->get_next()) {
    if (mptr->is_video()) {
      ret = m_video_sync->initialize_video(m_session_name);
    } else {
      ret = m_audio_sync->initialize_audio(m_video_sync != NULL);
    }
  }
  if (ret == -1) {
    sync_message(LOG_CRIT, "Fatal error while initializing hardware");
    if (m_video_sync != NULL) {
      m_video_sync->flush_sync_buffers();
    }
    if (m_audio_sync != NULL) {
      m_audio_sync->flush_sync_buffers();
    }
    m_master_msg_queue->send_message(MSG_RECEIVED_QUIT, 
				     NULL, 
				     0, 
				     m_master_msg_queue_sem);
    m_hardware_error = 1;
    return (SYNC_STATE_DONE);
  }
	
  if (ret == 1) {
    return (SYNC_STATE_WAIT_SYNC); 
  } 
  CMsg *newmsg;

  newmsg = m_sync_thread_msg_queue.get_message();
  if (newmsg != NULL) {
    int value = newmsg->get_value();
    delete newmsg;

    if (value == MSG_STOP_THREAD) {
      return (SYNC_STATE_EXIT);
    }
    if (value == MSG_PAUSE_SESSION) {
      m_sync_pause_done = 1;
    }
  }

  SDL_Delay(100);
	
  return (SYNC_STATE_INIT);
}

/*
 * sync_thread_wait_sync - wait until all decoding threads have put
 * data into the sync classes.
 */
int CPlayerSession::sync_thread_wait_sync (void)
{
  int state;

  state = process_msg_queue(SYNC_STATE_WAIT_SYNC);
  if (state == SYNC_STATE_WAIT_SYNC) {
      
      // We're not synced.  See if the video is ready, and if the audio
      // is ready.  Then start them going...
      int vsynced = 1, asynced = 1;
      uint64_t astart, vstart;

      if (m_video_sync) {
	vsynced = m_video_sync->is_video_ready(vstart);
      } 

      if (m_audio_sync) {
	asynced = m_audio_sync->is_audio_ready(astart); 
      }
      if (vsynced == 1 && asynced == 1) {
	/*
	 * Audio and video are synced. 
	 */
	if (m_audio_sync) {
	  /* 
	   * If we have audio, we use that for syncing.  Start it up
	   */
	  m_first_time_played = astart;
	  m_current_time = astart;
	  sync_message(LOG_DEBUG, "Astart is %llu", astart);
	  m_waiting_for_audio = 1;
	  state = SYNC_STATE_WAIT_AUDIO;
	  m_audio_sync->play_audio();
	} else 	if (m_video_sync) {
	  /*
	   * Video only - set up the start time based on the video time
	   * returned
	   */
	  m_first_time_played = vstart;
	  m_current_time = vstart;
	  m_waiting_for_audio = 0;
	  m_start = get_time_of_day();
	  m_start -= m_current_time;
	  state = SYNC_STATE_PLAYING;
	}
	sync_message(LOG_DEBUG, 
		     "Resynced at time "U64 " "U64, m_current_time, vstart);
      } else {
	SDL_Delay(10);
      }
    }
  return (state);
}

/*
 * sync_thread_wait_audio - wait until the audio thread starts and signals
 * us.
 */
int CPlayerSession::sync_thread_wait_audio (void)
{
  int state;

  state = process_msg_queue(SYNC_STATE_WAIT_AUDIO);
  if (state == SYNC_STATE_WAIT_AUDIO) {
    if (m_waiting_for_audio != 0) {
      SDL_SemWaitTimeout(m_sync_sem, 10);
    } else {
      // make sure we set the current time
      get_current_time();
      sync_message(LOG_DEBUG, "Current time is %llu", m_current_time);
      return (SYNC_STATE_PLAYING);
    }
  }
  return (state);
}

/*
 * sync_thread_playing - do the work of displaying the video and making
 * sure we're in sync.
 */
int CPlayerSession::sync_thread_playing (void) 
{
  int state;
  uint64_t audio_resync_time = 0;
  int64_t video_status = 0;
  int have_audio_eof = 0, have_video_eof = 0;

  state = process_msg_queue(SYNC_STATE_PLAYING);
  if (state == SYNC_STATE_PLAYING) {
    get_current_time();
    if (m_audio_sync) {
      audio_resync_time = m_audio_sync->check_audio_sync(m_current_time, 
							 have_audio_eof);
    }
    if (m_video_sync) {
      video_status = m_video_sync->play_video_at(m_current_time, 
						 have_video_eof);
    }

    int delay = 9;
    int wait_for_signal = 0;

    if (m_video_sync && m_audio_sync) {
      if (have_video_eof && have_audio_eof) {
	return (SYNC_STATE_DONE);
      }
      if (video_status > 0 || audio_resync_time != 0) {
	if (audio_resync_time != 0) {
	  int64_t diff = audio_resync_time - m_current_time;
	  delay = (int)MIN(diff, video_status);
	} else {
	  if (video_status < 9) {
	    delay = 0;
	  }
	}
	if (delay < 9) {
	  wait_for_signal = 0;
	} else {
	  wait_for_signal = 1;
	}
      } else {
	wait_for_signal = 0;
      }
    } else if (m_video_sync) {
      if (have_video_eof == 1) {
	return (SYNC_STATE_DONE);
      } 
      if (video_status >= 9) {
	wait_for_signal = 1;
	delay = (int)video_status;
      } else {
	wait_for_signal = 0;
      }
    } else {
      // audio only
      if (have_audio_eof == 1) {
	return (SYNC_STATE_DONE);
      }
      if (audio_resync_time != 0) {
	if (audio_resync_time - m_current_time > 10) {
	  wait_for_signal = 1;
	  delay = (int)(audio_resync_time - m_current_time);
	} else {
	  wait_for_signal = 0;
	}
      } else {
	wait_for_signal = 1;
      }
    }
    //player_debug_message("w %d d %d", wait_for_signal, delay);
    if (wait_for_signal) {
      if (delay > 9) {
	delay = 9;
      }
      //player_debug_message("sw %d", delay);

      SDL_SemWaitTimeout(m_sync_sem, delay);
    }
  }
  return (state);
}

/*
 * sync_thread_pause - wait for messages to continue or finish
 */
int CPlayerSession::sync_thread_paused (void)
{
  int state;
  state = process_msg_queue(SYNC_STATE_PAUSED);
  if (state == SYNC_STATE_PAUSED) {
    SDL_SemWaitTimeout(m_sync_sem, 10);
  }
  return (state);
}

/*
 * sync_thread_pause - wait for messages to exit, pretty much.
 */
int CPlayerSession::sync_thread_done (void)
{
  int state;
  state = process_msg_queue(SYNC_STATE_DONE);
  if (state == SYNC_STATE_DONE) {
    SDL_SemWaitTimeout(m_sync_sem, 10);
  } 
  return (state);
}

/*
 * sync_thread - call the correct handler, and wait for the state changes.
 * Each handler should only return the new state...
 */  
int CPlayerSession::sync_thread (int state)
{
  int newstate = 0;
  switch (state) {
  case SYNC_STATE_INIT:
    m_session_state = SESSION_BUFFERING;
    newstate = sync_thread_init();
    break;
  case SYNC_STATE_WAIT_SYNC:
    newstate = sync_thread_wait_sync();
    break;
  case SYNC_STATE_WAIT_AUDIO:
    newstate = sync_thread_wait_audio();
    break;
  case SYNC_STATE_PLAYING:
    newstate = sync_thread_playing();
    break;
  case SYNC_STATE_PAUSED:
    newstate = sync_thread_paused();
    break;
  case SYNC_STATE_DONE:
    newstate = sync_thread_done();
    break;
  }
#ifdef DEBUG_SYNC_STATE
  if (state != newstate)
    sync_message(LOG_INFO, "sync changed state %s to %s", 
		 sync_state[state], sync_state[newstate]);
#endif
  if (state != newstate) {
    state = newstate;
    switch (state) {
    case SYNC_STATE_WAIT_SYNC:
      m_session_state = SESSION_BUFFERING;
      break;
    case SYNC_STATE_WAIT_AUDIO:
      break;
    case SYNC_STATE_PLAYING:
      m_session_state = SESSION_PLAYING;
      break;
    case SYNC_STATE_PAUSED:
      if (m_video_sync != NULL) 
	m_video_sync->flush_sync_buffers();
      if (m_audio_sync != NULL) 
	m_audio_sync->flush_sync_buffers();
      m_session_state = SESSION_PAUSED;
      m_sync_pause_done = 1;
      break;
    case SYNC_STATE_DONE:
      m_master_msg_queue->send_message(MSG_SESSION_FINISHED, 
				       NULL, 
				       0, 
				       m_master_msg_queue_sem);
      m_session_state = SESSION_DONE;
      break;
    case SYNC_STATE_EXIT:
      if (m_video_sync != NULL) 
	m_video_sync->flush_sync_buffers();
      if (m_audio_sync != NULL) 
	m_audio_sync->flush_sync_buffers();
      break;
    }
  }
  return (state);
}

int pdp_sync_thread (void *data)
{
  CPlayerSession *p;
  int state = SYNC_STATE_INIT;
  p = (CPlayerSession *)data;
  do {
   state = p->sync_thread(state);
  } while (state != SYNC_STATE_EXIT);
  return (0);
}
