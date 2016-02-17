/* 
flext tutorial - attributes 1

Copyright (c) 2002,2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of a simple object doing a float addition
It is a variation of the tutorial "simple 2"
*/


// IMPORTANT: enable attribute processing (specify before inclusion of flext headers!)
// For clarity, this is done here, but you'd better specify it as a compiler definition
// FLEXT_ATTRIBUTES must be 0 or 1, 
#define FLEXT_ATTRIBUTES 1


// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif


class attr1:
	public flext_base
{
	FLEXT_HEADER(attr1,flext_base)
 
public:
	// constructor 
	attr1();

protected:
	void m_trigger(float f);   
	
	// stored argument
	float arg; 

private:
	// callback for method "m_trigger" (with one float argument)
	FLEXT_CALLBACK_F(m_trigger)

	// define attribute callbacks for variable "arg" (with GET and SET properties)
	FLEXT_ATTRVAR_F(arg)
};

// instantiate the class 
FLEXT_NEW("attr1",attr1)


attr1::attr1():
	arg(0)  // initialize argument 
{ 
	// define inlets
	AddInAnything();  // first inlet of type anything (index 0)
	
	// define outlets
	AddOutFloat();  // one float outlet (has index 0)
	
	// register methods
	FLEXT_ADDMETHOD(0,m_trigger);  // register method (for floats) "m_trigger" for inlet 0

	FLEXT_ADDATTR_VAR1("arg",arg);  // register attribute "arg" with variable arg
} 

void attr1::m_trigger(float f)
{
	float res = arg+f;
	
	// output value to outlet
	ToOutFloat(0,res); // (0 stands for the outlet index 0)
}

