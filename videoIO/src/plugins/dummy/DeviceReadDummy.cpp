//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   A dummy file read plugin.
//
//   FRDummy
//   implementation file
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

#include "DeviceReadDummy.h"

#include <stdlib.h>

bool DeviceReadDummy::openDevice(const string &filename, const string &devname)
{
  frame_.setFrameSize(20, 20, RGBA);

 // has_video_file_ = true;
  
  return true;
}

int DeviceReadDummy::processFrameData()
{
  int size = frame_.getXSize() * frame_.getYSize() * frame_.getColorSize();
  unsigned char *data = frame_.getFrameData();

  post("frame %dx%d=%d --> %x", frame_.getXSize(), frame_.getYSize(), size, data);
  
  while(size--)
    *data++ = (unsigned char) rand() % 256;

  return VIDEO_PLAYING;
}

  /// Tells us to register our functionality to an engine kernel
void registerPlugin(VIOKernel &K)
{
  K.getDeviceReadServer().addPlugin(
    auto_ptr<DeviceRead>(new DeviceReadDummy()));
  
//  post("VideoIO: registered DeviceReadDummy Plugin");
}

