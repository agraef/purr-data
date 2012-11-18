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

#include "FileWriteGst.h"
#include <locale.h>

bool FileWriteGst::is_initialized_ = false;

FileWriteGst::FileWriteGst() : 
    source_(NULL), videorate_(NULL), colorspace_(NULL), encode_(NULL),
    mux_(NULL), parse_(NULL), queue_(NULL), sink_(NULL),
    file_encode_(NULL), bus_(NULL),
    new_video_(false), have_pipeline_(false), port_(0)
{
  initGstreamer();
}

FileWriteGst::~FileWriteGst()
{
  if(have_pipeline_)
    freePipeline();
}

void FileWriteGst::pushFrame(VIOFrame &frame)
{
  if(!have_pipeline_) return;

  if(new_video_)
  {
    initRecording( frame.getXSize(), frame.getYSize(), 
                   frame.getColorspace() );
    new_video_=false;
  }

  // for gstreamer buffers
  int size = frame.getYSize() *
             GST_ROUND_UP_4(frame.getXSize()*frame.getColorSize());

  unsigned char *data = frame.getFrameData();
  if( !data ) return;

  unsigned char *rec_data = new unsigned char[size];
  unsigned char *tmprec = rec_data;

  if( !data ) return;

  // copy data and swap y axes (somehow optimized)
  int rowsize = frame.getXSize()*frame.getColorSize();
  int n;
  unsigned char *tmpdata;
  for(int y=(frame.getYSize()-1); y>=0; --y)
  {
    n = rowsize;
    tmpdata = data + (y*rowsize);
    while(n--)
      *tmprec++ = *tmpdata++;
  }

  GstBuffer *buf;
  buf = gst_app_buffer_new (rec_data, size, freeRecBuffer, (void*)rec_data);
 
  gst_app_src_push_buffer (GST_APP_SRC (source_), buf);
}

bool FileWriteGst::openFile(const string &uri)
{
  string filename = getSettingsFromURI( uri);

  if(have_pipeline_)
    freePipeline();

  if (codec_ == "ogg" || codec_ == "theora")
    setupOggPipeline(filename);
  else if (codec_ == "mpeg4")   
    setupMpeg4Pipeline(filename);
  else
    setupRawPipeline(filename);
  
  ///NOTE I only succeeded in playing the mpeg4 and raw files with mplayer, 
  /// vlc doesn't play it. But gstreamer (playbin, videoIO) can also play it
  
  // set ready state
  if(!gst_element_set_state (file_encode_, GST_STATE_READY))
  {
    post("The state could not be set to READY");
    return false;
  }

  have_pipeline_=true;
  new_video_=true;
  
  setlocale(LC_NUMERIC, "C"); 
  
  return true;
}

bool FileWriteGst::stopRecording()
{
  if(!have_pipeline_) return false;

  gst_app_src_end_of_stream ( GST_APP_SRC(source_) );
  gst_element_set_state (file_encode_, GST_STATE_NULL);

  return true;
}

