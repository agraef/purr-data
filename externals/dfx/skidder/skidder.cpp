/*
    dfx skidder : an effect plugin
    Copyright (C) 2000, Marc Poirier
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

/*-------------- by Marc Poirier  ][  December 2000 -------------*/
/*   pd parts : © 2002, martin pi */

#ifndef __skidder
#include "skidder.hpp"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>


//-----------------------------------------------------------------------------
// initializations & such

skidder::skidder(int argc, t_atom *argv) {

	post("_ ____  ---- :: _______  ___ _____ _ skidder~ ");

	tempoRateTable = new TempoRateTable;
//	m_setup();				// initialize a fresh skid cycle
	
	AddInSignal(2);
	AddInFloat(6);
	AddOutSignal(2);
	SetupInOut();  

	fTempo = tempoUnscaled(120.0f);
	currentTempoBPS = oldTempoBPS = tempoScaled(fTempo) / 60.0f;

	FLEXT_ADDMETHOD( 1, setRate);
	FLEXT_ADDMETHOD_F(0,"temposync",setTempoSync);
	FLEXT_ADDMETHOD( 2, setTempoRate);
	FLEXT_ADDMETHOD_F(0,"rrf",setRateRandFactor);
	FLEXT_ADDMETHOD( 3, setTempo);
	FLEXT_ADDMETHOD( 4, setPulsewidth);
	FLEXT_ADDMETHOD_F(0,"pwrm",setPulsewidthRandMin);
	FLEXT_ADDMETHOD( 5, setSlope);
	FLEXT_ADDMETHOD( 6, setFloor);
	FLEXT_ADDMETHOD_F(0,"frm",setFloorRandMin);
	FLEXT_ADDMETHOD( 7, setPan);
	FLEXT_ADDMETHOD_F(0,"noise",setNoise);

	post("_ ____ ____ _");

	srand((unsigned int)time(NULL));	// sets a seed value for rand() from the system clock
}


//-----------------------------------------------------------------------------------------
// titles of each parameter


void skidder::m_help() {

		post(" __ ---- %s %f", "1          rate  [ Hz ] ",fRate);
		post(" __ ---- %s %f", "temposync  tempo sync   ",fTempoSync);
		post(" __ ---- %s %f", "2          tempo rate  [ cycles/beat ]",fTempoRate);
		post(" __ ---- %s %f", "rrf        rate random factor",fRateRandFactor);
		post(" __ ---- %s %f", "3          tempo [ bpm ]",fTempo);
		post(" __ ---- %s %f", "4          pulsewidth  [ % of cycle ]",fPulsewidth);
		post(" __ ---- %s %f", "pwrm       pulsewidth random min.  [ % of cycle ]",fPulsewidthRandMin);
		post(" __ ---- %s %f", "5          slope  [ ms ]",fSlope);
		post(" __ ---- %s %f", "6          floor  [ dB ]",fFloor);
		post(" __ ---- %s %f", "frm        floor random min.  [ dB ]",fFloorRandMin);
		post(" __ ---- %s %f", "7          stereo spread[ amount ]",fPan);
		post(" __ ---- %s %f", "noise      rupture  ",fNoise);
}



// GIMME class:
FLEXT_NEW_TILDE_G("skidder~", skidder)


void skidder::m_signal(int n, float *const *in, float *const *out) {

	// directly move everything to the vst part	
	processReplacing((float **)in, (float **)out,(long)n, true);
}  // end m_signal

//-----------------------------------------------------------------------------------------
// this gets called when the plugin is de-activated

void skidder::m_setup() {

	state = valley;
	valleySamples = 0;
	panRander = 0.0f;
	rms = 0.0f;
	rmscount = 0;
	randomFloor = 0.0f;
	randomGainRange = 1.0f;

	// deallocate the memory from these arrays
//	if (tempoRateTable) delete tempoRateTable;
	
	
	fRate 		= rateUnscaled(3.0f);	// 0.1304347826087
	fTempoSync 	= 0.0f;			// default to no tempo sync; "free" control
	fTempoRate 	= 0.333f;
	fRateRandFactor = 0.0f;			// default to no randomness
	fTempo 		= 0.0f;			// default to "auto" (i.e. get it from the host)
	fPulsewidth 	= 0.5f;
	fPulsewidthRandMin = 0.5f;
	fSlope 		= 3.0f/SLOPEMAX;	// 0.2
	fFloor 		= 0.0f;
	fFloorRandMin 	= 0.0f;
	fPan 		= 0.0f;
	fNoise 		= 0.0f;
	
	
}

