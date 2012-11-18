/* 
pool - hierarchical storage object for PD and Max/MSP

Copyright (c) 2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate: 2008-01-03 10:14:29 -0500 (Thu, 03 Jan 2008) $
$LastChangedBy: thomas $
*/

#include "pool.h"
#include <string>
#include <map>

#define POOL_VERSION "0.2.2pre"

#define VCNT 32
#define DCNT 8


class pool:
	public flext_base
{
	FLEXT_HEADER_S(pool,flext_base,setup)

public:
	pool(int argc,const t_atom *argv);
	virtual ~pool();

	static void setup(t_classid);
	
	virtual bool Init();

	pooldata *Pool() { return pl; }

protected:

	// switch to other pool
	void ms_pool(const AtomList &l);
	void mg_pool(AtomList &l);

    void mg_priv(bool &p) const { p = pl && pl->Private(); }

    // print some help message
    static void m_help() { post("pool " POOL_VERSION " - hierarchical storage object, (C)2002-2008 Thomas Grill"); }

	// clear all data in pool
	void m_reset();

	// handle directories
	void m_getdir();

	void m_mkdir(int argc,const t_atom *argv,bool abs = true,bool chg = false); // make (and change) to dir
	void m_mkchdir(int argc,const t_atom *argv) { m_mkdir(argc,argv,true,true); } // make and change to dir
	void m_chdir(int argc,const t_atom *argv,bool abs = true);		// change to dir
	void m_rmdir(int argc,const t_atom *argv,bool abs = true);		// remove dir
	void m_updir(int argc,const t_atom *argv);		// one or more levels up

    void ms_curdir(const AtomList &l) { m_chdir(l.Count(),l.Atoms()); }

	void m_mksub(int argc,const t_atom *argv) { m_mkdir(argc,argv,false); }
	void m_mkchsub(int argc,const t_atom *argv) { m_mkdir(argc,argv,false,true); }
	void m_chsub(int argc,const t_atom *argv) { m_chdir(argc,argv,false); }
	void m_rmsub(int argc,const t_atom *argv) { m_rmdir(argc,argv,false); }

	// handle data
	void m_set(int argc,const t_atom *argv) { set(argc,argv,true); }
	void m_seti(int argc,const t_atom *argv); // set value at index
	void m_add(int argc,const t_atom *argv) { set(argc,argv,false); }
	void m_clr(int argc,const t_atom *argv);
	void m_clri(int ix); // clear value at index
	void m_clrall();	// only values
	void m_clrrec();	// also subdirectories
	void m_clrsub();	// only subdirectories
	void m_get(int argc,const t_atom *argv);
	void m_geti(int ix); // get value at index
	void m_getall();	// only values
	void m_getrec(int argc,const t_atom *argv);	// also subdirectories
	void m_getsub(int argc,const t_atom *argv);	// only subdirectories
	void m_ogetall(int argc,const t_atom *argv);	// only values (ordered)
	void m_ogetrec(int argc,const t_atom *argv);	// also subdirectories (ordered)
	void m_ogetsub(int argc,const t_atom *argv);	// only subdirectories (ordered)
	void m_cntall();	// only values
	void m_cntrec(int argc,const t_atom *argv);	// also subdirectories
	void m_cntsub(int argc,const t_atom *argv);	// only subdirectories

	// print directories
	void m_printall();   // print values in current dir
	void m_printrec(int argc,const t_atom *argv,bool fromroot = false);   // print values recursively
    void m_printroot() { m_printrec(0,NULL,true); }   // print values recursively from root

	// cut/copy/paste
	void m_paste(int argc,const t_atom *argv) { paste(thisTag(),argc,argv,true); } // paste contents of clipboard
	void m_pasteadd(int argc,const t_atom *argv) { paste(thisTag(),argc,argv,false); } // paste but don't replace
	void m_clrclip();  // clear clipboard
	void m_cut(int argc,const t_atom *argv) { copy(thisTag(),argc,argv,true); } // cut value into clipboard
	void m_copy(int argc,const t_atom *argv) { copy(thisTag(),argc,argv,false); } 	// copy value into clipboard
	void m_cutall() { copyall(thisTag(),true,0); }   // cut all values in current directory into clipboard
	void m_copyall() { copyall(thisTag(),false,0); }   // copy all values in current directory into clipboard
	void m_cutrec(int argc,const t_atom *argv) { copyrec(thisTag(),argc,argv,true); }   // cut directory (and subdirs) into clipboard
	void m_copyrec(int argc,const t_atom *argv) { copyrec(thisTag(),argc,argv,false); }   // cut directory (and subdirs) into clipboard

	// load/save from/to file
    void m_load(int argc,const t_atom *argv) { load(argc,argv,false); }
	void m_save(int argc,const t_atom *argv) { save(argc,argv,false); }
	void m_loadx(int argc,const t_atom *argv) { load(argc,argv,true); } // XML
	void m_savex(int argc,const t_atom *argv) { save(argc,argv,true); } // XML

	// load directories
	void m_lddir(int argc,const t_atom *argv) { lddir(argc,argv,false); }   // load values into current dir
	void m_ldrec(int argc,const t_atom *argv) { ldrec(argc,argv,false); }   // load values recursively
	void m_ldxdir(int argc,const t_atom *argv) { lddir(argc,argv,true); }   // load values into current dir (XML)
	void m_ldxrec(int argc,const t_atom *argv) { ldrec(argc,argv,true); }   // load values recursively (XML)

	// save directories
	void m_svdir(int argc,const t_atom *argv) { svdir(argc,argv,false); }   // save values in current dir
	void m_svrec(int argc,const t_atom *argv) { svrec(argc,argv,false); }   // save values recursively
	void m_svxdir(int argc,const t_atom *argv) { svdir(argc,argv,true); }   // save values in current dir (XML)
	void m_svxrec(int argc,const t_atom *argv) { svrec(argc,argv,true); }   // save values recursively (XML)

private:
	static bool KeyChk(const t_atom &a);
	static bool ValChk(int argc,const t_atom *argv);
	static bool ValChk(const AtomList &l) { return ValChk(l.Count(),l.Atoms()); }
	void ToOutAtom(int ix,const t_atom &a);

    static const t_symbol *sym_echo;
    static const t_symbol *sym_error;

    enum get_t { get_norm,get_cnt,get_print };

	void set(int argc,const t_atom *argv,bool over);
	void getdir(const t_symbol *tag);
	int getrec(const t_symbol *tag,int level,int order,bool rev,get_t how /*= get_norm*/,const AtomList &rdir);
	int getsub(const t_symbol *tag,int level,int order,bool rev,get_t how /*= get_norm*/,const AtomList &rdir);

	void paste(const t_symbol *tag,int argc,const t_atom *argv,bool repl);
	void copy(const t_symbol *tag,int argc,const t_atom *argv,bool cut);
	void copyall(const t_symbol *tag,bool cut,int lvls);
	void copyrec(const t_symbol *tag,int argc,const t_atom *argv,bool cut);

	void load(int argc,const t_atom *argv,bool xml);
	void save(int argc,const t_atom *argv,bool xml);
	void lddir(int argc,const t_atom *argv,bool xml);   // load values into current dir
	void ldrec(int argc,const t_atom *argv,bool xml);   // load values recursively
	void svdir(int argc,const t_atom *argv,bool xml);   // save values in current dir
	void svrec(int argc,const t_atom *argv,bool xml);   // save values recursively

	void echodir() { if(echo) getdir(sym_echo); }

	bool absdir,echo;
	int vcnt,dcnt;
	pooldata *pl;
	Atoms curdir;
	pooldir *clip;

	static const t_symbol *holdname; // used during initialization of new object (between constructor and Init method)

    typedef std::map<const t_symbol *,pooldata *> PoolMap;
	static PoolMap poolmap;

	void SetPool(const t_symbol *s);
	void FreePool();

	static pooldata *GetPool(const t_symbol *s);
	static void RmvPool(pooldata *p);

	string MakeFilename(const char *fn) const;

	FLEXT_CALLVAR_V(mg_pool,ms_pool)
	FLEXT_ATTRGET_V(curdir)
	FLEXT_CALLSET_V(ms_curdir)
	FLEXT_ATTRVAR_B(absdir)
	FLEXT_ATTRVAR_B(echo)
	FLEXT_CALLGET_B(mg_priv)
	FLEXT_ATTRVAR_I(vcnt)
	FLEXT_ATTRVAR_I(dcnt)

	FLEXT_CALLBACK(m_help)

	FLEXT_CALLBACK(m_reset)

	FLEXT_CALLBACK(m_getdir)
	FLEXT_CALLBACK_V(m_mkdir)
	FLEXT_CALLBACK_V(m_chdir)
	FLEXT_CALLBACK_V(m_mkchdir)
	FLEXT_CALLBACK_V(m_updir)
	FLEXT_CALLBACK_V(m_rmdir)
	FLEXT_CALLBACK_V(m_mksub)
	FLEXT_CALLBACK_V(m_chsub)
	FLEXT_CALLBACK_V(m_mkchsub)
	FLEXT_CALLBACK_V(m_rmsub)

	FLEXT_CALLBACK_V(m_set)
	FLEXT_CALLBACK_V(m_seti)
	FLEXT_CALLBACK_V(m_add)
	FLEXT_CALLBACK_V(m_clr)
	FLEXT_CALLBACK_I(m_clri)
	FLEXT_CALLBACK(m_clrall)
	FLEXT_CALLBACK(m_clrrec)
	FLEXT_CALLBACK(m_clrsub)
	FLEXT_CALLBACK_V(m_get)
	FLEXT_CALLBACK_I(m_geti)
	FLEXT_CALLBACK(m_getall)
	FLEXT_CALLBACK_V(m_getrec)
	FLEXT_CALLBACK_V(m_getsub)
	FLEXT_CALLBACK_V(m_ogetall)
	FLEXT_CALLBACK_V(m_ogetrec)
	FLEXT_CALLBACK_V(m_ogetsub)
	FLEXT_CALLBACK(m_cntall)
	FLEXT_CALLBACK_V(m_cntrec)
	FLEXT_CALLBACK_V(m_cntsub)
	FLEXT_CALLBACK(m_printall)
	FLEXT_CALLBACK_V(m_printrec)
	FLEXT_CALLBACK(m_printroot)

	FLEXT_CALLBACK_V(m_paste)
	FLEXT_CALLBACK_V(m_pasteadd)
	FLEXT_CALLBACK(m_clrclip)
	FLEXT_CALLBACK_V(m_copy)
	FLEXT_CALLBACK_V(m_cut)
	FLEXT_CALLBACK(m_copyall)
	FLEXT_CALLBACK(m_cutall)
	FLEXT_CALLBACK_V(m_copyrec)
	FLEXT_CALLBACK_V(m_cutrec)

	FLEXT_CALLBACK_V(m_load)
	FLEXT_CALLBACK_V(m_save)
	FLEXT_CALLBACK_V(m_lddir)
	FLEXT_CALLBACK_V(m_ldrec)
	FLEXT_CALLBACK_V(m_svdir)
	FLEXT_CALLBACK_V(m_svrec)
	FLEXT_CALLBACK_V(m_loadx)
	FLEXT_CALLBACK_V(m_savex)
	FLEXT_CALLBACK_V(m_ldxdir)
	FLEXT_CALLBACK_V(m_ldxrec)
	FLEXT_CALLBACK_V(m_svxdir)
	FLEXT_CALLBACK_V(m_svxrec)
};

