/******************************************/  
/*  Karplus-Strong drone string model   */
/*  by Perry Cook, 1995-96                */
/*                                        */
/*  There exist at least two patents,     */
/*  assigned to Stanford, bearing the     */
/*  names of Karplus and/or Strong.       */
/******************************************/

#if !defined(__drone_h)
#define __drone_h

#include "Instrmnt.h" 
#include "DelayA.h"
#include "OneZero.h"
#include "ADSR.h" 
#include "Noise.h" 

class drone : public Instrmnt
{
protected:  
  DelayA *delayLine;
  ADSR *envelope;
  Noise *noise;
  OneZero *loopFilt;
  long length;
  MY_FLOAT loopGain;
public:
  drone(MY_FLOAT lowestFreq);
  ~drone();
  void clear();
  virtual void setFreq(MY_FLOAT frequency);
  void pluck(MY_FLOAT amplitude);
  virtual void noteOn(MY_FLOAT freq, MY_FLOAT amp);
  virtual void noteOff(MY_FLOAT amp);
  virtual MY_FLOAT tick();
};

#endif

