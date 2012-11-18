#ifndef __lfo
#include "lfo.h"
#endif

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


//------------------------------------------------------------------------
LFO::LFO()
{
	sineTable = new float[NUM_LFO_POINTS];
	triangleTable = new float[NUM_LFO_POINTS];
	squareTable = new float[NUM_LFO_POINTS];
	sawTable = new float[NUM_LFO_POINTS];
	reverseSawTable = new float[NUM_LFO_POINTS];
	thornTable = new float[NUM_LFO_POINTS];

	fillLFOtables();
	table = sineTable;	// just to have it pointing to something at least

	srand((unsigned int)time(NULL));	// sets a seed value for rand() from the system clock

	reset();
}

//------------------------------------------------------------------------
LFO::~LFO()
{
	if (sineTable)
		delete[] sineTable;
	if (triangleTable)
		delete[] triangleTable;
	if (squareTable)
		delete[] squareTable;
	if (sawTable)
		delete[] sawTable;
	if (reverseSawTable)
		delete[] reverseSawTable;
	if (thornTable)
		delete[] thornTable;
}

//------------------------------------------------------------------------
void LFO::reset()
{
	position = 0.0f;
	stepSize = 1.0f;	// just to avoid anything really screwy
	oldRandomNumber = (float)rand() / (float)RAND_MAX;
	randomNumber = (float)rand() / (float)RAND_MAX;
	smoothSamples = 0;
	granularityCounter = 0;
}

//-----------------------------------------------------------------------------------------
// this function creates tables for mapping out the sine, triangle, & saw LFO shapes

void LFO::fillLFOtables()
{
  long i, n;


	// fill the sine waveform table (oscillates from 0 to 1 & back to 0)
	for (i = 0; (i < NUM_LFO_POINTS); i++)
		sineTable[i] = (sinf( ( ((float)i/(float)NUM_LFO_POINTS)-0.25f ) * 2.0f * PI ) + 1.0f) * 0.5f;

	// fill the triangle waveform table
	// ramp from 0 to 1 for the first half
	for (i = 0; (i < NUM_LFO_POINTS/2); i++)
		triangleTable[i] = (float)i / (float)(NUM_LFO_POINTS/2);
	// & ramp from 1 to 0 for the second half
	for (n = 0; (i < NUM_LFO_POINTS); n++)
	{
		triangleTable[i] = 1.0f - ((float)n / (float)(NUM_LFO_POINTS/2));
		i++;
	}

	// fill the square waveform table
	// stay at 1 for the first half
	for (i = 0; (i < NUM_LFO_POINTS/2); i++)
		squareTable[i] = 1.0f;
	// & 0 for the second half
	for (n = 0; (i < NUM_LFO_POINTS); n++)
	{
		squareTable[i] = 0.0f;
		i++;
	}

	// fill the sawtooth waveform table (ramps from 0 to 1)
	for (i = 0; (i < NUM_LFO_POINTS); i++)
		sawTable[i] = (float)i / (float)(NUM_LFO_POINTS-1);

	// fill the reverse sawtooth waveform table (ramps from 1 to 0)
	for (i = 0; (i < NUM_LFO_POINTS); i++)
		reverseSawTable[i] = (float)(NUM_LFO_POINTS-i-1) / (float)(NUM_LFO_POINTS-1);

	// fill the thorn waveform table
	// exponentially slope up from 0 to 1 for the first half
	for (i = 0; (i < NUM_LFO_POINTS/2); i++)
		thornTable[i] = powf( ((float)i / (float)(NUM_LFO_POINTS/2)), 2.0f );
	// & exponentially slope down from 1 to 0 for the second half
	for (n = 0; (i < NUM_LFO_POINTS); n++)
	{
		thornTable[i] = powf( (1.0f - ((float)n / (float)(NUM_LFO_POINTS/2))), 2.0f );
		i++;
	}
}


//--------------------------------------------------------------------------------------
void LFO::getShapeName(char *nameString)
{
	switch (LFOshapeScaled(fShape))
	{
		case kSineLFO                : strcpy(nameString, "sine");					break;
		case kTriangleLFO            : strcpy(nameString, "triangle");				break;
		case kSquareLFO              : strcpy(nameString, "square");				break;
		case kSawLFO                 : strcpy(nameString, "sawtooth");				break;
		case kReverseSawLFO          : strcpy(nameString, "reverse sawtooth");		break;
		case kThornLFO               : strcpy(nameString, "thorn");					break;
		case kRandomLFO              : strcpy(nameString, "random");				break;
		case kRandomInterpolatingLFO : strcpy(nameString, "random interpolating");	break;
		default :																	break;
	}
}


//--------------------------------------------------------------------------------------
// this function points the LFO table pointers to the correct waveform tables

void LFO::pickTheLFOwaveform()
{
	switch (LFOshapeScaled(fShape))
	{
		case kSineLFO :
			table = sineTable;
			break;
		case kTriangleLFO :
			table = triangleTable;
			break;
		case kSquareLFO :
			table = squareTable;
			break;
		case kSawLFO :
			table = sawTable;
			break;
		case kReverseSawLFO :
			table = reverseSawTable;
			break;
		case kThornLFO :
			table = thornTable;
			break;
		default :
			table = sineTable;
			break;
	}
}

//--------------------------------------------------------------------------------------
// calculates the position within an LFO's cycle needed to sync to the song's beat

void LFO::syncToTheBeat(long samplesToBar)
{
  float countdown, cyclesize;

	// calculate how many samples long the LFO cycle is
	cyclesize = NUM_LFO_POINTS_FLOAT / stepSize;
	// calculate many more samples it will take for this cycle to coincide with the beat
	countdown = fmodf( (float)samplesToBar,  cyclesize);
	// & convert that into the correct LFO position according to its table step size
	position = (cyclesize - countdown) * stepSize;
	// wrap around the new position if it is beyond the end of the LFO table
	if (position >= NUM_LFO_POINTS_FLOAT)
		position = fmodf(position, NUM_LFO_POINTS_FLOAT);
}