FLEXT_NEW_V("pool",pool);


pool::PoolMap pool::poolmap;	
const t_symbol *pool::sym_echo,*pool::sym_error;
const t_symbol *pool::holdname;


void pool::setup(t_classid c)
{
	post("");
    pool::m_help();
	post("");

    sym_echo = MakeSymbol("echo");
    sym_error = MakeSymbol("error");

	FLEXT_CADDATTR_VAR(c,"pool",mg_pool,ms_pool);
	FLEXT_CADDATTR_VAR(c,"curdir",curdir,ms_curdir);
	FLEXT_CADDATTR_VAR1(c,"absdir",absdir);
	FLEXT_CADDATTR_VAR1(c,"echodir",echo);
	FLEXT_CADDATTR_GET(c,"private",mg_priv);
	FLEXT_CADDATTR_VAR1(c,"valcnt",vcnt);
	FLEXT_CADDATTR_VAR1(c,"dircnt",dcnt);

	FLEXT_CADDMETHOD_(c,0,"help",m_help);
	FLEXT_CADDMETHOD_(c,0,"reset",m_reset);
	FLEXT_CADDMETHOD_(c,0,"getdir",m_getdir);
	FLEXT_CADDMETHOD_(c,0,"mkdir",m_mkdir);
	FLEXT_CADDMETHOD_(c,0,"chdir",m_chdir);
	FLEXT_CADDMETHOD_(c,0,"mkchdir",m_mkchdir);
	FLEXT_CADDMETHOD_(c,0,"rmdir",m_rmdir);
	FLEXT_CADDMETHOD_(c,0,"updir",m_updir);
	FLEXT_CADDMETHOD_(c,0,"mksub",m_mksub);
	FLEXT_CADDMETHOD_(c,0,"chsub",m_chsub);
	FLEXT_CADDMETHOD_(c,0,"mkchsub",m_mkchsub);
	FLEXT_CADDMETHOD_(c,0,"rmsub",m_rmsub);

	FLEXT_CADDMETHOD_(c,0,"set",m_set);
	FLEXT_CADDMETHOD_(c,0,"seti",m_seti);
	FLEXT_CADDMETHOD_(c,0,"add",m_add);
	FLEXT_CADDMETHOD_(c,0,"clr",m_clr);
	FLEXT_CADDMETHOD_(c,0,"clri",m_clri);
	FLEXT_CADDMETHOD_(c,0,"clrall",m_clrall);
	FLEXT_CADDMETHOD_(c,0,"clrrec",m_clrrec);
	FLEXT_CADDMETHOD_(c,0,"clrsub",m_clrsub);
	FLEXT_CADDMETHOD_(c,0,"get",m_get);
	FLEXT_CADDMETHOD_(c,0,"geti",m_geti);
	FLEXT_CADDMETHOD_(c,0,"getall",m_getall);
	FLEXT_CADDMETHOD_(c,0,"getrec",m_getrec);
	FLEXT_CADDMETHOD_(c,0,"getsub",m_getsub);
	FLEXT_CADDMETHOD_(c,0,"ogetall",m_ogetall);
	FLEXT_CADDMETHOD_(c,0,"ogetrec",m_ogetrec);
	FLEXT_CADDMETHOD_(c,0,"ogetsub",m_ogetsub);
	FLEXT_CADDMETHOD_(c,0,"cntall",m_cntall);
	FLEXT_CADDMETHOD_(c,0,"cntrec",m_cntrec);
	FLEXT_CADDMETHOD_(c,0,"cntsub",m_cntsub);

	FLEXT_CADDMETHOD_(c,0,"printall",m_printall);
	FLEXT_CADDMETHOD_(c,0,"printrec",m_printrec);
	FLEXT_CADDMETHOD_(c,0,"printroot",m_printroot);

	FLEXT_CADDMETHOD_(c,0,"paste",m_paste);
	FLEXT_CADDMETHOD_(c,0,"pasteadd",m_pasteadd);
	FLEXT_CADDMETHOD_(c,0,"clrclip",m_clrclip);
	FLEXT_CADDMETHOD_(c,0,"cut",m_cut);
	FLEXT_CADDMETHOD_(c,0,"copy",m_copy);
	FLEXT_CADDMETHOD_(c,0,"cutall",m_cutall);
	FLEXT_CADDMETHOD_(c,0,"copyall",m_copyall);
	FLEXT_CADDMETHOD_(c,0,"cutrec",m_cutrec);
	FLEXT_CADDMETHOD_(c,0,"copyrec",m_copyrec);

	FLEXT_CADDMETHOD_(c,0,"load",m_load);
	FLEXT_CADDMETHOD_(c,0,"save",m_save);
	FLEXT_CADDMETHOD_(c,0,"lddir",m_lddir);
	FLEXT_CADDMETHOD_(c,0,"ldrec",m_ldrec);
	FLEXT_CADDMETHOD_(c,0,"svdir",m_svdir);
	FLEXT_CADDMETHOD_(c,0,"svrec",m_svrec);
	FLEXT_CADDMETHOD_(c,0,"loadx",m_loadx);
	FLEXT_CADDMETHOD_(c,0,"savex",m_savex);
	FLEXT_CADDMETHOD_(c,0,"ldxdir",m_ldxdir);
	FLEXT_CADDMETHOD_(c,0,"ldxrec",m_ldxrec);
	FLEXT_CADDMETHOD_(c,0,"svxdir",m_svxdir);
	FLEXT_CADDMETHOD_(c,0,"svxrec",m_svxrec);
}

