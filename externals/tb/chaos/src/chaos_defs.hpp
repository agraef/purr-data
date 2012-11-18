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

#ifndef __chaos_defs_hpp


// macros for simplified system state functions
#define CHAOS_SYS_SETFUNC(NAME, NR)				\
	void set_##NAME(t_float f)					\
	{											\
		m_data[NR] = (data_t) f;				\
	}

#define CHAOS_SYS_SETFUNC_PRED(NAME, NR, PRED)							\
	void set_##NAME(t_float f)											\
	{																	\
		if ( PRED(f) )													\
			m_data[NR] = (data_t) f;									\
		else															\
			post("value for dimension " #NAME " %f out of range", f);	\
	}

#define CHAOS_SYS_GETFUNC(NAME, NR)				\
	t_float get_##NAME()						\
	{											\
		return (t_float)m_data[NR];				\
	}

/* to be called in the public part */			
#define CHAOS_SYSVAR_FUNCS_PRED(NAME, NR, PRED)	\
CHAOS_SYS_SETFUNC_PRED(NAME, NR, PRED)			\
CHAOS_SYS_GETFUNC(NAME, NR)

#define CHAOS_SYSVAR_FUNCS(NAME, NR)			\
CHAOS_SYS_SETFUNC(NAME, NR)						\
CHAOS_SYS_GETFUNC(NAME, NR)



// macros for simplified system parameter functions
#define CHAOS_PAR_SETFUNC(NAME)					\
	void set_##NAME(t_float f)					\
	{											\
		m_##NAME = (data_t) f;					\
	}

#define CHAOS_PAR_SETFUNC_PRED(NAME, PRED)								\
	void set_##NAME(t_float f)											\
	{																	\
		if ( PRED(f) )													\
			m_##NAME = (data_t) f;										\
		else															\
			post("value for parameter " #NAME " %f out of range", f);	\
	}

#define CHAOS_PAR_GETFUNC(NAME)					\
	t_float get_##NAME()						\
	{											\
		return (t_float)m_##NAME;				\
	}


#define CHAOS_SYSPAR_FUNCS_PRED(NAME, PRED)		\
CHAOS_PAR_SETFUNC_PRED(NAME, PRED)				\
CHAOS_PAR_GETFUNC(NAME)							\
data_t m_##NAME;


#define CHAOS_SYSPAR_FUNCS(NAME)				\
public:											\
CHAOS_PAR_SETFUNC(NAME)							\
CHAOS_PAR_GETFUNC(NAME)							\
data_t m_##NAME;


#define CHAOS_SYSPAR_FUNCS_I(NAME)				\
CHAOS_PAR_SETFUNC(NAME)							\
CHAOS_PAR_GETFUNC(NAME)							\
data_t m_##NAME;


#define CHAOS_SYS_CALLBACKS(NAME)				\
public:											\
void get_##NAME(t_float &f)						\
{												\
	f = m_system.get_##NAME();					\
}												\
void set_##NAME(t_float &f)						\
{												\
	m_system.set_##NAME(f);					\
}												\
FLEXT_CALLVAR_F(get_##NAME, set_##NAME);

 
#define CHAOS_SYS_CALLBACKS_I(NAME)				\
public:											\
void get_##NAME(int &i)							\
{												\
	i = m_system.get_##NAME();					\
}												\
void set_##NAME(int &i)							\
{												\
	m_system.set_##NAME(i);					\
}												\
FLEXT_CALLVAR_I(get_##NAME, set_##NAME);


#define CHAOS_SYS_ATTRIBUTE(NAME)					\
FLEXT_ADDATTR_VAR(#NAME,get_##NAME, set_##NAME);


#define CHAOS_INIT(NAME, VALUE)					\
set_##NAME(VALUE);

#define CHAOS_SYS_INIT(NAME, VALUE, INDEX)					\
set_##NAME(VALUE);											\
t_atom atom_##NAME;											\
flext::SetSymbol(atom_##NAME, flext::MakeSymbol(#NAME));	\
System.Append(atom_##NAME);									\
attr_ind[flext::MakeSymbol(#NAME)] = INDEX;

#define CHAOS_SYS_INIT_HIDDEN(NAME, VALUE, INDEX)	\
set_##NAME(VALUE);

#define CHAOS_PAR_INIT(NAME, VALUE)							\
set_##NAME(VALUE);											\
t_atom atom_##NAME;											\
flext::SetSymbol(atom_##NAME, flext::MakeSymbol(#NAME));	\
Parameter.Append(atom_##NAME);


#define CHAOS_PARAMETER(NAME) m_##NAME




/* macros for class generation */
#define CHAOS_DSP_CLASS(CLASSNAME,CLASSNAME_UC)				\
class CLASSNAME##_dsp:										\
	public chaos_dsp<CLASSNAME>								\
{															\
	CHAOS_DSP_INIT(CLASSNAME, CLASSNAME_UC##_ATTRIBUTES);	\
	CLASSNAME_UC##_CALLBACKS;								\
};															\
FLEXT_LIB_DSP_V(#CLASSNAME"~", CLASSNAME##_dsp);


#define CHAOS_DSP_CLASS_NAME(CLASSNAME,CLASSNAME_UC, NAME)	\
class CLASSNAME##_dsp:										\
	public chaos_dsp<CLASSNAME>								\
{															\
	CHAOS_DSP_INIT(CLASSNAME, CLASSNAME_UC##_ATTRIBUTES);	\
	CLASSNAME_UC##_CALLBACKS;								\
};															\
FLEXT_LIB_DSP_V(#NAME, CLASSNAME##_dsp);


#define CHAOS_MSG_CLASS(CLASSNAME,CLASSNAME_UC)				\
class CLASSNAME##_msg:										\
	public chaos_msg<CLASSNAME>								\
{															\
	CHAOS_MSG_INIT(CLASSNAME, CLASSNAME_UC##_ATTRIBUTES);	\
	CLASSNAME_UC##_CALLBACKS;								\
};															\
FLEXT_LIB_V(#CLASSNAME, CLASSNAME##_msg);


#define CHAOS_MSG_CLASS_NAME(CLASSNAME,CLASSNAME_UC, NAME)	\
class CLASSNAME##_msg:										\
	public chaos_msg<CLASSNAME>								\
{															\
	CHAOS_MSG_INIT(CLASSNAME, CLASSNAME_UC##_ATTRIBUTES);	\
	CLASSNAME_UC##_CALLBACKS;								\
};															\
FLEXT_LIB_V(#NAME, CLASSNAME##_msg);

#define CHAOS_SEARCH_CLASS(CLASSNAME,CLASSNAME_UC)				\
class CLASSNAME##_search:										\
	public chaos_search<CLASSNAME>								\
{																\
	CHAOS_SEARCH_INIT(CLASSNAME, CLASSNAME_UC##_ATTRIBUTES);	\
	CLASSNAME_UC##_CALLBACKS;									\
};																\
FLEXT_LIB_V(#CLASSNAME"_search", CLASSNAME##_search);


#define CHAOS_SEARCH_CLASS_NAME(CLASSNAME,CLASSNAME_UC, NAME)	\
class CLASSNAME##_msg:											\
	public chaos_search<CLASSNAME>								\
{																\
	CHAOS_SEARCH_INIT(CLASSNAME, CLASSNAME_UC##_ATTRIBUTES);	\
	CLASSNAME_UC##_CALLBACKS;									\
};																\
FLEXT_LIB_V(#NAME, CLASSNAME##_msg);



#define CHAOS_ADD(NAME)							\
FLEXT_DSP_SETUP(NAME##_dsp);					\
FLEXT_SETUP(NAME##_msg);						\
FLEXT_SETUP(NAME##_search);

#define __chaos_defs_hpp
#endif /* __chaos_defs_hpp */
