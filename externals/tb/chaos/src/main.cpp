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

#include "chaos.hpp"
#include "chaos_defs.hpp"

void chaos_library_setup()
{
	post("\nchaos~ version 0.01, compiled on "__DATE__);
	post("(C)2005 Tim Blechmann, www.mokabar.tk\n\n");


	CHAOS_ADD(bernoulli);
	CHAOS_ADD(bungalow_tent);
	CHAOS_ADD(circle_map);
	CHAOS_ADD(coupled_logistic);
	CHAOS_ADD(chua);
	CHAOS_ADD(delayed_logistic);
	CHAOS_ADD(driven_anharmonic);
	CHAOS_ADD(driven_van_der_pol);
	CHAOS_ADD(duffing);
	CHAOS_ADD(duffing_map);
	CHAOS_ADD(gauss_map);
	CHAOS_ADD(gaussian_map);
	CHAOS_ADD(henon);
	CHAOS_ADD(hydrogen);
	CHAOS_ADD(ikeda_laser_map);
	CHAOS_ADD(latoocarfian);
	CHAOS_ADD(latoomutalpha);
	CHAOS_ADD(latoomutbeta);
	CHAOS_ADD(latoomutgamma);
	CHAOS_ADD(linear_congruental);
	CHAOS_ADD(logistic);
	CHAOS_ADD(lorenz);
	CHAOS_ADD(lozi_map);
	CHAOS_ADD(roessler);
	CHAOS_ADD(sine_map);
	CHAOS_ADD(standard_map);
	CHAOS_ADD(tent_map);
}

FLEXT_LIB_SETUP(chaos, chaos_library_setup);
