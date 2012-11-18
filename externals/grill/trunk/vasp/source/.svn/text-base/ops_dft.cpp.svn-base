/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

/*! \file ops_dft.cpp
	\brief Implementation of DFT routines

	\todo align temporary memory allocations

	All DFTs are normalized by 1/sqrt(n), hence the complex ones are repeatable

	- complex FFT radix-2: in-place
	- real FFT radix-2 (split-radix): in-place 
	- complex DFT radix-n (split-radix): out-of-place
	- real DFT radix-n: out-of-place / based on complex DFT radix-n (inefficient)

	In-place transformation is only possible for stride=1
*/

#include "main.h"
#include "ops_dft.h"
#include "mixfft.hpp"
#include "rdx2fft.hpp"
#include "rvfft.hpp"
#include <cstring>

///////////////////////////////////////////////////////////////

#ifdef FLEXT_THREADS
static flext::ThrMutex mixmtx;
#endif

typedef flext::buffer::Element El;

template<typename T1,typename T2,typename T3,typename T4>
static BL call_mixfft(I cnt,T1 rstmp,T2 istmp,T3 rdtmp,T4 idtmp)
{
    BL ret;
	// mixfft is not thread-safe
#ifdef FLEXT_THREADS
	mixmtx.Lock();
#endif
    try {
        MixFFT<F> mix(cnt);
        mix(rstmp,istmp,rdtmp,idtmp);
        ret = true;
	}
	catch(std::exception const &e)
	{
        post(e.what());
        ret = false;
	}
#ifdef FLEXT_THREADS
	mixmtx.Unlock();
#endif
    return ret;
}

//! Real forward DFT radix-n (managing routine)
static BL fft_fwd_real_any(I cnt,El *rsdt,I _rss,El *rddt,I _rds) 
{
	if(!rddt) rddt = rsdt,_rds = _rss;

#ifdef VASP_CHN1
	const I rds = 1,rss = 1;
#else
	const I rds = _rds,rss = _rss;
#endif

	const BL rst = rss != 1;
	const BL rdt = rsdt == rddt || rds != 1;

	El *rstmp,*istmp;
	register I i;
	
	if(rst) {
		rstmp = new El[cnt];
		// happens only if rss != 1, no optimization necessary
		for(i = 0; i < cnt; ++i) rstmp[i] = rsdt[i*rss];
	}
	else
		rstmp = rsdt;

	istmp = new El[cnt];
    flext::ZeroSamples(istmp,cnt);

	El *rdtmp = rdt?new El[cnt]:rddt;
	El *idtmp = new El[cnt];

	BL ret = call_mixfft(cnt,rstmp,istmp,rdtmp,idtmp);

	if(ret) {
		const F nrm = 1./sqrt((F)cnt);
		const I n2 = cnt/2;

#ifndef VASP_COMPACT
		if(rds == 1) {
			for(i = 0; i <= n2; ++i) rddt[i] = rdtmp[i]*nrm;
			for(i = 1; i < cnt-n2; ++i) rddt[i+n2] = idtmp[i]*nrm;
		}
		else 
#endif
		{
			for(i = 0; i <= n2; ++i) rddt[i*rds] = rdtmp[i]*nrm;
			for(i = 1; i < cnt-n2; ++i) rddt[(i+n2)*rds] = idtmp[i]*nrm;
		}
	}

	if(rst) delete[] rstmp;
	delete[] istmp;
	if(rdt) delete[] rdtmp;
	delete[] idtmp;

	return ret;
}


//! Real inverse DFT radix-n (managing routine)
static BL fft_inv_real_any(I cnt,El *rsdt,I _rss,El *rddt,I _rds) 
{
	if(!rddt) rddt = rsdt,_rds = _rss;

#ifdef VASP_CHN1
	const I rds = 1,rss = 1;
#else
	const I rds = _rds,rss = _rss;
#endif

	const BL rst = rss != 1;
	const BL rdt = rsdt == rddt || rds != 1;

	const I n2 = cnt/2;
	El *rstmp,*istmp;
	istmp = new El[cnt];
	register I i;
	
	if(rst) {
		rstmp = new El[cnt];
		// happens only if rss != 1, no optimization necessary
		for(i = 0; i <= n2; ++i) rstmp[i] = rsdt[i*rss];
		for(i = 1; i < cnt-n2; ++i) istmp[cnt-i] = rsdt[(n2+i)*rss];
	}
	else {
		rstmp = rsdt;
		for(i = 1; i < cnt-n2; ++i) istmp[cnt-i] = rsdt[n2+i];
	}

	// make symmetric parts
	for(i = 1; i < cnt-n2; ++i) {
		istmp[i] = -istmp[cnt-i];
		rstmp[cnt-i] = rstmp[i];
	}
	istmp[0] = 0;
	if(cnt%2 == 0) istmp[n2] = 0;


	El *rdtmp = rdt?new El[cnt]:rddt;
	El *idtmp = new El[cnt];

	BL ret = call_mixfft(cnt,rstmp,istmp,rdtmp,idtmp);

	if(rst) delete[] rstmp;
	delete[] istmp;
	if(rdt) delete[] rdtmp;
	delete[] idtmp;

	return ret;
}

