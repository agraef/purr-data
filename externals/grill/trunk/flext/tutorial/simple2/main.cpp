/* 
flext tutorial - simple 2

Copyright (c) 2002,2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of a simple object doing a float addition
*/

// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif


class simple2:
	public flext_base
{
	FLEXT_HEADER(simple2,flext_base)
 
public:
	// constructor with float argument
	simple2(float init);

protected:
	void m_float1(float f);   
	void m_float2(float f);   
	
	// stored argument of right inlet
	float arg; 

private:
	// FLEXT_CALLBACK_F(...) is a shortcut for FLEXT_CALLBACK_1(...,float) 
	FLEXT_CALLBACK_F(m_float1)  // callback for method "m_float1" (with one float argument)
	FLEXT_CALLBACK_F(m_float2)  // callback for method "m_float2" (with one float argument)
};

// instantiate the class (constructor has one float argument)
FLEXT_NEW_1("simple2",simple2,float)


simple2::simple2(float init):
	arg(init)  // store argument
{ 
	// define inlets
	AddInAnything();  // first inlet of type anything (index 0)
	AddInFloat();     // additional float inlet (index 1)
	
	// define outlets
	AddOutFloat();  // one float outlet (has index 0)
	
	// register methods
	FLEXT_ADDMETHOD(0,m_float1);  // register method (for floats) "m_float1" for inlet 0
	FLEXT_ADDMETHOD(1,m_float2);  // register method (for floats) "m_float2" for inlet 1
} 

void simple2::m_float1(float f)
{
	float res;
	res = arg+f;
	
	// output value to outlet
	ToOutFloat(0,res); // (0 stands for the outlet index 0)
}

void simple2::m_float2(float f)
{
	// store float
	arg = f;
}

