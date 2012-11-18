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

#include "FileRead.h"

// ATTENTION: although there is not much code in this file it
// must exist, because we need this object file !

namespace VideoIO_
{
  // init static var
  bool FileRead::audio_ = true;

    FileRead::FileRead() :
      cspace_(-1), vtrack_(1), atrack_(1), duration_(0),
      framerate_(1), fr_host_(1)
    {}
}