///////////////////////////////////////////////////////////////

//! Complex forward DFT radix-n (managing routine)
static BL fft_fwd_complex_any(I cnt,El *rsdt,I _rss,El *isdt,I _iss,El *rddt,I _rds,El *iddt,I _ids) 
{
	if(!rddt) rddt = rsdt,_rds = _rss;
	if(!iddt) iddt = isdt,_ids = _iss;

#ifdef VASP_CHN1
	const I rds = 1,ids = 1,rss = 1,iss = 1;
#else
	const I rds = _rds,ids = _ids,rss = _rss,iss = _iss;
#endif

	const BL rst = rss != 1;
	const BL ist = iss != 1;
	const BL rdt = rsdt == rddt || rds != 1;
	const BL idt = isdt == iddt || ids != 1;

	El *rstmp,*istmp;
	register I i;
	
	if(rst) {
		rstmp = new El[cnt];
		// happens only if rss != 1, no optimization necessary
		for(i = 0; i < cnt; ++i) rstmp[i] = rsdt[i*rss];
	}
	else
		rstmp = rsdt;

	if(ist) {
		istmp = new El[cnt];
		// happens only if iss != 1, no optimization necessary
		for(i = 0; i < cnt; ++i) istmp[i] = isdt[i*iss];
	}
	else
		istmp = isdt;

	El *rdtmp = rdt?new El[cnt]:rddt;
	El *idtmp = idt?new El[cnt]:iddt;

	BL ret = call_mixfft(cnt,rstmp,istmp,rdtmp,idtmp);
		
	if(ret) {
		const F nrm = 1./sqrt((F)cnt);

#ifdef VASP_COMPACT
		for(i = 0; i < cnt; ++i) {
			rddt[i*rds] = rdtmp[i]*nrm;
			iddt[i*ids] = idtmp[i]*nrm;
		}
#else
		if(rdt) {
			if(rds != 1)
				for(i = 0; i < cnt; ++i) rddt[i*rds] = rdtmp[i]*nrm;
			else
				for(i = 0; i < cnt; ++i) rddt[i] = rdtmp[i]*nrm;
		}
		else // ok, this branch is not absolutely necessary
			if(rds != 1)
				for(i = 0; i < cnt; ++i) rddt[i*rds] *= nrm;
			else
				for(i = 0; i < cnt; ++i) rddt[i] *= nrm;

		if(idt) {
			if(ids != 1)
				for(i = 0; i < cnt; ++i) iddt[i*ids] = idtmp[i]*nrm;
			else
				for(i = 0; i < cnt; ++i) iddt[i] = idtmp[i]*nrm;
		}
		else // ok, this branch is not absolutely necessary
			if(ids != 1)
				for(i = 0; i < cnt; ++i) iddt[i*ids] *= nrm;
			else
				for(i = 0; i < cnt; ++i) iddt[i] *= nrm;
#endif
	}

	if(rst) delete[] rstmp;
	if(ist) delete[] istmp;
	if(rdt) delete[] rdtmp;
	if(idt) delete[] idtmp;

	return ret;
}

