/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "classes.h"


///////////////////////////////////////////////////////////////////////////
// vasp_base class
///////////////////////////////////////////////////////////////////////////

const t_symbol *vasp_base::sym_radio;
const t_symbol *vasp_base::sym_vasp;
const t_symbol *vasp_base::sym_env;
const t_symbol *vasp_base::sym_double;
const t_symbol *vasp_base::sym_complex;
const t_symbol *vasp_base::sym_vector;

V vasp_base::Setup(t_classid c)
{
	sym_radio = MakeSymbol("radio");
	sym_vasp = MakeSymbol("vasp");
	sym_env = MakeSymbol("env");
	sym_double = MakeSymbol("double");
	sym_complex = MakeSymbol("complex");
	sym_vector = MakeSymbol("vector");

	FLEXT_CADDMETHOD_(c,0,"radio",m_radio);
	FLEXT_CADDMETHOD_(c,0,"help",m_help);

	FLEXT_CADDATTR_VAR1(c,"defer",deferred);

// LATER!
/*
	FLEXT_CADDATTR_VAR1(c,"argchk",argchk);
	FLEXT_CADDATTR_VAR1(c,"loglvl",loglvl);
	FLEXT_CADDATTR_VAR1_E(c,"unit",unit);
*/
}

vasp_base::vasp_base():
	refresh(false),argchk(false),deferred(false),
	unit(xsu_sample),loglvl(0)
{}

vasp_base::~vasp_base() {}


V vasp_base::m_radio(I argc,const t_atom *argv)
{
	if(argc > 0 && IsSymbol(argv[0])) {
		// send command to self!
		ToSelfAnything(0,GetSymbol(argv[0]),argc-1,argv+1);

		// send command to the next objects in line
		ToOutAnything(0,sym_radio,argc,argv);
	}
	else 
		post("%s - radio message invalid",thisName());
}

/*
V vasp_base::m_unit(xs_unit u) { unit = u; }
V vasp_base::m_argchk(BL chk) {	argchk = chk; }
V vasp_base::m_loglvl(I lvl) { loglvl = lvl; }
*/

BL vasp_base::ToOutVasp(I oix,Vasp &v) 
{
	AtomList *lst = v.MakeList(false);
	if(lst) {
        if(deferred)
		    ToQueueAnything(oix,sym_vasp,lst->Count(),lst->Atoms());
        else
		    ToOutAnything(oix,sym_vasp,lst->Count(),lst->Atoms());
		delete lst;
		return true;
	}
	else return false;
}

///////////////////////////////////////////////////////////////////////////
// vasp_op class
///////////////////////////////////////////////////////////////////////////

vasp_op::vasp_op(BL op)
	:detach(false),prior(-2)
#ifdef FLEXT_THREADS
//	,thrid(0)
#endif
{
	if(op) FLEXT_ADDATTR_VAR("to",m_getto,m_setto);
}

V vasp_op::Setup(t_classid c)
{
	FLEXT_CADDBANG(c,0,m_dobang);
	FLEXT_CADDMETHOD_(c,0,"vasp",m_vasp);
	FLEXT_CADDMETHOD_(c,0,"set",m_set);

	FLEXT_CADDATTR_VAR(c,"ref",m_getref,m_setref);

	FLEXT_CADDMETHOD_(c,0,"stop",m_stop);

	FLEXT_CADDATTR_VAR(c,"update",m_getupd,m_setupd);
	
	FLEXT_CADDATTR_VAR1(c,"detach",detach);
	FLEXT_CADDATTR_VAR1(c,"prior",prior);
}

V vasp_op::m_dobang()
{
#ifdef FLEXT_THREADS
	if(detach)
		FLEXT_CALLMETHOD(m_bang);
	else
#endif
		m_bang();
}

