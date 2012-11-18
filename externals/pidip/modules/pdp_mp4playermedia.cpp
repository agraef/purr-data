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
 *
 * Adapted to PD/PDP by Yves Degoyon (ydegoyon@free.fr)
 */

/*
 * player_media.cpp - handle generic information about a stream
 */
#include "mpeg4ip.h"
#include "pdp_mp4playersession.h"
#include "pdp_mp4playermedia.h"
#include "pdp_mp4videosync.h"
#include "pdp_mp4audiosync.h"
#include "player_sdp.h"
#include "player_util.h"
#include <rtp/memory.h>
#include "pdp_mp4rtpbytestream.h"
#include "our_config_file.h"
#include "media_utils.h"
#include "ip_port.h"
#include "codec_plugin.h"
#include "audio.h"
#include <time.h>
#include <rtp/memory.h>
#include "our_config_file.h"
#include "rtp_plugin.h"
#include "media_utils.h"
#include "rfc3119_bytestream.h"
#include "mpeg3_rtp_bytestream.h"
#include "codec/mp3/mp3_rtp_bytestream.h"
#include "rtp_bytestream_plugin.h"
#include "codec_plugin_private.h"

static int pdp_recv_thread (void *data)
{
  CPlayerMedia *media;

  media = (CPlayerMedia *)data;
  return (media->recv_thread());
}

static int pdp_decode_thread (void *data)
{
  CPlayerMedia *media;
  media = (CPlayerMedia *)data;
  return (media->decode_thread());
}

static void pdp_rtp_packet_callback (void *data, 
				   unsigned char interleaved, 
				   struct rtp_packet *pak, 
				   int len)
{
  ((CPlayerMedia *)data)->rtp_receive_packet(interleaved, pak, len);
}

static int pdp_init_rtp_tcp (void *data)
{
  ((CPlayerMedia *)data)->rtp_init_tcp();
  return 0;
}

static int pdp_rtp_start (void *data)
{
  ((CPlayerMedia *)data)->rtp_start();
  return 0;
}

static int pdp_rtp_periodic (void *data)
{
  ((CPlayerMedia *)data)->rtp_periodic();
  return 0;
}

static void pdp_recv_callback (struct rtp *session, rtp_event *e)
{
  CPlayerMedia *m = (CPlayerMedia *)rtp_get_userdata(session);
  m->recv_callback(session, e);
}

static int pdp_rtcp_send_packet (void *ud, uint8_t *buffer, int buflen)
{
  return ((CPlayerMedia *)ud)->rtcp_send_packet(buffer, buflen);
}

CPlayerMedia::CPlayerMedia (CPlayerSession *p)
{
  m_plugin = NULL;
  m_plugin_data = NULL;
  m_next = NULL;
  m_parent = p;
  m_media_info = NULL;
  m_media_fmt = NULL;
  m_our_port = 0;
  m_ports = NULL;
  m_server_port = 0;
  m_source_addr = NULL;
  m_recv_thread = NULL;
  m_rtptime_tickpersec = 0;
  m_rtsp_base_seq_received = 0;
  m_rtsp_base_ts_received = 0;

  m_head = NULL;
  m_rtp_queue_len = 0;

  m_rtp_ssrc_set = FALSE;
  
  m_rtsp_session = NULL;
  m_decode_thread_waiting = 0;
  m_sync_time_set = FALSE;
  m_decode_thread = NULL;
  m_decode_thread_sem = NULL;
  m_video_sync = NULL;
  m_audio_sync = NULL;
  m_paused = 0;
  m_byte_stream = NULL;
  m_rtp_byte_stream = NULL;
  m_video_info = NULL;
  m_audio_info = NULL;
  m_user_data = NULL;
  m_rtcp_received = 0;
  m_streaming = 0;
  m_stream_ondemand = 0;
  m_rtp_use_rtsp = 0;
}

CPlayerMedia::~CPlayerMedia()
{
  rtsp_decode_t *rtsp_decode;

  media_message(LOG_DEBUG, "closing down media %d", m_is_video);
  if (m_rtsp_session) {
    // If control is aggregate, m_rtsp_session will be freed by
    // CPDPPlayerSession
    if (m_parent->session_control_is_aggregate() == 0) {
      rtsp_send_teardown(m_rtsp_session, NULL, &rtsp_decode);
      free_decode_response(rtsp_decode);
    }
    m_rtsp_session = NULL;
  }
  
  if (m_recv_thread) {
    m_rtp_msg_queue.send_message(MSG_STOP_THREAD);
    SDL_WaitThread(m_recv_thread, NULL);
    m_recv_thread = NULL;
  }

  if (m_decode_thread) {
    m_decode_msg_queue.send_message(MSG_STOP_THREAD, 
				    NULL, 
				    0, 
				    m_decode_thread_sem);
    SDL_WaitThread(m_decode_thread, NULL);
    m_decode_thread = NULL;
  }


    
  if (m_source_addr != NULL) free(m_source_addr);
  m_next = NULL;
  m_parent = NULL;

  if (m_ports) {
    delete m_ports;
    m_ports = NULL;
  }
  if (m_rtp_byte_stream) {
    double diff;
    diff = difftime(time(NULL), m_start_time);
    media_message(LOG_INFO, "Media %s", m_media_info->media);
    
    media_message(LOG_INFO, "Time: %g seconds", diff);
#if 0
    double div;
    player_debug_message("Packets received: %u", m_rtp_packet_received);
    player_debug_message("Payload received: "LLU" bytes", m_rtp_data_received);
    div = m_rtp_packet_received / diff;
    player_debug_message("Packets per sec : %g", div);
    div = UINT64_TO_DOUBLE(m_rtp_data_received);
	div *= 8.0;
	div /= diff;
    media_message(LOG_INFO, "Bits per sec   : %g", div);
#endif
			 
  }
  if (m_byte_stream) {
    delete m_byte_stream;
    m_byte_stream = NULL;
    m_rtp_byte_stream = NULL;
  }
  if (m_video_info) {
    free(m_video_info);
    m_video_info = NULL;
  }
  if (m_audio_info) {
    free(m_audio_info);
    m_audio_info = NULL;
  }
  if (m_user_data) {
    free((void *)m_user_data);
    m_user_data = NULL;
  }
  if (m_decode_thread_sem) {
    SDL_DestroySemaphore(m_decode_thread_sem);
    m_decode_thread_sem = NULL;
  }
}

void CPlayerMedia::clear_rtp_packets (void)
{
  if (m_head != NULL) {
    m_tail->rtp_next = NULL;
    while (m_head != NULL) {
      rtp_packet *p;
      p = m_head;
      m_head = m_head->rtp_next;
      p->rtp_next = p->rtp_prev = NULL;
      xfree(p);
    }
  }
  m_tail = NULL;
  m_rtp_queue_len = 0;
}

int CPlayerMedia::create_common (int is_video, char *errmsg, uint32_t errlen)
{
  m_parent->add_media(this);
  m_is_video = is_video;

  m_decode_thread_sem = SDL_CreateSemaphore(0);
  m_decode_thread = SDL_CreateThread(pdp_decode_thread, this);
  if (m_decode_thread_sem == NULL || m_decode_thread == NULL) {
    const char *outmedia;
    if (m_media_info == NULL) {
      outmedia = m_is_video ? "video" : "audio";
    } else outmedia = m_media_info->media;

    if (errmsg != NULL)
      snprintf(errmsg, errlen, "Couldn't start media thread for %s", 
	       outmedia);
    media_message(LOG_ERR, "Failed to create decode thread for media %s",
		  outmedia);
    return (-1);
  }
  return 0;
}
/*
 * CPlayerMedia::create - create when we've already got a
 * bytestream
 */
