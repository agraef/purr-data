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

#include "pix_file_read.h"
#include <ctype.h>

#include "Base/GemMan.h"

#include <stdio.h>

#include <typeinfo>

CPPEXTERN_NEW_WITH_ONE_ARG(pix_file_read, t_symbol *, A_DEFSYM)

/////////////////////////////////////////////////////////
//
// pix_file_read
//
/////////////////////////////////////////////////////////
// Constructor
//
/////////////////////////////////////////////////////////
pix_file_read :: pix_file_read(t_symbol *filename) :
    fileReader(NULL), m_newfilm(false), m_already_banged(true)
{
  // create audio outlets
  m_outAudio[0]=outlet_new(this->x_obj, &s_signal);
  m_outAudio[1]=outlet_new(this->x_obj, &s_signal);

  // create outlet for frame data and bang at the end
  m_outNumFrames = outlet_new(this->x_obj, 0);
  m_outEnd       = outlet_new(this->x_obj, 0);

  if(gensym("")!=filename) 
    openFile(filename);
}

/////////////////////////////////////////////////////////
// Destructor
//
/////////////////////////////////////////////////////////
pix_file_read :: ~pix_file_read()
{
  // Clean up the movie
  closeFile();

  outlet_free(m_outAudio[0]);
  outlet_free(m_outAudio[1]);
  outlet_free(m_outNumFrames);
  outlet_free(m_outEnd);
}

/////////////////////////////////////////////////////////
// closeFile
//
/////////////////////////////////////////////////////////
void pix_file_read :: closeFile(void)
{
  if(fileReader) {
    fileReader->closeFile();
    delete fileReader;
  }
  fileReader=NULL;
}

/////////////////////////////////////////////////////////
// openFile
//
/////////////////////////////////////////////////////////
void pix_file_read :: openFile(t_symbol *filename)
{
  closeFile();

  // make the right filename
  char tmp_buff[MAXPDSTRING];
  char *path=tmp_buff;
  canvas_makefilename(getCanvas(), filename->s_name, tmp_buff, MAXPDSTRING);
  if (FILE*fd=fopen(tmp_buff, "r")) fclose(fd);
  else path=filename->s_name;

  // TODO: actually the fileReader should be returned based on the <file>
  // and not the other way round...

  fileReader = m_kernel.getFileReadServer().getPlugin();

  if(fileReader) {
    // get GEM framerate
    fileReader->setHostFramerate( GemMan::getFramerate() );

    // open file
    if(!(fileReader->openFile(path)))
      {
        error("could not open file %s", path);
        closeFile();
        return;
      }
    reallocate_m_image();
    infoSize();
  } else {
    error("couldn't find (suitable) VideoIO plugin for reading '%s'", path);
    return;
  }
}

