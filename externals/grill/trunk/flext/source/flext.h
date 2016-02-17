/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
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
#define FLEXT_VERSION 600

//! \brief flext version string
#define FLEXT_VERSTR "0.6.0 alpha"

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

#ifdef FLEXT_INLINE
// include all source code files
#   include "flatom.cpp"
#   include "flatom_part.cpp"
#   include "flatom_pr.cpp"
#   include "flattr.cpp"
#   include "flattr_ed.cpp"
#   include "flbase.cpp"
#   include "flbind.cpp"
#   include "flbuf.cpp"
#   include "fldsp.cpp"
#   include "flext.cpp"
#   include "flitem.cpp"
#   include "fllib.cpp"
#   include "flmap.cpp"
#   include "flmeth.cpp"
#   include "flmsg.cpp"
#   include "flout.cpp"
#   include "flproxy.cpp"
#   include "flqueue.cpp"
#   include "flsimd.cpp"
#   include "flsupport.cpp"
#   include "flthr.cpp"
#   include "fltimer.cpp"
#   include "flutil.cpp"
#   include "flxlet.cpp"
#endif

#endif // FLEXT_H
