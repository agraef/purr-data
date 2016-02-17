/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flattr.cpp
    \brief Attribute handling for the flext base class
*/
 
#ifndef __FLEXT_ATTR_CPP
#define __FLEXT_ATTR_CPP

#include "flext.h"

#include <cstring>
#include <cctype>
#include <set>

#include "flpushns.h"

#ifdef __MWERKS__
#define STD std
#else
#define STD
#endif

FLEXT_TEMPIMPL(FLEXT_CLASSDEF(flext_base))::AttrItem::AttrItem(const t_symbol *t,metharg tp,methfun f,int fl):
	Item(NULL),index(0),
	flags(fl|afl_shown),
	argtp(tp),fun(f),
	counter(NULL),tag(t)
{}


/*
FLEXT_CLASSDEF(flext_base)::AttrDataCont::AttrDataCont() {}

FLEXT_CLASSDEF(flext_base)::AttrDataCont::~AttrDataCont()
{
	for(iterator it = begin(); it != end(); ++it)
		if(it.data()) delete it.data();
}
*/

FLEXT_TEMPIMPL(FLEXT_CLASSDEF(flext_base))::AttrDataCont::~AttrDataCont() { clear(); }

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::AttrDataCont::clear()
{
    for(FLEXT_TEMP_TYPENAME AttrDataCont::iterator it(*this); it; ++it) delete it.data();
    TablePtrMap<const t_symbol *,AttrData *,8>::clear();
}

