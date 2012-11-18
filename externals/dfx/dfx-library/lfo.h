/*------------------- by Marc Poirier  ][  January 2002 ------------------*/

#ifndef __lfo
#define __lfo

#include <math.h>
#include <stdlib.h>

#include "dfxmisc.h"
#include "TempoRateTable.h"


//-------------------------------------------------------------------------------------
// these are the 8 LFO waveforms:
enum
{
	kSineLFO,
	kTriangleLFO,
	kSquareLFO,
	kSawLFO,
	kReverseSawLFO,
	kThornLFO,
	kRandomLFO,
	kRandomInterpolatingLFO,

	numLFOshapes
};

//-------------------------------------------------------------------------------------
// constants & macros

#define LFOshapeScaled(A)   (paramSteppedScaled((A), numLFOshapes))
#define LFOshapeUnscaled(A)   (paramSteppedUnscaled((A), numLFOshapes))

#define NUM_LFO_POINTS 512
const float NUM_LFO_POINTS_FLOAT = (float)NUM_LFO_POINTS;	// to reduce casting later on
const float LFO_TABLE_STEP = 1.0f / (float)NUM_LFO_POINTS;	// to reduce division & encourage multiplication
const long SQUARE_HALF_POINT = NUM_LFO_POINTS / 2;	// the point in the table when the square waveform drops to zero

#define LFO_SMOOTH_DUR 48
const float LFO_SMOOTH_STEP = 1.0f / (float)LFO_SMOOTH_DUR;

// this scales the return of processLFO() from 0.0 - 1.0 output to 0.0 - 2.0 (oscillating around 1.0)
#define processLFOzero2two(A)   ( ((A)->processLFO() * 2.0f) - (A)->fDepth + 1.0f );


//----------------------------------------------------------------------------- 
class LFO
{
public:
	LFO();
	~LFO();

	void reset();
	void fillLFOtables();

	void pickTheLFOwaveform();
	void getShapeName(char *nameString);

	void syncToTheBeat(long samplesToBar);

	// the LFO waveform tables
	float *sineTable, *triangleTable, *squareTable, *sawTable, *reverseSawTable, *thornTable;

	// the following are intended to be used as 0.0 - 1.0 VST parameter values:
	float fOnOff;	// parameter value for turning the LFO on or off
	float fTempoSync;	// parameter value for toggling tempo sync
	float fRate;	// parameter value for LFO rate (in Hz)
	float fTempoRate;	// parameter value for LFO rate (in cycles per beat)
	float fDepth;	// parameter value LFO depth
	float fShape;	// parameter value for LFO shape

	bool onOff;	// in case it's easier to have a bool version of fOnOff
	float position;	// this tracks the position in the LFO table
	float stepSize;	// size of the steps through the LFO table
	float *table;	// pointer to the LFO table
	float randomNumber;	// this stores random values for the random LFO waveforms
	float oldRandomNumber;	// this stores previous random values for the random interpolating LFO waveform
	float cycleRate;	// the rate in Hz of the LFO (only used for first layer LFOs)
	long smoothSamples;	// a counter for the position during a smoothing fade
	long granularityCounter;	// a counter for implementing LFO processing on a block basis
	long granularity;	// the number of samples to wait before processing


	//--------------------------------------------------------------------------------------
	// This function wraps around the LFO table position when it passes the cycle end.
	// It also sets up the smoothing counter if a discontiguous LFO waveform is being used.
	void updatePosition(long numSteps = 1)
	{
		// increment the LFO position tracker
		position += (stepSize * (float)numSteps);

		if (position >= NUM_LFO_POINTS_FLOAT)
		{
			// wrap around the position tracker if it has made it past the end of the LFO table
			position = fmodf(position, NUM_LFO_POINTS_FLOAT);
			// get new random LFO values, too
			oldRandomNumber = randomNumber;
			randomNumber = (float)rand() / (float)RAND_MAX;
			// set up the sample smoothing if a discontiguous waveform's cycle just ended
			switch (LFOshapeScaled(fShape))
			{
				case kSquareLFO     :
				case kSawLFO        :
				case kReverseSawLFO :
				case kRandomLFO     :
					smoothSamples = LFO_SMOOTH_DUR;
				default:
					break;
			}
		}

		// special check for the square waveform - it also needs smoothing at the half point
		else if (LFOshapeScaled(fShape) == kSquareLFO)
		{
			// check to see if it has just passed the halfway point
			if ( ((long)position >= SQUARE_HALF_POINT) && 
				 ((long)(position - stepSize) < SQUARE_HALF_POINT) )
				smoothSamples = LFO_SMOOTH_DUR;
		}
	}

	//--------------------------------------------------------------------------------------
	// this function gets the current 0.0 - 1.0 output value of the LFO & increments its position
	float processLFO()
	{
	  float randiScalar, outValue;
	  int shape = LFOshapeScaled(fShape);

		if (shape == kRandomInterpolatingLFO)
		{
			// calculate how far into this LFO cycle we are so far, scaled from 0.0 to 1.0
			randiScalar = position * LFO_TABLE_STEP;
			// interpolate between the previous random number & the new one
			outValue = (randomNumber * randiScalar) + (oldRandomNumber * (1.0f-randiScalar));
		}
		//
		else if (shape == kRandomLFO)
			outValue = randomNumber;
		//
		else
			outValue = table[(long)position];

		return (outValue * fDepth);
	}

};


#endif