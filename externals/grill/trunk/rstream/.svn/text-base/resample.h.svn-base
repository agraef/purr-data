#ifndef __RESAMPLE_H
#define __RESAMPLE_H

#include "fifo.h"

class Resample
{
public:
    Resample(int c);
    virtual ~Resample();

    virtual int Do(int chns,Fifo<float> *input,float *const *output,int frames,double ratio);

protected:
    const int channels; // maximum channels

private:
    double pos;
    typedef float Temp[4];
    Temp *temp; // temporary 4-point interpolation space

    void Clear()
    {
        pos = 0;
        for(int i = 0; i < channels; ++i) 
            memset(temp[i],0,sizeof(*temp));
    }

    int Shift(int sh)
    {
    }
};

#endif
