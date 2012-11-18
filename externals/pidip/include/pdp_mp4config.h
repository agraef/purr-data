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

#ifndef __LIVE_CONFIG_H__
#define __LIVE_CONFIG_H__

#include <sys/types.h>
#include <linux/videodev.h>

#include "pdp_mp4configset.h"

#include "media_time.h"
#include "video_util_tv.h"

#define FILE_SOURCE		"FILE"
#define URL_SOURCE		"URL"

#define AUDIO_SOURCE_OSS	"OSS"
#define AUDIO_SOURCE_PDP	"PDP"

#define AUDIO_ENCODER_FAAC	"faac"
#define AUDIO_ENCODER_LAME	"lame"
#define AUDIO_ENCODING_NONE	"None"
#define AUDIO_ENCODING_PCM16	"PCM16"
#define AUDIO_ENCODING_MP3	"MP3"
#define AUDIO_ENCODING_AAC	"AAC"
#define AUDIO_ENCODING_AC3	"AC3"
#define AUDIO_ENCODING_VORBIS	"VORBIS"

#define VIDEO_SOURCE_V4L	"V4L"
#define VIDEO_SOURCE_PDP	"PDP"

#define VIDEO_ENCODER_FFMPEG	"ffmpeg"
#define VIDEO_ENCODER_DIVX	"divx"
#define VIDEO_ENCODER_H26L	"h26l"
#define VIDEO_ENCODER_XVID	"xvid"
#define VIDEO_ENCODER_H261      "h261"

#define VIDEO_ENCODING_NONE	"None"
#define VIDEO_ENCODING_YUV12	"YUV12"
#define VIDEO_ENCODING_MPEG2	"MPEG2"
#define VIDEO_ENCODING_MPEG4	"MPEG4"
#define VIDEO_ENCODING_H26L	"H26L"
#define VIDEO_ENCODING_H261     "H261"

#define VIDEO_NTSC_FRAME_RATE	((float)29.97)
#define VIDEO_PAL_FRAME_RATE	((float)25.00)

#define VIDEO_STD_ASPECT_RATIO 	((float)1.33)	// standard 4:3
#define VIDEO_LB1_ASPECT_RATIO 	((float)2.35)	// typical "widescreen" format
#define VIDEO_LB2_ASPECT_RATIO 	((float)1.85)	// alternate widescreen format
#define VIDEO_LB3_ASPECT_RATIO 	((float)1.78)	// hdtv 16:9

#define MP3_MPEG1_SAMPLES_PER_FRAME	1152	// for MPEG-1 bitrates
#define MP3_MPEG2_SAMPLES_PER_FRAME	576		// for MPEG-2 bitrates

#define VIDEO_SIGNAL_PAL 0
#define VIDEO_SIGNAL_NTSC 1
#define VIDEO_SIGNAL_SECAM 2

DECLARE_CONFIG(CONFIG_APP_REAL_TIME);
DECLARE_CONFIG(CONFIG_APP_REAL_TIME_SCHEDULER);
DECLARE_CONFIG(CONFIG_APP_DURATION);
DECLARE_CONFIG(CONFIG_APP_DURATION_UNITS);
DECLARE_CONFIG(CONFIG_APP_FILE_0);
DECLARE_CONFIG(CONFIG_APP_FILE_1);
DECLARE_CONFIG(CONFIG_APP_FILE_2);
DECLARE_CONFIG(CONFIG_APP_FILE_3);
DECLARE_CONFIG(CONFIG_APP_FILE_4);
DECLARE_CONFIG(CONFIG_APP_FILE_5);
DECLARE_CONFIG(CONFIG_APP_FILE_6);
DECLARE_CONFIG(CONFIG_APP_FILE_7);
DECLARE_CONFIG(CONFIG_APP_DEBUG);
DECLARE_CONFIG(CONFIG_APP_LOGLEVEL);
DECLARE_CONFIG(CONFIG_APP_SIGNAL_HALT);

