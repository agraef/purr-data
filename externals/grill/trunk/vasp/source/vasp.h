/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP__H
#define __VASP__H

#include "vbuffer.h"

class Vasp:
    public flext
{
public:
    class Ref:
        public flext
    {
	public:
		Ref(): sym(NULL) {}
		Ref(VBuffer &b);
		Ref(VSymbol &s,I c,I o);
		Ref(const Ref &r) { operator =(r); }
		~Ref();

		Ref &operator =(const Ref &r);

		V Clear() { sym.Clear(); }
		BL Ok() const { return sym.Ok(); }

		VSymbol &Symbol() { return sym; }
		const VSymbol &Symbol() const { return sym; }
		V Symbol(const VSymbol &s);
		I Channel() const { return chn; }
		V Channel(I c) { chn = c; }
		I Offset() const { return offs; }
		V Offset(I o) { offs = o; }
		V OffsetD(I o) { offs += o; }

	protected:
		VSymbol sym;
		I chn;
		I offs; // counted in frames
	};

	Vasp();
	Vasp(I argc,const t_atom *argv);
	Vasp(const Vasp &v);
	Vasp(I frames,const Ref &r);
	~Vasp();

	static BL ChkArgs(I argc,const t_atom *argv);

	const C *thisName() const { return typeid(*this).name(); }

	// check if vasp reference is valid
	BL Check() const;

	Vasp &operator =(const Vasp &v);
	Vasp &operator ()(I argc,const t_atom *argv /*,BL withvasp = false*/);

	// set used channels to 0
	Vasp &Clear();

	// used vectors
	I Vectors() const { return chns; }

	// length of the vasp (in frames)
	I Frames() const { return frames; }
	// set frame count
	V Frames(I fr) { frames = fr; }
	// set frame count differentially
	V FramesD(I frd) { if(frames >= 0) frames += frd; }
	// set frame count 
	V FramesM(R f) { if(frames >= 0) frames = (int)(frames*f); }
	// set frame count 
	V FramesR(R f) { if(f) FramesM(1./f); else Frames(0); }

	// set buffer sizes
	V Size(I fr,BL keep = true,BL zero = true);
	// set frame count differentially
	V SizeD(I frd,BL keep = true,BL zero = true);
	// set frame count 
	V SizeM(R f,BL keep = true,BL zero = true);
	// set frame count 
	V SizeR(R f,BL keep = true,BL zero = true) { if(f) SizeM(1./f,keep,zero); else Size(0,false); }

	// actual length of the vasp (in frames)
	I ChkFrames() const;

	// set offset(s)
	V Offset(I fr);
	// set offset(s) differentially
	V OffsetD(I fr);

	// set channel(s)
	V Channel(I ch);

	BL Ok() const { return ref && Vectors() > 0; }
	BL IsComplex() const { return ref && Vectors() >= 2 && ref[1].Ok(); }

	// get any vector - test if in range 0..Vectors()-1!
	const Ref &Vector(I ix) const { return ref[ix]; }
	Ref &Vector(I ix) { return ref[ix]; }

	// get real part - be sure that Ok!
	const Ref &Real() const { return Vector(0); }
	Ref &Real() { return Vector(0); }

	// get imaginary part - be sure that Complex!
	const Ref &Imag() const { return Vector(1); }
	Ref &Imag() { return Vector(1); }

	// get buffer associated to a channel
	VBuffer *Buffer(I ix) const;

	// add another vector
	Vasp &AddVector(const Ref &r);

	// Real/Complex
	VBuffer *ReBuffer() const { return Buffer(0); }
	VBuffer *ImBuffer() const { return Buffer(1); }

	// prepare and reference t_atom list for output
	V MakeList(flext::AtomList &ret,BL withvasp = true) const;
	// prepare and reference t_atom list for output
	flext::AtomList *MakeList(BL withvasp = true) const;

	// make a graphical update of all buffers in vasp
	V Refresh();
	
protected:
	I frames; // length counted in frames
	I chns; // used channels
	I refs; // allocated channels (>= chns)
	Ref *ref;

	V Resize(I rcnt);
};

/*! \brief Checked vasp
	\remark Only use that for immediate operation!
*/
class CVasp:
	public Vasp
{
public:
	CVasp();
	CVasp(const Vasp &v);

	// add vectors of another vasp
	CVasp &operator +=(const CVasp &v);

};


#endif
