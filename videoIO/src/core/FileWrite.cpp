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

#include "FileWrite.h"

// NOTE: don't remove this (small) cpp file, because
// we need an object generated out of the cpp

namespace VideoIO_
{

  FileWrite::FileWrite() :
    framerate_(1)
  {}

  void FileWrite::setFramerate(float fr)
  {
    framerate_ = fr;
  }
  
  void FileWrite::setCodec(int argc, t_atom *argv)
  {
    // get codec
//    post("a\n");
    codec_ = atom_getsymbol(argv)->s_name;
    argc -= 1; argv += 1;
    bool par=false;
    
    // clear current parameters
    cparameters_.clear();

    // parse codec parameters
    while ( argc > 0 &&
            argv->a_type == A_SYMBOL &&
            argv[1].a_type == A_FLOAT )
    {
      cparameters_[atom_getsymbol(argv)->s_name] =
      (int) argv[1].a_w.w_float;
      argc -= 2; argv += 2;
      par=true;
    }

    // give feedback about the current parameters

    if(!par )
    {
      post("");
      post("set %s codec", codec_.c_str());
      post("");
      return;
    }

    map<string, int>::iterator iter;
    iter = cparameters_.begin();
    post("");
    post("set %s codec with parameters:", codec_.c_str());
    while( iter != cparameters_.end() ) 
    {
      post("\t%s: %d", (*iter).first.c_str(), (*iter).second );
      iter++;
    }
    post("");
  }
  
  void FileWrite::getCodec()
  {
    post("videoIO: Sorry, no plugins for file writing avaliable");
  }
}