DECLARE_CONFIG(CONFIG_AUDIO_ENABLE);
DECLARE_CONFIG(CONFIG_AUDIO_SOURCE_TYPE);
DECLARE_CONFIG(CONFIG_AUDIO_SOURCE_NAME);
DECLARE_CONFIG(CONFIG_AUDIO_MIXER_NAME);
DECLARE_CONFIG(CONFIG_AUDIO_INPUT_NAME);
DECLARE_CONFIG(CONFIG_AUDIO_SOURCE_TRACK);
DECLARE_CONFIG(CONFIG_AUDIO_CHANNELS);
DECLARE_CONFIG(CONFIG_AUDIO_SAMPLE_RATE);
DECLARE_CONFIG(CONFIG_AUDIO_BIT_RATE_KBPS);
DECLARE_CONFIG(CONFIG_AUDIO_BIT_RATE);
DECLARE_CONFIG(CONFIG_AUDIO_ENCODING);
DECLARE_CONFIG(CONFIG_AUDIO_ENCODER);
DECLARE_CONFIG(CONFIG_AUDIO_OSS_USE_SMALL_FRAGS);
DECLARE_CONFIG(CONFIG_AUDIO_OSS_FRAGMENTS);
DECLARE_CONFIG(CONFIG_AUDIO_OSS_FRAG_SIZE);

DECLARE_CONFIG(CONFIG_VIDEO_ENABLE);
DECLARE_CONFIG(CONFIG_VIDEO_SOURCE_TYPE);
DECLARE_CONFIG(CONFIG_VIDEO_SOURCE_NAME);
DECLARE_CONFIG(CONFIG_VIDEO_INPUT);
DECLARE_CONFIG(CONFIG_VIDEO_SIGNAL);
DECLARE_CONFIG(CONFIG_VIDEO_TUNER);
DECLARE_CONFIG(CONFIG_VIDEO_CHANNEL_LIST_INDEX);
DECLARE_CONFIG(CONFIG_VIDEO_CHANNEL_INDEX);
DECLARE_CONFIG(CONFIG_VIDEO_SOURCE_TRACK);
DECLARE_CONFIG(CONFIG_VIDEO_PREVIEW);
DECLARE_CONFIG(CONFIG_VIDEO_RAW_PREVIEW);
DECLARE_CONFIG(CONFIG_VIDEO_ENCODED_PREVIEW);
DECLARE_CONFIG(CONFIG_VIDEO_ENCODER);
DECLARE_CONFIG(CONFIG_VIDEO_ENCODING);
DECLARE_CONFIG(CONFIG_VIDEO_RAW_WIDTH);
DECLARE_CONFIG(CONFIG_VIDEO_RAW_HEIGHT);
DECLARE_CONFIG(CONFIG_VIDEO_ASPECT_RATIO);
DECLARE_CONFIG(CONFIG_VIDEO_FRAME_RATE);
DECLARE_CONFIG(CONFIG_VIDEO_KEY_FRAME_INTERVAL);
DECLARE_CONFIG(CONFIG_VIDEO_BIT_RATE);
DECLARE_CONFIG(CONFIG_VIDEO_PROFILE_ID);
DECLARE_CONFIG(CONFIG_VIDEO_BRIGHTNESS);
DECLARE_CONFIG(CONFIG_VIDEO_HUE);
DECLARE_CONFIG(CONFIG_VIDEO_COLOR);
DECLARE_CONFIG(CONFIG_VIDEO_CONTRAST);
DECLARE_CONFIG(CONFIG_VIDEO_TIMEBITS);
DECLARE_CONFIG(CONFIG_V4L_CACHE_TIMESTAMP);
DECLARE_CONFIG(CONFIG_VIDEO_H261_QUALITY);
DECLARE_CONFIG(CONFIG_VIDEO_H261_QUALITY_ADJ_FRAMES);
DECLARE_CONFIG(CONFIG_VIDEO_CAP_BUFF_COUNT);


DECLARE_CONFIG(CONFIG_RECORD_ENABLE);
DECLARE_CONFIG(CONFIG_RECORD_RAW_AUDIO);
DECLARE_CONFIG(CONFIG_RECORD_RAW_VIDEO);
DECLARE_CONFIG(CONFIG_RECORD_ENCODED_AUDIO);
DECLARE_CONFIG(CONFIG_RECORD_ENCODED_VIDEO);
DECLARE_CONFIG(CONFIG_RECORD_MP4_FILE_NAME);
DECLARE_CONFIG(CONFIG_RECORD_MP4_HINT_TRACKS);
DECLARE_CONFIG(CONFIG_RECORD_MP4_OVERWRITE);
DECLARE_CONFIG(CONFIG_RECORD_MP4_OPTIMIZE);

