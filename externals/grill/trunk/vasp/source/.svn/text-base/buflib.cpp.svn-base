/* 

VASP modular - vector assembling signal processor / objects for Max/MSP and PD

Copyright (c) 2002-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#include "main.h"
#include "buflib.h"
#include <cstdio>

#define LIBTICK 0.1 // tick time in s
#define LIBTOL 3  // how many ticks till release

#define REUSE_MAXLOSEREL 0.1  // max. fraction of lost buffer size 
#define REUSE_MAXLOSEABS 10000 // max. lost buffer size


#ifdef __MWERKS__
#define STD	std
#else
#define STD
#endif


class FreeEntry:
    public flext
{
public:
	FreeEntry(const t_symbol *s): sym(s),nxt(NULL) {}

	const t_symbol *sym;
	FreeEntry *nxt;
};

class BufEntry:
    public flext
{
public:
	BufEntry(const t_symbol *s,I fr,BL zero = true);
	~BufEntry();

	V IncRef();
	V DecRef();

	const t_symbol *sym;
	I refcnt,tick;
	BufEntry *nxt;

	I alloc,len;
	BS *data;
};


static BufEntry *libhead = NULL,*libtail = NULL;
static FreeEntry *freehead = NULL,*freetail = NULL;
static I libcnt = 0,libtick = 0;

#ifdef FLEXT_THREADS
static flext::ThrMutex libmtx,freemtx;
#endif

static V FreeLibSym(const t_symbol *s);




BufEntry::BufEntry(const t_symbol *s,I fr,BL zero): 
	sym(s), 
	alloc(fr),len(fr),
	refcnt(0),nxt(NULL) 
{
    data = (BS *)NewAligned(len*sizeof(*data));
	if(zero) flext::ZeroMem(data,len*sizeof(*data));
}

BufEntry::~BufEntry()
{
	if(sym) FreeLibSym(sym);
	if(data) FreeAligned(data);
}

V BufEntry::IncRef() { ++refcnt; }
V BufEntry::DecRef() { --refcnt; tick = libtick; }

static BufEntry *FindInLib(const t_symbol *s) 
{
	BufEntry *e;
	for(e = libhead; e && e->sym != s; e = e->nxt) (void)0;
	return e?e:NULL;
}

#ifdef FLEXT_DEBUG
static V DumpLib() 
{
	post("Dump {");
	BufEntry *e;
	for(e = libhead; e; e = e->nxt) {
		post("\t%s -> refs:%i, alloc:%i, len:%i -> %p",flext::GetString(e->sym),e->refcnt,e->alloc,e->len,e->data);
	}
	post("}");
}
#endif

VBuffer *BufLib::Get(const VSymbol &s,I chn,I len,I offs)
{
	BufEntry *e = FindInLib(s.Symbol());
	if(e) 
		return new ImmBuf(e,len,offs);
	else
		return new SysBuf(s,chn,len,offs);
}

V BufLib::IncRef(const t_symbol *s) 
{ 
	if(s) {
		BufEntry *e = FindInLib(s);
		if(e) e->IncRef();
	}
}

V BufLib::DecRef(const t_symbol *s)
{ 
	if(s) {
		BufEntry *e = FindInLib(s);
		if(e) e->DecRef();
	}
}

static V Collect()
{
#ifdef FLEXT_THREADS
	libmtx.Lock();
#endif

	// collect garbage
	BufEntry *e,*p;
	for(p = NULL,e = libhead; e; ) {
		if(e->refcnt <= 0 && e->tick+LIBTOL < libtick) {
			FLEXT_ASSERT(e->refcnt == 0);

			BufEntry *n = e->nxt;

			if(p) p->nxt = n;
			else libhead = n;

			if(!n) libtail = p;
			else e->nxt = NULL;

			delete e;

			e = n;
		}
		else
			p = e,e = e->nxt;
	}

#ifdef FLEXT_THREADS
	libmtx.Unlock();
#endif
}


#ifdef FLEXT_THREADS
static bool libthractive = false;
//static flext::thrid_t libthrid;
static bool libthrexit = false; // currently not used
static flext::ThrCond *libthrcond = NULL;

static V LibThr(flext::thr_params *)
{
	flext::RelPriority(-2);

	while(!libthrexit) {
		libthrcond->TimedWait(1); // don't go below 1 here as TimedWait might not support fractions of seconds!!!
		// TODO - should process return value of TimedWait
		Collect();	
	}
}
#endif

static flext::Timer *libclk = NULL;

static V LibTick(V *)
{
#ifdef FLEXT_THREADS
	libthrcond->Signal();
#else
	Collect();
#endif

	++libtick;
}

static const t_symbol *GetLibSym()
{
#ifdef FLEXT_THREADS
	freemtx.Lock();
#endif
	const t_symbol *ret;

	if(freehead) {
		// reuse from free-list
		FreeEntry *r = freehead;
		freehead = r->nxt;
		if(!freehead) freetail = NULL;
		const t_symbol *s = r->sym;
		delete r;
		ret = s;
	}
	else {
		// allocate new symbol
		char tmp[20];
		if(libcnt > 0xffff)
			STD::sprintf(tmp,"vasp!%08x",libcnt); 
		else // better hash lookup for 4 digits
			STD::sprintf(tmp,"vasp!%04x",libcnt); 
		libcnt++;
		ret = gensym(tmp);
	}

#ifdef FLEXT_THREADS
	freemtx.Unlock();
#endif
	return ret;
}

static V FreeLibSym(const t_symbol *sym)
{
#ifdef FLEXT_DEBUG
//	post("free %s",flext::GetString(sym));
#endif

#ifdef FLEXT_THREADS
	freemtx.Lock();
#endif

	FreeEntry *f = new FreeEntry(sym);
	if(!freehead) freehead = f;
	else freetail->nxt = f;
	freetail = f;

#ifdef FLEXT_THREADS
	freemtx.Unlock();
#endif
}


BufEntry *BufLib::NewImm(I fr,BL zero)
{
#ifdef FLEXT_THREADS
	if(!libthractive) {
		bool ret = flext::LaunchThread(LibThr,NULL);
		if(!ret)
			error("vasp - Could not launch helper thread");
		else {
			libthrcond = new flext::ThrCond;
			libthractive = true;
		}
	}
#endif
	if(!libclk) {
		libclk = new flext::Timer(true);
		libclk->SetCallback(LibTick);
		libclk->Periodic(LIBTICK);
	}

	const t_symbol *s = GetLibSym();
	BufEntry *entry = new BufEntry(s,fr,zero);

#ifdef FLEXT_THREADS
	libmtx.Lock();
#endif

	if(libtail) libtail->nxt = entry; 
	else libhead = entry;
	libtail = entry;

#ifdef FLEXT_DEBUG
//	DumpLib();
#endif

#ifdef FLEXT_THREADS
	libmtx.Unlock();
#endif

	return entry;
}

static F reuse_maxloserel = (F)REUSE_MAXLOSEREL;
static I reuse_maxloseabs = REUSE_MAXLOSEABS;

BufEntry *BufLib::Resize(BufEntry *e,I fr,BL keep,BL zero)
{ 
	if(e->alloc >= fr && fr >= e->alloc*(1-reuse_maxloserel) && fr >= (e->alloc-reuse_maxloseabs)) {
		// reuse buffer
		e->len = fr;
	}
	else {
		BS *nd = new BS[fr]; 
		if(keep) {
			I l = fr;
			if(e->len < l) {
				l = e->len;
				if(zero) flext::ZeroMem(nd+l,(fr-l)*sizeof(*nd));
			}
			flext::CopyMem(nd,e->data,l*sizeof(*nd));
		}

		delete[] e->data;
		e->data = nd;
		e->len = e->alloc = fr;
	}
	return e;
}



ImmBuf::ImmBuf(I len,BL zero):
	VBuffer(0,len),
	entry(BufLib::NewImm(len,zero))
{}

ImmBuf::ImmBuf(BufEntry *e,I len,I offs): 
	VBuffer(0,len,offs),
	entry(e) 
{
	if(Length() > e->alloc) {
		Length(e->alloc);
		post("vasp - buffer %s: Length (%i) is out of range, corrected to %i",GetString(e->sym),len,e->alloc);
	}
}

VSymbol ImmBuf::Symbol() const { return entry->sym; }

I ImmBuf::Frames() const { return entry->len; }

V ImmBuf::Frames(I fr,BL keep,BL zero) { entry = BufLib::Resize(entry,fr,keep,zero); }

BS *ImmBuf::Data() { return entry->data; }
