/* Copyright (c) 2003 Tim Blechmann.                                            */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/*                                                                              */
/* tbfft2~ transforms the fft spectrum. it reverses the order of the samples in */
/* the fft spectrum. see the help file for further instruction...               */
/*                                                                              */
/*                                                                              */
/* tbfft2~ uses the flext C++ layer for Max/MSP and PD externals.               */
/* get it at http://www.parasitaere-kapazitaeten.de/PD/ext                      */
/* thanks to Thomas Grill                                                       */
/*                                                                              */
/*                                                                              */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* See file LICENSE for further informations on licensing terms.                */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* Based on PureData by Miller Puckette and others.                             */
/*                                                                              */
/*                                                                              */
/*                                                                              */
/* coded while listening to: Naked City: Heretic, Jeux Des Dames Cruelles       */
/*                           Bob Ostertag: Attention Span                       */
/*                                                                              */
/*                                                                              */



#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error upgrade your flext version!!!!!!
#endif


class tbfft2: public flext_dsp
{
    FLEXT_HEADER(tbfft2,flext_dsp);
    
public: // constructor
    tbfft2();
    ~tbfft2();

protected:
    virtual void m_signal (int n, float *const *in, float *const *out);
    virtual void m_dsp (int n, float *const *in, float *const *out);
    void set_freq(t_float);
    void set_width(t_float);
    
private:
    FLEXT_CALLBACK_1(set_freq,t_float);
    FLEXT_CALLBACK_1(set_width,t_float);
  
    t_int center;
    t_int width;
    
    t_float * tmp;
    
    t_int n0;
};


FLEXT_LIB_DSP("tbfft2~",tbfft2)

tbfft2::tbfft2()
{
  AddInSignal();
  AddOutSignal();
  FLEXT_ADDMETHOD_F(0,"center",set_freq);
  FLEXT_ADDMETHOD_F(0,"width",set_width);
} 

tbfft2::~tbfft2()
{
    free(tmp);
}

void tbfft2::m_dsp(int n, t_float *const *in, t_float *const *out)
{
    free(tmp);
    tmp=(t_float*)malloc(n*sizeof(t_float));
}



void tbfft2::m_signal(int n, t_float *const *in, t_float *const *out)
{
    t_float * ins = in[0];
    t_float * outs = out[0];

    CopySamples(tmp,ins,n);
    
    n0=n/2;  
    
    if (center-width>0)
    {
	n=center-width;
    }
    else
	n=0;
    
    while (n<center+width)
    {
	tmp[n]=*(ins+2*center-n);
	++n;
    }
    

    
    //memcpy
    CopySamples(outs,tmp,n0*2);
  
}

void tbfft2::set_freq(t_float freq)
{
    center=freq;
    set_width(width);
}

void tbfft2::set_width(t_float w)
{

  if (w+center>n0)
    {
      width=n0-center;
      return;
    }
  if (center-w<0)
    {
      width=center;
      return;
    }

  width=w;
}