DECLARE_CONFIG(CONFIG_RTP_ENABLE);
DECLARE_CONFIG(CONFIG_RTP_DEST_ADDRESS); // for video
DECLARE_CONFIG(CONFIG_RTP_AUDIO_DEST_PORT);
DECLARE_CONFIG(CONFIG_RTP_VIDEO_DEST_PORT);
DECLARE_CONFIG(CONFIG_RTP_RECV_BUFFER_TIME);
DECLARE_CONFIG(CONFIG_RTP_PAYLOAD_SIZE);
DECLARE_CONFIG(CONFIG_RTP_MCAST_TTL);
DECLARE_CONFIG(CONFIG_RTP_DISABLE_TS_OFFSET);
DECLARE_CONFIG(CONFIG_RTP_USE_SSM);
DECLARE_CONFIG(CONFIG_SDP_FILE_NAME);
DECLARE_CONFIG(CONFIG_RTP_AUDIO_DEST_ADDRESS);
DECLARE_CONFIG(CONFIG_RTP_USE_MP3_PAYLOAD_14);
DECLARE_CONFIG(CONFIG_RTP_NO_B_RR_0);
DECLARE_CONFIG(CONFIG_RAW_ENABLE);
DECLARE_CONFIG(CONFIG_RAW_PCM_FILE_NAME);
DECLARE_CONFIG(CONFIG_RAW_PCM_FIFO);
DECLARE_CONFIG(CONFIG_RAW_YUV_FILE_NAME);
DECLARE_CONFIG(CONFIG_RAW_YUV_FIFO);


