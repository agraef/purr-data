//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   Utilities for the VideoIO-Framework.
//
//   vioutils
//   header file
//
//   copyright            : (C) 2007 by Thomas Holzmann, Georg Holzmann
//   email                : holzi1@gmx.at, grh _at_ mut _dot_ at
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 3 of the License, or
//   (at your option) any later version.
//
///////////////////////////////////////////////////////////////////////////
    
    
#ifndef VIOUTILS_H_
#define VIOUTILS_H_
    
    
/*!
 * \file VIOUtils.h
 * Some global defines for der VideoIO-Framework
 */

#include "m_pd.h"
#include <string>
    
namespace VideoIO_
{

/// file extension used by the plugins
/// TODO auf windows anders machen (mac gleich ?)
const std::string PLUGIN_FILE_EXTENSION = ".so";

/// video process defines
const int VIDEO_STOPPED = 0;       //! current video is over
const int VIDEO_SIZE_CHANGED = 1;  //! frame size changed
const int VIDEO_PLAYING = 2;       //! video is playing


// color spaces defines:
const int RGB = 0;       //! RGB colorspace
const int RGBA = 1;      //! RGBA colorspace
const int YUV422 = 2;    //! YUV422 colorspace
const int GRAY = 3;      //! GRAY colorspace


// color component defines:
// RGBA
const int chRed = 0;     //! channel Red
const int chGreen = 1;   //! channel Green
const int chBlue = 2;    //! channel Blue
const int chAlpha = 3;   //! channel Alpha

// YUV 422
const int chU = 0;       //! channel U
const int chY0 = 1;      //! channel Y0
const int chV = 2;       //! channel V
const int chY1 = 3;      //! channel Y1

// Gray
const int chGray = 0;    //! channel Gray
}

#endif
