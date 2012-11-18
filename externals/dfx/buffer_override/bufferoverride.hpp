/*------------------- by Marc Poirier  ][  March 2001 -------------------*/


#ifndef __bufferoverride
#define __bufferoverride

#include "dfxmisc.h"
#include "lfo.h"
#include "TempoRateTable.h"

#include "flext.h"

//----------------------------------------------------------------------------- 
// these are the plugin parameters:
enum
{
	kDivisor,
	kBuffer,
	kBufferTempoSync,
	kBufferInterrupt,

	kDivisorLFOrate,
	kDivisorLFOdepth,
	kDivisorLFOshape,
	kDivisorLFOtempoSync,
	kBufferLFOrate,
	kBufferLFOdepth,
	kBufferLFOshape,
	kBufferLFOtempoSync,

	kSmooth,
	kDryWetMix,

	kPitchbend,
	kMidiMode,

	kTempo,

	NUM_PARAMETERS
};

//----------------------------------------------------------------------------- 
// constants & macros

#define DIVISOR_MIN 1.92f
#define DIVISOR_MAX 222.0f
#define bufferDivisorScaled(A) ( paramRangeSquaredScaled((A), DIVISOR_MIN, DIVISOR_MAX) )
#define bufferDivisorUnscaled(A) ( paramRangeSquaredUnscaled((A), DIVISOR_MIN, DIVISOR_MAX) )

#define BUFFER_MIN 1.0f
#define BUFFER_MAX 999.0f
#define forcedBufferSizeScaled(A) ( paramRangeSquaredScaled((1.0f-(A)), BUFFER_MIN, BUFFER_MAX) )
#define forcedBufferSizeUnscaled(A) ( 1.0f - paramRangeSquaredUnscaled((A), BUFFER_MIN, BUFFER_MAX) )
#define forcedBufferSizeSamples(A) ( (long)(forcedBufferSizeScaled((A)) * SAMPLERATE * 0.001f) )

#define TEMPO_MIN 57.0f
#define TEMPO_MAX 480.0f
#define tempoScaled(A)   ( paramRangeScaled((A), TEMPO_MIN, TEMPO_MAX) )
#define tempoUnscaled(A)   ( paramRangeUnscaled((A), TEMPO_MIN, TEMPO_MAX) )

#define LFO_RATE_MIN 0.03f
#define LFO_RATE_MAX 21.0f
#define LFOrateScaled(A)   ( paramRangeSquaredScaled((A), LFO_RATE_MIN, LFO_RATE_MAX) )
#define LFOrateUnscaled(A)   ( paramRangeSquaredUnscaled((A), LFO_RATE_MIN, LFO_RATE_MAX) )

#define normalize(f,a,b)	((((f < b) ? f : b) > a) ? f : a)


// you need this stuff to get some maximum buffer size & allocate for that
// this is 42 bpm - should be sufficient
#define MIN_ALLOWABLE_BPS 0.7f


//----------------------------------------------------------------------------- 
class bufferoverrideProgram
{
friend class bufferoverride;
public:
	bufferoverrideProgram();
	~bufferoverrideProgram();
private:
	float *param;
	char *name;
};


//----------------------------------------------------------------------------- 

class bufferoverride : public flext_dsp  {

	FLEXT_HEADER(bufferoverride, flext_dsp)

public:
	bufferoverride(int argc, t_atom *argv);
	~bufferoverride();
	
	virtual void m_signal(int, t_sample *const *, t_sample *const *);
	virtual void m_help();

protected:
	void doTheProcess(float **inputs, float **outputs, long sampleFrames, bool replacing);
	void updateBuffer(long samplePos);

	void heedbufferoverrideEvents(long samplePos);
	float getDivisorParameterFromNote(int currentNote);
	float getDivisorParameterFromPitchbend(int pitchbendByte);

	void initPresets();
	void createAudioBuffers();

	// the parameters
	float fDivisor, fBuffer, fBufferTempoSync, fBufferInterrupt, fSmooth, 
	fDryWetMix, fPitchbend, fMidiMode, fTempo;

