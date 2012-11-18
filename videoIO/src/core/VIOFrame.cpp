//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   The Frame class
//
//   VIOFrame
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

#include "VIOFrame.h"


#include "VIOUtils.h"


namespace VideoIO_
{
  VIOFrame::VIOFrame() : data_(0), x_size_(0), y_size_(0),
                         color_size_(0)
  {
  }

  void VIOFrame::setColorSize (int format)
  {
    switch(format)
    {
      case GRAY:  
        color_size_ = 1; 
        break;
        
      case YUV422:
        color_size_ = 2;
        break;
        
      case RGB: 
        color_size_ = 3;
        break;
    
      case RGBA:
      default:
        color_size_ = 4; 
        break;
    }
  }

  void VIOFrame::setColorSpace(int color_size)
  {
    switch(color_size)
    {
      case 1:
        color_space_ = GRAY; 
        break;
        
      case 2:
        color_space_ = YUV422;
        break;
        
      case 3: 
        color_space_ = RGB;
        break;
    
      case 4:
      default:
        color_space_ = RGBA; 
        break;
    }
  }

}
