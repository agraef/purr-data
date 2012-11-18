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

#include "ode_base.hpp"

//  lorenz model: dx1/dt = sigma * (x2 - x1)
//                dx2/dt = - x1 * x3 + r * x1 - x2
//                dx3/dt = x1 * x2 - b * x3
//  taken from Willi-Hans Steeb: Chaos and Fractals

class lorenz
	: public ode_base<3>
{
public:
	lorenz()
	{
		CHAOS_PAR_INIT(method,0);
		CHAOS_PAR_INIT(dt,0.01);

		CHAOS_SYS_INIT(x1,0.8,0);
		CHAOS_SYS_INIT(x2,0.7,1);
		CHAOS_SYS_INIT(x3,0.6,2);

		CHAOS_PAR_INIT(sigma,16);
		CHAOS_PAR_INIT(b,4);
		CHAOS_PAR_INIT(r,40);
	}
	
	~lorenz()
	{
	}

	virtual void m_system(data_t* deriv, data_t* data)
	{
		data_t x1 = data[0], x2 = data[1], x3 = data[2];
		
		deriv[0] = CHAOS_PARAMETER(sigma) * (x2 - x1);
		deriv[1] = - x1 * x3 + CHAOS_PARAMETER(r) * x1 - x2;
		deriv[2] = x1 * x2 - CHAOS_PARAMETER(b) * x3;
	}
	

	/* function has a fix point for x1 == x2 == x3 == 0 */
	virtual void m_verify() 
	{
		if (m_data[0] == 0 && m_data[1] == 0 && m_data[2] == 0)
			for (int i = 0; i != 3; ++i)
				m_data[i] = rand_range(0,3);
	}


	CHAOS_SYSVAR_FUNCS(x1, 0);
	CHAOS_SYSVAR_FUNCS(x2, 1);
	CHAOS_SYSVAR_FUNCS(x3, 2);

	CHAOS_SYSPAR_FUNCS_PRED(sigma, m_pred);
	CHAOS_SYSPAR_FUNCS_PRED(b, m_pred);
	CHAOS_SYSPAR_FUNCS_PRED(r, m_pred);

	bool m_pred (t_float f)
	{
		return (f > 0);
	}

};


#define LORENZ_CALLBACKS						\
ODE_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(x1);						\
CHAOS_SYS_CALLBACKS(x2);						\
CHAOS_SYS_CALLBACKS(x3);						\
CHAOS_SYS_CALLBACKS(sigma);						\
CHAOS_SYS_CALLBACKS(r);							\
CHAOS_SYS_CALLBACKS(b);

#define LORENZ_ATTRIBUTES						\
ODE_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(x1);						\
CHAOS_SYS_ATTRIBUTE(x2);						\
CHAOS_SYS_ATTRIBUTE(x3);						\
CHAOS_SYS_ATTRIBUTE(sigma);						\
CHAOS_SYS_ATTRIBUTE(r);							\
CHAOS_SYS_ATTRIBUTE(b);
