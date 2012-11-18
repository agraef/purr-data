#ifndef __TempoRateTable
#define __TempoRateTable


//--------------------------------------------------------------------------
// the number of tempo beat division options
#ifdef USE_SLOW_TEMPO_RATES
	#define NUM_TEMPO_RATES 25
#else
	#ifdef USE_BUFFER_OVERRIDE_TEMPO_RATES
		#define NUM_TEMPO_RATES 21
	#else
		#define NUM_TEMPO_RATES 24
	#endif
#endif



//-------------------------------------------------------------------------- 
// this holds the beat scalar values & textual displays for the tempo rates
class TempoRateTable
{
public:
	TempoRateTable();
	~TempoRateTable();

	float getScalar(float paramValue)
		{	return scalars[float2index(paramValue)];	}
	char * getDisplay(float paramValue)
		{	return displays[float2index(paramValue)];	}

protected:
	int float2index(float f)
	{
		if (f < 0.0f)
			f = 0.0f;
		else if (f > 1.0f)
			f = 1.0f;
		return (int) (f * ((float)NUM_TEMPO_RATES-0.9f));
	}
		
	float *scalars;
	char **displays;
};


#endif