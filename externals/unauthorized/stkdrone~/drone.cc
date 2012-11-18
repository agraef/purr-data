 /******************************************/  
/*  Karplus-Strong drone string model   */
/*  by Perry Cook, 1995-96                */
/*					  */
/*  There exist at least two patents,     */
/*  assigned to Stanford, bearing the     */
/*  names of Karplus and/or Strong.       */
/******************************************/

#include "drone.h"

drone :: drone(StkFloat lowestFreq)
{
  length = (long) (SRATE / lowestFreq + 1);
  loopGain = (StkFloat) 0.999;
  loopFilt = new OneZero();
  delayLine = new DelayA(0.5*length, length);
  envelope = new ADSR();
  noise = new Noise;
  envelope->setAllTimes(2.0,0.5,0.0,0.5);
  this->clear();
}

drone :: ~drone()
{
  delete loopFilt;
  delete delayLine;
  delete envelope;
  delete noise;
}

void drone :: clear()
{
  loopFilt->clear();
  delayLine->clear();
}

void drone :: setFreq(StkFloat frequency)
{
  StkFloat delay;
  delay = (SRATE / frequency);
  delayLine->setDelay(delay - 0.5);
  loopGain = (StkFloat) 0.997 + (frequency * (StkFloat)  0.000002);
  if (loopGain>1.0) loopGain = (StkFloat) 0.99999;
}

void drone :: pluck(StkFloat amplitude)
{
  envelope->keyOn();
}

void drone :: noteOn(StkFloat freq, StkFloat amp)
{
  this->setFreq(freq);
  this->pluck(amp);
#if defined(_debug_)        
  printf("drone : NoteOn: Freq=%lf Amp=%lf\n",freq,amp);
#endif    
}

void drone :: noteOff(StkFloat amp)
{
  loopGain = (StkFloat) 1.0 - amp;
#if defined(_debug_)        
  printf("drone : NoteOff: Amp=%lf\n",amp);
#endif    
}

StkFloat drone :: tick()
{
  /* check this out */
  /* here's the whole inner loop of the instrument!!  */
  lastOutput_ = delayLine->tick(loopFilt->tick((delayLine->lastOut() * loopGain))
		+ (0.005 * envelope->tick() * noise->tick())); 
  return lastOutput_;
}

