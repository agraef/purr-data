/*
    dfx skidder : an effect plugin
    Copyright (C) 2000, Marcberg Soft und Hardware GmbH, All Rights Reserved
    Copyright (C) 2002, martin pi

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*---------------------------------------------------------------

   © 2000, Marcberg Soft und Hardware GmbH, All Rights Reserved
   pd parts : © 2002, martin pi

---------------------------------------------------------------*/

#ifndef __skidder
#define __skidder

#include <math.h>
#include <stdlib.h>

#include "dfxmisc.h"
//#include "vstchunk.h"
#include "TempoRateTable.h"

#include "flext.h"

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 202)
#error You need at least flext version 0.2.2 
#endif



//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kRate,
	kTempoSync,
	kTempoRate,
	kRateRandFactor,
	kTempo,
	kPulsewidth,
	kPulsewidthRandMin,
	kSlope,
	kPan,
	kNoise,

	kFloor,
	kFloorRandMin,

	NUM_PARAMETERS
};

//----------------------------------------------------------------------------- 
// these are the 4 states of the process:
enum
{
	slopeIn,
	plateau,
	slopeOut,
	valley
};

//----------------------------------------------------------------------------- 
// constants & macros

#define RATEMIN 0.3f
#define RATEMAX 21.0f
// this is for converting from parameter entries to the real values
#define rateScaled(A)   ( paramRangePowScaled((A), RATEMIN, RATEMAX, 1.65f) )
#define rateUnscaled(A)   ( paramRangePowUnscaled((A), RATEMIN, RATEMAX, 1.65f) )

#define RATE_RAND_FACTOR_MIN 1.0f
#define RATE_RAND_FACTOR_MAX 9.0f
#define rateRandFactorScaled(A)   ( paramRangeSquaredScaled((A), RATE_RAND_FACTOR_MIN, RATE_RAND_FACTOR_MAX) )
#define rateRandFactorUnscaled(A)   ( paramRangeSquaredUnscaled((A), RATE_RAND_FACTOR_MIN, RATE_RAND_FACTOR_MAX) )

#define PULSEMIN 0.001f
#define PULSEMAX 0.999f
#define pulsewidthScaled(A)   ( paramRangeScaled((A), PULSEMIN, PULSEMAX) )
#define pulsewidthUnscaled(A)   ( paramRangeUnscaled((A), PULSEMIN, PULSEMAX) )

#define TEMPO_MIN 39.0f
#define TEMPO_MAX 480.0f
#define tempoScaled(A)   ( paramRangeScaled((A), TEMPO_MIN, TEMPO_MAX) )
#define tempoUnscaled(A)   ( paramRangeUnscaled((A), TEMPO_MIN, TEMPO_MAX) )

#define SLOPEMAX 15.0f

// this is for scaling the slider values of rupture (more emphasis on lower values, dB-style)
#define fNoise_squared (fNoise*fNoise)

#define gainScaled(A) ((A)*(A)*(A))
#define normalize(f,a,b)	((((f < b) ? f : b) > a) ? f : a)


//----------------------------------------------------------------------------- 

class skidder : public flext_dsp {

	FLEXT_HEADER(skidder, flext_dsp)

public:
	skidder(int argc, t_atom *argv);
//	~skidder();

	virtual void m_signal(int, t_sample *const *, t_sample *const *);
	virtual void m_help();

//	virtual void process(float **inputs, float **outputs, long sampleFrames);
	virtual void m_setup();

//	virtual void setParameter(long index, float value);


protected:
	void processSlopeIn();
	void processPlateau();
	void processSlopeOut();
	void processValley(float SAMPLERATE);
	float processOutput(float in1, float in2, float pan);
	void processReplacing(float **inputs, float **outputs, long sampleFrames, bool replacing);

	// the parameters
	float fRate, fTempoSync, fTempoRate, fRateRandFactor, fTempo;
	float fPulsewidth, fPulsewidthRandMin, fSlope, fPan, fNoise, fFloor, fFloorRandMin;
	float floor, gainRange;	// a scaled version of fFloor & the difference between that & 1.0

	float randomFloor, randomGainRange;
	bool useRandomFloor;
	float amp; 		// output sample scalar
	long cycleSamples, pulseSamples, slopeSamples, slopeDur, plateauSamples, valleySamples;	// sample counters
	float slopeStep;	// the scalar for each step of the fade during a slope in or out
	float panRander;	// the actual pan value for each cycle
	int state;		// the state of the process
	float rms;
	long rmscount;

	float currentTempoBPS;	// tempo in beats per second
	float oldTempoBPS;	// holds the previous value of currentTempoBPS for comparison
	bool tempoHasChanged;	// tells the GUI that the rate random range display needs updating
	bool mustUpdateTempoHasChanged;
	TempoRateTable *tempoRateTable;	// a table of tempo rate values
	
	
	FLEXT_CALLBACK_F(setRate)
	void setRate(float f) {
		fRate = rateUnscaled(f);
	}

	FLEXT_CALLBACK_F(setTempoSync)
	void setTempoSync(float f) {
		fTempoSync = 1;
	}

	FLEXT_CALLBACK_F(setTempoRate)
	void setTempoRate(float f) {
		fTempoRate = rateUnscaled(f);
	}

	FLEXT_CALLBACK_F(setRateRandFactor)
	void setRateRandFactor(float f) {
		fRateRandFactor = rateRandFactorUnscaled(f);
	}

	FLEXT_CALLBACK_F(setTempo)
	void setTempo(float f) {
		fTempo = tempoUnscaled(f);
	}
	
	FLEXT_CALLBACK_F(setPulsewidth)
	void setPulsewidth(float f) {
		fPulsewidth = pulsewidthUnscaled(f);
	}
	
	FLEXT_CALLBACK_F(setPulsewidthRandMin)
	void setPulsewidthRandMin(float f) {
		fPulsewidthRandMin = pulsewidthUnscaled(f);
	}
	
	FLEXT_CALLBACK_F(setSlope)
	void setSlope(float f) {
		fSlope = f/SLOPEMAX;
	}
		
	FLEXT_CALLBACK_F(setFloor)
	void setFloor(float f) {
		fFloor = f;
	}
		
	FLEXT_CALLBACK_F(setFloorRandMin)
	void setFloorRandMin(float f) {
		fFloorRandMin = f;
	}
		
	FLEXT_CALLBACK_F(setPan)
	void setPan(float f) {
		fPan = f;
	}
	
	FLEXT_CALLBACK_F(setNoise)
	void setNoise(float f) {
		fNoise = normalize(f,0,1);
	}

//	FLEXT_CALLBACK(m_print)


};

#endif