pool::pool(int argc,const t_atom *argv):
	absdir(true),echo(false),
    pl(NULL),
	clip(NULL),
	vcnt(VCNT),dcnt(DCNT)
{
	holdname = argc >= 1 && IsSymbol(argv[0])?GetSymbol(argv[0]):NULL;

	AddInAnything("Commands in");
	AddOutList();
	AddOutAnything();
	AddOutList();
	AddOutAnything();
}

pool::~pool()
{
	FreePool();
}

bool pool::Init()
{
	if(flext_base::Init()) {
		SetPool(holdname);	
		return true;
	}
	else return false;
}

void pool::SetPool(const t_symbol *s)
{
	if(s) {
		if(pl)
			// check if new symbol equals the current one
			if(pl->sym == s) 
				return;
			else
				FreePool();
		pl = GetPool(s);
	}
	else {
        if(pl) {
    		// if already private no need to allocate new storage
            if(pl->Private()) 
                return;
            else
    		    FreePool();
        }
		pl = new pooldata(NULL,vcnt,dcnt);
	}
}

void pool::FreePool()
{
	curdir(); // reset current directory

	if(pl) {
		if(!pl->Private()) 
			RmvPool(pl);
		else
			delete pl;
		pl = NULL;
	}

	if(clip) { delete clip; clip = NULL; }
}