	long currentForcedBufferSize;	// the size of the larger, imposed buffer
	// these store the forced buffer
	float *buffer1;

	long writePos;	// the current sample position within the forced buffer

	long minibufferSize;	// the current size of the divided "mini" buffer
	long prevMinibufferSize;	// the previous size
	long readPos;	// the current sample position within the minibuffer
	float currentBufferDivisor;	// the current value of the divisor with LFO possibly applied

	float numLFOpointsDivSR;	// the number of LFO table points divided by the sampling rate

	float currentTempoBPS;	// tempo in beats per second
	TempoRateTable *tempoRateTable;	// a table of tempo rate values
	long hostCanDoTempo;	// my semi-booly dude who knows something about the host's VstTimeInfo implementation
	bool needResync;

	long SUPER_MAX_BUFFER;
	float SAMPLERATE;

	long smoothDur, smoothcount;	// total duration & sample counter for the minibuffer transition smoothing period
	float smoothStep;	// the gain increment for each sample "step" during the smoothing period
	float sqrtFadeIn, sqrtFadeOut;	// square root of the smoothing gains, for equal power crossfading
	float smoothFract;

	bool divisorWasChangedByHand;	// for MIDI trigger mode - tells us to respect the fDivisor value
	bool divisorWasChangedByMIDI;	// tells the GUI that the divisor displays need updating

	LFO *divisorLFO, *bufferLFO;

	float fadeOutGain, fadeInGain, realFadePart, imaginaryFadePart;	// for trig crossfading







	virtual void suspend();

	// flext
	
	FLEXT_CALLBACK_F(setDivisor)
	void setDivisor(float f) {
		fDivisor = bufferDivisorUnscaled(normalize(f,DIVISOR_MIN,DIVISOR_MAX));
	}
	
	FLEXT_CALLBACK_F(setBuffer)
	void setBuffer(float f) {
		fBuffer = forcedBufferSizeUnscaled(normalize(f,BUFFER_MIN,BUFFER_MAX));
	}
	
/*	FLEXT_CALLBACK_F(setBufferTempoSync)
	void setBufferTempoSync(float f) {
		fBufferTempoSync = normalize(f,0,1);
	}
	
	FLEXT_CALLBACK_F(setBufferInterrupt)
	void setBufferInterrupt(float f) {
		fBufferInterrupt = normalize(f,0,1);
	}
	
	FLEXT_CALLBACK_F(setSmooth)
	void setSmooth(float f) {
		fSmooth = normalize(f,0,1);
	}
	
	FLEXT_CALLBACK_F(setDryWetMix)
	void setDryWetMix(float f) {
		fDryWetMix = normalize(f,0,1);
	}
*/	
	FLEXT_CALLBACK_F(setTempo)
	void setTempo(float f) {
		fTempo = normalize(f,0,1);
	}
	
	FLEXT_CALLBACK_F(setDivisorLFOrate)
	void setDivisorLFOrate(float f) {
		divisorLFO->fRate = paramSteppedUnscaled(f, NUM_TEMPO_RATES);
	}
	
	FLEXT_CALLBACK_F(setDivisorLFOdepth)
	void setDivisorLFOdepth(float f) {
		divisorLFO->fDepth = normalize(f,0,1);
	}
	
	FLEXT_CALLBACK_F(setDivisorLFOshape)
	void setDivisorLFOshape(float f) {
		divisorLFO->fShape = LFOshapeUnscaled(normalize(f,0,7));
	}
	
	
	FLEXT_CALLBACK_F(setBufferLFOrate)
	void setBufferLFOrate(float f) {
		bufferLFO->fRate = paramSteppedUnscaled(f, NUM_TEMPO_RATES);
	}
	
	FLEXT_CALLBACK_F(setBufferLFOdepth)
	void setBufferLFOdepth(float f) {
		bufferLFO->fDepth = normalize(f,0,1);
	}
	
	FLEXT_CALLBACK_F(setBufferLFOshape)
	void setBufferLFOshape(float f) {
		bufferLFO->fShape = LFOshapeUnscaled(normalize(f,0,7));
	}


};

#endif