I vasp_op::m_set(I argc,const t_atom *argv)
{
	Vasp arg(argc,argv);

	if(argc && !arg.Ok()) {
		ref.Clear();
		post("%s - invalid vasp detected and ignored",thisName());
	}
	else {
		if(arg.Check())
			ref = arg;
		else {
			ref.Clear();
			post("%s - vasp reference is invalid",thisName());
		}
	}

	return 0; 
}

V vasp_op::m_vasp(I argc,const t_atom *argv)
{
	m_set(argc,argv);
	m_dobang();
}


V vasp_op::m_to(I argc,const t_atom *argv)
{
	Vasp to(argc,argv);

	if(argc && !to.Ok()) {
		// empty vasp
		dst.Clear();
	}
	else 
		dst = to;
}

V vasp_op::m_update(I argc,const t_atom *argv) 
{
	if(argc == 0) 
		ref.Refresh();
	else {
		if(CanbeInt(argv[0]))
			refresh = GetAInt(argv[0]) != 0;
		else 
			post("%s(update) - argument should be omitted or integer",thisName());
	}
}

V vasp_op::m_stop() {}

///////////////////////////////////////////////////////////////////////////
// vasp_tx class
///////////////////////////////////////////////////////////////////////////

vasp_tx::vasp_tx(BL to): vasp_op(to) {}

V vasp_tx::m_bang()
{
	// Thread has to wait until previous is finished
	Lock(); 

#ifdef FLEXT_THREADS
	if(prior && IsThreadRegistered()) RelPriority(prior);
#endif

	if(ref.Check()) 
	{
		Vasp *ret = x_work();
		if(ret) {
			if(!ToOutVasp(0,*ret))
				post("%s - empty list",thisName());
			if(refresh) ret->Refresh();
			delete ret;
		}
		else {
#ifdef FLEXT_DEBUG
			post("%s - no valid return",thisName());
#endif
		}
	}
	else {
		post("%s - no valid vasp to work with",thisName());
	}

#ifdef FLEXT_THREADS
//	thrid = 0; 
#endif

	Unlock();
}


///////////////////////////////////////////////////////////////////////////
// vasp_unop class
///////////////////////////////////////////////////////////////////////////

vasp_unop::vasp_unop(BL op,UL outcode):
	vasp_tx(op)
{
	AddInAnything();
	AddOutAnything(1);
	AddOutlets(outcode);
}

Vasp *vasp_unop::x_work() { return tx_work(); }

Vasp *vasp_unop::tx_work() 
{
	error("%s - no work method implemented",thisName());
	return NULL;
}

///////////////////////////////////////////////////////////////////////////
// vasp_binop class
///////////////////////////////////////////////////////////////////////////


vasp_binop::vasp_binop(I argc,const t_atom *argv,const Argument &def,BL op,UL outcode):
	vasp_tx(op)
{
	a_list(argc,argv);
	if(arg.IsNone() && !def.IsNone()) arg = def;

	AddInAnything(2);
	AddOutAnything(1);
	AddOutlets(outcode);
}

V vasp_binop::Setup(t_classid c)
{
	FLEXT_CADDMETHOD(c,1,a_list);
	FLEXT_CADDMETHOD_(c,1,"vasp",a_vasp);
	FLEXT_CADDMETHOD_(c,1,"env",a_env);
	FLEXT_CADDMETHOD_(c,1,"float",a_float);
	FLEXT_CADDMETHOD_(c,1,"double",a_double);
	FLEXT_CADDMETHOD_(c,1,"int",a_int);
	FLEXT_CADDMETHOD_(c,1,"complex",a_complex);
	FLEXT_CADDMETHOD_(c,1,"vector",a_vector);
	FLEXT_CADDMETHOD_(c,1,"radio",a_radio);

	FLEXT_CADDATTR_VAR(c,"arg",m_getarg,m_setarg);
}

