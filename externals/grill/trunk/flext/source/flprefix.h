/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flprefix.h
    \brief Try to find out the platform.
*/
 
#ifndef __FLEXT_PREFIX_H
#define __FLEXT_PREFIX_H

// --- definitions for FLEXT_SYS ---------------------
#define FLEXT_SYS_UNKNOWN   0

#ifndef FLEXT_SYS_MAX
    #define FLEXT_SYS_MAX   1
#else
    // already defined
    #undef FLEXT_SYS_MAX
    #define FLEXT_SYS_MAX   1
    #define FLEXT_SYS FLEXT_SYS_MAX
#endif

#ifndef FLEXT_SYS_PD
    #define FLEXT_SYS_PD    2
#else
    // already defined
    #undef FLEXT_SYS_PD
    #define FLEXT_SYS_PD    2
    #define FLEXT_SYS FLEXT_SYS_PD
#endif

#ifndef FLEXT_SYS_JMAX
    #define FLEXT_SYS_JMAX  3
#else
    // already defined
    #undef FLEXT_SYS_JMAX
    #define FLEXT_SYS_JMAX  3
    #define FLEXT_SYS FLEXT_SYS_JMAX
#endif

// --- definitions for FLEXT_OS ----------------------
#define FLEXT_OS_UNKNOWN    0
#define FLEXT_OS_WIN    1
#define FLEXT_OS_MAC    2  
#define FLEXT_OS_LINUX  3
#define FLEXT_OS_IRIX   4

// --- definitions for FLEXT_OS_API ---------------------
#define FLEXT_OSAPI_UNKNOWN 0

#define FLEXT_OSAPI_UNIX_POSIX 1

#define FLEXT_OSAPI_MAC_CLASSIC 2
#define FLEXT_OSAPI_MAC_CARBON 3
#define FLEXT_OSAPI_MAC_MACH 4

#define FLEXT_OSAPI_WIN_NATIVE 5  // WIN32 Platform
#define FLEXT_OSAPI_WIN_POSIX 6    // POSIX API (e.g. cygwin)

// --- definitions for FLEXT_CPU ---------------------
#define FLEXT_CPU_UNKNOWN   0
#define FLEXT_CPU_IA32   1
#define FLEXT_CPU_PPC    2
#define FLEXT_CPU_MIPS   3
#define FLEXT_CPU_ALPHA  4

#define FLEXT_CPU_IA64   5 // Itanium
#define FLEXT_CPU_X86_64 6 // AMD-K8, EMT64
#define FLEXT_CPU_PPC64  7 // G5 in 64 bit mode

// compatibility
#define FLEXT_CPU_INTEL FLEXT_CPU_IA32

// --- definitions for FLEXT_THREADS -----------------
#define FLEXT_THR_POSIX 1 // pthreads
#define FLEXT_THR_WIN32 2 // Win32 native
#define FLEXT_THR_MP    3 // MacOS MPThreads

// ---------------------------------------------------
// support old definitions

#ifndef FLEXT_SYS
    #if defined(MAXMSP)
        #define FLEXT_SYS FLEXT_SYS_MAX
    //  #undef MAXMSP
    #elif defined(PD)
        #define FLEXT_SYS FLEXT_SYS_PD
    //  #undef PD
    //  #undef NT
    #endif
#endif

#if defined(_DEBUG) && !defined(FLEXT_DEBUG)
    #define FLEXT_DEBUG
#endif

// ---------------------------------------------------

// Definition of supported real-time systems
#if FLEXT_SYS == FLEXT_SYS_MAX || FLEXT_SYS == FLEXT_SYS_PD
#else
    #error "System must be defined by either FLEXT_SYS_MAX or FLEXT_SYS_PD"
#endif

