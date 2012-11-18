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
#include <cmath>

//  driven anharmonic:
//      d2u/dt2 + a * du/dt + b*u + c * u*u*u = k1 + k2*cos(Omega*t)
//  equivalent:
//      du1/dt = u2
//      du2/dt = -a*u2 - b*u1 -c*u*u*u + k1 + k2*cos(Omega*t)
//  taken from Willi-Hans Steeb: Chaos and Fractals

class driven_anharmonic
	: public ode_base<2>
{
public:
	driven_anharmonic()
	{
		CHAOS_PAR_INIT(method,0);
		CHAOS_PAR_INIT(dt,0.01);

		CHAOS_SYS_INIT(u1,0,0);
		CHAOS_SYS_INIT(u2,1,1);

		CHAOS_PAR_INIT(a,1);
		CHAOS_PAR_INIT(b,-10);
		CHAOS_PAR_INIT(c,100);
		CHAOS_PAR_INIT(Omega,3.5);
		CHAOS_PAR_INIT(k1,0.01);
		CHAOS_PAR_INIT(k2,1);

		m_t = 0;
	}


	virtual void m_system(data_t* deriv, data_t* data)
	{
		data_t u1 = data[0], u2 = data[1];
		
		deriv[0] = u2;
		deriv[1] = - CHAOS_PARAMETER(a) * u2 - CHAOS_PARAMETER(b) * u1 - 
			CHAOS_PARAMETER(c) * u1*u1*u1 + CHAOS_PARAMETER(k1) +
			CHAOS_PARAMETER(k2) * cos (CHAOS_PARAMETER(Omega) * m_t);
		m_t += m_dt;
		
		if (m_t > 2 * M_PI)
			m_t = fmod(m_t, (data_t)(M_PI*2));
	}
	
	data_t m_t;
	
	CHAOS_SYSVAR_FUNCS(u1, 0);
	CHAOS_SYSVAR_FUNCS(u2, 1);

	CHAOS_SYSPAR_FUNCS(a);
	CHAOS_SYSPAR_FUNCS(b);
	CHAOS_SYSPAR_FUNCS(c);
	CHAOS_SYSPAR_FUNCS(k1);
	CHAOS_SYSPAR_FUNCS(k2);
	CHAOS_SYSPAR_FUNCS(Omega);
};


#define DRIVEN_ANHARMONIC_CALLBACKS				\
ODE_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(u1);						\
CHAOS_SYS_CALLBACKS(u2);						\
CHAOS_SYS_CALLBACKS(a);							\
CHAOS_SYS_CALLBACKS(b);							\
CHAOS_SYS_CALLBACKS(c);							\
CHAOS_SYS_CALLBACKS(k1);						\
CHAOS_SYS_CALLBACKS(k2);						\
CHAOS_SYS_CALLBACKS(Omega);

#define DRIVEN_ANHARMONIC_ATTRIBUTES			\
ODE_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(u1);						\
CHAOS_SYS_ATTRIBUTE(u2);						\
CHAOS_SYS_ATTRIBUTE(a);							\
CHAOS_SYS_ATTRIBUTE(b);							\
CHAOS_SYS_ATTRIBUTE(c);							\
CHAOS_SYS_ATTRIBUTE(k1);						\
CHAOS_SYS_ATTRIBUTE(k2);						\
CHAOS_SYS_ATTRIBUTE(Omega);
