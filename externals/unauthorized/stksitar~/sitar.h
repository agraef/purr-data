/******************************************/  
/*  Karplus-Strong Sitar1 string model   */
/*  by Perry Cook, 1995-96                */
/*                                        */
/*  There exist at least two patents,     */
/*  assigned to Stanford, bearing the     */
/*  names of Karplus and/or Strong.       */
/******************************************/

#if !defined(__sitar_h)
#define __sitar_h

#include "Instrmnt.h"
#include "DelayA.h"
#include "OneZero.h"
#include "Noise.h"
#include "ADSR.h"

class sitar : public Instrmnt
{
protected:  
  DelayA *delayLine;
  OneZero *loopFilt;
  ADSR *envelope;
  Noise *noise;
  long length;
  StkFloat loopGain;
  StkFloat amPluck;
  StkFloat delay;
  StkFloat delayTarg;
public:
  sitar(StkFloat lowestFreq);
  ~sitar();
  void clear();
  virtual void setFreq(StkFloat frequency);
  void pluck(StkFloat amplitude);
  virtual void noteOn(StkFloat freq, StkFloat amp);
  virtual void noteOff(StkFloat amp);
  virtual StkFloat tick();
};

#endif