//! Complex inverse DFT radix-n (managing routine)
static BL fft_inv_complex_any(I cnt,El *rsdt,I _rss,El *isdt,I _iss,El *rddt,I _rds,El *iddt,I _ids)
{
	I i;

	if(!rddt) rddt = rsdt,_rds = _rss;
	if(!iddt) iddt = isdt,_ids = _iss;

#ifdef VASP_CHN1
	const I rds = 1,ids = 1,rss = 1,iss = 1;
#else
	const I rds = _rds,ids = _ids,rss = _rss,iss = _iss;
#endif

#ifndef VASP_COMPACT
	if(iss == 1)
		for(i = 0; i < cnt; ++i) isdt[i] = -isdt[i];
	else
#endif
		for(i = 0; i < cnt; ++i) isdt[i*iss] *= -1;

	BL ret = fft_fwd_complex_any(cnt,rsdt,rss,isdt,iss,rddt,rds,iddt,ids);

	if(ret) {
#ifndef VASP_COMPACT
		if(ids == 1)
			for(i = 0; i < cnt; ++i) iddt[i] = -iddt[i];
		else
#endif
			for(i = 0; i < cnt; ++i) iddt[i*ids] *= -1;
	}

	// reverse minus on input
	if(isdt != iddt) {
#ifndef VASP_COMPACT
		if(iss == 1)
			for(i = 0; i < cnt; ++i) isdt[i] = -isdt[i];
		else
#endif
			for(i = 0; i < cnt; ++i) isdt[i*iss] *= -1;
	}
	return ret;
}

///////////////////////////////////////////////////////////////

//! Complex forward FFT radix-2 (managing routine)
static BL fft_complex_radix2(I cnt,El *rsdt,I _rss,El *isdt,I _iss,El *rddt,I _rds,El *iddt,I _ids,I dir)
{
	if(!rddt) rddt = rsdt,_rds = _rss;
	if(!iddt) iddt = isdt,_ids = _iss;

#ifdef VASP_CHN1
	const I rds = 1,ids = 1,rss = 1,iss = 1;
#else
	const I rds = _rds,ids = _ids,rss = _rss,iss = _iss;
#endif

	BL rt = false,it = false;
	El *rtmp,*itmp;
	register I i;

	if(rss == 1)
		rtmp = rsdt;
	else {
		if(rsdt == rddt || rds != 1) 
			rtmp = new El[cnt],rt = true;
		else
			rtmp = rddt;
		for(i = 0; i < cnt; ++i) rtmp[i] = rsdt[i*rss];
	}

	if(iss == 1) 
		itmp = isdt;
	else {
		if(isdt == iddt || ids != 1) 
			itmp = new El[cnt],it = true;
		else
			itmp = iddt;
		for(i = 0; i < cnt; ++i) itmp[i] = isdt[i*iss];
	}

	BL ret = fft_bidir_complex_radix2(cnt,rtmp,itmp,dir);

	if(ret) {
		const F nrm = 1./sqrt((F)cnt);

#ifndef VASP_COMPACT
		if(rtmp == rddt)
			for(i = 0; i < cnt; ++i) rddt[i] *= nrm;
		else if(rds == 1)
			for(i = 0; i < cnt; ++i) rddt[i] = rtmp[i]*nrm;
		else
#endif
			for(i = 0; i < cnt; ++i) rddt[i*rds] = rtmp[i]*nrm;

#ifndef VASP_COMPACT
		if(itmp == iddt)
			for(i = 0; i < cnt; ++i) iddt[i] *= nrm;
		else if(ids == 1)
			for(i = 0; i < cnt; ++i) iddt[i] = itmp[i]*nrm;
		else
#endif
			for(i = 0; i < cnt; ++i) iddt[i*ids] = itmp[i]*nrm;	
	}

	if(rt) delete[] rtmp;
	if(it) delete[] itmp;

	return ret;
}

inline BL fft_fwd_complex_radix2(I cnt,El *rsdt,I _rss,El *isdt,I _iss,El *rddt,I _rds,El *iddt,I _ids)
{
	return fft_complex_radix2(cnt,rsdt,_rss,isdt,_iss,rddt,_rds,iddt,_ids,1);
}

inline BL fft_inv_complex_radix2(I cnt,El *rsdt,I _rss,El *isdt,I _iss,El *rddt,I _rds,El *iddt,I _ids)
{
	return fft_complex_radix2(cnt,rsdt,_rss,isdt,_iss,rddt,_rds,iddt,_ids,-1);
}

///////////////////////////////////////////////////////////////

// normalize and reverse imaginary part in-place
template<typename T>
static void nrmirev(T *data,int n,float fn)
{
	int i;
	const I n2 = n/2,n4 = n2/2;
	for(i = 0; i <= n2; ++i) data[i] *= fn;
	for(i = 1; i < n4; ++i) { 
		register F tmp = data[n2+i];
		data[n2+i] = data[n-i]*fn;
		data[n-i] = tmp*fn;
	}
	if(n2%2 == 0) data[n2+n4] *= fn;
}

