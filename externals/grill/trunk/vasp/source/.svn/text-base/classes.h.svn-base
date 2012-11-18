/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_CLASSES_H
#define __VASP_CLASSES_H

#include "vasp.h"
#include "arg.h"


class vasp_base:
	public flext_base
{
	FLEXT_HEADER_S(vasp_base,flext_base,Setup)

public:
	enum xs_unit {
		xsu__ = -1,  // don't change
		xsu_sample = 0,xsu_buffer,xsu_ms,xsu_s
	};	

	static const t_symbol *sym_vasp;
	static const t_symbol *sym_env;
	static const t_symbol *sym_double;
	static const t_symbol *sym_complex;
	static const t_symbol *sym_vector;
	static const t_symbol *sym_radio;

protected:
	vasp_base();
	virtual ~vasp_base();

	virtual V m_radio(I argc,const t_atom *argv);  // commands for all

    virtual V m_help() = 0;

/*
	V m_argchk(BL chk);  // precheck argument on arrival
	V m_loglvl(I lvl);  // noise level of log messages
	V m_unit(xs_unit u);  // unit command
*/
	BL refresh;  // immediate graphics refresh?
	BL argchk;   // pre-operation argument feasibility check
    BL deferred;    // queue outgoing vasp messages
	xs_unit unit;  // time units
	I loglvl;	// noise level for log messages

	friend class Vasp;

	BL ToOutVasp(I outlet,Vasp &v);

private:
	static V Setup(t_classid);

	FLEXT_CALLBACK_V(m_radio)
	FLEXT_CALLBACK(m_help)

	FLEXT_ATTRVAR_B(argchk)
	FLEXT_ATTRVAR_B(deferred)
	FLEXT_ATTRVAR_I(loglvl)
	FLEXT_ATTRVAR_E(unit,xs_unit)
};


class vasp_op:
	public vasp_base
{
	FLEXT_HEADER_S(vasp_op,vasp_base,Setup)

protected:
	vasp_op(BL withto = false);

	virtual V m_dobang();						// bang method

	virtual V m_vasp(I argc,const t_atom *argv); // trigger
	virtual I m_set(I argc,const t_atom *argv);  // non trigger
	virtual V m_to(I argc,const t_atom *argv); // set destination
//	V m_detach(BL thr);		// detached thread
//	virtual V m_prior(I dp);  // thread priority +-
	virtual V m_stop();				// stop working

	virtual V m_update(I argc = 0,const t_atom *argv = NULL);  // graphics update

	V m_setupd(const AtomList &l) { m_update(l.Count(),l.Atoms()); }
	V m_getupd(AtomList &l) { l(1); SetBool(l[0],refresh); }

	// destination vasp
	Vasp ref,dst;

	V m_setref(const AtomList &l) { m_set(l.Count(),l.Atoms()); }
	V m_getref(AtomList &l) { ref.MakeList(l); }
	V m_setto(const AtomList &l) { m_to(l.Count(),l.Atoms()); }
	V m_getto(AtomList &l) { dst.MakeList(l); }

	FLEXT_CALLBACK_V(m_to)

	FLEXT_CALLBACK(m_dobang)
#ifdef FLEXT_THREADS
	FLEXT_THREAD(m_bang)

	ThrMutex runmtx;
	V Lock() { runmtx.Lock(); }
	V Unlock() { runmtx.Unlock(); }

//	thrid_t thrid; 
#else
	FLEXT_CALLBACK(m_bang)

	V Lock() {}
	V Unlock() {}
#endif
	BL detach;	// detached operation?
	I prior;  // thread priority

	FLEXT_CALLBACK_V(m_vasp)
	FLEXT_CALLBACK_V(m_set)

	FLEXT_CALLVAR_V(m_getref,m_setref)
	FLEXT_CALLVAR_V(m_getto,m_setto)
	
	FLEXT_CALLBACK(m_stop)

	FLEXT_CALLVAR_V(m_getupd,m_setupd)
	FLEXT_ATTRVAR_B(detach)
	FLEXT_ATTRVAR_I(prior)

private:
	static V Setup(t_classid);

	virtual V m_bang() = 0;						// do! and output current Vasp
};



class vasp_tx:
	public vasp_op
{
	FLEXT_HEADER(vasp_tx,vasp_op)

protected:
	vasp_tx(BL withto = false);

	virtual V m_bang();						// do! and output current Vasp

	virtual Vasp *x_work() = 0;
};




#define VASP_SETUP(op) FLEXT_SETUP(vasp_##op);  



// base class for unary operations

class vasp_unop:
	public vasp_tx
{
	FLEXT_HEADER(vasp_unop,vasp_tx)

protected:
	vasp_unop(BL withto = false,UL outcode = 0);

	virtual Vasp *x_work();
	virtual Vasp *tx_work();
};


// base class for binary operations

