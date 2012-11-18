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

//  roessler model: dx1/dt = - (x2 + x3)
//                  dx2/dt = x1 + a * x2
//                  dx3/dt = b + (x1 - c) * x3
//  taken from Peitgen / J�rgens / Saupe: Chaos and Fractals

class roessler
	: public ode_base<3>
{
public:
	roessler()
	{
		CHAOS_PAR_INIT(method,0);
		CHAOS_PAR_INIT(dt,0.001);

		CHAOS_SYS_INIT(x1,0.2,0);
		CHAOS_SYS_INIT(x2,0.1,1);
		CHAOS_SYS_INIT(x3,0.3,2);

		CHAOS_PAR_INIT(a,4);
		CHAOS_PAR_INIT(b,4);
		CHAOS_PAR_INIT(c,4);
	}
	

	virtual void m_system(data_t* deriv, data_t* data)
	{
		data_t x1 = data[0], x2 = data[1], x3 = data[2];
		
		deriv[0] = - (x2 + x3);
		deriv[1] = x1 + CHAOS_PARAMETER(a) * x2;
		deriv[2] = CHAOS_PARAMETER(b) + (x1 - CHAOS_PARAMETER(c)) * x3;
	}

	CHAOS_SYSVAR_FUNCS(x1, 0);
	CHAOS_SYSVAR_FUNCS(x2, 1);
	CHAOS_SYSVAR_FUNCS(x3, 2);

	CHAOS_SYSPAR_FUNCS(a);
	CHAOS_SYSPAR_FUNCS(b);
	CHAOS_SYSPAR_FUNCS(c);
};


#define ROESSLER_CALLBACKS						\
ODE_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(x1);						\
CHAOS_SYS_CALLBACKS(x2);						\
CHAOS_SYS_CALLBACKS(x3);						\
CHAOS_SYS_CALLBACKS(a);							\
CHAOS_SYS_CALLBACKS(b);							\
CHAOS_SYS_CALLBACKS(c);

#define ROESSLER_ATTRIBUTES						\
ODE_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(x1);						\
CHAOS_SYS_ATTRIBUTE(x2);						\
CHAOS_SYS_ATTRIBUTE(x3);						\
CHAOS_SYS_ATTRIBUTE(a);							\
CHAOS_SYS_ATTRIBUTE(b);							\
CHAOS_SYS_ATTRIBUTE(c);
