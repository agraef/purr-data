/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file flout.cpp
    \brief Implementation of the flext outlet functionality.
*/

#include "flext.h"
#include "flinternal.h"
#include <cstring>
 
#include "flpushns.h"

#if FLEXT_SYS == FLEXT_SYS_PD || FLEXT_SYS == FLEXT_SYS_MAX
void flext_base::ToSysAtom(int n,const t_atom &at) const 
{ 
    outlet *o = GetOut(n); 
    if(LIKELY(o)) { 
        CRITON(); 
        if(IsSymbol(at))
            outlet_symbol((t_outlet *)o,const_cast<t_symbol *>(GetSymbol(at))); 
        else if(IsFloat(at))
            outlet_float((t_outlet *)o,GetFloat(at)); 
#if FLEXT_SYS == FLEXT_SYS_MAX
        else if(IsInt(at))
            outlet_flint((t_outlet *)o,GetInt(at));
#endif
#if FLEXT_SYS == FLEXT_SYS_PD
        else if(IsPointer(at))
            outlet_pointer((t_outlet *)o,GetPointer(at)); 
#endif
        else
            error("Atom type not supported");
        CRITOFF(); 
    } 
}
#else
#error Not implemented
#endif

#if defined(FLEXT_THREADS)
    #if FLEXT_QMODE == 2
        #define CHKTHR() (LIKELY((!IsThreadRegistered() || IsThread(flext::thrmsgid)) && !InDSP()))
    #else
        #define CHKTHR() (LIKELY(!IsThreadRegistered() && !InDSP()))
    #endif
#else
    #define CHKTHR() (LIKELY(!InDSP()))
#endif

void flext_base::ToOutBang(int n) const { if(CHKTHR()) ToSysBang(n); else ToQueueBang(n); }
void flext_base::ToOutFloat(int n,float f) const { if(CHKTHR()) ToSysFloat(n,f); else ToQueueFloat(n,f); }
void flext_base::ToOutInt(int n,int f) const { if(CHKTHR()) ToSysInt(n,f); else ToQueueInt(n,f); }
void flext_base::ToOutSymbol(int n,const t_symbol *s) const { if(CHKTHR()) ToSysSymbol(n,s); else ToQueueSymbol(n,s); }
void flext_base::ToOutAtom(int n,const t_atom &at) const { if(CHKTHR()) ToSysAtom(n,at); else ToQueueAtom(n,at); }
void flext_base::ToOutList(int n,int argc,const t_atom *argv) const { if(CHKTHR()) ToSysList(n,argc,argv); else ToQueueList(n,argc,argv); }
void flext_base::ToOutAnything(int n,const t_symbol *s,int argc,const t_atom *argv) const { if(CHKTHR()) ToSysAnything(n,s,argc,argv); else ToQueueAnything(n,s,argc,argv); }

void flext::ToOutMsg(MsgBundle *mb) { if(CHKTHR()) ToSysMsg(mb); else ToQueueMsg(mb); }

bool flext::Forward(const t_symbol *recv,const t_symbol *s,int argc,const t_atom *argv)
{
    return CHKTHR()?SysForward(recv,s,argc,argv):QueueForward(recv,s,argc,argv); 
}


