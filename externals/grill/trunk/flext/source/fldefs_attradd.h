/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 1233 $
$LastChangedDate: 2008-01-17 11:11:19 -0500 (Thu, 17 Jan 2008) $
$LastChangedBy: thomas $
*/

/*! \file fldefs_attradd.h
    \brief This file contains all #defines for actual usage
    
*/

#ifndef __FLEXT_DEFS_ATTRADD_H
#define __FLEXT_DEFS_ATTRADD_H


/*! \defgroup FLEXT_D_CADDATTR Announce object attributes at class scope
    \ingroup FLEXT_D_ATTRIB
    \note These can only be used at class construction time
    @{ 
*/

//! Add handler for a gettable attribute
#define FLEXT_CADDATTR_GET(CL,NAME,GFUN) \
\
AddAttrib(CL,flext::MakeSymbol(NAME),(FLEXT_GET_PRE(GFUN)),NULL)

//! Add handler for a settable attribute
#define FLEXT_CADDATTR_SET(CL,NAME,SFUN) \
\
AddAttrib(CL,flext::MakeSymbol(NAME),NULL,(FLEXT_SET_PRE(SFUN)))

//! Add handlers for a both get- and settable attribute
#define FLEXT_CADDATTR_VAR(CL,NAME,GFUN,SFUN) \
\
AddAttrib(CL,flext::MakeSymbol(NAME),(FLEXT_GET_PRE(GFUN)),(FLEXT_SET_PRE(SFUN)))

//! Add handlers for a both get- and settable attribute
#define FLEXT_CADDATTR_VAR1(CL,NAME,FUN) \
\
AddAttrib(CL,flext::MakeSymbol(NAME),(FLEXT_GET_PRE(FUN)),(FLEXT_SET_PRE(FUN)))


//! Add handler for a gettable enum attribute
#define FLEXT_CADDATTR_GET_E(CL,NAME,GFUN) \
\
AddAttrib(CL,flext::MakeSymbol(NAME),(bool (*)(flext_base *,int &))(FLEXT_GET_PRE(GFUN)),NULL)

//! Add handler for a settable enum attribute
#define FLEXT_CADDATTR_SET_E(CL,NAME,SFUN) \
\
AddAttrib(CL,flext::MakeSymbol(NAME),NULL,(bool (*)(flext_base *,int &))(FLEXT_SET_PRE(SFUN)))

//! Add handlers for a both get- and settable enum attribute
#define FLEXT_CADDATTR_VAR_E(CL,NAME,GFUN,SFUN) \
\
AddAttrib(CL,flext::MakeSymbol(NAME),(bool (*)(flext_base *,int &))(FLEXT_GET_PRE(GFUN)),(bool (*)(flext_base *,int &))(FLEXT_SET_PRE(SFUN)))

//! Add handlers for a both get- and settable enum attribute
#define FLEXT_CADDATTR_VAR1_E(CL,NAME,FUN) \
\
AddAttrib(CL,flext::MakeSymbol(NAME),(bool (*)(flext_base *,int &))(FLEXT_GET_PRE(FUN)),(bool (*)(flext_base *,int &))(FLEXT_SET_PRE(FUN)))

//! @} FLEXT_D_CADDATTR


/*! \defgroup FLEXT_D_ADDATTR Announce object attributes 
    \ingroup FLEXT_D_ATTRIB
    \note These can only be used at object construction time
    \note (in constructor or in Init() function before call to parent's Init())
    @{ 
*/

//! Add handler for a gettable attribute
#define FLEXT_ADDATTR_GET(NAME,GFUN) \
\
AddAttrib(flext::MakeSymbol(NAME),(FLEXT_GET_PRE(GFUN)),NULL)

//! Add handler for a settable attribute
#define FLEXT_ADDATTR_SET(NAME,SFUN) \
\
AddAttrib(flext::MakeSymbol(NAME),NULL,(FLEXT_SET_PRE(SFUN)))

//! Add handlers for a both get- and settable attribute
#define FLEXT_ADDATTR_VAR(NAME,GFUN,SFUN) \
\
AddAttrib(flext::MakeSymbol(NAME),(FLEXT_GET_PRE(GFUN)),(FLEXT_SET_PRE(SFUN)))

//! Add handlers for a both get- and settable attribute
#define FLEXT_ADDATTR_VAR1(NAME,FUN) \
\
AddAttrib(flext::MakeSymbol(NAME),(FLEXT_GET_PRE(FUN)),(FLEXT_SET_PRE(FUN)))


//! Add handler for a gettable enum attribute
#define FLEXT_ADDATTR_GET_E(NAME,GFUN) \
\
AddAttrib(flext::MakeSymbol(NAME),(bool (*)(flext_base *,int &))(FLEXT_GET_PRE(GFUN)),NULL)

//! Add handler for a settable enum attribute
#define FLEXT_ADDATTR_SET_E(NAME,SFUN) \
\
AddAttrib(flext::MakeSymbol(NAME),NULL,(bool (*)(flext_base *,int &))(FLEXT_SET_PRE(SFUN)))

//! Add handlers for a both get- and settable enum attribute
#define FLEXT_ADDATTR_VAR_E(NAME,GFUN,SFUN) \
\
AddAttrib(flext::MakeSymbol(NAME),(bool (*)(flext_base *,int &))(FLEXT_GET_PRE(GFUN)),(bool (*)(flext_base *,int &))(FLEXT_SET_PRE(SFUN)))

//! Add handlers for a both get- and settable enum attribute
#define FLEXT_ADDATTR_VAR1_E(NAME,FUN) \
\
AddAttrib(flext::MakeSymbol(NAME),(bool (*)(flext_base *,int &))(FLEXT_GET_PRE(FUN)),(bool (*)(flext_base *,int &))(FLEXT_SET_PRE(FUN)))

//! @} FLEXT_D_ADDATTR


#endif
