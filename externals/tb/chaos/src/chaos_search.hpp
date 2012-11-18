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

#include "chaos_base.hpp"

#define MAXDIMENSION 5

template <class system> class chaos_search
	: public flext_base
{
	FLEXT_HEADER(chaos_search, flext_base);

public:

	/* local data for system, output and interpolation */
	system m_system; /* the system */
	

 	data_t min[MAXDIMENSION];      /* minimal coordinates */
 	data_t max[MAXDIMENSION];      /* maximal coordinates */
	data_t final[MAXDIMENSION];    /* initial coordinates for outlet */
	
	data_t ly;            /* lyapunov exponent */

	int m_transient_steps;  /* steps before starting the analysis */
	int m_asymptotic_steps; /* steps for the analysis */
	
	
	void get_tsteps(int &i)
	{
		i = m_transient_steps;
	}
	
	void set_tsteps(int i)
	{
		if (i > 0)
			m_transient_steps = i;
		else
			m_transient_steps = 0;
	}

	void get_asteps(int &i)
	{
		i = m_asymptotic_steps;
	}

	void set_asteps(int &i)
	{
		if (i > 0)
			m_asymptotic_steps = i;
		else
			m_asymptotic_steps = 0;
	}	

	void print_results(void)
	{
		/* - send parameters to 1
		   - send initial coordinates to 2
		   - send minimal coordinates to 3
		   - send lyapunov exponent to 4
		*/
		
		for (std::map<const t_symbol*,int>::iterator it = m_system.attr_ind.begin();
			 it != m_system.attr_ind.end(); ++it)
		{
			post("key %s", it->first->s_name);
			post("value %f", m_system.get_data(it->second));
		}
	}	
	
	void m_search();
	
	FLEXT_CALLBACK(m_bang);
	FLEXT_CALLVAR_I(get_tsteps, set_tsteps);
	FLEXT_CALLVAR_I(get_asteps, set_asteps);
	FLEXT_THREAD(m_search);
};


/* create constructor / destructor */

#define CHAOS_SEARCH_INIT(SYSTEM, ATTRIBUTES)						\
FLEXT_HEADER(SYSTEM##_search, chaos_search<SYSTEM>)					\
																	\
SYSTEM##_search(int argc, t_atom* argv )							\
{																	\
	int size = m_system.get_num_eq();								\
																	\
																	\
    m_asymptotic_steps = 10000; 									\
    m_transient_steps = 100;										\
																	\
    AddOutList("parameters");										\
    AddOutList("initial coordinates");								\
    AddOutList("minimal coordinates");								\
    AddOutList("maximal coordinates");								\
    AddOutFloat("lyapunov exponent");								\
																	\
	FLEXT_ADDATTR_VAR("transient_steps", get_tsteps, set_tsteps);	\
	FLEXT_ADDATTR_VAR("steps", get_asteps, set_asteps);				\
    ATTRIBUTES;														\
    FLEXT_ADDMETHOD_(0,"search", m_search);							\
}																	\
																	\
																	\
FLEXT_ATTRVAR_I(m_transient_steps);									\
FLEXT_ATTRVAR_I(m_asymptotic_steps);
 


template <class system> 
void chaos_search<system>::m_search()
{
	int dimensions = m_system.get_num_eq();
	
	ly = 0;
	data_t diff_old = 0.1;
	data_t last[MAXDIMENSION];

	/* transient dynamics */
	for (int i = 0; i != m_transient_steps; ++i)
	{
        m_system.m_step();
        m_system.m_bash_denormals();
        m_system.m_verify();
	}

	for (int i = 0; i != dimensions; ++i)
	{
		last[i] = min[i] = max[i] = m_system.m_data[i];
	}
	
	/* now we start the analysis */

	for (int i = 0; i != m_asymptotic_steps; ++i)
	{
        m_system.m_step();
        m_system.m_bash_denormals();
        m_system.m_verify();
		
		data_t diff = 0;
		for (int j = 0; j != dimensions; ++j)
		{
			/* update min/max */
			data_t datum = m_system.m_data[j];
			if (datum > max[j])
				max[j] = datum;
			else if (datum < min[j])
				min[j] = datum;
			
			/* sum up diff */
			diff += (last[j] - datum) * (last[j] - datum);
			
			last[j] = datum;
		}
		diff = sqrt(diff);
		
		if (diff < 0)
			diff = -diff;

		ly += log(diff / diff_old);
		diff_old = diff;
	
		/* todo: maybe some overflow checking */
		if (diff == 0)
			break;
	}
	

	ly /= m_asymptotic_steps;
	
	print_results();
}
