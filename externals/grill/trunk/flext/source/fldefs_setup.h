/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file fldefs_setup.h
    \brief This file contains all #defines for actual usage
    
*/

#ifndef __FLEXT_DEFS_SETUP_H
#define __FLEXT_DEFS_SETUP_H

// ====================================================================================

/*! \defgroup FLEXT_D_INSTANCE Class instantiation
    \note For stand-alone externals (not part of a library) the name of your class 
    \note is of importance! It must be the same as the external (excluded an eventual ~ (tilde))

    There are additional parameters that can be included in the NAME field of FLEXT_NEW etc.:

    - There may be additional names (aliases) appened, separated by spaces
    - There may be a help path prepended, separated by a colon
    - This help path doesn't work for Max/MSP. There you'll have to use a object mapping file (Max/MSP version >= 4.2)

    @{
*/


/*! \defgroup FLEXT_D_NEW Stand-alone class instantiation
    Makes an actual instance of a stand-alone class.
*/

/*! \defgroup FLEXT_D_NEW_DSP Dsp class instantiation
    Makes an actual instance of a dsp (aka "tilde") class (with signal processing).
*/

/*! \defgroup FLEXT_D_LIB Library class instantiation
    Makes an actual instance of a class which is part of an object library (and not stand-alone).
*/

/*! \defgroup FLEXT_D_LIB_DSP Dsp library class instantiation
    Makes an actual instance of a dsp (aka "tilde") class with signal processing
    which is part of an object library (and not stand-alone).
*/

// NO ARGUMENTS
// ----------------------------------------

/*! \brief Implementation of a flext class with no arguments
    \ingroup FLEXT_D_NEW
    \param NAME the object's actual name(s) as a string (like "*", "trigger", "noise~", etc.)
    \param NEW_CLASS the object's C++ class name 
*/
#define FLEXT_NEW(NAME,NEW_CLASS)       \
\
REAL_NEW(NAME,NEW_CLASS,0,0,0)

/*! \brief Implementation of a flext dsp class with no arguments
    \ingroup FLEXT_D_NEW_DSP
*/
#define FLEXT_NEW_DSP(NAME,NEW_CLASS)   \
\
REAL_NEW(NAME,NEW_CLASS,1,0,0)

/*! \brief Implementation of a flext dsp class with no arguments and no dsp inlet
    \ingroup FLEXT_D_NEW_DSP
*/
#define FLEXT_NEW_DSP0(NAME,NEW_CLASS)   \
\
REAL_NEW(NAME,NEW_CLASS,1,1,0)

/*! \brief Implementation of a flext class (part of a library) with no arguments
    \ingroup FLEXT_D_LIB
*/
#define FLEXT_LIB(NAME,NEW_CLASS) \
\
REAL_NEW(NAME,NEW_CLASS,0,0,1) 

/*! \brief Implementation of a flext dsp class (part of a library) with no arguments
    \ingroup FLEXT_D_LIB_DSP
*/
#define FLEXT_LIB_DSP(NAME,NEW_CLASS)   \
\
REAL_NEW(NAME,NEW_CLASS,1,0,1) 

/*! \brief Implementation of a flext dsp class (part of a library) with no arguments and no dsp inlet
    \ingroup FLEXT_D_LIB_DSP
*/
#define FLEXT_LIB_DSP0(NAME,NEW_CLASS)   \
\
REAL_NEW(NAME,NEW_CLASS,1,1,1) 


// VARIABLE ARGUMENT LIST
// ----------------------------------------

/*! \brief Implementation of a flext class with a variable argument list
    \ingroup FLEXT_D_NEW
*/
#define FLEXT_NEW_V(NAME,NEW_CLASS)         \
\
REAL_NEW_V(NAME,NEW_CLASS,0,0,0)

/*! \brief Implementation of a flext dsp class with a variable argument list
    \ingroup FLEXT_D_NEW_DSP
*/
#define FLEXT_NEW_DSP_V(NAME,NEW_CLASS) \
\
REAL_NEW_V(NAME,NEW_CLASS,1,0,0)

/*! \brief Implementation of a flext dsp class with a variable argument list and no dsp inlet
    \ingroup FLEXT_D_NEW_DSP
*/
#define FLEXT_NEW_DSP0_V(NAME,NEW_CLASS) \
\
REAL_NEW_V(NAME,NEW_CLASS,1,1,0)

/*! \brief Implementation of a flext class (part of a library) with a variable argument list
    \ingroup FLEXT_D_LIB
*/
#define FLEXT_LIB_V(NAME,NEW_CLASS)         \
\
REAL_NEW_V(NAME,NEW_CLASS, 0,0,1) 

