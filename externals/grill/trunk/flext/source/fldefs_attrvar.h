/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 1233 $
$LastChangedDate: 2008-01-17 11:11:19 -0500 (Thu, 17 Jan 2008) $
$LastChangedBy: thomas $
*/

/*! \file fldefs_attrvar.h
    \brief This file contains all #defines for actual usage
    
*/

#ifndef __FLEXT_DEFS_ATTRVAR_H
#define __FLEXT_DEFS_ATTRVAR_H


/*! \brief Declare an implicit attribute set function
    \internal
*/
#define FLEXT_ATTRSET_(VAR,TP) \
static bool FLEXT_SET_PRE(VAR)(flext_base *c,TP &arg) \
{ FLEXT_CAST<thisType *>(c)->VAR = arg; return true; }

/*! \brief Declare an implicit attribute get function
    \internal
*/
#define FLEXT_ATTRGET_(VAR,TP) \
static bool FLEXT_GET_PRE(VAR)(flext_base *c,TP &arg) \
{ arg = (TP)FLEXT_CAST<thisType *>(c)->VAR; return true; }



/*! \defgroup FLEXT_DA_ATTRSET Definition of implicit attribute set handlers
    \ingroup FLEXT_D_ATTRIB
    @{ 
*/

//! Declare an implicit set function for a float attribute
#define FLEXT_ATTRSET_F(VAR) \
\
FLEXT_ATTRSET_(VAR,float)

//! Declare an implicit set function for an integer attribute
#define FLEXT_ATTRSET_I(VAR) \
\
FLEXT_ATTRSET_(VAR,int)

//! Declare an implicit set function for a symbol attribute
#define FLEXT_ATTRSET_S(VAR) \
\
FLEXT_ATTRSET_(VAR,const t_symbol *)

//! Declare an implicit set function for a boolean attribute
#define FLEXT_ATTRSET_B(VAR) \
\
FLEXT_ATTRSET_(VAR,bool)
/*
static bool FLEXT_SET_PRE(VAR)(flext_base *c,int &arg) \
{ FLEXT_CAST<thisType *>(c)->VAR = arg != 0; return true; }
*/

//! Declare an implicit set function for an enum attribute
#define FLEXT_ATTRSET_E(VAR,TP) \
\
FLEXT_ATTRSET_(VAR,TP)

//! Declare an implicit set function for a variable list attribute
#define FLEXT_ATTRSET_V(VAR) \
static bool FLEXT_SET_PRE(VAR)(flext_base *c,AtomList *&arg) \
{ FLEXT_CAST<thisType *>(c)->VAR = *arg; return true; }

//! @} FLEXT_DA_ATTRSET

/*! \defgroup FLEXT_DA_ATTRGET Definition of implicit attribute get handlers
    \ingroup FLEXT_D_ATTRIB
    @{ 
*/

//! Declare an implicit get function for a float attribute
#define FLEXT_ATTRGET_F(VAR) \
\
FLEXT_ATTRGET_(VAR,float)

//! Declare an implicit get function for an integer attribute
#define FLEXT_ATTRGET_I(VAR) \
\
FLEXT_ATTRGET_(VAR,int)

//! Declare an implicit get function for a symbol attribute
#define FLEXT_ATTRGET_S(VAR) \
\
FLEXT_ATTRGET_(VAR,const t_symbol *)

//! Declare an implicit get function for a boolean attribute
#define FLEXT_ATTRGET_B(VAR) \
\
FLEXT_ATTRGET_(VAR,bool)
/*
static bool FLEXT_GET_PRE(VAR)(flext_base *c,int &arg) \
{ arg = FLEXT_CAST<thisType *>(c)->VAR?1:0; return true; }
*/

//! Declare an implicit get function for an enum attribute
#define FLEXT_ATTRGET_E(VAR,TP) \
\
FLEXT_ATTRGET_(VAR,TP)

//! Declare an implicit get function for a variable list attribute
#define FLEXT_ATTRGET_V(VAR) \
static bool FLEXT_GET_PRE(VAR)(flext_base *c,AtomList *&arg) \
{ *arg = FLEXT_CAST<thisType *>(c)->VAR; return true; }

//! @} FLEXT_DA_ATTRGET


/*! \defgroup FLEXT_DA_ATTRVAR Definition of implicit attribute transfer handlers (both get and set)
    \ingroup FLEXT_D_ATTRIB
    @{ 
*/

//! Declare both implicit get and set functions for a float attribute
#define FLEXT_ATTRVAR_F(VAR) \
\
FLEXT_ATTRGET_F(VAR) FLEXT_ATTRSET_F(VAR) 

//! Declare both implicit get and set functions for an integer attribute
#define FLEXT_ATTRVAR_I(VAR) \
\
FLEXT_ATTRGET_I(VAR) FLEXT_ATTRSET_I(VAR) 

//! Declare both implicit get and set functions for a symbol attribute
#define FLEXT_ATTRVAR_S(VAR) \
\
FLEXT_ATTRGET_S(VAR) FLEXT_ATTRSET_S(VAR) 

//! Declare both implicit get and set functions for a boolean attribute
#define FLEXT_ATTRVAR_B(VAR) \
\
FLEXT_ATTRGET_B(VAR) FLEXT_ATTRSET_B(VAR) 

//! Declare both implicit get and set functions for an enum attribute
#define FLEXT_ATTRVAR_E(VAR,TP) \
\
FLEXT_ATTRGET_(VAR,TP) FLEXT_ATTRSET_(VAR,TP) 

//! Declare both implicit get and set functions for a variable list attribute
#define FLEXT_ATTRVAR_V(VAR) \
\
FLEXT_ATTRGET_V(VAR) FLEXT_ATTRSET_V(VAR) 


//! @} FLEXT_DA_ATTRVAR


#endif
