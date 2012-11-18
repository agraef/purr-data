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

#include "chaos_base.hpp"

template <class system> 
class chaos_dsp
	: public flext_dsp
{
	FLEXT_HEADER(chaos_dsp, flext_dsp);

protected:
    chaos_dsp(int argc, t_atom* argv)
    {
        m_sr = 44100; /* assume default sampling rate */
        int size = m_system.get_num_eq();
    
        m_values = new t_float[size];
        m_slopes = new t_float[size];
        m_nextvalues = new t_float[size];
        m_nextmidpts = new t_float[size];
        m_curves = new t_float[size];

        /* create inlets and zero arrays*/
        for (int i = 0; i != size; ++i)
        {
            AddOutSignal();
            m_values[i] = 0;
            m_slopes[i] = 0;
            m_nextvalues[i] = 0;
            m_nextmidpts[i] = 0;
            m_curves[i] = 0;
        }
    
        FLEXT_ADDATTR_VAR("frequency", get_freq, set_freq);
        FLEXT_ADDATTR_VAR("interpolation_method",get_imethod, set_imethod);

        if (argc > 0)
        {
            CHAOS_INIT(freq, GetAInt(argv[0]));
        }
        else
        {
            CHAOS_INIT(freq, 440);
        }

        if (argc > 1)
        {
            CHAOS_INIT(imethod, GetAInt(argv[1]));
        }
        else
        {
            CHAOS_INIT(imethod, 0);
        }
        m_phase = 0;
    }

    ~chaos_dsp()
    {
        delete[] m_values;
        delete[] m_slopes;
        delete[] m_nextvalues;
        delete[] m_nextmidpts;
        delete[] m_curves;
    }
    
public:
	/* signal functions: */
	/* for frequency = sr */
	void m_signal_(int n, t_sample *const *insigs,t_sample *const *outsigs);
	/* sample & hold */
	void m_signal_n(int n, t_sample *const *insigs,t_sample *const *outsigs);
	/* sample & hold for high frequencies */
	void m_signal_n_hf(int n, t_sample *const *insigs,t_sample *const *outsigs);
	/* linear interpolation */
	void m_signal_l(int n, t_sample *const *insigs,t_sample *const *outsigs);
	/* linear interpolation for high frequencies */
	void m_signal_l_hf(int n, t_sample *const *insigs,t_sample *const *outsigs);
	/* cubic interpolation */
	void m_signal_c(int n, t_sample *const *insigs,t_sample *const *outsigs);
	/* cubic interpolation for high frequencies */
	void m_signal_c_hf(int n, t_sample *const *insigs,t_sample *const *outsigs);
	
	virtual void CbSignal()
	{
 		(this->*m_routine)(Blocksize(),InSig(),OutSig());
	}

	virtual bool CbDsp()
	{
		m_sr = Samplerate();
		set_freq(m_freq); /* maybe we have to change the interpolation mode */
		return true;
	}
	
	void (thisType::*m_routine)(int n, t_sample *const *insigs,t_sample *const *outsigs);
	
	/* local data for system, output and interpolation */
	system m_system; /* the system */

	t_sample * m_values;   /* actual value */
	t_sample * m_slopes;   /* actual slope for cubic interpolation */

    t_sample * m_nextvalues;
    t_sample * m_nextmidpts;
    t_sample * m_curves;
	
	/* local data for signal functions */
	float m_freq;        /* frequency of oscillations */
	float m_invfreq;     /* inverse frequency */
	float m_phase;       /* phase */
	float m_sr;          /* sample rate */
	
	int m_imethod;       /* interpolation method */

	void get_imethod(int &i)
	{
		i = m_imethod;
	}

	void set_imethod(int i)
	{
		int imethod = m_imethod;
		if( (i >= 0) && (i <= 2) )
		{
			m_imethod = i;
			switch (i)
			{
			case 0:
				m_routine = &thisType::m_signal_n;
				break;
			case 1:
				m_routine = &thisType::m_signal_l;
				break;
			case 2:
				m_routine = &thisType::m_signal_c;
				break;
			}
		}
		else
		{
			post("interpolation method out of range");
			return;
		}

		if (imethod == 0)
			for (int j = 0; j != m_system.get_num_eq(); ++j)
			{
				m_values[j] = m_system.get_data(j);
				m_slopes[j] = 0;
			}

		if(i == 2 && imethod != 2)
		{
			for (int j = 0; j != m_system.get_num_eq(); ++j)
			{
				m_phase = 0; /* reschedule to avoid click, find a better way later*/
				m_nextvalues[j] = m_values[j];
				m_nextmidpts[j] = m_values[j];
			}
		}
		set_freq(m_freq);
	}

	void get_freq(float &f)
	{
		f = m_freq;
	}

	void set_freq(float f)
	{
		if (f < 0) /* we can't go back in time :-) */
			f = -f;

		if( f <= m_sr * 0.01 ) 
		{
			switch(m_imethod)
			{
			case 0:
				m_routine = &thisType::m_signal_n;
				break;
			case 1:
				m_routine = &thisType::m_signal_l;
				break;
			case 2:
				m_routine = &thisType::m_signal_c;
				break;
			default:
				assert(false);
			}
		}
		else
		{
			switch(m_imethod)
			{
			case 0:
				m_routine = &thisType::m_signal_n_hf;
				break;
			case 1:
				m_routine = &thisType::m_signal_l_hf;
				break;
			case 2:
				m_routine = &thisType::m_signal_c_hf;
				break;
			default:
				assert(false);
			}
		}
		
		m_freq = f;
		m_invfreq = 1.f / f;
	}
	
	FLEXT_CALLVAR_F(get_freq, set_freq);
	FLEXT_CALLVAR_I(get_imethod, set_imethod);
};



