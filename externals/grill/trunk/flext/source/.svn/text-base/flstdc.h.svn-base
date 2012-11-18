/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file flstdc.h
	\brief Definitions to unite Max/MSP and PD notions
    
	This file contains a few definitions to unite a few of the notions that 
	once drifted apart in Max and PD. It is not elegant but helps.
*/

#ifndef __FLEXT_STDC_H
#define __FLEXT_STDC_H

#if defined(_MSC_VER) && (_MSC_VER < 0x1300)
/* 
    include math.h here - when included with PD or Max/MSP headers,  
    C linkage would be used which disturbs MSVC6
*/
#include <cmath>
#endif

#ifdef _MSC_VER
#include <crtdbg.h>
#else
#include <cassert>
#endif

// PD stuff

#if FLEXT_SYS == FLEXT_SYS_PD

/* PD definitions start here */

#ifdef _MSC_VER
	#pragma warning (push)
	#pragma warning (disable:4091 4005)
#endif

#if FLEXT_OS == FLEXT_OS_WIN && !defined(NT)
#define NT
#endif

extern "C" {	    	    
	// Include the relevant PD header files
	#ifdef FLEXT_DEBUG
        /*  PD header file structure has changed with version 0.37
            from then on m_imp.h needs m_pd.h to be included before
            on the other hand versions < 0.37 don't like that....
            (they want m_imp.h solely as m_pd.h is included therein)
            So better use the m_pd.h here also for the debug version.
            Change that if really needed for debugging PD internals...
        */

		#ifndef PD_VERSION
			// include only if not already included
			#include <m_pd.h>
		#endif
//		#include <m_imp.h>  // for easier debugging
    #else
		#ifndef PD_VERSION
			// include only if not already included
			#include <m_pd.h>
		#endif
	#endif
}

#ifdef _MSC_VER
	#pragma warning (pop)
#endif

#include "flpushns.h"

#ifdef cabs
#undef cabs // this is defined in m_pd.h (clashes with math.h in MacOSX)
#endif

typedef t_object t_sigobj;
typedef t_gpointer *t_ptrtype;

typedef t_float t_flint;
typedef t_symbol *t_symtype;
typedef t_class **t_thing;

typedef t_clock t_qelem;

#define A_NOTHING A_NULL
#define A_FLINT A_FLOAT
#define A_DEFFLINT A_DEFFLOAT
#define A_DEFSYMBOL A_DEFSYM

#include "flpopns.h"


#elif FLEXT_SYS == FLEXT_SYS_MAX

/* -------------- Max/MSP ------------------- */

// 2-byte alignment for Max/MSP structures
#ifdef _MSC_VER
#pragma pack(push,flext_maxsdk)
#pragma pack(2)
#endif

// Include the relevant Max/MSP header files

#if FLEXT_OS == FLEXT_OS_MAC
	#if FLEXT_OSAPI == FLEXT_OSAPI_MAC_MACH
		// MachO version - must insert prefix header
         #include <Carbon/Carbon.h>
	#else
		// CFM version
		#ifndef __MRC__
			#define powerc
		#endif
		#define __MOTO__ 0

		#include <MacTypes.h>
	#endif
#elif FLEXT_OS == FLEXT_OS_WIN
	#define WIN_VERSION 1
	#define WIN_EXT_VERSION 1
#endif

// necessary for the old OS9 SDK
extern "C" {

#include "ext.h"
#include "ext_user.h"
#if FLEXT_OS != FLEXT_OS_MAC || defined(MAC_VERSION)
// doesn't exist for OS9
#include "ext_critical.h"
#include "buffer.h"
#else
// for OS9 include "inofficial" header file
#include "flmspbuffer.h"
#endif
#include "z_dsp.h"
#include "ext_obex.h"

// check for Max5 SDK
#include "commonsyms.h"
#if C74_MAX_SDK_VERSION >= 0x0500 || COMMON_SYMBOLS_VERSION >= 500 
    #define _FLEXT_MAX5SDK
#endif

} // extern "C"

#include "flpushns.h"

