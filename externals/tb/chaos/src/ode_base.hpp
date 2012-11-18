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


#ifndef __ode_base_hpp

#include "chaos_base.hpp"

template <int dimensions>
class ode_base:
    public chaos_base<dimensions>
{
public:

	void set_method(int i)
	{
		if (i >= 0 && i < 4)
		{
			m_method = (unsigned char) i;
			switch (i)
			{
			case 0:
				m_routine = &ode_base<dimensions>::rk1;
				break;
			case 1:
				m_routine = &ode_base<dimensions>::rk2;
				break;
			case 2:
				m_routine = &ode_base<dimensions>::rk4;
			}
		}
		else
			post("no such method");
	}

	t_int get_method()
	{
		return (int) m_method;
	}

	CHAOS_SYSPAR_FUNCS_PRED(dt, m_pred_dt);

	bool m_pred_dt(t_float f)
	{
		return (f >= 0);
	}

	void m_step()
	{
		(this->*m_routine)();
	}
	
protected:
 	void (ode_base<dimensions>::*m_routine)(void);

	unsigned char m_method; /* 0: rk1, 1: rk2, 3: rk4 */

    data_t m_k0[dimensions];
    data_t m_k1[dimensions];
    data_t m_k2[dimensions];
    data_t m_k3[dimensions];
	data_t m_tmp[dimensions];   

	virtual void m_system (data_t* deriv, data_t* data) = 0;

    void rk1()
    {
        m_system (m_k0, chaos_base<dimensions>::m_data);
        for (int i = 0; i != dimensions; ++i)
            chaos_base<dimensions>::m_data[i] += m_dt * m_k0[i];
    }
    
    void rk2()
    {
        for (int i = 0; i != dimensions; ++i)
            m_k0[i] = m_k0[i] * 0.5 * m_dt + chaos_base<dimensions>::m_data[i];
        
        m_system (m_k0, chaos_base<dimensions>::m_data);
        m_system (m_k1, m_k0);
        for (int i = 0; i != dimensions; ++i)
            chaos_base<dimensions>::m_data[i] += m_dt * m_k1[i];
    }
    
    void rk4()
    {
        m_system (m_k0, chaos_base<dimensions>::m_data);
        for (int i = 0; i != dimensions; ++i)
        {
            m_k0[i] *= m_dt;
            m_tmp[i] = chaos_base<dimensions>::m_data[i] + 0.5 * m_k0[i];
        }
    
        m_system (m_k1, m_tmp);
        for (int i = 0; i != dimensions; ++i)
        {
            m_k1[i] *= m_dt;
            m_tmp[i] = chaos_base<dimensions>::m_data[i] + 0.5 * m_k1[i];
        }
        
        m_system (m_k2, m_tmp);
        for (int i = 0; i != dimensions; ++i)
        {
            m_k2[i] *= m_dt;
            m_tmp[i] = chaos_base<dimensions>::m_data[i] + m_k2[i];
        }
    
        m_system (m_k3, m_tmp);
        for (int i = 0; i != dimensions; ++i)
            m_k3[i] *= m_dt;
    
        for (int i = 0; i != dimensions; ++i)
            chaos_base<dimensions>::m_data[i] += (m_k0[i] + 2. * (m_k1[i] + m_k2[i]) + m_k3[i]) 
                / 6.;
    }
};

#define ODE_CALLBACKS							\
CHAOS_SYS_CALLBACKS_I(method);					\
CHAOS_SYS_CALLBACKS(dt);

#define ODE_ATTRIBUTES							\
CHAOS_SYS_ATTRIBUTE(method);					\
CHAOS_SYS_ATTRIBUTE(dt);


#define __ode_base_hpp
#endif /* __ode_base_hpp */
