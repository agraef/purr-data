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

#ifndef INCLUDE_PIX_FILE_WRITE_H_
#define INCLUDE_PIX_FILE_WRITE_H_

#include "Base/GemBase.h"
#include "Base/GemPixUtil.h"
#include "VIOKernel.h"

/*-----------------------------------------------------------------
-------------------------------------------------------------------
CLASS
    pix_file_write
    
    Writes a pix with the VideoIO Framework
-----------------------------------------------------------------*/

class GEM_EXTERN pix_file_write : public GemBase
{
    CPPEXTERN_HEADER(pix_file_write, GemBase)

    public:

        //////////
        // Constructor
    	pix_file_write(t_symbol *filename);
    	
    protected:
    	
    	//////////
    	// Destructor
    	virtual ~pix_file_write();

    	//////////
    	// stop recording and close file
    	virtual void closeFile(void);

    	//////////
    	// Set the filename and filetype
    	virtual void openFile(t_symbol *filename);
		
    	//////////
    	// Do the rendering
    	virtual void render(GemState *state);
		
    private:
      
        // the file reader
        VideoIO_::FileWrite *fileWriter;
        VideoIO_::VIOKernel m_kernel;
    	
        //////////////
        // helper vars
        
        // stores the current frame
        VideoIO_::VIOFrame m_frame;
        
        // true if we should push frames
        bool m_recording;
        
        // first frame of the recording, used
        // to initialize the format
        bool m_first_time;
        
    	//////////
    	// static member callback functions
        static void openMessCallback(void *data, t_symbol *s);
        static void startCallback(void *data, t_floatarg start);
        static void stopCallback(void *data, t_floatarg stop);
        static void setCodecCallback(void *data, t_symbol *s, int argc, t_atom *argv);
        static void getCodecCallback(void *data);
};

#endif  // for header file
