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
 *		Dave Mackie		dmackie@cisco.com
 *		Bill May 		wmay@cisco.com
 *
 * Adapted for PD/PDP by Yves Degoyon (ydegoyon@free.fr)
 */

#ifndef __PDP_MP4AUDIOSOURCE__
#define __PDP_MP4AUDIOSOURCE__

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "media_source.h"
#include "audio_encoder.h"

class CPDPAudioSource : public CMediaSource {
 public:
  CPDPAudioSource(CLiveConfig *pConfig);

  ~CPDPAudioSource() {
    free(m_pcmFrameBuffer);
  }

  bool IsDone() {
    return false;
  }

  float GetProgress() {
    return 0.0;	
  }

  void CPDPAudioSource::DoStart();

  void CPDPAudioSource::DoStop();

  void ProcessAudio(u_int8_t* pcmBuffer, u_int32_t pcmBufferSize);

 protected:
  int ThreadMain();

  bool Init();


 protected:
  int           m_maxPasses;
  Timestamp     m_prevTimestamp;
  int           m_audioOssMaxBufferSize;
  int           m_audioOssMaxBufferFrames;
  Timestamp     *m_timestampOverflowArray;
  size_t        m_timestampOverflowArrayIndex;
  u_int8_t*     m_pcmFrameBuffer;
  u_int32_t     m_pcmFrameSize;
  uint32_t      m_channelsConfigured;
};


#endif /* __PDP_MP4AUDIOSOURCE__ */
