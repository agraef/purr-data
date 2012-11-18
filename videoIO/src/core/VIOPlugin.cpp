//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   
//
//   VIOPlugin
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

#include "VIOPlugin.h"


namespace VideoIO_
{
  VIOPlugin::VIOPlugin(const string &filename):
      handle_(0),
      dll_ref_count_(0),
      pfn_get_engine_version_(0),
      pfn_register_plugin_(0)
  {
    handle_ = dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    
    if(handle_ == NULL)
    {
      error("VideoIO Plugin Error: open/load error of dynamic.so failed: %s", dlerror());
      throw "error in initialization";
    }
    else
    {
      pfn_register_plugin_ = reinterpret_cast<fnRegisterPlugin *>(
                           dlsym(handle_, "registerPlugin") );
    
      // Initialize a new DLL reference counter
      dll_ref_count_ = new size_t(1);
    }
  }
  
  VIOPlugin::VIOPlugin(const VIOPlugin &other) :
      handle_(other.handle_),
      dll_ref_count_(other.dll_ref_count_),
      pfn_get_engine_version_(other.pfn_get_engine_version_),
      pfn_register_plugin_(other.pfn_register_plugin_)
  {
    ++*dll_ref_count_;
  }
  
  VIOPlugin::~VIOPlugin()
  {
    if(!--*dll_ref_count_)
    {
      delete dll_ref_count_;
      /// TODO die dyn library unter linux wieder freigeben
    }
    
    
  // Only unload the DLL if there are no more references to it
  //if(!--*m_pDLLRefCount) {
  //  delete m_pDLLRefCount;
  //  ::FreeLibrary(m_hDLL);
  //}
  }
  
  int VIOPlugin::getEngineVersion()
  {
    /// TODO
    return -1;
  }
  
  void VIOPlugin::registerPlugin(VIOKernel &K)
  {
    //pfn_register_plugin_(K);
    
    pfn_register_plugin_ = reinterpret_cast<fnRegisterPlugin *>(
                           dlsym(handle_, "registerPlugin") );
    pfn_register_plugin_(K);
  }
  
}