void pool::ms_pool(const AtomList &l) 
{
	const t_symbol *s = NULL;
	if(l.Count()) {
		if(l.Count() > 1) post("%s - pool: superfluous arguments ignored",thisName());
		s = GetASymbol(l[0]);
		if(!s) post("%s - pool: invalid pool name, pool set to private",thisName());
	}

	SetPool(s);
}

void pool::mg_pool(AtomList &l)
{
	if(!pl || pl->Private()) l();
	else { l(1); SetSymbol(l[0],pl->sym); }
}

void pool::m_reset() 
{
    curdir();
	pl->Reset();
}


void pool::getdir(const t_symbol *tag)
{
	ToSysAnything(3,tag,0,NULL);
	ToSysList(2,curdir);
}

void pool::m_getdir() { getdir(thisTag()); }

void pool::m_mkdir(int argc,const t_atom *argv,bool abs,bool chg)
{
//    const char *nm = chg?"mkchdir":"mkdir";
	if(!ValChk(argc,argv))
		post("%s - %s: invalid directory name",thisName(),GetString(thisTag()));
	else {
		Atoms ndir;
		if(abs) ndir(argc,argv);
		else (ndir = curdir).Append(argc,argv);
		if(!pl->MkDir(ndir,vcnt,dcnt)) {
			post("%s - %s: directory couldn't be created",thisName(),GetString(thisTag()));
		}
        else if(chg) 
            // change to newly created directory
            curdir = ndir;
	}

	echodir();
}

void pool::m_chdir(int argc,const t_atom *argv,bool abs)
{
	if(!ValChk(argc,argv)) 
		post("%s - %s: invalid directory name",thisName(),GetString(thisTag()));
	else {
		Atoms prv(curdir);
		if(abs) curdir(argc,argv);
		else curdir.Append(argc,argv);
		if(!pl->ChkDir(curdir)) {
			post("%s - %s: directory couldn't be changed",thisName(),GetString(thisTag()));
			curdir = prv;
		}
	}

	echodir();
}

void pool::m_updir(int argc,const t_atom *argv)
{
	int lvls = 1;
	if(argc > 0) {
		if(CanbeInt(argv[0])) {
			if(argc > 1)
				post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
			lvls = GetAInt(argv[0]);
			if(lvls < 0)
				post("%s - %s: invalid level specification - set to 1",thisName(),GetString(thisTag()));
		}
		else
			post("%s - %s: invalid level specification - set to 1",thisName(),GetString(thisTag()));
	}

	Atoms prv(curdir);

	if(lvls > curdir.Count()) {
		post("%s - %s: level exceeds directory depth - corrected",thisName(),GetString(thisTag()));
		curdir();
	}
	else
		curdir.Part(0,curdir.Count()-lvls);

	if(!pl->ChkDir(curdir)) {
		post("%s - %s: directory couldn't be changed",thisName(),GetString(thisTag()));
		curdir = prv;
	}

	echodir();
}

void pool::m_rmdir(int argc,const t_atom *argv,bool abs)
{
	if(abs) curdir(argc,argv);
	else curdir.Append(argc,argv);

	if(!pl->RmDir(curdir)) 
		post("%s - %s: directory couldn't be removed",thisName(),GetString(thisTag()));
	curdir();

	echodir();
}

void pool::set(int argc,const t_atom *argv,bool over)
{
	if(!argc || !KeyChk(argv[0])) 
		post("%s - %s: invalid key",thisName(),GetString(thisTag()));
	else if(!ValChk(argc-1,argv+1)) {
		post("%s - %s: invalid data values",thisName(),GetString(thisTag()));
	}
	else 
		if(!pl->Set(curdir,argv[0],new AtomList(argc-1,argv+1),over))
			post("%s - %s: value couldn't be set",thisName(),GetString(thisTag()));

	echodir();
}

void pool::m_seti(int argc,const t_atom *argv)
{
	if(!argc || !CanbeInt(argv[0])) 
		post("%s - %s: invalid index",thisName(),GetString(thisTag()));
	else if(!ValChk(argc-1,argv+1)) {
		post("%s - %s: invalid data values",thisName(),GetString(thisTag()));
	}
	else 
		if(!pl->Seti(curdir,GetAInt(argv[0]),new Atoms(argc-1,argv+1)))
			post("%s - %s: value couldn't be set",thisName(),GetString(thisTag()));

	echodir();
}

void pool::m_clr(int argc,const t_atom *argv)
{
	if(!argc || !KeyChk(argv[0]))
		post("%s - %s: invalid key",thisName(),GetString(thisTag()));
	else {
		if(argc > 1) 
			post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));

		if(!pl->Clr(curdir,argv[0]))
			post("%s - %s: value couldn't be cleared",thisName(),GetString(thisTag()));
	}

	echodir();
}