// Definition of OS/CPU
#if defined(_MSC_VER) || (defined(__ICC) && (FLEXT_OS == FLEXT_OS_WIN || defined(_WIN32)))
    // Microsoft C++
    // and Intel C++ (as guessed)
    
    #ifndef FLEXT_CPU
        #if defined(_M_AMD64)
            #define FLEXT_CPU FLEXT_CPU_X86_64
        #elif defined(_M_IA64)
            #define FLEXT_CPU FLEXT_CPU_IA64
        #elif defined(_M_IX86)
            #define FLEXT_CPU FLEXT_CPU_IA32
        #elif defined(_M_PPC)
            #define FLEXT_CPU FLEXT_CPU_PPC
        #elif defined(_M_MRX000)
            #define FLEXT_CPU FLEXT_CPU_MIPS
        #elif defined(_M_ALPHA)
            #define FLEXT_CPU FLEXT_CPU_ALPHA
        #else
            #define FLEXT_CPU FLEXT_CPU_UNKNOWN
        #endif
    #endif

    #ifndef FLEXT_OS
        #if defined(_WIN32) || defined(_WIN64)
            #define FLEXT_OS FLEXT_OS_WIN
            #define FLEXT_OSAPI FLEXT_OSAPI_WIN_NATIVE
        #else
            #define FLEXT_OS FLEXT_OS_UNKNOWN
            #define FLEXT_OSAPI FLEXT_OSAPI_UNKNOWN
        #endif
    #endif


#elif defined(__BORLANDC__) 
    // Borland C++

    #ifndef FLEXT_CPU
        #define FLEXT_CPU FLEXT_CPU_INTEL
    #endif
    #ifndef FLEXT_OS
        #define FLEXT_OS FLEXT_OS_WIN
        #define FLEXT_OSAPI FLEXT_OSAPI_WIN_NATIVE
    #else   
        #define FLEXT_OSAPI FLEXT_OSAPI_UNKNOWN
    #endif


#elif defined(__MWERKS__)
    // Metrowerks CodeWarrior

    #ifdef __MACH__
        // quick fix for OSX Mach-O
        #ifdef __POWERPC__
            #ifdef __LP64__
                #define TARGET_CPU_PPC64 1
            #else
                #define TARGET_CPU_PPC 1
            #endif
        #else
            #ifdef __LP64__
                #define TARGET_CPU_X86_64 1
            #else
                #define TARGET_CPU_IA32 1
            #endif
        #endif
        #define TARGET_OS_MAC 1
        #define TARGET_API_MAC_OSX 1
    #else
        #ifndef __CONDITIONALMACROS__
        #include <ConditionalMacros.h>
        #endif
    #endif

    #ifndef FLEXT_CPU
        #if TARGET_CPU_X86_64
            #define FLEXT_CPU FLEXT_CPU_X86_64
        #elif TARGET_CPU_X86
            #define FLEXT_CPU FLEXT_CPU_IA32
        #elif TARGET_CPU_PPC64
            #define FLEXT_CPU FLEXT_CPU_PPC64
        #elif TARGET_CPU_PPC
            #define FLEXT_CPU FLEXT_CPU_PPC
        #elif TARGET_CPU_MIPS
            #define FLEXT_CPU FLEXT_CPU_MIPS
        #elif TARGET_CPU_ALPHA
            #define FLEXT_CPU FLEXT_CPU_ALPHA
        #else
            #define FLEXT_CPU FLEXT_CPU_UNKNOWN
        #endif
    #endif

    #ifndef FLEXT_OS
        #if TARGET_OS_MAC
            #define FLEXT_OS FLEXT_OS_MAC
        #elif TARGET_OS_WIN32
            // assume Windows
            #define FLEXT_OS FLEXT_OS_WIN
        #else
            #define FLEXT_OS FLEXT_OS_UNKNOWN
        #endif
    #endif
    
    #ifndef FLEXT_OSAPI
        #if TARGET_API_MAC_MACH
            // this is for Mach-O
            // this has the precedence (MACH also supports Carbon, of course)
            #define FLEXT_OSAPI FLEXT_OSAPI_MAC_MACH
        #elif TARGET_API_MAC_CARBON
            // this is for CFM
            #define FLEXT_OSAPI FLEXT_OSAPI_MAC_CARBON
        #else
            #define FLEXT_OSAPI FLEXT_OSAPI_UNKNOWN
        #endif
    #endif
    
    // This is important for method and attribute callbacks
    #pragma enumsalwaysint on
    // This is important for everything
    #pragma bool on

