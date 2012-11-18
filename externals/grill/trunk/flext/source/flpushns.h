/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 3660 $
$LastChangedDate: 2009-02-10 14:17:03 -0500 (Tue, 10 Feb 2009) $
$LastChangedBy: thomas $
*/

#ifdef FLEXT_USE_NAMESPACE

#ifndef _FLEXT_IN_NAMESPACE
    #define _FLEXT_IN_NAMESPACE 0
#endif

#if _FLEXT_IN_NAMESPACE == 0

    #if 1 //defined(FLEXT_SHARED)
    namespace flext_ns {
    #elif defined(__GNUC__)
    namespace {  // anonymous namespace (don't export symbols)
    #endif

#endif

#define __FLEXT_IN_NAMESPACE (_FLEXT_IN_NAMESPACE+1)
#undef _FLEXT_IN_NAMESPACE
#define _FLEXT_IN_NAMESPACE __FLEXT_IN_NAMESPACE
#undef __FLEXT_IN_NAMESPACE

#endif
