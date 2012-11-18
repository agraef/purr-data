//////////////////////////////////////////////////////////////////////////
//
//   VideoIO-Framework for GEM/PD
//
//   The server of the FileRead plugins.
//
//   PluginServer
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

#ifndef PLUGIN_SERVER_H_
#define PLUGIN_SERVER_H_

#include <vector>
#include <iostream>
#include <memory>

using namespace std;

namespace VideoIO_
{
  /*!
   * \class PluginServer
   *
   * This is the server of the plugins.
   */
  template<class PluginType> class PluginServer
  {
   public:
    
    /// constructor
    PluginServer(){};
    
    /// destructor
    virtual ~PluginServer();

    /*!
    * adds a plugin
    */
    void addPlugin(auto_ptr<PluginType> plug)
    { plugins_.push_back( plug.release() ); }

    /*!
    * @param index the index of the plugin
    * @return the plugin with the choosen index
    */
    PluginType *getPlugin(int index=0);
    
    /// @return the Nr. of plugins
    int getPluginCount()
    { return plugins_.size(); }

   protected:

    /// the map which holds all the file read plugins
    vector<PluginType *> plugins_;
  };

  // Implementations
  template<class PluginType>
  PluginServer<PluginType>::~PluginServer()
  {
    for(unsigned int i=0; i<plugins_.size(); ++i)
    {
      delete plugins_[i];
    }
  }

  template<class PluginType>
  PluginType *PluginServer<PluginType>::getPlugin(int index)
  {
    /// TODO bessere priorities der einzelnen plugins machen
    /// d.h. nicht in einem vector speichern, sondern irgendwo
    /// anders und eine Liste fuer welchen mimetype welches
    /// plugin am besten genommen werden soll!

    if( plugins_.size() == 0 )
    {
      /// TODO throw also exception here so that the program
      ///      doesnt crash !
      cerr << "No VideoIO Plugin found! "
           << "Be sure that they are in the right path "
           << "(/usr/lib/videoIO) !\n";

      return NULL;
    }

    return plugins_.at(index)->clone();
  }

}

#endif
