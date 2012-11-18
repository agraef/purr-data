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

//  latoocarfian model, mutation beta:
//           x1[n+1] = sin(x2[n]*b) + pow(sin(x1[n]*b),2)
//           x2[n+1] = sin(x1[n]*a) + pow(sin(x2[n]*a),2)
//  taken from Pickover: Chaos In Wonderland

class latoomutbeta
	: public map_base<2>
{
public:
	latoomutbeta()
	{
		CHAOS_SYS_INIT(x1,0.5,0);
		CHAOS_SYS_INIT(x2,0.5,1);
		
		CHAOS_PAR_INIT(a,-0.966918);
		CHAOS_PAR_INIT(b,2.879879);
	}
	
	void m_step()
	{
		data_t x1 = m_data[0], x2 = m_data[1];
		data_t a = CHAOS_PARAMETER(a), b = CHAOS_PARAMETER(b);
		data_t tmp;
		
		tmp = sin(x1*b);
		m_data[0] = sin(x2*b) + tmp*tmp;
		tmp = sin(x2*a);
		m_data[1] = sin(x1*a) + tmp*tmp;
	}

	/* function has a fix point for x1 == x2 == 0 */
	virtual void m_verify() 
	{
		if (m_data[0] == 0 && m_data[1] == 0)
			for (int i = 0; i != 2; ++i)
				m_data[i] = rand_range(0,0.1);
	}

	CHAOS_SYSVAR_FUNCS(x1, 0);
	CHAOS_SYSVAR_FUNCS(x2, 1);

	CHAOS_SYSPAR_FUNCS(a);
	CHAOS_SYSPAR_FUNCS(b);
};


#define LATOOMUTBETA_CALLBACKS					\
MAP_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(x1);						\
CHAOS_SYS_CALLBACKS(x2);						\
CHAOS_SYS_CALLBACKS(a);							\
CHAOS_SYS_CALLBACKS(b);


#define LATOOMUTBETA_ATTRIBUTES					\
MAP_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(x1);						\
CHAOS_SYS_ATTRIBUTE(x2);						\
CHAOS_SYS_ATTRIBUTE(a);							\
CHAOS_SYS_ATTRIBUTE(b);