int CPlayerMedia::create (COurInByteStream *b, 
			  int is_video,
			  char *errmsg,
			  uint32_t errlen,
			  int streaming)
{
  m_byte_stream = b;
  m_streaming = streaming;
  return create_common(is_video, errmsg, errlen);
}

/*
 * CPlayerMedia::create_streaming - create a streaming media session,
 * including setting up rtsp session, rtp and rtp bytestream
 */
int CPlayerMedia::create_streaming (media_desc_t *sdp_media,
				    char *errmsg,
				    uint32_t errlen,
				    int ondemand,
				    int use_rtsp,
				    int media_number_in_session)
{
  char buffer[80];
  rtsp_command_t cmd;
  rtsp_decode_t *decode;
  
  m_streaming = 1;
  if (sdp_media == NULL) {
    snprintf(errmsg, errlen, "Internal media error - sdp is NULL");
    return(-1);
  }

  if (strncasecmp(sdp_media->proto, "RTP", strlen("RTP")) != 0) {
    snprintf(errmsg, errlen, "Media %s doesn't use RTP", sdp_media->media);
    media_message(LOG_ERR, "%s doesn't use RTP", sdp_media->media);
    return (-1);
  }
  if (sdp_media->fmt == NULL) {
    snprintf(errmsg, errlen, "Media %s doesn't have any usuable formats",
	     sdp_media->media);
    media_message(LOG_ERR, "%s doesn't have any formats", 
		  sdp_media->media);
    return (-1);
  }

  m_media_info = sdp_media;
  m_stream_ondemand = ondemand;
  if (ondemand != 0) {
    /*
     * Get 2 consecutive IP ports.  If we don't have this, don't even
     * bother
     */
    if (use_rtsp == 0) {
      m_ports = new C2ConsecIpPort(m_parent->get_unused_ip_port_ptr());
      if (m_ports == NULL || !m_ports->valid()) {
	snprintf(errmsg, errlen, "Could not find any valid IP ports");
	media_message(LOG_ERR, "Couldn't get valid IP ports");
	return (-1);
      }
      m_our_port = m_ports->first_port();

      /*
       * Send RTSP setup message - first create the transport string for that
       * message
       */
      create_rtsp_transport_from_sdp(m_parent->get_sdp_info(),
				     m_media_info,
				     m_our_port,
				     buffer,
				     sizeof(buffer));
    } else {
      m_rtp_use_rtsp = 1;
      m_rtp_media_number_in_session = media_number_in_session;
      snprintf(buffer, sizeof(buffer), "RTP/AVP/TCP;unicast;interleaved=%d-%d",
	       media_number_in_session * 2, (media_number_in_session * 2) + 1);
    }
    memset(&cmd, 0, sizeof(rtsp_command_t));
    cmd.transport = buffer;
    int err = 
      rtsp_send_setup(m_parent->get_rtsp_client(),
		      m_media_info->control_string,
		      &cmd,
		      &m_rtsp_session,
		      &decode,
		      m_parent->session_control_is_aggregate());
    if (err != 0) {
      snprintf(errmsg, errlen, "Couldn't set up session %s", 
	       m_media_info->control_string);
      media_message(LOG_ERR, "Can't create session %s - error code %d", 
		    m_media_info->media, err);
      if (decode != NULL)
	free_decode_response(decode);
      return (-1);
    }
    cmd.transport = NULL;
    media_message(LOG_INFO, "Transport returned is %s", decode->transport);

    /*
     * process the transport they sent.  They need to send port numbers, 
     * addresses, rtptime information, that sort of thing
     */
    if (m_source_addr == NULL) {
      m_source_addr = rtsp_get_server_ip_address_string(m_rtsp_session);
      media_message(LOG_INFO, "setting default source address from rtsp %s", m_source_addr);
    }

    if (process_rtsp_transport(decode->transport) != 0) {
      snprintf(errmsg, errlen, "Couldn't process transport information in RTSP response: %s", decode->transport);
      free_decode_response(decode);
      return(-1);
    }
    free_decode_response(decode);
  } else {
    m_server_port = m_our_port = m_media_info->port;
  }
  connect_desc_t *cptr;
  cptr = get_connect_desc_from_media(m_media_info);
  if (cptr == NULL) {
    snprintf(errmsg, errlen, "Server did not return address");
    return (-1);
  }

  //
  // okay - here we want to check that the server port is set up, and
  // go ahead and init rtp, and the associated task
  //
  m_start_time = time(NULL);

  if (create_common(strcmp(sdp_media->media, "video") == 0, 
		    errmsg, errlen) < 0) {
    return -1;
  }

  if (ondemand == 0 || use_rtsp == 0) {
    m_rtp_inited = 0;
    m_recv_thread = SDL_CreateThread(pdp_recv_thread, this);
    if (m_recv_thread == NULL) {
      snprintf(errmsg, errlen, "Couldn't create media %s RTP recv thread",
	       m_media_info->media);
      media_message(LOG_ERR, errmsg);
      return (-1);
    }
    while (m_rtp_inited == 0) {
      SDL_Delay(10);
    }
    if (m_rtp_session == NULL) {
      snprintf(errmsg, errlen, "Could not start RTP - check debug log");
      media_message(LOG_ERR, errmsg);
      return (-1);
    }
  } else {
    int ret;
    ret = rtsp_thread_set_rtp_callback(m_parent->get_rtsp_client(),
				       pdp_rtp_packet_callback,
				       pdp_rtp_periodic,
				       m_rtp_media_number_in_session,
				       this);
    if (ret < 0) {
      snprintf(errmsg, errlen, "Can't setup TCP/RTP callback");
      return -1;
    }
    ret = rtsp_thread_perform_callback(m_parent->get_rtsp_client(),
				       pdp_init_rtp_tcp,
				       this);
    if (ret < 0) {
      snprintf(errmsg, errlen,  "Can't init RTP in RTSP thread");
      return -1;
    }
  }
  if (m_rtp_session == NULL) {
    snprintf(errmsg, errlen, "Couldn't create RTP session for media %s",
	     m_media_info->media);
    media_message(LOG_ERR, errmsg);
    return (-1);
  }
  return (0);
}

int CPlayerMedia::create_video_plugin (const codec_plugin_t *p,
				       const char *compressor, 
				       int type, 
				       int profile, 
				       format_list_t *sdp_media,
				       video_info_t *video,
				       const uint8_t *user_data,
				       uint32_t userdata_size)
{
  if (m_video_sync == NULL) {
    m_video_sync = m_parent->set_up_video_sync();
  }
  if (m_video_sync == NULL) return -1;

  m_plugin = p;
  m_video_info = video;
  m_plugin_data = (p->vc_create)(compressor, 
				 type,
				 profile, sdp_media,
				 video,
				 user_data,
				 userdata_size,
				 get_video_vft(),
				 m_video_sync);
  if (m_plugin_data == NULL) 
    return -1;

  if (user_data != NULL) 
    set_user_data(user_data, userdata_size);
  return 0;
}

void CPlayerMedia::set_plugin_data (const codec_plugin_t *p, 
				    codec_data_t *d, 
				    video_vft_t *v, 
				    audio_vft_t *a)
{
  m_plugin = p;
  m_plugin_data = d;
  if (is_video()) {
    if (m_video_sync == NULL) {
      m_video_sync = m_parent->set_up_video_sync();
    }
    d->ifptr = m_video_sync;
    d->v.video_vft = v;
  } else {
    if (m_audio_sync == NULL) {
      m_audio_sync = m_parent->set_up_audio_sync();
    }
    d->ifptr = m_audio_sync;
    d->v.audio_vft = a;
  }
    
}

int CPlayerMedia::get_plugin_status (char *buffer, uint32_t buflen)
{
  if (m_plugin == NULL) return -1;

  if (m_plugin->c_print_status == NULL) return -1;

  return ((m_plugin->c_print_status)(m_plugin_data, buffer, buflen));
}

