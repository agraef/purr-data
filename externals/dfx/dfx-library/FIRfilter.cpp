#ifndef __FIRfilter
#include "FIRfilter.h"
#endif

//----------------------------------------------------------------------------- 
// you're supposed to use use an odd number of taps
void calculateFIRidealLowpassCoefficients(float cutoff, float samplerate, 
											int numTaps, float *coefficients)
{
  int middleCoeff;
  float corner, value;

	// get the cutoff as a ratio of cutoff to Nyquist, scaled from 0 to Pi
	corner = (cutoff / (samplerate*0.5f)) * PI;

	if (numTaps%2)
	{
		middleCoeff = (numTaps-1) / 2;
		coefficients[middleCoeff] = corner/PI;
	}
	else
		middleCoeff = numTaps/2;

	for (int n=0; n < middleCoeff; n++)
	{
		value = (float)n - ((float)(numTaps-1) * 0.5f);
		coefficients[n] = sinf(value*corner) / (value*PI);
		coefficients[numTaps-1-n] = coefficients[n];
	}
}

//----------------------------------------------------------------------------- 
void applyKaiserWindow(int numTaps, float *coefficients, float attenuation)
{
  int halfLength;


	// beta is 0 if the attenuation is less than 21 dB
	float beta = 0.0f;
	if (attenuation >= 50.0f)
		beta = 0.1102f * (attenuation - 8.71f);
	else if ( (attenuation < 50.0f) && (attenuation >= 21.0f) )
	{
		beta = 0.5842f * powf( (attenuation - 21.0f), 0.4f);
		beta += 0.07886f * (attenuation - 21.0f);
	}

	if (numTaps%2)
		halfLength = (numTaps+1) / 2;
	else
		halfLength = numTaps / 2;

	for (int n=0; n < halfLength; n++)
	{
		coefficients[n] *= besselIzero(beta * 
					sqrtf(1.0f - powf( (1.0f-((2.0f*n)/((float)(numTaps-1)))), 2.0f ))) 
				/ besselIzero(beta);
		coefficients[numTaps-1-n] = coefficients[n];
	}
} 

//----------------------------------------------------------------------------- 
float besselIzero(float in)
{
  float sum, numerator, denominator, term, halfIn;

	sum = 1.0f;
	halfIn = in * 0.5f;
	denominator = 1.0f;
	numerator = 1.0f;
	for (int m=1; m <= 32; m++)
	{
		numerator *= halfIn;
		denominator *= (float)m;
		term = numerator / denominator;
		sum += term * term;
	}
	return sum;
}

//----------------------------------------------------------------------------- 
float besselIzero2(float in)
{
  float sum = 1.0f;
  float ds = 1.0f;
  float d = 0.0f;

	do
	{
		d += 2.0f;
		ds *= ( in * in ) / ( d * d );
		sum += ds;
	}
	while ( ds > (1E-7f * sum) );

	return sum;
}
