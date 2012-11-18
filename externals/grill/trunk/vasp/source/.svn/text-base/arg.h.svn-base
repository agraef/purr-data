/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_ARG_H
#define __VASP_ARG_H

#include "vasp.h"
#include "env.h"

#define VASP_ARG() Argument()
#define VASP_ARG_I(VAL) Argument().SetR(VAL)
#define VASP_ARG_R(VAL) Argument().SetR(VAL)
#define VASP_ARG_CX(RV,IV) Argument().SetCX(RV,IV)

class Argument:
    public flext
{
public:
	Argument();
	~Argument();
	
	Argument &Parse(I argc,const t_atom *argv);
	Argument &Clear();
	Argument &ClearAll();

	Argument &SetVasp(Vasp *v);
	Argument &SetEnv(Env *e);
	Argument &SetList(I argc,const t_atom *argv);
	Argument &SetI(I i);
	Argument &SetR(F f);
	Argument &SetR(D d);
	Argument &SetR(I i) { return SetR((F)i); }
	Argument &SetCX(F re,F im);
	Argument &SetVX(VX *vec);

	Argument *Next() { return nxt; }
	Argument &Next(I i);
	Argument &Add(Argument *a);

	Argument &AddVasp(Vasp *v);
	Argument &AddEnv(Env *e);
	Argument &AddList(I argc,const t_atom *argv);
	Argument &AddI(I i);
	Argument &AddR(F f);
	Argument &AddR(D d);
	Argument &AddR(I i) { return AddR((F)i); }
	Argument &AddCX(F re,F im);
	Argument &AddVX(VX *vec);

	BL IsNone() const { return tp == tp_none; }
	BL IsList() const { return tp == tp_list; }
	BL IsVasp() const { return tp == tp_vasp; }
	BL CanbeVasp() const { return tp == tp_vasp || (tp == tp_list && Vasp::ChkArgs(dt.atoms->Count(),dt.atoms->Atoms())); }
	BL IsEnv() const { return tp == tp_env; }
	BL CanbeEnv() const { return tp == tp_env || (tp == tp_env && Env::ChkArgs(dt.atoms->Count(),dt.atoms->Atoms())); }
	BL IsInt() const { return tp == tp_int; }
	BL CanbeInt() const { return tp == tp_int || tp == tp_float || tp_double; }
	BL IsFloat() const { return tp == tp_float; }
	BL CanbeFloat() const { return tp == tp_float || tp == tp_double || tp == tp_int; }
	BL IsDouble() const { return tp == tp_double; }
	BL CanbeDouble() const { return tp == tp_double || tp == tp_float || tp == tp_int; }
	BL IsComplex() const { return tp == tp_cx; }
	BL CanbeComplex() const { return tp == tp_cx || CanbeFloat(); }
	BL IsVector() const { return tp == tp_vx; }
	BL CanbeVector() const { return tp == tp_vx || CanbeComplex(); }

	const flext::AtomList &GetList() const { return *dt.atoms; }
	const Vasp &GetVasp() const { return *dt.v; }
	Vasp GetAVasp() const;
	const Env &GetEnv() const { return *dt.env; }
	Env GetAEnv() const;
	I GetInt() const { return dt.i; }
	I GetAInt() const;
	F GetFloat() const { return dt.f; }
	F GetAFloat() const;
	D GetDouble() const { return dt.d; }
	D GetADouble() const;
	const CX &GetComplex() const { return *dt.cx; }
	CX GetAComplex() const;
	const VX &GetVector() const { return *dt.vx; }
	VX GetAVector() const;

	V MakeList(flext::AtomList &ret);

protected:
	enum {
		tp_none,tp_vasp,tp_env,tp_list,tp_int,tp_float,tp_double,tp_cx,tp_vx
	} tp;

	union {
		Vasp *v;
		Env *env;
		flext::AtomList *atoms;
		F f;
		D d;
		I i;
		CX *cx;
		VX *vx;
	} dt;

	Argument *nxt;
};

#endif
