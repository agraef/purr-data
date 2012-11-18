#ifndef __IIRfilter
#define __IIRfilter


#define PI   3.1415926535897932384626433832795f
#define SHELF_START_IIR   0.333f

class IIRfilter
{
public:
	IIRfilter();
	~IIRfilter();

	void calculateLowpassCoefficients(float cutoff, float samplerate);
	void calculateHighpassCoefficients(float cutoff, float samplerate);
	void copyCoefficients(IIRfilter *source);

	void reset()
	{
		prevIn = prevprevIn = prevOut = prevprevOut = prevprevprevOut = currentOut = 0.0f;
	}

	float prevIn, prevprevIn, prevOut, prevprevOut, prevprevprevOut, currentOut;
	float pOutCoeff, ppOutCoeff, pInCoeff, ppInCoeff, inCoeff;


#ifdef USING_HERMITE
	void process(float currentIn)
#else
	float process(float currentIn)
#endif
	{
	#ifdef USING_HERMITE
		// store 4 samples of history if we're preprocessing for Hermite interpolation
		prevprevprevOut = prevprevOut;
	#endif
		prevprevOut = prevOut;
		prevOut = currentOut;

//		currentOut = (currentIn*inCoeff) + (prevIn*pInCoeff) + (prevprevIn*ppInCoeff) 
//					- (prevOut*pOutCoeff) - (prevprevOut*ppOutCoeff);
		currentOut = ((currentIn+prevprevIn)*inCoeff) + (prevIn*pInCoeff) 
					- (prevOut*pOutCoeff) - (prevprevOut*ppOutCoeff);

		prevprevIn = prevIn;
		prevIn = currentIn;

	#ifndef USING_HERMITE
		return currentOut;
	#endif
	}

#ifdef USING_HERMITE
// start of pre-Hermite-specific functions
// there are 4 versions, 3 of which unroll for loops of 2, 3, & 4 iterations

	void processH1(float currentIn)
	{
		prevprevprevOut = prevprevOut;
		prevprevOut = prevOut;
		prevOut = currentOut;
		//
		currentOut = ( (currentIn+prevprevIn) * inCoeff ) + (prevIn  * pInCoeff)
					- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		//
		prevprevIn = prevIn;
		prevIn = currentIn;
	}

	void processH2(float *in, long inPos, long arraySize)
	{
	  float in0 = in[inPos];
	  float in1 = in[(inPos+1) % arraySize];

		prevprevprevOut = prevprevOut;
		prevprevOut = prevOut;
		prevOut = currentOut;
		currentOut = ( (in0+prevprevIn) * inCoeff ) + (prevIn * pInCoeff)
					- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		//
		prevprevprevOut = prevprevOut;
		prevprevOut = prevOut;
		prevOut = currentOut;
		currentOut = ( (in1+prevIn) * inCoeff ) + (in0 * pInCoeff)
					- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		//
		prevprevIn = in0;
		prevIn = in1;
	}

	void processH3(float *in, long inPos, long arraySize)
	{
	  float in0 = in[inPos];
	  float in1 = in[(inPos+1) % arraySize];
	  float in2 = in[(inPos+2) % arraySize];

		prevprevprevOut = ( (in0+prevprevIn) * inCoeff ) + (prevIn * pInCoeff)
						- (currentOut * pOutCoeff) - (prevOut * ppOutCoeff);
		prevprevOut = ((in1+prevIn) * inCoeff) + (in0 * pInCoeff)
						- (prevprevprevOut * pOutCoeff) - (currentOut * ppOutCoeff);
		prevOut = ((in2+in0) * inCoeff) + (in1 * pInCoeff)
						- (prevprevOut * pOutCoeff) - (prevprevprevOut * ppOutCoeff);
		//
		currentOut = prevOut;
		prevOut = prevprevOut;
		prevprevOut = prevprevprevOut;
		prevprevprevOut = currentOut;
		//
		prevprevIn = in1;
		prevIn = in2;
	}

	void processH4(float *in, long inPos, long arraySize)
	{
	  float in0 = in[inPos];
	  float in1 = in[(inPos+1) % arraySize];
	  float in2 = in[(inPos+2) % arraySize];
	  float in3 = in[(inPos+3) % arraySize];

		prevprevprevOut = ( (in0+prevprevIn) * inCoeff ) + (prevIn * pInCoeff)
						- (currentOut * pOutCoeff) - (prevOut * ppOutCoeff);
		prevprevOut = ((in1+prevIn) * inCoeff) + (in0 * pInCoeff)
						- (prevprevprevOut * pOutCoeff) - (currentOut * ppOutCoeff);
		prevOut = ((in2+in0) * inCoeff) + (in1 * pInCoeff)
						- (prevprevOut * pOutCoeff) - (prevprevprevOut * ppOutCoeff);
		currentOut = ((in3+in1) * inCoeff) + (in2 * pInCoeff)
						- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		//
		prevprevIn = in2;
		prevIn = in3;
	}

#endif
// end of USING_HERMITE batch of functions

};	// end of IIRfilter class definition



// 4-point Hermite spline interpolation for use with IIR filter output histories
inline float interpolateHermitePostFilter(IIRfilter *filter, double address)
{
	long pos = (long)address;
	float posFract = (float) (address - (double)pos);

	float a = ( (3.0f*(filter->prevprevOut-filter->prevOut)) - 
				filter->prevprevprevOut + filter->currentOut ) * 0.5f;
	float b = (2.0f*filter->prevOut) + filter->prevprevprevOut - 
				(2.5f*filter->prevprevOut) - (filter->currentOut*0.5f);
	float c = (filter->prevOut - filter->prevprevprevOut) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + filter->prevprevOut;
}


#endif