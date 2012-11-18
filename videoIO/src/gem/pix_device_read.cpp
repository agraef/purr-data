////////////////////////////////////////////////////////
//
// GEM - Graphics Environment for Multimedia
//
// Implementation file
//
// Copyright (c) 2007, Thomas Holzmann, Georg Holzmann
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.
//
/////////////////////////////////////////////////////////

#include "pix_device_read.h"

#include "Base/GemMan.h"

CPPEXTERN_NEW(pix_device_read)

pix_device_read :: pix_device_read() :
  m_deviceReader(NULL), m_newfilm(false)
{

}

pix_device_read :: ~pix_device_read()
{
  closeDevice();
}

void pix_device_read :: render(GemState *state)
{
  if(!m_deviceReader)return;


  // read frame data into m_image
  unsigned char *image_ptr = m_image.image.data;

  // set Frame Data
  m_deviceReader->setFrameData(image_ptr, m_image.image.xsize,
                           m_image.image.ysize, m_image.image.csize);

  int status = m_deviceReader->processFrameData();

  if( status == VideoIO_::VIDEO_STOPPED )
    return;

  if( status == VideoIO_::VIDEO_SIZE_CHANGED )
  {
    // check if image size changed
    if( m_image.image.xsize != m_deviceReader->getWidth() ||
      m_image.image.ysize != m_deviceReader->getHeight() ||
      m_image.image.csize != m_deviceReader->getColorSize() )
      reallocate_m_image();

    // process frame with new size again
    m_deviceReader->processFrameData();
  }

  // set flag if we have a new film
  if(m_newfilm)
  {
    m_image.newfilm = true;
    m_newfilm = false;
  }
  m_image.newimage = true;

  // set image
  state->image = &m_image;
}

void pix_device_read :: openDevice(t_symbol *name, t_symbol *dev)
{
  closeDevice();

  bool suc=false;
  // TODO: actually the deviceReader should be returned based on the <device>
  // and not the other way round... 
  m_deviceReader = m_kernel.getDeviceReadServer().getPlugin();

  if(m_deviceReader) {

    // get GEM framerate
    m_deviceReader->setFramerate( GemMan::getFramerate() );

    // open device
    if( dev )
      suc = m_deviceReader->openDevice(name->s_name, dev->s_name);
    else
      suc = m_deviceReader->openDevice(name->s_name);
    
    if( !suc ) {
      error("could not open device %s", dev->s_name);
      closeDevice();
      return;
    }
  } else {
    error("unable to find (suitable) VideoIO plugin for device '%s:%s'", name?(name->s_name):NULL, dev?(dev->s_name):NULL);
    return;
  }
}

void pix_device_read :: closeDevice()
{
  if(m_deviceReader) {
    m_deviceReader->closeDevice();
    delete m_deviceReader;
  }
  m_deviceReader=NULL;

}

void pix_device_read :: postrender(GemState *state)
{
  if (state && state->image)
    state->image->newimage = 0;
}

void pix_device_read :: forceColorspace(t_symbol *cs)
{
  char c =*cs->s_name;
  if(!m_deviceReader) {
    error("you have to open a device first");
    return;
  }
  switch (c)
  {
    case 'g': case 'G':
      m_deviceReader->forceColorspace(VideoIO_::GRAY);
      break;
    case 'y': case 'Y':
      m_deviceReader->forceColorspace(VideoIO_::YUV422);
      break;
    case 'r': case 'R': 
      if(gensym("RGB")==cs||gensym("rgb")==cs)
        m_deviceReader->forceColorspace(VideoIO_::RGB);
      else
        m_deviceReader->forceColorspace(VideoIO_::RGBA);
      break;
    default:
      error("colorspace must be 'RGBA', 'YUV' or 'Gray'");
  }
}

void pix_device_read :: reallocate_m_image()
{
  if(!m_deviceReader) {
    error("you have to open a device first");
    return;
  }

  // allocate memory for m_image
  m_image.image.xsize = m_deviceReader->getWidth();
  m_image.image.ysize = m_deviceReader->getHeight();

  switch( m_deviceReader->getColorspace() )
  {
    case VideoIO_::GRAY:
      m_image.image.setCsizeByFormat(GL_LUMINANCE);
      break;
    case VideoIO_::YUV422:
      m_image.image.setCsizeByFormat(GL_YCBCR_422_GEM);
      break;
    case VideoIO_::RGB:
      m_image.image.setCsizeByFormat(GL_RGB);
      break;
    case VideoIO_::RGBA:
      m_image.image.setCsizeByFormat(GL_RGBA);
      break;
    default:
      error("error in reallocate_m_image");
  }

  m_image.image.reallocate();
  m_newfilm = true;
}


/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_device_read :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_device_read::openMessCallback,
              gensym("open"), A_GIMME, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_device_read::startCallback,
              gensym("start"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_device_read::stopCallback,
              gensym("stop"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_device_read::seekCallback,
              gensym("seek"), A_DEFFLOAT, A_NULL);

  class_addmethod(classPtr, (t_method)&pix_device_read::csCallback,
              gensym("forceColorspace"), A_DEFSYM, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_device_read::setDVQualityCallback,
              gensym("quality"), A_FLOAT, A_NULL);
}

void pix_device_read :: openMessCallback(void *data, t_symbol *s, int argc, t_atom*argv)
{
  switch(argc)
  {
    case 2:
      GetMyClass(data)->openDevice( atom_getsymbol(argv),
                                    atom_getsymbol(argv+1) );
      break;
    case 1:
      GetMyClass(data)->openDevice( atom_getsymbol(argv), 0 );
      break;
    default:
      GetMyClass(data)->error("openDevice name [device]");
  }
}

void pix_device_read :: startCallback(void *data, t_floatarg start)
{
  if(!GetMyClass(data)->m_deviceReader) {
    GetMyClass(data)->error("you have to open a device first");
    return;
  }
  GetMyClass(data)->m_deviceReader->startDevice();
}

void pix_device_read :: stopCallback(void *data, t_floatarg stop)
{
  if(!GetMyClass(data)->m_deviceReader) {
    GetMyClass(data)->error("you have to open a device first");
    return;
  }
  GetMyClass(data)->m_deviceReader->stopDevice();
}

void pix_device_read :: seekCallback(void *data, t_floatarg seek)
{
  if(!GetMyClass(data)->m_deviceReader) {
    GetMyClass(data)->error("you have to open a device first");
    return;
  }
  GetMyClass(data)->m_deviceReader->seekDevice( (int)seek );
}

void pix_device_read :: csCallback(void *data, t_symbol *s)
{
  GetMyClass(data)->forceColorspace(s);
}

void pix_device_read :: setDVQualityCallback(void *data, t_floatarg qual)
{
  if(!GetMyClass(data)->m_deviceReader) {
    GetMyClass(data)->error("you have to open a device first");
    return;
  }
  GetMyClass(data)->m_deviceReader->setDVQuality( (int)qual );
}
