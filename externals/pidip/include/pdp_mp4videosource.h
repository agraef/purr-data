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

#ifndef __PDP_MP4VIDEOSOURCE__
#define __PDP_MP4VIDEOSOURCE__

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>

#include "media_source.h"
#include "video_encoder.h"

class CPDPVideoSource : public CMediaSource {
public:
    CPDPVideoSource() : CMediaSource() {
        m_videoMap = NULL;
        m_videoFrameMap = NULL;
    }

    bool IsDone() {
        return false;   
    }

    float GetProgress() {
        return 0.0;     
    }

    void ProcessVideo(u_int8_t *pY, u_int8_t *pU, u_int8_t *pV);

    void DoStart(void);

    void DoStop(void);

protected:
    int ThreadMain(void);

    bool Init(void);

    int8_t StartTimeStamp(Timestamp &frameTimestamp);

    bool EndTimeStamp(int8_t frameNumber);

    Timestamp CalculateVideoTimestampFromFrames (uint64_t frame) {
      double duration = frame;
      duration *= TimestampTicks;
      duration /= m_videoSrcFrameRate;
      return m_videoCaptureStartTimestamp + (Timestamp)duration;
    }
protected:
    u_int8_t             m_maxPasses;

    struct video_mbuf    m_videoMbuf;
    void*                m_videoMap;
    struct video_mmap*   m_videoFrameMap;
    Timestamp            m_videoCaptureStartTimestamp;
    uint64_t             m_videoFrames;
    Duration             m_videoSrcFrameDuration;
    int8_t               m_captureHead;
    int8_t               m_encodeHead;
    float                m_videoSrcFrameRate;
    uint64_t             *m_videoFrameMapFrame;
    Timestamp            *m_videoFrameMapTimestamp;
    uint64_t             m_lastVideoFrameMapFrameLoaded;
    Timestamp            m_lastVideoFrameMapTimestampLoaded;
    bool                 m_cacheTimestamp;
};

#endif /* __PDP_MP4VIDEOSOURCE__ */
