#ifndef __dfxmisc
#define __dfxmisc

#include <math.h>
#include <stdlib.h>

#ifdef WIN32
/* turn off warnings about default but no cases in switch, etc. */
   #pragma warning( disable : 4065 57 4200 4244 )
   #include <windows.h>
#endif


//----------------------------------------------------------------------------- 
// constants & macros

#define DESTROY_FX_RULEZ

#define onOffTest(fvalue)   ((fvalue) > 0.5f)

#define paramRangeScaled(value,min,max)   ( ((value) * ((max)-(min))) + (min) )
#define paramRangeUnscaled(value,min,max)   ( ((value)-(min)) / ((max)-(min)) )
#define paramRangeSquaredScaled(value,min,max)   ( ((value)*(value) * ((max)-(min))) + (min) )
#define paramRangeSquaredUnscaled(value,min,max)   ( sqrtf(((value)-(min)) / ((max)-(min))) )
#define paramRangeCubedScaled(value,min,max)   ( ((value)*(value)*(value) * ((max)-(min))) + (min) )
#define paramRangeCubedUnscaled(value,min,max)   ( powf(((value)-(min)) / ((max)-(min)), 1.0f/3.0f) )
#define paramRangePowScaled(value,min,max,power)   ( (powf((value),(power)) * ((max)-(min))) + (min) )
#define paramRangePowUnscaled(value,min,max,power)   ( powf(((value)-(min)) / ((max)-(min)), 1.0f/(power)) )
#define paramRangeExpScaled(value,min,max)   ( expf(logf((max)-(min)+1.0f)*(value)) + (min) - 1.0f )
#define paramRangeExpUnscaled(value,min,max)   ( logf(1.0f-(min)+(value)) / logf(1.0f-(min)+(max)) )
#define paramSteppedScaled(value,numSteps)   ( (long)((value) * ((float)(numSteps)-0.01f)) )
#define paramSteppedUnscaled(step,numSteps)   ( (float)(step) / ((float)((numSteps)-1)) )
#define paramRangeIntScaled(value,min,max)   ( (long)((value) * ((float)((max)-(min)+1)-0.01f)) + (min) )
#define paramRangeIntUnscaled(step,min,max)   ( (float)((step)-(min)) / (float)((max)-(min)) )
// scale logarithmicly from 20 Hz to 20 kHz
#define paramFrequencyScaled(value)   (20.0f * powf(2.0f, (value) * 9.965784284662088765571752446703612804412841796875f))
#define paramFrequencyUnscaled(value)   ( (logf((value)/20.0f)/logf(2.0f)) / 9.965784284662088765571752446703612804412841796875f )

#define dBconvert(fvalue) ( 20.0f * log10f((fvalue)) )

#ifndef PI
#define PI 3.1415926535897932384626433832795f
#endif
#ifndef PId
#define PId 3.1415926535897932384626433832795
#endif

// reduces wasteful casting & division
const float ONE_DIV_RAND_MAX = 1.0f / (float)RAND_MAX;
#define randFloat()   ( (float)rand() * ONE_DIV_RAND_MAX )

#ifndef clip
#define clip(fvalue)   (if (fvalue < -1.0f) fvalue = -1.0f; else if (fvalue > 1.0f) fvalue = 1.0f)
#endif

#ifndef undenormalize
#define undenormalize(fvalue)   if (fabs(fvalue) < 1.0e-15)   fvalue = 0.0
//#define undenormalize(fvalue)  (((*(unsigned int*)&(fvalue))&0x7f800000)==0)?0.0f:(fvalue)
#endif

//#define kBeatSyncTimeInfoFlags   (kVstTempoValid | kVstTransportChanged | kVstBarsValid | kVstPpqPosValid | kVstTimeSigValid)
#define kBeatSyncTimeInfoFlags		1

/* return the parameter with larger magnitude */
inline float magmax(float a, float b) {
  if (fabs(a) > fabs(b)) return a;
  else return b;
}


//----------------------------------------------------------------------------- 
// function prototypes

//pi
//long samplesToNextBar(VstTimeInfo *timeInfo);

// pi
//void processProgramChangeEvents(VstEvents *events, AudioEffectX *effect);

double LambertW(double input);


//----------------------------------------------------------------------------- 
// inline functions

//----------------------------------------------------------------------------- 
inline float interpolateHermite(float *data, double address, long arraysize)
{
	long pos = (long)address;
	float posFract = (float) (address - (double)pos);

	long posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
	long posPlus1 = (pos+1) % arraysize;
	long posPlus2 = (pos+2) % arraysize;

	float a = ( (3.0f*(data[pos]-data[posPlus1])) - data[posMinus1] + data[posPlus2] ) * 0.5f;
	float b = (2.0f*data[posPlus1]) + data[posMinus1] - (2.5f*data[pos]) - (data[posPlus2]*0.5f);
	float c = (data[posPlus1] - data[posMinus1]) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + data[pos];
}

inline float interpolateLinear(float *data, double address, long arraysize)
{
	long pos = (long)address;
	float posFract = (float) (address - (double)pos);
	return (data[pos] * (1.0f-posFract)) + (data[(pos+1)%arraysize] * posFract);
}

inline float interpolateRandom(float randMin, float randMax)
{
	float randy = (float)rand() * ONE_DIV_RAND_MAX;
	return ((randMax-randMin) * randy) + randMin;
}

inline float interpolateLinear2values(float point1, float point2, double address)
{
	float posFract = (float) (address - (double)((long)address));
	return (point1 * (1.0f-posFract)) + (point2 * posFract);
}

//----------------------------------------------------------------------------- 
// I found this somewhere on the internet
/*
inline float anotherSqrt(float x)
{
	float result = 1.0f;
	float store = 0.0f;
	while (store != result)
	{
		store = result;
		result = 0.5f * (result + (x / result));
	}
}
*/


//----------------------------------------------------------------------------- 
// mutex stuff

#if WIN32

struct dfxmutex {
	CRITICAL_SECTION c;
	dfxmutex() { InitializeCriticalSection(&c); }
	~dfxmutex() { DeleteCriticalSection(&c); }
	void grab() { EnterCriticalSection(&c); }
	void release() { LeaveCriticalSection(&c); }
};

/*
#elif MAC

// Multiprocessing Services
#include <Multiprocessing.h>
struct dfxmutex {
	OSStatus initErr, deleteErr, enterErr, exitErr;
	MPCriticalRegionID c;
	Duration timeout;	// in ms   (available constants:  kDurationImmediate, kDurationForever, kDurationMillisecond, kDurationMicrosecond)
	dfxmutex() { initErr = MPCreateCriticalRegion(&c); }
	~dfxmutex() { deleteErr = MPDeleteCriticalRegion(c); }
	void grab () { enterErr = MPEnterCriticalRegion(c, kDurationForever); }
	void release () { exitErr = MPExitCriticalRegion(c); }
};

// POSIX
// can this even work for CFM?  perhaps only mach-o
#include <pthread.h>
//#include <Carbon/Carbon.h>
struct dfxmutex {
	int initErr, deleteErr, enterErr, exitErr;
	pthread_mutex_t c;
	dfxmutex() { initErr = pthread_mutex_init(&c, NULL); }
	~dfxmutex() { deleteErr = pthread_mutex_destroy(&c); }
	void grab () { enterErr = pthread_mutex_lock(&c); }
	void release () { exitErr = pthread_mutex_unlock(&c);
					//pthread_testcancel();
	}
};
*/

#else

struct dfxmutex {
	dfxmutex() {}
	~dfxmutex() {}
	void grab () {}
	void release () {}
};

#endif


#endif
