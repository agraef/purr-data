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
 *        Dave Mackie        dmackie@cisco.com
 *        Bill May         wmay@cisco.com
 *
 * Adapted for PD/PDP by Yves Degoyon (ydegoyon@free.fr)
 */

#ifndef debug_message
#define debug_message post 
#endif

#include <sys/mman.h>

#include "m_pd.h"
#include "pdp_mp4videosource.h"
#include "video_util_rgb.h"

//#define DEBUG_TIMESTAMPS 1

int CPDPVideoSource::ThreadMain(void)
{
   // just a stub, the threaded mode is not used ( never called )
   return 0;
}

void CPDPVideoSource::DoStart(void)
{
  if (m_source) return;
  if (!Init()) return;
  m_source = true;
}

void CPDPVideoSource::DoStop(void)
{
  if (!m_source) return;
  DoStopVideo();
  m_source = false;
}

bool CPDPVideoSource::Init(void)
{

    if (m_pConfig->GetIntegerValue(CONFIG_VIDEO_SIGNAL) == VIDEO_SIGNAL_NTSC) 
    {
      m_videoSrcFrameRate = VIDEO_NTSC_FRAME_RATE;
    } 
    else 
    {
      m_videoSrcFrameRate = VIDEO_PAL_FRAME_RATE;
    }

    m_videoMbuf.frames = 2;
    m_cacheTimestamp = false;
    m_videoFrameMap = (struct video_mmap*) malloc(m_videoMbuf.frames * sizeof(struct video_mmap));
    if ( !m_videoFrameMap ) 
    {
      post("pdp_mp4live~ : video source init : failed to allocate enough memory");
      return false;
    }

    m_videoFrameMapFrame = (uint64_t *)malloc(m_videoMbuf.frames * sizeof(uint64_t));
    m_videoFrameMapTimestamp = (uint64_t *)malloc(m_videoMbuf.frames * sizeof(Timestamp));
    m_captureHead = 0;
    m_encodeHead = -1;

    m_videoFrames = 0;
    m_videoSrcFrameDuration = (Duration)(((float)TimestampTicks / m_videoSrcFrameRate) + 0.5);
    for (int i = 0; i < m_videoMbuf.frames; i++) 
    {
       // initialize frame map
       m_videoFrameMap[i].frame = i;
       m_videoFrameMap[i].width = m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
       m_videoFrameMap[i].height = m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
       m_videoFrameMap[i].format = VIDEO_PALETTE_YUV420P;

       if (i == 0) 
       {
         m_videoCaptureStartTimestamp = GetTimestamp();
       }
       m_lastVideoFrameMapFrameLoaded = m_videoFrameMapFrame[i] = i;
       m_lastVideoFrameMapTimestampLoaded =
       m_videoFrameMapTimestamp[i] = CalculateVideoTimestampFromFrames(i);
    }

    m_pConfig->CalculateVideoFrameSize();

    InitVideo(
        (m_pConfig->m_videoNeedRgbToYuv ?
            RGBVIDEOFRAME :
            YUVVIDEOFRAME),
        true);

    SetVideoSrcSize(
        m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH),
        m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT),
        m_pConfig->GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH),
        true);

    m_maxPasses = (u_int8_t)(m_videoSrcFrameRate + 0.5);

    return true;
}

int8_t CPDPVideoSource::StartTimeStamp(Timestamp &frameTimestamp)
{
    if (m_cacheTimestamp)
      frameTimestamp = m_videoFrameMapTimestamp[m_captureHead];
    else
      frameTimestamp = GetTimestamp();

    int8_t capturedFrame = m_captureHead;
    m_captureHead = (m_captureHead + 1) % m_videoMbuf.frames;

    return capturedFrame;
}

bool CPDPVideoSource::EndTimeStamp(int8_t frameNumber)
{
  Timestamp calc = GetTimestamp();

  if (calc > m_videoSrcFrameDuration + m_lastVideoFrameMapTimestampLoaded) {
#ifdef DEBUG_TIMESTAMPS
    debug_message("pdp_mp4live~ : video frame delay past end of buffer - time is %llu should be %llu",
          calc,
          m_videoSrcFrameDuration + m_lastVideoFrameMapTimestampLoaded);
#endif
    m_videoCaptureStartTimestamp = calc;
    m_videoFrameMapFrame[frameNumber] = 0;
    m_videoFrameMapTimestamp[frameNumber] = calc;
  } else {
    m_videoFrameMapFrame[frameNumber] = m_lastVideoFrameMapFrameLoaded + 1;
    m_videoFrameMapTimestamp[frameNumber] =
      CalculateVideoTimestampFromFrames(m_videoFrameMapFrame[frameNumber]);
  }

  m_lastVideoFrameMapFrameLoaded = m_videoFrameMapFrame[frameNumber];
  m_lastVideoFrameMapTimestampLoaded = m_videoFrameMapTimestamp[frameNumber];
  return true;
}

void CPDPVideoSource::ProcessVideo(u_int8_t *pY, u_int8_t *pU, u_int8_t *pV)
{
    // for efficiency, process ~1 second before returning to check for commands
  Timestamp frameTimestamp;
    for (int pass = 0; pass < m_maxPasses; pass++) {

        // get next frame from video capture device
        m_encodeHead = StartTimeStamp(frameTimestamp);
        if (m_encodeHead == -1) {
            continue;
        }

        ProcessVideoYUVFrame(
            pY, 
            pU, 
            pV,
            m_videoSrcWidth,
            m_videoSrcWidth >> 1,
            frameTimestamp);

        // release video frame buffer back to video capture device
        if (EndTimeStamp(m_encodeHead)) {
            m_encodeHead = (m_encodeHead + 1) % m_videoMbuf.frames;
        } else {
            debug_message("pdp_mp4live~ : couldn't release capture buffer!");
        }
    }
}
