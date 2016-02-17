/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flmsg.cpp
    \brief Message processing of flext base class.
*/
 
#ifndef __FLEXT_MSG_CPP
#define __FLEXT_MSG_CPP

#include "flext.h"

#include "flpushns.h"

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::TryMethTag(Item *lst,const t_symbol *tag,int argc,const t_atom *argv)
{
    for(; lst; lst = lst->nxt) {
        MethItem *m = (MethItem *)lst;

//        FLEXT_LOG3("found method tag %s: inlet=%i, argc=%i",GetString(tag),m->inlet,argc);
    
        if(m->attr) {
            // attributes are treated differently

            if(m->attr->IsGet())
                return DumpAttrib(tag,m->attr);
            else
                return SetAttrib(tag,m->attr,argc,argv);
        }
        else {
            if(m->argc == 1) {
                if(m->args[0] == a_list) {
                    // try list
                    if(((methfun_V)m->fun)(this,argc,const_cast<t_atom *>(argv))) return true;
                }
                else if(m->args[0] == a_any) {
                    // try anything
                    if(((methfun_A)m->fun)(this,tag,argc,const_cast<t_atom *>(argv))) return true;
                }
            }

            // try matching number of args
            if(m->argc == argc) {
                int ix;
                t_any aargs[FLEXT_MAXMETHARGS];
                bool ok = true;
                for(ix = 0; ix < argc && ok; ++ix) {
                    switch(m->args[ix]) {
                    case a_float: {
                        if(IsFloat(argv[ix])) aargs[ix].ft = GetFloat(argv[ix]);
                        else if(IsInt(argv[ix])) aargs[ix].ft = (float)GetInt(argv[ix]);
                        else ok = false;
                        
                        if(ok) FLEXT_LOG2("int arg %i = %f",ix,aargs[ix].ft);
                        break;
                    }
                    case a_int: {
                        if(IsFloat(argv[ix])) aargs[ix].it = (int)GetFloat(argv[ix]);
                        else if(IsInt(argv[ix])) aargs[ix].it = GetInt(argv[ix]);
                        else ok = false;
                        
                        if(ok) FLEXT_LOG2("float arg %i = %i",ix,aargs[ix].it);
                        break;
                    }
                    case a_symbol: {
                        if(IsSymbol(argv[ix])) aargs[ix].st = GetSymbol(argv[ix]);
                        else ok = false;
                        
                        if(ok) FLEXT_LOG2("symbol arg %i = %s",ix,GetString(aargs[ix].st));
                        break;
                    }
#if FLEXT_SYS == FLEXT_SYS_PD
                    case a_pointer: {
                        if(IsPointer(argv[ix])) aargs[ix].pt = (t_gpointer *)GetPointer(argv[ix]);
                        else ok = false;
                        break;
                    }
#endif
                    default:
                        error("Argument type illegal");
                        ok = false;
                    }
                }

                if(ok && ix == argc) {
                    switch(argc) {
                    case 0: return ((methfun_0)m->fun)(this); 
                    case 1: return ((methfun_1)m->fun)(this,aargs[0]); 
                    case 2: return ((methfun_2)m->fun)(this,aargs[0],aargs[1]); 
                    case 3: return ((methfun_3)m->fun)(this,aargs[0],aargs[1],aargs[2]); 
                    case 4: return ((methfun_4)m->fun)(this,aargs[0],aargs[1],aargs[2],aargs[3]); 
                    case 5: return ((methfun_5)m->fun)(this,aargs[0],aargs[1],aargs[2],aargs[3],aargs[4]); 
                    default:
                        FLEXT_ASSERT(false);
                    }
                }
            }
        }
    }
    return false;
}


FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::TryMethAny(Item *lst,const t_symbol *s,int argc,const t_atom *argv)
{
    for(; lst; lst = lst->nxt) {
        MethItem *m = (MethItem *)lst;

        if(!m->IsAttr() && m->argc == 1 && m->args[0] == a_any) {
//          FLEXT_LOG4("found any method for %s: inlet=%i, symbol=%s, argc=%i",GetString(m->tag),m->inlet,GetString(s),argc);

            if(((methfun_A)m->fun)(this,s,argc,const_cast<t_atom *>(argv))) return true;
        }
    }
    return false;
}

/*! \brief Find a method item for a specific tag and arguments
    \remark All attributes are also stored in the method list and retrieved by a member of the method item
*/
FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::FindMeth(int inlet,const t_symbol *s,int argc,const t_atom *argv)
{
    Item *lst;
    ItemCont *clmethhead = ClMeths(thisClassId());

    // search for exactly matching tag
    if(UNLIKELY(methhead) && (lst = methhead->FindList(s,inlet)) != NULL && TryMethTag(lst,s,argc,argv)) return true;
    if((lst = clmethhead->FindList(s,inlet)) != NULL && TryMethTag(lst,s,argc,argv)) return true;

    // if nothing found try any inlet
    if(UNLIKELY(methhead) && (lst = methhead->FindList(s,-1)) != NULL && TryMethTag(lst,s,argc,argv)) return true;
    if((lst = clmethhead->FindList(s,-1)) != NULL && TryMethTag(lst,s,argc,argv)) return true;

    return false;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::FindMethAny(int inlet,const t_symbol *s,int argc,const t_atom *argv)
{
    Item *lst;
    ItemCont *clmethhead = ClMeths(thisClassId());

    if(UNLIKELY(methhead) && (lst = methhead->FindList(sym_anything,inlet)) != NULL && TryMethAny(lst,s,argc,argv)) return true;
    if((lst = clmethhead->FindList(sym_anything,inlet)) != NULL && TryMethAny(lst,s,argc,argv)) return true;

    // if nothing found try any inlet
    if(UNLIKELY(methhead) && (lst = methhead->FindList(sym_anything,-1)) != NULL && TryMethAny(lst,s,argc,argv)) return true;
    if((lst = clmethhead->FindList(sym_anything,-1)) != NULL && TryMethAny(lst,s,argc,argv)) return true;

    return false;
}

/*! \brief All the message processing
    The messages of all the inlets go here and are promoted to the registered callback functions
*/
FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::CbMethodHandler(int inlet,const t_symbol *s,int argc,const t_atom *argv)
{
    static bool trap = false;
    bool ret;

    curtag = s;

#ifdef FLEXT_LOG_MSGS
	post("methodmain inlet:%i args:%i symbol:%s",inlet,argc,s?GetString(s):"");
#endif

    try {
        ret = FindMeth(inlet,s,argc,argv);
#ifdef FLEXT_LOG_MSGS
		if(ret) post("found %s message in %s,%i",GetString(s),__FILE__,__LINE__);
#endif
        if(ret) goto end;

        if(argc == 1) {
            if(s == sym_list) {
                // for 1-element lists try the single atom (this is the format output by [route])
                if(IsFloat(argv[0]))
                    ret = FindMeth(inlet,sym_float,1,argv);
                else if(IsInt(argv[0]))
                    ret = FindMeth(inlet,sym_int,1,argv);
                else if(IsSymbol(argv[0]))
                    ret = FindMeth(inlet,sym_symbol,1,argv);
    #if FLEXT_SYS == FLEXT_SYS_PD && !defined(FLEXT_COMPATIBLE)
                else if(IsPointer(argv[0]))
                    ret = FindMeth(inlet,sym_pointer,1,argv);
    #endif
                if(ret) goto end;
            }
            else {
                if(s == sym_float) {
    #if FLEXT_SYS == FLEXT_SYS_MAX
                    t_atom at;
                    // If float message is not explicitly handled: try int handler instead
                    SetInt(at,(int)GetFloat(argv[0]));
                    ret = FindMeth(inlet,sym_int,1,&at);
                    if(ret) goto end;
    #endif
                    // If not explicitly handled: try list handler instead
                    ret = FindMeth(inlet,sym_list,1,argv);
                    if(ret) goto end;
                }
    #if FLEXT_SYS == FLEXT_SYS_MAX
                else if(s == sym_int) {
                    t_atom at;
                    // If int message is not explicitly handled: try float handler instead
                    SetFloat(at,(float)GetInt(argv[0]));
                    ret = FindMeth(inlet,sym_float,1,&at);
                    if(ret) goto end;
                    // If not explicitly handled: try list handler instead
                    ret = FindMeth(inlet,sym_list,1,argv);
                    if(ret) goto end;
                }
    #endif
                else if(s == sym_symbol) {
                    ret = FindMeth(inlet,sym_list,1,argv);
                    if(ret) goto end;
                }
    #if FLEXT_SYS == FLEXT_SYS_PD && !defined(FLEXT_COMPATIBLE)
                else if(s == sym_pointer) {
                    ret = FindMeth(inlet,sym_list,1,argv);
                    if(ret) goto end;
                }
    #endif
            }
        }
        else if(argc == 0) {
            // If symbol message (pure anything without args) is not explicitly handled: try list handler instead
            if(s == sym_bang)
                // bang is equal to an empty list
                ret = FindMeth(inlet,sym_list,0,NULL);
            else {
                t_atom at;
                SetSymbol(at,s);
                ret = FindMeth(inlet,sym_list,1,&at);
            }
#ifdef FLEXT_LOG_MSGS
			if(ret) post("found %s message in %s,%i",GetString(sym_list),__FILE__,__LINE__);
#endif
            if(ret) goto end;
        }

        // if distmsgs is switched on then distribute list elements over inlets (Max/MSP behavior)
        if(DoDist() && inlet == 0 && s == sym_list && insigs <= 1 && !trap) {
            int i = incnt;
            if(i > argc) i = argc;
            for(--i; i >= 0; --i) { // right to left distribution
                const t_symbol *sym = NULL;
                if(IsFloat(argv[i])) sym = sym_float;
                else if(IsInt(argv[i])) sym = sym_int;
                else if(IsSymbol(argv[i])) sym = sym_symbol;
    #if FLEXT_SYS == FLEXT_SYS_PD && !defined(FLEXT_COMPATIBLE)
                else if(IsPointer(argv[i])) sym = sym_pointer;  // can pointer atoms occur here?
    #endif

                if(sym) {
                    trap = true;
                    CbMethodHandler(i,sym,1,argv+i);
                    trap = false;
                }
            }
            
            goto end;
        }
        
        ret = FindMethAny(inlet,s,argc,argv);

        if(!ret) ret = CbMethodResort(inlet,s,argc,argv);
    }
    catch(std::exception &x) {
        error("%s - %s: %s",thisName(),GetString(s),x.what());
        ret = false;
    }
    catch(const char *txt) {
        error("%s - %s: %s",thisName(),GetString(s),txt);
        ret = false;
    }
    catch(...) {
        error("%s - %s : Unknown exception while processing method",thisName(),GetString(s));
        ret = false;
    }

end:
    curtag = NULL;

    return ret; // true if appropriate handler was found and called
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::m_method_(int inlet,const t_symbol *s,int argc,const t_atom *argv)
{
    post("%s: message unhandled - inlet:%i args:%i symbol:%s",thisName(),inlet,argc,s?GetString(s):"");
    return false;
}

FLEXT_TEMPIMPL(bool FLEXT_CLASSDEF(flext_base))::CbMethodResort(int inlet,const t_symbol *s,int argc,const t_atom *argv)
{
    // call deprecated version
    return m_method_(inlet,s,argc,argv);
}

#include "flpopns.h"

#endif // __FLEXT_MSG_CPP


