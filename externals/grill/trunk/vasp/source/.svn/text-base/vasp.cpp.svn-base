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

///////////////////////////////////////////////////////////////////////////
// Vasp class
///////////////////////////////////////////////////////////////////////////

Vasp::Ref::Ref(VBuffer &b): sym(b.Symbol()),chn(b.Channel()),offs(b.Offset()) {}
Vasp::Ref::Ref(VSymbol &s,I c,I o): sym(s),chn(c),offs(o) {}
Vasp::Ref::~Ref() {}

Vasp::Ref &Vasp::Ref::operator =(const Ref &r)
{
	sym = r.sym,chn = r.chn,offs = r.offs;
	return *this;
}

V Vasp::Ref::Symbol(const VSymbol &s) { sym = s; }


Vasp::Vasp():
	refs(0),chns(0),ref(NULL),
	frames(0)
{
}

Vasp::Vasp(I argc,const t_atom *argv):
	refs(0),chns(0),ref(NULL),
	frames(0)
{
	operator ()(argc,argv);
}

Vasp::Vasp(const Vasp &v):
	refs(0),chns(0),ref(NULL),
	frames(0)
{
	operator =(v);
}

Vasp::Vasp(I fr,const Ref &r):
	refs(0),chns(0),ref(NULL),
	frames(fr)
{
	AddVector(r);
}


Vasp::~Vasp()
{
	Clear();
}

Vasp &Vasp::Clear()
{
	refs = frames = chns = 0;
	if(ref) { delete[] ref; ref = NULL; }
	return *this;
}


BL Vasp::ChkArgs(I argc,const t_atom *argv)
{
	I ix = 0;

	// vasp keyword
	const t_symbol *v = ix < argc?flext::GetASymbol(argv[ix]):NULL;
	if(v && v == vasp_base::sym_vasp) ix++; // if it is "vasp" ignore it

	// length argument
	if(argc > ix && flext::CanbeInt(argv[ix])) ix++;

	while(argc > ix) {
		// check for symbol
		const t_symbol *bsym = flext::GetASymbol(argv[ix]);
		if(!bsym || !flext::GetString(bsym) || !flext::GetString(bsym)[0]) {  // expect a symbol
			// not symbol -> bail out
			return false;
		}
		else
			ix++;

		// check for offset
		if(argc > ix && flext::CanbeInt(argv[ix])) ix++;

		// check for channel
		if(argc > ix && flext::CanbeInt(argv[ix])) ix++;
	}

	return true;
}

V Vasp::Resize(I rcnt) {
	if(!ref) {
		ref = new Ref[refs = rcnt];
		chns = 0;
	}
	else if(rcnt > refs) {
		Ref *rnew = new Ref[refs = rcnt];
		for(I ix = 0; ix < chns; ++ix) rnew[ix] = ref[ix];
		delete[] ref;
		ref = rnew;
	}
}



Vasp &Vasp::operator =(const Vasp &v)
{
	if(!v.Ok())
		Clear();
	else {
		frames = v.frames;
		if(!ref || v.chns > refs) {
			if(ref) delete[] ref;
			ref = new Ref[refs = v.chns];
		}

		chns = v.chns;
		for(I ix = 0; ix < chns; ++ix) {
			ref[ix] = v.ref[ix];
		}
	}

	return *this;
}


Vasp &Vasp::AddVector(const Ref &r)
{
	Resize(chns+1);
	ref[chns++] = r;
	return *this;
}


// parse argument list
Vasp &Vasp::operator ()(I argc,const t_atom *argv)
{
	BL lenset = false;
	I ix = 0;

	I maxneeded = argc; // maximum number of ref'd buffers
	// rather use a temp storage
	if(!ref || refs < maxneeded) {
		if(ref) delete[] ref;
		ref = new Ref[refs = maxneeded];
	}

	const t_symbol *v = ix < argc?flext::GetASymbol(argv[ix]):NULL;
	if(v && v == vasp_base::sym_vasp) ix++; // if it is "vasp" ignore it

	if(argc > ix && flext::CanbeInt(argv[ix])) {
		frames = flext::GetAInt(argv[ix]);
		lenset = true;
		ix++;
	}
	else
		frames = -1;

	chns = 0;
	while(argc > ix) {
		const t_symbol *bsym = flext::GetASymbol(argv[ix]);
		if(!bsym || !flext::GetString(bsym) || !flext::GetString(bsym)[0]) {  // expect a symbol
			Clear();
			return *this;
		}
		else
			ix++;

		// is a symbol!
		Ref &r = ref[chns];
		r.Symbol(VSymbol(bsym));

		if(argc > ix && flext::CanbeInt(argv[ix])) {
			r.Offset((I)flext::GetAInt(argv[ix]));
			ix++;
		}
		else
			r.Offset(0);

		if(argc > ix && flext::CanbeInt(argv[ix])) {
			r.Channel((I)flext::GetAInt(argv[ix]));
			ix++;
		}
		else
			r.Channel(0);

		chns++;
	}

	if(!lenset) {
		// set length to maximum!
		// or let it be -1 to represent the maximum?!
		frames = -1;
		// if len is already set then where to check for oversize?
	}

	return *this;
}


