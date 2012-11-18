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

//  driven van_der_pol:
//      d2u/dt2 + a*(u*u -1)*du/dt + u = k * cos(Omega*t)
//  equivalent:
//      du1/dt = u2
//      du2/dt = -a*(u1*u1 - 1)*u2 - u1 + k*cos(u3)
//      du3/dt = Omega
//  taken from Willi-Hans Steeb: Chaos and Fractals

class driven_van_der_pol
    : public ode_base<3>
{
public:
    driven_van_der_pol()
    {
        CHAOS_PAR_INIT(method,2);
        CHAOS_PAR_INIT(dt,0.01);

        CHAOS_SYS_INIT(u1,0.8, 0);
        CHAOS_SYS_INIT(u2,0.6, 1);
        CHAOS_SYS_INIT(u3,0.4, 2);

        CHAOS_PAR_INIT(a,5);
        CHAOS_PAR_INIT(Omega,2.466);
        CHAOS_PAR_INIT(k,5);
    }



    void m_system(data_t* deriv, data_t* data)
    {
        data_t u1 = data[0], u2 = data[1], u3 = data[2];

        deriv[0] = u2;
        deriv[1] = - CHAOS_PARAMETER(a) * (u1*u1 -1) * u2 - u1 +
            CHAOS_PARAMETER(k)*cos(u3);
        deriv[2] = CHAOS_PARAMETER(Omega);
    }

    void m_verify()
    {
        /* make sure to stay in the range of 2 pi */
        if (m_data[2] > 2*M_PI)
            m_data[2] = std::fmod(m_data[2], data_t(2*M_PI));
    }

    CHAOS_SYSVAR_FUNCS(u1, 0);
    CHAOS_SYSVAR_FUNCS(u2, 1);
    CHAOS_SYSVAR_FUNCS(u3, 2);

    CHAOS_SYSPAR_FUNCS(a);
    CHAOS_SYSPAR_FUNCS(k);
    CHAOS_SYSPAR_FUNCS(Omega);
};


#define DRIVEN_VAN_DER_POL_CALLBACKS            \
ODE_CALLBACKS;                                  \
CHAOS_SYS_CALLBACKS(u1);                        \
CHAOS_SYS_CALLBACKS(u2);                        \
CHAOS_SYS_CALLBACKS(u3);                        \
CHAOS_SYS_CALLBACKS(a);                         \
CHAOS_SYS_CALLBACKS(k);                         \
CHAOS_SYS_CALLBACKS(Omega);

#define DRIVEN_VAN_DER_POL_ATTRIBUTES           \
ODE_ATTRIBUTES;                                 \
CHAOS_SYS_ATTRIBUTE(u1);                        \
CHAOS_SYS_ATTRIBUTE(u2);                        \
CHAOS_SYS_ATTRIBUTE(u3);                        \
CHAOS_SYS_ATTRIBUTE(a);                         \
CHAOS_SYS_ATTRIBUTE(k);                         \
CHAOS_SYS_ATTRIBUTE(Omega);
