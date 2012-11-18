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
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fstream>
#include <vector>

#if FLEXT_OS == FLEXT_OS_WIN
#include <windows.h> // for charset conversion functions
#elif FLEXT_OS == FLEXT_OS_MAC
#include <Carbon/Carbon.h>
#else
static bool WCStoUTF8(char *sdst,const wchar_t *src,int dstlen)
{
    unsigned char *dst = (unsigned char *)sdst;
    unsigned char *max = dst+dstlen;
    for(;;) {
        wchar_t ud = *(src++);
        if(ud < 128) {
            if(dst+1 >= max) return false;
            *(dst++) = (unsigned char)ud;
        }
        else if(ud < 2048) {
            if(dst+2 >= max) return false;
            *(dst++) = 192+(unsigned char)(ud/64);
            *(dst++) = 128+(unsigned char)(ud%64);
        }
        else if(ud < 65535) {
            if(dst+3 >= max) return false;
            *(dst++) = 224+(unsigned char)(ud/4096);
            *(dst++) = 128+(unsigned char)((ud/64)%64);
            *(dst++) = 128+(unsigned char)(ud%64);
        }
        else if(ud < 2097151) {
            if(dst+4 >= max) return false;
            *(dst++) = 240+(unsigned char)(ud/262144);
            *(dst++) = 128+(unsigned char)((ud/4096)%64);
            *(dst++) = 128+(unsigned char)((ud/64)%64);
            *(dst++) = 128+(unsigned char)(ud%64);
        }
        else if(ud < 67108863) {
            if(dst+5 >= max) return false;
            *(dst++) = 248+(unsigned char)(ud/16777216);
            *(dst++) = 128+(unsigned char)((ud/262144)%64);
            *(dst++) = 128+(unsigned char)((ud/4096)%64);
            *(dst++) = 128+(unsigned char)((ud/64)%64);
            *(dst++) = 128+(unsigned char)(ud%64);
        }
        else {
            if(dst+6 >= max) return false;
            *(dst++) = 252+(unsigned char)(ud/1073741824);
            *(dst++) = 128+(unsigned char)((ud/16777216)%64);
            *(dst++) = 128+(unsigned char)((ud/262144)%64);
            *(dst++) = 128+(unsigned char)((ud/4096)%64);
            *(dst++) = 128+(unsigned char)((ud/64)%64);
            *(dst++) = 128+(unsigned char)(ud%64);
        }
        if(!ud) break;
    }
    return true;
}

static bool UTF8toWCS(wchar_t *dst,const char *ssrc,int dstlen)
{
    const unsigned char *src = (const unsigned char *)ssrc;
    wchar_t *max = dst+dstlen;
    for(;;) {
        if(*src < 128) {
            *dst = *(src++);
            if(!*dst) break;
        }
        else if(*src < 224) {
            *dst = wchar_t(src[0]-192)*64+wchar_t(src[1]-128);
            src += 2;
        }
        else if(*src < 240) {
            *dst = wchar_t(src[0]-224)*4096+wchar_t(src[1]-128)*64+wchar_t(src[2]-128);
            src += 3;
        }
        else if(*src < 248) {
            *dst = wchar_t(src[0]-240)*262144+wchar_t(src[1]-128)*4096+wchar_t(src[2]-128)*64+wchar_t(src[3]-128);
            src += 4;
        }
        else if(*src < 252) {
            *dst = wchar_t(src[0]-248)*16777216+wchar_t(src[1]-128)*262144+wchar_t(src[2]-128)*4096+wchar_t(src[3]-128)*64+wchar_t(src[4]-128);
            src += 5;
        }
        else if(*src < 254) {
            *dst = wchar_t(src[0]-252)*1073741824+wchar_t(src[1]-128)*16777216+wchar_t(src[2]-128)*262144+wchar_t(src[3]-128)*4096+wchar_t(src[4]-128)*64+wchar_t(src[5]-128);
            src += 6;
        }
        else
			// invalid string
            return false;

        if(++dst >= max) return false;
    }
    return true;
}

#endif

using namespace std;



inline int compare(int a,int b) { return a == b?0:(a < b?-1:1); }
inline int compare(float a,float b) { return a == b?0:(a < b?-1:1); }

static int compare(const t_symbol *a,const t_symbol *b) 
{
	if(a == b)
		return 0;
	else
		return strcmp(flext::GetString(a),flext::GetString(b));
}