VBuffer *Vasp::Buffer(I ix) const
{
	if(ix >= Vectors())
		return NULL;
	else {
		const Ref &r = Vector(ix);
		VBuffer *ret = BufLib::Get(r.Symbol(),r.Channel(),Frames(),r.Offset());
		return ret;
	}
}

// generate Vasp list of buffer references
V Vasp::MakeList(flext::AtomList &ret,BL withvasp) const
{
	I voffs = withvasp?1:0;
	I needed = voffs+1+Vectors()*3;
	ret(needed);

	if(withvasp)
		flext::SetSymbol(ret[0],vasp_base::sym_vasp);  // VASP

	flext::SetInt(ret[voffs],frames);  // frames

	for(I ix = 0; ix < Vectors(); ++ix) {
		const Ref &r = Vector(ix);
		flext::SetSymbol(ret[voffs+1+ix*3],r.Symbol().Symbol());  // buf
		flext::SetInt(ret[voffs+2+ix*3],r.Offset());  // offs
		flext::SetInt(ret[voffs+3+ix*3],r.Channel());  // chn
	}
}


// generate Vasp list of buffer references
flext::AtomList *Vasp::MakeList(BL withvasp) const
{
	flext::AtomList *ret = new flext::AtomList;
	MakeList(*ret,withvasp);
	return ret;
}


V Vasp::Refresh()
{
	for(I i = 0; i < Vectors(); ++i) {
		VBuffer *vb = Buffer(i);
		if(vb) {
			vb->Refresh();
			delete vb;
		}
	}
}

V Vasp::Offset(I o)
{
	for(I i = 0; i < Vectors(); ++i) Vector(i).Offset(o);
}

V Vasp::OffsetD(I od)
{
	for(I i = 0; i < Vectors(); ++i) Vector(i).OffsetD(od);
}


V Vasp::Channel(I c)
{
	for(I i = 0; i < Vectors(); ++i) Vector(i).Channel(c);
}


V Vasp::Size(I s,BL keep,BL zero)
{
	for(I i = 0; i < Vectors(); ++i) {
		VBuffer *buf = Buffer(i);
		if(buf) {
			buf->Frames(s,keep,zero);
			delete buf;
		}
	}
}

V Vasp::SizeD(I sd,BL keep,BL zero)
{
	for(I i = 0; i < Vectors(); ++i) {
		VBuffer *buf = Buffer(i);
		if(buf) {
			I s = buf->Frames()+sd;
			buf->Frames(s >= 0?s:0,keep,zero);
			delete buf;
		}
	}
}


V Vasp::SizeM(R f,BL keep,BL zero)
{
	for(I i = 0; i < Vectors(); ++i) {
		VBuffer *buf = Buffer(i);
		if(buf) {
			I s = (I)(buf->Frames()*f);
			buf->Frames(s >= 0?s:0,keep,zero);
			delete buf;
		}
	}
}

BL Vasp::Check() const
{
	BL ok = true;
	for(I i = 0; ok && i < Vectors(); ++i) {
		VBuffer *buf = Buffer(i);
		if(!buf)
			ok = false;
		else {
			ok = buf->Data() != NULL;
			delete buf;
		}
	}
	return ok;
}

I Vasp::ChkFrames() const
{
	if(Vectors() == 0) return 0;

	I frms = -1;
	for(I i = 0; i < Vectors(); ++i) {
		VBuffer *buf = Buffer(i);
		if(buf) {
			I f = buf->Length();
			if(frms < 0 || f < frms) frms = f;
			delete buf;
		}
	}

	return frms < 0?0:frms;
}


// ------------------------------------

CVasp::CVasp() {}

CVasp::CVasp(const Vasp &v):
	Vasp(v)
{
	if(!Check())
		Clear();
	else
		Frames(ChkFrames());
}

CVasp &CVasp::operator +=(const CVasp &v)
{
	if(v.Ok()) {
		if(!Ok()) *this = v;
		else {
			I f = Frames(),vf = v.Frames();

			if(f != vf) {
				post("vasp - Frame count of joined vasps is different - taking the minimum");
				Frames(min(f,vf));
			}

			Resize(Vectors()+v.Vectors());
			for(I i = 0; i < v.Vectors(); ++i) AddVector(v.Vector(i));
		}
	}
	return *this;
}

