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

//  ikeda laser map: z[n+1] = roh + c2 * z[n] * 
//                            exp (j * (c1 - c3 / (1 + abs(z) * abs(z))))
//                   z is complex
//
//            equal: x[n+1] = roh + c2 * (x[n] * cos(tau) - y[n] * sin (tau))
//                   y[n+1] = c2 * (x[n] * sin(tau) + y[n] * cos(tau))
//                   tau = c1 - (c2 / (1 + x*x + y*y))
//
//  taken from Willi-Hans Steeb: Chaos and Fractals

class ikeda_laser_map:
	public map_base<1>
{
public:
	ikeda_laser_map()
	{
		CHAOS_PAR_INIT(c1,0.4);
		CHAOS_PAR_INIT(c2,0.9);
		CHAOS_PAR_INIT(c3,9);
		CHAOS_PAR_INIT(roh,0.85);

		CHAOS_SYS_INIT(x,0.5,0);
		CHAOS_SYS_INIT(y,0.5,1);
	}

	void m_step()
	{
		data_t x = m_data[0];
		data_t y = m_data[1];
		
		data_t tau = CHAOS_PARAMETER(c1) - 
			CHAOS_PARAMETER(c3) / (1 + x*x + y*y);
		data_t cos_tau = cos(tau);
		data_t sin_tau = sin(tau);

		m_data[0] = CHAOS_PARAMETER(roh) + CHAOS_PARAMETER(c2) 
			* (x * cos_tau - y * sin_tau);
		m_data[1] = CHAOS_PARAMETER(c2) * (x * sin_tau + y * cos_tau);
	}
	
	CHAOS_SYSVAR_FUNCS(x, 0);
	CHAOS_SYSVAR_FUNCS(y, 1);
	
	CHAOS_SYSPAR_FUNCS(c1);
	CHAOS_SYSPAR_FUNCS(c2);
	CHAOS_SYSPAR_FUNCS(c3);
	CHAOS_SYSPAR_FUNCS(roh);

};


#define IKEDA_LASER_MAP_CALLBACKS				\
MAP_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(c1);						\
CHAOS_SYS_CALLBACKS(c2);						\
CHAOS_SYS_CALLBACKS(c3);						\
CHAOS_SYS_CALLBACKS(roh);						\
CHAOS_SYS_CALLBACKS(x);							\
CHAOS_SYS_CALLBACKS(y);

#define IKEDA_LASER_MAP_ATTRIBUTES				\
MAP_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(c1);						\
CHAOS_SYS_ATTRIBUTE(c2);						\
CHAOS_SYS_ATTRIBUTE(c3);						\
CHAOS_SYS_ATTRIBUTE(roh);						\
CHAOS_SYS_ATTRIBUTE(x);							\
CHAOS_SYS_ATTRIBUTE(y);