static int compare(const t_atom &a,const t_atom &b) 
{
	if(flext::GetType(a) == flext::GetType(b)) {
		switch(flext::GetType(a)) {
		case A_FLOAT:
			return compare(flext::GetFloat(a),flext::GetFloat(b));
#if FLEXT_SYS == FLEXT_SYS_MAX
		case A_LONG:
			return compare(flext::GetInt(a),flext::GetInt(b));
#endif
		case A_SYMBOL:
			return compare(flext::GetSymbol(a),flext::GetSymbol(b));
#if FLEXT_SYS == FLEXT_SYS_PD
		case A_POINTER:
			return flext::GetPointer(a) == flext::GetPointer(b)?0:(flext::GetPointer(a) < flext::GetPointer(b)?-1:1);
#endif
		default:
			FLEXT_LOG("pool - atom comparison: type not handled");
			return -1;
		}
	}
	else
		return flext::GetType(a) < flext::GetType(b)?-1:1;
}


poolval::poolval(const t_atom &k,AtomList *d):
	data(d),nxt(NULL)
{
	SetAtom(key,k);
}

poolval::~poolval()
{
	if(data) delete data;

    FLEXT_ASSERT(nxt == NULL);
}

poolval &poolval::Set(AtomList *d)
{
	if(data) delete data;
	data = d;
	return *this;
}

poolval *poolval::Dup() const
{
	return new poolval(key,data?new Atoms(*data):NULL); 
}


pooldir::pooldir(const t_atom &d,pooldir *p,int vcnt,int dcnt):
	parent(p),nxt(NULL),vals(NULL),dirs(NULL),
	vbits(Int2Bits(vcnt)),dbits(Int2Bits(dcnt)),
	vsize(1<<vbits),dsize(1<<dbits)
{
	Reset();
	CopyAtom(&dir,&d);
}

pooldir::~pooldir()
{
	Reset(false);
		
    FLEXT_ASSERT(nxt == NULL);
}

void pooldir::Clear(bool rec,bool dironly)
{
	if(rec && dirs) { 
        for(int i = 0; i < dsize; ++i) {
            pooldir *d = dirs[i].d,*d1; 
            if(d) {
                do {
                    d1 = d->nxt;
                    d->nxt = NULL;
                    delete d;
                } while((d = d1) != NULL);
                dirs[i].d = NULL; 
                dirs[i].cnt = 0;
            }
        }
	}
	if(!dironly && vals) { 
        for(int i = 0; i < vsize; ++i) {
            poolval *v = vals[i].v,*v1;
            if(v) {
                do {
                    v1 = v->nxt;
                    v->nxt = NULL;
                    delete v;
                } while((v = v1) != NULL);
                vals[i].v = NULL; 
                vals[i].cnt = 0;
            }
        }
    }
}

void pooldir::Reset(bool realloc)
{
	Clear(true,false);

	if(dirs) delete[] dirs; 
	if(vals) delete[] vals;

	if(realloc) {
		dirs = new direntry[dsize];
		ZeroMem(dirs,dsize*sizeof *dirs);
		vals = new valentry[vsize];
		ZeroMem(vals,vsize*sizeof *vals);
	}
	else 
		dirs = NULL,vals = NULL;
}

pooldir *pooldir::AddDir(int argc,const t_atom *argv,int vcnt,int dcnt)
{
	if(!argc) return this;

	int c = 1,dix = DIdx(argv[0]);
	pooldir *prv = NULL,*ix = dirs[dix].d;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(argv[0],ix->dir);
		if(c <= 0) break;
	}

	if(c || !ix) {
		pooldir *nd = new pooldir(argv[0],this,vcnt,dcnt);
		nd->nxt = ix;

		if(prv) prv->nxt = nd;
		else dirs[dix].d = nd;
		dirs[dix].cnt++;
		ix = nd;
	}

	return ix->AddDir(argc-1,argv+1);
}

pooldir *pooldir::GetDir(int argc,const t_atom *argv,bool rmv)
{
	if(!argc) return this;

	int c = 1,dix = DIdx(argv[0]);
	pooldir *prv = NULL,*ix = dirs[dix].d;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(argv[0],ix->dir);
		if(c <= 0) break;
	}

	if(c || !ix) 
		return NULL;
	else {
		if(argc > 1)
			return ix->GetDir(argc-1,argv+1,rmv);
		else if(rmv) {
			pooldir *nd = ix->nxt;
			if(prv) prv->nxt = nd;
			else dirs[dix].d = nd;
			dirs[dix].cnt--;
			ix->nxt = NULL;
			return ix;
		}
		else 
			return ix;
	}
}

