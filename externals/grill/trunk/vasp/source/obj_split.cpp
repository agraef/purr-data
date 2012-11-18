/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"
#include "util.h"


/*! \class vasp_split
	\remark \b vasp.split
	\brief Splits a vasp into a number of vectors and the remainder.
	\since 0.0.1
	\param cmdln.1 int - number of vectors to split vasp into (excl. one for the remainder vectors)
	\param inlet.1 vasp - is stored and triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
	\retval outlet.n vasp - vector of stored vasp
	\retval outlet.+ vasp - remainder of stored vasp

	\note if there is no remainder outputs a bang
*/
class vasp_split:
	public vasp_op
{
	FLEXT_HEADER(vasp_split,vasp_op)

public:
	vasp_split(I argc,const t_atom *argv)
	{
		I cnt = -1;
		if(argc) {
			if(CanbeInt(argv[0])) cnt = GetAInt(argv[0]);
			if(cnt <= 1) {
				post("%s - integer argument invalid: set to 2",thisName());
				cnt = 2;
			}
		}
		else cnt = 2;

		AddInAnything();
		AddOutAnything(cnt+1);
	}

	virtual V m_bang() 
	{ 
		if(!ref.Ok()) {
			post("%s - Invalid vasp!",thisName());
			return;
		}

		I outs = CntOut()-1,rem = ref.Vectors()-outs;
		for(I i = min(outs,ref.Vectors())-1; i >= 0; --i) {
			Vasp v(ref.Frames(),ref.Vector(i));
			ToOutVasp(i,v);
		}
		if(rem > 0) {
			Vasp v(ref.Frames(),ref.Vector(outs));
			for(I i = 1; i < rem; ++i) v.AddVector(ref.Vector(outs+i));
			ToOutVasp(outs,v);
		}
		else
			ToOutBang(outs);
	}

	virtual V m_help() { post("%s - Split a vasp into its vectors",thisName()); }
};

VASP_LIB_V("vasp.split",vasp_split)


/*! \class vasp_join
	\remark \b vasp.join
	\brief Joins several vasps into one.
	\since 0.0.1
	\param cmdln.1 int - number of vasp slots
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
	\param inlet.1 reset - clears slots
	\param inlet.+n vasp - is stored in this slot 
	\retval outlet vasp - joined vasp

	The several vectors of the several vasps are all joined into one vasp.

	\note On different vasp frame count the minmum frame count is taken.
	\note The latest vector input to a slot is taken for the resulting vasp
*/
class vasp_join:
	public vasp_tx
{
	FLEXT_HEADER_S(vasp_join,vasp_tx,Setup)

public:
	vasp_join(I argc,const t_atom *argv):
		cnt(-1),vi(NULL)
	{
		if(argc) {
			if(CanbeInt(argv[0])) cnt = GetAInt(argv[0]);
			if(cnt <= 1) {
				post("%s - integer argument invalid: set to 2",thisName());
				cnt = 2;
			}
		}
		else cnt = 2;

		vi = new Vasp *[cnt-1];
		for(I i = 0; i < cnt-1; ++i) vi[i] = NULL;

		AddInAnything(cnt);
		AddOutAnything();
	}

	~vasp_join()	{ if(vi) delete[] vi; }

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
	}

	virtual Vasp *x_work() { 
		CVasp *ret = new CVasp(ref);
		for(I i = 0; i < cnt-1; ++i) if(vi[i]) *ret += *vi[i];
		return ret;
	}

	V m_reset() 
	{ 
		ref.Clear();
		for(I i = 0; i < cnt-1; ++i) if(vi[i]) { delete vi[i]; vi[i] = NULL; }
	}

	virtual bool m_method_(I inlet,const t_symbol *s,I argc,const t_atom *argv)
	{
		if(inlet > 0 && s == sym_vasp) {
			if(vi[inlet-1]) delete vi[inlet-1];
			vi[inlet-1] = new Vasp(argc,argv);
			return true;
		}
		else
			return vasp_tx::m_method_(inlet,s,argc,argv);
	}

	virtual V m_help() { post("%s - Join several vasps into one",thisName()); }
