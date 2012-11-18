//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   A dummy file read plugin.
//
//   FRDummy
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


#ifndef FILE_READ_DUMMY_
#define FILE_READ_DUMMY_

#include "DeviceRead.h"
#include "VIOKernel.h"

#include <string>

using namespace std;
using namespace VideoIO_;

class DeviceReadDummy : public DeviceRead
{
  public:

  /// clone method, returns a new instance
  virtual DeviceReadDummy *clone() const
  { return new DeviceReadDummy(); }

  /// opens the file at the given path
  /// @param filename the path of the file
    bool openDevice(const string &filename, const string &devname);
  
  bool closeDevice(){return true;};
  
  void startDevice(){};
  
  void stopDevice(){};
  
  bool setPosition(float sec)
  { return false; }
  
  void setSpeed(float speed){};
  
  /*!
   * processes the data of current frame
   * @return VIDEO_STOPPED, VIDEO_SIZE_CHANGED or VIDEO_PLAYING
   */
  inline int processFrameData();

  /// @return the frames per second
  double getFPS() { return 20.; };
  
  /// @return the width of the video
  int getWidth() { return 20; };
  
  /// @return the height of the video
  int getHeight() { return 20; };
  
  void getAudioBlock(t_float *left, t_float *right, int n){};

};

/// Tells us to register our functionality to an engine kernel
extern "C" void registerPlugin(VIOKernel &K);


#endif
