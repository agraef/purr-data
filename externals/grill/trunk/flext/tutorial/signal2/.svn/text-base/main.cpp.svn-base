/* 
flext tutorial - signal 2 

Copyright (c) 2002,2003 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an object showing varous parameters of the pd audio system
*/

// include flext header
#include <flext.h>

// check for appropriate flext version
#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0
#endif


// define the class that stands for a pd/Max object
// Attention: the class name must be the same as the object name!! (without the ~)

class signal2:
	// inherit from flext dsp class
	public flext_dsp
{
	// obligatory flext header (class name,base class name)
	FLEXT_HEADER(signal2,flext_dsp)
 
public:
	// constructor
	signal2();

protected:
	void m_bang();  // method for bang

private:
	FLEXT_CALLBACK(m_bang)  // callback for method "m_bang" 
};

// instantiate the class
FLEXT_NEW_DSP("signal2~",signal2)


signal2::signal2()
{ 
	// define inlets:
	// first inlet must always by of type anything (or signal for dsp objects)
	AddInAnything();  // add one inlet for any message
	
	// add outlets for sample rate, block size, audio in and out channel count
	AddOutFloat(1);
	AddOutInt(3); // although PD knows no int type, flext does!
	
	// register methods
	FLEXT_ADDBANG(0,m_bang);  // register method "m_bang" for bang message into inlet 0
} 

void signal2::m_bang()
{
	// output various parameters of the pd audio system
	ToOutFloat(0,Samplerate()); 
	ToOutInt(1,Blocksize()); 
	ToOutInt(2,CntInSig()); 
	ToOutInt(3,CntOutSig()); 
}

