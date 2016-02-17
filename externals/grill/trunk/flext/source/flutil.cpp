/*
flext - C++ layer for Max and Pure Data externals

Copyright (c) 2001-2015 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.
*/

/*! \file flutil.cpp
    \brief Implementation of the various utility functions.
*/
 
#ifndef __FLEXT_UTIL_CPP
#define __FLEXT_UTIL_CPP

#include "flext.h"
#include <cstring>

#if FLEXT_OS == FLEXT_OS_WIN
#include <windows.h>
#elif FLEXT_OS == FLEXT_OS_MAC
    #if FLEXT_OSAPI != FLEXT_OSAPI_MAC_MACH
        #include <MacMemory.h>
    #else
        #include <Carbon/Carbon.h>
    #endif
#endif

#include "flpushns.h"

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::CopyMem(void *dst,const void *src,int bytes)
{
#if FLEXT_OS == FLEXT_OS_WIN
    MoveMemory(dst,src,bytes);
#elif FLEXT_OS == FLEXT_OS_MAC && !defined(__LP64__)
    BlockMoveData(src,dst,bytes);   // not available for 64 bits
#else
    memmove(dst,src,bytes);
#endif
}

FLEXT_TEMPIMPL(void FLEXT_CLASSDEF(flext))::ZeroMem(void *dst,int bytes)
{
#if FLEXT_OS == FLEXT_OS_WIN
    ZeroMemory(dst,bytes);
#elif FLEXT_OS == FLEXT_OS_MAC
#   ifdef __LP64__  // 64 bits compilation
    bzero(dst,bytes);
#   else
    BlockZero(dst,bytes);
#   endif
#else
    memset(dst,0,bytes);
#endif
}

#include "flpopns.h"

#endif // __FLEXT_UTIL_CPP


