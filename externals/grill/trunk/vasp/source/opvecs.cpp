/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2010 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

/*! \file vasp__ctrl.cpp
	\brief Methods for handling of vector data for real, complex and multi-vector cases.

*/

#include "main.h"
#include "opbase.h"
#include "classes.h"
#include "vecblk.h"
#include "util.h"

/*! \brief Corrects for the common vector frame count
	\param frms frame count to correct
	\param bl new frame count
	\return true if a correction was made
*/
static BL corrlen(I &frms,I bl,I bf = -1,I bo = 0) 
{
	if(bf < 0) bf = bl;

	BL corr = false;
	BL all = frms < 0;
	if(all)
		frms = bl;
	else if(frms > bl) {
		// longer than vector length -> correct
		frms = bl;
		corr = true;
	}

	if(bo+frms > bf) {
		// now check if buffer size is exceeded
//			post("%s - %s vector (%s) exceeds buffer size: cropped",op,bli == 0?"src":"dst",bref->Name());
		frms = bf-bo;
		if(frms < 0) frms = 0;
		corr = true;
	}

	return corr;
}


inline BL corrlen(I &frms,VBuffer &b) 
{
	return corrlen(frms,b.Length(),b.Frames(),b.Offset());
}


/*! \brief Make real vector block for unary operations.

	\param op operation name
	\param src source vasp
	\param dst optional destination vasp
	\return struct with vector data 

	\remark working size is maximum common vector size
*/
RVecBlock *VaspOp::GetRVecs(const C *op,CVasp &src,CVasp *dst)
{
	I nvecs = src.Vectors();
	if(dst && dst->Ok() && dst->Vectors() != nvecs) {
		nvecs = min(nvecs,dst->Vectors());
		post("%s - src/dst vector number not equal -> taking minimum",op);
	}

	RVecBlock *ret = new RVecBlock(nvecs);

	BL ok = true,dlens = false;
	I tfrms = -1;

	Vasp *vbl[2] = {&src,dst};

	for(I bli = 0; bli < 2; ++bli)
		for(I ci = 0; ok && ci < nvecs; ++ci) {
			VBuffer *bref = NULL;
			if(vbl[bli] && vbl[bli]->Ok()) {
				bref = vbl[bli]->Buffer(ci);		
				if(!bref->Data()) {
					post("%s - %s vector (%s) is invalid",op,bli == 0?"src":"dst",bref->Name());
					delete bref; bref = NULL;
					ok = false; 
				}
				else
					dlens = dlens || corrlen(tfrms,*bref);
			}
			
			if(bli == 0) 
				ret->Src(ci,bref);
			else 
				ret->Dst(ci,bref);
		}

	if(dlens) post("%s - vector length has been limited to maximum common length (%i)",op,tfrms);

	ret->Frames(tfrms < 0?0:tfrms);

	if(ok) return ret;
	else { delete ret; return NULL;	}
}