void FileWriteGst::initRecording(int xsize, int ysize, int cs)
{
  int fr1 = (int) framerate_ * 10000;
  int fr2 = 10000;

  /// NOTE endianess is set to G_BIG_ENDIAN because this is needed
  /// for some internal reasons (also for Intels !)
  switch(cs)
  {
    case GRAY:
      gst_app_src_set_caps ( GST_APP_SRC(source_),
                       gst_caps_new_simple ("video/x-raw-gray",
				     "width", G_TYPE_INT, xsize,
				     "height", G_TYPE_INT, ysize,
				     "bpp", G_TYPE_INT, 8,
				     "depth", G_TYPE_INT, 8,
			             "framerate", GST_TYPE_FRACTION, fr1, fr2,
				     "endianness", G_TYPE_INT, G_BIG_ENDIAN,
				     NULL)
                       );
      break;

    case YUV422:
      gst_app_src_set_caps ( GST_APP_SRC(source_),
                       gst_caps_new_simple ("video/x-raw-yuv",
			             "framerate", GST_TYPE_FRACTION, fr1, fr2,
				     "width", G_TYPE_INT, xsize,
				     "height", G_TYPE_INT, ysize,
                                     "format", GST_TYPE_FOURCC,
                                     GST_MAKE_FOURCC('U', 'Y', 'V', 'Y'),
				     NULL)
                       );
      break;

    case RGB:
      gst_app_src_set_caps ( GST_APP_SRC(source_),
                       gst_caps_new_simple ("video/x-raw-rgb",
				     "width", G_TYPE_INT, xsize,
				     "height", G_TYPE_INT, ysize,
				     "bpp", G_TYPE_INT, 24,
				     "depth", G_TYPE_INT, 24,
				     "red_mask",   G_TYPE_INT, 0x00ff0000,
				     "green_mask", G_TYPE_INT, 0x0000ff00,
				     "blue_mask",  G_TYPE_INT, 0x000000ff,
			             "framerate", GST_TYPE_FRACTION, fr1, fr2,
				     "endianness", G_TYPE_INT, G_BIG_ENDIAN,
				     NULL)
                       );
      break;

    case RGBA:
    default:
      gst_app_src_set_caps ( GST_APP_SRC(source_),
                       gst_caps_new_simple ("video/x-raw-rgb",
				     "width", G_TYPE_INT, xsize,
				     "height", G_TYPE_INT, ysize,
				     "bpp", G_TYPE_INT, 32,
				     "depth", G_TYPE_INT, 32,
				     "red_mask",   G_TYPE_INT, 0xff000000,
				     "green_mask", G_TYPE_INT, 0x00ff0000,
				     "blue_mask",  G_TYPE_INT, 0x0000ff00,
				     "alpha_mask", G_TYPE_INT, 0x000000ff,
			             "framerate", GST_TYPE_FRACTION, fr1, fr2,
				     "endianness", G_TYPE_INT, G_BIG_ENDIAN,
				     NULL)
                       );
      break;
  }

  // set playing state
  if(!gst_element_set_state (file_encode_, GST_STATE_PLAYING))
    post("FileWriteGst: recording could not be started!");
  else post("FileWriteGst: started recording");
}

void FileWriteGst::getCodec()
{
  post("-----------------------------------------");
  post("FileWriteGst available codecs:");
  post("raw");
  post("theora:");
  post("  quality: 0 - 63, default: 16");
  post("  bitrate: 0 - 2000");
  post("mpeg4: ");
  post("  bitrate: 0 - 4294967295, default: 300000");
  post("-----------------------------------------");
}

bool FileWriteGst::setupRawPipeline(const string &filename)
{
  file_encode_ = gst_pipeline_new( "file_encode_");

  source_ = gst_element_factory_make ("appsrc", "source_");
  g_assert(source_);
//   videorate_ = gst_element_factory_make ("videorate", "videorate_");
//   g_assert(videorate_);
  colorspace_ = gst_element_factory_make ("ffmpegcolorspace", "colorspace_");
  g_assert(colorspace_);
  mux_ = gst_element_factory_make("avimux", "mux_");
  g_assert(mux_);
  queue_ = gst_element_factory_make("queue", "queue_");
  g_assert(queue_);
  sink_ = gst_element_factory_make (sink_element_.c_str(), "sink_");
  g_assert(sink_);

  if (sink_element_ == "filesink")
  {
    g_object_set (G_OBJECT(sink_), "location", filename.c_str(), NULL);
    g_object_set (G_OBJECT(sink_), "sync", true, NULL);
  }
  
  else if (sink_element_ == "udpsink")
  {
    // set the host
    g_object_set (G_OBJECT(sink_), "host", filename.c_str(), NULL);

    // set the port
    if (port_ != 0)
      g_object_set (G_OBJECT(sink_), "port", port_, NULL);
  }

  gst_bin_add_many (GST_BIN (file_encode_), source_, colorspace_, mux_, queue_, sink_, NULL);
  gst_element_link_many (source_, colorspace_, mux_, queue_, sink_, NULL);

  return true;
}