bool pooldir::DelDir(int argc,const t_atom *argv)
{
	pooldir *pd = GetDir(argc,argv,true);
	if(pd && pd != this) {
		delete pd;
		return true;
	}
	else 
		return false;
}

void pooldir::SetVal(const t_atom &key,AtomList *data,bool over)
{
    int c = 1,vix = VIdx(key);
	poolval *prv = NULL,*ix = vals[vix].v;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(key,ix->key);
		if(c <= 0) break;
	}

	if(c || !ix) {
		// no existing data found
	
		if(data) {
			poolval *nv = new poolval(key,data);
			nv->nxt = ix;

			if(prv) prv->nxt = nv;
			else vals[vix].v = nv;
			vals[vix].cnt++;
		}
	}
	else if(over) { 
		// data exists... only set if overwriting enabled
		
		if(data)
			ix->Set(data);
		else {
			// delete key
		
			poolval *nv = ix->nxt;
			if(prv) prv->nxt = nv;
			else vals[vix].v = nv;
			vals[vix].cnt--;
			ix->nxt = NULL;
			delete ix;
		}
	}
}

bool pooldir::SetVali(int rix,AtomList *data)
{
    poolval *prv = NULL,*ix = NULL;
    int vix;
	for(vix = 0; vix < vsize; ++vix) 
		if(rix > vals[vix].cnt) rix -= vals[vix].cnt;
		else {
			ix = vals[vix].v;
			for(; ix && rix; prv = ix,ix = ix->nxt) --rix;
			if(ix && !rix) break;
		}  

	if(ix) { 
		// data exists... overwrite it
		
		if(data)
			ix->Set(data);
		else {
			// delete key
		
			poolval *nv = ix->nxt;
			if(prv) prv->nxt = nv;
			else vals[vix].v = nv;
			vals[vix].cnt--;
			ix->nxt = NULL;
			delete ix;
		}
        return true;
	}
    else
        return false;
}

poolval *pooldir::RefVal(const t_atom &key)
{
	int c = 1,vix = VIdx(key);
	poolval *ix = vals[vix].v;
	for(; ix; ix = ix->nxt) {
		c = compare(key,ix->key);
		if(c <= 0) break;
	}

	return c || !ix?NULL:ix;
}

poolval *pooldir::RefVali(int rix)
{
	for(int vix = 0; vix < vsize; ++vix) 
		if(rix > vals[vix].cnt) rix -= vals[vix].cnt;
		else {
			poolval *ix = vals[vix].v;
			for(; ix && rix; ix = ix->nxt) --rix;
			if(ix && !rix) return ix;
		}
	return NULL;
}

flext::AtomList *pooldir::PeekVal(const t_atom &key)
{
	poolval *ix = RefVal(key);
	return ix?ix->data:NULL;
}

flext::AtomList *pooldir::GetVal(const t_atom &key,bool cut)
{
	int c = 1,vix = VIdx(key);
	poolval *prv = NULL,*ix = vals[vix].v;
	for(; ix; prv = ix,ix = ix->nxt) {
		c = compare(key,ix->key);
		if(c <= 0) break;
	}

	if(c || !ix) 
		return NULL;
	else {
		AtomList *ret;
		if(cut) {
			poolval *nv = ix->nxt;
			if(prv) prv->nxt = nv;
			else vals[vix].v = nv;
			vals[vix].cnt--;
			ix->nxt = NULL;
			ret = ix->data; ix->data = NULL;
			delete ix;
		}
		else
			ret = new Atoms(*ix->data);
		return ret;
	}
}

int pooldir::CntAll() const
{
	int cnt = 0;
	for(int vix = 0; vix < vsize; ++vix) cnt += vals[vix].cnt;
	return cnt;
}

int pooldir::PrintAll(char *buf,int len) const
{
    int offs = strlen(buf);

    int cnt = 0;
    for(int vix = 0; vix < vsize; ++vix) {
		poolval *ix = vals[vix].v;
        for(int i = 0; ix; ++i,ix = ix->nxt) {
			PrintAtom(ix->key,buf+offs,len-offs);
            strcat(buf+offs," , ");
            int l = strlen(buf+offs)+offs;
			ix->data->Print(buf+l,len-l);
            post(buf);
        }
        cnt += vals[vix].cnt;
    }
    
    buf[offs] = 0;

	return cnt;
}