void pool::m_clri(int ix)
{
	if(ix < 0)
		post("%s - %s: invalid index",thisName(),GetString(thisTag()));
	else {
		if(!pl->Clri(curdir,ix))
			post("%s - %s: value couldn't be cleared",thisName(),GetString(thisTag()));
	}

	echodir();
}

void pool::m_clrall()
{
	if(!pl->ClrAll(curdir,false))
		post("%s - %s: values couldn't be cleared",thisName(),GetString(thisTag()));

	echodir();
}

void pool::m_clrrec()
{
	if(!pl->ClrAll(curdir,true))
		post("%s - %s: values couldn't be cleared",thisName(),GetString(thisTag()));

	echodir();
}

void pool::m_clrsub()
{
	if(!pl->ClrAll(curdir,true,true))
		post("%s - %s: directories couldn't be cleared",thisName(),GetString(thisTag()));

	echodir();
}

void pool::m_get(int argc,const t_atom *argv)
{
	if(!argc || !KeyChk(argv[0]))
		post("%s - %s: invalid key",thisName(),GetString(thisTag()));
	else {
		if(argc > 1) 
			post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));

		poolval *r = pl->Ref(curdir,argv[0]);

		ToSysAnything(3,thisTag(),0,NULL);
		if(absdir)
			ToSysList(2,curdir);
		else
			ToSysList(2,0,NULL);
		if(r) {
			ToOutAtom(1,r->key);
			ToSysList(0,*r->data);
		}
		else {
			ToSysBang(1);
			ToSysBang(0);
		}
	}

	echodir();
}

void pool::m_geti(int ix)
{
	if(ix < 0)
		post("%s - %s: invalid index",thisName(),GetString(thisTag()));
	else {
		poolval *r = pl->Refi(curdir,ix);

		ToSysAnything(3,thisTag(),0,NULL);
		if(absdir)
			ToSysList(2,curdir);
		else
			ToSysList(2,0,NULL);
		if(r) {
			ToOutAtom(1,r->key);
			ToSysList(0,*r->data);
		}
		else {
			ToSysBang(1);
			ToSysBang(0);
		}
	}

	echodir();
}


// ---- some sorting stuff ----------------------------------

inline bool smaller(const t_atom &a,const t_atom &b,int index) { return a < b; }
inline void swap(t_atom &a,t_atom &b) { t_atom c = a; a = b; b = c; }

inline bool smaller(const t_atom *a,const t_atom *b,int index) { return *a < *b; }
inline void swap(t_atom *a,t_atom *b) { t_atom *c = a; a = b; b = c; }

inline bool smaller(const Atoms &a,const Atoms &b,int index) 
{ 
	if(a.Count()-1 < index)
		return true;
	else if(b.Count()-1 < index)
		return false;
	else
		return a[index] < b[index];
}

inline void swap(Atoms &a,Atoms &b) { Atoms c(a); a = b; b = c; }

inline bool smaller(const Atoms *a,const Atoms *b,int index) { return smaller(*a,*b,index); }
inline void swap(Atoms *a,Atoms *b) { Atoms *c = a; a = b; b = c; }

template <typename T1,typename T2>
void sift(T1 *a,T2 *b,int start,int count,int index,bool rev) 
{
	int root = start;                    // Point to a root node
	int child;

	while((child = root * 2 + 1) < count) {             // While the root has child(ren) point to its left child
		// If the child has a sibling and the child's value is less than its sibling's...
		if(child < count-1 && smaller(a[child],a[child+1],index) != rev)
			child++;                // ... point to the right child instead
			 
		if(smaller(a[root],a[child],index) == rev) break;
		
		// If the value in root is less than in child...
		swap(a[root], a[child]);           // ... swap the values in root and child and...
		if(b) swap(b[root], b[child]);

		root = child;                // ... make root point to its child
	}
}

template <typename T1,typename T2>
void heapsort(T1 *a,T2 *b,int count,int index,bool rev) 
{
	int start = count/2-1;
	int end = count-1;

	for(; start >= 0; start--)
		sift(a, b, start, count, index, rev);

	for(; end > 0; --end) {
		swap(a[end], a[0]);
		if(b) swap(b[end], b[0]);
		sift(a, b, 0, end, index, rev);
	}
}
 
template <typename T1,typename T2>
static void orderpairs(T1 *keys,T2 *atoms,int count,int index,bool rev)
{
	FLEXT_ASSERT(index >= 0);

	if(!count) return;
	
	if(index)
		heapsort(atoms,keys,count,index-1,rev);
	else
		heapsort(keys,atoms,count,0,rev);
}

// ---- sorting stuff ends ----------------------------------

int pool::getrec(const t_symbol *tag,int level,int order,bool rev,get_t how,const AtomList &rdir)
{
	Atoms gldir(curdir);
	gldir.Append(rdir);

	int ret = 0;

    switch(how) {
		case get_cnt:
			ret = pl->CntAll(gldir);
			break;
		case get_print:
			ret = pl->PrintAll(gldir);
			break;
		case get_norm: {
			t_atom *k;
			Atoms *r;
			int cnt = pl->GetAll(gldir,k,r);
			if(!k) {
				FLEXT_ASSERT(!k);
				post("%s - %s: error retrieving values",thisName(),GetString(tag));
			}
			else {
				FLEXT_ASSERT(r);
			
				if(order >= 0)
					orderpairs(k,r,cnt,order,rev);
			
				for(int i = 0; i < cnt; ++i) {
					ToSysAnything(3,tag,0,NULL);
					ToSysList(2,absdir?gldir:rdir);
					ToOutAtom(1,k[i]);
					ToSysList(0,r[i]);
				}
				delete[] k;
				delete[] r;
			}
			ret = cnt;
		}
	}

	if(level != 0) {
		const t_atom **r;
		int cnt = pl->GetSub(gldir,r);
		if(!r) 
			post("%s - %s: error retrieving directories",thisName(),GetString(tag));
		else {
			if(order >= 0)
				orderpairs(r,(Atoms *)NULL,cnt,order,rev);

			int lv = level > 0?level-1:-1;
			for(int i = 0; i < cnt; ++i) {
				Atoms l(rdir); l.Append(*r[i]);
				ret += getrec(tag,lv,order,rev,how,l);
			}
			delete[] r;
		}
	}
	
	return ret;
}

