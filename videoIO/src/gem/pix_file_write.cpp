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

#include "pix_file_write.h"

#include "Base/GemMan.h"

CPPEXTERN_NEW_WITH_ONE_ARG(pix_file_write, t_symbol *, A_DEFSYM)

pix_file_write :: pix_file_write(t_symbol *filename):
    fileWriter(NULL), 
    m_recording(false), 
    m_first_time(false)
{
  if(gensym("")!=filename) 
    openFile(filename);
}

pix_file_write :: ~pix_file_write()
{
  closeFile();
}
void pix_file_write :: closeFile(void)
{
  m_recording = false;
  if(fileWriter) {
    fileWriter->stopRecording();
    delete fileWriter;
  }
  fileWriter=NULL;
}

void pix_file_write :: openFile(t_symbol *filename)
{
  closeFile();

  // make the right filename
  char tmp_buff[MAXPDSTRING];
  char *path=tmp_buff;
  canvas_makefilename(getCanvas(), filename->s_name, tmp_buff, MAXPDSTRING);
  if (FILE*fd=fopen(tmp_buff, "r")) fclose(fd);
  else path=filename->s_name;

  // get the FileWrite plugin
  // TODO: the fileWriter should be returned based on the <file>
  // and not the other way round...
  fileWriter = m_kernel.getFileWriteServer().getPlugin();

  if(fileWriter) {
    // open file
    if( !fileWriter->openFile(path) )
      {
        error("could not open file %s", path);
        closeFile();
        return;
      }
  } else {
    error("couldn't find (suitable) VideoIO plugin for writing '%s'!", path);
    return;
  }

}

void pix_file_write :: render(GemState *state)
{
  if(!fileWriter ) return;
  if(!m_recording) return;
  if(!state || !state->image)return;
  
  imageStruct *im = &state->image->image;

  if(im)return;
  
  if( m_first_time )
  {
    // get format data from GEM
    int xsize = im->xsize;
    int ysize = im->ysize;
    ///TODO if no movie is loaded to play and you start recording
    /// and create the gemwin it gets segmentation fault here
    int format;

    switch(im->format)
    {
      case GL_LUMINANCE:
        format = VideoIO_::GRAY;
        break;

      case GL_YCBCR_422_GEM:
        format = VideoIO_::YUV422;
        break;
        
      case GL_RGB:
        format = VideoIO_::RGB;
        break;
    
      case GL_RGBA:
      default:
        format = VideoIO_::RGBA;
        break;
    }

    post("writing to video file ...");

    // set frame size
    m_frame.setFrameSize(xsize, ysize, format);

    float framerate = GemMan::getFramerate();
    fileWriter->setFramerate( framerate );

    m_first_time = false;
  }
  
  // set data of frame
  m_frame.setFrameData(im->data, m_frame.getXSize(),
                       m_frame.getYSize(), m_frame.getColorSize());

  fileWriter->pushFrame(m_frame);
}


/////////////////////////////////////////////////////////
// static member functions
//
/////////////////////////////////////////////////////////

void pix_file_write :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_file_write::openMessCallback,
		  gensym("open"), A_DEFSYM, A_NULL);

  class_addmethod(classPtr, (t_method)&pix_file_write::startCallback,
                  gensym("start"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_file_write::stopCallback,
                  gensym("stop"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_file_write::setCodecCallback,
		  gensym("codec"), A_GIMME, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_file_write::getCodecCallback,
		  gensym("getCodec"), A_NULL);
}

void pix_file_write :: openMessCallback(void *data, t_symbol*s)
{
  GetMyClass(data)->openFile(s);
}

void pix_file_write :: startCallback(void *data, t_floatarg start)
{
  printf("VideoIO: start recording");
  GetMyClass(data)->m_recording = true;
  GetMyClass(data)->m_first_time = true;
}

void pix_file_write :: stopCallback(void *data, t_floatarg stop)
{
  printf("VideoIO: stopped recording");
  GetMyClass(data)->m_recording = false;
  GetMyClass(data)->closeFile();
}

void pix_file_write :: setCodecCallback(void *data, t_symbol *s, int argc, t_atom *argv)
{
  GetMyClass(data)->fileWriter->setCodec(argc,argv); 
}

void pix_file_write :: getCodecCallback(void *data)
{
  GetMyClass(data)->fileWriter->getCodec(); 
}
