/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file fldefs_methadd.h
    \brief This file contains all #defines for actual usage
    
*/

#ifndef __FLEXT_DEFS_METHADD_H
#define __FLEXT_DEFS_METHADD_H


/*! \defgroup FLEXT_D_CADDMETHOD Add flext methods within class scope
    \ingroup FLEXT_D_METHOD
    \note These can only be used at class construction time
    @{ 
*/

/*! Add a method handler for bang 
    \note This is for compatibility - better use the method below
*/
#define FLEXT_CADDBANG(CL,IX,M_FUN) \
\
AddMethod(CL,IX,FLEXT_CALL_PRE(M_FUN))   

//! Add a handler for a method with either no, list or anything arguments
#define FLEXT_CADDMETHOD(CL,IX,M_FUN) \
\
AddMethod(CL,IX,FLEXT_CALL_PRE(M_FUN))  

//! Add a a handler for a method with implicit arguments
#define FLEXT_CADDMETHOD_(CL,IX,M_TAG,M_FUN) \
\
AddMethod(CL,IX,flext::MakeSymbol(M_TAG),FLEXT_CALL_PRE(M_FUN))    

//! Add a handler for a method with 1 enum type argument
#define FLEXT_CADDMETHOD_E(CL,IX,M_TAG,M_FUN) \
\
AddMethod(ClMeths(CL),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),a_int,a_null)

//! Add a handler for a method with 1 argument
#define FLEXT_CADDMETHOD_1(CL,IX,M_TAG,M_FUN,TP1) \
\
AddMethod(ClMeths(CL),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),FLEXTARG(TP1),a_null)   

//! Add a handler for a method with 2 arguments
#define FLEXT_CADDMETHOD_2(CL,IX,M_TAG,M_FUN,TP1,TP2) \
\
AddMethod(ClMeths(CL),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),FLEXTARG(TP1),FLEXTARG(TP2),a_null)

//! Add a handler for a method with 3 arguments
#define FLEXT_CADDMETHOD_3(CL,IX,M_TAG,M_FUN,TP1,TP2,TP3) \
\
AddMethod(ClMeths(CL),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),FLEXTARG(TP1),FLEXTARG(TP2),FLEXTARG(TP3),a_null)

//! Add a handler for a method with 4 arguments
#define FLEXT_CADDMETHOD_4(CL,IX,M_TAG,M_FUN,TP1,TP2,TP3,TP4) \
\
AddMethod(ClMeths(CL),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),FLEXTARG(TP1),FLEXTARG(TP2),FLEXTARG(TP3),FLEXTARG(TP4),a_null)

//! Add a handler for a method with 5 arguments
#define FLEXT_CADDMETHOD_5(CL,IX,M_TAG,M_FUN,TP1,TP2,TP3,TP4,TP5) \
\
AddMethod(ClMeths(CL),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),FLEXTARG(TP1),FLEXTARG(TP2),FLEXTARG(TP3),FLEXTARG(TP4),FLEXTARG(TP5),a_null)


//  Shortcuts

//! Add a handler for a method with a boolean argument
#define FLEXT_CADDMETHOD_B(CL,IX,M_TAG,M_FUN) \
\
FLEXT_CADDMETHOD_1(CL,IX,flext::MakeSymbol(M_TAG),M_FUN,bool)

//! Add a handler for a method with 1 float argument
#define FLEXT_CADDMETHOD_F(CL,IX,M_TAG,M_FUN) \
\
FLEXT_CADDMETHOD_1(CL,IX,flext::MakeSymbol(M_TAG),M_FUN,float)

//! Add a handler for a method with 2 float arguments
#define FLEXT_CADDMETHOD_FF(CL,IX,M_TAG,M_FUN) \
\
FLEXT_CADDMETHOD_2(CL,IX,flext::MakeSymbol(M_TAG),M_FUN,float,float)

//! Add a handler for a method with 3 float arguments
#define FLEXT_CADDMETHOD_FFF(CL,IX,M_TAG,M_FUN) \
\
FLEXT_CADDMETHOD_3(CL,IX,flext::MakeSymbol(M_TAG),M_FUN,float,float,float)

//! Add a handler for a method with 1 integer argument
#define FLEXT_CADDMETHOD_I(CL,IX,M_TAG,M_FUN) \
\
FLEXT_CADDMETHOD_1(CL,IX,flext::MakeSymbol(M_TAG),M_FUN,int)

//! Add a handler for a method with 2 integer arguments
#define FLEXT_CADDMETHOD_II(CL,IX,M_TAG,M_FUN) \
\
FLEXT_CADDMETHOD_2(CL,IX,flext::MakeSymbol(M_TAG),M_FUN,int,int)

//! Add a handler for a method with 3 integer arguments
#define FLEXT_CADDMETHOD_III(CL,IX,M_TAG,M_FUN) \
\
FLEXT_CADDMETHOD_3(CL,IX,flext::MakeSymbol(M_TAG),M_FUN,int,int,int)

//! @} FLEXT_D_CADDMETHOD


/*! \defgroup FLEXT_D_ADDMETHOD Add flext methods
    \ingroup FLEXT_D_METHOD
    \note These can only be used at object construction time 
    \note (in constructor or in Init() function before call to parent's Init())
    @{ 
*/

//! Set timer callback
#define FLEXT_ADDTIMER(TMR,M_FUN) \
\
TMR.SetCallback(*this,FLEXT_CALL_PRE(M_FUN))

