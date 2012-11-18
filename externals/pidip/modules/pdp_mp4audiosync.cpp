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
 * audio.cpp provides an interface (CPDPAudioSync) between the codec and
 * the SDL audio APIs.
 */
#include <stdlib.h>
#include <string.h>
#include "pdp_mp4playersession.h"
#include "pdp_mp4audiosync.h"
#include "player_util.h"
#include "our_config_file.h"
#include "m_pd.h"

static void pdp_audio_callback (void *userdata, Uint8 *stream, int len)
{
  CPDPAudioSync *a = (CPDPAudioSync *)userdata;
  // post( "pdp_mp4audiosync : audio callback" );
  a->audio_callback(stream, len);
}

CPDPAudioSync::CPDPAudioSync (CPlayerSession *psptr, t_pdp_mp4player *pdp_father) : CAudioSync(psptr)
{
  m_config_set = 0;
  m_audio_initialized = 1;
  m_audio_paused = 0;
  m_resync_required = 0;
  m_dont_fill = 0;
  m_play_time = 0         ;
  m_buffer_latency = 0;
  m_first_time = 1;
  m_convert_buffer = NULL;
  m_father = pdp_father;
  post( "pdp_mp4audiosync : created audio sync" );
}

CPDPAudioSync::~CPDPAudioSync (void)
{
  if (m_sample_buffer[0] != NULL)
  {
    free(m_sample_buffer[0]);
  }
  m_sample_buffer[0] = NULL;
  CHECK_AND_FREE(m_convert_buffer);
}

void CPDPAudioSync::set_config (int freq, 
			     int channels, 
			     int format, 
			     uint32_t sample_size) 
{
  if (m_config_set != 0) 
    return;
  
  if (format == AUDIO_U8 || format == AUDIO_S8)
    m_bytes_per_sample = 1;
  else
    m_bytes_per_sample = 2;

  if (sample_size == 0) {
    int temp;
    temp = freq;
    while ((temp & 0x1) == 0) temp >>= 1;
    sample_size = temp;
    while (sample_size < 1024) sample_size *= 2;
    while (((sample_size * 1000) % freq) != 0) sample_size *= 2;
  } 
  
  m_buffer_size = channels * sample_size * m_bytes_per_sample;

  m_buffer_filled[0] = 0;
  m_sample_buffer[0] = (uint8_t *)malloc(2 * m_buffer_size);

  m_freq = freq;
  m_channels = channels;
  m_format = format;
  if (m_format == AUDIO_U8) {
    m_silence = 0x80;
  } else {
    m_silence = 0x00;
  }
  m_config_set = 1;
  m_msec_per_frame = (sample_size * 1000) / m_freq;
  post("pdp_mp4audiosync : buffer size %d msec per frame %d", m_buffer_size, m_msec_per_frame);
};

uint8_t *CPDPAudioSync::get_audio_buffer (void)
{
  int ret;
  int locked = 0;
  if (m_dont_fill == 1) {
    return (NULL);
  }

  ret = m_buffer_filled[0];
  if (ret == 1) 
  {
    post("pdp_mp4audiosync : no buffer");
    return (NULL);
  }
  // post("pdp_mp4audiosync : get_audio_buffer : return %x", m_sample_buffer[0]);
  return (m_sample_buffer[0]);
}

void CPDPAudioSync::load_audio_buffer (uint8_t *from, 
				       uint32_t bytes, 
				       uint64_t ts, 
				       int resync)
{
  uint8_t *to;
  uint32_t copied;
  copied = 0;

  post( "pdp_mp4audiosync : load audio buffer : length=%d", bytes );

  to = get_audio_buffer();
  if (to == NULL) 
  {
    return;
  }
  int copy;
  uint32_t left;

  bytes = MIN(m_buffer_size, bytes);
  memcpy(to, from, bytes);

  return;
}

void CPDPAudioSync::filled_audio_buffer (uint64_t ts, int resync)
{
  // post( "pdp_mp4audiosync : filled audio buffer" );
  // if (resync) m_psptr->wake_sync_thread();

  if (  m_father->x_audio )
  {
    // copy the buffer filled by the codec towards pdp
    if ( (m_father->x_audioin_position*sizeof(short)+m_buffer_size) < (4*MAX_AUDIO_PACKET_SIZE*sizeof(short)) )
    {
      memcpy( m_father->x_audio_in+m_father->x_audioin_position, m_sample_buffer[0], m_buffer_size );
      m_father->x_audioin_position+=(m_buffer_size/sizeof(short));
      // post( "pdp_mp4audiosync : filled_audio_buffer : copied %d PCM samples : audio in : %d : resync : %d", 
      //                           m_buffer_size/sizeof(short), m_father->x_audioin_position, resync );
      if ( ( m_father->x_audioin_position > DEFAULT_CHANNELS*m_father->x_blocksize ) 
           && (!m_father->x_audioon) )
      {
        m_father->x_audioon = 1;
        // post( "pdp_mp4audiosync : audio on" );
      }
    }
    else
    {
      post( "pdp_mp4audiosync : filled_audio_buffer : skipped buffer : (in : %d)", 
             m_father->x_audioin_position );
    }
  }
  
  return;
}

