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
 * Copyright (C) Cisco Systems Inc. 2000-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 *
 * Adapted for PD/PDP by Yves Degoyon (ydegoyon@free.fr)
 */

#include "m_pd.h"

#ifndef debug_message
#define debug_message post
#endif
#include "pdp_mp4audiosource.h"

//#define DEBUG_TIMESTAMPS 1

CPDPAudioSource::CPDPAudioSource(CLiveConfig *pConfig) : CMediaSource() 
{
  SetConfig(pConfig);

  m_pcmFrameBuffer = NULL;
  m_prevTimestamp = 0;
  m_timestampOverflowArray = NULL;
  m_timestampOverflowArrayIndex = 0;
  m_audioOssMaxBufferSize = 0;
}

int CPDPAudioSource::ThreadMain(void) 
{
  // just a stub, we don't use the threaded mode
  return 0;
}

void CPDPAudioSource::DoStart()
{
  if (m_source) {
    return;
  }

  if (!Init()) {
    return;
  }

  m_source = true;
}

void CPDPAudioSource::DoStop()
{
  if (!m_source) {
    return;
  }

  CMediaSource::DoStopAudio();

  CHECK_AND_FREE(m_timestampOverflowArray);
  free(m_pcmFrameBuffer);
  m_pcmFrameBuffer = NULL;

  m_source = false;
}

bool CPDPAudioSource::Init(void)
{
  bool rc = InitAudio(true);
  if (!rc) {
    return false;
  }

  m_channelsConfigured = m_pConfig->GetIntegerValue(CONFIG_AUDIO_CHANNELS);
  
  debug_message("pdp_mp4live~ : init audio : (m_audioDstChannels=%d m_audioDstSampleRate=%d )", 
                           m_audioDstChannels, m_audioDstSampleRate);

  rc = SetAudioSrc(PCMAUDIOFRAME,
                   m_channelsConfigured,
                   m_pConfig->GetIntegerValue(CONFIG_AUDIO_SAMPLE_RATE));

  if (!rc) {
    return false;
  }

  debug_message("pdp_mp4live~ : set audio src : (m_audioDstSamplesPerFrame=%d m_audioSrcChannels=%d)", 
                           m_audioDstSamplesPerFrame, m_audioSrcChannels);

  // for live capture we can match the source to the destination
  m_audioSrcSamplesPerFrame = m_audioDstSamplesPerFrame;
  m_pcmFrameSize = m_audioSrcSamplesPerFrame * m_audioSrcChannels * sizeof(u_int16_t);

  if (m_audioOssMaxBufferSize > 0) {
    size_t array_size;
    m_audioOssMaxBufferFrames = m_audioOssMaxBufferSize / m_pcmFrameSize;
    array_size = m_audioOssMaxBufferFrames * sizeof(*m_timestampOverflowArray);
    m_timestampOverflowArray = (Timestamp *)malloc(array_size);
    memset(m_timestampOverflowArray, 0, array_size);
  }
    
  m_pcmFrameBuffer = (u_int8_t*)malloc(m_pcmFrameSize);
  if (!m_pcmFrameBuffer) {
    goto init_failure;
  }

  // maximum number of passes in ProcessAudio, approx 1 sec.
  m_maxPasses = m_audioSrcSampleRate / m_audioSrcSamplesPerFrame;

  debug_message("pdp_mp4live~ : audio source initialization done : ( frame size=%d )", m_pcmFrameSize );

  return true;

 init_failure:
  debug_message("pdp_mp4live~ : audio initialization failed");

  free(m_pcmFrameBuffer);
  m_pcmFrameBuffer = NULL;

  return false;
}

void CPDPAudioSource::ProcessAudio(u_int8_t* pcmBuffer, u_int32_t pcmBufferSize)
{
  Timestamp currentTime = GetTimestamp();
  Timestamp timestamp;

    if ( pcmBufferSize > m_pcmFrameSize )
    {
       debug_message( "pdp_mp4live~ : too many audio samples : %d should be %d",
                      pcmBufferSize, m_pcmFrameSize );
       memcpy( m_pcmFrameBuffer, pcmBuffer, m_pcmFrameSize );
    }
    else if ( pcmBufferSize < m_pcmFrameSize )
    {
       debug_message( "pdp_mp4live~ : too little audio samples : %d should be %d",
                      pcmBufferSize, m_pcmFrameSize );
       memcpy( m_pcmFrameBuffer, pcmBuffer, pcmBufferSize );
    }
    else
    {
       memcpy( m_pcmFrameBuffer, pcmBuffer, pcmBufferSize );
    }

    timestamp = currentTime - SrcSamplesToTicks(SrcBytesToSamples(pcmBufferSize));

#ifdef DEBUG_TIMESTAMPS
    debug_message("pdp_mp4live~ : bytes=%d t=%llu timestamp=%llu delta=%llu",
                  pcmBufferSize, currentTime, timestamp, timestamp - m_prevTimestamp);
#endif

    m_prevTimestamp = timestamp;

    ProcessAudioFrame(m_pcmFrameBuffer, m_pcmFrameSize, timestamp, false);
}
