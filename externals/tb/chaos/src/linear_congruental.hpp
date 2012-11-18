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

//  tent map: x[n+1] = A*x[n] + B mod C
//
//  taken from Julien C. Sprott, Chaos and Time-Series Analysis

class linear_congruental:
    public map_base<2>
{
public:
    linear_congruental()
    {
        CHAOS_SYS_INIT(x, 0, 0);

        CHAOS_PAR_INIT(A, 1741);
        CHAOS_PAR_INIT(B, 54773);
        CHAOS_PAR_INIT(C, 259200);
    }

    void m_step()
    {
        data_t x = m_data[0];

        m_data[0] = std::fmod( CHAOS_PARAMETER(A) * x + CHAOS_PARAMETER(B), CHAOS_PARAMETER(C));
    }

    CHAOS_SYSVAR_FUNCS(x,0);

    CHAOS_SYSPAR_FUNCS(A);
    CHAOS_SYSPAR_FUNCS(B);
    CHAOS_SYSPAR_FUNCS(C);
};

#define LINEAR_CONGRUENTAL_CALLBACKS            \
MAP_CALLBACKS;                                  \
CHAOS_SYS_CALLBACKS(A);                         \
CHAOS_SYS_CALLBACKS(B);                         \
CHAOS_SYS_CALLBACKS(C);                         \
CHAOS_SYS_CALLBACKS(x);

#define LINEAR_CONGRUENTAL_ATTRIBUTES           \
MAP_ATTRIBUTES;                                 \
CHAOS_SYS_ATTRIBUTE(A);                         \
CHAOS_SYS_ATTRIBUTE(B);                         \
CHAOS_SYS_ATTRIBUTE(C);                         \
CHAOS_SYS_ATTRIBUTE(x);