bool flext_base::InitInlets()
{
    bool ok = true;

    // incnt has number of inlets (any type)
    // insigs should be 0

    FLEXT_ASSERT(!insigs && !inlets);

    // ----------------------------------
    // create inlets
    // ----------------------------------

#if FLEXT_SYS == FLEXT_SYS_MAX      
    // copy inlet descriptions
    indesc = new char *[incnt];
    for(int i = 0; i < incnt; ++i) {
        xlet &x = inlist[i];
        indesc[i] = x.desc;
        x.desc = NULL;
    }
#endif

#if FLEXT_SYS == FLEXT_SYS_PD || FLEXT_SYS == FLEXT_SYS_MAX
    inlets = incnt > 1?new px_object *[incnt-1]:NULL;
#endif
    
    // type info is now in list array
#if FLEXT_SYS == FLEXT_SYS_PD
    {
        int cnt = 0;
        if(incnt >= 1) {
            xlet &xi = inlist[0]; // points to first inlet
            if(xi.tp == xlet_sig) ++insigs;
            // else leftmost inlet is already there...
            ++cnt;

#if PD_MINOR_VERSION >= 37 && defined(PD_DEVEL_VERSION)
            // set tooltip
// this is on a per-class basis... we cannot really use it here
//            if(xi.desc && *xi.desc) class_settip(thisClass(),gensym(xi.desc));
#endif
        }       

        for(int ix = 1; ix < incnt; ++ix,++cnt) {
            xlet &xi = inlist[ix]; // points to first inlet
            t_inlet *in = NULL;
            switch(xi.tp) {
                case xlet_float:
                case xlet_int: {
                    if(ix > 9) { 
                        // proxy inlet needed
                        (inlets[ix-1] = (px_object *)pd_new(px_class))->init(this,ix);  // proxy for 2nd inlet messages 
                        in = inlet_new(&x_obj->obj,&inlets[ix-1]->obj.ob_pd, (t_symbol *)sym_float, (t_symbol *)sym_float);  
                    }
                    else { 
                        inlets[ix-1] = NULL;
                        static char sym[] = " ft ?";
                        sym[4] = '0'+ix;  
                        in = inlet_new(&x_obj->obj, &x_obj->obj.ob_pd, (t_symbol *)sym_float, gensym(sym)); 
                    }
                    break;
                }
                case xlet_sym: 
                    (inlets[ix-1] = (px_object *)pd_new(px_class))->init(this,ix);  // proxy for 2nd inlet messages 
                    in = inlet_new(&x_obj->obj,&inlets[ix-1]->obj.ob_pd, (t_symbol *)sym_symbol, (t_symbol *)sym_symbol);  
                    break;
                case xlet_list:
                    (inlets[ix-1] = (px_object *)pd_new(px_class))->init(this,ix);  // proxy for 2nd inlet messages 
                    in = inlet_new(&x_obj->obj,&inlets[ix-1]->obj.ob_pd, (t_symbol *)sym_list, (t_symbol *)sym_list);  
                    break;
                case xlet_any:
                    (inlets[ix-1] = (px_object *)pd_new(px_class))->init(this,ix);  // proxy for 2nd inlet messages 
                    in = inlet_new(&x_obj->obj,&inlets[ix-1]->obj.ob_pd, 0, 0);  
                    break;
                case xlet_sig:
                    inlets[ix-1] = NULL;
#ifdef FLEXT_COMPATIBLE
                    if(inlist[ix-1].tp != xlet_sig) {
                        post("%s: All signal inlets must be left-aligned in compatibility mode",thisName());
                        ok = false;
                    }
                    else 
#endif
                    {
                        // pd is not able to handle signals and messages into the same inlet...
                        in = inlet_new(&x_obj->obj, &x_obj->obj.ob_pd, (t_symbol *)sym_signal, (t_symbol *)sym_signal);  
                        ++insigs;
                    }
                    break;
                default:
                    inlets[ix-1] = NULL;
                    error("%s: Wrong type for inlet #%i: %i",thisName(),ix,(int)inlist[ix].tp);
                    ok = false;
            } 

#if PD_MINOR_VERSION >= 37 && defined(PD_DEVEL_VERSION)
            // set tooltip
            if(in && xi.desc && *xi.desc) inlet_settip(in,gensym(xi.desc));
#endif
        }

        incnt = cnt;
    }
#elif FLEXT_SYS == FLEXT_SYS_MAX
    {
        int ix,cnt;
        // count leftmost signal inlets
        while(insigs < incnt && inlist[insigs].tp == xlet_sig) ++insigs;
        
        for(cnt = 0,ix = incnt-1; ix >= insigs; --ix,++cnt) {
            xlet &xi = inlist[ix];
            if(!ix) {
                if(xi.tp != xlet_any) {
                    error("%s: Leftmost inlet must be of type signal or anything",thisName());
                    ok = false;
                }
            }
            else {
                FLEXT_ASSERT(inlets);
                switch(xi.tp) {
                    case xlet_sig:
                        inlets[ix-1] = NULL;
                        error("%s: All signal inlets must be left-aligned",thisName());
                        ok = false;
                        break;
                    case xlet_float: {
						if(ix < 10) {
							inlets[ix-1] = NULL;
                            floatin(x_obj,ix);
							break;
						}
						else
							goto makeproxy;
					}
                    case xlet_int: {
						if(ix < 10) {
							inlets[ix-1] = NULL;
                            intin(x_obj,ix);
							break;
						}
						else
							goto makeproxy;
					}
					makeproxy:
                    case xlet_any: // non-leftmost
                    case xlet_sym:
                    case xlet_list:
                        inlets[ix-1] = (px_object *)proxy_new(x_obj,ix,&((flext_hdr *)x_obj)->curinlet);  
                        break;
                    default:
                        inlets[ix-1] = NULL;
                        error("%s: Wrong type for inlet #%i: %i",thisName(),ix,(int)xi.tp);
                        ok = false;
                } 
            }
        }
        
        if(inlets)
            while(ix >= 0) inlets[ix--] = NULL;
	}
#else
#error
#endif

    return ok;  
}

