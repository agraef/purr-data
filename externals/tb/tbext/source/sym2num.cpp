/* Copyright (c) 2004 Tim Blechmann.                                            */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/* sym2num interpretes a symbol as decimal number that is related to the ascii  */
/* representation.                                                              */
/*                                                                              */
/*                                                                              */
/* sym2num uses the flext C++ layer for Max/MSP and PD externals.               */
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
/* coded while listening to: Phil Minton & Veryan Weston: Ways                  */
/*                                                                              */
/*                                                                              */
/*                                                                              */



#include <flext.h>

#include <cstring>
#include <cmath>


#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error upgrade your flext version!!!!!!
#endif

class sym2num: public flext_base
{
  FLEXT_HEADER(sym2num,flext_base);

public:
    sym2num();
  
protected:
    void m_symbol(t_symbol *s);

private:

    FLEXT_CALLBACK_S(m_symbol);
};

FLEXT_LIB("sym2num",sym2num);

sym2num::sym2num()
{
  AddInSymbol();
  
  FLEXT_ADDMETHOD(0,m_symbol);

  AddOutFloat();
}

void sym2num::m_symbol(t_symbol * s)
{
    const char* str = GetString(s);
    
    int length = strlen(str);
    
    int ret(0);
    while (length--)
    {
	ret+=str[length]*pow(2,length);
    }
    ToOutFloat(0,ret);
}
