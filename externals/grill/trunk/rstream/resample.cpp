#include "resample.h"
#include <assert.h>

Resample::Resample(int c)
    : channels(c)
{
    temp = new Temp[c];
    Clear();
}

Resample::~Resample() 
{
    if(temp) delete[] temp;
}

int Resample::Do(int chns,Fifo<float> *input,float *const *output,int frames,double ratio)
{
    assert(chns <= channels && chns);
    assert(ratio);
    if(ratio == 1) {
	    // no resampling necessary

	    // hopefully all channel fifos advance uniformly.....
        int got = 0;
	    for(;;) {
		    int need = frames-got;
            if(need <= 0) break;

            int have = input[0].Have();
            if(!have) break; // buffer underrun

    	    int read = -1;
            for(int i = 0; i < chns; ++i) {
			    int r = input[i].Read(need,output[i]+got);
                ASSERT(read < 0 || r == read);
                read = r;
            }
            assert(read >= 0);
            got += read;
	    }
        return got;
    }
    else {
        double inc = 1./ratio; // stepsize over input stream
        double pos = this->pos;

	    // hopefully all channel fifos advance uniformly.....
        int f = 0;
	    for(; f < frames; ++f) {
            pos += inc;
            int pint = (int)pos;

            int have = input[0].Have();
            if(have < pint) {
                Clear();  // clear interpolation buffer
                break; // buffer underrun
            }

            if(pint == 1) {
                Temp *t = temp;
                for(int c = 0; c < chns; ++c,++t) {
                    (*t)[0] = (*t)[1],(*t)[1] = (*t)[2],(*t)[2] = (*t)[3];
                    input[c].Read(1,*t+3);
                }
            }
            else if(pint == 2) {
                Temp *t = temp;
                for(int c = 0; c < chns; ++c,++t) {
                    (*t)[0] = (*t)[2],(*t)[1] = (*t)[3];
                    input[c].Read(2,*t+2);
                }
            }
            else if(pint == 0) {
                // nothing to do
            }
            else if(pint == 3) {
                Temp *t = temp;
                for(int c = 0; c < chns; ++c,++t) {
                    (*t)[0] = (*t)[3];
                    input[c].Read(3,*t+1);
                }
            }
            else if(pint == 4) {
                for(int c = 0; c < chns; ++c)
                    input[c].Read(4,temp[c]);
            }
            else { // pint > 4
                // simply consume buffer without copying data
                int cons = pint-4;
                for(int c = 0; c < chns; ++c) {
                    input[c].Read(cons);
                    input[c].Read(4,temp[c]);
                }
            }

            float frac = pos-pint;
		    const float f1 = 0.5f*(frac-1.0f);
		    const float f3 = frac*3.0f-1.0f;
            for(int c = 0; c < chns; ++c) {
                const Temp &t = temp[c];
                const float amdf = (t[0]-t[3])*frac;
		        const float cmb = t[2]-t[1];
		        const float bma = t[1]-t[0];
		        output[c][f] = t[1] + frac*( cmb - f1 * ( amdf+bma+cmb*f3 ) );
            }

            pos -= pint;
	    }
        this->pos = pos;

        return f;
    }
}
