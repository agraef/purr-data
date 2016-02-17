/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flitem.cpp
    \brief Processing of method and attribute lists.
*/
 
#ifndef __FLEXT_ITEM_CPP
#define __FLEXT_ITEM_CPP

#include "flext.h"
#include <cstring>

#include "flpushns.h"

FLEXT_TEMPIMPL(FLEXT_CLASSDEF(flext_base))::ItemSet::~ItemSet() { clear(); }

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::ItemSet::clear()
{
    for(FLEXT_TEMP_TYPENAME TablePtrMapDef::iterator it(*this); it; ++it) delete it.data();
    TablePtrMap<const t_symbol *,Item *,8>::clear();
}


FLEXT_TEMPIMPL(FLEXT_CLASSDEF(flext_base))::Item::~Item()
{
    if(nxt) delete nxt;
}

FLEXT_TEMPIMPL(FLEXT_CLASSDEF(flext_base))::ItemCont::ItemCont():
    members(0),memsize(0),size(0),cont(NULL)
{}

FLEXT_TEMPIMPL(FLEXT_CLASSDEF(flext_base))::ItemCont::~ItemCont()
{
    if(cont) {
        for(int i = 0; i < size; ++i) delete cont[i];
        delete[] cont;
    }
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::ItemCont::Resize(int nsz)
{
    if(nsz > memsize) {
        int nmemsz = nsz+10;  // increment maximum allocation size
        ItemSet **ncont = new ItemSet *[nmemsz]; // make new array
        if(cont) {
            memcpy(ncont,cont,size*sizeof(*cont)); // copy existing entries
            delete[] cont; 
        }
        cont = ncont;  // set current array
        memsize = nmemsz;  // set new allocation size
    }

    // make new items
    while(size < nsz) cont[size++] = new ItemSet;
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::ItemCont::Add(Item *item,const t_symbol *tag,int inlet)
{
    FLEXT_ASSERT(tag);

    if(!Contained(inlet)) Resize(inlet+2);
    ItemSet &set = GetInlet(inlet);
    Item *lst = set.find(tag);
    if(!lst) { 
        Item *old = set.insert(tag,lst = item);
        FLEXT_ASSERT(!old);
    }
    else
        for(;;)
            if(!lst->nxt) { lst->nxt = item; break; }
            else lst = lst->nxt;
    members++;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::ItemCont::Remove(Item *item,const t_symbol *tag,int inlet,bool free)
{
    FLEXT_ASSERT(tag);

    if(Contained(inlet)) {
        ItemSet &set = GetInlet(inlet);
        Item *lit = set.find(tag);
        for(Item *prv = NULL; lit; prv = lit,lit = lit->nxt) {
            if(lit == item) {
                if(prv) prv->nxt = lit->nxt;
                else if(lit->nxt) {
                    Item *old = set.insert(tag,lit->nxt);
                    FLEXT_ASSERT(!old);
                }
                else {
                    Item *l = set.remove(tag);
                    FLEXT_ASSERT(l == lit);
                }

                lit->nxt = NULL; 
                if(free) delete lit;
                return true;
            }
        }
    }
    return false;
}

FLEXT_TEMPIMPL(FLEXT_TEMPSUB(FLEXT_CLASSDEF(flext_base))::Item *FLEXT_CLASSDEF(flext_base))::ItemCont::FindList(const t_symbol *tag,int inlet)
{
    FLEXT_ASSERT(tag);
    return Contained(inlet)?GetInlet(inlet).find(tag):NULL;
}

// --- class item lists (methods and attributes) ----------------

/*
typedef TablePtrMap<FLEXT_CLASSDEF(flext_base)::t_classid,FLEXT_CLASSDEF(flext_base)::ItemCont *,8> ClassMap;

static ClassMap classarr[2];

FLEXT_TEMPIMPL(FLEXT_TEMPSUB(FLEXT_CLASSDEF(flext_base))::ItemCont *FLEXT_CLASSDEF(flext_base))::GetClassArr(t_classid c,int ix)
{
    ClassMap &map = classarr[ix];
    ItemCont *cont = map.find(c);
    if(!cont) map.insert(c,cont = new ItemCont);
    return cont;
}
*/

#include "flpopns.h"

#endif // __FLEXT_ITEM_CPP


