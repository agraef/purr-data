/* 
flext tutorial - stk 1

Copyright (c) 2002-2010 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

-------------------------------------------------------------------------

This is an example of an external using the STK ("synthesis toolkit") library.
For STK see http://ccrma-www.stanford.edu/software/stk

STK needs C++ exceptions switched on.

The STK tutorial examples assume that a static stk library exists which contains all the
source files (except rt*.cpp) of the stk/src directory.
The library should be compiled multithreaded and with the appropriate compiler flags for 
the respective platform (e.g. __OS_WINDOWS__ and __LITTLE_ENDIAN__ for Windows)

*/

#include <flstk.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error You need at least flext version 0.4.1 with STK support
#endif

#include "Noise.h"

using namespace stk;

class stk1:
	public flext_stk
{
	FLEXT_HEADER(stk1,flext_stk)

public:
	stk1();

protected:
	// these are obligatory!
	virtual bool NewObjs(); // create STK instruments
	virtual void FreeObjs(); // destroy STK instruments
	virtual void ProcessObjs(int n);  // do DSP processing

private:
	Noise *inst;
};

FLEXT_NEW_DSP("stk1~",stk1)
 

stk1::stk1():
	inst(NULL)
{ 
	AddInAnything();
	AddOutSignal();  
}


// create STK instruments
bool stk1::NewObjs()
{
	bool ok = true;

	// set up objects
	try {
		inst = new Noise;
	}
	catch (StkError &) {
		post("%s - Noise() setup failed!",thisName());
		ok = false;
	}
	return ok;
}


// destroy the STK instruments
void stk1::FreeObjs()
{
	if(inst) delete inst;
}

// this is called on every DSP block
void stk1::ProcessObjs(int n)
{
	while(n--) Outlet(0).tick(inst->tick());
}