int CPlayerMedia::create_audio_plugin (const codec_plugin_t *p,
				       const char *compressor, 
				       int type, 
				       int profile,
				       format_list_t *sdp_media,
				       audio_info_t *audio,
				       const uint8_t *user_data,
				       uint32_t userdata_size)
{
  if (m_audio_sync == NULL) {
    m_audio_sync = m_parent->set_up_audio_sync();
  }
  if (m_audio_sync == NULL) return -1;

  m_audio_info = audio;
  m_plugin = p;
  m_plugin_data = (p->ac_create)(compressor,
				 type, 
				 profile, 
				 sdp_media,
				 audio,
				 user_data,
				 userdata_size,
				 get_audio_vft(),
				 m_audio_sync);
  if (m_plugin_data == NULL) return -1;

  if (user_data != NULL)
    set_user_data(user_data, userdata_size);
  return 0;
}

/*
 * CPlayerMedia::do_play - get play command
 */
int CPlayerMedia::do_play (double start_time_offset,
			   char *errmsg, 
			   uint32_t errlen)
{

  if (m_streaming != 0) {
    if (m_stream_ondemand != 0) {
      /*
       * We're streaming - send the RTSP play command
       */
      if (m_parent->session_control_is_aggregate() == 0) {
	char buffer[80];
	rtsp_command_t cmd;
	rtsp_decode_t *decode;
	range_desc_t *range;
	memset(&cmd, 0, sizeof(rtsp_command_t));

	// only do range if we're not paused
	range = get_range_from_media(m_media_info);
	if (range != NULL) {
	  if (start_time_offset < range->range_start || 
	      start_time_offset > range->range_end) 
	    start_time_offset = range->range_start;
	  // need to check for smpte
	  sprintf(buffer, "npt=%g-%g", start_time_offset, range->range_end);
	  cmd.range = buffer;
	}

	if (rtsp_send_play(m_rtsp_session, &cmd, &decode) != 0) {
	  media_message(LOG_ERR, "RTSP play command failed");
	  if (errmsg != NULL) {
	    snprintf(errmsg, errlen, "RTSP Play Error %s-%s", 
		     decode->retcode,
		     decode->retresp != NULL ? decode->retresp : "");
	  }
	  free_decode_response(decode);
	  return (-1);
	}

	/*
	 * process the return information
	 */
	int ret = pdp_process_rtsp_rtpinfo(decode->rtp_info, m_parent, this);
	if (ret < 0) {
	  media_message(LOG_ERR, "rtsp rtpinfo failed");
	  free_decode_response(decode);
	  if (errmsg != NULL) {
	    snprintf(errmsg, errlen, "RTSP aggregate RtpInfo response failure");
	  }
	  return (-1);
	}
	free_decode_response(decode);
      }
      if (m_source_addr == NULL) {
	// get the ip address of the server from the rtsp stack
	m_source_addr = rtsp_get_server_ip_address_string(m_rtsp_session);
	media_message(LOG_INFO, "Setting source address from rtsp - %s", 
		      m_source_addr);
      }
      // ASDF - probably need to do some stuff here for no rtpinfo...
      /*
       * set the various play times, and send a message to the recv task
       * that it needs to start
       */
      m_play_start_time = start_time_offset;
    }
    if (m_byte_stream != NULL) {
      m_byte_stream->play((uint64_t)(start_time_offset * 1000.0));
    }
    m_paused = 0;
    if (m_rtp_use_rtsp) {
      rtsp_thread_perform_callback(m_parent->get_rtsp_client(),
				   pdp_rtp_start, 
				   this);
    } 
  } else {
    /*
     * File (or other) playback.
     */
    if (m_paused == 0 || start_time_offset == 0.0) {
      m_byte_stream->reset();
    }
    m_byte_stream->play((uint64_t)(start_time_offset * 1000.0));
    m_play_start_time = start_time_offset;
    m_paused = 0;
    start_decoding();
  }
  return (0);
}

/*
 * CPlayerMedia::do_pause - stop what we're doing
 */
int CPlayerMedia::do_pause (void)
{

  if (m_streaming != 0) {
    if (m_stream_ondemand != 0) {
      /*
     * streaming - send RTSP pause
     */
      if (m_parent->session_control_is_aggregate() == 0) {
	rtsp_command_t cmd;
	rtsp_decode_t *decode;
	memset(&cmd, 0, sizeof(rtsp_command_t));

	if (rtsp_send_pause(m_rtsp_session, &cmd, &decode) != 0) {
	  media_message(LOG_ERR, "RTSP play command failed");
	  free_decode_response(decode);
	  return (-1);
	}
	free_decode_response(decode);
      }
    }
    if (m_recv_thread != NULL) {
      m_rtp_msg_queue.send_message(MSG_PAUSE_SESSION);
    }
  }

  if (m_byte_stream != NULL) 
    m_byte_stream->pause();
  /*
   * Pause the various threads
   */
  m_decode_msg_queue.send_message(MSG_PAUSE_SESSION, 
				  NULL, 
				  0, 
				  m_decode_thread_sem);
  m_paused = 1;
  return (0);
}

double CPlayerMedia::get_max_playtime (void) 
{
  if (m_byte_stream) {
    return (m_byte_stream->get_max_playtime());
  }
  return (0.0);
}

/***************************************************************************
 * Transport and RTP-Info RTSP header line parsing.
 ***************************************************************************/
#define ADV_SPACE(a) {while (isspace(*(a)) && (*(a) != '\0'))(a)++;}

#define TTYPE(a,b) {a, sizeof(a), b}

static char *transport_parse_unicast (char *transport, CPlayerMedia *m)
{
  ADV_SPACE(transport);
  if (*transport == '\0') return (transport);

  if (*transport != ';')
    return (NULL);
  transport++;
  ADV_SPACE(transport);
  return (transport);
}

static char *transport_parse_multicast (char *transport, CPlayerMedia *m)
{
  media_message(LOG_ERR,"Received multicast indication during SETUP");
  return (NULL);
}

static char *convert_number (char *transport, uint32_t &value)
{
  value = 0;
  while (isdigit(*transport)) {
    value *= 10;
    value += *transport - '0';
    transport++;
  }
  return (transport);
}

static char *convert_hex (char *transport, uint32_t &value)
{
  value = 0;
  while (isxdigit(*transport)) {
    value *= 16;
    if (isdigit(*transport))
      value += *transport - '0';
    else
      value += tolower(*transport) - 'a' + 10;
    transport++;
  }
  return (transport);
}

static char *transport_parse_client_port (char *transport, CPlayerMedia *m)
{
  uint32_t port;
  uint16_t our_port, our_port_max;
  if (*transport++ != '=') {
    return (NULL);
  }
  ADV_SPACE(transport);
  transport = convert_number(transport, port);
  ADV_SPACE(transport);
  our_port = m->get_our_port();
  our_port_max = our_port + 1;

  if (port != our_port) {
    media_message(LOG_ERR, "Returned client port %u doesn't match sent %u",
		  port, our_port);
    return (NULL);
  }
  if (*transport == ';') {
    transport++;
    return (transport);
  }
  if (*transport == '\0') {
    return (transport);
  }
  if (*transport != '-') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_number(transport, port);
  if ((port < our_port) || 
      (port > our_port_max)) {
    media_message(LOG_ERR, "Illegal client to port %u, range %u to %u",
			 port, our_port, our_port_max);
    return (NULL);
  }
  ADV_SPACE(transport);
  if (*transport == ';') {
    transport++;
  }
  return(transport);
}

