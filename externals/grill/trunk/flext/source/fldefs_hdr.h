/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 1233 $
$LastChangedDate: 2008-01-17 11:11:19 -0500 (Thu, 17 Jan 2008) $
$LastChangedBy: thomas $
*/

/*! \file fldefs_hdr.h
    \brief This file contains all #defines for actual usage
    
*/

#ifndef __FLEXT_DEFS_HEADER_H
#define __FLEXT_DEFS_HEADER_H


/*!	\defgroup FLEXT_D_HEADER Flext class header
	\note One (and only one!) of these definitions is compulsory for the class declaration. 
	\note It has to be placed somewhere in the class definition (not necessarily in a public section).

	@{ 
*/

/*! \brief Plain flext class header
	\param NEW_CLASS name of the current C++ class
	\param PARENT_CLASS name of the base C++ class (e.g. flext_base or flext_dsp)
*/
#define FLEXT_HEADER(NEW_CLASS,PARENT_CLASS) \
\
FLEXT_REALHDR(NEW_CLASS, PARENT_CLASS)    	    	

#define FLEXT_HEADER_T(NEW_CLASS,PARENT_CLASS) \
\
FLEXT_REALHDR_T(NEW_CLASS, PARENT_CLASS)    	    	

/*! \brief Flext class header with setup function
	\param NEW_CLASS name of the current C++ class
	\param PARENT_CLASS name of the base C++ class (e.g. flext_base or flext_dsp)
	\param SETUPFUN setup function, of type "void (*setupfn)(t_class *)"

	The setup function is called after class creation. It corresponds to the
	original PD "[object]_setup" function, apart from the
	fact that all necessary class initializations have already been taken care of by flext. 
	The setup function can e.g. be used for a message to the console upon first creation of an object.
*/
#define FLEXT_HEADER_S(NEW_CLASS, PARENT_CLASS, SETUPFUN)\
\
FLEXT_REALHDR_S(NEW_CLASS, PARENT_CLASS, SETUPFUN)    	    	

#define FLEXT_HEADER_TS(NEW_CLASS, PARENT_CLASS, SETUPFUN)\
\
FLEXT_REALHDR_TS(NEW_CLASS, PARENT_CLASS, SETUPFUN)    	    	


//! @} FLEXT_D_HEADER


#endif
