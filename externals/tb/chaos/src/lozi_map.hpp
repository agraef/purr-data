// 
//  
//  chaos~
//  Copyright (C) 2004  Tim Blechmann
//  
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#include "map_base.hpp"

//  lozi map: x[n+1] = y[n] + 1 - a * abs(x[n])
//            y[n+1] = b * x[n]
//            b != 0
//  taken from Willi-Hans Steeb: Chaos and Fractals

class lozi_map:
	public map_base<2>
{
public:
	lozi_map()
	{
		CHAOS_SYS_INIT(x,0,0);
		CHAOS_SYS_INIT(y,0,1);

		CHAOS_PAR_INIT(a,1.4);
		CHAOS_PAR_INIT(b,0.3);
	}

	void m_step()
	{
		data_t x = m_data[0];
		data_t y = m_data[1];
		
		if (x > 0)
			m_data[0] = 1 + y - CHAOS_PARAMETER(a) * x;
		else
			m_data[0] = 1 + y + CHAOS_PARAMETER(a) * x;
			
		m_data[1] = CHAOS_PARAMETER(b) * x;
		
	}
							
	CHAOS_SYSVAR_FUNCS(x, 0);
	CHAOS_SYSVAR_FUNCS(y, 1);

	CHAOS_SYSPAR_FUNCS(a);

	CHAOS_SYSPAR_FUNCS_PRED(b, m_pred_b);
	bool m_pred_b(t_float f)
	{
		return (f != 0);
	}
};


#define LOZI_MAP_CALLBACKS						\
MAP_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(a);							\
CHAOS_SYS_CALLBACKS(b);							\
CHAOS_SYS_CALLBACKS(x);							\
CHAOS_SYS_CALLBACKS(y);

#define LOZI_MAP_ATTRIBUTES						\
MAP_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(a);							\
CHAOS_SYS_ATTRIBUTE(b);							\
CHAOS_SYS_ATTRIBUTE(x);							\
CHAOS_SYS_ATTRIBUTE(y);