//-----------------------------------------------------------------------------------------
void skidder::processSlopeIn()
{
	// dividing the growing slopeDur-slopeSamples by slopeDur makes ascending values
	if (useRandomFloor)
		amp = ( ((float)(slopeDur-slopeSamples)) * slopeStep * randomGainRange ) + randomFloor;
	else
		amp = ( ((float)(slopeDur-slopeSamples)) * slopeStep * gainRange ) + floor;

	slopeSamples--;

	if (slopeSamples <= 0)
	{
		state = plateau;
	}
}

//-----------------------------------------------------------------------------------------
void skidder::processPlateau()
{

	// amp in the plateau is 1.0, i.e. this sample is unaffected
	amp = 1.0f;

	plateauSamples--;

	if (plateauSamples <= 0)
	{
		// average & then sqare the sample squareroots for the RMS value
		rms = powf((rms/(float)rmscount), 2.0f);
		// because RMS tends to be < 0.5, thus unfairly limiting rupture's range
		rms *= 2.0f;
		// avoids clipping or unexpected values (like wraparound)
		if ( (rms > 1.0f) || (rms < 0.0f) )
			rms = 1.0f;
		rmscount = 0;	// reset the RMS counter
		//
		// set up the random floor values
		useRandomFloor = fFloorRandMin < fFloor;
		randomFloor = ( ((float)rand()*ONE_DIV_RAND_MAX) * gainScaled(fFloor-fFloorRandMin) ) 
							+ gainScaled(fFloorRandMin);
		randomGainRange = 1.0f - randomFloor;	// the range of the skidding on/off gain
		//
		if (slopeDur > 0)
		{
			state = slopeOut;
			slopeSamples = slopeDur; // refill slopeSamples
			slopeStep = 1.0f / (float)slopeDur;	// calculate the fade increment scalar
		}
		else
			state = valley;
	}
}


//-----------------------------------------------------------------------------------------
void skidder::processSlopeOut()
{
	// dividing the decrementing slopeSamples by slopeDur makes descending values
	if (useRandomFloor)
		amp = ( ((float)slopeSamples) * slopeStep * randomGainRange ) + randomFloor;
	else
		amp = ( ((float)slopeSamples) * slopeStep * gainRange ) + floor;

	slopeSamples--;

	if (slopeSamples <= 0)
	{
		state = valley;
	}
}

//-----------------------------------------------------------------------------------------
void skidder::processValley(float SAMPLERATE)
{
  float rateRandFactor = rateRandFactorScaled(fRateRandFactor);	// stores the real value
  float cycleRate;	// the base current skid rate value
  float randFloat, randomRate;	// the current randomized rate value
  float fPulsewidthRandomized;	// stores the latest randomized pulsewidth 0.0 - 1.0 value
  bool barSync = false;	// true if we need to sync up with the next bar start
  long countdown;


	if (useRandomFloor)
		amp = randomFloor;
	else
		amp = floor;

	valleySamples--;

	if (valleySamples <= 0)
	{
		rms = 0.0f;	// reset rms now because valley is over
		//
		// This is where we figure out how many samples long each 
		// envelope section is for the next skid cycle.
		//
		if (onOffTest(fTempoSync))	// the user wants to do tempo sync / beat division rate
		{
			cycleRate = currentTempoBPS * (tempoRateTable->getScalar(fTempoRate));
			// set this true so that we make sure to do the measure syncronisation later on
		}
		else
			cycleRate = rateScaled(fRate);
		//
		if (fRateRandFactor > 0.0f)
		{
			// get a random value from 0.0 to 1.0
			randFloat = (float)rand() * ONE_DIV_RAND_MAX;
			// square-scale the random value & then scale it with the random rate range
			randomRate = ( randFloat * randFloat * 
							((cycleRate*rateRandFactor)-(cycleRate/rateRandFactor)) ) + 
							(cycleRate/rateRandFactor);
			cycleSamples = (long) (SAMPLERATE / randomRate);
			barSync = false;	// we can't do the bar sync if the skids durations are random
		}
		else
			cycleSamples = (long) (SAMPLERATE / cycleRate);
		//
		if (fPulsewidth > fPulsewidthRandMin)
		{
			fPulsewidthRandomized = ( ((float)rand()*ONE_DIV_RAND_MAX) * (fPulsewidth-fPulsewidthRandMin) ) + fPulsewidthRandMin;
			pulseSamples = (long) ( ((float)cycleSamples) * pulsewidthScaled(fPulsewidthRandomized) );
		}
		else
			pulseSamples = (long) ( ((float)cycleSamples) * pulsewidthScaled(fPulsewidth) );
		valleySamples = cycleSamples - pulseSamples;
		slopeSamples = (long) ((SAMPLERATE/1000.0f)*(fSlope*(SLOPEMAX)));
		slopeDur = slopeSamples;
		slopeStep = 1.0f / (float)slopeDur;	// calculate the fade increment scalar
		plateauSamples = pulseSamples - (slopeSamples * 2);
		if (plateauSamples < 1)	// this shrinks the slope to 1/3 of the pulse if the user sets slope too high
		{
			slopeSamples = (long) (((float)pulseSamples) / 3.0f);
			slopeDur = slopeSamples;
			slopeStep = 1.0f / (float)slopeDur;	// calculate the fade increment scalar
			plateauSamples = pulseSamples - (slopeSamples * 2);
		}

		// go to slopeIn next if slope is not 0.0, otherwise go to plateau
		if (slopeDur > 0)
			state = slopeIn;
		else
			state = plateau;

		// this puts random float values from -1.0 to 1.0 into panRander
		panRander = ( ((float)rand()*ONE_DIV_RAND_MAX) * 2.0f ) - 1.0f;

	} //end of the "valley is over" if-statement
}

