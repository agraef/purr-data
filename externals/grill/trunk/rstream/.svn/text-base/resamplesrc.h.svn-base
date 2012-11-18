#ifndef __RESAMPLESRC_H
#define __RESAMPLESRC_H

#ifdef HAVE_SRC

#include "resample.h"
#include <samplerate.h>

class ResampleSRC
    : public Resample
{
public:
    ResampleSRC(int c);
    virtual ~ResampleSRC();

    virtual int Do(int chns,Fifo<float> *input,float *const *output,int need,double ratio);

private:
	SRC_STATE **src_state;
};

#endif

#endif