static char *transport_parse_server_port (char *transport, CPlayerMedia *m)
{
  uint32_t fromport, toport;

  if (*transport++ != '=') {
    return (NULL);
  }
  ADV_SPACE(transport);
  transport = convert_number(transport, fromport);
  ADV_SPACE(transport);

  m->set_server_port((uint16_t)fromport);

  if (*transport == ';') {
    transport++;
    return (transport);
  }
  if (*transport == '\0') {
    return (transport);
  }
  if (*transport != '-') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_number(transport, toport);
  if (toport < fromport || toport > fromport + 1) {
    media_message(LOG_ERR, "Illegal server to port %u, from is %u",
			 toport, fromport);
    return (NULL);
  }
  ADV_SPACE(transport);
  if (*transport == ';') {
    transport++;
  }
  return(transport);
}

static char *transport_parse_source (char *transport, CPlayerMedia *m)
{
  char *ptr, *newone;
  uint32_t addrlen;

  if (*transport != '=') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  ptr = transport;
  while (*transport != ';' && *transport != '\0') transport++;
  addrlen = transport - ptr;
  if (addrlen == 0) {
    return (NULL);
  }
  newone = (char *)malloc(addrlen + 1);
  if (newone == NULL) {
    media_message(LOG_ERR, "Can't alloc memory for transport source");
    return (NULL);
  }
  strncpy(newone, ptr, addrlen);
  newone[addrlen] = '\0';
  m->set_source_addr(newone);
  if (*transport == ';') transport++;
  return (transport);
}

static char *transport_parse_ssrc (char *transport, CPlayerMedia *m)
{
  uint32_t ssrc;
  if (*transport != '=') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_hex(transport, ssrc);
  ADV_SPACE(transport);
  if (*transport != '\0') {
    if (*transport != ';') {
      return (NULL);
    }
    transport++;
  }
  m->set_rtp_ssrc(ssrc);
  return (transport);
}

static char *transport_parse_interleave (char *transport, CPlayerMedia *m)
{
  uint32_t chan, chan2;
  if (*transport != '=') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_number(transport, chan);
  chan2 = m->get_rtp_media_number() * 2;
  if (chan != chan2) {
    media_message(LOG_ERR, "Transport interleave not what was requested %d %d", 
			 chan, chan2);
    return NULL;
  }
  ADV_SPACE(transport);
  if (*transport != '\0') {
    if (*transport != '-') {
      return (NULL);
    }
    transport++;
    transport = convert_number(transport, chan2);
    if (chan + 1 != chan2) {
      media_message(LOG_ERR, "Error in transport interleaved field");
      return (NULL);
    }
    
    if (*transport == '\0') return (transport);
  }
  if (*transport != ';') return (NULL);
  transport++;
  return (transport);
}

static char *rtpinfo_parse_ssrc (char *transport, CPlayerMedia *m, int &end)
{
  uint32_t ssrc;
  if (*transport != '=') {
    return (NULL);
  }
  transport++;
  ADV_SPACE(transport);
  transport = convert_hex(transport, ssrc);
  ADV_SPACE(transport);
  if (*transport != '\0') {
    if (*transport == ',') {
      end = 1;
    } else if (*transport != ';') {
      return (NULL);
    }
    transport++;
  }
  m->set_rtp_ssrc(ssrc);
  return (transport);
}

static char *rtpinfo_parse_seq (char *rtpinfo, CPlayerMedia *m, int &endofurl)
{
  uint32_t seq;
  if (*rtpinfo != '=') {
    return (NULL);
  }
  rtpinfo++;
  ADV_SPACE(rtpinfo);
  rtpinfo = convert_number(rtpinfo, seq);
  ADV_SPACE(rtpinfo);
  if (*rtpinfo != '\0') {
    if (*rtpinfo == ',') {
      endofurl = 1;
    } else if (*rtpinfo != ';') {
      return (NULL);
    }
    rtpinfo++;
  }
  m->set_rtp_base_seq(seq);
  return (rtpinfo);
}

static char *rtpinfo_parse_rtptime (char *rtpinfo, 
				    CPlayerMedia *m, 
				    int &endofurl)
{
  uint32_t rtptime;
  int neg = 0;
  if (*rtpinfo != '=') {
    return (NULL);
  }
  rtpinfo++;
  ADV_SPACE(rtpinfo);
  if (*rtpinfo == '-') {
    neg = 1;
    rtpinfo++;
    ADV_SPACE(rtpinfo);
  }
  rtpinfo = convert_number(rtpinfo, rtptime);
  ADV_SPACE(rtpinfo);
  if (*rtpinfo != '\0') {
    if (*rtpinfo == ',') {
      endofurl = 1;
    } else if (*rtpinfo != ';') {
      return (NULL);
    }
    rtpinfo++;
  }
  if (neg != 0) {
    player_error_message("Warning - negative time returned in rtpinfo");
    rtptime = 0 - rtptime;
  }
  m->set_rtp_base_ts(rtptime);
  return (rtpinfo);
}
struct {
  const char *name;
  uint32_t namelen;
  char *(*routine)(char *transport, CPlayerMedia *);
} transport_types[] = 
{
  TTYPE("unicast", transport_parse_unicast),
  TTYPE("multicast", transport_parse_multicast),
  TTYPE("client_port", transport_parse_client_port),
  TTYPE("server_port", transport_parse_server_port),
  TTYPE("source", transport_parse_source),
  TTYPE("ssrc", transport_parse_ssrc),
  TTYPE("interleaved", transport_parse_interleave),
  {NULL, 0, NULL},
}; 

int CPlayerMedia::process_rtsp_transport (char *transport)
{
  uint32_t protolen;
  int ix;

  if (transport == NULL) 
    return (-1);

  protolen = strlen(m_media_info->proto);
  
  if (strncasecmp(transport, m_media_info->proto, protolen) != 0) {
    media_message(LOG_ERR, "transport %s doesn't match %s", transport, 
			 m_media_info->proto);
    return (-1);
  }
  transport += protolen;
  if (*transport == '/') {
    transport++;
    if (m_rtp_use_rtsp) {
      if (strncasecmp(transport, "TCP", strlen("TCP")) != 0) {
	media_message(LOG_ERR, "Transport is not TCP");
	return (-1);
      }
      transport += strlen("TCP");
    } else {
      if (strncasecmp(transport, "UDP", strlen("UDP")) != 0) {
	media_message(LOG_ERR, "Transport is not UDP");
	return (-1);
      }
      transport += strlen("UDP");
    }
  }
  if (*transport != ';') {
    return (-1);
  }
  transport++;
  do {
    ADV_SPACE(transport);
    for (ix = 0; transport_types[ix].name != NULL; ix++) {
      if (strncasecmp(transport, 
		      transport_types[ix].name, 
		      transport_types[ix].namelen - 1) == 0) {
	transport += transport_types[ix].namelen - 1;
	ADV_SPACE(transport);
	transport = (transport_types[ix].routine)(transport, this);
	break;
      }
    }
    if (transport_types[ix].name == NULL) {
      media_message(LOG_INFO, "Illegal mime type in transport - skipping %s", 
			   transport);
      while (*transport != ';' && *transport != '\0') transport++;
      if (*transport != '\0') transport++;
    }
  } while (transport != NULL && *transport != '\0');

  if (transport == NULL) {
    return (-1);
  }
  return (0);
}

struct {
  const char *name;
  uint32_t namelen;
  char *(*routine)(char *transport, CPlayerMedia *, int &end_for_url);
} rtpinfo_types[] = 
{
  TTYPE("seq", rtpinfo_parse_seq),
  TTYPE("rtptime", rtpinfo_parse_rtptime),
  TTYPE("ssrc", rtpinfo_parse_ssrc),
  {NULL, 0, NULL},
};

