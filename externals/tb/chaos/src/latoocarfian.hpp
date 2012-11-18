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

//  latoocarfian model: x1[n+1] = sin(x2[n]*b) + c*sin(x1[n]*b)
//                      x2[n+1] = sin(x1[n]*a) + d*sin(x2[n]*a)
//                      -3 < a,b < 3
//                      -0.5 < c,d < 1.5
//  taken from Pickover: Chaos In Wonderland

class latoocarfian
	: public map_base<2>
{
public:
	latoocarfian()
	{
		CHAOS_SYS_INIT(x1,0.5,0);
		CHAOS_SYS_INIT(x2,0,1);

		CHAOS_PAR_INIT(a,-0.966918);
		CHAOS_PAR_INIT(b,2.879879);
		CHAOS_PAR_INIT(c,0.765145);
		CHAOS_PAR_INIT(d,0.744728);
	}

	void m_step()
	{
		data_t x1 = m_data[0], x2 = m_data[1];
		data_t a = CHAOS_PARAMETER(a), b = CHAOS_PARAMETER(b),
			c = CHAOS_PARAMETER(c), d = CHAOS_PARAMETER(d);
		
		m_data[0] = sin(x2*b) + c*sin(x1*b);
		m_data[1] = sin(x1*a) + d*sin(x2*a);
	}

	CHAOS_SYSVAR_FUNCS(x1, 0);
	CHAOS_SYSVAR_FUNCS(x2, 1);

	CHAOS_SYSPAR_FUNCS_PRED(a,m_pred_1);
	CHAOS_SYSPAR_FUNCS_PRED(b,m_pred_1);
	CHAOS_SYSPAR_FUNCS_PRED(c,m_pred_2);
	CHAOS_SYSPAR_FUNCS_PRED(d,m_pred_2);

	bool m_pred_1(t_float f)
	{
		return (f > -3.f) && (f < 3.f);
	}

	bool m_pred_2(t_float f)
	{
		return (f > -0.5) && (f < 1.5);
	}
	
	/* function has a fix point for x1 == x2 == 0 */
	virtual void m_verify() 
	{
		if (m_data[0] == 0 && m_data[1] == 0)
			for (int i = 0; i != 2; ++i)
				m_data[i] = rand_range(0,0.1);
	}
};


#define LATOOCARFIAN_CALLBACKS					\
MAP_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(x1);						\
CHAOS_SYS_CALLBACKS(x2);						\
CHAOS_SYS_CALLBACKS(a);							\
CHAOS_SYS_CALLBACKS(b);							\
CHAOS_SYS_CALLBACKS(c);							\
CHAOS_SYS_CALLBACKS(d);

#define LATOOCARFIAN_ATTRIBUTES					\
MAP_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(x1);						\
CHAOS_SYS_ATTRIBUTE(x2);						\
CHAOS_SYS_ATTRIBUTE(a);							\
CHAOS_SYS_ATTRIBUTE(b);							\
CHAOS_SYS_ATTRIBUTE(c);							\
CHAOS_SYS_ATTRIBUTE(d);