#elif defined(__GNUG__) || (defined(__ICC) && (FLEXT_OS == FLEXT_OS_LINUX || defined(linux) || defined(__linux__)))

    // GNU C++
    // and Intel (as suggested by Tim Blechmann)

    #ifndef FLEXT_CPU
        #if defined(__x86_64__)
            #define FLEXT_CPU FLEXT_CPU_X86_64
        #elif defined(_X86_) || defined(__i386__) || defined(__i586__) || defined(__i686__)
            #define FLEXT_CPU FLEXT_CPU_IA32
        #elif defined(__ppc64__)
            #define FLEXT_CPU FLEXT_CPU_PPC64
        #elif defined(__ppc__)
            #define FLEXT_CPU FLEXT_CPU_PPC
        #elif defined(__MIPS__)
            #define FLEXT_CPU FLEXT_CPU_MIPS
        #else
            #define FLEXT_CPU FLEXT_CPU_UNKNOWN
        #endif
    #endif

    #ifndef FLEXT_OS
        #if defined(linux) || defined(__linux__)
            #define FLEXT_OS FLEXT_OS_LINUX
        #elif defined(__CYGWIN__) || defined(__CYGWIN32__) || defined(__MINGW32__)
            #define FLEXT_OS FLEXT_OS_WIN
        #elif defined(__APPLE__) && defined(__MACH__)
            #define FLEXT_OS FLEXT_OS_MAC
        // how about IRIX??
        #else
            #define FLEXT_OS FLEXT_OS_UNKNOWN
        #endif
    #endif

    #ifndef FLEXT_OSAPI
        #if FLEXT_OS == FLEXT_OS_MAC
            #define FLEXT_OSAPI FLEXT_OSAPI_MAC_MACH
        #elif FLEXT_OS == FLEXT_OS_WIN
            #if defined(__MINGW32__)
                #define FLEXT_OSAPI FLEXT_OSAPI_WIN_NATIVE
            #else
                #define FLEXT_OSAPI FLEXT_OSAPI_WIN_POSIX
            #endif
        #elif FLEXT_OS == FLEXT_OS_LINUX || FLEXT_OS == FLEXT_OS_IRIX
            #define FLEXT_OSAPI FLEXT_OSAPI_UNIX_POSIX
        #else
            #define FLEXT_OSAPI FLEXT_OSAPI_UNKNOWN
        #endif
    #endif

#elif defined(__MRC__) && defined(MPW_CPLUS)
    // Apple MPW MrCpp

    #if __MRC__ < 0x500
        #error Apple MPW MrCpp v.5.0.0 or later compiler required
    #endif

    #ifndef FLEXT_CPU
        #if defined(__POWERPC__)
            #define FLEXT_CPU FLEXT_CPU_PPC
        #else
            #define FLEXT_CPU FLEXT_CPU_UNKNOWN
        #endif
    #endif

    #ifndef FLEXT_OS
        #if defined(macintosh)
            #define FLEXT_OS FLEXT_OS_MAC
        #else
            #define FLEXT_OS FLEXT_OS_UNKNOWN
        #endif
    #endif

    #ifndef FLEXT_OSAPI
        #if FLEXT_OS == FLEXT_OS_MAC
            #define FLEXT_OSAPI FLEXT_OSAPI_MAC_CLASSIC
        #else
            #define FLEXT_OSAPI FLEXT_OSAPI_UNKNOWN
        #endif
    #endif
#endif



#if FLEXT_OS == FLEXT_OS_WIN
//  #pragma message("Compiling for Windows")

    #if FLEXT_SYS == FLEXT_SYS_MAX
//      #define WIN_VERSION 1
    #elif FLEXT_SYS == FLEXT_SYS_PD
//      #define PD
//      #define NT
    #endif
#elif FLEXT_OS == FLEXT_OS_LINUX
//  #pragma message("Compiling for Linux")

    #if FLEXT_SYS == FLEXT_SYS_PD
//      #define PD
    #else
        #error "Flext SYS/OS combination unknown"
    #endif
#elif FLEXT_OS == FLEXT_OS_IRIX
//  #pragma message("Compiling for Irix")

    #if FLEXT_SYS == FLEXT_SYS_PD
//      #define PD
    #else
        #error "Flext SYS/OS combination unknown"
    #endif
#elif FLEXT_OS == FLEXT_OS_MAC
//  #pragma message("Compiling for MacOS")

    #if FLEXT_SYS == FLEXT_SYS_PD
