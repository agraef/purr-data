/*------------------- by Marc Poirier  ][  March 2001 -------------------*/

#ifndef __bufferoverride
#include "bufferoverride.hpp"
#endif



//-----------------------------------------------------------------------------
void bufferoverride::updateBuffer(long samplePos)
{
  bool doSmoothing = true;	// but in some situations, we shouldn't
  float divisorLFOvalue, bufferLFOvalue;	// the current output values of the LFOs
  long prevForcedBufferSize;	// the previous forced buffer size


	readPos = 0;	// reset for starting a new minibuffer
	prevMinibufferSize = minibufferSize;
	prevForcedBufferSize = currentForcedBufferSize;

	//--------------------------PROCESS THE LFOs----------------------------
	// update the LFOs' positions to the current position
	divisorLFO->updatePosition(prevMinibufferSize);
	bufferLFO->updatePosition(prevMinibufferSize);
	// Then get the current output values of the LFOs, which also updates their positions once more.  
	// Scale the 0.0 - 1.0 LFO output values to 0.0 - 2.0 (oscillating around 1.0).
	divisorLFOvalue = processLFOzero2two(divisorLFO);
	bufferLFOvalue = 2.0f - processLFOzero2two(bufferLFO);	// inverting it makes more pitch sense
	// & then update the stepSize for each LFO, in case the LFO parameters have changed
	if (onOffTest(divisorLFO->fTempoSync))
		divisorLFO->stepSize = currentTempoBPS * (tempoRateTable->getScalar(divisorLFO->fRate)) * numLFOpointsDivSR;
	else
		divisorLFO->stepSize = LFOrateScaled(divisorLFO->fRate) * numLFOpointsDivSR;
	if (onOffTest(bufferLFO->fTempoSync))
		bufferLFO->stepSize = currentTempoBPS * (tempoRateTable->getScalar(bufferLFO->fRate)) * numLFOpointsDivSR;
	else
		bufferLFO->stepSize = LFOrateScaled(bufferLFO->fRate) * numLFOpointsDivSR;

	//---------------------------CALCULATE FORCED BUFFER SIZE----------------------------
	// check if it's the end of this forced buffer
	if (writePos >= currentForcedBufferSize)
	{
		writePos = 0;	// start up a new forced buffer

		// check on the previous forced & minibuffers; don't smooth if the last forced buffer wasn't divided
		if (prevMinibufferSize >= currentForcedBufferSize)
			doSmoothing = false;
		else
			doSmoothing = true;

		// now update the the size of the current force buffer
		if ( onOffTest(fBufferTempoSync) &&	// the user wants to do tempo sync / beat division rate
			 (currentTempoBPS > 0.0f) ) // avoid division by zero
		{
			currentForcedBufferSize = (long) ( SAMPLERATE / (currentTempoBPS * tempoRateTable->getScalar(fBuffer)) );
			// set this true so that we make sure to do the measure syncronisation later on
		}
		else
			currentForcedBufferSize = forcedBufferSizeSamples(fBuffer);
		// apply the buffer LFO to the forced buffer size
		currentForcedBufferSize = (long) ((float)currentForcedBufferSize * bufferLFOvalue);
		// really low tempos & tempo rate values can cause huge forced buffer sizes,
		// so prevent going outside of the allocated buffer space
		if (currentForcedBufferSize > SUPER_MAX_BUFFER)
			currentForcedBufferSize = SUPER_MAX_BUFFER;
		if (currentForcedBufferSize < 2)
			currentForcedBufferSize = 2;

		// untrue this so that we don't do the measure sync calculations again unnecessarily
		needResync = false;
	}

	//-----------------------CALCULATE THE DIVISOR-------------------------
	currentBufferDivisor = bufferDivisorScaled(fDivisor);
	// apply the divisor LFO to the divisor value if there's an "active" divisor (i.e. 2 or greater)
	if (currentBufferDivisor >= 2.0f)
	{
		currentBufferDivisor *= divisorLFOvalue;
		// now it's possible that the LFO could make the divisor less than 2, 
		// which will essentially turn the effect off, so we stop the modulation at 2
		if (currentBufferDivisor < 2.0f)
			currentBufferDivisor = 2.0f;
	}

	//-----------------------CALCULATE THE MINIBUFFER SIZE-------------------------
	// this is not a new forced buffer starting up
	if (writePos > 0)
	{
		// if it's allowed, update the minibuffer size midway through this forced buffer
		if (onOffTest(fBufferInterrupt))
			minibufferSize = (long) ( (float)currentForcedBufferSize / currentBufferDivisor );
		// if it's the last minibuffer, then fill up the forced buffer to the end 
		// by extending this last minibuffer to fill up the end of the forced buffer
		long remainingForcedBuffer = currentForcedBufferSize - writePos;
		if ( (minibufferSize*2) >= remainingForcedBuffer )
			minibufferSize = remainingForcedBuffer;
	}
	// this is a new forced buffer just beginning, act accordingly, do bar sync if necessary
	else {
		// because there isn't really any division (given my implementation) when the divisor is < 2
		if (currentBufferDivisor < 2.0f) minibufferSize = currentForcedBufferSize;
		else minibufferSize = (long) ( (float)currentForcedBufferSize / currentBufferDivisor );
	}

	//-----------------------CALCULATE SMOOTHING DURATION-------------------------
	// no smoothing if the previous forced buffer wasn't divided
	if (!doSmoothing)
		smoothcount = smoothDur = 0;
	else
	{
		smoothDur = (long) (fSmooth * (float)minibufferSize);
		long maxSmoothDur;
		// if we're just starting a new forced buffer, 
		// then the samples beyond the end of the previous one are not valid
		if (writePos <= 0)
			maxSmoothDur = prevForcedBufferSize - prevMinibufferSize;
		// otherwise just make sure that we don't go outside of the allocated arrays
		else
			maxSmoothDur = SUPER_MAX_BUFFER - prevMinibufferSize;
		if (smoothDur > maxSmoothDur)
			smoothDur = maxSmoothDur;
		smoothcount = smoothDur;
		smoothStep = 1.0f / (float)(smoothDur+1);	// the gain increment for each smoothing step

		fadeOutGain = cosf(PI/(float)(4*smoothDur));
		fadeInGain = sinf(PI/(float)(4*smoothDur));
		realFadePart = (fadeOutGain * fadeOutGain) - (fadeInGain * fadeInGain);	// cosf(3.141592/2/n)
		imaginaryFadePart = 2.0f * fadeOutGain * fadeInGain;	// sinf(3.141592/2/n)
	}
}

