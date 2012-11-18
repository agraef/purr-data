/* Copyright (c) 2003 Tim Blechmann.                                            */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/*                                                                              */
/* fftgrshuf divides the incoming fft signal into single grains and reverses    */
/* the samples in every grain                                                   */
/*                                                                              */
/* fftgrrev uses the flext C++ layer for Max/MSP and PD externals.              */
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
/* coded while listening to: Howard Skempton/John Tilbury: Well, well, Cornelius*/
/*                                                                              */
/*                                                                              */



#include <flext.h>
#include <algorithm>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error upgrade your flext version!!!!!!
#endif

class fftgrrev: public flext_dsp
{
  FLEXT_HEADER(fftgrrev,flext_dsp);

public: // constructor
  fftgrrev(int);

protected:
  virtual void m_signal (int n, float *const *in, float *const *out);
  void set_grains(t_int);
  void set_offset(t_int);
  void set_reverse();
  
private:
  FLEXT_CALLBACK_1(set_grains,t_int)
  FLEXT_CALLBACK_1(set_offset,t_int)
    
  t_int grains;
  t_int grainsize;
  t_int offset;
  
  t_int bs; //blocksize
  t_int bs1; //bs+1

  t_sample * data; //array with data
  t_sample * d1; //1. element in array with data
  t_sample * dend; //1 element after the last element
  

  bool reverse;
  
};


FLEXT_LIB_DSP_1("fftgrrev~",fftgrrev,int)

fftgrrev::fftgrrev(int arg):
  grains(1),offset(0)
{
  bs=arg/2;
  grainsize=bs;
  bs1=bs+1;
  post("blocksize: %i",bs);
  
  data = new t_sample[bs+1];
  
  data[0]=0;
  d1=data+1;
  dend=data+bs+1;
  
  AddInSignal();
  AddOutSignal();
  FLEXT_ADDMETHOD_I(0,"grains",set_grains);
  FLEXT_ADDMETHOD_I(0,"offset",set_offset);
} 

void fftgrrev::m_signal(int n, t_float * const *in, t_float *const *out)
{
  t_sample * ins = in[0];
  t_sample * outs = out[0];

  if (offset>0)
    {
      CopySamples(d1+bs-offset,ins,offset);
      CopySamples(d1,ins+offset,bs-offset);
    }
  else if (offset<0)
    {
      CopySamples(d1-offset,ins,bs+offset);
      CopySamples(d1,ins+bs+offset,-offset);
    } 
  else 
    CopySamples(data,ins,bs1);
  
  
  //grains
  
  int counter=1;
  
  while (counter!=grains)
    {
      std::reverse(d1+grainsize*(counter-1),d1+grainsize*counter);
      ++counter;
    }
  
  std::reverse(d1+grainsize*(counter-1),dend);
  
  CopySamples(outs,data,bs1);
}

void fftgrrev::set_offset(t_int o)
{
  if (o-bs<0 && o+bs>0)
    {
      offset=-o;
    }
  else
    post("Offset out of range!");
}


void fftgrrev::set_grains(t_int g)
{
  if (  (g > 0) )
    {
      grains=g;
      grainsize=(bs)/grains;
    }
}




