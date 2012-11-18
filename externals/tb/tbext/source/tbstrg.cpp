/* Copyright (c) 2003 Tim Blechmann.                                            */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/* tbstrg can be used to switch between several modules. it requires a creation */
/* argument (number of outlets).                                                */
/* if you send an integer to the inlet this outlet will get the message 1, the  */
/* outlet, that was active earlier, will get the message 0.                     */
/*                                                                              */
/*                                                                              */
/* tbstrg uses the flext C++ layer for Max/MSP and PD externals.                */
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
/* coded while listening to: Bob Ostertag - DJ Of The Month                     */
/*                           John Zorn's Cobra: Tokyo Operations '94            */
/*                                                                              */
/*                                                                              */



#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error upgrade your flext version!!!!!!
#endif

class tbstrg: public flext_base
{
  FLEXT_HEADER(tbstrg,flext_base);

public: // constructor
  tbstrg(t_int chan);
  
protected:
  void set_route(t_int i);

private:

  FLEXT_CALLBACK(set_route,t_int);
  t_int dest;
};


FLEXT_LIB_1("tbstrg",tbstrg,int);

tbstrg::tbstrg(t_int chan)
{
  AddInInt();
  
  for (t_int i=0; i!=chan;++i)
    {    
      AddOutAnything();
    }
  

  FLEXT_ADDMETHOD(0,set_route);
  dest=0;
}

void tbstrg::set_route(t_int i)
{
  if (i != dest)
    {
      --i;
      if ((i>-1) && (i<CntOut()))
	{  
	  ToOutInt(dest,0);
	  dest=i;
	  ToOutInt(dest,1);
	}
    }
}
