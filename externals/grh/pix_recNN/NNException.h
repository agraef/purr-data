/////////////////////////////////////////////////////////////////////////////
//
// NNDefines.h
//
//   global stuff for all the nets
//
//   header file
//
//   Copyright (c) 2005 Georg Holzmann <grh@gmx.at>
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your option) any later version.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef _INCLUDE_NNDEFINES_NET__
#define _INCLUDE_NNDEFINES_NET__

#include <string>

using std::string;

namespace TheBrain
{

//------------------------------------------------------
/* the exception class for all the neural network stuff
 */
class NNExcept
{
 protected:
  string message_;

 public:
  NNExcept(string message="")
    { message_ = message; }
  virtual ~NNExcept() { }

  virtual string what()
  { return message_; }
};

} // end of namespace NNet

#endif //_INCLUDE_NNDEFINES_NET__

