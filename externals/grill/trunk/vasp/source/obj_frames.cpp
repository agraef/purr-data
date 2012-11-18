/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"
#include "util.h"


/*! \class vasp_frames
	\remark \b vasp.frames
	\brief Sets frame count of vasp.
	\since 0.0.1
	\param cmdln.1 [_time=0] - frame count in time units
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 _time - frame count in time units
	\retval outlet vasp - modified vasp

	\todo Implement unit processing.
*/
class vasp_frames:
	public vasp_tx
{
	FLEXT_HEADER_S(vasp_frames,vasp_tx,Setup)

public:
	vasp_frames(I argc,const t_atom *argv,BL abs = true):
		frms(0),setf(false)
	{
		if(argc && CanbeFloat(argv[0]))
			m_arg(GetAFloat(argv[0]));
		else if(argc) {
			post("%s - argument invalid -> ignored",thisName());
		}

		AddInAnything();
		AddInFloat();
		AddOutAnything();

		if(abs) FLEXT_ADDATTR_VAR("frames",frms,m_arg);
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD(c,1,m_arg);
	}

	virtual V m_arg(F f) 
	{ 
		frms = (I)f; //! \todo unit processing
		setf = true; 
	}

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref); 
		if(setf) ret->Frames(frms);
		return ret;
	}

	virtual V m_help() { post("%s - Set a vasp's frame count",thisName()); }
protected:
	I frms;
	BL setf;

private:
	FLEXT_CALLBACK_F(m_arg);
	FLEXT_CALLSET_I(m_arg);
	FLEXT_ATTRGET_I(frms);
};

VASP_LIB_V("vasp.frames vasp.f",vasp_frames)




/*! \class vasp_dframes
	\remark \b vasp.frames+
	\brief Sets frame count of vasp differentially.
	\since 0.0.1
	\param cmdln.1 [_time=0] - increase of frame count in time units
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 _time - increase of frame count in time units
	\retval outlet vasp - modified vasp

	\todo Implement unit processing.
*/
class vasp_dframes:
	public vasp_frames
{
	FLEXT_HEADER(vasp_dframes,vasp_frames)

public:
	vasp_dframes(I argc,const t_atom *argv): vasp_frames(argc,argv) {}

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref); 
		if(setf) ret->FramesD(frms);
		return ret;
	}

	virtual V m_help() { post("%s - Raise/lower a vasp's frame count",thisName()); }
};

VASP_LIB_V("vasp.frames+ vasp.f+",vasp_dframes)



/*! \class vasp_mframes
	\remark \b vasp.frames*
	\brief Sets frame count of vasp by a factor
	\since 0.0.6
	\param cmdln.1 [_number=1] - multiply of frame count 
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 _number - multiply of frame count 
	\retval outlet vasp - modified vasp
*/
class vasp_mframes:
	public vasp_frames
{
	FLEXT_HEADER_S(vasp_mframes,vasp_frames,Setup)

public:
	vasp_mframes(I argc,const t_atom *argv): 
		vasp_frames(argc,argv,false) 
	{
		if(argc && CanbeFloat(argv[0])) m_arg(GetAFloat(argv[0]));
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDATTR_VAR(c,"factor",factor,m_arg);
	}

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref); 
		if(setf) ret->FramesM(factor);
		return ret;
	}

	virtual V m_help() { post("%s - Multiply a vasp's frame count",thisName()); }
	
	virtual V m_arg(F f) 
	{ 
		factor = f; 
		setf = true; 
	}

protected:
	F factor;
	FLEXT_CALLSET_F(m_arg);
	FLEXT_ATTRGET_F(factor);
};

VASP_LIB_V("vasp.frames* vasp.f*",vasp_mframes)



/*! \class vasp_rframes
	\remark \b vasp.frames/
	\brief Sets frame count of vasp by a divisor
	\since 0.0.6
	\param cmdln.1 [_number=1] - multiply of frame count 
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
    \param inlet.2 _number - divisor of frame count 
	\retval outlet vasp - modified vasp
*/
class vasp_rframes:
	public vasp_mframes
{
	FLEXT_HEADER(vasp_rframes,vasp_mframes)

public:
	vasp_rframes(I argc,const t_atom *argv): vasp_mframes(argc,argv) {}

	virtual Vasp *x_work() 
	{ 
		Vasp *ret = new Vasp(ref); 
		if(setf) ret->FramesR(factor);
		return ret;
	}

	virtual V m_help() { post("%s - Divide a vasp's frame count",thisName()); }
};

VASP_LIB_V("vasp.frames/ vasp.f/",vasp_rframes)



/*! \class vasp_qframes
	\remark \b vasp.frames?
	\brief Get frame count in time units
	\since 0.0.1
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet _time - frame count of vasp in time units

	\note Outputs 0 if vasp is undefined or invalid

	\todo Implement unit processing
	\todo Should we provide a cmdln default vasp?
	\todo Should we inhibit output for invalid vasps?
*/
class vasp_qframes:
	public vasp_op
{
	FLEXT_HEADER(vasp_qframes,vasp_op)

public:

	vasp_qframes()
	{
		AddInAnything();
		AddOutInt();
	}

	virtual V m_bang() { ToOutInt(0,ref.ChkFrames()); }	//! \todo unit processing

	virtual V m_help() { post("%s - Get a vasp's frame count",thisName()); }
};

VASP_LIB("vasp.frames? vasp.f?",vasp_qframes)


