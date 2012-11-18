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

#include "pdp_mp4config.h"

CLiveConfig::CLiveConfig(
	SConfigVariable* variables, 
	config_index_t numVariables, 
	const char* defaultFileName)
: CConfigSet(variables, numVariables, defaultFileName) 
{
	m_appAutomatic = false;
	m_videoEncode = true;
	m_videoMaxWidth = 768;
	m_videoMaxHeight = 576;
	m_videoNeedRgbToYuv = false;
	m_videoMpeg4ConfigLength = 0;
	m_videoMpeg4Config = NULL;
	m_videoMaxVopSize = 128 * 1024;
	m_audioEncode = true;
}

CLiveConfig::~CLiveConfig()
{
	CHECK_AND_FREE(m_videoMpeg4Config);
}

// recalculate derived values
void CLiveConfig::Update() 
{
	UpdateVideo();
	UpdateAudio();
}

void CLiveConfig::UpdateVideo() 
{
	m_videoEncode = true;

	CalculateVideoFrameSize();

	GenerateMpeg4VideoConfig(this);
}

void CLiveConfig::UpdateFileHistory(const char* fileName)
{
}

void CLiveConfig::CalculateVideoFrameSize()
{
	u_int16_t frameHeight;
	float aspectRatio = GetFloatValue(CONFIG_VIDEO_ASPECT_RATIO);

	// crop video to appropriate aspect ratio modulo 16 pixels
	if ((aspectRatio - VIDEO_STD_ASPECT_RATIO) < 0.1) {
		frameHeight = GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
	} else {
		frameHeight = (u_int16_t)(
			(float)GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH) 
			/ aspectRatio);

		if ((frameHeight % 16) != 0) {
			frameHeight += 16 - (frameHeight % 16);
		}

		if (frameHeight > GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT)) {
			// OPTION might be better to insert black lines 
			// to pad image but for now we crop down
			frameHeight = GetIntegerValue(CONFIG_VIDEO_RAW_HEIGHT);
			if ((frameHeight % 16) != 0) {
				frameHeight -= (frameHeight % 16);
			}
		}
	}

	m_videoWidth = GetIntegerValue(CONFIG_VIDEO_RAW_WIDTH);
	m_videoHeight = frameHeight;

	m_ySize = m_videoWidth * m_videoHeight;
	m_uvSize = m_ySize / 4;
	m_yuvSize = (m_ySize * 3) / 2;
}

void CLiveConfig::UpdateAudio() 
{
	m_audioEncode = true;
}

void CLiveConfig::UpdateRecord()
{
}

bool CLiveConfig::IsOneSource()
{
   return true;
}

bool CLiveConfig::IsCaptureVideoSource()
{
   return false;
}

bool CLiveConfig::IsCaptureAudioSource()
{
   return false;
}

bool CLiveConfig::IsFileVideoSource()
{
   return false;
}

bool CLiveConfig::IsFileAudioSource()
{
   return false;
}