#ifdef DECLARE_CONFIG_VARIABLES
static SConfigVariable PdpConfigVariables[] = {

  CONFIG_BOOL(CONFIG_APP_REAL_TIME, "isRealTime", true),
  CONFIG_BOOL(CONFIG_APP_REAL_TIME_SCHEDULER, "useRealTimeScheduler", true),
  CONFIG_INT(CONFIG_APP_DURATION, "duration", 1),
  CONFIG_INT(CONFIG_APP_DURATION_UNITS, "durationUnits", 60),

  CONFIG_STRING(CONFIG_APP_FILE_0, "file0", ""),
  CONFIG_STRING(CONFIG_APP_FILE_1, "file1", ""),
  CONFIG_STRING(CONFIG_APP_FILE_2, "file2", ""),
  CONFIG_STRING(CONFIG_APP_FILE_3, "file3", ""),
  CONFIG_STRING(CONFIG_APP_FILE_4, "file4", ""),
  CONFIG_STRING(CONFIG_APP_FILE_5, "file5", ""),
  CONFIG_STRING(CONFIG_APP_FILE_6, "file6", ""),
  CONFIG_STRING(CONFIG_APP_FILE_7, "file7", ""),

  CONFIG_BOOL(CONFIG_APP_DEBUG, "debug", false),
  CONFIG_INT(CONFIG_APP_LOGLEVEL, "logLevel", 0),
  CONFIG_STRING(CONFIG_APP_SIGNAL_HALT, "signalHalt", "sighup"),
    
  // AUDIO
    
  CONFIG_BOOL(CONFIG_AUDIO_ENABLE, "audioEnable", true),
  CONFIG_STRING(CONFIG_AUDIO_SOURCE_TYPE, "audioSourceType", AUDIO_SOURCE_PDP),
  CONFIG_STRING(CONFIG_AUDIO_SOURCE_NAME, "audioDevice", "/dev/dsp"),
  CONFIG_STRING(CONFIG_AUDIO_MIXER_NAME, "audioMixer", "/dev/mixer"),
  CONFIG_STRING(CONFIG_AUDIO_INPUT_NAME, "audioInput", "mix"),

  CONFIG_INT(CONFIG_AUDIO_SOURCE_TRACK, "audioSourceTrack", 0),
  CONFIG_INT(CONFIG_AUDIO_CHANNELS, "audioChannels", 2),
  CONFIG_INT(CONFIG_AUDIO_SAMPLE_RATE, "audioSampleRate", 44100),
  CONFIG_INT(CONFIG_AUDIO_BIT_RATE_KBPS, "audioBitRate", 128),
  CONFIG_INT(CONFIG_AUDIO_BIT_RATE, "audioBitRateBps", 128000),
  CONFIG_STRING(CONFIG_AUDIO_ENCODING, "audioEncoding", AUDIO_ENCODING_AAC),
  CONFIG_STRING(CONFIG_AUDIO_ENCODER, "audioEncoder", AUDIO_ENCODER_FAAC),

  CONFIG_BOOL(CONFIG_AUDIO_OSS_USE_SMALL_FRAGS, "audioOssUseSmallFrags", true),
  CONFIG_INT(CONFIG_AUDIO_OSS_FRAGMENTS, "audioOssFragments", 128),
  CONFIG_INT(CONFIG_AUDIO_OSS_FRAG_SIZE, "audioOssFragSize", 8),

  // VIDEO 
    
  CONFIG_BOOL(CONFIG_VIDEO_ENABLE, "videoEnable", true),
  CONFIG_STRING(CONFIG_VIDEO_SOURCE_TYPE, "videoSourceType", VIDEO_SOURCE_PDP),
  CONFIG_STRING(CONFIG_VIDEO_SOURCE_NAME, "videoDevice", "/dev/video0"),
  CONFIG_INT(CONFIG_VIDEO_INPUT, "videoInput", 1),
  CONFIG_INT(CONFIG_VIDEO_SIGNAL, "videoSignal", VIDEO_SIGNAL_NTSC),

  CONFIG_INT(CONFIG_VIDEO_TUNER, "videoTuner", -1),
  CONFIG_INT(CONFIG_VIDEO_CHANNEL_LIST_INDEX, "videoChannelListIndex", 0),
  CONFIG_INT(CONFIG_VIDEO_CHANNEL_INDEX, "videoChannelIndex", 1),

  CONFIG_INT(CONFIG_VIDEO_SOURCE_TRACK, "videoSourceTrack", 0),

  CONFIG_BOOL(CONFIG_VIDEO_PREVIEW, "videoPreview", true),
  CONFIG_BOOL(CONFIG_VIDEO_RAW_PREVIEW, "videoRawPreview", false),
  CONFIG_BOOL(CONFIG_VIDEO_ENCODED_PREVIEW, "videoEncodedPreview", true),

  CONFIG_STRING(CONFIG_VIDEO_ENCODER, "videoEncoder", VIDEO_ENCODER_XVID),
  CONFIG_STRING(CONFIG_VIDEO_ENCODING, "videoEncoding", VIDEO_ENCODING_MPEG4),

  CONFIG_INT(CONFIG_VIDEO_RAW_WIDTH, "videoRawWidth", 320),
  CONFIG_INT(CONFIG_VIDEO_RAW_HEIGHT, "videoRawHeight", 240),
  CONFIG_FLOAT(CONFIG_VIDEO_ASPECT_RATIO, "videoAspectRatio", VIDEO_STD_ASPECT_RATIO),
  CONFIG_FLOAT(CONFIG_VIDEO_FRAME_RATE, "videoFrameRate", VIDEO_PAL_FRAME_RATE),
  CONFIG_FLOAT(CONFIG_VIDEO_KEY_FRAME_INTERVAL, "videoKeyFrameInterval", 2.0),

  CONFIG_INT(CONFIG_VIDEO_BIT_RATE, "videoBitRate", 128),
  CONFIG_INT(CONFIG_VIDEO_PROFILE_ID, "videoProfileId", MPEG4_SP_L3),

  CONFIG_INT(CONFIG_VIDEO_BRIGHTNESS, "videoBrightness", 50),
  CONFIG_INT(CONFIG_VIDEO_HUE, "videoHue", 50),
  CONFIG_INT(CONFIG_VIDEO_COLOR, "videoColor", 50),
  CONFIG_INT(CONFIG_VIDEO_CONTRAST, "videoContrast", 50),

  CONFIG_INT(CONFIG_VIDEO_TIMEBITS, "videoTimebits", 0),

  CONFIG_BOOL(CONFIG_V4L_CACHE_TIMESTAMP, "videoTimestampCache", true),
  CONFIG_INT(CONFIG_VIDEO_H261_QUALITY, "videoH261Quality", 10),
  CONFIG_INT(CONFIG_VIDEO_H261_QUALITY_ADJ_FRAMES, "videoH261QualityAdjFrames", 8),

  CONFIG_INT(CONFIG_VIDEO_CAP_BUFF_COUNT, "videoCaptureBuffersCount", 16),

  // RECORD
  CONFIG_BOOL(CONFIG_RECORD_ENABLE, "recordEnable", true),
  CONFIG_BOOL(CONFIG_RECORD_RAW_AUDIO, "recordRawAudio", false),
  CONFIG_BOOL(CONFIG_RECORD_RAW_VIDEO, "recordRawVideo", false),
  CONFIG_BOOL(CONFIG_RECORD_ENCODED_AUDIO, "recordEncodedAudio", true),
  CONFIG_BOOL(CONFIG_RECORD_ENCODED_VIDEO, "recordEncodedVideo", true),

  CONFIG_STRING(CONFIG_RECORD_MP4_FILE_NAME, "recordMp4File", "capture.mp4"),
  CONFIG_BOOL(CONFIG_RECORD_MP4_HINT_TRACKS, "recordMp4HintTracks", true),
  CONFIG_BOOL(CONFIG_RECORD_MP4_OVERWRITE, "recordMp4Overwrite", true),
  CONFIG_BOOL(CONFIG_RECORD_MP4_OPTIMIZE, "recordMp4Optimize", false),

  // RTP
    
  CONFIG_BOOL(CONFIG_RTP_ENABLE, "rtpEnable", true),
  CONFIG_STRING(CONFIG_RTP_DEST_ADDRESS, "rtpDestAddress", "127.0.0.1"),
  CONFIG_STRING(CONFIG_RTP_AUDIO_DEST_ADDRESS, "audioRtpDestAddress", "127.0.0.1"),

  CONFIG_INT(CONFIG_RTP_AUDIO_DEST_PORT, "rtpAudioDestPort", 8000),
  CONFIG_INT(CONFIG_RTP_VIDEO_DEST_PORT, "rtpVideoDestPort", 7070),

  CONFIG_INT(CONFIG_RTP_PAYLOAD_SIZE, "rtpPayloadSize", 1460),
  CONFIG_INT(CONFIG_RTP_MCAST_TTL, "rtpMulticastTtl", 15),

  CONFIG_BOOL(CONFIG_RTP_DISABLE_TS_OFFSET, "rtpDisableTimestampOffset", false),
  CONFIG_BOOL(CONFIG_RTP_USE_SSM, "rtpUseSingleSourceMulticast", false),

  CONFIG_STRING(CONFIG_SDP_FILE_NAME, "sdpFile", "capture.sdp"),

  CONFIG_BOOL(CONFIG_RTP_USE_MP3_PAYLOAD_14, "rtpUseMp4RtpPayload14", false),
  CONFIG_BOOL(CONFIG_RTP_NO_B_RR_0, "rtpNoBRR0", false),

  // RAW sink

  CONFIG_BOOL(CONFIG_RAW_ENABLE, "rawEnable", false),
  CONFIG_STRING(CONFIG_RAW_PCM_FILE_NAME, "rawAudioFile", "capture.pcm"),
  CONFIG_BOOL(CONFIG_RAW_PCM_FIFO, "rawAudioUseFifo", false),

  CONFIG_STRING(CONFIG_RAW_YUV_FILE_NAME, "rawVideoFile", "capture.yuv"),
  CONFIG_BOOL(CONFIG_RAW_YUV_FIFO, "rawVideoUseFifo", false)

};
#endif

