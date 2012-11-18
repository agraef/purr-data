/* Copyright (c) 2003 Tim Blechmann.                                            */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/*                                                                              */
/* tbroute is an advanced router.                                               */
/* the signal to the first inlet is being routed to the outlet specified        */
/* by the second inlet.                                                         */
/* the number of outlets is specified by the creation argument                  */
/*                                                                              */
/*                                                                              */
/* tbroute uses the flext C++ layer for Max/MSP and PD externals.               */
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
/* coded while listening to: Hamid Drake & Assif Tsahar: Soul Bodies, Vol. 1    */
/*                           I.S.O.: I.S.O                                      */
/*                                                                              */
/*                                                                              */

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error upgrade your flext version!!!!!!
#endif

class tbroute: public flext_base
{
  FLEXT_HEADER(tbroute,flext_base);
  
public: // constructor
  tbroute(int chan);

protected:
  void route(t_float f);

  void routebang();
  void set_route(int i);

private:
  FLEXT_CALLBACK_1(route,t_float);
  
  FLEXT_CALLBACK(routebang);
  FLEXT_CALLBACK_1(set_route,int);
  
  int dest;
};


FLEXT_LIB_1("tbroute",tbroute,int);

tbroute::tbroute(int chan)
{
  AddInAnything();
  AddInInt();
  
  for (int i=0; i!=chan;++i)
    {    
      AddOutAnything();
    }
  
  FLEXT_ADDMETHOD(0,route);
  FLEXT_ADDBANG(0,routebang);
  FLEXT_ADDMETHOD(1,set_route);
  dest=0;
}

void tbroute::route(t_float f)
{
  ToOutFloat(dest,f);
}


void tbroute::routebang()
{
  ToOutBang(dest);
}

void tbroute::set_route(int i)
{
  --i;
  if ((i>-1) && (i<CntOut()))
    {  
      dest=i;
    }
  else
    {
      post("no such outlet");
    }
}
