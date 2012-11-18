/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#ifndef __VASP_VECBLK_H
#define __VASP_VECBLK_H

#include "vasp.h"

class VecBlock:
    public flext
{
public:

	I Frames() const { return frms; }
	V Frames(I fr) { frms = fr; }
	I ArgFrames() const { return afrms; }
	V ArgFrames(I fr) { afrms = fr; }
	I ArgBlks() const { return barg; }

	BL Complex() { return cplx; }

protected:
	VecBlock(BL cplx,I msrc,I mdst,I marg,I blarg);
	~VecBlock();

	Vasp *_SrcVasp(I n);
	Vasp *_DstVasp(I n);
	Vasp *_ResVasp(I n); // either Dst or Src

	VBuffer *_Src(I ix) { return vecs[ix]; }
	VBuffer *_Dst(I ix) { return vecs[asrc+aarg*barg+ix]; }
	V _Src(I ix,VBuffer *v) { vecs[ix] = v; }
	V _Dst(I ix,VBuffer *v) { vecs[asrc+aarg*barg+ix] = v; }

	VBuffer *_Arg(I ix,I bl = 0) { return vecs[asrc+bl*aarg+ix]; }
	V _Arg(I ix,VBuffer *v,I bl = 0) { vecs[asrc+bl*aarg+ix] = v; }

private:
	BL cplx;
	I asrc,adst,aarg,barg;
	VBuffer **vecs;
	I frms,afrms;
};


class RVecBlock:
	public VecBlock
{
public:
	RVecBlock(I _n,I _a = 0,I _ba = 0): VecBlock(false,_n,_n,_a,_ba),n(_n),a(_a) {}

	VBuffer *Src(I ix) { return _Src(ix); }
	VBuffer *Dst(I ix) { return _Dst(ix); }
	V Src(I ix,VBuffer *v) { _Src(ix,v); }
	V Dst(I ix,VBuffer *v) { _Dst(ix,v); }

	VBuffer *Arg(I ix,I bl = 0) { return _Arg(ix,bl); }
	V Arg(I ix,VBuffer *v,I bl = 0) { _Arg(ix,v,bl); }

	I Vecs() const { return n; }
	I Args() const { return a; }

	Vasp *SrcVasp() { return _SrcVasp(n); }
	Vasp *DstVasp() { return _DstVasp(n); }
	Vasp *ResVasp() { return _ResVasp(n); }

protected:
	I n,a;
};

class CVecBlock:
	public VecBlock
{
public:
	CVecBlock(I _np,I _ap = 0,I _bap = 0): VecBlock(true,_np*2,_np*2,_ap*2,_bap),np(_np),ap(_ap) {}

	VBuffer *ReSrc(I ix) { return _Src(ix*2); }
	VBuffer *ImSrc(I ix) { return _Src(ix*2+1); }
	VBuffer *ReDst(I ix) { return _Dst(ix*2); }
	VBuffer *ImDst(I ix) { return _Dst(ix*2+1); }
	V Src(I ix,VBuffer *vre,VBuffer *vim) { _Src(ix*2,vre); _Src(ix*2+1,vim); }
	V Dst(I ix,VBuffer *vre,VBuffer *vim) { _Dst(ix*2,vre); _Dst(ix*2+1,vim); }

	VBuffer *ReArg(I ix,I bl = 0) { return _Arg(ix*2,bl); }
	VBuffer *ImArg(I ix,I bl = 0) { return _Arg(ix*2+1,bl); }
	V Arg(I ix,VBuffer *vre,VBuffer *vim,I bl = 0) { _Arg(ix*2,vre,bl); _Arg(ix*2+1,vim,bl); }

	I Pairs() const { return np; }
	I Args() const { return ap; }

	Vasp *SrcVasp() { return _SrcVasp(np*2); }
	Vasp *DstVasp() { return _DstVasp(np*2); }
	Vasp *ResVasp() { return _ResVasp(np*2); }

protected:
	I np,ap;
};

#endif
