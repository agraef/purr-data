/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"
#include "util.h"
#include "buflib.h"


/*! \class vasp_channel
	\remark \b vasp.channel
	\brief Sets channel index of vasp.
	\since 0.0.8
	\param cmdln.1 int - index of vasp vector
	\param inlet vasp - is stored and indexed vasp vector output
	\param inlet bang - triggers indexed vasp vector output
	\param inlet set - vasp to be stored (and not immediately output)
	\retval outlet modified vasp
*/
class vasp_channel:
	public vasp_tx
{
	FLEXT_HEADER_S(vasp_channel,vasp_tx,Setup)

public:
	vasp_channel(I argc,const t_atom *argv):
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

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref);
		ret->Channel(ix);
		return ret; 
	}

	virtual V m_help() { post("%s - Set channel index of vectors in vasp",thisName()); }

protected:
	I ix;

private:
	FLEXT_CALLBACK_I(m_ix);
	FLEXT_CALLSET_I(m_ix);
	FLEXT_ATTRGET_I(ix);
};

VASP_LIB_V("vasp.channel vasp.c",vasp_channel)



/*! \class vasp_qc 
	\remark \b vasp.c?
	\brief Gets channel index of a vasp.
	\since 0.0.8
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet int - channel index of stored vasp

	\note Always returns index of 0th vasp
	\note No output for invalid vasp?
*/
class vasp_qchannel:
	public vasp_op
{
	FLEXT_HEADER(vasp_qchannel,vasp_op)

public:
	vasp_qchannel()
	{
		AddInAnything();
		AddOutInt();
	}

	virtual V m_bang() 
	{ 
		if(ref.Ok()) {
			if(ref.Vectors() > 1)
				post("%s - more vectors in vasp, only considering first",thisName());
			
			ToOutInt(0,ref.Vector(0).Channel()); 
		}
		else
			post("%s - Invalid vasp, no output",thisName());
	}

	virtual V m_help() { post("%s - Get channel index of 0th vector in vasp",thisName()); }
};

VASP_LIB("vasp.channel? vasp.c?",vasp_qchannel)


/*! \class vasp_qchannels
	\remark \b vasp.channels?
	\brief Gets number of channels of a vasp.
	\since 0.1.3
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet int - channels of stored vasp

	\note No output for invalid vasp?
*/
class vasp_qchannels:
	public vasp_op
{
	FLEXT_HEADER(vasp_qchannels,vasp_op)

public:
	vasp_qchannels()
	{
		AddInAnything();
		AddOutInt();
	}

	virtual V m_bang() 
	{ 
		if(ref.Ok()) {
			if(ref.Vectors() > 1)
				post("%s - more vectors in vasp, only considering first",thisName());
			
			VBuffer *buf = BufLib::Get(ref.Vector(0).Symbol());
			ToOutInt(0,buf->Channels()); 
			delete buf;
		}
		else
			post("%s - Invalid vasp, no output",thisName());
	}

	virtual V m_help() { post("%s - Get channel index of 0th vector in vasp",thisName()); }
};

VASP_LIB("vasp.channels?",vasp_qchannels)