void bufferoverride::m_signal(int n, float *const *in, float *const *out) {
	// directly move everything to the vst part	
	doTheProcess((float **)in, (float **)out, (long)n, true);
}  // end m_signal


void bufferoverride::m_help() {
}

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
void bufferoverride::doTheProcess(float **inputs, float **outputs, long sampleFrames, bool replacing)
{

//-------------------------SAFETY CHECK----------------------

	// there must have not been available memory or something (like WaveLab goofing up), 
	// so try to allocate buffers now
	if ((buffer1 == NULL)) createAudioBuffers();

	// if the creation failed, then abort audio processing
	if (buffer1 == NULL) return;


//-------------------------INITIALIZATIONS----------------------
	// this is a handy value to have during LFO calculations & wasteful to recalculate at every sample
	numLFOpointsDivSR = NUM_LFO_POINTS_FLOAT / SAMPLERATE;
	divisorLFO->pickTheLFOwaveform();
	bufferLFO->pickTheLFOwaveform();

	// calculate this scaler value to minimize calculations later during processOutput()
//	float inputGain = 1.0f - fDryWetMix;
//	float outputGain = fDryWetMix;
	float inputGain = sqrtf(1.0f - fDryWetMix);
	float outputGain = sqrtf(fDryWetMix);


//-----------------------TEMPO STUFF---------------------------
	// figure out the current tempo if we're doing tempo sync
	if ( onOffTest(fBufferTempoSync) || 
			(onOffTest(divisorLFO->fTempoSync) || onOffTest(bufferLFO->fTempoSync)) )
	{
		// calculate the tempo at the current processing buffer
		if ( (fTempo > 0.0f) || (hostCanDoTempo != 1) )	// get the tempo from the user parameter
		{
			currentTempoBPS = tempoScaled(fTempo) / 60.0f;
			needResync = false;	// we don't want it true if we're not syncing to host tempo
		}
		else	// get the tempo from the host
		{
/*			timeInfo = getTimeInfo(kBeatSyncTimeInfoFlags);
			if (timeInfo)
			{
				if (kVstTempoValid & timeInfo->flags)
					currentTempoBPS = (float)timeInfo->tempo / 60.0f;
				else
					currentTempoBPS = tempoScaled(fTempo) / 60.0f;
//				currentTempoBPS = ((float)tempoAt(reportCurrentPosition())) / 600000.0f;
				// but zero & negative tempos are bad, so get the user tempo value instead if that happens
				if (currentTempoBPS <= 0.0f)
					currentTempoBPS = tempoScaled(fTempo) / 60.0f;
				//
				// check if audio playback has just restarted & reset buffer stuff if it has (for measure sync)
				if (timeInfo->flags & kVstTransportChanged)
				{
					needResync = true;
					currentForcedBufferSize = 1;
					writePos = 1;
					minibufferSize = 1;
					prevMinibufferSize = 0;
					smoothcount = smoothDur = 0;
				}
			}
			else	// do the same stuff as above if the timeInfo gets a null pointer
			{
*/				currentTempoBPS = tempoScaled(fTempo) / 60.0f;
				needResync = false;	// we don't want it true if we're not syncing to host tempo
//			}
		}
	}


//-----------------------AUDIO STUFF---------------------------
	// here we begin the audio output loop, which has two checkpoints at the beginning
	for (long samplecount = 0; (samplecount < sampleFrames); samplecount++)
	{
		// check if it's the end of this minibuffer
		if (readPos >= minibufferSize)
			updateBuffer(samplecount);

		// store the latest input samples into the buffers
		buffer1[writePos] = inputs[0][samplecount];

		// get the current output without any smoothing
		float out1 = buffer1[readPos];

		// and if smoothing is taking place, get the smoothed audio output
		if (smoothcount > 0)
		{
			out1 = (out1 * fadeInGain) + (buffer1[readPos+prevMinibufferSize] * fadeOutGain);

			smoothcount--;

			fadeInGain = (fadeOutGain * imaginaryFadePart) + (fadeInGain * realFadePart);
			fadeOutGain = (realFadePart * fadeOutGain) - (imaginaryFadePart * fadeInGain);
		}

		// write the output samples into the output stream
		if (replacing)
		{
			outputs[0][samplecount] = (out1 * outputGain) + (inputs[0][samplecount] * inputGain);
		}
		else
		{
			outputs[0][samplecount] += (out1 * outputGain) + (inputs[0][samplecount] * inputGain);
		}

		// increment the position trackers
		readPos++;
		writePos++;
	}
}
