/*---------------------------------------------------------------

   © 2001, Marcberg Soft & Hard GmbH, All Rights Reserved

---------------------------------------------------------------*/

#ifndef __polarizer_H
#define __polarizer_H

#include <stdlib.h>
#include <math.h>

#include "flext.h"

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 202)
#error You need at least flext version 0.2.2 
#endif

#define normalize(f,a,b)	((((f < b) ? f : b) > a) ? f : a)

// these are the 2 states of the process:
enum {
	unaffected,
	polarized
};

//----------------------------------------------------------------------------- 
// constants

const long SKIPMIN = 1;
const long SKIPMAX = 81;
const long SKIPRANGE = SKIPMAX - SKIPMIN;

// this is for converting from parameter entries to the real values
#define leapScaled(A)   ( ((long)(powf((A),1.5f)*SKIPRANGE)) + SKIPMIN )

//----------------------------------------------------------------------------- 

class polarizer :
	public flext_dsp {

	FLEXT_HEADER(polarizer, flext_dsp)

public:
	polarizer(int argc, t_atom *argv);
//	~polarizer();

	virtual void processReplacing(float **inputs, float **outputs, long sampleFrames);

	virtual void m_signal(int, t_sample *const *, t_sample *const *);
	virtual void m_help();

protected:
	float processSampleAmp();
	float processOutValue(float amp, float in);

	float fSkip, fAmount, fImplode;	// the parameters
	char *programName;
	long unaffectedSamples;	// sample counter
	int state;	// the state of the process

	FLEXT_CALLBACK_F(setSkip)
	void setSkip(float f) {
		fSkip = normalize(f,0,1);
	}

	FLEXT_CALLBACK_F(setAmount)
	void setAmount(float f) {
		fAmount = normalize(f,0,1);
	}

	FLEXT_CALLBACK_F(setImplode)
	void setImplode(float f) {
		fImplode = normalize(f,0,1);
	}
};

#endif