//! Add get and set attributes
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::AddAttrib(ItemCont *aa,ItemCont *ma,const t_symbol *asym,metharg tp,methfun gfun,methfun sfun)
{
	AttrItem *a,*b;

    FLEXT_ASSERT(asym != sym__ && asym != sym_list && asym != sym_float && asym != sym_symbol && asym != sym_anything);

	if(sfun) // if commented out, there will be a warning at run-time (more user-friendly)
	{
		a = new AttrItem(asym,tp,sfun,AttrItem::afl_set);
        a->index = aa->Members();
		aa->Add(a,asym); 

		// bind attribute to a method
		MethItem *mi = new MethItem(a);
		mi->SetArgs(sfun,1,new metharg(tp));
		ma->Add(mi,asym);
	}
	else
		a = NULL;

	if(gfun) // if commented out, there will be a warning at run-time (more user-friendly)
	{
		b = new AttrItem(asym,tp,gfun,AttrItem::afl_get);
        b->index = aa->Members();
		aa->Add(b,asym); 

		static char tmp[256] = "get";
		strcpy(tmp+3,GetString(asym));

		// bind attribute to a method
		MethItem *mi = new MethItem(b);
		mi->SetArgs(gfun,0,NULL);
		ma->Add(mi,MakeSymbol(tmp));
	}
	else
		b = NULL;

	if(a && b) {
		a->counter = b;
		b->counter = a;
	}
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::AddAttrib(const t_symbol *attr,metharg tp,methfun gfun,methfun sfun)
{
	if(HasAttributes())
		AddAttrib(ThAttrs(),ThMeths(),attr,tp,gfun,sfun);
	else
		error("%s - attribute procession is not enabled!",thisName());
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::AddAttrib(t_classid c,const t_symbol *attr,metharg tp,methfun gfun,methfun sfun)
{
	AddAttrib(ClAttrs(c),ClMeths(c),attr,tp,gfun,sfun);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::ListAttrib(AtomList &la) const
{
	typedef TablePtrMap<int,const t_symbol *,32> AttrList;
	AttrList list[2];
    ItemCont *clattrhead = ClAttrs(thisClassId());

	int i;
	for(i = 0; i <= 1; ++i) {
        ItemCont *a = i?attrhead:clattrhead;
		if(a && a->Contained(0)) {
            ItemSet &ai = a->GetInlet();
            for(FLEXT_TEMP_TYPENAME ItemSet::iterator as(ai); as; ++as) {
                for(Item *al = as.data(); al; al = al->nxt) {
					AttrItem *aa = (AttrItem *)al;
					list[i].insert(aa->index,as.key());
                    break;
                }
			}
		}
	}

	la((int)(list[0].size()+list[1].size()));
	int ix = 0;
	for(i = 0; i <= 1; ++i)
		for(AttrList::iterator it(list[i]); it; ++it) 
			SetSymbol(la[ix++],it.data());
}

FLEXT_TEMPIMPL(int FLEXT_CLASSDEF(flext_base))::CheckAttrib(int argc,const t_atom *argv)
{
	int offs = 0;
	for(; offs < argc; ++offs)
		if(IsString(argv[offs]) && *GetString(argv[offs]) == '@') break;
	return offs;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::InitAttrib(int argc,const t_atom *argv)
{
	int cur,nxt;
	for(cur = 0; cur < argc; cur = nxt) {
		// find next @symbol
		for(nxt = cur+1; nxt < argc; ++nxt)
			if(IsString(argv[nxt]) && *GetString(argv[nxt]) == '@') break;

		const t_symbol *tag = MakeSymbol(GetString(argv[cur])+1);

		// find puttable attribute
		AttrItem *attr = FindAttrib(tag,false,true);
		if(attr) {
			// make an entry (there are none beforehand...)
/*
			AttrDataCont::iterator it = attrdata->find(tag);
			if(it == attrdata->end()) {
				AttrDataCont::pair pair; 
				pair.key() = tag;
				pair.data() = new AttrData;
				it = attrdata->insert(attrdata->begin(),pair);
			}

			AttrData &a = *it.data();
			a.SetInit(true);
			a.SetInitValue(nxt-cur-1,argv+cur+1);

			// pass value to object
			SetAttrib(tag,attr,a.GetInitValue());
*/
			AttrData *a = attrdata->find(tag);
            if(!a) {
                AttrData *old = attrdata->insert(tag,a = new AttrData);
                FLEXT_ASSERT(!old);
            }

			a->SetInit(true);
			a->SetInitValue(nxt-cur-1,argv+cur+1);

			// pass value to object
			SetAttrib(tag,attr,a->GetInitValue());
		}
	}
	return true;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::ListAttrib() const
{
    if(HasAttributes()) {
        // defined in flsupport.cpp
		AtomListStatic<32> la;
		ListAttrib(la);
		ToOutAnything(GetOutAttr(),sym_attributes,la.Count(),la.Atoms());
		return true;
	}
	else
		return false;
}

FLEXT_TEMPIMPL(FLEXT_TEMPSUB(FLEXT_CLASSDEF(flext_base))::AttrItem *FLEXT_CLASSDEF(flext_base))::FindAttrib(const t_symbol *tag,bool get,bool msg) const
{
    ItemCont *clattrhead = ClAttrs(thisClassId());

    // first search within object scope
	AttrItem *a = NULL;
    {
        for(Item *lst = attrhead->FindList(tag); lst; lst = lst->nxt) {
            AttrItem *b = (AttrItem *)lst;
            if(get?b->IsGet():b->IsSet()) { a = b; break; }
        }
    }

    // then (if nothing found) search within class scope
	if(!a) {
        for(Item *lst = clattrhead->FindList(tag); lst; lst = lst->nxt) {
            AttrItem *b = (AttrItem *)lst;
            if(get?b->IsGet():b->IsSet()) { a = b; break; }
        }
	}

    if(!a && msg) {
		// print a message
		error("%s - %s: attribute not found",thisName(),GetString(tag));
	}
	return a;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::SetAttrib(const t_symbol *tag,int argc,const t_atom *argv)
{
	// search for matching attribute
	AttrItem *a = FindAttrib(tag,false,true);
	return a && SetAttrib(tag,a,argc,argv);
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::SetAttrib(const t_symbol *tag,AttrItem *a,int argc,const t_atom *argv)
{
	if(a->fun) {
		bool ok = true;

		t_any any;
		switch(a->argtp) {
		case a_float:
			if(argc == 1 && CanbeFloat(argv[0])) {
				any.ft = GetAFloat(argv[0]);
				((methfun_1)a->fun)(this,any);				
			}
			else ok = false;
			break;
		case a_int:
			if(argc == 1 && CanbeInt(argv[0])) {
				any.it = GetAInt(argv[0]);
				((methfun_1)a->fun)(this,any);				
			}
			else ok = false;
			break;
		case a_symbol:
			if(argc == 1 && IsSymbol(argv[0])) {
                t_atom at;
                GetParamSym(at,GetSymbol(argv[0]),thisCanvas());
				any.st = const_cast<t_symbol *>(GetSymbol(at));
				((methfun_1)a->fun)(this,any);				
			}
			else ok = false;
			break;
		case a_bool:
			if(argc == 1 && CanbeBool(argv[0])) {
				any.bt = GetABool(argv[0]);
				((methfun_1)a->fun)(this,any);				
			}
			else ok = false;
			break;
		case a_LIST: {
			AtomListStatic<16> la(argc);
			for(int i = 0; i < argc; ++i)
				if(IsSymbol(argv[i])) 
					GetParamSym(la[i],GetSymbol(argv[i]),thisCanvas());
				else
					la[i] = argv[i];

			any.vt = &la;
			((methfun_1)a->fun)(this,any);				
			break;
		}
		default:
			ERRINTERNAL();
		}

		if(!ok)
			post("%s - wrong arguments for attribute %s",thisName(),GetString(tag));
	}
	else
		post("%s - attribute %s has no get method",thisName(),GetString(tag));
	return true;
}


FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::GetAttrib(const t_symbol *tag,AttrItem *a,AtomList &la) const
{
	bool ok = true;
	// main attribute tag
	if(a) {
		if(a->fun) {
			t_any any;
			switch(a->argtp) {
			case a_float: {
				((methfun_1)a->fun)(const_cast<flext_base *>(this),any);				
				la(1);
				SetFloat(la[0],any.ft);
				break;
			}
			case a_int: {
				((methfun_1)a->fun)(const_cast<flext_base *>(this),any);				
				la(1);
				SetInt(la[0],any.it);
				break;
			}
			case a_bool: {
				((methfun_1)a->fun)(const_cast<flext_base *>(this),any);				
				la(1);
				SetBool(la[0],any.bt);
				break;
			}
			case a_symbol: {
				((methfun_1)a->fun)(const_cast<flext_base *>(this),any);				
				la(1);
				SetSymbol(la[0],any.st);
				break;
			}
			case a_LIST: {
				any.vt = &la;
				((methfun_1)a->fun)(const_cast<flext_base *>(this),any);				
				break;
			}
			default:
				ERRINTERNAL();
				ok = false;
			}
		}
		else {
			post("%s - attribute %s has no get method",thisName(),GetString(tag));
			ok = false;
		}
	}
	else {
		error("%s - %s: attribute not found",thisName(),GetString(tag));
		ok = false;
	}
	return ok;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::GetAttrib(const t_symbol *s,AtomList &a) const
{
	AttrItem *attr = FindAttrib(s,true);
	return attr && GetAttrib(s,attr,a);
}

//! \param tag symbol "get[attribute]"
FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::DumpAttrib(const t_symbol *tag,AttrItem *a) const
{
	AtomListStatic<16> la;
	bool ret = GetAttrib(tag,a,la);
	if(ret) {
		ToOutAnything(GetOutAttr(),a->tag,la.Count(),la.Atoms());
	}
	return ret;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::DumpAttrib(const t_symbol *attr) const
{
	AttrItem *item = FindAttrib(attr,true);
	return item && DumpAttrib(attr,item);
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::BangAttrib(const t_symbol *attr,AttrItem *item)
{
	AtomListStatic<16> val;
	AttrItem *item2;
	if(!item->IsGet()) 
		item = item->Counterpart();
	if(item) {
		item2 = item->Counterpart();
		return item2 && GetAttrib(attr,item,val) && SetAttrib(attr,item2,val);
	}
	else
		return false;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::BangAttrib(const t_symbol *attr)
{
	AttrItem *item = FindAttrib(attr,true);
	return item && BangAttrib(attr,item);
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::BangAttribAll()
{
    ItemCont *clattrhead = ClAttrs(thisClassId());

	for(int i = 0; i <= 1; ++i) {
        ItemCont *a = i?attrhead:clattrhead;
		if(a) {
            ItemSet &ai = a->GetInlet(); // \todo need to check for presence of inlet 0?
/*
            for(ItemSet::iterator as = ai.begin(); as != ai.end(); ++as) {
                for(Item *al = as.data(); al; al = al->nxt) {
					AttrItem *a = (AttrItem *)al;
	        		if(a->IsGet() && a->BothExist()) BangAttrib(as.key(),a);
                }
			}
*/
            for(FLEXT_TEMP_TYPENAME ItemSet::iterator as(ai); as; ++as) {
                for(Item *al = as.data(); al; al = al->nxt) {
					AttrItem *a = (AttrItem *)al;
	        		if(a->IsGet() && a->BothExist()) BangAttrib(as.key(),a);
                }
			}
		}
	}
	return true;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::ShowAttrib(AttrItem *a,bool show) const
{
	if(show) a->flags |= AttrItem::afl_shown;
	else a->flags &= ~AttrItem::afl_shown;

	// also change counterpart, if present
	AttrItem *ca = a->Counterpart();
	if(ca) {
		if(show) ca->flags |= AttrItem::afl_shown;
		else ca->flags &= ~AttrItem::afl_shown;
	}
	return true;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::ShowAttrib(const t_symbol *attr,bool show) const
{
	AttrItem *item = FindAttrib(attr,true);
	return item && ShowAttrib(item,show);
}

#include "flpopns.h"

#endif // __FLEXT_ATTR_CPP