#undef WIN_VERSION

typedef t_pxobject t_sigobj;  // that's the all-in-one object type of Max/MSP (not very memory-efficent, i guess)
typedef t_patcher t_canvas;

typedef t_int t_flint;
typedef t_symbol *t_symtype;
typedef t_object *t_thing;

#ifndef _FLEXT_MAX5SDK
    // for the following to work you should have the latest SDK
    #if FLEXT_OS == FLEXT_OS_MAC //&& !defined(MAC_VERSION)
    typedef struct qelem t_qelem;
    #else
    typedef void *t_qelem;
    #endif
#endif

typedef method t_method;
typedef method t_newmethod;
typedef int t_atomtype;

#ifndef _FLEXT_MAX5SDK
typedef struct clock t_clock;  // this is defined in the Max5 SDK
#endif

typedef void t_binbuf;

#undef clock_free
#define clock_free(tick) freeobject((object *)tick)

#define A_NULL A_NOTHING
#define A_DEFFLINT A_DEFLONG

#ifndef A_INT
#define A_INT A_LONG
#endif

#ifndef A_DEFINT
#define A_DEFINT A_DEFLONG
#endif

#ifndef A_SYMBOL
#define A_SYMBOL A_SYM
#endif

#ifndef A_DEFSYMBOL
#define A_DEFSYMBOL A_DEFSYM
#endif

#if FLEXT_OS == FLEXT_OS_MAC && !defined(MAC_VERSION)
// simulate non-existing functions for OS9
#define critical_enter(N)
#define critical_exit(N)
#endif

#ifdef _MSC_VER
#pragma pack(pop,flext_maxsdk)
#endif

#include "flpopns.h"

#else
#error Platform not supported
#endif // FLEXT_SYS


// general definitions

#include "flpushns.h"

typedef t_symbol *t_symptr;

// -------------------------

#ifdef FLEXT_LOGGING
/* If FLEXT_LOGGING is defined implement logging */

#ifdef _MSC_VER
#define FLEXT_LOG(s) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",s)
#define FLEXT_LOG1(s,v1) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",s,v1)
#define FLEXT_LOG2(s,v1,v2) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",s,v1,v2)
#define FLEXT_LOG3(s,v1,v2,v3) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",s,v1,v2,v3)
#define FLEXT_LOG4(s,v1,v2,v3,v4) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",s,v1,v2,v3,v4)
#define FLEXT_LOG5(s,v1,v2,v3,v4,v5) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",s,v1,v2,v3,v4,v5)
#define FLEXT_LOG6(s,v1,v2,v3,v4,v5,v6) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",s,v1,v2,v3,v4,v5,v6)
#define FLEXT_LOG7(s,v1,v2,v3,v4,v5,v6,v7) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",s,v1,v2,v3,v4,v5,v6,v7)
#define FLEXT_LOG8(s,v1,v2,v3,v4,v5,v6,v7,v8) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",s,v1,v2,v3,v4,v5,v6,v7,v8)
#define FLEXT_LOG9(s,v1,v2,v3,v4,v5,v6,v7,v8,v9) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",s,v1,v2,v3,v4,v5,v6,v7,v8,v9)
#else
#define FLEXT_LOG(s) post(s)
#define FLEXT_LOG1(s,v1) post(s,v1)
#define FLEXT_LOG2(s,v1,v2) post(s,v1,v2)
#define FLEXT_LOG3(s,v1,v2,v3) post(s,v1,v2,v3)
#define FLEXT_LOG4(s,v1,v2,v3,v4) post(s,v1,v2,v3,v4)
#define FLEXT_LOG5(s,v1,v2,v3,v4,v5) post(s,v1,v2,v3,v4,v5)
#define FLEXT_LOG6(s,v1,v2,v3,v4,v5,v6) post(s,v1,v2,v3,v4,v5,v6)
#define FLEXT_LOG7(s,v1,v2,v3,v4,v5,v6,v7) post(s,v1,v2,v3,v4,v5,v6,v7)
#define FLEXT_LOG8(s,v1,v2,v3,v4,v5,v6,v7,v8) post(s,v1,v2,v3,v4,v5,v6,v7,v8)
#define FLEXT_LOG9(s,v1,v2,v3,v4,v5,v6,v7,v8,v9) post(s,v1,v2,v3,v4,v5,v6,v7,v8,v9)
#endif

