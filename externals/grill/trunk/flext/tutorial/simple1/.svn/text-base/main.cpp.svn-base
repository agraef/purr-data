/* 
flext tutorial - simple 1 

Copyright (c) 2002,2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of a simple object doing a float inversion
*/

// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif


// define the class that stands for a pd/Max object
// Attention: the class name must be the same as the object name!! (without an eventual ~)
// Special names are possible with the usage of libraries (see the lib1 tutorial example)

class simple1:
	// inherit from basic flext class
	public flext_base
{
	// obligatory flext header (class name,base class name)
	FLEXT_HEADER(simple1,flext_base)
 
public:
	// constructor
	simple1()
	{ 
		// define inlets:
		// first inlet must always be of type anything (or signal for dsp objects)
		AddInAnything();  // add one inlet for any message
		
		// define outlets:
		AddOutFloat();  // add one float outlet (has index 0)
		
		// register methods
		FLEXT_ADDMETHOD(0,m_float);  // register method (for float messages) "m_float" for inlet 0
	} 

protected:
	void m_float(float input)  // method for float values
	{
		float result;

		if(input == 0) {
			// special case 0
			post("%s - zero can't be inverted!",thisName());
			result = 0;
		}
		else 
			// normal case
			result = 1/input;

		// output value to outlet
		ToOutFloat(0,result); // (0 stands for the outlet index 0 - the leftmost outlet)
	}

private:
	FLEXT_CALLBACK_1(m_float,float)  // callback for method "m_float" (with one float argument)
};

// instantiate the class
FLEXT_NEW("simple1",simple1)