int pooldir::GetKeys(AtomList &keys)
{
	int cnt = CntAll();
	keys(cnt);

	for(int vix = 0; vix < vsize; ++vix) {
		poolval *ix = vals[vix].v;
		for(int i = 0; ix; ++i,ix = ix->nxt) 
			SetAtom(keys[i],ix->key);
	}
	return cnt;
}

int pooldir::GetAll(t_atom *&keys,Atoms *&lst,bool cut)
{
	int cnt = CntAll();
	keys = new t_atom[cnt];
	lst = new Atoms[cnt];

	for(int i = 0,vix = 0; vix < vsize; ++vix) {
		poolval *ix = vals[vix].v;
		for(; ix; ++i) {
			SetAtom(keys[i],ix->key);
			lst[i] = *ix->data;

			if(cut) {
				poolval *t = ix;
				vals[vix].v = ix = ix->nxt;
				vals[vix].cnt--;				
				t->nxt = NULL; delete t;
			}
			else
				ix = ix->nxt;
		}
	}
	return cnt;
}


int pooldir::CntSub() const
{
	int cnt = 0;
	for(int dix = 0; dix < dsize; ++dix) cnt += dirs[dix].cnt;
	return cnt;
}


int pooldir::GetSub(const t_atom **&lst)
{
	const int cnt = CntSub();
	lst = new const t_atom *[cnt];
	for(int i = 0,dix = 0; i < cnt; ++dix) {
		pooldir *ix = dirs[dix].d;
		for(; ix; ix = ix->nxt) lst[i++] = &ix->dir;
	}
	return cnt;
}


bool pooldir::Paste(const pooldir *p,int depth,bool repl,bool mkdir)
{
	bool ok = true;

	for(int vi = 0; vi < p->vsize; ++vi) {
		for(poolval *ix = p->vals[vi].v; ix; ix = ix->nxt) {
			SetVal(ix->key,new Atoms(*ix->data),repl);
		}
	}

	if(ok && depth) {
		for(int di = 0; di < p->dsize; ++di) {
			for(pooldir *dix = p->dirs[di].d; ok && dix; dix = dix->nxt) {
				pooldir *ndir = mkdir?AddDir(1,&dix->dir):GetDir(1,&dix->dir);
				if(ndir) { 
					ok = ndir->Paste(dix,depth > 0?depth-1:depth,repl,mkdir);
				}
			}
		}
	}

	return ok;
}

bool pooldir::Copy(pooldir *p,int depth,bool cut)
{
	bool ok = true;

	if(cut) {
		for(int vi = 0; vi < vsize; ++vi) {
			for(poolval *ix = vals[vi].v; ix; ix = ix->nxt)
				p->SetVal(ix->key,ix->data);
			vals[vi].cnt = 0;
			vals[vi].v = NULL;
		}
	}
	else {
		for(int vi = 0; vi < vsize; ++vi) {
			for(poolval *ix = vals[vi].v; ix; ix = ix->nxt) {
				p->SetVal(ix->key,new Atoms(*ix->data));
			}
		}
	}

	if(ok && depth) {
		for(int di = 0; di < dsize; ++di) {
			for(pooldir *dix = dirs[di].d; ok && dix; dix = dix->nxt) {
				pooldir *ndir = p->AddDir(1,&dix->dir);
				if(ndir)
					ok = dix->Copy(ndir,depth > 0?depth-1:depth,cut);
				else
					ok = false;
			}
		}
	}

	return ok;
}

static bool _isspace(char c) { return c > 0 && isspace(c); }