#else

/* If FLEXT_LOGGING is not defined avoid logging */
#define FLEXT_LOG(s) ((void)0)
#define FLEXT_LOG1(s,v1) ((void)0)
#define FLEXT_LOG2(s,v1,v2) ((void)0)
#define FLEXT_LOG3(s,v1,v2,v3) ((void)0)
#define FLEXT_LOG4(s,v1,v2,v3,v4) ((void)0)
#define FLEXT_LOG5(s,v1,v2,v3,v4,v5) ((void)0)
#define FLEXT_LOG6(s,v1,v2,v3,v4,v5,v6) ((void)0)
#define FLEXT_LOG7(s,v1,v2,v3,v4,v5,v6,v7) ((void)0)
#define FLEXT_LOG8(s,v1,v2,v3,v4,v5,v6,v7,v8) ((void)0)
#define FLEXT_LOG9(s,v1,v2,v3,v4,v5,v6,v7,v8,v9) ((void)0)

#endif

#ifdef FLEXT_DEBUG
#ifdef _MSC_VER
#define FLEXT_ASSERT(b) do { if(!(b)) _CrtDbgReport(_CRT_ASSERT,__FILE__,__LINE__,"flext",#b); } while(false)
#define FLEXT_WARN(str) _CrtDbgReport(_CRT_WARN,__FILE__,__LINE__,"flext",NULL)
#define FLEXT_ERROR(str) _CrtDbgReport(_CRT_ERROR,__FILE__,__LINE__,"flext",NULL)
#else
#define FLEXT_ASSERT(b) assert(b)
//#define FLEXT_ASSERT(b) do { if(!(b)) error("Assertion failed: " #b " - in " __FILE__ " line %i",(int)__LINE__); } while(false)
#define FLEXT_WARN(str) error("Warning: in " __FILE__ " line %i",(int)__LINE__)
#define FLEXT_ERROR(str) error("Error: in " __FILE__ " line %i",(int)__LINE__)
#endif
#else
#define FLEXT_ASSERT(b) (1)
#define FLEXT_WARN(str) (1)
#define FLEXT_ERROR(str) error("Error: in " __FILE__ " line %i",(int)__LINE__)
#endif

#define ERRINTERNAL() error("flext: Internal error in file " __FILE__ ", line %i - please report",(int)__LINE__)


// ----- disable attribute editor for PD version < devel_0_36 or 0.37
#ifndef PD_MAJOR_VERSION
#	undef FLEXT_NOATTREDIT
#	define FLEXT_NOATTREDIT
#endif


// ----- set message queue mode -----
#if FLEXT_SYS == FLEXT_SYS_PD && PD_MINOR_VERSION >= 37
//	for PD version >= 0.37test10 FLEXT_PDLOCK is standard
#	undef FLEXT_PDLOCK
#	define FLEXT_PDLOCK
#endif

#ifndef FLEXT_QMODE
#	if FLEXT_SYS == FLEXT_SYS_PD && PD_MINOR_VERSION >= 38 && defined(PD_DEVEL_VERSION)
//		use idle callback
#		define FLEXT_QMODE 1
#	elif defined(FLEXT_PDLOCK)
//		new PD thread locking functionality shall be used
#		if FLEXT_SYS == FLEXT_SYS_PD
#			ifdef FLEXT_THREADS
//				can only be used with PD and threaded build
#				define FLEXT_QMODE 2
#			else
#				define FLEXT_QMODE 0
#			endif
#		else
#			error FLEXT_PDLOCK can only be defined with PD
#		endif
#	else
#		define FLEXT_QMODE 0
#	endif
#endif

#ifndef FLEXT_QMODE
#	error Internal error: Queueing mode not defined
#endif

#include "flpopns.h"

#endif
