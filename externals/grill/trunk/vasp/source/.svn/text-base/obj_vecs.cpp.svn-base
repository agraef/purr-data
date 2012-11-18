/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"
#include "util.h"


/*! \class vasp_vector
	\remark \b vasp.vector
	\brief Gets indexed vector of a vasp.
	\since 0.0.1
	\param cmdln.1 int - index of vasp vector
	\param inlet vasp - is stored and indexed vasp vector output
	\param inlet bang - triggers indexed vasp vector output
	\param inlet set - vasp to be stored (and not immediately output)
	\retval outlet.1 vasp - single indexed vector of vasp
	\retval outlet.2 vasp - remainder of vasp

	\note Outputs only on valid index
	\todo Output remainder as vasp.
*/
class vasp_vector:
	public vasp_tx
{
	FLEXT_HEADER_S(vasp_vector,vasp_tx,Setup)

public:
	vasp_vector(I argc,const t_atom *argv):
		ix(0)
	{
		if(argc >= 1 && CanbeInt(argv[0]))
			ix = GetAInt(argv[0]);
		else if(argc)
			post("%s - Index argument invalid -> set to 0",thisName());

		AddInAnything(2);
		AddOutAnything();
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD(c,1,m_ix);
		FLEXT_CADDATTR_VAR(c,"index",ix,m_ix);
	}

	V m_ix(I i) { ix = i; }

	virtual Vasp *x_work() { return ix < ref.Vectors()?new Vasp(ref.Frames(),ref.Vector(ix)):NULL; }

	virtual V m_help() { post("%s - Get one vector of a vasp",thisName()); }

protected:
	I ix;

private:
	FLEXT_CALLBACK_I(m_ix);
	FLEXT_CALLSET_I(m_ix);
	FLEXT_ATTRGET_I(ix);
};

VASP_LIB_V("vasp.vector vasp.n",vasp_vector)



/*! \class vasp_qn 
	\remark \b vasp.n?
	\brief Gets number of vector of a vasp.
	\since 0.0.1
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet int - number of vectors in stored vasp

	\note Outputs 0 if vasp is undefined or invalid.

	\todo Should we disable output with invalid vasp?
*/
class vasp_qvectors:
	public vasp_op
{
	FLEXT_HEADER(vasp_qvectors,vasp_op)

public:
	vasp_qvectors()
	{
		AddInAnything();
//		AddOutAnything();
		AddOutInt();
	}

	virtual V m_bang() 
	{ 
//		ToOutVasp(0,ref); 
		ToOutInt(0,ref.Ok()?ref.Vectors():0); 
	}

	virtual V m_help() { post("%s - Get number of vectors of a vasp",thisName()); }
};

VASP_LIB("vasp.vectors? vasp.n?",vasp_qvectors)