/*! \brief Make real vector block for unary operations.

	\param op operation name
	\param src source vasp
	\param dst optional destination vasp
	\param full true if imaginary part is compulsory
	\return struct with vector data 
*/
CVecBlock *VaspOp::GetCVecs(const C *op,CVasp &src,CVasp *dst,BL full)
{
	I nvecs = src.Vectors();
	if(dst && dst->Ok() && dst->Vectors() != nvecs) {
		nvecs = min(nvecs,dst->Vectors());
		post("%s - src/dst vector number not equal -> taking minimum",op);
	}

	I pairs = nvecs/2;
	if(nvecs != pairs*2) 
		if(full) {
			post("%s - number of vectors is odd - not allowed",op);
			return NULL;
		}
		else {
			post("%s - number of vectors is odd - omitting last vector",op);
		}

	CVecBlock *ret = new CVecBlock(pairs);
	BL ok = true,dlens = false;
	I tfrms = -1;

	Vasp *vbl[2] = {&src,dst};

	for(I bli = 0; bli < 2; ++bli) 
		for(I ci = 0; ci < pairs; ++ci) {
			VBuffer *bre = NULL,*bim = NULL; // complex channels
			if(vbl[bli] && vbl[bli]->Ok()) {
				const C *vnm = bli == 0?"src":"dst";
				bre = vbl[bli]->Buffer(ci*2);
				bim = vbl[bli]->Buffer(ci*2+1); // complex channels

				if(!bre->Data()) {
					post("%s - real %s vector (%s) is invalid",op,vnm,bre->Name());
					delete bre; bre = NULL;
					ok = false; 
				}
				if(bim && !bim->Data()) {
					post("%s - imag %s vector (%s) is invalid",op,vnm,bim->Name());
					delete bim; bim = NULL;
					ok = false; 
				}

				// check against common vector length
				if(bre) {
					dlens = dlens || corrlen(tfrms,*bre);
				}
				if(bim)	{
					dlens = dlens || corrlen(tfrms,*bim);
				}

			}

			if(bli == 0) 
				ret->Src(ci,bre,bim);
			else 
				ret->Dst(ci,bre,bim);
		}

	if(dlens) post("%s - vector src/dst length has been limited to maximum common length (%i)",op,tfrms);

	ret->Frames(tfrms < 0?0:tfrms);

	if(ok) return ret;
	else { delete ret; return NULL;	}
}


/*! \brief Make real vector block for binary operations.

	\param op operation name
	\param src source vasp
	\param arg argument vasp
	\param dst optional destination vasp
	\param multi 0 off/1 on/-1 auto... controls whether argument vector is single- or multi-vectored
	\return struct with vector data 
*/
RVecBlock *VaspOp::GetRVecs(const C *op,CVasp &src,const CVasp &arg,CVasp *dst,I multi,BL ssize)
{
	if(!arg.Ok()) {
		post("%s - invalid argument vasp detected and ignored",op);
		return NULL;
	}

	I nvecs = src.Vectors();
	if(dst && dst->Ok() && dst->Vectors() != nvecs) {
		nvecs = min(nvecs,dst->Vectors());
		post("%s - src/dst vector number not equal -> taking minimum",op);
	}

	RVecBlock *ret;

	if(multi < 0) { // auto mode
		multi = arg.Vectors() > 1;
	}

	if(multi) {
		if(arg.Vectors() < nvecs) {
			nvecs = arg.Vectors();
			post("%s - too few arg vectors, operating on only first %i vectors",op,nvecs);
		}
		ret = new RVecBlock(nvecs,nvecs,1);
		for(I i = 0; i < nvecs; ++i) 
			ret->Arg(i,arg.Buffer(i));
	}
	else {
		if(arg.Vectors() > 1) {
			post("%s - using only first arg vector for all operations",op);
		}
		ret = new RVecBlock(nvecs,nvecs,1);
		for(I i = 0; i < nvecs; ++i) 
			ret->Arg(i,arg.Buffer(0));
	}

	BL ok = true,dlens = false,dalens = false;
	I tfrms = -1,afrms = -1;

	for(I ci = 0; ok && ci < nvecs; ++ci) {
		VBuffer *bref = src.Buffer(ci);	
		VBuffer *barg = ret->Arg(multi?ci:0);
		VBuffer *bdst = dst && dst->Ok()?dst->Buffer(ci):NULL;

		if(barg && (multi || ci == 0) && !barg->Data()) {
			post("%s - arg vector (%s) is invalid",op,barg->Name());
			ok = false; break; // really break?
		}
		else if(!bref->Data()) {
			post("%s - src vector (%s) is invalid",op,bref->Name());
			ok = false; break; // really break?
		}
	
		// check src/dst frame lengths
		dlens = dlens || corrlen(tfrms,*bref);
		if(bdst) dlens = dlens || corrlen(tfrms,*bdst);

		// check arg frame length
		if(barg) dalens = dalens || corrlen(afrms,*barg);

		ret->Src(ci,bref);
		if(bdst) ret->Dst(ci,bdst);
	}

	if(dlens) post("%s - vector src/dst length has been limited to maximum common length (%i)",op,tfrms);
	if(dalens) post("%s - vector arg length has been limited to maximum common length (%i)",op,afrms);

	if(ssize) {
		if(corrlen(tfrms,afrms))
			post("%s - vector src/dst and arg lengths are unequal -> set to max. common length (%i)",op,tfrms);
		afrms = tfrms;
	}

	ret->Frames(tfrms < 0?0:tfrms);
	ret->ArgFrames(afrms < 0?0:afrms);

	if(ok) return ret;
	else { delete ret; return NULL;	}
}


