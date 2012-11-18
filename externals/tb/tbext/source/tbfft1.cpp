/* Copyright (c) 2003 Tim Blechmann.                                            */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/*                                                                              */
/* tbfft1~ transforms the fft spectrum                                          */
/*                                                                              */
/* tbfft1~ uses the flext C++ layer for Max/MSP and PD externals.               */
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
/* coded while listening to: John Zorn / Mike Patton / Ikue Mori: Hemophiliac   */
/*                           Rashied Ali / Frank Lowe: Duo Exchange             */
/*                           Keith Rowe / John Tilbury: Duos For Doris          */
/*                                                                              */



#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error upgrade your flext version!!!!!!
#endif


class tbfft1: public flext_dsp
{
  FLEXT_HEADER(tbfft1,flext_dsp);

public: // constructor
  tbfft1();

protected:
  virtual void m_signal (int n, float *const *in, float *const *out);
  void set_freq(t_float);
  void set_fact(t_float);

private:
  FLEXT_CALLBACK_1(set_freq,t_float)
  FLEXT_CALLBACK_1(set_fact,t_float)
  
  t_int center;
  t_float factor;
  
  t_float pos;
  t_int posi;

  float *ins;
  float *outs;

  t_float s;
  t_float b;


};


FLEXT_LIB_DSP("tbfft1~",tbfft1)

tbfft1::tbfft1()
{
  AddInSignal();
  AddOutSignal();
  FLEXT_ADDMETHOD_F(0,"center",set_freq);
  FLEXT_ADDMETHOD_F(0,"factor",set_fact);
} 


void tbfft1::m_signal(int n, t_float *const *in, t_float *const *out)
{
  ins = in[0];
  outs = out[0];

  n=n/2+1;
  while(n!=0)
    {
      pos= n + factor * (center-n);
      posi=t_int(pos);
      

      if (n < t_int(center))
	{
	  *(outs+n) = ((pos-t_float(posi)) * (*(ins+posi)))/2 
	    +((1-pos+t_float(posi)) * (*(ins+posi+1)))/2;
	}
      else
	{
	  *(outs+n) = ((pos-t_float(posi)) * (*(ins+posi-1)))/2 
	    +((1-pos+t_float(posi)) * (*(ins+posi)))/2;
	}
      
      --n;
    }
  
  
}

void tbfft1::set_freq(t_float freq)
{
  s=Samplerate(); 
  
  b=Blocksize();     
  
  center=freq/(s/b);
}


void tbfft1::set_fact(t_float f)
{
  if (f<2)
    factor=f;
}