//      #define PD
    #endif
#else
    #error "Operating system could not be determined"
#endif

#if FLEXT_SYS == FLEXT_SYS_MAX
//  #pragma message("Compiling for Max/MSP")
#elif FLEXT_SYS == FLEXT_SYS_PD
//  #pragma message("Compiling for PD")
#endif

// ----- set threading model -----
// shared builds are always threaded
#ifdef FLEXT_SHARED
    #undef FLEXT_THREADS
    #define FLEXT_THREADS
#endif

#ifdef FLEXT_THREADS
    #undef FLEXT_THREADS
    #if FLEXT_SYS == FLEXT_SYS_MAX && FLEXT_OS == FLEXT_OS_MAC && FLEXT_OSAPI != FLEXT_OSAPI_MAC_MACH
        // Max for CFM doesn't like posix threads
        #define FLEXT_THREADS FLEXT_THR_MP      
    #elif FLEXT_SYS == FLEXT_SYS_MAX && FLEXT_OS == FLEXT_OS_WIN
        // for wmax use native Windows threads
        #define FLEXT_THREADS FLEXT_THR_WIN32
    #else
        #define FLEXT_THREADS FLEXT_THR_POSIX
    #endif
#endif

// ----- macros for class names -----
/*
        With linux (flat linker namespace) and more than one flext-based external loaded all calls to static 
        exported functions refer to the first instance loaded!
        Therefore different class names are used so that the correct type of flext function is called.
*/
#ifdef __DOXYGEN__
    #define FLEXT_CLASSDEF(CL) CL
#elif defined(FLEXT_DEBUG)
    #if defined(FLEXT_SHARED)
        #define FLEXT_CLASSDEF(CL) CL##_shared_d
    #elif defined(FLEXT_THREADS)
        #define FLEXT_CLASSDEF(CL) CL##_multi_d
    #else
        #define FLEXT_CLASSDEF(CL) CL##_single_d
    #endif
#else
    #if defined(FLEXT_SHARED)
        #define FLEXT_CLASSDEF(CL) CL##_shared
    #elif defined(FLEXT_THREADS)
        #define FLEXT_CLASSDEF(CL) CL##_multi
    #else
        #define FLEXT_CLASSDEF(CL) CL##_single
    #endif
#endif


/* Set the right calling convention (and exporting) for the OS */

#if defined(_MSC_VER)
	#ifdef FLEXT_SHARED
        // for compiling a shared flext library FLEXT_EXPORTS must be defined
        #ifdef FLEXT_EXPORTS
		    #define FLEXT_SHARE __declspec(dllexport)
        #else
		    #define FLEXT_SHARE __declspec(dllimport)
        #endif
	#else
		#define FLEXT_SHARE
	#endif
	#define FLEXT_EXT __declspec(dllexport)
#else                   // other OS's
	#define FLEXT_SHARE
	#define FLEXT_EXT
#endif


// std namespace
#ifdef __MWERKS__
#	define STD std
#else
#	define STD
#endif

// branching hints
#if __GNUC__ >= 3
#	ifndef LIKELY
#		define LIKELY(expression) (__builtin_expect(!!(expression), 1))
#	endif
#	ifndef UNLIKELY
#		define UNLIKELY(expression) (__builtin_expect(!!(expression), 0))
#	endif
#else
#	ifndef LIKELY
#		define LIKELY(expression) (expression)
#	endif
#	ifndef UNLIKELY
#		define UNLIKELY(expression) (expression)
#	endif
#endif

// macro definitions for inline flext usage
#ifdef FLEXT_INLINE
#   define FLEXT_TEMPLATE template<typename flext_T>
#   define FLEXT_TEMPIMPL(fun) template<typename flext_T> fun<flext_T>
#   define FLEXT_TEMPINST(fun) fun<void>
#   define FLEXT_TEMPSUB(fun) typename fun<flext_T>
#   define FLEXT_TEMP_TYPENAME typename
#else
#   define FLEXT_TEMPLATE
#   define FLEXT_TEMPIMPL(fun) fun
#   define FLEXT_TEMPINST(fun) fun
#   define FLEXT_TEMPSUB(fun) fun
#   define FLEXT_TEMP_TYPENAME
#endif

#endif // __FLEXT_PREFIX_H
