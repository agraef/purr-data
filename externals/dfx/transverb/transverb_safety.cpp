
/* DFX Transverb transverb by Tom 7 and Marc 3 */

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "transverb.h"
#include "FIRfilter.h"


// _ ___

transverb::transverb(int argc, t_atom *argv)  {

  fBsize   = (2700.0f-BUFFER_MIN)/(BUFFER_MAX-BUFFER_MIN); 		// "ms"
  drymix 	= 0.5f; 						// "dB"

  mix1 		= 1.0f; 						// "dB"
  dist1		= 0.90009f; 						// "units"
  speed1   = (0.0f-SPEED_MIN)/(SPEED_MAX-SPEED_MIN);			// "units"
  feed1		= 0.0f; 						// "units"

  mix2		= 0.0f;							// "dB"
  dist2		= 0.1f;							// "units"
  speed2   = (1.0f-SPEED_MIN)/(SPEED_MAX-SPEED_MIN);			// "units"
  feed2 	= 0.0f; 						// "units"
  
  fQuality	= 1.0f;
  fTomsound	= 0.0f;
  fSpeed1mode	= 0.0f;
  fSpeed2mode	= 0.0f;

  // default this to something, at least for the sake of getGetTailSize()
  MAXBUF = (int) (BUFFER_MAX * 44.1f);
  buf1[0] = NULL;
  buf2[0] = NULL;
#ifdef TRANSVERB_STEREO
  buf1[1] = NULL;
  buf2[1] = NULL;
#endif
  filter1 = new IIRfilter[NUM_CHANNELS];
  filter2 = new IIRfilter[NUM_CHANNELS];
  firCoefficients1 = new float[numFIRtaps];
  firCoefficients2 = new float[numFIRtaps];
  suspend();
  srand((unsigned int)time(NULL));	// sets a seed value for rand() from the system clock

	post("_ ____transverb~ with flext");

	sr = (int) Samplerate();
	blocksize = Blocksize(); 
	
	post("_ samplerat :: %d",sr);
	
	// The constructor of your class is responsible for
	// setting up inlets and outlets and for registering
	// inlet-methods:
	
	AddInSignal();
//	AddInAnything();
	AddInFloat(9);
	AddOutSignal();         // 1 audio out [ == AddOutSignal(1) ]
	
	SetupInOut();           // set up inlets and outlets. 
				// Must get called once!

	// Now we need to bind the handler function to our
	// inlets, 
	FLEXT_ADDMETHOD( 0,setMix);
	FLEXT_ADDMETHOD( 1,setBsize);
	FLEXT_ADDMETHOD( 2,setMix1);
	FLEXT_ADDMETHOD( 3,setSpeed1);
	FLEXT_ADDMETHOD( 4,setFeed1);
	FLEXT_ADDMETHOD( 5,setDist1);
	FLEXT_ADDMETHOD( 6,setMix2);
	FLEXT_ADDMETHOD( 7,setSpeed2);
	FLEXT_ADDMETHOD( 8,setFeed2);
//	FLEXT_ADDMETHOD( 9,setDist2);
//	FLEXT_ADDMETHOD(10,setQuality);
//	FLEXT_ADDMETHOD(11,setTom);
	
	// We're done constructing:
	post("_ ____ ____ _");
	
} // end of constructor


transverb::~transverb() {

  if (buf1[0])
    free(buf1[0]);
  if (buf2[0])
    free(buf2[0]);
#ifdef TRANSVERB_STEREO
  if (buf1[1])
    free(buf1[1]);
  if (buf2[1])
    free(buf2[1]);
#endif
  if (filter1)
    delete[] filter1;
  if (filter2)
    delete[] filter2;
  if (firCoefficients1)
    delete[] firCoefficients1;
  if (firCoefficients2)
    delete[] firCoefficients2;

}

// GIMME class:
FLEXT_NEW_TILDE_G("transverb~", transverb)



void transverb::suspend () {

  clearBuffers();
  writer = 0;
  read1 = read2 = 0.0;
  smoothcount1[0] = smoothcount2[0] = 0;
  lastr1val[0] = lastr2val[0] = 0.0f;
  filter1[0].reset();
  filter2[0].reset();
#ifdef TRANSVERB_STEREO
  smoothcount1[1] = smoothcount2[1] = 0;
  lastr1val[1] = lastr2val[1] = 0.0f;
  filter1[1].reset();
  filter2[1].reset();
#endif
  SAMPLERATE = 44100.0f;
  bsize = bufferScaled(fBsize);
  speed1hasChanged = speed2hasChanged = true;
}

