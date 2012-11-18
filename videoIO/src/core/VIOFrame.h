//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   The Frame class
//
//   VIOFrame
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
    
#ifndef VIOFRAME_H_
#define VIOFRAME_H_

using namespace std;

namespace VideoIO_
{
  class VIOUtils;
  
  /*!
  * \class VIOFrame
  * 
  * This class stores a pointer to the frame data and has
  * some more methods needed for the frame.
  */
  class VIOFrame
  {
    public:

    /// constructor
    VIOFrame();
    
    /// destructor
    virtual ~VIOFrame() {}

    /*!
    * sets the frame data pointer
    * @param data pointer to the pixel data
    * @param x_size x-size of the image
    * @param y_size y-size of the image
    * @param color_size size of one pixel in bytes
    *        (e.g. gray=1, yuv422=2, rgb=3, rgba=4, ...)
    */
    inline void setFrameData(unsigned char *data, int x_size,
                             int y_size, int color_size)
    {
      data_=data; 
      x_size_=x_size; 
      y_size_=y_size; 
      color_size_=color_size;
      setColorSpace(color_size);
    }

    /*!
    * sets the size without frame data pointer
    * @param x_size x-size of the image
    * @param y_size y-size of the image
    * @param format color space define
    */
    inline void setFrameSize(int x_size, int y_size, int format)
    {
      x_size_=x_size; 
      y_size_=y_size;
      color_space_=format;
      setColorSize(format);
    }

    /// @return a pointer to the frame data
    inline unsigned char *getFrameData()
    { return data_; }

    /*!
    * @param x the x coordinate
    * @param y the y coordinate
    * @param color the colorspace (e.g. GRAY, RGB, YUV422, ...)
    * @return the pixel data
    */
    inline unsigned char getPixel(int x, int y, int color)
    { return data_[y * x_size_ * color_size_ + x * color_size_ + color]; }
    
    /*!
    * sets a pixel
    * @param x the x coordinate
    * @param y the y coordinate
    * @param color the colorspace
    * @param value the value to set
    */
    inline void setPixel(int x, int y, int color, unsigned char value)
    { data_[y * x_size_ * color_size_ + x * color_size_ + color] = value; }
      
    /// @return the x size
    inline int getXSize()
    { return x_size_; }
    
    /// @return the y size
    inline int getYSize()
    { return y_size_; }
    
    /// @return the colorsize
    inline int getColorSize()
    { return color_size_; }

    /// @return the colorspace define
    inline int getColorspace()
    { return color_space_; }
    
  protected:

    /*!
    * sets the color_size_ variable
    * @param format the colorspace format
    */
    void setColorSize(int format);

    /*!
    * sets the color_space_ variable
    * @param color_size size of one pixel in bytes
    */
    void setColorSpace(int color_size);
    
    /// the frame data
    unsigned char *data_;
    
    int x_size_;
    int y_size_;
    int color_size_;
    int color_space_;
    
  };
}

#endif