bool flext_base::InitOutlets()
{
    bool ok = true;
    int procattr = HasAttributes()?1:0;

    // outcnt has number of inlets (any type)
    // outsigs should be 0

    FLEXT_ASSERT(outsigs == 0);

    // ----------------------------------
    // create outlets
    // ----------------------------------

#if FLEXT_SYS == FLEXT_SYS_MAX
    // for Max/MSP the rightmost outlet has to be created first
    outlet *attrtmp = NULL;
    if(procattr) 
        attrtmp = (outlet *)newout_anything(thisHdr());
#endif

#if FLEXT_SYS == FLEXT_SYS_MAX      
    // copy outlet descriptions
    outdesc = new char *[outcnt];
    for(int i = 0; i < outcnt; ++i) {
        xlet &xi = outlist[i];
        outdesc[i] = xi.desc; 
        xi.desc = NULL;
    }
#endif

#if FLEXT_SYS == FLEXT_SYS_PD || FLEXT_SYS == FLEXT_SYS_MAX
    if(outcnt+procattr)
        outlets = new outlet *[outcnt+procattr];
    else
        outlets = NULL;

    // type info is now in list array
#if FLEXT_SYS == FLEXT_SYS_PD
    for(int ix = 0; ix < outcnt; ++ix) 
#elif FLEXT_SYS == FLEXT_SYS_MAX
    for(int ix = outcnt-1; ix >= 0; --ix) 
#else
#error
#endif
    {
        switch(outlist[ix].tp) {
            case xlet_float:
                outlets[ix] = (outlet *)newout_float(&x_obj->obj);
                break;
            case xlet_int: 
                outlets[ix] = (outlet *)newout_flint(&x_obj->obj);
                break;
            case xlet_sig:
                outlets[ix] = (outlet *)newout_signal(&x_obj->obj);
                ++outsigs;
                break;
            case xlet_sym:
                outlets[ix] = (outlet *)newout_symbol(&x_obj->obj);
                break;
            case xlet_list:
                outlets[ix] = (outlet *)newout_list(&x_obj->obj);
                break;
            case xlet_any:
                outlets[ix] = (outlet *)newout_anything(&x_obj->obj);
                break;
#ifdef FLEXT_DEBUG
            default:
                ERRINTERNAL();
                ok = false;
#endif
        } 
    }
#else
#error
#endif

#if FLEXT_SYS == FLEXT_SYS_PD || FLEXT_SYS == FLEXT_SYS_MAX
    if(procattr) {
        // attribute dump outlet is the last one
        outlets[outcnt] = 
#if FLEXT_SYS == FLEXT_SYS_PD
        // attribute dump outlet is the last one
            (outlet *)newout_anything(&x_obj->obj);
#elif FLEXT_SYS == FLEXT_SYS_MAX
            attrtmp;
#endif

    }
#endif
    
    return ok;
}

#include "flpopns.h"

