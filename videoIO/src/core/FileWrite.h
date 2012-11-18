//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   Writes a digital video (like AVI, Mpeg, Quicktime) to the harddisc.
//
//   FileWrite
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

#ifndef FILE_WRITE_PLUGIN_
#define FILE_WRITE_PLUGIN_

using namespace std;

#include "VIOUtils.h"
#include "VIOFrame.h"

#include <map>

namespace VideoIO_
{
  class FileWrite
  {
    public:
    /// constructor
    FileWrite();
    
    /// destructor
    virtual ~FileWrite(){};

    /// clone method, returns a new instance
    /// (virtual constructor idiom)
    virtual FileWrite *clone() const = 0;

    /*!
     * stops recording
     * @return false if file was written
     */
    virtual bool stopRecording() = 0;
    
    /*!
     * writes one frame in the video file
     * @param frame written in video file
     */
    virtual void pushFrame(VIOFrame &frame) = 0;
    
    /*!
     * opens the file at the given path
     * @param filename the path of the file
     * @return true if open worked
     */
    virtual bool openFile(const string &filename) = 0;

    /// set framerate of the video
    void setFramerate(float fr);
    
    /// sets the desired codec
    void setCodec(int argc, t_atom *argv);
    
    /// prints the avaliable codecs
    virtual void getCodec();

  protected:
    
    float framerate_;
    string codec_;

    /// map with codec parameters
    map<string, int> cparameters_;
  };
}

#endif
