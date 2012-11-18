/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "ops_cmp.h"
#include "opdefs.h"
#include "util.h"

// --------------------------------------------------------------

// --------------------------------------------------------------


/*! \class vasp_qmin
	\remark \b vasp.min?
	\brief Get minimum sample value
	\since 0.0.2
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet float - minimum sample value

	\remark Returns 0 for a vasp with 0 frames
*/
class vasp_qmin:
	public vasp_op
{
	FLEXT_HEADER(vasp_qmin,vasp_op)

public:
	vasp_qmin() { 
		AddInAnything(); 
		AddOutList(); 
	}

	virtual F do_opt(OpParam &p,CVasp &v) 
	{ 
		p.norm.minmax = std::numeric_limits<R>::max();
		Vasp *ret = VaspOp::m_qmin(p,v); 
		delete ret;
		return p.norm.minmax == std::numeric_limits<R>::max()?0:p.norm.minmax;
	}
		
	virtual V m_bang() 
	{ 
		if(!ref.Ok()) return;

		AtomList ret(ref.Vectors());
		OpParam p(thisName(),0);	
		
		for(I i = 0; i < ret.Count(); ++i) {
			Vasp vasp(ref.Frames(),ref.Vector(i));
			CVasp ref(vasp);
			F v = do_opt(p,ref); 
			SetFloat(ret[i],v);
		}
		ToOutList(0,ret);
	}

	virtual V m_help() { post("%s - Get a vasp's minimum sample value",thisName()); }
};

VASP_LIB("vasp.min?",vasp_qmin)


/*! \class vasp_qamin
	\remark \b vasp.amin?
	\brief Get minimum absolute sample value
	\since 0.0.2
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet float - minimum absolute sample value

	\todo Should we provide a cmdln default vasp?
	\todo Should we inhibit output for invalid vasps?
	\remark Returns 0 for a vasp with 0 frames
*/
class vasp_qamin:
	public vasp_qmin
{
	FLEXT_HEADER(vasp_qamin,vasp_qmin)
public:
	virtual F do_opt(OpParam &p,CVasp &v) 
	{ 
		p.norm.minmax = std::numeric_limits<R>::max();
		Vasp *ret = VaspOp::m_qmin(p,v); 
		delete ret;
		return p.norm.minmax == std::numeric_limits<R>::max()?0:p.norm.minmax;
	}
		
	virtual V m_help() { post("%s - Get a vasp's minimum absolute sample value",thisName()); }
};

VASP_LIB("vasp.amin?",vasp_qamin)



/*! \class vasp_qmax
	\remark \b vasp.max?
	\brief Get maximum sample value
	\since 0.0.2
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet float - maximum sample value

	\todo Should we provide a cmdln default vasp?
	\todo Should we inhibit output for invalid vasps?
	\remark Returns 0 for a vasp with 0 frames
*/
class vasp_qmax:
	public vasp_qmin
{
	FLEXT_HEADER(vasp_qmax,vasp_qmin)
public:
	virtual F do_opt(OpParam &p,CVasp &v) 
	{ 
		p.norm.minmax = -std::numeric_limits<R>::max();
		Vasp *ret = VaspOp::m_qmax(p,v); 
		delete ret;
		return p.norm.minmax == -std::numeric_limits<R>::max()?0:p.norm.minmax;
	}

	virtual V m_help() { post("%s - Get a vasp's maximum sample value",thisName()); }
};

VASP_LIB("vasp.max?",vasp_qmax)



/*! \class vasp_qamax
	\remark \b vasp.amax?
	\brief Get minimum absolute sample value
	\since 0.0.2
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet float - maximum absolute sample value

	\todo Should we provide a cmdln default vasp?
	\todo Should we inhibit output for invalid vasps?
	\remark Returns 0 for a vasp with 0 frames
*/
class vasp_qamax:
	public vasp_qmax
{
	FLEXT_HEADER(vasp_qamax,vasp_qmax)
public:
	virtual F do_opt(OpParam &p,CVasp &v) 
	{ 
		p.norm.minmax = 0;
		Vasp *ret = VaspOp::m_qamax(p,v); 
		delete ret;
		return p.norm.minmax;
	}

	virtual V m_help() { post("%s - Get a vasp's maximum absolute sample value",thisName()); }
};

VASP_LIB("vasp.amax?",vasp_qamax)




/*! \class vasp_qrmin
	\remark \b vasp.rmin?
	\brief Get minimum complex radius of samples
	\since 0.0.2
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet list - minimum radius value per complex vector pair

	\todo Should we provide a cmdln default vasp?
	\todo Should we inhibit output for invalid vasps?
	\remark Returns 0 for a vasp with 0 frames
*/
class vasp_qrmin:
	public vasp_op
{
	FLEXT_HEADER(vasp_qrmin,vasp_op)
public:
	vasp_qrmin() { 
		AddInAnything(); 
		AddOutList(); 
	}

	virtual F do_opt(OpParam &p,CVasp &v) 
	{ 
		p.norm.minmax = std::numeric_limits<R>::max();
		Vasp *ret = VaspOp::m_qrmin(p,v);
		delete ret; 
		return sqrt(p.norm.minmax == std::numeric_limits<R>::max()?0:p.norm.minmax);
	}
		
	virtual V m_bang() 
	{ 
		if(!ref.Ok()) return;

		AtomList ret(ref.Vectors()/2);
		OpParam p(thisName(),0);	
		
		for(I i = 0; i < ret.Count(); ++i) {
			Vasp vasp(ref.Frames(),ref.Vector(i*2));
			vasp.AddVector(ref.Vector(i*2+1));
			CVasp ref(vasp);
			F v = do_opt(p,ref); 
			SetFloat(ret[i],v);
		}
		
		if(ref.Vectors()%2) {
			post("%s - omitting dangling vector of complex pairs",thisName());
		}
		
		ToOutList(0,ret);
	}

	virtual V m_help() { post("%s - Get a vasp's minimum complex radius",thisName()); }
};

VASP_LIB("vasp.rmin?",vasp_qrmin)



/*! \class vasp_qrmax
	\remark \b vasp.rmax?
	\brief Get maximum complex radius of samples
	\since 0.0.2
	\param inlet vasp - is stored and output triggered
	\param inlet bang - triggers output
	\param inlet set - vasp to be stored 
	\retval outlet float - maximum radius value per complex vector pair

	\todo Should we provide a cmdln default vasp?
	\todo Should we inhibit output for invalid vasps?
	\remark Returns 0 for a vasp with 0 frames
*/
class vasp_qrmax:
	public vasp_qrmin
{
	FLEXT_HEADER(vasp_qrmax,vasp_qrmin)
public:
	virtual F do_opt(OpParam &p,CVasp &v) 
	{ 
		p.norm.minmax = 0;
		Vasp *ret = VaspOp::m_qrmax(p,v); 
		delete ret;
		return sqrt(p.norm.minmax);
	}

	virtual V m_help() { post("%s - Get a vasp's maximum complex radius",thisName()); }
};

VASP_LIB("vasp.rmax?",vasp_qrmax)




