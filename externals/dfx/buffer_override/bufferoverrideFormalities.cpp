/*------------------- by Marc Poirier  ][  March 2001 -------------------*/

#ifndef __bufferoverride
#include "bufferoverride.hpp"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


//-------------------------------------------------------------------------
// titles of each parameter
/*
	fDivisor		"buffer divisor"
	fBuffer			"forced buffer size"
	fBufferTempoSync	"forced buffer tempo sync"
	fBufferInterrupt	"stuck buffer"
	fDivisorLFOrate		"divisor LFO rate"
	fDivisorLFOdepth	"divisor LFO depth"
	fDivisorLFOshape	"divisor LFO shape"
	fDivisorLFOtempoSync	"divisor LFO tempo sync"
	fBufferLFOrate		"buffer LFO rate"
	fBufferLFOdepth		"buffer LFO depth"
	fBufferLFOshape		"buffer LFO shape"
	fBufferLFOtempoSync	"buffer LFO tempo sync"
	fSmooth			"smooth"
	fDryWetMix		"dry/wet mix"
	fPitchbend		"pitchbend"
	fTempo			"tempo"
*/


//-----------------------------------------------------------------------------
// initializations & such

bufferoverride::bufferoverride(int argc, t_atom *argv) {

	buffer1 = NULL;

	SUPER_MAX_BUFFER = (long) ((44100.0f / MIN_ALLOWABLE_BPS) * 4.0f);

	// allocate memory for these structures
	tempoRateTable	= new TempoRateTable;
	divisorLFO	= new LFO;
	bufferLFO	= new LFO;

	suspend();
	initPresets();

	fTempo = tempoUnscaled(120.0f);
	currentTempoBPS = tempoScaled(fTempo) / 60.0f;

	
	post("_ ____bufferoverride~ --_ - __ _____");

	SAMPLERATE = (int) Samplerate();
//	blocksize = Blocksize(); 
	
/*	fTomsound = (float)GetFloat(argv[0]);	// creation arguments 
	fQuality  = (float)GetFloat(argv[1]);	// [ yet they default to reasonable values ]
	drymix    = (float)GetFloat(argv[2]);
	ireplace  =   (int)GetFloat(argv[3]);*/

	AddInSignal();
	AddInFloat(9);
	AddOutSignal();         // 1 audio out [ == AddOutSignal(1) ]
	
	SetupInOut();           // set up inlets and outlets. 
				// Must get called once!

	// Now we need to bind the handler function to our
	// inlets, 
	
/*	fDivisor		"buffer divisor"
	fBuffer			"forced buffer size"
	fBufferTempoSync	"forced buffer tempo sync"
	fBufferInterrupt	"stuck buffer"
	fDivisorLFOrate		"divisor LFO rate"
	fDivisorLFOdepth	"divisor LFO depth"
	fDivisorLFOshape	"divisor LFO shape"
	fDivisorLFOtempoSync	"divisor LFO tempo sync"
	fBufferLFOrate		"buffer LFO rate"
	fBufferLFOdepth		"buffer LFO depth"
	fBufferLFOshape		"buffer LFO shape"
	fBufferLFOtempoSync	"buffer LFO tempo sync"
	fSmooth			"smooth"
	fDryWetMix		"dry/wet mix"
	fPitchbend		"pitchbend"
	fTempo			"tempo" 
	
*/	
	FLEXT_ADDMETHOD( 1,setDivisor);
	FLEXT_ADDMETHOD( 2,setBuffer);
	FLEXT_ADDMETHOD( 3,setTempo);
	FLEXT_ADDMETHOD( 4,setDivisorLFOrate);
	FLEXT_ADDMETHOD( 5,setDivisorLFOdepth);
	FLEXT_ADDMETHOD( 6,setDivisorLFOshape);
	FLEXT_ADDMETHOD( 7,setBufferLFOrate);
	FLEXT_ADDMETHOD( 8,setBufferLFOdepth);
	FLEXT_ADDMETHOD( 9,setBufferLFOshape);

	post("_ ____ ____ _");

}


// GIMME class:
FLEXT_NEW_TILDE_G("bufferoverride~", bufferoverride)


//-------------------------------------------------------------------------
bufferoverride::~bufferoverride() {

	// deallocate the memory from these arrays
	if (buffer1) delete[] buffer1;
	if (tempoRateTable) delete tempoRateTable;
	if (divisorLFO) delete divisorLFO;
	if (bufferLFO) delete bufferLFO;


}

//-------------------------------------------------------------------------
void bufferoverride::suspend()
{
	// setting the values like this will restart the forced buffer in the next process()
	currentForcedBufferSize = 1;
	writePos = readPos = 1;
	minibufferSize = 1;
	prevMinibufferSize = 0;
	smoothcount = smoothDur = 0;
	sqrtFadeIn = sqrtFadeOut = 1.0f;

	divisorLFO->reset();
	bufferLFO->reset();
}

//-------------------------------------------------------------------------
void bufferoverride::createAudioBuffers() {

	// update the sample rate value
	SAMPLERATE = (float)Samplerate();
	// just in case the host responds with something wacky
	if (SAMPLERATE <= 0.0f)
		SAMPLERATE = 44100.0f;
	long oldMax = SUPER_MAX_BUFFER;
	SUPER_MAX_BUFFER = (long) ((SAMPLERATE / MIN_ALLOWABLE_BPS) * 4.0f);

	// if the sampling rate (& therefore the max buffer size) has changed, 
	// then delete & reallocate the buffers according to the sampling rate
	if (SUPER_MAX_BUFFER != oldMax) 	{
		if (buffer1 != NULL)
			delete[] buffer1;
		buffer1 = NULL;
	}
	
	if (buffer1 == NULL) buffer1 = new float[SUPER_MAX_BUFFER];
}


//-------------------------------------------------------------------------
void bufferoverride::initPresets()
{

	fDivisor = bufferDivisorUnscaled(27.0f);
	fBuffer = forcedBufferSizeUnscaled(81.0f);
	fBufferTempoSync	= 0.0f;
	fBufferInterrupt	= 1.0f;
	fSmooth			= 0.06f;
	fDryWetMix		= 1.0f;
	fTempo			= 0.0f;

	divisorLFO->fRate = paramSteppedUnscaled(6.6f, NUM_TEMPO_RATES);
	divisorLFO->fDepth = 0.333f;
	divisorLFO->fShape = LFOshapeUnscaled(kSineLFO);
	divisorLFO->fTempoSync = 1.0f;
	bufferLFO->fRate = 0.0f;
	bufferLFO->fDepth = 0.06f;
	bufferLFO->fShape = LFOshapeUnscaled(kSawLFO);
	bufferLFO->fTempoSync = 1.0f;
}