void pool::m_getall()
{
	AtomList l;
	getrec(thisTag(),0,-1,false,get_norm,l);
	ToSysBang(3);

	echodir();
}

void pool::m_ogetall(int argc,const t_atom *argv)
{
	int index = 0;
	if(argc) {
		if(!CanbeInt(*argv) || (index = GetAInt(*argv)) < 0) {
			index = 0;
			post("%s - %s: invalid sort index specification - set to 0",thisName(),GetString(thisTag()));
		}
		--argc,++argv;
	}

	bool rev = false;
	if(argc) {
		if(!CanbeBool(*argv))
			post("%s - %s: invalid sort direction specification - set to forward",thisName(),GetString(thisTag()));
		else
			rev = GetABool(*argv);
		--argc,++argv;
	}

	if(argc)
		post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
	
	AtomList l;
	getrec(thisTag(),0,index,rev,get_norm,l);
	ToSysBang(3);

	echodir();
}

void pool::m_getrec(int argc,const t_atom *argv)
{
	int lvls = -1;
	if(argc) {
		if(!CanbeInt(*argv) || (lvls = GetAInt(*argv)) < -1) {
			lvls = -1;
			post("%s - %s: invalid level specification - set to %i",thisName(),GetString(thisTag()),lvls);
		}
		--argc,++argv;
	}

	if(argc)
		post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
	
	AtomList l;
	getrec(thisTag(),lvls,-1,false,get_norm,l);
	ToSysBang(3);

	echodir();
}


void pool::m_ogetrec(int argc,const t_atom *argv)
{
	int lvls = -1;
	if(argc) {
		if(!CanbeInt(*argv) || (lvls = GetAInt(*argv)) < -1) {
			lvls = -1;
			post("%s - %s: invalid level specification - set to %i",thisName(),GetString(thisTag()),lvls);
		}
		--argc,++argv;
	}

	int index = 0;
	if(argc) {
		if(!CanbeInt(*argv) || (index = GetAInt(*argv)) < 0) {
			index = 0;
			post("%s - %s: invalid sort index specification - set to 0",thisName(),GetString(thisTag()));
		}
		--argc,++argv;
	}

	bool rev = false;
	if(argc) {
		if(!CanbeBool(*argv))
			post("%s - %s: invalid sort direction specification - set to forward",thisName(),GetString(thisTag()));
		else
			rev = GetABool(*argv);
		--argc,++argv;
	}

	if(argc)
		post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
	
	AtomList l;
	getrec(thisTag(),lvls,index,rev,get_norm,l);
	ToSysBang(3);

	echodir();
}


int pool::getsub(const t_symbol *tag,int level,int order,bool rev,get_t how,const AtomList &rdir)
{
	Atoms gldir(curdir);
	gldir.Append(rdir);
	
	int ret = 0;

	const t_atom **r = NULL;
	// CntSub is not used here because it doesn't allow checking for valid directory
	int cnt = pl->GetSub(gldir,r);
	if(!r) 
		post("%s - %s: error retrieving directories",thisName(),GetString(tag));
	else {
		if(order >= 0)
			orderpairs(r,(Atoms *)NULL,cnt,order,rev);

		int lv = level > 0?level-1:-1;
		for(int i = 0; i < cnt; ++i) {
			Atoms ndir(absdir?gldir:rdir);
			ndir.Append(*r[i]);
            ++ret;

			if(how == get_norm) {
				ToSysAnything(3,tag,0,NULL);
				ToSysList(2,curdir);
				ToSysList(1,ndir);
				ToSysBang(0);
			}

			if(level != 0) {
				AtomList l(rdir); l.Append(*r[i]);
				ret += getsub(tag,lv,order,rev,how,l);
			}
		}
		delete[] r;
	}
	
	return ret;
}

void pool::m_getsub(int argc,const t_atom *argv)
{
	int lvls = 0;
	if(argc) {
		if(!CanbeInt(*argv) || (lvls = GetAInt(*argv)) < -1) {
			lvls = 0;
			post("%s - %s: invalid level specification - set to %i",thisName(),GetString(thisTag()),lvls);
		}
		--argc,++argv;
	}

	if(argc)
		post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
	
	AtomList l;
	getsub(thisTag(),lvls,-1,false,get_norm,l);
	ToSysBang(3);

	echodir();
}


void pool::m_ogetsub(int argc,const t_atom *argv)
{
	int lvls = 0;
	if(argc) {
		if(!CanbeInt(*argv) || (lvls = GetAInt(*argv)) < -1) {
			lvls = 0;
			post("%s - %s: invalid level specification - set to %i",thisName(),GetString(thisTag()),lvls);
		}
		--argc,++argv;
	}

	int index = 0;
	if(argc) {
		if(!CanbeInt(*argv) || (index = GetAInt(*argv)) < 0) {
			index = 0;
			post("%s - %s: invalid sort index specification - set to 0",thisName(),GetString(thisTag()));
		}
		--argc,++argv;
	}

	bool rev = false;
	if(argc) {
		if(!CanbeBool(*argv))
			post("%s - %s: invalid sort direction specification - set to forward",thisName(),GetString(thisTag()));
		else
			rev = GetABool(*argv);
		--argc,++argv;
	}

	if(argc)
		post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
	
	AtomList l;
	getsub(thisTag(),lvls,index,rev,get_norm,l); 
	ToSysBang(3);

	echodir();
}


