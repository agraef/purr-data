/* Copyright (c) 2003 Tim Blechmann.                                            */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/*                                                                              */
/* tbssel~ selects one signal from the incoming signals                         */
/* the number of inlets is specified by the creation argument                   */
/*                                                                              */
/*                                                                              */
/* tbssel~ uses the flext C++ layer for Max/MSP and PD externals.               */
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

class tbssel: public flext_dsp
{
  FLEXT_HEADER(tbssel,flext_dsp)

public: // constructor
  tbssel(t_int chan);

protected:
  virtual void m_signal (int n, float *const *in, float *const *out);
  void set_source(int i);

private:
  FLEXT_CALLBACK_1(set_source,int)
  t_int source;
};


FLEXT_LIB_DSP_1("tbroute~",tbssel,int);

tbssel::tbssel (t_int chan):
  source(0)
{
  for (t_int i=0; i!=chan;++i)
    {    
      AddInSignal();
    }
  
  AddOutSignal();
  
  FLEXT_ADDMETHOD(1,set_source);
} 


void tbssel::m_signal(int n, float *const *in, float *const *out)
{

  CopySamples(out[0],in[source],n);
  

}

void tbssel::set_source(int i)
{
  if ((i>-1) && (i<CntInSig()))
    {  
      source=i;
      post("selecting inlet %i",i);
    }
  else
    {
      post("no such inlet");
    }
  
}
