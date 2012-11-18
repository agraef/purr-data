//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   The base class of the DeviceRead plugins.
//
//   DeviceRead
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

#include "DeviceRead.h"

namespace VideoIO_
{
  DeviceRead::DeviceRead() : cspace_(-1), dv_quality_(5), framerate_(1)
  {
  }

  void DeviceRead::seekDevice(int seek)
  {
    post("videoIO: Sorry, seeking is not supported for your device!");
  }

  void DeviceRead::setDVQuality(int quality)
  {
    dv_quality_ =   (quality<0) ? 0 :
                  ( (quality>5) ? 5 : quality );
  }
}
