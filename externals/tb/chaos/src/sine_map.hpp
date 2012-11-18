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
#include <cmath>

//  sine map: x[n+1] = sin(pi * x)
//                     0 <= x[n] <=  1
//  taken from Willi-Hans Steeb: Chaos and Fractals

class sine_map:
	public map_base<1>
{
	
public:
	sine_map()
	{
		CHAOS_SYS_INIT(x,0,0);
	}

	~sine_map()
	{
		
	}

	void m_step()
	{
		m_data[0] = sin (M_PI * m_data[0]);
	}

	CHAOS_SYSVAR_FUNCS_PRED(x, 0, m_pred);

	bool m_pred(t_float f)
	{
		return (f >= 0) && (f <= 1);
	}
};

#define SINE_MAP_CALLBACKS						\
MAP_CALLBACKS									\
CHAOS_SYS_CALLBACKS(x);

#define SINE_MAP_ATTRIBUTES						\
MAP_ATTRIBUTES									\
CHAOS_SYS_ATTRIBUTE(x);


