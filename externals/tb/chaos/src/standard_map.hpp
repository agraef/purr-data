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

//  standard map: I[n+1] = I[n] + k * sin(theta[n])
//                theta[n+1] = theta[n] + I[n] + k * sin(theta[n])
//                0 <= theta <= 2*pi
//  taken from Willi-Hans Steeb: Chaos and Fractals

class standard_map:
	public map_base<2>
{
public:
	standard_map()
	{
		CHAOS_SYS_INIT(I,0.1,0);
		CHAOS_SYS_INIT(theta,0.2,1);

		CHAOS_PAR_INIT(k, 0.8);
	}

	~standard_map()
	{
		
	}

	void m_step()
	{
		data_t I = m_data[0];
		data_t theta = m_data[1];
		data_t k = CHAOS_PARAMETER(k);
		
		m_data[0] = I + k * sin(theta);
		theta = theta + I + k * sin(theta);

		if (theta > 2 * M_PI)
		{
			do
			{
				theta -= 2*M_PI;
			}
			while (theta > 2 * M_PI);
			goto put_data;
		}
		
		if (theta < 0)
		{
			do
			{
				theta += 2*M_PI;
			}
			while (theta < 0);
		}
		
	put_data:
		m_data[1] = theta;
	}


	CHAOS_SYSVAR_FUNCS(I, 0);

	CHAOS_SYSVAR_FUNCS_PRED(theta, 1, m_pred_theta);
	bool m_pred_theta(t_float f)
	{
		return (f >= 0) && (f < 2*M_PI);
	}

	CHAOS_SYSPAR_FUNCS(k);
};


#define STANDARD_MAP_CALLBACKS					\
MAP_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(I);							\
CHAOS_SYS_CALLBACKS(theta);						\
CHAOS_SYS_CALLBACKS(k);


#define STANDARD_MAP_ATTRIBUTES					\
MAP_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(I);							\
CHAOS_SYS_ATTRIBUTE(theta);						\
CHAOS_SYS_ATTRIBUTE(k);
