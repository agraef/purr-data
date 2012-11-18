//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   The kernel of the plugin loader.
//
//   VIOKernel
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

#include "VIOKernel.h"
#include <fstream>

#ifdef _WIN32
#define _WIN32_WINNT 0x0400
# include <windows.h>
# include <stdio.h>
#else
# include <stdlib.h>
# include <glob.h>
#endif



using namespace std;

namespace VideoIO_
{
  // init static variables
  PluginMap VIOKernel::loaded_plugins_;
  PluginServer<FileRead> VIOKernel::file_read_server_;
  PluginServer<FileWrite> VIOKernel::file_write_server_;
  PluginServer<DeviceRead> VIOKernel::device_read_server_;
  PathList VIOKernel::search_path_;
  bool VIOKernel::first_time_ = true;


  VIOKernel::VIOKernel()
  {
    if(first_time_)
    {
      // add standard relative path
      addSearchPath(".");
      /// TODO naechster pfad jetzt nur zum Testen, dann wieder wegtun,
      /// weil sie e eher im gleichen pfad wie gem sein sollen ?
      addSearchPath("videoIO/plugins");

      // add standard system path
      /// TODO schauen ob man diese Standard Systempfade
      /// ueberhaupt hinzufügen soll
      /// TODO unter Windows/OSX sind die natürlich auch anders
      addSearchPath("/usr/lib/videoIO");
      addSearchPath("/usr/local/lib/videoIO");

      // load plugins
      loadPlugins();
    }
    else first_time_ = false;
  }

  void VIOKernel::loadPlugins()
  {
#ifdef _WIN32
#  error no globbing on W32 yet
    // see hcs/folder_list for how to do it...
#else
    PathList::iterator iter;
    string pattern;

    for( iter = search_path_.begin(); iter != search_path_.end(); iter++ )  {
      unsigned int i;
      glob_t glob_buffer;
      pattern = *iter + "/*"+PLUGIN_FILE_EXTENSION;

      switch( glob( pattern.c_str() , GLOB_TILDE, NULL, &glob_buffer ) ) {
      default:
        break;
      }
      for(i = 0; i < glob_buffer.gl_pathc; i++) {
        registerPlugin(glob_buffer.gl_pathv[i]);
      }
      globfree( &(glob_buffer) );
    }
#endif
  }

  void VIOKernel::addSearchPath(const string &path)
  {
    search_path_.insert(path);
  }

  void VIOKernel::registerPlugin(const string &name)
  {
    fstream tmp;
    bool file_exists;

    post("VideoIO: registering %s", name.c_str());

      // check if file exists
      tmp.open(name.c_str(),ios::in);
      file_exists = tmp.is_open();
      tmp.close();

//       DEBUG
//       post("testfile: %s - exists: %d", filename.c_str(), file_exists);

      if(!file_exists) return;
      // now try to open plugin
      if( loaded_plugins_.find(name) == loaded_plugins_.end() )
      {
        // exception are the only way to get a error message
        // from the VIOPlugin Constructor
        try
        {
          /* hold on: crash: */

          loaded_plugins_.insert(
              make_pair( name, VIOPlugin(name) )
             ).first->second.registerPlugin(*this);
        }
        catch(const char*)
        {
          error("could not load %s", name.c_str() );
        }
      }

  }

}
