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

//  duffing map: x1[n+1] = x2[n]
//               x2[n+1] = -b*x1[n] + a*x2[n] - pow(x2,3)
//  
//  taken from Viktor Avrutin: AnT-Demos-4.669

class duffing_map:
	public map_base<2>
{
public:
	duffing_map()
	{
		CHAOS_SYS_INIT(x1, 0.5, 0);
		CHAOS_SYS_INIT(x2, 0.5, 1);
		CHAOS_PAR_INIT(a, 0.5);
		CHAOS_PAR_INIT(b, 0.5);
	}



	void m_step()
	{
		data_t x1 = m_data[0], x2 = m_data[1];

		m_data[0] = x2;
		m_data[1] = - CHAOS_PARAMETER(b)*x1 + CHAOS_PARAMETER(a)*x2
			- x2*x2*x2;
	}

	CHAOS_SYSVAR_FUNCS(x1,0);
	CHAOS_SYSVAR_FUNCS(x2,1);

	CHAOS_SYSPAR_FUNCS(a);
	CHAOS_SYSPAR_FUNCS(b);
};

#define DUFFING_MAP_CALLBACKS					\
MAP_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(x1);						\
CHAOS_SYS_CALLBACKS(x2);						\
CHAOS_SYS_CALLBACKS(a);							\
CHAOS_SYS_CALLBACKS(b);

#define DUFFING_MAP_ATTRIBUTES					\
MAP_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(x1);						\
CHAOS_SYS_ATTRIBUTE(x2);						\
CHAOS_SYS_ATTRIBUTE(a);							\
CHAOS_SYS_ATTRIBUTE(b);

