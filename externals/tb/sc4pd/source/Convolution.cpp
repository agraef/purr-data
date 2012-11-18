/* sc4pd 
   Convolution~

   Copyright (c) 2004 Tim Blechmann.

   This code is derived from:
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   Based on:
     PureData by Miller Puckette and others.
         http://www.crca.ucsd.edu/~msp/software.html
     FLEXT by Thomas Grill
         http://www.parasitaere-kapazitaeten.net/ext
     SuperCollider by James McCartney
         http://www.audiosynth.com
     
   Coded while listening to: Ambarchi/Muller/Voice Crack: Oystered 

*/

#include "sc4pd.hpp"
#include "fftlib.h"

/* ------------------------ Convolution~ -------------------------------*/

class Convolution_ar:
    public sc4pd_dsp
{
    FLEXT_HEADER(Convolution_ar,sc4pd_dsp);
    
public:
    Convolution_ar(int argc, t_atom *argv);
    ~Convolution_ar();
    
protected:
    virtual void m_signal(int n, t_sample *const *in, t_sample *const *out);
    virtual void m_dsp(int n, t_sample *const *in, t_sample *const *out);

private:
    int m_pos, m_insize, m_fftsize,m_mask;
    int m_log2n;
    
    float *m_inbuf1,*m_inbuf2, *m_fftbuf1, *m_fftbuf2, *m_outbuf,*m_overlapbuf;

};

FLEXT_LIB_DSP_V("Convolution~",Convolution_ar);

Convolution_ar::Convolution_ar(int argc, t_atom *argv)
{

    //parse arguments
    AtomList Args(argc,argv);

    m_insize = sc_getfloatarg(Args,0);

    AddInSignal("signal");
    AddInSignal("kernel");
    AddOutSignal();

	        
    //require size N+M-1 to be a power of two
    
    m_fftsize=2*(m_insize);

    //just use memory for the input buffers and fft buffers
    int insize = m_insize * sizeof(float);
    int fftsize = m_fftsize * sizeof(float);
        
    m_inbuf1 = new float[m_insize];
    m_inbuf2 = new float[m_insize];
    
    m_fftbuf1 = new float[m_fftsize];
    m_fftbuf2 = new float[m_fftsize];
    
    m_outbuf = new float[m_fftsize];
    m_overlapbuf = new float[m_insize];
    
    memset(m_outbuf, 0, fftsize);
    memset(m_overlapbuf, 0, insize);
    
    m_log2n = LOG2CEIL(m_fftsize);
    
    //test for full input buffer
    m_mask = m_insize;
    m_pos = 0;
}    

Convolution_ar::~Convolution_ar()
{
    delete m_inbuf1;
    delete m_inbuf2;
    
    delete m_fftbuf1;
    delete m_fftbuf2;
    
    delete m_outbuf;
    delete m_overlapbuf;
}

void Convolution_ar::m_dsp(int n, t_sample *const *in, 
			 t_sample *const *out)
{

}

extern float* cosTable[32];

void Convolution_ar::m_signal(int n, t_sample *const *in, 
			    t_sample *const *out)
{
    float *in1 = in[0];
    float *in2 = in[1];
    
    float *out1 = m_inbuf1 + m_pos;
    float *out2 = m_inbuf2 + m_pos;
	
    int numSamples = 2*n; //??? mWorld->mFullRate.mBufLength;
    
    // copy input
    CopySamples(out1, in1, numSamples);
    CopySamples(out2, in2, numSamples);
	
    m_pos += numSamples;

    if (m_pos & m_insize) 
    {
        
        //have collected enough samples to transform next frame
        m_pos = 0; //reset collection counter
		
        // copy to fftbuf

        uint32 insize=m_insize * sizeof(float);
        memcpy(m_fftbuf1, m_inbuf1, insize);
        memcpy(m_fftbuf2, m_inbuf2, insize);
	
        //zero pad second part of buffer to allow for convolution
        memset(m_fftbuf1+m_insize, 0, insize);
        memset(m_fftbuf2+m_insize, 0, insize);
                   
        int log2n = m_log2n;                     	
        
    
        // do windowing
        DoWindowing(log2n, m_fftbuf1, m_fftsize);
        DoWindowing(log2n, m_fftbuf2, m_fftsize);
		
		// do fft
/*		#if __VEC__
		ctoz(m_fftbuf1, 2, outbuf1, 1, 1L<<log2n); ctoz(m_fftbuf2, 2, outbuf2, 1, 1L<<log2n);
		#else      */

//in place transform for now
	rffts(m_fftbuf1, log2n, 1, cosTable[log2n]);
	rffts(m_fftbuf2, log2n, 1, cosTable[log2n]);
//#endif

//complex multiply time
	int numbins = m_fftsize >> 1; //m_fftsize - 2 >> 1;
  
        float * p1= m_fftbuf1;
        float * p2= m_fftbuf2;
        
        p1[0] *= p2[0];
        p1[1] *= p2[1];
    
        //complex multiply
        for (int i=1; i<numbins; ++i) {
            float real,imag;
            int realind,imagind;
            realind= 2*i; imagind= realind+1;
            real= p1[realind]*p2[realind]- p1[imagind]*p2[imagind];
            imag= p1[realind]*p2[imagind]+ p1[imagind]*p2[realind];
                p1[realind] = real; //p2->bin[i];
                p1[imagind]= imag;
	}
        
        //copy second part from before to overlap                 
        memcpy(m_overlapbuf, m_outbuf+m_insize, m_insize * sizeof(float));	
     
        //inverse fft into outbuf        
        memcpy(m_outbuf, m_fftbuf1, m_fftsize * sizeof(float));

         //in place
        riffts(m_outbuf, log2n, 1, cosTable[log2n]);	
        
        DoWindowing(log2n, m_outbuf, m_fftsize);
    }

    //write out samples copied from outbuf, with overlap added in 
	 
    float *output = out[0];
    float *nout= m_outbuf+m_pos; 
    float *overlap= m_overlapbuf+m_pos; 
    
    for (int i=0; i<numSamples; ++i) 
    {
	*++output = *++nout + *++overlap;
    }
    
}




