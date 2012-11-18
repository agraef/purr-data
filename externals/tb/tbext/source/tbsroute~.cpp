/* Copyright (c) 2003 Tim Blechmann.                                            */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/*                                                                              */
/* tbsroute~ is an advanced signal router.                                      */
/* the signal to the first inlet is being routed to the outlet specified        */
/* by the second inlet.                                                         */
/* the number of outlets is specified by the creation argument                  */
/*                                                                              */
/*                                                                              */
/* tbsroute~ uses the flext C++ layer for Max/MSP and PD externals.             */
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
/* coded while listening to: AMM: AMMMUSIC 1966                                 */
/*                           Sun Ra: Dancing Shadows                            */
/*                                                                              */


#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 401)
#error upgrade your flext version!!!!!!
#endif

class tbsroute: public flext_dsp
{
  FLEXT_HEADER(tbsroute,flext_dsp)

public: // constructor
  tbsroute(t_int chan);

protected:
  virtual void m_signal (int n, float *const *in, float *const *out);
  void set_route(int i);

private:
  FLEXT_CALLBACK_1(set_route,int)
  t_int dest;
};


FLEXT_LIB_DSP_1("tbroute~",tbsroute,int);

tbsroute::tbsroute (t_int chan):
  dest(0)
{
  AddInSignal();
  AddInInt();
  
  for (t_int i=0; i!=chan;++i)
    {    
      AddOutSignal();
    }
  
  FLEXT_ADDMETHOD(1,set_route);
} 


void tbsroute::m_signal(int n, float *const *in, float *const *out)
{

  CopySamples(out[dest],in[0],n);

  for (int i = 0; i != CntOutSig(); i++)
    if (i!=dest) ZeroSamples(out[i],n);
  
  


}

void tbsroute::set_route(int i)
{
  --i;
  if ((i>-1) && (i<CntOutSig()))
    {  
      dest=i;
      post("routing to outlet %i",i+1);
    }
  else
    {
	post("no such outlet");
    }
  
}