//! Real forward FFT radix-2 (managing routine)
BL fft_fwd_real_radix2(I cnt,El *src,I _sstr,El *dst,I _dstr)
{
#ifdef VASP_CHN1
	const I dstr = 1,sstr = 1;
#else
	const I dstr = _dstr,sstr = _sstr;
#endif

	register I i;
	const I n2 = cnt/2;
	const F fn = (F)(1./sqrt((F)cnt));
	El *stmp;
	if(!dst || src == dst) {
		// in-place

		if(sstr == 1) 
			stmp = src;
		else {
			stmp = new El[cnt];
			for(i = 0; i < cnt; ++i) stmp[i] = src[i*sstr];
		}
		
		realfft_split(stmp,cnt);

		if(sstr == 1) {
			// src == stmp !!!
			nrmirev(stmp,cnt,fn);
		}
		else {
			for(i = 0; i <= n2; ++i) src[i*sstr] = stmp[i]*fn;
			for(i = 1; i < n2; ++i) src[(n2+i)*sstr] = stmp[cnt-i]*fn;
			delete[] stmp;
		}
	}
	else {
		// out of place

		if(sstr == 1) 
			stmp = src;
		else {
			stmp = dstr == 1?dst:new El[cnt];
			for(i = 0; i < cnt; ++i) stmp[i] = src[i*sstr];
		}

		realfft_split(stmp,cnt);

		if(sstr == 1) {
#ifdef VASP_COMPACT
			if(dstr == 1) {
				for(i = 0; i <= n2; ++i) dst[i] = stmp[i]*fn;
				for(i = 1; i < n2; ++i) dst[n2+i] = stmp[cnt-i]*fn;
			}
			else 
#endif
			{
				for(i = 0; i <= n2; ++i) dst[i*dstr] = stmp[i]*fn;
				for(i = 1; i < n2; ++i) dst[(n2+i)*dstr] = stmp[cnt-i]*fn;
			}
		}
		else {
			if(dstr == 1) {
				// dst == stmp !!!
				nrmirev(stmp,cnt,fn);
			}
			else {
				for(i = 0; i <= n2; ++i) dst[i*dstr] = stmp[i]*fn;
				for(i = 1; i < n2; ++i) dst[(n2+i)*dstr] = stmp[cnt-i]*fn;
				delete[] dst;
			}
		}
	}
	
	return true;
}