/////////////////////////////////////////////////////////
// DSP Message
//
/////////////////////////////////////////////////////////
void pix_file_read :: dspMess(void *data, t_signal** sp)
{
  // TODO: allow other formats as stereo!
  dsp_add(perform, 4, data, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

/////////////////////////////////////////////////////////
// DSP Loop
//
/////////////////////////////////////////////////////////
t_int* pix_file_read :: perform(t_int* w)
{
  pix_file_read *x = GetMyClass((void*)w[1]);

  // stereo only for now
  t_float* left = (t_float*)(w[2]);
  t_float* right = (t_float*)(w[3]);
  int N = (t_int)(w[4]);

  if(x->fileReader) {
    x->fileReader->processAudioBlock(left, right, N);
  } else {
    while(N-->=0) {
      *left++  = 0.;
      *right++ = 0.;
    }
  }

  return (w+5);
}

/////////////////////////////////////////////////////////
// render
//
// TODO: check what georg had in mind with the framesize things
// TODO: fix the re-allocate thing
/////////////////////////////////////////////////////////
void pix_file_read :: render(GemState *state)
{
  if(!fileReader) return;


  // set Frame Data
  unsigned char *image_ptr = m_image.image.data;

  fileReader->setFrameData(image_ptr, 
                           m_image.image.xsize, m_image.image.ysize, 
                           m_image.image.csize);

  // read frame data into m_image
  int status = fileReader->processFrameData();

  if( status == VideoIO_::VIDEO_STOPPED )
  {
    // output end of video bang in playing mode
    // and stop video
    if(!m_already_banged)
    {
      outlet_bang(m_outEnd);
      m_already_banged = true;
    }
    return;
  }

  if( status == VideoIO_::VIDEO_SIZE_CHANGED )
  {
    // check if image size changed
    if( m_image.image.xsize != fileReader->getWidth() ||
      m_image.image.ysize != fileReader->getHeight() ||
      m_image.image.csize != fileReader->getColorSize() ) 
      reallocate_m_image();

    infoSize();
    // process frame with new size again
    fileReader->processFrameData();
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

/////////////////////////////////////////////////////////
// postrender
//
/////////////////////////////////////////////////////////
void pix_file_read :: postrender(GemState *state)
{
  if (state && state->image)
    state->image->newimage = 0;
}

/////////////////////////////////////////////////////////
// forceColorspace
//
/////////////////////////////////////////////////////////
void pix_file_read :: forceColorspace(t_symbol *cs)
{
  char c =*cs->s_name;
  if(!fileReader)return;

  switch (c)
  {
    case 'g': case 'G':
      fileReader->forceColorspace(VideoIO_::GRAY);
      break;
    case 'y': case 'Y':
      fileReader->forceColorspace(VideoIO_::YUV422);
      break;
    case 'r': case 'R': 
      if(gensym("RGB")==cs||gensym("rgb")==cs)
        fileReader->forceColorspace(VideoIO_::RGB);
      else
        fileReader->forceColorspace(VideoIO_::RGBA);
      break;
    default:
      error("colorspace must be 'RGBA', 'YUV' or 'Gray'");
  }
}

/////////////////////////////////////////////////////////
// reallocate_m_image
//
/////////////////////////////////////////////////////////
void pix_file_read :: reallocate_m_image()
{
  // allocate memory for m_image
  if(!fileReader)return;
  m_image.image.xsize = fileReader->getWidth();
  m_image.image.ysize = fileReader->getHeight();

  switch( fileReader->getColorspace() )
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


void pix_file_read :: infoSize(void)
{
  t_atom ap[4];
  if(!fileReader)return;
  SETFLOAT(ap, fileReader->getDuration() );
  SETFLOAT(ap+1, fileReader->getWidth() );
  SETFLOAT(ap+2, fileReader->getHeight() );
  SETFLOAT(ap+3, fileReader->getFPS() );
    
  post("loaded file with %f msec (%dx%d) at %f fps", 
       fileReader->getDuration(), 
       fileReader->getWidth(), 
       fileReader->getHeight(), 
       (float)fileReader->getFPS());
  outlet_list(m_outNumFrames, 0, 4, ap);
}


/////////////////////////////////////////////////////////
// static member function
//
/////////////////////////////////////////////////////////
void pix_file_read :: obj_setupCallback(t_class *classPtr)
{
  class_addmethod(classPtr, (t_method)&pix_file_read::openMessCallback,
                  gensym("open"), A_DEFSYM, A_NULL);

  class_addmethod(classPtr, (t_method)&pix_file_read::startCallback,
                  gensym("start"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_file_read::stopCallback,
                  gensym("stop"), A_DEFFLOAT, A_NULL);

  class_addmethod(classPtr, (t_method)&pix_file_read::seekCallback,
                  gensym("seek"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_file_read::speedCallback,
                  gensym("speed"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_file_read::csCallback,
                  gensym("forceColorspace"), A_DEFSYM, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_file_read::atrackCallback,
                  gensym("audioTrack"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_file_read::vtrackCallback,
                  gensym("videoTrack"), A_DEFFLOAT, A_NULL);
  class_addmethod(classPtr, (t_method)&pix_file_read::audioIOCallback,
                  gensym("audio"), A_DEFFLOAT, A_NULL);

  class_addmethod(classPtr, (t_method)&pix_file_read::dspMessCallback,
		  gensym("dsp"), A_NULL);
}

void pix_file_read :: openMessCallback(void *data, t_symbol*s)
{
  GetMyClass(data)->openFile(s);
}

void pix_file_read :: startCallback(void *data, t_floatarg start)
{
  if(!(GetMyClass(data)->fileReader))return;
  GetMyClass(data)->fileReader->startVideo();
  GetMyClass(data)->m_already_banged=false;
}

void pix_file_read :: stopCallback(void *data, t_floatarg stop)
{
  if(!(GetMyClass(data)->fileReader))return;
  GetMyClass(data)->fileReader->stopVideo();
  GetMyClass(data)->m_already_banged=true;
}

void pix_file_read :: seekCallback(void *data, t_floatarg seek)
{
  if(!(GetMyClass(data)->fileReader))return;
  GetMyClass(data)->fileReader->setPosition( seek );
}

void pix_file_read :: speedCallback(void *data, t_floatarg speed)
{
  if(!(GetMyClass(data)->fileReader))return;
  GetMyClass(data)->fileReader->setSpeed(speed);
}

void pix_file_read :: csCallback(void *data, t_symbol *s)
{
  if(!(GetMyClass(data)->fileReader))return;
  GetMyClass(data)->forceColorspace(s);
}

void pix_file_read :: atrackCallback(void *data, t_floatarg track)
{
  if(!(GetMyClass(data)->fileReader))return;
  GetMyClass(data)->fileReader->setAudioTrack((int)track);
}

void pix_file_read :: vtrackCallback(void *data, t_floatarg track)
{
  if(!(GetMyClass(data)->fileReader))return;
  GetMyClass(data)->fileReader->setVideoTrack((int)track);
}

void pix_file_read :: audioIOCallback(void *data, t_floatarg io)
{
  if(!(GetMyClass(data)->fileReader))return;
  if( io == 0 )
    GetMyClass(data)->fileReader->setAudioIO(false);
  else
    GetMyClass(data)->fileReader->setAudioIO(true);
}

void pix_file_read :: dspMessCallback(void *data, t_signal **sp)
{
  GetMyClass(data)->dspMess(data, sp);
}
