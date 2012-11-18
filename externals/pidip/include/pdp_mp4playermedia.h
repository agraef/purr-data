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
 * player_media.h - provides CPlayerMedia class, which defines the
 * interface to a particular media steam.
 */

#ifndef __PLAYER_MEDIA_H__
#define __PLAYER_MEDIA_H__

#include <SDL.h>
#include <SDL_thread.h>
#include <sdp/sdp.h>
#include <rtsp/rtsp_client.h>
#include <rtp/rtp.h>
#include "our_bytestream.h"
#include "our_msg_queue.h"
#include "codec_plugin.h"

class CPlayerSession;
class CPDPAudioSync;
class CPDPVideoSync;
class C2ConsecIpPort;
class COurInByteStream;
class CRtpByteStreamBase;

class CPlayerMedia {
 public:
  CPlayerMedia(CPlayerSession *p);
  ~CPlayerMedia();
  /* API routine - create - for RTP stream */
  int create_streaming(media_desc_t *sdp_media,
		       char *errmsg,
		       uint32_t errlen,
		       int on_demand,
		       int use_rtsp,
		       int media_number_in_session);
  /* API routine - create - where we provide the bytestream */
  int create(COurInByteStream *b, 
	     int is_video, 
	     char *errmsg = NULL, 
	     uint32_t errlen = 0, 
	     int streaming = 0);
  /* API routine - play, pause */
  int do_play(double start_time_offset, char *errmsg, uint32_t errlen);
  int do_pause(void);
  int is_video(void) { return (m_is_video); };
  double get_max_playtime(void);
  /* API routine - interface for decoding start/continue */
  void start_decoding(void);
  void bytestream_primed(void); 
  /* API routine - ip port information */
  uint16_t get_our_port (void) { return m_our_port; };
  void set_server_port (uint16_t port) { m_server_port = port; };
  uint16_t get_server_port (void) { return m_server_port; };

  media_desc_t *get_sdp_media_desc (void) { return m_media_info; };
  void set_source_addr (char *s)
    {
      if (m_source_addr != NULL) free(m_source_addr);
      m_source_addr = s;
    }
  const char *get_source_addr(void);
  CPlayerMedia *get_next (void) { return m_next; };
  void set_next (CPlayerMedia *newone) { m_next = newone; };
  int decode_thread(void);

  /* Public RTP routines  - receive thread, callback, and routines to
   * pass information from rtsp to rtp byte stream
   */
  int recv_thread(void);
  void recv_callback(struct rtp *session, rtp_event *e);
  void set_rtp_ssrc (uint32_t ssrc)
    { m_rtp_ssrc = ssrc; m_rtp_ssrc_set = TRUE;};
  void set_rtp_base_ts(uint32_t time);
  void set_rtp_base_seq(uint16_t seq);
  
  void set_video_sync(CPDPVideoSync *p) {m_video_sync = p;};
  void set_audio_sync(CPDPAudioSync *p) {m_audio_sync = p;};

  const video_info_t *get_video_info (void) { return m_video_info; };
  const audio_info_t *get_audio_info (void) { return m_audio_info; };

  int create_video_plugin(const codec_plugin_t *p,
			  const char *compressor, 
			  int profile, 
			  int type, 
			  format_list_t *sdp_media,
			  video_info_t *video,
			  const uint8_t *user_data,
			  uint32_t userdata_size);
  int create_audio_plugin(const codec_plugin_t *p,
			  const char *compressor, 
			  int profile, 
			  int type, 
			  format_list_t *sdp_media,
			  audio_info_t *audio,
			  const uint8_t *user_data,
			  uint32_t userdata_size);
  void set_plugin_data (const codec_plugin_t *p, 
			codec_data_t *d, 
			video_vft_t *v, 
			audio_vft_t *a);
  int get_plugin_status(char *buffer, uint32_t buflen);
  void set_user_data (const uint8_t *udata, int length) {
    m_user_data = udata;
    m_user_data_size = length;
  }
  rtsp_session_t *get_rtsp_session(void) { return m_rtsp_session; };
  void rtp_init_tcp(void);
  void rtp_periodic(void);
  void rtp_start(void);
  void rtp_end(void);
  int rtp_receive_packet(unsigned char interleaved, struct rtp_packet *, int len);
  int rtcp_send_packet(uint8_t *buffer, int buflen);
  int get_rtp_media_number (void) { return m_rtp_media_number_in_session; };
  void syncronize_rtp_bytestreams(rtcp_sync_t *sync);
 private:
  int create_common(int is_video, char *errmsg, uint32_t errlen);
  void wait_on_bytestream(void);
  int m_streaming;
  int m_is_video;
  int m_paused;
  CPlayerMedia *m_next;
  CPlayerSession *m_parent;
  media_desc_t *m_media_info;
  format_list_t *m_media_fmt;        // format currently running.
  rtsp_session_t *m_rtsp_session;
  C2ConsecIpPort *m_ports;
  in_port_t m_our_port;
  in_port_t m_server_port;
  char *m_source_addr;

  time_t m_start_time;
  int m_stream_ondemand;
  int m_sync_time_set;
  uint64_t m_sync_time_offset;
  uint32_t m_rtptime_tickpersec;
  double m_play_start_time;
  // Receive thread variables
  SDL_Thread *m_recv_thread;

  /*************************************************************************
   * RTP variables - used to pass info to the bytestream
   *************************************************************************/
  int m_rtp_ondemand;
  int m_rtp_use_rtsp;
  int m_rtp_media_number_in_session;
  int m_rtp_buffering;
  struct rtp *m_rtp_session;
  CRtpByteStreamBase *m_rtp_byte_stream;
  CMsgQueue m_rtp_msg_queue;

  rtp_packet *m_head, *m_tail; 
  uint32_t m_rtp_queue_len;
  
  // from rtsp...
  int m_rtp_ssrc_set;
  uint32_t m_rtp_ssrc;
  int m_rtsp_base_ts_received;
  uint32_t m_rtp_base_ts;
  int m_rtsp_base_seq_received;
  uint16_t m_rtp_base_seq;

  int determine_payload_type_from_rtp(void);
  void create_rtp_byte_stream(uint8_t payload, uint64_t tps, format_list_t *fmt);
  void clear_rtp_packets(void);

  // from rtcp, for broadcast, in case we get an RTCP before we determine
  // the payload type
  uint32_t m_rtcp_ntp_frac;
  uint32_t m_rtcp_ntp_sec;
  uint32_t m_rtcp_rtp_ts;
  int m_rtcp_received;

  volatile int m_rtp_inited;

  /*************************************************************************
   * Decoder thread variables
   *************************************************************************/
  SDL_Thread *m_decode_thread;
  volatile int m_decode_thread_waiting;
  SDL_sem *m_decode_thread_sem;

  const codec_plugin_t *m_plugin;
  codec_data_t *m_plugin_data;
  
  // State change variable
  CMsgQueue m_decode_msg_queue;
  // Private routines
  int process_rtsp_transport(char *transport);
  CPDPAudioSync *m_audio_sync;
  CPDPVideoSync *m_video_sync;
  void parse_decode_message(int &thread_stop, int &decoding);
  COurInByteStream *m_byte_stream;
  video_info_t *m_video_info;
  audio_info_t *m_audio_info;

  const uint8_t *m_user_data;
  int m_user_data_size;

};

int pdp_process_rtsp_rtpinfo(char *rtpinfo, CPlayerSession *session, CPlayerMedia *media);

extern audio_vft_t audio_vft;
extern video_vft_t video_vft;

#define media_message(loglevel, fmt...) message(loglevel, "media", fmt)

#endif
