//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   The plugin for GStreamer.
//
//   FileReadGst
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

#ifndef FILE_READ_GST_
#define FILE_READ_GST_

#include "FileRead.h"
#include "VIOKernel.h"
#include "gst/gst.h"
#include "gst/app/gstappsink.h"
#include "gst/app/gstappbuffer.h"
#include "gst/base/gstadapter.h"

#include <string>

using namespace std;
using namespace VideoIO_;

/*!
 * \class FileReadGst
 *
 * file reader with gstreamer
 * NOTE you can debug gstreamer based applications
 * with "GST_DEBUG=3 pd -lib Gem ..."
 * to debug one single plugin, e.g.:
 * "GST_DEBUG=appsink:5 pd -lib Gem ..."
 */
class FileReadGst : public FileRead
{
 public:
  FileReadGst();
  
  ~FileReadGst();

  /// clone method, returns a new instance
  virtual FileReadGst *clone() const
  { return new FileReadGst(); }

  /*!
   * opens the file at the given path
   * @param filename the path of the file
   * @return true if open worked
   */
  bool openFile(string filename);
  
  /*!
   * closes the file
   */
  void closeFile();

  /*!
   * starts playing the video asynchronus to pd
   */
  void startVideo();

  /*!
   * stops playing the video
   */
  void stopVideo();

  /*!
   * changes the position in the video
   * @param msec desired position in the stream in milliseconds
   * @return false if there was a problem
   */
  bool setPosition(float msec);
  
  /*!
   * sets the playing speed of the video
   * @param speed the desired speed (e.g. 2.0 for double speed)
   */
  void setSpeed(float speed);

  /*!
   * processes the data of current frame
   * @return VIDEO_STOPPED, VIDEO_SIZE_CHANGED or VIDEO_PLAYING
   */
  inline int processFrameData();

  /*!
   * writes stereo audio data for one block to pointers
   * each pointer is responsible to allocate memory of
   * size n before !
   * @param left pointer to channel left audio samples
   * @param right pointer to channel right audio samples
   * @param n blocksize, nr of sample to grab for each channel
   */
  void processAudioBlock(t_float *left, t_float *right, int n);

 protected:
   
  /// creates the audio bin on demand
  /// @return true if successful
  bool createAudioBin();
  
  /// creates the video bin on demand
  /// @return true if successful
  bool createVideoBin();
  
  /// @param filename the filename string from pd
  /// @return the uri made from the filename
  string getURIFromFilename(const string &filename);
  
  // the gsreamer elements
  GstElement *source_;
  GstElement *decode_;
  GstElement *videorate_;
  GstElement *colorspace_;
  GstElement *vqueue_;
  GstElement *vsink_;
  GstElement *aconvert_;
  GstElement *aresample_;
  GstElement *aqueue_;
  GstElement *asink_;
  GstElement *file_decode_;
  GstElement *video_bin_;
  GstElement *audio_bin_;
  GstAdapter *adapter_;
  GstBus *bus_;
  
  bool have_pipeline_;
  bool new_video_;
  
  // true if we want to write a udp stream
  bool is_udp_;
  
  // is needed to get the right track
  int vtrack_count_;
  int atrack_count_;

  /// initializes gstreamer
  static void initGstreamer();
  static bool is_initialized_;
  
  /// the callback to connect dynamically to a newly created pad
  static void cbNewpad(GstElement *decodebin, GstPad *pad, gpointer data);
};

/// Tells us to register our functionality to an engine kernel
extern "C" void registerPlugin(VIOKernel &K);

#endif