/*! \brief Make real complex block for binary operations.

	\param op operation name
	\param src source vasp
	\param arg argument vasp
	\param dst optional destination vasp
	\param multi 0 off/1 on/-1 auto... controls whether argument vector is single- or multi-vectored
	\param full true if imaginary part is compulsory
	\return struct with vector data 
*/
CVecBlock *VaspOp::GetCVecs(const C *op,CVasp &src,const CVasp &arg,CVasp *dst,I multi,BL ssize,BL full)
{
	if(!arg.Ok()) {
		post("%s - invalid argument vasp detected and ignored",op);
		return NULL;
	}

	I nvecs = src.Vectors();
	if(dst && dst->Ok() && dst->Vectors() != nvecs) {
		nvecs = min(nvecs,dst->Vectors());
		post("%s - src/dst vector number not equal -> taking minimum",op);
	}

	I pairs = nvecs/2;
	CVecBlock *ret;

	if(multi < 0) { // auto mode
		multi = arg.Vectors() > 2; // more than one argument pair -> multi
	}

	if(multi) {
		I apairs = arg.Vectors()/2;
		if(arg.Vectors() != apairs*2) 
			if(full) {
				post("%s - number of arg vectors is odd - not allowed",op);
				return NULL;
			}
			else {
				post("%s - number of arg vectors is odd - assuming complex part as 0",op);
				++apairs;
			}

		if(apairs < pairs) {
			pairs = apairs;
			post("%s - too few arg vectors, operating on only first %i vector pairs",op,pairs);
		}
		ret = new CVecBlock(pairs,pairs,1);
		for(I i = 0; i < pairs; ++i) 
			ret->Arg(i,arg.Buffer(i*2),arg.Buffer(i*2+1));
	}
	else {
		if(arg.Vectors() > 2) {
			post("%s - using only first arg vector pair for all operations",op);
		}
		ret = new CVecBlock(pairs,pairs,1);
		for(I i = 0; i < pairs; ++i) 
			ret->Arg(i,arg.Buffer(0),arg.Buffer(1));
	}

	BL ok = true,dlens = false,dalens = false;
	I tfrms = -1,afrms = -1;

	{
		if(nvecs != pairs*2) {
			post("%s - number of src vectors is odd - omitting last vector",op);
			// clear superfluous vector?
		}

		for(I ci = 0; ok && ci < pairs; ++ci) {
			// --- arg stuff ----------------

			VBuffer *brarg = ret->ReArg(ci),*biarg = ret->ImArg(ci);

			if(multi || ci == 0) {
				if(!brarg->Data()) {
					post("%s - real arg vector (%s) is invalid",op,brarg->Name());
					ok = false; break;
				}
				else if(biarg && !biarg->Data()) {
					post("%s - imag arg vector (%s) is invalid",op,biarg->Name());
					ok = false; break;
				}
			}

			// check against common arg length
			if(brarg) dalens = dalens || corrlen(afrms,*brarg);
			if(biarg) dalens = dalens || corrlen(afrms,*biarg);

			// --- src/dst stuff ----------------

			VBuffer *brref = src.Buffer(ci*2),*biref = src.Buffer(ci*2+1);		
			VBuffer *brdst,*bidst;
			if(dst && dst->Ok()) brdst = dst->Buffer(ci*2),bidst = dst->Buffer(ci*2+1);
			else brdst = bidst = NULL;

			if(!brref->Data()) {
				post("%s - real src vector (%s) is invalid",op,brref->Name());
				ok = false; break; // really break?
			}
			else if(biref && !biref->Data()) {
				post("%s - imag src vector (%s) is invalid",op,biref->Name());
				ok = false; break; // really break?
			}
			else {
				dlens = dlens || corrlen(tfrms,*brref);
				if(biref) dlens = dlens || corrlen(tfrms,*biref);
				if(brdst) dlens = dlens || corrlen(tfrms,*brdst);
				if(bidst) dlens = dlens || corrlen(tfrms,*bidst);
			}

			ret->Src(ci,brref,biref);
			if(brdst) ret->Dst(ci,brdst,bidst);
		}
	}

	if(dlens) post("%s - vector src/dst length has been limited to maximum common length (%i)",op,tfrms);
	if(dalens) post("%s - vector arg length has been limited to maximum common length (%i)",op,afrms);

	if(ssize) {
		if(corrlen(tfrms,afrms))
			post("%s - vector src/dst and arg lengths are unequal -> set to max. common length (%i)",op,tfrms);
		afrms = tfrms;
	}

	ret->Frames(tfrms < 0?0:tfrms);
	ret->ArgFrames(afrms < 0?0:afrms);

	if(ok) return ret;
	else { delete ret; return NULL;	}
}


