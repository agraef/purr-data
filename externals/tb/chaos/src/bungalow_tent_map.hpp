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

//  bungalow_tent map: x[n+1] = 1 + 2 * r + 2 * (r + 1) * x[n] 
//                                                (for -1 <= x[n] < -0.5f)
//                              1 + 2 * (1 - r) * (x[n])
//                                                (for -0.5 <= x[n] < 0)
//                              1 + 2 * (r - 1) * (x[n])
//                                                (for 0 <= x[n] < 0.5)
//                              1 + 2 * r - 2 * (r + 1) * (x[n])
//                                                (for 0.5 <= x[n] < 1)
//                              -1 <= x[n] <  1
//                              -0.5 <= r < 1
//  taken from Willi-Hans Steeb: Chaos and Fractals

class bungalow_tent:
	public map_base<1>
{
public:
	bungalow_tent()
	{
		CHAOS_SYS_INIT(x, 0.6,0);
		CHAOS_PAR_INIT(r, 0.5);
	}

	void m_step()
	{
		data_t x = m_data[0];
		data_t r = CHAOS_PARAMETER(r);
		
		if ( x < - 0.5)
		{
			m_data[0] = 1 + 2 * r + 2 * (r + 1) * x;
			return;
		}
		if ( x < 0)
		{
			m_data[0] = 1 + 2 * (1 - r) * x;
			return;
		}
		if ( x < 0.5)
		{
			m_data[0] = 1 + 2 * (r - 1) * x;
			return;
		}
		else
		{
			m_data[0] = 1 + 2 * r - 2 * (r + 1) * x;
			return;
		}

	}

	CHAOS_SYSVAR_FUNCS_PRED(x, 0, m_pred_x);
	bool m_pred_x(t_float f)
	{
		return (f >= -1) && (f < 1);
	}

	CHAOS_SYSPAR_FUNCS_PRED(r, m_pred_r);
	bool m_pred_r(t_float f)
	{
		return (f >= -0.5) && (f < 1);
	}
};

#define BUNGALOW_TENT_CALLBACKS					\
MAP_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(x);							\
CHAOS_SYS_CALLBACKS(r);

#define BUNGALOW_TENT_ATTRIBUTES				\
MAP_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(x)							\
CHAOS_SYS_ATTRIBUTE(r)