//! Enable list element distribution over inlets (if no better handler found)
#define FLEXT_ADDDIST() \
\
SetDist(true)   

//! Add a method handler for bang 
#define FLEXT_ADDBANG(IX,M_FUN) \
\
AddMethod(IX,"bang",FLEXT_CALL_PRE(M_FUN))  

//! Add a handler for a method with either no, list or anything arguments
#define FLEXT_ADDMETHOD(IX,M_FUN) \
\
AddMethod(IX,FLEXT_CALL_PRE(M_FUN)) 

/*! \brief Add a handler for a method with a (variable argument) list
    \deprecated This definition obscures that _ indicates the usage of a message tag - use FLEXT_ADDMETHOD instead
    \note This is already covered by FLEXT_ADDMETHOD, but here for the sake of clarity
*/
#define FLEXT_ADDMETHOD_V(IX,M_FUN) \
\
AddMethod(IX,FLEXT_CALL_PRE(M_FUN)) 

/*! \brief Add a handler for a method with an anything argument
    \deprecated This definition obscures that _ indicates the usage of a message tag - use FLEXT_ADDMETHOD instead
    \note This is already covered by FLEXT_ADDMETHOD, but here for the sake of clarity
*/
#define FLEXT_ADDMETHOD_A(IX,M_FUN) \
\
AddMethod(IX,FLEXT_CALL_PRE(M_FUN)) 

//! Add a a handler for a tagged method with implicit arguments
#define FLEXT_ADDMETHOD_(IX,M_TAG,M_FUN) \
\
AddMethod(IX,flext::MakeSymbol(M_TAG),FLEXT_CALL_PRE(M_FUN))   

//! Add a handler for a method with 1 enum type argument
#define FLEXT_ADDMETHOD_E(IX,M_TAG,M_FUN) \
\
AddMethod(ThMeths(),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),a_int,a_null)

//! Add a handler for a method with 1 argument
#define FLEXT_ADDMETHOD_1(IX,M_TAG,M_FUN,TP1) \
\
AddMethod(ThMeths(),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),FLEXTARG(TP1),a_null) 

//! Add a handler for a method with 2 arguments
#define FLEXT_ADDMETHOD_2(IX,M_TAG,M_FUN,TP1,TP2) \
\
AddMethod(ThMeths(),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),FLEXTARG(TP1),FLEXTARG(TP2),a_null)

//! Add a handler for a method with 3 arguments
#define FLEXT_ADDMETHOD_3(IX,M_TAG,M_FUN,TP1,TP2,TP3) \
\
AddMethod(ThMeths(),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),FLEXTARG(TP1),FLEXTARG(TP2),FLEXTARG(TP3),a_null)

//! Add a handler for a method with 4 arguments
#define FLEXT_ADDMETHOD_4(IX,M_TAG,M_FUN,TP1,TP2,TP3,TP4) \
\
AddMethod(ThMeths(),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),FLEXTARG(TP1),FLEXTARG(TP2),FLEXTARG(TP3),FLEXTARG(TP4),a_null)

//! Add a handler for a method with 5 arguments
#define FLEXT_ADDMETHOD_5(IX,M_TAG,M_FUN,TP1,TP2,TP3,TP4,TP5) \
\
AddMethod(ThMeths(),IX,flext::MakeSymbol(M_TAG),(methfun)(FLEXT_CALL_PRE(M_FUN)),FLEXTARG(TP1),FLEXTARG(TP2),FLEXTARG(TP3),FLEXTARG(TP4),FLEXTARG(TP5),a_null)


//  Shortcuts

//! Add a handler for a method with a boolean argument
#define FLEXT_ADDMETHOD_B(IX,M_TAG,M_FUN) \
\
FLEXT_ADDMETHOD_1(IX,flext::MakeSymbol(M_TAG),M_FUN,bool)

//! Add a handler for a method with 1 float argument
#define FLEXT_ADDMETHOD_F(IX,M_TAG,M_FUN) \
\
FLEXT_ADDMETHOD_1(IX,flext::MakeSymbol(M_TAG),M_FUN,float)

//! Add a handler for a method with 2 float arguments
#define FLEXT_ADDMETHOD_FF(IX,M_TAG,M_FUN) \
\
FLEXT_ADDMETHOD_2(IX,flext::MakeSymbol(M_TAG),M_FUN,float,float)

//! Add a handler for a method with 3 float arguments
#define FLEXT_ADDMETHOD_FFF(IX,M_TAG,M_FUN) \
\
FLEXT_ADDMETHOD_3(IX,flext::MakeSymbol(M_TAG),M_FUN,float,float,float)

//! Add a handler for a method with 1 integer argument
#define FLEXT_ADDMETHOD_I(IX,M_TAG,M_FUN) \
\
FLEXT_ADDMETHOD_1(IX,flext::MakeSymbol(M_TAG),M_FUN,int)

//! Add a handler for a method with 2 integer arguments
#define FLEXT_ADDMETHOD_II(IX,M_TAG,M_FUN) \
\
FLEXT_ADDMETHOD_2(IX,flext::MakeSymbol(M_TAG),M_FUN,int,int)

//! Add a handler for a method with 3 integer arguments
#define FLEXT_ADDMETHOD_III(IX,M_TAG,M_FUN) \
\
FLEXT_ADDMETHOD_3(IX,flext::MakeSymbol(M_TAG),M_FUN,int,int,int)


//! @} FLEXT_D_ADDMETHOD

#endif