/* create constructor / destructor */
#define CHAOS_DSP_INIT(SYSTEM, ATTRIBUTES)								\
FLEXT_HEADER(SYSTEM##_dsp, chaos_dsp<SYSTEM>)							\
																		\
SYSTEM##_dsp(int argc, t_atom* argv):                                   \
    chaos_dsp<SYSTEM>(argc, argv)          			                   	\
{																		\
   ATTRIBUTES;															\
}																		\
																		\
FLEXT_ATTRVAR_F(m_freq);											     	\
FLEXT_ATTRVAR_I(m_imethod);



template <class system> 
void chaos_dsp<system>::m_signal_(int n, t_sample *const *insigs,
	t_sample *const *outsigs)
{
	int outlets = m_system.get_num_eq();

	for (int i = 0; i!=n; ++i)
	{
        m_system.m_step();
        m_system.m_bash_denormals();
        m_system.m_verify();

		for (int j = 0; j != outlets; ++j)
		{
			outsigs[j][i] = m_system.get_data(j);
		}
	}
}


template <class system> 
void chaos_dsp<system>::m_signal_n_hf(int n, t_sample *const *insigs,
	t_sample *const *outsigs)
{
	int outlets = m_system.get_num_eq();
	
	float phase = m_phase;
	
	int offset = 0;
	while (n)
	{
		while (phase <= 0)
		{
            m_system.m_step();
            m_system.m_bash_denormals();
            m_system.m_verify();

			phase += m_sr * m_invfreq;
		}
		int next = (phase < n) ? int(ceilf (phase)) : n;
		n -= next;
		phase -=next;
		
		for (int i = 0; i != outlets; ++i)
		{
			SetSamples(outsigs[i]+offset, next, m_system.get_data(i));
		}
		offset += next;
	}
	m_phase = phase;
}


template <class system> 
void chaos_dsp<system>::m_signal_n(int n, t_sample *const *insigs,
	t_sample *const *outsigs)
{
	int outlets = m_system.get_num_eq();
	
	int phase = int(m_phase);
	
	int offset = 0;
	while (n)
	{
		if (phase == 0)
		{
            m_system.m_step();
            m_system.m_bash_denormals();
            m_system.m_verify();

			phase = int (m_sr * m_invfreq);
		}
		
		int next = (phase < n) ? phase : n;
		n -= next;
		phase -=next;
		
		for (int i = 0; i != outlets; ++i)
		{
			SetSamples(outsigs[i]+offset, next, m_system.get_data(i));
		}
		offset += next;
	}
	m_phase = phase;
}

/* linear and cubic interpolation adapted from supercollider by James McCartney */
template <class system> 
void chaos_dsp<system>::m_signal_l(int n, t_sample *const *insigs,
	t_sample *const *outsigs)
{
	int outlets = m_system.get_num_eq();
	
	int phase = int(m_phase);

	int i = 0;

	while (n)
	{
		if (phase == 0)
		{
            m_system.m_step();
            m_system.m_bash_denormals();
            m_system.m_verify();

			phase = int (m_sr * m_invfreq);

			for (int j = 0; j != outlets; ++j)
				m_slopes[j] = (m_system.get_data(j) - m_values[j]) / phase;
		}
		
		int next = (phase < n) ? phase : n;
		n -= next;
		phase -=next;
		
		while (next--)
		{
			for (int j = 0; j != outlets; ++j)
			{
				outsigs[j][i] = m_values[j];
				m_values[j]+=m_slopes[j];
			}
			++i;
		}
	}
	m_phase = phase;
}



template <class system> 
void chaos_dsp<system>::m_signal_l_hf(int n, t_sample *const *insigs,
	t_sample *const *outsigs)
{
	int outlets = m_system.get_num_eq();
	
	float phase = m_phase;

	int i = 0;

	while (n)
	{
		if (phase <= 0)
		{
            m_system.m_step();
            m_system.m_bash_denormals();
            m_system.m_verify();

			phase = m_sr * m_invfreq;

			for (int j = 0; j != outlets; ++j)
				m_slopes[j] = (m_system.get_data(j) - m_values[j]) / phase;
		}
		
		int next = (phase < n) ? int(ceilf (phase)) : n;
		n -= next;
		phase -=next;
		
		while (next--)
		{
			for (int j = 0; j != outlets; ++j)
			{
				outsigs[j][i] = m_values[j];
				m_values[j]+=m_slopes[j];
			}
			++i;
		}
	}
	m_phase = phase;
}


template <class system> 
void chaos_dsp<system>::m_signal_c(int n, t_sample *const *insigs,
	t_sample *const *outsigs)
{
	int outlets = m_system.get_num_eq();
	
	int phase = int(m_phase);

	int i = 0;

	while (n)
	{
		if (phase == 0)
		{
            m_system.m_step();
            m_system.m_bash_denormals();
            m_system.m_verify();

			phase = int (m_sr * m_invfreq);
			phase = (phase > 2) ? phase : 2;
			
			for (int j = 0; j != outlets; ++j)
			{
				t_sample value = m_nextvalues[j];
				m_nextvalues[j]= m_system.get_data(j);
				
				m_values[j] =  m_nextmidpts[j];
				m_nextmidpts[j] = (m_nextvalues[j] + value) * 0.5f;
				
				float fseglen = (float)phase;
				m_curves[j] = 2.f * (m_nextmidpts[j] - m_values[j] - 
					fseglen * m_slopes[j]) 
					/ (fseglen * fseglen + fseglen);
			}
		}
		
		int next = (phase < n) ? phase : n;
		n -= next;
		phase -=next;
		
		while (next--)
		{
			for (int j = 0; j != outlets; ++j)
			{
				outsigs[j][i] = m_values[j];
				m_slopes[j]+=m_curves[j];
				m_values[j]+=m_slopes[j];
			}
			++i;
		}
	}
	m_phase = phase;
}


template <class system> 
void chaos_dsp<system>::m_signal_c_hf(int n, t_sample *const *insigs,
	t_sample *const *outsigs)
{
	int outlets = m_system.get_num_eq();
	
	float phase = m_phase;

	int i = 0;

	while (n)
	{
		if (phase == 0)
		{
            m_system.m_step();
            m_system.m_bash_denormals();
            m_system.m_verify();


			phase = int (m_sr * m_invfreq);
			phase = (phase > 2) ? phase : 2;
			
			for (int j = 0; j != outlets; ++j)
			{
				t_sample value = m_nextvalues[j];
				m_nextvalues[j]= m_system.get_data(j);
				
				m_values[j] =  m_nextmidpts[j];
				m_nextmidpts[j] = (m_nextvalues[j] + value) * 0.5f;
				
				float fseglen = (float)phase;
				m_curves[j] = 2.f * (m_nextmidpts[j] - m_values[j] - 
					fseglen * m_slopes[j]) 
					/ (fseglen * fseglen + fseglen);
			}
		}
		
		int next = (phase < n) ? int(ceilf (phase)) : n;
		n -= next;
		phase -=next;
		
		while (next--)
		{
			for (int j = 0; j != outlets; ++j)
			{
				outsigs[j][i] = m_values[j];
				m_slopes[j]+=m_curves[j];
				m_values[j]+=m_slopes[j];
			}
			++i;
		}
	}
	m_phase = phase;
}
