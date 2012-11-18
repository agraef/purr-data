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

//  logistic map: x[n+1] = alpha * x[n] * (1 - x[n])
//                0 < x[n] <  1
//                0 <= alpha <= 4

class logistic:
	public map_base<1>
{
public:
	logistic()
	{
		CHAOS_PAR_INIT(alpha, 3.8);
		CHAOS_SYS_INIT(x, 0.5,0);
	}

	~logistic()
	{
	}

	void m_step()
	{
		data_t x = m_data[0];
		data_t alpha = CHAOS_PARAMETER(alpha);
		m_data[0] = alpha * x * (1.f - x);
	}

	CHAOS_SYSPAR_FUNCS_PRED(alpha, m_pred_alpha);
	bool m_pred_alpha(data_t f)
	{
		return (f > 0) && (f < 4);
	}

	CHAOS_SYSVAR_FUNCS_PRED(x, 0, m_pred_x);
	
	bool m_pred_x(data_t f)
	{
		return (f > 0) && (f < 1);
	}

	virtual void m_verify()
	{
		data_t x = m_data[0];
		if (m_pred_x(x))
			return;
		m_data[0] = 0.5;
	}
};

#define LOGISTIC_CALLBACKS						\
MAP_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(alpha);						\
CHAOS_SYS_CALLBACKS(x);

#define LOGISTIC_ATTRIBUTES						\
MAP_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(alpha);						\
CHAOS_SYS_ATTRIBUTE(x);