int pdp_process_rtsp_rtpinfo (char *rtpinfo, 
			  CPlayerSession *session,
			  CPlayerMedia *media)
{
  int ix;
  CPlayerMedia *newmedia;
  if (rtpinfo == NULL) 
    return (0);

  do {
    int no_mimes = 0;
    ADV_SPACE(rtpinfo);
    if (strncasecmp(rtpinfo, "url", strlen("url")) != 0) {
      media_message(LOG_ERR, "Url not found");
      return (-1);
    }
    rtpinfo += strlen("url");
    ADV_SPACE(rtpinfo);
    if (*rtpinfo != '=') {
      media_message(LOG_ERR, "Can't find = after url");
      return (-1);
    }
    rtpinfo++;
    ADV_SPACE(rtpinfo);
    char *url = rtpinfo;
    while (*rtpinfo != '\0' && *rtpinfo != ';' && *rtpinfo != ',') {
      rtpinfo++;
    }
    if (*rtpinfo == '\0') {
      no_mimes = 1;
    } else {
      if (*rtpinfo == ',') {
	no_mimes = 1;
      }
      *rtpinfo++ = '\0';
    }
    char *temp = url;
    newmedia = session->rtsp_url_to_media(url);
    if (newmedia == NULL) {
      media_message(LOG_ERR, "Can't find media from %s", url);
      return -1;
    } else if (media != NULL && media != newmedia) {
      media_message(LOG_ERR, "Url in rtpinfo does not match media %s", url);
      return -1;
    }
    if (temp != url) 
      free(url);

    if (no_mimes == 0) {
    int endofurl = 0;
    do {
      ADV_SPACE(rtpinfo);
      for (ix = 0; rtpinfo_types[ix].name != NULL; ix++) {
	if (strncasecmp(rtpinfo,
			rtpinfo_types[ix].name, 
			rtpinfo_types[ix].namelen - 1) == 0) {
	  rtpinfo += rtpinfo_types[ix].namelen - 1;
	  ADV_SPACE(rtpinfo);
	  rtpinfo = (rtpinfo_types[ix].routine)(rtpinfo, newmedia, endofurl);
	  break;
	}
      }
      if (rtpinfo_types[ix].name == NULL) {
#if 1
	media_message(LOG_INFO, "Unknown mime-type in RtpInfo - skipping %s", 
			     rtpinfo);
#endif
	while (*rtpinfo != ';' && *rtpinfo != '\0') rtpinfo++;
	if (*rtpinfo != '\0') rtpinfo++;
      }
    } while (endofurl == 0 && rtpinfo != NULL && *rtpinfo != '\0');
    } 
    newmedia = NULL;
  } while (rtpinfo != NULL && *rtpinfo != '\0');

  if (rtpinfo == NULL) {
    return (-1);
  }

  return (1);
}

int CPlayerMedia::rtp_receive_packet (unsigned char interleaved, 
				      struct rtp_packet *pak, 
				      int len)
{
  int ret;
  if ((interleaved & 1) == 0) {
    ret = rtp_process_recv_data(m_rtp_session, 0, pak, len);
    if (ret < 0) {
      xfree(pak);
    }
  } else {
    uint8_t *pakbuf = (uint8_t *)pak;
    pakbuf += sizeof(rtp_packet_data);
	    
    rtp_process_ctrl(m_rtp_session, pakbuf, len);
    xfree(pak);
    ret = 0;
  }
  return ret;
}

void CPlayerMedia::rtp_periodic (void)
{
  rtp_send_ctrl(m_rtp_session, 
		m_rtp_byte_stream != NULL ? 
		m_rtp_byte_stream->get_last_rtp_timestamp() : 0, 
		NULL);
  rtp_update(m_rtp_session);
  if (m_rtp_byte_stream != NULL) {
    int ret = m_rtp_byte_stream->recv_task(m_decode_thread_waiting);
    if (ret > 0) {
      if (m_rtp_buffering == 0) {
	m_rtp_buffering = 1;
	start_decoding();
      } else {
	bytestream_primed();
      }
    }
    return;
  }
  if (m_head != NULL) {
    /*
     * Make sure that the payload type is the same
     */
    if (m_head->rtp_pak_pt == m_tail->rtp_pak_pt) {
      if (m_rtp_queue_len > 10) { // 10 packets consecutive proto same
	if (determine_payload_type_from_rtp() == FALSE) {
	  clear_rtp_packets(); 
	}
      }
    } else {
      clear_rtp_packets();
    }
  }
}

void CPlayerMedia::rtp_start (void)
{
  if (m_rtp_ssrc_set == TRUE) {
    rtp_set_my_ssrc(m_rtp_session, m_rtp_ssrc);
  } else {
    // For now - we'll set up not to wait for RTCP validation 
    // before indicating if rtp library should accept.
    rtp_set_option(m_rtp_session, RTP_OPT_WEAK_VALIDATION, FALSE);
    rtp_set_option(m_rtp_session, RTP_OPT_PROMISC, TRUE);
  }
  if (m_rtp_byte_stream != NULL) {
    //m_rtp_byte_stream->reset(); - gets called when pausing
    m_rtp_byte_stream->flush_rtp_packets();
  }
  m_rtp_buffering = 0;
}

void CPlayerMedia::rtp_end(void)
{
  if (m_rtp_session != NULL) {
    rtp_send_bye(m_rtp_session);
    rtp_done(m_rtp_session);
  }
  m_rtp_session = NULL;
}

int CPlayerMedia::rtcp_send_packet (uint8_t *buffer, int buflen)
{
  if (config.get_config_value(CONFIG_SEND_RTCP_IN_RTP_OVER_RTSP) != 0) {
    return rtsp_thread_send_rtcp(m_parent->get_rtsp_client(),
				 m_rtp_media_number_in_session,
				 buffer, 
				 buflen);
  }
  return buflen;
}

int CPlayerMedia::recv_thread (void)
{
  struct timeval timeout;
  int retcode;
  CMsg *newmsg;
  int recv_thread_stop = 0;
  connect_desc_t *cptr;
  cptr = get_connect_desc_from_media(m_media_info);


  m_rtp_buffering = 0;
  if (m_stream_ondemand != 0) {
    /*
     * We need to free up the ports that we got before RTP tries to set 
     * them up, so we don't have any re-use conflicts.  There is a small
     * window here that they might get used...
     */
    delete m_ports; // free up the port numbers
    m_ports = NULL;
  }

  double bw;

  if (find_rtcp_bandwidth_from_media(m_media_info, &bw) < 0) {
    bw = 5000.0;
  } else {
    media_message(LOG_DEBUG, "Using bw from sdp %g", bw);
  }
  m_rtp_session = rtp_init(m_source_addr == NULL ? 
			   cptr->conn_addr : m_source_addr,
			   m_our_port,
			   m_server_port,
			   cptr == NULL ? 1 : cptr->ttl, // need ttl here
			   bw, // rtcp bandwidth ?
			   pdp_recv_callback,
			   (uint8_t *)this);
  if (m_rtp_session != NULL) {
    rtp_set_option(m_rtp_session, RTP_OPT_WEAK_VALIDATION, FALSE);
    rtp_set_option(m_rtp_session, RTP_OPT_PROMISC, TRUE);
    rtp_start();
  }
  m_rtp_inited = 1;
  
  while (recv_thread_stop == 0) {
    if ((newmsg = m_rtp_msg_queue.get_message()) != NULL) {
      //player_debug_message("recv thread message %d", newmsg->get_value());
      switch (newmsg->get_value()) {
      case MSG_STOP_THREAD:
	recv_thread_stop = 1;
	break;
      case MSG_PAUSE_SESSION:
	// Indicate that we need to restart the session.
	// But keep going...
	rtp_start();
	break;
      }
      delete newmsg;
      newmsg = NULL;
    }
    if (recv_thread_stop == 1) {
      continue;
    }
    if (m_rtp_session == NULL) {
      SDL_Delay(50); 
    } else {
      timeout.tv_sec = 0;
      timeout.tv_usec = 500000;
      retcode = rtp_recv(m_rtp_session, &timeout, 0);
      //      player_debug_message("rtp_recv return %d", retcode);
      // Run rtp periodic after each packet received or idle time.
      if (m_paused == 0 || m_stream_ondemand != 0)
	rtp_periodic();
    }
    
  }
  /*
   * When we're done, send a bye, close up rtp, and go home
   */
  rtp_end();
  return (0);
}

