/*-----------------------------------------------------------------

    GEM - Graphics Environment for Multimedia

    Interface to the VideoIO framework
    this object is for video file reading
    
    Copyright (c) 2007 Thomas Holzmann, Georg Holzmann
    For information on usage and redistribution, and for a DISCLAIMER OF ALL
    WARRANTIES, see the file, "GEM.LICENSE.TERMS" in this distribution.

    -----------------------------------------------------------------*/

#ifndef INCLUDE_PIX_FILEREAD_H_
#define INCLUDE_PIX_FILEREAD_H_
#include "Base/config.h"

#define MAX_FILM_HANDLES 8

#include <stdio.h>

#include "Base/GemBase.h"
#include "Base/GemPixUtil.h"
#include "VIOKernel.h"

/*-----------------------------------------------------------------
    
  CLASS
    pix_file_read
    
    Loads in a movie with the videoIO framework
    
    KEYWORDS
    pix
    
    DESCRIPTION

-----------------------------------------------------------------*/
class GEM_EXTERN pix_file_read : public GemBase
{
  CPPEXTERN_HEADER(pix_file_read, GemBase)
    
  public:  
    
    //////////
    // Constructor
    pix_file_read(t_symbol *filename);

  protected:
    
    //////////
    // Destructor
    virtual ~pix_file_read();

    //////////
    // close the movie file
    virtual void closeFile(void);
    
    //////////
    // open a movie up
    virtual void openFile(t_symbol *filename);

    //////////
    // DSP-Message
    virtual void dspMess(void *data, t_signal** sp);

    //////////
    // DSP perform
    static t_int* perform(t_int* w);

    //////////
    // Do the rendering
    virtual void render(GemState *state);

    //////////
    // Clear the dirty flag on the pixBlock
    virtual void postrender(GemState *state);

    //////////
    // force a specific colorspace
    virtual void forceColorspace(t_symbol *cs);

  //-----------------------------------
  // GROUP:	Movie data
  //-----------------------------------

    //////////
    // a outlet for information like #frames and "reached end"
    t_outlet     *m_outNumFrames;
    t_outlet     *m_outEnd;
    // the audio outlets~
    t_outlet *m_outAudio[2];

    // the file reader
    VideoIO_::FileRead *fileReader;
    VideoIO_::VIOKernel m_kernel;
    
    // here the frame is stored
    pixBlock m_image;
    // true if we loaded a new film
    bool m_newfilm;
    // true if stopband already sent
    bool m_already_banged;
 
  protected:
    
    // reallocate frame data
    void reallocate_m_image(void);

    void infoSize(void);
    
    //////////
    // static member functions
    static void openMessCallback(void *data, t_symbol *s);
    static void startCallback(void *data, t_floatarg start);
    static void stopCallback(void *data, t_floatarg stop);
    static void seekCallback(void *data, t_floatarg seek);
    static void speedCallback(void *data, t_floatarg speed);
    static void csCallback(void *data, t_symbol *s);
    static void atrackCallback(void *data, t_floatarg track);
    static void vtrackCallback(void *data, t_floatarg track);
    static void audioIOCallback(void *data, t_floatarg io);

    // audio callback
    static void dspMessCallback(void *data,t_signal **sp);
};

#endif	// for header file
