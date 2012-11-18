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

//  chua system: dx1/dt = alpha * (x2 - x1 - h(x1))
//               dx2/dt = x1 - x2 + x3
//               dx3/dt = - beta * x2
//
//          with h(x) = b*x + a - b    (for x > 1)
//                      a*x            (for -1 <= x <= 1
//                      b*x - a + b    (for x < -1)
//
//  taken from Viktor Avrutin: lecture notes

class chua:
    public ode_base<3>
{
public:
    chua()
    {
        CHAOS_PAR_INIT(method,2);
        CHAOS_PAR_INIT(dt,0.05);

        CHAOS_SYS_INIT(x1,1,1);
        CHAOS_SYS_INIT(x2,1,1);
        CHAOS_SYS_INIT(x3,1,1);

        CHAOS_PAR_INIT(a,140);
        CHAOS_PAR_INIT(b,30);
        CHAOS_PAR_INIT(alpha,30);
        CHAOS_PAR_INIT(beta,30);
    }

    virtual void m_system(data_t* deriv, data_t* data)
    {
        data_t x1 = data[0];
        data_t x2 = data[1];
        data_t x3 = data[2];

        data_t a = CHAOS_PARAMETER(a), b = CHAOS_PARAMETER(b);

        data_t h;

        if (x1 > 1)
            h = b*x1 + a - b;
        else if (x1 < -1)
            h = b*x1 - a + b;
        else
            h = a*x1;

        deriv[0] = CHAOS_PARAMETER(alpha) * (x2 - x1 - h);
        deriv[1] = x1 - x2 + x3;
        deriv[2] = - CHAOS_PARAMETER(beta) * x2;
    }

    virtual void m_verify()
    {
        data_t x1 = m_data[0];
        data_t x2 = m_data[1];
        data_t x3 = m_data[2];

        if ( std::abs(x3) < 1e-10)
            x3 = 0;

        if (x1 == 0 && x2 == 0 && x3 == 0) /* fixpoint at (0,0,0) */
        {
            reset();
            return;
        }
        else
        {
            data_t c = m_c;
            if (m_c)
            {
                data_t mc = - c;
                if (x1 == c && x3 == mc) /* fixpoint at (c,0,-c) */
                {
                    reset();
                    return;
                }
                if (x1 == mc && x3 == c) /* fixpoint at (-c,0,c) */
                {
                    reset();
                    return;
                }
            }
        }
    }

    inline void reset()
    {
        m_data[0] = rand_range(-1,1);
        m_data[1] = rand_range(-1,1);
        m_data[2] = rand_range(-1,1);
    }

    CHAOS_SYSVAR_FUNCS(x1, 0);
    CHAOS_SYSVAR_FUNCS(x2, 1);
    CHAOS_SYSVAR_FUNCS(x3, 2);

    /* due to stability issues, we hook into the predicates */
    data_t m_c;
    bool m_pred_a(data_t a)
    {
        data_t b = CHAOS_PARAMETER(b);
        m_c = (b - a) / (b + 1);
        return true;
    }

    bool m_pred_b(data_t b)
    {
        if (b == -1)
            m_c = 0;
        else
        {
            data_t a = CHAOS_PARAMETER(a);
            m_c = (b - a) / (b + 1);
        }
        return true;
    }

    CHAOS_SYSPAR_FUNCS_PRED(a, m_pred_a);
    CHAOS_SYSPAR_FUNCS_PRED(b, m_pred_b);
    CHAOS_SYSPAR_FUNCS(alpha);
    CHAOS_SYSPAR_FUNCS(beta);
};


#define CHUA_CALLBACKS                          \
ODE_CALLBACKS;                                  \
CHAOS_SYS_CALLBACKS(alpha);                     \
CHAOS_SYS_CALLBACKS(beta);                      \
CHAOS_SYS_CALLBACKS(a);                         \
CHAOS_SYS_CALLBACKS(b);                         \
CHAOS_SYS_CALLBACKS(x1);                        \
CHAOS_SYS_CALLBACKS(x2);                        \
CHAOS_SYS_CALLBACKS(x3);

#define CHUA_ATTRIBUTES                         \
ODE_ATTRIBUTES;                                 \
CHAOS_SYS_ATTRIBUTE(a);                         \
CHAOS_SYS_ATTRIBUTE(b);                         \
CHAOS_SYS_ATTRIBUTE(alpha);                     \
CHAOS_SYS_ATTRIBUTE(beta);                      \
CHAOS_SYS_ATTRIBUTE(x1);                        \
CHAOS_SYS_ATTRIBUTE(x2);                        \
CHAOS_SYS_ATTRIBUTE(x3);