void CPlayerMedia::recv_callback (struct rtp *session, rtp_event *e)
{
  if (e == NULL) return;
  /*
   * If we're paused, just dump the packet.  Multicast case
   */
  if (m_paused != 0) {
    if (e->type == RX_RTP) {
      xfree(e->data);
      return;
    }
  }
#if DROP_PAKS
    if (e->type == RX_RTP && dropcount >= 50) {
      xfree((rtp_packet *)e->data);
      dropcount = 0;
      return;
    } else { 
      dropcount++;
    }
#endif
  if (m_rtp_byte_stream != NULL) {
    m_rtp_byte_stream->recv_callback(session, e);
    return;
  }
  switch (e->type) {
  case RX_RTP:
    /* regular rtp packet - add it to the queue */
    rtp_packet *rpak;

      
    rpak = (rtp_packet *)e->data;
    if (rpak->rtp_data_len == 0) {
      xfree(rpak);
    } else {
      rpak->pd.rtp_pd_timestamp = get_time_of_day();
      rpak->pd.rtp_pd_have_timestamp = true;
      add_rtp_packet_to_queue(rpak, &m_head, &m_tail, m_is_video ? "video" : "audio");
      m_rtp_queue_len++;
    }
    break;
  case RX_SR:
    rtcp_sr *srpak;
    srpak = (rtcp_sr *)e->data;

    m_rtcp_ntp_frac = srpak->ntp_frac;
    m_rtcp_ntp_sec = srpak->ntp_sec;
    m_rtcp_rtp_ts = srpak->rtp_ts;
    m_rtcp_received = 1;
    break;
  case RX_APP:
    free(e->data);
    break;
  default:
#if 0
    media_message(LOG_DEBUG, "Thread %u - Callback from rtp with %d %p", 
		  SDL_ThreadID(),e->type, e->data);
#endif
    break;
  }
}

int CPlayerMedia::determine_payload_type_from_rtp(void)
{
  uint8_t payload_type = m_head->rtp_pak_pt, temp;
  format_list_t *fmt;
  uint64_t tickpersec;

  fmt = m_media_info->fmt;
  while (fmt != NULL) {
    // rtp payloads are all numeric
    temp = atoi(fmt->fmt);
    if (temp == payload_type) {
      m_media_fmt = fmt;
      if (fmt->rtpmap != NULL) {
	tickpersec = fmt->rtpmap->clock_rate;
      } else {
	if (payload_type >= 96) {
	  media_message(LOG_ERR, "Media %s, rtp payload type of %u, no rtp map",
			m_media_info->media, payload_type);
	  return (FALSE);
	} else {
	  // generic payload type.  between 0 and 23 are audio - most
	  // are 8000
	  // all video (above 25) are 90000
	  tickpersec = 90000;
	  // this will handle the >= 0 case as well.
	  if (payload_type <= 23) {
	    tickpersec = 8000;
	    if (payload_type == 6) {
	      tickpersec = 16000;
	    } else if (payload_type == 10 || payload_type == 11) {
	      tickpersec = 44100;
	    } else if (payload_type == 14) 
	      tickpersec = 90000;
	  }
	}
      }

      create_rtp_byte_stream(payload_type,
			     tickpersec,
			     fmt);
      m_rtp_byte_stream->play((uint64_t)(m_play_start_time * 1000.0));
      m_byte_stream = m_rtp_byte_stream;
      if (!is_video()) {
	m_rtp_byte_stream->set_sync(m_parent);
      } else {
	m_parent->syncronize_rtp_bytestreams(NULL);
      }
#if 1
      media_message(LOG_DEBUG, "media %s - rtp tps %u ntp per rtp ",
			   m_media_info->media,
			   m_rtptime_tickpersec);
#endif

      return (TRUE);
    }
    fmt = fmt->next;
  }
  media_message(LOG_ERR, "Payload type %d not in format list for media %s", 
		payload_type, m_is_video ? "video" : "audio");
  return (FALSE);
}

/*
 * set up rtptime
 */
void CPlayerMedia::set_rtp_base_ts (uint32_t time)
{
  m_rtsp_base_ts_received = 1;
  m_rtp_base_ts = time;
  if (m_rtp_byte_stream != NULL) {
    m_rtp_byte_stream->set_rtp_base_ts(time);
  }
}

void CPlayerMedia::set_rtp_base_seq (uint16_t seq)
{
  m_rtsp_base_seq_received = 1; 
  m_rtp_base_seq = seq;
  if (m_rtp_byte_stream != NULL) {
    m_rtp_byte_stream->set_rtp_base_seq(seq);
  }
}

void CPlayerMedia::rtp_init_tcp (void) 
{
  connect_desc_t *cptr;
  double bw;

  if (find_rtcp_bandwidth_from_media(m_media_info, &bw) < 0) {
    bw = 5000.0;
  } 
  cptr = get_connect_desc_from_media(m_media_info);
  m_rtp_session = rtp_init_extern_net(m_source_addr == NULL ? 
				      cptr->conn_addr : m_source_addr,
				      m_our_port,
				      m_server_port,
				      cptr->ttl,
				      bw, // rtcp bandwidth ?
				      pdp_recv_callback,
				      pdp_rtcp_send_packet,
				      (uint8_t *)this);
  rtp_set_option(m_rtp_session, RTP_OPT_WEAK_VALIDATION, FALSE);
  rtp_set_option(m_rtp_session, RTP_OPT_PROMISC, TRUE);
  m_rtp_inited = 1;

}

