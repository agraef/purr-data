/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 1233 $
$LastChangedDate: 2008-01-17 11:11:19 -0500 (Thu, 17 Jan 2008) $
$LastChangedBy: thomas $
*/

/*! \file fldefs_methbind.h
    \brief This file contains all #defines for actual usage
    
*/

#ifndef __FLEXT_DEFS_METHBIND_H
#define __FLEXT_DEFS_METHBIND_H


/*!	\defgroup FLEXT_D_BINDMETHOD Bind flext methods to symbols
	@{ 
*/

/*! \brief Bind a handler for a method with an anything argument to a symbol 
*/
#define FLEXT_BINDMETHOD(SYM,M_FUN,DATA) \
\
BindMethod(SYM,FLEXT_CALL_PRE(M_FUN),DATA)	

/*! \brief Unbind any handler for a method from a symbol 
    \note Memory associated to the DATA parameter of FLEXT_BINDMETHOD will *not* be freed here.
*/
#define FLEXT_UNBINDMETHOD(SYM) \
\
UnbindMethod(SYM)

/*! \brief Unbind any handler for a method from a symbol and return user data pointer by DATA
    \note Memory associated to the DATA parameter of FLEXT_BINDMETHOD will *not* be freed here.
*/
#define FLEXT_UNBINDMETHOD_X(SYM,DATA) \
\
UnbindMethod(SYM,&DATA)


//! @} FLEXT_D_BINDMETHOD


#endif
