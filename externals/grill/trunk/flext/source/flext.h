/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2010 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3715 $
$LastChangedDate: 2010-01-11 11:04:57 -0500 (Mon, 11 Jan 2010) $
$LastChangedBy: thomas $
*/

/*! \file flext.h 
    \brief This is the main flext include file.
    
    The basic definitions are set here and the necessary header files are included
*/

#ifndef __FLEXT_H
#define __FLEXT_H


/*!	\defgroup FLEXT_GLOBAL Flext global definitions
	@{
*/

//! \brief flext version number
#define FLEXT_VERSION 502

//! \brief flext version string
#define FLEXT_VERSTR "0.5.2 beta"

//! @}

// determine System/OS/CPU
#include "flprefix.h"

// include headers necessary for multi-threading
#ifdef FLEXT_THREADS
	#if FLEXT_THREADS == FLEXT_THR_POSIX
		extern "C" {
			#include <pthread.h>
			#include <sched.h>
		}
	#elif FLEXT_THREADS == FLEXT_THR_MP
		#include <multiprocessing.h>
	#elif FLEXT_THREADS == FLEXT_THR_WIN32
        #if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x500
    		#include <windows.h>
            #include <process.h>
        #else
            #error "Win32 threading model only supported for Win2000/XP or newer"
        #endif
	#else
		#error "Thread model not supported"
	#endif
#endif


// include all the flext interface definitions
#include "fldefs.h"

// include the basic flext object classes
#include "flclass.h"

// include the flext dsp class
#include "fldsp.h"

#endif // FLEXT_H
