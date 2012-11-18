//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   The plugin for GStreamer.
//
//   FileWriteGst
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

#ifndef FILE_WRITE_GST_
#define FILE_WRITE_GST_

#include "FileWrite.h"
#include "VIOKernel.h"
#include "gst/gst.h"
#include "gst/app/gstappsrc.h"
#include "gst/app/gstappbuffer.h"

#include <string>

using namespace std;
using namespace VideoIO_;

class FileWriteGst : public FileWrite
{
 public:
  
   FileWriteGst();
  
  ~FileWriteGst();

  /// clone method, returns a new instance
  virtual FileWriteGst *clone() const
  { return new FileWriteGst(); }

  /*!
   * writes one frame in the video file
   * @param frame written in video file
   */
  void pushFrame(VIOFrame &frame);
  
  /*!
   * opens the file at the given path
   * @param filename the path of the file
   * @return true if open worked
   */
  bool openFile(const string &uri);

  /*!
   * stops recording
   * @return false if file was written
   */
  bool stopRecording();

  /// prints the avaliable codecs
  void getCodec();

 protected:
   
  /// inits video file
  void initRecording(int xsize, int ysize, int cs);
  
  /// sets up a pipeline for raw video
  /// @param filename the name of the file/destination to write to
  /// @return true if successful
  bool setupRawPipeline(const string &filename);
  
  /// sets up a pipeline for ogg theora encoding
  /// @param filename the name of the file/destination to write to
  /// @return true if successful
  bool setupOggPipeline(const string &filename);
  
  /// sets up a pipeline for mpeg4 encoding
  /// @param filename the name of the file/destination to write to
  /// @return true if successful
  bool setupMpeg4Pipeline(const string &filename);

  /// cleans up the memory allocated by the pipeline
  void freePipeline();
  
  /// reads the settings needed to write out of the uri
  /// @param uri the uri commited from pd
  /// @return the filename/destination to write to
  string getSettingsFromURI(const string &uri);
   
  // the Gstreamer elements
  GstElement *source_;
  GstElement *videorate_;
  GstElement *colorspace_; 
  GstElement *encode_;
  GstElement *mux_;
  GstElement *parse_;
  GstElement *queue_;
  GstElement *sink_;
  GstElement *file_encode_;
  GstBus *bus_;

  bool new_video_;
  bool have_pipeline_;
  
  /// the desired port (for stream writing)
  int port_;
  
  /// the desired sink element (e.g.filesink or udpsink)
  string sink_element_;
  
  /// initializes gstreamer
  static void initGstreamer();
  static bool is_initialized_;

  /// callback to free our buffer
  static void freeRecBuffer(void *data);
};

/// Tells us to register our functionality to an engine kernel
extern "C" void registerPlugin(VIOKernel &K);

#endif
