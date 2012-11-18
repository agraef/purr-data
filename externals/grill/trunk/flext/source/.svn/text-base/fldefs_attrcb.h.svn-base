/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file fldefs_attrcb.h
    \brief This file contains all #defines for actual usage
    
*/

#ifndef __FLEXT_DEFS_ATTRCB_H
#define __FLEXT_DEFS_ATTRCB_H



/*! \brief Declare a attribute set function
    \internal
*/
#define FLEXT_CALLSET_(FUN,TP) \
static bool FLEXT_SET_PRE(FUN)(flext_base *c,TP &arg) \
{ FLEXT_CAST<thisType *>(c)->FUN(arg); return true; }

/*! \brief Declare a attribute get function
    \internal
*/
#define FLEXT_CALLGET_(FUN,TP) \
static bool FLEXT_GET_PRE(FUN)(flext_base *c,TP &arg) \
{ FLEXT_CAST<thisType *>(c)->FUN(arg); return true; }



/*! \defgroup FLEXT_DA_CALLSET Definition of attribute set handlers
    \ingroup FLEXT_D_ATTRIB
    @{ 
*/

//! Declare a set function for a float attribute
#define FLEXT_CALLSET_F(SFUN) \
\
FLEXT_CALLSET_(SFUN,float)

//! Declare a set function for an integer attribute
#define FLEXT_CALLSET_I(SFUN) \
\
FLEXT_CALLSET_(SFUN,int)

//! Declare a set function for a boolean attribute
#define FLEXT_CALLSET_B(SFUN) \
\
FLEXT_CALLSET_(SFUN,bool)
/*
static bool FLEXT_SET_PRE(FUN)(flext_base *c,int &arg) \
{ bool b = arg != 0; FLEXT_CAST<thisType *>(c)->FUN(b); return true; }
*/

//! Declare a set function for an enum attribute
#define FLEXT_CALLSET_E(SFUN,TP) \
\
FLEXT_CALLSET_(SFUN,TP)

//! Declare a set function for a symbol attribute
#define FLEXT_CALLSET_S(FUN) \
static bool FLEXT_SET_PRE(FUN)(flext_base *c,const t_symbol *&arg) \
{ FLEXT_CAST<thisType *>(c)->FUN(arg); return true; }

//! Declare a set function for a variable list attribute
#define FLEXT_CALLSET_V(FUN) \
static bool FLEXT_SET_PRE(FUN)(flext_base *c,AtomList *&arg) \
{ FLEXT_CAST<thisType *>(c)->FUN(*arg); return true; }

//! @} FLEXT_DA_CALLSET

/*! \defgroup FLEXT_DA_CALLGET Definition of attribute get handlers
    \ingroup FLEXT_D_ATTRIB
    @{ 
*/

//! Declare a get function for a float attribute
#define FLEXT_CALLGET_F(GFUN) \
\
FLEXT_CALLGET_(GFUN,float)

//! Declare a get function for an integer attribute
#define FLEXT_CALLGET_I(GFUN) \
\
FLEXT_CALLGET_(GFUN,int)

//! Declare a get function for a boolean attribute
#define FLEXT_CALLGET_B(GFUN) \
\
FLEXT_CALLGET_(GFUN,bool)
/*
static bool FLEXT_GET_PRE(FUN)(flext_base *c,int &arg) \
{ bool b; FLEXT_CAST<thisType *>(c)->FUN(b); arg = b?1:0; return true; }
*/

//! Declare a get function for an enum attribute
#define FLEXT_CALLGET_E(GFUN,TP) \
\
FLEXT_CALLGET_(GFUN,TP)

//! Declare a get function for a symbol attribute
#define FLEXT_CALLGET_S(FUN) \
static bool FLEXT_GET_PRE(FUN)(flext_base *c,const t_symbol *&arg) \
{ FLEXT_CAST<thisType *>(c)->FUN(arg); return true; }

//! Declare a get function for a variable list attribute
#define FLEXT_CALLGET_V(FUN) \
static bool FLEXT_GET_PRE(FUN)(flext_base *c,AtomList *&arg) \
{ FLEXT_CAST<thisType *>(c)->FUN(*arg); return true; }

//! @} FLEXT_DA_CALLGET


/*! \defgroup FLEXT_DA_CALLVAR Definition of attribute transfer handlers (both get and set)
    \ingroup FLEXT_D_ATTRIB
    @{ 
*/

//! Declare both get and set functions for a float attribute
#define FLEXT_CALLVAR_F(GFUN,SFUN) \
\
FLEXT_CALLGET_F(GFUN) FLEXT_CALLSET_F(SFUN) 

//! Declare both get and set functions for an integer attribute
#define FLEXT_CALLVAR_I(GFUN,SFUN) \
\
FLEXT_CALLGET_I(GFUN) FLEXT_CALLSET_I(SFUN) 

//! Declare both get and set functions for a symbol attribute
#define FLEXT_CALLVAR_S(GFUN,SFUN) \
\
FLEXT_CALLGET_S(GFUN) FLEXT_CALLSET_S(SFUN) 

//! Declare both get and set functions for a boolean attribute
#define FLEXT_CALLVAR_B(GFUN,SFUN) \
\
FLEXT_CALLGET_B(GFUN) FLEXT_CALLSET_B(SFUN) 

//! Declare both get and set functions for an enum attribute
#define FLEXT_CALLVAR_E(GFUN,SFUN,TP) \
\
FLEXT_CALLGET_E(GFUN,TP) FLEXT_CALLSET_E(SFUN,TP) 

//! Declare both get and set functions for a variable list attribute
#define FLEXT_CALLVAR_V(GFUN,SFUN) \
\
FLEXT_CALLGET_V(GFUN) FLEXT_CALLSET_V(SFUN) 

//! @} FLEXT_DA_CALLVAR


#endif
