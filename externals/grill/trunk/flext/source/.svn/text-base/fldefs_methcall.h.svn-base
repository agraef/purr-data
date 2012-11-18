/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file fldefs_methbind.h
    \brief This file contains all #defines for actual usage
    
*/

#ifndef __FLEXT_DEFS_METHCALL_H
#define __FLEXT_DEFS_METHCALL_H


/*! \defgroup FLEXT_D_CALLMETHOD Call flext methods manually
    \ingroup FLEXT_D_METHOD
    @{ 
*/

//! Call a (already defined) method with no arguments
#define FLEXT_CALLMETHOD(M_FUN) \
\
FLEXT_CALL_PRE(M_FUN)(this)

//! Call a (already defined) method with variable list arguments
#define FLEXT_CALLMETHOD_V(M_FUN,ARGC,ARGV) \
\
FLEXT_CALL_PRE(M_FUN)(this,ARGC,(t_atom *)(ARGV))

//! Call a (already defined) method with anything arguments
#define FLEXT_CALLMETHOD_A(M_FUN,HDR,ARGC,ARGV) \
\
FLEXT_CALL_PRE(M_FUN)(this,(t_symbol *)(HDR),ARGC,(t_atom *)(ARGV))

//! Call a (already defined) method with a data package (void *)
#define FLEXT_CALLMETHOD_X(M_FUN,DATA) \
\
FLEXT_CALL_PRE(M_FUN)(this,DATA)

//! Call a (already defined) method with 1 enum type argument
#define FLEXT_CALLMETHOD_E(M_FUN,ARG) \
\
FLEXT_CALL_PRE(M_FUN)(this,ARG)

//! Call a (already defined) method with 1 argument
#define FLEXT_CALLMETHOD_1(M_FUN,ARG) \
\
FLEXT_CALL_PRE(M_FUN)(this,ARG)

//! Call a (already defined) method with 2 arguments
#define FLEXT_CALLMETHOD_2(M_FUN,ARG1,ARG2) \
\
FLEXT_CALL_PRE(M_FUN)(this,ARG1,ARG2)

//! Call a (already defined) method with 3 arguments
#define FLEXT_CALLMETHOD_3(M_FUN,ARG1,ARG2,ARG3) \
\
FLEXT_CALL_PRE(M_FUN)(this,ARG1,ARG2,ARG3)

//! Call a (already defined) method with 4 arguments
#define FLEXT_CALLMETHOD_4(M_FUN,ARG1,ARG2,ARG3,ARG4) \
\
FLEXT_CALL_PRE(M_FUN)(this,ARG1,ARG2,ARG3,ARG4)

//! Call a (already defined) method with 5 arguments
#define FLEXT_CALLMETHOD_5(M_FUN,ARG1,ARG2,ARG3,ARG4,ARG5) \
\
FLEXT_CALL_PRE(M_FUN)(this,ARG1,ARG2,ARG3,ARG4,ARG5)

//! @} FLEXT_D_CALLMETHOD


#endif
