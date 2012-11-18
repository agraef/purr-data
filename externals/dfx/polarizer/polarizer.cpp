/*---------------------------------------------------------------

   © 2001, Marcberg Soft und Hard GmbH, All Rights Perversed

---------------------------------------------------------------*/

#ifndef __polarizer
#include "polarizer.hpp"
#endif

#include <stdio.h>

//-----------------------------------------------------------------------------
// initializations & such

polarizer::polarizer(int argc, t_atom *argv) {

	fSkip	 = 0.0f;
	fAmount  = 0.5f;
	fImplode = 0.0f;

	state = polarized;	// the first sample is polarized

	post("_ ____polarize~ ");

	AddInSignal();
	AddInFloat(3);
	AddOutSignal();
	SetupInOut();  

	FLEXT_ADDMETHOD( 1,setSkip);
	FLEXT_ADDMETHOD( 2,setAmount);
	FLEXT_ADDMETHOD( 3,setImplode);

	post("_ ____ ____ _");
}

// GIMME class:
FLEXT_NEW_TILDE_G("polarizer~", polarizer)

//-----------------------------------------------------------------------------------------
void polarizer::processReplacing(float **inputs, float **outputs, long sampleFrames) {
	float amp;

	for (long samplecount=0; (samplecount < sampleFrames); samplecount++)
	{
		amp = processSampleAmp();
		outputs[0][samplecount] = processOutValue(amp, inputs[0][samplecount]);
	}
}


void polarizer::m_signal(int n, float *const *in, float *const *out) {

	// directly move everything to the vst part	
	processReplacing((float **)in, (float **)out,(long)n);
}  // end m_signal



//-----------------------------------------------------------------------------------------
// titles of each parameter

void polarizer::m_help() {
		post(" inlet --- uno ----> ___ _ leap [ ranging from about 1 to 81 ] samples");
		post(" inlet -- due  -- >>>>> ___ _ _  polarize :: 0~1 __ ------ _ amount");
		post(" inlet ------- >> implode << ------- | triggered madness | burn the speakers");
}


//-----------------------------------------------------------------------------------------
// this is for getting the scalar value amp for the current sample

float polarizer::processSampleAmp() {

	switch (state)
	{
		case unaffected:	// nothing much happens in this section
			if (--unaffectedSamples <= 0)	// go to polarized when the leap is done
				state = polarized;
			return 1.0f;
			break;
			// end unaffected

		case polarized:
			state = unaffected;	// go right back to unaffected
			// but first figure out how long unaffected will last this time
			unaffectedSamples = leapScaled(fSkip);
			return (0.5f - fAmount) * 2.0f;	// this is the polarization scalar
			break;
			// end polarized

		default : return 1.0f;
	}
}

//-----------------------------------------------------------------------------------------
// this is for calculating & sending the current sample output value to the output stream

float polarizer::processOutValue(float amp, float in) {
	float out = (in * amp);	// if implode is off, just do the regular polarizing thing

	if (fImplode >= 1.0f)	// if it's on, then implode the audio signal
	{
		if (out > 0.0f)	// invert the sample between 1 & 0
			out = 1.0f - out;
		else	// invert the sample between -1 & 0
			out = -1.0f - out;
	}

	return out;
}
