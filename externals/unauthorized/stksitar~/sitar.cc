 /******************************************/  
/*  Karplus-Strong sitar string model   */
/*  by Perry Cook, 1995-96                */
/*					  */
/*  There exist at least two patents,     */
/*  assigned to Stanford, bearing the     */
/*  names of Karplus and/or Strong.       */
/******************************************/

#include "sitar.h"

sitar :: sitar(StkFloat lowestFreq)
{
  length = (long) (SRATE / lowestFreq + 1);
  loopGain = (StkFloat) 0.999;
  loopFilt = new OneZero();
  loopFilt->setZero(0.01);
  delayLine = new DelayA(0.5 * length, length);
  delay = length/2;
  delayTarg = delay;
  envelope = new ADSR();
  noise = new Noise;
  envelope->setAllTimes(0.001,0.04,0.0,0.5);
  this->clear();
}

sitar :: ~sitar()
{
  delete loopFilt;
  delete delayLine;
  delete envelope;
  delete noise;
}

void sitar :: clear()
{
  loopFilt->clear();
  delayLine->clear();
}

void sitar :: setFreq(StkFloat frequency)
{
  delayTarg = (SRATE / frequency);
  delay = delayTarg * (1.0 + (0.05 * noise->tick()));
  delayLine->setDelay(delay);
  loopGain = (StkFloat) 0.995 + (frequency * (StkFloat)  0.000001);
  if (loopGain>1.0) loopGain = (StkFloat) 0.9995;
}

void sitar :: pluck(StkFloat amplitude)
{
  envelope->keyOn();
}

void sitar :: noteOn(StkFloat freq, StkFloat amp)
{
  this->setFreq(freq);
  this->pluck(amp);
  amPluck = 0.05 * amp;
#if defined(_debug_)        
  printf("sitar : NoteOn: Freq=%lf Amp=%lf\n",freq,amp);
#endif    
}

void sitar :: noteOff(StkFloat amp)
{
  loopGain = (StkFloat) 1.0 - amp;
#if defined(_debug_)        
  printf("sitar : NoteOff: Amp=%lf\n",amp);
#endif    
}

StkFloat sitar :: tick()
{
  StkFloat temp;

  temp = delayLine->lastOut();
  if (fabs(temp) > 1.0) {
    loopGain = 0.1;
    this->noteOff(0.9);
    delay = delayTarg;
    delayLine->setDelay(delay);
  }

  temp *= loopGain;

  if (fabs(delayTarg - delay) > 0.001)	{
    if (delayTarg < delay)
      delay *= 0.99999;
    else
      delay *= 1.00001;
    delayLine->setDelay(delay);
  }

  lastOutput_ = delayLine->tick(loopFilt->tick(temp)
                               + (amPluck * envelope->tick() * noise->tick()));
  
  return lastOutput_;
}

