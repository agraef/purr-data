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

//  tent map: x[n+1] = 1 - 2*abs(x[n])
//                     -1 < x[n] < 1
//  taken from Julien C. Sprott, Strange Attractors: Creating Patterns in Chaos


class tent_map:
    public map_base<1>
{
public:
    tent_map()
    {
        CHAOS_SYS_INIT(x, 0.6,0);
    }


    void m_step()
    {
        data_t data = m_data[0];

        m_data[0] = 1 - 2*std::abs(data);
    }

    CHAOS_SYSVAR_FUNCS_PRED(x, 0, m_pred_x);
    bool m_pred_x(t_float f)
    {
        return (f > -1) && (f < 1);
    }
};


#define TENT_MAP_CALLBACKS                      \
MAP_CALLBACKS                                   \
CHAOS_SYS_CALLBACKS(x);

#define TENT_MAP_ATTRIBUTES                     \
MAP_ATTRIBUTES                                  \
CHAOS_SYS_ATTRIBUTE(x);


