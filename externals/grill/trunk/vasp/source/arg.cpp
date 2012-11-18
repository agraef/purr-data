/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "arg.h"
#include "classes.h"

Argument::Argument(): tp(tp_none),nxt(NULL) {}
Argument::~Argument() { ClearAll(); }

Argument &Argument::Parse(I argc,const t_atom *argv)
{
	if(argc == 0)
		Clear();
	else // real?
	if(argc == 1 && flext::CanbeFloat(argv[0])) 
		SetR(flext::GetAFloat(argv[0]));
	else // complex?
	if(argc == 2 && flext::CanbeFloat(argv[0]) && flext::CanbeFloat(argv[1]))
		SetCX(flext::GetAFloat(argv[1]),flext::GetAFloat(argv[2]));
	else // double?
	if(argc >= 2 && flext::GetASymbol(argv[0]) == vasp_base::sym_double &&
		flext::CanbeFloat(argv[1]) && (argc == 2 || flext::CanbeFloat(argv[2]))
		)
		SetR((D)flext::GetAFloat(argv[1])+(D)flext::GetAFloat(argv[2]));
	else // envelope?
	if(Env::ChkArgs(argc,argv)) {
		Env *e = new Env(argc,argv);
		if(e && e->Ok()) SetEnv(e);
		else {
			Clear();
			post("vasp - env argument is invalid");
			delete e;
		}
	}
	else // vasp?
	if(Vasp::ChkArgs(argc,argv)) {
		Vasp *v = new Vasp(argc,argv);
		if(v && v->Ok()) SetVasp(v);
		else {
			Clear();
			post("vasp - vasp argument is invalid");
			delete v;
		}
	}
	else {
		Clear();
		post("vasp - invalid arguments");
	}
	return *this;
}

V Argument::MakeList(flext::AtomList &ret)
{
	switch(tp) {
	case tp_none:
		ret();
		break;
	case tp_list:
		ret = *dt.atoms;
		break;
	case tp_vasp:
		dt.v->MakeList(ret);
		break;
	case tp_env:
		dt.env->MakeList(ret);
		break;
	case tp_vx: {
		I d = dt.vx->size();
		ret(d+1);
		flext::SetSymbol(ret[0],vasp_base::sym_vector);
		for(I i = 0; i < d; ++i)
			flext::SetFloat(ret[i+1],(*dt.vx)[i]);
		break;
	}
	case tp_cx:
		ret(3);
		flext::SetSymbol(ret[0],vasp_base::sym_complex);
		flext::SetFloat(ret[1],dt.cx->real());
		flext::SetFloat(ret[2],dt.cx->imag());
		break;
	case tp_int:
		ret(1);
		flext::SetInt(ret[0],dt.i);
		break;
	case tp_float:
		ret(1);
		flext::SetFloat(ret[0],dt.f);
		break;
	case tp_double: {
		F f = (F)dt.d;
		ret(3);
		flext::SetSymbol(ret[0],vasp_base::sym_double);
		flext::SetFloat(ret[1],f);
		flext::SetFloat(ret[2],dt.d-f);
		break;
	}
	default:
		ERRINTERNAL();
	}
}


Argument &Argument::Clear()
{
	switch(tp) {
	case tp_none:
		break;
	case tp_list:
		if(dt.atoms) { delete dt.atoms; dt.atoms = NULL; }
		break;
	case tp_vasp:
		if(dt.v) { delete dt.v; dt.v = NULL; }
		break;
	case tp_env:
		if(dt.env) { delete dt.env; dt.env = NULL; }
		break;
	case tp_vx:
		if(dt.vx) { delete dt.vx; dt.vx = NULL; }
		break;
	case tp_cx:
		if(dt.cx) { delete dt.cx; dt.cx = NULL; }
		break;
	case tp_int:
	case tp_float:
	case tp_double:
		break;
	default:
		ERRINTERNAL();
	}
	tp = tp_none;
	return *this;
}

Argument &Argument::ClearAll()
{
	Clear();
	if(nxt) { delete nxt; nxt = NULL; }
	return *this;
}

Argument &Argument::SetVasp(Vasp *v)
{
	if(tp != tp_none) Clear();
	dt.v = v; tp = tp_vasp;
	return *this;
}

Argument &Argument::SetEnv(Env *e)
{
	if(tp != tp_none) Clear();
	dt.env = e; tp = tp_env;
	return *this;
}

Argument &Argument::SetList(I argc,const t_atom *argv)
{
	if(tp != tp_none) Clear();
	dt.atoms = new flext::AtomList(argc,argv); tp = tp_list;
	return *this;
}

Argument &Argument::SetR(F f)
{
	if(tp != tp_none) Clear();
	dt.f = f; tp = tp_float;
	return *this;
}

Argument &Argument::SetR(D f)
{
	if(tp != tp_none) Clear();
	dt.d = f; tp = tp_double;
	return *this;
}

Argument &Argument::SetI(I i)
{
	if(tp != tp_none) Clear();
	dt.i = i; tp = tp_int;
	return *this;
}

Argument &Argument::SetCX(F re,F im)
{
	if(tp != tp_none) Clear();
	dt.cx = new CX(re,im); tp = tp_cx;
	return *this;
}

Argument &Argument::SetVX(VX *vec)
{
	if(tp != tp_none) Clear();
	dt.vx = vec; tp = tp_vx;
	return *this;
}

I Argument::GetAInt() const { return (I)GetADouble(); }

F Argument::GetAFloat() const { return GetADouble(); }

D Argument::GetADouble() const
{
	if(IsInt()) return GetInt();
	else if(IsFloat()) return GetFloat();
	else if(IsDouble()) return GetDouble();
	else return 0;
}

CX Argument::GetAComplex() const
{
	if(IsInt()) return (F)GetInt();
	else if(IsFloat()) return GetFloat();
	else if(IsDouble()) return GetDouble();
	else if(IsComplex()) return GetComplex();
	else return 0;
}

Vasp Argument::GetAVasp() const 
{
	if(IsVasp()) return GetVasp();
	else if(IsList()) return Vasp(dt.atoms->Count(),dt.atoms->Atoms());
	else return Vasp();
}

Env Argument::GetAEnv() const 
{
	if(IsEnv()) return GetEnv();
	else if(IsList()) return Env(dt.atoms->Count(),dt.atoms->Atoms());
	else return Env();
}


Argument &Argument::Add(Argument *n) 
{ 
	if(nxt) nxt->Add(n);
	else nxt = n;
	return *n;
}

Argument &Argument::Next(I i)
{
	if(i <= 0) return *this;
	else {
		Argument *n = Next();
		if(n) return n->Next(i-1);
		else {
			error("Argument: index not found!");
			return *this;
		}
	}
}

Argument &Argument::AddVasp(Vasp *v) { Argument *a = new Argument; a->SetVasp(v); return Add(a); }

Argument &Argument::AddEnv(Env *e) { Argument *a = new Argument; a->SetEnv(e); return Add(a); }

Argument &Argument::AddList(I argc,const t_atom *argv) { Argument *a = new Argument; a->SetList(argc,argv); return Add(a); }

Argument &Argument::AddI(I i) { Argument *a = new Argument; a->SetI(i); return Add(a); }

Argument &Argument::AddR(F f) { Argument *a = new Argument; a->SetR(f); return Add(a); }

Argument &Argument::AddCX(F re,F im) { Argument *a = new Argument; a->SetCX(re,im); return Add(a); }

Argument &Argument::AddVX(VX *vec) { Argument *a = new Argument; a->SetVX(vec); return Add(a); }