void transverb::resume() {

  createAudioBuffers();
  bsize = bufferScaled(fBsize);
}

void transverb::createAudioBuffers() {

  SAMPLERATE = 44100.0f;

  long oldmax = MAXBUF;
  MAXBUF = (int) (BUFFER_MAX * 0.001f * SAMPLERATE);

  // if the sampling rate (& therefore the max buffer size) has changed, 
  // then delete & reallocate the buffers according to the sampling rate
  if (MAXBUF != oldmax)
  {
    if (buf1[0] != NULL)
      free(buf1[0]);
    buf1[0] = NULL;
    if (buf2[0] != NULL)
      free(buf2[0]);
    buf2[0] = NULL;
#ifdef TRANSVERB_STEREO
    if (buf1[1] != NULL)
      free(buf1[1]);
    buf1[1] = NULL;
    if (buf2[1] != NULL)
      free(buf2[1]);
    buf2[1] = NULL;
#endif
  }
  if (buf1[0] == NULL)
    buf1[0] = (float*)malloc(MAXBUF * sizeof (float));
  if (buf2[0] == NULL)
    buf2[0] = (float*)malloc(MAXBUF * sizeof (float));
#ifdef TRANSVERB_STEREO
  if (buf1[1] == NULL)
    buf1[1] = (float*)malloc(MAXBUF * sizeof (float));
  if (buf2[1] == NULL)
    buf2[1] = (float*)malloc(MAXBUF * sizeof (float));
#endif
}

void transverb::clearBuffers() {
  if ( (buf1[0] != NULL) && (buf2[0] != NULL) ) {
    for (int j=0; j < MAXBUF; j++) buf1[0][j] = buf2[0][j] = 0.0f;
  }
#ifdef TRANSVERB_STEREO
  if ( (buf1[1] != NULL) && (buf2[1] != NULL) ) {
    for (int k=0; k < MAXBUF; k++) buf1[1][k] = buf2[1][k] = 0.0f;
  }
#endif
}


/* this randomizes the values of all of Transverb's parameters, sometimes in smart ways 
void transverb::randomizeParameters(bool writeAutomation) {

  float mixSum, newDrymix, newMix1, newMix2, mixScalar, tempRand;


	// randomize the first 7 parameters
	for (long i=0; (i < kDrymix); i++)
	{
		// make slow speeds more probable (for fairer distribution)
		if ( (i == kSpeed1) || (i == kSpeed1) )
		{
			tempRand = (float)rand() / RAND_MAX_FLOAT;
			if ( tempRand < 0.5f )
				tempRand *= fabsf(SPEED_MIN/(SPEED_MAX-SPEED_MIN)) * 2.0f;
			else
				tempRand = fabsf(SPEED_MIN/(SPEED_MAX-SPEED_MIN)) + 
							( tempRand * fabsf(SPEED_MAX/(SPEED_MAX-SPEED_MIN)) );
			setParameter(i, (float)rand()/RAND_MAX_FLOAT);
		}
		// make smaller buffer sizes more probable (because they sound better)
		else if (i == kBsize)
		{
			setParameter( i, powf(((float)rand()/RAND_MAX_FLOAT*0.93f)+0.07f, 1.38f) );
		}
		else
		{
			setParameter(i, (float)rand()/RAND_MAX_FLOAT);
		}
	}

	// store the current total gain sum
	mixSum = gainScaled(drymixParam) + 
				gainScaled(mix1param) + 
				gainScaled(mix2param);

	// randomize the mix parameters
	drymix = (float)rand() / RAND_MAX_FLOAT;
	mix1   = (float)rand() / RAND_MAX_FLOAT;
	mix2   = (float)rand() / RAND_MAX_FLOAT;

	// apply the scalar to the new mix parameter values
	newDrymix = sqrtf( mixScalar * gainScaled(newDrymix) );
	newMix1 = sqrtf( mixScalar * gainScaled(newMix1) );
	newMix2 = sqrtf( mixScalar * gainScaled(newMix2) );

	// clip the the mix values at 1.0 so that we don't get mega-feedback blasts
	newDrymix = ( newDrymix > 1.0f ? 1.0f : newDrymix );
	newMix1 = ( newMix1 > 1.0f ? 1.0f : newMix1 );
	newMix2 = ( newMix2 > 1.0f ? 1.0f : newMix2 );

	setParameter(kDrymix, newDrymix);
	setParameter(kMix1, newMix1);
	setParameter(kMix2, newMix2);
	setParameter( kQuality, (float)(((rand()%5)+1)%3) * 0.5f );
	setParameter( kTomsound, (float)((rand()%3)%2) );


}
*/