static const char *ReadAtom(const char *c,t_atom &a,bool utf8)
{
	// skip leading whitespace (NON-ASCII character are < 0)
	while(*c && _isspace(*c)) ++c;
	if(!*c) return NULL;

	char tmp[1024];
    char *m = tmp; // write position

    bool issymbol;
    if(*c == '"') {
        issymbol = true;
        ++c;
    }
    else
        issymbol = false;

    // go to next whitespace
    for(bool escaped = false;; ++c)
        if(*c == '\\') {
            if(escaped) {
                *m++ = *c;
                escaped = false;
            }
            else
                escaped = true;
        }
        else if(*c == '"' && issymbol && !escaped) {
            // end of string
            ++c;
            FLEXT_ASSERT(!*c || _isspace(*c));
            *m = 0;
            break;
        }
        else if(!*c || (_isspace(*c) && !escaped)) {
            *m = 0;
            break;
        }
        else {
            *m++ = *c;
            escaped = false;
        }

    // save character and set delimiter

    float fres;
    // first try float
#if 0
    if(!issymbol && sscanf(tmp,"%f",&fres) == 1) {
#else
    char *endp;
    // see if it's a float - thanks to Frank Barknecht
    fres = (float)strtod(tmp,&endp);   
    if(!issymbol && !*endp && endp != tmp) { 
#endif
        int ires = (int)fres; // try a cast
        if(fres == ires)
            flext::SetInt(a,ires);
        else
            flext::SetFloat(a,fres);
    }
    // no, it's a symbol
    else {
		const char *c;
        if(utf8) {
#if FLEXT_OS == FLEXT_OS_WIN
            wchar_t wtmp[1024];
            int err = MultiByteToWideChar(CP_UTF8,0,tmp,strlen(tmp),wtmp,1024);
            if(!err) return NULL;
            err = WideCharToMultiByte(CP_ACP,0,wtmp,err,tmp,1024,NULL,FALSE);
            if(!err) return NULL;
            tmp[err] = 0;
			c = tmp;
#elif FLEXT_OS == FLEXT_OS_MAC
            char ctmp[1024];

			// is the output always MacRoman?
			TextEncoding inconv = CreateTextEncoding(kTextEncodingUnicodeDefault,kTextEncodingDefaultVariant,kUnicodeUTF8Format);
			TextEncoding outconv = CreateTextEncoding(kTextEncodingMacRoman,kTextEncodingDefaultVariant,kTextEncodingDefaultFormat);

			TECObjectRef converter;
			OSStatus status = TECCreateConverter(&converter,inconv,outconv);
			if(status) return NULL;
			
			ByteCount inlen,outlen;
			status = TECConvertText(
			   converter,
			   (ConstTextPtr)tmp,strlen(tmp),&inlen,
			   (TextPtr)ctmp,sizeof(ctmp),&outlen
			);
			ctmp[outlen] = 0;
	
			TECDisposeConverter(converter);
			c = ctmp;
			if(status) return NULL;
#else
            wchar_t wtmp[1024];
			size_t len = mbstowcs(wtmp,tmp,1024);
			if(len < 0) return false;
			if(!WCStoUTF8(tmp,wtmp,sizeof(tmp))) return NULL;
			c = tmp;
#endif
		}
		else 
			c = tmp;
        flext::SetString(a,c);
	}

	return c;
}

static bool ParseAtoms(const char *tmp,flext::AtomList &l,bool utf8)
{
    FLEXT_ASSERT(tmp);
    vector<t_atom> atoms;
    while(*tmp) {
        t_atom at;
		tmp = ReadAtom(tmp,at,utf8);
        if(!tmp) break;
        atoms.push_back(at);
    }
    l(atoms.size(),&atoms[0]);
    return true;
}

static bool ParseAtoms(string &s,flext::AtomList &l,bool utf8) 
{ 
    return ParseAtoms((char *)s.c_str(),l,utf8); 
}

static bool ReadAtoms(istream &is,flext::AtomList &l,char del,bool utf8)
{
    vector<char> tmp;
    for(;;) {
        char c = is.get();
        if(is.eof() || c == del) break;
        tmp.push_back(c);
    }
    tmp.push_back(0); // end-of-string marker

	return is.good() && ParseAtoms(&tmp[0],l,utf8);
}

static bool WriteAtom(ostream &os,const t_atom &a,bool utf8)
{
	if(flext::IsFloat(a))
		os << flext::GetFloat(a);
    else if(flext::IsInt(a))
		os << flext::GetInt(a);
    else if(flext::IsSymbol(a)) {
        const char *c = flext::GetString(a);
        if(utf8) {
#if FLEXT_OS == FLEXT_OS_WIN
            char tmp[1024];
            wchar_t wtmp[1024];
            int err = MultiByteToWideChar(CP_ACP,0,c,strlen(c),wtmp,1024);
            if(!err) return false;
            err = WideCharToMultiByte(CP_UTF8,0,wtmp,err,tmp,1024,NULL,FALSE);
            if(!err) return false;
            tmp[err] = 0;
            c = tmp;
#elif FLEXT_OS == FLEXT_OS_MAC
            char tmp[1024];

			// is the input always MacRoman?
			TextEncoding inconv = CreateTextEncoding(kTextEncodingMacRoman,kTextEncodingDefaultVariant,kTextEncodingDefaultFormat);
			TextEncoding outconv = CreateTextEncoding(kTextEncodingUnicodeDefault,kTextEncodingDefaultVariant,kUnicodeUTF8Format);

			TECObjectRef converter;
			OSStatus status = TECCreateConverter(&converter,inconv,outconv);
			if(status) return false;
			
			ByteCount inlen,outlen;
			status = TECConvertText(
			   converter,
			   (ConstTextPtr)c,strlen(c),&inlen,
			   (TextPtr)tmp,sizeof(tmp),&outlen
			);
			tmp[outlen] = 0;
	
			TECDisposeConverter(converter);

			if(status) return false;
            c = tmp;
#else
            char tmp[1024];
            wchar_t wtmp[1024];
			if(!UTF8toWCS(wtmp,c,1024)) return false;
			size_t len = wcstombs(tmp,wtmp,sizeof(tmp));
			if(len < 0) return false;
            c = tmp;
#endif
        }

        os << '"';
        for(; *c; ++c) {
			// escape some special characters
            if(_isspace(*c) || *c == '\\' || *c == ',' || *c == '"')
                os << '\\';
	        os << *c;
        }
        os << '"';
	}
    else
        FLEXT_ASSERT(false);
    return true;
}

static void WriteAtoms(ostream &os,const flext::AtomList &l,bool utf8)
{
	for(int i = 0; i < l.Count(); ++i) {
		WriteAtom(os,l[i],utf8);
		if(i < l.Count()-1) os << ' ';
	}
}

bool pooldir::LdDir(istream &is,int depth,bool mkdir)
{
	for(int i = 1; !is.eof(); ++i) {
		Atoms d,k,*v = new Atoms;
		bool r = 
            ReadAtoms(is,d,',',false) && 
            ReadAtoms(is,k,',',false) &&
            ReadAtoms(is,*v,'\n',false);

		if(r) {
			if(depth < 0 || d.Count() <= depth) {
				pooldir *nd = mkdir?AddDir(d):GetDir(d);
				if(nd) {
                    if(k.Count() == 1) {
	    				nd->SetVal(k[0],v); v = NULL;
                    }
                    else if(k.Count() > 1)
                        post("pool - file format invalid: key must be a single word");
				}
	#ifdef FLEXT_DEBUG
				else
					post("pool - directory was not found",i);
	#endif
			}
		}
		else if(!is.eof())
			post("pool - format mismatch encountered, skipped line %i",i);

		if(v) delete v;
	}
	return true;
}

bool pooldir::SvDir(ostream &os,int depth,const AtomList &dir)
{
    int cnt = 0;
	for(int vi = 0; vi < vsize; ++vi) {
		for(poolval *ix = vals[vi].v; ix; ix = ix->nxt) {
			WriteAtoms(os,dir,false);
			os << " , ";
			WriteAtom(os,ix->key,false);
			os << " , ";
			WriteAtoms(os,*ix->data,false);
			os << endl;
            ++cnt;
		}
	}
    if(!cnt) {
        // no key/value pairs present -> force empty directory
		WriteAtoms(os,dir,false);
		os << " , ," << endl;
    }
	if(depth) {
        // save sub-directories
		int nd = depth > 0?depth-1:-1;
		for(int di = 0; di < dsize; ++di) {
			for(pooldir *ix = dirs[di].d; ix; ix = ix->nxt) {
				ix->SvDir(os,nd,Atoms(dir).Append(ix->dir));
			}
		}
	}
	return true;
}

class xmltag {
public:
    string tag,attr;
    bool Ok() const { return tag.length() > 0; }
    bool operator ==(const char *t) const { return !tag.compare(t); }

    void Clear() 
    { 
#if defined(_MSC_VER) && (_MSC_VER < 0x1200)
        // incomplete STL implementation
        tag = ""; attr = ""; 
#else
        tag.clear(); attr.clear(); 
#endif
    }

    enum { t_start,t_end,t_empty } type;
};

static bool gettag(istream &is,xmltag &tag)
{
    static const char *commstt = "<!--",*commend = "-->";

    for(;;) {
        // eat whitespace
        while(_isspace(is.peek())) is.get();

        // no tag begin -> break
        if(is.peek() != '<') break;
        is.get(); // swallow <

        char tmp[1024],*t = tmp;

        // parse for comment start
        const char *c = commstt;
        while(*++c) {
            if(*c != is.peek()) break;
            *(t++) = is.get();
        }

        if(!*c) { // is comment
            char cmp[2] = {0,0}; // set to some unusual initial value

            for(int ic = 0; ; ic = (++ic)%2) {
                char c = is.get();
                if(c == '>') {
                    // if third character is > then check also the former two
                    int i;
                    for(i = 0; i < 2 && cmp[(ic+i)%2] == commend[i]; ++i) {}
                    if(i == 2) break; // match: comment end found!
                }
                else
                    cmp[ic] = c;
            }
        }
        else {
            // parse until > with consideration of "s
            bool intx = false;
            for(;;) {
                *t = is.get();
                if(*t == '"') intx = !intx;
                else if(*t == '>' && !intx) {
                    *t = 0;
                    break;
                }
                t++;
            }

            // look for tag slashes

            char *tb = tmp,*te = t-1,*tf;

            for(; _isspace(*tb); ++tb) {}
            if(*tb == '/') { 
                // slash at the beginning -> end tag
                tag.type = xmltag::t_end;
                for(++tb; _isspace(*tb); ++tb) {}
            }
            else {
                for(; _isspace(*te); --te) {}
                if(*te == '/') { 
                    // slash at the end -> empty tag
                    for(--te; _isspace(*te); --te) {}
                    tag.type = xmltag::t_empty;
                }
                else 
                    // no slash -> begin tag
                    tag.type = xmltag::t_start;
            }

            // copy tag text without slashes
            for(tf = tb; tf <= te && *tf && !_isspace(*tf); ++tf) {}
            tag.tag.assign(tb,tf-tb);
            while(_isspace(*tf)) ++tf;
            tag.attr.assign(tf,te-tf+1);

            return true;
        }
    }

    tag.Clear();
    return false;
}

static void getvalue(istream &is,string &s)
{
    char tmp[1024],*t = tmp; 
    bool intx = false;
    for(;;) {
        char c = is.peek();
        if(c == '"') intx = !intx;
        else if(c == '<' && !intx) break;
        *(t++) = is.get();
    }
    *t = 0;
    s = tmp;
}

bool pooldir::LdDirXMLRec(istream &is,int depth,bool mkdir,AtomList &d)
{
    Atoms k,v;
    bool inval = false,inkey = false,indata = false;
    int cntval = 0;

	while(!is.eof()) {
        xmltag tag;
        gettag(is,tag);
        if(!tag.Ok()) {
            // look for value
            string s;
            getvalue(is,s);

            if(s.length() &&
                (
                    (!inval && inkey && d.Count()) ||  /* dir */
                    (inval && (inkey || indata)) /* value */
                )
            ) {
                bool ret = true;
                if(indata) {
                    if(v.Count())
                        post("pool - XML load: value data already given, ignoring new data");
                    else
                        ret = ParseAtoms(s,v,true);
                }
                else // inkey
                    if(inval) {
                        if(k.Count())
                            post("pool - XML load, value key already given, ignoring new key");
                        else
                            ret = ParseAtoms(s,k,true);
                    }
                    else {
                        t_atom &dkey = d[d.Count()-1];
                        FLEXT_ASSERT(IsSymbol(dkey));
                        const char *ds = GetString(dkey);
                        FLEXT_ASSERT(ds);
                        if(*ds) 
                            post("pool - XML load: dir key already given, ignoring new key");
                        else
                            ReadAtom(s.c_str(),dkey,true);

                        ret = true;
                    }
                if(!ret) post("pool - error interpreting XML value (%s)",s.c_str());
            }
            else
                post("pool - error reading XML data");
        }
        else if(tag == "dir") {
            if(tag.type == xmltag::t_start) {
                // warn if last directory key was not given
                if(d.Count() && GetSymbol(d[d.Count()-1]) == sym__)
                    post("pool - XML load: dir key must be given prior to subdirs, ignoring items");

                Atoms dnext(d.Count()+1);
                // copy existing dir
                dnext.Set(d.Count(),d.Atoms(),0,false);
                // initialize current dir key as empty
                SetSymbol(dnext[d.Count()],sym__);

                // read next level
                LdDirXMLRec(is,depth,mkdir,dnext); 
            }
            else if(tag.type == xmltag::t_end) {
                if(!cntval && mkdir) {
                    // no values have been found in dir -> make empty dir
                    AddDir(d);
                }

                // break tag loop
                break;
            }
        }
        else if(tag == "value") {
            if(tag.type == xmltag::t_start) {
                inval = true;
                ++cntval;
                k.Clear(); v.Clear();
            }
            else if(tag.type == xmltag::t_end) {
                // set value after tag closing, but only if level <= depth
        	    if(depth < 0 || d.Count() <= depth) {
                    int fnd;
                    for(fnd = d.Count()-1; fnd >= 0; --fnd)
                        if(GetSymbol(d[fnd]) == sym__) break;

                    // look if last dir key has been given
                    if(fnd >= 0) {
                        if(fnd == d.Count()-1)
                            post("pool - XML load: dir key must be given prior to values");

                        // else: one directory level has been left unintialized, ignore items
                    }
                    else {
                        // only use first word of key
                        if(k.Count() == 1) {
		        		    pooldir *nd = mkdir?AddDir(d):GetDir(d);
        				    if(nd) 
                                nd->SetVal(k[0],new Atoms(v));
                            else
                                post("pool - XML load: value key must be exactly one word, value not stored");
				        }
                    }
                }
                inval = false;
            }
        }
        else if(tag == "key") {
            if(tag.type == xmltag::t_start) {
                inkey = true;
            }
            else if(tag.type == xmltag::t_end) {
                inkey = false;
            }
        }
        else if(tag == "data") {
            if(!inval) 
                post("pool - XML tag <data> not within <value>");

            if(tag.type == xmltag::t_start) {
                indata = true;
            }
            else if(tag.type == xmltag::t_end) {
                indata = false;
            }
        }
        else if(!d.Count() && tag == "pool" && tag.type == xmltag::t_end) {
            // break tag loop
            break;
        }
#ifdef FLEXT_DEBUG
        else {
            post("pool - unknown XML tag '%s'",tag.tag.c_str());
        }
#endif
    }
    return true;
}

bool pooldir::LdDirXML(istream &is,int depth,bool mkdir)
{
	while(!is.eof()) {
        xmltag tag;
        if(!gettag(is,tag)) break;

        if(tag == "pool") {
            if(tag.type == xmltag::t_start) {
                Atoms empty; // must be a separate definition for gcc
                LdDirXMLRec(is,depth,mkdir,empty);
            }
            else
                post("pool - pool not initialized yet");
        }
        else if(tag == "!DOCTYPE") {
            // ignore
        }
#ifdef FLEXT_DEBUG
        else {
            post("pool - unknown XML tag '%s'",tag.tag.c_str());
        }
#endif
    }
    return true;
}

static void indent(ostream &s,int cnt) 
{
    for(int i = 0; i < cnt; ++i) s << '\t';
}

bool pooldir::SvDirXML(ostream &os,int depth,const AtomList &dir,int ind)
{
	int i,lvls = ind?(dir.Count()?1:0):dir.Count();

	for(i = 0; i < lvls; ++i) {
		indent(os,ind+i);
		os << "<dir>" << endl;
		indent(os,ind+i+1);
		os << "<key>";
		WriteAtom(os,dir[ind+i],true);
		os << "</key>" << endl;
	}

	for(int vi = 0; vi < vsize; ++vi) {
		for(poolval *ix = vals[vi].v; ix; ix = ix->nxt) {
            indent(os,ind+lvls);
            os << "<value><key>";
			WriteAtom(os,ix->key,true);
            os << "</key><data>";
			WriteAtoms(os,*ix->data,true);
			os << "</data></value>" << endl;
		}
	}

	if(depth) {
		int nd = depth > 0?depth-1:-1;
		for(int di = 0; di < dsize; ++di) {
			for(pooldir *ix = dirs[di].d; ix; ix = ix->nxt) {
				ix->SvDirXML(os,nd,Atoms(dir).Append(ix->dir),ind+lvls);
			}
		}
	}

	for(i = lvls-1; i >= 0; --i) {
		indent(os,ind+i);
		os << "</dir>" << endl;
	}
	return true;
}

unsigned int pooldir::FoldBits(unsigned long h,int bits)
{
	if(!bits) return 0;
	const int hmax = (1<<bits)-1;
	unsigned int ret = 0;
	for(unsigned int i = 0; i < sizeof(h)*8; i += bits)
		ret ^= (h>>i)&hmax;
	return ret;
}

int pooldir::Int2Bits(unsigned long n)
{
	int b;
	for(b = 0; n; ++b) n >>= 1;
	return b;
}