//! Real inverse FFT radix-2 (managing routine)
BL fft_inv_real_radix2(I cnt,El *src,I _sstr,El *dst,I _dstr)
{
#ifdef VASP_CHN1
	const I dstr = 1,sstr = 1;
#else
	const I dstr = _dstr,sstr = _sstr;
#endif

	register I i;
	const I n2 = cnt/2;
	const F fn = (F)(1./sqrt((F)cnt));
	El *stmp;
	if(!dst || src == dst) {
		// in-place

		if(sstr == 1) {
			stmp = src;
			nrmirev(stmp,cnt,fn);
		}
		else {
			stmp = new El[cnt];

#ifdef VASP_COMPACT
			if(sstr == 1) {
				for(i = 0; i <= n2; ++i) stmp[i] = src[i]*fn;
				for(i = 1; i < n2; ++i) stmp[cnt-i] = src[n2+i]*fn;
			}
			else 
#endif
			{
				for(i = 0; i <= n2; ++i) stmp[i] = src[i*sstr]*fn;
				for(i = 1; i < n2; ++i) stmp[cnt-i] = src[(n2+i)*sstr]*fn;
			}
		}
		
		irealfft_split(stmp,cnt);

		if(sstr != 1) {
			for(i = 0; i < cnt; ++i) src[i*sstr] = stmp[i];
			delete[] stmp;
		}
	}
	else {
		// out of place

		if(dstr == 1) {
			stmp = dst;
#ifdef VASP_COMPACT
			if(sstr == 1) {
				for(i = 0; i <= n2; ++i) stmp[i] = src[i]*fn;
				for(i = 1; i < n2; ++i) stmp[cnt-i] = src[n2+i]*fn;
			}
			else
#endif
			{
				for(i = 0; i <= n2; ++i) stmp[i] = src[i*sstr]*fn;
				for(i = 1; i < n2; ++i) stmp[cnt-i] = src[(n2+i)*sstr]*fn;
			}
		}
		else {
			stmp = new El[cnt];
			if(sstr == 1) {
				// dst == stmp !!!
				nrmirev(stmp,cnt,fn);
			}
			else {
				for(i = 0; i <= n2; ++i) stmp[i] = src[i*sstr]*fn;
				for(i = 1; i < n2; ++i) stmp[cnt-i] = src[(n2+i)*sstr]*fn;
			}
		}

		irealfft_split(stmp,cnt);

		if(dstr != 1) {
			for(i = 0; i < cnt; ++i) dst[i*dstr] = stmp[i];
			delete[] stmp;
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////

//! Determine if size is radix-2
static I radix2(I size)
{
	I i,j;
	for(i = j = 1; j < size; i++,j <<= 1) (void)0;
	return j == size?i:-1;
}

Vasp *VaspOp::m_rfft(OpParam &p,CVasp &src,CVasp *dst,BL inv) 
{ 
	RVecBlock *vecs = GetRVecs(p.opname,src,dst);
	if(vecs) {
		BL ok = true;
		for(I i = 0; ok && i < vecs->Vecs(); ++i) {
			VBuffer *s = vecs->Src(i);
			VBuffer *d = vecs->Dst(i);
			if(!d) d = s;

			if(vecs->Frames() > 1)
				if(radix2(vecs->Frames()) >= 1) 
					// radix-2
					if(inv)
						ok = fft_inv_real_radix2(vecs->Frames(),s->Pointer(),s->Channels(),d->Pointer(),d->Channels());
					else
						ok = fft_fwd_real_radix2(vecs->Frames(),s->Pointer(),s->Channels(),d->Pointer(),d->Channels());
				else
					// radix-n
					if(inv)
						ok = fft_inv_real_any(vecs->Frames(),s->Pointer(),s->Channels(),d->Pointer(),d->Channels());
					else
						ok = fft_fwd_real_any(vecs->Frames(),s->Pointer(),s->Channels(),d->Pointer(),d->Channels());
		}
		return ok?vecs->ResVasp():NULL;
	}
	else
		return NULL;
}

Vasp *VaspOp::m_cfft(OpParam &p,CVasp &src,CVasp *dst,BL inv) 
{ 
	CVecBlock *vecs = GetCVecs(p.opname,src,dst,true);
	if(vecs) {
		BL ok = true;
		for(I i = 0; ok && i < vecs->Pairs(); ++i) {
			VBuffer *sre = vecs->ReSrc(i),*sim = vecs->ImSrc(i);
			VBuffer *dre = vecs->ReDst(i),*dim = vecs->ImDst(i);
			if(!dre) dre = sre;
			if(!dim) dim = sim;

			if(vecs->Frames() > 1)
				if(radix2(vecs->Frames()) >= 1) 
					// radix-2
					if(inv)
						ok = fft_inv_complex_radix2(vecs->Frames(),sre->Pointer(),sre->Channels(),sim?sim->Pointer():NULL,sim?sim->Channels():0,dre->Pointer(),dre->Channels(),dim->Pointer(),dim->Channels());
					else
						ok = fft_fwd_complex_radix2(vecs->Frames(),sre->Pointer(),sre->Channels(),sim?sim->Pointer():NULL,sim?sim->Channels():0,dre->Pointer(),dre->Channels(),dim->Pointer(),dim->Channels());
				else
					// radix-n
					if(inv)
						ok = fft_inv_complex_any(vecs->Frames(),sre->Pointer(),sre->Channels(),sim?sim->Pointer():NULL,sim?sim->Channels():0,dre->Pointer(),dre->Channels(),dim->Pointer(),dim->Channels());
					else
						ok = fft_fwd_complex_any(vecs->Frames(),sre->Pointer(),sre->Channels(),sim?sim->Pointer():NULL,sim?sim->Channels():0,dre->Pointer(),dre->Channels(),dim->Pointer(),dim->Channels());
		}
		return ok?vecs->ResVasp():NULL;
	}
	else
		return NULL;
}

VASP_UNARY("vasp.rfft",rfft,true,"Real DFT")
VASP_UNARY("vasp.r!fft",rifft,true,"Real inverse DFT")
VASP_UNARY("vasp.cfft",cfft,true,"Complex DFT")
VASP_UNARY("vasp.c!fft",cifft,true,"Complex inverse DFT")

