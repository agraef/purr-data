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

//  coupled_logistic map: x[n+1] = r * x[n] * (1 - x[n]) + e * (y[n] - x[n])
//                        y[n+1] = r * y[n] * (1 - y[n]) + e * (x[n] - y[n])
//                        0 <= r <= 4
//  taken from Willi-Hans Steeb: Chaos and Fractals

class coupled_logistic:
	public map_base<2>
{
public:
	coupled_logistic()
	{
		CHAOS_PAR_INIT(e, 0.06);
		CHAOS_PAR_INIT(r, 3.7);

		CHAOS_SYS_INIT(x, 0.1,0);
		CHAOS_SYS_INIT(y, 0.2,1);
	}

	virtual inline void m_step()
	{
		data_t x = m_data[0];
		data_t y = m_data[1];
		data_t e = CHAOS_PARAMETER(e);
		data_t r = CHAOS_PARAMETER(r);
		m_data[0] = r * x * (1.f - x) + e * (y - x);
		m_data[1] = r * y * (1.f - y) + e * (x - y);
		m_verify();
	}

	CHAOS_SYSPAR_FUNCS(e);
	CHAOS_SYSPAR_FUNCS_PRED(r, m_pred_r);
	static bool m_pred_r(t_float f)
	{
		return (f > 0) && (f < 4);
	}

	CHAOS_SYSVAR_FUNCS_PRED(x, 0, m_pred_xy);
	CHAOS_SYSVAR_FUNCS_PRED(y, 0, m_pred_xy);
	
	static bool m_pred_xy(t_float f)
	{
		return (f > 0) && (f < 1);
	}

	inline void m_verify()
	{
		data_t x = m_data[0];
		data_t y = m_data[1];
		if (!m_pred_xy(x))
			m_data[0] = rand_range(0,0.08);
		if (!m_pred_xy(y))
			m_data[1] = rand_range(0,0.08);
		if (x == y)
		{
			m_data[0] += rand_range(0,0.2);
			m_data[1] += -rand_range(0,0.2);
		}
	}
};

#define COUPLED_LOGISTIC_CALLBACKS				\
MAP_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(e);							\
CHAOS_SYS_CALLBACKS(r);							\
CHAOS_SYS_CALLBACKS(x);							\
CHAOS_SYS_CALLBACKS(y);

#define COUPLED_LOGISTIC_ATTRIBUTES				\
MAP_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(e);							\
CHAOS_SYS_ATTRIBUTE(r);							\
CHAOS_SYS_ATTRIBUTE(x);							\
CHAOS_SYS_ATTRIBUTE(y);