/*! \brief Implementation of a flext dsp class (part of a library) with a variable argument list
    \ingroup FLEXT_D_LIB_DSP
*/
#define FLEXT_LIB_DSP_V(NAME,NEW_CLASS) \
\
REAL_NEW_V(NAME,NEW_CLASS, 1,0,1) 

/*! \brief Implementation of a flext dsp class (part of a library) with a variable argument list and no dsp inlet
    \ingroup FLEXT_D_LIB_DSP
*/
#define FLEXT_LIB_DSP0_V(NAME,NEW_CLASS) \
\
REAL_NEW_V(NAME,NEW_CLASS, 1,1,1) 


// ONE ARGUMENT
// ----------------------------------------

/*! \brief Implementation of a flext class with one argument
    \ingroup FLEXT_D_NEW
*/
#define FLEXT_NEW_1(NAME,NEW_CLASS, TYPE)       \
\
REAL_NEW_1(NAME,NEW_CLASS, 0,0,0, TYPE)

/*! \brief Implementation of a flext dsp class with one argument
    \ingroup FLEXT_D_NEW_DSP
*/
#define FLEXT_NEW_DSP_1(NAME,NEW_CLASS, TYPE)   \
\
REAL_NEW_1(NAME,NEW_CLASS, 1,0,0, TYPE)

/*! \brief Implementation of a flext dsp class with one argument and no dsp inlet
    \ingroup FLEXT_D_NEW_DSP
*/
#define FLEXT_NEW_DSP0_1(NAME,NEW_CLASS, TYPE)   \
\
REAL_NEW_1(NAME,NEW_CLASS, 1,1,0, TYPE)

/*! \brief Implementation of a flext class (part of a library) with one argument
    \ingroup FLEXT_D_LIB
*/
#define FLEXT_LIB_1(NAME,NEW_CLASS, TYPE) \
\
REAL_NEW_1(NAME,NEW_CLASS, 0,0,1, TYPE)

/*! \brief Implementation of a flext dsp class (part of a library) with one argument
    \ingroup FLEXT_D_LIB_DSP
*/
#define FLEXT_LIB_DSP_1(NAME,NEW_CLASS, TYPE)   \
\
REAL_NEW_1(NAME,NEW_CLASS, 1,0,1, TYPE)

/*! \brief Implementation of a flext dsp class (part of a library) with one argument and no dsp inlet
    \ingroup FLEXT_D_LIB_DSP
*/
#define FLEXT_LIB_DSP0_1(NAME,NEW_CLASS, TYPE)   \
\
REAL_NEW_1(NAME,NEW_CLASS, 1,1,1, TYPE)


// TWO ARGUMENTS
// ----------------------------------------

/*! \brief Implementation of a flext class with 2 arguments
    \ingroup FLEXT_D_NEW
*/
#define FLEXT_NEW_2(NAME,NEW_CLASS, TYPE1, TYPE2)           \
\
REAL_NEW_2(NAME,NEW_CLASS, 0,0,0, TYPE1, TYPE2)

/*! \brief Implementation of a flext dsp class with 2 arguments
    \ingroup FLEXT_D_NEW_DSP
*/
#define FLEXT_NEW_DSP_2(NAME,NEW_CLASS, TYPE1, TYPE2)   \
\
REAL_NEW_2(NAME,NEW_CLASS, 1,0,0, TYPE1, TYPE2)

/*! \brief Implementation of a flext dsp class with 2 arguments and no dsp inlet
    \ingroup FLEXT_D_NEW_DSP
*/
#define FLEXT_NEW_DSP0_2(NAME,NEW_CLASS, TYPE1, TYPE2)   \
\
REAL_NEW_2(NAME,NEW_CLASS, 1,1,0, TYPE1, TYPE2)

/*! \brief Implementation of a flext class (part of a library) with 2 arguments
    \ingroup FLEXT_D_LIB
*/
#define FLEXT_LIB_2(NAME,NEW_CLASS, TYPE1, TYPE2)       \
\
REAL_NEW_2(NAME,NEW_CLASS, 0,0,1, TYPE1, TYPE2)

/*! \brief Implementation of a flext dsp class (part of a library) with 2 arguments
    \ingroup FLEXT_D_LIB_DSP
*/
#define FLEXT_LIB_DSP_2(NAME,NEW_CLASS, TYPE1, TYPE2)   \
\
REAL_NEW_2(NAME,NEW_CLASS, 1,0,1, TYPE1, TYPE2)

/*! \brief Implementation of a flext dsp class (part of a library) with 2 arguments and no dsp inlet
    \ingroup FLEXT_D_LIB_DSP
*/
#define FLEXT_LIB_DSP0_2(NAME,NEW_CLASS, TYPE1, TYPE2)   \
\
REAL_NEW_2(NAME,NEW_CLASS, 1,1,1, TYPE1, TYPE2)


