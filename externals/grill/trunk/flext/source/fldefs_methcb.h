/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 1233 $
$LastChangedDate: 2008-01-17 11:11:19 -0500 (Thu, 17 Jan 2008) $
$LastChangedBy: thomas $
*/

/*! \file fldefs_methcb.h
    \brief This file contains all #defines for actual usage
    
*/

#ifndef __FLEXT_DEFS_METHCB_H
#define __FLEXT_DEFS_METHCB_H


/*! \defgroup FLEXT_D_CALLBACK Declare callbacks for class methods
    \ingroup FLEXT_D_METHOD
    @{ 
*/

//! Set up a method callback with no arguments
#define FLEXT_CALLBACK(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(); return true; }

//! Set up a method callback for an anything argument
#define FLEXT_CALLBACK_A(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,t_symbol *s,int argc,t_atom *argv) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(s,argc,argv); return true; }

//! Set up a method callback for a variable argument list
#define FLEXT_CALLBACK_V(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,int argc,t_atom *argv) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(argc,argv); return true; }

//! Set up a method callback for a data package (void *) argument
#define FLEXT_CALLBACK_X(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,void *data) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(data); return true; }

//! Set up a method callback for an anything argument and a data package (e.g. for symbol-bound methods).
#define FLEXT_CALLBACK_AX(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,t_symbol *s,int argc,t_atom *argv,void *data) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(s,argc,argv,data); return true; }

//! Set up a timer callback
#define FLEXT_CALLBACK_T(M_FUN) \
\
FLEXT_CALLBACK_X(M_FUN)

//! Set up a method callback for a boolean argument
#define FLEXT_CALLBACK_B(M_FUN) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,int &arg1) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(arg1 != 0); return true; }

//! Set up a method callback for 1 argument
#define FLEXT_CALLBACK_1(M_FUN,TP1) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,TP1 &arg1) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(arg1); return true; }

//! Set up a method callback for 2 arguments
#define FLEXT_CALLBACK_2(M_FUN,TP1,TP2) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,TP1 &arg1,TP2 &arg2) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(arg1,arg2); return true; }

//! Set up a method callback for 3 arguments
#define FLEXT_CALLBACK_3(M_FUN,TP1,TP2,TP3) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,TP1 &arg1,TP2 &arg2,TP3 &arg3) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(arg1,arg2,arg3); return true; }

//! Set up a method callback for 4 arguments
#define FLEXT_CALLBACK_4(M_FUN,TP1,TP2,TP3,TP4) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,TP1 &arg1,TP2 &arg2,TP3 &arg3,TP4 &arg4) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(arg1,arg2,arg3,arg4); return true; }

//! Set up a method callback for 5 arguments
#define FLEXT_CALLBACK_5(M_FUN,TP1,TP2,TP3,TP4,TP5) \
static bool FLEXT_CALL_PRE(M_FUN)(flext_base *c,TP1 &arg1,TP2 &arg2,TP3 &arg3,TP4 &arg4,TP5 &arg5) \
{ FLEXT_CAST<thisType *>(c)->M_FUN(arg1,arg2,arg3,arg4,arg5); return true; }


//  Shortcuts

//! Set up a method callback for 1 float argument
#define FLEXT_CALLBACK_F(M_FUN) \
\
FLEXT_CALLBACK_1(M_FUN,float)

//! Set up a method callback for 2 float arguments
#define FLEXT_CALLBACK_FF(M_FUN) \
\
FLEXT_CALLBACK_2(M_FUN,float,float)

//! Set up a method callback for 3 float arguments
#define FLEXT_CALLBACK_FFF(M_FUN) \
\
FLEXT_CALLBACK_3(M_FUN,float,float,float)

//! Set up a method callback for 1 integer argument
#define FLEXT_CALLBACK_I(M_FUN) \
\
FLEXT_CALLBACK_1(M_FUN,int)

//! Set up a method callback for 2 integer arguments
#define FLEXT_CALLBACK_II(M_FUN) \
\
FLEXT_CALLBACK_2(M_FUN,int,int)

//! Set up a method callback for 3 integer arguments
#define FLEXT_CALLBACK_III(M_FUN) \
\
FLEXT_CALLBACK_3(M_FUN,int,int,int)

//! Set up a method callback for 1 symbol argument
#define FLEXT_CALLBACK_S(M_FUN) \
\
FLEXT_CALLBACK_1(M_FUN,t_symptr)


//! \deprecated
#define FLEXT_CALLBACK_G FLEXT_CALLBACK_V

//! @} FLEXT_D_CALLBACK

#endif
