//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   The gstreamer device read plugin.
//
//   DeviceReadGst
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

#ifndef DEVICE_READ_GST_
#define DEVICE_READ_GST_

#include "DeviceRead.h"
#include "VIOKernel.h"
#include "gst/gst.h"
#include "gst/app/gstappsink.h"
#include "gst/app/gstappbuffer.h"

#include <string>

using namespace std;
using namespace VideoIO_;

class DeviceReadGst : public DeviceRead
{
 public:
      
  /// constructor
  DeviceReadGst();
  
  /// destructor
  ~DeviceReadGst();

  /// clone method, returns a new instance
  virtual DeviceReadGst *clone() const
  { return new DeviceReadGst(); }

  /*!
   * opens the device
   * @param name can be "video" (capture card, webcam, etc.)
   *             or "dv" (dv input)
   * @param device the device (e.g. /dev/video0), optional
   * @return true if successfully opened
   */
  bool openDevice(const string &name, const string &device="");
  
  /// closes the device
  /// @return true if successfully closed
  bool closeDevice();
  
  /// starts grabbing data from the device
  void startDevice();
  
  /// stops grabbing data from the device
  void stopDevice();

  /*!
   * processes the data of current frame
   * @return VIDEO_STOPPED, VIDEO_SIZE_CHANGED or VIDEO_PLAYING
   */
  inline int processFrameData();

 protected:
  
  /// sets up the pipeline for a DV device
  void setupDVPipeline();
  
  /// sets up the pipeline for a V4L device
  /// @param device the device name
  void setupV4LPipeline(const string &device);
  
  /// frees the pipeline
  void freePipeline();
  
  // the gstreamer elements
  GstElement *source_;
  GstElement *demux_;
  GstElement *decode_;
  GstElement *videorate_;
  GstElement *colorspace_;
  GstElement *sink_;
  GstElement *device_decode_;
  
  bool have_pipeline_;
  bool new_device_;

  /// initializes gstreamer
  static void initGstreamer();
  static bool is_initialized_;
  
  /// the callback to connect dynamically to a newly created pad
  static void cbNewpad(GstElement *element, GstPad *pad, gpointer data);
  
  /// the callback for dropped frames
  static void cbFrameDropped(GstElement *element, gpointer data);
};

/// Tells us to register our functionality to an engine kernel
extern "C" void registerPlugin(VIOKernel &K);

#endif