private:
	I cnt;
	Vasp **vi;

	FLEXT_CALLBACK(m_reset)
};

VASP_LIB_V("vasp.join",vasp_join)



/*! \class vasp_spit
	\remark \b vasp.spit
	\brief Spit out vectors of a vasp consecutively.
	\since 0.0.1
	\param inlet.1 vasp - is stored and triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - vasp to be stored 
	\retval outlet.n vasp - vectors of stored vasp
	\retval outlet.+ bang - triggered after last spit 
*/
class vasp_spit:
	public vasp_op
{
	FLEXT_HEADER(vasp_spit,vasp_op)

public:
	vasp_spit(I argc,const t_atom *argv)
	{
		I n = 1;
		if(argc >= 1) n = GetAInt(argv[0]);
		if(n < 1) {
			post("%s - illegal outlet count (%i) -> set to 1",thisName(),n);
			n = 1;
		}

		AddInAnything();
		AddOutAnything(n);
		AddOutBang();
	}

	virtual V m_bang() 
	{ 
		if(!ref.Ok()) {
			post("%s - Invalid vasp!",thisName());
			return;
		}

		I outs = CntOut()-1,rem = ref.Vectors();
		for(I vi = 0; rem;) {
			I r = min(rem,outs);
			for(I i = 0; i < r; ++i) {
				Vasp v(ref.Frames(),ref.Vector(vi+i));
				ToOutVasp(outs-1-i,v);
			}
			vi += r;
			rem -= r;
		}
		ToOutBang(outs);
	}

	virtual V m_help() { post("%s - Spit out vectors of a vasp",thisName()); }
};

VASP_LIB_V("vasp.spit",vasp_spit)


/*! \class vasp_gather
	\remark \b vasp.gather
	\brief Gathers several consecutive vasps into one.
	\since 0.0.1
	\param cmdln.1 int - number of vasp slots
	\param inlet.1 vasp - is stored and output triggered
	\param inlet.1 bang - triggers output
	\param inlet.1 set - sets result vasp 
	\param inlet.1 reset - clears result
	\param inlet.2 vasp - add to result vasp
	\retval outlet vasp - gathered vasp

	The several incoming vectors are all gathered into one vasp.

	\note On different vasp frame count the minimum frame count is taken.
*/
class vasp_gather:
	public vasp_tx
{
	FLEXT_HEADER_S(vasp_gather,vasp_tx,Setup)

public:
	vasp_gather(I argc,const t_atom *argv)
	{
		cnt = 0;
		if(argc >= 1) cnt = GetAInt(argv[0]);
		if(cnt < 0) {
			post("%s - illegal count (%i) -> set to 0 (triggered mode)",thisName(),cnt);
			cnt = 0;
		}
		rem = cnt;

		AddInAnything(2);
		AddOutAnything();
	}

	static V Setup(t_classid c)
	{
		FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
		FLEXT_CADDMETHOD_(c,1,"vasp",m_add);
	}

	virtual Vasp *x_work() 
	{ 
		CVasp *ret = new CVasp(ref); 
		*ret += cdst;
		m_reset(); 
		return ret; 
	}

	V m_reset() { ref.Clear(); cdst.Clear(); rem = cnt; }

	virtual I m_set(I argc,const t_atom *argv) { rem = cnt; return vasp_tx::m_set(argc,argv); }

	V m_add(I argc,const t_atom *argv) 
	{ 
		cdst += Vasp(argc,argv);
		if(cnt && !--rem) m_bang();
	}

	virtual V m_help() { post("%s - Gather several vasps into one",thisName()); }
private:
	I cnt,rem;
	CVasp cdst;

	FLEXT_CALLBACK(m_reset)
	FLEXT_CALLBACK_V(m_add)
};

VASP_LIB_V("vasp.gather",vasp_gather)