void CPlayerMedia::create_rtp_byte_stream (uint8_t rtp_pt,
					   uint64_t tps,
					   format_list_t *fmt)
{
  int codec;
  rtp_check_return_t plugin_ret;
  rtp_plugin_t *rtp_plugin;

  rtp_plugin = NULL;
  plugin_ret = check_for_rtp_plugins(fmt, rtp_pt, &rtp_plugin);

  if (plugin_ret != RTP_PLUGIN_NO_MATCH) {
    switch (plugin_ret) {
    case RTP_PLUGIN_MATCH:
      player_debug_message("Starting rtp bytestream %s from plugin", 
			   rtp_plugin->name);
      m_rtp_byte_stream = new CPluginRtpByteStream(rtp_plugin,
						 fmt,
						 rtp_pt,
						 m_stream_ondemand,
						 tps,
						 &m_head,
						 &m_tail,
						 m_rtsp_base_seq_received,
						 m_rtp_base_seq,
						 m_rtsp_base_ts_received,
						 m_rtp_base_ts,
						 m_rtcp_received,
						 m_rtcp_ntp_frac,
						 m_rtcp_ntp_sec,
						 m_rtcp_rtp_ts);
      return;
    case RTP_PLUGIN_MATCH_USE_VIDEO_DEFAULT:
      // just fall through...
      break; 
    case RTP_PLUGIN_MATCH_USE_AUDIO_DEFAULT:
      m_rtp_byte_stream = 
	new CAudioRtpByteStream(rtp_pt, 
				fmt, 
				m_stream_ondemand,
				tps,
				&m_head,
				&m_tail,
				m_rtsp_base_seq_received,
				m_rtp_base_seq,
				m_rtsp_base_ts_received,
				m_rtp_base_ts,
				m_rtcp_received,
				m_rtcp_ntp_frac,
				m_rtcp_ntp_sec,
				m_rtcp_rtp_ts);
      if (m_rtp_byte_stream != NULL) {
	player_debug_message("Starting generic audio byte stream");
	return;
      }

    default:
      break;
    }
  } else {
    if (is_video() && (rtp_pt == 32)) {
      codec = VIDEO_MPEG12;
      m_rtp_byte_stream = new CMpeg3RtpByteStream(rtp_pt,
						  fmt, 
						m_stream_ondemand,
						tps,
						&m_head,
						&m_tail,
						m_rtsp_base_seq_received,
						m_rtp_base_seq,
						m_rtsp_base_ts_received,
						m_rtp_base_ts,
						m_rtcp_received,
						m_rtcp_ntp_frac,
						m_rtcp_ntp_sec,
						m_rtcp_rtp_ts);
      if (m_rtp_byte_stream != NULL) {
	return;
      }
  } else {
    if (rtp_pt == 14) {
      codec = MPEG4IP_AUDIO_MP3;
    } else if (rtp_pt <= 23) {
      codec = MPEG4IP_AUDIO_GENERIC;
    }  else {
      if (fmt->rtpmap == NULL) return;

      codec = lookup_audio_codec_by_name(fmt->rtpmap->encode_name);
      if (codec < 0) {
	codec = MPEG4IP_AUDIO_NONE; // fall through everything to generic
      }
    }
    switch (codec) {
    case MPEG4IP_AUDIO_MP3: {
      m_rtp_byte_stream = 
	new CAudioRtpByteStream(rtp_pt, fmt, 
				m_stream_ondemand,
				tps,
				&m_head,
				&m_tail,
				m_rtsp_base_seq_received,
				m_rtp_base_seq,
				m_rtsp_base_ts_received,
				m_rtp_base_ts,
				m_rtcp_received,
				m_rtcp_ntp_frac,
				m_rtcp_ntp_sec,
				m_rtcp_rtp_ts);
      if (m_rtp_byte_stream != NULL) {
	m_rtp_byte_stream->set_skip_on_advance(4);
	player_debug_message("Starting mp3 2250 audio byte stream");
	return;
      }
    }
      break;
    case MPEG4IP_AUDIO_MP3_ROBUST:
      m_rtp_byte_stream = 
	new CRfc3119RtpByteStream(rtp_pt, fmt, 
				m_stream_ondemand,
				tps,
				&m_head,
				&m_tail,
				m_rtsp_base_seq_received,
				m_rtp_base_seq,
				m_rtsp_base_ts_received,
				m_rtp_base_ts,
				m_rtcp_received,
				m_rtcp_ntp_frac,
				m_rtcp_ntp_sec,
				m_rtcp_rtp_ts);
      if (m_rtp_byte_stream != NULL) {
	player_debug_message("Starting mpa robust byte stream");
	return;
      }
      break;
    case MPEG4IP_AUDIO_GENERIC:
      m_rtp_byte_stream = 
	new CAudioRtpByteStream(rtp_pt, fmt, 
				m_stream_ondemand,
				tps,
				&m_head,
				&m_tail,
				m_rtsp_base_seq_received,
				m_rtp_base_seq,
				m_rtsp_base_ts_received,
				m_rtp_base_ts,
				m_rtcp_received,
				m_rtcp_ntp_frac,
				m_rtcp_ntp_sec,
				m_rtcp_rtp_ts);
      if (m_rtp_byte_stream != NULL) {
	player_debug_message("Starting generic audio byte stream");
	return;
      }
    default:
      break;
    }
  }
  m_rtp_byte_stream = new CRtpByteStream(fmt->media->media,
					 fmt, 
					 rtp_pt,
					 m_stream_ondemand,
					 tps,
					 &m_head,
					 &m_tail,
					 m_rtsp_base_seq_received,
					 m_rtp_base_seq,
					 m_rtsp_base_ts_received,
					 m_rtp_base_ts,
					 m_rtcp_received,
					 m_rtcp_ntp_frac,
					 m_rtcp_ntp_sec,
					 m_rtcp_rtp_ts);
  }
}

void CPlayerMedia::syncronize_rtp_bytestreams (rtcp_sync_t *sync)
{
  if (!is_video()) {
    player_error_message("Attempt to syncronize audio byte stream");
    return;
  }
  if (m_rtp_byte_stream != NULL) 
    m_rtp_byte_stream->syncronize(sync);
}

void CPlayerMedia::start_decoding (void)
{
  m_decode_msg_queue.send_message(MSG_START_DECODING, 
				  NULL, 
				  0, 
				  m_decode_thread_sem);
}

void CPlayerMedia::bytestream_primed (void)
{
  if (m_decode_thread_waiting != 0) {
    m_decode_thread_waiting = 0;
    SDL_SemPost(m_decode_thread_sem);
  }
}

void CPlayerMedia::wait_on_bytestream (void)
{
  m_decode_thread_waiting = 1;
#ifdef DEBUG_DECODE
  if (m_media_info)
    media_message(LOG_INFO, "decode thread %s waiting", m_media_info->media);
  else
    media_message(LOG_INFO, "decode thread waiting");
#endif
  SDL_SemWait(m_decode_thread_sem);
  m_decode_thread_waiting = 0;
} 

void CPlayerMedia::parse_decode_message (int &thread_stop, int &decoding)
{
  CMsg *newmsg;

  if ((newmsg = m_decode_msg_queue.get_message()) != NULL) {
#ifdef DEBUG_DECODE_MSGS
    media_message(LOG_DEBUG, "decode thread message %d",newmsg->get_value());
#endif
    switch (newmsg->get_value()) {
    case MSG_STOP_THREAD:
      thread_stop = 1;
      break;
    case MSG_PAUSE_SESSION:
      decoding = 0;
      if (m_video_sync != NULL) {
	m_video_sync->flush_decode_buffers();
      }
      if (m_audio_sync != NULL) {
	m_audio_sync->flush_decode_buffers();
      }
      break;
    case MSG_START_DECODING:
      if (m_video_sync != NULL) {
	m_video_sync->flush_decode_buffers();
      }
      if (m_audio_sync != NULL) {
	m_audio_sync->flush_decode_buffers();
      }
      decoding = 1;
      break;
    }
    delete newmsg;
  }
}

