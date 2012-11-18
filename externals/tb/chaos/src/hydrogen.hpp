// 
//  
//  chaos~
//  Copyright (C) 2005 Tim Blechmann
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

// hydrogen atom in a magnetic field

class hydrogen
	: public ode_base<4>
{


public:
	hydrogen()
	{
		CHAOS_PAR_INIT(method,2);
		CHAOS_PAR_INIT(dt,0.01);

		CHAOS_SYS_INIT(mu,0.8, 0);
		CHAOS_SYS_INIT(muv,0.6, 1);
		CHAOS_SYS_INIT(nu,0.4, 2);
		CHAOS_SYS_INIT(nuv,0.4, 3);
		
		CHAOS_PAR_INIT(etilde,-0.3);

		reset = 0;
	}
	

	virtual void m_system(data_t* deriv, data_t* data)
	{
		if (reset)
		{
			(this->*reset)();
			reset = 0;
		}
		
		data_t mu = m_data[0], muv = m_data[1], nu = m_data[2], nuv = m_data[3];
		data_t E = CHAOS_PARAMETER(etilde);
		
		deriv[0] = muv;
		deriv[1] = 2* E * mu - 0.25 * mu * nu * nu * (2*mu*mu+nu*nu);
		deriv[2] = nuv;
		deriv[3] = 2* E * nu - 0.25 * nu * mu * mu * (2*nu*nu+mu*mu);
	}
	
	virtual void m_verify()
	{
		/* make sure to stay in the range of 2 pi */
		for (int i = 0; i != get_num_eq(); ++ i)
			{
				if (m_data[i] > 1)
					m_data[i] = 1;
				else if (m_data[i] < -1)
					m_data[i] = -1;
			}
	}

	void reset_nuv()
    {
		data_t mu = m_data[0], muv = m_data[1], nu = m_data[2];
		data_t E = CHAOS_PARAMETER(etilde);
		
		m_data[1]= sqrt ( 2 * E * (mu*mu + nu*nu) - muv*muv - ( mu*mu * nu*nu *
							  ( mu*mu + nu*nu )) * 0.25);
		
//      	if (fabs((data[3]))<1e-5)
// 			data[3]=0;
    }
    
    void reset_muv()
    {
		data_t mu = m_data[0], nu = m_data[2], nuv = m_data[3];
		data_t E = CHAOS_PARAMETER(etilde);

		m_data[1]= sqrt ( 2 * E * (mu*mu + nu*nu) - nuv*nuv - ( mu*mu * nu*nu *
							  ( mu*mu + nu*nu )) * 0.25);
		
// 		if (fabs((data[1]))<1e-5)
// 			data[1]=0;
    }
	

	/* hook into the predicate to reset the system */
	bool m_pred_pos(t_float f)
	{
		if (fabs(f) > 1)
			return false;
		reset = &hydrogen::reset_nuv;
		return true;
	}

	bool m_pred_nuv(t_float f)
	{
		reset = &hydrogen::reset_muv;
		return true;
	}

	bool m_pred_muv(t_float f)
	{
		reset = &hydrogen::reset_nuv;
		return true;
	}
	
	void (hydrogen::*reset)(void);
	
	CHAOS_SYSVAR_FUNCS_PRED(mu, 0, m_pred_pos);
	CHAOS_SYSVAR_FUNCS_PRED(muv, 1, m_pred_nuv);
 	CHAOS_SYSVAR_FUNCS_PRED(nu, 2, m_pred_pos);
 	CHAOS_SYSVAR_FUNCS_PRED(nuv, 3, m_pred_muv);

	CHAOS_SYSPAR_FUNCS(etilde);
};


#define HYDROGEN_CALLBACKS						\
ODE_CALLBACKS;									\
CHAOS_SYS_CALLBACKS(mu);						\
CHAOS_SYS_CALLBACKS(muv);						\
CHAOS_SYS_CALLBACKS(nu);						\
CHAOS_SYS_CALLBACKS(nuv);						\
CHAOS_SYS_CALLBACKS(etilde);

#define HYDROGEN_ATTRIBUTES						\
ODE_ATTRIBUTES;									\
CHAOS_SYS_ATTRIBUTE(mu);						\
CHAOS_SYS_ATTRIBUTE(muv);						\
CHAOS_SYS_ATTRIBUTE(nu);						\
CHAOS_SYS_ATTRIBUTE(nuv);						\
CHAOS_SYS_ATTRIBUTE(etilde);