/*! \brief Run the operation on the various real vectors.

	\param vecs src/arg/dst vector block
	\param fun operative function
	\param p parameter block for operative function
	\return normalized vasp or NULL on error

	\todo set overlap flag

	\remark operative function must be capable of handling reversed direction
*/
Vasp *VaspOp::DoOp(RVecBlock *vecs,opfun *fun,OpParam &p,BL symm)
{
	BL ok = true;

	if(vecs->ArgBlks() && (!p.arg || p.args < vecs->ArgBlks())) {
		post("%s - not enough argument blocks",p.opname);
		ok = false;
	}

	const I scnt = symm?2:1;
	for(I i = 0; ok && i < vecs->Vecs(); ++i) 
		for(I si = 0; ok && si < scnt; ++si) {
			p.frames = vecs->Frames();

			VBuffer *s = vecs->Src(i),*d = vecs->Dst(i); 
			p.rsdt = s->Pointer(),p.rss = s->Channels();
		
			if(d) p.rddt = d->Pointer(),p.rds = d->Channels();
			else p.rddt = p.rsdt,p.rds = p.rss;

			for(I bi = 0; bi < vecs->ArgBlks(); ++bi) {
				VBuffer *a = vecs->Arg(i,bi);
				p.arg[bi].SetV(a?a->Pointer():NULL,a?a->Channels():0);
			}
		
			if(!symm) 
				p.symm = -1;
			else {
				const I hcnt = p.frames/2;
				p.oddrem = p.frames != 2*hcnt;

				if((p.symm = si) == 0) {
					p.frames = hcnt+(p.oddrem?1:0); 
				}
				else {
					const I r = p.frames-hcnt;
					p.frames = hcnt;
					p.rsdt += r*p.rss,p.rddt += r*p.rds;

					// What to do with arguments in symmetric mode?
					// let the object decide!!
				}				
			}

			{	// ---- Check out and try to resolve overlap situation ------------

				BL sovr = p.SR_In(); // check whether dst is before src 
				if(p.HasArg()) { 
					// has argument
					if(sovr) {
						// src/dst needs reversal -> check if ok for arg/dst
						p.ovrlap = true;

						if(p.AR_Can()) 
							p.R_Rev(); // Revert vectors
						else {
							post("%s - vector overlap situation can't be resolved",p.opname);
							ok = false;
						}
					}
					else if(p.AR_In()) { 
						// arg/dst needs reversal -> check if ok for src/dst
						p.ovrlap = true;

						if(p.SR_Can()) 
							p.R_Rev(); // Revert vectors
						else {
							post("%s - vector overlap situation can't be resolved",p.opname);
							ok = false;
						}
					}
				}
				else { // No arg
					if(sovr) {
						p.ovrlap = true;
						p.R_Rev(); // if overlapping revert vectors
					}
					else 
						p.ovrlap = p.SR_Ovr();
				}
			}

			ok = fun(p);

#ifdef FLEXT_THREAD
			flext_base::ThrYield();
#endif
		}
	return ok?vecs->ResVasp():NULL;
}


