/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flproxy.cpp
    \brief Proxy classes for the flext base class.
*/
 
#ifndef __FLEXT_PROXY_CPP
#define __FLEXT_PROXY_CPP

#include "flext.h"
#include "flinternal.h"

#include "flpushns.h"

// === proxy class for flext_base ============================

#if FLEXT_SYS == FLEXT_SYS_PD

FLEXT_TEMPIMPL(t_class *FLEXT_CLASSDEF(flext_base))::px_class = NULL;

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::px_object::px_bang(px_object *obj)
{
    Locker lock(obj->base);
    obj->base->CbMethodHandler(obj->index,sym_bang,0,NULL);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::px_object::px_float(px_object *obj,t_float f)
{
    t_atom a; SetFloat(a,f);
    Locker lock(obj->base);
    obj->base->CbMethodHandler(obj->index,sym_float,1,&a);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::px_object::px_symbol(px_object *obj,const t_symbol *s)
{
    t_atom a; SetSymbol(a,s);
    Locker lock(obj->base);
    obj->base->CbMethodHandler(obj->index,sym_symbol,1,&a);
}

/*
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::px_object::px_pointer(px_object *obj,const t_gpointer *p)
{
    t_atom a; SetPointer(a,p);
    Locker lock(obj->base);
    obj->base->CbMethodHandler(obj->index,sym_pointer,1,&a);
}
*/

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::px_object::px_anything(px_object *obj,const t_symbol *s,int argc,t_atom *argv)
{
    Locker lock(obj->base);
    obj->base->CbMethodHandler(obj->index,s,argc,argv);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_bang(flext_hdr *c)
{
    Locker lock(c);
    thisObject(c)->CbMethodHandler(0,sym_bang,0,NULL);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_float(flext_hdr *c,t_float f)
{
    t_atom a; SetFloat(a,f);
    Locker lock(c);
    thisObject(c)->CbMethodHandler(0,sym_float,1,&a);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_symbol(flext_hdr *c,const t_symbol *s)
{
    t_atom a; SetSymbol(a,s);
    Locker lock(c);
    thisObject(c)->CbMethodHandler(0,sym_symbol,1,&a);
}

/*
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_pointer(flext_hdr *c,const t_gpointer *p)
{
    t_atom a; SetPointer(a,p);
    Locker lock(c);
    thisObject(c)->CbMethodHandler(0,sym_pointer,1,&a);
}
*/

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_anything(flext_hdr *c,const t_symbol *s,int argc,t_atom *argv)
{
    Locker lock(c);
    if(UNLIKELY(!s)) {
        // apparently, this happens only in one case... object is a DSP object, but has no main DSP inlet...

        // interpret tag from args
        if(!argc)
            s = sym_bang;
        else if(argc == 1) {
            if(IsFloat(*argv))
                s = sym_float;
            else if(IsSymbol(*argv))
                s = sym_symbol;
            else if(IsPointer(*argv))
                s = sym_pointer;
            else
                FLEXT_ASSERT(false);
        }
        else
            s = sym_list;
    }

    thisObject(c)->CbMethodHandler(0,s,argc,argv);
}

#define DEF_PROXYMSG(IX) \
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_px_ft ## IX(flext_hdr *c,t_float v) { t_atom atom; SetFloat(atom,v); Locker lock(c); thisObject(c)->CbMethodHandler(IX,sym_float,1,&atom); }

#define ADD_PROXYMSG(c,IX) \
add_method1(c,cb_px_ft ## IX," ft " #IX,A_FLOAT)

//AddMethod(c,0,flext::MakeSymbol("ft" #IX),cb_px_ft ## IX) 


#elif FLEXT_SYS == FLEXT_SYS_MAX

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_anything(flext_hdr *c,const t_symbol *s,short argc,t_atom *argv)
{
    Locker lock(c);
    int const ci = proxy_getinlet((t_object *)&c->obj);
//    post("%s %i, cb_anything(%i)",__FILE__,__LINE__,ci);
    thisObject(c)->CbMethodHandler(ci,s,argc,argv);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_int(flext_hdr *c,long v)
{
    t_atom atom; SetInt(atom,v);
    Locker lock(c);
    int const ci = proxy_getinlet((t_object *)&c->obj);
    thisObject(c)->CbMethodHandler(ci,sym_int,1,&atom);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_float(flext_hdr *c,double v)
{
    t_atom atom; SetFloat(atom,v);
    Locker lock(c);
    int const ci = proxy_getinlet((t_object *)&c->obj);
    thisObject(c)->CbMethodHandler(ci,sym_float,1,&atom);
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_bang(flext_hdr *c)
{
    Locker lock(c);
    int const ci = proxy_getinlet((t_object *)&c->obj);
    thisObject(c)->CbMethodHandler(ci,sym_bang,0,NULL);
}


#define DEF_PROXYMSG(IX) \
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_px_in ## IX(flext_hdr *c,long v) { t_atom atom; SetInt(atom,v); Locker lock(c); thisObject(c)->CbMethodHandler(IX,sym_int,1,&atom); } \
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_px_ft ## IX(flext_hdr *c,double v) { t_atom atom; SetFloat(atom,v); Locker lock(c); thisObject(c)->CbMethodHandler(IX,sym_float,1,&atom); }

/*
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_px_in ## IX(flext_hdr *c,long v) { t_atom atom; SetInt(atom,v); Locker lock(c); thisObject(c)->CbMethodHandler(IX,sym_int,1,&atom); } \
FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::cb_px_ft ## IX(flext_hdr *c,double v) { t_atom atom; SetFloat(atom,v); Locker lock(c); thisObject(c)->CbMethodHandler(IX,sym_float,1,&atom); }
*/

#define ADD_PROXYMSG(c,IX) \
addinx((method)(cb_px_in ## IX),IX); \
addftx((method)(cb_px_ft ## IX),IX)

/*
add_method1(c,cb_px_in ## IX,"in" #IX,A_INT); \
add_method1(c,cb_px_ft ## IX,"ft" #IX,A_FLOAT)

AddMethod(c,0,flext::MakeSymbol("in" #IX),cb_px_in ## IX); \
AddMethod(c,0,flext::MakeSymbol("ft" #IX),cb_px_ft ## IX) 
*/

#endif 

#if FLEXT_SYS == FLEXT_SYS_PD || FLEXT_SYS == FLEXT_SYS_MAX

DEF_PROXYMSG(1)
DEF_PROXYMSG(2)
DEF_PROXYMSG(3)
DEF_PROXYMSG(4)
DEF_PROXYMSG(5)
DEF_PROXYMSG(6)
DEF_PROXYMSG(7)
DEF_PROXYMSG(8)
DEF_PROXYMSG(9)


FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext_base))::SetProxies(t_class *c,bool dsp)
{
#if FLEXT_SYS == FLEXT_SYS_PD
    // for leftmost inlet
    class_addbang(c,cb_bang);
    if(!dsp) class_addfloat(c,cb_float);
    class_addsymbol(c,cb_symbol);
//    class_addpointer(c,cb_pointer);
    class_addlist(c,cb_anything);
    class_addanything(c,cb_anything);

    // proxy for extra inlets
    if(UNLIKELY(!px_class)) {
        // only once
        px_class = class_new(gensym(const_cast<char *>(" flext_base proxy ")),NULL,NULL,sizeof(px_object),CLASS_PD|CLASS_NOINLET, A_NULL);
        class_addbang(px_class,px_object::px_bang); // for other inlets
        class_addfloat(px_class,px_object::px_float); // for other inlets
        class_addsymbol(px_class,px_object::px_symbol); // for other inlets
//        class_addpointer(px_class,px_object::px_pointer); // for other inlets
        class_addlist(px_class,px_object::px_anything); // for other inlets
        class_addanything(px_class,px_object::px_anything); // for other inlets
    }
#elif FLEXT_SYS == FLEXT_SYS_MAX
    addbang((method)cb_bang);
    addint((method)cb_int);  
    addfloat((method)cb_float);  
    addmess((method)cb_anything,const_cast<char *>("list"),A_GIMME,A_NOTHING); // must be explicitly given, otherwise list elements are distributed over inlets
    addmess((method)cb_anything,const_cast<char *>("anything"),A_GIMME,A_NOTHING);
#else
#error Not implemented!
#endif  

    // setup non-leftmost ints and floats
    ADD_PROXYMSG(c,1);
    ADD_PROXYMSG(c,2);
    ADD_PROXYMSG(c,3);
    ADD_PROXYMSG(c,4);
    ADD_PROXYMSG(c,5);
    ADD_PROXYMSG(c,6);
    ADD_PROXYMSG(c,7);
    ADD_PROXYMSG(c,8);
    ADD_PROXYMSG(c,9);
}
#endif

#include "flpopns.h"

#endif // __FLEXT_PROXY_CPP


