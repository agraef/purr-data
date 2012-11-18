/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "opparam.h"

/*
// Duplication of breakpoint lists should be avoided
OpParam::Arg &OpParam::Arg::operator =(const Arg &op)
{
	Clear();

	switch(argtp = op.argtp) {
	case arg_x:	x = op.x; break;
	case arg_v:	v = op.v; break;
	case arg_bp:	{
		// Copy breakpoint list (find a different way, e.g. store them in a pool)
		bp.pts = op.bp.pts;
		bp.pt = new R[bp.pts];
		for(I i = 0; i < bp.pts; ++i) 
			bp.pt[i] = op.bp.pt[i];
		break;
	}
	}

	return *this;
}
*/
V OpParam::Arg::Clear()
{
//	if(argtp == arg_bp && bp.pt) delete[] bp.pt;	
	argtp = arg_;
}

OpParam::Arg &OpParam::Arg::SetX(S r,S i)
{
	Clear();
	argtp = arg_x;
	x.r = r,x.i = i;
	return *this;
}

OpParam::Arg &OpParam::Arg::SetV(BS *r,I rs,BS *i,I is)
{
	Clear();
	argtp = arg_v;
	v.rdt = r,v.rs = rs;
	v.idt = i,v.is = is;
	return *this;
}

/*
OpParam::Arg &OpParam::Arg::SetB(I pts,const R *pt)
{
	Clear();
	argtp = arg_bp;
	bp.pts = pts;
	bp.pt = new R[pts];
	for(I ix = 0; ix < pts; ix) bp.pt[ix] = pt[ix];
	return *this;
}
*/

OpParam::Arg &OpParam::Arg::SetE(const Env *env)
{
	Clear();
	argtp = arg_env;
	e.env = env;
	return *this;
}


/*
V OpParam::SDR_Rev() { SR_Rev(); DR_Rev(); }
V OpParam::SDI_Rev() { SI_Rev(); DI_Rev(); }
V OpParam::SDC_Rev() { SDR_Rev(); SDI_Rev(); }
V OpParam::ADR_Rev() { AR_Rev(); DR_Rev(); }
V OpParam::ADI_Rev() { AI_Rev(); DI_Rev(); }
V OpParam::ADC_Rev() { ADR_Rev(); ADI_Rev(); }
V OpParam::SADR_Rev() { SR_Rev(); AR_Rev(); DR_Rev(); }
V OpParam::SADI_Rev() { SI_Rev(); AI_Rev(); DI_Rev(); }
V OpParam::SADC_Rev() { SADR_Rev(); SADI_Rev(); }
*/

OpParam::OpParam(const C *opnm,I nargs): 
	opname(opnm),frames(0),args(0),arg(NULL),
	/*part(false),*/ ovrlap(false),revdir(false),oddrem(false)
{
	InitArgs(nargs);
}

OpParam::~OpParam() { Clear(); }

V OpParam::InitArgs(I n)
{
	if(arg) Clear();
	args = n;
	if(args) arg = new Arg[args];
}

V OpParam::Clear()
{
	if(arg) { delete[] arg; arg = NULL;	}
	args = 0;
}


/*! \brief Reverse direction of real vector operation 
	\todo Check for existence of vectors!
*/
V OpParam::R_Rev() 
{ 

	SR_Rev(); 
	DR_Rev();
	AR_Rev(); 
	revdir = true;
}

/*! \brief Reverse direction of complex vector operation 
	\todo Check for existence of vectors!
*/
V OpParam::C_Rev() 
{ 
	SR_Rev(); SI_Rev(); 
	DR_Rev(); DI_Rev();
	AR_Rev(); AI_Rev(); 
	revdir = true;
}


V OpParam::AR_Rev(I bl) 
{ 
	if(arg[bl].argtp == Arg::arg_v && arg[bl].v.rdt) 
		arg[bl].v.rdt -= (frames-1)*(arg[bl].v.rs = -arg[bl].v.rs); 
}

V OpParam::AI_Rev(I bl) 
{ 
	if(arg[bl].argtp == Arg::arg_v && arg[bl].v.idt) 
		arg[bl].v.idt -= (frames-1)*(arg[bl].v.is = -arg[bl].v.is); 
}

BL OpParam::AR_In(I bl) const 
{ 
    return arg[bl].argtp == Arg::arg_v && arg[bl].v.rdt && rddt > arg[bl].v.rdt && rddt < arg[bl].v.rdt+frames*arg[bl].v.rs; 
} 

BL OpParam::AI_In(I bl) const 
{ 
    return arg[bl].argtp == Arg::arg_v && arg[bl].v.idt && iddt > arg[bl].v.idt && iddt < arg[bl].v.idt+frames*arg[bl].v.is; 
} 

BL OpParam::AR_Can(I bl) const 
{ 
    return arg[bl].argtp != Arg::arg_v || !arg[bl].v.rdt || arg[bl].v.rdt <= rddt || arg[bl].v.rdt >= rddt+frames*rds; 
} 

BL OpParam::AI_Can(I bl) const 
{ 
    return arg[bl].argtp != Arg::arg_v || !arg[bl].v.idt || arg[bl].v.idt <= iddt || arg[bl].v.idt >= iddt+frames*ids; 
} 

BL OpParam::AR_Ovr(I bl) const 
{ 
    return arg[bl].argtp == Arg::arg_v && arg[bl].v.rdt && rddt != arg[bl].v.rdt && rddt < arg[bl].v.rdt+frames*arg[bl].v.rs && arg[bl].v.rdt < rddt+frames*rds; 
} 

BL OpParam::AI_Ovr(I bl) const 
{ 
    return arg[bl].argtp == Arg::arg_v && arg[bl].v.idt && iddt != arg[bl].v.idt && iddt < arg[bl].v.idt+frames*arg[bl].v.is && arg[bl].v.idt < iddt+frames*ids; 
} 



BL OpParam::AR_In() const
{
	for(I i = 0; i < args; ++i) 
		if(AR_In(i)) return true;
	return false;
}

BL OpParam::AI_In() const
{
	for(I i = 0; i < args; ++i) 
		if(AI_In(i)) return true;
	return false;
}

BL OpParam::AR_Can() const
{
	for(I i = 0; i < args; ++i) 
		if(!AR_Can(i)) return false;
	return true;
}

BL OpParam::AI_Can() const
{
	for(I i = 0; i < args; ++i) 
		if(!AI_Can(i)) return false;
	return true;
}

BL OpParam::AR_Ovr() const
{
	for(I i = 0; i < args; ++i) 
		if(!AR_Ovr(i)) return false;
	return true;
}

BL OpParam::AI_Ovr() const
{
	for(I i = 0; i < args; ++i) 
		if(!AI_Ovr(i)) return false;
	return true;
}


V OpParam::AR_Rev()
{
	for(I i = 0; i < args; ++i) AR_Rev(i);
}

V OpParam::AI_Rev()
{
	for(I i = 0; i < args; ++i) AI_Rev(i);
}

V OpParam::SkipOddMiddle()
{
	if(symm == 0 && oddrem) {
		// don't process middle sample!
		if(revdir) rsdt += rss,rddt += rds;
		frames--; 
	}
}
	
V OpParam::SkipOddMiddle(S m)
{
	if(symm == 0 && oddrem) {
		// set and skip middle sample!
		frames--; 
		if(revdir) *rddt = m,rsdt += rss,rddt += rds;
		else rddt[frames] = m;
	}
}
	