void pool::m_cntall()
{
	AtomList l;
	int cnt = getrec(thisTag(),0,-1,false,get_cnt,l);
	ToSysSymbol(3,thisTag());
	ToSysBang(2);
	ToSysBang(1);
	ToSysInt(0,cnt);

	echodir();
}

void pool::m_cntrec(int argc,const t_atom *argv)
{
	int lvls = -1;
	if(argc) {
		if(!CanbeInt(*argv) || (lvls = GetAInt(*argv)) < -1) {
			lvls = -1;
			post("%s - %s: invalid level specification - set to %i",thisName(),GetString(thisTag()),lvls);
		}
		--argc,++argv;
	}

	if(argc)
		post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
	
	AtomList l;
	int cnt = getrec(thisTag(),lvls,-1,false,get_cnt,l);
	ToSysSymbol(3,thisTag());
	ToSysBang(2);
	ToSysBang(1);
	ToSysInt(0,cnt);

	echodir();
}


void pool::m_cntsub(int argc,const t_atom *argv)
{
	int lvls = 0;
	if(argc) {
		if(!CanbeInt(*argv) || (lvls = GetAInt(*argv)) < -1) {
			lvls = 0;
			post("%s - %s: invalid level specification - set to %i",thisName(),GetString(thisTag()),lvls);
		}
		--argc,++argv;
	}

	if(argc)
		post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
	
	AtomList l;
	int cnt = getsub(thisTag(),lvls,-1,false,get_cnt,l);
	ToSysSymbol(3,thisTag());
	ToSysBang(2);
	ToSysBang(1);
	ToSysInt(0,cnt);

	echodir();
}

void pool::m_printall()
{
	AtomList l;
	int cnt = getrec(thisTag(),0,-1,false,get_print,l);
    post("");
}

void pool::m_printrec(int argc,const t_atom *argv,bool fromroot)
{
    const t_symbol *tag = thisTag();
	int lvls = -1;
	
	if(argc) {
		if(!CanbeInt(*argv) || (lvls = GetAInt(*argv)) < -1) {
			lvls = 0;
			post("%s - %s: invalid level specification - set to %i",thisName(),GetString(tag),lvls);
		}
		--argc,++argv;
	}

	if(argc)
		post("%s - %s: superfluous arguments ignored",thisName(),GetString(tag));
	
	Atoms svdir(curdir);
    if(fromroot) curdir.Clear();

	AtomList l;
	int cnt = getrec(tag,lvls,-1,false,get_print,l);
    post("");

    curdir = svdir;
}


void pool::paste(const t_symbol *tag,int argc,const t_atom *argv,bool repl)
{
	if(clip) {
		bool mkdir = true;
		int depth = -1;

		if(argc >= 1) {
			if(CanbeInt(argv[0])) depth = GetAInt(argv[1]);
			else
				post("%s - %s: invalid depth argument - set to -1",thisName(),GetString(tag));

			if(argc >= 2) {
				if(CanbeBool(argv[1])) mkdir = GetABool(argv[1]);
				else
					post("%s - %s: invalid mkdir argument - set to true",thisName(),GetString(tag));

				if(argc > 2) post("%s - %s: superfluous arguments ignored",thisName(),GetString(tag));
			}
		}
		
		pl->Paste(curdir,clip,depth,repl,mkdir);
	}
	else
		post("%s - %s: clipboard is empty",thisName(),GetString(tag));

	echodir();
}


void pool::m_clrclip()
{
	if(clip) { delete clip; clip = NULL; }
}


void pool::copy(const t_symbol *tag,int argc,const t_atom *argv,bool cut)
{
	if(!argc || !KeyChk(argv[0]))
		post("%s - %s: invalid key",thisName(),GetString(tag));
	else {
		if(argc > 1) 
			post("%s - %s: superfluous arguments ignored",thisName(),GetString(tag));

		m_clrclip();
		clip = pl->Copy(curdir,argv[0],cut);

		if(!clip)
			post("%s - %s: Copying into clipboard failed",thisName(),GetString(tag));
	}

	echodir();
}


void pool::copyall(const t_symbol *tag,bool cut,int depth)
{
	m_clrclip();
	clip = pl->CopyAll(curdir,depth,cut);

	if(!clip)
		post("%s - %s: Copying into clipboard failed",thisName(),GetString(tag));

	echodir();
}


void pool::copyrec(const t_symbol *tag,int argc,const t_atom *argv,bool cut) 
{
	int lvls = -1;
	if(argc > 0) {
		if(CanbeInt(argv[0])) {
			if(argc > 1)
				post("%s - %s: superfluous arguments ignored",thisName(),GetString(tag));
			lvls = GetAInt(argv[0]);
		}
		else 
			post("%s - %s: invalid level specification - set to infinite",thisName(),GetString(tag));
	}

	copyall(tag,cut,lvls);
}

void pool::load(int argc,const t_atom *argv,bool xml)
{
    const char *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

    bool ok = false;
	if(!flnm) 
		post("%s - %s: no filename given",thisName(),GetString(thisTag()));
	else {
		string file(MakeFilename(flnm));
        ok = xml?pl->LoadXML(file.c_str()):pl->Load(file.c_str());
		if(!ok)
			post("%s - %s: error loading data",thisName(),GetString(thisTag()));
	}

    t_atom at; SetBool(at,ok);
    ToOutAnything(GetOutAttr(),thisTag(),1,&at);

	echodir();
}

void pool::save(int argc,const t_atom *argv,bool xml)
{
	const char *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

    bool ok = false;
	if(!flnm) 
		post("%s - %s: no filename given",thisName(),GetString(thisTag()));
	else {
		string file(MakeFilename(flnm));
        ok = xml?pl->SaveXML(file.c_str()):pl->Save(file.c_str());
		if(!ok)
			post("%s - %s: error saving data",thisName(),GetString(thisTag()));
	}

    t_atom at; SetBool(at,ok);
    ToOutAnything(GetOutAttr(),thisTag(),1,&at);

	echodir();
}