// THREE ARGUMENTS
// ----------------------------------------

/*! \brief Implementation of a flext class with 3 arguments
    \ingroup FLEXT_D_NEW
*/
#define FLEXT_NEW_3(NAME,NEW_CLASS, TYPE1, TYPE2, TYPE3) \
\
REAL_NEW_3(NAME,NEW_CLASS, 0,0,0, TYPE1, TYPE2, TYPE3)

/*! \brief Implementation of a flext dsp class with 3 arguments
    \ingroup FLEXT_D_NEW_DSP
*/
#define FLEXT_NEW_DSP_3(NAME,NEW_CLASS, TYPE1, TYPE2, TYPE3)    \
\
REAL_NEW_3(NAME,NEW_CLASS, 1,0,0, TYPE1, TYPE2, TYPE3)

/*! \brief Implementation of a flext dsp class with 3 arguments and no dsp inlet
    \ingroup FLEXT_D_NEW_DSP
*/
#define FLEXT_NEW_DSP0_3(NAME,NEW_CLASS, TYPE1, TYPE2, TYPE3)    \
\
REAL_NEW_3(NAME,NEW_CLASS, 1,1,0, TYPE1, TYPE2, TYPE3)

/*! \brief Implementation of a flext class (part of a library) with 3 arguments
    \ingroup FLEXT_D_LIB
*/
#define FLEXT_LIB_3(NAME,NEW_CLASS, TYPE1, TYPE2, TYPE3)        \
\
REAL_NEW_3(NAME,NEW_CLASS, 0,0,1, TYPE1, TYPE2, TYPE3)

/*! \brief Implementation of a flext dsp class (part of a library) with 3 arguments
    \ingroup FLEXT_D_LIB_DSP
*/
#define FLEXT_LIB_DSP_3(NAME,NEW_CLASS, TYPE1, TYPE2, TYPE3)    \
\
REAL_NEW_3(NAME,NEW_CLASS, 1,0,1, TYPE1, TYPE2, TYPE3)

/*! \brief Implementation of a flext dsp class (part of a library) with 3 arguments and no dsp inlet
    \ingroup FLEXT_D_LIB_DSP
*/
#define FLEXT_LIB_DSP0_3(NAME,NEW_CLASS, TYPE1, TYPE2, TYPE3)    \
\
REAL_NEW_3(NAME,NEW_CLASS, 1,1,1, TYPE1, TYPE2, TYPE3)


// deprecated stuff

/*! \defgroup FLEXT_D_DEPRECATED Deprecated definitions 
    \deprecated
    @{ 
*/

#define FLEXT_NEW_G FLEXT_NEW_V

#define FLEXT_NEW_TILDE FLEXT_NEW_DSP
#define FLEXT_NEW_TILDE_G FLEXT_NEW_DSP_V
#define FLEXT_NEW_TILDE_1 FLEXT_NEW_DSP_1
#define FLEXT_NEW_TILDE_2 FLEXT_NEW_DSP_2
#define FLEXT_NEW_TILDE_3 FLEXT_NEW_DSP_3

#define FLEXT_LIB_G FLEXT_LIB_V

#define FLEXT_LIB_TILDE FLEXT_LIB_DSP
#define FLEXT_LIB_TILDE_G FLEXT_LIB_DSP_V
#define FLEXT_LIB_TILDE_1 FLEXT_LIB_DSP_1
#define FLEXT_LIB_TILDE_2 FLEXT_LIB_DSP_2
#define FLEXT_LIB_TILDE_3 FLEXT_LIB_DSP_3

#define FLEXT_TILDE_SETUP FLEXT_DSP_SETUP

//! @} FLEXT_D_DEPRECATED


/*! \defgroup FLEXT_D_LIBRARY Definitions for library objects
    @{ 
*/

/*! \brief Specify that to declare the library itself.
    \note If you have a library this is compulsory (to register all the objects of the library)
*/
#define FLEXT_LIB_SETUP(NAME,SETUPFUN) REAL_LIB_SETUP(NAME,SETUPFUN)

/*! \brief Register an object in the library.
    \note This is used in the library setup function
*/
#define FLEXT_SETUP(cl) REAL_SETUP(cl,0)

/*! \brief Register a DSP object in the library.
    \note This is used in the library setup function
*/
#define FLEXT_DSP_SETUP(cl) REAL_SETUP(cl,1)

//! @} FLEXT_D_LIBRARY 


//! @} FLEXT_D_INSTANCE


#endif