bool FileWriteGst::setupOggPipeline(const string &filename)
{
  file_encode_ = gst_pipeline_new( "file_encode_");

  source_ = gst_element_factory_make ("appsrc", "source_");
  g_assert(source_);
  colorspace_ = gst_element_factory_make ("ffmpegcolorspace", "colorspace_");
  g_assert(colorspace_);
  encode_ = gst_element_factory_make ("theoraenc", "encode_");
  g_assert(encode_);
  mux_ = gst_element_factory_make("oggmux", "mux_");
  g_assert(mux_);
  sink_ = gst_element_factory_make (sink_element_.c_str(), "sink_");
  g_assert(sink_);

  if (sink_element_ == "filesink")
    g_object_set (G_OBJECT(sink_), "location", filename.c_str(), NULL);
  
  else if (sink_element_ == "udpsink")
  {
    // set the host
    g_object_set (G_OBJECT(sink_), "host", filename.c_str(), NULL);
    // set the port
    if (port_ != 0)
      g_object_set (G_OBJECT(sink_), "port", port_, NULL);
  }

  // set theora parameter
  if( cparameters_.find("quality") != cparameters_.end() )
    g_object_set (G_OBJECT(encode_), "quality", cparameters_["quality"], NULL);
  if( cparameters_.find("bitrate") != cparameters_.end() )
    g_object_set (G_OBJECT(encode_), "bitrate", cparameters_["bitrate"], NULL);

  gst_bin_add_many (GST_BIN (file_encode_), source_, colorspace_, encode_, mux_, sink_, NULL);
  gst_element_link_many (source_, colorspace_, encode_, mux_, sink_, NULL);

  return true;
}

bool FileWriteGst::setupMpeg4Pipeline(const string &filename)
{
  file_encode_ = gst_pipeline_new( "file_encode_");

  source_ = gst_element_factory_make ("appsrc", "source_");
  g_assert(source_);
  colorspace_ = gst_element_factory_make ("ffmpegcolorspace", "colorspace_");
  g_assert(colorspace_);
  encode_ = gst_element_factory_make ("ffenc_mpeg4", "encode_");
  g_assert(encode_);
  mux_ = gst_element_factory_make("avimux", "mux_");
  g_assert(mux_);
  sink_ = gst_element_factory_make (sink_element_.c_str(), "sink_");
  g_assert(sink_);

  if (sink_element_ == "filesink")
    g_object_set (G_OBJECT(sink_), "location", filename.c_str(), NULL);
  
  else if (sink_element_ == "udpsink")
  {
    // set the host
    g_object_set (G_OBJECT(sink_), "host", filename.c_str(), NULL);
    // set the port
    if (port_ != 0)
      g_object_set (G_OBJECT(sink_), "port", port_, NULL);
  }

  // set mpeg4 parameter
  if( cparameters_.find("bitrate") != cparameters_.end() )
    g_object_set (G_OBJECT(encode_), "bitrate", cparameters_["bitrate"], NULL);

  gst_bin_add_many (GST_BIN (file_encode_), source_, colorspace_, encode_, mux_, sink_, NULL);
  gst_element_link_many (source_, colorspace_, encode_, mux_, sink_, NULL);

  return true;
}

void FileWriteGst::freePipeline()
{
  if(!have_pipeline_) return;

  // Gstreamer clean up
  gst_element_set_state (file_encode_, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (file_encode_));
  have_pipeline_ = false;
}

string FileWriteGst::getSettingsFromURI(const string &uri)
{
  if(uri.compare(0, 6, "udp://") == 0)
  {
    sink_element_ = "udpsink";
    
    string str = uri;
    str.erase(0, 6);
    string host = str;
    
    int index = str.find_first_of( ':', 0 );
    if (index != -1)
    {
      str.erase(0, index + 1);
      port_ = atoi(str.c_str());
      host = host.erase(index);
    }

    return host;
  }
  else
  {
    sink_element_ = "filesink";
    return uri;
  }

}

void FileWriteGst::initGstreamer()
{
  if(is_initialized_) return;

  gst_init(NULL,NULL);
  is_initialized_=true;
  
  setlocale(LC_NUMERIC, "C"); 
}


void FileWriteGst::freeRecBuffer(void *data)
{
  if(data) delete[] (unsigned char*)data;
}


/// Tells us to register our functionality to an engine kernel
void registerPlugin(VIOKernel &K)
{
  K.getFileWriteServer().addPlugin(
    auto_ptr<FileWrite>(new FileWriteGst()));
}
