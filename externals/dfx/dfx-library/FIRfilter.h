#ifndef __FIRfilter
#define __FIRfilter

#include <math.h>


#define PI   3.1415926535897932384626433832795f
#define SHELF_START_FIR   0.333f


//----------------------------------------------------------------------------- 
void calculateFIRidealLowpassCoefficients(float cutoff, float samplerate, 
											int numTaps, float *coefficients);
void applyKaiserWindow(int numTaps, float *coefficients, float attenuation);
float besselIzero(float in);
float besselIzero2(float in);

//----------------------------------------------------------------------------- 
inline float processFIRfilter(float *in, int numTaps, float *coefficients, 
								long inPos, long arraySize)
{
	float out = 0.0f;
	if ( (inPos+numTaps) > arraySize )
	{
		for (long i=0; i < numTaps; i++)
			out += in[(inPos+i)%arraySize] * coefficients[i];
	}
	else
	{
		for (long i=0; i < numTaps; i++)
			out += in[inPos+i] * coefficients[i];
	}
	return out;
} 


#endif