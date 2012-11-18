#ifndef __TOM7_TRANSVERB_H
#define __TOM7_TRANSVERB_H

//#define	TRANSVERB_STEREO	1
#define	USING_HERMITE		1

/* DFX Transverb transverb by Tom 7 and Marc 3 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "flext.h"

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 202)
#error You need at least flext version 0.2.2 
#endif

#include "dfxmisc.h"
#include "IIRfilter.h"



//----------------------------------------------------------------------------- 
// these are the transverb parameters:

#define fsign(f) ((f<0.0f)?-1.0f:1.0f)

#ifdef TRANSVERB_STEREO
  #define NUM_CHANNELS 2
#else
  #define NUM_CHANNELS 1
#endif

#define BUFFER_MIN 1.0f
#define BUFFER_MAX 3000.0f
#define bufferMsScaled(A)   ( paramRangeScaled((A), BUFFER_MIN, BUFFER_MAX) )
#define bufferScaled(A)   ( ((int)(bufferMsScaled(A)*SAMPLERATE*0.001f) > MAXBUF) ? MAXBUF : (int)(bufferMsScaled(A)*SAMPLERATE*0.001f) )

#define gainScaled(A)   ((A)*(A))

#define SPEED_MIN (-3.0f)
#define SPEED_MAX 6.0f
#define speedScaled(A)   ( paramRangeScaled((A), SPEED_MIN, SPEED_MAX) )
#define speedUnscaled(A)   ( paramRangeUnscaled((A), SPEED_MIN, SPEED_MAX) )
// for backwards compatibility with versions 1.0 & 1.0.1
#define OLD_SPEED_MIN 0.03f
#define OLD_SPEED_MAX 10.0f
#define oldSpeedScaled(A)   ( paramRangeSquaredScaled((A), OLD_SPEED_MIN, OLD_SPEED_MAX) )

#define qualityScaled(A)   ( paramSteppedScaled((A), numQualities) )
#define qualityUnscaled(A)   ( paramSteppedUnscaled((A), numQualities) )

#define SMOOTH_DUR 42

// this is for converting version 1.0 speed parameter valuess to the current format
//#define newSpeed(A)   ((log2f(oldSpeedScaled((A)))-SPEED_MIN) / (SPEED_MAX-SPEED_MIN))
#define newSpeed(A)   (((logf(oldSpeedScaled((A)))/logf(2.0f))-SPEED_MIN) / (SPEED_MAX-SPEED_MIN))

// this stuff is for the speed parameter adjustment mode switch on the GUI
enum { kFineMode, kSemitoneMode, kOctaveMode, numSpeedModes };
#define speedModeScaled(A)   ( paramSteppedScaled((A), numSpeedModes) )

#define numFIRtaps 23

const float RAND_MAX_FLOAT = (float) RAND_MAX;	// reduces wasteful casting

enum { dirtfi, hifi, ultrahifi, numQualities };

enum { useNothing, useHighpass, useLowpassIIR, useLowpassFIR, numFilterModes };


class transverb: 
	public flext_dsp {

	FLEXT_HEADER(transverb, flext_dsp)
  
public:
	transverb(int argc, t_atom *argv);
	~transverb();


protected:

	void initPresets();
	void createAudioBuffers();
	void clearBuffers();

	virtual void m_signal(int n, float *const *in, float *const *out);
	virtual void m_help();

	float drymix;
	int bsize, ireplace;
	float mix1, speed1, feed1, dist1;
	float mix2, speed2, feed2, dist2;
	float fQuality, fTomsound;
	long quality;
	bool tomsound;

	int writer;
	double read1, read2;
  
	int sr; int blocksize;

	float * buf1[2];
	float * buf2[2];
	int MAXBUF;	// the size of the audio buffer (dependant on sampling rate)

	IIRfilter *filter1, *filter2;
	bool speed1hasChanged, speed2hasChanged;
	float fSpeed1mode, fSpeed2mode;

	int smoothcount1[2], smoothcount2[2], smoothdur1[2], smoothdur2[2];
	float smoothstep1[2], smoothstep2[2], lastr1val[2], lastr2val[2];

	float SAMPLERATE;

	float fBsize;

	float *firCoefficients1, *firCoefficients2;

	FLEXT_CALLBACK_F(setBsize)
	void setBsize(float f) {
		f = (f < 2999) ? f : 2999;
		f = (f > 0) ? f : 0;
		bsize = bufferScaled((int)f);
		writer %= bsize;
		read1 = fmod(fabs(read1), (double)bsize);
		read2 = fmod(fabs(read2), (double)bsize);
	}

	FLEXT_CALLBACK_F(setMix1)
	void setMix1(float f) {
		f = (f < 1) ? f : 1;
		f = (f > 0) ? f : 0;
		mix1 = gainScaled(f);
	}

	FLEXT_CALLBACK_F(setSpeed1)
	void setSpeed1(float f) {
		f = (f < 6) ? f : 6;
		f = (f > -3) ? f : -3;
		speed1 = powf(2.0f, speedScaled(f));
		speed1hasChanged = true;
	}

	FLEXT_CALLBACK_F(setFeed1)
	void setFeed1(float f) {
		f = (f < 1) ? f : 1;
		f = (f > 0) ? f : 0;
		feed1 = f;
	}

	FLEXT_CALLBACK_F(setDist1)
	void setDist1(float f) {
		f = (f < 1) ? f : 1;
		f = (f > 0) ? f : 0;
		dist1 = f;
		read1 = fmod(fabs((double)writer + (double)dist1 *
				  (double)MAXBUF), (double)bsize);
	}

	FLEXT_CALLBACK_F(setMix2)
	void setMix2(float f) {
		f = (f < 1) ? f : 1;
		f = (f > 0) ? f : 0;
		mix2 = gainScaled(f);
	}

	FLEXT_CALLBACK_F(setSpeed2)
	void setSpeed2(float f) {
		f = (f < 6) ? f : 6;
		f = (f > -3) ? f : -3;
		speed2 = powf(2.0f, speedScaled(f));
		speed2hasChanged = true;
	}

	FLEXT_CALLBACK_F(setFeed2)
	void setFeed2(float f) {
		f = (f < 1) ? f : 1;
		f = (f > 0) ? f : 0;
		feed2 = f;
	}

	FLEXT_CALLBACK_F(setDist2)
	void setDist2(float f) {
		f = (f < 1) ? f : 1;
		f = (f > 0) ? f : 0;
		dist2 = f;
		read2 = fmod(fabs((double)writer + (double)dist2 *
				  (double)MAXBUF), (double)bsize);
	}

private:
	// this one does the work
	virtual void processX(float *in, float *out, long n, int replacing);

	// init would be better name
	virtual void suspend();
};


inline float interpolateHermite (float *data, double address, 
				 int arraysize, int danger) {
  int pos, posMinus1, posPlus1, posPlus2;
  float posFract, a, b, c;

  pos = (long)address;
  posFract = (float) (address - (double)pos);

  // because the readers & writer are not necessarilly aligned, 
  // upcoming or previous samples could be discontiguous, in which case 
  // just "interpolate" with repeated samples
  switch (danger) {
    case 0:		// the previous sample is bogus
      posMinus1 = pos;
      posPlus1 = (pos+1) % arraysize;
      posPlus2 = (pos+2) % arraysize;
      break;
    case 1:		// the next 2 samples are bogus
      posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
      posPlus1 = posPlus2 = pos;
      break;
    case 2:		// the sample 2 steps ahead is bogus
      posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
      posPlus1 = posPlus2 = (pos+1) % arraysize;
      break;
    default:	// everything's cool
      posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
      posPlus1 = (pos+1) % arraysize;
      posPlus2 = (pos+2) % arraysize;
      break;
    }

  a = ( (3.0f*(data[pos]-data[posPlus1])) - 
	 data[posMinus1] + data[posPlus2] ) * 0.5f;
  b = (2.0f*data[posPlus1]) + data[posMinus1] - 
         (2.5f*data[pos]) - (data[posPlus2]*0.5f);
  c = (data[posPlus1] - data[posMinus1]) * 0.5f;

  return ( ((a*posFract)+b) * posFract + c ) * posFract + data[pos];
}

inline float interpolateLinear(float *data, double address, 
				int arraysize, int danger) {
	int posPlus1, pos = (long)address;
	float posFract = (float) (address - (double)pos);

	if (danger == 1) {
	  /* the upcoming sample is not contiguous because 
	     the write head is about to write to it */
	  posPlus1 = pos;
	} else {
	  // it's all right
	  posPlus1 = (pos + 1) % arraysize;
	}
	return (data[pos] * (1.0f-posFract)) + 
	       (data[posPlus1] * posFract);
}



#endif
