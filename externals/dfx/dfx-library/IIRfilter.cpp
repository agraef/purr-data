#ifndef __IIRfilter
#include "IIRfilter.h"
#endif

#include <math.h>


//------------------------------------------------------------------------
IIRfilter::IIRfilter()
{
	reset();
}

//------------------------------------------------------------------------
IIRfilter::~IIRfilter() { }

//------------------------------------------------------------------------
void IIRfilter::calculateLowpassCoefficients(float cutoff, float samplerate)
{
  float twoPiFreqDivSR, cosTwoPiFreqDivSR, Q, slopeFactor, coeffScalar;

	Q = 0.5f;
	twoPiFreqDivSR = 2.0f * PI * cutoff / samplerate;	// ¹ ... 0
	cosTwoPiFreqDivSR = cosf(twoPiFreqDivSR);			// 1 ... -1
	slopeFactor = sinf(twoPiFreqDivSR) / (Q * 2.0f);	// 0 ... 1 ... 0
	coeffScalar = 1.0f / (1.0f + slopeFactor);			// 1 ... 0.5 ... 1

	// calculate filter coefficients
	pInCoeff = (1.0f - cosTwoPiFreqDivSR) * coeffScalar;	// 0 ... 2
	inCoeff = ppInCoeff = pInCoeff * 0.5f;					// 0 ... 1
	pOutCoeff = (-2.0f * cosTwoPiFreqDivSR) * coeffScalar;	// -2 ... 2
	ppOutCoeff = (1.0f - slopeFactor) * coeffScalar;		// 1 ... 0 ... 1
}

//------------------------------------------------------------------------
void IIRfilter::calculateHighpassCoefficients(float cutoff, float samplerate)
{
  float twoPiFreqDivSR, cosTwoPiFreqDivSR, Q, slopeFactor, coeffScalar;

	Q = 0.5f;
	twoPiFreqDivSR = 2.0f * PI * cutoff / samplerate;	// ¹ ... 0
	cosTwoPiFreqDivSR = cosf(twoPiFreqDivSR);			// 1 ... -1
	slopeFactor = sinf(twoPiFreqDivSR) / (Q * 2.0f);	// 0 ... 1 ... 0
	coeffScalar = 1.0f / (1.0f + slopeFactor);			// 1 ... 0.5 ... 1

	// calculate filter coefficients
	pInCoeff = (-1.0f - cosTwoPiFreqDivSR) * coeffScalar;	// 2 ... 0
	inCoeff = ppInCoeff = pInCoeff * (-0.5f);				// -1 ... 0
	pOutCoeff = (-2.0f * cosTwoPiFreqDivSR) * coeffScalar;	// -2 ... 2
	ppOutCoeff = (1.0f - slopeFactor) * coeffScalar;		// 1 ... 0 ... 1
}

//------------------------------------------------------------------------
void IIRfilter::copyCoefficients(IIRfilter *source)
{
	pOutCoeff = source->pOutCoeff;
	ppOutCoeff = source->ppOutCoeff;
	pInCoeff = source->pInCoeff;
	ppInCoeff = source->ppInCoeff;
	inCoeff = source->inCoeff;
}