void transverb::m_signal(int n, float *const *in, float *const *out)
{
	float *outs = out[0];	
	
	// We are now ready for the main signal loop
	// float **in, float **outputs, long samples, int replacing
	
	int i = 0;
//	while (n--) {
		processX((float **)in,(float **)out,(long int)n,0);
//		*outs++  = (i++);
//	}
}  // end m_signal


void transverb::processReplacing(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples, 1);
}

void transverb::process(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples, 0);
}















void transverb::processX(float **in, float **outputs, long samples, 
		      int replacing) {
  // float these 3 temp variables are for preserving states when looping through channels
  int writertemp;
  double read1temp, read2temp;
  // int versions of these float values, for reducing casting operations
  int speed1int, speed2int, read1int, read2int;
  int lowpass1pos, lowpass2pos;	// position trackers for the lowpass filters
  float r1val, r2val;	// delay buffer output values
  double bsize_float = (double)bsize;	// cut down on casting
  int filterMode1, filterMode2;	// the type of filtering to use in ultra hi-fi mode
  float mug1, mug2;	// make-up gain for lowpass filtering
  float quietNoise = 1.0e-15f;	// value added into the buffers to prevent denormals


  // there must have not been available memory or something (like WaveLab goofing up), 
  // so try to allocate buffers now
  if ( ((buf1[0] == NULL) || (buf2[0] == NULL)) 
  #ifdef TRANSVERB_STEREO
       || ((buf1[1] == NULL) || (buf2[1] == NULL)) 
  #endif
     ) createAudioBuffers();
    
  // if the creation failed, then abort audio processing
  if ( (buf1[0] == NULL) || (buf2[0] == NULL) )
    return;
#ifdef TRANSVERB_STEREO
  if ( (buf1[1] == NULL) || (buf2[1] == NULL) )
    return;
#endif

  SAMPLERATE = 44100.0f;

  filterMode1 = filterMode2 = useNothing;	// reset these for now
  if (quality == ultrahifi)
  {
    // check to see if we need to lowpass the first delay head & init coefficients if so
    if (speed1 > 1.0f)
    {
      filterMode1 = useLowpassIIR;
      speed1int = (int)speed1;
      // it becomes too costly to try to IIR at > 5x speeds, so switch to FIR filtering
      if (speed1int >= 5)
      {
        filterMode1 = useLowpassFIR;
        mug1 = powf( (speed1*0.2f), 0.78f );	// compensate for gain lost from filtering
        // update the coefficients only if necessary
        if (speed1hasChanged)
        {
          calculateFIRidealLowpassCoefficients((SAMPLERATE/speed1)*SHELF_START_IIR, SAMPLERATE, numFIRtaps, firCoefficients1);
          applyKaiserWindow(numFIRtaps, firCoefficients1, 60.0f);
          speed1hasChanged = false;
        }
      }
      else if (speed1hasChanged)
      {
        filter1[0].calculateLowpassCoefficients((SAMPLERATE/speed1)*SHELF_START_IIR, SAMPLERATE);
      #ifdef TRANSVERB_STEREO
        filter1[1].copyCoefficients(filter1);
      #endif
        speed1hasChanged = false;
      }
    }
    // we need to highpass the delay head to remove mega sub bass
    else
    {
      filterMode1 = useHighpass;
      if (speed1hasChanged)
      {
        filter1[0].calculateHighpassCoefficients(33.3f/speed1, SAMPLERATE);
      #ifdef TRANSVERB_STEREO
        filter1[1].copyCoefficients(filter1);
      #endif
        speed1hasChanged = false;
      }
    }

    // check to see if we need to lowpass the second delay head & init coefficients if so
    if (speed2 > 1.0f)
    {
      filterMode2 = useLowpassIIR;
      speed2int = (int)speed2;
      if (speed2int >= 5)
      {
        filterMode2 = useLowpassFIR;
        mug2 = powf( (speed2*0.2f), 0.78f );	// compensate for gain lost from filtering
        if (speed2hasChanged)
        {
          calculateFIRidealLowpassCoefficients((SAMPLERATE/speed2)*SHELF_START_IIR, SAMPLERATE, numFIRtaps, firCoefficients2);
          applyKaiserWindow(numFIRtaps, firCoefficients2, 60.0f);
          speed2hasChanged = false;
        }
      }
      else if (speed2hasChanged)
      {
        filter2[0].calculateLowpassCoefficients((SAMPLERATE/speed2)*SHELF_START_IIR, SAMPLERATE);
      #ifdef TRANSVERB_STEREO
        filter2[1].copyCoefficients(filter2);
      #endif
        speed2hasChanged = false;
      }
    }
    // we need to highpass the delay head to remove mega sub bass
    else
    {
      filterMode2 = useHighpass;
      if (speed2hasChanged)
      {
        filter2[0].calculateHighpassCoefficients(33.3f/speed2, SAMPLERATE);
      #ifdef TRANSVERB_STEREO
        filter2[1].copyCoefficients(filter2);
      #endif
        speed2hasChanged = false;
      }
    }
  }

  /////////////   M A R C S O U N D   //////////////
  // do it proper
  if (!tomsound) {
    // store these so that they can be restored before the next loop iteration
    read1temp = read1;
    read2temp = read2;
    writertemp = writer;


    for(int i=0; i < NUM_CHANNELS; i++) {	// channels loop
      lowpass1pos = (int)read1;
      lowpass2pos = (int)read2;

      for(long j=0; j < samples; j++) {		// samples loop

        read1int = (int)read1;
        read2int = (int)read2;

        /* read from read heads */
        switch(quality)
        {
          // no interpolation or filtering
          case dirtfi:
            r1val = buf1[i][read1int];
            r2val = buf2[i][read2int];
            break;
          // spline interpolation, but no filtering
          case hifi:
//            r1val = interpolateLinear(buf1[i], read1, bsize, writer-read1int);
            r1val = interpolateHermite(buf1[i], read1, bsize, writer-read1int);
            r2val = interpolateHermite(buf2[i], read2, bsize, writer-read2int);
            break;
          // spline interpolation plus anti-aliasing lowpass filtering for high speeds 
          // or sub-bass-removing highpass filtering for low speeds
          case ultrahifi:
            float lp1, lp2;
            switch (filterMode1)
            {
              case useHighpass:
              case useLowpassIIR:
                // interpolate the values in the IIR output history
                r1val = interpolateHermitePostFilter(&filter1[i], read1);
                break;
              case useLowpassFIR:
                // get 2 consecutive FIR output values for linear interpolation
                lp1 = processFIRfilter(buf1[i], numFIRtaps, firCoefficients1, 
                                       (read1int-numFIRtaps+bsize)%bsize, bsize);
                lp2 = processFIRfilter(buf1[i], numFIRtaps, firCoefficients1, 
                                       (read1int-numFIRtaps+1+bsize)%bsize, bsize);
                // interpolate output linearly (avoid shit sound) & compensate gain
                r1val = interpolateLinear2values(lp1, lp2, read1) * mug1;
                break;
              default:
                r1val = interpolateHermite(buf1[i], read1, bsize, writer-read1int);
                break;
            }
            switch (filterMode2)
            {
              case useHighpass:
              case useLowpassIIR:
                // interpolate the values in the IIR output history
                r2val = interpolateHermitePostFilter(&filter2[i], read2);
                break;
              case useLowpassFIR:
                // get 2 consecutive FIR output values for linear interpolation
                lp1 = processFIRfilter(buf2[i], numFIRtaps, firCoefficients2, 
                                       (read2int-numFIRtaps+bsize)%bsize, bsize);
                lp2 = processFIRfilter(buf2[i], numFIRtaps, firCoefficients2, 
                                       (read2int-numFIRtaps+1+bsize)%bsize, bsize);
                // interpolate output linearly (avoid shit sound) & compensate gain
                r2val = interpolateLinear2values(lp1, lp2, read2) * mug2;
                break;
              default:
                r2val = interpolateHermite(buf2[i], read2, bsize, writer-read2int);
                break;
            }
            break;
          // dirt-fi style again for the safety net
          default:
            r1val = buf1[i][read1int];
            r2val = buf2[i][read2int];
            break;
        }	// end of quality switch

        // crossfade the last stored smoothing sample with 
        // the current sample if smoothing is in progress
        if (smoothcount1[i]) {
          r1val = ( r1val * (1.0f - (smoothstep1[i]*(float)smoothcount1[i])) ) 
                  + (lastr1val[i] * smoothstep1[i]*(float)smoothcount1[i]);
          (smoothcount1[i])--;
        }
        if (smoothcount2[i]) {
          r2val = ( r2val * (1.0f - (smoothstep2[i]*(float)smoothcount2[i])) ) 
                  + (lastr2val[i] * smoothstep2[i]*(float)smoothcount2[i]);
          (smoothcount2[i])--;
        }
        
        /* then write into buffer (w/ feedback) */

        // mix very quiet noise (-300 dB) into the input singal 
        // to hopefully avoid any denormal values from IIR filtering
/*        buf1[i][writer] = in[i][j] + (feed1 * r1val * mix1) + quietNoise;
        buf2[i][writer] = in[i][j] + (feed2 * r2val * mix2) + quietNoise;
        quietNoise = -quietNoise;	// flip its sign
*/
        buf1[i][writer] = in[i][j] + (feed1 * r1val * mix1);
        buf2[i][writer] = in[i][j] + (feed2 * r2val * mix2);
        undenormalize(buf1[i][writer]);
        undenormalize(buf2[i][writer]);

        /* make output 
        if (replacing) 
          outputs[i][j] = (in[i][j]*drymix) + (r1val*mix1) + (r2val*mix2);
        else
	  outputs[i][j] += (in[i][j]*drymix) + (r1val*mix1) + (r2val*mix2);
	*/
	
        *outputs[j] += (*in[j]*drymix) + (r1val*mix1) + (r2val*mix2);


        /* start smoothing stuff if the writer has 
           passed a reader or vice versa.
             (check the positions before wrapping around the heads)
        */

        if ( ( (read1int < writer) && 
               (((int)(read1+(double)speed1)) >= (writer+1)) ) || 
             ( (read1int >= writer) && 
               (((int)(read1+(double)speed1)) <= (writer+1)) ) ) {
        /* check because, at slow speeds, 
        it's possible to go into this twice or more in a row */
          if (smoothcount1[i] <= 0) {
            // store the most recent output as the channel 1 smoothing sample
            lastr1val[i] = r1val;
            // truncate the smoothing duration if we're using too small of a buffer size
            smoothdur1[i] = 
              (SMOOTH_DUR > (int)(bsize_float/(double)speed1)) ? 
              (int)(bsize_float/(double)speed1) : SMOOTH_DUR;
            smoothstep1[i] = 1.0f / (float)smoothdur1[i];	// the scalar step value
            smoothcount1[i] = smoothdur1[i];	// set the counter to the total duration
          }
        }

        // channel 2 smoothing stuff
        if ( ( (read2int < writer) && 
               (((int)(read2+(double)speed2)) >= (writer+1)) ) || 
             ( (read2int >= writer) && 
               (((int)(read2+(double)speed2)) <= (writer+1)) ) ) {
          if (smoothcount2[i] <= 0) {
            // store the most recent output as the channel 2 smoothing sample
            lastr2val[i] = r2val;
            // truncate the smoothing duration if we're using too small of a buffer size
            smoothdur2[i] = 
              (SMOOTH_DUR > (int)(bsize_float/(double)speed2)) ? 
              (int)(bsize_float/(double)speed2) : SMOOTH_DUR;
            smoothstep2[i] = 1.0f / (float)smoothdur2[i];	// the scalar step value
            smoothcount2[i] = smoothdur2[i];	// set the counter to the total duration
          }
        }

        /* update rw heads */
        writer++;
        read1 += (double)speed1;
        read2 += (double)speed2;

        // wrap around the rw heads if they've gone past the end of the buffer
        writer %= bsize;
        if (read1 >= bsize_float)
          read1 = fmod(fabs(read1), bsize_float);
        if (read2 >= bsize_float)
          read2 = fmod(fabs(read2), bsize_float);

        // if we're doing IIR lowpass filtering, 
        // then we probably need to process a few consecutive samples in order 
        // to get the continuous impulse (or whatever you call that), 
        // probably whatever the speed multiplier is, that's how many samples
        if (filterMode1 == useLowpassIIR)
        {
          int lowpasscount = 0;
          while (lowpasscount < speed1int)
          {
            switch (speed1int - lowpasscount)
            {
              case 1:
                filter1[i].processH1(buf1[i][lowpass1pos]);
                lowpass1pos = (lowpass1pos + 1) % bsize;
                lowpasscount++;
                break;
              case 2:
                filter1[i].processH2(buf1[i], lowpass1pos, bsize);
                lowpass1pos = (lowpass1pos + 2) % bsize;
                lowpasscount += 2;
                break;
              case 3:
                filter1[i].processH3(buf1[i], lowpass1pos, bsize);
                lowpass1pos = (lowpass1pos + 3) % bsize;
                lowpasscount += 3;
                break;
              default:
                filter1[i].processH4(buf1[i], lowpass1pos, bsize);
                lowpass1pos = (lowpass1pos + 4) % bsize;
                lowpasscount += 4;
                break;
            }
          }
          read1int = (int)read1;
          // make sure that we don't need to do one more sample
          if ( ((lowpass1pos < read1int) && ((lowpass1pos+1) == read1int)) ||
               ((lowpass1pos == (bsize-1)) && (read1int == 0)) )
          {
            filter1[i].processH1(buf1[i][lowpass1pos]);
            lowpass1pos = (lowpass1pos+1) % bsize;
          }
        }
        // it's simpler for highpassing; 
        // we may not even need to process anything for this sample
        else if (filterMode1 == useHighpass)
        {
          // only if we've traversed to a new integer sample position
          if ((int)read1 != read1int)
            filter1[i].process(buf1[i][read1int]);
        }

        // channel 2 filtering stuff
        if (filterMode2 == useLowpassIIR)
        {
          int lowpasscount = 0;
          while (lowpasscount < speed2int)
          {
            switch (speed2int - lowpasscount)
            {
              case 1:
                filter2[i].processH1(buf2[i][lowpass2pos]);
                lowpass2pos = (lowpass2pos + 1) % bsize;
                lowpasscount++;
                break;
              case 2:
                filter2[i].processH2(buf2[i], lowpass2pos, bsize);
                lowpass2pos = (lowpass2pos + 2) % bsize;
                lowpasscount += 2;
                break;
              case 3:
                filter2[i].processH3(buf2[i], lowpass2pos, bsize);
                lowpass2pos = (lowpass2pos + 3) % bsize;
                lowpasscount += 3;
                break;
              default:
                filter2[i].processH4(buf2[i], lowpass2pos, bsize);
                lowpass2pos = (lowpass2pos + 4) % bsize;
                lowpasscount += 4;
                break;
            }
          }
          read2int = (int)read2;
          if ( ((lowpass2pos < read2int) && ((lowpass2pos+1) == read2int)) ||
               ((lowpass2pos == (bsize-1)) && (read2int == 0)) )
          {
            filter2[i].processH1(buf2[i][lowpass2pos]);
            lowpass2pos = (lowpass2pos+1) % bsize;
          }
        }
        else if (filterMode2 == useHighpass)
        {
          if ((int)read2 != read2int)
            filter2[i].process(buf2[i][read2int]);
        }
      }	/* end of samples loop */

    #ifdef TRANSVERB_STEREO
      if (i == 0) {
        // restore these place-holders for the second loop iteration
        read1 = read1temp;
        read2 = read2temp;
        writer = writertemp;
      }
    #endif
    }	/* end of channels loop */
  }	/* end of if(TOMSOUND) */




  /////////////   T O M S O U N D   //////////////
  else {
      for(long j=0; j < samples; j++) {
        for(int i=0; i < NUM_CHANNELS; i++) {

	  /* read from read heads */

	  switch(quality) {
	    case dirtfi:
	      r1val = mix1 * buf1[i][(int)read1];
	      r2val = mix2 * buf1[i][(int)read2];
	      break;
	    case hifi:
	    case ultrahifi:
	      r1val = mix1 * interpolateHermite(buf1[i], read1, bsize, 333);
	      r2val = mix2 * interpolateHermite(buf1[i], read2, bsize, 333);
	      break;
	    default:
	      r1val = mix1 * buf1[i][(int)read1];
	      r2val = mix2 * buf1[i][(int)read2];
	      break;
	    }

	  /* then write into buffer (w/ feedback) */

	  buf1[i][writer] = 
	    in[i][j] + 
	    feed1 * r1val + 
	    feed2 * r2val;
      
	  /* update rw heads */
	  writer++;
	  writer %= bsize;

	  read1 += (double)speed1;
	  read2 += (double)speed2;

	  if (read1 >= bsize_float)
	    read1 = fmod(fabs(read1), bsize_float);
	  if (read2 >= bsize_float)
	    read2 = fmod(fabs(read2), bsize_float);

	  /* make output */
	  if (replacing) 
	    outputs[i][j] = in[i][j] * drymix + r1val + r2val;
	  else
	    outputs[i][j] += in[i][j] * drymix + r1val + r2val;
        }
      }
    }

}
