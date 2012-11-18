/* 
flext tutorial - library 1 

Copyright (c) 2002,2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of an external library containing a few simple objects

It uses attributes, so be sure that you've already worked through attr1 and attr2

*/

// Enable attribute processing 
// For clarity, this is done here, but you'd better specify it as a compiler definition
#define FLEXT_ATTRIBUTES 1

// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif


// -------------------------------------------------------------------------------------
// Define the base class 
// Note that you don't have to instantiate the base class (with FLEXT_NEW or variants)

class libbase:
	// inherit from basic flext class
	public flext_base
{
	// obligatory flext header (class name,base class name)
	FLEXT_HEADER(libbase,flext_base)
 
public:
	// constructor
	libbase();

protected:
	void Output(float f) const { ToOutFloat(0,f); }

	// method for floats into left inlet
	virtual void m_trigger(float f) = 0;  

	float arg;  // argument variable
private:
	FLEXT_CALLBACK_F(m_trigger)  // callback for method "m_trigger" (with one float argument)
	FLEXT_ATTRVAR_F(arg)
};

libbase::libbase():
	arg(0)  // initialize argument
{ 
	// define inlets:
	// first inlet must always by of type anything (or signal for dsp objects)
	AddInAnything();  // add one inlet for any message
	
	// define outlets:
	AddOutFloat();  // add one float outlet (has index 0)
	
	// register methods
	FLEXT_ADDMETHOD(0,m_trigger);  // register method (for float messages) "m_float" for inlet 0

	// register attributes
	FLEXT_ADDATTR_VAR1("arg",arg);  // register attribute "arg"
} 


// ------------------------------------------------------------------
// Define the actual library objects (derived from the base class)
// These classes have an implementation of the virtual function m_trigger 

class libadd:
	public libbase
{
	// obligatory flext header, inherit from libbase
	FLEXT_HEADER(libadd,libbase)
public:
	virtual void m_trigger(float f) { Output(f+arg); }
};

FLEXT_LIB("lib1.+",libadd);



class libsub:
	public libbase
{
	// obligatory flext header, inherit from libbase
	FLEXT_HEADER(libsub,libbase)
public:
	virtual void m_trigger(float f) { Output(f-arg); }
};

FLEXT_LIB("lib1.-",libsub);



class libmul:
	public libbase
{
	// obligatory flext header, inherit from libbase
	FLEXT_HEADER(libmul,libbase)
public:
	virtual void m_trigger(float f) { Output(f*arg); }
};

FLEXT_LIB("lib1.*",libmul);


// ------------------------------------------------
// Do the library setup

static void lib_setup()
{
	post("flext tutorial lib1, (C)2002 Thomas Grill");
	post("lib1: lib1.+ lib1.- lib1.*");
	post("");

	// call the objects' setup routines
	FLEXT_SETUP(libadd);
	FLEXT_SETUP(libsub);
	FLEXT_SETUP(libmul);
}

// setup the library
FLEXT_LIB_SETUP(lib1,lib_setup)