void pool::lddir(int argc,const t_atom *argv,bool xml)
{
	const char *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

    bool ok = false;
	if(!flnm)
		post("%s - %s: invalid filename",thisName(),GetString(thisTag()));
	else {
		string file(MakeFilename(flnm));
        ok = xml?pl->LdDirXML(curdir,file.c_str(),0):pl->LdDir(curdir,file.c_str(),0);
		if(!ok) 
			post("%s - %s: directory couldn't be loaded",thisName(),GetString(thisTag()));
	}

    t_atom at; SetBool(at,ok);
    ToOutAnything(GetOutAttr(),thisTag(),1,&at);

	echodir();
}

void pool::ldrec(int argc,const t_atom *argv,bool xml)
{
	const char *flnm = NULL;
	int depth = -1;
	bool mkdir = true;
	if(argc >= 1) {
		if(IsString(argv[0])) flnm = GetString(argv[0]);

		if(argc >= 2) {
			if(CanbeInt(argv[1])) depth = GetAInt(argv[1]);
			else
				post("%s - %s: invalid depth argument - set to -1",thisName(),GetString(thisTag()));

			if(argc >= 3) {
				if(CanbeBool(argv[2])) mkdir = GetABool(argv[2]);
				else
					post("%s - %s: invalid mkdir argument - set to true",thisName(),GetString(thisTag()));

				if(argc > 3) post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
			}
		}
	}

    bool ok = false;
	if(!flnm)
		post("%s - %s: invalid filename",thisName(),GetString(thisTag()));
	else {
		string file(MakeFilename(flnm));
        ok = xml?pl->LdDirXML(curdir,file.c_str(),depth,mkdir):pl->LdDir(curdir,file.c_str(),depth,mkdir);
        if(!ok) 
		    post("%s - %s: directory couldn't be saved",thisName(),GetString(thisTag()));
	}

    t_atom at; SetBool(at,ok);
    ToOutAnything(GetOutAttr(),thisTag(),1,&at);

	echodir();
}

void pool::svdir(int argc,const t_atom *argv,bool xml)
{
	const char *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

    bool ok = false;
	if(!flnm)
		post("%s - %s: invalid filename",thisName(),GetString(thisTag()));
	else {
		string file(MakeFilename(flnm));
        ok = xml?pl->SvDirXML(curdir,file.c_str(),0,absdir):pl->SvDir(curdir,file.c_str(),0,absdir);
        if(!ok) 
		    post("%s - %s: directory couldn't be saved",thisName(),GetString(thisTag()));
	}

    t_atom at; SetBool(at,ok);
    ToOutAnything(GetOutAttr(),thisTag(),1,&at);

	echodir();
}

void pool::svrec(int argc,const t_atom *argv,bool xml)
{
	const char *flnm = NULL;
	if(argc > 0) {
		if(argc > 1) post("%s - %s: superfluous arguments ignored",thisName(),GetString(thisTag()));
		if(IsString(argv[0])) flnm = GetString(argv[0]);
	}

    bool ok = false;
	if(!flnm)
		post("%s - %s: invalid filename",thisName(),GetString(thisTag()));
	else {
		string file(MakeFilename(flnm));
        ok = xml?pl->SvDirXML(curdir,file.c_str(),-1,absdir):pl->SvDir(curdir,file.c_str(),-1,absdir);
        if(!ok) 
		    post("%s - %s: directory couldn't be saved",thisName(),GetString(thisTag()));
	}

    t_atom at; SetBool(at,ok);
    ToOutAnything(GetOutAttr(),thisTag(),1,&at);

	echodir();
}



bool pool::KeyChk(const t_atom &a)
{
	return IsSymbol(a) || IsFloat(a) || IsInt(a);
}

bool pool::ValChk(int argc,const t_atom *argv)
{
	for(int i = 0; i < argc; ++i) {
		const t_atom &a = argv[i];
		if(!IsSymbol(a) && !IsFloat(a) && !IsInt(a)) return false;
	}
	return true;
}

void pool::ToOutAtom(int ix,const t_atom &a)
{
	if(IsSymbol(a))
		ToSysSymbol(ix,GetSymbol(a));
	else if(IsFloat(a))
		ToSysFloat(ix,GetFloat(a));
	else if(IsInt(a))
		ToSysInt(ix,GetInt(a));
	else
		post("%s - %s type not supported!",thisName(),GetString(thisTag()));
}



pooldata *pool::GetPool(const t_symbol *s)
{
    PoolMap::iterator it = poolmap.find(s);
    pooldata *p;   
	if(it != poolmap.end())
        p = it->second;
	else
		poolmap[s] = p = new pooldata(s);
    p->Push();
	return p;
}

void pool::RmvPool(pooldata *p)
{
    FLEXT_ASSERT(p->sym);
    PoolMap::iterator it = poolmap.find(p->sym);
    FLEXT_ASSERT(it != poolmap.end());
    FLEXT_ASSERT(p == it->second);
	if(!p->Pop()) {
        poolmap.erase(it);
		delete p;
	}
}

string pool::MakeFilename(const char *fn) const
{
#if FLEXT_SYS == FLEXT_SYS_PD
    // / and \ must not be mixed!
    // (char *) type casts for BorlandC++
	char *sl = strchr((char *)fn,'/');
	if(!sl) sl = strchr((char *)fn,'\\');
    if(!sl || (sl != fn 
#if FLEXT_OS == FLEXT_OS_WIN
        && sl[-1] != ':' // look for drive specification with ":/" or ":\\"
#endif
    )) {
        // prepend absolute canvas path if filename has no absolute path
		const char *p = GetString(canvas_getdir(thisCanvas()));
		return string(p)+'/'+fn;
	}
	else
		return fn;
#else
#pragma message("Relative file paths not implemented")
	return fn;
#endif
}
