#ifndef __FLEXT_DOXYGEN_H
#define __FLEXT_DOXYGEN_H

/*! \file fldoxygen.h
    \brief Doxygen definitions
    \remark There is no code in here, just documentation stuff.
*/

/*! 

\mainpage flext - a C++ layer for cross-platform development of PD and Max/MSP objects

\section INTRO Introduction

Currently there exist two widely used modular systems for real-time audio that can be
extended by self-written objects (so called "externals"):<br>
Max/MSP (http://www.cycling74.com) and Pure Data (http://www.pure-data.org) .

Both come with APIs that are not very different (as they share their origin), but as well not quite the same.
Flext seeks to provide a unifying interface for the APIs of those real-time systems while also
concentrating on making use of the advantages of the object orientation of the C++ language.

Consequently, flext allows to write externals (or libraries of a number of these), that can
be compiled for both systems (with various compilers on a few platforms) without changes to the
source code.
Flext also tries to overcome some limitations of the real-time systems and introduces new features.

The advantages of flext are:
<ul>
<li>Identical source code for PD and Max/MSP objects on a number of platforms
<li>Better readability of code compared to straight C externals
<li>Faster development, more robust coding
<li>Sharing of common methods and data by using base classes
<li>Transparent use of threads for methods
<li>Libraries of externals in Max/MSP
<li>More than 3 typed creation arguments possible for Max/MSP
<li>Any input to any object's inlet (with the exception of signal streams)
<li>Control of the object state by use of Max/Jitter-like "attributes"
</ul>

Naturally there are some cons, too:
<ul>
<li>Introduces a small overhead to speed of message handling 
<li>Overhead in object size (due to possibly unneeded library code) when statically linked
</ul>

Currently, flext supports 
<ul>
<li>PD on Windows with Microsoft Visual C++, Borland C++ and gcc(cygwin) compilers
<li>PD on Linux with gcc
<li>PD on Mac OSX with gcc (makefile or Xcode)
<li>Max/MSP on Mac OS9 and OSX with Metrowerks CodeWarrior
</ul>

\section LICENSE License

Flext is covered by the GPL.

flext - C++ layer for Max/MSP and pd (pure data) externals<BR>
Copyright (C) 2001-2005 Thomas Grill

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.<BR>
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.<BR>
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

In the official flext distribution, the GNU General Public License is
in the file gpl.txt<BR> 
Also see the file license.txt for notes on 
referenced works and their license texts.

\section DOWNLOAD Download 

Download the latest flext version from http://grrrr.org/ext/flext .<br>
Alternatively, you can check out the cvs version from http://sourceforge.net/projects/pure-data .

\section USAGE Usage

As a developer, you should know the C++ language, how to use a makefile (especially necessary for linux)
and how to steer your compiler.<br>
Flext can be compiled as a static library which has then to be linked to the code of your external.
For most applications you won't have to use any of the native PD or Max/MSP API functions as they are all
encapsulated by flext.

So let's come to the point... how does a typical flext object look like?

This is the object "attr1", one of the flext tutorial examples:

\verbatim
// enable attribute processing
#define FLEXT_ATTRIBUTES 1

// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif
\endverbatim

With these lines, all the necessary definitions from the flext package have been included.

\verbatim
class attr1:
    public flext_base
{
    FLEXT_HEADER(attr1,flext_base)
\endverbatim

A flext class is simply defined by inheriting from the flext_base (see also \ref FLEXT_CLASS) 
or flext_dsp (see also \ref FLEXT_DSP) classes.
Additionally some information has to be added using FLEXT_HEADER (see \ref FLEXT_D_HEADER)

\verbatim
public:
    // constructor 
    attr1();
\endverbatim

Normally the constructor takes the creation arguments of an object. Here there are none.

\verbatim
protected:
    void m_trigger(float f);   
    
    // stored argument
    float arg; 
\endverbatim

These are methods and data elements for internal class usage. Flext doesn't know about them
as long as they are not registered.

\verbatim
private:
    // callback for method "m_trigger" (with one float argument)
    FLEXT_CALLBACK_F(m_trigger);  

    // define attribute callbacks for variable "arg" (with GET and SET properties)
    FLEXT_ATTRVAR_F(arg);  
};
\endverbatim

For each method that shall be exposed to the realtime-system (for receiving messages) and 
every attribute (for setting and getting values) callbacks have to be set up.
The functions in the groups \ref FLEXT_D_CALLBACK and \ref FLEXT_D_ATTRIB allow for their 
convenient definition.

\verbatim
// instantiate the class 
FLEXT_NEW("attr1",attr1)
\endverbatim

With FLEXT_NEW the class is registered for the real-time system.
The number of creation arguments and their types must be taken into account here.
There are several variants depending on whether a message oriented (see \ref FLEXT_D_NEW)
or a DSP object (see \ref FLEXT_D_NEW_DSP) is created and whether it resides in a object library
(see \ref FLEXT_D_LIB and \ref FLEXT_D_LIB_DSP).<BR>

\verbatim
attr1::attr1():
    arg(0)  // initialize argument 
{ 
    // define inlets
    AddInAnything();  // first inlet of type anything (index 0)
    
    // define outlets
    AddOutFloat();  // one float outlet (has index 0)
\endverbatim
    
Every inlet and outlet that the object shall have has to be registered.
This is done with the functions in \ref FLEXT_C_IO_ADD.

\verbatim
    // register methods
    FLEXT_ADDMETHOD(0,m_trigger);  // register method (for floats) "m_trigger" for inlet 0

    FLEXT_ADDATTR_VAR1("arg",arg);  // register attribute "arg" with variable arg
} 
\endverbatim

Likewise, every method (called by a message) (see \ref FLEXT_D_ADDMETHOD) and every
attribute (see \ref FLEXT_D_ADDATTR) exposed to the system has to be registered.
Here the registration at instance creation is shown - there's another way by registering at
class setup level, which is more efficient but can only be used if the methods or attributes
used are known beforehand (see \ref FLEXT_D_CADDMETHOD and \ref FLEXT_D_CADDATTR).

\verbatim
void attr1::m_trigger(float f)
{
    float res = arg+f;
    
    // output value to outlet
    ToOutFloat(0,res); // (0 stands for the outlet index 0)
}
\endverbatim

This is a method that is triggered with a message. It does some calculation and then outputs
a value to an outlet. There are numerous functions (see \ref FLEXT_C_IO_OUT) supporting 
that functionality. 

Be sure to work through the examples provided with the flext tutorial. These should give you
an overview about the possibilities of flext.
The "modules" link at the top of the page leads to a complete reference
of flext functions and classes.

*/

#endif