class vasp_binop:
	public vasp_tx
{
	FLEXT_HEADER_S(vasp_binop,vasp_tx,Setup)

protected:
	vasp_binop(I argc,const t_atom *argv,const Argument &def = Argument(),BL withto = false,UL outcode = 0);

	// assignment functions
	virtual V a_list(I argc,const t_atom *argv); 
	/*virtual*/ V a_vasp(I argc,const t_atom *argv);
	/*virtual*/ V a_env(I argc,const t_atom *argv);
	/*virtual*/ V a_float(F f); 
	/*virtual*/ V a_int(I f); 
	/*virtual*/ V a_double(I argc,const t_atom *argv); 
	/*virtual*/ V a_complex(I argc,const t_atom *argv); 
	/*virtual*/ V a_vector(I argc,const t_atom *argv); 

	V a_radio(I,const t_atom *) {}

	virtual Vasp *x_work();
	virtual Vasp *tx_work(const Argument &arg);

	Argument arg;

	V m_setarg(const AtomList &l) { a_list(l.Count(),l.Atoms()); }
	V m_getarg(AtomList &l) { arg.MakeList(l); }

private:
	static V Setup(t_classid);

	FLEXT_CALLBACK_V(a_list)
	FLEXT_CALLBACK_V(a_vasp)
	FLEXT_CALLBACK_V(a_env)
	FLEXT_CALLBACK_1(a_float,F)
	FLEXT_CALLBACK_1(a_int,I)
	FLEXT_CALLBACK_V(a_double)
	FLEXT_CALLBACK_V(a_complex)
	FLEXT_CALLBACK_V(a_vector)
	FLEXT_CALLBACK_V(a_radio)

	FLEXT_CALLVAR_V(m_getarg,m_setarg)
};


// base class for non-parsed (list) arguments

class vasp_anyop:
	public vasp_tx
{
	FLEXT_HEADER_S(vasp_anyop,vasp_tx,Setup)

protected:
	vasp_anyop(I argc,const t_atom *argv,const Argument &def = Argument(),BL withto = false,UL outcode = 0);

	// assignment functions
	virtual V a_list(I argc,const t_atom *argv); 

	V a_radio(I,const t_atom *) {}

	virtual Vasp *x_work();
	virtual Vasp *tx_work(const Argument &arg);

	Argument arg;

	V m_setarg(const AtomList &l) { a_list(l.Count(),l.Atoms()); }
	V m_getarg(AtomList &l) { arg.MakeList(l); }

private:
	static V Setup(t_classid);

	FLEXT_CALLBACK_V(a_list)
	FLEXT_CALLBACK_V(a_radio)

	FLEXT_CALLVAR_V(m_getarg,m_setarg)
};



#define VASP_UNARY(name,op,to,help)												\
class vasp_##op:																\
	public vasp_unop															\
{																				\
	FLEXT_HEADER(vasp_##op,vasp_unop)											\
public:																			\
	vasp_##op(): vasp_unop(to) {}												\
protected:																		\
	virtual Vasp *tx_work()														\
	{																			\
		OpParam p(thisName(),0);												\
		CVasp cdst(dst),cref(ref);												\
		return VaspOp::m_##op(p,cref,&cdst);									\
	}																			\
	virtual V m_help() { post("%s - " help,thisName()); }						\
};																				\
    FLEXT_LIB(name ", vasp_" #op,vasp_##op)												


#define VASP_BINARY(name,op,to,def,help)										\
class vasp_##op:																\
	public vasp_binop															\
{																				\
	FLEXT_HEADER(vasp_##op,vasp_binop)											\
public:																			\
	vasp_##op(I argc,const t_atom *argv): vasp_binop(argc,argv,def,to) {}		\
protected:																		\
	virtual Vasp *tx_work(const Argument &arg)									\
	{																			\
		OpParam p(thisName(),1);												\
		CVasp cdst(dst),cref(ref);												\
		return VaspOp::m_##op(p,cref,arg,&cdst);								\
	}																			\
	virtual V m_help() { post("%s - " help,thisName()); }						\
};																				\
FLEXT_LIB_V(name ", vasp_" #op,vasp_##op)												


#define VASP_ANYOP(name,op,args,to,def,help)									\
class vasp_##op:																\
	public vasp_anyop															\
{																				\
	FLEXT_HEADER(vasp_##op,vasp_anyop)											\
public:																			\
	vasp_##op(I argc,const t_atom *argv): vasp_anyop(argc,argv,def,to) {}		\
protected:																		\
	virtual Vasp *tx_work(const Argument &arg)									\
	{																			\
		OpParam p(thisName(),args);												\
		CVasp cdst(dst),cref(ref);												\
		return VaspOp::m_##op(p,cref,arg,&cdst);								\
	}																			\
	virtual V m_help() { post("%s - " help,thisName()); }						\
};																				\
FLEXT_LIB_V(name ", vasp_" #op,vasp_##op)												

#define VASP_LIB(name,cl) FLEXT_LIB(name ", " #cl,cl);  
#define VASP_LIB_V(name,cl) FLEXT_LIB_V(name ", " #cl,cl);  


#define VASP__SETUP(op) FLEXT_SETUP(vasp_##op);  

#endif
