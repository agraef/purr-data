/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file fldefs.h
    \brief This file includes all the #define header files 
*/

#ifndef __FLEXT_DEFS_H
#define __FLEXT_DEFS_H

/*! \defgroup FLEXT_DEFS Definitions for basic flext functionality
    @{ 
*/

/*! \brief Switch for compilation of derived virtual classes
    \remark These need dynamic type casts (and RTTI, naturally)
    \ingroup FLEXT_GLOBALS
*/
#ifdef FLEXT_VIRT
#define FLEXT_CAST dynamic_cast
#else
#define FLEXT_CAST static_cast
#endif

//! @}  FLEXT_DEFS

#include "fldefs_hdr.h"

#include "fldefs_setup.h"


// ====================================================================================

/*! \defgroup FLEXT_D_METHOD Declarations for flext methods
    @{ 
*/

#include "fldefs_methcb.h"
#include "fldefs_meththr.h"
#include "fldefs_methadd.h"
#include "fldefs_methbind.h"
#include "fldefs_methcall.h"

//! @} FLEXT_D_METHOD



#ifdef FLEXT_ATTRIBUTES 

/*! \defgroup FLEXT_D_ATTRIB Attribute definition
    \note These have to reside inside the class declaration
    @{ 
*/

#include "fldefs_attrcb.h"
#include "fldefs_attrvar.h"
#include "fldefs_attradd.h"

//! @} FLEXT_D_ATTRIB

#endif // FLEXT_ATTRIBUTES

#endif // __FLEXT_DEFS_H