int CPlayerMedia::decode_thread (void) 
{
  //  uint32_t msec_per_frame = 0;
  int ret = 0;
  int thread_stop = 0, decoding = 0;
  uint32_t decode_skipped_frames = 0;
  uint64_t ourtime;
      // Tell bytestream we're starting the next frame - they'll give us
      // the time.
  uint8_t *frame_buffer;
  uint32_t frame_len;
  void *ud = NULL;
  
  uint32_t frames_decoded;
  uint64_t bytes_decoded;
  uint32_t frames_decoded_last_sec;
  uint64_t bytes_decoded_last_sec;
  uint64_t current_second;
  uint32_t total_secs;
  uint32_t last_div = 0;

  total_secs = 0;
  frames_decoded = 0;
  bytes_decoded = 0;
  frames_decoded_last_sec = 0;
  bytes_decoded_last_sec = 0;
  current_second = 0;

  while (thread_stop == 0) {
    // waiting here for decoding or thread stop
    ret = SDL_SemWait(m_decode_thread_sem);
#ifdef DEBUG_DECODE
    media_message(LOG_DEBUG, "%s Decode thread awake",
		  is_video() ? "video" : "audio");
#endif
    parse_decode_message(thread_stop, decoding);

    if (decoding == 1) {
      // We've been told to start decoding - if we don't have a codec, 
      // create one
      if (is_video()) {
	if (m_video_sync == NULL) {
	  m_video_sync = m_parent->set_up_video_sync();
	}
	m_video_sync->set_wait_sem(m_decode_thread_sem);
      } else {
	if (m_audio_sync == NULL) {
	  m_audio_sync = m_parent->set_up_audio_sync();
	}
	m_audio_sync->set_wait_sem(m_decode_thread_sem);
      }
      if (m_plugin == NULL) {
	if (is_video()) {
	  m_plugin = check_for_video_codec(NULL,
					   m_media_fmt,
					   -1,
					   -1,
					   m_user_data,
					   m_user_data_size);
	  if (m_plugin != NULL) {
	    m_plugin_data = (m_plugin->vc_create)(NULL, // must figure from sdp
						  -1,
						  -1,
						  m_media_fmt,
						  m_video_info,
						  m_user_data,
						  m_user_data_size,
						  get_video_vft(),
						  m_video_sync);
	    if (m_plugin_data == NULL) {
	      m_plugin = NULL;
	    } else {
	      media_message(LOG_DEBUG, "Starting %s codec from decode thread",
			    m_plugin->c_name);
	    }
	  }
	} else {
	  m_plugin = check_for_audio_codec(NULL,
					   m_media_fmt,
					   -1, 
					   -1, 
					   m_user_data,
					   m_user_data_size);
	  if (m_plugin != NULL) {
	    m_plugin_data = (m_plugin->ac_create)(NULL, 
						  -1,
						  -1,
						  m_media_fmt,
						  m_audio_info,
						  m_user_data,
						  m_user_data_size,
						  get_audio_vft(),
						  m_audio_sync);
	    if (m_plugin_data == NULL) {
	      m_plugin = NULL;
	    } else {
	      media_message(LOG_DEBUG, "Starting %s codec from decode thread",
			    m_plugin->c_name);
	    }
	  }
	}
      }
      if (m_plugin != NULL) {
	m_plugin->c_do_pause(m_plugin_data);
      } else {
	while (thread_stop == 0 && decoding) {
	  SDL_Delay(100);
	  if (m_rtp_byte_stream) {
	    m_rtp_byte_stream->flush_rtp_packets();
	  }
	  parse_decode_message(thread_stop, decoding);
	}
      }
    }
    /*
     * this is our main decode loop
     */
#ifdef DEBUG_DECODE
    media_message(LOG_DEBUG, "%s Into decode loop",
		  is_video() ? "video" : "audio");
#endif
    frames_decoded_last_sec = 0;
    bytes_decoded_last_sec = 0;
    current_second = 0;
    while ((thread_stop == 0) && decoding) {
      parse_decode_message(thread_stop, decoding);
      if (thread_stop != 0)
	continue;
      if (decoding == 0) {
	m_plugin->c_do_pause(m_plugin_data);
	continue;
      }
      if (m_byte_stream->eof()) {
	media_message(LOG_INFO, "%s hit eof", m_is_video ? "video" : "audio");
	if (m_audio_sync) m_audio_sync->set_eof();
	if (m_video_sync) m_video_sync->set_eof();
	decoding = 0;
	continue;
      }
      if (m_byte_stream->have_no_data()) {
	// Indicate that we're waiting, and wait for a message from RTP
	// task.
	wait_on_bytestream();
	continue;
      }

      frame_buffer = NULL;
      ourtime = m_byte_stream->start_next_frame(&frame_buffer, 
						&frame_len,
						&ud);
      /*
       * If we're decoding video, see if we're playing - if so, check
       * if we've fallen significantly behind the audio
       */
      if (is_video() &&
	  (m_parent->get_session_state() == SESSION_PLAYING)) {
	uint64_t current_time = m_parent->get_playing_time();
	if (current_time >= ourtime) {
#if 1
	  media_message(LOG_INFO, "Candidate for skip decode "U64" our "U64, 
			       current_time, ourtime);
#endif
	  // If the bytestream can skip ahead, let's do so
	  if (m_byte_stream->can_skip_frame() != 0) {
	    int ret;
	    int hassync;
	    int count;
	    current_time += 200; 
	    count = 0;
	    // Skip up to the current time + 200 msec
	    ud = NULL;
	    do {
	      if (ud != NULL) free(ud);
	      ret = m_byte_stream->skip_next_frame(&ourtime, &hassync,
						   &frame_buffer, &frame_len,
						   &ud);
	      decode_skipped_frames++;
	    } while (ret != 0 &&
		     !m_byte_stream->eof() && 
		     current_time > ourtime);
	    if (m_byte_stream->eof() || ret == 0) continue;
#if 1
	    media_message(LOG_INFO, "Skipped ahead "U64 " to "U64, 
			  current_time - 200, ourtime);
#endif
	    /*
	     * Ooh - fun - try to match to the next sync value - if not, 
	     * 15 frames
	     */
	    do {
	      if (ud != NULL) free(ud);
	      ret = m_byte_stream->skip_next_frame(&ourtime, &hassync,
						   &frame_buffer, &frame_len,
						   &ud);
	      if (hassync < 0) {
		uint64_t diff = ourtime - current_time;
		if (diff > (2 * C_64)) {
		  hassync = 1;
		}
	      }
	      decode_skipped_frames++;
	      count++;
	    } while (ret != 0 &&
		     hassync <= 0 &&
		     count < 30 &&
		     !m_byte_stream->eof());
	    if (m_byte_stream->eof() || ret == 0) continue;
#ifdef DEBUG_DECODE
	    media_message(LOG_INFO, "Matched ahead - count %d, sync %d time "U64,
				 count, hassync, ourtime);
#endif
	  }
	}
      }
#ifdef DEBUG_DECODE
      media_message(LOG_DEBUG, "Decoding %c frame " U64, 
		    m_is_video ? 'v' : 'a', ourtime);
#endif
      if (frame_buffer != NULL && frame_len != 0) {
	int sync_frame;
	ret = m_plugin->c_decode_frame(m_plugin_data,
				       ourtime,
				       m_streaming != 0,
				       &sync_frame,
				       frame_buffer, 
				       frame_len,
				       ud);
#ifdef DEBUG_DECODE
	media_message(LOG_DEBUG, "Decoding %c frame return %d", 
		      m_is_video ? 'v' : 'a', ret);
#endif
	if (ret > 0) {
	  frames_decoded++;
	  m_byte_stream->used_bytes_for_frame(ret);
	  bytes_decoded += ret;
	  last_div = ourtime % 1000;
	  if ((ourtime / 1000) > current_second) {
	    if (frames_decoded_last_sec != 0) {
#if 0
	      media_message(LOG_DEBUG, "%s - Second "U64", frames %d bytes "U64,
			    m_is_video ? "video" : "audio", 
			    current_second,
			    frames_decoded_last_sec,
			    bytes_decoded_last_sec);
#endif
	    }
	    current_second = ourtime / 1000;
	    total_secs++;
	    frames_decoded_last_sec = 1;
	    bytes_decoded_last_sec = ret;
	  } else {
	    frames_decoded_last_sec++;
	    bytes_decoded_last_sec += ret;
	  }
	} else {
	  m_byte_stream->used_bytes_for_frame(frame_len);
	}

      }
    }
    // calculate frame rate for session
  }
  if (m_is_video)
    media_message(LOG_NOTICE, "Video decoder skipped %u frames", 
		  decode_skipped_frames);
  if (total_secs != 0) {
    double fps, bps;
    double secs;
    secs = last_div;
    secs /= 1000.0;
    secs += total_secs;

    fps = frames_decoded;
    fps /= secs;
    bps = UINT64_TO_DOUBLE(bytes_decoded);
    bps *= 8.0 / secs;
    media_message(LOG_NOTICE, "%s - bytes "U64", seconds %g, fps %g bps "U64,
		  m_is_video ? "video" : "audio", 
		  bytes_decoded, secs, 
		  fps, bytes_decoded * 8 / total_secs);
  }
  if (m_plugin) {
    m_plugin->c_close(m_plugin_data);
    m_plugin_data = NULL;
  }
  return (0);
}
