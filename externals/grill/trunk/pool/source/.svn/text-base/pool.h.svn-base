/* 
pool - hierarchical storage object for PD and Max/MSP

Copyright (c) 2002-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 26 $
$LastChangedDate$
$LastChangedBy$
*/

#ifndef __POOL_H
#define __POOL_H

#define FLEXT_ATTRIBUTES 1

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5.0
#endif

#include <iostream>

using namespace std;


typedef flext::AtomListStatic<8> Atoms;


class poolval:
	public flext
{
public:
	poolval(const t_atom &key,AtomList *data);
	~poolval();

	poolval &Set(AtomList *data);
	poolval *Dup() const;

	t_atom key;
	AtomList *data;
	poolval *nxt;
};

class pooldir:
	public flext
{
public:
	pooldir(const t_atom &dir,pooldir *parent,int vcnt,int dcnt);
	~pooldir();

	void Clear(bool rec,bool dironly = false);
	void Reset(bool realloc = true);

	bool Empty() const { return !dirs && !vals; }
	bool HasDirs() const { return dirs != NULL; }
	bool HasVals() const { return vals != NULL; }

	pooldir *GetDir(int argc,const t_atom *argv,bool cut = false);
	pooldir *GetDir(const AtomList &d,bool cut = false) { return GetDir(d.Count(),d.Atoms(),cut); }
	bool DelDir(int argc,const t_atom *argv);
	bool DelDir(const AtomList &d) { return DelDir(d.Count(),d.Atoms()); }
	pooldir *AddDir(int argc,const t_atom *argv,int vcnt = 0,int dcnt = 0);
	pooldir *AddDir(const AtomList &d,int vcnt = 0,int dcnt = 0) { return AddDir(d.Count(),d.Atoms(),vcnt,dcnt); }

	void SetVal(const t_atom &key,AtomList *data,bool over = true);
	bool SetVali(int ix,AtomList *data);
	void ClrVal(const t_atom &key) { SetVal(key,NULL); }
    bool ClrVali(int ix) { return SetVali(ix,NULL); }
	AtomList *PeekVal(const t_atom &key);
	AtomList *GetVal(const t_atom &key,bool cut = false);
	int CntAll() const;
	int GetAll(t_atom *&keys,Atoms *&lst,bool cut = false);
	int PrintAll(char *buf,int len) const;
	int GetKeys(AtomList &keys);
	int CntSub() const;
	int GetSub(const t_atom **&dirs);

	poolval *RefVal(const t_atom &key);
	poolval *RefVali(int ix);
	
	bool Paste(const pooldir *p,int depth,bool repl,bool mkdir);
	bool Copy(pooldir *p,int depth,bool cur);

	bool LdDir(istream &is,int depth,bool mkdir);
	bool LdDirXML(istream &is,int depth,bool mkdir);
	bool SvDir(ostream &os,int depth,const AtomList &dir = AtomList());
	bool SvDirXML(ostream &os,int depth,const AtomList &dir = AtomList(),int ind = 0);

	int VSize() const { return vsize; }
	int DSize() const { return dsize; }

protected:
	int VIdx(const t_atom &v) const { return FoldBits(AtomHash(v),vbits); }
	int DIdx(const t_atom &d) const { return FoldBits(AtomHash(d),dbits); }

	t_atom dir;
	pooldir *nxt;

	pooldir *parent;
	const int vbits,dbits,vsize,dsize;
	
	static unsigned int FoldBits(unsigned long h,int bits);
	static int Int2Bits(unsigned long n);

	struct valentry { int cnt; poolval *v; };
	struct direntry { int cnt; pooldir *d; };
	
	valentry *vals;
	direntry *dirs;

private:
  	bool LdDirXMLRec(istream &is,int depth,bool mkdir,AtomList &d);
};


