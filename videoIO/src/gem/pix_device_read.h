////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// Header file
//
// Copyright (c) 2007, Thomas Holzmann, Georg Holzmann
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#ifndef INCLUDE_PIX_DEVICE_READ_H_
#define INCLUDE_PIX_DEVICE_READ_H_

#include "Base/GemBase.h"
#include "Base/GemPixUtil.h"
#include "VIOKernel.h"

/*-----------------------------------------------------------------
  CLASS
  pix_device_read
    
  reads video stream from a device (dv camera, web camera, ...)
    
  KEYWORDS
  pix
  -----------------------------------------------------------------*/
class GEM_EXTERN pix_device_read : public GemBase
{
  CPPEXTERN_HEADER(pix_device_read, GemBase)
    
 public:

  //////////
  // Constructor
  pix_device_read();
  
 protected:
    	
  //////////
  // Destructor
  virtual ~pix_device_read();

  //////////
  // Do the rendering
  virtual void 	render(GemState *state);

  //////////
  // Clear the dirty flag on the pixBlock
  virtual void 	postrender(GemState *state);

  //////////
  // Opens the specified device
  virtual void	openDevice(t_symbol *name, t_symbol *dev);

  //////////
  // If you care about the stop of rendering
  virtual void	closeDevice();

  //////////
  // force a specific colorspace
  virtual void forceColorspace(t_symbol *cs);

  //-----------------------------------
  // GROUP:	Video data
  //-----------------------------------
    
  pixBlock m_image;
  
  // the device reader
  VideoIO_::DeviceRead *m_deviceReader;
  VideoIO_::VIOKernel m_kernel;
  
 private:
   
  // reallocate frame data
  void reallocate_m_image();
  // true if we loaded a new device
  bool m_newfilm;

  //////////
  // static member functions
  static void openMessCallback(void *data, t_symbol *s, int argc, t_atom *argv);
  static void startCallback(void *data, t_floatarg start);
  static void stopCallback(void *data, t_floatarg stop);
  static void seekCallback(void *data, t_floatarg seek);
  static void csCallback(void *data, t_symbol*cs);
  static void setDVQualityCallback(void *data, t_floatarg qual);
};

#endif	// for header file
