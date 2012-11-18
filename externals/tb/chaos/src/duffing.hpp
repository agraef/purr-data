//
//
//  chaos~
//  Copyright (C) 2005  Tim Blechmann
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

//  duffing equation: dx/dt = y
//                    dy/dt = x * (1 - x^2) + R*cos(omega * t) - gamma * y
//


class duffing:
    public ode_base<2>
{
public:
    duffing()
    {
        CHAOS_PAR_INIT(method,0);
        CHAOS_PAR_INIT(dt,0.01);

        CHAOS_SYS_INIT(x, 0.5, 0);
        CHAOS_SYS_INIT(y, 0.5, 1);

        CHAOS_PAR_INIT(R, 0.4);
        CHAOS_PAR_INIT(omega, 1);
        CHAOS_PAR_INIT(gamma, 0.25);

        t = 0;
    }


    virtual void m_system(data_t* deriv, data_t* data)
    {
        data_t x = data[0], y = data[1];

        deriv[0] = y;
        deriv[1] = x * (1 - x*x) +
            CHAOS_PARAMETER(R) * cos( CHAOS_PARAMETER(omega) * t) -
            CHAOS_PARAMETER(gamma) * y;
        t += CHAOS_PARAMETER(dt);
        t = std::fmod(t, data_t(M_PI));
    }

    virtual void m_verify()
    {
        if (m_data[0] == 0 && m_data[1] == 0 && m_data[2] == 0)
            for (int i = 0; i != 3; ++i)
                m_data[i] = rand_range(0,3);
    }


    CHAOS_SYSVAR_FUNCS(x,0);
    CHAOS_SYSVAR_FUNCS(y,1);

    CHAOS_SYSPAR_FUNCS(R);
    CHAOS_SYSPAR_FUNCS(omega);
    CHAOS_SYSPAR_FUNCS(gamma);

private:
    data_t t;
};

#define DUFFING_CALLBACKS                       \
ODE_CALLBACKS;                                  \
CHAOS_SYS_CALLBACKS(x);                         \
CHAOS_SYS_CALLBACKS(y);                         \
CHAOS_SYS_CALLBACKS(R);                         \
CHAOS_SYS_CALLBACKS(gamma);                     \
CHAOS_SYS_CALLBACKS(omega);

#define DUFFING_ATTRIBUTES                      \
ODE_ATTRIBUTES;                                 \
CHAOS_SYS_ATTRIBUTE(x);                         \
CHAOS_SYS_ATTRIBUTE(y);                         \
CHAOS_SYS_ATTRIBUTE(R);                         \
CHAOS_SYS_ATTRIBUTE(gamma);                     \
CHAOS_SYS_ATTRIBUTE(omega);