//-----------------------------------------------------------------------------------------
float skidder::processOutput(float in1, float in2, float pan)
{
	// output noise
	if ( (state == valley) && (fNoise != 0.0f) )
		// out gets random noise with samples from -1.0 to 1.0 times the random pan times rupture times the RMS scalar
		return ((((float)rand()*ONE_DIV_RAND_MAX)*2.0f)-1.0f) * pan * fNoise_squared * rms;

	// do regular skidding output
	else
	{
		// only output a bit of the first input
		if (pan <= 1.0f)
			return in1 * pan * amp;
		// output all of the first input & a bit of the second input
		else
			return ( in1 + (in2*(pan-1.0f)) ) * amp;
	}
}

//-----------------------------------------------------------------------------------------
void skidder::processReplacing(float **inputs, float **outputs, long sampleFrames, bool replacing) {

  float *in1  = inputs[0];
  float *in2  = inputs[1];
  float *out1 = outputs[0];
  float *out2 = outputs[1];

  float SAMPLERATE = Samplerate();
	// just in case the host responds with something wacky
	if (SAMPLERATE <= 0.0f)   SAMPLERATE = 44100.0f;

  long samplecount;


	floor = gainScaled(fFloor);	// the parameter scaled real value
	gainRange = 1.0f - floor;	// the range of the skidding on/off gain
	useRandomFloor = (fFloorRandMin < fFloor);

	// figure out the current tempo if we're doing tempo sync
	if (onOffTest(fTempoSync))
	{
		// calculate the tempo at the current processing buffer
		currentTempoBPS = tempoScaled(fTempo) / 60.0f;

	}

	for (samplecount=0; (samplecount < sampleFrames); samplecount++)
	{
		switch (state)
		{
			case slopeIn:
				// get the average sqareroot of the current input samples
				rms += sqrtf( fabsf(((*in1)+(*in2))*0.5f) );
				rmscount++;	// this counter is later used for getting the mean
				processSlopeIn();
				break;
			case plateau:
				rms += sqrtf( fabsf(((*in1)+(*in2))*0.5f) );
				rmscount++;
				processPlateau();
				break;
			case slopeOut:
				processSlopeOut();
				break;
			case valley:
				processValley(SAMPLERATE);
				break;
		}

		// ((panRander*fPan)+1.0) ranges from 0.0 to 2.0
		if (replacing)
		{
			*out1 = processOutput( *in1, *in2, ((panRander*fPan)+1.0f) );
			*out2 = processOutput( *in2, *in1, (2.0f - ((panRander*fPan)+1.0f)) );
		}
		else
		{
			*out1 += processOutput( *in1, *in2, ((panRander*fPan)+1.0f) );
			*out2 += processOutput( *in2, *in1, (2.0f - ((panRander*fPan)+1.0f)) );
		}
		// move forward in the i/o sample streams
		in1++;
		in2++;
		out1++;
		out2++;
	}
}