/*! \brief Run the operation on the various complex vector pairs.

	\param vecs src/arg/dst vector block
	\param fun operative function
	\param p parameter block for operative function
	\return normalized vasp or NULL on error

	\todo set overlap flag

	\remark operative function must be capable of handling reversed direction
*/
Vasp *VaspOp::DoOp(CVecBlock *vecs,opfun *fun,OpParam &p,BL symm)
{
	BL ok = true;

	if(vecs->ArgBlks() && (!p.arg || p.args < vecs->ArgBlks())) {
		post("%s - not enough argument blocks",p.opname);
		ok = false;
	}

	const I scnt = symm?2:1;
	for(I i = 0; ok && i < vecs->Pairs(); ++i) 
		for(I si = 0; ok && si < scnt; ++si) {
			p.frames = vecs->Frames();
		
			VBuffer *rsv = vecs->ReSrc(i),*isv = vecs->ImSrc(i);
			p.rsdt = rsv->Pointer(),p.rss = rsv->Channels();
			p.isdt = isv->Pointer(),p.iss = isv->Channels();

			VBuffer *rdv = vecs->ReDst(i),*idv = vecs->ImDst(i);
			if(rdv) {
				p.rddt = rdv->Pointer(),p.rds = rdv->Channels();
				if(idv) p.iddt = idv->Pointer(),p.ids = idv->Channels();
				else p.iddt = NULL; //,p.ids = 0; // Can that be NULL??
			}
			else { 
				p.rddt = p.rsdt,p.rds = p.rss,p.iddt = p.isdt,p.ids = p.iss;
			}
			
			for(I bi = 0; bi < vecs->ArgBlks(); ++bi) {
				VBuffer *rav = vecs->ReArg(i,bi),*iav = vecs->ImArg(i,bi);
				p.arg[bi].SetV(rav?rav->Pointer():NULL,rav?rav->Channels():0,iav?iav->Pointer():NULL,iav?iav->Channels():0);
			}

			if(!symm) 
				p.symm = -1;
			else {
				const I hcnt = p.frames/2;
				p.oddrem = p.frames != 2*hcnt;

				if((p.symm = si) == 0) {
					p.frames = hcnt+(p.oddrem?1:0); 
				}
				else {
					const I r = p.frames-hcnt;
					p.frames = hcnt;
					p.rsdt += r*p.rss,p.isdt += r*p.iss;
					p.rddt += r*p.rds;
					if(p.iddt) p.iddt += r*p.ids; // Can that be NULL??

					// What to do with arguments?
					// let objects decide!!
				}
			}

			{	// ---- Check out and try to resolve overlap situation ------------

				BL sovr = p.SR_In(); // check whether dst is before src 
				if(sovr && !p.SI_Can()) {
					post("%s - src/dst overlap of re/im vectors not resolvable",p.opname);			
					ok = false;
				}

				if(ok && p.HasArg()) { 
					// has argument
					if(sovr) {
						// src/dst needs reversal -> check if ok for arg/dst
						p.ovrlap = true;

						if(p.AR_Can() && p.AI_Can()) 
							p.C_Rev(); // Revert vectors
						else {
							post("%s - vector overlap situation can't be resolved",p.opname);
							ok = false;
						}
					}
					else if(p.AR_In() || p.AI_In()) { 
						// arg/dst needs reversal -> check if ok for src/dst
						p.ovrlap = true;

						if(p.AR_Can() && p.AI_Can() && p.SR_Can() && p.SI_Can()) 
							p.C_Rev(); // Revert vectors
						else {
							post("%s - vector overlap situation can't be resolved",p.opname);
							ok = false;
						}
					}
				}
				else { // No arg
					if(sovr) {
						p.ovrlap = true;
						p.C_Rev(); // if overlapping revert vectors
					}
					else 
						p.ovrlap = p.SR_Ovr() || p.SI_Ovr();
				}
			}

			ok = fun(p);

#ifdef FLEXT_THREAD
			flext_base::ThrYield();
#endif
		}
	return ok?vecs->ResVasp():NULL;
}