V vasp_binop::a_list(I argc,const t_atom *argv) 
{ 
	if(argc) {
		arg.Parse(argc,argv);
		if(arg.IsNone()) 
			post("%s - list argument could not be evaluated (ignored)",thisName());
		else if(argchk) {
			// check argument feasibility
		}
	}
	else {
//		post("%s - Empty list argument (ignored)",thisName());
	}
}

V vasp_binop::a_vasp(I argc,const t_atom *argv) 
{ 
	Vasp *v = new Vasp(argc,argv);
	if(v->Ok()) {
		arg.SetVasp(v);
		if(argchk) {
			// check argument feasibility
		}
	}
	else {
		post("%s - invalid vasp argument (ignored)",thisName());
		delete v;
	}
}

V vasp_binop::a_env(I argc,const t_atom *argv) 
{ 
	Env *bp = new Env(argc,argv);
	if(bp->Ok()) {
		arg.SetEnv(bp);
		if(argchk) {
			// check argument feasibility
		}
	}
	else {
		post("%s - invalid env argument (ignored)",thisName());
		delete bp;
	}
}

V vasp_binop::a_float(F v) { arg.SetR(v); }

V vasp_binop::a_double(I argc,const t_atom *argv) 
{ 
	if(
		(argc == 1 && CanbeFloat(argv[0])) || 
		(argc == 2 && CanbeFloat(argv[0]) && CanbeFloat(argv[1]))
	) {
		arg.SetR((D)GetAFloat(argv[0])+(D)GetAFloat(argv[1]));
		if(argchk) {
			// check argument feasibility
		}
	}
	else 
		post("%s - invalid double argument (ignored)",thisName());
}

V vasp_binop::a_int(I v) { arg.SetI(v); }

V vasp_binop::a_complex(I argc,const t_atom *argv) 
{ 
	if(
		(argc == 1 && CanbeFloat(argv[0])) || 
		(argc == 2 && CanbeFloat(argv[0]) && CanbeFloat(argv[1]))
	) {
		arg.SetCX(GetAFloat(argv[0]),GetAFloat(argv[1]));
		if(argchk) {
			// check argument feasibility
		}
	}
	else 
		post("%s - invalid complex argument (ignored)",thisName());
}

V vasp_binop::a_vector(I argc,const t_atom *argv)
{
	error("%s - vector type not implemented",thisName());
}


Vasp *vasp_binop::x_work() { return tx_work(arg); }

Vasp *vasp_binop::tx_work(const Argument &arg) 
{
	error("%s - no work method implemented",thisName());
	return NULL;
}


///////////////////////////////////////////////////////////////////////////
// vasp_anyop class
///////////////////////////////////////////////////////////////////////////


vasp_anyop::vasp_anyop(I argc,const t_atom *argv,const Argument &def,BL op,UL outcode):
	vasp_tx(op)
{
	a_list(argc,argv);
	if(arg.IsNone() && !def.IsNone()) arg = def;

	AddInAnything(2);
	AddOutAnything(1);
	AddOutlets(outcode);
}

V vasp_anyop::Setup(t_classid c)
{
	FLEXT_CADDMETHOD(c,1,a_list);
	FLEXT_CADDMETHOD_(c,1,"vasp",a_list);
	FLEXT_CADDMETHOD_(c,1,"radio",a_radio);

	FLEXT_CADDATTR_VAR(c,"arg",m_getarg,m_setarg);
}

V vasp_anyop::a_list(I argc,const t_atom *argv) 
{ 
	if(argc) {
		arg.SetList(argc,argv);
		if(arg.IsNone()) 
			post("%s - argument could not be evaluated (ignored)",thisName());
		else if(argchk) {
			// check argument feasibility
		}
	}
	else {
//		post("%s - Empty list argument (ignored)",thisName());
	}
}

Vasp *vasp_anyop::x_work() { return tx_work(arg); }

Vasp *vasp_anyop::tx_work(const Argument &arg) 
{
	error("%s - no work method implemented",thisName());
	return NULL;
}