// forward declarations
class CVideoCapabilities;
class CAudioCapabilities;
class CLiveConfig;

// some configuration utility routines
void GenerateMpeg4VideoConfig(CLiveConfig* pConfig);
bool GenerateSdpFile(CLiveConfig* pConfig);
struct session_desc_t;

session_desc_t *createSdpDescription(CLiveConfig *pConfig, 
				     char *sAudioDestAddr,
				     char *sVideoDestAddr,
				     int ttl,
				     bool allow_rtcp,
				     int video_port,
				     int audio_port);

class CLiveConfig : public CConfigSet {
public:
        CLiveConfig(SConfigVariable* variables,
                config_index_t numVariables, const char* defaultFileName);

        ~CLiveConfig();

        // recalculate derived values
        void Update();
        void UpdateFileHistory(const char* fileName);
        void UpdateVideo();
        void CalculateVideoFrameSize();
        void UpdateAudio();
        void UpdateRecord();

        bool IsOneSource();
        bool IsCaptureVideoSource();
        bool IsCaptureAudioSource();
        bool IsFileVideoSource();
        bool IsFileAudioSource();

        bool SourceRawVideo() {
                return false;
        }

        bool SourceRawAudio() {
                return false;
        }

public:
        // command line configuration
        bool            m_appAutomatic;

        // derived, shared video configuration
        CVideoCapabilities* m_videoCapabilities;
        bool            m_videoEncode;
        u_int32_t       m_videoPreviewWindowId;
        u_int16_t       m_videoWidth;
        u_int16_t       m_videoHeight;
        u_int16_t       m_videoMaxWidth;
        u_int16_t       m_videoMaxHeight;
        u_int32_t       m_ySize;
        u_int32_t       m_uvSize;
        u_int32_t       m_yuvSize;
        bool            m_videoNeedRgbToYuv;
        u_int16_t       m_videoMpeg4ConfigLength;
        u_int8_t*       m_videoMpeg4Config;
        u_int32_t       m_videoMaxVopSize;
        u_int8_t        m_videoTimeIncrBits;

        // derived, shared audio configuration
        CAudioCapabilities* m_audioCapabilities;
        bool            m_audioEncode;

        // derived, shared file configuration
        u_int64_t       m_recordEstFileSize;
};

#endif /* __LIVE_CONFIG_H__ */