void CPDPAudioSync::set_eof(void) 
{ 
  uint8_t *to;

  to = get_audio_buffer();
  CAudioSync::set_eof();
}

int CPDPAudioSync::initialize_audio (int have_audio) 
{
  m_audio_initialized = 1;
  return (1);
}

int CPDPAudioSync::is_audio_ready (uint64_t &disptime)
{
  return (1);
}

uint64_t CPDPAudioSync::check_audio_sync (uint64_t current_time, int &have_eof)
{
  return (0);
}

void CPDPAudioSync::audio_callback (Uint8 *stream, int ilen)
{
  int freed_buffer = 0;
  uint32_t bufferBytes = (uint32_t)ilen;
  uint64_t this_time;
  int delay = 0;
  int playtime;

  post( "pdp_mp4audiosync : audio callback" );

}

void CPDPAudioSync::play_audio (void)
{
  m_first_time = 1;
  m_audio_paused = 0;
  m_play_sample_index = 0;
}

void CPDPAudioSync::flush_sync_buffers (void)
{
  post( "pdp_mp4audiosync : flush sync buffer" );
  clear_eof();
}

void CPDPAudioSync::flush_decode_buffers (void)
{
  post( "pdp_mp4audiosync : flush decode buffer" );
  m_buffer_filled[0] = 0;
}

void CPDPAudioSync::set_volume (int volume)
{
  m_volume = (volume * SDL_MIX_MAXVOLUME)/100;
}

void CPDPAudioSync::audio_convert_data (void *from, uint32_t samples)
{
  if (m_obtained.format == AUDIO_U8 || m_obtained.format == AUDIO_S8) {
    // bytewise - easy
    int8_t *src, *dst;
    src = (int8_t *) from;
    dst = (int8_t *) m_convert_buffer;
    if (m_channels == 2) {
      // we got 1, wanted 2
      for (uint32_t ix = 0; ix < samples; ix++) {
	int16_t sum = *src++;
	sum += *src++;
	sum /= 2;
	if (sum < -128) sum = -128;
	else if (sum > 128) sum = 128;
	*dst++ = sum & 0xff;
      }
    } else {
      // we got 2, wanted 1
      for (uint32_t ix = 0; ix < samples; ix++) {
	*dst++ = *src;
	*dst++ = *src++;
      }
    }
  } else {
    int16_t *src, *dst;
    src = (int16_t *) from;
    dst = (int16_t *) m_convert_buffer;
    samples /= 2;
    if (m_channels == 1) {
      // 1 channel to 2
      for (uint32_t ix = 0; ix < samples; ix++) {
	*dst++ = *src;
	*dst++ = *src;
	src++;
      }
    } else {
      // 2 channels to 1
      for (uint32_t ix = 0; ix < samples; ix++) {
	int32_t sum = *src++;
	sum += *src++;
	sum /= 2;
	if (sum < -32768) sum = -32768;
	else if (sum > 32767) sum = 32767;
	*dst++ = sum & 0xffff;
      }
    }

  }
}

static void pdp_audio_config (void *ifptr, int freq, 
			    int chans, int format, uint32_t max_buffer_size)
{
  ((CPDPAudioSync *)ifptr)->set_config(freq,
				    chans,
				    format,
				    max_buffer_size);
}

static uint8_t *pdp_get_audio_buffer (void *ifptr)
{
  return ((CPDPAudioSync *)ifptr)->get_audio_buffer();
}

static void pdp_filled_audio_buffer (void *ifptr,
				   uint64_t ts,
				   int resync_req)
{
  ((CPDPAudioSync *)ifptr)->filled_audio_buffer(ts, 
				   resync_req);
}

static void pdp_load_audio_buffer (void *ifptr, 
				     uint8_t *from, 
				     uint32_t bytes, 
				     uint64_t ts, 
				     int resync)
{
  ((CPDPAudioSync *)ifptr)->load_audio_buffer(from,
					      bytes,
					      ts, 
					      resync);
}
  
audio_vft_t audio_vft = {
  message,
  pdp_audio_config,
  pdp_get_audio_buffer,
  pdp_filled_audio_buffer,
  pdp_load_audio_buffer
};

audio_vft_t *get_audio_vft (void)
{
  return &audio_vft;
}

CPDPAudioSync *pdp_create_audio_sync (CPlayerSession *psptr, t_pdp_mp4player *pdp_father)
{
  return new CPDPAudioSync(psptr, pdp_father);
}

int do_we_have_audio (void) 
{
  return 1;
}
