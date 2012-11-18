//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   
//
//   VIOPlugin
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

#ifndef VIOPLUGIN_H_
#define VIOPLUGIN_H_

#include <string>
#include <dlfcn.h> /// TODO Achtung, nur für Linux, d.h. je nach
                   ///      Plattform dann unterschiedlich handeln
#include "VIOUtils.h"

using namespace std;



namespace VideoIO_
{
  const int engineVersion = 1;

  class VIOKernel;
  
  /// Representation of a plugin
  class VIOPlugin {
    public:
      /// Initialize and load plugin
      VIOPlugin(const string &filename);
      /// Copy existing plugin instance
      VIOPlugin(const VIOPlugin &other);
      /// Unload a plugin
      virtual ~VIOPlugin();
  
    //
    // Plugin implementation
    //
    public:
      /// Query the plugin for its expected engine version
      int getEngineVersion();
  
      /// Register the plugin to a kernel
      void registerPlugin(VIOKernel &K);
      
    private:
      /// Too lazy for this now...
      VIOPlugin &operator =(const VIOPlugin &Other)
      { return *this; }
  
      /// Signature for the version query function
      typedef int  fnGetEngineVersion();
      /// Signature for the plugin's registration function
      typedef void fnRegisterPlugin(VIOKernel &);
  
      void *handle_; /// TODO nur Linux
      
      //HMODULE             h_dll_;                ///< Win32 DLL handle
      size_t             *dll_ref_count_;        ///< Number of references to the DLL
      fnGetEngineVersion *pfn_get_engine_version_; ///< Version query function
      fnRegisterPlugin   *pfn_register_plugin_;   ///< Plugin registration function
  };
}
#endif