class pooldata:
	public flext
{
public:
	pooldata(const t_symbol *s = NULL,int vcnt = 0,int dcnt = 0);
	~pooldata();

    bool Private() const { return sym == NULL; }

	void Push() { ++refs; }
	bool Pop() { return --refs > 0; }

    void Reset() { root.Reset(); }

    bool MkDir(const AtomList &d,int vcnt = 0,int dcnt = 0) 
    { 
        root.AddDir(d,vcnt,dcnt); 
        return true; 
    }

    bool ChkDir(const AtomList &d) 
    { 
        return root.GetDir(d) != NULL; 
    }

    bool RmDir(const AtomList &d) 
    { 
        return root.DelDir(d); 
    }

    bool Set(const AtomList &d,const t_atom &key,AtomList *data,bool over = true)
    {
	    pooldir *pd = root.GetDir(d);
	    if(!pd) return false;
	    pd->SetVal(key,data,over);
	    return true;
    }

    bool Seti(const AtomList &d,int ix,AtomList *data)
    {
	    pooldir *pd = root.GetDir(d);
	    if(!pd) return false;
	    pd->SetVali(ix,data);
	    return true;
    }

	bool Clr(const AtomList &d,const t_atom &key)
    {
	    pooldir *pd = root.GetDir(d);
	    if(!pd) return false;
	    pd->ClrVal(key);
	    return true;
    }

	bool Clri(const AtomList &d,int ix)
    {
	    pooldir *pd = root.GetDir(d);
	    if(!pd) return false;
	    pd->ClrVali(ix);
	    return true;
    }

	bool ClrAll(const AtomList &d,bool rec,bool dironly = false)
    {
	    pooldir *pd = root.GetDir(d);
	    if(!pd) return false;
	    pd->Clear(rec,dironly);
	    return true;
    }

	AtomList *Peek(const AtomList &d,const t_atom &key)
    {
	    pooldir *pd = root.GetDir(d);
	    return pd?pd->PeekVal(key):NULL;
    }

	AtomList *Get(const AtomList &d,const t_atom &key)
    {
	    pooldir *pd = root.GetDir(d);
	    return pd?pd->GetVal(key):NULL;
    }

	poolval *Ref(const AtomList &d,const t_atom &key)
    {
	    pooldir *pd = root.GetDir(d);
	    return pd?pd->RefVal(key):NULL;
    }

	poolval *Refi(const AtomList &d,int ix)
    {
	    pooldir *pd = root.GetDir(d);
	    return pd?pd->RefVali(ix):NULL;
    }

	int CntAll(const AtomList &d)
    {
	    pooldir *pd = root.GetDir(d);
	    return pd?pd->CntAll():0;
    }

	int PrintAll(const AtomList &d);
	int GetAll(const AtomList &d,t_atom *&keys,Atoms *&lst);

    int CntSub(const AtomList &d)
    {
	    pooldir *pd = root.GetDir(d);
	    return pd?pd->CntSub():0;
    }

	int GetSub(const AtomList &d,const t_atom **&dirs);

	bool Paste(const AtomList &d,const pooldir *clip,int depth = -1,bool repl = true,bool mkdir = true);
	pooldir *Copy(const AtomList &d,const t_atom &key,bool cut);
	pooldir *CopyAll(const AtomList &d,int depth,bool cut);

	bool LdDir(const AtomList &d,const char *flnm,int depth,bool mkdir = true);
	bool SvDir(const AtomList &d,const char *flnm,int depth,bool absdir);
	bool Load(const char *flnm) { AtomList l; return LdDir(l,flnm,-1); }
	bool Save(const char *flnm) { AtomList l; return SvDir(l,flnm,-1,true); }
	bool LdDirXML(const AtomList &d,const char *flnm,int depth,bool mkdir = true);
	bool SvDirXML(const AtomList &d,const char *flnm,int depth,bool absdir);
	bool LoadXML(const char *flnm) { AtomList l; return LdDirXML(l,flnm,-1); }
	bool SaveXML(const char *flnm) { AtomList l; return SvDirXML(l,flnm,-1,true); }

	int refs;
	const t_symbol *sym;
	pooldata *nxt;

	pooldir root;

private:
	static const t_atom nullatom;
};

#endif
