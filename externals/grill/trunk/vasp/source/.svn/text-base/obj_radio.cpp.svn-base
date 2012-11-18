/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

/*! \file obj_radio.cpp
	\brief objects for radio messages.
*/

#include "main.h"
#include "classes.h"


/*! \class vasp_radio
	\remark \b vasp.radio
	\brief Lets only radio messages pass through.
	\since 0.0.6
	\param inlet.1 * - any message
	\retval outlet.1 radio messages
	\retval outlet.2 other messages
*/
class vasp_radio:
	public flext_base
{
	FLEXT_HEADER_S(vasp_radio,flext_base,Setup)

public:

	vasp_radio()
	{
		AddInAnything();
		AddOutAnything(2);
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD(c,0,m_any);
	}

	virtual V m_any(const t_symbol *s,I argc,const t_atom *argv);

	virtual V m_help() { post("%s - split into radio and non-radio messages",thisName()); }
private:
	FLEXT_CALLBACK_A(m_any);
};

VASP_LIB("vasp.radio",vasp_radio)


V vasp_radio::m_any(const t_symbol *s,I argc,const t_atom *argv) 
{
	ToOutAnything(s == vasp_base::sym_radio?0:1,s,argc,argv);
}

